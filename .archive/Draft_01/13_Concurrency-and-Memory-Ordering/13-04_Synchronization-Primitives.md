# Cursive Language Specification

## Clause 13 — Concurrency and Memory Ordering

**Part**: XIII — Concurrency and Memory Ordering  
**File**: 14-4_Synchronization-Primitives.md  
**Section**: §13.4 Synchronization Primitives  
**Stable label**: [concurrency.synchronization]  
**Forward references**: §14.1 [concurrency.model], §14.2 [concurrency.memory], §14.3 [concurrency.atomic], Clause 7 §7.6 [type.modal], Clause 11 §11.4 [memory.permission]

---

### §13.4 Synchronization Primitives [concurrency.synchronization]

#### §13.4.1 Overview

[1] Synchronization primitives coordinate access to `shared` data across threads. Cursive provides mutex locks, read-write locks, channels, and barriers as built-in modal types integrating with the permission system and happens-before memory model.

[2] All synchronization primitives are modal types tracking their lifecycle states. Lock acquisition and release are state transitions enforced by the type system.

[3] This subclause specifies the core synchronization primitives provided by the language. Additional primitives (semaphores, condition variables, barriers) are standard library types built on the same modal foundation.

#### §13.4.2 Mutex (Mutual Exclusion Lock)

##### §13.4.2.1 Mutex Modal Type

[4] `Mutex` is a built-in modal type providing exclusive access to shared data:

```cursive
modal Mutex<T> {
    @Unlocked {
        data: T,
    }
    
    @Unlocked::lock(~!) -> @Locked
    @Unlocked::try_lock(~!) -> @Locked \/ LockError
    
    @Locked {
        data: T,
    }
    
    @Locked::unlock(~!) -> @Unlocked
}
```

[5] **State semantics**:

- **@Unlocked**: Mutex is available for locking. Data is inaccessible.
- **@Locked**: Mutex is locked; data is accessible exclusively. Only the thread holding the lock may access the data.

[6] **RAII Guard Pattern**: The standard library provides `MutexGuard` for automatic unlocking:

```cursive
record MutexGuard<T> {
    mutex: unique Mutex<T>@Locked,
    
    procedure get(~!): unique T
        [[ |- true => true ]]
    {
        result self.mutex.data
    }
}

behavior Drop for MutexGuard<T> {
    procedure drop(~!)
        [[ sync::lock |- true => true ]]
    {
        self.mutex.unlock()
    }
}
```

##### §13.4.2.2 Usage

**Example 14.4.2.1 (Mutex protecting shared counter)**:

```cursive
var counter_mutex: Mutex<i32>@Unlocked = Mutex::new(0)

procedure increment_with_lock()
    [[ sync::lock |- true => true ]]
{
    let locked = counter_mutex.lock()  // Mutex<i32>@Locked
    locked.data = locked.data + 1
    locked.unlock()  // Back to @Unlocked
}

procedure safe_increment()
    [[ sync::lock |- true => true ]]
{
    let guard = MutexGuard { mutex: counter_mutex.lock() }
    guard.get() = guard.get() + 1
    // guard.drop() called automatically - unlocks mutex
}
```

**Example 14.4.2.2 (Mutex across threads)**:

```cursive
procedure parallel_increment(iterations: usize)
    [[ thread::spawn, thread::join, sync::lock, alloc::heap |- true => true ]]
{
    let mutex = Mutex::new(0)  // Mutex<i32>@Unlocked
    let thread_count = 4
    
    var threads: [Thread<()>@Spawned] = []
    
    loop i in 0..thread_count {
        let thread = spawn(|| [[ sync::lock |- true => true ]] {
            loop j in 0..iterations {
                let locked = mutex.lock()
                locked.data = locked.data + 1
                locked.unlock()
            }
        })
        
        threads.push(thread)
    }
    
    loop thread in threads {
        thread.join()
    }
    
    let final_locked = mutex.lock()
    println("Final count: {}", final_locked.data)  // thread_count * iterations
    final_locked.unlock()
}
```

##### §13.4.2.3 Synchronization Semantics

[7] Mutex unlock synchronizes-with the next mutex lock:

$$
\frac{\text{Thread}_1: \texttt{unlock}(m) \quad \text{Thread}_2: \texttt{lock}(m)}
     {\texttt{unlock}(m) \text{ synchronizes-with } \texttt{lock}(m)}
\tag{SW-Mutex}
$$

[8] All operations before unlock in Thread 1 happen-before all operations after lock in Thread 2.

#### §13.4.3 Channel (Message Passing)

##### §13.4.3.1 Channel Modal Type

[9] `Channel` is a built-in modal type for message passing between threads:

```cursive
modal Channel<T> {
    @Open {
        sender_count: usize,
        receiver_count: usize,
    }
    
    @Open::send(~%, value: T) -> @Open
    @Open::receive(~%) -> T \/ ChannelError
    @Open::close_sender(~!) -> @SenderClosed
    @Open::close_receiver(~!) -> @ReceiverClosed
    
    @SenderClosed {
        receiver_count: usize,
    }
    
    @SenderClosed::receive(~%) -> T \/ ChannelError
    
    @ReceiverClosed {
        sender_count: usize,
    }
    
    @Closed { }
}

enum ChannelError {
    Disconnected,
    Closed,
}
```

