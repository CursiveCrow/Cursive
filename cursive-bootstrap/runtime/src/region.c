#include "rt_internal.h"

enum {
  C0_REGION_ACTIVE = 0,
  C0_REGION_FROZEN = 1,
  C0_REGION_FREED = 2,
};

typedef struct C0RegionBlock {
  struct C0RegionBlock* prev;
  void* base;
  uint8_t* data;
  size_t size;
  size_t used;
  size_t align;
} C0RegionBlock;

typedef struct C0RegionTag {
  struct C0RegionTag* prev;
  uint64_t id;
} C0RegionTag;

typedef struct C0RegionMark {
  struct C0RegionMark* prev;
  C0RegionBlock* block;
  C0RegionTag* tag;
  size_t used;
} C0RegionMark;

typedef struct C0RegionArena {
  uint64_t handle;
  uint8_t state;
  size_t prealloc;
  C0RegionBlock* head;
  C0RegionMark* marks;
  C0RegionTag* tags;
} C0RegionArena;

typedef struct C0RegionSlot {
  uint64_t handle;
  C0RegionArena* arena;
} C0RegionSlot;

typedef struct C0AddrTag {
  void* addr;
  uint64_t tag;
} C0AddrTag;

typedef struct C0RegionState {
  C0RegionSlot* slots;
  size_t count;
  size_t cap;
  C0AddrTag* addr_tags;
  size_t addr_count;
  size_t addr_cap;
  uint64_t next_handle;
  uint64_t next_tag;
  SRWLOCK lock;
} C0RegionState;

static C0RegionState g_region_state;
static INIT_ONCE g_region_once = INIT_ONCE_STATIC_INIT;

static BOOL CALLBACK c0_region_init_once(PINIT_ONCE init_once, PVOID param,
                                         PVOID* context) {
  (void)init_once;
  (void)param;
  (void)context;
  c0_memset(&g_region_state, 0, sizeof(g_region_state));
  g_region_state.next_handle = 1;
  g_region_state.next_tag = 1;
  InitializeSRWLock(&g_region_state.lock);
  return TRUE;
}

static C0RegionState* c0_region_state(void) {
  InitOnceExecuteOnce(&g_region_once, c0_region_init_once, NULL, NULL);
  return &g_region_state;
}

static C0Region c0_region_make(uint8_t disc, uint64_t handle) {
  C0Region region;
  c0_memset(&region, 0, sizeof(C0Region));
  region.disc = disc;
  region.handle = handle;
  return region;
}

static bool c0_region_reserve_addr_tags(C0RegionState* state, size_t needed) {
  if (needed <= state->addr_cap) {
    return true;
  }
  size_t new_cap = state->addr_cap ? state->addr_cap : 16;
  while (new_cap < needed) {
    new_cap *= 2;
  }
  C0AddrTag* next = (C0AddrTag*)c0_heap_alloc_raw(new_cap * sizeof(C0AddrTag));
  if (!next) {
    return false;
  }
  if (state->addr_tags && state->addr_count > 0) {
    c0_memcpy(next, state->addr_tags, state->addr_count * sizeof(C0AddrTag));
  }
  c0_heap_free_raw(state->addr_tags);
  state->addr_tags = next;
  state->addr_cap = new_cap;
  return true;
}

static void c0_region_addr_tag_set(C0RegionState* state, void* addr, uint64_t tag) {
  if (!addr) {
    return;
  }
  for (size_t i = 0; i < state->addr_count; ++i) {
    if (state->addr_tags[i].addr == addr) {
      state->addr_tags[i].tag = tag;
      return;
    }
  }
  if (!c0_region_reserve_addr_tags(state, state->addr_count + 1)) {
    return;
  }
  state->addr_tags[state->addr_count].addr = addr;
  state->addr_tags[state->addr_count].tag = tag;
  state->addr_count += 1;
}

