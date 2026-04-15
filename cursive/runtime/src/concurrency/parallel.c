// C0X Extension: Structured Concurrency Runtime Support (§18)
// 
// This file provides runtime support for:
// - §18.1 Parallel blocks (fork-join semantics)
// - §18.4 Spawn/wait (task management)
// - §18.5 Dispatch (data parallelism)
// - §18.6 Cancellation
// - §18.7 Panic handling in parallel contexts

#include <stdint.h>
#include <stddef.h>
#include <limits.h>

#include "../internal/rt_internal.h"

// Note: <string.h> is NOT included - we use c0_memset/c0_memcpy from rt_internal.h

// Ensure INT64_MAX/MIN are defined
#ifndef INT64_MAX
#define INT64_MAX 9223372036854775807LL
#endif
#ifndef INT64_MIN
#define INT64_MIN (-9223372036854775807LL - 1)
#endif

// Forward declarations for internal types
typedef struct WorkItem WorkItem;
typedef struct WorkerPool WorkerPool;
typedef struct ParallelContext ParallelContext;
typedef struct SpawnHandle SpawnHandle;
typedef size_t C0CancelId;

// -----------------------------------------------------------------------------
// Panic unwinding (SEH) + TLS state
// -----------------------------------------------------------------------------

#define C0_PANIC_EXCEPTION_CODE 0xE000C0DEu

typedef struct {
    ParallelContext* ctx;
    WorkItem* item;
    int in_panic_scope;
} C0ThreadState;

static INIT_ONCE c0_tls_once = INIT_ONCE_STATIC_INIT;
static DWORD c0_tls_index = TLS_OUT_OF_INDEXES;
static C0ThreadState c0_tls_fallback = {0};

static BOOL CALLBACK c0_tls_init(PINIT_ONCE init_once, PVOID param,
                                 PVOID* context) {
    (void)init_once;
    (void)param;
    (void)context;
    DWORD idx = TlsAlloc();
    if (idx == TLS_OUT_OF_INDEXES) {
        return FALSE;
    }
    c0_tls_index = idx;
    return TRUE;
}

static C0ThreadState* c0_tls_state(void) {
    if (!InitOnceExecuteOnce(&c0_tls_once, c0_tls_init, NULL, NULL)) {
        return &c0_tls_fallback;
    }
    C0ThreadState* state = (C0ThreadState*)TlsGetValue(c0_tls_index);
    if (!state) {
        state = (C0ThreadState*)c0_heap_alloc_raw(sizeof(C0ThreadState));
        if (!state) {
            return &c0_tls_fallback;
        }
        state->ctx = NULL;
        state->item = NULL;
        state->in_panic_scope = 0;
        TlsSetValue(c0_tls_index, state);
    }
    return state;
}

static uint32_t c0_panic_code_from_exception(const EXCEPTION_POINTERS* info) {
    if (!info || !info->ExceptionRecord) {
        return 0;
    }
    if (info->ExceptionRecord->NumberParameters < 1) {
        return 0;
    }
    return (uint32_t)info->ExceptionRecord->ExceptionInformation[0];
}

static ParallelContext* c0_current_ctx(void) {
    return c0_tls_state()->ctx;
}

static WorkItem* c0_current_item(void) {
    return c0_tls_state()->item;
}

static void c0_set_current_ctx(ParallelContext* ctx) {
    c0_tls_state()->ctx = ctx;
}

static void c0_set_current_item(WorkItem* item) {
    c0_tls_state()->item = item;
}

// §18.1.2 Work item state
typedef enum {
    WORK_PENDING,
    WORK_RUNNING,
    WORK_COMPLETED,
    WORK_CANCELLED,
    WORK_PANICKED
} WorkState;

// Work item (created by spawn/dispatch)
struct WorkItem {
    WorkState state;
    void* captured_env;      // Captured environment
    void* hosted_env;        // Hosted session environment
    void (*body)(void* hosted_env, void* env, void* result, void* panic_out);     // Work function
    void* result;            // Result value
    size_t result_size;      // Size of result
    uint64_t affinity_mask;  // CpuSet affinity hint (0 => domain default)
    int32_t priority_hint;   // Priority::Low(0) / Normal(1) / High(2)
    uint32_t panic_code;     // Panic code if panicked
    WorkItem* next;          // Linked list for work queue
    WorkItem* all_next;      // Linked list for cleanup
    HANDLE done_event;       // Signaled on completion
    SpawnHandle* handle;     // Owning handle
};

// §18.4.2 Spawned runtime representation (internal: SpawnHandle)
struct SpawnHandle {
    WorkItem* item;
    int is_ready;
};

enum {
    C0_CANCEL_STATUS_ACTIVE = 0,
    C0_CANCEL_STATUS_CANCELLED = 1
};

#define C0_CANCEL_INVALID_ID ((C0CancelId)SIZE_MAX)

typedef struct {
    C0CancelId parent;
    uint8_t status;
    uint8_t _pad[7];
} C0CancelStateEntry;

typedef struct {
    CRITICAL_SECTION lock;
    C0CancelStateEntry* entries;
    size_t count;
    size_t capacity;
} C0CancelRegistry;

static INIT_ONCE c0_cancel_registry_once = INIT_ONCE_STATIC_INIT;
static C0CancelRegistry c0_cancel_registry = {0};
static int c0_token_is_cancelled(C0CancelId token_id);

typedef struct C0CancelWaitFrame {
    uint64_t resume_state;
    void* resume_fn;
    void* hosted_env;
    C0CancelId token_id;
} C0CancelWaitFrame;

enum {
    C0_ASYNC_DISC_SUSPENDED_LOCAL = 0,
    C0_ASYNC_DISC_COMPLETED_LOCAL = 1,
    C0_ASYNC_PAYLOAD_FRAME_PTR_OFFSET_LOCAL = 8,
};

static void c0_cancel_wait_write_completed(C0AsyncResumeValue* out) {
    if (!out) {
        return;
    }
    c0_memset(out, 0, sizeof(*out));
    out->disc = C0_ASYNC_DISC_COMPLETED_LOCAL;
}

static void c0_cancel_wait_write_suspended(C0AsyncResumeValue* out,
                                           C0CancelWaitFrame* frame) {
    if (!out || !frame) {
        return;
    }
    c0_memset(out, 0, sizeof(*out));
    out->disc = C0_ASYNC_DISC_SUSPENDED_LOCAL;
    void* frame_ptr = frame;
    c0_memcpy(out->payload + C0_ASYNC_PAYLOAD_FRAME_PTR_OFFSET_LOCAL,
              &frame_ptr,
              sizeof(frame_ptr));
}

static void c0_cancel_wait_resume(void* hosted_env,
                                  C0AsyncResumeValue* out,
                                  void* frame_ptr,
                                  void* input,
                                  void* panic_out) {
    (void)hosted_env;
    (void)input;
    (void)panic_out;
    C0CancelWaitFrame* frame = (C0CancelWaitFrame*)frame_ptr;
    if (!frame || frame->token_id == C0_CANCEL_INVALID_ID) {
        c0_cancel_wait_write_completed(out);
        if (frame) {
            c0_heap_free_raw(frame);
        }
        return;
    }
    if (c0_token_is_cancelled(frame->token_id)) {
        c0_cancel_wait_write_completed(out);
        c0_heap_free_raw(frame);
        return;
    }
    c0_cancel_wait_write_suspended(out, frame);
}

typedef struct {
    uint8_t panic;
    uint8_t _pad[3];
    uint32_t code;
} C0PanicRecord;

// Worker pool for parallel execution
struct WorkerPool {
    int num_workers;
    int active_workers;
    WorkItem* queue_head;
    WorkItem* queue_tail;
    HANDLE* threads;
    CRITICAL_SECTION lock;
    CONDITION_VARIABLE work_cv;
    CONDITION_VARIABLE done_cv;
    volatile int shutdown;
    size_t pending_count;
    C0CancelId cancel_token;
    ParallelContext* ctx;
};

