# Cursive Language Specification

## Clause 15 — Compile-Time Evaluation and Reflection

**Part**: XV — Compile-Time Evaluation and Reflection  
**File**: 16-9_Hygiene-Validation-and-Conformance.md  
**Section**: §15.9 Hygiene, Validation, and Conformance  
**Stable label**: [comptime.conformance]  
**Forward references**: §§16.1–16.8 (all compile-time subclauses), Clause 2 §2.2 [lex.phases], Annex E §E.5 [implementation.diagnostics]

---

### §15.9 Hygiene, Validation, and Conformance [comptime.conformance]

#### §15.9.1 Overview [comptime.conformance.overview]

[1] This subclause consolidates hygiene mechanisms, validation requirements, and conformance obligations for the complete compile-time evaluation and reflection system. It specifies name collision prevention, generated code validation, and the integration requirements that ensure compile-time features work correctly with the broader language.

[2] Hygiene ensures generated code does not accidentally capture user-defined names. Validation ensures generated code is type-safe and well-formed. Conformance requirements enumerate implementation obligations across all compile-time subsystems.

#### §15.9.2 Hygienic Code Generation [comptime.conformance.hygiene]

##### §15.9.2.1 Name Collision Problem

[3] Generated code may inadvertently use identifiers that collide with user-defined names, causing:

- Accidental shadowing of user bindings
- Type conflicts between generated and user types
- Compilation errors from duplicate declarations

[4] **Unsafe pattern (name collision risk):**

```cursive
comptime {
    codegen::insert_statement(quote {
        let buffer = allocate()  // May collide with user's 'buffer'
    })
}

let buffer = "user data"
// Generated code shadows user binding
```

##### §15.9.2.2 The gensym Intrinsic

[5] The `gensym` intrinsic generates globally unique identifiers:

```cursive
comptime procedure gensym(prefix: string@View): string@View
    [[ comptime::alloc |- prefix.len() > 0 => true ]]
```

[6] **Semantics.** Returns a unique identifier string of the form `{prefix}_gNNNNN` where `NNNNN` is a globally unique numeric suffix. Each call produces a distinct identifier.

[7] **Uniqueness guarantee:**

$$
\frac{\texttt{gensym}(p_1) = id_1 \quad \texttt{gensym}(p_2) = id_2}{id_1 \ne id_2}
\tag{P-Gensym-Unique}
$$

[8] The numeric suffix is globally unique across the entire compilation unit, ensuring no collisions with user code or other generated code.

##### §15.9.2.3 Hygienic Pattern

[9] **Best practice.** Use `gensym` for all local bindings in generated code:

```cursive
comptime {
    let buffer_name <- gensym("buffer")
    let temp_name <- gensym("temp")

    codegen::insert_statement(quote {
        let $(buffer_name) = allocate()
        let $(temp_name) = process($(buffer_name))
        output($(temp_name))
    })
}

// User code can safely use 'buffer' and 'temp'
let buffer = "safe"
let temp = "safe"

// Generated code uses buffer_g00001, temp_g00002
```

[10] **Collision-free guarantee.** Using `gensym` ensures generated identifiers never collide with:

- User-defined identifiers
- Other generated identifiers
- Reserved keywords (gensym avoids keywords automatically)
- Predeclared bindings

##### §15.9.2.4 Public Name Documentation

[11] When code generation creates public APIs, the generated names should be predictable and documented:

**Example 16.9.2.1** (Documented public names):

```cursive
// Documentation: This derive generates:
//   - procedure serialize(~): [u8]
//   - procedure deserialize(data: [u8]): T \/ ParseError
comptime procedure derive_serializable<T>(): ()
    [[ comptime::alloc, comptime::codegen |- true => true ]]
{
    // Generate with predictable names (documented)
    codegen::add_procedure(
        target: TypeRef::TypeId(type_id<T>()),
        spec: codegen::ProcedureSpec {
            name: "serialize",  // Documented public name
            visibility: codegen::Visibility::Public,
            // ...
        }
    )
}
```

[12] Public generated names are part of the API contract; internal generated names should use `gensym`.

#### §15.9.3 Generated Code Validation [comptime.conformance.validation]

##### §15.9.3.1 Validation Requirements

[13] All generated code shall undergo the same validation as hand-written code:

**Syntactic validation:**

[14] Generated code shall be syntactically valid. Malformed quote interpolation or invalid specifications produce diagnostic E16-240.

**Type checking:**

