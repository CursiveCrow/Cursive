#ifndef CURSIVE0_RT_H
#define CURSIVE0_RT_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Basic view/managed string and bytes types
typedef struct C0StringView {
  const uint8_t* data;
  uint64_t len;
} C0StringView;

typedef struct C0StringManaged {
  uint8_t* data;
  uint64_t len;
  uint64_t cap;
} C0StringManaged;

typedef struct C0BytesView {
  const uint8_t* data;
  uint64_t len;
} C0BytesView;

typedef struct C0BytesManaged {
  uint8_t* data;
  uint64_t len;
  uint64_t cap;
} C0BytesManaged;

// Dynamic object layout (data pointer + vtable pointer)
typedef struct C0DynObject {
  void* data;
  void* vtable;
} C0DynObject;

// Context record: { fs: dyn FileSystem, heap: dyn HeapAllocator, sys: System (empty), reactor: dyn Reactor }
typedef struct C0Context {
  C0DynObject fs;
  C0DynObject heap;
  C0DynObject reactor;
} C0Context;

typedef enum {
  C0_DOMAIN_CPU = 0,
  C0_DOMAIN_GPU = 1,
  C0_DOMAIN_INLINE = 2,
} C0DomainKind;

typedef struct C0ExecutionDomain {
  uint8_t kind;
  uint8_t _pad[7];
  uint64_t max_concurrency;
} C0ExecutionDomain;

// Modal string layout (string with unspecified state)
typedef struct C0StringModal {
  uint8_t disc;
  uint8_t _pad[7];
  C0StringManaged payload;
} C0StringModal;

// RegionOptions record: { stack_size: usize, name: string }
typedef struct C0RegionOptions {
  uint64_t stack_size;
  C0StringModal name;
} C0RegionOptions;

// Region modal layout (disc + handle payload)
typedef struct C0Region {
  uint8_t disc;
  uint8_t _pad[7];
  uint64_t handle;
} C0Region;

// Range layout (tag, lo, hi)
typedef struct C0Range {
  uint8_t tag;
  uint8_t _pad[7];
  uint64_t lo;
  uint64_t hi;
} C0Range;

// Slice layout for [u8]
typedef struct C0SliceU8 {
  const uint8_t* data;
  uint64_t len;
} C0SliceU8;

// File/DirIter state payloads (handle: usize)
typedef struct C0FileHandle {
  uint64_t handle;
} C0FileHandle;

typedef struct C0DirIterHandle {
  uint64_t handle;
} C0DirIterHandle;

// IoError enum (u8)
typedef uint8_t C0IoError;

enum {
  C0_IO_NOTFOUND = 0,
  C0_IO_PERMISSION_DENIED = 1,
  C0_IO_ALREADY_EXISTS = 2,
  C0_IO_INVALID_PATH = 3,
  C0_IO_BUSY = 4,
  C0_IO_FAILURE = 5,
};

// FileKind enum (u8)
typedef uint8_t C0FileKind;

enum {
  C0_FILE_KIND_FILE = 0,
  C0_FILE_KIND_DIR = 1,
  C0_FILE_KIND_OTHER = 2,
};

// AllocationError enum with payload (u8 disc + usize payload)
typedef struct C0AllocationError {
  uint8_t disc;
  uint8_t _pad[7];
  uint64_t size;
} C0AllocationError;

enum {
  C0_ALLOC_OUT_OF_MEMORY = 0,
  C0_ALLOC_QUOTA_EXCEEDED = 1,
};

// DirEntry record: { name: string@Managed, path: string@Managed, kind: FileKind }
typedef struct C0DirEntry {
  C0StringManaged name;
  C0StringManaged path;
  uint8_t kind;
  uint8_t _pad[7];
} C0DirEntry;

// Union layouts (tagged) used by runtime APIs

typedef struct C0Union_StringManaged_AllocError {
  uint8_t disc;
  uint8_t _pad[7];
  union {
    C0AllocationError alloc_error;
    C0StringManaged value;
  } payload;
} C0Union_StringManaged_AllocError;

typedef struct C0Union_BytesManaged_AllocError {
  uint8_t disc;
  uint8_t _pad[7];
  union {
    C0AllocationError alloc_error;
    C0BytesManaged value;
  } payload;
} C0Union_BytesManaged_AllocError;

