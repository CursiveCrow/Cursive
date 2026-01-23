# The Cursive Language Specification

## Clause 2 — Lexical Structure and Translation

**Part**: II — Lexical Structure and Translation  
**File**: 02-5_Compilation-Units-and-Top-Level-Forms.md  
**Section**: §2.5 Compilation Units and Top-Level Forms  
**Stable label**: [lex.units]  
**Forward references**: Clause 4 [module], Clause 5 [decl]

---

### §2.5 Compilation Units and Top-Level Forms [lex.units]

#### §2.5.1 Overview

[1] A compilation unit is the syntactic content of a single source file after preprocessing (§2.1 [lex.source]). Clause 2 governs the translation of those files; Clause 4 derives module paths from the file system layout. This clause enumerates the declarations permitted at module scope and describes module-level initialisation.

### §2.5.2 Syntax

[1] Top-level structure is constrained by the following grammar, with non-terminals defined in later clauses:

```ebnf
compilation_unit
    ::= top_level_item*

top_level_item
    ::= import_declaration
     | use_declaration
     | variable_declaration
     | procedure_declaration
     | type_declaration
     | predicate_declaration
     | contract_declaration
     | grant_declaration
```

[2] Expression statements, control-flow constructs (`if`, `match`, `loop`, etc.), and local bindings are not permitted at module scope.

[Note: Constants are expressed via type qualifiers on variable declarations (`let x: const Y = 0`) rather than separate declaration forms. Grant declarations (§5.9 [decl.grant]) introduce user-defined capability tokens for use in procedure contractual sequent specifications. — end note]

### §2.5.3 Constraints

[1] _Visibility._ Declarations without an explicit visibility modifier default to `internal`. The modifiers `public`, `internal`, `private`, and `protected` control accessibility across modules and packages; see Clause 4 [module] for re-exporting behaviour.

[2] _Uniqueness._ The names introduced by top-level items shall be unique within the compilation unit. Redeclaration without explicit `shadow` is diagnosed as E02-400.

[3] _Forward references._ Forward references to declarations in the same compilation unit are permitted because the parser records declarations before type checking (§2.2 [lex.phases]).

[4] _Disallowed forms._ Expression statements, control-flow constructs, and block scopes at module level shall raise diagnostic E02-301.

[5] _Empty compilation units._ A compilation unit may contain zero top-level items. An empty compilation unit defines a valid module with no exports. Such modules may be imported for their side effects (e.g., module-level initialisation) but contribute no bindings to the importing module's namespace.

[6] _Whitespace and comments only._ A compilation unit containing only whitespace, line comments, block comments, or documentation comments (with no top-level items) is treated as an empty compilation unit per constraint [5]. No diagnostic is emitted for such files.

### §2.5.4 Semantics

#### §2.5.4.1 Module-Level Initialisation

[1] Module-level `let` and `var` bindings are initialised before the program entry point executes. Implementations construct a dependency graph between initialisers and evaluate bindings in topological order. Cycles are rejected with diagnostic E02-401.

[2] Initialisers execute exactly once per program instance. Mutable `var` bindings retain their state for the life of the program unless explicitly mutated or reset by user code.

#### §2.5.4.2 Entry Point

[1] Executable programs shall provide exactly one procedure named `main`. The canonical forms and required diagnostics are specified in §5.8 [decl.entry], which owns entry-point validation (codes E05-801–E05-804).

### §2.5.5 Examples

**Example 2.5.5.1 (Valid Compilation Unit):**

```cursive
use std::io::println

public record Point { x: f64, y: f64 }

let ORIGIN: Point = Point { x: 0.0, y: 0.0 }

public procedure distance(a: Point, b: Point): f64
    [[ |- true => true ]]
{
    let dx = b.x - a.x
    let dy = b.y - a.y
    result (dx * dx + dy * dy).sqrt()
}
```

[1] The example contains only permitted top-level items and relies on the implicit initialisation rules of §2.5.4.1.

**Example 2.5.5.2 - invalid (Expression Statement at Module Scope):**

```cursive
let value = 5
value + 1  // error[E02-301]
```

### §2.5.6 Conformance Requirements

[1] Implementations shall expose APIs or metadata that identify the module path corresponding to each compilation unit so tooling can correlate files with module identifiers.

[2] Diagnostically, implementations shall detect redeclarations (E02-400), prohibited constructs (E02-301), and entry-point violations (E05-801–E05-804, see §5.8 [decl.entry]) during or before type checking.

[3] Implementations may lazily evaluate module-level initialisers provided that observable behaviour matches the eager semantics described in §2.5.4.1.

---

**Previous**: §2.4 Tokenization and Statement Termination (§2.4 [lex.terminators]) | **Next**: Clause 3 — Modules (§3.1 [module.overview])
