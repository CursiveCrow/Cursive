# Part IX: Grant System
## Section 9.4: Built-In Grants

**File**: `09-4_Built-In-Grants.md`
**Version**: 1.0
**Status**: Normative
**Previous**: [09-3_Grant-Polymorphism.md](09-3_Grant-Polymorphism.md) | **Next**: [09-5_User-Defined-Grants.md](09-5_User-Defined-Grants.md)

---

## 9.4.1 Built-In Grant Catalog

**Normative Statement 9.4.1**: A conforming implementation shall recognize and enforce the built-in grants specified in this section.

**Organization**: Built-in grants are organized in hierarchical categories. Each category contains specific operation grants and a wildcard grant.

**Wildcard Semantics**: A wildcard grant `category::*` grants all specific operations within that category.

---

## 9.4.2 Allocation Grants (`alloc`)

### 9.4.2.1 Heap Allocation (`alloc::heap`)

**Grant**: `alloc::heap`

**Purpose**: Authorizes heap allocation of memory.

**Operations Authorized**:
- Explicit heap allocation procedures
- Conversion of region-allocated data to heap (`.to_heap()` pattern)
- Dynamic memory allocation that persists beyond procedure scope

**Example (informative)**:
```cursive
procedure create_buffer(size: usize): [u8]
    sequent { [alloc::heap] |- true => true }
{
    result heap_allocate_array<u8>(size)
}
```

**Cross-Reference**: See Part VI §6.5 (Storage Duration) for complete heap allocation semantics.

### 9.4.2.2 Region Allocation (`alloc::region`)

**Grant**: `alloc::region`

**Purpose**: Authorizes allocation within regions.

**Operations Authorized**:
- Region creation (`region r { ... }`)
- Allocation within regions (`alloc_in<r>(value)`)
- Region-scoped memory management

**Example (informative)**:
```cursive
procedure process_in_region(data: [i32]): i32
    sequent { [alloc::region] |- true => true }
{
    region temp {
        let buffer = alloc_in<temp>([0; 1024])
        result compute(buffer, data)
    }
}
```

**Cross-Reference**: See Part VI §6.4 (Regions and Lifetimes) for complete region semantics.

### 9.4.2.3 Stack Allocation (`alloc::stack`)

**Grant**: `alloc::stack`

**Purpose**: Reserved for future use. Stack allocation is typically implicit and does not require explicit grants.

**Status**: This grant is defined but not currently enforced by implementations.

### 9.4.2.4 Temporary Allocation (`alloc::temp`)

**Grant**: `alloc::temp`

**Purpose**: Reserved for future use. May authorize temporary allocations with automatic cleanup.

**Status**: This grant is defined but not currently enforced by implementations.

### 9.4.2.5 All Allocation (`alloc::*`)

**Grant**: `alloc::*`

**Purpose**: Wildcard grant authorizing all allocation operations.

**Expansion**: `alloc::*` ≡ `{alloc::heap, alloc::region, alloc::stack, alloc::temp}`

**Example (informative)**:
```cursive
procedure unrestricted_allocation()
    sequent { [alloc::*] |- true => true }
{
    // Can perform any allocation operation
}
```

---

## 9.4.3 File System Grants (`fs`)

### 9.4.3.1 File Read (`fs::read`)

**Grant**: `fs::read`

**Purpose**: Authorizes reading from files and directories.

**Operations Authorized**:
- Opening files for reading
- Reading file contents
- Listing directory contents
- Checking file existence (read-only queries)

**Example (informative)**:
```cursive
procedure load_config(path: string): [u8]
    sequent { [fs::read] |- true => true }
{
    result read_file_bytes(path)
}
```

**Security Note**: This grant does not authorize reading file metadata such as permissions or timestamps. See `fs::metadata` for metadata operations.

### 9.4.3.2 File Write (`fs::write`)

**Grant**: `fs::write`

**Purpose**: Authorizes writing to files and creating/modifying files.

**Operations Authorized**:
- Creating new files
- Writing to existing files
- Truncating files
- Appending to files
- Creating directories

**Example (informative)**:
```cursive
procedure save_data(path: string, data: [u8])
    sequent { [fs::write] |- true => true }
{
    write_file_bytes(path, data)
}
```