// §18.1 Parallel context
struct ParallelContext {
    WorkerPool* pool;
    C0CancelId cancel_token;
    WorkItem* first_panic;    // First panicked work item
    int panic_count;          // Number of panics
    const char* name;         // Debug name
    WorkItem* all_items;      // All work items for cleanup
    ParallelContext* prev_ctx;
    int inline_domain;
};

static BOOL CALLBACK c0_cancel_registry_init(PINIT_ONCE init_once,
                                             PVOID param,
                                             PVOID* context) {
    (void)init_once;
    (void)param;
    (void)context;
    InitializeCriticalSection(&c0_cancel_registry.lock);
    c0_cancel_registry.entries = NULL;
    c0_cancel_registry.count = 0;
    c0_cancel_registry.capacity = 0;
    return TRUE;
}

static int c0_cancel_registry_ready(void) {
    return InitOnceExecuteOnce(&c0_cancel_registry_once,
                               c0_cancel_registry_init,
                               NULL,
                               NULL)
               ? 1
               : 0;
}

static int c0_cancel_registry_valid_id_locked(C0CancelId id) {
    return id != C0_CANCEL_INVALID_ID && id < c0_cancel_registry.count;
}

static int c0_cancel_registry_reserve_locked(size_t needed) {
    if (needed <= c0_cancel_registry.capacity) {
        return 1;
    }

    size_t new_capacity = c0_cancel_registry.capacity ? c0_cancel_registry.capacity : 16u;
    while (new_capacity < needed) {
        if (new_capacity > (SIZE_MAX / 2u)) {
            new_capacity = needed;
            break;
        }
        new_capacity *= 2u;
    }

    const size_t bytes = new_capacity * sizeof(C0CancelStateEntry);
    C0CancelStateEntry* new_entries =
        (C0CancelStateEntry*)c0_heap_alloc_raw(bytes);
    if (!new_entries) {
        return 0;
    }

    c0_memset(new_entries, 0, bytes);
    if (c0_cancel_registry.entries && c0_cancel_registry.count > 0) {
        c0_memcpy(new_entries,
                  c0_cancel_registry.entries,
                  c0_cancel_registry.count * sizeof(C0CancelStateEntry));
        c0_heap_free_raw(c0_cancel_registry.entries);
    }

    c0_cancel_registry.entries = new_entries;
    c0_cancel_registry.capacity = new_capacity;
    return 1;
}

static C0CancelId c0_cancel_registry_new_locked(C0CancelId parent) {
    const size_t next = c0_cancel_registry.count;
    if (!c0_cancel_registry_reserve_locked(next + 1u)) {
        return C0_CANCEL_INVALID_ID;
    }

    c0_cancel_registry.entries[next].parent = parent;
    c0_cancel_registry.entries[next].status = C0_CANCEL_STATUS_ACTIVE;
    c0_cancel_registry.count = next + 1u;
    return (C0CancelId)next;
}

static int c0_cancel_registry_descendant_locked(C0CancelId root,
                                                C0CancelId candidate) {
    if (!c0_cancel_registry_valid_id_locked(root) ||
        !c0_cancel_registry_valid_id_locked(candidate)) {
        return 0;
    }

    C0CancelId current = candidate;
    for (;;) {
        if (current == root) {
            return 1;
        }
        if (!c0_cancel_registry_valid_id_locked(current)) {
            return 0;
        }
        const C0CancelId parent = c0_cancel_registry.entries[current].parent;
        if (parent == C0_CANCEL_INVALID_ID) {
            return 0;
        }
        current = parent;
    }
}

static void c0_cancel_registry_cancel_locked(C0CancelId id) {
    if (!c0_cancel_registry_valid_id_locked(id)) {
        return;
    }

    for (size_t i = 0; i < c0_cancel_registry.count; ++i) {
        if (c0_cancel_registry_descendant_locked(id, (C0CancelId)i)) {
            c0_cancel_registry.entries[i].status = C0_CANCEL_STATUS_CANCELLED;
        }
    }
}

// Thread-local parallel context tracking (for nested parallel support)

static int c0_token_is_cancelled(C0CancelId token_id) {
    int cancelled = 0;
    if (!c0_cancel_registry_ready()) {
        return 0;
    }

    EnterCriticalSection(&c0_cancel_registry.lock);
    if (c0_cancel_registry_valid_id_locked(token_id)) {
        cancelled =
            c0_cancel_registry.entries[token_id].status == C0_CANCEL_STATUS_CANCELLED;
    }
    LeaveCriticalSection(&c0_cancel_registry.lock);
    return cancelled;
}

static uint32_t c0_u64_to_dec(uint64_t value, char* out) {
    char rev[32];
    uint32_t count = 0;
    do {
        rev[count++] = (char)('0' + (value % 10));
        value /= 10;
    } while (value != 0 && count < (uint32_t)sizeof(rev));
    for (uint32_t i = 0; i < count; ++i) {
        out[i] = rev[count - 1 - i];
    }
    return count;
}

static int32_t c0_priority_rank(int32_t priority_hint) {
    if (priority_hint <= 0) {
        return 0;
    }
    if (priority_hint == 1) {
        return 1;
    }
    return 2;
}

static int c0_thread_priority_from_rank(int32_t rank) {
    switch (rank) {
        case 0:
            return THREAD_PRIORITY_BELOW_NORMAL;
        case 2:
            return THREAD_PRIORITY_ABOVE_NORMAL;
        case 1:
        default:
            return THREAD_PRIORITY_NORMAL;
    }
}

typedef struct {
    DWORD_PTR prev_affinity;
    int prev_priority;
    int affinity_changed;
    int priority_changed;
} C0WorkHintScope;

static void c0_apply_work_hints(const WorkItem* item, C0WorkHintScope* scope) {
    if (!scope) {
        return;
    }

    scope->prev_affinity = 0;
    scope->prev_priority = THREAD_PRIORITY_NORMAL;
    scope->affinity_changed = 0;
    scope->priority_changed = 0;

    if (!item) {
        return;
    }

    if (item->affinity_mask != 0) {
        DWORD_PTR mask = (DWORD_PTR)item->affinity_mask;
        if (mask != 0) {
            scope->prev_affinity = SetThreadAffinityMask(GetCurrentThread(), mask);
            if (scope->prev_affinity != 0) {
                scope->affinity_changed = 1;
            }
        }
    }

    const int desired_priority =
        c0_thread_priority_from_rank(c0_priority_rank(item->priority_hint));
    if (desired_priority != THREAD_PRIORITY_NORMAL) {
        int prev = GetThreadPriority(GetCurrentThread());
        if (prev != THREAD_PRIORITY_ERROR_RETURN &&
            SetThreadPriority(GetCurrentThread(), desired_priority)) {
            scope->prev_priority = prev;
            scope->priority_changed = 1;
        }
    }
}

static void c0_restore_work_hints(const C0WorkHintScope* scope) {
    if (!scope) {
        return;
    }
    if (scope->priority_changed) {
        SetThreadPriority(GetCurrentThread(), scope->prev_priority);
    }
    if (scope->affinity_changed && scope->prev_affinity != 0) {
        SetThreadAffinityMask(GetCurrentThread(), scope->prev_affinity);
    }
}

static uint32_t c0_copy_cstr(char* out, const char* text) {
    uint32_t count = 0;
    if (!out || !text) {
        return 0;
    }
    while (text[count] != 0) {
        out[count] = text[count];
        ++count;
    }
    return count;
}

static int c0_debug_flag_enabled(const char* name) {
    if (!name || name[0] == '\0') {
        return 0;
    }
    char probe[2];
    DWORD len = GetEnvironmentVariableA(name, probe, (DWORD)sizeof(probe));
    return len > 0;
}

