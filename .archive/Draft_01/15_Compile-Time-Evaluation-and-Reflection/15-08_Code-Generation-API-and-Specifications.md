# Cursive Language Specification

## Clause 15 — Compile-Time Evaluation and Reflection

**Part**: XV — Compile-Time Evaluation and Reflection  
**File**: 16-8_Code-Generation-API-and-Specifications.md  
**Section**: §15.8 Code Generation API and Specifications  
**Stable label**: [comptime.codegen.api]  
**Forward references**: §16.2 [comptime.procedures], §16.3 [comptime.blocks], §16.6 [comptime.reflect.queries], §16.7 [comptime.quote], §16.9 [comptime.conformance], Clause 2 §2.2 [lex.phases], Clause 5 §5.4 [decl.function], Clause 5 §5.5 [decl.type], Clause 7 [type], Clause 10 [generic], Clause 12 [contract]

---

### §15.8 Code Generation API and Specifications [comptime.codegen.api]

#### §15.8.1 Overview [comptime.codegen.api.overview]

[1] The code generation API provides explicit procedures for programmatically creating declarations during compilation. All code generation uses type-safe specification structures rather than string templates, ensuring generated code is validated and type-checked.

[2] This subclause specifies the complete codegen API: TypeRef enumeration, specification structures (ProcedureSpec, TypeSpec, etc.), codegen namespace procedures, and the integration with the code generation phase (§2.2.4.4).

[3] Code generation requires the `comptime::codegen` grant and executes during the compile-time execution phase, with declarations emitted during the code generation phase for subsequent type checking.

#### §15.8.2 TypeRef Enumeration [comptime.codegen.api.typeref]

##### §15.8.2.1 TypeRef Structure

[4] The `TypeRef` enumeration represents type references in code generation specifications:

```cursive
enum TypeRef {
    Named(string@View),
    SelfType,
    Generic(string@View, [TypeRef]),
    Tuple([TypeRef]),
    Array(TypeRef, usize),
    Slice(TypeRef),
    Pointer(TypeRef, Permission),
    Function([TypeRef], TypeRef, [string@View]),
    TypeId(u64),
}
```

[5] **Variant semantics:**

- `Named(name)`: Reference type by fully-qualified name (e.g., "i32", "module::Point")
- `SelfType`: Reference to the implementing type (used in procedure receivers and return types)
- `Generic(name, args)`: Generic type instantiation (e.g., "Container" with args [TypeRef::Named("i32")])
- `Tuple(types)`: Tuple type with element type array
- `Array(elem, size)`: Array type `[elem; size]`
- `Slice(elem)`: Slice type `[elem]`
- `Pointer(pointee, perm)`: Pointer type with permission (const/unique/shared)
- `Function(params, ret, grants)`: Callable type with parameter types, return type, grant identifiers
- `TypeId(id)`: Reference type by its compile-time type ID from `type_id<T>()`

##### §15.8.2.2 TypeRef Construction

[6] TypeRef values are constructed in comptime contexts:

**Example 16.8.2.1** (TypeRef construction examples):

```cursive
comptime {
    // Primitive type
    let int_ref = TypeRef::Named("i32")

    // Array type
    let buffer_ref = TypeRef::Array(TypeRef::Named("u8"), 1024)

    // Generic type
    let container_ref = TypeRef::Generic(
        "Container",
        [TypeRef::Named("i32")]
    )

    // Tuple type
    let pair_ref = TypeRef::Tuple([
        TypeRef::Named("i32"),
        TypeRef::Named("string@View")
    ])

    // Pointer type with permission
    let ptr_ref = TypeRef::Pointer(
        TypeRef::Named("Buffer"),
        Permission::Unique
    )

    // Function type
    let callback_ref = TypeRef::Function(
        [TypeRef::Named("i32")],
        TypeRef::Named("bool"),
        []  // No grants
    )

    // Type by ID
    let by_id = TypeRef::TypeId(type_id<Point>())
}
```

#### §15.8.3 Specification Structures [comptime.codegen.api.specs]

