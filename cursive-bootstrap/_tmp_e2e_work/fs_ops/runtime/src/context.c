#include "rt_internal.h"

void cursive_x3a_x3aruntime_x3a_x3acontext_x5finit(C0Context* out) {
  if (!out) {
    return;
  }

  out->fs.data = NULL;
  out->fs.vtable = NULL;
  out->heap.data = NULL;
  out->heap.vtable = NULL;

  C0FsState* fs = (C0FsState*)c0_heap_alloc_raw(sizeof(C0FsState));
  if (!fs) {
    return;
  }
  fs->base_utf8 = NULL;
  fs->base_len = 0;
  fs->restricted = 0;
  fs->valid = 1;

  C0HeapState* heap = (C0HeapState*)c0_heap_alloc_raw(sizeof(C0HeapState));
  if (!heap) {
    c0_heap_free_raw(fs);
    return;
  }
  heap->parent = NULL;
  heap->quota = 0;
  heap->used = 0;

  out->fs.data = fs;
  out->fs.vtable = NULL;
  out->heap.data = heap;
  out->heap.vtable = NULL;
}