static void c0_debug_write_spawn_result(const WorkItem* item) {
    if (!item || !item->result || item->result_size == 0) {
        return;
    }
    HANDLE h = GetStdHandle(STD_ERROR_HANDLE);
    if (h == NULL || h == INVALID_HANDLE_VALUE) {
        return;
    }

    uint32_t first_u32 = 0;
    if (item->result_size >= 4) {
        first_u32 = *(const uint32_t*)item->result;
    }

    char dbg_result[128];
    uint32_t pos = 0;
    pos += c0_copy_cstr(dbg_result + pos, "[SPAWN-RESULT size=");
    pos += c0_u64_to_dec((uint64_t)item->result_size, dbg_result + pos);
    pos += c0_copy_cstr(dbg_result + pos, " first_u32=");
    pos += c0_u64_to_dec((uint64_t)first_u32, dbg_result + pos);
    dbg_result[pos++] = ']';
    dbg_result[pos++] = '\n';

    DWORD written = 0;
    WriteFile(h, dbg_result, (DWORD)pos, &written, NULL);
}

static void c0_debug_write_wait_result(const WorkItem* item) {
    if (!item || !item->result || item->result_size == 0) {
        return;
    }
    HANDLE h = GetStdHandle(STD_ERROR_HANDLE);
    if (h == NULL || h == INVALID_HANDLE_VALUE) {
        return;
    }

    uint32_t first_u32 = 0;
    if (item->result_size >= 4) {
        first_u32 = *(const uint32_t*)item->result;
    }

    char dbg_result[128];
    uint32_t pos = 0;
    pos += c0_copy_cstr(dbg_result + pos, "[WAIT-RESULT size=");
    pos += c0_u64_to_dec((uint64_t)item->result_size, dbg_result + pos);
    pos += c0_copy_cstr(dbg_result + pos, " first_u32=");
    pos += c0_u64_to_dec((uint64_t)first_u32, dbg_result + pos);
    dbg_result[pos++] = ']';
    dbg_result[pos++] = '\n';

    DWORD written = 0;
    WriteFile(h, dbg_result, (DWORD)pos, &written, NULL);
}

static void c0_debug_write_dispatch_range(C0Range range,
                                          uint64_t start,
                                          uint64_t end,
                                          size_t elem_size,
                                          size_t result_size,
                                          int ordered,
                                          size_t chunk_size) {
    if (!c0_debug_flag_enabled("CURSIVE_DEBUG_DISPATCH_RANGE_RUNTIME")) {
        return;
    }
    HANDLE h = GetStdHandle(STD_ERROR_HANDLE);
    if (h == NULL || h == INVALID_HANDLE_VALUE) {
        return;
    }

    char dbg[256];
    uint32_t pos = 0;
    pos += c0_copy_cstr(dbg + pos, "[DISPATCH tag=");
    pos += c0_u64_to_dec((uint64_t)range.tag, dbg + pos);
    pos += c0_copy_cstr(dbg + pos, " lo=");
    pos += c0_u64_to_dec(range.lo, dbg + pos);
    pos += c0_copy_cstr(dbg + pos, " hi=");
    pos += c0_u64_to_dec(range.hi, dbg + pos);
    pos += c0_copy_cstr(dbg + pos, " start=");
    pos += c0_u64_to_dec(start, dbg + pos);
    pos += c0_copy_cstr(dbg + pos, " end=");
    pos += c0_u64_to_dec(end, dbg + pos);
    pos += c0_copy_cstr(dbg + pos, " elem_size=");
    pos += c0_u64_to_dec((uint64_t)elem_size, dbg + pos);
    pos += c0_copy_cstr(dbg + pos, " result_size=");
    pos += c0_u64_to_dec((uint64_t)result_size, dbg + pos);
    pos += c0_copy_cstr(dbg + pos, " ordered=");
    pos += c0_u64_to_dec((uint64_t)(ordered ? 1 : 0), dbg + pos);
    pos += c0_copy_cstr(dbg + pos, " chunk=");
    pos += c0_u64_to_dec((uint64_t)chunk_size, dbg + pos);
    dbg[pos++] = ']';
    dbg[pos++] = '\n';

    DWORD written = 0;
    WriteFile(h, dbg, (DWORD)pos, &written, NULL);
}

static void c0_debug_write_dispatch_chunk_value(const char* label,
                                                uint64_t start,
                                                uint64_t end,
                                                const void* result_ptr,
                                                size_t result_size) {
    if (!label || !result_ptr || result_size == 0) {
        return;
    }
    HANDLE h = GetStdHandle(STD_ERROR_HANDLE);
    if (h == NULL || h == INVALID_HANDLE_VALUE) {
        return;
    }
    uint32_t first_u32 = 0;
    if (result_size >= 4) {
        first_u32 = *(const uint32_t*)result_ptr;
    }
    char dbg[192];
    uint32_t pos = 0;
    pos += c0_copy_cstr(dbg + pos, "[DISPATCH-CHUNK ");
    pos += c0_copy_cstr(dbg + pos, label);
    pos += c0_copy_cstr(dbg + pos, " start=");
    pos += c0_u64_to_dec(start, dbg + pos);
    pos += c0_copy_cstr(dbg + pos, " end=");
    pos += c0_u64_to_dec(end, dbg + pos);
    pos += c0_copy_cstr(dbg + pos, " size=");
    pos += c0_u64_to_dec((uint64_t)result_size, dbg + pos);
    pos += c0_copy_cstr(dbg + pos, " first_u32=");
    pos += c0_u64_to_dec((uint64_t)first_u32, dbg + pos);
    dbg[pos++] = ']';
    dbg[pos++] = '\n';
    DWORD written = 0;
    WriteFile(h, dbg, (DWORD)pos, &written, NULL);
}

static void c0_debug_write_cancel_state(const char* stage,
                                        const ParallelContext* ctx,
                                        const WorkItem* item,
                                        C0CancelId token_id,
                                        int cancelled) {
    if (!c0_debug_flag_enabled("CURSIVE_DEBUG_PARALLEL_RUNTIME")) {
        return;
    }
    HANDLE h = GetStdHandle(STD_ERROR_HANDLE);
    if (h == NULL || h == INVALID_HANDLE_VALUE) {
        return;
    }
    if (!stage) {
        stage = "unknown";
    }

    char dbg[256];
    uint32_t pos = 0;
    pos += c0_copy_cstr(dbg + pos, "[CANCEL stage=");
    pos += c0_copy_cstr(dbg + pos, stage);
    pos += c0_copy_cstr(dbg + pos, " ctx=");
    pos += c0_u64_to_dec((uint64_t)(uintptr_t)ctx, dbg + pos);
    pos += c0_copy_cstr(dbg + pos, " item=");
    pos += c0_u64_to_dec((uint64_t)(uintptr_t)item, dbg + pos);
    pos += c0_copy_cstr(dbg + pos, " token=");
    if (token_id == C0_CANCEL_INVALID_ID) {
        pos += c0_copy_cstr(dbg + pos, "none");
    } else {
        pos += c0_u64_to_dec((uint64_t)token_id, dbg + pos);
    }
    pos += c0_copy_cstr(dbg + pos, " cancelled=");
    pos += c0_u64_to_dec((uint64_t)(cancelled ? 1 : 0), dbg + pos);
    dbg[pos++] = ']';
    dbg[pos++] = '\n';
    DWORD written = 0;
    WriteFile(h, dbg, (DWORD)pos, &written, NULL);
}

