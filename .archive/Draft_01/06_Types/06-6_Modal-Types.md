# Cursive Language Specification

## Clause 6 — Types

**Part**: VI — Type System  
**File**: 07-6_Modal-Types.md
**Section**: §6.6 Modal Types  
**Stable label**: [type.modal]  
**Forward references**: §6.7 [type.relation], Clause 7 [expr], Clause 9 [generic], Clause 10 [memory], Clause 11 [contract], Clause 12 [witness]

---

### §6.6 Modal Types [type.modal]

[1] Modal types define compile-time state machines. Each modal value inhabits exactly one named state at a time; state transitions are enforced by the type system, ensuring that operations occur in valid orders without runtime overhead. Modal types integrate with the contract system (Clause 12) and the witness system (Clause 13) to express and verify obligations.

[1.1] **Built-in modal types**: Cursive provides two built-in modal types:

- `Ptr<T>` (§7.5): Pointer states (@Null, @Valid, @Weak, @Expired)
- `Arena` (§11.3.2): Memory arena states (@Active, @Frozen, @Freed)

User-defined modal types follow the same syntax and semantics as these built-in types.

(1.2) **Transition Signatures vs. Implementations**: Modal types use two distinct syntactic forms with semantically meaningful operators:

- **Transition signatures** (`@Source::name(params) -> @Target`): Declare valid state transitions within the modal body. These lightweight declarations define the state machine graph and use the **mapping operator `->` (reads as "transitions to" or "maps to")** to indicate the state-to-state relationship.

- **Procedure implementations**: Provide the actual transition logic using standard procedure syntax (§5.4) with the **type operator `:` (reads as "is of type")** for return type annotations. Each transition signature must have a corresponding procedure implementation named `ModalType.name` that takes a receiver in the source state and returns the target state.

[ Note: The semantic distinction between operators clarifies intent:

- **`->` (mapping operator)**: Declares a state transition relationship (`@Source` maps to `@Target`)
- **`:` (type operator)**: Declares a type annotation (`return value : Type`)

Transition signatures use `->` because they declare state graph edges (how states map to each other). Procedure implementations use `:` because they follow standard procedure syntax where return types are type annotations. The operators have distinct semantic meanings that reflect their different roles.
— end note ]

[2] Examples include file handles (Closed → Open → Closed), parser contexts, transactional resources, capability tokens, and memory arenas (Active → Frozen → Freed). The built-in modal types `Ptr<T>` (§7.5) and `Arena` (§11.3.2) demonstrate this mechanism.

#### §7.6.1 Syntax [type.modal.syntax]

[3] The grammar in Annex A §A.3 defines modal declarations:

```
ModalDecl        ::= 'modal' Ident GenericParams? '{' StateBlock+ '}'
StateBlock       ::= '@' Ident StateFields? StateBody?
StateFields      ::= '{' FieldDecl* '}'
StateBody        ::= '{' StateMember* '}'
StateMember      ::= ProcedureDecl | FunctionDecl | TransitionSignature
TransitionSignature ::= '@' Ident '::' Ident '(' ParamList? ')' '->' '@' Ident
```

[4] **Transition Declarations and Implementations**: Modal types distinguish between transition _signatures_ and transition _implementations_:

- **Transition signatures** (inside modal body): Use the form `@SourceState::name(params) -> @TargetState` to declare valid state transitions. The **mapping operator `->` (reads as "transitions to")** indicates that invoking this transition maps the value from the source state to the target state. These lightweight declarations define the state machine graph.

- **Procedure implementations**: Each transition signature shall have a corresponding procedure implementation using standard procedure syntax (§5.4). The procedure is named `ModalType.name`, takes receiver using shorthand (`~`, `~%`, or `~!` corresponding to `const`, `shared`, or `unique` permission on `Self@Source`), and returns a transition type `@Source -> @Target` using the **type operator `:` (reads as "is of type")**. The transition type `@Source -> @Target` is syntactic sugar for the function type `(Self@Source, ...params) -> Self@Target`, where `Self` is the modal type and `@State` desugars to `Self@State` in modal scope. Implementations may appear at module scope or within the state body as full procedure declarations.

