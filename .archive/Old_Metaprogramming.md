# The Cursive Language Specification

**Part**: XI — Metaprogramming and Reflection  
**File**: 11_Metaprogramming.md  
**Previous**: [Modal Types](10_Modals.md) | **Next**: [Modules, Packages, and Imports](12_Modules-Packages-and-Imports.md)

---

## Abstract

This chapter specifies Cursive's **metaprogramming and reflection** system, a compile-time and optional runtime introspection facility designed for systems programming, game development, and code generation. The system provides:

- **Compile-time reflection**: Type introspection, layout queries, and metadata access during compilation
- **Comptime execution**: Pure Turing-complete computation at compile time with controlled termination
- **Code generation**: Programmatic creation of types, functions, and procedures through explicit APIs
- **Runtime reflection**: Optional type information for dynamic dispatch, serialization, and debugging
- **Zero-cost abstraction**: Metaprogramming features incur no runtime overhead when unused

Cursive's metaprogramming follows the language's core principles: **no macros**, **explicit over implicit**, **determinism**, **local reasoning**, and **AI-friendliness**. All metaprogramming uses regular function calls—no special macro syntax. All comptime code is pure (no unrestricted I/O or effects) and terminates within implementation-defined limits.

**Key Innovation**: Unlike macro-based systems (C++ templates, Rust macros), Cursive metaprogramming uses **ordinary functions** executed at compile time, with **explicit APIs** for code generation. This makes metaprogramming code as readable and debuggable as regular code.

**Design Goals:**

1. **Game engine support**: ECS code generation, asset pipeline, shader compilation
2. **Serialization**: Derive encoding/decoding for any type
3. **Testing frameworks**: Mock generation, property-based testing
4. **DSL embedding**: Domain-specific languages with compile-time validation
5. **Performance optimization**: Compile-time lookup tables, state machines

---

## 11.0 Foundations

### 11.0.1 Scope

This chapter provides the complete normative specification for:

- **Comptime functions and blocks** (§11.1): Pure compile-time execution contexts
- **Type reflection** (§11.2): Introspection of type structure, layout, and metadata
- **Code generation** (§11.3): Programmatic creation of declarations through APIs
- **Compile-time type functions** (§11.4): Functions that compute and return types
- **Runtime reflection** (§11.5): Optional type information for dynamic introspection
- **Metaprogramming patterns** (§11.6): Common idioms and best practices
- **Integration** (§11.7): Interaction with permissions, effects, modals, and unsafe code

### 11.0.2 Cross-Part Dependencies

This chapter integrates with other parts as follows:

**Part I (Foundations):**
- §1.3: Metavariables and notational conventions
- §1.4: Judgment forms
- §4: Attributes (§4.3 for [[reflect]] attribute)

**Part II (Type System):**
- §2.0: Type grammar and well-formedness rules
- §2.3: Modal types and state machines
- §2.10: Function types and signatures
- §2.13: Witnesses and dynamic dispatch

**Part III (Declarations and Scope):**
- §3.3: Compile-time evaluation contexts
- §3.1: Declaration forms and visibility

**Part VII (Contracts and Effects):**
- §7.3: Effect clauses (`uses`)
- §7.4: Precondition clauses (`must`)
- §7.5: Postcondition clauses (`will`)

**Part IX (Functions and Procedures):**
- §9.2.9: Comptime functions (extended here)
- §9.0: Callable semantics

**Part XIII (Memory, Permissions, and Concurrency):**
- §13.2: Memory layout and alignment
- §13.3: Size and offset calculations

CITE: Part I §1.3 — Metavariables; Part II §2.0 — Type System; Part III §3.3 — Compile-Time Evaluation; Part VII §7.3 — Effect Clauses; Part IX §9.2.9 — Comptime Functions; Part XIII §13.2 — Memory Layout.

### 11.0.3 Design Principles

Cursive's metaprogramming system adheres to core language principles:

1. **Explicit over implicit**: All metaprogramming is marked with `comptime` keyword
2. **Deterministic**: Compilation order and evaluation are predictable
3. **Pure with controlled effects**: Comptime code uses explicit comptime effect system
4. **Terminating**: All comptime computation must terminate or compilation fails
5. **Zero-cost when unused**: Opting out of reflection incurs zero overhead
6. **Local reasoning**: Metaprogramming code is readable without global context
7. **No macros**: Code generation uses functions, not macro syntax

### 11.0.4 Notational Conventions

**Metavariables:**

```
τ, σ, ρ ∈ Type           (types)
T, U, V ∈ TypeExpr        (type expressions)
e ∈ Expr                 (expressions)
d ∈ Decl                 (declarations)
```

**Judgment Forms:**

```
Γ_ct ⊢ e : τ                (comptime type checking)
Γ_ct ⊢ e ⇓ v               (comptime evaluation)
⊢ reflect(τ) : TypeInfo    (type reflection)
⊢ codegen(spec) : d        (code generation)
```

---

## 11.1 Comptime Execution

### 11.1.1 Comptime Functions (Extended)

This section extends §9.2.9 with advanced comptime function capabilities.

**Definition 11.1 (Comptime Function):** A comptime function is a pure, deterministic function that executes during compilation and produces compile-time constants. Comptime functions support:

- All compile-time evaluable expressions
- Compile-time control flow (if, loop, match)
- Compile-time data structures (arrays, tuples, records)
- Recursion (within depth limits)
- Type-level computation

CITE: Part IX §9.2.9 — Comptime Functions.

#### Extended Capabilities

**Comptime data structures:**

```cursive
comptime function build_lookup_table<const N: usize>(): [u8; N] {
    var table = [0u8; N]
    loop i in 0..N {
        table[i] = ((i * i) % 256) as u8
    }
    result table
}

const SQUARES: [u8; 256] = build_lookup_table::<256>()
```

**Comptime string manipulation:**

```cursive
comptime function concat_all(strings: [string]): string {
    var result = string::from("")
    loop s in strings {
        result::append(s)
    }
    result result
}

const MESSAGE: string = concat_all([
    "Hello, ",
    "world!",
])
```

**Comptime assertions:**

```cursive
comptime function validate_config<const SIZE: usize>(): () {
    comptime_assert(SIZE > 0, "SIZE must be positive")
    comptime_assert(SIZE <= 65536, "SIZE too large")
    comptime_assert(is_power_of_two(SIZE), "SIZE must be power of 2")
    result ()
}

// Validated at compile time
type Buffer<const N: usize> = {
    let _ = validate_config::<N>()
    [u8; N]
}
```

### 11.1.2 Comptime Blocks (Extended)

This section extends comptime block syntax from §3.3 and §6.15.

**Definition 11.2 (Comptime Block):** A comptime block executes during compilation and may:
- Perform compile-time computation
- Generate code through explicit APIs
- Query type information
- Declare compile-time constants

CITE: §3.3 — Compile-Time Evaluation; §6.15 — Effect-Gated Compilation.

#### Static Code Generation

```cursive
// Generate functions for all primitive types
comptime {
    let types = [i8, i16, i32, i64, u8, u16, u32, u64]
    var specs = Vec::new()

    loop ty in types {
        let fn_name = string::concat("max_", type_name_of(ty))
        
        specs::push(FunctionSpec {
            name: fn_name,
            visibility: Visibility::Public,
            params: [
                ParamSpec { 
                    name: "a", 
                    ty: TypeRef::Named(type_name_of(ty)), 
                    permission: Permission::Imm 
                },
                ParamSpec { 
                    name: "b", 
                    ty: TypeRef::Named(type_name_of(ty)), 
                    permission: Permission::Imm 
                },
            ],
            return_type: TypeRef::Named(type_name_of(ty)),
            uses_clause: EffectSet::empty(),
            must_clause: ContractClause::None,
            will_clause: ContractClause::None,
            body: quote {
                result if a > b { a } else { b }
            },
        })
    }
    
    loop spec in specs {
        codegen::declare_function(spec)
    }
}

// Generated: max_i8, max_i16, max_i32, max_i64, max_u8, max_u16, max_u32, max_u64
let x = max_i32(10, 20)
```

#### Conditional Compilation

```cursive
procedure log_debug(msg: string)
    uses io.write
{
    comptime {
        if cfg("debug_assertions") {
            // Generate debug logging code
            codegen::insert_statement(quote {
                println("[DEBUG] {}", msg)
            })
        }
    }
}
```

#### Comptime Variables

Variables declared in comptime blocks are compile-time constants:

```cursive
comptime {
    let max_size = if cfg("target_arch") == "x86_64" {
        result 8192
    } else {
        result 4096
    }
}

type CacheBuffer = [u8; max_size]
```

### 11.1.3 Comptime Intrinsics

**Definition 11.3 (Comptime Intrinsics):** The compiler provides built-in comptime-only functions for metaprogramming.

#### Assertion and Validation

```cursive
comptime function comptime_assert(condition: bool, message: string): ()
comptime function comptime_error(message: string): !
comptime function comptime_warning(message: string): ()
comptime function comptime_note(message: string): ()
```

**Static semantics:**

```
[CT-Assert]
Γ_ct ⊢ condition : bool
Γ_ct ⊢ message : string
condition evaluates to true at compile time
-------------------------------------------
Γ_ct ⊢ comptime_assert(condition, message) : ()

[CT-Assert-Fail]
Γ_ct ⊢ condition : bool
Γ_ct ⊢ message : string
condition evaluates to false at compile time
-------------------------------------------
ERROR E11CT-0101: Comptime assertion failed: message
```

