# Cursive Language Specification

## Clause 14 — Interoperability and ABI

**Part**: XIV — Interoperability and ABI  
**File**: 15-5_Linkage-Symbol-Visibility.md  
**Section**: §14.5 Linkage and Symbol Visibility  
**Stable label**: [interop.linkage]  
**Forward references**: §15.1 [interop.ffi], §15.6 [interop.abi], Clause 4 [module], Clause 5 §5.6 [decl.visibility], Annex B §B.2 [behavior.undefined]

---

### §14.5 Linkage and Symbol Visibility [interop.linkage]

#### §14.5.1 Overview

[1] Linkage determines whether a symbol (the name of an entity) is visible across compilation unit boundaries in the compiled binary and at link time. Linkage is distinct from but related to visibility: visibility controls compile-time name resolution and access control (§5.6), while linkage controls symbol table entries in compiled binaries and cross-unit linking behavior.

[2] This subclause specifies linkage categories (external, internal, no linkage), the relationship between visibility and linkage, the One Definition Rule (ODR), and linkage control attributes.

#### §14.5.2 Linkage Categories

##### §14.5.2.1 Definition

[3] _Linkage_ is the property of a declared entity that determines whether its symbol appears in the binary symbol table and whether that symbol is visible across compilation unit boundaries.

[4] Every declared entity has exactly one of three linkage categories:

- **External linkage**: Symbol exported in binary, visible across compilation units
- **Internal linkage**: Symbol local to compilation unit or package, not exported
- **No linkage**: No symbol in binary (inline or eliminated)

##### §14.5.2.2 Linkage Determination Rule

[5] Linkage is determined by visibility modifiers and entity kind:

| Visibility | Entity Kind        | Linkage  | Symbol Exported |
| ---------- | ------------------ | -------- | --------------- |
| `public`   | Module-scope       | External | YES             |
| `internal` | Module-scope       | Internal | NO              |
| `private`  | Module-scope       | None     | NO              |
| (any)      | Block-scope (local | ) None   | NO              |
| (any)      | Parameter          | None     | NO              |

[6] **Linkage determination judgment**:

[ Given: Entity $E$ with visibility $V$ and scope $S$ ]

$$
\frac{V = \texttt{public} \quad S = \texttt{module}}
     {\text{linkage}(E) = \texttt{External}}
\tag{Linkage-External}
$$

$$
\frac{V = \texttt{internal} \quad S = \texttt{module}}
     {\text{linkage}(E) = \texttt{Internal}}
\tag{Linkage-Internal}
$$

$$
\frac{V = \texttt{private} \lor S = \texttt{block}}
     {\text{linkage}(E) = \texttt{None}}
\tag{Linkage-None}
$$

#### §14.5.3 External Linkage

##### §14.5.3.1 Definition

[7] An entity has _external linkage_ when its symbol is exported in the compiled binary and can be referenced from other compilation units after linking.

[8] **Properties of external linkage**:

(8.1) Symbol appears in binary's exported symbol table

(8.2) Entity may be referenced from other compilation units via qualified names

(8.3) Name mangling may be applied (§15.5.6) unless overridden by attributes

(8.4) Subject to One Definition Rule across entire program (§15.5.5)

##### §14.5.3.2 External Linkage Example

**Example 15.5.3.1 (External linkage):**

```cursive
// File: math.cursive
public procedure add(x: i32, y: i32): i32
    [[ |- true => true ]]
{
    result x + y
}
// Symbol exported with external linkage
// Binary symbol: _Cursive_math_add (example mangled name)

// File: main.cursive
import math

procedure main(): i32
    [[ |- true => true ]]
{
    let sum = math::add(5, 3)  // Links to external symbol
    result 0
}
```

#### §14.5.4 Internal Linkage

##### §14.5.4.1 Definition

[9] An entity has _internal linkage_ when its symbol may appear in the binary as a local symbol but is not exported for cross-compilation-unit access.

[10] **Properties of internal linkage**:

(10.1) Symbol may appear in binary as local (non-exported) symbol

(10.2) Not accessible from other compilation units outside the package