[10] **State semantics**:

- **@Open**: Channel is active; can send and receive
- **@SenderClosed**: Sender closed; can still receive pending values
- **@ReceiverClosed**: Receiver closed; sends will fail
- **@Closed**: Channel fully closed; no operations possible

##### §13.4.3.2 Usage

**Example 14.4.3.1 (Producer-consumer with channel)**:

```cursive
procedure producer_consumer()
    [[ thread::spawn, thread::join, sync::channel, io::write |- true => true ]]
{
    let channel: Channel<i32>@Open = Channel::new()
    
    let producer = spawn(move || [[ sync::channel |- true => true ]] {
        loop i in 0..100 {
            channel.send(i)
        }
        channel.close_sender()
    })
    
    let consumer = spawn(move || [[ sync::channel, io::write |- true => true ]] {
        loop {
            match channel.receive() {
                value: i32 => println("Received: {}", value),
                err: ChannelError::Disconnected => break,
                err: ChannelError::Closed => break,
            }
        }
    })
    
    producer.join()
    consumer.join()
}
```

##### §13.4.3.3 Synchronization Semantics

[11] Channel send synchronizes-with channel receive:

$$
\frac{\texttt{send}(c, v) \text{ in Thread}_1 \quad \texttt{receive}(c) = v \text{ in Thread}_2}
     {\texttt{send}(v) \text{ synchronizes-with } \texttt{receive}()}
\tag{SW-Channel}
$$

[12] All operations before send happen-before all operations after receive.

#### §13.4.4 RwLock (Read-Write Lock)

##### §13.4.4.1 RwLock Modal Type

[13] `RwLock` allows multiple readers or single writer:

```cursive
modal RwLock<T> {
    @Unlocked {
        data: T,
    }
    
    @Unlocked::read_lock(~%) -> @ReadLocked
    @Unlocked::write_lock(~!) -> @WriteLocked
    
    @ReadLocked {
        data: const T,
        reader_count: usize,
    }
    
    @ReadLocked::read_unlock(~%) -> @Unlocked \/ @ReadLocked
    
    @WriteLocked {
        data: unique T,
    }
    
    @WriteLocked::write_unlock(~!) -> @Unlocked
}
```

[14] **State semantics**:

- **@Unlocked**: No locks held; available for read or write lock
- **@ReadLocked**: One or more read locks held; data accessible as const
- **@WriteLocked**: Write lock held; data accessible as unique

[15] **Permission reflection**: The locked state determines data permission:
- @ReadLocked: data has `const` permission (multiple readers)
- @WriteLocked: data has `unique` permission (exclusive writer)

**Example 14.4.4.1 (RwLock for shared configuration)**:

```cursive
var config_lock: RwLock<Config>@Unlocked = RwLock::new(default_config())

procedure read_config(): Config
    [[ sync::lock |- true => true ]]
{
    let read_locked = config_lock.read_lock()  // @ReadLocked
    let config_copy = read_locked.data.clone()
    read_locked.read_unlock()
    result config_copy
}

procedure update_config(new_config: Config)
    [[ sync::lock |- true => true ]]
{
    let write_locked = config_lock.write_lock()  // @WriteLocked
    write_locked.data = new_config
    write_locked.write_unlock()
}
```

#### §13.4.5 Constraints

[1] _Type constraint for synchronization primitives._ Types stored in Mutex, RwLock, or sent through channels shall satisfy permission requirements:
- Mutex<T>: T may be any type
- RwLock<T>: T may be any type
- Channel<T>: T shall satisfy `T: const` for safe message passing

Violating type constraints produces diagnostic E13-300.

[2] _Deadlock prevention._ The specification does not prevent deadlocks at compile time. Programmers are responsible for avoiding circular lock dependencies. Implementations should provide runtime deadlock detection in debug builds.

[3] _Grant requirements._ Synchronization operations require grants:
- Mutex lock/unlock: `sync::lock`
- Channel send/receive: `sync::channel`
- RwLock operations: `sync::lock`

Missing grants produce diagnostic E14-301.

[4] _Lifecycle requirements._ Mutexes shall be unlocked before being dropped. Dropping a locked mutex produces undefined behavior [UB-ID: B.2.52].

#### §13.4.6 Conformance Requirements

[5] Implementations shall:

1. Provide Mutex<T>, Channel<T>, and RwLock<T> as built-in modal types
2. Implement lock/unlock as state transitions enforced by type system
3. Establish synchronizes-with relations for all synchronization operations
4. Integrate permissions with lock states (const for read locks, unique for write locks)
5. Support channel message passing with synchronization guarantees
6. Enforce type constraints (const requirement for channel messages)
7. Require appropriate grants for all synchronization operations
8. Emit diagnostics E13-300, E13-301 for constraint violations
9. Provide runtime deadlock detection in debug builds (recommended)
10. Document synchronization primitive performance characteristics

---

**Previous**: §14.3 Atomic Operations and Orderings (§14.3 [concurrency.atomic]) | **Next**: Clause 15 — Interoperability and ABI (§15 [interop])