static void c0_run_item(ParallelContext* ctx, WorkItem* item) {
    if (!item) {
        return;
    }
    C0ThreadState* state = c0_tls_state();
    ParallelContext* prev_ctx = state->ctx;
    WorkItem* prev_item = state->item;
    int prev_scope = state->in_panic_scope;
    state->ctx = ctx;
    state->item = item;
    state->in_panic_scope = 1;

    C0PanicRecord panic_record;
    panic_record.panic = 0;
    panic_record.code = 0;

    __try {
        const int has_cancel_token =
            ctx && ctx->cancel_token != C0_CANCEL_INVALID_ID;
        const int is_cancelled =
            has_cancel_token ? c0_token_is_cancelled(ctx->cancel_token) : 0;
        if (has_cancel_token && is_cancelled) {
            c0_debug_write_cancel_state("run_item-cancel", ctx, item, ctx->cancel_token, 1);
            item->state = WORK_CANCELLED;
            if (item->result && item->result_size > 0) {
                c0_memset(item->result, 0, item->result_size);
            }
        } else {
            if (ctx) {
                c0_debug_write_cancel_state("run_item-start", ctx, item, ctx->cancel_token, is_cancelled);
            } else {
                c0_debug_write_cancel_state("run_item-start",
                                            ctx,
                                            item,
                                            C0_CANCEL_INVALID_ID,
                                            0);
            }
            item->state = WORK_RUNNING;
            if (item->body) {
                item->body(item->hosted_env, item->captured_env, item->result, &panic_record);
                if (item->result && item->result_size > 0 &&
                    c0_debug_flag_enabled("CURSIVE_DEBUG_SPAWN_RESULT_RUNTIME")) {
                    c0_debug_write_spawn_result(item);
                }
            }
            if (panic_record.panic) {
                cursive_parallel_work_panic(ctx, panic_record.code);
            }
            if (item->state == WORK_RUNNING) {
                item->state = WORK_COMPLETED;
            }
        }
    } __except (GetExceptionCode() == C0_PANIC_EXCEPTION_CODE
                    ? EXCEPTION_EXECUTE_HANDLER
                    : EXCEPTION_CONTINUE_SEARCH) {
        uint32_t code = c0_panic_code_from_exception(GetExceptionInformation());
        cursive_parallel_work_panic(ctx, code);
    }
    
    state->in_panic_scope = prev_scope;
    state->item = prev_item;
    state->ctx = prev_ctx;
}

static void c0_enqueue_item(WorkerPool* pool, WorkItem* item) {
    if (!pool || !item) {
        return;
    }
    EnterCriticalSection(&pool->lock);
    if (pool->queue_tail) {
        pool->queue_tail->next = item;
        pool->queue_tail = item;
    } else {
        pool->queue_head = item;
        pool->queue_tail = item;
    }
    pool->pending_count += 1;
    WakeConditionVariable(&pool->work_cv);
    LeaveCriticalSection(&pool->lock);
}

static WorkItem* c0_dequeue_item_locked(WorkerPool* pool) {
    if (!pool || !pool->queue_head) {
        return NULL;
    }

    WorkItem* best = pool->queue_head;
    WorkItem* best_prev = NULL;
    WorkItem* prev = pool->queue_head;
    WorkItem* cur = pool->queue_head->next;

    while (cur) {
        if (cur->priority_hint > best->priority_hint) {
            best = cur;
            best_prev = prev;
        }
        prev = cur;
        cur = cur->next;
    }

    if (best_prev) {
        best_prev->next = best->next;
    } else {
        pool->queue_head = best->next;
    }
    if (pool->queue_tail == best) {
        pool->queue_tail = best_prev;
    }
    best->next = NULL;
    return best;
}

int c0_parallel_in_panic_scope(void) {
    return c0_tls_state()->in_panic_scope != 0;
}

void c0_parallel_raise_panic(uint32_t code) {
    if (!c0_parallel_in_panic_scope()) {
        cursive_panic(code);
        return;
    }
    ULONG_PTR args[1];
    args[0] = (ULONG_PTR)code;
    RaiseException(C0_PANIC_EXCEPTION_CODE, 0, 1, args);
}

static DWORD WINAPI c0_worker_thread_proc(LPVOID param) {
    WorkerPool* pool = (WorkerPool*)param;
    for (;;) {
        EnterCriticalSection(&pool->lock);
        while (!pool->shutdown && pool->queue_head == NULL) {
            SleepConditionVariableCS(&pool->work_cv, &pool->lock, INFINITE);
        }
        if (pool->shutdown) {
            LeaveCriticalSection(&pool->lock);
            return 0;
        }

        WorkItem* item = c0_dequeue_item_locked(pool);
        pool->active_workers += 1;
        LeaveCriticalSection(&pool->lock);

        if (item) {
            C0WorkHintScope hints;
            c0_apply_work_hints(item, &hints);
            c0_run_item(pool->ctx, item);
            c0_restore_work_hints(&hints);
            SetEvent(item->done_event);
        }

        EnterCriticalSection(&pool->lock);
        pool->active_workers -= 1;
        if (pool->pending_count > 0) {
            pool->pending_count -= 1;
        }
        if (pool->pending_count == 0) {
            WakeAllConditionVariable(&pool->done_cv);
        }
        LeaveCriticalSection(&pool->lock);
    }
}

// §18.1.1 Begin parallel block
// runtime_parallel_begin(domain) -> ParallelContext*
void* cursive_parallel_begin(C0DynObject domain,
                             C0CancelId cancel_token,
                             const char* name) {
    const C0ExecutionDomain* dom = (const C0ExecutionDomain*)domain.data;
    const int inline_domain = dom && dom->kind == C0_DOMAIN_INLINE;
    if (c0_debug_flag_enabled("CURSIVE_DEBUG_PARALLEL_RUNTIME")) {
        char dbg[256];
        uint32_t pos = 0;
        pos += c0_copy_cstr(dbg + pos, "[PAR-BEGIN domain_data=");
        pos += c0_u64_to_dec((uint64_t)(uintptr_t)domain.data, dbg + pos);
        pos += c0_copy_cstr(dbg + pos, " domain_vtable=");
        pos += c0_u64_to_dec((uint64_t)(uintptr_t)domain.vtable, dbg + pos);
        pos += c0_copy_cstr(dbg + pos, " cancel=");
        if (cancel_token == C0_CANCEL_INVALID_ID) {
            pos += c0_copy_cstr(dbg + pos, "none");
        } else {
            pos += c0_u64_to_dec((uint64_t)cancel_token, dbg + pos);
        }
        pos += c0_copy_cstr(dbg + pos, " inline=");
        pos += c0_u64_to_dec((uint64_t)(inline_domain ? 1 : 0), dbg + pos);
        if (dom) {
            pos += c0_copy_cstr(dbg + pos, " kind=");
            pos += c0_u64_to_dec((uint64_t)dom->kind, dbg + pos);
            pos += c0_copy_cstr(dbg + pos, " max=");
            pos += c0_u64_to_dec((uint64_t)dom->max_concurrency, dbg + pos);
        } else {
            pos += c0_copy_cstr(dbg + pos, " kind=null");
        }
        dbg[pos++] = ']';
        dbg[pos++] = '\n';
        WriteFile(GetStdHandle(STD_ERROR_HANDLE), dbg, (DWORD)pos, NULL, NULL);
    }

    ParallelContext* ctx =
        (ParallelContext*)c0_heap_alloc_raw(sizeof(ParallelContext));
    if (!ctx) return NULL;

    ctx->pool = NULL;
    if (!inline_domain) {
        ctx->pool = (WorkerPool*)c0_heap_alloc_raw(sizeof(WorkerPool));
        if (!ctx->pool) {
            c0_heap_free_raw(ctx);
            return NULL;
        }
        int workers = dom && dom->max_concurrency > 0 ? (int)dom->max_concurrency : 4;
        if (workers < 1) {
            workers = 1;
        }
        ctx->pool->num_workers = workers;
        ctx->pool->active_workers = 0;
        ctx->pool->queue_head = NULL;
        ctx->pool->queue_tail = NULL;
        ctx->pool->threads = NULL;
        ctx->pool->shutdown = 0;
        ctx->pool->pending_count = 0;
        ctx->pool->cancel_token = cancel_token;
        ctx->pool->ctx = ctx;
        InitializeCriticalSection(&ctx->pool->lock);
        InitializeConditionVariable(&ctx->pool->work_cv);
        InitializeConditionVariable(&ctx->pool->done_cv);

        // Start worker threads
        ctx->pool->threads =
            (HANDLE*)c0_heap_alloc_raw(sizeof(HANDLE) * ctx->pool->num_workers);
        if (ctx->pool->threads) {
            for (int i = 0; i < ctx->pool->num_workers; ++i) {
                ctx->pool->threads[i] = CreateThread(NULL, 0, c0_worker_thread_proc, ctx->pool, 0, NULL);
            }
        }
    }

    ctx->cancel_token = cancel_token;
    ctx->first_panic = NULL;
    ctx->panic_count = 0;
    ctx->name = name;
    ctx->all_items = NULL;
    ctx->prev_ctx = c0_current_ctx();
    ctx->inline_domain = inline_domain;

    c0_set_current_ctx(ctx);

    return ctx;
}