##### §15.8.3.1 ProcedureSpec

[7] Complete procedure specification:

```cursive
record ProcedureSpec {
    name: string@View,
    visibility: Visibility,
    receiver: ReceiverSpec,
    params: [ParamSpec],
    return_type: TypeRef,
    sequent: SequentSpec,
    body: QuotedBlock,
}

enum ReceiverSpec {
    None,
    Const,
    Shared,
    Unique,
}

record ParamSpec {
    name: string@View,
    ty: TypeRef,
    permission: Permission,
    responsible: bool,
}

enum Permission {
    Const,
    Unique,
    Shared,
}

record SequentSpec {
    grants: [string@View],
    must: [string@View],
    will: [string@View],
}

enum Visibility {
    Public,
    Internal,
    Private,
    Protected,
}
```

[8] **ProcedureSpec field semantics:**

- `name`: Procedure identifier
- `visibility`: Visibility modifier (public/internal/private/protected)
- `receiver`: Receiver specification (None for static, Const/Shared/Unique for instance procedures)
- `params`: Parameter array in declaration order
- `return_type`: Return type reference
- `sequent`: Contractual sequent specification
- `body`: Quoted procedure body

[9] **ParamSpec field semantics:**

- `name`: Parameter identifier
- `ty`: Parameter type reference
- `permission`: Parameter permission (const/unique/shared)
- `responsible`: `true` if parameter should have `move` modifier (responsible), `false` for non-responsible

[10] **SequentSpec field semantics:**

- `grants`: Array of grant identifiers (e.g., ["alloc::heap", "io::write"])
- `must`: Array of precondition expressions as strings (empty for `true`)
- `will`: Array of postcondition expressions as strings (empty for `true`)

##### §15.8.3.2 TypeSpec

[11] Complete type specification:

```cursive
record TypeSpec {
    kind: TypeKind,
    name: string@View,
    visibility: Visibility,
    generics: [GenericParamSpec],
    fields: [FieldSpec],
    procedures: [ProcedureSpec],
    variants: [VariantSpec],
}

enum TypeKind {
    Record,
    Enum,
    Tuple,
}

record FieldSpec {
    name: string@View,
    ty: TypeRef,
    visibility: Visibility,
}

record VariantSpec {
    name: string@View,
    payload: [TypeRef],
}

record GenericParamSpec {
    name: string@View,
    kind: GenericKind,
    bounds: [string@View],
}

enum GenericKind {
    Type,
    Const(TypeRef),
    Grant,
}
```

[12] **TypeSpec field semantics:**

- `kind`: Type category (Record, Enum, or Tuple)
- `name`: Type identifier
- `visibility`: Visibility modifier
- `generics`: Array of generic parameter specifications
- `fields`: Array of field specifications (for records/tuples)
- `procedures`: Array of associated procedure specifications
- `variants`: Array of variant specifications (for enums)

##### §15.8.3.3 Helper Constructors

[13] Helper procedures for common specification patterns:

```cursive
comptime procedure sequent_pure(): SequentSpec
    [[ |- true => true ]]
{
    result SequentSpec {
        grants: [],
        must: [],
        will: [],
    }
}

comptime procedure sequent_with_grants(grants: [string@View]): SequentSpec
    [[ |- grants.len() > 0 => true ]]
{
    result SequentSpec {
        grants: grants,
        must: [],
        will: [],
    }
}

comptime procedure param_const(name: string@View, ty: TypeRef): ParamSpec
    [[ |- name.len() > 0 => true ]]
{
    result ParamSpec {
        name: name,
        ty: ty,
        permission: Permission::Const,
        responsible: false,
    }
}

comptime procedure param_unique(name: string@View, ty: TypeRef): ParamSpec
    [[ |- name.len() > 0 => true ]]
{
    result ParamSpec {
        name: name,
        ty: ty,
        permission: Permission::Unique,
        responsible: false,
    }
}
```

#### §15.8.4 Codegen API Procedures [comptime.codegen.api.procedures]

##### §15.8.4.1 Declaration Generation