[15] Generated code shall type-check successfully during the type checking phase (§2.2.4.3). Type errors produce diagnostic E16-230 with context identifying the generating comptime block.

**Semantic validation:**

[16] Generated code shall satisfy all semantic constraints:

- Grant coverage (procedure grants cover all operations)
- Permission correctness (receivers and parameters have valid permissions)
- Definite assignment (all bindings initialized before use)
- Move semantics (moved values not used after move)
- Contract satisfaction (sequent preconditions/postconditions valid)

[17] Semantic violations produce standard diagnostics (E05-xxx, E07-xxx, E11-xxx, E12-xxx) annotated with generation context.

##### §15.9.3.2 Validation Failures

[18] **Diagnostic augmentation.** When generated code fails validation, diagnostics shall include:

- Original error diagnostic (e.g., E07-700 for type mismatch)
- Note identifying the comptime block or procedure that generated the code
- Source location of the generation site
- Generated code snippet showing the error
- Suggested fixes for the generation logic

**Example 16.9.3.1 - invalid** (Generated code with type error):

```cursive
comptime {
    codegen::declare_procedure(codegen::ProcedureSpec {
        name: "broken",
        visibility: codegen::Visibility::Public,
        receiver: codegen::ReceiverSpec::None,
        params: [],
        return_type: TypeRef::Named("i32"),
        sequent: sequent_pure(),
        body: quote {
            result "not an integer"  // Type error
        },
    })
}

// error[E16-230]: generated code failed type checking
//   in procedure 'broken' generated at line 1
//   type mismatch: expected i32, found string@View
// note: generated by comptime block at line 1, column 1
```

#### §15.9.4 Consolidated Conformance Requirements [comptime.conformance.requirements]

##### §15.9.4.1 Comptime Execution Requirements

[19] A conforming implementation SHALL (§§16.1–16.4):

1. Support `comptime procedure` declarations with syntax per §16.2
2. Execute comptime procedures and blocks during compile-time execution phase (§2.2.4.2)
3. Restrict comptime grants to: comptime::alloc, comptime::codegen, comptime::config, comptime::diag
4. Enforce minimum resource limits: 256 recursion, 1M steps, 64MiB memory, 1MiB strings, 10K collections
5. Provide intrinsics: comptime_assert, comptime_error, comptime_warning, comptime_note
6. Provide config queries: cfg, cfg_value, target_os, target_arch, target_endian, target_pointer_width
7. Provide type queries: type_name, type_id, size_of, align_of, is_copy, is_sized
8. Guarantee deterministic comptime evaluation
9. Evaluate comptime dependencies in topological order
10. Detect cyclic dependencies (E16-021)

##### §15.9.4.2 Reflection Requirements

[20] A conforming implementation SHALL (§§16.5–16.6):

1. Support `[[reflect]]` attribute on record, enum, modal type declarations
2. Enforce zero metadata overhead for types without `[[reflect]]`
3. Reject reflection queries on non-reflected types (E16-101)
4. Provide complete reflection API: reflect_type, fields_of, procedures_of, variants_of, states_of, transitions_of
5. Include accurate metadata: names, sizes, offsets, alignments, visibility
6. Include procedure sequent information in ProcedureInfo (grants, must, will)
7. Include parameter responsibility flags in ParamInfo
8. Restrict reflection queries to comptime contexts (E16-110)
9. Respect visibility modifiers during reflection
10. Support reflection on instantiated generic types

##### §15.9.4.3 Code Generation Requirements

[21] A conforming implementation SHALL (§§16.7–16.8):

1. Support quote expressions with `quote { ... }` and `quote expr` syntax
2. Implement quote interpolation using `$(...)` syntax (dollar sign)
3. Provide TypeRef enumeration with all variants
4. Provide specification structures: ProcedureSpec, TypeSpec, ParamSpec, SequentSpec, etc.
5. Implement codegen API: declare_procedure, declare_type, declare_constant, add_procedure, add_function
6. Provide gensym() for unique identifier generation
7. Type-check generated code during type checking phase
8. Detect name collisions (E16-210 through E16-213)
9. Validate TypeRef references (E16-220)
10. Validate codegen targets exist (E16-221)

##### §15.9.4.4 Hygiene Requirements

[22] A conforming implementation SHALL:

1. Implement gensym() producing globally unique identifiers
2. Guarantee gensym() never produces duplicate identifiers
3. Support identifier interpolation via `$(string_value)`
4. Allow quote expressions to use gensym() for collision-free names
5. Detect and report name collisions in generated code

