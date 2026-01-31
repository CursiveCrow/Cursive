// =============================================================================
// MIGRATION MAPPING: refinement_clause.cpp
// =============================================================================
// This file should contain parsing logic for type refinement clauses:
// T where { predicate } representing types with compile-time predicates.
//
// SPEC REFERENCE: C0updated.md, Section 3.3.7, Lines 4674-4686
// =============================================================================
//
// FORMAL RULES FROM SPEC:
// -----------------------------------------------------------------------------
//
// **(Parse-RefinementOpt-None)** Lines 4676-4679
// ¬ IsKw(Tok(P), `where`)
// ────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseRefinementOpt(P) ⇓ (P, ⊥)
//
// **(Parse-RefinementOpt-Yes)** Lines 4681-4684
// IsKw(Tok(P), `where`)
// IsPunc(Tok(Advance(P)), "{")
// Γ ⊢ ParsePredicateExpr(Advance(Advance(P))) ⇓ (P_1, pred)
// IsPunc(Tok(P_1), "}")
// ────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseRefinementOpt(P) ⇓ (Advance(P_1), pred)
//
// PREDICATE EXPRESSION (Line 4686):
// -----------------------------------------------------------------------------
// ParsePredicateExpr(P) ⇓ (P_1, e) ⇔ Γ ⊢ ParseExpr(P) ⇓ (P_1, e)
//
// SEMANTICS:
// - Refinement types add compile-time predicates to base types
// - Syntax: BaseType where { predicate_expression }
// - The predicate must be pure (no side effects, no capability access)
// - `self` refers to a value of the base type in the predicate
// - Examples: i32 where { self > 0 }, f64 where { self >= 0.0f64 }
//
// =============================================================================
// SOURCE FILE: cursive-bootstrap/src/02_syntax/parser_types.cpp
// =============================================================================
//
// CONTENT TO MIGRATE:
// -----------------------------------------------------------------------------
//
// 1. TypeWhereTok predicate (in keyword_policy.h or similar):
//    ─────────────────────────────────────────────────────────────────────────
//    bool TypeWhereTok(const Token& tok) {
//      return tok.kind == TokenKind::Keyword && tok.lexeme == "where";
//    }
//
// 2. Refinement parsing in ParseType (Lines 751-778)
//    ─────────────────────────────────────────────────────────────────────────
//    const Token* where_tok = Tok(out);
//    if (where_tok && TypeWhereTok(*where_tok)) {
//      SPEC_RULE("Parse-Refinement-Type");
//      Parser after_where = out;
//      Advance(after_where);  // consume where
//      if (!IsPunc(after_where, "{")) {
//        EmitParseSyntaxErr(after_where, TokSpan(after_where));
//        Parser sync = after_where;
//        SyncType(sync);
//        return {sync, merged};
//      }
//      Parser after_l = after_where;
//      Advance(after_l);  // consume {
//      ParseElemResult<std::shared_ptr<Expr>> pred = ParseExpr(after_l);
//      if (!IsPunc(pred.parser, "}")) {
//        EmitParseSyntaxErr(pred.parser, TokSpan(pred.parser));
//        Parser sync = pred.parser;
//        SyncType(sync);
//        return {sync, merged};
//      }
//      Parser after_r = pred.parser;
//      Advance(after_r);  // consume }
//      TypeRefine refine;
//      refine.base = merged;
//      refine.predicate = pred.elem;
//      merged = MakeTypeNode(SpanBetween(start, after_r), refine);
//      out = after_r;
//    }
//
// PARSING FLOW:
// -----------------------------------------------------------------------------
//   1. After parsing base type (with optional permission and union)
//   2. Check for "where" keyword
//   3. If found:
//      a. Advance past "where"
//      b. Expect "{" opening brace
//      c. Parse predicate expression via ParseExpr
//      d. Expect "}" closing brace
//      e. Wrap base type in TypeRefine node
//   4. If not found, return base type unchanged
//
// ERROR HANDLING:
// -----------------------------------------------------------------------------
//   - If "where" is not followed by "{":
//     - Emit syntax error
//     - Synchronize and return base type
//   - If predicate is not followed by "}":
//     - Emit syntax error
//     - Synchronize and return base type
//
// DEPENDENCIES:
// =============================================================================
// - ParseExpr function (for predicate expression)
// - TypeWhereTok predicate (keyword_policy.h)
// - MakeTypeNode helper (type_common.cpp)
// - TypeRefine AST node type
// - SyncType recovery helper
// - Token predicates: IsPunc
// - Parser utilities: Tok, Advance, SpanBetween, TokSpan
// - EmitParseSyntaxErr diagnostic helper
//
// =============================================================================
// REFACTORING NOTES:
// =============================================================================
// - "where" is overloaded: also used for where clauses on generics
// - Context distinguishes: type refinement uses `where {`, generic uses `where`
// - Predicate must be pure - validated during semantic analysis
// - The `self` identifier in predicates refers to the value being typed
// - @entry and @result are NOT valid in type predicates (only contracts)
// - Consider whether nested refinements are allowed: (i32 where {...}) where {...}
// - Span covers from base type start to closing "}"
// =============================================================================
