# Transition Type Design Analysis

**Question**: Should transitions be a new type form (type annotation) or a function type (first-class values)?

---

## Option A: Transition as Type Annotation (Desugars)

**Current Proposal**:

- `@Closed -> @Open` is syntactic sugar for `Self@Open` with transition metadata
- Desugars to: procedure returns `Self@Open`, compiler tracks transition relationship
- Not first-class: transitions cannot be bound or passed

**Syntax**:

```cursive
procedure File.open(self: unique Self@Closed, path: Path): @Closed -> @Open
// Desugars to: returns Self@Open, transition metadata tracked by compiler
```

**Pros**:

- Simpler type system (no new type form)
- Transitions are inherently tied to modal instances (can't exist independently)
- Matches current design where transitions operate on `self`
- No need to handle transition values at runtime

**Cons**:

- Cannot pass transitions as parameters
- Cannot bind transitions to variables
- Less flexible for higher-order operations
- Transition relationship is compile-time only

---

## Option B: Transition as Function Type (First-Class)

**Proposal**:

- `@Closed -> @Open` is a function type: `(Self@Closed, ...params) -> Self@Open`
- Transitions can be bound: `let open_transition: @Closed -> @Open = File.open`
- Transitions can be passed as parameters
- First-class values

**Syntax**:

```cursive
// Transition type is a function type
type Transition = @Closed -> @Open  // Equivalent to (Self@Closed, Path) -> Self@Open

procedure File.open(self: unique Self@Closed, path: Path): @Closed -> @Open
{
    // Returns Self@Open
}

// Can bind transitions
let open_transition: @Closed -> @Open = File.open

// Can pass transitions
procedure apply_transition<T>(transition: @State1 -> @State2, value: T@State1): T@State2
{
    result transition(value)
}
```

**Pros**:

- First-class transitions enable higher-order operations
- Can pass transitions as parameters
- Can bind transitions to variables
- More flexible design
- Transitions become reusable abstractions

**Cons**:

- More complex type system (new function type form)
- Transitions are tied to modal instances - can they really be independent?
- Need to handle transition values at runtime
- May conflict with current design where transitions require `self` receiver

**Key Question**: Can a transition exist without a modal instance? Current design has transitions as methods with `self` receiver, so `File.open` requires a `File@Closed` instance. If transitions are function types, they'd need to capture the instance somehow.

---

## Option C: Hybrid Approach

**Proposal**:

- Transition type annotation `@Closed -> @Open` desugars to function type `(Self@Closed, ...params) -> Self@Open`
- But transitions are still methods - they can't be extracted without the instance
- The type annotation expresses the transition relationship
- Transitions can be passed when bound to an instance method

**Syntax**:

```cursive
// Type annotation expresses transition
procedure File.open(self: unique Self@Closed, path: Path): @Closed -> @Open
// Type is: (unique Self@Closed, Path) -> Self@Open

// Can bind method (which is a transition)
let open_method = File.open  // Type: (unique Self@Closed, Path) -> Self@Open

// But still requires instance to call
let file: File@Closed = File::new_closed(path)
let opened = open_method(file, path)  // Returns File@Open
```

**Pros**:

- Transitions are function types (first-class)
- But still tied to modal type (can't exist without Self)
- Type annotation clearly expresses transition relationship
- Enables method passing while maintaining modal semantics

**Cons**:

- Still need to clarify: is `@Closed -> @Open` the same as `(Self@Closed, ...) -> Self@Open`?
- Or is it a special transition type that's a subtype of function type?

---

## Analysis Questions

1. **Use Case**: Do we need to pass transitions as parameters? What would that enable?

   - State machine builders?
   - Transition composition?
   - Generic transition handlers?

2. **Semantics**: Can a transition exist without a modal instance?

   - Current design: transitions are methods with `self` receiver
   - If first-class: transitions would be functions that take instance as first parameter

3. **Type System**: Should `@Closed -> @Open` be:

   - A new primitive type form?
   - Syntactic sugar for function type?
   - A subtype/specialization of function type?

4. **Runtime**: Do transitions need runtime representation?
   - If type annotation only: compile-time metadata
   - If function type: runtime function values

---

## Recommendation

**Option C (Hybrid)** seems most promising:

- `@Closed -> @Open` is syntactic sugar for `(Self@Closed, ...params) -> Self@Open`
- Transitions are function types (first-class)
- But they're methods, so they require the modal type context
- Type annotation clearly expresses transition relationship
- Enables method passing while maintaining modal semantics

**Key Insight**: The transition type `@Closed -> @Open` is essentially a function type where:

- First parameter is implicitly `Self@Closed` (the receiver)
- Return type is `Self@Open` (desugared from `@Open`)
- The `->` syntax makes the transition relationship explicit

This gives us:

- First-class transitions (can pass methods)
- Clear transition semantics (type annotation)
- No lookahead needed (after `:` parse type, `->` indicates transition)
- Consistent with function types

---

**Decision Needed**: Which approach should we take?