#### §15.9.5 Complete Diagnostic Catalog [comptime.conformance.diagnostics]

[23] [Note: All diagnostic codes defined in Clause 15 are cataloged in Annex E §E.5.1.15. — end note]

#### §15.9.6 Integration Summary [comptime.conformance.integration]

[25] The compile-time system integrates with:

**Translation phases (Clause 2):**

- Comptime execution during §2.2.4.2
- Code generation during §2.2.4.4
- Type checking validates generated code in §2.2.4.3

**Declarations (Clause 5):**

- Comptime procedures follow §5.4 with `comptime` modifier
- Generated declarations follow §5.4 and §5.5

**Type system (Clause 7):**

- TypeRef references all type constructors
- Generated code type-checked per Clause 7 rules
- Reflection builds on §7.8

**Expressions (Clause 8):**

- Comptime blocks are expressions (§8.7)
- Quote extends expression grammar

**Generics (Clause 10):**

- Generic parameters in comptime procedures
- Reflection on generic types when instantiated

**Memory model (Clause 11):**

- Comptime::alloc provides compile-time allocation
- No region escape analysis for comptime allocations

**Contracts (Clause 12):**

- Comptime grant system extends general grants
- Generated procedures include full sequents

**Annex A (Grammar):**

- Quote expression productions
- Comptime block productions
- [[reflect]] attribute production

**Annex E (Diagnostics):**

- E16-001 through E16-299 diagnostic codes
- Structured payload specifications

#### §15.9.7 Best Practices [comptime.conformance.practices]

##### §15.9.7.1 Recommended Patterns

[26] **Use gensym for local names:**

```cursive
// GOOD: Hygienic
let temp <- gensym("temp")
codegen::insert_statement(quote {
    let $(temp) = compute()
})

// AVOID: May collide
codegen::insert_statement(quote {
    let temp = compute()  // Collision risk
})
```

[27] **Document public APIs:**

```cursive
// Generate public procedures with documented names
// GOOD: Predictable API
codegen::add_procedure(target, spec {
    name: "serialize",  // Documented public API
    visibility: Public,
    // ...
})
```

[28] **Validate generated code:**

```cursive
// Add assertions to detect generation errors early
comptime {
    let type_count = 0

    loop type_name in type_names {
        generate_for_type(type_name)
        type_count = type_count + 1
    }

    comptime_assert(type_count > 0, "Should have generated at least one type")
}
```

##### §15.9.7.2 Anti-Patterns

[29] **Avoid:**

- String-based code generation without quotes (type-unsafe)
- Hardcoded identifier names in quotes without gensym (collision-prone)
- Excessive comptime computation approaching resource limits (brittle)
- Reflection on types without `[[reflect]]` attribute (will fail)
- Generating code with runtime grants in comptime procedures (will fail)

#### §15.9.8 Complete Conformance Requirements [comptime.conformance.complete]

[30] A conforming implementation of Clause 16 SHALL:

**Execution and Evaluation:**

1. Support comptime procedures with `comptime` keyword modifier
2. Support comptime blocks with `comptime { ... }` syntax
3. Execute during compile-time execution phase (§2.2.4.2)
4. Restrict to comptime-safe grants: comptime::alloc, comptime::codegen, comptime::config, comptime::diag
5. Enforce purity (no runtime side effects except codegen)
6. Guarantee termination within resource limits
7. Provide deterministic evaluation for identical inputs
8. Evaluate dependencies in topological order

**Intrinsics and Configuration:**

9. Provide assertion intrinsics: comptime_assert, comptime_error, comptime_warning, comptime_note
10. Provide config queries: cfg, cfg_value
11. Provide platform queries: target_os, target_arch, target_endian, target_pointer_width
12. Provide type queries: type_name, type_id, size_of, align_of, is_copy, is_sized
13. Provide string utilities: string_concat, usize_to_string, parse_usize

**Reflection:**

14. Support `[[reflect]]` attribute on record, enum, modal types
15. Enforce zero metadata overhead for non-reflected types
16. Provide complete reflection API with accurate metadata
17. Restrict reflection to comptime contexts
18. Respect visibility during reflection queries
19. Include procedure sequent and parameter responsibility in metadata

**Code Generation:**

20. Support quote expressions with `$(...)` interpolation syntax
21. Provide TypeRef enumeration with all type constructor variants
22. Provide complete specification structures (ProcedureSpec, TypeSpec, etc.)
23. Implement codegen API (declare_procedure, declare_type, declare_constant, add_procedure, add_function)
24. Provide gensym() for unique identifier generation
25. Type-check generated code during type checking phase
26. Detect name collisions in generated code