static uint64_t c0_region_addr_tag_get(const C0RegionState* state, const void* addr) {
  if (!addr) {
    return 0;
  }
  for (size_t i = 0; i < state->addr_count; ++i) {
    if (state->addr_tags[i].addr == addr) {
      return state->addr_tags[i].tag;
    }
  }
  return 0;
}

static uint64_t c0_region_fresh_tag(C0RegionState* state) {
  uint64_t tag = state->next_tag++;
  if (tag == 0) {
    tag = state->next_tag++;
  }
  return tag;
}

static C0RegionTag* c0_region_tag_push(C0RegionState* state, C0RegionArena* arena) {
  if (!arena) {
    return NULL;
  }
  C0RegionTag* tag = (C0RegionTag*)c0_heap_alloc_raw(sizeof(C0RegionTag));
  if (!tag) {
    return NULL;
  }
  tag->id = c0_region_fresh_tag(state);
  tag->prev = arena->tags;
  arena->tags = tag;
  return tag;
}

static void c0_region_tag_pop_until(C0RegionArena* arena, C0RegionTag* tag) {
  if (!arena || !tag) {
    return;
  }
  while (arena->tags && arena->tags != tag) {
    C0RegionTag* next = arena->tags->prev;
    c0_heap_free_raw(arena->tags);
    arena->tags = next;
  }
  if (arena->tags == tag) {
    C0RegionTag* next = arena->tags->prev;
    c0_heap_free_raw(arena->tags);
    arena->tags = next;
  }
}

static void c0_region_tag_clear(C0RegionArena* arena) {
  if (!arena) {
    return;
  }
  while (arena->tags) {
    C0RegionTag* next = arena->tags->prev;
    c0_heap_free_raw(arena->tags);
    arena->tags = next;
  }
}

static bool c0_region_tag_active(const C0RegionState* state, uint64_t tag) {
  if (tag == 0) {
    return true;
  }
  for (size_t i = 0; i < state->count; ++i) {
    const C0RegionArena* arena = state->slots[i].arena;
    for (const C0RegionTag* it = arena ? arena->tags : NULL; it; it = it->prev) {
      if (it->id == tag) {
        return true;
      }
    }
  }
  return false;
}

static bool c0_region_reserve_slots(C0RegionState* state, size_t needed) {
  if (needed <= state->cap) {
    return true;
  }
  size_t new_cap = state->cap ? state->cap : 8;
  while (new_cap < needed) {
    new_cap *= 2;
  }
  C0RegionSlot* next =
      (C0RegionSlot*)c0_heap_alloc_raw(new_cap * sizeof(C0RegionSlot));
  if (!next) {
    return false;
  }
  if (state->slots && state->count > 0) {
    c0_memcpy(next, state->slots, state->count * sizeof(C0RegionSlot));
  }
  c0_heap_free_raw(state->slots);
  state->slots = next;
  state->cap = new_cap;
  return true;
}

static C0RegionArena* c0_region_find(C0RegionState* state, uint64_t handle) {
  for (size_t i = 0; i < state->count; ++i) {
    if (state->slots[i].handle == handle) {
      return state->slots[i].arena;
    }
  }
  return NULL;
}

static bool c0_region_insert(C0RegionState* state, C0RegionArena* arena) {
  if (!arena) {
    return false;
  }
  if (!c0_region_reserve_slots(state, state->count + 1)) {
    return false;
  }
  state->slots[state->count].handle = arena->handle;
  state->slots[state->count].arena = arena;
  state->count += 1;
  return true;
}

static void c0_region_remove(C0RegionState* state, uint64_t handle) {
  size_t out = 0;
  for (size_t i = 0; i < state->count; ++i) {
    if (state->slots[i].handle == handle) {
      continue;
    }
    if (out != i) {
      state->slots[out] = state->slots[i];
    }
    ++out;
  }
  state->count = out;
}

