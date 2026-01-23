# The Cursive Language Specification (Draft 3)

## Clause 1: General Principles and Conformance
### 1.1 Conformance Obligations {Source: Draft 2 §6.2}

  <u>Definition</u>
    Defines the requirements for implementations (must accept well-formed programs, reject ill-formed ones) and programs (must be well-formed).
  
  <u>Constraints & Legality</u>
    *   **Implementations**: MUST support `permissive` and `strict` modes. MUST generate a Conformance Dossier (JSON) matching the schema in Appendix C in both modes.
    *   **Programs**: A program is conforming if it meets safety requirements in `strict` mode.
  

### 1.2 Conformance Modes {Source: Draft 2 §6.3}

  <u>Definition</u>
    Two distinct modes of compilation governing the strictness of safety checks.
  
  <u>Static Semantics</u>
    *   **Permissive Mode**: Missing attestations on `unsafe` blocks generate `warnings`.
    *   **Strict Mode**: Missing attestations on `unsafe` blocks generate `errors`.
  

### 1.3 Behavior Classifications {Source: Draft 2 §6.1, Appendix H}

  <u>Definition</u>
    Categorizes outcomes into Defined, Implementation-Defined (IDB), Unspecified (USB), and Unverifiable (UVB) behaviors.
  
  <u>Concurrency & Safety</u>
    *   **UVB**: Occurs only in `unsafe` or FFI. Outside language guarantees.
    *   **USB**: Unspecified Behavior. Implementation chooses an outcome from a permitted set; choice need not be documented or consistent.
    *   **IDB**: Implementation MUST document choices in the Dossier.
    *   **IFNDR**: Ill-Formed, No Diagnostic Required (computationally infeasible checks).
  

### 1.3.1 Ill-Formed, No Diagnostic Required (IFNDR)

  <u>Definition</u>
    A category of program errors where the compiler is NOT required to emit a diagnostic because detecting the error is undecidable or computationally infeasible.
  
  <u>Examples</u>
    *   **OOB in Const Eval**: Index out of bounds during constant evaluation where the index is not a static constant.
    *   **Recursion Limits**: Exceeding implementation limits in complex template/macro expansions that cannot be bounded statically.
  
  <u>Constraints & Legality</u>
    *   **Safety Boundary**: The behavior of an IFNDR program is unspecified (USB), but when occurring in `safe` code, it **MUST NOT** introduce Unverifiable Behavior (UVB). Its effects MUST be bounded by the language's safety guarantees (e.g., predictable termination/panic rather than memory corruption).
    *   **Tracking Requirement**: Implementations MUST track all instances where IFNDR rules apply during compilation, even when no diagnostic is issued.
    *   **Reporting Requirement**: All IFNDR instances MUST be reported in the Conformance Dossier (see Appendix C).
    *   **Mode Applicability**: IFNDR reporting is required in both `strict` and `permissive` modes.
    *   **Instance Data**: Each IFNDR instance MUST include:
        - Location information (file, line, module)
        - Category/type identifier (e.g., "OOB in Const Eval", "Recursion Limits")
  

### 1.4 The Attestation System {Source: Draft 2 §6.7}

  <u>Definition</u>
    Attribute-based mechanism to justify `unsafe` code usage.
  
  <u>Syntax & Declaration</u>
    `[[attestation(method: "...", auditor: "...", ...)]]`
  
  <u>Constraints & Legality</u>
    *   **Mandatory Fields**: The following fields are REQUIRED: `method`, `auditor`, `date`, `proof`, `comment`.
    *   **Strict Mode**: 
        *   Missing mandatory fields trigger `E-DEC-2450`.
        *   `unsafe` blocks MUST be immediately preceded by an `[[attestation]]`. Missing attestation triggers `E-MEM-3030`.
    *   **Permissive Mode**: Missing attestations generate `warnings` (W-MEM-3030).
  

### 1.5 Implementation Limits {Source: Draft 2 §6.5, §8.2.2, §10.2}

  <u>Complexity & Limits</u>
    Minimum guaranteed limits that all implementations MUST support:
    *   **Source**: File size (1MiB), line length (16k chars), logical lines (65k).
    *   **Syntax**: Nesting depth (256 blocks/exprs).
    *   **Identifiers**: Length (1023 chars).
    *   **Declarations**: Parameters (255), Fields (1024).
  

### 1.6 Language Evolution {Source: Draft 2 §7}

  <u>Definition</u>
    Versioning (SemVer), feature stability (Stable/Preview/Experimental), editions, and extension namespaces.
  
  <u>Constraints & Legality</u>
    *   **Versioning**: Format MUST be `MAJOR.MINOR.PATCH`. Implementations MUST reject incompatible MAJOR versions.
    *   **Editions**: Optional grouping mechanism for incompatible changes. When supported, semantics from different editions MUST NOT mix within single compilation unit. Editions MAY introduce new keywords or alter semantic rules.
    *   **Stability Classes**:
        - **Stable**: Available by default. No breaking changes except in MAJOR increments or editions.
        - **Preview**: Requires opt-in via feature flag. MAY change between MINOR versions.
        - **Experimental**: Highly unstable. MAY change or be removed in any version.
    *   **Deprecation**: Deprecated features MUST remain functional for at least one full MINOR version. Usage MUST trigger a warning.
    *   **Removal**: Features (except Experimental) MAY only be removed in MAJOR versions.
    *   **Feature Flags**: Extensions requiring opt-in MUST be controlled via feature flags. Unknown flags MUST trigger error. Spec-defined preview features use identifiers without vendor prefixes.
    *   **Vendor Extensions**: Vendor-specific extensions MUST use reverse-domain namespace (e.g., `com.vendor.feature`). The `cursive.*` namespace is reserved for spec-defined features.
  

---

## Clause 2: Lexical Structure and Source
### 2.1 Source Text & Encoding {Source: Draft 2 §8.1, §8.2, §8.1.5}

  <u>Definition</u>
    Physical source file format.
  
  <u>Preprocessing Pipeline</u>
    **Mandatory 6-step Pipeline** (Strict Order):
    1.  **Size Check**: Verify file size limits.
    2.  **Encoding**: Verify UTF-8 validity (`E-SRC-0101`).
    3.  **BOM**: Strip Byte Order Mark.
    4.  **Line Normalization**: Normalize to `\n`.
    5.  **Control Characters**: Reject forbidden controls (`E-SRC-0104`).
    6.  **Grammar**: Tokenization.
  
  <u>Constraints & Legality</u>
    *   **Encoding**: Valid UTF-8 required (`E-SRC-0101`).
    *   **BOM**: BOM is permitted but stripped. Presence triggers warning `W-SRC-0101`. Embedded BOMs triggers `E-SRC-0103`.
    *   **Normalization**: Identifier normalization (NFC) is mandatory. General source normalization is IDB.
    *   **Locations**: Diagnostics MUST report byte offsets for encoding errors.
  

### 2.2 Translation Phases {Source: Draft 2 §8.4, §8.3.2}

  <u>Definition</u>
    The compilation process MUST follow four strict phases.
  
  <u>Static Semantics</u>
    **Mandatory 4-Phase Pipeline**:
    1.  **Parsing**: Source -> AST. (Includes macro expansion).
    2.  **Comptime Execution**: Evaluation of `comptime` blocks and static constants.
    3.  **Type Checking**: Semantic analysis, permission checking, and verification.
    4.  **Codegen**: IR generation and optimization.
    *   **Ordering**: Phases MUST run in this exact order.
    *   **Two-Phase Invariant**: Phase 1 (Parsing) MUST complete for every compilation unit before Phase 2 (Comptime + Type Checking) begins, ensuring forward references resolve against the complete symbol table.
    *   **No Preprocessing**: C-style textual inclusion (`#include`) is prohibited.
  
  <u>Constraints & Legality</u>
    *   **Encoding**: Valid UTF-8 required (`E-SRC-0101`).
    *   **Phasing**: Macro expansion (Phase 1) MUST complete before Comptime Execution (Phase 2) begins.
    *   **Forward References**: Symbols referenced before their declaration within the same compilation unit MUST resolve via the Phase 1 table; rejecting such references violates conformance (only missing symbols emit `E-NAM-1301`).
  
  <u>Examples</u>
  ```cursive
  public procedure main(ctx: Context): i32 {
      helper(ctx) // allowed: helper recorded during Phase 1
      result 0
  }

  procedure helper(ctx: Context) {
      // ...
  }
  ```
  

### 2.3 Lexical Elements {Source: Draft 2 §9.1, §9.2}

  <u>Definition</u>
    Decomposition into tokens.
    **Whitespace**: The only allowed whitespace characters are Space (`U+0020`), Horizontal Tab (`U+0009`), Form Feed (`U+000C`), and Line Feed (`U+000A`). All other whitespace characters are treated as invalid tokens (`E-SRC-0104`) unless inside string literals.
    **Comments**: 
    *   Line: `//` (terminates at newline).
    *   Block: `/* ... */` (MUST nest properly; recursive counter required). `E-SRC-0306` for unterminated.
    *   Doc: `///`, `//!`.
  
  <u>Syntax & Declaration</u>
    **Keywords**: Reserved words (`E-SRC-0305`).
    *   **Operators**: Maximal munch rule.
    *   **Generics Exception**: The sequence `>>` inside a generic argument list MUST be split into two `>` tokens (context-sensitive lexing rule).
  
  <u>Constraints & Legality</u>
    **Lexical Security** (§9.1.4): Lexically sensitive characters are restricted in identifiers, operators, punctuators, and token boundaries.
    *   **Bidirectional Formatting**: Characters U+202A through U+202E, U+2066 through U+2069
    *   **Zero-Width Characters**: Zero-width joiner (U+200D), zero-width non-joiner (U+200C)
    *   **Diagnostics**: 
        - Permissive Mode: `W-SRC-0308` (warning)
        - Strict Mode: `E-SRC-0308` (error, compilation fails)
    *   **Exemption**: Characters within string/character literals are NOT subject to this check (escape sequences validated separately)
  

### 2.4 Identifiers {Source: Draft 2 §9.3}

  <u>Definition</u>
    Unicode UAX31 identifiers.
  
  <u>Constraints & Legality</u>
    *   **Normalization**: NFC required.
    *   **Conformance**: Must adhere to **Unicode Standard Annex #31** with the recommended profile (XID_Start + XID_Continue).
    *   **Diagnostics**: `E-SRC-0307` for invalid Unicode in identifier.
  

### 2.5 Reserved Namespaces & Patterns {Source: Draft 2 §6.6}

  <u>Constraints & Legality</u>
    *   **Reserved Namespaces**: `cursive.*` is reserved for specification features.
    *   **Implementation Patterns**: Identifiers starting with `__` (double underscore) are reserved for implementation use.
    *   **Shadowing**: Core type identifiers (e.g., `i32`, `bool`) MUST NOT be shadowed (`E-NAM-1302` implied).
  

### 2.6 Literals {Source: Draft 2 §9.4}

  <u>Syntax & Declaration</u>
    *   **Integer**: 
        - Decimal: `123`, `1_000`.
        - Hex: `0xFE`, `0x_FE`.
        - Octal: `0o77`.
        - Binary: `0b1010`.
    *   **Float**: `1.0`, `1e10`, `1.5e-10`.
    *   **String**: `"..."` with standard escapes.
    *   **Char**: `'c'`, `'\n'`, `'\u{1F4A9}'`.
    *   **Bool**: `true`, `false`.
  
  <u>Constraints & Legality</u>
    *   **Malformed**: `E-SRC-0304`.
    *   **Leading Zeros**: `0`-prefixed integer literals MUST be parsed as decimal (`W-SRC-0301`).
    *   **Underscores**: Forbidden at start/end, after prefix (`0x_`), or adjacent to exponents/suffixes.
    *   **Allowed Escapes**: Implementations MUST support ONLY: `\\n`, `\\r`, `\\t`, `\\\\`, `\\\"`, `\\'`, `\\0`, `\\xHH`, `\\u{H+}`. All others trigger `E-SRC-0302`.
    *   **Suffixes**: Numeric suffixes (e.g., `u8`) MUST be valid for the literal value.
    *   **Strings**: Unterminated `E-SRC-0301`.
  
  <u>Static Semantics</u>
    *   **String Literals**: Have type `string@View` (static lifetime).
    *   **Array Literals**: `[a, b]` infers type `[T; N]` (Fixed-size array).
    *   **Type Inference Defaults** (Bidirectional Synthesis):
        -   **Integers**: Unsuffixed decimal literals default to `i32`.
        -   **Floats**: Unsuffixed floating-point literals default to `f64`.
    *   **Contextual Coercion**: In checking mode, literals adapt to the expected type if representable (e.g., `let x: u8 = 100` is valid).
    *   **Suffix Priority**: Suffixed literals (e.g., `100u8`) have a fixed type that overrides inference defaults.
  

### 2.7 Syntactic Stability {Source: Draft 2 §4.8}

  <u>Definition</u>
    Rules to ensure consistent parsing and tooling support.
  
  <u>Constraints & Legality</u>
    *   **Context-Insensitive**: Keywords are reserved context-insensitively.
    *   **Stable Ordering**: Declarations should maintain stable ordering rules for determinism.


### 2.8 Statement Termination {Source: Draft 2 §10.3}

  <u>Definition</u>
    Rules for delimiting statements via semicolons or newlines.
  
  <u>Static Semantics</u>
    *   **Terminators**: `;` (explicit) or `<newline>` token (implicit).
    *   **Line Continuation**: Newline does NOT terminate statement if:
        1. Inside unclosed delimiters `()`, `[]`, `{}`
        2. Last token on line is binary operator (e.g., `+`, `-`, `*`, `&&`, `|`) or comma `,`
        3. First token on following line is `.` (field access) or `::` (scope resolution)
    *   These rules enable multi-line expressions and method chains without backslash escapes

---

## Clause 3: The Object & Memory Model
### 3.1 The Object Model {Source: Draft 2 §29.1}

  <u>Definition</u>
    Objects defined by (Storage, Type, Lifetime, Value).
    
    **Memory Safety Principles** (foundational architectural requirements):
    *   **Liveness vs. Aliasing**: Two independent safety axes:
        - **Liveness**: Ensures pointers/bindings refer to allocated, initialized memory. Enforced via RAII (§3.2), Modal Pointers (§4.10), Region Escape Analysis (§3.3).
        - **Aliasing**: Ensures concurrent/reentrant access doesn't violate data integrity. Enforced via Permissions (§4.3) and Partitioning (§3.4).
    *   **Explicit-Over-Implicit**: Memory operations affecting ownership, allocation, or synchronization MUST be syntactically explicit:
        - Allocation: `^` for region, explicit allocator methods for heap
        - Ownership transfer: `move` keyword at call site
        - Synchronization: Explicit primitives (Mutex, Atomic), no hidden locks
    *   **Zero Runtime Overhead**: All memory safety checks (except dynamic bounds checks and explicit dynamic verification) MUST be resolved at compile time. Conforming implementations MUST NOT insert reference counting, GC barriers, or dynamic lifetime tracking.
  
  <u>Memory & Layout</u>
    Storage durations: Static, Automatic (Stack), Region, Dynamic (Heap). Layout rules for primitives and composites.
    
    **Object Lifecycle** (three phases):
    1. **Allocation & Initialization**: Storage reserved, valid initial value written (lifetime start)
    2. **Usage**: Value read/modified via valid bindings or pointers
    3. **Destruction & Deallocation**: Drop logic executed, storage released (lifetime end)
    
    **Storage Duration Categories**:
    *   **Static**: Module-level, lifetime = entire program execution, provenance = Global
    *   **Automatic (Stack)**: Local bindings, LIFO deallocation, provenance = Stack
    *   **Region (Arena)**: `^` operator, bulk deallocation at region exit, provenance = Region(ID)
    *   **Dynamic (Heap)**: Explicit allocator, manual or RAII management, provenance = Heap
  