**Security Note**: This grant does not authorize file deletion. See `fs::delete`.

### 9.4.3.3 File Delete (`fs::delete`)

**Grant**: `fs::delete`

**Purpose**: Authorizes deletion of files and directories.

**Operations Authorized**:
- Deleting files
- Deleting directories
- Removing directory trees

**Example (informative)**:
```cursive
procedure cleanup(path: string)
    sequent { [fs::delete] |- true => true }
{
    delete_file(path)
}
```

### 9.4.3.4 File Metadata (`fs::metadata`)

**Grant**: `fs::metadata`

**Purpose**: Authorizes reading and modifying file metadata.

**Operations Authorized**:
- Reading file permissions
- Modifying file permissions
- Reading file timestamps
- Modifying file timestamps
- Reading file ownership information
- Modifying file ownership (if permitted by OS)

**Example (informative)**:
```cursive
procedure get_file_size(path: string): usize
    sequent { [fs::metadata] |- true => true }
{
    let metadata = file_metadata(path)
    result metadata.size
}
```

### 9.4.3.5 All File System (`fs::*`)

**Grant**: `fs::*`

**Purpose**: Wildcard grant authorizing all file system operations.

**Expansion**: `fs::*` ≡ `{fs::read, fs::write, fs::delete, fs::metadata}`

**Example (informative)**:
```cursive
procedure file_manager()
    sequent { [fs::*] |- true => true }
{
    // Can perform any file system operation
}
```

---

## 9.4.4 Network Grants (`net`)

### 9.4.4.1 Network Read (`net::read`)

**Grant**: `net::read`

**Purpose**: Authorizes receiving data from network connections.

**Operations Authorized**:
- Reading from sockets
- Receiving UDP packets
- Reading from network streams
- Accepting incoming connections (receiving side)

**Example (informative)**:
```cursive
procedure receive_data(socket: Socket): [u8]
    sequent { [net::read, alloc::heap] |- true => true }
{
    let buffer = heap_allocate_array<u8>(1024)
    socket_recv(socket, buffer)
    result buffer
}
```

### 9.4.4.2 Network Write (`net::write`)

**Grant**: `net::write`

**Purpose**: Authorizes sending data over network connections.

**Operations Authorized**:
- Writing to sockets
- Sending UDP packets
- Writing to network streams

**Example (informative)**:
```cursive
procedure send_data(socket: Socket, data: [u8])
    sequent { [net::write] |- true => true }
{
    socket_send(socket, data)
}
```

### 9.4.4.3 Network Connect (`net::connect`)

**Grant**: `net::connect`

**Purpose**: Authorizes initiating outbound network connections.

**Operations Authorized**:
- Creating TCP connections
- Creating UDP sockets
- Resolving DNS names
- Initiating network protocols

**Example (informative)**:
```cursive
procedure connect_to_server(host: string, port: u16): Socket
    sequent { [net::connect] |- true => true }
{
    result tcp_connect(host, port)
}
```

### 9.4.4.4 Network Listen (`net::listen`)

**Grant**: `net::listen`

**Purpose**: Authorizes creating server sockets that listen for incoming connections.

**Operations Authorized**:
- Binding to ports
- Listening for connections
- Creating server sockets

**Example (informative)**:
```cursive
procedure start_server(port: u16): Socket
    sequent { [net::listen] |- true => true }
{
    result tcp_listen(port)
}
```

### 9.4.4.5 All Network (`net::*`)

**Grant**: `net::*`

**Purpose**: Wildcard grant authorizing all network operations.

**Expansion**: `net::*` ≡ `{net::read, net::write, net::connect, net::listen}`

---

## 9.4.5 Time Grants (`time`)

### 9.4.5.1 Time Read (`time::read`)

**Grant**: `time::read`

**Purpose**: Authorizes reading the current time and date.

**Operations Authorized**:
- Reading system clock
- Getting current timestamp
- Reading timezone information
- Querying time-related system state

**Example (informative)**:
```cursive
procedure get_timestamp(): i64
    sequent { [time::read] |- true => true }
{
    result system_time_now()
}
```

**Rationale**: Time reading is capability-restricted because it enables side-channel attacks and non-deterministic behavior.

### 9.4.5.2 Time Sleep (`time::sleep`)