**Diagnostics:**

27. Emit diagnostics E16-001 through E16-299 for all violations
28. Provide structured diagnostic payloads per Annex E §E.5
29. Trace generated code errors to generation sites

**Resource Limits (minimum):**

30. Recursion depth: 256 levels
31. Evaluation steps: 1,000,000 per comptime block
32. Memory allocation: 64 MiB per compilation unit
33. String size: 1 MiB per string value
34. Collection size: 10,000 elements

[31] A conforming implementation MAY:

1. Exceed minimum resource limits
2. Provide additional comptime intrinsics
3. Provide additional codegen API procedures
4. Optimize comptime evaluation and code generation
5. Cache comptime results across compilations
6. Provide runtime reflection (outside specification scope)

[32] A conforming implementation SHALL NOT:

1. Allow runtime grants in comptime contexts
2. Execute comptime code non-deterministically
3. Generate metadata for non-reflected types
4. Skip validation of generated code
5. Allow name collisions without diagnostics
6. Use interpolation syntax other than `$(...)`
7. Execute reflection queries at runtime (in base specification)
8. Violate zero-cost guarantee for unused features

#### §15.9.9 Quality of Implementation Recommendations [comptime.conformance.quality]

[33] Implementations SHOULD:

1. Provide clear diagnostic messages for comptime errors
2. Show comptime evaluation progress for long-running generation
3. Warn when approaching resource limits (50% thresholds)
4. Provide debugging tools for comptime code
5. Cache comptime results for incremental compilation
6. Optimize generated code alongside hand-written code
7. Provide IDE support for quote expressions and generated code
8. Generate readable code with proper formatting
9. Preserve comments in generated code when applicable
10. Provide tooling to inspect generated code

#### §15.9.10 Examples [comptime.conformance.examples]

**Example 16.9.10.1** (Complete derive pattern with hygiene):

```cursive
[[reflect]]
record Data {
    value: i32,
    label: string@View,
}

comptime procedure derive_debug<T>(): ()
    [[ comptime::alloc, comptime::codegen |- true => true ]]
{
    let type_name_val <- type_name<T>()
    let fields <- fields_of::<T>()

    let result_name <- gensym("result")
    let field_value_name <- gensym("field_value")

    codegen::add_procedure(
        target: TypeRef::TypeId(type_id<T>()),
        spec: codegen::ProcedureSpec {
            name: "debug",  // Public API name (documented)
            visibility: codegen::Visibility::Public,
            receiver: codegen::ReceiverSpec::Const,
            params: [],
            return_type: TypeRef::Named("string@Managed"),
            sequent: sequent_with_grants(["alloc::heap"]),
            body: quote {
                let $(result_name) = string::from($(type_name_val))
                $(result_name) = string_concat($(result_name), " { ")

                // Hygienic field iteration
                loop field_idx in 0..$(fields.len()) {
                    let $(field_value_name) = get_field_string(field_idx)
                    $(result_name) = string_concat($(result_name), $(field_value_name))
                }

                $(result_name) = string_concat($(result_name), " }")
                result $(result_name)
            },
        }
    )

    comptime_note(string_concat("Generated debug for ", type_name_val))
}

comptime { derive_debug::<Data>() }

// Usage:
let data = Data { value: 42, label: "test" }
println("{}", data::debug())
// Output: "Data { value: 42, label: test }"
```

[1] Demonstrates hygiene (gensym), reflection (fields_of), quote (interpolation), and code generation (add_procedure) working together.

#### §15.9.11 Conformance Summary [comptime.conformance.summary]

[34] Conforming implementations shall implement all features specified in §§16.1 through 16.9, emit all diagnostics E16-001 through E16-299, and satisfy all SHALL requirements. Deviations render an implementation non-conforming unless explicitly categorized as implementation-defined elsewhere in this specification.

[35] The compile-time evaluation and reflection system is a cohesive whole: comptime execution enables code generation, reflection provides the data for metaprogramming, quotes and interpolation enable type-safe code templates, and hygiene prevents name collisions. All components work together to provide powerful, safe, zero-cost compile-time capabilities.

---

**Previous**: §16.8 Code Generation API and Specifications (§16.8 [comptime.codegen.api]) | **Next**: Annex A — Grammar (Annex A [grammar])