### 3.2 Responsibility & Move Semantics {Source: Draft 2 §29.2}

  <u>Definition</u>
    RAII ownership model. "Responsible bindings" manage lifetime.
    *   **Panic**: Unrecoverable error resulting in stack unwinding.
    *   **Abort**: Immediate process termination (`noreturn`).
  
  <u>Static Semantics</u>
    *   **Move**: `move x` transfers responsibility.
    *   **Invalidation**: Source binding transitions to **Moved** state. Access is forbidden (`E-MEM-3001`).
    *   **Partial Moves**: Moving a field (`move x.f`) invalidates the parent record `x`.
        - **Constraint**: Partial moves are permitted **ONLY** from bindings with `unique` permission or mutable `var` bindings. Partial moves from `partitioned` or `const` bindings are forbidden to preserve alias integrity.
        - **Partial Access**: While `x` is Partially Moved:
          * Accessing moved field `x.f` is error
          * Accessing other valid fields permitted
          * Using `x` as whole (e.g., `move x`, `f(x)`) is error
        - **Partial Drop**: At scope exit, compiler MUST generate cleanup only for valid fields. Parent's top-level `Drop::drop` MUST NOT be called (object no longer coherent).
    *   **Reinitialization**: `var` in Moved state MAY be reassigned; `let` MUST NOT.

  <u>Dynamic Semantics</u>
    *   **Deterministic Destruction**: Bindings are destroyed in strict **LIFO (Last-In, First-Out)** order relative to their declaration at scope exit.
    *   **Unwinding**: Destructors run during unwinding.
    *   **Double Panic**: If a destructor panics while the thread is already unwinding from a panic, the runtime MUST abort the process.
  
  <u>Constraints & Legality</u>
    **Manual Drop**: Explicit calls to `Drop::drop` are forbidden (`E-MEM-3005` / `E-TRS-2920`). Use `mem::drop`.
  

### 3.2.1 Temporary Values {Source: Draft 2 §29.2.4}

  <u>Definition</u>
    Objects resulting from expression evaluation that are not immediately bound to named identifiers.
  
  <u>Dynamic Semantics</u>
    *   **Scope**: Extends from creation until end of innermost enclosing statement.
    *   **Promotion**: If temporary used to initialize `let`/`var` binding, lifetime extends to binding's scope.
    *   **Cleanup**: Temporaries not moved or bound MUST be destroyed at statement end.
  

### 3.3 Memory Management (Regions) {Source: Draft 2 §29.5}

  <u>Definition</u>
    Lexical arenas providing deterministic, stack-like allocation without explicit deallocation.
    **Arena Modal Type**:
    *   **States**: `@Active` (Allocatable), `@Frozen` (Read-only), `@Freed` (Invalid).
    *   **Transitions**: `new() -> @Active`, `freeze() -> @Frozen`, `free() -> @Freed`.
  
  <u>Syntax & Declaration</u>
    *   `region r { ... }` block syntax
    *   Allocation operator `^T { ... }` allocates within current region
    *   **Caret stacking**: `^^T { ... }` allocates in parent region (lexically enclosing region block)
  
  <u>Static Semantics</u>
    *   **Provenance Tracking Algorithm**: All pointers carry compile-time provenance tag $\pi$:
        - Stack provenance: $\pi_{Stack}$
        - Heap provenance: $\pi_{Heap}$ 
        - Region provenance: $\pi_{Region(ID)}$ (unique ID per region block)
    *   **Escape Analysis**: Compiler MUST track all region pointer assignments and enforce:
        - For pointer with $\pi_{Region(ID)}$, assignment target MUST have lifetime $L \leq \pi_{Region(ID)}$
        - Upon region exit, all pointers with that region's provenance transition to `Ptr<T>@Expired` state
    *   **Escape Rule**: Value with $\pi_{Region}$ MUST NOT escape to storage with longer lifetime
    *   **Lifetime Partial Order ($\le$)**:
        -   **Definition**: $\pi_A \le \pi_B$ if and only if region $B$ lexically encloses or is identical to region $A$.
        -   **Hierarchy**: $\pi_{Region(Inner)} < \pi_{Region(Outer)} < \pi_{Stack} < \pi_{Heap}$.
        -   **Transitivity**: The relation is transitive and reflexive.
  
  <u>Constraints & Legality</u>
    *   **Scope**: Usage of `^` outside lexically enclosing `region` block is prohibited (`E-MEM-3021`)
    *   **Escape**: Escape analysis prevents region pointers from outliving block (`E-MEM-3020`)
    *   **State Transition**: On region exit, compiler automatically transitions region pointers to `@Expired`
  

### 3.4 The Partitioning System {Source: Draft 2 §29.3, §29.4}

  <u>Definition</u>
    System enabling safe aliased mutability through disjointness verification.
    **Verification Modes**:
    *   **Static (Default)**: Compile-time proof via Canonical Linear Form. Zero runtime overhead.
    *   **Dynamic**: Runtime intersection checks injected at the partition site. Panics on overlap.
  
  <u>Syntax & Declaration</u>
    *   **Static Partitions**: `partition PartitionName { field1, field2 }` in record body
    *   **Contract Partitions**: `[[verify(mode)]] partition expr by (...)`
  
  <u>Static Semantics</u>
    *   **Implicit Partitions**: Any field NOT declared within a named `partition` block belongs to a unique, anonymous partition containing only that field. This ensures maximal concurrency by default.
    *   **Static Mode**: 
        - Uses **Canonical Linear Form** analysis ($C_{base} + \sum C_i V_i$).
        - Failure to prove disjointness triggers `E-MEM-3012`.
    *   **Invariant Consumption**: The `GatherFacts` algorithm **MUST** include:
        1.  **Explicit Invariants**: Predicates attached to function parameters (e.g., `i: usize where { i < N }`).
        2.  **Implicit Invariants**: Facts derived from flow-sensitive narrowing.
    *   **Proof Goal**: The verifier uses these invariants to prove the disjointness required by the `partition` statement.
    *   **Dynamic Mode**:
        - Suspends static linear form analysis.
        - Requires `[[verify(dynamic)]]` attribute.
        - Synthesizes runtime intersection logic.

  <u>Dynamic Semantics</u>
    *   **Hybrid Enforcement**: Bounds and overflow checks are always dynamic (unless statically proven).
    *   **Dynamic Disjointness Check**:
        - For `[[verify(dynamic)]]`, compiler injects code to verify `Intersection(RangeA, RangeB) is Empty`.
        - Failure triggers **Panic** (`P-MEM-3013`).
  

### 3.5 Unsafe Semantics {Source: Draft 2 §29.6}

  <u>Definition</u>
    Operations suspended in `unsafe` blocks.

  <u>Static Semantics</u>
    *   **Suspension of Constraints**: The following static safety checks are **DISABLED** within `unsafe` blocks:
        1.  **Liveness**: Validity of memory addresses (allows dereferencing potentially invalid pointers).
        2.  **Aliasing**: Exclusivity of mutable access paths (allows multiple `*mut T` to same address).
        3.  **Partitioning**: Disjointness of indices or fields.
    *   **Enabled Operations**: The following operations are permitted **ONLY** within `unsafe`:
        1.  **Dereference**: Accessing `*imm T` and `*mut T`.
        2.  **Unsafe Calls**: Invoking procedures marked `unsafe` or `extern`.
        3.  **Transmute**: Bitwise reinterpretation via `transmute`.
        4.  **Pointer Arithmetic**: Usage of `+` and `-` on raw pointers.

  <u>Constraints & Legality</u>
    *   **Transmute Size Check**: The `transmute<S, T>` intrinsic MUST verify at compile time that `sizeof(S) == sizeof(T)`. Size mismatches MUST trigger `E-MEM-3031`.

  <u>Concurrency & Safety</u>
    *   **Transmute validity**: Programmer MUST guarantee the source bit pattern is a valid representation of the target type. Violations are **UVB**.
  

---

## Clause 4: Types, Declarations, and Generics
### 4.1 Foundations {Source: Draft 2 §15.1-15.5}

  <u>Definition</u>
    Static, Nominal, and Structural typing.
    *   **Static**: All type checking at compile time
    *   **Nominal**: Records, enums, modals distinguished by name/declaration identity
    *   **Structural**: Tuples, unions, function types distinguished by shape
  
  <u>Static Semantics</u>
    *   **Equivalence**: 
        - Nominal identity for records/enums.
        - **Structural Identity**:
            - **Tuples**: `(T1..Tn) ≡ (U1..Un)` iff `n=m` AND `∀i. Ti ≡ Ui`.
            - **Functions**: `(P1..Pn) -> R ≡ (Q1..Qn) -> S` iff `R ≡ S` AND `∀i. (Pi ≡ Qi AND move(Pi) == move(Qi))`.
        - **Permission Identity**: `P1 T1 == P2 T2` iff `P1 == P2` AND `T1 == T2`. (`unique T` != `const T`).
    *   **Subtyping**: Permission subtyping (`unique <: partitioned <: const`), trait implementation, `! <: T`.
    *   **Bottom Type**: The never type `!` is a subtype of all types (`! <: T`).
    *   **Bidirectional Type Inference** (§15.5):
        - **Synthesis Mode**: Compiler infers type "upward" from expression (e.g., literals, operations)
        - **Checking Mode**: Compiler checks expression against expected type "downward" (e.g., in assignments, returns)
        - Enables local type omission while maintaining explicit API boundaries
  
  <u>Constraints & Legality</u>
    *   **Inference Boundary**: Type inference is intra-procedural only. 
    *   **Explicit Signatures**: Top-level declarations (procedures, globals) MUST have fully explicit type annotations.
    *   **Missing Annotation**: Omission of required top-level type annotation is `E-TYP-1505`.
    *   **Type Layout** (§15.4): 
        - Specific layouts generally IDB (implementation-defined)
        - Implementations MUST document layout strategy in conformance dossier
        - `[[repr(C)]]` mandates C-compatible layout for FFI
  

### 4.2 Variance & Polarity {Source: Draft 2 §15.6}

  <u>Definition</u>
    Polarity inference for generic parameters: Covariant (+), Contravariant (-), Invariant (=), Bivariant (*).
  
  <u>Static Semantics</u>
    *   **Covariant (+)**: Output positions only (Return types, const fields).
    *   **Contravariant (-)**: Input positions only (Method parameters).
    *   **Invariant (=)**: Both input/output, or mutable `unique`/`var` fields.
    *   **Bivariant (*)**: Unused (Phantom data).
    *   **Generic Subtyping**: `Type<A> <: Type<B>` iff for every parameter `T_i`:
        - If `T_i` is Covariant: `A_i <: B_i`
        - If `T_i` is Contravariant: `B_i <: A_i`
        - If `T_i` is Invariant: `A_i ≡ B_i`
    *   **Interaction**: `const` permission allows treating Invariant fields as Covariant.
  

### 4.3 Permission Types {Source: Draft 2 §16; Amendment 1}

  <u>Definition</u>
    The permission system consists of four permissions forming a directed lattice:
    *   **`unique`**: Exclusive access. Mutable. Non-aliased.
    *   **`partitioned`**: Spatially disjoint access. Mutable. Aliased (but disjoint).
    *   **`concurrent`**: Temporally synchronized access. Mutable (via interior mutability). Aliased.
    *   **`const`**: Read-only access. Immutable. Aliased.

  <u>Static Semantics</u>
    *   **The Permission Lattice**:
        -   `unique` is a subtype of both `partitioned` and `concurrent`.
        -   `partitioned` is a subtype of `const`.
        -   `concurrent` is a subtype of `const`.
    *   **Sibling Incompatibility**: `partitioned` and `concurrent` are **incompatible siblings**.
        -   `partitioned` (spatial) cannot coerce to `concurrent` (temporal) because `partitioned` guarantees no overlap, while `concurrent` implies overlap managed by locks.
        -   `concurrent` cannot coerce to `partitioned` because `concurrent` implies shared state that cannot be statically split.
    *   **The `Sync` Requirement**: A type `T` may only be referenced via the `concurrent` permission if `T` implements the **`Sync`** marker trait (see Appx D). This indicates `T` possesses hardware or OS synchronization primitives (e.g., `Mutex`, `Atomic`, `FileHandle`).
    *   **Exclusion Principle**:
        -   **Unique**: Valid ONLY if no other live permissions (read or write) exist.
        -   **Partitioned**: Allows multiple mutable aliases (disjoint) but forbids `unique`.
        -   **Concurrent**: Allows multiple mutable aliases (synchronized) but forbids `unique`.
        -   **Const**: Allows multiple read-only aliases but forbids `unique`, `partitioned`, or `concurrent`.
    *   **Downgrading**: `unique` MAY be temporarily downgraded to subtypes for a bounded scope.
    *   **Coercion**: Implicit downgrading is permitted. Upgrading is forbidden.
    *   **Mutation**: Requires `unique` or `partitioned` (or `concurrent` via interior mutability).

  <u>Constraints & Legality</u>
    Mutation requires `unique` or `partitioned`. `const` prohibits mutation (`E-TYP-1601`). `concurrent` requires `Sync` (`E-TYP-1604`).
  

### 4.4 Primitive Types {Source: Draft 2 §17}

  <u>Definition</u>
    *   `iN`/`uN`: Fixed width integers. Signed integers MUST use two's complement representation.
    *   `fN`: IEEE 754 floats (binary16, binary32, binary64).
    *   `char`: Unicode Scalar Value (U+0000-D7FF, U+E000-10FFFF).
    *   `bool`, `()` (unit, one value, zero size), `!` (never, uninhabited).
  
  <u>Syntax & Declaration</u>
    *   **Integers**: `i8`..`i128`, `u8`..`u128`, `isize`/`usize` (pointer-sized).
    *   **Aliases**: `int` (`i32`), `uint` (`u32`) - fully equivalent in all contexts.
    *   **Floats**: `f16`, `f32`, `f64`.
    *   **Aliases**: `half` (`f16`), `float` (`f32`), `double` (`f64`) - fully equivalent in all contexts.
  
  <u>Memory & Layout</u>
    Implementations MUST match target platform pointer width for `isize`/`usize`/Ptr.
    **Mandatory Layouts** (size/alignment in bytes):
    *   1/1: `i8`, `u8`, `bool`.
    *   2/2: `i16`, `u16`, `f16`.
    *   4/4: `i32`, `u32`, `f32`, `char`.
    *   8/8: `i64`, `u64`, `f64`.
    *   16/(8 or 16): `i128`, `u128`.
    
    **Representation Requirements**:
    *   Signed integers: Two's complement representation (MUST)
    *   Floats: IEEE 754 conformance (MUST)
    *   `char`: 32-bit capable of holding any Unicode Scalar Value
  
  <u>Constraints & Legality</u>
    *   **f16 Emulation**: Implementations SHOULD issue warning `W-TYP-1701` for `f16` arithmetic if emulated.

  <u>Dynamic Semantics</u>
    **Integer Overflow**: Implementations MUST provide a checked mode in which signed and unsigned integer overflow panics. Behavior in release mode is IDB.
  

