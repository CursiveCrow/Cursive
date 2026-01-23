# Cursive Language Specification

## Clause 6 — Types

**Clause**: 6 — Types
**File**: 07-1_Type-Foundations.md
**Section**: §6.1 Type System Overview
**Stable label**: [type.overview]  
**Forward references**: §6.2 [type.primitive], §6.3 [type.composite], §6.4 [type.function], §6.5 [type.pointer], §6.6 [type.modal], §6.7 [type.relation], §6.8 [type.introspection], Clause 7 [expr], Clause 8 [generic], Clause 9 [memory], Clause 10 [contract], Clause 12 [witness]

---

### §6.1 Type System Overview [type.overview]

[1] This clause specifies the static type system that governs every well-formed Cursive program. The type system classifies values, constrains expressions, coordinates permissions and grants, and ensures that well-typed programs avoid undefined behavior attributable to type errors.

[2] Cursive's type discipline is primarily nominal, augmented with structural forms where doing so preserves zero abstraction cost and local reasoning. Bidirectional type inference is an intrinsic design goal: the type checker flows information both from expressions to their contexts and from contexts into expressions so that most local declarations do not require redundant annotations while still keeping inference predictable.

#### §6.1.1 Design Objectives [type.overview.objectives]

[3] The type system shall satisfy the following objectives:

- **Safety**: Typing judgments preclude use-after-free, type confusion, and contract violations when combined with the permission (§10) and contract (§11) systems.
- **Determinism**: Type checking is deterministic with respect to the source program, the available grants, and the compilation environment.
- **Explicitness**: Grants, modal states, and region usage are reflected in type constructors, ensuring that each obligation is visible in source.
- **Composability**: Types compose predictably through generics, unions, and composite constructors without hidden coercions or implicit special cases.
- **LLM-friendly regularity**: Type formation rules, inference heuristics, and rule names follow consistent patterns so that automated tooling can reason locally.

#### §6.1.2 Permission-Qualified Types [type.overview.permissions]

[3.1] Permission qualifiers (const, unique, shared) attach to types at binding sites. Complete permission semantics are specified in §10.4 [memory.permission].

#### §6.1.3 Typing Judgments and Notation [type.overview.judgments]

[4] The core judgments reused throughout Clause 6 are:

- $\Gamma \vdash \tau : \text{Type}$ — $\tau$ is a well-formed type under environment $\Gamma$.
- $\Gamma \vdash e : \tau \; ! \varepsilon$ — expression $e$ has type $\tau$ and requires grant set $\varepsilon$.
- $\tau_1 <: \tau_2$ — subtyping relation defined in §6.7 [type.relation].
- $\tau_1 \simeq \tau_2$ — type equivalence relation that respects aliases and structural normalization (§6.7.2).

[5] Type environments bind term variables to types, type variables to kinds (with optional behavior bounds), grant parameters to grant sets, and modal parameters to state constraints. Environment formation and lookup rules follow §1.3 [intro.terms] conventions; §6.7 restates any additional obligations introduced by subtype or modal analysis.

#### §6.1.4 Guarantees [type.overview.guarantees]

[6] A program that satisfies all typing, permission, and contract judgments shall not:

- Invoke undefined behavior listed in Annex B due to type mismatches.
- Access memory through an invalid pointer representation (permissions enforce aliasing rules).
- Invoke a procedure without possessing the grants named in its signature.
- Transition a modal value to a state not permitted by its modal type.

[7] Bidirectional inference is sound and complete with respect to the declarative typing rules stated in §§6.2–6.7. When local annotations are omitted, inference succeeds if and only if a unique principal type exists under the rules of this clause. Situations requiring programmer guidance (e.g., ambiguous unions or polymorphic recursion) shall elicit diagnostics that point to the relevant obligation.

#### §6.1.5 Examples (Informative) [type.overview.examples]

**Example 6.1.6.1 (Bidirectional inference with unions):**

```cursive
procedure normalize(input: stream::Line): string \/ io::Error
    [[ io::read |- input.is_open() => result is string \/ io::Error ]]
{
    let trimmed = input.text().trim();        // Context infers `string`
    if trimmed.is_empty() {
        result io::Error::invalid_data("empty line")
    } else {
        result trimmed
    }
}
```

[21] The return type annotation supplies the expected union. The `result` statements rely on automatic widening (`string <: string \/ io::Error`) and on the contractual sequent to document the grant requirements.

**Example 6.1.6.2 (Pointer state integrated with permissions):**

```cursive
procedure swap_current(~!, nodes: list::Iterator@Active, value: Node)
    [[ allocator::unique |- nodes.has_current() => nodes.current() == value ]]
{
    let target: Ptr<Node>@Valid = nodes.current_ptr();
    target.write(value);   // Pointer type ensures state validity and unique access
}
```

[22] The pointer type records both the region (`PointerType`, §7.5) and the modal state (`@Valid`). The contract and grant annotations rely on the type system to guarantee that the write is safe.

---

**Previous**: _(start of clause)_ | **Next**: §6.2 Primitive Types (§6.2 [type.primitive])
