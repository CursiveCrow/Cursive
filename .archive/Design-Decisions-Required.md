# Design Decisions Required

**Date**: November 8, 2025  
**Purpose**: Present design decisions needed before implementing fixes for validated issues

---

## Design Decision 1: Contract Clause Optionality (P0.4, P1.14)

**Issue**: Grammar says `contract_clause+` (required, 1+) but prose says optional (0-1).

**Current State**:

- Annex A line 947: `contract_clause+` (requires 1+)
- §5.4.3[3]: "Contractual sequent specifications are optional"
- §5.4.2[2]: "The sequent clause is **optional**"
- §12.2.2.2: Smart defaulting allows omitting entire sequent

**Options**:

**Option A**: Make grammar match prose (recommended)

- Change Annex A to `contract_clause?` (optional, 0-1)
- Pure procedures can omit sequent entirely
- Matches current prose and examples

**Option B**: Make prose match grammar

- Require sequent clause for all procedures
- Pure procedures must write `[[ |- true => true ]]` explicitly
- More verbose but explicit

**Recommendation**: **Option A** - The prose is clear that sequents are optional, and this matches the design goal of reducing boilerplate for pure functions.

**Decision Required**: [ ] Option A [ ] Option B

---

## Design Decision 2: Modal Operator Disambiguation (P0.6)

**Issue**: Specification claims "no backtracking or lookahead beyond `)` required" but parser must peek ahead to distinguish `->` (transition signature) from `:` (procedure return type).

**Current State**:

- §7.6.2.5[8]: Claims no lookahead needed
- Reality: Parser must peek after `)` to see `->` vs `:`
- Both appear after parameter lists

**Options**:

**Option A**: Keep both operators, update claim

- Keep `->` for transition signatures, `:` for implementations
- Update §7.6.2.5 to state lookahead is required
- Document parser requirement accurately

**Option B**: Unify on single operator (`:`)

- Use `:` for both transition signatures and implementations
- Transition signatures: `@Closed::open(~!, path: Path): @Open`
- Implementations: `procedure File.open(...): Self@Open` (unchanged)
- **Pros**: Simpler grammar, no special case, no lookahead needed
- **Cons**: Loses semantic distinction between transition declaration and type annotation

**Option C**: Include transition in return type (NEW PROPOSAL)

- Transition signatures: Keep `->` (unchanged): `@Closed::open(~!, path: Path) -> @Open`
- Implementations: Use `->` in return type: `procedure File.open(self: unique Self@Closed, path: Path): @Closed -> @Open`
- The return type `@Closed -> @Open` expresses the transition as part of the type
- **Pros**:
  - Keeps `->` for transitions (semantic meaning preserved)
  - Uses `:` for type annotation (consistent with other procedures)
  - Transition is part of the type system
  - No lookahead needed (after `:` always expect type, which may contain `->`)
- **Cons**:
  - New type form `@State1 -> @State2` needs definition
  - May need to clarify: does procedure return `Self@Open` or transition type?
  - More complex type system

**Clarification for Option C** (CONFIRMED):

- Return type uses just states: `@Closed -> @Open` (not `Self@Closed -> Self@Open`)
- `@State` desugars to `Self@State` implicitly (since transition procedures must be in modal scope, `Self` is always known)
- The `Self` prefix is unnecessary syntax since it's the only possible type
- Procedure returns value of type `Self@Open` (desugared from `@Open`)
- `@Closed -> @Open` is a transition type annotation that expresses the state transition
- No lookahead needed: after `:` we always parse a type; if it contains `->` between state identifiers, it's a transition type

**Example**:

```cursive
modal File {
    @Closed { path: Path }
    @Closed::open(~!, path: Path) -> @Open  // Signature unchanged

    @Open { handle: Handle }
}

// Implementation uses transition type in return annotation
procedure File.open(self: unique Self@Closed, path: Path): @Closed -> @Open
    [[ fs::open |- path.exists() => result.state() == @Open ]]
{
    let handle = os::open(path)
    result File@Open { handle, path: path.to_owned() }  // Returns Self@Open
}
```