### 4.5 Composite Types & Declarations {Source: Draft 2 §18, §23.3}

  <u>Syntax & Declaration</u>
    *   **Type Declarations**: `record`, `enum`, `type` (alias).
    *   **Enum Syntax**:
        ```ebnf
        enum_decl ::= "enum" identifier "{" variant* "}"
        variant   ::= identifier [ payload ] [ "=" integer_constant ]
        payload   ::= "(" type_list ")" | "{" field_list "}"
        ```
    *   **Enum Discriminants**: Variants optionally carry explicit integer discriminants: `VariantName = N`
        - Discriminant values MUST be compile-time integer constants
        - Unspecified discriminants assigned sequentially (0, or previous + 1)
    *   **Literals/Constructors**: 
        - Tuples: `(A, B, ...)`
        - Arrays: `[e1, e2, ...]` (list form) or `[e; N]` (repeat form where `e` duplicated `N` times)
        - Record literals: `TypeName { field1: v1, field2: v2 }` (field shorthand `{ field }` ≡ `{ field: field }`)
        - Slices: `[T]` (type only, created via array coercion or slice operations)
  
  <u>Static Semantics</u>
    *   **Default Visibility**: Record fields default to `private` (module-local) unless explicitly marked `public` or `internal`.
    *   **Tuple Access**: Index-based only.
  
  <u>Memory & Layout</u>
    *   **Tuples**: Structural layout.
    *   **Arrays**: Contiguous, no padding between elements.
    *   **Slices**: Dense pointer layout (`struct { ptr: *imm T, len: usize }`).
  
  <u>Constraints & Legality</u>
    *   **Recursive**: Recursive aliases forbidden (`E-DEC-2420`).
    *   **Indexing**: 
        - Index expression MUST have type `usize`.
        - Requires runtime bounds checks (`E-TYP-1801` on failure).
  

### 4.6 Union Types {Source: Draft 2 §18.4}

  <u>Definition</u>
    Structural sum types (`A | B`). Safe, tagged union of types.

  <u>Static Semantics</u>
    *   **Equivalence**: Set-based (order independent). `A | B` ≡ `B | A`.
    *   **Subtyping**: `T <: U` if set of types in `T` is subset of types in `U`.
    *   **Access**: Requires `match` (exhaustive).

  <u>Memory & Layout</u>
    *   **Structure**: `(tag, payload)`.
    *   **Tag**: Integer discriminant (size implementation-defined, typically `u8` or `u16`).
    *   **Payload**: Union of fields (overlapping storage).
    *   **Size**: `sizeof(Tag) + sizeof(LargestField) + Padding`.
    *   **Alignment**: `max(alignof(Tag), alignof(LargestField))`.
  

### 4.7 Range Types {Source: Draft 2 §18.7}

  <u>Definition</u>
    Structural record types produced by range expressions (`..`).
  
  <u>Syntax & Declaration</u>
    Implementations MUST provide generic records with **public fields**:
    *   `start`, `end`: `Range`, `RangeInclusive`.
    *   `start`: `RangeFrom`.
    *   `end`: `RangeTo`, `RangeToInclusive`.
    *   (None): `RangeFull`.
    Desugaring of `..` expressions MUST map strictly to these types/fields.
  
  <u>Constraints & Legality</u>
    *   **Copy**: Range types MUST implement `Copy` if and only if `T` implements `Copy`.
  

