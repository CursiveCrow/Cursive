// =============================================================================
// MIGRATION MAPPING: Where Clause Parsing
// =============================================================================
// Destination: cursive/src/02_source/parser/item/where_clause.cpp
// Source:      cursive-bootstrap/src/02_syntax/parser_items.cpp (lines 139-202)
//
// PURPOSE:
// Parse where clauses for type constraints: where Bitcopy(T), Clone(U)
// Supports both predicate syntax and class bound syntax.
// =============================================================================

// =============================================================================
// SPEC RULES IMPLEMENTED
// =============================================================================
// From C0updated.md Section 3.3.6.13:
//
// (Parse-WhereClauseOpt-None)
// ¬ IsKw(Tok(P), `where`) ∨ IsPunc(Tok(Advance(P)), "{")
// ──────────────────────────────────────────────
// Γ ⊢ ParseWhereClauseOpt(P) ⇓ (P, ⊥)
//
// (Parse-WhereClauseOpt-Yes)
// IsKw(Tok(P), `where`)    ¬ IsPunc(Tok(Advance(P)), "{")
// Γ ⊢ ParseWherePredList(Advance(P)) ⇓ (P_1, preds)
// ────────────────────────────────────────────────────────────
// Γ ⊢ ParseWhereClauseOpt(P) ⇓ (P_1, preds)
//
// (Parse-WherePredList-Cons)
// Γ ⊢ ParseWherePred(P) ⇓ (P_1, p)
// Γ ⊢ ParseWherePredListTail(P_1, [p]) ⇓ (P_2, ps)
// ────────────────────────────────────────────────────────────
// Γ ⊢ ParseWherePredList(P) ⇓ (P_2, ps)
//
// (Parse-WherePredListTail-End)
// ¬ IsTerminator(Tok(P))
// ──────────────────────────────────────────────
// Γ ⊢ ParseWherePredListTail(P, ps) ⇓ (P, ps)
//
// (Parse-WherePredListTail-Cons)
// IsTerminator(Tok(P))    Γ ⊢ ParseWherePred(Advance(P)) ⇓ (P_1, p)
// Γ ⊢ ParseWherePredListTail(P_1, ps ++ [p]) ⇓ (P_2, ps')
// ────────────────────────────────────────────────────────────
// Γ ⊢ ParseWherePredListTail(P, ps) ⇓ (P_2, ps')
//
// (Parse-WherePred-Predicate)
// Γ ⊢ ParseIdent(P) ⇓ (P_1, name)    IsPredName(name)
// IsPunc(Tok(P_1), "(")    Γ ⊢ ParseType(Advance(P_1)) ⇓ (P_2, ty)
// IsPunc(Tok(P_2), ")")
// ────────────────────────────────────────────────────────────
// Γ ⊢ ParseWherePred(P) ⇓ (Advance(P_2), PredWherePred(name, ty))
//
// IsPredName(name) ⇔ name ∈ {`Bitcopy`, `Clone`, `Drop`, `FfiSafe`}

// =============================================================================
// SOURCE CODE MAPPING
// =============================================================================

/*
SOURCE: cursive-bootstrap/src/02_syntax/parser_items.cpp (lines 139-202)
FUNCTION: ParseWhereClauseOpt

ORIGINAL:
ParseElemResult<std::optional<WhereClause>> ParseWhereClauseOpt(Parser parser) {
  // Skip any newlines before where
  while (Tok(parser) && Tok(parser)->kind == TokenKind::Newline) {
    Advance(parser);
  }
  if (!IsKw(parser, "where")) {
    return {parser, std::nullopt};
  }

  SPEC_RULE("Parse-Where-Clause");
  Parser start = parser;
  Parser next = parser;
  Advance(next);  // consume where

  // Skip newlines after where
  while (Tok(next) && Tok(next)->kind == TokenKind::Newline) {
    Advance(next);
  }

  WhereClause clause;

  // Parse first predicate
  ParseElemResult<Identifier> first_name = ParseIdent(next);
  ParseElemResult<std::vector<TypeBound>> first_bounds = ParseTypeBounds(first_name.parser);
  WherePredicate first_pred;
  first_pred.type_param = first_name.elem;
  first_pred.bounds = first_bounds.elem;
  first_pred.span = SpanBetween(next, first_bounds.parser);
  clause.predicates.push_back(first_pred);
  next = first_bounds.parser;

  // Parse additional predicates after newline/terminator
  const Token* tok = Tok(next);
  while (tok && (tok->kind == TokenKind::Newline || tok->lexeme == ";")) {
    Parser after_term = next;
    Advance(after_term);
    const Token* next_tok = Tok(after_term);
    if (next_tok && IsIdentTok(*next_tok) &&
        !IsPunc(after_term, "{") && !IsKw(after_term, "where")) {
      // Check if followed by <: (indicates another predicate)
      Parser probe = after_term;
      Advance(probe);
      if (IsOp(probe, "<:")) {
        next = after_term;
        ParseElemResult<Identifier> pred_name = ParseIdent(next);
        ParseElemResult<std::vector<TypeBound>> pred_bounds = ParseTypeBounds(pred_name.parser);
        WherePredicate pred;
        pred.type_param = pred_name.elem;
        pred.bounds = pred_bounds.elem;
        pred.span = SpanBetween(next, pred_bounds.parser);
        clause.predicates.push_back(pred);
        next = pred_bounds.parser;
        tok = Tok(next);
        continue;
      }
    }
    break;
  }

  clause.span = SpanBetween(start, next);
  return {next, clause};
}

IMPLEMENTATION NOTES:
- "where" is a contextual keyword (checked via IsKw helper)
- IMPORTANT: Distinguish "where" for constraints from "where { ... }" for invariants
  - If "where" followed by "{", it's a type invariant, not a where clause
- Predicates separated by newlines or semicolons
- Class bound syntax: T <: Clone
- Predicate syntax (spec): Bitcopy(T), Clone(T), Drop(T), FfiSafe(T)
- Source implementation uses class bound syntax; spec prefers predicate syntax
*/