// §18.1.2 Join parallel block
// Waits for all work to complete and propagates first panic
int cursive_parallel_join(void* ctx_ptr) {
    if (c0_debug_flag_enabled("CURSIVE_DEBUG_PARALLEL_RUNTIME")) {
        const char* dbg1 = "[JOIN] enter\n";
        WriteFile(GetStdHandle(STD_ERROR_HANDLE), dbg1, 13, NULL, NULL);
    }
    
    ParallelContext* ctx = (ParallelContext*)ctx_ptr;
    if (!ctx) {
        if (c0_debug_flag_enabled("CURSIVE_DEBUG_PARALLEL_RUNTIME")) {
            const char* dbg2 = "[JOIN] null ctx\n";
            WriteFile(GetStdHandle(STD_ERROR_HANDLE), dbg2, 15, NULL, NULL);
        }
        return 0;
    }
    
    if (ctx->pool) {
        // §18.7.1 Wait for all work to complete
        EnterCriticalSection(&ctx->pool->lock);
        while (ctx->pool->pending_count > 0) {
            SleepConditionVariableCS(&ctx->pool->done_cv, &ctx->pool->lock, INFINITE);
        }
        ctx->pool->shutdown = 1;
        WakeAllConditionVariable(&ctx->pool->work_cv);
        LeaveCriticalSection(&ctx->pool->lock);

        if (ctx->pool->threads) {
            for (int i = 0; i < ctx->pool->num_workers; ++i) {
                if (ctx->pool->threads[i]) {
                    WaitForSingleObject(ctx->pool->threads[i], INFINITE);
                    CloseHandle(ctx->pool->threads[i]);
                }
            }
            c0_heap_free_raw(ctx->pool->threads);
            ctx->pool->threads = NULL;
        }
    }
    
    // §18.7.2 Check for panics
    int had_panic = (ctx->first_panic != NULL);
    
    // Cleanup
    WorkItem* item = ctx->all_items;
    while (item) {
        WorkItem* next = item->all_next;
        if (item->captured_env) {
            c0_heap_free_raw(item->captured_env);
        }
        if (item->result) {
            c0_heap_free_raw(item->result);
        }
        if (item->done_event) {
            CloseHandle(item->done_event);
        }
        if (item->handle) {
            c0_heap_free_raw(item->handle);
        }
        c0_heap_free_raw(item);
        item = next;
    }

    if (ctx->pool) {
        DeleteCriticalSection(&ctx->pool->lock);
        c0_heap_free_raw(ctx->pool);
    }
    c0_set_current_ctx(ctx->prev_ctx);
    c0_heap_free_raw(ctx);
    
    // §18.7.1 Report panic-at-boundary to caller.
    // The caller is responsible for re-emitting panic in the active boundary
    // mechanism (panic record / catch-zero at FFI boundary).
    if (had_panic && c0_debug_flag_enabled("CURSIVE_DEBUG_PARALLEL_RUNTIME")) {
        const char* dbg = "[JOIN] propagating panic\n";
        WriteFile(GetStdHandle(STD_ERROR_HANDLE), dbg, 24, NULL, NULL);
    }
    
    if (c0_debug_flag_enabled("CURSIVE_DEBUG_PARALLEL_RUNTIME")) {
        const char* dbg_exit = "[JOIN] done\n";
        WriteFile(GetStdHandle(STD_ERROR_HANDLE), dbg_exit, 12, NULL, NULL);
    }
    return had_panic ? 1 : 0;
}

// §18.4.2 Create spawn handle
// Returns Spawned<T>@Pending (or @Ready if body is NULL for inline execution)
void* cursive_spawn_create(void* env, size_t env_size,
                            void (*body)(void* hosted_env, void* env, void* result, void* panic_out),
                            void* hosted_env,
                            size_t result_size,
                            uint64_t affinity_mask,
                            int32_t priority_hint) {
    if (c0_debug_flag_enabled("CURSIVE_DEBUG_PARALLEL_RUNTIME")) {
        static int spawn_count = 0;
        spawn_count++;
        char dbg[32];
        dbg[0] = '['; dbg[1] = 'S'; dbg[2] = 'P'; dbg[3] = 'A'; dbg[4] = 'W'; dbg[5] = 'N';
        dbg[6] = ' '; dbg[7] = '#'; dbg[8] = '0' + (spawn_count % 10); dbg[9] = ']'; dbg[10] = '\n';
        WriteFile(GetStdHandle(STD_ERROR_HANDLE), dbg, 11, NULL, NULL);
    }
    SpawnHandle* handle =
        (SpawnHandle*)c0_heap_alloc_raw(sizeof(SpawnHandle));
    if (!handle) return NULL;
    
    WorkItem* item = (WorkItem*)c0_heap_alloc_raw(sizeof(WorkItem));
    if (!item) {
        c0_heap_free_raw(handle);
        return NULL;
    }
    
    // Copy captured environment
    item->captured_env = NULL;
    if (env && env_size > 0) {
        item->captured_env = c0_heap_alloc_raw(env_size);
        if (item->captured_env) {
            c0_memcpy(item->captured_env, env, env_size);
        }
    }

    item->done_event = CreateEvent(NULL, TRUE, FALSE, NULL);

    item->state = WORK_PENDING;
    item->hosted_env = hosted_env;
    item->body = body;
    item->result = result_size > 0 ? c0_heap_alloc_raw(result_size) : NULL;
    item->result_size = result_size;
    item->affinity_mask = affinity_mask;
    item->priority_hint = c0_priority_rank(priority_hint);
    item->panic_code = 0;
    item->next = NULL;
    item->all_next = NULL;
    handle->is_ready = 0;

    if (body == NULL) {
        item->state = WORK_COMPLETED;
        handle->is_ready = 1;
        if (item->done_event) {
            SetEvent(item->done_event);
        }
    } else if (c0_current_ctx() && c0_current_ctx()->pool) {
        c0_enqueue_item(c0_current_ctx()->pool, item);
    } else {
        // No active parallel context or inline domain: execute inline
        C0WorkHintScope hints;
        c0_apply_work_hints(item, &hints);
        c0_run_item(c0_current_ctx(), item);
        c0_restore_work_hints(&hints);
        handle->is_ready = 1;
        if (item->done_event) {
            SetEvent(item->done_event);
        }
    }
    item->handle = handle;
    
    handle->item = item;
    if (c0_current_ctx()) {
        item->all_next = c0_current_ctx()->all_items;
        c0_current_ctx()->all_items = item;
    }
    
    return handle;
}