static C0RegionBlock* c0_region_block_new(size_t size, size_t align) {
  if (align == 0) {
    align = 1;
  }
  const size_t total = sizeof(C0RegionBlock) + size + align;
  uint8_t* raw = (uint8_t*)c0_heap_alloc_raw(total);
  if (!raw) {
    return NULL;
  }
  C0RegionBlock* block = (C0RegionBlock*)raw;
  uint8_t* data_start = raw + sizeof(C0RegionBlock);
  uintptr_t aligned = (uintptr_t)c0_align_up((uint64_t)(uintptr_t)data_start,
                                             (uint64_t)align);
  block->prev = NULL;
  block->base = raw;
  block->data = (uint8_t*)aligned;
  block->size = size;
  block->used = 0;
  block->align = align;
  return block;
}

static void c0_region_block_free(C0RegionBlock* block) {
  if (!block) {
    return;
  }
  c0_heap_free_raw(block->base);
}

static void c0_region_free_blocks(C0RegionArena* arena) {
  C0RegionBlock* block = arena->head;
  while (block) {
    C0RegionBlock* prev = block->prev;
    c0_region_block_free(block);
    block = prev;
  }
  arena->head = NULL;
}

static void c0_region_free_marks(C0RegionArena* arena) {
  C0RegionMark* mark = arena->marks;
  while (mark) {
    C0RegionMark* prev = mark->prev;
    c0_heap_free_raw(mark);
    mark = prev;
  }
  arena->marks = NULL;
}

static void* c0_region_alloc_arena(C0RegionArena* arena, size_t size, size_t align) {
  if (!arena || arena->state != C0_REGION_ACTIVE) {
    return NULL;
  }
  if (align == 0) {
    align = 1;
  }
  C0RegionBlock* block = arena->head;
  if (!block || align > block->align) {
    size_t block_size = size;
    if (arena->prealloc > block_size) {
      block_size = arena->prealloc;
    } else if (block && block->size > block_size) {
      block_size = block->size * 2;
    }
    if (block_size < size) {
      block_size = size;
    }
    C0RegionBlock* next = c0_region_block_new(block_size, align);
    if (!next) {
      return NULL;
    }
    next->prev = arena->head;
    arena->head = next;
    block = next;
  }

  for (;;) {
    size_t used = block->used;
    size_t aligned = (size_t)c0_align_up((uint64_t)used, (uint64_t)align);
    if (aligned > block->size || block->size - aligned < size) {
      size_t block_size = size;
      if (block->size > block_size) {
        block_size = block->size * 2;
      }
      if (block_size < size) {
        block_size = size;
      }
      C0RegionBlock* next = c0_region_block_new(block_size, align);
      if (!next) {
        return NULL;
      }
      next->prev = arena->head;
      arena->head = next;
      block = next;
      continue;
    }
    block->used = aligned + size;
    return block->data + aligned;
  }
}

C0Region cursive_x3a_x3aruntime_x3a_x3aregion_x3a_x3anew_x5fscoped(
    const C0RegionOptions* options) {
  c0_trace_emit_rule("RegionSym-NewScoped");
  C0RegionState* state = c0_region_state();
  C0RegionArena* arena = (C0RegionArena*)c0_heap_alloc_raw(sizeof(C0RegionArena));
  if (!arena) {
    return c0_region_make(C0_REGION_FREED, 0);
  }
  c0_memset(arena, 0, sizeof(C0RegionArena));
  size_t prealloc = 0;
  if (options) {
    prealloc = (size_t)options->stack_size;
  }
  arena->prealloc = prealloc;
  arena->state = C0_REGION_ACTIVE;

  if (prealloc > 0) {
    C0RegionBlock* block = c0_region_block_new(prealloc, 8);
    if (block) {
      arena->head = block;
    }
  }

  AcquireSRWLockExclusive(&state->lock);
  arena->handle = state->next_handle++;
  if (arena->handle == 0) {
    arena->handle = state->next_handle++;
  }
  if (!c0_region_tag_push(state, arena)) {
    ReleaseSRWLockExclusive(&state->lock);
    c0_region_free_blocks(arena);
    c0_heap_free_raw(arena);
    return c0_region_make(C0_REGION_FREED, 0);
  }
  if (!c0_region_insert(state, arena)) {
    ReleaseSRWLockExclusive(&state->lock);
    c0_region_tag_clear(arena);
    c0_region_free_blocks(arena);
    c0_heap_free_raw(arena);
    return c0_region_make(C0_REGION_FREED, 0);
  }
  ReleaseSRWLockExclusive(&state->lock);

  return c0_region_make(C0_REGION_ACTIVE, arena->handle);
}

