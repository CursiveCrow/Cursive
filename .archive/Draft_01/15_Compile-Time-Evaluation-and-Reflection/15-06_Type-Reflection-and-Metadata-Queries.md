# Cursive Language Specification

## Clause 15 — Compile-Time Evaluation and Reflection

**Part**: XV — Compile-Time Evaluation and Reflection  
**File**: 16-6_Type-Reflection-and-Metadata-Queries.md  
**Section**: §15.6 Type Reflection and Metadata Queries  
**Stable label**: [comptime.reflect.queries]  
**Forward references**: §15.5 [comptime.reflect.optin], §15.8 [comptime.codegen.api], Clause 4 §4.5 [decl.type], Clause 6 §6.2 [type.primitive], Clause 6 §6.3 [type.composite], Clause 6 §6.6 [type.modal], Clause 6 §6.8 [type.introspection], Clause 9 §9.6 [memory.layout]

---

### §15.6 Type Reflection and Metadata Queries [comptime.reflect.queries]

#### §15.6.1 Overview [comptime.reflect.queries.overview]

[1] This subclause specifies the complete API for querying type metadata at compile time. Reflection queries build upon the basic type introspection in §6.8 and the opt-in mechanism in §15.5 to provide comprehensive access to type structure.

[2] Reflection queries operate exclusively at compile time and require types to be marked with `[[reflect]]`. The queries return structured metadata describing fields, procedures, variants, modal states, and other type properties.

[3] The reflection API enables metaprogramming patterns: automatic serialization, debug formatting, builder pattern generation, ECS component registration, and other derive-like functionality.

#### §15.6.2 Core Type Reflection [comptime.reflect.queries.core]

##### §15.6.2.1 TypeInfo Structure

[4] The `TypeInfo` record (introduced in §16.2.3) provides core type metadata:

```cursive
record TypeInfo {
    name: string@View,
    size: usize,
    align: usize,
    kind: TypeKind,
}

enum TypeKind {
    Primitive,
    Record,
    Enum,
    Modal,
    Tuple,
    Array,
    Slice,
    Pointer,
    Function,
}
```

[5] **Field semantics:**

- `name`: Fully-qualified type name including module path
- `size`: Size in bytes per §10.6 [memory.layout]
- `align`: Alignment requirement (power of two)
- `kind`: Type category for dispatch in metaprogramming

##### §15.6.2.2 reflect_type Procedure

[6] Core reflection procedure:

```cursive
comptime procedure reflect_type<T>(): TypeInfo
    [[ comptime::alloc |- true => true ]]
```

[7] **Constraints:**

- Type `T` shall be marked with `[[reflect]]` (diagnostic E15-101 if not)
- Shall be called in comptime context (diagnostic E15-110 if runtime)
- Generic type `T` shall be instantiated with concrete arguments (diagnostic E15-112 if uninstantiated)

[8] **Typing rule:**

$$
\frac{\Gamma \vdash T : \text{Type} \quad T \text{ marked } \texttt{[[reflect]]}}{\Gamma_{\text{ct}} \vdash \texttt{reflect\_type}\langle T \rangle() : \texttt{TypeInfo}}
\tag{T-Reflect-Type}
$$

[9] **Semantics.** Returns a `TypeInfo` record populated with type metadata. The procedure executes at compile time using `comptime::alloc` grant to construct the result.

**Example 16.6.2.1** (Basic type reflection):

```cursive
[[reflect]]
record Vector3 {
    x: f32,
    y: f32,
    z: f32,
}

comptime {
    let info <- reflect_type::<Vector3>()

    comptime_assert(info.name == "Vector3", "Type name")
    comptime_assert(info.size == 12, "3 floats = 12 bytes")
    comptime_assert(info.align == 4, "f32 alignment")
    comptime_assert(info.kind == TypeKind::Record, "Is record")
}
```

#### §15.6.3 Field Reflection [comptime.reflect.queries.fields]

##### §15.6.3.1 FieldInfo Structure

[10] Field metadata structure:

```cursive
record FieldInfo {
    name: string@View,
    ty_name: string@View,
    offset: usize,
    visibility: Visibility,
}

enum Visibility {
    Public,
    Internal,
    Private,
    Protected,
}
```

[11] **Field semantics:**

