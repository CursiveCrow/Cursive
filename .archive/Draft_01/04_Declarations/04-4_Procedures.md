# Cursive Language Specification

## Clause 4 — Declarations

**Clause**: 4 — Declarations
**File**: 05-4_Procedures.md
**Section**: §4.4 Procedures
**Stable label**: [decl.function]  
**Forward references**: Clause 7 [expr], Clause 8 [stmt], Clause 9 [generic], Clause 10 [memory], Clause 11 [contract], Clause 14 [interop]

---

### §4.4 Procedures [decl.function]

#### §4.4.1 Overview

[1] Procedure declarations introduce named computations at module scope or as associated members of types.

[2] Cursive provides a single callable form: _procedures_. Purity is determined by the sequent's grant set. Procedures whose sequents declare an empty grant set (e.g., omitted sequents defaulting to `[[ ∅ |- true => true ]]`, or explicit `[[ P => Q ]]` without grants) are pure and may be evaluated anywhere without requiring additional capabilities. Procedures whose sequents declare non-empty grant sets may perform the capabilities named in that set.

[3] Procedures may declare a receiver parameter `self` typed as `Self` with permission qualifiers `const`, `shared`, or `unique` (§10.4). When omitted, the procedure behaves as a static callable that still participates in grant checking.

[4] This subclause specifies the declaration surface for procedures; Clause 8 details callable bodies and evaluation, while Clause 11 defines sequent semantics and the grant system.

#### §4.4.2 Syntax

[ Note: See Annex A §A.6 [grammar.declaration] for the normative `procedure_decl` production. — end note ]

[1] Procedure declarations consist of optional attributes and visibility modifiers, a procedure head (procedure kind and identifier), a signature (optional generic parameters, parameter list, optional return type, optional sequent clause), and a callable body (block, expression body for pure procedures, or semicolon for extern declarations). Procedure kinds are `procedure`, `comptime procedure`, or `extern [string] procedure`. Parameters may include receiver shorthands (`~`, `~%`, `~!`) or regular parameters with optional `move` modifier. Sequent clauses use `[[ sequent_expr ]]` syntax per Annex A §A.7.

[1] `callable_attributes` encompasses optimisation, diagnostic, and linkage attributes defined in Clauses 9 and 16.

[2] Sequents attach to procedures using form `[[ grants |- must => will ]]`. The sequent clause is **optional**: when omitted, defaults to pure procedure `[[ |- true => true ]]`. Abbreviated forms (e.g., `[[ io::write ]]`, `[[ x > 0 => result > x ]]`) are supported with deterministic expansion. Complete sequent syntax, smart defaulting rules, and canonical forms are specified in §11.2 [contract.sequent].

[3] Expression-bodied forms (`= expression ;`) are syntactic sugar for pure procedures and shall not include a sequent (the default `[[ ∅ |- true => true ]]` is implicit, canonical: `[[ |- true => true ]]` with empty grant set). Violations: E05-408.

#### §4.4.3 Constraints

[1] _Visibility defaults._ Module-scope procedures default to `internal` visibility. Associated procedures inherit the containing type’s visibility unless overridden.

[2] _Receiver parameter._ Procedures that include a receiver must declare it as the first parameter, either explicitly (`self: const/shared/unique Self`) or by using the shorthand `~`/`~%`/`~!`, which desugars respectively to `self: const/shared/unique Self`. A receiver parameter shall not appear in any position other than first. Violations: E05-401.

(2.1) _Parameter responsibility modifier._ Parameters may optionally include the `move` modifier to indicate that the procedure assumes cleanup responsibility for the parameter value:

```cursive
procedure process(data: Buffer)             // Non-responsible (default)
procedure consume(move data: Buffer)        // Responsible (takes ownership)
```

(2.2) Without `move`, parameters are non-responsible: they behave like non-responsible bindings (`let param <- argument`) and do not invoke destructors when the procedure returns. With `move`, parameters are responsible: they behave like responsible bindings (`let param = argument`) and invoke destructors when the procedure returns.

(2.3) Call sites must use `move` when calling procedures with `move` parameters: `consume(move x)`. Omitting `move` when required: E05-409. Using `move` when parameter lacks modifier: E05-410.

[3] _Sequents optional._ Sequent specifications are optional. When omitted, the procedure defaults to `[[ ∅ |- true => true ]]` (empty grant set, canonical: `[[ |- true => true ]]`, pure procedure). Procedures shall include explicit sequents when they require grants, enforce preconditions, or guarantee postconditions. Sequents may appear on the same line as the signature (for simple cases) or on the following line (recommended style for complex contracts).