[ Note: Transition types are first-class function types. The syntax `@Source -> @Target` in a procedure return type is syntactic sugar for `(Self@Source, ...params) -> Self@Target`, where `Self` is the modal type. This allows transitions to be bound, passed as parameters, and used in higher-order operations. The `->` operator in transition types expresses the state transition relationship as part of the type system, making transitions first-class values while preserving their semantic meaning.
— end note ]

[5] Each state may:

- Declare fields (state-specific payload).
- Provide methods (procedures) available while in that state.
- Define transition signatures using `@Source::name(params) -> @Target` syntax.

#### §7.6.2 Formation Rules [type.modal.formation]

[6] A modal declaration `modal M { @S₁ … @Sₙ }` is well-formed when:

1. State names `Sᵢ` are unique.
2. Field types within each state are well-formed under the enclosing context.
3. Transition clauses reference existing states.
4. Methods use receivers consistent with pointer semantics: receiver shorthand (`~`, `~%`, `~!`) corresponding to permissions `{const, shared, unique}` on `Self@State`.

(6.1) _Missing transition implementation._ If a transition signature `@Source::name(params) -> @Target` is declared but no corresponding procedure `ModalType.name` with matching signature exists, diagnostic E07-505 is emitted. The implementation must match the signature exactly: receiver permission, parameter types, and target state must align. Implementations with mismatched signatures do not satisfy the signature requirement.

(6.2) _Orphan transition implementations._ If a procedure `ModalType.name` exists but no corresponding transition signature is declared, the procedure is treated as a regular associated procedure (not a transition). Such procedures may still be called but do not participate in state tracking. No diagnostic is emitted for orphan implementations; they are valid for non-transition operations.

(6.3) _State field access outside state._ When a state-specific field is accessed through a modal value that is not in the field's defining state, diagnostic E07-504 is emitted. This check occurs during type checking; the compiler tracks the current state of each modal value through pattern matching and transition calls. Accessing fields from a different state is always ill-formed, even if the access would be safe at runtime.

Formally:

$$
\dfrac{\text{modal } M \{ @S_1, \ldots, @S_n \} \text{declared} \quad S_i \ne S_j \quad \Gamma \vdash \text{fields/members}}{\Gamma \vdash M : \text{Type}}
\tag{WF-Modal-Type}
$$

#### §7.6.2.5 Parser Disambiguation [type.modal.disambiguation]

[7] The `->` and `:` operators are grammatically unambiguous in modal contexts:

**Context 1: Transition signatures (within modal body)**

- Form: `@StateIdentifier::name(params) -> @StateIdentifier`
- Operator: `->` (mapping operator)
- Context markers: Appears after parameter list within modal state block
- Parser rule: When `@Ident::Ident(...)` is followed by `->`, parse as transition signature

**Context 2: Procedure implementations (module or state scope)**

- Form: `procedure ModalType.name(params): ReturnType`
- Operator: `:` (type operator)
- Context markers: Appears after parameter list in procedure declaration
- Parser rule: When `procedure` keyword precedes, `:` introduces return type

**Disambiguation table:**

| Syntactic Context                       | Operator | Parsed As              |
| --------------------------------------- | -------- | ---------------------- |
| Inside modal body after `@Ident::(...)` | `->`     | Transition signature   |
| After `procedure` keyword and params    | `:`      | Return type annotation |

[8] **Transition type parsing**: When parsing a return type after `:` in a procedure declaration, if the type contains `->` between state identifiers (`@Ident -> @Ident`), it is parsed as a transition type. Transition types may only appear in return type positions of procedures declared within modal scope (i.e., procedures that are methods of a modal type). The parser recognizes transition types by the pattern `@` followed by an identifier, `->`, `@`, and another identifier.

#### §7.6.3 State Types [type.modal.state]

[9] Each state induces a nominal subtype `M@State`. Values of type `M` have unknown state. Pattern matching refines the state:

```cursive
match conn {
    @Closed { path } => { ... }   // conn : Connection@Closed
    @Open { handle } => { ... }   // conn : Connection@Open
}
```

Typing rule:

$$
\dfrac{\Gamma \vdash e : M \quad \text{states cover all }@S_i}{\Gamma \vdash \text{match } e \text{ with }@S_i : \tau}
\tag{T-Match-Modal}
$$

Within each branch, the value is treated as `M@S_i` and state fields are accessible.

#### §7.6.4 Transitions [type.modal.transition]

[8] Transitions are defined through two components:

**Transition signature** (inside modal body):

```cursive
modal Connection {
    @Closed { path: string@Managed }
    @Closed::open(~!, path: string@View) -> @Open

    @Open { handle: Handle }
}
```

**Procedure implementation** (at module scope or inline):

```cursive
procedure Connection.open(~!, path: string@View): @Closed -> @Open
    [[ io::open |- true => witness Connection@Open ]]
{
    let handle = os::open(path)
    result Connection@Open { handle, path: path.to_owned() }
}
```

[9] The signature `@Source::name(params) -> @Target` declares the transition, including receiver shorthand (`~`, `~%`, `~!`) if the transition requires a self parameter. The implementation shall:

- Be named `ModalType.name`
- Take receiver using shorthand matching the signature: `~` (const), `~%` (shared), `~!` (unique)
- Return a transition type `@Source -> @Target` where Source and Target match the signature
- The transition type `@Source -> @Target` desugars to function type `(Self@Source, ...params) -> Self@Target`, where `@State` desugars to `Self@State` in modal scope

Typing rule:

$$
\dfrac{\Gamma \vdash self : M@S \quad \Gamma \vdash \text{body}: M@Target \quad \text{contract ok}}{\Gamma \vdash \text{procedure } M.name(self: perm M@S, ...): @S \to @Target}
\tag{T-Transition}
$$

The body must `result` a value of type `M@Target` (where `M@Target` is the desugared form of `@Target` in modal scope). The transition type `@S -> @Target` is equivalent to the function type `(M@S, ...params) -> M@Target`. Failing to return the correct type triggers E07-502. Witness semantics are described in §7.6.5.

[ Rationale: For transitions on `unique` receivers (`~!`), returning `result ModalType@NewState { ... }` performs an in-place update of the object's memory. The fields are overwritten, and the object's conceptual state changes without requiring a new allocation. This ensures that state transitions on unique values are efficient, zero-cost abstractions. Transitions on `const` or `shared` receivers must construct and return a new object in the target state, as they cannot modify the original in place. — end rationale ]

#### §7.6.5 Contracts and Witnesses [type.modal.contract]

[10] Modal transition procedures follow all standard procedure semantics (§5.4), including contractual sequent specifications `[[ grants … |- must => will ]]`. In addition, each transition signature imposes the following witness obligations:

1. **Entry witness** — the caller shall provide `witness M@Source` (or an equivalent fact provable from the current control-flow) before invoking the procedure.
2. **Exit witness** — the procedure shall prove or emit `witness M@Target` on every non-diverging exit path. Returning a value of type `M@Target` without supplying the witness is ill-formed and triggers E07-502.
3. **Exclusive result states** — a transition signature maps exactly one `@Source` to one `@Target`. A procedure that wishes to return multiple states shall declare separate signatures so that each branch has an explicit witness obligation.

(10.1) Implementations shall enforce that every transition signature has a matching procedure whose contractual sequent names the entry/exit witnesses. This requirement ensures that the witness system (Clause 13) can track linear state changes without heuristic inference.

**Example:**

```cursive
procedure close(~!): @Open -> @Closed
    [[ io::close, witness Connection@Open
       |- true => witness Connection@Closed ]]
{
    os::close(self.handle)
    result Connection@Closed { path: self.path }
}
```

#### §7.6.6 Modal Field Access [type.modal.fields]

[11] State fields are accessible only when the value is known to be in that state. Attempting to read `value.field` when the current type is `M` (unknown state) is ill-formed (E07-504). Pattern matching or witness-based refinement is required.

