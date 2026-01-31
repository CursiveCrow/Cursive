// =============================================================================
// MIGRATION MAPPING: Contract Clause Parsing
// =============================================================================
// Destination: cursive/src/02_source/parser/item/contract_clause.cpp
// Source:      cursive-bootstrap/src/02_syntax/parser_items.cpp (lines 362-405)
//
// PURPOSE:
// Parse contract clauses: |= precondition => postcondition
// Supports precondition only, postcondition only, or both.
// =============================================================================

// =============================================================================
// SPEC RULES IMPLEMENTED
// =============================================================================
// From C0updated.md Section 3.3.6.13:
//
// (Parse-ContractClauseOpt-None)
// ¬ IsOp(Tok(P), "|=") ∨ ForeignContractStart(P)
// ──────────────────────────────────────────────
// Γ ⊢ ParseContractClauseOpt(P) ⇓ (P, ⊥)
//
// (Parse-ContractClauseOpt-Yes)
// IsOp(Tok(P), "|=")    Γ ⊢ ParseContractBody(Advance(P)) ⇓ (P_1, clause)
// ────────────────────────────────────────────────────────────────
// Γ ⊢ ParseContractClauseOpt(P) ⇓ (P_1, clause)
//
// (Parse-ContractBody-PostOnly)
// IsOp(Tok(P), "=>")    Γ ⊢ ParsePredicateExpr(Advance(P)) ⇓ (P_1, post)
// ────────────────────────────────────────────────────────────────
// Γ ⊢ ParseContractBody(P) ⇓ (P_1, ⟨⊥, post⟩)
//
// (Parse-ContractBody-PrePost)
// Γ ⊢ ParsePredicateExpr(P) ⇓ (P_1, pre)    IsOp(Tok(P_1), "=>")
// Γ ⊢ ParsePredicateExpr(Advance(P_1)) ⇓ (P_2, post)
// ────────────────────────────────────────────────────────────────
// Γ ⊢ ParseContractBody(P) ⇓ (P_2, ⟨pre, post⟩)
//
// (Parse-ContractBody-PreOnly)
// Γ ⊢ ParsePredicateExpr(P) ⇓ (P_1, pre)    ¬ IsOp(Tok(P_1), "=>")
// ────────────────────────────────────────────────────────────────
// Γ ⊢ ParseContractBody(P) ⇓ (P_1, ⟨pre, ⊥⟩)
//
// ForeignContractStart(P) ⇔ IsOp(Tok(P), "|=") ∧ IsOp(Tok(Advance(P)), "@")
//   ∧ IsIdent(Tok(Advance(Advance(P))))
//   ∧ Lexeme(Tok(Advance(Advance(P)))) ∈ {`foreign_assumes`, `foreign_ensures`}

// =============================================================================
// SOURCE CODE MAPPING
// =============================================================================

/*
SOURCE: cursive-bootstrap/src/02_syntax/parser_items.cpp (lines 362-405)
FUNCTION: ParseContractClauseOpt

ORIGINAL:
ParseElemResult<std::optional<ContractClause>> ParseContractClauseOpt(Parser parser) {
  // Skip any newlines before contract
  while (Tok(parser) && Tok(parser)->kind == TokenKind::Newline) {
    Advance(parser);
  }
  if (!IsOp(parser, "|=")) {
    return {parser, std::nullopt};
  }

  SPEC_RULE("Parse-Contract-Clause");
  Parser start = parser;
  Parser next = parser;
  Advance(next);  // consume |=

  ContractClause clause;

  // Check for => (postcondition only case)
  if (IsOp(next, "=>")) {
    Advance(next);
    ParseElemResult<ExprPtr> post = ParseExpr(next);
    clause.postcondition = post.elem;
    clause.span = SpanBetween(start, post.parser);
    return {post.parser, clause};
  }

  // Parse precondition
  ParseElemResult<ExprPtr> pre = ParseExpr(next);
  clause.precondition = pre.elem;
  next = pre.parser;

  // Check for => postcondition
  if (IsOp(next, "=>")) {
    Advance(next);
    ParseElemResult<ExprPtr> post = ParseExpr(next);
    clause.postcondition = post.elem;
    next = post.parser;
  }

  clause.span = SpanBetween(start, next);
  return {next, clause};
}

IMPLEMENTATION NOTES:
- Contract starts with |= operator
- Three forms:
  1. |= pre              -- precondition only
  2. |= pre => post      -- precondition and postcondition
  3. |= => post          -- postcondition only (precondition is true)
- Must distinguish from foreign contracts (|= @foreign_assumes/ensures)
*/

// =============================================================================
// TYPE DEFINITIONS USED
// =============================================================================

/*
STRUCT: ContractClause
struct ContractClause {
  std::shared_ptr<Expr> precondition;   // nullable
  std::shared_ptr<Expr> postcondition;  // nullable
  core::Span span;
};

RESULT TYPE:
ParseElemResult<std::optional<ContractClause>>
*/