typedef struct C0Union_Unit_AllocError {
  uint8_t disc;
  uint8_t _pad[7];
  C0AllocationError error;
} C0Union_Unit_AllocError;

typedef struct C0Union_StringManaged_IoError {
  uint8_t disc;
  uint8_t _pad[7];
  union {
    uint8_t io_error;
    C0StringManaged value;
  } payload;
} C0Union_StringManaged_IoError;

typedef struct C0Union_BytesManaged_IoError {
  uint8_t disc;
  uint8_t _pad[7];
  union {
    uint8_t io_error;
    C0BytesManaged value;
  } payload;
} C0Union_BytesManaged_IoError;

typedef struct C0Union_File_IoError {
  uint8_t disc;
  uint8_t _pad[7];
  union {
    uint8_t io_error;
    uint64_t handle;
  } payload;
} C0Union_File_IoError;

typedef struct C0Union_DirIter_IoError {
  uint8_t disc;
  uint8_t _pad[7];
  union {
    uint8_t io_error;
    uint64_t handle;
  } payload;
} C0Union_DirIter_IoError;

typedef struct C0Union_Unit_IoError {
  uint8_t disc;
  uint8_t payload;
} C0Union_Unit_IoError;

typedef struct C0Union_FileKind_IoError {
  uint8_t disc;
  uint8_t payload;
} C0Union_FileKind_IoError;

typedef struct C0Union_DirEntry_Unit_IoError {
  uint8_t disc;
  uint8_t _pad[7];
  union {
    C0DirEntry entry;
    uint8_t io_error;
  } payload;
} C0Union_DirEntry_Unit_IoError;

// -----------------------------------------------------------------------------
// Runtime functions (mangled symbol names)
// -----------------------------------------------------------------------------

// Panic
void cursive_x3a_x3aruntime_x3a_x3apanic(uint32_t code);

// Spec trace
void cursive_x3a_x3aruntime_x3a_x3aspec_x5ftrace_x3a_x3aemit(
  const C0StringView* rule_id,
  const C0StringView* payload);

// Context initialization
void cursive_x3a_x3aruntime_x3a_x3acontext_x5finit(C0Context* out);

// String/bytes drop
void cursive_x3a_x3aruntime_x3a_x3astring_x3a_x3adrop_x5fmanaged(C0StringManaged* value);
void cursive_x3a_x3aruntime_x3a_x3abytes_x3a_x3adrop_x5fmanaged(C0BytesManaged* value);

// Region procs
C0Region cursive_x3a_x3aruntime_x3a_x3aregion_x3a_x3anew_x5fscoped(const C0RegionOptions* options);
void cursive_x3a_x3aruntime_x3a_x3aregion_x3a_x3aalloc(C0Region self, const void* value);
C0Region cursive_x3a_x3aruntime_x3a_x3aregion_x3a_x3areset_x5funchecked(C0Region self);
C0Region cursive_x3a_x3aruntime_x3a_x3aregion_x3a_x3afreeze(C0Region self);
C0Region cursive_x3a_x3aruntime_x3a_x3aregion_x3a_x3athaw(C0Region self);
C0Region cursive_x3a_x3aruntime_x3a_x3aregion_x3a_x3afree_x5funchecked(C0Region self);

// String builtins
void cursive_x3a_x3aruntime_x3a_x3astring_x3a_x3afrom(
  C0Union_StringManaged_AllocError* out,
  const C0StringView* source,
  const C0DynObject* heap);

C0StringView cursive_x3a_x3aruntime_x3a_x3astring_x3a_x3aas_x5fview(
  const C0StringManaged* self);

void cursive_x3a_x3aruntime_x3a_x3astring_x3a_x3ato_x5fmanaged(
  C0Union_StringManaged_AllocError* out,
  const C0StringView* self,
  const C0DynObject* heap);

void cursive_x3a_x3aruntime_x3a_x3astring_x3a_x3aclone_x5fwith(
  C0Union_StringManaged_AllocError* out,
  const C0StringManaged* self,
  const C0DynObject* heap);

void cursive_x3a_x3aruntime_x3a_x3astring_x3a_x3aappend(
  C0Union_Unit_AllocError* out,
  C0StringManaged* self,
  const C0StringView* data,
  const C0DynObject* heap);

uint64_t cursive_x3a_x3aruntime_x3a_x3astring_x3a_x3alength(
  const C0StringView* self);