**Recommendation**: **Option C** - Elegant solution that preserves semantic meaning while eliminating lookahead. The transition becomes part of the type annotation, which is semantically correct.

**Decision**: ✅ **Option C** (CONFIRMED - transitions as function types)

**Final Design**:

- `@Closed -> @Open` is syntactic sugar for function type `(Self@Closed, ...params) -> Self@Open`
- Transitions are first-class function types (can be bound, passed as parameters)
- Return type uses just states: `@Closed -> @Open` (not `Self@Closed -> Self@Open`)
- `@State` desugars to `Self@State` implicitly in modal scope
- No lookahead needed: after `:` parse type, `->` between state identifiers indicates transition function type
- Consistent with existing function type syntax using `->`

---

## Design Decision 3: Permission Syntax Clarification (P0.11)

**Issue**: Permission syntax is consistent (`const Type`) but specification could be clearer about whether `const` is part of the type or a binding modifier.

**Current State**:

- Syntax is consistent: `let x: const Type`
- §11.4.2.1[5]: States `const` is default when omitted
- Design appears sound

**Options**:

**Option A**: Clarify in prose only

- Add explicit note: "Permissions are type qualifiers, not binding modifiers"
- Keep current syntax
- Minimal change

**Option B**: No change needed

- Current specification is sufficient
- Design is sound, just needs better explanation

**Recommendation**: **Option A** - Add clarification note. The design is sound but the explanation could be clearer.

**Decision Required**: [ ] Option A [ ] Option B

---

## Design Decision 4: `<-` Operator Terminology (P0.12)

**Issue**: Operator called "reference assignment" but doesn't create reference types. Terminology is confusing.

**Current State**:

- §5.2.5.2: "Reference Assignment Operator (`<-`)"
- §5.2.5.2[8]: Explicitly states "does NOT create a reference type"
- Terminology contradicts itself

**Options**:

**Option A**: Rename to "non-responsible binding operator"

- More accurate description
- Emphasizes responsibility, not type
- Update all references

**Option B**: Keep "reference assignment" but clarify

- Add prominent note explaining it's about responsibility, not types
- Keep existing terminology

**Recommendation**: **Option A** - "Non-responsible binding operator" is more accurate and less confusing than "reference assignment" which suggests reference types.

**Decision Required**: [ ] Option A [ ] Option B

---

## Design Decision 5: Procedure Entry Point Sequent Requirements (P1.44)

**Issue**: Unclear if sequents are required, recommended, or optional for `main`.

**Current State**:

- §5.8.2[4]: "`main` shall declare a contractual sequent that explicitly lists any grants"
- §5.4.3[3]: "Contractual sequent specifications are optional"
- Contradiction

**Options**:

**Option A**: Required for `main` (if grants needed)

- `main` must declare sequent if it uses any grants
- Pure `main` can omit sequent
- Explicit capability tracking for entry point

**Option B**: Optional for `main` (same as other procedures)

- `main` follows same rules as other procedures
- Can omit sequent, defaults to pure
- Consistent with general rule

**Recommendation**: **Option A** - Entry points should explicitly declare grants for clarity and security. Pure `main` can omit, but any `main` with effects should declare grants.

**Decision Required**: [ ] Option A [ ] Option B

---

## Summary

Please indicate your decisions for each issue above. Once decisions are made, I will:

1. **Implement consistency/terminology fixes** (no design decisions needed):

   - Fix predicate/behavior terminology
   - Remove outdated note
   - Fix spelling (behaviour → behavior)
   - Add missing keywords
   - Standardize manifest terminology
   - Add missing grammar productions

2. **Implement design decisions** (after approval):

   - Update grammar/prose based on decisions
   - Fix operator disambiguation claim
   - Clarify permission syntax
   - Update `<-` operator terminology
   - Clarify entry point sequent requirements

3. **Note content creation tasks** (not implementing):
   - Clause 17 (Standard Library)
   - Annex B catalogs
   - Annex C formal semantics
   - Annex E algorithms

---

**Please review and provide decisions for each item above.**
