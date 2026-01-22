#ifndef CURSIVE0_RT_INTERNAL_H
#define CURSIVE0_RT_INTERNAL_H

#include "cursive0_rt.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <stdbool.h>

// -----------------------------------------------------------------------------
// Internal runtime state
// -----------------------------------------------------------------------------

typedef struct C0HeapState {
  struct C0HeapState* parent;
  uint64_t quota;
  uint64_t used;
} C0HeapState;

typedef struct C0AllocHeader {
  C0HeapState* heap;
  uint64_t cap;
} C0AllocHeader;

typedef struct C0FsState {
  uint8_t* base_utf8;
  uint32_t base_len;
  int restricted;
  int valid;
} C0FsState;

typedef struct C0DirIterState {
  wchar_t* base_path;
  uint32_t base_len;
  uint8_t* base_utf8;
  uint32_t base_utf8_len;
  wchar_t** names;
  uint32_t* name_lens;
  uint8_t* kinds;
  uint32_t count;
  uint32_t index;
} C0DirIterState;

// -----------------------------------------------------------------------------
// Utility helpers (no CRT)
// -----------------------------------------------------------------------------

static __inline uint64_t c0_min_u64(uint64_t a, uint64_t b) {
  return a < b ? a : b;
}

static __inline uint64_t c0_align_up(uint64_t value, uint64_t align) {
  if (align == 0) {
    return value;
  }
  const uint64_t rem = value % align;
  if (rem == 0) {
    return value;
  }
  return value + (align - rem);
}

static __inline HANDLE c0_process_heap(void) {
  static HANDLE heap = NULL;
  if (!heap) {
    heap = GetProcessHeap();
  }
  return heap;
}

static __inline void* c0_heap_alloc_raw(size_t size) {
  return HeapAlloc(c0_process_heap(), 0, size);
}

static __inline void c0_heap_free_raw(void* ptr) {
  if (ptr) {
    HeapFree(c0_process_heap(), 0, ptr);
  }
}

static __inline void* c0_memcpy(void* dst, const void* src, size_t n) {
  unsigned char* d = (unsigned char*)dst;
  const unsigned char* s = (const unsigned char*)src;
  for (size_t i = 0; i < n; ++i) {
    d[i] = s[i];
  }
  return dst;
}

static __inline void* c0_memmove(void* dst, const void* src, size_t n) {
  unsigned char* d = (unsigned char*)dst;
  const unsigned char* s = (const unsigned char*)src;
  if (d == s || n == 0) {
    return dst;
  }
  if (d < s) {
    for (size_t i = 0; i < n; ++i) {
      d[i] = s[i];
    }
  } else {
    for (size_t i = n; i > 0; --i) {
      d[i - 1] = s[i - 1];
    }
  }
  return dst;
}

static __inline void* c0_memset(void* dst, int c, size_t n) {
  unsigned char* d = (unsigned char*)dst;
  const unsigned char v = (unsigned char)c;
  for (size_t i = 0; i < n; ++i) {
    d[i] = v;
  }
  return dst;
}

static __inline size_t c0_wcslen(const wchar_t* s) {
  size_t n = 0;
  if (!s) {
    return 0;
  }
  while (s[n] != 0) {
    ++n;
  }
  return n;
}

static __inline uint64_t c0_cstr_len(const char* s) {
  uint64_t n = 0;
  if (!s) {
    return 0;
  }
  while (s[n] != 0) {
    ++n;
  }
  return n;
}

static __inline void c0_trace_emit_rule(const char* rule_id) {
  if (!rule_id) {
    return;
  }
  C0StringView rule_view;
  rule_view.data = (const uint8_t*)rule_id;
  rule_view.len = c0_cstr_len(rule_id);
  C0StringView payload;
  payload.data = NULL;
  payload.len = 0;
  cursive_x3a_x3aruntime_x3a_x3aspec_x5ftrace_x5femit(&rule_view, &payload);
}

static __inline int c0_utf8_has_null(const uint8_t* data, uint64_t len) {
  for (uint64_t i = 0; i < len; ++i) {
    if (data[i] == 0) {
      return 1;
    }
  }
  return 0;
}

static __inline int c0_is_abs_path(const wchar_t* path, uint32_t len) {
  if (!path || len == 0) {
    return 0;
  }
  if (len >= 2 && path[1] == L':') {
    return 1;
  }
  if (len >= 2 && path[0] == L'\\' && path[1] == L'\\') {
    return 1;
  }
  if (path[0] == L'\\' || path[0] == L'/') {
    return 1;
  }
  return 0;
}