void* cursive_x3a_x3aruntime_x3a_x3aregion_x3a_x3aalloc(
    C0Region self,
    uint64_t size,
    uint64_t align) {
  c0_trace_emit_rule("RegionSym-Alloc");
  C0RegionState* state = c0_region_state();
  AcquireSRWLockExclusive(&state->lock);
  C0RegionArena* arena = c0_region_find(state, self.handle);
  void* ptr = c0_region_alloc_arena(arena, (size_t)size, (size_t)align);
  if (ptr && arena && arena->tags) {
    c0_region_addr_tag_set(state, ptr, arena->tags->id);
  }
  ReleaseSRWLockExclusive(&state->lock);
  return ptr;
}

uint64_t cursive_x3a_x3aruntime_x3a_x3aregion_x3a_x3amark(C0Region self) {
  c0_trace_emit_rule("RegionSym-Mark");
  C0RegionState* state = c0_region_state();
  AcquireSRWLockExclusive(&state->lock);
  C0RegionArena* arena = c0_region_find(state, self.handle);
  if (!arena || arena->state != C0_REGION_ACTIVE) {
    ReleaseSRWLockExclusive(&state->lock);
    return 0;
  }
  C0RegionMark* mark = (C0RegionMark*)c0_heap_alloc_raw(sizeof(C0RegionMark));
  if (!mark) {
    ReleaseSRWLockExclusive(&state->lock);
    return 0;
  }
  C0RegionTag* tag = c0_region_tag_push(state, arena);
  if (!tag) {
    c0_heap_free_raw(mark);
    ReleaseSRWLockExclusive(&state->lock);
    return 0;
  }
  mark->block = arena->head;
  mark->used = arena->head ? arena->head->used : 0;
  mark->tag = tag;
  mark->prev = arena->marks;
  arena->marks = mark;
  const uint64_t out = (uint64_t)(uintptr_t)mark;
  ReleaseSRWLockExclusive(&state->lock);
  return out;
}

void cursive_x3a_x3aruntime_x3a_x3aregion_x3a_x3areset_x5fto(
    C0Region self,
    uint64_t mark_value) {
  c0_trace_emit_rule("RegionSym-ResetTo");
  if (mark_value == 0) {
    return;
  }
  C0RegionState* state = c0_region_state();
  AcquireSRWLockExclusive(&state->lock);
  C0RegionArena* arena = c0_region_find(state, self.handle);
  if (!arena || arena->state != C0_REGION_ACTIVE) {
    ReleaseSRWLockExclusive(&state->lock);
    return;
  }
  C0RegionMark* mark = (C0RegionMark*)(uintptr_t)mark_value;
  C0RegionMark* it = arena->marks;
  while (it && it != mark) {
    C0RegionMark* prev = it->prev;
    c0_heap_free_raw(it);
    it = prev;
  }
  if (!it) {
    ReleaseSRWLockExclusive(&state->lock);
    return;
  }

  c0_region_tag_pop_until(arena, mark->tag);
  arena->marks = mark->prev;

  if (!mark->block) {
    c0_region_free_blocks(arena);
  } else {
    while (arena->head && arena->head != mark->block) {
      C0RegionBlock* prev = arena->head->prev;
      c0_region_block_free(arena->head);
      arena->head = prev;
    }
    if (arena->head) {
      arena->head->used = mark->used;
    }
  }

  c0_heap_free_raw(mark);
  ReleaseSRWLockExclusive(&state->lock);
}

