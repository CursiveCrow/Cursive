# Cursive Language Specification

## Clause 7 — Expressions

**Clause**: 7 — Expressions  
**File**: 08-8_Typing-and-Diagnostics.md  
**Section**: §7.8 Expression Typing Rules and Ill-formed Cases  
**Stable label**: [expr.typing]  
**Forward references**: Clause 5 §5.7 [decl.initialization], Clause 6 §6.4 [name.lookup], Clause 7 §7.7 [type.relation], Clause 11 [memory], Clause 12 [contract], Annex E §E.5 [implementation.diagnostics]

---

### §7.8 Expression Typing Rules and Ill-formed Cases [expr.typing]

#### §7.8.1 Overview

[1] This subclause consolidates the global typing judgment (`Γ ⊢ e : τ ! ε [cat]`) introduced in §8.1, clarifies how it interacts with definite assignment, permissions, and grants, and catalogues common ill-formed expressions together with their diagnostics and required payloads. It also documents the tooling obligations that make expression metadata available to IDEs and analysis tools.

#### §7.8.2 Typing judgment recap

[2] Every expression typing derivation has the form `Γ ⊢ e : τ ! ε [cat]`, where:

- `Γ` stores bindings, region stack, grant budget, and contextual type expectations.
- `τ` is the expression’s type after alias expansion (§7.7).
- `ε` is the cumulative grant set (§8.1.5).
- `[cat] ∈ {value, place, divergent}` is the category (§8.1.4).

[3] The following invariants shall hold simultaneously:

- **Name resolution:** identifiers resolve using Clause 6 §6.4. Names bound to types/modules/contracts cannot appear in expression position (E08-211).
- **Definite assignment:** Clause 5 §5.7 ensures bindings are initialised before use. Violations emit E08-210 at the expression site.
- **Permission safety:** assignments and moves operate only on places with sufficient permission (`mut` or `own`). Violations emit E11-403.
- **Grant coverage:** the enclosing procedure's grants clause (within its contractual sequent) covers ε; missing grants produce E08-004.
- **Type compatibility:** operands satisfy the subtyping/compatibility rules in Clause 7. Failures emit E08-800 with both types and the violated rule.

#### §7.8.3 Ill-formed expression catalogue

[4] Table 8.8.1 lists representative ill-formed constructs. Each diagnostic references Annex E §E.5 for payload requirements.

| ID    | Description                                           | Diagnostic |
| ----- | ----------------------------------------------------- | ---------- |
| IF-01 | Identifier resolves to non-value entity               | E08-211    |
| IF-02 | Type inference failed and no context provided         | E08-002    |
| IF-03 | Place used without required permission                | E11-403    |
| IF-04 | Runtime-only construct used in const/comptime context | E08-700    |
| IF-05 | Match result type inference failed                    | E08-450    |
| IF-06 | Pipeline stage omitted type while changing type       | E08-020    |
| IF-07 | Array/slice index out of range                        | E08-250    |
| IF-08 | Assignment target is not a place                      | E08-340    |
| IF-09 | Match is non-exhaustive                               | E08-451    |
| IF-10 | Cast outside allowed categories                       | E08-601    |

#### §7.8.4 Drop semantics and temporaries

[5] Temporaries created during expression evaluation are destroyed at scope exit in reverse creation order (Clause 12 §12.2). When an expression spans multiple branches (`if`, `match`), each branch is analysed independently for drop order. Developers must not rely on unspecified destruction timing across branches; the specification guarantees only that every temporary drops exactly once.

#### §7.8.5 Tooling integration

[6] Conforming implementations shall expose expression metadata (type, grant set, category, evaluation order index) via the tooling interface in Annex E §E.3. Diagnostics shall embed this metadata in their JSON payloads when it aids remediation—for example, E11-403 specifies the binding name and required permission, while E08-004 includes both required and supplied grant sets.

#### §7.8.6 Example diagnostic payload

```json
{
  "code": "E11-403",
  "message": "assignment requires mutable place",
  "expr_range": "src/lib.rs:42:9-42:22",
  "binding": "total",
  "required_permission": "mut",
  "observed_binding_permission": "let",
  "suggested_fixes": [
    {
      "description": "declare binding as 'var total'",
      "edits": [
        /* implementation-defined */
      ]
    }
  ]
}
```

[7] Annex E §E.5 mandates that every diagnostic payload identify the offending source span, the relevant contextual metadata, and any fix-it hints. This ensures IDEs and automated tooling can present actionable feedback consistent with the deterministic semantics defined in Clause 8.

---