// §10.3 Wait for spawn result
// Blocks until handle is ready, returns extracted value
void* cursive_spawn_wait(void* handle_ptr) {
    SpawnHandle* handle = (SpawnHandle*)handle_ptr;
    if (!handle || !handle->item) {
        if (c0_debug_flag_enabled("CURSIVE_DEBUG_PARALLEL_RUNTIME")) {
            const char* dbg = "[WAIT] null handle\n";
            WriteFile(GetStdHandle(STD_ERROR_HANDLE), dbg, 18, NULL, NULL);
        }
        return NULL;
    }
    
    WorkItem* item = handle->item;
    
    if (item->done_event) {
        WaitForSingleObject(item->done_event, INFINITE);
    } else if (item->state == WORK_PENDING) {
        // Fallback: execute inline if event is missing
        c0_run_item(c0_current_ctx(), item);
    }
    
    handle->is_ready = 1;
    
    if (c0_debug_flag_enabled("CURSIVE_DEBUG_PARALLEL_RUNTIME")) {
        char dbg[32];
        dbg[0] = '['; dbg[1] = 'W'; dbg[2] = 'A'; dbg[3] = 'I'; dbg[4] = 'T';
        dbg[5] = ' '; dbg[6] = 's'; dbg[7] = 't'; dbg[8] = '=';
        dbg[9] = '0' + item->state; dbg[10] = ']'; dbg[11] = '\n';
        WriteFile(GetStdHandle(STD_ERROR_HANDLE), dbg, 12, NULL, NULL);
    }
    
    // §18.7.1 Panic is propagated at the enclosing parallel boundary after
    // all started work settles. wait returns the current value slot and does
    // not directly abort/rethrow.
    if (item->state == WORK_PANICKED) {
        if (c0_debug_flag_enabled("CURSIVE_DEBUG_PARALLEL_RUNTIME")) {
            const char* dbg2 = "[WAIT] propagating panic\n";
            WriteFile(GetStdHandle(STD_ERROR_HANDLE), dbg2, 24, NULL, NULL);
        }
        if (item->result && item->result_size > 0) {
            c0_memset(item->result, 0, item->result_size);
        }
    }

    if (c0_debug_flag_enabled("CURSIVE_DEBUG_SPAWN_RESULT_RUNTIME")) {
        c0_debug_write_wait_result(item);
    }
    
    return item->result;
}

// Reactor::register runtime hook.
// ABI is type-erased at this boundary: both reactor and future are passed by
// pointer, and the returned tracked handle is the same opaque spawn handle used
// by wait. For Cursive0 exercises, Future<T,E> values here are immediate and we
// materialize a ready handle carrying T|E in a compact tagged payload.
void* cursive_x3a_x3aruntime_x3a_x3areactor_x3a_x3aregister(void* reactor,
                                                             void* future) {
    (void)reactor;

    SpawnHandle* handle =
        (SpawnHandle*)cursive_spawn_create(NULL, 0, NULL, NULL, 8, 0, 1);
    if (!handle || !handle->item || !handle->item->result) {
        return handle;
    }

    uint8_t* out = (uint8_t*)handle->item->result;
    c0_memset(out, 0, 8);

    if (!future) {
        return handle;
    }

    const uint8_t* future_bytes = (const uint8_t*)future;
    const uint8_t future_disc = future_bytes[0];
    const uint8_t* future_payload = future_bytes + 8;

    // Async@Completed -> union success arm
    if (future_disc == 1) {
        out[0] = 1;
        c0_memcpy(out + 4, future_payload, 4);
        return handle;
    }

    // Async@Failed -> union error arm
    if (future_disc == 2) {
        out[0] = 0;
        out[4] = future_payload[0];
        return handle;
    }

    // Async@Suspended maps to error arm in this runtime fallback path.
    out[0] = 0;
    out[4] = 0;
    return handle;
}

static int64_t c0_read_int(const void* ptr, size_t size) {
    switch (size) {
        case 1: return *(const int8_t*)ptr;
        case 2: return *(const int16_t*)ptr;
        case 4: return *(const int32_t*)ptr;
        case 8: return *(const int64_t*)ptr;
        default: return 0;
    }
}

static void c0_write_int(void* ptr, size_t size, int64_t value) {
    switch (size) {
        case 1: *(int8_t*)ptr = (int8_t)value; break;
        case 2: *(int16_t*)ptr = (int16_t)value; break;
        case 4: *(int32_t*)ptr = (int32_t)value; break;
        case 8: *(int64_t*)ptr = (int64_t)value; break;
        default: break;
    }
}

static int64_t c0_int_max_for_size(size_t size) {
    switch (size) {
        case 1: return INT8_MAX;
        case 2: return INT16_MAX;
        case 4: return INT32_MAX;
        case 8: return INT64_MAX;
        default: return INT64_MAX;
    }
}

static int64_t c0_int_min_for_size(size_t size) {
    switch (size) {
        case 1: return INT8_MIN;
        case 2: return INT16_MIN;
        case 4: return INT32_MIN;
        case 8: return INT64_MIN;
        default: return INT64_MIN;
    }
}

static int c0_reduce_has_op(C0StringView op) {
    return op.data && op.len > 0;
}

static char c0_reduce_char(C0StringView op, size_t idx) {
    if (!op.data || idx >= op.len) {
        return 0;
    }
    return (char)op.data[idx];
}

static void c0_reduce_init(C0StringView op, void* accum, size_t size) {
    if (!c0_reduce_has_op(op) || !accum) {
        return;
    }
    const char c0 = c0_reduce_char(op, 0);
    const char c1 = c0_reduce_char(op, 1);
    if (c0 == '*') {
        c0_write_int(accum, size, 1);
        return;
    }
    if (c0 == 'a') {
        c0_write_int(accum, size, 1);
        return;
    }
    if (c0 == 'm' && c1 == 'i') {
        c0_write_int(accum, size, c0_int_max_for_size(size));
        return;
    }
    if (c0 == 'm' && c1 == 'a') {
        c0_write_int(accum, size, c0_int_min_for_size(size));
        return;
    }
    c0_write_int(accum, size, 0);
}

static void c0_reduce_apply(C0StringView op, void* accum, const void* value, size_t size) {
    if (!c0_reduce_has_op(op) || !accum || !value) {
        return;
    }
    int64_t a = c0_read_int(accum, size);
    int64_t b = c0_read_int(value, size);
    const char c0 = c0_reduce_char(op, 0);
    const char c1 = c0_reduce_char(op, 1);
    if (c0 == '+') {
        c0_write_int(accum, size, a + b);
        return;
    }
    if (c0 == '*') {
        c0_write_int(accum, size, a * b);
        return;
    }
    if (c0 == 'm' && c1 == 'i') {
        c0_write_int(accum, size, a < b ? a : b);
        return;
    }
    if (c0 == 'm' && c1 == 'a') {
        c0_write_int(accum, size, a > b ? a : b);
        return;
    }
    if (c0 == 'a') {
        c0_write_int(accum, size, (a != 0 && b != 0) ? 1 : 0);
        return;
    }
    if (c0 == 'o') {
        c0_write_int(accum, size, (a != 0 || b != 0) ? 1 : 0);
        return;
    }
    c0_write_int(accum, size, b);
}

typedef struct {
    uint64_t start;
    uint64_t end;
    size_t elem_size;
    size_t result_size;
    void (*body)(void* hosted_env, void* elem, void* captured, void* result, void* panic_out);
    void* hosted_env;
    void* captured_env;
    C0StringView reduce_op;
    void (*reduce_fn)(void* hosted_env, void* lhs, void* rhs, void* out, void* panic_out);
} DispatchChunkEnv;

