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

#include "rt_internal.h"

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
    void (*body)(void* env, void* result, void* panic_out);     // Work function
    void* result;            // Result value
    size_t result_size;      // Size of result
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

// §18.6.1 CancelToken runtime representation
typedef struct CancelToken {
    volatile int cancelled;
    struct CancelToken* parent;
} CancelToken;

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
    CancelToken* cancel_token;
    ParallelContext* ctx;
};

// §18.1 Parallel context
struct ParallelContext {
    WorkerPool* pool;
    CancelToken* cancel_token;
    WorkItem* first_panic;    // First panicked work item
    int panic_count;          // Number of panics
    const char* name;         // Debug name
    WorkItem* all_items;      // All work items for cleanup
    ParallelContext* prev_ctx;
    int inline_domain;
};

// Thread-local parallel context tracking (for nested parallel support)

static int c0_token_is_cancelled(const CancelToken* token) {
    const CancelToken* cur = token;
    while (cur) {
        if (cur->cancelled) {
            return 1;
        }
        cur = cur->parent;
    }
    return 0;
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
        if (ctx && ctx->cancel_token && c0_token_is_cancelled(ctx->cancel_token)) {
            item->state = WORK_CANCELLED;
            if (item->result && item->result_size > 0) {
                c0_memset(item->result, 0, item->result_size);
            }
        } else {
            item->state = WORK_RUNNING;
            if (item->body) {
                item->body(item->captured_env, item->result, &panic_record);
            }
            if (panic_record.panic) {
                cursive0_parallel_work_panic(ctx, panic_record.code);
            }
            if (item->state == WORK_RUNNING) {
                item->state = WORK_COMPLETED;
            }
        }
    } __except (GetExceptionCode() == C0_PANIC_EXCEPTION_CODE
                    ? EXCEPTION_EXECUTE_HANDLER
                    : EXCEPTION_CONTINUE_SEARCH) {
        uint32_t code = c0_panic_code_from_exception(GetExceptionInformation());
        cursive0_parallel_work_panic(ctx, code);
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

int c0_parallel_in_panic_scope(void) {
    return c0_tls_state()->in_panic_scope != 0;
}

void c0_parallel_raise_panic(uint32_t code) {
    if (!c0_parallel_in_panic_scope()) {
        cursive0_panic(code);
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

        WorkItem* item = pool->queue_head;
        if (item) {
            pool->queue_head = item->next;
            if (!pool->queue_head) {
                pool->queue_tail = NULL;
            }
        }
        pool->active_workers += 1;
        LeaveCriticalSection(&pool->lock);

        if (item) {
            c0_run_item(pool->ctx, item);
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
void* cursive0_parallel_begin(C0DynObject domain, void* cancel_token, const char* name) {
    const C0ExecutionDomain* dom = (const C0ExecutionDomain*)domain.data;
    const int inline_domain = dom && dom->kind == C0_DOMAIN_INLINE;

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
        ctx->pool->cancel_token = (CancelToken*)cancel_token;
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

    ctx->cancel_token = (CancelToken*)cancel_token;
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
int cursive0_parallel_join(void* ctx_ptr) {
    const char* dbg1 = "[JOIN] enter\n";
    WriteFile(GetStdHandle(STD_ERROR_HANDLE), dbg1, 13, NULL, NULL);
    
    ParallelContext* ctx = (ParallelContext*)ctx_ptr;
    if (!ctx) {
        const char* dbg2 = "[JOIN] null ctx\n";
        WriteFile(GetStdHandle(STD_ERROR_HANDLE), dbg2, 15, NULL, NULL);
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
    uint32_t panic_code = had_panic ? ctx->first_panic->panic_code : 0;
    
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
    
    // §18.7.1 Re-raise first panic at block boundary
    if (had_panic) {
        const char* dbg = "[JOIN] propagating panic\n";
        WriteFile(GetStdHandle(STD_ERROR_HANDLE), dbg, 24, NULL, NULL);
        cursive0_panic(panic_code);
    }
    
    const char* dbg_exit = "[JOIN] done\n";
    WriteFile(GetStdHandle(STD_ERROR_HANDLE), dbg_exit, 12, NULL, NULL);
    return had_panic ? 1 : 0;
}

// §18.4.2 Create spawn handle
// Returns Spawned<T>@Pending (or @Ready if body is NULL for inline execution)
void* cursive0_spawn_create(void* env, size_t env_size,
                            void (*body)(void* env, void* result, void* panic_out),
                            size_t result_size) {
    // Debug: track spawn calls
    static int spawn_count = 0;
    spawn_count++;
    char dbg[32];
    dbg[0] = '['; dbg[1] = 'S'; dbg[2] = 'P'; dbg[3] = 'A'; dbg[4] = 'W'; dbg[5] = 'N';
    dbg[6] = ' '; dbg[7] = '#'; dbg[8] = '0' + spawn_count; dbg[9] = ']'; dbg[10] = '\n';
    WriteFile(GetStdHandle(STD_ERROR_HANDLE), dbg, 11, NULL, NULL);
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
    item->body = body;
    item->result = result_size > 0 ? c0_heap_alloc_raw(result_size) : NULL;
    item->result_size = result_size;
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
        c0_run_item(c0_current_ctx(), item);
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
void* cursive0_spawn_wait(void* handle_ptr) {
    SpawnHandle* handle = (SpawnHandle*)handle_ptr;
    if (!handle || !handle->item) {
        const char* dbg = "[WAIT] null handle\n";
        WriteFile(GetStdHandle(STD_ERROR_HANDLE), dbg, 18, NULL, NULL);
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
    
    // Debug: show item state
    char dbg[32];
    dbg[0] = '['; dbg[1] = 'W'; dbg[2] = 'A'; dbg[3] = 'I'; dbg[4] = 'T';
    dbg[5] = ' '; dbg[6] = 's'; dbg[7] = 't'; dbg[8] = '=';
    dbg[9] = '0' + item->state; dbg[10] = ']'; dbg[11] = '\n';
    WriteFile(GetStdHandle(STD_ERROR_HANDLE), dbg, 12, NULL, NULL);
    
    // §18.7 Check for panic and propagate
    if (item->state == WORK_PANICKED) {
        const char* dbg2 = "[WAIT] propagating panic\n";
        WriteFile(GetStdHandle(STD_ERROR_HANDLE), dbg2, 24, NULL, NULL);
        cursive0_panic(item->panic_code);
    }
    
    return item->result;
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
        c0_write_int(accum, size, INT64_MAX);
        return;
    }
    if (c0 == 'm' && c1 == 'a') {
        c0_write_int(accum, size, INT64_MIN);
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
    void (*body)(void* elem, void* captured, void* result, void* panic_out);
    void* captured_env;
    C0StringView reduce_op;
    void (*reduce_fn)(void* lhs, void* rhs, void* out, void* panic_out);
} DispatchChunkEnv;

static void c0_dispatch_chunk(void* env_ptr, void* result_ptr, void* panic_out) {
    DispatchChunkEnv* env = (DispatchChunkEnv*)env_ptr;
    if (!env || !env->body) {
        return;
    }
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
        env->body(idx_buf, env->captured_env, out_ptr, panic_out);
        if (has_reduce && iter_result) {
            if (use_custom) {
                if (!has_accum) {
                    c0_memcpy(result_ptr, iter_result, env->result_size);
                    has_accum = 1;
                } else {
                    env->reduce_fn(result_ptr, iter_result, result_ptr, panic_out);
                }
            } else if (use_builtin) {
                c0_reduce_apply(env->reduce_op, result_ptr, iter_result, env->result_size);
            }
        }
    }
    if (use_custom && has_reduce && !has_accum) {
        c0_memset(result_ptr, 0, env->result_size);
    }
    if (iter_result) {
        c0_heap_free_raw(iter_result);
    }
}

// §18.5.2 Dispatch iteration
// Executes body for each element in range with optional reduction
void cursive0_dispatch_run(C0Range range, size_t elem_size, size_t result_size,
                           void (*body)(void* elem, void* captured, void* result, void* panic_out),
                           void* captured_env,
                           C0StringView reduce_op,
                           void* reduce_result,
                           void (*reduce_fn)(void* lhs, void* rhs, void* out, void* panic_out),
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
            return;
    }
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

    if (!c0_current_ctx() || !c0_current_ctx()->pool || ordered) {
        DispatchChunkEnv env;
        env.start = start;
        env.end = end;
        env.elem_size = elem_size;
        env.result_size = result_size;
        env.body = body;
        env.captured_env = captured_env;
        env.reduce_op = reduce_op;
        env.reduce_fn = reduce_fn;
    WorkItem* item = (WorkItem*)c0_heap_alloc_raw(sizeof(WorkItem));
        if (!item) {
            c0_dispatch_chunk(&env, reduce_result, NULL);
            return;
        }
        item->state = WORK_PENDING;
        item->captured_env = &env;
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
        env.captured_env = captured_env;
        env.reduce_op = reduce_op;
        c0_dispatch_chunk(&env, reduce_result, NULL);
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
                        reduce_fn(reduce_result, item->result, reduce_result, &panic_record);
                        if (panic_record.panic) {
                            cursive0_parallel_work_panic(ctx, panic_record.code);
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
                            reduce_fn(reduce_result, item->result, reduce_result, &panic_record);
                            if (panic_record.panic) {
                                cursive0_parallel_work_panic(ctx, panic_record.code);
                            }
                        } __except (GetExceptionCode() == C0_PANIC_EXCEPTION_CODE
                                        ? EXCEPTION_EXECUTE_HANDLER
                                        : EXCEPTION_CONTINUE_SEARCH) {
                            uint32_t code =
                                c0_panic_code_from_exception(GetExceptionInformation());
                            cursive0_parallel_work_panic(ctx, code);
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

    c0_heap_free_raw(items);
}

// §18.6.1 Create cancellation token
void* cursive0_cancel_token_new(void) {
    CancelToken* token =
        (CancelToken*)c0_heap_alloc_raw(sizeof(CancelToken));
    if (token) {
        token->cancelled = 0;
        token->parent = NULL;
    }
    return token;
}

// §18.6.1 Request cancellation
void cursive0_cancel_token_cancel(void* token_ptr) {
    CancelToken* token = (CancelToken*)token_ptr;
    if (token) {
        token->cancelled = 1;
    }
}

// §18.6.1 Check if cancelled
int cursive0_cancel_token_is_cancelled(void* token_ptr) {
    CancelToken* token = (CancelToken*)token_ptr;
    return token ? c0_token_is_cancelled(token) : 0;
}

// §18.7 Record panic in work item
void cursive0_parallel_work_panic(void* ctx_ptr, uint32_t code) {
    ParallelContext* ctx = (ParallelContext*)ctx_ptr;
    if (!ctx) {
        ctx = c0_current_ctx();
    }
    WorkItem* item = c0_current_item();
    if (!ctx || !item) {
        cursive0_panic(code);
        return;
    }

    item->state = WORK_PANICKED;
    item->panic_code = code;
    ctx->panic_count += 1;
    if (!ctx->first_panic) {
        ctx->first_panic = item;
    }

    if (ctx->cancel_token && ctx->panic_count == 1) {
        cursive0_cancel_token_cancel(ctx->cancel_token);
    }
}

void* CancelToken_x3a_x3anew(void) {
    return cursive0_cancel_token_new();
}

void CancelToken_x3a_x3aActive_x3a_x3acancel(void* self) {
    cursive0_cancel_token_cancel(self);
}

uint8_t CancelToken_x3a_x3aActive_x3a_x3ais_x5fcancelled(void* self) {
    return (uint8_t)cursive0_cancel_token_is_cancelled(self);
}

uint8_t CancelToken_x3a_x3aCancelled_x3a_x3ais_x5fcancelled(void* self) {
    return (uint8_t)cursive0_cancel_token_is_cancelled(self);
}

void* CancelToken_x3a_x3aActive_x3a_x3achild(void* self) {
    CancelToken* parent = (CancelToken*)self;
    CancelToken* token =
        (CancelToken*)c0_heap_alloc_raw(sizeof(CancelToken));
    if (token) {
        token->cancelled = 0;
        token->parent = parent;
    }
    return token;
}
