#include "rt_internal.h"

void* cursive_x3a_x3aruntime_x3a_x3aasync_x3a_x3aalloc_x5fframe(uint64_t size,
                                                               uint64_t align) {
  c0_trace_emit_rule("BuiltinSym-Async-AllocFrame");
  if (size == 0) {
    return NULL;
  }
  if (align == 0) {
    align = 1;
  }
  if (align < (uint64_t)sizeof(void*)) {
    align = (uint64_t)sizeof(void*);
  }
  if ((align & (align - 1)) != 0) {
    uint64_t pow2 = 1;
    while (pow2 < align) {
      pow2 <<= 1;
    }
    align = pow2;
  }

  uint64_t total = size + align - 1 + (uint64_t)sizeof(void*);
  if (total < size) {
    return NULL;
  }

  void* base = c0_heap_alloc_raw((size_t)total);
  if (!base) {
    return NULL;
  }

  uintptr_t raw = (uintptr_t)base + (uintptr_t)sizeof(void*);
  uintptr_t aligned = (raw + (uintptr_t)(align - 1)) & ~(uintptr_t)(align - 1);
  ((void**)aligned)[-1] = base;
  return (void*)aligned;
}

void cursive_x3a_x3aruntime_x3a_x3aasync_x3a_x3afree_x5fframe(void* frame) {
  c0_trace_emit_rule("BuiltinSym-Async-FreeFrame");
  if (!frame) {
    return;
  }
  void* base = ((void**)frame)[-1];
  c0_heap_free_raw(base);
}