**Examples:**

```cursive
comptime function validate_size<const N: usize>(): () {
    comptime_assert(N > 0, "Size must be positive")
    comptime_assert(N <= 4096, "Size too large")
}

comptime {
    if !cfg("feature_enabled") {
        comptime_error("This code requires feature_enabled")
    }
    
    if cfg("deprecated_api") {
        comptime_warning("deprecated_api is deprecated, use new_api instead")
    }
    
    comptime_note("Compiling with optimization level 3")
}
```

#### Configuration Queries

```cursive
comptime function cfg(key: string): bool
comptime function cfg_value(key: string): string
comptime function target_os(): string
comptime function target_arch(): string
comptime function target_endian(): string  // "little" | "big"
comptime function target_pointer_width(): usize  // 32 | 64
```

#### Type Information Queries

```cursive
comptime function type_name_of<T>(): string
comptime function type_id_of<T>(): u64
comptime function size_of<T>(): usize
comptime function align_of<T>(): usize
comptime function is_copy<T>(): bool
comptime function is_send<T>(): bool
comptime function is_sync<T>(): bool
comptime function field_count_of<T>(): usize
comptime function has_field<T>(field_name: string): bool
comptime function field_type_of<T>(field_name: string): Option<TypeInfo>
comptime function field_offset<T>(field_name: string): Option<usize>
```

#### Unique Name Generation

**Definition 11.3.1 (Hygiene Support):** The `gensym` intrinsic generates unique identifiers for hygienic code generation.

```cursive
comptime function gensym(prefix: string): string
    uses comptime.alloc
```

**Semantics**: Returns a unique identifier string of the form `prefix_gNNNNN` where `NNNNN` is a unique numeric suffix.

**Example:**

```cursive
comptime {
    let buffer_name = gensym("buffer")  // Returns "buffer_g12345"
    let temp_name = gensym("temp")      // Returns "temp_g12346"
}
```

CITE: §11.3.7.4 — Hygiene and Name Capture Prevention.

#### Performance Profiling

**Definition 11.3.2 (Comptime Profiling):** Profiling intrinsics measure compile-time performance characteristics.

```cursive
comptime function comptime_profile_start(name: string): ProfileHandle
    uses comptime.alloc

comptime function comptime_profile_end(handle: ProfileHandle): ProfileStats

record ProfileHandle {
    id: u64
    name: string
}

record ProfileStats {
    steps: usize
    memory_bytes: usize
    duration_ms: f64
}
```

**Example:**

```cursive
comptime {
    let handle = comptime_profile_start("code_generation")
    
    loop i in 0..1000 {
        generate_function_for_number(i)
    }
    
    let stats = comptime_profile_end(handle)
    
    if stats.steps > 500000 {
        comptime_warning(string::format(
            "Code generation used {} steps (consider optimization)",
            [stats.steps]
        ))
    }
    
    comptime_note(string::format(
        "Generation completed in {:.2}ms",
        [stats.duration_ms]
    ))
}
```

#### Termination and Resource Limits

**NORMATIVE REQUIREMENTS:**

A conforming implementation MUST enforce the following minimum limits for comptime evaluation:

1. **Recursion depth**: Minimum 256 levels of recursion
2. **Evaluation steps**: Minimum 1,000,000 evaluation steps per comptime block
3. **Memory allocation**: Minimum 64 MB per compilation unit for comptime data
4. **String operations**: Minimum 1 MB for individual string values
5. **Collection size**: Minimum 10,000 elements in comptime arrays/vectors

**NORMATIVE RECOMMENDATION:**

Implementations SHOULD issue a warning when a single comptime block exceeds:
- 10 seconds of evaluation time
- 50% of the maximum evaluation steps
- 50% of the maximum memory allocation

**Diagnostic:**

```
warning[E11CT-0108]: comptime evaluation time excessive
  --> src/main.cur:42:1
   |
42 | comptime {
   | ^^^^^^^^^^ this comptime block has been evaluating for 12.4 seconds
   |
   = note: consider optimizing or splitting into multiple blocks
   = help: use --comptime-time-limit to adjust the warning threshold
```

**Static semantics:**

```
[CT-Limits]
comptime block b
eval_steps(b) > LIMIT_STEPS  ∨  
recursion_depth(b) > LIMIT_DEPTH  ∨
memory_usage(b) > LIMIT_MEMORY
----------------------------------
ERROR E11CT-0103/0104/0105: Comptime limit exceeded
```

### 11.1.4 Comptime Effect Model

**Definition 11.4 (Comptime Effects):** Comptime code operates under a restricted effect system that permits only compile-time-safe operations.

#### Standard Comptime Effects

```cursive
// Comptime memory allocation (arena-based)
effect comptime.alloc {
    operation allocate(size: usize): *u8
    operation deallocate(ptr: *u8): ()
}

// Comptime configuration access
effect comptime.config {
    operation read_config(key: string): Option<string>
    operation read_env(key: string): Option<string>
}

// Comptime code generation
effect comptime.codegen {
    operation emit_declaration(decl: DeclSpec): ()
    operation insert_statement(stmt: QuotedBlock): ()
}

// Comptime diagnostics
effect comptime.diag {
    operation emit_error(message: string): !
    operation emit_warning(message: string): ()
    operation emit_note(message: string): ()
}
```

#### Effect Requirements

**All comptime functions MUST declare their effects explicitly:**

```cursive
comptime function build_config(): Config
    uses comptime.config, comptime.alloc
{
    let host = cfg_value("build.host")
    let port = cfg_value("build.port")
    result Config { host, port }
}

comptime function generate_boilerplate<T>(): ()
    uses comptime.alloc, comptime.codegen
{
    let type_name = type_name_of::<T>()
    // Codegen operations use comptime.codegen
    codegen::declare_function(FunctionSpec { /* ... */ })
}
```

**Static semantics:**

```
[CT-Effect-Check]
comptime function f : (...) → τ uses ε_decl
f_body uses effects ε_body
ε_body ⊆ ε_decl ∪ COMPTIME_EFFECTS
------------------------------------
Γ_ct ⊢ f : well-formed

where COMPTIME_EFFECTS = {
    comptime.alloc,
    comptime.config,
    comptime.codegen,
    comptime.diag
}
```

#### Prohibited Operations

The following runtime effects are **prohibited** in comptime contexts:

```cursive
// ❌ ERROR: Cannot use runtime I/O effects
comptime function read_file(): string
    uses fs.read  // ERROR E11CT-0201: Runtime effect in comptime context
{
    fs::read_to_string("data.txt")
}

// ❌ ERROR: Cannot use runtime allocation
comptime function make_heap_vec(): Vec<i32>
    uses alloc.heap  // ERROR E11CT-0201: Runtime effect in comptime context
{
    Vec::new()  // Uses alloc.heap
}

// ✅ OK: Use comptime.alloc instead
comptime function make_comptime_vec(): Vec<i32>
    uses comptime.alloc
{
    Vec::new()  // Uses comptime.alloc
}
```

**Error diagnostic:**

```
[CT-Runtime-Effect-Prohibited]
comptime function f uses ε
ε ∩ RUNTIME_EFFECTS ≠ ∅
------------------------------------
ERROR E11CT-0201: Runtime effect prohibited in comptime context
```

---

## 11.2 Type Reflection

### 11.2.1 Reflection Opt-In

**Definition 11.5 (Reflection Opt-In):** Type reflection is opt-in per type via the `[[reflect]]` attribute. Types without this attribute cannot be reflected upon.

```cursive
[[reflect]]
record Point {
    x: f64
    y: f64
}

// Reflection queries now work for Point
comptime {
    let size = size_of::<Point>()        // OK
    let fields = fields_of::<Point>()    // OK
}

record Private {
    data: i32
}

comptime {
    // let size = size_of::<Private>()     // ERROR: Private not marked [[reflect]]
    // let fields = fields_of::<Private>() // ERROR: Private not marked [[reflect]]
}
```

**Rationale:**

- **Zero-cost when unused**: Types without `[[reflect]]` have no metadata overhead
- **Explicit opt-in**: Prevents accidental exposure of internal structure
- **Security**: Sensitive types can opt out of reflection
- **Binary size**: Only reflected types contribute metadata to binary

**Static semantics:**

```
[Reflect-Opt-In]
type T declared with [[reflect]] attribute
⊢ reflect(T) : permitted

[Reflect-Opt-Out]
type T declared without [[reflect]] attribute
comptime code attempts reflect(T)
------------------------------------
ERROR E11RF-0101: Type T not marked [[reflect]]
```

### 11.2.2 Type Information API

**Definition 11.6 (TypeInfo):** The `TypeInfo` record provides compile-time introspection of type structure.

```cursive
record TypeInfo {
    name: string
    size: usize
    align: usize
    kind: TypeKind
}

enum TypeKind {
    Primitive
    Record
    Enum
    Union
    Modal
    Tuple
    Array
    Slice
    Pointer
    Function
}

comptime function reflect_type<T>(): TypeInfo
    uses comptime.alloc
```

**Example:**

