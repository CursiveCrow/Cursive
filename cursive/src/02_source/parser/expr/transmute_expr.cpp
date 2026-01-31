// =============================================================================
// MIGRATION MAPPING: transmute_expr.cpp
// =============================================================================
// This file should contain parsing logic for transmute expressions.
//
// SPEC REFERENCE: C0updated.md, Section 3.3.8.6, Lines 5379-5387
// =============================================================================
//
// FORMAL RULES FROM SPEC:
// -----------------------------------------------------------------------------
// **(Parse-Transmute-Expr-ShiftSplit)** Lines 5379-5382
// IsKw(Tok(P), `transmute`)
// IsOp(Tok(Advance(P)), "<")
// G |- ParseType(Advance(Advance(P))) => (P_1, t1)
// IsPunc(Tok(P_1), ",")
// G |- ParseType(Advance(P_1)) => (P_2, t2)
// IsOp(Tok(P_2), ">>")
// P_2' = SplitShiftR(P_2)
// IsOp(Tok(P_2'), ">")
// IsPunc(Tok(Advance(P_2')), "(")
// G |- ParseExpr(Advance(Advance(P_2'))) => (P_3, e)
// IsPunc(Tok(P_3), ")")
// -----------------------------------------------------------------------
// G |- ParsePrimary(P) => (Advance(P_3), TransmuteExpr(t1, t2, e))
//
// **(Parse-Transmute-Expr)** Lines 5384-5387
// IsKw(Tok(P), `transmute`)
// IsOp(Tok(Advance(P)), "<")
// G |- ParseType(Advance(Advance(P))) => (P_1, t1)
// IsPunc(Tok(P_1), ",")
// G |- ParseType(Advance(P_1)) => (P_2, t2)
// IsOp(Tok(P_2), ">")
// IsPunc(Tok(Advance(P_2)), "(")
// G |- ParseExpr(Advance(Advance(P_2))) => (P_3, e)
// IsPunc(Tok(P_3), ")")
// -----------------------------------------------------------------------
// G |- ParsePrimary(P) => (Advance(P_3), TransmuteExpr(t1, t2, e))
//
// GRAMMAR:
// -----------------------------------------------------------------------------
// TransmuteExpr = "transmute" "::" "<" Type "," Type ">" "(" Expr ")"
//
// NOTE: The spec rules show generic angle brackets `<...>`, but the bootstrap
// implementation uses `transmute::<T1, T2>(expr)` syntax with `::` prefix.
//
// SEMANTICS:
// - Transmute reinterprets the bits of a value as a different type
// - MUST appear inside an `unsafe { }` block
// - t1: source type (type being converted FROM)
// - t2: destination type (type being converted TO)
// - e: the expression to transmute
// - Both types must have the same size; violation is undefined behavior
// - No runtime conversion occurs; purely a type-system override
//
// =============================================================================
// SOURCE FILE: cursive-bootstrap/src/02_syntax/parser_expr.cpp
// =============================================================================
//
// CONTENT TO MIGRATE:
// -----------------------------------------------------------------------------
//
// 1. Transmute Expression Parsing (Lines 1821-1916)
//    ---------------------------------------------------------------------------
//
// KEYWORD AND :: CHECK (Lines 1821-1837):
//    Line 1821: Condition check
//      - Check: tok && IsKwTok(*tok, "transmute")
//
//    Lines 1822-1829: Verify :: follows transmute
//      - Parser next = parser;
//      - Advance(next);
//      - if (!IsOp(next, "::")) { emit error, sync, return ErrorExpr }
//
//    Lines 1830-1837: Verify < follows ::
//      - Parser after_colon = next;
//      - Advance(after_colon);
//      - if (!IsOp(after_colon, "<")) { emit error, sync, return ErrorExpr }
//
// TYPE PARAMETERS (Lines 1838-1849):
//    Lines 1838-1840: Parse first type
//      - Parser after_lt = after_colon;
//      - Advance(after_lt);
//      - ParseElemResult<std::shared_ptr<Type>> t1 = ParseType(after_lt);
//
//    Lines 1841-1846: Expect comma, parse second type
//      - if (!IsPunc(t1.parser, ",")) { emit error, sync, return ErrorExpr }
//      - Parser after_comma = t1.parser;
//      - Advance(after_comma);
//      - ParseElemResult<std::shared_ptr<Type>> t2 = ParseType(after_comma);
//
// SHIFT-SPLIT CASE (Lines 1850-1884):
// When t2 ends at ">>" (nested generics), must split into two ">" tokens.
//
//    Lines 1850-1852: Check for >> token
//      - if (IsOp(t2.parser, ">>")) {
//      - SPEC_RULE("Parse-Transmute-Expr-ShiftSplit");
//      - Parser split = SplitShiftR(t2.parser);
//
//    Lines 1853-1868: Process first > and expect (
//      - Parser at_second = split;
//      - Advance(at_second);
//      - if (!IsOp(at_second, ">")) { error }
//      - Parser after_gt = at_second;
//      - Advance(after_gt);
//      - if (!IsPunc(after_gt, "(")) { error }
//
//    Lines 1869-1884: Parse expression and closing paren
//      - Parser after_l = after_gt;
//      - Advance(after_l);
//      - ParseElemResult<ExprPtr> expr = ParseExpr(after_l);
//      - if (!IsPunc(expr.parser, ")")) { error }
//      - Construct TransmuteExpr{t1, t2, expr}
//      - return result
//
// NORMAL CASE (Lines 1886-1916):
//    Lines 1886-1888: Check for single >
//      - if (IsOp(t2.parser, ">")) {
//      - SPEC_RULE("Parse-Transmute-Expr");
//
//    Lines 1888-1912: Process > and parse expression
//      - Parser after_gt = t2.parser;
//      - Advance(after_gt);
//      - if (!IsPunc(after_gt, "(")) { error }
//      - Parse expression
//      - if (!IsPunc(expr.parser, ")")) { error }
//      - Construct TransmuteExpr{t1, t2, expr}
//
//    Lines 1913-1916: Error case (neither > nor >>)
//      - EmitParseSyntaxErr(t2.parser, TokSpan(t2.parser));
//      - SyncStmt(sync);
//      - return ErrorExpr
//
// COMPLETE SOURCE CODE (Lines 1821-1916):
// -----------------------------------------------------------------------------
//   if (tok && IsKwTok(*tok, "transmute")) {
//     Parser next = parser;
//     Advance(next);
//     if (!IsOp(next, "::")) {
//       EmitParseSyntaxErr(next, TokSpan(next));
//       Parser sync = next;
//       SyncStmt(sync);
//       return {sync, MakeExpr(SpanBetween(parser, sync), ErrorExpr{})};
//     }
//     Parser after_colon = next;
//     Advance(after_colon);
//     if (!IsOp(after_colon, "<")) {
//       EmitParseSyntaxErr(after_colon, TokSpan(after_colon));
//       Parser sync = after_colon;
//       SyncStmt(sync);
//       return {sync, MakeExpr(SpanBetween(parser, sync), ErrorExpr{})};
//     }
//     Parser after_lt = after_colon;
//     Advance(after_lt);
//     ParseElemResult<std::shared_ptr<Type>> t1 = ParseType(after_lt);
//     if (!IsPunc(t1.parser, ",")) {
//       EmitParseSyntaxErr(t1.parser, TokSpan(t1.parser));
//       Parser sync = t1.parser;
//       SyncStmt(sync);
//       return {sync, MakeExpr(SpanBetween(parser, sync), ErrorExpr{})};
//     }
//     Parser after_comma = t1.parser;
//     Advance(after_comma);
//     ParseElemResult<std::shared_ptr<Type>> t2 = ParseType(after_comma);
//     if (IsOp(t2.parser, ">>")) {
//       SPEC_RULE("Parse-Transmute-Expr-ShiftSplit");
//       Parser split = SplitShiftR(t2.parser);
//       Parser at_second = split;
//       Advance(at_second);
//       if (!IsOp(at_second, ">")) {
//         EmitParseSyntaxErr(at_second, TokSpan(at_second));
//         Parser sync = at_second;
//         SyncStmt(sync);
//         return {sync, MakeExpr(SpanBetween(parser, sync), ErrorExpr{})};
//       }
//       Parser after_gt = at_second;
//       Advance(after_gt);
//       if (!IsPunc(after_gt, "(")) {
//         EmitParseSyntaxErr(after_gt, TokSpan(after_gt));
//         Parser sync = after_gt;
//         SyncStmt(sync);
//         return {sync, MakeExpr(SpanBetween(parser, sync), ErrorExpr{})};
//       }
//       Parser after_l = after_gt;
//       Advance(after_l);
//       ParseElemResult<ExprPtr> expr = ParseExpr(after_l);
//       if (!IsPunc(expr.parser, ")")) {
//         EmitParseSyntaxErr(expr.parser, TokSpan(expr.parser));
//         Parser sync = expr.parser;
//         SyncStmt(sync);
//         return {sync, MakeExpr(SpanBetween(parser, sync), ErrorExpr{})};
//       }
//       Parser after = expr.parser;
//       Advance(after);
//       TransmuteExpr trans;
//       trans.from = t1.elem;
//       trans.to = t2.elem;
//       trans.value = expr.elem;
//       return {after, MakeExpr(SpanBetween(parser, after), trans)};
//     }
//     if (IsOp(t2.parser, ">")) {
//       SPEC_RULE("Parse-Transmute-Expr");
//       Parser after_gt = t2.parser;
//       Advance(after_gt);
//       if (!IsPunc(after_gt, "(")) {
//         EmitParseSyntaxErr(after_gt, TokSpan(after_gt));
//         Parser sync = after_gt;
//         SyncStmt(sync);
//         return {sync, MakeExpr(SpanBetween(parser, sync), ErrorExpr{})};
//       }
//       Parser after_l = after_gt;
//       Advance(after_l);
//       ParseElemResult<ExprPtr> expr = ParseExpr(after_l);
//       if (!IsPunc(expr.parser, ")")) {
//         EmitParseSyntaxErr(expr.parser, TokSpan(expr.parser));
//         Parser sync = expr.parser;
//         SyncStmt(sync);
//         return {sync, MakeExpr(SpanBetween(parser, sync), ErrorExpr{})};
//       }
//       Parser after = expr.parser;
//       Advance(after);
//       TransmuteExpr trans;
//       trans.from = t1.elem;
//       trans.to = t2.elem;
//       trans.value = expr.elem;
//       return {after, MakeExpr(SpanBetween(parser, after), trans)};
//     }
//     EmitParseSyntaxErr(t2.parser, TokSpan(t2.parser));
//     Parser sync = t2.parser;
//     SyncStmt(sync);
//     return {sync, MakeExpr(SpanBetween(parser, sync), ErrorExpr{})};
//   }
//
// 2. SplitShiftR Helper Function (location varies)
//    ---------------------------------------------------------------------------
//    Splits a ">>" token into two ">" tokens.
//    Required for parsing nested generics: transmute::<T, Vec<i32>>(...)
//    When the inner generic ends with ">", combined with outer ">" makes ">>".
//
// =============================================================================
// DEPENDENCIES:
// =============================================================================
// - ParseType(Parser) -> ParseElemResult<std::shared_ptr<Type>>
//   Parses a type expression
// - ParseExpr(Parser) -> ParseElemResult<ExprPtr>
//   Parses any expression
// - SplitShiftR(Parser) -> Parser
//   Splits ">>" token into two ">" tokens; returns parser at first ">"
// - MakeExpr(Span, ExprNode) -> ExprPtr
//   Constructs an expression node with the given span
// - SpanBetween(Parser start, Parser end) -> Span
//   Creates span covering from start position to end position
// - IsKwTok(Token, string_view) -> bool
//   Checks if token is a keyword with given lexeme
// - IsOp(Parser, string_view) -> bool
//   Checks if current token is operator with given lexeme
// - IsPunc(Parser, string_view) -> bool
//   Checks if current token is punctuator with given lexeme
// - Advance(Parser&) -> void
//   Moves parser forward one token
// - SyncStmt(Parser&) -> void
//   Error recovery: skips to statement boundary
// - EmitParseSyntaxErr(Parser, Span) -> void
//   Emits syntax error diagnostic
// - TransmuteExpr AST node type
//   Fields: from (Type), to (Type), value (ExprPtr)
//
// =============================================================================
// REFACTORING NOTES:
// =============================================================================
// - Complex parsing with multiple error points
// - The ">>" split handling is critical for nested generics
// - Implementation uses `transmute::<T1, T2>` syntax (with ::)
// - Spec shows `transmute<T1, T2>` syntax (without ::)
// - VERIFY: Which syntax is canonical? Spec or bootstrap?
// - Consider extracting to standalone function:
//     ParseElemResult<ExprPtr> ParseTransmuteExpr(Parser parser);
// - Would be called from ParsePrimary when 'transmute' keyword detected
// - Error recovery at each step prevents cascade failures
// - The SplitShiftR mechanism is shared with generic type parsing
// =============================================================================