[14] Core codegen procedures:

**declare_procedure**:

```cursive
comptime procedure codegen::declare_procedure(spec: ProcedureSpec): ()
    [[ comptime::codegen |- true => true ]]
```

[15] **Semantics.** Emits a procedure declaration based on the specification. The procedure name shall not conflict with existing declarations (diagnostic E16-210). Generated procedure is validated during type checking phase.

**declare_type**:

```cursive
comptime procedure codegen::declare_type(spec: TypeSpec): ()
    [[ comptime::codegen |- true => true ]]
```

[16] **Semantics.** Emits a type declaration (record or enum). Type name shall not conflict (diagnostic E16-211). Fields and procedures are validated for well-formedness.

**declare_constant**:

```cursive
comptime procedure codegen::declare_constant(
    name: string@View,
    ty: TypeRef,
    value: QuotedExpr
): ()
    [[ comptime::codegen |- name.len() > 0 => true ]]
```

[17] **Semantics.** Emits a module-level constant binding. The constant name shall not conflict (diagnostic E16-212). The quoted value shall be compile-time evaluable.

##### §15.8.4.2 Type Extension Procedures

[18] Procedures for extending existing types:

**add_procedure**:

```cursive
comptime procedure codegen::add_procedure(
    target: TypeRef,
    spec: ProcedureSpec
): ()
    [[ comptime::codegen |- true => true ]]
```

[19] **Semantics.** Adds a procedure to an existing type. Target type shall exist and be visible. Procedure name shall not conflict with existing procedures on the type (diagnostic E16-213).

**add_function**:

```cursive
comptime procedure codegen::add_function(
    target: TypeRef,
    spec: ProcedureSpec
): ()
    [[ comptime::codegen |- true => true ]]
```

[20] **Semantics.** Adds an associated function (static procedure) to an existing type. Similar constraints as `add_procedure`.

##### §15.8.4.3 Validation Rules

[21] **Name collision detection:**

$$
\frac{\text{Generated name } n \text{ already exists in scope}}{\text{ERROR E16-210/E16-211/E16-212: name collision}}
\tag{WF-Codegen-UniqueNames}
$$

[22] **Type well-formedness:**

$$
\frac{\text{TypeRef } r \text{ references non-existent type}}{\text{ERROR E16-220: type reference invalid}}
\tag{WF-Codegen-TypeRef}
$$

[23] **Target existence:**

$$
\frac{\texttt{add\_procedure}(\text{target}, \text{spec}) \quad target \text{ not found}}{\text{ERROR E16-221: target type not found}}
\tag{WF-Codegen-Target}
$$

#### §15.8.5 Complete Code Generation Examples [comptime.codegen.api.examples]

##### §15.8.5.1 Automatic Serialization

**Example 16.8.5.1** (Complete serialization generation):

```cursive
[[reflect]]
record Message {
    id: u64,
    content: string@Managed,
    timestamp: i64,
}

comptime procedure generate_serialization<T>(): ()
    [[ comptime::alloc, comptime::codegen |- true => true ]]
{
    let fields <- fields_of::<T>()

    // Generate serialize procedure
    codegen::add_procedure(
        target: TypeRef::TypeId(type_id<T>()),
        spec: codegen::ProcedureSpec {
            name: "serialize",
            visibility: codegen::Visibility::Public,
            receiver: codegen::ReceiverSpec::Const,
            params: [],
            return_type: TypeRef::Slice(TypeRef::Named("u8")),
            sequent: sequent_with_grants(["alloc::heap"]),
            body: build_serialize_body(fields),
        }
    )

    // Generate deserialize function
    codegen::add_function(
        target: TypeRef::TypeId(type_id<T>()),
        spec: codegen::ProcedureSpec {
            name: "deserialize",
            visibility: codegen::Visibility::Public,
            receiver: codegen::ReceiverSpec::None,
            params: [
                param_const("data", TypeRef::Slice(TypeRef::Named("u8"))),
            ],
            return_type: TypeRef::Generic("Result", [
                TypeRef::Named(type_name<T>()),
                TypeRef::Named("ParseError")
            ]),
            sequent: sequent_with_grants(["alloc::heap"]),
            body: build_deserialize_body(fields),
        }
    )
}

comptime procedure build_serialize_body(fields: [FieldInfo]): QuotedBlock
    [[ comptime::alloc |- fields.len() > 0 => true ]]
{
    let buffer_name <- gensym("buffer")

    result quote {
        let $(buffer_name): [u8] = []

        // Serialize each field (simplified - actual implementation would iterate properly)
        $(buffer_name) = append_field_bytes($(buffer_name), field_data)

        result $(buffer_name)
    }
}

comptime procedure build_deserialize_body(fields: [FieldInfo]): QuotedBlock
    [[ comptime::alloc |- fields.len() > 0 => true ]]
{
    result quote {
        // Parse fields from data (simplified)
        result parse_from_bytes(data)
    }
}

comptime { generate_serialization::<Message>() }

// Generated procedures available:
let msg = Message { id: 1, content: string::from("Hello"), timestamp: 1234567890 }
let serialized = msg::serialize()
```