```cursive
[[reflect]]
record Vector3 {
    x: f32
    y: f32
    z: f32
}

comptime {
    let info = reflect_type::<Vector3>()
    comptime_assert(info.name == "Vector3", "Type name")
    comptime_assert(info.size == 12, "Size of 3 f32s")
    comptime_assert(info.align == 4, "f32 alignment")
    comptime_assert(info.kind == TypeKind::Record, "Is a record")
}
```

#### Memory Layout Validation

**Pattern for validating GPU buffer layouts:**

```cursive
[[repr(C, packed)]]
[[reflect]]
record GpuVertex {
    position: [f32; 3]    // 12 bytes, offset 0
    normal: [f32; 3]      // 12 bytes, offset 12
    uv: [f32; 2]          // 8 bytes, offset 24
}

comptime {
    // Validate at compile time
    comptime_assert(
        size_of::<GpuVertex>() == 32,
        "Vertex must be exactly 32 bytes"
    )
    
    comptime_assert(
        align_of::<GpuVertex>() == 1,
        "Packed struct must have alignment 1"
    )
    
    let uv_offset = field_offset::<GpuVertex>("uv")
    comptime_assert(
        uv_offset == Some(24),
        "UV field must be at offset 24"
    )
    
    comptime_note("GPU vertex layout validated at compile time")
}
```

### 11.2.3 Field Reflection

```cursive
record FieldInfo {
    name: string
    ty_name: string
    offset: usize
    visibility: Visibility
}

comptime function fields_of<T>(): [FieldInfo]
    uses comptime.alloc

comptime function has_field<T>(name: string): bool

comptime function field_info_of<T>(name: string): Option<FieldInfo>
    uses comptime.alloc
```

**Examples:**

```cursive
[[reflect]]
record Person {
    public name: string
    public age: u32
    private ssn: string
}

comptime {
    let fields = fields_of::<Person>()
    comptime_assert(fields.len() == 3, "Expected 3 fields")
    
    comptime_assert(has_field::<Person>("name"), "Has name field")
    comptime_assert(has_field::<Person>("age"), "Has age field")
    comptime_assert(!has_field::<Person>("unknown"), "No unknown field")
    
    let name_field = field_info_of::<Person>("name")
    match name_field {
        Some(info) => {
            comptime_assert(info.ty_name == "string", "name is string")
            comptime_assert(info.visibility == Visibility::Public, "name is public")
        },
        None => comptime_error("name field should exist"),
    }
}
```

### 11.2.4 Procedure Reflection

```cursive
record ProcedureInfo {
    name: string
    receiver_permission: Permission
    params: [ParamInfo]
    return_type: string
    uses_effects: [string]
}

record ParamInfo {
    name: string
    ty_name: string
    permission: Permission
}

comptime function procedures_of<T>(): [ProcedureInfo]
    uses comptime.alloc

comptime function has_procedure<T>(name: string): bool

comptime function procedure_info_of<T>(name: string): Option<ProcedureInfo>
    uses comptime.alloc
```

**Examples:**

```cursive
[[reflect]]
record Counter {
    value: i32
    
    procedure increment(self: mut Counter) {
        self.value += 1
    }
    
    procedure get(self: Counter): i32 {
        result self.value
    }
}

comptime {
    let procedures = procedures_of::<Counter>()
    comptime_assert(procedures.len() == 2, "Expected 2 procedures")
    
    comptime_assert(has_procedure::<Counter>("increment"), "Should have increment")
    
    let get_info = procedure_info_of::<Counter>("get")
    match get_info {
        Some(info) => {
            comptime_assert(info.receiver_permission == Permission::Imm, "get is immutable")
            comptime_assert(info.return_type == "i32", "get returns i32")
        },
        None => comptime_error("get procedure not found"),
    }
}
```

### 11.2.5 Enum Reflection

```cursive
record VariantInfo {
    name: string
    discriminant: Option<i64>
    payload: Option<[string]>  // Type names of payload fields
}

comptime function variants_of<T>(): [VariantInfo]
comptime function has_variant<T>(name: string): bool
comptime function variant_info_of<T>(name: string): Option<VariantInfo>
```

**Examples:**

```cursive
[[reflect]]
enum Result<T, E> {
    Ok(T)
    Err(E)
}

comptime {
    let variants = variants_of::<Result<i32, string>>()
    comptime_assert(variants.len() == 2, "Expected 2 variants")
    
    let ok_variant = variant_info_of::<Result<i32, string>>("Ok")
    match ok_variant {
        Some(info) => {
            comptime_assert(info.name == "Ok", "Variant name")
            match info.payload {
                Some(types) => comptime_assert(types.len() == 1, "Ok has one payload"),
                None => comptime_error("Ok should have payload"),
            }
        },
        None => comptime_error("Ok variant not found"),
    }
}
```

### 11.2.6 Modal State Reflection

```cursive
record StateInfo {
    name: string
    fields: [FieldInfo]
}

record TransitionInfo {
    procedure_name: string
    from_state: string
    to_state: string
}

comptime function states_of<T>(): [StateInfo]
comptime function transitions_of<T>(): [TransitionInfo]
```

**Examples:**

```cursive
[[reflect]]
modal File {
    @Closed { path: string }
    @Open { path: string, handle: Handle }
    
    procedure open(self: own File@Closed): Result<File@Open, Error> {
        // Implementation
    }
    
    procedure close(self: own File@Open): File@Closed {
        // Implementation
    }
}

comptime {
    let states = states_of::<File>()
    comptime_assert(states.len() == 2, "Expected 2 states")
    
    let transitions = transitions_of::<File>()
    comptime_assert(transitions.len() == 2, "Expected 2 transitions")
    
    loop trans in transitions {
        comptime_note(string::format(
            "Transition: {} -> {}",
            [trans.from_state, trans.to_state]
        ))
    }
}
```

---

## 11.3 Code Generation

### 11.3.1 Code Generation Model

**Definition 11.9 (Code Generation):** Code generation in Cursive uses explicit API calls within comptime blocks to programmatically create declarations.

**Key Principles:**

1. **Explicit side effects**: All code generation uses functions marked with `comptime.codegen` effect
2. **Type-safe specifications**: Generated code is specified using structured types, not strings
3. **Validation**: Generated code is type-checked like hand-written code
4. **Deterministic**: Code generation order is deterministic and predictable
5. **Composable**: Generated code can reference other generated code

**Code generation workflow:**

```
1. comptime block executes
   ↓
2. Build specification structures (FunctionSpec, TypeSpec, etc.)
   ↓
3. Call codegen::* functions (uses comptime.codegen effect)
   ↓
4. Compiler validates and emits declarations
   ↓
5. Generated code available for type checking
```

### 11.3.2 Function Generation

```cursive
record FunctionSpec {
    name: string
    visibility: Visibility
    generics: [GenericParam]
    params: [ParamSpec]
    return_type: TypeRef
    uses_clause: EffectSet
    must_clause: ContractClause
    will_clause: ContractClause
    body: QuotedBlock
}

record ParamSpec {
    name: string
    ty: TypeRef
    permission: Permission
}

enum GenericParam {
    Type { name: string, bounds: [string] }
    Const { name: string, ty: TypeRef }
    Effect { name: string }
}

comptime function codegen::declare_function(spec: FunctionSpec): ()
    uses comptime.codegen
```

**Static semantics:**

```
[Codegen-Function]
Γ_ct ⊢ spec : FunctionSpec
spec.name not already declared in current scope
all types in spec well-formed
-------------------------------------------
⊢ codegen::declare_function(spec) generates function declaration
  function spec.name(params): return_type
    uses spec.uses_clause
    must spec.must_clause
    will spec.will_clause
  { spec.body }

[Codegen-Name-Collision]
Γ_ct ⊢ spec : FunctionSpec
spec.name already declared in current scope
-------------------------------------------
ERROR E11CG-0102: Name collision
```

**Example:**

```cursive
comptime function generate_max_for_type(ty: TypeRef): ()
    uses comptime.codegen
{
    let type_name = match ty {
        TypeRef::Named(name) => name,
        _ => comptime_error("Expected named type"),
    }
    
    codegen::declare_function(FunctionSpec {
        name: string::format("max_{}", [type_name]),
        visibility: Visibility::Public,
        generics: [],
        params: [
            ParamSpec { name: "a", ty: ty, permission: Permission::Imm },
            ParamSpec { name: "b", ty: ty, permission: Permission::Imm },
        ],
        return_type: ty,
        uses_clause: EffectSet::empty(),
        must_clause: ContractClause::None,
        will_clause: ContractClause::None,
        body: quote {
            result if a > b { a } else { b }
        },
    })
}

comptime {
    generate_max_for_type(TypeRef::Named("i32"))
    generate_max_for_type(TypeRef::Named("f64"))
}

// Generated functions available:
let x = max_i32(10, 20)
let y = max_f64(1.5, 2.5)
```

### 11.3.3 Type Generation

**Definition 11.10 (Type Generation):** Types are generated as complete declarations including all fields and procedures.