static void c0_dispatch_chunk(void* hosted_env, void* env_ptr, void* result_ptr, void* panic_out) {
    (void)hosted_env;
    DispatchChunkEnv* env = (DispatchChunkEnv*)env_ptr;
    if (!env || !env->body) {
        return;
    }
    C0PanicRecord local_panic;
    local_panic.panic = 0;
    local_panic.code = 0;
    C0PanicRecord* panic_record =
        panic_out ? (C0PanicRecord*)panic_out : &local_panic;
    uint8_t idx_buf[8];
    uint8_t* iter_result = NULL;
    if (env->result_size > 0) {
        iter_result = (uint8_t*)c0_heap_alloc_raw(env->result_size);
    }
    const int has_reduce = result_ptr && env->result_size > 0;
    const int use_custom = env->reduce_fn != NULL;
    const int use_builtin = c0_reduce_has_op(env->reduce_op);
    int has_accum = 0;
    if (use_builtin && has_reduce) {
        c0_reduce_init(env->reduce_op, result_ptr, env->result_size);
    }
    for (uint64_t i = env->start; i < env->end; ++i) {
        c0_memset(idx_buf, 0, sizeof(idx_buf));
        const size_t copy = env->elem_size < sizeof(uint64_t) ? env->elem_size : sizeof(uint64_t);
        c0_memcpy(idx_buf, &i, copy);
        void* out_ptr = iter_result ? (void*)iter_result : result_ptr;
        env->body(env->hosted_env, idx_buf, env->captured_env, out_ptr, panic_record);
        if (panic_record->panic) {
            cursive_parallel_work_panic(c0_current_ctx(), panic_record->code);
            break;
        }
        if (has_reduce && iter_result) {
            if (use_custom) {
                if (!has_accum) {
                    c0_memcpy(result_ptr, iter_result, env->result_size);
                    has_accum = 1;
                } else {
                    env->reduce_fn(env->hosted_env, result_ptr, iter_result, result_ptr, panic_record);
                    if (panic_record->panic) {
                        cursive_parallel_work_panic(c0_current_ctx(), panic_record->code);
                        break;
                    }
                }
            } else if (use_builtin) {
                c0_reduce_apply(env->reduce_op, result_ptr, iter_result, env->result_size);
            }
        }
    }
    if (use_custom && has_reduce && !has_accum) {
        c0_memset(result_ptr, 0, env->result_size);
    }
    if (c0_debug_flag_enabled("CURSIVE_DEBUG_DISPATCH_RESULT_RUNTIME")) {
        c0_debug_write_dispatch_chunk_value("local",
                                            env->start,
                                            env->end,
                                            result_ptr,
                                            env->result_size);
    }
    if (iter_result) {
        c0_heap_free_raw(iter_result);
    }
}

// §18.5.2 Dispatch iteration
// Executes body for each element in range with optional reduction
void cursive_dispatch_run(C0Range range, size_t elem_size, size_t result_size,
                           void (*body)(void* hosted_env, void* elem, void* captured, void* result, void* panic_out),
                           void* hosted_env,
                           void* captured_env,
                           C0StringView reduce_op,
                           void* reduce_result,
                           void (*reduce_fn)(void* hosted_env, void* lhs, void* rhs, void* out, void* panic_out),
                           int ordered,
                           size_t chunk_size) {
    if (!body) return;

    uint64_t start = 0;
    uint64_t end = 0;
    switch (range.tag) {
        case 0:  // To
            start = 0;
            end = range.hi;
            break;
        case 1:  // ToInclusive
            start = 0;
            end = range.hi + 1;
            break;
        case 2:  // Full
            start = range.lo;
            end = range.hi;
            break;
        case 3:  // From
            start = range.lo;
            end = range.hi;
            break;
        case 4:  // Exclusive
            start = range.lo;
            end = range.hi;
            break;
        case 5:  // Inclusive
            start = range.lo;
            end = range.hi + 1;
            break;
        default:
            c0_debug_write_dispatch_range(range,
                                          0,
                                          0,
                                          elem_size,
                                          result_size,
                                          ordered,
                                          chunk_size);
            return;
    }
    c0_debug_write_dispatch_range(range,
                                  start,
                                  end,
                                  elem_size,
                                  result_size,
                                  ordered,
                                  chunk_size);
    if (end <= start) {
        if (reduce_result && result_size > 0) {
            if (reduce_fn) {
                c0_memset(reduce_result, 0, result_size);
            } else if (c0_reduce_has_op(reduce_op)) {
                c0_reduce_init(reduce_op, reduce_result, result_size);
            }
        }
        return;
    }

    // Spec permits concurrency but does not require it; execute dispatch
    // deterministically in-process to preserve result correctness.
    const int use_threaded_dispatch = 0;
    if (!use_threaded_dispatch || !c0_current_ctx() || !c0_current_ctx()->pool || ordered) {
        DispatchChunkEnv env;
        env.start = start;
        env.end = end;
        env.elem_size = elem_size;
        env.result_size = result_size;
        env.body = body;
        env.hosted_env = hosted_env;
        env.captured_env = captured_env;
        env.reduce_op = reduce_op;
        env.reduce_fn = reduce_fn;
    WorkItem* item = (WorkItem*)c0_heap_alloc_raw(sizeof(WorkItem));
        if (!item) {
            c0_dispatch_chunk(NULL, &env, reduce_result, NULL);
            return;
        }
        item->state = WORK_PENDING;
        item->captured_env = &env;
        item->hosted_env = NULL;
        item->body = c0_dispatch_chunk;
        item->result = reduce_result;
        item->result_size = result_size;
        item->panic_code = 0;
        item->next = NULL;
        item->all_next = NULL;
        item->handle = NULL;
        item->done_event = NULL;
        if (c0_current_ctx()) {
            item->all_next = c0_current_ctx()->all_items;
            c0_current_ctx()->all_items = item;
        }
        c0_run_item(c0_current_ctx(), item);
        item->captured_env = NULL;
        item->result = NULL;
        item->result_size = 0;
        if (!c0_current_ctx()) {
        c0_heap_free_raw(item);
        }
        return;
    }

    ParallelContext* ctx = c0_current_ctx();
    WorkerPool* pool = ctx->pool;
    uint64_t count = end - start;
    if (chunk_size == 0) {
        size_t denom = pool->num_workers > 0 ? (size_t)pool->num_workers : 1;
        chunk_size = (size_t)((count + denom - 1) / denom);
    }
    if (chunk_size == 0) {
        chunk_size = 1;
    }

    size_t num_chunks = (size_t)((count + chunk_size - 1) / chunk_size);
    WorkItem** items =
        (WorkItem**)c0_heap_alloc_raw(sizeof(WorkItem*) * num_chunks);
    if (!items) {
        DispatchChunkEnv env;
        env.start = start;
        env.end = end;
        env.elem_size = elem_size;
        env.result_size = result_size;
        env.body = body;
        env.hosted_env = hosted_env;
        env.captured_env = captured_env;
        env.reduce_op = reduce_op;
        env.reduce_fn = reduce_fn;
        c0_dispatch_chunk(NULL, &env, reduce_result, NULL);
        return;
    }

    for (size_t c = 0; c < num_chunks; ++c) {
        uint64_t chunk_start = start + (uint64_t)c * (uint64_t)chunk_size;
        uint64_t chunk_end = chunk_start + (uint64_t)chunk_size;
        if (chunk_end > end) {
            chunk_end = end;
        }
        DispatchChunkEnv* env =
            (DispatchChunkEnv*)c0_heap_alloc_raw(sizeof(DispatchChunkEnv));
        if (!env) {
            items[c] = NULL;
            continue;
        }
        env->start = chunk_start;
        env->end = chunk_end;
        env->elem_size = elem_size;
        env->result_size = result_size;
        env->body = body;
        env->hosted_env = hosted_env;
        env->captured_env = captured_env;
        env->reduce_op = reduce_op;
        env->reduce_fn = reduce_fn;

        WorkItem* item = (WorkItem*)c0_heap_alloc_raw(sizeof(WorkItem));
        if (!item) {
            c0_heap_free_raw(env);
            items[c] = NULL;
            continue;
        }
        item->state = WORK_PENDING;
        item->captured_env = env;
        item->hosted_env = NULL;
        item->body = c0_dispatch_chunk;
        item->result = (reduce_fn || c0_reduce_has_op(reduce_op)) && result_size > 0
                           ? c0_heap_alloc_raw(result_size)
                           : NULL;
        item->result_size = result_size;
        item->panic_code = 0;
        item->next = NULL;
        item->all_next = NULL;
        item->handle = NULL;
        item->done_event = CreateEvent(NULL, TRUE, FALSE, NULL);

        items[c] = item;
        if (ctx) {
            item->all_next = ctx->all_items;
            ctx->all_items = item;
        }
        c0_enqueue_item(pool, item);
    }

    const int use_custom = reduce_fn != NULL;
    const int use_builtin = c0_reduce_has_op(reduce_op);
    int has_accum = 0;
    WorkItem* reduce_item = NULL;
    if (use_builtin && reduce_result && result_size > 0) {
        c0_reduce_init(reduce_op, reduce_result, result_size);
    }

    for (size_t c = 0; c < num_chunks; ++c) {
        WorkItem* item = items[c];
        if (!item) {
            continue;
        }
        if (item->done_event) {
            WaitForSingleObject(item->done_event, INFINITE);
        }
        if (reduce_result && item->result && result_size > 0) {
            if (c0_debug_flag_enabled("CURSIVE_DEBUG_DISPATCH_RESULT_RUNTIME")) {
                uint64_t chunk_start = start + (uint64_t)c * (uint64_t)chunk_size;
                uint64_t chunk_end = chunk_start + (uint64_t)chunk_size;
                if (chunk_end > end) {
                    chunk_end = end;
                }
                c0_debug_write_dispatch_chunk_value("merge-in",
                                                    chunk_start,
                                                    chunk_end,
                                                    item->result,
                                                    result_size);
            }
            if (use_custom) {
                if (!has_accum) {
                    c0_memcpy(reduce_result, item->result, result_size);
                    has_accum = 1;
                } else {
                    if (!reduce_item && ctx) {
                        reduce_item =
                            (WorkItem*)c0_heap_alloc_raw(sizeof(WorkItem));
                        if (reduce_item) {
                            c0_memset(reduce_item, 0, sizeof(WorkItem));
                            reduce_item->state = WORK_RUNNING;
                            reduce_item->all_next = ctx->all_items;
                            ctx->all_items = reduce_item;
                        }
                    }
                    if (!reduce_item) {
                        C0PanicRecord panic_record;
                        panic_record.panic = 0;
                        panic_record.code = 0;
                        reduce_fn(hosted_env, reduce_result, item->result, reduce_result, &panic_record);
                        if (panic_record.panic) {
                            cursive_parallel_work_panic(ctx, panic_record.code);
                            break;
                        }
                    } else {
                        C0PanicRecord panic_record;
                        panic_record.panic = 0;
                        panic_record.code = 0;
                        C0ThreadState* state = c0_tls_state();
                        ParallelContext* prev_ctx = state->ctx;
                        WorkItem* prev_item = state->item;
                        int prev_scope = state->in_panic_scope;
                        state->ctx = ctx;
                        state->item = reduce_item;
                        state->in_panic_scope = 1;
                        __try {
                            reduce_fn(hosted_env, reduce_result, item->result, reduce_result, &panic_record);
                            if (panic_record.panic) {
                                cursive_parallel_work_panic(ctx, panic_record.code);
                            }
                        } __except (GetExceptionCode() == C0_PANIC_EXCEPTION_CODE
                                        ? EXCEPTION_EXECUTE_HANDLER
                                        : EXCEPTION_CONTINUE_SEARCH) {
                            uint32_t code =
                                c0_panic_code_from_exception(GetExceptionInformation());
                            cursive_parallel_work_panic(ctx, code);
                            state->ctx = prev_ctx;
                            state->item = prev_item;
                            state->in_panic_scope = prev_scope;
                            break;
                        }
                        state->ctx = prev_ctx;
                        state->item = prev_item;
                        state->in_panic_scope = prev_scope;
                    }
                }
            } else if (use_builtin) {
                c0_reduce_apply(reduce_op, reduce_result, item->result, result_size);
            }
        }
    }

    if (c0_debug_flag_enabled("CURSIVE_DEBUG_DISPATCH_RESULT_RUNTIME")) {
        c0_debug_write_dispatch_chunk_value("final",
                                            start,
                                            end,
                                            reduce_result,
                                            result_size);
    }

    c0_heap_free_raw(items);
}