### 4.8 Modal Types {Source: Draft 2 §19, §23.3}

  <u>Definition</u>
    Nominal types with compile-time states (`Type@State`) and state transitions. Two type forms:
    *   State-specific types (`M@S`): Zero-overhead, tag-free at runtime (state known statically)
    *   General modal type (`M`): Tagged union holding any state (state determined at runtime)

  <u>Syntax & Declaration</u>
    *   **Declaration**: `modal Name { @State { fields... } { methods... } ... }`.
    *   **Payload Visibility**: Fields defined within a `@State` block are implicitly `protected` (accessible only to the modal type's implementation).
    *   **Transition Signature**: `transition name(...) -> @TargetState` (declared in source state block).
    *   **Transition Impl**: `transition Type::name(self: SourceState, ...) -> TargetState { ... }`.
    *   At least one state required; duplicate state names forbidden (E-TYP-1911).

  <u>Memory & Layout</u>
    *   **State-Specific Type (`M@S`)**: Layout equivalent to record with state payload fields (zero size if empty payload).
    *   **General Modal Type (`M`)**: Tagged union (enum-like) containing:
        - Discriminant tag (identifies active state)
        - Union of all state payloads
    *   **Niche Optimization** (STRONGLY RECOMMENDED):
        - Implementations SHOULD elide separate discriminant if states distinguishable by payload bit-patterns
    *   **MANDATE** (Normative):
        - Implementations MUST apply niche optimization when any state payload contains invalid bit patterns
        - **Example**: `Ptr<T>` has `@Null` (0x0) and `@Valid` (non-zero). The general `Ptr<T>` type MUST have same size/alignment as raw pointer (one machine word), NOT size of pointer + discriminant.
        - This requirement ensures zero-cost abstraction for nullable pointers

  <u>Static Semantics</u>
    *   **Incomparability**: `T@A` is NOT compatible with `T@B` (different states are distinct types).
    *   **Widening/Coercion**: `T@State` implicitly coerces to `T` (general type).
    *   **Transition Desugaring**: 
        - Transition keyword MUST be used (not `procedure`)
        - Receiver type determined by enclosing state block
        - Return type MUST be target state type
        - Signature: `(self: SourceState, params...) -> TargetState`

  <u>Dynamic Semantics</u>
    *   **Pattern Matching**:
        - **General Type (`M`)**: Runtime dispatch on discriminant/niche; match MUST be exhaustive (E-TYP-1920)
        - **Specific Type (`M@S`)**: Irrefutable payload destructuring (state statically known)
  
  <u>Examples</u>
    Provide complete modal type example showing states, transitions, and pattern matching.
  

### 4.9 String Types {Source: Draft 2 §20}

  <u>Definition</u>
    Built-in modal type: `string@Managed` (owned) and `string@View` (slice).

  <u>Constraints & Legality</u>
    *   UTF-8 guaranteed.
    *   **Indexing**: Direct indexing (`s[i]`) on any string type is forbidden (`E-TYP-1902`).
    *   **Slicing**: 
        - Indices interpreted as **byte offsets**.
        - Boundaries MUST fall on valid UTF-8 character starts; failure triggers panic (`E-TYP-1901`).
    *   `string@Managed` MUST NOT implement `Copy` (ownership) or `Clone` (requires capability).

  <u>Memory & Layout</u>
    *   **Representation**: `string@View` is a fat pointer `(ptr: *u8, len: usize)`.
    *   **Storage**: Not null-terminated. content MUST be valid UTF-8.
  

### 4.10 Pointer Types {Source: Draft 2 §21}

  <u>Definition</u>
    *   **Safe**: `Ptr<T>@Valid`, `@Null`, `@Expired`.
    *   **Raw**: `*imm T`, `*mut T` (Unsafe).

  <u>Constraints & Legality</u>
    Dereferencing `@Null`/`@Expired` is compile-time error.

  <u>Memory & Layout</u>
    *   **Size**: Matches target platform word size (`usize`).
    *   **Alignment**: Matches target platform word alignment.
  

### 4.11 Function Types & Procedures {Source: Draft 2 §22, §23.2}

  <u>Syntax & Declaration</u>
    **Procedures**: `procedure name(params): ret [[contract]] <body>`
    
    **Body Forms** (three variants MUST be supported):
    *   **Block body**: `{ statements }` - Standard implementation
    *   **Expression body**: `= expression;` - Pure function shorthand
    *   **Extern declaration**: `;` - No body (FFI import from external library)
    *   **Function Types**:

```ebnf
function_type ::= "(" [ param_type_list ] ")" "->" type
param_type_list ::= param_type ("," param_type)*
param_type ::= [ "move" ] type
```
        A function type’s identity includes:
        - The full ordered list of parameter types (including any capability types and the presence or absence of the `move` modifier on each parameter), and
        - The return type.
        Parameter names and the `[[...]]` contract sequent are **not** part of the function type.
  
  <u>Definition</u>
    *   **Sparse**: `(T) -> U` (Function pointer).
    *   **Dense**: `witness (T) -> U` (Closure).
  
  <u>Memory & Layout</u>
    *   **Sparse function pointers (`(T) -> U`)**:
       - Represented as a single machine-word code pointer.
       - FFI-safe and ABI-compatible with C function pointers (see Clause 12).
    *   **Witness closures (`witness (T) -> U`)**:
       - Represented as a dense two-word pointer `(env_ptr, code_ptr)`, where `env_ptr` points to the captured environment and `code_ptr` points to the closure’s code.
       - **Not** FFI-safe and MUST NOT appear in `extern` signatures.
    *   **Subtyping relation**:
       - For the same parameter and return types, a sparse function pointer `(T) -> U` MAY be widened to a witness closure `witness (T) -> U` by pairing the code pointer with an empty or implementation-defined environment representation.
  
  <u>Static Semantics</u>
    *   **Variance**: Parameters are Contravariant. Return type is Covariant.
    *   **Move**: `move` modifier is part of the type identity.
    *   **Parameters**: `move` indicates responsibility transfer (Consuming).
    *   **Call Site**: Arguments for `move` parameters MUST be prefixed with `move` keyword (`E-DEC-2411`).
    *   **Receiver Shorthands**:
        | Shorthand | Desugared Form           | Permission              | Meaning                                              |
        | :-------- | :----------------------- | :---------------------- | :--------------------------------------------------- |
        | `~`       | `self: const Self`       | `const`                 | **Immutable**. Pure read access.                     |
        | `~        | `                        | `self: concurrent Self` | `concurrent`                                         | **Synchronized**. Shared mutation via locks/atomics. |
        | `~%`      | `self: partitioned Self` | `partitioned`           | **Disjoint**. Aliased mutation via static splitting. |
        | `~!`      | `self: unique Self`      | `unique`                | **Exclusive**. Full ownership/mutation.              |
    *   **Function type equivalence**:
        Two function types are equivalent if and only if:
        - They have the same number of parameters,
        - Each corresponding pair of parameters has equivalent types and matching presence/absence of `move`, and
        - Their return types are equivalent.
    *   **Function type subtyping**:
        Let `T = (T1, ..., Tn) -> Tret` and `U = (U1, ..., Un) -> Uret`. Then `T` is a subtype of `U` if and only if:
        - `Tret` is a subtype of `Uret` (covariant result), and
        - For every parameter index `i`, `Ui` is a subtype of `Ti` (contravariant parameters), including the treatment of `move` in the parameter position.
    *   **Pointer vs closure subtyping**:
        For the same parameter and return types, `(T) -> U` is a subtype of `witness (T) -> U`. Non-capturing procedures and closures may be used where a witness closure is expected, but not vice versa.
  

### 4.12 Static Polymorphism (Generics) {Source: Draft 2 §28.3}

  <u>Definition</u>
    Path 1. Monomorphization. Zero overhead. Polymorphism on *inputs* resolved at compile time.

  <u>Syntax & Declaration</u>
    *   **Declaration**: `<T <: Trait>` (Generic Constraints).
    *   **Instantiation**: Implicit at call site.

  <u>Static Semantics</u>
    *   **Monomorphization**: Implementations MUST generate specialized code for each concrete type combination.
    *   **Zero Overhead**: Generic calls MUST resolve to direct static calls; no vtable lookup permitted.
    *   **Resolution**: Generic parameters are invariant types within the procedure body unless bounded otherwise.

  <u>Constraints & Legality</u>
    *   **Extern**: Generic parameters are PROHIBITED in `extern` procedures.
    *   **Recursion**: Infinite monomorphization recursion (e.g., `f<T>() { f<Option<T>>() }`) MUST be detected and rejected (Implementation Limit).
  

### 4.13 Attributes {Source: Draft 2 §23.5}

  <u>Syntax & Declaration</u>
    `[[ attribute_spec ("," attribute_spec)* ]]`
    
    **Attribute Spec**: `name [ "(" argument_list ")" ]`
    **Argument List**: Comma-separated list of:
    *   `literal` (Value)
    *   `identifier` (Flag)
    *   `identifier ":" literal` (Key-Value Pair)

  <u>Definition</u>
    **Normative Attribute Registry**:
    *   `[[attestation(...)]]`: Unsafe justification (§1.4).
    *   `[[repr(C)]]`: Layout control (§12.3).
    *   `[[link_name("...")]]`: FFI symbol naming (§12.1).
    *   `[[unwind(...)]]`: FFI panic handling (§12.1).
    *   `[[deprecated("...")]]`: Warn on usage (§1.6).
    *   `[[verify(...)]]`: Contract mode selection (§7.3).

  <u>Constraints & Legality</u>
    *   **Validation**: Unknown attributes trigger `E-DEC-2451`.
    *   **Targeting**: Attributes must match the target declaration type (e.g., `[[repr(C)]]` on `record`/`enum` only).
  

### 4.14 Global Bindings {Source: Draft 2 §23.1, §14}

  <u>Definition</u>
    Module-level variable declarations with static storage duration.

  <u>Syntax & Declaration</u>
    *   `[visibility] static let name: type = expr`
    *   `[visibility] static var name: type = expr`

  <u>Constraints & Legality</u>
    *   **Initialization**: 
        - **Static Init**: Expressions evaluable at compile-time (constants) are baked into the data section.
        - **Dynamic Init**: Expressions requiring runtime execution are permitted but **MUST** adhere to the **Acyclic Eager Subgraph** rule defined in §5.5 (Initialization).
    *   **Safety**: Accessing `static var` (mutable global) is `unsafe` unless synchronized.



---

## Clause 5: Modules and Resolution
### 5.1 Compilation Units {Source: Draft 2 §8.3, §10.1}

  <u>Definition</u>
    A collection of source files constituting a single module ("Folder-as-Module").
    Each compilation unit defines exactly one module. Empty modules (only whitespace/comments) are valid.
  
  <u>Syntax & Declaration</u>
    **Top-Level Items** - Only these declaration kinds permitted at module scope:
    *   `import` declaration
    *   `use` declaration
    *   `variable` declaration (`static let`, `static var`)
    *   `procedure` declaration
    *   `type` declaration (`record`, `enum`, `modal`, `type`)
    *   `trait` declaration
    
    **Forbidden**: Control-flow constructs (`if`, `loop`) and expression statements MUST NOT appear at top level.
  
  <u>Constraints & Legality</u>
    *   **Visibility Default**: Top-level items without explicit visibility modifier default to `internal`.
    *   **Uniqueness**: Each identifier MUST be unique within module scope (unified namespace).
  

### 5.2 Project Manifest {Source: Draft 2 §11}

  <u>Definition</u>
    `Cursive.toml` structure.
  
  <u>Constraints & Legality</u>
    Required tables: `[project]`, `[language]`, `[paths]`, `[[assembly]]`. Missing manifest is `E-MOD-1101`.
  
  <u>Syntax & Declaration</u>
    *   `[project]`: MUST declare `name` (identifier) and `version` (SemVer). Missing keys trigger `E-MOD-1107`.
    *   `[language]`: MUST declare a `version` compatible with the compiler’s supported MAJOR; incompatible entries trigger `E-MOD-1109`.
    *   `[paths]`: MUST contain at least one key/value pair. Values are normalized, relative paths; absence or invalid entries trigger `E-MOD-1102`.
    *   `[[assembly]]`: Each entry MUST provide `name`, `root` (key from `[paths]`), and `path`. Duplicate `name` emits `E-MOD-1108`; unknown `root` emits `E-MOD-1103`.
  
  <u>Static Semantics</u>
    *   The manifest judgment `⊢ M : WF` holds only if `[project]`, `[language]`, `[paths]`, and every `[[assembly]]` satisfy their formation rules.
    *   Assemblies inherit visibility from the referenced `root`; unresolved tables render the entire manifest ill-formed.
  
  <u>Examples</u>
  ```toml
  [project]
  name = "acme.cli"
  version = "1.2.3"

  [language]
  version = "1.2.0"

  [paths]
  core = "src"
  vendor = "deps/vendor"

  [[assembly]]
  name = "core"
  root = "core"
  path = "."
  ```


### 5.3 Module Discovery & Paths {Source: Draft 2 §11.2, §11.4}

  <u>Definition</u>
    *   **Derivation**: Project root-relative directory path.
    *   **Separator Replacement**: OS separators MUST be replaced by `::`.
    *   **Folder-as-Module**: Directory contains the module; files contribute to the namespace.
  
  <u>Constraints & Legality</u>
    Case-sensitivity collisions (`E-MOD-1104`). Reserved keywords in paths (`E-MOD-1105`).
  

### 5.4 Names, Imports, and Resolution {Source: Draft 2 §12, §13}

  <u>Definition</u>
    Modifiers: `public`, `internal` (default), `private`, `protected`.
    Unified namespace per scope.
    
    **Scope Context Formal Structure**:
    *   Scope context $\Gamma$ is an ordered list of mappings: $\Gamma = [S_{local}, S_{proc}, S_{module}, S_{universe}]$
    *   Each scope $S$ maps identifiers to entities (terms, types, modules)
    *   Resolution proceeds from innermost (local) to outermost (universe)
  
  <u>Syntax & Declaration</u>
    *   `import_decl ::= "import" <module_path>`
    *   `use_decl ::= "use" <module_path> ["as" <identifier>]`
    *   `module_path ::= <identifier> ("::" <identifier>)*`
  
  <u>Static Semantics</u>
    *   **Import**: `import path` (Assembly dependency).
    *   **Use**: `use path::item` (Scope alias).
    *   **Re-export**: `public use`.
    *   **Protected**: Accessible only within the defining type and `trait` implementations for that type **within the same assembly**. MUST NOT be used on top-level items.
    *   **Resolution**: Lexical lookup (innermost to outermost).
    *   **Universe Scope**: The resolution chain MUST terminate in the implicit "Universe" scope, which contains all primitive types (`i32`, `bool`), core literals (`true`, `false`), and the `cursive.*` namespace.
    *   **Shadowing**: Explicit `shadow` keyword required (`E-NAM-1303`).
    *   **Unqualified Lookup**: Scope stack is searched from innermost to outermost (`Γ ⊢ x ⇒ entity`). Failure emits `E-NAM-1301`.
    *   **Qualified Lookup**: Prefix module resolves first, then the member; visibility enforced per Chapter 12. Missing prefix emits `E-NAM-1304`; inaccessible members emit `E-NAM-1305`.
  
  <u>Constraints & Legality</u>
    *   **Unified Namespace**: Terms (variables, procedures) and Types (records, enums) share a single namespace. Declaring a type and a term with the same name in the same scope is forbidden (`E-NAM-1302`).
    *   **Shadowing**: 
        - **Strict Mode**: Explicit `shadow` keyword required to shadow (`E-NAM-1303`). Using `shadow` when NO binding is shadowed is an error (`E-NAM-1306`).
        - **Permissive Mode**: Implicit shadowing triggers warning (`W-NAM-1303`).
  
  <u>Examples</u>
  ```cursive
  import core::io
  use io::File as shadow File

  shadow File = io::File // explicit keyword required to redeclare
  ```
  

### 5.5 Initialization {Source: Draft 2 §14}

  <u>Definition</u>
    *   **Two-Stage Process**: (1) Static (compile-time constants), (2) Dynamic (runtime execution).
    *   **Edge Classification**:
        *   **Value-Level Edge**: Dependency on a value (e.g., function call, global read).
            *   **Eager**: A Value-Level edge originating from a **module-level initializer**.
            *   **Lazy**: A Value-Level edge occurring **only** within procedure bodies (and not reachable from initializers), OR a Type-Level edge.
        *   **Type-Level Edge**: Dependency on a type signature or constant definition only (Always Lazy).
  
  <u>Dynamic Semantics</u>
    *   **Graph**: Dependency graph of modules.
    *   **Cycles**: The subgraph of **Eager (Value-Level) edges** MUST be Acyclic (`E-MOD-1401`). Lazy (Type-Level) cycles are permitted.
    *   **Execution**: 
        1. **Inter-Module**: Topological sort of Eager graph.
        2. **Intra-Module**: Initializers executed in strictly sequential lexical order within the source file.
    *   **Failure**: Panic poisons module.
  

### 5.6 Program Entry Point {Source: Draft 2 §30.1, §30.2}

  <u>Definition</u>
    No ambient authority.
  
  <u>Syntax & Declaration</u>
    `public procedure main(ctx: Context): i32`
    **Context Record**: The `Context` record MUST define the following fields:
    *   `fs`: `witness FileSystem`
    *   `net`: `witness Network`
    *   `sys`: `System` (implements `Time`)
    *   `heap`: `witness HeapAllocator`
  
  <u>Static Semantics</u>
    *   `Context` contains `fs`, `net`, `sys`, `heap`.
    *   **Return Semantics**: `main` returns `0` for success, non-zero for failure.
  
  <u>Constraints & Legality</u>
    Global mutable state is prohibited.
    *   **Signature**: Must be `main(ctx: Context): i32` (`E-DEC-2431`).
    *   **Uniqueness**: Only one `main` permitted (`E-DEC-2430`).
  

---

## Clause 6: Traits
### 6.1 Trait Definitions {Source: Draft 2 §28.1}

  <u>Syntax & Declaration</u>
    `trait Name { ... }`.
    *   **Associated Types**: `type Name;` (Abstract) or `type Name = T;` (Concrete).
    *   **Procedures**: `abstract(...);` or `concrete(...) { ... }`.
    *   **Self**: `Self` type refers to the implementing type.

  <u>Definition</u>
    Defines a set of behaviors (procedures) and types (associated types) that a type must provide.

  <u>Static Semantics</u>
    *   **Coherence**: A type may implement a trait at most once.
    *   **Orphan Rule**: Either the Type or the Trait MUST be local to the assembly.
  

### 6.2 Implementation {Source: Draft 2 §28.2}

  <u>Static Semantics</u>
    *   **Definition-Site Only**: Trait implementation (`record T <: Trait`) MUST occur as part of the `record` or `enum` declaration. "Extension" implementations (implementing a trait for a type defined elsewhere) are PROHIBITED.
    *   **Completeness**: Type MUST implement all abstract procedures declared in the trait (`E-TRS-2903`).
    *   **Overrides**: Concrete trait methods MAY be overridden; abstract methods MUST be implemented.
    *   **Keyword**: `override` keyword required when replacing a concrete method (`E-TRS-2902`). Forbidden when implementing an abstract method (`E-TRS-2901`).
  

### 6.3 Dynamic Polymorphism (Witnesses) {Source: Draft 2 §28.4}

  <u>Definition</u>
    Path 2: Dynamic dispatch via virtual table (vtable). Enables runtime polymorphism over trait bounds.
  
  <u>Syntax & Declaration</u>
    `witness Trait` - creates trait object (fat pointer: data pointer + vtable pointer).
  
  <u>Memory & Layout</u>
    **Dense Pointer Layout**: Two-word structure containing:
    1. Data pointer: `*imm T` to concrete instance
    2. VTable pointer: `*imm VTable` to trait's virtual method table
    
    **Stable VTable Layout** (MANDATED ORDER - ABI stability requirement):
    VTable entries MUST appear in this exact order:
    1. **Size** (`usize`): Size of concrete type in bytes
    2. **Alignment** (`usize`): Alignment requirement of concrete type
    3. **Destructor** (`*imm fn`): Drop glue function pointer (may be null if no Drop)
    4. **Methods**: Function pointers for trait methods, in declaration order
    
    **Rationale**: Fixed layout enables separate compilation and FFI vtables.
  
  <u>Constraints & Legality</u>
    *   **Witness Safety**: Trait MUST be "witness-safe" (object-safe).
    *   **VTable Eligibility** - Method eligible for vtable if ALL true:
        1. Has receiver parameter (`self`)
        2. Has NO generic type parameters
        3. Does NOT return `Self` by value (except as `box Self` or `witness Self`)
        4. Does NOT use `Self` in parameter/return types except via pointer indirection
    *   **Exclusion Mechanism**: Ineligible methods MUST be marked `where Self: Sized` to exclude from vtable.
    *   **Diagnostic**: Attempting to call excluded method on witness triggers `E-TRS-2940`.
  
  <u>Static Semantics</u>
    *   **Concrete-to-Witness Coercion** (§28.4.5): Value of concrete type `T` implementing witness-safe trait `Tr` MAY be implicitly coerced to `witness Tr`. Coercion MUST construct dense pointer (data + vtable) for `T`/`Tr` pair.
    *   **Dispatch**: Calls on witness perform runtime vtable lookup (one indirect call cost).
  

### 6.4 Opaque Polymorphism {Source: Draft 2 §28.5}

  <u>Definition</u>
    Path 3. Abstract return types.
  
  <u>Syntax & Declaration</u>
    `-> opaque Trait`.
  
  <u>Constraints & Legality</u>
    *   **Erasure**: Accessing members of the concrete underlying type is forbidden (`E-TRS-2910`). Only trait interface methods are visible.
  

### 6.5 Standard Traits {Source: Draft 2 Appx D.1}

  <u>Definition</u>
    Normative semantics for `Drop`, `Copy`, `Clone`, and `Iterator`.
    *   **Drop**: `procedure drop(~!)` (Required unique receiver).
    *   **Clone**: `procedure clone(~): Self` (Explicit deep copy).
    *   **Iterator**: 
        - `type Item` (Associated type).
        - `procedure next(~!): Option<Item>` (Stateful advancement).

  <u>Constraints & Legality</u>
    *   **Direct Calls**: Direct calls to `Drop::drop` are forbidden (`E-TRS-2920`).
    *   **Compiler Insertion**: The compiler automatically inserts `drop` calls for responsible bindings at scope exit.
    *   **Mutual Exclusion**: A type MUST NOT implement both `Copy` and `Drop` (`E-TRS-2921`).
    *   **Copy Fields**: `Copy` types MUST NOT contain non-`Copy` fields (`E-TRS-2922`).
    *   **Loop Protocol**: Types used in `loop ... in` MUST implement `Iterator`. The loop desugars to repeated calls to `next()`.
  

---

## Clause 7: Contracts
### 7.1 Contract Clauses {Source: Draft 2 §27.1-§27.5}

  <u>Syntax & Declaration</u>
    `[[ must => will ]]`. `where` invariants.
  
  <u>Constraints & Legality</u>
    **Purity**: Expressions in contracts MUST be pure (`E-CON-2802`).
  
  <u>Static Semantics</u>
    *   **Preconditions** (`must`): Caller obligation. Failure attributed to caller.
    *   **Postconditions** (`will`): Callee obligation. Failure attributed to callee.
    *   **Invariants** (`where`): Enforced at **Boundary Points**:
        1.  Post-Construction (Constructor/Literal).
        2.  Pre-Call (Public methods).
        3.  Post-Call (Methods taking `unique`/`partitioned` self).
    *   **Type Invariants**: Desugared as postcondition conjunction: `EffectivePost(P) = Post ∧ Inv(self)`
    *   **Loop Invariants**: MUST hold at initialization, every iteration start, and termination.
    *   **Contract Intrinsics**:
        - `@result`: Available in `will` clause. Refers to procedure's return value. Type matches return type. Cannot be shadowed.
        - `@entry(expr)`: Available in `will` clause. Evaluates `expr` at procedure entry (after param binding, before body). Expression MUST be pure, depend only on inputs, and evaluate to `Copy` or `Clone` type (`E-CON-2805`).
        - Usage of `@result` outside `will` triggers `E-CON-2806`.
    *   **Liskov Substitution**: Implementations may weaken preconditions ($P_{trait} \implies P_{impl}$) and strengthen postconditions ($Q_{impl} \implies Q_{trait}$). Violations are errors (`E-CON-2803`, `E-CON-2804`).
  

### 7.2 Verification Facts (Flow-Sensitive Typing) {Source: Draft 2 §27.7}

  <u>Definition</u>
    Virtual zero-size records of proved predicates used by the verifier.

  <u>Static Semantics</u>
    *   **Fact Dominance**: Fact $F(P, L)$ satisfies requirement at $S$ iff $L$ dominates $S$.
    *   **Fact Generation**: Control flow structures (`if`, `match`, `loop`) generate Verification Facts (§27.7.1).
    *   **Type Narrowing**: If a Fact $F(P, L)$ is active for a binding `x`, the type of `x` at location $L$ is effectively refined to `typeof(x) where { P }`.
    *   **Automatic Coercion**: The compiler **MUST** automatically coerce a binding to a refined subtype if the required invariant is satisfied by the active Verification Facts.
    *   **Dynamic Injection**: In `dynamic` mode, the compiler MUST synthesize facts by inserting runtime checks (panic guards) that dominate the requirement.
    *   **Optimizer Integration**: Implementations SHOULD use facts as assumptions for optimization (e.g., dead code elimination).
  

### 7.3 Verification Modes {Source: Draft 2 §27.6}

  <u>Definition</u>
    Three verification strategies for contract clauses:
    *   `static`: Require compile-time proof
    *   `dynamic`: Insert runtime checks  
    *   `trusted`: Assume without verification (UVB)
  
  <u>Constraints & Legality</u>
    *   **static mode**: Compiler MUST discharge every `must`/`will`/`where` clause via static proof.
        - Failure to prove: `E-CON-2801` (program is ill-formed)
        - No runtime overhead (checks proven at compile time)
    *   **dynamic mode**: Compiler MUST synthesize runtime checks at procedure boundaries.
        - Checks inserted at prologue (preconditions) and epilogue (postconditions)
        - Impure predicates MUST still be rejected (same purity rules as static)
        - Runtime check failure: panic
    *   **trusted mode**: NO verification performed; compiler assumes predicates true.
        - Violations produce Unverifiable Behavior (UVB)
        - MUST be explicitly requested via `[[verify(trusted)]]` attribute
        - Shifts responsibility entirely to programmer
    *   **Hybrid Default**: If no `[[verify(mode)]]` specified:
        1. Compiler first attempts static proof
        2. If proof fails, automatically falls back to dynamic checks
        3. Provides best effort static verification with dynamic safety net
  
  <u>Dynamic Semantics</u>
    *   **Runtime Check Behavior**: Checks evaluating to `false` MUST:
        1. Trigger panic in current thread
        2. Poison the current task state
        3. Prevent further execution of verified code region
    *   **Fact Injection**: Successful runtime checks synthesize verification facts:
        - Facts are inserted into CFG at check location
        - Facts dominate all program points reachable from check
        - Enables optimizer to use contract as assumption (e.g., dead code elimination)
  
  <u>Verification & Invariants</u>
    *   Static mode provides strongest guarantees (no runtime cost)
    *   Dynamic mode provides runtime safety without proof burden
    *   Trusted mode enables interfacing with external invariants not expressible in contract language
  
  <u>Examples</u>
  ```cursive
  // Dynamic verification example
  [[verify(dynamic)]]
  procedure pop(~! stack: Stack) [[must => stack.len > 0]] {
      // Runtime check: if stack.len <= 0, panic
      // Otherwise, synthesize fact (stack.len > 0) for this scope
  }

  // Trusted verification example (UVB)
  [[verify(trusted)]]
  procedure ffi_bridge(~ ctx: Context) [[must => external_invariant_holds()]] {
      // NO checks emitted
      // Programmer guarantees external_invariant_holds() is true
  }
  ```


### 7.4 Invariants and Refinements (where) {Source: Draft 2 §27.4}

  <u>Definition</u>
    Predicates that constrain the set of valid values for a type.
    *   **Nominal Invariants**: Defined on `record`/`enum` declarations (as currently specified).
    *   **Anonymous Invariants (Refinements)**: Defined on arbitrary type references (e.g., `usize`, `[T]`) in signatures or bindings.

  <u>Syntax & Declaration</u>
    *   **Type Syntax**: `<type> "where" "{" <predicate> "}"`
    *   **Example**: `procedure get(idx: usize where { self < len })`
    *   **Self Reference**: The keyword `self` in an anonymous invariant refers to the value being constrained.

  <u>Static Semantics</u>
    *   **Invariant Subtyping**: Type `T where { P }` is a subtype of `T where { Q }` if and only if the compiler can prove $P \implies Q$.
    *   **Intersection**: `(T where { P }) where { Q }` is equivalent to `T where { P && Q }`.
    *   **Verification Obligation**: Passing a value to a binding with an anonymous invariant generates a verification requirement (handled via Static Proof or Dynamic Check per §27.6).


---

## Clause 8: Expressions & Statements
### 8.1 Fundamentals {Source: Draft 2 §24.1-§24.2}

  <u>Definition</u>
    **Value Categories**:
    *   **Place Expression** (l-value): An expression representing a persistent, addressable memory location. Includes variable bindings, dereferences (`*p`), and field access (`x.f`) on places. Can be target of assignment (`=`) and address-of (`&`).
    *   **Value Expression** (r-value): An expression representing a temporary value without a persistent location. Includes literals, arithmetic results, and function return values.
    
    **Evaluation Order**: Left-to-Right.

  <u>Static Semantics</u>
    **Precedence & Associativity Table** (Highest to Lowest):
    1.  **Postfix** (Left): `()`, `[]`, `.`, `~>`, `::`
    2.  **Pipeline** (Left): `=>`
    3.  **Unary** (Right): `!`, `-`, `&`, `*`, `^`, `move`
    4.  **Power** (Right): `**`
    5.  **Multiplicative** (Left): `*`, `/`, `%`
    6.  **Additive** (Left): `+`, `-`
    7.  **Shift** (Left): `<<`, `>>`
    8.  **BitAnd** (Left): `&`
    9.  **BitXor** (Left): `^`
    10. **BitOr** (Left): `|`
    11. **Compare** (Left): `==`, `!=`, `<`, `<=`, `>`, `>=`
    12. **LogAnd** (Left): `&&`
    13. **LogOr** (Left): `||`
    14. **Assign** (Right): `=`, `+=`, `-=`, `*=`, `/=`, `%=`, `&=`, `|=`, `^=`, `<<=`, `>>=`

  <u>Constraints & Legality</u>
    *   **Bitwise NOT**: The `!` operator functions as Bitwise NOT when applied to integers (reusing the Logical NOT token).


### 8.2 Primary Expressions {Source: Draft 2 §24.3}

  <u>Syntax & Declaration</u>
    *   **Field Access**: `x.f`.
    *   **Indexing**: `x[i]`.
    *   **Method Calls**: `x~>m()` (Instance) vs `T::m()` (Static/Disambiguated).
    *   **Pipeline**: `x => f`.
    *   **Closure Literal**: `|params| body_expr` or `|params| { body_stmts }`.

  <u>Static Semantics</u>
    **Receiver Dispatch Algorithm (`~>`)**:
    1.  Search for method `m` in type `T` of receiver `x`.
    2.  If not found, search for method `m` in all Traits implemented by `T` that are visible in scope.
    3.  If ambiguous (multiple traits provide `m`), trigger `E-NAM-1305` (require disambiguation via `Trait::m(x)`).
    4.  Auto-dereference/Auto-reference `x` to match method receiver (`self`, `*self`, `&self`) is NOT performed (strict matching required).

  <u>Constraints & Legality</u>
    `~>` requires receiver. `::` requires type/trait.
  

### 8.3 Operators {Source: Draft 2 §24.4}

  <u>Syntax & Declaration</u>
    *   **Logical**: `&&`, `||`, `!` (Not).
    *   **Arithmetic**: `+`, `-`, `*`, `/`, `%`, `**`.
    *   **Bitwise**: `&`, `|`, `^` (Xor), `<<`, `>>`, `!` (Not - Overloaded).
    *   **Comparison**: `==`, `!=`, `<`, `<=`.
    *   **Address**: `&` (Ref), `*` (Deref), `^` (Alloc).
    *   **Conversion**: `as` (Explicit Cast).

  <u>Static Semantics</u>
    *   **Short-Circuiting**: Logical `&&` and `||` MUST implement short-circuit evaluation (right operand not evaluated if result determined by left).
    *   **Bitwise NOT**: The `!` operator acts as Bitwise NOT when applied to integer operands.

  <u>Constraints & Legality</u>
    *   **Pointer Arithmetic**: Restricted to `unsafe`.
        - **Offset**: `Ptr + Int` / `Ptr - Int` -> `Ptr` (scaled by sizeof type).
        - **Distance**: `Ptr - Ptr` -> `isize` (returns number of elements between pointers).
    *   **Cast Safety**: 
        - Numeric casts (truncating/extending) are Safe.
        - **Truncation**: High-order bits discarded.
        - **Extension**: Zero-extension for `uN`, Sign-extension for `iN`.
        - Raw pointer casts are Safe.
        - `transmute` is a separate `unsafe` intrinsic, NOT an operator.

  <u>Dynamic Semantics</u>
    *   **Overflow**: Integer arithmetic (`+`, `-`, `*`) MUST panic on overflow in `strict` mode. Wrap-around behavior in `release` mode is IDB.
    *   **Division**: Division by zero MUST panic in all modes.
  

### 8.4 Control Flow & Patterns {Source: Draft 2 §24.5-§24.8, §26}

  <u>Definition</u>
    *   **Move**: `move x` (Invalidation).
    *   **If**: `if cond { } else { }`.
    *   **Loop Forms**: 
        - **Infinite**: `loop { ... }`
        - **Conditional**: `loop condition { ... }` (Desugars to `loop { if !condition break; ... }`)
        - **Iterator**: `loop pattern in iterator { ... }` (Desugars to `match iterator.next()`)
    *   **Labels**: `'label: loop ...` (Labeled statement).
    *   **Control**: `break 'label`, `continue 'label`.
    *   **Match**: `match x { ... }`. Destructuring. Irrefutable vs Refutable.
  
  <u>Syntax & Declaration</u>
    **Patterns**: Literal, Wildcard, Tuple, Record, Enum, Modal State.

  <u>Static Semantics</u>
    **Matching Logic**:
    1.  **Literal Pattern**: Matches if `scrutinee == literal`.
    2.  **Wildcard (`_`)**: Matches any value; binds nothing.
    3.  **Identifier Pattern (`x`)**: Matches any value; binds to `x` (Move semantics unless `ref` keyword used).
    4.  **Variant Pattern (`Enum::Variant(p...)`)**: Matches if scrutinee discriminant equals Variant; recursively matches fields against `p...`.
    5.  **Range Pattern (`start..end`)**: Matches if `start <= scrutinee < end`.
    6.  **Type Pattern (`x: Type`)**: (Required for Unions) Matches if scrutinee's runtime type matches `Type`. Binds `x` with that specific type.
    7.  **Modal Pattern (`@State { ... }`)**: Matches if scrutinee's active state is `@State`. Destructures state payload.
    
    **Pattern Binding Scopes**:
    *   **Let statement**: Bindings introduced into current scope. Requires `shadow` keyword if shadowing outer binding.
    *   **Match arm**: Bindings introduced into arm-local scope. Implicitly shadow outer bindings (NO `shadow` keyword required - special case).
  
  <u>Constraints & Legality</u>
    *   **Exhaustiveness**: Exhaustiveness checking is MANDATORY for `enum`, `modal`, and `bool` scrutinees (`E-PAT-2741`).
    *   **Type Consistency**: All arms of a `match` expression MUST produce values of the equivalent type. This common type is the type of the match expression (`E-EXP-2571`).
    *   **Unreachability**: Match arms MUST NOT be unreachable. An arm is unreachable if its pattern covers only values already covered by preceding arms (`E-PAT-2751`).
    *   **Loop Break**: All `break` expressions within a single loop MUST produce values of the same type.
  
  <u>Dynamic Semantics</u>
    *   **If-Expression**: Evaluates condition; if `true` executes `then` block; otherwise executes `else` block (if present) or produces `()` (unit).
    *   **Loop**: Executes body indefinitely until `break` is encountered.
    *   **Break**: Terminates the nearest enclosing loop and yields the break-value to the loop expression's result.
    *   **Match**: Evaluates scrutinee once, then tests patterns in declaration order. Executes the arm of the *first* matching pattern.
  

### 8.5 Structured Expressions {Source: Draft 2 §24.9}

  <u>Definition</u>
    Expressions that introduce new lexical scopes or execution contexts.

  <u>Syntax & Declaration</u>
    *   **Block**: `{ stmt*; [result expr] }`.
    *   **Unsafe**: `unsafe { ... }`.
    *   **Region**: `region name { ... }` (See §3.3).
    *   **Parallel**: `parallel(bindings) { ... }` (See §9.1).
    *   **Comptime**: `comptime { ... }` (See §11.1).

  <u>Dynamic Semantics</u>
    *   **Block Evaluation**:
        1.  Executes statements sequentially.
        2.  Returns the value of the explicit `result` statement, OR the value of the final expression if unterminated.
        3.  If empty or terminated by `;`, returns `()`.
    *   **Unsafe Expression**: Evaluates body with safety checks suspended (See §3.5).
  

### 8.6 Statement Kinds {Source: Draft 2 §25}

  <u>Syntax & Declaration</u>
    *   **Declaration**: `let`/`var`.
    *   **Assignment**: `x = y` and Compound (`+=`, `-=`, `*=`, etc.).
    *   **Expression Stmt**: `expr;`.
    *   **Defer**: `defer { }`.
    *   **Control**: `return`, `result`.
    *   **Partition**: `partition ...`.
  
  <u>Definition</u>
    **Control Flow Statements**:
    *   **`return`**: Terminates the enclosing procedure and returns control to the caller. The return value (if any) becomes the procedure's result.
    *   **`result`**: Terminates the enclosing block expression and yields its value as the block's result. Used within block expressions to produce a value; does not exit the procedure.
    *   **Distinction**: `return` exits the procedure scope; `result` exits only the immediate block scope.
  
  <u>Constraints & Legality</u>
    *   `defer`: No non-local jumps.
    *   `return`: Prohibited at top-level scope.
    *   **Assignment**: Target must be mutable place.
  
  <u>Dynamic Semantics</u>
    *   **Defer**: Executed LIFO at scope exit.
    *   **Assignment Drop**: If the target `place` refers to an initialized responsible binding, the implementation MUST invoke `Drop::drop` on the existing value before or immediately after installation.
  

### 8.7 Variable Declaration {Source: Draft 2 §23.1}

  <u>Syntax & Declaration</u>
    `let pattern [: type] = expr` or `var pattern [: type] = expr`. (Type optional if inferable).
  
  <u>Static Semantics</u>
    *   **Binding Mutability**: `let` is immutable binding; `var` is mutable binding.
    *   **Orthogonality**: Binding mutability is distinct from data permission (`unique`, `const`).
    *   **Responsibility**: `=` operator establishes cleanup responsibility.
  
  <u>Constraints & Legality</u>
    *   **Irrefutability**: Patterns in `let` and `var` declarations MUST be irrefutable (`E-PAT-2711`).
  

### 8.8 Statement Termination {Source: Draft 2 §10.3}

  <u>Definition</u>
    Rules for delimiting statements via semicolons or newlines.
  
  <u>Static Semantics</u>
    *   **Terminators**: `;` or `<newline>`.
    *   **Continuation**: Newline ignored if inside delimiters `()`, `[]`, `{}` or after binary operators.

### 8.9 Special Contract Statements {Source: Draft 2 §25.7}

  <u>Syntax & Declaration</u>
    *   **Partition Statement**:
        ```ebnf
        partition_stmt ::= 
            [ "[[" "verify" "(" ("static" | "dynamic") ")" "]]" ]
            "partition" collection "by" "(" indices ")"
            [ "where" "(" proof_expr ")" ]
            block
        ```
  
  <u>Constraints & Legality</u>
    *   **Static Mode**: `proof_expr` MUST be provable by the linear verifier (`E-MEM-3012` on failure).
    *   **Dynamic Mode**: `proof_expr` (if present) and disjointness of `indices` are checked at runtime.
    *   **Consistency**: `[[verify(trusted)]]` is permitted (UVB) but requires `unsafe`.
  

---

## Clause 9: Concurrency
### 9.1 Path 1: Data Parallelism {Source: Draft 2 §31.2}

  <u>Definition</u>
    CREW Model. `parallel` epoch.

  <u>Syntax & Declaration</u>
    *   **Parallel Block**: `parallel(binding_list) { ... }`.
    *   **Fork Expression**: `fork closure_expr` (Returns `JobHandle<T>`).
    *   **Binding List**: Explicit list of `unique`/`partitioned` bindings to capture.

  <u>Static Semantics</u>
    *   **Invalidation**: Captured bindings are statically invalidated in the outer scope for the duration of the block.
    *   **Capture Downgrading Rules**:
        -   **Sync Types (`unique T` where `T: Sync`)**: Downgrades to **`concurrent T`**. Inner scope can perform synchronized operations.
        -   **Non-Sync Types (`unique T` where `T: !Sync`)**: Downgrades to **`const T`**. Pure data becomes deeply immutable to prevent data races.
    *   **JobHandle**: MUST NOT implement `Copy`, `Clone`, or `detach`.
    *   **Linear Usage Analysis** (Static Join Requirement `E-CON-3202`):
        - **Usage Count**: Compiler tracks usage of every `JobHandle` returned by `spawn`.
        - **Consumption**: Calling `join()` consumes the handle (Usage count -> 0).
        - **Invariant**: On exit of `parallel` block, Usage Count of all handles MUST be exactly 0.
        - **Branching**: In conditional branches, handle MUST be joined in ALL paths or NO paths.
  
  <u>Dynamic Semantics</u>
    *   **Fork-Join Model**: The `parallel` block suspends the current thread and spawns a logical task for the block body.
    *   **Barrier**: The end of the `parallel` block acts as a strict synchronization barrier. Control returns to the parent thread only after all spawned tasks have completed.
    *   **Panic Propagation**: If any task panics, the parent thread MUST panic upon synchronization.
  

### 9.2 Path 2: Stateful Coordination {Source: Draft 2 §31.3}

  <u>Definition</u>
    Threads and Locks via Capabilities.

  <u>Static Semantics</u>
    *   `System.spawn`: Requires thread-safe captures (no `partitioned`).
    *   **Mutex & Atomic Signatures**:
        -   **Mutex**: `procedure lock(~|): MutexGuard<T>` (Requires `concurrent` permission).
        -   **Atomic**: `procedure fetch_add(~|, v: i32): i32` (Requires `concurrent` permission).
    *   **Mutex<T> States**: 
        *   `@Unlocked` (Empty).
        *   `@Locked` (Contains `T`).
        *   **Transitions**: `lock` (@Unlocked -> @Locked), `unlock` (@Locked -> @Unlocked).
    *   **MutexGuard<T> RAII Pattern** (§31.3.3):
        - `lock()` transition MUST return `MutexGuard<T>` (responsible binding)
        - Guard holds `unique` reference to protected data `T`
        - Guard implements `Drop`; destructor MUST invoke `unlock()` transition
        - Prevents forgetting to unlock (automatic via RAII)
    *   **Thread<T> States**: 
        *   `@Spawned` (Running).
        *   `@Joined` (Complete).
        *   `@Detached` (Discarded).
        *   **Transitions**: `join` (@Spawned -> @Joined), `detach` (@Spawned -> @Detached).

  <u>Concurrency & Safety</u>
    *   **Deadlocks**: Allowed (Safe). Accessing a locked Mutex blocks the thread.
    *   **Ordering**: `lock()` and `unlock()` imply release/acquire memory barriers.
  

### 9.3 Thread Safety Rules {Source: Draft 2 §31.1}

  <u>Constraints & Legality</u>
    *   `const`: Safe to share.
    *   `unique`: Safe to transfer.
    *   `partitioned`: Unsafe to share or transfer (`E-CON-3201`).
  

---

## Clause 10: The Capability System
### 10.1 Capability Mechanics {Source: Draft 2 §30.4–§30.6}

  <u>Definition</u>
    Rules for managing authority distribution beyond the root.
  
  <u>Static Semantics</u>
    *   **Attenuation**: Capabilities must provide methods to derive restricted subsets (e.g., `fs.restrict(path)`).
    *   **Propagation**: Capabilities must be passed as explicit parameters; no ambient authority.
    *   **User-Defined**: Wrappers around system capabilities permitted.
  

### 10.2 System Capabilities {Source: Draft 2 Appx D.2; Amendment 1}

  <u>Definition</u>
    System capabilities are **Synchronized Resources**.
    *   **Traits**: `FileSystem`, `Network`, `HeapAllocator`, `Time`.
    *   **System Record**: Must provide `exit(code)` and `get_env(key)`.

  <u>Constraints & Legality</u>
    *   **Sync Implementation**: All System Capability traits (`FileSystem`, `Network`, `HeapAllocator`) MUST implement the `Sync` marker trait.
    *   **Method Signatures**: Methods on these traits that perform side effects (IO, allocation) MUST accept `self` as **`concurrent`** (or `witness concurrent`).
    *   **Const Purity**: Methods accepting `const` (`~`) MUST guarantee ZERO logical state changes to the resource (e.g., `time.now()` is const, but `file.write()` is concurrent).
  

---

## Clause 11: Metaprogramming
### 11.1 Compile-Time Execution {Source: Draft 2 §33.1, §33.6}

  <u>Definition</u>
    `comptime` blocks and procedures.
  
  <u>Constraints & Legality</u>
    Sandboxed (no runtime side effects). Resource limits (`E-MET-3402`).
    *   **Prohibitions**: `unsafe` blocks and FFI calls are STRICTLY FORBIDDEN in `comptime` context.
    *   **Caching**: Implementations **SHOULD** cache the result of `comptime` blocks and procedures annotated with the `[[cache]]` attribute.
        - **Cache Key**: Must include the AST of the block, the values of all captured identifiers, and the compiler version.
        - **Determinism**: Blocks marked `[[cache]]` MUST be purely deterministic; reliance on non-deterministic compiler state (e.g., memory addresses of AST nodes) in cached blocks is **Unspecified Behavior**.
    *   **Parameters**: Parameters to `comptime` procedures MUST be:
        - Primitive types (`iN`, `uN`, `fN`, `bool`, `char`)
        - `string` (literals)
        - `Type` (introspection handles)
        - `Expr`, `Stmt`, `Decl` (AST nodes)
        - Composites of the above.
        - **Prohibited**: Capabilities, Handles, Raw Pointers.
  
  <u>Complexity & Limits</u>
    Minimums: Recursion (256), Steps (1M), Memory (64MiB), AST Depth (1024).
  

### 11.2 Introspection {Source: Draft 2 §33.2}

  <u>Definition</u>
    `reflect_type<T>()` intrinsic returns metadata about T.

  <u>Syntax & Declaration</u>
    **Mandatory `TypeInfo` Structure**:
    *   `kind`: Enum (`Primitive`, `Record`, `Enum`, `Pointer`, etc.)
    *   `name`: Fully qualified string name.
    *   `fields`: List of `FieldInfo` (name, type, offset, attributes) [Records only].
    *   `variants`: List of `VariantInfo` (name, discriminant, payload) [Enums only].
    *   `layout`: Size and alignment.
  
  <u>Constraints & Legality</u>
    *   **Completeness**: `reflect_type<T>` MUST only be invoked on types `T` that are fully defined at the point of invocation.
    *   **Recursion**: Recursive type dependencies that prevent layout resolution during introspection MUST be diagnosed as errors.

### 11.3 Quasiquoting and Emission {Source: Draft 2 §33.3-§33.5}

  <u>Definition</u>
    **The AST Model**:
    *   `Node`: Base trait for all syntax elements.
    *   `Expr`: Tagged union of expression nodes (Binary, Call, Literal, etc.).
    *   `Stmt`: Tagged union of statement nodes (Let, Return, ExprStmt, etc.).
    *   `Decl`: Tagged union of declaration nodes (Proc, Record, Enum, etc.).
    *   `Type`: Representation of resolved types.

  <u>Syntax & Declaration</u>
    *   **Quote**: `quote { ... }`.
    *   **Splice**: `$(...)`. Supports values, identifiers, ASTs.
    *   **Emit**: `cg.emit(ast)`. Injects code into module scope.
  
  <u>Static Semantics</u>
    *   **Quote Result**: `quote { ... }` evaluates to `QuotedExpr` or `QuotedBlock`.
    *   **Hygiene Algorithm** (prevents identifier capture bugs):
        - **Captured Variables**: Free variables in quote use lexical scope at `quote` definition site
        - **New Bindings**: Identifiers introduced within quote generate fresh, unique names to prevent collision with splice-site bindings
        - **Rationale**: Prevents accidental variable capture when spliced AST introduces bindings
    *   **Emission Requirement**: The `emit` operation requires the `ComptimeCodegen` capability.
        - `cg.emit(ast)` where `cg` is `witness ComptimeCodegen`.
        - This capability is provided only to `comptime` entry points.
    *   **Value Splicing**: Literals interpolated as values.
    *   **Identifier Splicing**: Strings interpolated as identifiers (`$(str_var)`). Validates identifier syntax (`E-MET-3403`).
    *   **AST Splicing**: `QuotedBlock`/`QuotedExpr` interpolated structurally.
  

---

## Clause 12: Interoperability
### 12.1 Extern Declarations {Source: Draft 2 §32.1, §32.5.2}

  <u>Syntax & Declaration</u>
    *   **Import**: `extern "ABI" procedure name(params): ret;` (No body).
    *   **Export**: `extern "ABI" procedure name(params): ret { body }`.
    *   **Attributes**: `[[link_name]]`, `[[no_mangle]]`, `[[unwind]]`.
  
  <u>Constraints & Legality</u>
    *   **Variadics**: Variadic arguments (`...`) are prohibited (`E-FFI-3304`).
    *   **Unwind Attribute Values**: `abort` (default) or `catch`.
    *   **Export Constraints**: Procedures exported to FFI (defined with a body):
        1. MUST be `public`.
        2. MUST NOT capture environment (no closures).
        3. MUST use FFI-safe types in signature.

  <u>Dynamic Semantics</u>
    **Unwind Behavior** (critical for panic safety across FFI boundary):
    
    *   `[[unwind(abort)]]` (DEFAULT):
        - If Cursive code panics while executing within extern procedure call
        - Process MUST immediately abort (no unwinding across FFI boundary)
        - Rationale: Foreign code may not be exception-safe
    
    *   `[[unwind(catch)]]`:
        - Panic is caught at FFI boundary
        - Procedure returns failure value to caller instead of unwinding
        - **REQUIREMENT**: Procedure return type MUST be capable of representing failure state
        - Examples: `T | Error` (Union), `Option<T>`, nullable pointer
        - Undefined behavior if return type cannot represent failure
    
    *   **Safety Rationale**: Prevents unwinding through foreign stack frames which may violate RAII assumptions of C/C++ code.
  
  <u>Dynamic Semantics</u>
    *   **Unwind**: `[[unwind(abort)]]` (default) terminates process on panic. `[[unwind(catch)]]` returns failure value.
    *   **Catch Requirement**: Procedures marked `[[unwind(catch)]]` MUST return a type capable of representing a failure state (undefined behavior otherwise).
  

### 12.2 FFI Safety {Source: Draft 2 §32.2, §32.4}

  <u>Definition</u>
    **FFI-Safe Catalog**:
    *   **Primitives**: Integers (`i8`..`i128`, `u8`..`u128`), Floats (`f16`..`f64`), Unit `()`.
    *   **Raw Pointers**: `*imm T`, `*mut T` (where `T` is FFI-Safe).
    *   **Function Pointers**: `extern` ABI function pointers.
    *   **Composites**: `record` and `enum` types marked `[[repr(C)]]` containing only FFI-Safe types.
    
    **ABI String Literals** (calling conventions):
    *   **Mandatory**: Implementations MUST support `"C"` (default C convention) and `"system"` (OS syscall convention: stdcall on Win32, C on Linux)
    *   **Optional**: MAY support platform-specific conventions (e.g., `"stdcall"`, `"fastcall"`)
  
  <u>Constraints & Legality</u>
    *   **UVB**: Calls are UVB, require `unsafe`.
    *   **Variadics**: `...` arguments prohibited (`E-FFI-3304`).
    *   **Types**: Only FFI-Safe types (primitives, raw ptrs, repr-C) allowed (`E-FFI-3301`).
  

### 12.3 Representation {Source: Draft 2 §32.3}

  <u>Static Semantics</u>
    *   `[[repr(C)]]`: Standard C layout.
    *   Default: Unspecified layout.
  

---

## Appendices
### Appendix A: Formal Grammar (ANTLR) {Source: Draft 2 Appx A}

  <u>Definition</u>
    Complete normative grammar for Cursive in ANTLR4 format. This grammar defines all lexical and syntactic productions required to parse valid Cursive source code.
  
  <u>Syntax & Declaration</u>
    **Grammar Structure**:
    *   **Lexer Rules**: Keywords, identifiers (XID_START/XID_Continue), literals (integer/float/string/char), operators, comments
    *   **Parser Rules**: All declarations (record, enum, modal, trait, procedure), expressions (precedence-encoded), statements, patterns
    *   **Operator Precedence**: Encoded in grammar hierarchy (14 levels from Postfix to Assignment)
    *   **Comment Nesting**: Block comments (`/* ... */`) MUST nest recursively; lexer maintains nesting counter
    *   **Maximal Munch Exception**: Sequence `>>` inside generic argument list MUST be split into two `>` tokens (context-sensitive lexing)
    
  <u>Constraints & Legality</u>
    *   Keywords are reserved context-insensitively
    *   Receiver shorthands (`~`, `~%`, `~!`) MUST appear only as first parameter in type method declarations
    *   All lexical rules MUST conform to preprocessing pipeline output (§2.1)
  
  <u>Examples</u>
    Complete ANTLR4 grammar suitable for parser generators. Includes all production rules from top-level `sourceFile` to terminal tokens.

### Appendix B: Diagnostic Code Taxonomy {Source: Draft 2 Appx B}

This appendix defines the normative taxonomy for compiler diagnostics. All conforming implementations **MUST** use these codes when reporting the corresponding conditions to ensure consistent error reporting across toolchains.

#### B.1 Diagnostic Code Format

Diagnostic codes follow the format `K-CAT-FFNN`:

*   **K (Kind/Severity):**
    *   `E`: **Error**. A violation of a normative requirement. Compilation cannot proceed to codegen.
    *   `W`: **Warning**. Code is well-formed but contains potential issues (e.g., deprecated usage).
    *   `N`: **Note**. Informational message attached to an error or warning.
*   **CAT (Category):** A three-letter code identifying the language subsystem.
*   **FF (Feature Bucket):** Two digits identifying the specific feature area or chapter within the category.
*   **NN (Number):** Two digits uniquely identifying the specific condition within the bucket.

#### B.2 Feature Buckets (FF Values)

The following tables define the "Feature Bucket" (`FF`) values for each Category (`CAT`). These values generally align with the specification chapters but are grouped for diagnostic utility.

##### B.2.1 Source, Syntax, & Modules

| Category | FF   | Feature Name         | Relevant Chapters |
| :------- | :--- | :------------------- | :---------------- |
| **SRC**  | 01   | Encoding & Structure | §8.1, §8.2        |
|          | 02   | Resource Limits      | §6.5, §8.2.2      |
|          | 03   | Lexical Elements     | §9                |
| **SYN**  | 01   | Recursive Limits     | §10.2             |
|          | 02   | General Grammar      | Appx A            |
| **MOD**  | 11   | Manifest & Projects  | §11               |
|          | 12   | Imports & Visibility | §12               |
|          | 14   | Initialization       | §14               |
| **NAM**  | 13   | Scopes & Shadowing   | §13               |

##### B.2.2 Type System

| Category | FF   | Feature Name               | Relevant Chapters |
| :------- | :--- | :------------------------- | :---------------- |
| **TYP**  | 15   | Foundations                | §15               |
|          | 16   | Permissions                | §16               |
|          | 17   | Primitives                 | §17               |
|          | 18   | Composites                 | §18               |
|          | 19   | Modal Types (incl. String) | §19, §20          |
|          | 20   | Pointers                   | §21               |
|          | 23   | Function Types             | §22               |

##### B.2.3 Logic & Semantics

| Category | FF   | Feature Name     | Relevant Chapters |
| :------- | :--- | :--------------- | :---------------- |
| **DEC**  | 24   | Declarations     | §24               |
| **EXP**  | 25   | Expressions      | §25               |
| **STM**  | 26   | Statements       | §26               |
| **PAT**  | 27   | Pattern Matching | §27               |
| **CON**  | 28   | Contracts        | §28               |
|          | 32   | Concurrency      | §32               |

##### B.2.4 Systems & Metaprogramming

| Category | FF   | Feature Name        | Relevant Chapters |
| :------- | :--- | :------------------ | :---------------- |
| **TRS**  | 29   | Traits              | §29               |
| **MEM**  | 30   | Memory Model        | §30               |
|          | 31   | Object Capabilities | §31               |
| **FFI**  | 33   | FFI & ABI           | §33               |
| **MET**  | 34   | Metaprogramming     | §34               |

#### B.3 Normative Diagnostic Catalog

**IMPORTANT NOTE FOR WRITERS**: This section MUST enumerate EVERY diagnostic code from EVERY chapter's "Diagnostics Summary" section in Draft2. The catalog below is illustrative structure only.

**Requirements**:
- Each code entry MUST include: Code identifier, Severity, Description
- Codes MUST be organized by Category (SRC, SYN, MOD, NAM, TYP, DEC, EXP, STM, PAT, CON, TRS, MEM, FFI, MET)
- Writers MUST cross-reference each chapter's diagnostic summary to ensure no codes omitted
- Every diagnostic triggered in specification text must have entry here

The following tables list all diagnostic codes required by this specification, organized by their Category and Feature Bucket.

##### B.3.1 SRC (Source)

| Code         | Severity | Description                                                                  |
| :----------- | :------- | :--------------------------------------------------------------------------- |
| `E-SRC-0101` | Error    | Invalid UTF-8 byte sequence in source file.                                  |
| `E-SRC-0102` | Error    | Source file exceeds implementation-defined maximum size.                     |
| `E-SRC-0103` | Error    | UTF-8 Byte Order Mark (BOM) found after the first byte.                      |
| `E-SRC-0104` | Error    | Source contains prohibited control character or null byte.                   |
| `E-SRC-0105` | Error    | Maximum logical line count exceeded.                                         |
| `E-SRC-0106` | Error    | Maximum line length exceeded.                                                |
| `W-SRC-0101` | Warning  | Source file begins with UTF-8 BOM (should be stripped).                      |
| `E-SRC-0204` | Error    | String literal length exceeds implementation limit during compile-time eval. |
| `E-SRC-0301` | Error    | Unterminated string literal.                                                 |
| `E-SRC-0302` | Error    | Invalid escape sequence in literal.                                          |
| `E-SRC-0303` | Error    | Invalid character literal (empty, multiple chars).                           |
| `E-SRC-0304` | Error    | Malformed numeric literal.                                                   |
| `E-SRC-0305` | Error    | Reserved keyword used as identifier.                                         |
| `E-SRC-0306` | Error    | Unterminated block comment.                                                  |
| `E-SRC-0307` | Error    | Invalid Unicode in identifier (violates UAX31).                              |
| `E-SRC-0308` | Error    | Lexically sensitive Unicode character in identifier (Strict Mode).           |
| `W-SRC-0301` | Warning  | Integer literal has leading zeros (interpreted as decimal, not octal).       |
| `W-SRC-0308` | Warning  | Lexically sensitive Unicode character in identifier (Permissive Mode).       |

##### B.3.2 SYN (Syntax)

| Code         | Severity | Description                                             |
| :----------- | :------- | :------------------------------------------------------ |
| `E-SYN-0101` | Error    | Block nesting depth exceeded implementation limit.      |
| `E-SYN-0102` | Error    | Expression nesting depth exceeded implementation limit. |
| `E-SYN-0103` | Error    | Delimiter nesting depth exceeded implementation limit.  |

##### B.3.3 MOD (Modules) & NAM (Naming)

| Code         | Severity | Description                                           |
| :----------- | :------- | :---------------------------------------------------- |
| `E-MOD-1101` | Error    | `Cursive.toml` missing or syntactically malformed.    |
| `E-MOD-1102` | Error    | Manifest `[paths]` table missing or invalid.          |
| `E-MOD-1103` | Error    | Assembly references undefined root in `[paths]`.      |
| `E-MOD-1104` | Error    | Module path collision on case-insensitive filesystem. |
| `E-MOD-1105` | Error    | Module path component is a reserved keyword.          |
| `E-MOD-1106` | Error    | Module path component is not a valid identifier.      |
| `E-MOD-1107` | Error    | Manifest `[project]` table missing required keys.     |
| `E-MOD-1108` | Error    | Duplicate assembly name in manifest.                  |
| `E-MOD-1109` | Error    | Incompatible language version in manifest.            |
| `E-MOD-1201` | Error    | `import` path does not resolve to external module.    |
| `E-MOD-1202` | Error    | `use` path does not resolve to accessible module.     |
| `E-MOD-1203` | Error    | Name conflict in `use` or `import as`.                |
| `E-MOD-1204` | Error    | Item in `use` path not found or not visible.          |
| `E-MOD-1205` | Error    | Attempt to `public use` a non-public item.            |
| `E-MOD-1206` | Error    | Duplicate item in `use` list.                         |
| `E-MOD-1207` | Error    | Access to `protected` item denied.                    |
| `E-MOD-1401` | Error    | Cyclic dependency in eager module initialization.     |
| `W-MOD-1101` | Warning  | Potential module path collision (case-sensitivity).   |
| `W-MOD-1201` | Warning  | Wildcard `use` (`*`) usage.                           |
| `E-NAM-1301` | Error    | Unresolved name.                                      |
| `E-NAM-1302` | Error    | Duplicate name declaration in same scope.             |
| `E-NAM-1303` | Error    | Shadowing existing binding without `shadow` keyword.  |
| `E-NAM-1304` | Error    | Unresolved module in qualified path.                  |
| `E-NAM-1305` | Error    | Unresolved or inaccessible member in path.            |
| `E-NAM-1306` | Error    | Unnecessary use of `shadow` keyword.                  |
| `W-NAM-1303` | Warning  | Implicit shadowing (Permissive Mode).                 |

##### B.3.4 TYP (Type System)

| Code         | Severity | Description                                             |
| :----------- | :------- | :------------------------------------------------------ |
| `E-TYP-1501` | Error    | Type mismatch or other type checking failure.           |
| `E-TYP-1505` | Error    | Missing required top-level type annotation.             |
| `E-TYP-1601` | Error    | Attempt to mutate data via a `const` reference.         |
| `E-TYP-1602` | Error    | Violation of `unique` exclusion (aliasing detected).    |
| `E-TYP-1603` | Error    | Partitioning system violation.                          |
| `W-TYP-1701` | Warning  | `f16` arithmetic may be emulated and slow.              |
| `E-TYP-1801` | Error    | Tuple or Array index out of bounds.                     |
| `E-TYP-1901` | Panic    | String slice boundary is not a valid `char` boundary.   |
| `E-TYP-1902` | Error    | Direct indexing of `string` is forbidden.               |
| `E-TYP-1910` | Error    | Modal type declares no states.                          |
| `E-TYP-1911` | Error    | Duplicate state in modal declaration.                   |
| `E-TYP-1912` | Error    | Accessing field existing only in specific modal state.  |
| `E-TYP-1913` | Error    | Accessing method existing only in specific modal state. |
| `E-TYP-1914` | Error    | Missing implementation for modal transition.            |
| `E-TYP-1915` | Error    | Transition body does not return target state type.      |
| `E-TYP-1920` | Error    | Non-exhaustive match on modal type.                     |
| `E-TYP-2001` | Error    | Dereference of `Ptr<T>@Null`.                           |
| `E-TYP-2002` | Error    | Dereference of `Ptr<T>@Expired`.                        |
| `E-TYP-2003` | Error    | Dereference of raw pointer outside `unsafe`.            |
| `E-TYP-2301` | Error    | Function argument count mismatch.                       |
| `E-TYP-2302` | Error    | Type mismatch (general).                                |
| `E-TYP-2304` | Error    | Control flow path missing `result` value.               |

##### B.3.5 DEC (Declarations), EXP (Expressions), STM (Statements)

| Code         | Severity | Description                                           |
| :----------- | :------- | :---------------------------------------------------- |
| `E-DEC-2401` | Error    | Re-assignment of immutable `let` binding.             |
| `E-DEC-2411` | Error    | `move` keyword mismatch at call site.                 |
| `E-DEC-2420` | Error    | Recursive type alias detected.                        |
| `E-DEC-2430` | Error    | Missing or duplicate `main` procedure.                |
| `E-DEC-2431` | Error    | Invalid signature for `main` (must accept `Context`). |
| `E-DEC-2450` | Error    | Malformed attribute syntax.                           |
| `E-DEC-2451` | Error    | Unknown attribute name.                               |
| `E-EXP-2501` | Error    | Expression type mismatch.                             |
| `E-EXP-2502` | Error    | `value` expression used where `place` required.       |
| `E-EXP-2511` | Error    | Identifier resolves to type/module in value context.  |
| `E-EXP-2531` | Error    | Invalid field/tuple access (not found or invisible).  |
| `E-EXP-2532` | Error    | Procedure call argument count mismatch.               |
| `E-EXP-2533` | Error    | Method call using `.` instead of `::`.                |
| `E-EXP-2535` | Error    | Invalid pipeline `=>` right-hand side.                |
| `E-EXP-2541` | Error    | Logical operator applied to non-bool.                 |
| `E-EXP-2542` | Error    | Invalid types for arithmetic/bitwise operator.        |
| `E-EXP-2545` | Error    | Address-of `&` applied to non-place.                  |
| `E-EXP-2561` | Error    | `if` without `else` in value context.                 |
| `E-EXP-2571` | Error    | Incompatible types in `match` arms.                   |
| `E-EXP-2582` | Error    | Mismatched types in `loop` `break` values.            |
| `E-EXP-2591` | Error    | Returning region-allocated value from `region` block. |
| `E-EXP-2592` | Error    | `parallel` result depends on invalidated binding.     |
| `E-STM-2631` | Error    | Assignment target is not a place.                     |
| `E-STM-2651` | Error    | `defer` block returns non-unit value.                 |
| `E-STM-2652` | Error    | Non-local control flow (return/break) in `defer`.     |
| `E-STM-2661` | Error    | `return` value type mismatch.                         |
| `E-STM-2662` | Error    | `break` used outside loop.                            |
| `E-STM-2663` | Error    | `continue` used outside loop.                         |
| `E-STM-2664` | Error    | `result` value mismatch with block type.              |
| `E-STM-2671` | Error    | `partition` proof verification failed.                |

##### B.3.6 PAT (Patterns)

| Code         | Severity | Description                                    |
| :----------- | :------- | :--------------------------------------------- |
| `E-PAT-2711` | Error    | Refutable pattern used in irrefutable context. |
| `E-PAT-2741` | Error    | Non-exhaustive pattern match.                  |
| `E-PAT-2751` | Error    | Unreachable match arm.                         |

##### B.3.7 CON (Contracts & Concurrency)

| Code         | Severity | Description                                                              |
| :----------- | :------- | :----------------------------------------------------------------------- |
| `E-CON-2801` | Error    | Static contract verification failed.                                     |
| `E-CON-2802` | Error    | Impure/Unsafe expression in contract.                                    |
| `E-CON-2803` | Error    | Liskov violation: Precondition strengthened.                             |
| `E-CON-2804` | Error    | Liskov violation: Postcondition weakened.                                |
| `E-CON-2805` | Error    | `@entry` applied to non-Copy/Clone type.                                 |
| `E-CON-2806` | Error    | `@result` used outside `will` clause.                                    |
| `E-CON-3201` | Error    | Thread Safety: Capturing `partitioned` binding in concurrency primitive. |
| `E-CON-3202` | Error    | Static Join violation: `JobHandle` not joined in `parallel`.             |
| `E-CON-3203` | Error    | Spawn capture violation (e.g. `unique` not moved).                       |

##### B.3.8 TRS (Traits)

| Code         | Severity | Description                                            |
| :----------- | :------- | :----------------------------------------------------- |
| `E-TRS-2901` | Error    | Abstract implementation incorrectly marked `override`. |
| `E-TRS-2902` | Error    | Concrete override missing `override` keyword.          |
| `E-TRS-2903` | Error    | Missing implementation for required trait procedure.   |
| `E-TRS-2910` | Error    | Accessing non-trait member on opaque type.             |
| `E-TRS-2920` | Error    | Explicit call to `Drop::drop`.                         |
| `E-TRS-2921` | Error    | Type implements both `Copy` and `Drop`.                |
| `E-TRS-2922` | Error    | `Copy` implementation on type with non-Copy fields.    |
| `E-TRS-2940` | Error    | Calling `where Self: Sized` procedure on witness.      |

##### B.3.9 MEM (Memory & Safety)

| Code         | Severity | Description                                 |
| :----------- | :------- | :------------------------------------------ |
| `E-MEM-3001` | Error    | Use of moved value.                         |
| `E-MEM-3005` | Error    | Explicit call to destructor.                |
| `E-MEM-3010` | Error    | Static record partition conflict.           |
| `E-MEM-3012` | Error    | Partition contract proof failed (Static).   |
| `P-MEM-3013` | Panic    | Dynamic partition check failed (Runtime).   |
| `E-MEM-3020` | Error    | Region pointer escape.                      |
| `E-MEM-3021` | Error    | Region allocation `^` outside region scope. |
| `E-MEM-3030` | Error    | Unsafe operation in safe code.              |
| `E-MEM-3031` | Error    | `transmute` size mismatch.                  |

##### B.3.10 FFI (Interoperability)

| Code         | Severity | Description                                        |
| :----------- | :------- | :------------------------------------------------- |
| `E-FFI-3301` | Error    | Non-FFI-Safe type in `extern` signature.           |
| `E-FFI-3302` | Error    | Call to `extern` procedure outside `unsafe` block. |
| `E-FFI-3303` | Error    | Invalid application of `[[repr(C)]]`.              |
| `E-FFI-3304` | Error    | Variadic arguments not supported.                  |

##### B.3.11 MET (Metaprogramming)

| Code         | Severity | Description                                       |
| :----------- | :------- | :------------------------------------------------ |
| `E-MET-3401` | Error    | `comptime` procedure called from runtime context. |
| `E-MET-3402` | Error    | Compile-time resource limit exceeded.             |
| `E-MET-3403` | Error    | Invalid identifier string in interpolation.       |
| `E-MET-3404` | Error    | Syntax error in `quote` block.                    |
| `E-MET-3405` | Error    | Emitted AST failed type checking.                 |
| `E-MET-3406` | Error    | `emit` called without capability.                 |

### Appendix C: Conformance Dossier Schema {Source: Draft 2 Appx C}

This appendix defines the normative JSON Schema for the Conformance Dossier. A conforming implementation **MUST** produce a JSON artifact matching this schema when the `dossier` emission phase is active.

#### C.1 File Format
The dossier **MUST** be a valid JSON document encoded in UTF-8.

#### C.2 Schema Definition
```json
{
  "$schema": "http://json-schema.org/draft-07/schema#",
  "title": "Cursive Conformance Dossier",
  "type": "object",
  "required": ["metadata", "configuration", "safety_report", "implementation_limits"],
  "properties": {
    
    "metadata": {
      "type": "object",
      "description": "Information about the compiler and build environment.",
      "required": ["compiler_id", "compiler_version", "target_triple", "build_timestamp"],
      "properties": {
        "compiler_id": { "type": "string", "example": "cursive-ref-impl" },
        "compiler_version": { "type": "string", "pattern": "^\\d+\\.\\d+\\.\\d+$" },
        "target_triple": { "type": "string", "example": "x86_64-unknown-linux-gnu" },
        "build_timestamp": { "type": "string", "format": "date-time" }
      }
    },

    "configuration": {
      "type": "object",
      "description": "The conformance mode and settings used for this compilation.",
      "required": ["mode", "features"],
      "properties": {
        "mode": { 
          "type": "string", 
          "enum": ["strict", "permissive"],
          "description": "Strict mode rejects unsafe blocks without attestation." 
        },
        "features": {
          "type": "array",
          "items": { "type": "string" },
          "description": "List of enabled language feature flags."
        }
      }
    },

    "safety_report": {
      "type": "object",
      "description": "Inventory of unsafe usage and attestations.",
      "required": ["unsafe_blocks_count", "attestations", "ifndr_instances"],
      "properties": {
        "unsafe_blocks_count": { "type": "integer", "minimum": 0 },
        "attestations": {
          "type": "array",
          "description": "List of all attested unsafe blocks in the program closure.",
          "items": {
            "type": "object",
            "required": ["location", "mechanism", "auditor", "justification"],
            "properties": {
              "location": {
                "type": "object",
                "properties": {
                  "file": { "type": "string" },
                  "line": { "type": "integer" },
                  "module": { "type": "string" }
                }
              },
              "mechanism": { "type": "string", "example": "Manual Audit" },
              "auditor": { "type": "string" },
              "date": { "type": "string", "format": "date" },
              "justification": { "type": "string" },
              "proof_uri": { "type": "string", "format": "uri" }
            }
          }
        },
        "unattested_violations": {
          "type": "array",
          "description": "In Permissive mode, lists unsafe blocks missing attestation.",
          "items": {
            "type": "object",
            "properties": {
              "file": { "type": "string" },
              "line": { "type": "integer" }
            }
          }
        },
        "ifndr_instances": {
          "type": "array",
          "description": "List of all IFNDR (Ill-Formed, No Diagnostic Required) instances encountered during compilation.",
          "items": {
            "type": "object",
            "required": ["location", "category"],
            "properties": {
              "location": {
                "type": "object",
                "properties": {
                  "file": { "type": "string" },
                  "line": { "type": "integer" },
                  "module": { "type": "string" }
                }
              },
              "category": {
                "type": "string",
                "description": "IFNDR category (e.g., 'OOB in Const Eval', 'Recursion Limits')"
              },
              "description": {
                "type": "string",
                "description": "Optional additional context about the IFNDR condition"
              }
            }
          }
        }
      }
    },

    "implementation_defined_behavior": {
      "type": "object",
      "description": "Documentation of IDB choices made by this implementation.",
      "required": ["type_layout", "pointer_width"],
      "properties": {
        "pointer_width": { "type": "integer", "enum": [32, 64] },
        "type_layout": {
          "type": "object",
          "additionalProperties": {
             "type": "object",
             "properties": {
               "size": { "type": "integer" },
               "alignment": { "type": "integer" }
             }
          },
          "description": "Layout map for primitive types (i32, usize, etc)."
        }
      }
    },
    
    "implementation_limits": {
      "type": "object",
      "description": "Actual limits enforced by this implementation.",
      "required": ["max_recursion_depth", "max_identifier_length"],
      "properties": {
        "max_recursion_depth": { "type": "integer" },
        "max_identifier_length": { "type": "integer" },
        "max_source_size": { "type": "integer" }
      }
    }
  }
}
```

### Appendix D: Standard Trait Catalog {Source: Draft 2 Appx D}

This appendix provides normative definitions for foundational traits and system capability traits built into Cursive or its core library.

  <u>Definition</u>
    Normative definitions for foundational traits and system capability traits that are deeply integrated with language mechanics.
  
  <u>Syntax & Declaration</u>
    **Foundational Traits** (§D.1):
    *   `Drop`: `procedure drop(~!)` - RAII cleanup, compiler-invoked only
    *   `Copy`: Marker trait for implicit bitwise duplication
    *   `Clone`: `procedure clone(~): Self` - explicit deep copy
    *   `Sync`: Marker trait (`public trait Sync {}`)
        -   **Semantics**: Safe to access concurrently (`concurrent` permission).
        -   **Auto-Derive**:
            -   Primitives (`i32`, `bool`) are `Sync`.
            -   Records/Enums are `Sync` if ALL fields are `Sync`.
            -   `Mutex<T>`, `Atomic<T>` are explicitly `Sync`.
            -   `Cell<T>` (interior mutability without locks) is `!Sync`.
    
    **System Capability Traits** (§D.2):
    *   `FileSystem`:
        -   `open(path: string@View, mode: FileMode): FileHandle | IoError`
        -   `exists(path: string@View): bool`
        -   `restrict(path: string@View): witness FileSystem` (Attenuation)
    *   `Network`:
        -   `connect(addr: NetAddr): Stream | NetError`
        -   `bind(addr: NetAddr): Listener | NetError`
        -   `restrict_to_host(addr: NetAddr): witness Network` (Attenuation)
    *   `HeapAllocator`:
        -   `alloc<T>(count: usize): *mut T`
        -   `dealloc<T>(ptr: *mut T, count: usize)`
        -   `with_quota(size: usize): witness HeapAllocator` (Attenuation)
    *   `System`:
        -   `exit(code: i32): !`
        -   `get_env(key: string@View): string`
        -   `spawn<T>(closure: () -> T): Thread<T>@Spawned` (Concurrency Path 2)
        -   `create_mutex<T>(value: T): Mutex<T>@Unlocked` (Concurrency Path 2)
        -   `time(): Timestamp`
    *   `Time`: Monotonic time access with `now(): Timestamp` procedure
  
  <u>Constraints & Legality</u>
    *   `Drop::drop` MUST NOT be called directly by user code (E-TRS-2920)
    *   `Copy` and `Drop` are mutually exclusive on same type (E-TRS-2921)
    *   `Copy` requires all fields implement `Copy` (E-TRS-2922)
    *   `HeapAllocator::alloc` MUST panic on OOM (never return null)
    *   Variadic implementations across all trait types are prohibited
  
  <u>Static Semantics</u>
    *   Compiler automatically inserts `Drop::drop` calls at scope exit for responsible bindings
    *   `Copy` types are duplicated (not moved) on assignment/parameter passing
    *   Static invalidation applies to aliases when Drop is invoked on owner
    *   Capability traits enable attenuation patterns (e.g., `with_quota` on HeapAllocator)
  
  <u>Dynamic Semantics</u>
    *   Drop execution grants temporary exclusive (unique) access to self
    *   Drop during panic unwind: second panic causes process abort
    *   HeapAllocator failures cause panic (no null returns)
  
  <u>Examples</u>
    Complete trait signatures must be provided for: Drop, Copy, Clone, HeapAllocator, FileSystem, Network, Time, System (which implements Time and provides `exit`, `get_env`).

### Appendix E: Core Library Specification {Source: Draft 2 Appx E}

  <u>Definition</u>
    Minimal normative definitions for core types assumed to be available by the language without explicit import.
  
  <u>Syntax & Declaration</u>
    **Context Capability Record**:
    *   `Context` record structure (see §30.1, §30.2):
        - `fs: witness FileSystem`
        - `net: witness Network`
        - `sys: System` (implements Time)
        - `heap: witness HeapAllocator`

    *(Note: `Option` and `Result` are removed. Optionality is handled via Union Types e.g., `T | ()`, and failure via `T | Error`. Pointers handle nullability via `Ptr<T>@Null` states).*
  
  <u>Constraints & Legality</u>
    *   `Context` field types MUST match the capability trait witnesses defined in Appendix D
    *   Core library types (`Option`, `Result`, `Context`, primitives) MUST be available in the universe scope (no import required)
    *   `main` procedure MUST accept `Context` parameter as defined
  
  <u>Static Semantics</u>
    *   All context capabilities are passed explicitly (no ambient authority)
  


### Appendix F: Implementation Guide (Informative) {Source: Draft 2 Appx G}

**Note**: This appendix provides non-normative guidance for compiler implementers.

#### F.1 Control Flow Graph (CFG) for Verification

  <u>Static Semantics</u>
    Guidance for implementing Fact Injection (§27.7, Clause 7.2) and Partitioning verification (§29.4, Clause 3.4):
    *   **Dominator Trees**: Use Lengauer-Tarjan algorithm to build dominator tree
    *   **Verification Facts**: Facts are valid only if origin node strictly dominates consumption node
    *   **Loop Headers**: Synthesize entry facts for loop induction variables at loop header node
    *   **Example**: For loop `i in 0..N`, inject facts `i >= 0` and `i < N` dominating loop body
  
#### F.2 Niche Optimization for Modal Types

  <u>Memory & Layout</u>
    Strongly encouraged optimization to reduce modal type size:
    *   Use invalid bit-patterns (niches) in state payloads to encode discriminant
    *   **Example**: `Ptr<T>` uses address `0x0` for `@Null` state, non-zero for `@Valid` state
    *   **Result**: `Ptr<T>` general type occupies same space as raw pointer (64 bits, not 128)
    *   **General Algorithm**: Identify niches in payload types, map empty states to niche patterns
  
#### F.3 Canonical Formatting

  <u>Definition</u>
    Recommended formatting to satisfy syntactic stability goals (§4.8, Clause 1.6):
    *   **Indentation**: 4 spaces (no tabs)
    *   **Brace Style**: 1TBS (One True Brace Style) - opening brace on same line as declaration
    *   **Import Ordering**: Alphabetically sorted
    *   **Declaration Ordering**: public declarations first, then internal, then private within each category

### Appendix G: Behavior Classification Index (Normative) {Source: Draft 2 Appx H}
**H.1 Unverifiable Behavior (UVB)**:
*   FFI Call (§32.2)
*   Raw Deref (§29.6.2)
*   Transmute (§29.6.3)
*   Pointer Arithmetic (§24.4.2)
*   Trusted Contracts (§27.6.3)
*   Trusted Partitions (`[[verify(trusted)]]`)

**H.2 Implementation-Defined Behavior (IDB)**:
*   Type Layout (non-C)
*   Integer Overflow (Release)
*   Pointer Width
*   Resource Limits
*   Panic Abort Mechanism

### H.3 Unspecified Behavior (USB) {Source: Draft 2 Appx H.3}

The following behaviors are bounded by the language safety guarantees but are not documented or consistent between executions.

*   **Map Iteration**: Order of iteration for hash-based collections.
*   **Padding Bytes**: The values of padding bytes in non-`[[repr(C)]]` records.

### H.4 Defined Runtime Failure (Panics)

*   Integer Overflow (Checked Mode)
*   Array/Slice Bounds Check
*   **Dynamic Partition Overlap** (`[[verify(dynamic)]]`)

### Appendix H: Formal Core Semantics (Normative) {Source: Draft 2 Appx I}

This appendix defines the **Cursive Core Calculus**, a simplified formal model of the language's memory and permission system. Implementations **MUST** preserve the safety properties defined here.

#### H.1 Syntax of the Core

$$\begin{aligned} v &::= \ell \mid \text{const } \ell \mid \text{null} \\ e &::= v \mid \text{let } x = e \text{ in } e \mid x \mid x.f \mid x.f \leftarrow v \mid \text{fork}(e) \\ \tau &::= \text{const } T \mid \text{unique } T \mid \text{partitioned } T \end{aligned}$$

#### H.2 Operational Semantics (Small-Step)

State is defined as a pair $(H, e)$ where $H$ is the heap mapping locations $\ell$ to values.

**Read Rule:**

$$\frac{H(\ell) = v}{(H, \ell.f) \longrightarrow (H, v.f)}$$  
**Write Rule (Unique):**

$$\frac{H(\ell) \text{ is live}}{(H, \ell.f \leftarrow v') \longrightarrow (H[\ell.f \mapsto v'], ())}$$