#### §7.6.7 Subtyping [type.modal.subtyping]

[12] The subtyping relationship includes:

- `M@State <: M` (state-specific type is a subtype of the general modal type).
- Distinct states are incomparable (`M@S₁` and `M@S₂` only subtype each other if `S₁ = S₂`).

These rules integrate with the general subtyping relation in §7.7.

#### §7.6.8 Diagnostics [type.modal.diagnostics]

[13] Minimal diagnostics:

| Code    | Condition                                       |
| ------- | ----------------------------------------------- |
| E07-500 | Duplicate state name                            |
| E07-501 | Transition references undeclared state          |
| E07-502 | Transition body fails to return target state    |
| E07-503 | Exhaustiveness failure in modal pattern match   |
| E07-504 | State-specific field accessed outside its state |
| E07-505 | Missing transition implementation               |

#### §7.6.9 Examples (Informative) [type.modal.examples]

**Example 7.6.9.1 (File lifecycle):**

```cursive
modal FileHandle {
    @Closed { path: string@Managed }
    @Closed::open(~!, path: string@View) -> @Open

    @Open { path: string@Managed, handle: os::Handle }
    @Open::read(~%, buffer: [u8]) -> @Open
    @Open::close(~!) -> @Closed
}
```

**Transition implementations at module scope:**

```cursive
procedure FileHandle.open(~!, path: string@View): @Closed -> @Open
[[io::open |- true => witness FileHandle@Open]]
{
let handle = os::open(path)
result FileHandle@Open { path: path.to_owned(), handle }
}

procedure FileHandle.read(~%, buffer: [u8]): @Open -> @Open
[[io::read, witness FileHandle@Open |- buffer.len() > 0 => true]]
{
os::read(self.handle, buffer)
result self
}

procedure FileHandle.close(~!): @Open -> @Closed
[[io::close, witness FileHandle@Open |- true => witness FileHandle@Closed]]
{
os::close(self.handle)
result FileHandle@Closed { path: self.path }
}

procedure use_file(path: string@View)
grants io::open, io::read, io::close
{
let file = FileHandle::from_path(path)
match file {
@Closed => {
let open = file.open()
open.read(buffer)
let closed = open.close()
// closed : FileHandle@Closed
}
}
}
```

**Example 7.6.9.2 (Token-based capability):**

```cursive
modal Transaction {
    @Idle {}
    @Idle::begin(~!) -> @Active

    @Active {}
    @Active::commit(~!) -> @Idle
    @Active::rollback(~!) -> @Idle
}

// Transition implementations
procedure Transaction.begin(~!): @Idle -> @Active
    [[ db::begin |- true => witness Transaction@Active ]]
{
    db::begin()
    result Transaction@Active {}
}

procedure Transaction.commit(~!): @Active -> @Idle
    [[ db::commit, witness Transaction@Active |- true => witness Transaction@Idle ]]
{
    db::commit()
    result Transaction@Idle {}
}

procedure Transaction.rollback(~!): @Active -> @Idle
    [[ db::rollback, witness Transaction@Active |- true => witness Transaction@Idle ]]
{
    db::rollback()
    result Transaction@Idle {}
}
```

#### §7.6.10 Conformance Requirements [type.modal.requirements]

[14] Implementations shall:

1. Support modal syntax as described in §7.6.1 with unique state names.
2. Enforce that state-specific fields and methods are used only in their defining state.
3. Ensure transitions return values of the target state; emit diagnostic E07-502 otherwise.
4. Refine modal types in pattern matches and enforce exhaustiveness (E07-503).
5. Track witnesses per Clause 13, ensuring sequent clauses are respected.
6. Provide diagnostics in §7.6.8 with contextual information (location of state declaration and offending use).

[15] Deviations render an implementation non-conforming unless explicitly noted elsewhere.

---

**Previous**: §7.5 Pointer and Reference Types (§7.5 [type.pointer]) | **Next**: §7.7 Type Relations (§7.7 [type.relation])
