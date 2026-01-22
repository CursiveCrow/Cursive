#include "rt_internal.h"

static C0HeapState* c0_heap_state(const C0DynObject* heap) {
  if (!heap) {
    return NULL;
  }
  return (C0HeapState*)heap->data;
}

C0DynObject cursive_x3a_x3aruntime_x3a_x3aheap_x3a_x3awith_x5fquota(
    const C0DynObject* self,
    const uint64_t* size) {
  c0_trace_emit_rule("BuiltinSym-HeapAllocator-WithQuota");
  C0DynObject out;
  out.data = NULL;
  out.vtable = self ? self->vtable : NULL;

  C0HeapState* parent = c0_heap_state(self);
  C0HeapState* heap = (C0HeapState*)c0_heap_alloc_raw(sizeof(C0HeapState));
  if (!heap) {
    return out;
  }
  heap->parent = parent;
  heap->quota = size ? *size : 0;
  heap->used = 0;

  out.data = heap;
  return out;
}

void* cursive_x3a_x3aruntime_x3a_x3aheap_x3a_x3aalloc_x5fraw(
    const C0DynObject* self,
    const uint64_t* count) {
  c0_trace_emit_rule("BuiltinSym-HeapAllocator-AllocRaw");
  const uint64_t size = count ? *count : 0;
  if (size == 0) {
    return NULL;
  }
  if (size > (uint64_t)SIZE_MAX) {
    return NULL;
  }

  C0HeapState* heap = c0_heap_state(self);
  int quota_exceeded = 0;
  if (!c0_heap_can_alloc(heap, size, &quota_exceeded)) {
    return NULL;
  }

  void* ptr = c0_heap_alloc_raw((size_t)size);
  if (!ptr) {
    return NULL;
  }
  c0_heap_add_used(heap, size);
  return ptr;
}

void cursive_x3a_x3aruntime_x3a_x3aheap_x3a_x3adealloc_x5fraw(
    const C0DynObject* self,
    void** ptr,
    const uint64_t* count) {
  c0_trace_emit_rule("BuiltinSym-HeapAllocator-DeallocRaw");
  if (!ptr || !*ptr) {
    return;
  }
  const uint64_t size = count ? *count : 0;
  C0HeapState* heap = c0_heap_state(self);
  c0_heap_free_raw(*ptr);
  c0_heap_sub_used(heap, size);
}