// =============================================================================
// GRAMMAR
// =============================================================================

/*
ContractClause ::= '|=' ContractBody
ContractBody ::=
    '=>' PredicateExpr           -- postcondition only
  | PredicateExpr '=>' PredicateExpr  -- pre and post
  | PredicateExpr                     -- precondition only

PredicateExpr ::= Expr  -- must be pure (no capability calls, no mutation)
*/

// =============================================================================
// CONTRACT INTRINSICS
// =============================================================================

/*
Available in postconditions (right of =>):
- @result     -- references the return value
- @entry(expr) -- captures the value of expr at procedure entry

@result:
  - Only valid in postcondition context
  - Type is the procedure's return type

@entry(expr):
  - Valid in postcondition context
  - expr is evaluated at procedure entry
  - Result type must satisfy BitcopyType or CloneType

Example:
procedure increment(~!) -> i32
    |= self.value >= 0 => @result > @entry(self.value)
{
    self.value = self.value + 1
    return self.value
}
*/

// =============================================================================
// PURITY CONSTRAINTS
// =============================================================================

/*
Contract predicates MUST be pure:
1. MUST NOT invoke any procedure that accepts capability parameters
2. MUST NOT mutate state observable outside the expression's evaluation
3. Built-in operators on primitive types are always pure

WRONG - contract calls capability method:
procedure example(ctx: Context, x: i32) -> i32
    |= ctx.fs~>exists("file.txt")  // ERROR: impure
{
    return x
}

CORRECT - only pure expressions:
procedure example(x: i32) -> i32
    |= x > 0
{
    return x
}
*/

// =============================================================================
// VERIFICATION MODES
// =============================================================================

/*
Static verification is REQUIRED by default. If the contract cannot be
statically proven, the program is ill-formed.

To allow runtime verification, use [[dynamic]]:

[[dynamic]]
procedure user_input_divide(a: i32, b: i32) -> i32
    |= b != 0
{
    return a / b
}
*/

// =============================================================================
// EXAMPLES
// =============================================================================

/*
// Precondition only
procedure divide(a: i32, b: i32) -> i32
    |= b != 0
{
    return a / b
}

// Postcondition only (using @result)
procedure abs(x: i32) -> i32
    |= => @result >= 0
{
    return if x < 0 { -x } else { x }
}

// Both precondition and postcondition
procedure clamp(x: i32, min: i32, max: i32) -> i32
    |= min <= max => @result >= min && @result <= max
{
    return if x < min { min } else if x > max { max } else { x }
}

// Using @entry to compare against initial value
procedure increment(~!) -> i32
    |= self.value >= 0 => @result > @entry(self.value)
{
    self.value = self.value + 1
    return self.value
}
*/

// =============================================================================
// FOREIGN CONTRACT DISTINCTION
// =============================================================================

/*
Normal contract: |= precondition => postcondition
Foreign contract: |= @foreign_assumes(...) or |= @foreign_ensures(...)

The parser must distinguish these. A contract starting with |= followed by
@ and then foreign_assumes or foreign_ensures is a foreign contract, not
a normal contract.

See foreign_contract_clause.cpp for foreign contract parsing.
*/

// =============================================================================
// IMPLEMENTATION CHECKLIST
// =============================================================================
// [ ] ParseContractClauseOpt function
//     - Skip leading newlines
//     - Check for |= operator
//     - Check NOT a foreign contract (|= @foreign_...)
//     - Parse contract body (pre, post, or both)
//     - Return nullopt if no contract
// [ ] ParseContractBody function
//     - Check for => (postcondition only)
//     - Otherwise parse precondition
//     - Check for => postcondition after precondition
// [ ] IsForeignContractStart helper
//     - Check for |= @ foreign_assumes/ensures pattern

// =============================================================================
// PSEUDO-IMPLEMENTATION
// =============================================================================

/*
ParseElemResult<std::optional<ContractClause>> ParseContractClauseOpt(Parser parser) {
  SkipNewlines(parser);

  if (!IsOp(parser, "|=")) {
    return {parser, std::nullopt};
  }

  // Check for foreign contract
  if (IsForeignContractStart(parser)) {
    return {parser, std::nullopt};
  }

  Parser start = parser;
  Advance(parser);  // consume |=

  ContractClause clause;

  // Check for => (postcondition only)
  if (IsOp(parser, "=>")) {
    Advance(parser);
    auto post = ParseExpr(parser);
    clause.postcondition = post.elem;
    clause.span = SpanBetween(start, post.parser);
    return {post.parser, clause};
  }

  // Parse precondition
  auto pre = ParseExpr(parser);
  clause.precondition = pre.elem;
  parser = pre.parser;

  // Check for => postcondition
  if (IsOp(parser, "=>")) {
    Advance(parser);
    auto post = ParseExpr(parser);
    clause.postcondition = post.elem;
    parser = post.parser;
  }

  clause.span = SpanBetween(start, parser);
  return {parser, clause};
}
*/