- `name`: Field identifier
- `ty_name`: Fully-qualified type name of the field
- `offset`: Byte offset from structure base address (§11.6)
- `visibility`: Visibility modifier per §5.6

##### §15.6.3.2 Field Query Procedures

[12] Field reflection procedures:

```cursive
comptime procedure fields_of<T>(): [FieldInfo]
    [[ comptime::alloc |- true => true ]]

comptime procedure has_field<T>(name: string@View): bool

comptime procedure field_info_of<T>(name: string@View): FieldInfo \/ Unit
    [[ comptime::alloc |- true => true ]]

comptime procedure field_offset<T>(name: string@View): usize \/ Unit
```

[13] **Semantics:**

- `fields_of<T>()`: Returns array of `FieldInfo` for all fields in declaration order
- `has_field<T>(name)`: Returns `true` if field with identifier `name` exists
- `field_info_of<T>(name)`: Returns `FieldInfo` for field, or unit if not found
- `field_offset<T>(name)`: Returns byte offset for field, or unit if not found

[14] For non-record types (primitives, enums without fields), `fields_of` returns empty array.

##### §15.6.3.3 Examples

**Example 16.6.3.1** (Field iteration):

```cursive
[[reflect]]
record Person {
    public name: string@Managed,
    public age: u32,
    private ssn: string@Managed,
}

comptime {
    let fields <- fields_of::<Person>()

    loop field: FieldInfo in fields {
        comptime_note(string_concat("Field: ", field.name))
        comptime_note(string_concat("  Type: ", field.ty_name))
        comptime_note(string_concat("  Offset: ", usize_to_string(field.offset)))
    }
}
```

**Example 16.6.3.2** (Offset validation):

```cursive
[[repr(C)]]
[[reflect]]
record CHeader {
    magic: u32,
    version: u16,
    padding: u16,
    length: u32,
}

comptime {
    match field_offset::<CHeader>("magic") {
        offset: usize => comptime_assert(offset == 0, "magic at offset 0"),
        _: () => comptime_error("magic field missing"),
    }

    match field_offset::<CHeader>("version") {
        offset: usize => comptime_assert(offset == 4, "version at offset 4"),
        _: () => comptime_error("version field missing"),
    }
}
```

#### §15.6.4 Procedure Reflection [comptime.reflect.queries.procedures]

##### §15.6.4.1 ProcedureInfo Structure

[15] Procedure metadata structure:

```cursive
record ProcedureInfo {
    name: string@View,
    receiver_permission: Permission \/ Unit,
    params: [ParamInfo],
    return_type: string@View,
    grants: [string@View],
}

enum Permission {
    Const,
    Unique,
    Shared,
}

record ParamInfo {
    name: string@View,
    ty_name: string@View,
    permission: Permission,
    responsible: bool,
}
```

[16] **ProcedureInfo field semantics:**

- `name`: Procedure identifier
- `receiver_permission`: Permission if procedure has receiver (Const/Unique/Shared), unit if static procedure
- `params`: Array of parameter metadata in declaration order
- `return_type`: Fully-qualified return type name
- `grants`: Array of grant identifiers from sequent grants clause

[17] **ParamInfo field semantics:**

- `name`: Parameter identifier
- `ty_name`: Fully-qualified parameter type name
- `permission`: Parameter permission (const, unique, or shared)
- `responsible`: `true` if parameter has `move` modifier, `false` for non-responsible parameters

##### §15.6.4.2 Procedure Query Procedures

[18] Procedure reflection procedures:

```cursive
comptime procedure procedures_of<T>(): [ProcedureInfo]
    [[ comptime::alloc |- true => true ]]

comptime procedure has_procedure<T>(name: string@View): bool

comptime procedure procedure_info_of<T>(name: string@View): ProcedureInfo \/ Unit
    [[ comptime::alloc |- true => true ]]
```

[19] **Semantics:**

- `procedures_of<T>()`: Returns array of `ProcedureInfo` for all procedures on type `T`
- `has_procedure<T>(name)`: Returns `true` if procedure exists on type
- `procedure_info_of<T>(name)`: Returns `ProcedureInfo` for procedure, or unit if not found

##### §15.6.4.3 Examples

**Example 16.6.4.1** (Procedure reflection with grant inspection):