```cursive
enum TypeKind {
    Record
    Enum
    Union
    TupleStruct
}

record FieldSpec {
    name: string
    ty: TypeRef
    visibility: Visibility
}

record ProcedureSpec {
    name: string
    visibility: Visibility
    receiver: ParamSpec
    params: [ParamSpec]
    return_type: TypeRef
    uses_clause: EffectSet
    must_clause: ContractClause
    will_clause: ContractClause
    body: QuotedBlock
}

record FunctionSpec {
    name: string
    visibility: Visibility
    generics: [GenericParam]
    params: [ParamSpec]
    return_type: TypeRef
    uses_clause: EffectSet
    must_clause: ContractClause
    will_clause: ContractClause
    body: QuotedBlock
}

record VariantSpec {
    name: string
    payload: Option<[TypeRef]>
}

record TypeSpec {
    kind: TypeKind
    name: string
    visibility: Visibility
    generics: [GenericParam]
    fields: [FieldSpec]
    procedures: [ProcedureSpec]
    functions: [FunctionSpec]
    variants: [VariantSpec]
}

comptime function codegen::declare_type(spec: TypeSpec): ()
    uses comptime.codegen
```

**Static semantics:**

```
[Codegen-Type-Record]
Γ_ct ⊢ spec : TypeSpec
spec.kind == TypeKind::Record
spec.name not already declared
all field types well-formed
all procedure bodies well-formed in record context
-------------------------------------------
⊢ codegen::declare_type(spec) generates:

record spec.name {
    spec.fields...
    
    spec.procedures...
    
    spec.functions...
}

[Codegen-Type-Enum]
Γ_ct ⊢ spec : TypeSpec
spec.kind == TypeKind::Enum
spec.name not already declared
all variant payload types well-formed
-------------------------------------------
⊢ codegen::declare_type(spec) generates:

enum spec.name {
    spec.variants...
}
```

**Example: Complete Record Generation**

```cursive
comptime function generate_config_type(): ()
    uses comptime.codegen
{
    codegen::declare_type(TypeSpec {
        kind: TypeKind::Record,
        name: "ServerConfig",
        visibility: Visibility::Public,
        generics: [],
        fields: [
            FieldSpec { 
                name: "host", 
                ty: TypeRef::Named("string"), 
                visibility: Visibility::Public 
            },
            FieldSpec { 
                name: "port", 
                ty: TypeRef::Named("u16"), 
                visibility: Visibility::Public 
            },
            FieldSpec { 
                name: "timeout", 
                ty: TypeRef::Named("Duration"), 
                visibility: Visibility::Public 
            },
        ],
        procedures: [
            ProcedureSpec {
                name: "is_valid",
                visibility: Visibility::Public,
                receiver: ParamSpec {
                    name: "self",
                    ty: TypeRef::SelfType,
                    permission: Permission::Imm,
                },
                params: [],
                return_type: TypeRef::Named("bool"),
                uses_clause: EffectSet::empty(),
                must_clause: ContractClause::None,
                will_clause: ContractClause::None,
                body: quote {
                    result self.port > 0 && self.port < 65536
                },
            },
        ],
        functions: [
            FunctionSpec {
                name: "default",
                visibility: Visibility::Public,
                generics: [],
                params: [],
                return_type: TypeRef::Named("ServerConfig"),
                uses_clause: EffectSet::empty(),
                must_clause: ContractClause::None,
                will_clause: ContractClause::None,
                body: quote {
                    result ServerConfig {
                        host: "localhost",
                        port: 8080,
                        timeout: Duration::seconds(30),
                    }
                },
            },
        ],
        variants: [],
    })
}

comptime { generate_config_type() }

// Generated type available:
let config = ServerConfig::default()
comptime_assert(config::is_valid(), "Default config is valid")
```

### 11.3.4 Extending Existing Types

**Definition 11.11 (Type Extension):** Procedures and functions can be added to existing types after declaration.

```cursive
comptime function codegen::add_procedure(
    target: TypeRef,
    spec: ProcedureSpec
): ()
    uses comptime.codegen

comptime function codegen::add_function(
    target: TypeRef,
    spec: FunctionSpec
): ()
    uses comptime.codegen
```

**Static semantics:**

```
[Codegen-Add-Procedure]
Γ_ct ⊢ target : TypeRef
target refers to existing record type T
spec.name not already declared on T
spec.receiver.ty compatible with T
-------------------------------------------
⊢ codegen::add_procedure(target, spec) 
  adds procedure to T's declaration

[Codegen-Add-Function]
Γ_ct ⊢ target : TypeRef
target refers to existing type T
spec.name not already declared on T
-------------------------------------------
⊢ codegen::add_function(target, spec)
  adds associated function to T's declaration
```

**Example:**

```cursive
[[reflect]]
record Point {
    x: f64
    y: f64
}

comptime function add_distance_procedure(): ()
    uses comptime.codegen
{
    codegen::add_procedure(
        target: TypeRef::Named("Point"),
        spec: ProcedureSpec {
            name: "distance",
            visibility: Visibility::Public,
            receiver: ParamSpec {
                name: "self",
                ty: TypeRef::SelfType,
                permission: Permission::Imm,
            },
            params: [
                ParamSpec {
                    name: "other",
                    ty: TypeRef::Named("Point"),
                    permission: Permission::Imm,
                },
            ],
            return_type: TypeRef::Named("f64"),
            uses_clause: EffectSet::empty(),
            must_clause: ContractClause::None,
            will_clause: ContractClause::None,
            body: quote {
                let dx = self.x - other.x
                let dy = self.y - other.y
                result ((dx * dx) + (dy * dy)).sqrt()
            },
        }
    )
}

comptime { add_distance_procedure() }

// Now Point has distance procedure:
let p1 = Point { x: 0.0, y: 0.0 }
let p2 = Point { x: 3.0, y: 4.0 }
let dist = p1::distance(p2)  // 5.0
```

### 11.3.5 Statement and Constant Generation

```cursive
comptime function codegen::insert_statement(stmt: QuotedBlock): ()
    uses comptime.codegen

comptime function codegen::declare_constant(
    name: string,
    ty: TypeRef,
    value: QuotedExpr
): ()
    uses comptime.codegen
```

**Static semantics:**

```
[Codegen-Insert-Statement]
Γ_ct ⊢ stmt : QuotedBlock
current context allows statement insertion
-------------------------------------------
⊢ codegen::insert_statement(stmt) 
  inserts stmt at current position

[Codegen-Constant]
Γ_ct ⊢ name : string
Γ_ct ⊢ ty : TypeRef
Γ_ct ⊢ value : QuotedExpr
value evaluates to constant of type ty
-------------------------------------------
⊢ codegen::declare_constant(name, ty, value)
  generates: const name: ty = value
```

**Examples:**

```cursive
comptime {
    // Insert debugging statement
    if cfg("debug_mode") {
        codegen::insert_statement(quote {
            println("Debug checkpoint reached")
        })
    }
    
    // Generate constant
    codegen::declare_constant(
        name: "MAX_CONNECTIONS",
        ty: TypeRef::Named("usize"),
        value: quote { 1000 }
    )
}
```

### 11.3.6 Attribute Generation

```cursive
record AttributeSpec {
    name: string
    args: [(string, string)]
}

comptime function codegen::add_attribute(
    target: TypeRef,
    attr: AttributeSpec
): ()
    uses comptime.codegen
```

**Static semantics:**

```
[Codegen-Attribute]
Γ_ct ⊢ target : TypeRef
Γ_ct ⊢ attr : AttributeSpec
target refers to existing type or generated type
-------------------------------------------
⊢ codegen::add_attribute(target, attr) 
  adds [[attr.name(...)]] to target
```

**Example:**

```cursive
comptime function mark_as_component<T>(): ()
    uses comptime.codegen
{
    codegen::add_attribute(
        target: TypeRef::TypeId(type_id_of::<T>()),
        attr: AttributeSpec {
            name: "component",
            args: [],
        }
    )
}

record Transform {
    position: Vec3
    rotation: Quat
}

comptime { mark_as_component::<Transform>() }
// Equivalent to: [[component]] record Transform { ... }
```

### 11.3.7 Quote Expressions and Interpolation

**Definition 11.12 (Quote Expression):** A quote expression captures code as data for later emission.

```cursive
comptime function quote(block: BlockExpr): QuotedBlock
comptime function quote(expr: Expr): QuotedExpr
```

#### Quote Syntax

```ebnf
QuoteExpr       ::= "quote" BlockExpr
                 | "quote" Expr

(* Inside quote blocks, use #(...) for interpolation *)
Interpolation   ::= "#" "(" Expr ")"
```

**CRITICAL NOTE**: The interpolation syntax is `#(...)` NOT `$(...)`. The `$` glyph is reserved for self parameters (see Appendix A.6: `SelfParam ::= "$" | ...`). The `#` glyph is unused elsewhere in Cursive and provides clear visual distinction for metaprogramming interpolation.

#### Interpolation Rules

**Within a `quote` block:**

1. **Regular code** is captured literally
2. **Interpolations `#(expr)`** are evaluated at compile time and spliced into the generated code
3. **Interpolated identifiers** `#(ident_expr)` become actual identifiers in the generated code

**Static semantics:**

```
[Quote-Block]
Γ_ct ⊢ block : BlockExpr
block contains no interpolations
-------------------------------
Γ_ct ⊢ quote block : QuotedBlock

[Quote-Interpolation]
Γ_ct ⊢ block : BlockExpr with interpolations #(e₁), ..., #(eₙ)
Γ_ct ⊢ eᵢ : τᵢ for all i
each eᵢ evaluates to compile-time constant
-------------------------------
Γ_ct ⊢ quote block : QuotedBlock with interpolations resolved
```

