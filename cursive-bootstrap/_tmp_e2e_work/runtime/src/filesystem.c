#include "rt_internal.h"

static int c0_is_sep(uint8_t c) {
  return c == '/' || c == '\\';
}

static int c0_is_ascii_letter(uint8_t c) {
  return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

static int c0_path_is_drive_rooted(const uint8_t* data, uint64_t len) {
  if (!data || len < 3) {
    return 0;
  }
  if (!c0_is_ascii_letter(data[0])) {
    return 0;
  }
  if (data[1] != ':') {
    return 0;
  }
  return c0_is_sep(data[2]);
}

static int c0_path_is_unc(const uint8_t* data, uint64_t len) {
  if (!data || len < 2) {
    return 0;
  }
  return (data[0] == '/' && data[1] == '/') || (data[0] == '\\' && data[1] == '\\');
}

static int c0_path_is_root_relative(const uint8_t* data, uint64_t len) {
  if (!data || len == 0) {
    return 0;
  }
  if (c0_path_is_unc(data, len)) {
    return 0;
  }
  if (c0_path_is_drive_rooted(data, len)) {
    return 0;
  }
  return data[0] == '/' || data[0] == '\\';
}

static int c0_path_is_abs(const uint8_t* data, uint64_t len) {
  return c0_path_is_drive_rooted(data, len) ||
         c0_path_is_unc(data, len) ||
         c0_path_is_root_relative(data, len);
}

typedef struct C0Seg {
  const uint8_t* ptr;
  uint32_t len;
} C0Seg;
static uint8_t* c0_path_canon(const uint8_t* data,
                              uint64_t len,
                              uint32_t* out_len) {
  if (out_len) {
    *out_len = 0;
  }
  if (!data && len != 0) {
    return NULL;
  }
  if (len > UINT32_MAX) {
    return NULL;
  }

  uint8_t root_tag[2];
  uint32_t root_len = 0;
  uint64_t pos = 0;
  int drive_rooted = 0;

  if (c0_path_is_drive_rooted(data, len)) {
    root_tag[0] = data[0];
    root_tag[1] = ':';
    root_len = 2;
    pos = 3;
    drive_rooted = 1;
  } else if (c0_path_is_unc(data, len)) {
    root_tag[0] = '/';
    root_tag[1] = '/';
    root_len = 2;
    pos = 2;
  } else if (c0_path_is_root_relative(data, len)) {
    root_tag[0] = '/';
    root_len = 1;
    pos = 1;
  }

  uint32_t seg_count = 0;
  uint64_t seg_start = pos;
  for (uint64_t i = pos; i <= len; ++i) {
    if (i == len || c0_is_sep(data[i])) {
      if (i > seg_start) {
        ++seg_count;
      }
      seg_start = i + 1;
    }
  }

  C0Seg* segs = NULL;
  if (seg_count > 0) {
    segs = (C0Seg*)c0_heap_alloc_raw(sizeof(C0Seg) * seg_count);
    if (!segs) {
      return NULL;
    }
  }

  uint32_t out_count = 0;
  seg_start = pos;
  for (uint64_t i = pos; i <= len; ++i) {
    if (i == len || c0_is_sep(data[i])) {
      if (i > seg_start) {
        uint32_t seg_len = (uint32_t)(i - seg_start);
        const uint8_t* seg_ptr = data + seg_start;
        if (seg_len == 1 && seg_ptr[0] == '.') {
          // skip
        } else if (seg_len == 2 && seg_ptr[0] == '.' && seg_ptr[1] == '.') {
          if (segs) {
            c0_heap_free_raw(segs);
          }
          return NULL;
        } else {
          if (segs) {
            segs[out_count].ptr = seg_ptr;
            segs[out_count].len = seg_len;
          }
          ++out_count;
        }
      }
      seg_start = i + 1;
    }
  }

  uint64_t out_len64 = root_len;
  if (out_count > 0) {
    if (root_len > 0 && drive_rooted) {
      out_len64 += 1;
    }
    for (uint32_t i = 0; i < out_count; ++i) {
      out_len64 += segs[i].len;
    }
    if (out_count > 1) {
      out_len64 += (uint64_t)(out_count - 1);
    }
  }

  if (out_len64 > UINT32_MAX) {
    if (segs) {
      c0_heap_free_raw(segs);
    }
    return NULL;
  }

  uint32_t out_len_u32 = (uint32_t)out_len64;
  uint64_t alloc_size = (uint64_t)out_len_u32 + 1;
  if (alloc_size > (uint64_t)SIZE_MAX) {
    if (segs) {
      c0_heap_free_raw(segs);
    }
    return NULL;
  }

  uint8_t* out = (uint8_t*)c0_heap_alloc_raw((size_t)alloc_size);
  if (!out) {
    if (segs) {
      c0_heap_free_raw(segs);
    }
    return NULL;
  }

  uint32_t offset = 0;
  if (root_len > 0) {
    c0_memcpy(out + offset, root_tag, root_len);
    offset += root_len;
  }
  if (out_count > 0) {
    if (root_len > 0 && drive_rooted) {
      out[offset++] = '/';
    }
    for (uint32_t i = 0; i < out_count; ++i) {
      c0_memcpy(out + offset, segs[i].ptr, segs[i].len);
      offset += segs[i].len;
      if (i + 1 < out_count) {
        out[offset++] = '/';
      }
    }
  }
  out[offset] = 0;

  if (segs) {
    c0_heap_free_raw(segs);
  }
  if (out_len) {
    *out_len = out_len_u32;
  }
  return out;
}
static uint8_t* c0_path_join(const uint8_t* base,
                             uint32_t base_len,
                             const uint8_t* rel,
                             uint64_t rel_len,
                             uint32_t* out_len) {
  if (out_len) {
    *out_len = 0;
  }
  if (!base && base_len != 0) {
    return NULL;
  }
  if (!rel && rel_len != 0) {
    return NULL;
  }
  if (rel_len > UINT32_MAX) {
    return NULL;
  }

  if (base_len == 0) {
    uint32_t len = (uint32_t)rel_len;
    uint64_t alloc_size = (uint64_t)len + 1;
    if (alloc_size > (uint64_t)SIZE_MAX) {
      return NULL;
    }
    uint8_t* out = (uint8_t*)c0_heap_alloc_raw((size_t)alloc_size);
    if (!out) {
      return NULL;
    }
    if (len > 0) {
      c0_memcpy(out, rel, len);
    }
    out[len] = 0;
    if (out_len) {
      *out_len = len;
    }
    return out;
  }

  if (rel_len == 0) {
    uint64_t alloc_size = (uint64_t)base_len + 1;
    if (alloc_size > (uint64_t)SIZE_MAX) {
      return NULL;
    }
    uint8_t* out = (uint8_t*)c0_heap_alloc_raw((size_t)alloc_size);
    if (!out) {
      return NULL;
    }
    c0_memcpy(out, base, base_len);
    out[base_len] = 0;
    if (out_len) {
      *out_len = base_len;
    }
    return out;
  }

  uint32_t extra = (base[base_len - 1] == '/') ? 0 : 1;
  uint64_t total = (uint64_t)base_len + extra + rel_len;
  if (total > UINT32_MAX) {
    return NULL;
  }
  uint64_t alloc_size = total + 1;
  if (alloc_size > (uint64_t)SIZE_MAX) {
    return NULL;
  }

  uint8_t* out = (uint8_t*)c0_heap_alloc_raw((size_t)alloc_size);
  if (!out) {
    return NULL;
  }
  c0_memcpy(out, base, base_len);
  uint32_t offset = base_len;
  if (extra) {
    out[offset++] = '/';
  }
  c0_memcpy(out + offset, rel, (uint32_t)rel_len);
  offset += (uint32_t)rel_len;
  out[offset] = 0;
  if (out_len) {
    *out_len = (uint32_t)total;
  }
  return out;
}

static int c0_path_prefix(const uint8_t* path,
                          uint32_t path_len,
                          const uint8_t* base,
                          uint32_t base_len) {
  if (base_len == 0) {
    return 1;
  }
  if (!path || !base) {
    return 0;
  }
  if (path_len < base_len) {
    return 0;
  }
  for (uint32_t i = 0; i < base_len; ++i) {
    if (path[i] != base[i]) {
      return 0;
    }
  }
  if (path_len == base_len) {
    return 1;
  }
  if (base[base_len - 1] == '/') {
    return 1;
  }
  return path[base_len] == '/';
}
static wchar_t* c0_path_utf8_to_wide(const uint8_t* utf8,
                                     uint32_t len,
                                     uint32_t* out_len) {
  if (out_len) {
    *out_len = 0;
  }
  if (len == 0) {
    wchar_t* out = (wchar_t*)c0_heap_alloc_raw(sizeof(wchar_t));
    if (!out) {
      return NULL;
    }
    out[0] = 0;
    if (out_len) {
      *out_len = 0;
    }
    return out;
  }
  wchar_t* wide = c0_utf8_to_wide(utf8, len, out_len);
  if (!wide) {
    return NULL;
  }
  uint32_t wlen = out_len ? *out_len : (uint32_t)c0_wcslen(wide);
  for (uint32_t i = 0; i < wlen; ++i) {
    if (wide[i] == L'/') {
      wide[i] = L'\\';
    }
  }
  if (wlen == 2 && wide[1] == L':') {
    wchar_t* out = (wchar_t*)c0_heap_alloc_raw(sizeof(wchar_t) * 4);
    if (!out) {
      c0_heap_free_raw(wide);
      return NULL;
    }
    out[0] = wide[0];
    out[1] = wide[1];
    out[2] = L'\\';
    out[3] = 0;
    c0_heap_free_raw(wide);
    if (out_len) {
      *out_len = 3;
    }
    return out;
  }
  return wide;
}

static C0IoError c0_map_win_error(DWORD err) {
  switch (err) {
    case ERROR_FILE_NOT_FOUND:
    case ERROR_PATH_NOT_FOUND:
    case ERROR_INVALID_DRIVE:
      return C0_IO_NOTFOUND;
    case ERROR_ACCESS_DENIED:
    case ERROR_PRIVILEGE_NOT_HELD:
      return C0_IO_PERMISSION_DENIED;
    case ERROR_FILE_EXISTS:
    case ERROR_ALREADY_EXISTS:
      return C0_IO_ALREADY_EXISTS;
    case ERROR_INVALID_NAME:
    case ERROR_BAD_PATHNAME:
    case ERROR_FILENAME_EXCED_RANGE:
    case ERROR_DIRECTORY:
    case ERROR_INVALID_PARAMETER:
      return C0_IO_INVALID_PATH;
    case ERROR_BUSY:
    case ERROR_SHARING_VIOLATION:
    case ERROR_LOCK_VIOLATION:
    case ERROR_PIPE_BUSY:
      return C0_IO_BUSY;
    default:
      return C0_IO_FAILURE;
  }
}

static C0IoError c0_last_io_error(void) {
  DWORD err = GetLastError();
  if (err == 0) {
    return C0_IO_FAILURE;
  }
  return c0_map_win_error(err);
}

static C0FsState* c0_fs_state(const C0DynObject* fs) {
  if (!fs) {
    return NULL;
  }
  return (C0FsState*)fs->data;
}
static int c0_fs_resolve_path(const C0FsState* fs,
                              const C0StringView* path,
                              uint8_t** out_utf8,
                              uint32_t* out_len) {
  if (out_utf8) {
    *out_utf8 = NULL;
  }
  if (out_len) {
    *out_len = 0;
  }
  if (!path) {
    return 0;
  }
  if (!path->data && path->len != 0) {
    return 0;
  }
  if (!c0_utf8_valid(path->data, path->len)) {
    return 0;
  }
  if (c0_utf8_has_null(path->data, path->len)) {
    return 0;
  }

  if (fs && fs->restricted) {
    if (!fs->valid) {
      return 0;
    }
    if (c0_path_is_abs(path->data, path->len)) {
      return 0;
    }
    uint32_t joined_len = 0;
    uint8_t* joined = c0_path_join(fs->base_utf8, fs->base_len,
                                   path->data, path->len,
                                   &joined_len);
    if (!joined && (fs->base_len != 0 || path->len != 0)) {
      return 0;
    }
    uint32_t canon_len = 0;
    uint8_t* canon = c0_path_canon(joined ? joined : path->data,
                                   joined ? joined_len : path->len,
                                   &canon_len);
    if (joined) {
      c0_heap_free_raw(joined);
    }
    if (!canon) {
      return 0;
    }
    if (!c0_path_prefix(canon, canon_len, fs->base_utf8, fs->base_len)) {
      c0_heap_free_raw(canon);
      return 0;
    }
    if (out_utf8) {
      *out_utf8 = canon;
    }
    if (out_len) {
      *out_len = canon_len;
    }
    return 1;
  }

  uint32_t canon_len = 0;
  uint8_t* canon = c0_path_canon(path->data, path->len, &canon_len);
  if (!canon) {
    return 0;
  }
  if (out_utf8) {
    *out_utf8 = canon;
  }
  if (out_len) {
    *out_len = canon_len;
  }
  return 1;
}

static C0Union_File_IoError c0_file_err(C0IoError err) {
  C0Union_File_IoError out;
  out.disc = 0;
  out.payload.io_error = err;
  return out;
}

static C0Union_DirIter_IoError c0_dir_err(C0IoError err) {
  C0Union_DirIter_IoError out;
  out.disc = 0;
  out.payload.io_error = err;
  return out;
}

static C0Union_Unit_IoError c0_unit_err(C0IoError err) {
  C0Union_Unit_IoError out;
  out.disc = 1;
  out.payload = err;
  return out;
}

static C0Union_Unit_IoError c0_unit_ok(void) {
  C0Union_Unit_IoError out;
  out.disc = 0;
  out.payload = 0;
  return out;
}

static C0Union_FileKind_IoError c0_kind_err(C0IoError err) {
  C0Union_FileKind_IoError out;
  out.disc = 1;
  out.payload = err;
  return out;
}

static C0Union_FileKind_IoError c0_kind_ok(C0FileKind kind) {
  C0Union_FileKind_IoError out;
  out.disc = 0;
  out.payload = kind;
  return out;
}

static C0Union_StringManaged_IoError c0_string_io_err(C0IoError err) {
  C0Union_StringManaged_IoError out;
  out.disc = 0;
  out.payload.io_error = err;
  return out;
}

static C0Union_BytesManaged_IoError c0_bytes_io_err(C0IoError err) {
  C0Union_BytesManaged_IoError out;
  out.disc = 0;
  out.payload.io_error = err;
  return out;
}
static C0Union_BytesManaged_IoError c0_read_all_bytes_handle(HANDLE handle) {
  if (!handle || handle == INVALID_HANDLE_VALUE) {
    return c0_bytes_io_err(C0_IO_FAILURE);
  }

  LARGE_INTEGER size;
  if (!GetFileSizeEx(handle, &size)) {
    return c0_bytes_io_err(c0_last_io_error());
  }
  if (size.QuadPart < 0) {
    return c0_bytes_io_err(C0_IO_FAILURE);
  }
  if ((uint64_t)size.QuadPart > (uint64_t)SIZE_MAX) {
    return c0_bytes_io_err(C0_IO_FAILURE);
  }

  uint64_t len = (uint64_t)size.QuadPart;
  if (len == 0) {
    C0Union_BytesManaged_IoError out;
    out.disc = 1;
    out.payload.value.data = NULL;
    out.payload.value.len = 0;
    out.payload.value.cap = 0;
    return out;
  }

  uint8_t* data = c0_alloc_managed_bytes(NULL, len, NULL);
  if (!data) {
    return c0_bytes_io_err(C0_IO_FAILURE);
  }

  uint64_t total = 0;
  while (total < len) {
    DWORD chunk = 0;
    DWORD to_read = (DWORD)c0_min_u64(len - total, 0x7FFFFFFF);
    if (!ReadFile(handle, data + total, to_read, &chunk, NULL)) {
      c0_free_managed_bytes(data);
      return c0_bytes_io_err(c0_last_io_error());
    }
    if (chunk == 0) {
      break;
    }
    total += (uint64_t)chunk;
  }

  if (total != len) {
    c0_free_managed_bytes(data);
    return c0_bytes_io_err(C0_IO_FAILURE);
  }

  C0Union_BytesManaged_IoError out;
  out.disc = 1;
  out.payload.value.data = data;
  out.payload.value.len = len;
  out.payload.value.cap = len;
  return out;
}

static C0Union_StringManaged_IoError c0_read_all_string_handle(HANDLE handle) {
  C0Union_BytesManaged_IoError bytes = c0_read_all_bytes_handle(handle);
  if (bytes.disc == 0) {
    return c0_string_io_err(bytes.payload.io_error);
  }

  if (!c0_utf8_valid(bytes.payload.value.data, bytes.payload.value.len)) {
    c0_free_managed_bytes(bytes.payload.value.data);
    return c0_string_io_err(C0_IO_FAILURE);
  }

  C0Union_StringManaged_IoError out;
  out.disc = 1;
  out.payload.value.data = bytes.payload.value.data;
  out.payload.value.len = bytes.payload.value.len;
  out.payload.value.cap = bytes.payload.value.cap;
  return out;
}
C0Union_File_IoError cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fread(
    const C0DynObject* self,
    const C0StringView* path) {
  C0FsState* fs = c0_fs_state(self);
  uint8_t* canon = NULL;
  uint32_t canon_len = 0;
  if (!c0_fs_resolve_path(fs, path, &canon, &canon_len)) {
    return c0_file_err(C0_IO_INVALID_PATH);
  }
  wchar_t* wide = c0_path_utf8_to_wide(canon, canon_len, NULL);
  c0_heap_free_raw(canon);
  if (!wide) {
    return c0_file_err(C0_IO_INVALID_PATH);
  }

  HANDLE h = CreateFileW(wide, GENERIC_READ,
                         FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                         NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  c0_heap_free_raw(wide);
  if (h == INVALID_HANDLE_VALUE) {
    return c0_file_err(c0_last_io_error());
  }
  C0Union_File_IoError out;
  out.disc = 1;
  out.payload.handle = (uint64_t)(uintptr_t)h;
  return out;
}

C0Union_File_IoError cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fwrite(
    const C0DynObject* self,
    const C0StringView* path) {
  C0FsState* fs = c0_fs_state(self);
  uint8_t* canon = NULL;
  uint32_t canon_len = 0;
  if (!c0_fs_resolve_path(fs, path, &canon, &canon_len)) {
    return c0_file_err(C0_IO_INVALID_PATH);
  }
  wchar_t* wide = c0_path_utf8_to_wide(canon, canon_len, NULL);
  c0_heap_free_raw(canon);
  if (!wide) {
    return c0_file_err(C0_IO_INVALID_PATH);
  }

  HANDLE h = CreateFileW(wide, GENERIC_WRITE,
                         FILE_SHARE_READ,
                         NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  c0_heap_free_raw(wide);
  if (h == INVALID_HANDLE_VALUE) {
    return c0_file_err(c0_last_io_error());
  }
  C0Union_File_IoError out;
  out.disc = 1;
  out.payload.handle = (uint64_t)(uintptr_t)h;
  return out;
}

C0Union_File_IoError cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fappend(
    const C0DynObject* self,
    const C0StringView* path) {
  C0FsState* fs = c0_fs_state(self);
  uint8_t* canon = NULL;
  uint32_t canon_len = 0;
  if (!c0_fs_resolve_path(fs, path, &canon, &canon_len)) {
    return c0_file_err(C0_IO_INVALID_PATH);
  }
  wchar_t* wide = c0_path_utf8_to_wide(canon, canon_len, NULL);
  c0_heap_free_raw(canon);
  if (!wide) {
    return c0_file_err(C0_IO_INVALID_PATH);
  }

  HANDLE h = CreateFileW(wide, FILE_APPEND_DATA,
                         FILE_SHARE_READ,
                         NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  c0_heap_free_raw(wide);
  if (h == INVALID_HANDLE_VALUE) {
    return c0_file_err(c0_last_io_error());
  }
  C0Union_File_IoError out;
  out.disc = 1;
  out.payload.handle = (uint64_t)(uintptr_t)h;
  return out;
}

C0Union_File_IoError cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3acreate_x5fwrite(
    const C0DynObject* self,
    const C0StringView* path) {
  C0FsState* fs = c0_fs_state(self);
  uint8_t* canon = NULL;
  uint32_t canon_len = 0;
  if (!c0_fs_resolve_path(fs, path, &canon, &canon_len)) {
    return c0_file_err(C0_IO_INVALID_PATH);
  }
  wchar_t* wide = c0_path_utf8_to_wide(canon, canon_len, NULL);
  c0_heap_free_raw(canon);
  if (!wide) {
    return c0_file_err(C0_IO_INVALID_PATH);
  }

  HANDLE h = CreateFileW(wide, GENERIC_WRITE,
                         FILE_SHARE_READ,
                         NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
  c0_heap_free_raw(wide);
  if (h == INVALID_HANDLE_VALUE) {
    return c0_file_err(c0_last_io_error());
  }
  C0Union_File_IoError out;
  out.disc = 1;
  out.payload.handle = (uint64_t)(uintptr_t)h;
  return out;
}
void cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aread_x5ffile(
    C0Union_StringManaged_IoError* out,
    const C0DynObject* self,
    const C0StringView* path) {
  if (!out) {
    return;
  }
  C0Union_File_IoError file = cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fread(self, path);
  if (file.disc == 0) {
    *out = c0_string_io_err(file.payload.io_error);
    return;
  }
  HANDLE h = (HANDLE)(uintptr_t)file.payload.handle;
  C0Union_StringManaged_IoError result = c0_read_all_string_handle(h);
  CloseHandle(h);
  *out = result;
}

void cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aread_x5fbytes(
    C0Union_BytesManaged_IoError* out,
    const C0DynObject* self,
    const C0StringView* path) {
  if (!out) {
    return;
  }
  C0Union_File_IoError file = cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fread(self, path);
  if (file.disc == 0) {
    *out = c0_bytes_io_err(file.payload.io_error);
    return;
  }
  HANDLE h = (HANDLE)(uintptr_t)file.payload.handle;
  C0Union_BytesManaged_IoError result = c0_read_all_bytes_handle(h);
  CloseHandle(h);
  *out = result;
}

C0Union_Unit_IoError cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5ffile(
    const C0DynObject* self,
    const C0StringView* path,
    const C0BytesView* data) {
  C0FsState* fs = c0_fs_state(self);
  uint8_t* canon = NULL;
  uint32_t canon_len = 0;
  if (!c0_fs_resolve_path(fs, path, &canon, &canon_len)) {
    return c0_unit_err(C0_IO_INVALID_PATH);
  }
  wchar_t* wide = c0_path_utf8_to_wide(canon, canon_len, NULL);
  c0_heap_free_raw(canon);
  if (!wide) {
    return c0_unit_err(C0_IO_INVALID_PATH);
  }

  HANDLE h = CreateFileW(wide, GENERIC_WRITE,
                         FILE_SHARE_READ,
                         NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
  c0_heap_free_raw(wide);
  if (h == INVALID_HANDLE_VALUE) {
    return c0_unit_err(c0_last_io_error());
  }

  uint64_t len = data ? data->len : 0;
  uint64_t written = 0;
  while (written < len) {
    DWORD chunk = 0;
    DWORD to_write = (DWORD)c0_min_u64(len - written, 0x7FFFFFFF);
    if (!WriteFile(h, data->data + written, to_write, &chunk, NULL)) {
      CloseHandle(h);
      return c0_unit_err(c0_last_io_error());
    }
    if (chunk == 0) {
      break;
    }
    written += (uint64_t)chunk;
  }

  CloseHandle(h);
  if (written != len) {
    return c0_unit_err(C0_IO_FAILURE);
  }
  return c0_unit_ok();
}

C0Union_Unit_IoError cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5fstdout(
    const C0DynObject* self,
    const C0StringView* data) {
  (void)self;
  HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
  if (!h || h == INVALID_HANDLE_VALUE) {
    return c0_unit_err(C0_IO_FAILURE);
  }
  uint64_t len = data ? data->len : 0;
  uint64_t written = 0;
  while (written < len) {
    DWORD chunk = 0;
    DWORD to_write = (DWORD)c0_min_u64(len - written, 0x7FFFFFFF);
    if (!WriteFile(h, data->data + written, to_write, &chunk, NULL)) {
      return c0_unit_err(c0_last_io_error());
    }
    if (chunk == 0) {
      break;
    }
    written += (uint64_t)chunk;
  }
  if (written != len) {
    return c0_unit_err(C0_IO_FAILURE);
  }
  return c0_unit_ok();
}

C0Union_Unit_IoError cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5fstderr(
    const C0DynObject* self,
    const C0StringView* data) {
  (void)self;
  HANDLE h = GetStdHandle(STD_ERROR_HANDLE);
  if (!h || h == INVALID_HANDLE_VALUE) {
    return c0_unit_err(C0_IO_FAILURE);
  }
  uint64_t len = data ? data->len : 0;
  uint64_t written = 0;
  while (written < len) {
    DWORD chunk = 0;
    DWORD to_write = (DWORD)c0_min_u64(len - written, 0x7FFFFFFF);
    if (!WriteFile(h, data->data + written, to_write, &chunk, NULL)) {
      return c0_unit_err(c0_last_io_error());
    }
    if (chunk == 0) {
      break;
    }
    written += (uint64_t)chunk;
  }
  if (written != len) {
    return c0_unit_err(C0_IO_FAILURE);
  }
  return c0_unit_ok();
}

uint8_t cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aexists(
    const C0DynObject* self,
    const C0StringView* path) {
  C0FsState* fs = c0_fs_state(self);
  uint8_t* canon = NULL;
  uint32_t canon_len = 0;
  if (!c0_fs_resolve_path(fs, path, &canon, &canon_len)) {
    return 0;
  }
  wchar_t* wide = c0_path_utf8_to_wide(canon, canon_len, NULL);
  c0_heap_free_raw(canon);
  if (!wide) {
    return 0;
  }
  DWORD attrs = GetFileAttributesW(wide);
  c0_heap_free_raw(wide);
  if (attrs == INVALID_FILE_ATTRIBUTES) {
    return 0;
  }
  return 1;
}

C0Union_Unit_IoError cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aremove(
    const C0DynObject* self,
    const C0StringView* path) {
  C0FsState* fs = c0_fs_state(self);
  uint8_t* canon = NULL;
  uint32_t canon_len = 0;
  if (!c0_fs_resolve_path(fs, path, &canon, &canon_len)) {
    return c0_unit_err(C0_IO_INVALID_PATH);
  }
  wchar_t* wide = c0_path_utf8_to_wide(canon, canon_len, NULL);
  c0_heap_free_raw(canon);
  if (!wide) {
    return c0_unit_err(C0_IO_INVALID_PATH);
  }
  DWORD attrs = GetFileAttributesW(wide);
  if (attrs == INVALID_FILE_ATTRIBUTES) {
    c0_heap_free_raw(wide);
    return c0_unit_err(c0_last_io_error());
  }
  BOOL ok = FALSE;
  if (attrs & FILE_ATTRIBUTE_DIRECTORY) {
    ok = RemoveDirectoryW(wide);
  } else {
    ok = DeleteFileW(wide);
  }
  if (!ok) {
    C0IoError err = c0_last_io_error();
    c0_heap_free_raw(wide);
    return c0_unit_err(err);
  }
  c0_heap_free_raw(wide);
  return c0_unit_ok();
}
static uint8_t* c0_entry_key_utf8(const wchar_t* name,
                                  uint32_t name_len,
                                  uint32_t* out_len) {
  if (out_len) {
    *out_len = 0;
  }
  if (!name || name_len == 0) {
    return NULL;
  }

  int fold_len = FoldStringW(MAP_PRECOMPOSED, name, (int)name_len, NULL, 0);
  wchar_t* fold_buf = NULL;
  uint32_t fold_used = name_len;
  const wchar_t* folded = name;
  if (fold_len > 0) {
    fold_buf = (wchar_t*)c0_heap_alloc_raw(sizeof(wchar_t) * (size_t)(fold_len + 1));
    if (fold_buf) {
      int written = FoldStringW(MAP_PRECOMPOSED, name, (int)name_len,
                                fold_buf, fold_len);
      if (written > 0) {
        fold_buf[written] = 0;
        fold_used = (uint32_t)written;
        folded = fold_buf;
      } else {
        c0_heap_free_raw(fold_buf);
        fold_buf = NULL;
        folded = name;
        fold_used = name_len;
      }
    }
  }

  int lower_len = LCMapStringW(LOCALE_INVARIANT, LCMAP_LOWERCASE,
                               folded, (int)fold_used, NULL, 0);
  wchar_t* lower_buf = NULL;
  uint32_t lower_used = fold_used;
  const wchar_t* lowered = folded;
  if (lower_len > 0) {
    lower_buf = (wchar_t*)c0_heap_alloc_raw(sizeof(wchar_t) * (size_t)(lower_len + 1));
    if (lower_buf) {
      int written = LCMapStringW(LOCALE_INVARIANT, LCMAP_LOWERCASE,
                                 folded, (int)fold_used, lower_buf, lower_len);
      if (written > 0) {
        lower_buf[written] = 0;
        lower_used = (uint32_t)written;
        lowered = lower_buf;
      } else {
        c0_heap_free_raw(lower_buf);
        lower_buf = NULL;
        lowered = folded;
        lower_used = fold_used;
      }
    }
  }

  uint32_t key_len = 0;
  uint8_t* key_utf8 = c0_wide_to_utf8(lowered, lower_used, &key_len);

  if (fold_buf) {
    c0_heap_free_raw(fold_buf);
  }
  if (lower_buf) {
    c0_heap_free_raw(lower_buf);
  }
  if (out_len) {
    *out_len = key_len;
  }
  return key_utf8;
}

static int c0_lex_bytes(const uint8_t* a, uint32_t alen,
                        const uint8_t* b, uint32_t blen) {
  uint32_t n = alen < blen ? alen : blen;
  for (uint32_t i = 0; i < n; ++i) {
    if (a[i] != b[i]) {
      return a[i] < b[i] ? -1 : 1;
    }
  }
  if (alen < blen) {
    return -1;
  }
  if (alen > blen) {
    return 1;
  }
  return 0;
}

typedef struct DirEntryTmp {
  wchar_t* name_w;
  uint32_t name_w_len;
  uint8_t* name_utf8;
  uint32_t name_utf8_len;
  uint8_t* key_utf8;
  uint32_t key_utf8_len;
  uint8_t kind;
} DirEntryTmp;

static int c0_entry_cmp(const DirEntryTmp* a, const DirEntryTmp* b) {
  int key_cmp = c0_lex_bytes(a->key_utf8, a->key_utf8_len,
                             b->key_utf8, b->key_utf8_len);
  if (key_cmp != 0) {
    return key_cmp;
  }
  return c0_lex_bytes(a->name_utf8, a->name_utf8_len,
                      b->name_utf8, b->name_utf8_len);
}

static void c0_sort_entries(DirEntryTmp* entries, uint32_t count) {
  for (uint32_t i = 1; i < count; ++i) {
    DirEntryTmp tmp = entries[i];
    uint32_t j = i;
    while (j > 0 && c0_entry_cmp(&tmp, &entries[j - 1]) < 0) {
      entries[j] = entries[j - 1];
      --j;
    }
    entries[j] = tmp;
  }
}
C0Union_DirIter_IoError cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir(
    const C0DynObject* self,
    const C0StringView* path) {
  C0FsState* fs = c0_fs_state(self);
  uint8_t* canon = NULL;
  uint32_t canon_len = 0;
  if (!c0_fs_resolve_path(fs, path, &canon, &canon_len)) {
    return c0_dir_err(C0_IO_INVALID_PATH);
  }
  uint32_t wide_len = 0;
  wchar_t* wide = c0_path_utf8_to_wide(canon, canon_len, &wide_len);
  if (!wide) {
    c0_heap_free_raw(canon);
    return c0_dir_err(C0_IO_INVALID_PATH);
  }

  DWORD attrs = GetFileAttributesW(wide);
  if (attrs == INVALID_FILE_ATTRIBUTES) {
    C0IoError err = c0_last_io_error();
    c0_heap_free_raw(wide);
    c0_heap_free_raw(canon);
    return c0_dir_err(err);
  }
  if ((attrs & FILE_ATTRIBUTE_DIRECTORY) == 0) {
    c0_heap_free_raw(wide);
    c0_heap_free_raw(canon);
    return c0_dir_err(C0_IO_INVALID_PATH);
  }

  uint32_t pattern_len = wide_len + 2;
  wchar_t* pattern = (wchar_t*)c0_heap_alloc_raw(sizeof(wchar_t) * (pattern_len + 1));
  if (!pattern) {
    c0_heap_free_raw(wide);
    c0_heap_free_raw(canon);
    return c0_dir_err(C0_IO_FAILURE);
  }
  c0_memcpy(pattern, wide, sizeof(wchar_t) * wide_len);
  if (wide_len > 0 && wide[wide_len - 1] != L'\\') {
    pattern[wide_len] = L'\\';
    pattern[wide_len + 1] = L'*';
    pattern[wide_len + 2] = 0;
  } else {
    pattern[wide_len] = L'*';
    pattern[wide_len + 1] = 0;
  }

  WIN32_FIND_DATAW data;
  HANDLE find = FindFirstFileW(pattern, &data);
  c0_heap_free_raw(pattern);
  if (find == INVALID_HANDLE_VALUE) {
    C0IoError err = c0_last_io_error();
    c0_heap_free_raw(wide);
    c0_heap_free_raw(canon);
    return c0_dir_err(err);
  }

  uint32_t cap = 16;
  uint32_t count = 0;
  DirEntryTmp* entries = (DirEntryTmp*)c0_heap_alloc_raw(sizeof(DirEntryTmp) * cap);
  if (!entries) {
    FindClose(find);
    c0_heap_free_raw(wide);
    c0_heap_free_raw(canon);
    return c0_dir_err(C0_IO_FAILURE);
  }

  do {
    const wchar_t* name = data.cFileName;
    if (name[0] == L'.' && name[1] == 0) {
      continue;
    }
    if (name[0] == L'.' && name[1] == L'.' && name[2] == 0) {
      continue;
    }

    uint32_t name_len = (uint32_t)c0_wcslen(name);
    wchar_t* name_copy = (wchar_t*)c0_heap_alloc_raw(sizeof(wchar_t) * (name_len + 1));
    if (!name_copy) {
      FindClose(find);
      for (uint32_t i = 0; i < count; ++i) {
        c0_heap_free_raw(entries[i].name_w);
        c0_heap_free_raw(entries[i].name_utf8);
        c0_heap_free_raw(entries[i].key_utf8);
      }
      c0_heap_free_raw(entries);
      c0_heap_free_raw(wide);
      c0_heap_free_raw(canon);
      return c0_dir_err(C0_IO_FAILURE);
    }
    c0_memcpy(name_copy, name, sizeof(wchar_t) * name_len);
    name_copy[name_len] = 0;

    uint32_t name_utf8_len = 0;
    uint8_t* name_utf8 = c0_wide_to_utf8(name_copy, name_len, &name_utf8_len);
    if (!name_utf8) {
      c0_heap_free_raw(name_copy);
      FindClose(find);
      for (uint32_t i = 0; i < count; ++i) {
        c0_heap_free_raw(entries[i].name_w);
        c0_heap_free_raw(entries[i].name_utf8);
        c0_heap_free_raw(entries[i].key_utf8);
      }
      c0_heap_free_raw(entries);
      c0_heap_free_raw(wide);
      c0_heap_free_raw(canon);
      return c0_dir_err(C0_IO_FAILURE);
    }

    uint32_t key_utf8_len = 0;
    uint8_t* key_utf8 = c0_entry_key_utf8(name_copy, name_len, &key_utf8_len);
    if (!key_utf8) {
      key_utf8 = name_utf8;
      key_utf8_len = name_utf8_len;
    }

    if (count == cap) {
      uint32_t new_cap = cap * 2;
      DirEntryTmp* resized = (DirEntryTmp*)c0_heap_alloc_raw(sizeof(DirEntryTmp) * new_cap);
      if (!resized) {
        if (key_utf8 != name_utf8) {
          c0_heap_free_raw(key_utf8);
        }
        c0_heap_free_raw(name_utf8);
        c0_heap_free_raw(name_copy);
        FindClose(find);
        for (uint32_t i = 0; i < count; ++i) {
          c0_heap_free_raw(entries[i].name_w);
          c0_heap_free_raw(entries[i].name_utf8);
          c0_heap_free_raw(entries[i].key_utf8);
        }
        c0_heap_free_raw(entries);
        c0_heap_free_raw(wide);
        c0_heap_free_raw(canon);
        return c0_dir_err(C0_IO_FAILURE);
      }
      for (uint32_t i = 0; i < count; ++i) {
        resized[i] = entries[i];
      }
      c0_heap_free_raw(entries);
      entries = resized;
      cap = new_cap;
    }

    entries[count].name_w = name_copy;
    entries[count].name_w_len = name_len;
    entries[count].name_utf8 = name_utf8;
    entries[count].name_utf8_len = name_utf8_len;
    entries[count].key_utf8 = key_utf8;
    entries[count].key_utf8_len = key_utf8_len;
    entries[count].kind = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        ? C0_FILE_KIND_DIR
        : C0_FILE_KIND_FILE;
    ++count;
  } while (FindNextFileW(find, &data));

  FindClose(find);

  if (count > 1) {
    c0_sort_entries(entries, count);
  }

  C0DirIterState* state = (C0DirIterState*)c0_heap_alloc_raw(sizeof(C0DirIterState));
  if (!state) {
    for (uint32_t i = 0; i < count; ++i) {
      if (entries[i].key_utf8 != entries[i].name_utf8) {
        c0_heap_free_raw(entries[i].key_utf8);
      }
      c0_heap_free_raw(entries[i].name_utf8);
      c0_heap_free_raw(entries[i].name_w);
    }
    c0_heap_free_raw(entries);
    c0_heap_free_raw(wide);
    c0_heap_free_raw(canon);
    return c0_dir_err(C0_IO_FAILURE);
  }

  state->base_path = wide;
  state->base_len = wide_len;
  state->base_utf8 = canon;
  state->base_utf8_len = canon_len;
  state->count = count;
  state->index = 0;
  state->names = NULL;
  state->name_lens = NULL;
  state->kinds = NULL;

  if (count > 0) {
    state->names = (wchar_t**)c0_heap_alloc_raw(sizeof(wchar_t*) * count);
    state->name_lens = (uint32_t*)c0_heap_alloc_raw(sizeof(uint32_t) * count);
    state->kinds = (uint8_t*)c0_heap_alloc_raw(sizeof(uint8_t) * count);
    if (!state->names || !state->name_lens || !state->kinds) {
      for (uint32_t i = 0; i < count; ++i) {
        if (entries[i].key_utf8 != entries[i].name_utf8) {
          c0_heap_free_raw(entries[i].key_utf8);
        }
        c0_heap_free_raw(entries[i].name_utf8);
        c0_heap_free_raw(entries[i].name_w);
      }
      if (state->names) {
        c0_heap_free_raw(state->names);
      }
      if (state->name_lens) {
        c0_heap_free_raw(state->name_lens);
      }
      if (state->kinds) {
        c0_heap_free_raw(state->kinds);
      }
      c0_heap_free_raw(state);
      c0_heap_free_raw(wide);
      c0_heap_free_raw(canon);
      c0_heap_free_raw(entries);
      return c0_dir_err(C0_IO_FAILURE);
    }

    for (uint32_t i = 0; i < count; ++i) {
      state->names[i] = entries[i].name_w;
      state->name_lens[i] = entries[i].name_w_len;
      state->kinds[i] = entries[i].kind;
      if (entries[i].key_utf8 != entries[i].name_utf8) {
        c0_heap_free_raw(entries[i].key_utf8);
      }
      c0_heap_free_raw(entries[i].name_utf8);
    }
  }

  c0_heap_free_raw(entries);

  C0Union_DirIter_IoError out;
  out.disc = 1;
  out.payload.handle = (uint64_t)(uintptr_t)state;
  return out;
}

static C0DirIterState* c0_dir_state(uint64_t handle) {
  if (handle == 0) {
    return NULL;
  }
  return (C0DirIterState*)(uintptr_t)handle;
}
C0Union_Unit_IoError cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3acreate_x5fdir(
    const C0DynObject* self,
    const C0StringView* path) {
  C0FsState* fs = c0_fs_state(self);
  uint8_t* canon = NULL;
  uint32_t canon_len = 0;
  if (!c0_fs_resolve_path(fs, path, &canon, &canon_len)) {
    return c0_unit_err(C0_IO_INVALID_PATH);
  }
  wchar_t* wide = c0_path_utf8_to_wide(canon, canon_len, NULL);
  c0_heap_free_raw(canon);
  if (!wide) {
    return c0_unit_err(C0_IO_INVALID_PATH);
  }
  BOOL ok = CreateDirectoryW(wide, NULL);
  if (!ok) {
    C0IoError err = c0_last_io_error();
    c0_heap_free_raw(wide);
    return c0_unit_err(err);
  }
  c0_heap_free_raw(wide);
  return c0_unit_ok();
}

C0Union_Unit_IoError cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aensure_x5fdir(
    const C0DynObject* self,
    const C0StringView* path) {
  C0FsState* fs = c0_fs_state(self);
  uint8_t* canon = NULL;
  uint32_t canon_len = 0;
  if (!c0_fs_resolve_path(fs, path, &canon, &canon_len)) {
    return c0_unit_err(C0_IO_INVALID_PATH);
  }
  uint32_t wide_len = 0;
  wchar_t* wide = c0_path_utf8_to_wide(canon, canon_len, &wide_len);
  c0_heap_free_raw(canon);
  if (!wide) {
    return c0_unit_err(C0_IO_INVALID_PATH);
  }

  if (wide_len == 0) {
    c0_heap_free_raw(wide);
    return c0_unit_err(C0_IO_INVALID_PATH);
  }

  DWORD attrs = GetFileAttributesW(wide);
  if (attrs != INVALID_FILE_ATTRIBUTES) {
    if (attrs & FILE_ATTRIBUTE_DIRECTORY) {
      c0_heap_free_raw(wide);
      return c0_unit_ok();
    }
    c0_heap_free_raw(wide);
    return c0_unit_err(C0_IO_ALREADY_EXISTS);
  }

  wchar_t* buf = (wchar_t*)c0_heap_alloc_raw(sizeof(wchar_t) * (wide_len + 1));
  if (!buf) {
    c0_heap_free_raw(wide);
    return c0_unit_err(C0_IO_FAILURE);
  }
  c0_memcpy(buf, wide, sizeof(wchar_t) * wide_len);
  buf[wide_len] = 0;

  uint32_t start = 0;
  if (wide_len >= 3 && wide[1] == L':' && wide[2] == L'\\') {
    start = 3;
  } else if (wide_len >= 2 && wide[0] == L'\\' && wide[1] == L'\\') {
    uint32_t idx = 2;
    while (idx < wide_len && wide[idx] != L'\\') {
      ++idx;
    }
    if (idx < wide_len) {
      ++idx;
    }
    while (idx < wide_len && wide[idx] != L'\\') {
      ++idx;
    }
    if (idx < wide_len) {
      ++idx;
    }
    start = idx;
  } else if (wide[0] == L'\\') {
    start = 1;
  }

  for (uint32_t i = start; i <= wide_len; ++i) {
    if (i == wide_len || buf[i] == L'\\') {
      wchar_t saved = buf[i];
      buf[i] = 0;
      if (buf[0] != 0) {
        if (!CreateDirectoryW(buf, NULL)) {
          DWORD err = GetLastError();
          if (err == ERROR_ALREADY_EXISTS) {
            DWORD a = GetFileAttributesW(buf);
            if (a == INVALID_FILE_ATTRIBUTES || !(a & FILE_ATTRIBUTE_DIRECTORY)) {
              buf[i] = saved;
              c0_heap_free_raw(buf);
              c0_heap_free_raw(wide);
              return c0_unit_err(C0_IO_ALREADY_EXISTS);
            }
          } else {
            C0IoError mapped = c0_map_win_error(err);
            buf[i] = saved;
            c0_heap_free_raw(buf);
            c0_heap_free_raw(wide);
            return c0_unit_err(mapped);
          }
        }
      }
      buf[i] = saved;
    }
  }

  c0_heap_free_raw(buf);
  c0_heap_free_raw(wide);
  return c0_unit_ok();
}

C0Union_FileKind_IoError cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3akind(
    const C0DynObject* self,
    const C0StringView* path) {
  C0FsState* fs = c0_fs_state(self);
  uint8_t* canon = NULL;
  uint32_t canon_len = 0;
  if (!c0_fs_resolve_path(fs, path, &canon, &canon_len)) {
    return c0_kind_err(C0_IO_INVALID_PATH);
  }
  wchar_t* wide = c0_path_utf8_to_wide(canon, canon_len, NULL);
  c0_heap_free_raw(canon);
  if (!wide) {
    return c0_kind_err(C0_IO_INVALID_PATH);
  }

  DWORD attrs = GetFileAttributesW(wide);
  c0_heap_free_raw(wide);
  if (attrs == INVALID_FILE_ATTRIBUTES) {
    return c0_kind_err(c0_last_io_error());
  }
  if (attrs & FILE_ATTRIBUTE_DIRECTORY) {
    return c0_kind_ok(C0_FILE_KIND_DIR);
  }
  return c0_kind_ok(C0_FILE_KIND_FILE);
}

C0DynObject cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3arestrict(
    const C0DynObject* self,
    const C0StringView* path) {
  C0DynObject out;
  out.data = NULL;
  out.vtable = self ? self->vtable : NULL;

  C0FsState* state = (C0FsState*)c0_heap_alloc_raw(sizeof(C0FsState));
  if (!state) {
    return out;
  }
  state->restricted = 1;
  state->valid = 0;
  state->base_utf8 = NULL;
  state->base_len = 0;

  if (path && path->data && c0_utf8_valid(path->data, path->len) &&
      !c0_utf8_has_null(path->data, path->len)) {
    uint32_t base_len = 0;
    uint8_t* base = c0_path_canon(path->data, path->len, &base_len);
    if (base) {
      state->base_utf8 = base;
      state->base_len = base_len;
      state->valid = 1;
    }
  }

  out.data = state;
  return out;
}
C0Union_StringManaged_IoError File_x3a_x3aRead_x3a_x3aread_x5fall(
    const C0FileHandle* self) {
  if (!self) {
    return c0_string_io_err(C0_IO_FAILURE);
  }
  HANDLE h = (HANDLE)(uintptr_t)self->handle;
  return c0_read_all_string_handle(h);
}

C0Union_BytesManaged_IoError File_x3a_x3aRead_x3a_x3aread_x5fall_x5fbytes(
    const C0FileHandle* self) {
  if (!self) {
    return c0_bytes_io_err(C0_IO_FAILURE);
  }
  HANDLE h = (HANDLE)(uintptr_t)self->handle;
  return c0_read_all_bytes_handle(h);
}

void File_x3a_x3aRead_x3a_x3aclose(C0FileHandle self) {
  HANDLE h = (HANDLE)(uintptr_t)self.handle;
  if (h && h != INVALID_HANDLE_VALUE) {
    CloseHandle(h);
  }
}

static C0Union_Unit_IoError c0_file_write_handle(HANDLE h,
                                                 const C0BytesView* data) {
  if (!h || h == INVALID_HANDLE_VALUE) {
    return c0_unit_err(C0_IO_FAILURE);
  }
  uint64_t len = data ? data->len : 0;
  uint64_t written = 0;
  while (written < len) {
    DWORD chunk = 0;
    DWORD to_write = (DWORD)c0_min_u64(len - written, 0x7FFFFFFF);
    if (!WriteFile(h, data->data + written, to_write, &chunk, NULL)) {
      return c0_unit_err(c0_last_io_error());
    }
    if (chunk == 0) {
      break;
    }
    written += (uint64_t)chunk;
  }
  if (written != len) {
    return c0_unit_err(C0_IO_FAILURE);
  }
  return c0_unit_ok();
}

C0Union_Unit_IoError File_x3a_x3aWrite_x3a_x3awrite(
    C0FileHandle* self,
    const C0BytesView* data) {
  if (!self) {
    return c0_unit_err(C0_IO_FAILURE);
  }
  HANDLE h = (HANDLE)(uintptr_t)self->handle;
  return c0_file_write_handle(h, data);
}

C0Union_Unit_IoError File_x3a_x3aWrite_x3a_x3aflush(
    C0FileHandle* self) {
  if (!self) {
    return c0_unit_err(C0_IO_FAILURE);
  }
  HANDLE h = (HANDLE)(uintptr_t)self->handle;
  if (!h || h == INVALID_HANDLE_VALUE) {
    return c0_unit_err(C0_IO_FAILURE);
  }
  if (!FlushFileBuffers(h)) {
    return c0_unit_err(c0_last_io_error());
  }
  return c0_unit_ok();
}

void File_x3a_x3aWrite_x3a_x3aclose(C0FileHandle self) {
  HANDLE h = (HANDLE)(uintptr_t)self.handle;
  if (h && h != INVALID_HANDLE_VALUE) {
    CloseHandle(h);
  }
}

C0Union_Unit_IoError File_x3a_x3aAppend_x3a_x3awrite(
    C0FileHandle* self,
    const C0BytesView* data) {
  if (!self) {
    return c0_unit_err(C0_IO_FAILURE);
  }
  HANDLE h = (HANDLE)(uintptr_t)self->handle;
  return c0_file_write_handle(h, data);
}

C0Union_Unit_IoError File_x3a_x3aAppend_x3a_x3aflush(
    C0FileHandle* self) {
  if (!self) {
    return c0_unit_err(C0_IO_FAILURE);
  }
  HANDLE h = (HANDLE)(uintptr_t)self->handle;
  if (!h || h == INVALID_HANDLE_VALUE) {
    return c0_unit_err(C0_IO_FAILURE);
  }
  if (!FlushFileBuffers(h)) {
    return c0_unit_err(c0_last_io_error());
  }
  return c0_unit_ok();
}

void File_x3a_x3aAppend_x3a_x3aclose(C0FileHandle self) {
  HANDLE h = (HANDLE)(uintptr_t)self.handle;
  if (h && h != INVALID_HANDLE_VALUE) {
    CloseHandle(h);
  }
}
C0Union_DirEntry_Unit_IoError DirIter_x3a_x3aOpen_x3a_x3anext(
    C0DirIterHandle* self) {
  C0Union_DirEntry_Unit_IoError out;
  if (!self) {
    out.disc = 2;
    out.payload.io_error = C0_IO_FAILURE;
    return out;
  }
  C0DirIterState* state = c0_dir_state(self->handle);
  if (!state) {
    out.disc = 2;
    out.payload.io_error = C0_IO_FAILURE;
    return out;
  }
  if (state->index >= state->count) {
    out.disc = 0;
    out.payload.io_error = 0;
    return out;
  }

  uint32_t idx = state->index;
  wchar_t* name_w = state->names[idx];
  uint32_t name_w_len = state->name_lens[idx];
  uint32_t name_utf8_len = 0;
  uint8_t* name_utf8 = c0_wide_to_utf8(name_w, name_w_len, &name_utf8_len);
  if (!name_utf8 && name_w_len != 0) {
    out.disc = 2;
    out.payload.io_error = C0_IO_FAILURE;
    return out;
  }

  uint32_t path_len = 0;
  uint8_t* path_utf8 = c0_path_join(state->base_utf8, state->base_utf8_len,
                                    name_utf8, name_utf8_len, &path_len);
  if (!path_utf8 && (state->base_utf8_len != 0 || name_utf8_len != 0)) {
    if (name_utf8) {
      c0_heap_free_raw(name_utf8);
    }
    out.disc = 2;
    out.payload.io_error = C0_IO_FAILURE;
    return out;
  }

  uint8_t* name_managed = NULL;
  uint64_t name_len64 = name_utf8_len;
  if (name_len64 == 0) {
    name_managed = NULL;
  } else {
    name_managed = c0_alloc_managed_bytes(NULL, name_len64, NULL);
  }
  if (name_len64 != 0 && !name_managed) {
    if (name_utf8) {
      c0_heap_free_raw(name_utf8);
    }
    if (path_utf8) {
      c0_heap_free_raw(path_utf8);
    }
    out.disc = 2;
    out.payload.io_error = C0_IO_FAILURE;
    return out;
  }

  uint8_t* path_managed = NULL;
  uint64_t path_len64 = path_len;
  if (path_len64 == 0) {
    path_managed = NULL;
  } else {
    path_managed = c0_alloc_managed_bytes(NULL, path_len64, NULL);
  }
  if (path_len64 != 0 && !path_managed) {
    if (name_managed) {
      c0_free_managed_bytes(name_managed);
    }
    if (name_utf8) {
      c0_heap_free_raw(name_utf8);
    }
    if (path_utf8) {
      c0_heap_free_raw(path_utf8);
    }
    out.disc = 2;
    out.payload.io_error = C0_IO_FAILURE;
    return out;
  }

  if (name_len64 != 0) {
    c0_memcpy(name_managed, name_utf8, (size_t)name_len64);
  }
  if (path_len64 != 0) {
    c0_memcpy(path_managed, path_utf8, (size_t)path_len64);
  }

  if (name_utf8) {
    c0_heap_free_raw(name_utf8);
  }
  if (path_utf8) {
    c0_heap_free_raw(path_utf8);
  }

  C0DirEntry entry;
  entry.name.data = name_managed;
  entry.name.len = name_len64;
  entry.name.cap = name_len64;
  entry.path.data = path_managed;
  entry.path.len = path_len64;
  entry.path.cap = path_len64;
  entry.kind = state->kinds[idx];

  state->index += 1;

  out.disc = 1;
  out.payload.entry = entry;
  return out;
}

void DirIter_x3a_x3aOpen_x3a_x3aclose(C0DirIterHandle self) {
  C0DirIterState* state = c0_dir_state(self.handle);
  if (!state) {
    return;
  }
  if (state->names) {
    for (uint32_t i = 0; i < state->count; ++i) {
      c0_heap_free_raw(state->names[i]);
    }
    c0_heap_free_raw(state->names);
  }
  if (state->name_lens) {
    c0_heap_free_raw(state->name_lens);
  }
  if (state->kinds) {
    c0_heap_free_raw(state->kinds);
  }
  if (state->base_path) {
    c0_heap_free_raw(state->base_path);
  }
  if (state->base_utf8) {
    c0_heap_free_raw(state->base_utf8);
  }
  c0_heap_free_raw(state);
}
