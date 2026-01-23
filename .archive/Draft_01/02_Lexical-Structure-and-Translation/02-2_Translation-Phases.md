# The Cursive Language Specification

## Clause 2 — Lexical Structure and Translation

**Part**: II — Lexical Structure and Translation
**File**: 02-2_Translation-Phases.md  
**Section**: §2.2 Translation Phases  
**Stable label**: [lex.phases]  
**Forward references**: §2.1 [lex.source], §2.3 [lex.tokens], Annex E §E.1 [implementation.phases]

---

### §2.2 Translation Phases [lex.phases]

#### §2.2.1 Overview

[1] Cursive compilation proceeds through a deterministic pipeline of translation phases that convert validated UTF-8 source (§2.1 [lex.source]) into executable artifacts. The phases are designed to expose explicit intermediate products so that diagnostics, tooling, and contracts can observe well-defined boundaries.

[2] A _compilation unit_ is the textual content of a single source file after preprocessing (§2.1). Clause 4 derives module identity from these units; imports create dependencies between units but do not merge them. A program is _well-formed_ precisely when every compilation unit completes each translation phase without producing a fatal diagnostic.

[3] Cursive deliberately omits textual preprocessing: the pipeline begins immediately with lexical analysis (§2.3 [lex.tokens]), ensuring that all transformations are performed with full syntactic and semantic context rather than token substitution.

[4] The pipeline follows a **two-phase compilation model**: the parsing phase records the complete set of declarations and their structural metadata without enforcing semantic requirements, and the semantic phases (compile-time execution, code generation, type checking, and lowering) run only after parsing succeeds. This model guarantees that declarations may appear in any order within a compilation unit—forward references are resolved during the semantic phases rather than by separate stub declarations.

### §2.2.2 Syntax

[1] The sequencing of phases is described by the following grammar; the symbols correspond to the sections in §2.2.4.

```ebnf
phase_pipeline
    ::= parsing
     "→" comptime_execution
     "→" type_checking
     "→" code_generation
     "→" lowering
```

[2] Implementations may internally refactor work, but they shall present the externally observable phase boundaries in the order above. Type checking therefore gates code generation: no target code is produced until the program has passed parsing, compile-time execution, and type checking.

### §2.2.3 Constraints

[1] _Phase ordering._ Implementations shall execute phases strictly in the order defined by `phase_pipeline`. A phase that emits a diagnostic with severity _error_ shall terminate the pipeline for the affected compilation unit; later phases shall not observe partially processed artefacts.

(1.1) _Parsing boundary._ Parsing shall complete (successfully or with diagnostics) **before** compile-time execution begins. Subsequent phases are prohibited from mutating the parse tree in ways that would invalidate the recorded declaration inventory; they may annotate the tree with semantic metadata only.

(1.2) _Type-checking gate._ Type checking shall complete (successfully or with diagnostics) **before** code generation begins. Generated code is therefore guaranteed to correspond to a well-typed program that satisfies all permission, grant, and contract requirements available at that point in the pipeline.

[2] _Determinism._ The observable results of a phase (diagnostics, generated declarations, lowered IR) shall be deterministic with respect to the input compilation unit and compilation configuration.

[3] _Comptime evaluation limits._ The compile-time execution phase shall enforce resource limits at least as permissive as the following minima:

| Resource               | Minimum                     | Diagnostic |
| ---------------------- | --------------------------- | ---------- |
| Recursion depth        | 256 frames                  | E02-101    |
| Evaluation steps       | 1,000,000 per block         | E02-102    |
| Memory allocation      | 64 MiB per compilation unit | E02-103    |
| String size            | 1 MiB                       | E02-104    |
| Collection cardinality | 10,000 elements             | E02-105    |

[4] _Grant safety._ Comptime execution shall refuse any grant that is not listed in the grants clause of the executing item's contractual sequent. Runtime-only capabilities (e.g., file system or network grants as will be cataloged in §12.3 [contract.grant]) are forbidden and shall raise diagnostic E02-106. Comptime blocks declare their capability requirements explicitly through the same sequent mechanism used by procedures; complete grant semantics are specified in Clause 12 [contract].

[5] _Generated symbol hygiene._ Code generation shall ensure that generated declarations do not collide with declarations already present in the AST. Name collisions shall be diagnosed as E02-107.

### §2.2.4 Semantics