**Examples:**

```cursive
// Simple quote without interpolation
comptime {
    let simple_body = quote {
        result x * 2
    }
}

// Quote with value interpolation
comptime {
    let multiplier = 10
    let body_with_value = quote {
        result x * #(multiplier)  // Interpolates 10
    }
    // Generates: result x * 10
}

// Quote with identifier interpolation
comptime {
    let field_name = "position"
    let getter_body = quote {
        result self.#(field_name)  // Interpolates position as identifier
    }
    // Generates: result self.position
}

// Complex interpolation
comptime function generate_getter<T>(field: FieldInfo): FunctionSpec
    uses comptime.codegen
{
    result FunctionSpec {
        name: string::format("get_{}", [field.name]),
        visibility: Visibility::Public,
        generics: [],
        params: [
            ParamSpec {
                name: "value",
                ty: TypeRef::Named(type_name_of::<T>()),
                permission: Permission::Imm 
            },
        ],
        return_type: TypeRef::Named(field.ty_name),
        uses_clause: EffectSet::empty(),
        must_clause: ContractClause::None,
        will_clause: ContractClause::None,
        body: quote {
            // 'value' refers to generated parameter
            // #(field.name) interpolates the field name as identifier
            result value.#(field.name)
        },
    }
}
```

#### 11.3.7.4 Hygiene and Name Capture Prevention

**Problem**: Generated code can accidentally capture user-defined names, causing unexpected behavior or compilation errors.

**Unsafe example (name capture risk):**

```cursive
// BAD: Risk of name collision
comptime {
    codegen::insert_statement(quote {
        let buffer = Vec::new()  // What if user already has 'buffer'?
    })
}

// User code:
let buffer = "important data"
// Generated code shadows user's buffer!
```

**Solution**: Use `gensym` to generate unique identifiers:

```cursive
// GOOD: Hygienic code generation
comptime {
    let buffer_name = gensym("buffer")  // Returns "buffer_g12345"
    codegen::insert_statement(quote {
        let #(buffer_name) = Vec::new()  // Safe: unique identifier
    })
}

// User code:
let buffer = "important data"
// Generated: let buffer_g12345 = Vec::new()  ✅ No collision!
```

**Best Practices:**

1. **Always use `gensym` for local variables** in generated code
2. **Document public names** that your code generation exposes
3. **Namespace generated names** with prefixes (e.g., `__generated_`)
4. **Test with common names** (buffer, temp, result, etc.) to catch collisions

**Pattern for safe code generation:**

```cursive
comptime function generate_safe_wrapper<T>(): ()
    uses comptime.alloc, comptime.codegen
{
    let temp_name = gensym("temp")
    let result_name = gensym("result")
    
    codegen::declare_function(FunctionSpec {
        name: "safe_wrapper",
        // ...
        body: quote {
            let #(temp_name) = compute()
            let #(result_name) = process(#(temp_name))
            result #(result_name)
        },
    })
}
```

### 11.3.8 TypeRef Construction

**Definition 11.13 (Type References):** The `TypeRef` enum represents type references in code generation.

**NORMATIVE NOTE**: `TypeRef` is a **compiler-provided type** available only in comptime contexts and code generation APIs. It cannot be constructed in runtime code or used in runtime type signatures. It exists solely for metaprogramming purposes.

```cursive
enum TypeRef {
    Named(string)
    SelfType
    Generic(string, [TypeRef])
    Tuple([TypeRef])
    Array(TypeRef, usize)
    Slice(TypeRef)
    Pointer(TypeRef, Mutability)
    Function([TypeRef], TypeRef, EffectSet)
    TypeId(u64)
}

enum Mutability {
    Const
    Mut
}

record EffectSet {
    effects: [string]
}

enum ContractClause {
    None
    Single(string)
    Multiple([string])
}
```

**Examples:**

```cursive
// Simple named type
let int_ref = TypeRef::Named("i32")

// Generic type
let vec_int = TypeRef::Generic("Vec", [TypeRef::Named("i32")])

// Tuple type
let pair = TypeRef::Tuple([TypeRef::Named("i32"), TypeRef::Named("bool")])

// Array type
let buffer = TypeRef::Array(TypeRef::Named("u8"), 1024)

// Function type
let callback = TypeRef::Function(
    [TypeRef::Named("i32")],
    TypeRef::Named("bool"),
    EffectSet::empty()
)

// Type from type parameter
let ty_id = type_id_of::<Point>()
let point_ref = TypeRef::TypeId(ty_id)
```

### 11.3.9 Module Generation

**Definition 11.14 (Module Generation):** Complete modules can be generated programmatically with all their declarations.

```cursive
record ModuleSpec {
    name: string
    visibility: Visibility
    declarations: [Declaration]
}

enum Declaration {
    Function(FunctionSpec)
    Procedure(ProcedureSpec)
    Type(TypeSpec)
    Constant(ConstantSpec)
}

record ConstantSpec {
    name: string
    ty: TypeRef
    value: QuotedExpr
}

comptime function codegen::declare_module(spec: ModuleSpec): ()
    uses comptime.codegen
```

**Static semantics:**

```
[Codegen-Module]
Γ_ct ⊢ spec : ModuleSpec
spec.name not already declared
all declarations in spec well-formed
-------------------------------------------
⊢ codegen::declare_module(spec) generates:

module spec.name {
    spec.declarations...
}
```

**Example:**

```cursive
comptime function generate_test_fixtures(): ()
    uses comptime.alloc, comptime.codegen
{
    codegen::declare_module(ModuleSpec {
        name: "fixtures",
        visibility: Visibility::Internal,
        declarations: [
            Declaration::Type(TypeSpec {
                kind: TypeKind::Record,
                name: "TestUser",
                visibility: Visibility::Public,
                generics: [],
                fields: [
                    FieldSpec { name: "name", ty: TypeRef::Named("string"), visibility: Visibility::Public },
                    FieldSpec { name: "id", ty: TypeRef::Named("u64"), visibility: Visibility::Public },
                ],
                procedures: [],
                functions: [],
                variants: [],
            }),
            Declaration::Function(FunctionSpec {
                name: "create_test_user",
                visibility: Visibility::Public,
                generics: [],
                params: [],
                return_type: TypeRef::Named("TestUser"),
                uses_clause: EffectSet::empty(),
                must_clause: ContractClause::None,
                will_clause: ContractClause::None,
                body: quote {
                    result TestUser { name: "Test User", id: 123 }
                },
            }),
        ],
    })
}

comptime { generate_test_fixtures() }

// Generated module available:
// module fixtures {
//     record TestUser { name: string, id: u64 }
//     function create_test_user(): TestUser { ... }
// }
let user = fixtures::create_test_user()
```

### 11.3.10 Contract Implementation

**Definition 11.15 (Contract Implementation):** Contracts can be implemented on types programmatically.

```cursive
comptime function codegen::implement_contract(
    type_ref: TypeRef,
    contract_name: string,
    procedures: [ProcedureSpec]
): ()
    uses comptime.codegen
```

**Static semantics:**

```
[Codegen-Implement-Contract]
Γ_ct ⊢ type_ref : TypeRef
type_ref refers to existing type T
contract_name refers to existing contract C
all procedures in spec match contract requirements
-------------------------------------------
⊢ codegen::implement_contract(type_ref, contract_name, procedures)
  adds contract implementation to T
```

**Example:**

```cursive
contract Serializable {
    procedure serialize(self: Self): Vec<u8>
    function deserialize(data: [u8]): Result<Self, Error>
}

comptime function auto_implement_serializable<T>(): ()
    uses comptime.alloc, comptime.codegen
{
    let fields = fields_of::<T>()
    
    let serialize_proc = ProcedureSpec {
        name: "serialize",
        visibility: Visibility::Public,
        receiver: ParamSpec {
            name: "self",
            ty: TypeRef::SelfType,
            permission: Permission::Imm,
        },
        params: [],
        return_type: TypeRef::Named("Vec<u8>"),
        uses_clause: EffectSet { effects: ["alloc.heap"] },
        must_clause: ContractClause::None,
        will_clause: ContractClause::None,
        body: build_serialize_body(fields),
    }
    
    codegen::implement_contract(
        type_ref: TypeRef::TypeId(type_id_of::<T>()),
        contract_name: "Serializable",
        procedures: [serialize_proc],
    )
    
    // Add deserialize as associated function
    codegen::add_function(
        target: TypeRef::TypeId(type_id_of::<T>()),
        spec: build_deserialize_function::<T>(fields),
    )
}

[[reflect]]
record Message {
    id: u64
    content: string
}

comptime { auto_implement_serializable::<Message>() }

// Now Message implements Serializable
let msg = Message { id: 1, content: "Hello" }
let bytes = msg::serialize()
let recovered = Message::deserialize(bytes)?
```

---

## 11.4 Compile-Time Type Functions

**Definition 11.16 (Type Functions):** Functions that compute and return types at compile time.

