# Spec Revision Proposal — Remove `shadow`; `using` as Universal Aliasing

**Status:** APPROVED — decisions resolved; ready to edit `CursiveSpecification.md`
**Date:** 2026-04-15
**Scope:** Language surface + name-resolution spec + compiler + tests

---

## 1. Summary

Two coupled changes to the language:

1. **Remove the `shadow` keyword** and all `shadow`-related grammar, rules, AST nodes, and diagnostics. Name reuse across scopes becomes a hard error; users must pick distinct names.
2. **Extend `using`** from module-level only to any scope, as the single, universal aliasing keyword. `using x as y` creates a compile-time rename from `y` to the existing binding, type, class, or module referenced by `x`.

The two changes compose: there is no longer any mechanism for a second binding to occupy a name already in an outer scope; the only way to introduce an alternate name for an existing binding is `using ... as ...`, and `using` does not introduce a new binding — it renames.

---

## 2. Rationale

### Why pure-rename semantics for `using`

The spec already characterizes module-level `using` as a compile-time-only construct with no runtime lowering (§11.2, lines 7403 and 7407) and describes `using`/`import` aliases as "hygienic binders" (§24, line 24814). This is exactly pure-rename semantics: the alias is a second name for the same compile-time entity. No storage is allocated; no runtime indirection is introduced.

Extending `using` to local scope should preserve this property. A local `using x as y` binds `y` as a second name for the same storage cell (for bindings) or the same compile-time entity (for types, classes, modules). Both names are interchangeable. No runtime alias, no reference, no copy.

This choice over the two rejected alternatives:

- **Reference semantics** would require a new runtime mechanism (an address-backed alias), expanding the runtime model. Cursive's runtime scope model (§4.6.1, `BindingValue = Value ∪ {Alias(addr)}`) already has an alias form, but that is for internal compiler use (e.g., cross-scope captures), not a user-facing reference type. Cursive is not a reference/borrow language.
- **Value-snapshot semantics** is simply `let y = x` in disguise — it is a new binding with a new storage cell, not aliasing. Using the `using` keyword for this would mislead.

Pure rename is the most consistent extension: it matches module-level `using`, introduces no new runtime mechanism, and preserves the language's "no ambient authority, no implicit references" character.

### Why remove `shadow` entirely

`shadow` exists today as an explicit escape hatch for reusing an outer-scope name (`shadow let x = ...`). Its purpose is to make shadowing syntactically visible. Removing it has two consequences:

- **Name reuse across scopes becomes impossible.** Users must choose distinct names. This is a strictness choice consistent with Cursive's existing strictness elsewhere (explicit visibility, explicit regions, explicit capabilities).
- **The aliasing story is unified.** With `shadow` gone and `using` universalized, there is exactly one way to introduce an alternate name for any entity: `using`. No grammar branch, no separate rule family, no separate diagnostic set.

The user cost: patterns like `let x = compute(); let x = refine(x);` (common in Rust) must be rewritten, e.g., `let raw = compute(); let x = refine(raw);`. This is a source-incompatible change, but the language is early and the user has confirmed the direction.

---

## 3. Recommended Semantics — Local `using x as y`

Proposed to add in §7 (Name Resolution), adjacent to existing Intro rules:

```
UsingAlias = ⟨source_name, alias_name⟩

(Using-Alias-Ok)
¬ InScope(S_cur, alias_name)    ¬ InOuter(Γ, alias_name)    ¬ ReservedId(alias_name)
Γ ⊢ Lookup(source_name) ⇓ ent
Scopes(Γ') = [S_cur[IdKey(alias_name) ↦ ent]] ++ Γ_out
──────────────────────────────────────────────────────────────────────────────
Γ ⊢ UsingAlias(source_name, alias_name) ⇓ Γ'

(Using-Alias-Dup)
InScope(S_cur, alias_name) ∨ InOuter(Γ, alias_name)
c = Code(Using-Alias-Dup)
──────────────────────────────────────────────────────────────────────────────
Γ ⊢ UsingAlias(source_name, alias_name) ⇑ c

(Using-Alias-Unresolved)
Γ ⊢ Lookup(source_name) ↑
c = Code(Using-Alias-Unresolved)
──────────────────────────────────────────────────────────────────────────────
Γ ⊢ UsingAlias(source_name, alias_name) ⇑ c

(Using-Alias-Reserved)
ReservedId(alias_name)    c = Code(Using-Alias-Reserved)
──────────────────────────────────────────────────────────────────────────────
Γ ⊢ UsingAlias(source_name, alias_name) ⇑ c
```