#### H.3 Safety Theorems

##### Theorem 1: Progress
If $\vdash e : \tau$ and $e$ is not a value, then there exists a state $(H', e')$ such that $(H, e) \longrightarrow (H', e')$.  
Implication: A well-typed Cursive program never gets "stuck" (segfaults or undefined behavior).

##### Theorem 2: Preservation (Type Safety)
If $\vdash e : \tau$ and $(H, e) \longrightarrow (H', e')$, then $\vdash e' : \tau$.  
Implication: Operations never violate the permission rules defined in Part 4\.  

##### Theorem 3: Data Race Freedom
If $\Gamma \vdash e : \text{well-formed}$, and $e$ contains fork, no two threads can access location $\ell$ simultaneously unless both accesses are Reads.  
_Proof Sketch:_ The unique permission ($\ell$) cannot be duplicated. The const permission ($\text{const } \ell$) allows duplication but removes the Write Rule from the set of valid reductions for that value.

### Appendix I: Implementation Limits {Source: Draft 2 §6.5, Appx F}
**Minimum Guaranteed Limits**:
*   **Source Size**: 1 MiB
*   **Logical Lines**: 65,535
*   **Line Length**: 16,384 chars
*   **Nesting Depth**: 256
*   **Identifier Length**: 1,023 chars
*   **Parameters**: 255
*   **Fields**: 1,024
*   **Recursion Depth**: 256 (Comptime), Implementation-Defined (Runtime)