```cursive
comptime function conditional_type<const COND: bool>(): TypeRef {
    result if COND {
        TypeRef::Named("i64")
    } else {
        TypeRef::Named("i32")
    }
}

// Use in type alias
type ConditionalInt<const COND: bool> = conditional_type::<COND>()

// Expands to i64 when COND=true, i32 when COND=false
let x: ConditionalInt<true> = 42i64
let y: ConditionalInt<false> = 42i32
```

**Type-level arithmetic:**

```cursive
comptime function next_power_of_two<const N: usize>(): usize {
    var power = 1
    loop power < N {
        power *= 2
    }
    result power
}

type AlignedBuffer<const SIZE: usize> = [u8; next_power_of_two::<SIZE>()]

// AlignedBuffer<100> becomes [u8; 128]
// AlignedBuffer<200> becomes [u8; 256]
```

**Complex type generation:**

```cursive
comptime function tuple_repeat<T, const N: usize>(): TypeRef {
    var types = Vec::new()
    loop i in 0..N {
        types::push(TypeRef::Named(type_name_of::<T>()))
    }
    result TypeRef::Tuple(types)
}

type Quad<T> = tuple_repeat::<T, 4>()

// Quad<i32> becomes (i32, i32, i32, i32)
// Quad<f64> becomes (f64, f64, f64, f64)
```

---

## 11.5 Runtime Reflection (Optional)

**NOTE**: Runtime reflection is **optional** and **discouraged** in Cursive. It should only be used when compile-time reflection is insufficient (e.g., dynamic loading, scripting integration).

### 11.5.1 Runtime Type Information

Types marked with `[[reflect]]` can optionally include runtime metadata:

```cursive
record RuntimeTypeInfo {
    id: u64
    name: string
    size: usize
    align: usize
    kind: TypeKind
}

function reflect_type<T>(): Option<RuntimeTypeInfo>
    uses unsafe.reflect
```

**Static semantics:**

```
[Runtime-Reflect-Available]
T marked with [[reflect]]
compiler flag --runtime-reflection enabled
-------------------------------------------
reflect_type::<T>() returns Some(info)

[Runtime-Reflect-Unavailable]
T not marked with [[reflect]] ∨
compiler flag --runtime-reflection disabled
-------------------------------------------
reflect_type::<T>() returns None
```

### 11.5.2 Runtime Field Access

```cursive
record RuntimeFieldInfo {
    name: string
    offset: usize
    type_id: u64
}

function reflect_fields<T>(): Option<[RuntimeFieldInfo]>
    uses unsafe.reflect

function get_field_by_name<T>(
    value: T,
    field_name: string
): Option<*u8>
    uses unsafe.reflect, unsafe.ptr
```

**WARNING**: Runtime reflection requires unsafe operations and is not guaranteed to be type-safe.

### 11.5.3 Opting Out of Runtime Reflection

```cursive
// Compile without runtime reflection metadata
// Flag: --no-runtime-reflection (default)

// This eliminates all runtime reflection overhead
// Only compile-time reflection remains available
```

**Performance implications:**

- **Without runtime reflection**: Zero overhead (default)
- **With runtime reflection**: ~100-500 bytes metadata per reflected type
- **Reflection queries**: ~2-10 nanoseconds per call

---

## 11.6 Metaprogramming Patterns

### 11.6.1 Serialization Generation

```cursive
comptime function generate_serializer<T>(): ()
    uses comptime.alloc, comptime.codegen
{
    let fields = fields_of::<T>()
    let type_name = type_name_of::<T>()
    
    // Generate serialize procedure
    let serialize_body = build_serialize_body(fields)
    
    codegen::add_procedure(
        target: TypeRef::TypeId(type_id_of::<T>()),
        spec: ProcedureSpec {
            name: "serialize",
            visibility: Visibility::Public,
            receiver: ParamSpec {
                name: "self",
                ty: TypeRef::SelfType,
                permission: Permission::Imm,
            },
            params: [],
            return_type: TypeRef::Named("Vec<u8>"),
            uses_clause: EffectSet { effects: ["alloc.heap"] },
            must_clause: ContractClause::None,
            will_clause: ContractClause::None,
            body: serialize_body,
        }
    )
    
    // Generate deserialize function
    let deserialize_body = build_deserialize_body(fields)
    
    codegen::add_function(
        target: TypeRef::TypeId(type_id_of::<T>()),
        spec: FunctionSpec {
            name: "deserialize",
            visibility: Visibility::Public,
            generics: [],
            params: [
                ParamSpec {
                    name: "data",
                    ty: TypeRef::Slice(TypeRef::Named("u8")),
                    permission: Permission::Imm,
                },
            ],
            return_type: TypeRef::Generic("Result", [
                TypeRef::SelfType,
                TypeRef::Named("Error")
            ]),
            uses_clause: EffectSet { effects: ["alloc.heap"] },
            must_clause: ContractClause::None,
            will_clause: ContractClause::None,
            body: deserialize_body,
        }
    )
}

comptime function build_serialize_body(fields: [FieldInfo]): QuotedBlock {
    // Build serialization logic based on field types
    // This is a helper function to construct the quote block

    var field_serializations = Vec::new()
    loop field in fields {
        field_serializations::push(quote {
            buffer::extend(self.#(field.name).to_bytes())
        })
    }

    result quote {
        var buffer = Vec::new()
        #(field_serializations)
        result buffer
    }
}

[[reflect]]
record User {
    name: string
    email: string
    age: u32
}

comptime { generate_serializer::<User>() }

// Now User has serialize and deserialize:
let user = User { name: "Alice", email: "alice@example.com", age: 30 }
let bytes = user::serialize()
let recovered = User::deserialize(bytes)
```

### 11.6.2 Builder Pattern Generation

```cursive
comptime function generate_builder<T>(): ()
    uses comptime.alloc, comptime.codegen
{
    let type_name = type_name_of::<T>()
    let fields = fields_of::<T>()
    
    // Build list of builder fields (all wrapped in Option)
    var builder_fields = Vec::new()
    loop field in fields {
        builder_fields::push(FieldSpec {
            name: field.name,
            ty: TypeRef::Generic("Option", [TypeRef::Named(field.ty_name)]),
            visibility: Visibility::Private,
        })
    }
    
    // Generate builder procedures for each field
    var builder_procedures = Vec::new()
    loop field in fields {
        builder_procedures::push(ProcedureSpec {
            name: field.name,
            visibility: Visibility::Public,
            receiver: ParamSpec {
                name: "self",
                ty: TypeRef::Named(string::format("{}Builder", [type_name])),
                permission: Permission::Mut,
            },
            params: [
                ParamSpec {
                    name: "value",
                    ty: TypeRef::Named(field.ty_name),
                    permission: Permission::Own,
                },
            ],
            return_type: TypeRef::Named(string::format("{}Builder", [type_name])),
            uses_clause: EffectSet::empty(),
            must_clause: ContractClause::None,
            will_clause: ContractClause::None,
            body: quote {
                self.#(field.name) = Some(value)
                result self
            },
        })
    }
    
    // Add build procedure
    builder_procedures::push(ProcedureSpec {
        name: "build",
        visibility: Visibility::Public,
        receiver: ParamSpec {
            name: "self",
            ty: TypeRef::Named(string::format("{}Builder", [type_name])),
            permission: Permission::Own,
        },
        params: [],
        return_type: TypeRef::Generic("Result", [
            TypeRef::Named(type_name),
            TypeRef::Named("BuildError")
        ]),
        uses_clause: EffectSet::empty(),
        must_clause: ContractClause::None,
        will_clause: ContractClause::None,
        body: build_builder_body(fields),
    })
    
    // Generate the builder type
    codegen::declare_type(TypeSpec {
        kind: TypeKind::Record,
        name: string::format("{}Builder", [type_name]),
        visibility: Visibility::Public,
        generics: [],
        fields: builder_fields,
        procedures: builder_procedures,
        functions: [],
        variants: [],
    })
}

comptime function build_builder_body(fields: [FieldInfo]): QuotedBlock {
    // Generate validation and construction logic
    quote {
        // Validate all fields are present
        loop field_name in ["name", "email", "age"] {
            match self.#(field_name) {
                None => result Err(BuildError::MissingField(field_name)),
                Some(_) => {},
            }
        }
        
        // Construct the type
        result Ok(User {
            name: self.name.unwrap(),
            email: self.email.unwrap(),
            age: self.age.unwrap(),
        })
    }
}

[[reflect]]
record User {
    name: string
    email: string
    age: u32
}

comptime { generate_builder::<User>() }

// Generated UserBuilder available:
let user = UserBuilder::new()
    ::name("Alice")
    ::email("alice@example.com")
    ::age(30)
    ::build()?
```

### 11.6.3 Struct-of-Arrays Transformation

**Pattern**: Transform Array-of-Structs (AoS) to Struct-of-Arrays (SoA) for cache locality and SIMD optimization.

