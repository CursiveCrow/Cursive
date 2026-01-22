#include "rt_internal.h"

void cursive_x3a_x3aruntime_x3a_x3aregion_x3a_x3anew_x5fscoped(const C0RegionOptions* options) {
  c0_trace_emit_rule("RegionSym-NewScoped");
  (void)options;
}

void cursive_x3a_x3aruntime_x3a_x3aregion_x3a_x3aalloc(const void* self, const void* value) {
  c0_trace_emit_rule("RegionSym-Alloc");
  (void)self;
  (void)value;
}

void cursive_x3a_x3aruntime_x3a_x3aregion_x3a_x3areset_x5funchecked(const void* self) {
  c0_trace_emit_rule("RegionSym-ResetUnchecked");
  (void)self;
}

void cursive_x3a_x3aruntime_x3a_x3aregion_x3a_x3afreeze(const void* self) {
  c0_trace_emit_rule("RegionSym-Freeze");
  (void)self;
}

void cursive_x3a_x3aruntime_x3a_x3aregion_x3a_x3athaw(const void* self) {
  c0_trace_emit_rule("RegionSym-Thaw");
  (void)self;
}

void cursive_x3a_x3aruntime_x3a_x3aregion_x3a_x3afree_x5funchecked(const void* self) {
  c0_trace_emit_rule("RegionSym-FreeUnchecked");
  (void)self;
}
