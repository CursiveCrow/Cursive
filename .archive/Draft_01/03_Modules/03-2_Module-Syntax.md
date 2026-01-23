# Cursive Language Specification

## Clause 3 — Modules

**Clause**: 3 — Modules
**File**: 04-2_Module-Syntax.md
**Section**: §3.2 Module Syntax
**Stable label**: [module.syntax]  
**Forward references**: §2.5 [lex.units], Clause 4 [decl], Clause 5 [name], Annex A §A.7 [grammar.declaration], Annex E §E.2.1 [implementation.algorithms]

---

### §3.2 Module Syntax [module.syntax]

#### §3.2.1 Overview

[1] Module syntax encompasses the declarations that import other modules, bring exported items into scope, and re-export items for downstream consumers.

[2] This subclause provides the normative grammar; semantics for scope formation, visibility, and diagnostics appear in §§3.3–3.5.

#### §3.2.2 Syntax

[1] Module-level items extend §2.5's grammar as follows (authoritative production appears in Annex A §A.6):

**Import declarations** match the pattern:
```
"import" <module_path> [ "as" <identifier> ]
```

**Use declarations** match the pattern:
```
[ "public" ] "use" <use_clause>
```

**Use clauses** take one of the following forms:
```
<qualified_path>
<qualified_path> "as" <identifier>
<qualified_path> "::" "{" <use_list> "}"
<qualified_path> "::" "*"
```

**Qualified paths** match the pattern:
```
<module_path>
<module_path> "::" <identifier>
```

**Use lists** match the pattern:
```
<use_specifier> [ "," <use_specifier> [ "," <use_specifier> ... ] ]
```

**Use specifiers** take one of the following forms:
```
<identifier>
<identifier> "as" <identifier>
```

[ Note: The normative grammar production `use_decl` in Annex A §A.6 unifies import and use declarations. Statement termination follows §2.4 [lex.units] rules.
— end note ]

[2] `visibility_modifier` is limited to `public` for re-export intent; other visibility keywords belong to Clause 4.

[3] Statement termination interacts with implicit termination rules in §2.4; a newline suffices unless a continuation predicate applies.

#### §3.2.3 Constraints

[1] The `module_path` in any `import` or `use` shall resolve under §3.1.3; missing modules trigger diagnostics defined in §3.6.

[2] `public use` marks the clause as re-exporting the referenced items; its semantics are covered in §3.4.

[3] Aliases introduced with `as` must be valid identifiers (§2.3) and shall not collide with existing bindings. An import alias binds the module path to the alias for subsequent qualified references.

[4] Within a `use_list`, each specifier shall be unique; duplicates provoke diagnostic E03-100.

[5] Wildcard clauses (`:: *`) shall be expanded during scope formation (§3.3); conflicts at expansion time emit diagnostic E03-101.

[6] `qualified_path :: identifier` forms are resolved according to §3.5; ill-formed targets (missing or non-public items) are diagnosed in §3.4. A `qualified_path` whose head component is an import alias refers to the aliased module.

[7] `visibility_modifier` shall not appear on `import` declarations; attempts to do so provoke diagnostic E03-102.

#### §3.2.4 Semantics

[1] `import` declarations bring a module’s namespace into scope for qualified references. When an alias is provided, qualified references shall use the alias as the head component; otherwise the full module path is required. Imports establish the prerequisite for later `use` declarations (§4.3).

[2] `use` declarations introduce bindings into the current module scope. When marked `public`, those bindings are re-exported from the current module.

[3] The resulting bindings participate in scope formation rules (§4.3) and qualified-name resolution (§4.5).

[4] Wildcards expand to the set of public items exported by the target module at the time of analysis; the expansion respects visibility modifiers and re-export status described in §4.4.

#### §3.2.5 Examples

**Example 4.2.5.1 (Imports and uses):**

```cursive
import math::geometry          // qualified access: math::geometry::area
import math::statistics as statistics
use math::geometry::area_of_circle
public use math::geometry::{circumference as circle_circumference}
use statistics::*
```

[1] The module imports `math::geometry` for qualified access, adds the alias `statistics` for `math::statistics`, pulls `area_of_circle` unqualified, re-exports `circumference` under a new alias, and wildcard-imports all public items exposed by `statistics`.

**Example 4.2.5.2 - invalid (Duplicate specifier):**

```cursive
use math::geometry::{area_of_circle, area_of_circle}  // error[E03-100]
```

[2] Duplicate entries in the use list violate constraint [4].

**Example 4.2.5.3 - invalid (`public` on import):**

```cursive
public import math::geometry  // error[E03-102]
```

[3] Visibility modifiers on `import` are not permitted.

---