```cursive
comptime function generate_soa<T, const N: usize>(): TypeSpec
    uses comptime.alloc, comptime.codegen
{
    let aos_fields = fields_of::<T>()
    let type_name = type_name_of::<T>()
    
    // Transform each field F: T into field Fs: [T; N]
    let soa_fields = aos_fields::map(|field| {
        FieldSpec {
            name: string::concat(field.name, "s"),
            ty: TypeRef::Array(TypeRef::Named(field.ty_name), N),
            visibility: field.visibility,
        }
    })
    
    // Generate accessors
    var soa_procedures = Vec::new()

    // Generate get procedure: get(index: usize) -> T
    soa_procedures::push(ProcedureSpec {
        name: "get",
        visibility: Visibility::Public,
        receiver: ParamSpec {
            name: "self",
            ty: TypeRef::SelfType,
            permission: Permission::Imm,
        },
        params: [
            ParamSpec {
                name: "index",
                ty: TypeRef::Named("usize"),
                permission: Permission::Imm,
            },
        ],
        return_type: TypeRef::Named(type_name),
        uses_clause: EffectSet::empty(),
        must_clause: ContractClause::Single("index < N"),
        will_clause: ContractClause::None,
        body: build_soa_get_body(aos_fields),
    })
    
    // Generate set procedure: set(index: usize, value: T)
    soa_procedures::push(ProcedureSpec {
        name: "set",
        visibility: Visibility::Public,
        receiver: ParamSpec {
            name: "self",
            ty: TypeRef::SelfType,
            permission: Permission::Mut,
        },
        params: [
            ParamSpec { name: "index", ty: TypeRef::Named("usize"), permission: Permission::Imm },
            ParamSpec { name: "value", ty: TypeRef::Named(type_name), permission: Permission::Own },
        ],
        return_type: TypeRef::Named("()"),
        uses_clause: EffectSet::empty(),
        must_clause: ContractClause::Single("index < N"),
        will_clause: ContractClause::None,
        body: build_soa_set_body(aos_fields),
    })
    
    result TypeSpec {
        kind: TypeKind::Record,
        name: string::format("{}SoA{}", [type_name, N]),
        visibility: Visibility::Public,
        generics: [],
        fields: soa_fields,
        procedures: soa_procedures,
        functions: [],
        variants: [],
    }
}

comptime function build_soa_get_body(fields: [FieldInfo]): QuotedBlock {
    var field_inits = Vec::new()
    loop field in fields {
        let field_name_plural = string::concat(field.name, "s")
        field_inits::push(quote {
            #(field.name): self.#(field_name_plural)[index]
        })
    }
    
    result quote {
        result T {
            #(field_inits)
        }
    }
}

comptime function build_soa_set_body(fields: [FieldInfo]): QuotedBlock {
    var field_sets = Vec::new()
    loop field in fields {
        let field_name_plural = string::concat(field.name, "s")
        field_sets::push(quote {
            self.#(field_name_plural)[index] = value.#(field.name)
        })
    }
    
    result quote {
        #(field_sets)
    }
}

// Usage for game entities:
[[reflect]]
record Entity {
    position: Vec3
    velocity: Vec3
    health: f32
}

comptime {
    codegen::declare_type(generate_soa::<Entity, 1024>())
}

// Generated: EntitySoA1024 with cache-friendly layout
// record EntitySoA1024 {
//     positions: [Vec3; 1024]
//     velocitys: [Vec3; 1024]
//     healths: [f32; 1024]
//     
//     procedure get($, index: usize): Entity { ... }
//     procedure set(mut $, index: usize, value: own Entity) { ... }
// }

let entities = EntitySoA1024::default()
// SIMD-friendly: all positions contiguous in memory
loop i in 0..1024 {
    let entity = entities::get(i)
    process(entity)
}
```

### 11.6.4 ECS Component Registration

```cursive
comptime function register_component<T>(): ()
    uses comptime.alloc, comptime.codegen
{
    let type_id = type_id_of::<T>()
    let type_name = type_name_of::<T>()
    
    // Add component attribute
    codegen::add_attribute(
        target: TypeRef::TypeId(type_id),
        attr: AttributeSpec {
            name: "component",
            args: [],
        }
    )
    
    // Generate type ID constant
    codegen::declare_constant(
        name: string::format("COMPONENT_ID_{}", [type_name.to_uppercase()]),
        ty: TypeRef::Named("u64"),
        value: quote { #(type_id) }
    )
    
    // Generate registration function
    codegen::declare_function(FunctionSpec {
        name: string::format("register_{}_component", [type_name.to_lowercase()]),
        visibility: Visibility::Public,
        generics: [],
        params: [],
        return_type: TypeRef::Named("()"),
        uses_clause: EffectSet { effects: ["alloc.heap"] },
        must_clause: ContractClause::None,
        will_clause: ContractClause::None,
        body: quote {
            COMPONENT_REGISTRY::register::<T>(#(type_id))
        },
    })
}

[[reflect]]
record Transform {
    position: Vec3
    rotation: Quat
    scale: Vec3
}

comptime { register_component::<Transform>() }

// Generated:
// - [[component]] attribute on Transform
// - const COMPONENT_ID_TRANSFORM: u64 = ...
// - function register_transform_component(): () { ... }
```

### 11.6.5 Debug Formatting Generation

```cursive
comptime function generate_debug<T>(): ()
    uses comptime.alloc, comptime.codegen
{
    let fields = fields_of::<T>()
    let type_name = type_name_of::<T>()
    
    let debug_body = build_debug_body(type_name, fields)
    
    codegen::add_procedure(
        target: TypeRef::TypeId(type_id_of::<T>()),
        spec: ProcedureSpec {
            name: "fmt_debug",
            visibility: Visibility::Public,
            receiver: ParamSpec {
                name: "self",
                ty: TypeRef::SelfType,
                permission: Permission::Imm,
            },
            params: [
                ParamSpec {
                    name: "formatter",
                    ty: TypeRef::Named("Formatter"),
                    permission: Permission::Mut,
                },
            ],
            return_type: TypeRef::Generic("Result", [
                TypeRef::Named("()"),
                TypeRef::Named("FormatError")
            ]),
            uses_clause: EffectSet { effects: ["io.write"] },
            must_clause: ContractClause::None,
            will_clause: ContractClause::None,
            body: debug_body,
        }
    )
}

comptime function build_debug_body(type_name: string, fields: [FieldInfo]): QuotedBlock {
    quote {
        formatter::write_str(#(type_name))?
        formatter::write_str(" { ")?
        
        loop (i, field) in enumerate(#(fields)) {
            if i > 0 {
                formatter::write_str(", ")?
            }
            formatter::write_str(#(field.name))?
            formatter::write_str(": ")?
            formatter::write_debug(self.#(field.name))?
        }
        
        formatter::write_str(" }")?
        result Ok(())
    }
}

[[reflect]]
record Point {
    x: f64
    y: f64
}

comptime { generate_debug::<Point>() }

// Now can use: println("{:?}", point)
// Output: Point { x: 1.0, y: 2.0 }
```

---

## 11.7 Integration with Language Features

### 11.7.1 Metaprogramming and Permissions

Reflection respects permission information:

```cursive
comptime function requires_ownership<F>(): bool
    uses comptime.alloc
{
    // Inspect function signature for ownership requirements
    let info = function_info_of::<F>()

    var needs_own = false
    loop param in info.params {
        if param.permission == Permission::Own {
            needs_own = true
        }
    }
    
    result needs_own
}

function adapt_call<T, F>(value: T, func: F): ()
where
    F: (T) → ()
{
    comptime {
        if requires_ownership::<F>() {
            codegen::insert_statement(quote {
                func(value)  // value moved
            })
        } else {
            codegen::insert_statement(quote {
                func(value)  // value borrowed
            })
        }
    }
}
```

### 11.7.2 Metaprogramming and Effects

Generate functions with appropriate effect declarations:

```cursive
comptime function generate_io_wrapper<T>(): ()
    uses comptime.codegen
{
    let type_name = type_name_of::<T>()
    
    codegen::declare_function(FunctionSpec {
        name: string::format("read_{}", [type_name.to_lowercase()]),
        visibility: Visibility::Public,
        generics: [],
        params: [
            ParamSpec {
                name: "path",
                ty: TypeRef::Named("string"),
                permission: Permission::Imm,
            },
        ],
        return_type: TypeRef::Generic("Result", [
            TypeRef::Named(type_name),
            TypeRef::Named("IoError")
        ]),
        uses_clause: EffectSet { effects: ["fs.read", "alloc.heap"] },
        must_clause: ContractClause::None,
        will_clause: ContractClause::None,
        body: quote {
            let data = fs::read_to_string(path)?
            result T::deserialize(data)
        },
    })
}
```

### 11.7.3 Metaprogramming and Modals

Generate state transitions for modal types:

```cursive
comptime function generate_modal_transitions<T>(): ()
    uses comptime.alloc, comptime.codegen
{
    let transitions = transitions_of::<T>()
    
    loop trans in transitions {
        comptime_note(string::format(
            "Generating helper for transition: {} -> {}",
            [trans.from_state, trans.to_state]
        ))
        
        // Generate helper functions for common patterns
        generate_transition_helper(trans)
    }
}
```

### 11.7.4 Metaprogramming and Unsafe Code

Code generation can produce unsafe blocks when necessary:

```cursive
comptime function generate_ffi_wrapper(func_name: string): ()
    uses comptime.codegen
{
    codegen::declare_function(FunctionSpec {
        name: string::format("{}_safe", [func_name]),
        visibility: Visibility::Public,
        // ...
        uses_clause: EffectSet { effects: ["ffi.call", "unsafe.ptr"] },
        body: quote {
            unsafe {
                ffi_call(#(func_name))
            }
        },
    })
}
```