C0Region cursive_x3a_x3aruntime_x3a_x3aregion_x3a_x3areset_x5funchecked(
    C0Region self) {
  c0_trace_emit_rule("RegionSym-ResetUnchecked");
  C0RegionState* state = c0_region_state();
  AcquireSRWLockExclusive(&state->lock);
  C0RegionArena* arena = c0_region_find(state, self.handle);
  if (arena) {
    c0_region_free_blocks(arena);
    c0_region_free_marks(arena);
    c0_region_tag_clear(arena);
    if (arena->prealloc > 0) {
      C0RegionBlock* block = c0_region_block_new(arena->prealloc, 8);
      if (block) {
        arena->head = block;
      }
    }
    c0_region_tag_push(state, arena);
    arena->state = C0_REGION_ACTIVE;
  }
  ReleaseSRWLockExclusive(&state->lock);
  return c0_region_make(C0_REGION_ACTIVE, self.handle);
}

C0Region cursive_x3a_x3aruntime_x3a_x3aregion_x3a_x3afreeze(C0Region self) {
  c0_trace_emit_rule("RegionSym-Freeze");
  C0RegionState* state = c0_region_state();
  AcquireSRWLockExclusive(&state->lock);
  C0RegionArena* arena = c0_region_find(state, self.handle);
  if (arena) {
    arena->state = C0_REGION_FROZEN;
  }
  ReleaseSRWLockExclusive(&state->lock);
  return c0_region_make(C0_REGION_FROZEN, self.handle);
}

C0Region cursive_x3a_x3aruntime_x3a_x3aregion_x3a_x3athaw(C0Region self) {
  c0_trace_emit_rule("RegionSym-Thaw");
  C0RegionState* state = c0_region_state();
  AcquireSRWLockExclusive(&state->lock);
  C0RegionArena* arena = c0_region_find(state, self.handle);
  if (arena) {
    arena->state = C0_REGION_ACTIVE;
  }
  ReleaseSRWLockExclusive(&state->lock);
  return c0_region_make(C0_REGION_ACTIVE, self.handle);
}

C0Region cursive_x3a_x3aruntime_x3a_x3aregion_x3a_x3afree_x5funchecked(
    C0Region self) {
  c0_trace_emit_rule("RegionSym-FreeUnchecked");
  C0RegionState* state = c0_region_state();
  AcquireSRWLockExclusive(&state->lock);
  C0RegionArena* arena = c0_region_find(state, self.handle);
  if (arena) {
    c0_region_free_blocks(arena);
    c0_region_free_marks(arena);
    c0_region_tag_clear(arena);
    arena->state = C0_REGION_FREED;
    c0_region_remove(state, self.handle);
    c0_heap_free_raw(arena);
  }
  ReleaseSRWLockExclusive(&state->lock);
  return c0_region_make(C0_REGION_FREED, self.handle);
}

uint8_t cursive_x3a_x3aruntime_x3a_x3aregion_x3a_x3aaddr_x5fis_x5factive(
    const void* addr) {
  c0_trace_emit_rule("RegionSym-AddrIsActive");
  C0RegionState* state = c0_region_state();
  AcquireSRWLockExclusive(&state->lock);
  const uint64_t tag = c0_region_addr_tag_get(state, addr);
  const bool ok = (tag == 0) ? true : c0_region_tag_active(state, tag);
  ReleaseSRWLockExclusive(&state->lock);
  return ok ? 1 : 0;
}

void cursive_x3a_x3aruntime_x3a_x3aregion_x3a_x3aaddr_x5ftag_x5ffrom(
    const void* addr,
    const void* base) {
  c0_trace_emit_rule("RegionSym-AddrTagFrom");
  if (!addr || !base) {
    return;
  }
  C0RegionState* state = c0_region_state();
  AcquireSRWLockExclusive(&state->lock);
  const uint64_t tag = c0_region_addr_tag_get(state, base);
  if (tag != 0) {
    c0_region_addr_tag_set(state, (void*)addr, tag);
  }
  ReleaseSRWLockExclusive(&state->lock);
}