```cursive
[[reflect]]
record Database {
    connection: i32,

    procedure query(~%, sql: string@View): [Row]
        [[ db::query |- sql.len() > 0 => result.len() >= 0 ]]
    {
        // Implementation
    }

    procedure execute(~!, command: string@View): ()
        [[ db::execute |- command.len() > 0 => true ]]
    {
        // Implementation
    }
}

comptime {
    let procedures <- procedures_of::<Database>()

    loop proc: ProcedureInfo in procedures {
        comptime_note(string_concat("Procedure: ", proc.name))
        comptime_note(string_concat("  Grants: ", string_join(proc.grants, ", ")))

        match proc.receiver_permission {
            perm: Permission => {
                let perm_name = match perm {
                    Permission::Const => "const",
                    Permission::Unique => "unique",
                    Permission::Shared => "shared",
                }
                comptime_note(string_concat("  Receiver: ", perm_name))
            },
            _: () => comptime_note("  Static procedure"),
        }
    }
}
```

**Example 16.6.4.2** (Checking parameter responsibility):

```cursive
[[reflect]]
record Buffer {
    data: [u8],

    procedure consume(move ~!): ()
    {
        // Consumes buffer
    }

    procedure inspect(~): usize
    {
        result self.data.len()
    }
}

comptime {
    match procedure_info_of::<Buffer>("consume") {
        info: ProcedureInfo => {
            comptime_assert(info.params.len() == 1, "consume has self param")
            comptime_assert(info.params[0].responsible, "consume parameter is responsible")
        },
        _: () => comptime_error("consume not found"),
    }

    match procedure_info_of::<Buffer>("inspect") {
        info: ProcedureInfo => {
            comptime_assert(info.params.len() == 1, "inspect has self param")
            comptime_assert(!info.params[0].responsible, "inspect parameter non-responsible")
        },
        _: () => comptime_error("inspect not found"),
    }
}
```

#### §15.6.5 Enum Variant Reflection [comptime.reflect.queries.variants]

##### §15.6.5.1 VariantInfo Structure

[20] Enum variant metadata:

```cursive
record VariantInfo {
    name: string@View,
    discriminant: i64 \/ Unit,
    payload: [string@View],
}
```

[21] **Field semantics:**

- `name`: Variant identifier
- `discriminant`: Explicit discriminant value if specified, unit if sequential/default
- `payload`: Array of payload type names (empty for unit variants, length N for tuple variants, field type names for record variants)

##### §15.6.5.2 Variant Query Procedures

[22] Variant reflection procedures:

```cursive
comptime procedure variants_of<T>(): [VariantInfo]
    [[ comptime::alloc |- true => true ]]

comptime procedure has_variant<T>(name: string@View): bool

comptime procedure variant_info_of<T>(name: string@View): VariantInfo \/ Unit
    [[ comptime::alloc |- true => true ]]
```

[23] **Semantics:** Query enum variants. For non-enum types, `variants_of` returns empty array.

##### §15.6.5.3 Examples

**Example 16.6.5.1** (Enum variant reflection):

```cursive
[[reflect]]
enum Status {
    Pending = 0,
    Running = 1,
    Success = 2,
    Failed = 3,
}

comptime {
    let variants <- variants_of::<Status>()

    comptime_assert(variants.len() == 4, "4 variants")
    comptime_assert(variants[0].name == "Pending", "First variant")

    match variants[0].discriminant {
        disc: i64 => comptime_assert(disc == 0, "Pending discriminant"),
        _: () => comptime_error("Discriminant should be set"),
    }
}
```

**Example 16.6.5.2** (Payload type reflection):

```cursive
[[reflect]]
enum Message {
    Text(string@Managed),
    Data { id: u64, payload: [u8] },
    Close,
}

comptime {
    match variant_info_of::<Message>("Text") {
        info: VariantInfo => {
            comptime_assert(info.payload.len() == 1, "Text has 1 payload")
            comptime_assert(info.payload[0] == "string@Managed", "Payload type")
        },
        _: () => comptime_error("Text variant not found"),
    }

    match variant_info_of::<Message>("Data") {
        info: VariantInfo => {
            comptime_assert(info.payload.len() == 2, "Data has 2 fields")
        },
        _: () => comptime_error("Data variant not found"),
    }

    match variant_info_of::<Message>("Close") {
        info: VariantInfo => {
            comptime_assert(info.payload.len() == 0, "Close is unit variant")
        },
        _: () => comptime_error("Close variant not found"),
    }
}
```

