# Cursive Language Specification

## Clause 5 — Names, Scopes, and Resolution

**Clause**: 5 — Names, Scopes, and Resolution
**File**: 06-6_Predeclared-Bindings.md
**Section**: §5.6 Predeclared Bindings
**Stable label**: [name.predeclared]  
**Forward references**: Clause 2 [lex], Clause 6 [type], Clause 10 [memory], Annex E §E.5 [implementation.diagnostics]

---

### §5.6 Predeclared Bindings [name.predeclared]

#### §5.6.1 Overview [name.predeclared.overview]

[1] This subclause enumerates the identifiers that exist in the universe scope without explicit user declarations—primitive types, literals, and built-in operators—and defines the rules governing their visibility, protection, and interaction with user-defined bindings.

[2] Predeclared bindings are always reachable during lookup (Step 5 in §6.4) but cannot be redefined or shadowed. They provide the stable foundation required for type checking, literals, and intrinsic operations.

#### §6.6.2 Catalogue [name.predeclared.catalogue]

[3] Implementations shall provide at least the following universe-scope bindings:

- **Primitive scalar types**: `i8`, `i16`, `i32`, `i64`, `i128`, `isize`, `u8`, `u16`, `u32`, `u64`, `u128`, `usize`, `f32`, `f64`, `bool`, `char`, `string`.
- **Unit and never types**: `()` (unit), `!` (never).
- **Boolean constants**: `true`, `false`.
- **Numeric literal helpers**: literal suffix constructors matching the primitive types above.
- **Intrinsic operations**: `panic`, `assert` (if included in the core language; optional extensions may add more).
- **Pointer helpers**: `Ptr` (subject to §7.5).

[4] Implementations may extend this set with additional intrinsics, but they shall document the extensions and adhere to the protection rules below.

#### §6.6.3 Constraints [name.predeclared.constraints]

[5] Predeclared identifiers occupy the universe scope and are visible in every module without import. They do not obey visibility modifiers and cannot be hidden by module exports or `use` clauses.

[6] Any attempt to redeclare or shadow a predeclared identifier shall be rejected with diagnostic **E06-302** (alias for historical E3D12). The prohibition applies to both direct declarations (`let i32 = …`) and shadowing forms (`shadow let i32 = …`).

[7] Predeclared identifiers participate in lookup only after Steps 1–4 (§6.4.4). Because redeclaration is forbidden, user programs cannot mask them; if an implementation extension allows masking, it does not conform to this specification.

[8] Aliasing predeclared identifiers is permitted only through explicit declarations (for example, `type MyInt = i32`). `use` aliasing of universe-scope names is prohibited and shall be diagnosed as E06-302.

#### §6.6.4 Diagnostics [name.predeclared.diagnostics]

[9] Diagnostic **E06-302** shall include:

- The forbidden identifier.
- Whether the attempt was a redeclaration or a `shadow`.
- A note indicating that the identifier is predeclared.
- Suggested fix: choose a different name.

Structured payload requirements are defined in Annex E §E.5.

#### §6.6.5 Examples (Informative) [name.predeclared.examples]

**Example 6.6.5.1 (Using and protecting predeclared identifiers):**

```cursive
let value: i32 = 42       // OK: reference to predeclared primitive in Step 5
let flag = true && false  // OK: boolean constants from universe scope
let point: (f64, f64) = (0.0, 1.0)

// Invalid: attempting to redefine a primitive type
// let i32 = 10            // ERROR E06-302: 'i32' is predeclared

{
    // Invalid: attempting to shadow a predeclared identifier
    // shadow let bool = get_flag()  // ERROR E06-302
}

// Valid alias via type declaration
type MyInt = i32
let number: MyInt = 5
```

#### §6.6.6 Conformance Requirements [name.predeclared.requirements]

[10] Implementations shall populate the universe scope with at least the bindings listed in §6.6.2 and any additional ones promised by implementation documentation.

[11] Implementations shall reject any redeclaration, shadowing, or `use` aliasing of predeclared identifiers with E06-302 and ensure such identifiers never appear in Steps 1–4 of the lookup algorithm.

[12] Implementations shall ensure that diagnostics referencing predeclared bindings include the structured payload required by Annex E §E.5 for tooling.