static __inline int c0_heap_can_alloc(C0HeapState* heap,
                                      uint64_t size,
                                      int* out_quota_exceeded) {
  if (out_quota_exceeded) {
    *out_quota_exceeded = 0;
  }
  if (!heap || size == 0) {
    return 1;
  }
  for (C0HeapState* cur = heap; cur; cur = cur->parent) {
    if (cur->quota != 0) {
      if (size > cur->quota - cur->used) {
        if (out_quota_exceeded) {
          *out_quota_exceeded = 1;
        }
        return 0;
      }
    }
  }
  return 1;
}

static __inline void c0_heap_add_used(C0HeapState* heap, uint64_t size) {
  if (!heap || size == 0) {
    return;
  }
  for (C0HeapState* cur = heap; cur; cur = cur->parent) {
    cur->used += size;
  }
}

static __inline void c0_heap_sub_used(C0HeapState* heap, uint64_t size) {
  if (!heap || size == 0) {
    return;
  }
  for (C0HeapState* cur = heap; cur; cur = cur->parent) {
    if (cur->used >= size) {
      cur->used -= size;
    } else {
      cur->used = 0;
    }
  }
}

// -----------------------------------------------------------------------------
// Allocation helpers for managed string/bytes
// -----------------------------------------------------------------------------

static __inline C0AllocHeader* c0_header_from_data(uint8_t* data) {
  return data ? ((C0AllocHeader*)data) - 1 : NULL;
}

static __inline uint8_t* c0_alloc_managed_bytes(
    C0HeapState* heap,
    uint64_t cap,
    int* out_quota_exceeded) {
  if (cap == 0) {
    return NULL;
  }
  if (!c0_heap_can_alloc(heap, cap, out_quota_exceeded)) {
    return NULL;
  }
  const uint64_t total = cap + (uint64_t)sizeof(C0AllocHeader);
  if (total < cap || total > (uint64_t)SIZE_MAX) {
    return NULL;
  }
  C0AllocHeader* header = (C0AllocHeader*)c0_heap_alloc_raw((size_t)total);
  if (!header) {
    return NULL;
  }
  header->heap = heap;
  header->cap = cap;
  c0_heap_add_used(heap, cap);
  return (uint8_t*)(header + 1);
}

static __inline void c0_free_managed_bytes(uint8_t* data) {
  if (!data) {
    return;
  }
  C0AllocHeader* header = c0_header_from_data(data);
  if (header && header->heap) {
    c0_heap_sub_used(header->heap, header->cap);
  }
  c0_heap_free_raw(header);
}

// -----------------------------------------------------------------------------
// UTF-8 helpers
// -----------------------------------------------------------------------------

static __inline int c0_utf8_valid(const uint8_t* data, uint64_t len) {
  uint64_t i = 0;
  while (i < len) {
    uint8_t c = data[i];
    if (c < 0x80) {
      ++i;
      continue;
    }
    if (c >= 0xC2 && c <= 0xDF) {
      if (i + 1 >= len) return 0;
      if ((data[i + 1] & 0xC0) != 0x80) return 0;
      i += 2;
      continue;
    }
    if (c == 0xE0) {
      if (i + 2 >= len) return 0;
      if (data[i + 1] < 0xA0 || data[i + 1] > 0xBF) return 0;
      if ((data[i + 2] & 0xC0) != 0x80) return 0;
      i += 3;
      continue;
    }
    if (c >= 0xE1 && c <= 0xEC) {
      if (i + 2 >= len) return 0;
      if ((data[i + 1] & 0xC0) != 0x80) return 0;
      if ((data[i + 2] & 0xC0) != 0x80) return 0;
      i += 3;
      continue;
    }
    if (c == 0xED) {
      if (i + 2 >= len) return 0;
      if (data[i + 1] < 0x80 || data[i + 1] > 0x9F) return 0;
      if ((data[i + 2] & 0xC0) != 0x80) return 0;
      i += 3;
      continue;
    }
    if (c >= 0xEE && c <= 0xEF) {
      if (i + 2 >= len) return 0;
      if ((data[i + 1] & 0xC0) != 0x80) return 0;
      if ((data[i + 2] & 0xC0) != 0x80) return 0;
      i += 3;
      continue;
    }
    if (c == 0xF0) {
      if (i + 3 >= len) return 0;
      if (data[i + 1] < 0x90 || data[i + 1] > 0xBF) return 0;
      if ((data[i + 2] & 0xC0) != 0x80) return 0;
      if ((data[i + 3] & 0xC0) != 0x80) return 0;
      i += 4;
      continue;
    }
    if (c >= 0xF1 && c <= 0xF3) {
      if (i + 3 >= len) return 0;
      if ((data[i + 1] & 0xC0) != 0x80) return 0;
      if ((data[i + 2] & 0xC0) != 0x80) return 0;
      if ((data[i + 3] & 0xC0) != 0x80) return 0;
      i += 4;
      continue;
    }
    if (c == 0xF4) {
      if (i + 3 >= len) return 0;
      if (data[i + 1] < 0x80 || data[i + 1] > 0x8F) return 0;
      if ((data[i + 2] & 0xC0) != 0x80) return 0;
      if ((data[i + 3] & 0xC0) != 0x80) return 0;
      i += 4;
      continue;
    }
    return 0;
  }
  return 1;
}