#### §2.2.4.1 Parsing

[1] The parsing phase consumes the normalized token stream (§2.3 [lex.tokens]) and constructs an abstract syntax tree (AST) that preserves source spans needed for diagnostics and tooling.

[2] Parsing records every declaration, its identifier, structural metadata (e.g., parameter lists, field declarations), and source scope. The AST produced at this stage is the authoritative inventory of declarations for the compilation unit.

[3] Parsing shall not attempt semantic validation. All name lookup, type checking, permission/grant checks, and diagnostic enforcement occur during the semantic-analysis phase (§2.2.4.2–§2.2.4.4).

[4] Forward references are therefore permitted: declarations may appear after their uses in the source, and mutual recursion is resolved by the later semantic phases.

#### §2.2.4.2 Compile-Time Execution

[1] The comptime execution phase evaluates `comptime` blocks in dependency order. Implementations build a dependency graph whose nodes represent comptime blocks and whose edges reflect value or type dependencies discovered during parsing. The graph is constructed only after the parser has completed the compilation unit, ensuring all referenced declarations are known.

[2] Before executing a block, the compiler verifies that all predecessors in the dependency graph have succeeded. Cycles are ill-formed and shall be reported as diagnostic E02-100.

[3] Comptime code executes with access only to explicitly-granted capabilities (`comptime.alloc`, `comptime.codegen`, `comptime.config`, `comptime.diag`). All other grants are rejected per constraint [4].

#### §2.2.4.3 Type Checking

[1] The type-checking phase resolves names, validates type constraints, enforces grant clauses, and checks contracts. Because parsing has already recorded every declaration, name resolution always observes a complete declaration inventory.

(1.1) [ Note: Module scope formation (§4.3 [module.scope]) occurs during this phase, before name lookup. Wildcard imports (`use module::*`) expand at scope-formation time, after parsing has completed for all modules in the dependency graph. This ensures that the set of exported items is stable before expansion proceeds. — end note ]

[2] Permission checks, modal verification, and contract evaluation are delegated to their respective clauses (memory model, modals, contracts) but are orchestrated from this phase to guarantee that only semantically sound programs proceed to code generation and lowering.

#### §2.2.4.4 Code Generation

[1] Code generation materialises declarations requested by comptime execution (e.g., via `codegen::declare_procedure`) and produces backend-specific artefacts only after type checking succeeds. Generated declarations are appended to the AST and annotated so that diagnostics can reference both the generated item and the originating comptime site.

[2] Hygiene utilities such as `gensym` shall be used to avoid collisions; generated code is subject to the same visibility and validation rules as user-authored code.

#### §2.2.4.5 Lowering

[1] Lowering transforms the fully-validated AST into an intermediate representation (IR) suitable for target-specific code generation. Comptime constructs are eliminated, generic constructs are monomorphised, and deterministic cleanup paths (RAII) are made explicit.

[2] Implementations may perform optimisation-friendly rewrites during lowering provided the externally observable behaviour remains unchanged.

### §2.2.5 Examples

**Example 2.2.5.1 (Phase Interaction):**

```cursive
// Phase 1: Parse comptime block and procedure skeleton
comptime {
    let size = config::get_usize("buffer_size", default: 4096)
    codegen::declare_constant(
        name: "BUFFER_SIZE",
        ty: codegen::type_named("usize"),
        value: quote { #(size) }
    )
}

public procedure create_buffer(): Buffer
    [[ |- true => true ]]
{
    result Buffer { data: make_zeroes(len: BUFFER_SIZE) }
}
```

[1] Parsing records a comptime block and an incomplete procedure. During compile-time execution the configuration query runs, contributing a constant. Code generation publishes the constant via `declare_constant`; type checking resolves `BUFFER_SIZE` and validates the call to `make_zeroes`; lowering erases the comptime block and emits IR containing the numeric literal produced at compile time.

### §2.2.6 Conformance Requirements

[1] Implementations shall provide diagnostics cited in this clause and shall guarantee that fatal diagnostics terminate the compilation of the affected compilation unit.

[2] Implementations shall make the outputs of each phase observable to tools (for example through logs or compiler APIs) so that external tooling can integrate with the pipeline.

[3] Implementations may parallelise translation phases across independent compilation units, but the phases for any single compilation unit shall respect the deterministic pipeline defined in this clause.