#### §15.6.6 Modal State Reflection [comptime.reflect.queries.modal]

##### §15.6.6.1 Modal Reflection Structures

[24] Modal type metadata structures:

```cursive
record StateInfo {
    name: string@View,
    fields: [FieldInfo],
}

record TransitionInfo {
    procedure_name: string@View,
    from_state: string@View,
    to_state: string@View,
}
```

[25] **StateInfo field semantics:**

- `name`: State identifier (without @ prefix, e.g., "Open" not "@Open")
- `fields`: Array of state-specific field metadata

[26] **TransitionInfo field semantics:**

- `procedure_name`: Procedure implementing the state transition
- `from_state`: Source state identifier (without @ prefix)
- `to_state`: Target state identifier (without @ prefix)

##### §15.6.6.2 Modal Query Procedures

[27] Modal reflection procedures:

```cursive
comptime procedure states_of<T>(): [StateInfo]
    [[ comptime::alloc |- true => true ]]

comptime procedure transitions_of<T>(): [TransitionInfo]
    [[ comptime::alloc |- true => true ]]
```

[28] **Semantics:**

- `states_of<T>()`: Returns array of `StateInfo` for all modal states
- `transitions_of<T>()`: Returns array of `TransitionInfo` for all declared transitions

[29] For non-modal types, both procedures return empty arrays.

##### §15.6.6.3 Examples

**Example 16.6.6.1** (Modal state reflection):

```cursive
[[reflect]]
modal FileHandle {
    @Closed {
        path: string@Managed,
    }

    @Open {
        path: string@Managed,
        handle: i32,
    }

    procedure open(~!): FileHandle@Open
        [[ fs::open |- true => true ]]
    {
        // Implementation
    }

    procedure close(~!): FileHandle@Closed
        [[ fs::close |- true => true ]]
    {
        // Implementation
    }
}

comptime {
    let states <- states_of::<FileHandle>()

    comptime_assert(states.len() == 2, "2 states")
    comptime_assert(states[0].name == "Closed", "First state")
    comptime_assert(states[1].name == "Open", "Second state")

    // Check Open state has more fields than Closed
    comptime_assert(states[1].fields.len() > states[0].fields.len(), "Open has handle field")

    let transitions <- transitions_of::<FileHandle>()

    loop trans: TransitionInfo in transitions {
        comptime_note(string_concat(
            string_concat("Transition: ", trans.from_state),
            string_concat(" -> ", trans.to_state)
        ))
    }
}
```

#### §15.6.7 Advanced Reflection Patterns [comptime.reflect.queries.patterns]

##### §15.6.7.1 Layout Validation

**Example 16.6.7.1** (GPU vertex layout validation):

```cursive
[[repr(C, packed)]]
[[reflect]]
record GpuVertex {
    position: [f32; 3],
    normal: [f32; 3],
    uv: [f32; 2],
}

comptime {
    let info <- reflect_type::<GpuVertex>()

    comptime_assert(info.size == 32, "Vertex must be 32 bytes for GPU")
    comptime_assert(info.align == 1, "Packed alignment must be 1")

    let fields <- fields_of::<GpuVertex>()

    comptime_assert(fields[0].offset == 0, "position at offset 0")
    comptime_assert(fields[1].offset == 12, "normal at offset 12")
    comptime_assert(fields[2].offset == 24, "uv at offset 24")

    comptime_note("GPU vertex layout validated for shader compatibility")
}
```

##### §15.6.7.2 Component Registration

**Example 16.6.7.2** (ECS component registration with reflection):