##### §15.8.5.2 Builder Pattern

**Example 16.8.5.2** (Complete builder generation):

```cursive
[[reflect]]
record User {
    name: string@Managed,
    email: string@Managed,
    age: u32,
}

comptime procedure generate_builder<T>(): ()
    [[ comptime::alloc, comptime::codegen |- true => true ]]
{
    let type_name_val <- type_name<T>()
    let fields <- fields_of::<T>()

    var builder_fields: [codegen::FieldSpec] = []
    var setter_procedures: [codegen::ProcedureSpec] = []

    // Create optional field for each field
    loop field: FieldInfo in fields {
        builder_fields = array_push(builder_fields, codegen::FieldSpec {
            name: field.name,
            ty: TypeRef::Generic("Optional", [TypeRef::Named(field.ty_name)]),
            visibility: codegen::Visibility::Private,
        })

        // Create setter procedure
        setter_procedures = array_push(setter_procedures, codegen::ProcedureSpec {
            name: field.name,
            visibility: codegen::Visibility::Public,
            receiver: codegen::ReceiverSpec::Unique,
            params: [param_const("value", TypeRef::Named(field.ty_name))],
            return_type: TypeRef::SelfType,
            sequent: sequent_pure(),
            body: quote {
                self.$(field.name) = Optional::Some(value)
                result self
            },
        })
    }

    // Add build procedure
    let build_proc = codegen::ProcedureSpec {
        name: "build",
        visibility: codegen::Visibility::Public,
        receiver: codegen::ReceiverSpec::Unique,
        params: [],
        return_type: TypeRef::Generic("Result", [
            TypeRef::Named(type_name_val),
            TypeRef::Named("BuildError")
        ]),
        sequent: sequent_pure(),
        body: build_builder_build_procedure(fields),
    }

    setter_procedures = array_push(setter_procedures, build_proc)

    // Declare builder type
    codegen::declare_type(codegen::TypeSpec {
        kind: codegen::TypeKind::Record,
        name: string_concat(type_name_val, "Builder"),
        visibility: codegen::Visibility::Public,
        generics: [],
        fields: builder_fields,
        procedures: setter_procedures,
        variants: [],
    })
}

comptime procedure build_builder_build_procedure(fields: [FieldInfo]): QuotedBlock
    [[ comptime::alloc |- fields.len() > 0 => true ]]
{
    result quote {
        // Validate all fields are set (simplified)
        if !all_fields_set() {
            result Err(BuildError::MissingField)
        }

        result Ok(construct_from_optionals())
    }
}

comptime { generate_builder::<User>() }

// Usage:
let user = UserBuilder::new()
    ::name(string::from("Alice"))
    ::email(string::from("alice@example.com"))
    ::age(30)
    ::build()?
```

##### §15.8.5.3 Debug Formatting

**Example 16.8.5.3** (Debug formatting generation):