**Grant**: `time::sleep`

**Purpose**: Authorizes blocking execution for a specified duration.

**Operations Authorized**:
- Sleep/delay operations
- Timed waiting
- Scheduling delays

**Example (informative)**:
```cursive
procedure rate_limit()
    sequent { [time::sleep] |- true => true }
{
    sleep_milliseconds(100)
}
```

### 9.4.5.3 Monotonic Time (`time::monotonic`)

**Grant**: `time::monotonic`

**Purpose**: Authorizes reading monotonic clock for performance measurement.

**Operations Authorized**:
- Reading monotonic clock
- Performance counter queries
- High-resolution timing

**Example (informative)**:
```cursive
procedure measure_duration(): i64
    sequent { [time::monotonic] |- true => true }
{
    let start = monotonic_now()
    perform_operation()
    let end = monotonic_now()
    result end - start
}
```

### 9.4.5.4 All Time (`time::*`)

**Grant**: `time::*`

**Purpose**: Wildcard grant authorizing all time operations.

**Expansion**: `time::*` ≡ `{time::read, time::sleep, time::monotonic}`

---

## 9.4.6 Thread Grants (`thread`)

### 9.4.6.1 Thread Spawn (`thread::spawn`)

**Grant**: `thread::spawn`

**Purpose**: Authorizes creating new threads.

**Operations Authorized**:
- Spawning threads
- Creating thread pools
- Forking execution

**Example (informative)**:
```cursive
procedure spawn_worker()
    sequent { [thread::spawn] |- true => true }
{
    thread_spawn(|| worker_function())
}
```

**Forward Reference**: See Part XIII (Concurrency) for complete threading semantics.

### 9.4.6.2 Thread Join (`thread::join`)

**Grant**: `thread::join`

**Purpose**: Authorizes waiting for thread completion.

**Operations Authorized**:
- Joining threads
- Waiting for thread termination
- Collecting thread results

**Example (informative)**:
```cursive
procedure wait_for_worker(handle: ThreadHandle)
    sequent { [thread::join] |- true => true }
{
    thread_join(handle)
}
```

### 9.4.6.3 Atomic Operations (`thread::atomic`)

**Grant**: `thread::atomic`

**Purpose**: Authorizes atomic memory operations for synchronization.

**Operations Authorized**:
- Atomic load/store
- Compare-and-swap operations
- Atomic arithmetic
- Memory barriers

**Example (informative)**:
```cursive
procedure atomic_increment(counter: unique AtomicI32)
    sequent { [thread::atomic] |- true => true }
{
    counter.fetch_add(1)
}
```

**Forward Reference**: See Part XIII §13.3 (Atomic Operations) for complete atomic semantics.

### 9.4.6.4 All Thread (`thread::*`)

**Grant**: `thread::*`

**Purpose**: Wildcard grant authorizing all thread operations.

**Expansion**: `thread::*` ≡ `{thread::spawn, thread::join, thread::atomic}`

---

## 9.4.7 FFI Grants (`ffi`)

### 9.4.7.1 FFI Call (`ffi::call`)

**Grant**: `ffi::call`

**Purpose**: Authorizes calling foreign (non-Cursive) functions.

**Operations Authorized**:
- Calling C functions
- Calling functions from dynamic libraries
- FFI boundary crossing

**Example (informative)**:
```cursive
external procedure c_function(x: i32): i32

procedure call_c(x: i32): i32
    sequent { [ffi::call] |- true => true }
{
    result c_function(x)
}
```

**Security Note**: FFI calls are inherently unsafe as they bypass Cursive's safety guarantees. This grant should be carefully restricted.

**Forward Reference**: See Part XIV (Interoperability and Unsafe) for complete FFI specification.

### 9.4.7.2 All FFI (`ffi::*`)

**Grant**: `ffi::*`

**Purpose**: Wildcard grant authorizing all FFI operations.

**Expansion**: Currently equivalent to `{ffi::call}`. Reserved for future FFI-related grants.

---

## 9.4.8 Unsafe Grants (`unsafe`)

### 9.4.8.1 Raw Pointer Operations (`unsafe::ptr`)

**Grant**: `unsafe::ptr`

**Purpose**: Authorizes unsafe raw pointer operations.