```cursive
[[reflect]]
record Transform {
    position: [f32; 3],
    rotation: [f32; 4],
    scale: [f32; 3],
}

comptime procedure register_component<T>(): ()
    [[ comptime::codegen |- true => true ]]
{
    let type_id_val <- type_id<T>()
    let type_name_val <- type_name<T>()
    let size_val <- size_of<T>()

    codegen::declare_constant(
        name: string_concat("COMPONENT_ID_", type_name_val),
        ty: codegen::type_named("u64"),
        value: quote { $(type_id_val) }
    )

    codegen::declare_constant(
        name: string_concat("COMPONENT_SIZE_", type_name_val),
        ty: codegen::type_named("usize"),
        value: quote { $(size_val) }
    )

    comptime_note(string_concat("Registered component: ", type_name_val))
}

comptime { register_component::<Transform>() }

// Generated constants:
// let COMPONENT_ID_Transform: const u64 = <unique_id>
// let COMPONENT_SIZE_Transform: const usize = 44
```

##### §15.6.7.3 Automatic Getter Generation

**Example 16.6.7.3** (Reflection-driven getter generation):

```cursive
[[reflect]]
record Config {
    public database_url: string@View,
    public max_connections: usize,
    public timeout_seconds: u64,
}

comptime procedure generate_getters<T>(): ()
    [[ comptime::alloc, comptime::codegen |- true => true ]]
{
    let fields <- fields_of::<T>()

    loop field: FieldInfo in fields {
        if field.visibility == Visibility::Public {
            codegen::add_procedure(
                target: TypeRef::TypeId(type_id<T>()),
                spec: codegen::ProcedureSpec {
                    name: string_concat("get_", field.name),
                    visibility: codegen::Visibility::Public,
                    receiver: codegen::ReceiverSpec::Const,
                    params: [],
                    return_type: TypeRef::Named(field.ty_name),
                    sequent: codegen::sequent_pure(),
                    body: quote {
                        result self.$(field.name)
                    },
                }
            )
        }
    }
}

comptime { generate_getters::<Config>() }

// Generated procedures:
// procedure get_database_url(~): string@View { result self.database_url }
// procedure get_max_connections(~): usize { result self.max_connections }
// procedure get_timeout_seconds(~): u64 { result self.timeout_seconds }

let config = Config {
    database_url: "localhost:5432",
    max_connections: 100,
    timeout_seconds: 30,
}

let url = config::get_database_url()
```

#### §15.6.8 Reflection Constraints Summary [comptime.reflect.queries.constraints]

[30] All reflection query procedures share these constraints:

**Opt-in requirement:**

[31] Type `T` shall be marked with `[[reflect]]` attribute (diagnostic E15-101).

**Comptime context:**

[32] Reflection queries shall execute in comptime contexts only (diagnostic E15-110).

**Concrete types:**

[33] Generic types shall be instantiated with concrete arguments (diagnostic E15-112).

**Grant requirement:**

[34] Procedures returning structured metadata (`fields_of`, `procedures_of`, etc.) require `comptime::alloc` grant for constructing result arrays.

#### §15.6.9 Diagnostics [comptime.reflect.queries.diagnostics]

[35] [Note: Diagnostics defined in this subsection are cataloged in Annex E §E.5.1.15. — end note]

#### §15.6.10 Conformance Requirements [comptime.reflect.queries.requirements]

[36] A conforming implementation SHALL:

1. Provide complete reflection API: reflect_type, fields_of, has_field, field_info_of, field_offset, procedures_of, has_procedure, procedure_info_of, variants_of, has_variant, variant_info_of, states_of, transitions_of
2. Return accurate metadata matching actual type structure, layout, and declarations
3. Include field offsets computed per §9.6 [memory.layout]
4. Include procedure grant information from sequent grants clauses
5. Include parameter responsibility flags from `move` modifiers
6. Respect visibility modifiers: limit private field details cross-module
7. Support reflection on generic types when instantiated
8. Emit diagnostics E15-101, E15-110, E15-111, E15-112, E15-120, E15-121, E15-122
9. Execute reflection queries at compile time only
10. Provide deterministic results for identical type definitions

[37] A conforming implementation MAY:

1. Optimize reflection metadata storage format
2. Cache reflection query results
3. Provide additional reflection metadata as extensions

[38] A conforming implementation SHALL NOT:

1. Allow reflection on non-reflected types
2. Execute reflection queries at runtime (in base specification)
3. Return inaccurate metadata
4. Violate visibility constraints during reflection

---

**Previous**: §15.5 Reflection Opt-In and Attributes (§15.5 [comptime.reflect.optin]) | **Next**: §15.7 Quote Expressions and Interpolation (§15.7 [comptime.quote])