```cursive
[[reflect]]
record Point {
    x: f64,
    y: f64,
}

comptime procedure generate_debug<T>(): ()
    [[ comptime::alloc, comptime::codegen |- true => true ]]
{
    let type_name_val <- type_name<T>()
    let fields <- fields_of::<T>()

    codegen::add_procedure(
        target: TypeRef::TypeId(type_id<T>()),
        spec: codegen::ProcedureSpec {
            name: "debug_string",
            visibility: codegen::Visibility::Public,
            receiver: codegen::ReceiverSpec::Const,
            params: [],
            return_type: TypeRef::Named("string@Managed"),
            sequent: sequent_with_grants(["alloc::heap"]),
            body: build_debug_body(type_name_val, fields),
        }
    )
}

comptime procedure build_debug_body(type_name: string@View, fields: [FieldInfo]): QuotedBlock
    [[ comptime::alloc |- fields.len() > 0 => true ]]
{
    let result_name <- gensym("result")

    result quote {
        let $(result_name) = string::from($(type_name))
        $(result_name) = string_concat($(result_name), " { ")

        // Append field representations (simplified)
        $(result_name) = string_concat($(result_name), field_representations)

        $(result_name) = string_concat($(result_name), " }")
        result $(result_name)
    }
}

comptime { generate_debug::<Point>() }

let p = Point { x: 3.0, y: 4.0 }
println("{}", p::debug_string())
// Output: "Point { x: 3.0, y: 4.0 }"
```

#### §15.8.6 Struct-of-Arrays Transformation [comptime.codegen.api.soa]

**Example 16.8.6.1** (SoA for cache-friendly layout):

```cursive
[[reflect]]
record Entity {
    position: [f32; 3],
    velocity: [f32; 3],
    health: f32,
}

comptime procedure generate_soa<T, const N: usize>(): ()
    [[ comptime::alloc, comptime::codegen |- N > 0, N <= 10000 => true ]]
{
    let type_name_val <- type_name<T>()
    let fields <- fields_of::<T>()

    var soa_fields: [codegen::FieldSpec] = []

    // Transform F: T into Fs: [T; N]
    loop field: FieldInfo in fields {
        soa_fields = array_push(soa_fields, codegen::FieldSpec {
            name: string_concat(field.name, "s"),
            ty: TypeRef::Array(TypeRef::Named(field.ty_name), N),
            visibility: field.visibility,
        })
    }

    // Generate get(index) procedure
    let get_body = build_soa_get_body(fields)

    let get_proc = codegen::ProcedureSpec {
        name: "get",
        visibility: codegen::Visibility::Public,
        receiver: codegen::ReceiverSpec::Const,
        params: [param_const("index", TypeRef::Named("usize"))],
        return_type: TypeRef::Named(type_name_val),
        sequent: codegen::SequentSpec {
            grants: [],
            must: [string_concat("index < ", usize_to_string(N))],
            will: [],
        },
        body: get_body,
    }

    // Generate set(index, value) procedure
    let set_body = build_soa_set_body(fields)

    let set_proc = codegen::ProcedureSpec {
        name: "set",
        visibility: codegen::Visibility::Public,
        receiver: codegen::ReceiverSpec::Unique,
        params: [
            param_const("index", TypeRef::Named("usize")),
            param_const("value", TypeRef::Named(type_name_val)),
        ],
        return_type: TypeRef::Named("()"),
        sequent: codegen::SequentSpec {
            grants: [],
            must: [string_concat("index < ", usize_to_string(N))],
            will: [],
        },
        body: set_body,
    }

    // Declare SoA type
    codegen::declare_type(codegen::TypeSpec {
        kind: codegen::TypeKind::Record,
        name: string_concat(string_concat(type_name_val, "SoA"), usize_to_string(N)),
        visibility: codegen::Visibility::Public,
        generics: [],
        fields: soa_fields,
        procedures: [get_proc, set_proc],
        variants: [],
    })
}

comptime procedure build_soa_get_body(fields: [FieldInfo]): QuotedBlock
    [[ comptime::alloc |- fields.len() > 0 => true ]]
{
    result quote {
        // Construct T from SoA arrays
        result construct_from_arrays(index)
    }
}

comptime procedure build_soa_set_body(fields: [FieldInfo]): QuotedBlock
    [[ comptime::alloc |- fields.len() > 0 => true ]]
{
    result quote {
        // Store value fields into SoA arrays
        store_to_arrays(index, value)
    }
}

comptime { generate_soa::<Entity, 1024>() }

// Generated: EntitySoA1024 with array-of-structs layout transformed to struct-of-arrays
let entities = EntitySoA1024::default()
```