uint8_t cursive_x3a_x3aruntime_x3a_x3astring_x3a_x3ais_x5fempty(
  const C0StringView* self);

// Bytes builtins
void cursive_x3a_x3aruntime_x3a_x3abytes_x3a_x3awith_x5fcapacity(
  C0Union_BytesManaged_AllocError* out,
  const uint64_t* cap,
  const C0DynObject* heap);

void cursive_x3a_x3aruntime_x3a_x3abytes_x3a_x3afrom_x5fslice(
  C0Union_BytesManaged_AllocError* out,
  const C0SliceU8* data,
  const C0DynObject* heap);

C0BytesView cursive_x3a_x3aruntime_x3a_x3abytes_x3a_x3aas_x5fview(
  const C0BytesManaged* self);

void cursive_x3a_x3aruntime_x3a_x3abytes_x3a_x3ato_x5fmanaged(
  C0Union_BytesManaged_AllocError* out,
  const C0BytesView* self,
  const C0DynObject* heap);

C0BytesView cursive_x3a_x3aruntime_x3a_x3abytes_x3a_x3aview(
  const C0SliceU8* data);

C0BytesView cursive_x3a_x3aruntime_x3a_x3abytes_x3a_x3aview_x5fstring(
  const C0StringView* data);

void cursive_x3a_x3aruntime_x3a_x3abytes_x3a_x3aappend(
  C0Union_Unit_AllocError* out,
  C0BytesManaged* self,
  const C0BytesView* data,
  const C0DynObject* heap);

uint64_t cursive_x3a_x3aruntime_x3a_x3abytes_x3a_x3alength(
  const C0BytesView* self);

uint8_t cursive_x3a_x3aruntime_x3a_x3abytes_x3a_x3ais_x5fempty(
  const C0BytesView* self);

// FileSystem builtins
C0Union_File_IoError cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fread(
  const C0DynObject* self,
  const C0StringView* path);

C0Union_File_IoError cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fwrite(
  const C0DynObject* self,
  const C0StringView* path);

C0Union_File_IoError cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fappend(
  const C0DynObject* self,
  const C0StringView* path);

C0Union_File_IoError cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3acreate_x5fwrite(
  const C0DynObject* self,
  const C0StringView* path);

void cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aread_x5ffile(
  C0Union_StringManaged_IoError* out,
  const C0DynObject* self,
  const C0StringView* path);

void cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aread_x5fbytes(
  C0Union_BytesManaged_IoError* out,
  const C0DynObject* self,
  const C0StringView* path);

C0Union_Unit_IoError cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5ffile(
  const C0DynObject* self,
  const C0StringView* path,
  const C0BytesView* data);

C0Union_Unit_IoError cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5fstdout(
  const C0DynObject* self,
  const C0StringView* data);

C0Union_Unit_IoError cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5fstderr(
  const C0DynObject* self,
  const C0StringView* data);

uint8_t cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aexists(
  const C0DynObject* self,
  const C0StringView* path);

C0Union_Unit_IoError cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aremove(
  const C0DynObject* self,
  const C0StringView* path);

C0Union_DirIter_IoError cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir(
  const C0DynObject* self,
  const C0StringView* path);

C0Union_Unit_IoError cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3acreate_x5fdir(
  const C0DynObject* self,
  const C0StringView* path);

C0Union_Unit_IoError cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aensure_x5fdir(
  const C0DynObject* self,
  const C0StringView* path);

C0Union_FileKind_IoError cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3akind(
  const C0DynObject* self,
  const C0StringView* path);

C0DynObject cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3arestrict(
  const C0DynObject* self,
  const C0StringView* path);

// HeapAllocator builtins
C0DynObject cursive_x3a_x3aruntime_x3a_x3aheap_x3a_x3awith_x5fquota(
  const C0DynObject* self,
  const uint64_t* size);

void* cursive_x3a_x3aruntime_x3a_x3aheap_x3a_x3aalloc_x5fraw(
  const C0DynObject* self,
  const uint64_t* count);

void cursive_x3a_x3aruntime_x3a_x3aheap_x3a_x3adealloc_x5fraw(
  const C0DynObject* self,
  void** ptr,
  const uint64_t* count);

// -----------------------------------------------------------------------------
// C0X Extension: Structured Concurrency Runtime Support (§18)
// -----------------------------------------------------------------------------

