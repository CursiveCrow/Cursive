#include "rt_internal.h"

#define C0_SPEC_TRACE_ENV L"CURSIVE_SPEC_TRACE_RUNTIME"

static HANDLE c0_trace_handle = NULL;
static LONG c0_trace_init = 0;
static int c0_trace_enabled = 0;
static SRWLOCK c0_trace_lock = SRWLOCK_INIT;

static void c0_trace_write_bytes(const uint8_t* data, uint64_t len) {
  if (!data || len == 0 || !c0_trace_handle) {
    return;
  }
  uint64_t offset = 0;
  while (offset < len) {
    uint64_t remaining = len - offset;
    DWORD chunk = remaining > 0xFFFFFFFFu ? 0xFFFFFFFFu : (DWORD)remaining;
    DWORD written = 0;
    if (!WriteFile(c0_trace_handle, data + offset, chunk, &written, NULL)) {
      break;
    }
    if (written == 0) {
      break;
    }
    offset += written;
  }
}

static void c0_trace_write_cstr(const char* text) {
  if (!text) {
    return;
  }
  c0_trace_write_bytes((const uint8_t*)text, c0_cstr_len(text));
}

static void c0_trace_write_encoded(const uint8_t* data, uint64_t len) {
  static const char hex[] = "0123456789ABCDEF";
  for (uint64_t i = 0; i < len; ++i) {
    uint8_t c = data[i];
    if (c == '\t' || c == '\n' || c == '%' || c == ';' || c == '=') {
      char out[3];
      out[0] = '%';
      out[1] = hex[(c >> 4) & 0xF];
      out[2] = hex[c & 0xF];
      c0_trace_write_bytes((const uint8_t*)out, 3);
    } else {
      c0_trace_write_bytes(&c, 1);
    }
  }
}

static void c0_spec_trace_init(void) {
  if (InterlockedCompareExchange(&c0_trace_init, 1, 0) != 0) {
    return;
  }
  DWORD needed = GetEnvironmentVariableW(C0_SPEC_TRACE_ENV, NULL, 0);
  if (needed == 0) {
    return;
  }
  wchar_t* path = (wchar_t*)c0_heap_alloc_raw(sizeof(wchar_t) * needed);
  if (!path) {
    return;
  }
  DWORD got = GetEnvironmentVariableW(C0_SPEC_TRACE_ENV, path, needed);
  if (got == 0 || got >= needed) {
    c0_heap_free_raw(path);
    return;
  }
  HANDLE handle = CreateFileW(path, GENERIC_WRITE, FILE_SHARE_READ,
                              NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
  c0_heap_free_raw(path);
  if (handle == INVALID_HANDLE_VALUE) {
    return;
  }
  c0_trace_handle = handle;
  c0_trace_enabled = 1;
  c0_trace_write_cstr("spec_trace_v1\n");
}

void cursive_x3a_x3aruntime_x3a_x3aspec_x5ftrace_x3a_x3aemit(
    const C0StringView* rule_id,
    const C0StringView* payload) {
  c0_spec_trace_init();
  if (!c0_trace_enabled || !rule_id || !rule_id->data || rule_id->len == 0) {
    return;
  }
  AcquireSRWLockExclusive(&c0_trace_lock);
  c0_trace_write_cstr("runtime\truntime\t");
  c0_trace_write_bytes(rule_id->data, rule_id->len);
  c0_trace_write_cstr("\t-\t0\t0\t0\t0\t");
  if (payload && payload->data && payload->len != 0) {
    c0_trace_write_encoded(payload->data, payload->len);
  }
  c0_trace_write_cstr("\n");
  FlushFileBuffers(c0_trace_handle);
  ReleaseSRWLockExclusive(&c0_trace_lock);
}