**Key properties:**
- The `Entity` stored under `alias_name` is **the same Entity** that `source_name` resolves to. `ent.source` remains whatever it was (Decl, Import, Using, RegionAlias).
- Aliasing an alias is just another hop: `using y as z` after `using x as y` stores the same underlying Entity under `z`.
- `using` does not introduce storage. `FieldVis`, `ValueKind`, `TypeKind`, etc. all see through to the original Entity.
- `using` at any scope is compile-time only (existing §11.2 property preserved).

---

## 4. Spec Sections Requiring Edits

### 4.1 Grammar changes

| Line | Section | Change |
|------|---------|--------|
| 2076 | §4.2.3 Reserved Lexemes | Remove `shadow` from the reserved keyword list |
| 7271-7276 | §11.2.1 Using Declarations — Syntax | Add a fourth clause form for local alias: `using_local ::= "using" identifier "as" identifier terminator`. Note: this is a *statement* form when used locally, not an item. |
| 18943 | §18.3.1 Shadowing Statements — Syntax | **Delete the entire `shadow_binding` grammar production.** |
| 29886-29894 | Appendix B | Remove `shadow_binding` from the statement alternatives; add `using_local` to statement alternatives. |

### 4.2 Add a new statement form

Insert into §18 (Statements) alongside `let` and `var`:

```
using_local_stmt ::= "using" identifier "as" identifier terminator
UsingLocalStmt = ⟨source, alias, span⟩
```