**Operations Authorized**:
- Dereferencing raw pointers
- Creating raw pointers from addresses
- Pointer arithmetic
- Unmanaged memory access

**Example (informative)**:
```cursive
procedure dereference_raw(ptr: *i32): i32
    sequent { [unsafe::ptr] |- true => true }
{
    unsafe {
        result *ptr
    }
}
```

**Security Note**: This grant authorizes operations that can violate memory safety.

**Forward Reference**: See Part VI §6.8 (Unsafe Operations) for complete unsafe semantics.

### 9.4.8.2 Type Transmutation (`unsafe::transmute`)

**Grant**: `unsafe::transmute`

**Purpose**: Authorizes reinterpreting bytes as different types.

**Operations Authorized**:
- Transmuting between types
- Reinterpreting memory representations
- Type punning

**Example (informative)**:
```cursive
procedure bits_to_float(bits: u32): f32
    sequent { [unsafe::transmute] |- true => true }
{
    unsafe {
        result transmute<u32, f32>(bits)
    }
}
```

**Security Note**: Type transmutation can violate type safety and lead to undefined behavior.

### 9.4.8.3 All Unsafe (`unsafe::*`)

**Grant**: `unsafe::*`

**Purpose**: Wildcard grant authorizing all unsafe operations.

**Expansion**: `unsafe::*` ≡ `{unsafe::ptr, unsafe::transmute}`

---

## 9.4.9 Panic Grant (`panic`)

### 9.4.9.1 Panic Operations

**Grant**: `panic`

**Purpose**: Authorizes controlled program termination via panic.

**Operations Authorized**:
- Explicit panic calls
- Assertion failures
- Unwinding initiation

**Example (informative)**:
```cursive
procedure checked_divide(a: i32, b: i32): i32
    sequent { [panic] |- true => true }
{
    if b == 0 {
        panic("Division by zero")
    }
    result a / b
}
```

**Rationale**: Panic is capability-restricted to enable building panic-free subsystems.

---

## 9.4.10 Grant Hierarchy Summary

**Complete Grant Hierarchy**:

```
alloc
├── heap
├── region
├── stack
├── temp
└── * (all allocation)

fs
├── read
├── write
├── delete
├── metadata
└── * (all file system)

net
├── read
├── write
├── connect
├── listen
└── * (all network)

time
├── read
├── sleep
├── monotonic
└── * (all time)

thread
├── spawn
├── join
├── atomic
└── * (all thread)

ffi
├── call
└── * (all FFI)

unsafe
├── ptr
├── transmute
└── * (all unsafe)

panic (no subcategories)
```

---

## 9.4.11 Grant Subsumption

**Definition 9.4 (Grant Subsumption)**: Grant g₁ subsumes grant g₂ if every operation authorized by g₂ is also authorized by g₁.

**Subsumption Relations**:

| Grant | Subsumes |
|-------|----------|
| `alloc::*` | `alloc::heap`, `alloc::region`, `alloc::stack`, `alloc::temp` |
| `fs::*` | `fs::read`, `fs::write`, `fs::delete`, `fs::metadata` |
| `net::*` | `net::read`, `net::write`, `net::connect`, `net::listen` |
| `time::*` | `time::read`, `time::sleep`, `time::monotonic` |
| `thread::*` | `thread::spawn`, `thread::join`, `thread::atomic` |
| `ffi::*` | `ffi::call` |
| `unsafe::*` | `unsafe::ptr`, `unsafe::transmute` |

**Formal Rule**: If g₁ subsumes g₂, then `{g₁} <: {g₁}` and `{g₂} <: {g₁}`.

---

## 9.4.12 Conformance Requirements

A conforming implementation shall:

1. **Recognize all built-in grants** (§9.4.1): Accept and enforce all grants specified in this section
2. **Enforce grant requirements** (§9.4.2-§9.4.9): Require appropriate grants for operations
3. **Expand wildcards** (§9.4.10): Correctly expand wildcard grants to their constituent grants
4. **Verify subsumption** (§9.4.11): Recognize subsumption relations in subset checking

---

**Previous**: [09-3_Grant-Polymorphism.md](09-3_Grant-Polymorphism.md) | **Next**: [09-5_User-Defined-Grants.md](09-5_User-Defined-Grants.md)