// =============================================================================
// TYPE DEFINITIONS USED
// =============================================================================

/*
STRUCT: WherePredicate
struct WherePredicate {
  Identifier type_param;           // The type parameter being constrained
  std::vector<TypeBound> bounds;   // Class bounds: T <: Clone, Eq
  core::Span span;
};

// Alternative for predicate syntax:
// struct PredicatePred {
//   Identifier pred_name;  // e.g., "Bitcopy", "Clone"
//   std::shared_ptr<Type> type;
//   core::Span span;
// };

STRUCT: WhereClause
struct WhereClause {
  std::vector<WherePredicate> predicates;
  core::Span span;
};

RESULT TYPE:
ParseElemResult<std::optional<WhereClause>>
*/

// =============================================================================
// GRAMMAR
// =============================================================================

/*
WhereClause ::= 'where' WherePredicate (TERMINATOR WherePredicate)*

WherePredicate ::=
    TypeBoundPred      -- T <: Class1, Class2
  | PredicatePred      -- Bitcopy(T)

TypeBoundPred ::= IDENTIFIER '<:' ClassBound (',' ClassBound)*
PredicatePred ::= PredName '(' Type ')'
PredName ::= 'Bitcopy' | 'Clone' | 'Drop' | 'FfiSafe'

TERMINATOR ::= ';' | NEWLINE
*/

// =============================================================================
// DISAMBIGUATION: WHERE CLAUSE vs TYPE INVARIANT
// =============================================================================

/*
The "where" keyword has two meanings:
1. Where clause for type constraints: where T <: Clone
2. Type invariant: where { predicate_expr }

Disambiguation rule:
- If "where" is followed by "{", it's a type invariant
- Otherwise, it's a where clause

This is handled in the parse rules:
(Parse-WhereClauseOpt-None)
¬ IsKw(Tok(P), `where`) ∨ IsPunc(Tok(Advance(P)), "{")
                          ^^^^^^^^^^^^^^^^^^^^^^^^
                          This check excludes invariants
*/

// =============================================================================
// EXAMPLES
// =============================================================================

/*
// Single predicate with class bound syntax
procedure clone<T>(x: T) -> T
    where T <: Clone
{
    return x~>clone()
}

// Multiple predicates (newline separated)
procedure process<T; U>(x: T, y: U) -> ()
    where T <: Comparable
    U <: Printable
{
    // ...
}

// Predicate syntax (spec-preferred)
procedure copy_value<T>(x: T) -> T
    where Bitcopy(T)
{
    return x
}

// Multiple predicate-style bounds
procedure clone_pair<T; U>(p: Pair<T, U>) -> Pair<T, U>
    where Clone(T)
    Clone(U)
{
    return Pair{ first: p.first~>clone(), second: p.second~>clone() }
}
*/

// =============================================================================
// IMPLEMENTATION CHECKLIST
// =============================================================================
// [ ] ParseWhereClauseOpt function
//     - Skip leading newlines
//     - Check for "where" keyword
//     - Check NOT followed by "{" (would be type invariant)
//     - Parse first predicate
//     - Loop on terminator (newline or ;) for additional predicates
//     - Return nullopt if no where clause
// [ ] ParseWherePredicate function
//     - Parse identifier (type param name or predicate name)
//     - Check for <: (class bound syntax) or ( (predicate syntax)
//     - Parse bounds or predicate argument accordingly
// [ ] IsPredName helper
//     - Return true for: Bitcopy, Clone, Drop, FfiSafe

// =============================================================================
// PSEUDO-IMPLEMENTATION
// =============================================================================

/*
ParseElemResult<std::optional<WhereClause>> ParseWhereClauseOpt(Parser parser) {
  SkipNewlines(parser);

  if (!IsKw(parser, "where")) {
    return {parser, std::nullopt};
  }

  // Disambiguate: where clause vs type invariant
  Parser probe = parser;
  Advance(probe);
  SkipNewlines(probe);
  if (IsPunc(probe, "{")) {
    // This is a type invariant, not a where clause
    return {parser, std::nullopt};
  }

  Parser start = parser;
  Advance(parser);  // consume where
  SkipNewlines(parser);

  WhereClause clause;

  // Parse first predicate
  auto first = ParseWherePredicate(parser);
  clause.predicates.push_back(first.elem);
  parser = first.parser;

  // Parse additional predicates after terminators
  while (IsTerminator(parser)) {
    Advance(parser);
    if (!LooksLikePredicate(parser)) break;
    auto pred = ParseWherePredicate(parser);
    clause.predicates.push_back(pred.elem);
    parser = pred.parser;
  }

  clause.span = SpanBetween(start, parser);
  return {parser, clause};
}
*/