(10.3) Compiler may inline, optimize, or eliminate as dead code

(10.4) Not subject to ODR across compilation units (may have multiple definitions in different units)

##### §14.5.4.2 Internal Linkage Example

**Example 15.5.4.1 (Internal linkage):**

```cursive
// File: math.cursive
internal procedure helper(x: i32): i32
    [[ |- true => true ]]
{
    result x * 2
}
// Internal linkage: local to this compilation unit

public procedure compute(x: i32): i32
    [[ |- true => true ]]
{
    result helper(x) + 1  // OK: same compilation unit
}

// File: main.cursive
import math

procedure main(): i32
{
    // math::helper(5)  // ERROR: not visible (internal linkage)
    let result = math::compute(5)  // OK: public (external linkage)
    result 0
}
```

#### §14.5.5 One Definition Rule (ODR)

##### §14.5.5.1 ODR for Types

[11] Types with external linkage shall have exactly one definition across the entire program:

[ Given: Program $P$ with type definitions $D_1$, $D_2$ both defining type $T$ ]

$$
\frac{D_1 \neq D_2 \text{ (structurally distinct)} \quad \text{both have external linkage}}
     {\text{ERROR E15-060: Multiple conflicting definitions of } T}
\tag{WF-ODR-Type}
$$

[12] Structural equivalence: Two type definitions are identical when they have the same fields, field types, field order, and representation attributes.

**Example 15.5.5.1 - invalid (ODR violation for types):**

```cursive
// File: types_a.cursive
public record Point {
    x: f64,
    y: f64,
}

// File: types_b.cursive
public record Point {  // Same name
    x: f32,            // Different structure
    y: f32,
}

// ERROR E15-060: Multiple definitions of Point with different structure
```

##### §14.5.5.2 ODR for Procedures

[13] Procedures with external linkage shall have exactly one definition. Multiple declarations (signatures without bodies) are permitted; multiple definitions (with bodies) violate ODR:

[ Given: Procedure definitions $D_1$, $D_2$ both defining procedure $f$ with external linkage ]

$$
\frac{D_1 \text{ has body} \quad D_2 \text{ has body} \quad D_1 \neq D_2}
     {\text{ERROR E15-061: Multiple definitions of procedure } f}
\tag{WF-ODR-Procedure}
$$

[14] **Exception**: Inline procedures and generic procedure instantiations may have multiple definitions. The linker deduplicates identical definitions.

##### §14.5.5.3 ODR Violation Consequences

[15] ODR violations detected at link time produce errors. Undetected violations cause undefined behavior [UB-ID: B.2.54].

#### §14.5.6 Name Mangling

##### §14.5.6.1 Purpose and Mechanism

[16] _Name mangling_ encodes additional information (module path, parameter types, generic arguments) into symbol names to support overloading and avoid naming conflicts.

[17] **Mangling is implementation-defined** (Decision D10): Implementations may apply name mangling to symbols with external linkage. The mangling scheme is implementation-defined and must be documented.

[18] **Optional mangling**: Implementations may choose not to mangle symbols or to provide multiple mangling schemes (e.g., "stable" vs "optimized"). The choice shall be documented and selectable via compiler flags.

[19] **Demangling**: Implementations should provide tooling to demangle symbols for debugging and diagnostics.

##### §14.5.6.2 Mangling Bypass Attributes

[20] The `[[no_mangle]]` attribute disables name mangling:

```cursive
[[no_mangle]]
public procedure exact_symbol_name(): i32
    [[ |- true => true ]]
{
    result 42
}
// Binary symbol: exactly "exact_symbol_name" (no mangling)
```

[21] **Use cases**:

- FFI exports with specific symbol names
- Debugging (predictable symbols)
- Dynamic loading with dlsym/GetProcAddress

##### §14.5.6.3 Extern Implies No Mangle

[22] Procedures marked `[[extern(C)]]` have mangling automatically disabled. The C calling convention expects unmangled symbols:

```cursive
[[extern(C)]]
public procedure c_compatible(): i32
    [[ |- true => true ]]
{
    result 100
}
// Binary symbol: "c_compatible" (extern(C) implies no_mangle)
```

[23] Combining `[[extern(C)]]` with `[[no_mangle]]` is redundant but permitted.

#### §14.5.7 Weak Symbols

##### §14.5.7.1 Definition

[24] The `[[weak]]` attribute declares a weak symbol that may be overridden by a strong symbol with the same name at link time.

```cursive
[[weak]]
public procedure default_handler(msg: string@View)
    [[ io::write |- true => true ]]
{
    println("Default: {}", msg)
}
```

[25] If another compilation unit provides a strong (non-weak) definition of `default_handler`, the strong definition takes precedence. If only weak definitions exist, one is chosen arbitrarily.

##### §14.5.7.2 Platform Dependency

[26] Weak symbol support is platform-dependent:

- **Unix/Linux (ELF)**: Widely supported
- **macOS (Mach-O)**: Supported
- **Windows (PE)**: Limited support

[27] Implementations shall document weak symbol support for target platforms. Using `[[weak]]` on unsupported platforms produces diagnostic E15-070 (warning severity, not error).

##### §14.5.7.3 Use Cases

[28] Weak symbols enable:

- Default implementations overridable by users
- Plugin architectures (plugins provide strong symbols overriding weak defaults)
- Optional dependencies (weak symbols for missing libs fail gracefully)

**Example 15.5.7.1 (Weak symbol for default handler):**

```cursive
// Library provides weak default:
[[weak, no_mangle]]
public procedure custom_allocator(size: usize): *mut u8
    [[ unsafe::ptr, alloc::heap |- size > 0 => true ]]
{
    // Default allocator
    unsafe { system_malloc(size) }
}

// User can override with strong definition:
// (In user code)
[[no_mangle]]
public procedure custom_allocator(size: usize): *mut u8
    [[ unsafe::ptr, alloc::heap |- size > 0 => true ]]
{
    // Custom allocator replaces default
    unsafe { my_allocator(size) }
}
```

#### §14.5.8 Attribute Combinations

[29] Linkage control attributes may be combined subject to constraints:

| Combination             | Valid | Notes                                |
| ----------------------- | ----- | ------------------------------------ |
| `extern(C)`             | ✅    | C calling convention, no mangle      |
| `no_mangle`             | ✅    | Preserve symbol name                 |
| `weak`                  | ✅    | Weak symbol                          |
| `extern(C) + weak`      | ✅    | Weak C-compatible symbol             |
| `no_mangle + weak`      | ✅    | Weak symbol with exact name          |
| `extern(C) + no_mangle` | ⚠️    | Redundant (extern implies no_mangle) |

[30] All linkage attributes require `public` visibility. Using them on non-public declarations produces diagnostic E15-004.

#### §14.5.9 Diagnostics

[31] Linkage diagnostics:

| Code    | Condition                                     | Constraint |
| ------- | --------------------------------------------- | ---------- |
| E15-060 | Multiple definitions of type (ODR violation)  | [11]       |
| E15-061 | Multiple definitions of procedure (ODR)       | [13]       |
| E15-062 | Linkage attribute on non-public entity        | [30]       |
| E15-070 | Weak symbols not supported on target platform | [27]       |

#### §14.5.10 Conformance Requirements

[32] Implementations shall:

1. Determine linkage from visibility and entity kind per Table in §15.5.2.2
2. Export symbols with external linkage in binary symbol table
3. Keep symbols with internal linkage local to compilation unit
4. Omit symbols for entities with no linkage
5. Enforce One Definition Rule for external linkage entities (E15-060, E15-061)
6. Support `[[no_mangle]]` attribute disabling name mangling
7. Support `[[weak]]` attribute on platforms where available
8. Document name mangling scheme (if any) used by implementation
9. Provide demangling tools for debugging
10. Respect `[[extern(C)]]` implying no mangling

---

**Previous**: §15.4 Platform-Specific Features (§15.4 [interop.platform]) | **Next**: §15.6 ABI Specification (§15.6 [interop.abi])