#### §15.8.7 Generated Code Validation [comptime.codegen.api.validation]

##### §15.8.7.1 Type Checking Phase

[24] Generated declarations are type-checked during the type checking phase (§2.2.4.3), after all code generation completes:

**Validation sequence:**

```
1. Comptime execution phase (§2.2.4.2)
   → Execute comptime blocks
   → Call codegen API procedures
   → Build declaration specifications

2. Code generation phase (§2.2.4.4)
   → Emit generated declarations to AST
   → Merge with original code

3. Type checking phase (§2.2.4.3)
   → Type check all code (original + generated)
   → Validate well-formedness
   → Emit diagnostics for errors
```

[25] **Type checking requirement:**

$$
\frac{\text{Generated declaration } d \text{ added to AST}}{\Gamma \vdash d : \text{well-formed} \quad \text{or ERROR E16-230}}
\tag{WF-Generated-Typechecked}
$$

[26] If generated code fails type checking, diagnostic E16-230 includes:

- Generated code location
- Originating comptime block/procedure
- Type-checking error details
- Suggested fixes

##### §15.8.7.2 Semantic Validation

[27] Beyond syntax and types, generated code is validated for:

- Sequent grant subsumption (called procedures' grants covered by caller's grants)
- Permission correctness (receivers match procedure permissions)
- Definite assignment (all paths initialize bindings)
- Move semantics (moved values not used after move)

[28] Violations produce standard diagnostics (E05-xxx, E07-xxx, E11-xxx) with additional context noting the code was generated.

#### §15.8.8 Diagnostics [comptime.codegen.api.diagnostics]

[29] [Note: Diagnostics defined in this subsection are cataloged in Annex E §E.5.1.15. — end note]

#### §15.8.9 Conformance Requirements [comptime.codegen.api.requirements]

[30] A conforming implementation SHALL:

1. Provide complete TypeRef enumeration with all specified variants
2. Provide all specification structures: ProcedureSpec, TypeSpec, FieldSpec, ParamSpec, VariantSpec, SequentSpec, GenericParamSpec
3. Implement codegen API: declare_procedure, declare_type, declare_constant, add_procedure, add_function
4. Validate TypeRef references resolve to existing types (E16-220)
5. Detect name collisions between generated and existing declarations (E16-210 through E16-213)
6. Validate target types exist for add_procedure/add_function (E16-221)
7. Type-check generated code during type checking phase (E16-230)
8. Support permission specifications in ParamSpec (const/unique/shared)
9. Support parameter responsibility in ParamSpec (responsible flag)
10. Support receiver specifications (None/Const/Shared/Unique)
11. Support full sequent specifications with grants, must, will clauses
12. Execute code generation during code generation phase (§2.2.4.4)
13. Emit diagnostics E16-210 through E16-233 for violations

[31] A conforming implementation MAY:

1. Provide additional specification structure fields as extensions
2. Provide additional codegen API procedures
3. Optimize code generation for performance
4. Provide enhanced diagnostics tracing generated code

[32] A conforming implementation SHALL NOT:

1. Generate code that violates type safety
2. Skip type checking of generated code
3. Allow name collisions without diagnostics
4. Generate code from malformed specifications without validation

---

**Previous**: §16.7 Quote Expressions and Interpolation (§16.7 [comptime.quote]) | **Next**: §16.9 Hygiene, Validation, and Conformance (§16.9 [comptime.conformance])