// Convert UTF-8 bytes to wide string (allocates with process heap)
static __inline wchar_t* c0_utf8_to_wide(
    const uint8_t* data,
    uint64_t len,
    uint32_t* out_len) {
  if (out_len) {
    *out_len = 0;
  }
  if (!data && len != 0) {
    return NULL;
  }
  if (len == 0) {
    return NULL;
  }
  if (len > (uint64_t)INT32_MAX) {
    return NULL;
  }
  if (c0_utf8_has_null(data, len)) {
    return NULL;
  }
  int wide_len = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                                     (const char*)data, (int)len,
                                     NULL, 0);
  if (wide_len <= 0) {
    return NULL;
  }
  const size_t bytes = (size_t)(wide_len + 1) * sizeof(wchar_t);
  wchar_t* out = (wchar_t*)c0_heap_alloc_raw(bytes);
  if (!out) {
    return NULL;
  }
  int written = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                                    (const char*)data, (int)len,
                                    out, wide_len);
  if (written != wide_len) {
    c0_heap_free_raw(out);
    return NULL;
  }
  out[wide_len] = 0;
  if (out_len) {
    *out_len = (uint32_t)wide_len;
  }
  return out;
}

// Convert wide string to UTF-8 (allocates with process heap)
static __inline uint8_t* c0_wide_to_utf8(
    const wchar_t* data,
    uint32_t len,
    uint32_t* out_len) {
  if (out_len) {
    *out_len = 0;
  }
  if (!data || len == 0) {
    return NULL;
  }
  if (len > (uint32_t)INT32_MAX) {
    return NULL;
  }
  int byte_len = WideCharToMultiByte(CP_UTF8, 0,
                                     data, (int)len,
                                     NULL, 0,
                                     NULL, NULL);
  if (byte_len <= 0) {
    return NULL;
  }
  uint8_t* out = (uint8_t*)c0_heap_alloc_raw((size_t)byte_len);
  if (!out) {
    return NULL;
  }
  int written = WideCharToMultiByte(CP_UTF8, 0,
                                    data, (int)len,
                                    (char*)out, byte_len,
                                    NULL, NULL);
  if (written != byte_len) {
    c0_heap_free_raw(out);
    return NULL;
  }
  if (out_len) {
    *out_len = (uint32_t)byte_len;
  }
  return out;
}

// -----------------------------------------------------------------------------
// Size checks (Win64 expectations)
// -----------------------------------------------------------------------------

_Static_assert(sizeof(void*) == 8, "runtime expects 64-bit pointers");
_Static_assert(sizeof(C0StringView) == 16, "string@View layout");
_Static_assert(sizeof(C0StringManaged) == 24, "string@Managed layout");
_Static_assert(sizeof(C0BytesView) == 16, "bytes@View layout");
_Static_assert(sizeof(C0BytesManaged) == 24, "bytes@Managed layout");
_Static_assert(sizeof(C0DynObject) == 16, "dynamic object layout");
_Static_assert(sizeof(C0Context) == 32, "Context layout");
_Static_assert(sizeof(C0StringModal) == 32, "string modal layout");
_Static_assert(sizeof(C0RegionOptions) == 40, "RegionOptions layout");
_Static_assert(sizeof(C0AllocationError) == 16, "AllocationError layout");
_Static_assert(sizeof(C0DirEntry) == 56, "DirEntry layout");
_Static_assert(sizeof(C0Union_StringManaged_AllocError) == 32, "string|AllocError layout");
_Static_assert(sizeof(C0Union_BytesManaged_AllocError) == 32, "bytes|AllocError layout");
_Static_assert(sizeof(C0Union_Unit_AllocError) == 24, "unit|AllocError layout");
_Static_assert(sizeof(C0Union_StringManaged_IoError) == 32, "string|IoError layout");
_Static_assert(sizeof(C0Union_BytesManaged_IoError) == 32, "bytes|IoError layout");
_Static_assert(sizeof(C0Union_File_IoError) == 16, "file|IoError layout");
_Static_assert(sizeof(C0Union_DirIter_IoError) == 16, "diriter|IoError layout");
_Static_assert(sizeof(C0Union_Unit_IoError) == 2, "unit|IoError layout");
_Static_assert(sizeof(C0Union_FileKind_IoError) == 2, "filekind|IoError layout");
_Static_assert(sizeof(C0Union_DirEntry_Unit_IoError) == 64, "direntry|unit|IoError layout");

#endif  // CURSIVE0_RT_INTERNAL_H