// §18.6.1 Create cancellation token
C0CancelId cursive_cancel_token_new(void) {
    C0CancelId token_id = C0_CANCEL_INVALID_ID;
    if (!c0_cancel_registry_ready()) {
        return C0_CANCEL_INVALID_ID;
    }

    EnterCriticalSection(&c0_cancel_registry.lock);
    token_id = c0_cancel_registry_new_locked(C0_CANCEL_INVALID_ID);
    LeaveCriticalSection(&c0_cancel_registry.lock);
    return token_id;
}

// §18.6.1 Request cancellation
void cursive_cancel_token_cancel(C0CancelId token_id) {
    if (!c0_cancel_registry_ready()) {
        return;
    }

    EnterCriticalSection(&c0_cancel_registry.lock);
    c0_cancel_registry_cancel_locked(token_id);
    LeaveCriticalSection(&c0_cancel_registry.lock);
}

// §18.6.1 Check if cancelled
int cursive_cancel_token_is_cancelled(C0CancelId token_id) {
    return c0_token_is_cancelled(token_id);
}

// §18.7 Record panic in work item
void cursive_parallel_work_panic(void* ctx_ptr, uint32_t code) {
    ParallelContext* ctx = (ParallelContext*)ctx_ptr;
    if (!ctx) {
        ctx = c0_current_ctx();
    }
    WorkItem* item = c0_current_item();
    if (!ctx || !item) {
        cursive_panic(code);
        return;
    }

    item->state = WORK_PANICKED;
    item->panic_code = code;
    if (item->result && item->result_size > 0) {
        c0_memset(item->result, 0, item->result_size);
    }
    ctx->panic_count += 1;
    if (!ctx->first_panic) {
        ctx->first_panic = item;
    }

    if (ctx->cancel_token != C0_CANCEL_INVALID_ID && ctx->panic_count == 1) {
        cursive_cancel_token_cancel(ctx->cancel_token);
    }
}

C0CancelId CancelToken_x3a_x3anew(void) {
    return cursive_cancel_token_new();
}

static C0CancelId c0_cancel_token_from_self_ref(void* self_ref) {
    if (!self_ref) {
        return C0_CANCEL_INVALID_ID;
    }
    return *((const C0CancelId*)self_ref);
}

void CancelToken_x3a_x3aActive_x3a_x3acancel(void* self) {
    cursive_cancel_token_cancel(c0_cancel_token_from_self_ref(self));
}

uint8_t CancelToken_x3a_x3aActive_x3a_x3ais_x5fcancelled(void* self) {
    return (uint8_t)cursive_cancel_token_is_cancelled(
        c0_cancel_token_from_self_ref(self));
}

C0CancelId CancelToken_x3a_x3aActive_x3a_x3achild(void* self) {
    C0CancelId parent = c0_cancel_token_from_self_ref(self);
    C0CancelId child = C0_CANCEL_INVALID_ID;
    if (!c0_cancel_registry_ready()) {
        return C0_CANCEL_INVALID_ID;
    }

    EnterCriticalSection(&c0_cancel_registry.lock);
    if (c0_cancel_registry_valid_id_locked(parent)) {
        child = c0_cancel_registry_new_locked(parent);
    }
    LeaveCriticalSection(&c0_cancel_registry.lock);
    return child;
}

void CancelToken_x3a_x3aActive_x3a_x3await_x5fcancelled(void* out, void* self) {
    if (!out) {
        return;
    }
    C0AsyncResumeValue* async_out = (C0AsyncResumeValue*)out;
    C0CancelId token_id = c0_cancel_token_from_self_ref(self);
    if (token_id == C0_CANCEL_INVALID_ID || c0_token_is_cancelled(token_id)) {
        c0_cancel_wait_write_completed(async_out);
        return;
    }

    C0CancelWaitFrame* frame =
        (C0CancelWaitFrame*)c0_heap_alloc_raw(sizeof(C0CancelWaitFrame));
    if (!frame) {
        // Preserve progress on allocation failure by producing a completed
        // async value rather than an invalid suspended state.
        c0_cancel_wait_write_completed(async_out);
        return;
    }

    frame->resume_state = 0;
    frame->resume_fn = (void*)&c0_cancel_wait_resume;
    frame->hosted_env = NULL;
    frame->token_id = token_id;
    c0_cancel_wait_write_suspended(async_out, frame);
}