[4] _Expression bodies._ Expression-bodied procedures (`= expression ;`) shall not include explicit sequents; the default `[[ ∅ |- true => true ]]` is implicit (canonical: `[[ |- true => true ]]` with empty grant set). Violations: E05-408.

[5] _Pure procedures._ Procedures whose sequents declare an empty grant set may not call procedures whose grant sets are non-empty nor perform operations that require grants. Violations: E05-406.

[6] _Comptime procedures._ Bodies of `comptime procedure` declarations shall be compile-time evaluable. Any grants referenced must be valid in compile-time contexts. Violations: E05-402.

[7] _Extern procedures._ `extern` procedures declare their signature and sequent but omit a body (`;`). Their linkage is governed by Clause 14.

[8] _Recursion._ Procedures may reference themselves directly or indirectly. Implementations may warn when they detect unprovable termination but shall not reject recursive procedures solely for that reason.

#### §4.4.4 Semantics

[1] Procedure definitions introduce callable entities that, when referenced, produce values of FunctionType (§6.4). The type records parameter types, return type, grant set, and sequent.

[2] The sequent governs three responsibilities:

- **grant component** (before turnstile): grants that must be available when the procedure executes
- **must** (after turnstile, before arrow): predicates that must hold when the procedure begins execution
- **will** (after arrow): predicates that will hold when the procedure completes normally

[3] Receiver shorthands desugar before semantic analysis so that tooling observes an explicit `self` parameter bound to `Self` with the requested permission qualifier.

[4] When procedures include sequents, the sequent shall appear contiguous with the signature; implementations shall accept either the same line or the immediately following line. For pure procedures, the sequent may be omitted entirely.

[5] `comptime` procedures execute during compile-time evaluation. Failure to evaluate successfully renders any compile-time use ill-formed.

##### §4.4.4.1 Parameter Responsibility Semantics [decl.function.params.responsibility]

[6] Parameter cleanup responsibility is determined by the presence of the `move` modifier:

**Non-responsible parameters (default):**

```cursive
procedure process(data: Buffer)
{
    // data is non-responsible (like 'let data <- argument')
    // data.drop() NOT called when procedure returns
}
```

When a procedure is called with a non-responsible parameter, the argument remains valid after the call. The parameter binding does not invoke a destructor.

**Responsible parameters (`move` modifier):**

```cursive
procedure consume(move data: Buffer)
{
    // data is responsible (like 'let data = argument')
    // data.drop() WILL be called when procedure returns
}
```

When a procedure is called with a responsible parameter, cleanup responsibility transfers from the argument to the parameter. The argument binding becomes invalid (moved) and the parameter binding invokes the destructor at the end of the procedure body.

[7] **Call site requirement.** When calling a procedure with a `move` parameter, the call site must use the `move` keyword on the argument:

```cursive
let buffer = Buffer::new()
process(buffer)             // ✅ OK: non-responsible parameter, buffer still valid
consume(move buffer)        // ✅ OK: responsible parameter, buffer becomes invalid
consume(buffer)             // ❌ ERROR E05-409: missing move for responsible parameter
```

[8] **Binding semantics equivalence.** Parameter responsibility semantics are equivalent to the corresponding local binding forms: `data: T` is equivalent to `let data <- argument` (non-responsible), and `move data: T` is equivalent to `let data = argument` (responsible). This equivalence ensures consistent behavior between parameter passing and local bindings.

#### §4.4.5 Examples (Informative)

**Example 4.4.5.1 (Pure procedure with expression body):**

```cursive
procedure clamp(value: i32, min: i32, max: i32): i32
= value.max(min).min(max)
```

[1] Expression-bodied procedures are implicitly pure; no sequent is needed.

**Example 4.4.5.3 (Responsible parameter with move):**

```cursive
procedure consume(move data: Buffer)
{
    process_data(data)
    // data.drop() called automatically when procedure returns
}

let buffer = Buffer::new()
consume(move buffer)             // buffer becomes invalid
// buffer.use()                  // error[E11-503]: use of moved value
```

**Example 4.4.5.4 (Procedure with receiver and grants):**

```cursive
procedure deposit(~!, amount: i64)
    [[ ledger::post |- amount > 0 => self.balance >= amount ]]
{
    self.balance += amount
}
```

**Example 5.4.5.8 - invalid (Missing move at call site):**

```cursive
procedure consume(move data: Buffer)
{ }

let buffer = Buffer::new()
consume(buffer)              // error[E05-409]: parameter requires move
```

#### §4.4.6 Conformance Requirements [decl.function.requirements]

[1] Tooling shall record sequents, grant sets, receiver permissions, parameter responsibility modifiers, and callable kinds in reflection metadata so that downstream analyses can reason about capability requirements and cleanup responsibilities.