Typing/lowering:
- Typing: evaluates `UsingAlias(source, alias)` (rule above). No type is introduced; the environment is extended.
- Lowering: emits no IR. The alias is a symbol-table entry only. (Consistent with §11.2's statement: "`using` introduces no construct-specific lowering.")

### 4.3 Delete shadow rules (§7.2)

| Line | Rule | Action |
|------|------|--------|
| 4572-4575 | **(Intro-Shadow-Required)** | **Delete.** Replaced by a single **Intro-Outer-Err** rule that makes outer-scope name reuse a terminal error. |
| 4577-4580 | **(Shadow-Ok)** | **Delete.** |
| 4582-4585 | **(Shadow-Unnecessary)** | **Delete.** |
| 4597-4600 | **(Shadow-Reserved-Gen-Err)** | **Delete.** ReservedId check already covered by **Intro-Reserved-Id-Err**. |
| 4602-4605 | **(Shadow-Reserved-Cursive-Err)** | **Delete.** Same reason. |

Replace with a single rule:

```
(Intro-Outer-Err)
¬ InScope(S_cur, x)    InOuter(Γ, x)    c = Code(Intro-Outer-Err)
──────────────────────────────────────────────────────────────────
Γ ⊢ Intro(x, ent) ⇑ c
```

Rename the existing `(Intro-Ok)` premise: drop `(S_cur ≠ S_module ∨ x ∉ UniverseProtected)` if this was shadow-specific (check in detail during edit).

### 4.4 Delete shadow rules in §18.3

Lines 18938-19054 — entire subsection §18.3 "Shadowing Statements" — **delete.** (parse rules, AST form, typing, lowering, the duplicate copies of Intro-Shadow-Required/Shadow-Ok/Shadow-Unnecessary/Shadow-Reserved-*)

### 4.5 Delete shadow helper functions

| Line | Symbol | Action |
|------|--------|--------|
| 3647 | `ShadowIntro_B(𝔅, x, info) = Intro_B(𝔅, x, info)` | Delete; callers should use `Intro_B` directly. |
| 4057, 4059 | `ShadowIntro_π(Σ_π, x, π)` | Delete. Review callers in module-chain handling. |
| 4065-4066 | `ShadowAll_π(Σ_π, [x] ++ xs, π)` | Delete. |
| 18970, 18979 | `ShadowAll`, `ShadowAllVar` | Delete. |

### 4.6 Delete shadow AST nodes

| Line | Symbol | Action |
|------|--------|--------|
| 178-179 | `StmtKind(...) = 'shadow'` | Delete both `ShadowLetStmt` and `ShadowVarStmt` entries. |
| 18545 | `Stmt` union | Remove `ShadowLetStmt`, `ShadowVarStmt` from the alternatives; add `UsingLocalStmt`. |
| 18960-18961 | `ShadowLetStmt(name, type_opt, init)`, `ShadowVarStmt(name, type_opt, init)` | Delete. |
| 18816-18819 | `BindType` cases for `ShadowLetStmt`/`ShadowVarStmt` | Delete. |
| 3851-3852 | `StmtExprs` cases for `ShadowLetStmt`/`ShadowVarStmt` | Delete. Add no case for `UsingLocalStmt` (no expressions). |

### 4.7 Validate-Module rules (§7.5)

**Delete the three rules — they are redundant with `Intro-Outer-Err`.**

| Line | Rule | Action |
|------|------|--------|
| 4621-4624 | `Validate-Module-Prim-Shadow-Err` | **Delete.** |
| 4626-4629 | `Validate-Module-Special-Shadow-Err` | **Delete.** |
| 4631-4634 | `Validate-Module-Async-Shadow-Err` | **Delete.** |

**Rationale:** `i32`, `Drop`, `Async`, etc. live in `UniverseBindings` (§7.1), which is the outermost scope. At module scope, these names are in `InOuter(Γ, x)`. Reusing them at module scope is therefore exactly the `Intro-Outer-Err` condition and does not need a separate rule family. The diagnostic renderer can produce a more specific message ("`i32` is a primitive type, reserved by the universe scope") when it detects the outer-scope binding is a universe binding, without requiring separate spec rules.

Remove the `(S_cur ≠ S_module ∨ x ∉ UniverseProtected)` premise from `Intro-Ok` — universe-scope visibility is already enforced by the `¬ InOuter(Γ, x)` premise now that `UniverseBindings` is always in the outer scope chain.

### 4.8 Diagnostic code table (§A)

| Line | Code | Action |
|------|------|--------|
| 5677-5679 | `E-CNF-0403/0404/0405` (Validate-Module-*-Shadow-Err) | **Delete all three codes** (rules are redundant with `Intro-Outer-Err`; the renderer gives the specific message when the shadowed name is a universe binding). |
| 5685-5686 | Detailed shadow errors | Delete codes for `Intro-Shadow-Required`, `Shadow-Ok`, `Shadow-Unnecessary`, `Shadow-Reserved-*`. |
| 5677-5679 | `E-MOD-1303/1306` (missing/unnecessary shadow keyword) | **Delete both codes.** |
| — | New | Add `E-CNF-xxxx` for `Intro-Outer-Err`, `E-CNF-xxxx` for `Using-Alias-Dup`, `Using-Alias-Unresolved`, `Using-Alias-Reserved`. Numbers to be assigned in the diagnostic registry. |

### 4.9 Prose references

| Line | Section | Action |
|------|---------|--------|
| 4547 | §7.1 (Reserved names comment) | Update: "Reserved names prevent introduction." (was: "prevent shadowing"). |
| 4555 | §7.2 heading | Change: "Name Introduction and Module Validation" (drop "Shadowing"). |
| 5673 | §5.x section-ownership | Remove "shadowing" from the list. |
| 18938 | §18.3 heading | Delete entire subsection. |
| 24814 | §24 (Hygiene note) | Keep; `using` aliases remain hygienic binders (and now also apply to local scope). |

### 4.10 §11.2 Using Declarations — extension

§11.2 currently covers module-level `using`. Add a new subsection §11.2.6 "Local Using" with:

- Grammar production for `using_local_stmt`
- Typing rule: `UsingAlias` judgment (§7.2 addition above)
- Lowering: none (explicit "no lowering" statement)
- Hygiene: aliases remain hygienic binders (§24 already covers this)
- Visibility: local `using` has no visibility modifier; it is scope-local.

### 4.11 Outer-scope reuse now a hard error

Add to §7.2 alongside the new `Intro-Outer-Err`:

> **Rationale (non-normative):** A binding introduced in an outer scope cannot be reused as the name of a new binding in an inner scope. Users who wish to introduce a new binding under an already-taken name must choose a different name, or introduce their new binding in a separate sibling scope where the outer name is not visible. Users who wish to create a compile-time alternate name for an existing binding should use `using source as alias`.

---

## 5. Compiler Changes (Post-Spec Approval)

Summary of compiler work, to be planned separately after spec is approved:

### 5.1 Lexer / Keywords
- `cursive/include/00_core/keywords.h`: remove `"shadow"` from keyword list.

### 5.2 AST
- `cursive/src/02_source/ast/nodes/ast_enums.h` / stmt enums: remove `ShadowLetStmt`, `ShadowVarStmt` kinds.
- Add `UsingLocalStmt { Identifier source; Identifier alias; Span span; }` to the stmt variant.

### 5.3 Parser
- Remove `shadow_binding` parsing.
- Extend `using` statement parsing to accept `using identifier "as" identifier` at statement position (currently only at item position).

### 5.4 Resolver
- `cursive/src/04_analysis/resolve/scopes_intro.cpp`: remove `ShadowIntro` function entirely. Simplify `Intro` to use the new `Intro-Outer-Err` rule (currently returns `Intro-Shadow-Required`; new behavior is the same terminal error, just renamed).
- `cursive/src/04_analysis/typing/stmt/stmt_common.cpp`: remove `ShadowIntroBinding`; simplify `IntroBinding` similarly.
- Add `UsingAlias` resolution logic — a lookup followed by `Intro` under the new name with the *same* Entity.

### 5.5 Lowering & codegen
- Remove any `ShadowLetStmt`/`ShadowVarStmt` cases from lowering. They should no longer appear in the AST.
- `UsingLocalStmt` lowers to nothing.

### 5.6 Diagnostics
- Remove `E-MOD-1303`, `E-MOD-1306`, and Shadow-specific codes from `diagnostic_codes`/`diagnostic_messages`.
- Rename `Intro-Shadow-Required` → `Intro-Outer-Err` and update the registered diagnostic.
- Add codes for `Using-Alias-*`.

### 5.7 Tests
- Grep HelloCursive for `shadow let` / `shadow var` usages and rewrite them. Either:
  - Introduce distinct names (the strict form), or
  - Use `using x as x_outer` to rename the outer binding out of the way (but this requires the alias to not collide with the outer name — i.e., you must rename to a distinct name, in which case just use a distinct name for your new binding).

### 5.8 Audit CSV
**Delete** all rows corresponding to removed spec rules. The CSV is a table of spec rules — rules that no longer exist in the spec do not belong. Git history preserves the prior state. Rows to delete include (at minimum):
- `ShadowIntro`, `Shadow-Ok`, `Shadow-Unnecessary`, `Intro-Shadow-Required`
- `Shadow-Reserved-Gen-Err`, `Shadow-Reserved-Cursive-Err`
- `Validate-Module-Prim-Shadow-Err`, `Validate-Module-Special-Shadow-Err`, `Validate-Module-Async-Shadow-Err`
- `ShadowLetStmt`, `ShadowVarStmt` AST rows and any `Bind-Shadow*` rules
- `ShadowIntro_B`, `ShadowIntro_π`, `ShadowAll_π`, `ShadowAll`, `ShadowAllVar` helper rows

---

## 6. Decisions (Resolved)

1. **`Intro-Outer-Err` distinct from `Intro-Dup`:** ✅ **Keep distinct.** The spec pattern uses granular, specific diagnostics. The two conditions have different remediation paths (remove the duplicate here vs. pick a name not visible from here) and deserve distinct error codes.

2. **`Using-Alias-Ok` requires the alias name unused in any scope:** ✅ **Confirmed.** `Intro-Ok` requires `¬ InScope(S_cur, x) ∧ ¬ InOuter(Γ, x)`. If `using x as y` introduces `y` as a name, it must follow the same premise — otherwise a shadowing escape hatch re-enters through the back door and contradicts the "no shadow" principle.

3. **Audit CSV for removed rules:** ✅ **Delete the rows.** The CSV tracks spec rules; rules that no longer exist in the spec do not belong in the CSV. Git history preserves the prior state. See §5.8 for the specific row list.

4. **Validate-Module-*-Shadow-Err rules:** ✅ **Delete them, don't rename.** (Correction from the original proposal.) Universe-scope names live in `UniverseBindings` (§7.1), which is in every outer scope at module level. Reusing them at module scope is already covered by `Intro-Outer-Err`. The three rules are redundant. The diagnostic renderer can produce a more specific message when the shadowed name is a universe binding.

5. **HelloCursive migration:** ✅ **Alongside the compiler change.** Main must stay buildable. The spec-audit workflow's "verify the Windows build" step enforces this. Ship spec + compiler + HelloCursive as one coherent landing (branches are fine for isolation, but main never goes red).

---

## 7. Out of Scope for This Revision

- Interaction with comptime `BindPatternCt` and compile-time binding: the current spec has comptime-specific shadow rules in §22 that need a separate pass. **TODO:** Review §22 for any comptime-shadow interactions that also need removal.
- Interaction with region aliases (`RegionAlias` EntitySource): these already exist as aliases; this proposal does not change them. `using` continues to use `EntitySource::Using` for its bindings.