---

## 11.8 Grammar Additions

### 11.8.1 Comptime Keywords

Add to lexical tokens (Appendix A.1):

```ebnf
Keyword         ::= ...
                 | "comptime"
```

### 11.8.2 Quote Expressions

Add to expression grammar (Appendix A.4):

```ebnf
PrimaryExpr     ::= ...
                 | QuoteExpr

QuoteExpr       ::= "quote" BlockExpr
                 | "quote" Expr

(* Interpolation within quote blocks *)
Interpolation   ::= "#" "(" Expr ")"
                 | "#" Ident
```

**NORMATIVE NOTE**: The interpolation syntax uses `#(...)` not `$(...)` because `$` is reserved for self parameters (see A.6: `SelfParam ::= "$"`).

### 11.8.3 Comptime Blocks

Add to statement grammar (Appendix A.5):

```ebnf
Stmt            ::= ...
                 | ComptimeBlock

ComptimeBlock   ::= "comptime" BlockExpr
```

### 11.8.4 Attributes

Extend attribute grammar (Appendix A.1):

```ebnf
Attribute       ::= "[[" AttributeBody "]]"

AttributeBody   ::= Ident ("(" AttrArgs? ")")?
                 | "reflect"
                 | "comptime_test"

AttrArgs        ::= AttrArg ("," AttrArg)* ","?
AttrArg         ::= Ident "=" Literal
                 | Literal
```

---

## 11.9 Error Codes

### 11.9.1 Comptime Evaluation Errors

```
E11CT-0101: Comptime assertion failed
E11CT-0102: Comptime evaluation exceeded time limit
E11CT-0103: Comptime evaluation exceeded step limit  
E11CT-0104: Comptime evaluation exceeded memory limit
E11CT-0105: Comptime evaluation exceeded recursion depth
E11CT-0106: Comptime evaluation exceeded collection size limit
E11CT-0107: Comptime evaluation non-deterministic
E11CT-0108: Comptime evaluation time warning
E11CT-0201: Runtime effect prohibited in comptime context
E11CT-0202: Non-constant expression in comptime context
```

### 11.9.2 Reflection Errors

```
E11RF-0101: Type not marked [[reflect]]
E11RF-0102: Field not found during reflection
E11RF-0103: Procedure not found during reflection
E11RF-0104: Invalid type for reflection query
E11RF-0105: Reflection query requires --runtime-reflection flag
```

### 11.9.3 Code Generation Errors

```
E11CG-0101: Invalid code generation specification
E11CG-0102: Name collision in generated code
E11CG-0103: Type reference resolution failed
E11CG-0104: Generated code failed type checking
E11CG-0105: Quote interpolation type mismatch
E11CG-0106: Cannot add procedure to non-record type
E11CG-0107: Target type not found for extension
E11CG-0108: Procedure name already exists on target type
```

---

## 11.10 Examples

### 11.10.1 Complete Serialization Framework

```cursive
// Serialization contract
contract Serializable {
    procedure serialize(self: Self): Vec<u8>
    function deserialize(data: [u8]): Result<Self, Error>
}

// Generate implementation for any type
comptime function impl_serializable<T>(): ()
    uses comptime.alloc, comptime.codegen
{
    let fields = fields_of::<T>()
    
    // Build serialize procedure
    codegen::add_procedure(
        target: TypeRef::TypeId(type_id_of::<T>()),
        spec: ProcedureSpec {
            name: "serialize",
            visibility: Visibility::Public,
            receiver: ParamSpec {
                name: "self",
                ty: TypeRef::SelfType,
                permission: Permission::Imm,
            },
            params: [],
            return_type: TypeRef::Named("Vec<u8>"),
            uses_clause: EffectSet { effects: ["alloc.heap"] },
            must_clause: ContractClause::None,
            will_clause: ContractClause::None,
            body: build_serialize_impl(fields),
        }
    )
    
    // Build deserialize function
    codegen::add_function(
        target: TypeRef::TypeId(type_id_of::<T>()),
        spec: FunctionSpec {
            name: "deserialize",
            visibility: Visibility::Public,
            generics: [],
            params: [
                ParamSpec {
                    name: "data",
                    ty: TypeRef::Slice(TypeRef::Named("u8")),
                    permission: Permission::Imm,
                },
            ],
            return_type: TypeRef::Generic("Result", [
                TypeRef::SelfType,
                TypeRef::Named("Error")
            ]),
            uses_clause: EffectSet { effects: ["alloc.heap"] },
            must_clause: ContractClause::None,
            will_clause: ContractClause::None,
            body: build_deserialize_impl(fields),
        }
    )
}

[[reflect]]
record Message {
    id: u64
    content: string
    timestamp: i64
}

comptime { impl_serializable::<Message>() }

// Usage:
let msg = Message { id: 1, content: "Hello", timestamp: 1234567890 }
let bytes = msg::serialize()
let recovered = Message::deserialize(bytes)?
```

### 11.10.2 Property-Based Testing Generator

```cursive
comptime function generate_property_tests<T>(): ()
    uses comptime.codegen
{
    let type_name = type_name_of::<T>()
    
    // Generate various property tests
    codegen::declare_function(FunctionSpec {
        name: string::format("test_{}_roundtrip", [type_name.to_lowercase()]),
        visibility: Visibility::Public,
        generics: [],
        params: [],
        return_type: TypeRef::Named("()"),
        uses_clause: EffectSet::empty(),
        must_clause: ContractClause::None,
        will_clause: ContractClause::None,
        body: quote {
            loop _ in 0..100 {
                let original = random::<T>()
                let bytes = original::serialize()
                let recovered = T::deserialize(bytes)?
                assert(original == recovered, "Roundtrip failed")
            }
        },
    })
}
```

---

## 11.11 Implementation Notes

### 11.11.1 Compilation Phases

1. **Parse phase**: Collect all declarations including comptime blocks
2. **Comptime phase**: Execute all comptime blocks in dependency order
3. **Code generation phase**: Emit generated declarations to AST
4. **Type check phase**: Type check both original and generated code
5. **Lower phase**: Lower to IR, eliminate all comptime constructs

### 11.11.2 Debugging Comptime Code

```cursive
comptime {
    comptime_note("Starting code generation")
    
    let types = [i32, i64, f32, f64]
    loop ty in types {
        comptime_note(string::format("Generating for type: {}", [type_name_of(ty)]))
        generate_max_for_type(TypeRef::Named(type_name_of(ty)))
    }
    
    comptime_note("Code generation complete")
}
```

Compiler output:
```
note: Starting code generation
note: Generating for type: i32
note: Generating for type: i64
note: Generating for type: f32
note: Generating for type: f64
note: Code generation complete
```

### 11.11.3 Performance Considerations

**Comptime evaluation:**
- Minimize allocation in hot loops
- Cache reflection queries
- Use iterative over recursive algorithms when possible
- Split large comptime blocks into smaller chunks
- Profile with `comptime_profile_start/end` for optimization

**Code generation:**
- Generate only what's needed
- Use feature flags for conditional generation
- Consider binary size impact of reflection metadata

### 11.11.4 Debugging Techniques

**Technique 1: Comptime Notes**
```cursive
comptime {
    comptime_note("Starting generation")
    let types = [i32, i64, f32]
    loop ty in types {
        comptime_note(string::format("Processing {}", [type_name_of(ty)]))
        generate_for(ty)
    }
    comptime_note("Complete")
}
```

**Technique 2: Conditional Codegen**
```cursive
comptime {
    if cfg("debug_codegen") {
        // Generate extra validation code in debug mode
        codegen::insert_statement(quote {
            comptime_assert(condition, "Validation")
        })
    }
}
```

**Technique 3: Profiling**
```cursive
comptime {
    let handle = comptime_profile_start("expensive_generation")
    
    loop i in 0..10000 {
        generate_complex_type(i)
    }
    
    let stats = comptime_profile_end(handle)
    if stats.steps > 800000 {
        comptime_warning("Consider optimization")
    }
}
```

---

## 11.12 Conformance

A conforming implementation MUST:

1. Support all comptime intrinsics specified in §11.1.3
2. Enforce minimum resource limits specified in §11.1.3
3. Provide all reflection APIs specified in §11.2
4. Implement all code generation APIs specified in §11.3
5. Support quote expressions with interpolation specified in §11.3.7
6. Use `#(...)` syntax for interpolation (not `$(...)`)
7. Respect [[reflect]] opt-in for type reflection
8. Generate diagnostics with specified error codes
9. Support hygiene through `gensym` intrinsic

A conforming implementation MAY:

1. Exceed minimum resource limits
2. Provide additional comptime intrinsics as extensions
3. Optimize comptime evaluation
4. Provide enhanced debugging tools for comptime code
5. Implement runtime reflection (optional feature)

A conforming implementation MUST NOT:

1. Execute comptime code with non-deterministic results
2. Allow runtime effects in comptime contexts (except comptime.* effects)
3. Permit reflection on types without [[reflect]] attribute
4. Generate code that violates type safety
5. Silently ignore comptime errors
6. Use `$(...)` for interpolation (reserved for self parameters)

---

**Previous**: [Modal Types](10_Modals.md) | **Next**: [Modules, Packages, and Imports](12_Modules-Packages-and-Imports.md)

**END OF CHAPTER XI**