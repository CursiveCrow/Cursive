#include "rt_internal.h"

enum {
  C0_REGION_ACTIVE = 0,
  C0_REGION_FROZEN = 1,
  C0_REGION_FREED = 2,
};

static volatile LONG64 g_region_next = 0;

static C0Region c0_region_make(uint8_t disc, uint64_t handle) {
  C0Region region;
  c0_memset(&region, 0, sizeof(C0Region));
  region.disc = disc;
  region.handle = handle;
  return region;
}

static uint64_t c0_region_next_handle(void) {
  return (uint64_t)InterlockedIncrement64(&g_region_next);
}

C0Region cursive_x3a_x3aruntime_x3a_x3aregion_x3a_x3anew_x5fscoped(
    const C0RegionOptions* options) {
  c0_trace_emit_rule("RegionSym-NewScoped");
  (void)options;
  const uint64_t handle = c0_region_next_handle();
  return c0_region_make(C0_REGION_ACTIVE, handle);
}

void cursive_x3a_x3aruntime_x3a_x3aregion_x3a_x3aalloc(
    C0Region self,
    const void* value) {
  c0_trace_emit_rule("RegionSym-Alloc");
  (void)self;
  (void)value;
}

C0Region cursive_x3a_x3aruntime_x3a_x3aregion_x3a_x3areset_x5funchecked(
    C0Region self) {
  c0_trace_emit_rule("RegionSym-ResetUnchecked");
  return c0_region_make(C0_REGION_ACTIVE, self.handle);
}

C0Region cursive_x3a_x3aruntime_x3a_x3aregion_x3a_x3afreeze(C0Region self) {
  c0_trace_emit_rule("RegionSym-Freeze");
  return c0_region_make(C0_REGION_FROZEN, self.handle);
}

C0Region cursive_x3a_x3aruntime_x3a_x3aregion_x3a_x3athaw(C0Region self) {
  c0_trace_emit_rule("RegionSym-Thaw");
  return c0_region_make(C0_REGION_ACTIVE, self.handle);
}

C0Region cursive_x3a_x3aruntime_x3a_x3aregion_x3a_x3afree_x5funchecked(
    C0Region self) {
  c0_trace_emit_rule("RegionSym-FreeUnchecked");
  return c0_region_make(C0_REGION_FREED, self.handle);
}