// §18.2 Context execution domain constructors
C0DynObject cursive_x3a_x3aruntime_x3a_x3acontext_x3a_x3acpu(
  const C0Context* self);
C0DynObject cursive_x3a_x3aruntime_x3a_x3acontext_x3a_x3agpu(
  const C0Context* self);
C0DynObject cursive_x3a_x3aruntime_x3a_x3acontext_x3a_x3ainline(
  const C0Context* self);

// §18.2 ExecutionDomain methods
C0StringView cursive_x3a_x3aruntime_x3a_x3aexecution_x5fdomain_x3a_x3aname(
  const C0DynObject* self);
uint64_t cursive_x3a_x3aruntime_x3a_x3aexecution_x5fdomain_x3a_x3amax_x5fconcurrency(
  const C0DynObject* self);

// §18.1.1 Begin parallel block - creates execution context
void* cursive0_parallel_begin(C0DynObject domain, void* cancel_token, const char* name);

// §18.1.2 Join parallel block - waits for completion, propagates first panic
int cursive0_parallel_join(void* ctx_ptr);

// §18.4.2 Create spawn handle - returns SpawnHandle<T>@Pending
void* cursive0_spawn_create(void* env, size_t env_size,
                            void (*body)(void* env, void* result, void* panic_out),
                            size_t result_size);

// §10.3 Wait for spawn result - blocks until ready, extracts value
void* cursive0_spawn_wait(void* handle_ptr);

// §18.5.2 Dispatch iteration - parallel data iteration
void cursive0_dispatch_run(C0Range range, size_t elem_size, size_t result_size,
                           void (*body)(void* elem, void* captured, void* result, void* panic_out),
                           void* captured_env,
                           C0StringView reduce_op,
                           void* reduce_result,
                           void (*reduce_fn)(void* lhs, void* rhs, void* out, void* panic_out),
                           int ordered,
                           size_t chunk_size);

// §18.6.1 Cancellation token operations
void* cursive0_cancel_token_new(void);
void cursive0_cancel_token_cancel(void* token_ptr);
int cursive0_cancel_token_is_cancelled(void* token_ptr);

// §18.7 Panic handling in parallel contexts
void cursive0_parallel_work_panic(void* ctx_ptr, uint32_t code);

// Low-level panic helper (used by parallel runtime)
void cursive0_panic(uint32_t code);

// CancelToken modal methods
void* CancelToken_x3a_x3anew(void);
void CancelToken_x3a_x3aActive_x3a_x3acancel(void* self);
uint8_t CancelToken_x3a_x3aActive_x3a_x3ais_x5fcancelled(void* self);
uint8_t CancelToken_x3a_x3aCancelled_x3a_x3ais_x5fcancelled(void* self);
void* CancelToken_x3a_x3aActive_x3a_x3achild(void* self);

// File/DirIter methods (modal state methods / transitions)
C0Union_StringManaged_IoError File_x3a_x3aRead_x3a_x3aread_x5fall(
  const C0FileHandle* self);

C0Union_BytesManaged_IoError File_x3a_x3aRead_x3a_x3aread_x5fall_x5fbytes(
  const C0FileHandle* self);

void File_x3a_x3aRead_x3a_x3aclose(
  C0FileHandle self);

C0Union_Unit_IoError File_x3a_x3aWrite_x3a_x3awrite(
  C0FileHandle* self,
  const C0BytesView* data);

C0Union_Unit_IoError File_x3a_x3aWrite_x3a_x3aflush(
  C0FileHandle* self);

void File_x3a_x3aWrite_x3a_x3aclose(
  C0FileHandle self);

C0Union_Unit_IoError File_x3a_x3aAppend_x3a_x3awrite(
  C0FileHandle* self,
  const C0BytesView* data);

C0Union_Unit_IoError File_x3a_x3aAppend_x3a_x3aflush(
  C0FileHandle* self);

void File_x3a_x3aAppend_x3a_x3aclose(
  C0FileHandle self);

C0Union_DirEntry_Unit_IoError DirIter_x3a_x3aOpen_x3a_x3anext(
  C0DirIterHandle* self);

void DirIter_x3a_x3aOpen_x3a_x3aclose(
  C0DirIterHandle self);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // CURSIVE0_RT_H
