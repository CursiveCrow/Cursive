// =============================================================================
// MIGRATION MAPPING: range.cpp
// =============================================================================
// This file should contain parsing logic for range expressions.
//
// SPEC REFERENCE: C0updated.md, Section 3.3.8.1, Lines 4986-5030
// =============================================================================
//
// EXPR START PREDICATE (Lines 4988-4990):
// -----------------------------------------------------------------------------
// ExprStart(t) ⇔ IsIdent(t) ∨ (t ∈ LiteralToken) ∨ IsPunc(t, "(") ∨
//                IsPunc(t, "[") ∨ IsPunc(t, "{") ∨ IsOp(t, "|") ∨
//                IsOp(t, "!") ∨ IsOp(t, "-") ∨ IsOp(t, "&") ∨
//                IsOp(t, "*") ∨ IsOp(t, "^") ∨
//                IsKw(t, "if") ∨ IsKw(t, "match") ∨ IsKw(t, "loop") ∨
//                IsKw(t, "unsafe") ∨ IsKw(t, "move") ∨ IsKw(t, "transmute") ∨
//                IsKw(t, "widen") ∨ IsKw(t, "parallel") ∨ IsKw(t, "spawn") ∨
//                IsKw(t, "dispatch") ∨ IsKw(t, "yield") ∨ IsKw(t, "sync") ∨
//                IsKw(t, "race") ∨ IsKw(t, "all")
//
// FORMAL RULES FROM SPEC:
// -----------------------------------------------------------------------------
// **(Parse-Range-To)** Lines 4992-4995
// IsOp(Tok(P), "..")    Γ ⊢ ParseLogicalOr(Advance(P)) ⇓ (P_1, e)
// ────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseRange(P) ⇓ (P_1, Range(To, ⊥, e))
//
// **(Parse-Range-ToInc)** Lines 4997-5000
// IsOp(Tok(P), "..=")    Γ ⊢ ParseLogicalOr(Advance(P)) ⇓ (P_1, e)
// ────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseRange(P) ⇓ (P_1, Range(ToInclusive, ⊥, e))
//
// **(Parse-Range-Full)** Lines 5002-5005
// IsOp(Tok(P), "..")    Tok(Advance(P)) ∉ ExprStart
// ────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseRange(P) ⇓ (Advance(P), Range(Full, ⊥, ⊥))
//
// **(Parse-Range-Lhs)** Lines 5007-5010
// Γ ⊢ ParseLogicalOr(P) ⇓ (P_1, e_0)
// Γ ⊢ ParseRangeTail(P_1, e_0) ⇓ (P_2, e)
// ────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseRange(P) ⇓ (P_2, e)
//
// **(Parse-RangeTail-None)** Lines 5012-5015
// ¬ (IsOp(Tok(P), "..") ∨ IsOp(Tok(P), "..="))
// ────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseRangeTail(P, e) ⇓ (P, e)
//
// **(Parse-RangeTail-From)** Lines 5017-5020
// IsOp(Tok(P), "..")    Tok(Advance(P)) ∉ ExprStart
// ────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseRangeTail(P, e_0) ⇓ (Advance(P), Range(From, e_0, ⊥))
//
// **(Parse-RangeTail-Excl)** Lines 5022-5025
// IsOp(Tok(P), "..")    Tok(Advance(P)) ∈ ExprStart
// Γ ⊢ ParseLogicalOr(Advance(P)) ⇓ (P_1, e_1)
// ────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseRangeTail(P, e_0) ⇓ (P_1, Range(Exclusive, e_0, e_1))
//
// **(Parse-RangeTail-Incl)** Lines 5027-5030
// IsOp(Tok(P), "..=")    Γ ⊢ ParseLogicalOr(Advance(P)) ⇓ (P_1, e_1)
// ────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseRangeTail(P, e_0) ⇓ (P_1, Range(Inclusive, e_0, e_1))
//
// RANGE KINDS:
// -----------------------------------------------------------------------------
// - Full:        ..        (no bounds)
// - To:          ..e       (exclusive upper bound only)
// - ToInclusive: ..=e      (inclusive upper bound only)
// - From:        e..       (lower bound only)
// - Exclusive:   e_0..e_1  (both bounds, exclusive upper)
// - Inclusive:   e_0..=e_1 (both bounds, inclusive upper)
//
// SEMANTICS:
// - Range operators have LOWEST precedence (below logical or)
// - Range is NOT associative: `a..b..c` is a syntax error
// - ExprStart predicate determines if token can start an expression
// - Used to distinguish `..` (Full) from `..e` (To)
// - Used to distinguish `e..` (From) from `e..f` (Exclusive)
//
// =============================================================================
// SOURCE FILE: cursive-bootstrap/src/02_syntax/parser_expr.cpp
// =============================================================================
//
// CONTENT TO MIGRATE:
// -----------------------------------------------------------------------------
//
// 1. IsExprStart predicate (helper needed)
//    ─────────────────────────────────────────────────────────────────────────
//    Purpose: Determines if a token can start an expression
//    Used to distinguish between range variants when lookahead is needed
//    See ExprStart definition in spec (Lines 4988-4990)
//
// 2. ParseRange function (Lines 479-517)
//    ─────────────────────────────────────────────────────────────────────────
//    Lines 481-491: ToInclusive range (..=e)
//      - Check: IsOp(parser, "..=")
//      - Advance past "..="
//      - Parse RHS via ParseLogicalOr
//      - Create RangeExpr with kind=ToInclusive, lhs=nullptr, rhs=parsed
//      - Span: SpanBetween(start, rhs.parser)
//      - SPEC_RULE: "Parse-Range-ToInc"
//
//    Lines 493-511: To range (..e) or Full range (..)
//      - Check: IsOp(parser, "..")
//      - Advance past ".."
//      - Check if next token is ExprStart
//      - If NOT ExprStart:
//        - Create RangeExpr with kind=Full, lhs=nullptr, rhs=nullptr
//        - Span: SpanBetween(start, next)
//        - SPEC_RULE: "Parse-Range-Full"
//      - If IS ExprStart:
//        - Parse RHS via ParseLogicalOr
//        - Create RangeExpr with kind=To, lhs=nullptr, rhs=parsed
//        - Span: SpanBetween(start, rhs.parser)
//        - SPEC_RULE: "Parse-Range-To"
//
//    Lines 514-516: LHS-first parsing
//      - Parse LHS via ParseLogicalOr
//      - Delegate to ParseRangeTail with LHS
//      - SPEC_RULE: "Parse-Range-Lhs"
//
// 3. ParseRangeTail function (Lines 519-557)
//    ─────────────────────────────────────────────────────────────────────────
//    Lines 523-526: No range operator
//      - Check: !(IsOp(parser, "..") || IsOp(parser, "..="))
//      - If no range operator, return LHS unchanged
//      - SPEC_RULE: "Parse-RangeTail-None"
//
//    Lines 528-546: Exclusive range (e..) or (e..f)
//      - Check: IsOp(parser, "..")
//      - Advance past ".."
//      - Check if next token is ExprStart
//      - If NOT ExprStart:
//        - Create RangeExpr with kind=From, lhs=given, rhs=nullptr
//        - Span: SpanBetween(start, after)
//        - SPEC_RULE: "Parse-RangeTail-From"
//      - If IS ExprStart:
//        - Parse RHS via ParseLogicalOr
//        - Create RangeExpr with kind=Exclusive, lhs=given, rhs=parsed
//        - Span: SpanBetween(start, rhs.parser)
//        - SPEC_RULE: "Parse-RangeTail-Excl"
//
//    Lines 549-557: Inclusive range (e..=f)
//      - Check: IsOp(parser, "..=") (implicit from else branch)
//      - Advance past "..="
//      - Parse RHS via ParseLogicalOr
//      - Create RangeExpr with kind=Inclusive, lhs=given, rhs=parsed
//      - Span: SpanBetween(start, rhs.parser)
//      - SPEC_RULE: "Parse-RangeTail-Incl"
//
// =============================================================================
// DEPENDENCIES:
// =============================================================================
// - ParseLogicalOr function (binary.cpp) for range bounds
// - IsExprStart predicate (helper function)
// - MakeExpr, SpanBetween helpers (expr_common.cpp)
// - IsOp, Tok, Advance helpers (parser utilities)
// - RangeExpr AST node type
// - RangeKind enum: Full, To, ToInclusive, From, Exclusive, Inclusive
//
// =============================================================================
// REFACTORING NOTES:
// =============================================================================
// - ParseRange is the entry point for expression parsing (called by ParseExpr)
// - Range operators checked BEFORE delegating to lower-precedence parsing
// - Order matters: check "..=" before ".." (longer operator first)
// - IsExprStart lookahead required to distinguish:
//   - ".." (Full) vs "..e" (To)
//   - "e.." (From) vs "e..f" (Exclusive)
// - Span construction uses SpanBetween(start, end_parser) throughout
// - ParseRangeTail is NOT recursive; ranges do not chain
// - allow_brace, allow_bracket parameters propagate to ParseLogicalOr
// =============================================================================
