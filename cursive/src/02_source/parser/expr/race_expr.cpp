// =============================================================================
// MIGRATION MAPPING: race_expr.cpp
// =============================================================================
// This file should contain parsing logic for race expressions, which run
// multiple async operations concurrently and return/yield the first to complete.
//
// SPEC REFERENCE: C0updated.md, Section 3.3.8.6, Lines 5299-5302
// HELPER RULES: Section 3.3.8.7, Lines 5755-5795
// =============================================================================
//
// FORMAL RULE - Parse-Race-Expr (Lines 5299-5302):
// -----------------------------------------------------------------------------
// IsKw(Tok(P), `race`)
// IsPunc(Tok(Advance(P)), "{")
// Gamma |- ParseRaceArms(Advance(Advance(P))) => (P_1, arms)
// IsPunc(Tok(P_1), "}")
// ────────────────────────────────────────────────────────────────────────────
// Gamma |- ParsePrimary(P) => (Advance(P_1), RaceExpr(arms))
//
// HELPER RULES - Race Arms (Lines 5757-5795):
// -----------------------------------------------------------------------------
//
// **(Parse-RaceArms-Cons)** Lines 5757-5760
// Gamma |- ParseRaceArm(P) => (P_1, a)
// Gamma |- ParseRaceArmsTail(P_1, [a]) => (P_2, arms)
// ────────────────────────────────────────────────────────────────────────────
// Gamma |- ParseRaceArms(P) => (P_2, arms)
//
// **(Parse-RaceArms-Empty)** Lines 5762-5765
// Tok(P) = Punctuator("}")    c = Code(Parse-Syntax-Err)    Emit(c, Tok(P).span)
// ────────────────────────────────────────────────────────────────────────────
// Gamma |- ParseRaceArms(P) => (P, [])
//
// **(Parse-RaceArm)** Lines 5767-5770
// Gamma |- ParseExpr(P) => (P_1, e)
// IsOp(Tok(P_1), "->")
// IsPunc(Tok(Advance(P_1)), "|")
// Gamma |- ParsePattern(Advance(Advance(P_1))) => (P_2, pat)
// IsPunc(Tok(P_2), "|")
// Gamma |- ParseRaceHandler(Advance(P_2)) => (P_3, handler)
// ────────────────────────────────────────────────────────────────────────────
// Gamma |- ParseRaceArm(P) => (P_3, <e, pat, handler>)
//
// **(Parse-RaceArmsTail-End)** Lines 5772-5775
// Tok(P) = Punctuator("}")
// ────────────────────────────────────────────────────────────────────────────
// Gamma |- ParseRaceArmsTail(P, xs) => (P, xs)
//
// **(Parse-RaceArmsTail-TrailingComma)** Lines 5777-5780
// IsPunc(Tok(P), ",")    IsPunc(Tok(Advance(P)), "}")
// TrailingCommaAllowed(P_0, P, {Punctuator("}")})
// ────────────────────────────────────────────────────────────────────────────
// Gamma |- ParseRaceArmsTail(P, xs) => (Advance(P), xs)
//
// **(Parse-RaceArmsTail-Comma)** Lines 5782-5785
// IsPunc(Tok(P), ",")
// Gamma |- ParseRaceArm(Advance(P)) => (P_1, a)
// Gamma |- ParseRaceArmsTail(P_1, xs ++ [a]) => (P_2, ys)
// ────────────────────────────────────────────────────────────────────────────
// Gamma |- ParseRaceArmsTail(P, xs) => (P_2, ys)
//
// **(Parse-RaceHandler-Yield)** Lines 5787-5790
// IsKw(Tok(P), `yield`)
// Gamma |- ParseExpr(Advance(P)) => (P_1, e)
// ────────────────────────────────────────────────────────────────────────────
// Gamma |- ParseRaceHandler(P) => (P_1, RaceYield(e))
//
// **(Parse-RaceHandler-Return)** Lines 5792-5795
// NOT IsKw(Tok(P), `yield`)
// Gamma |- ParseExpr(P) => (P_1, e)
// ────────────────────────────────────────────────────────────────────────────
// Gamma |- ParseRaceHandler(P) => (P_1, RaceReturn(e))
//
// SEMANTICS:
// - `race { arm, arm, ... }` - Race multiple async operations
// - Each arm: `async_expr -> |pattern| handler_expr`
// - Handler types:
//   * RaceReturn: Return the transformed value (expression without yield)
//   * RaceYield: Yield the transformed value (expression with yield prefix)
// - All handlers must be the same type (all return OR all yield)
// - At least 2 arms required
// - First to complete wins; others are cancelled
//
// =============================================================================
// SOURCE FILE: cursive-bootstrap/src/02_syntax/parser_expr.cpp
// =============================================================================
//
// CONTENT TO MIGRATE:
// -----------------------------------------------------------------------------
//
// 1. ParseRaceExpr (Lines 951-974)
//    ─────────────────────────────────────────────────────────────────────────
//    Lines 951-954: Keyword check and initialization
//      - Check for `race` keyword (IsKwTok)
//      - SPEC_RULE("Parse-Race-Expr")
//      - Advance past keyword
//
//    Lines 955-962: Expect and consume opening brace
//      - Check for "{" punctuator
//      - Error recovery if missing
//      - Advance past "{"
//
//    Lines 963-969: Parse race arms
//      - ParseRaceArms(after_l) for list of arms
//      - Check for "}" closing brace
//      - Error recovery if missing
//
//    Lines 970-974: Consume closing brace and construct result
//      - Advance past "}"
//      - Construct RaceExpr{arms}
//      - Return with span
//
// 2. ParseRaceHandler (Lines 2040-2056)
//    ─────────────────────────────────────────────────────────────────────────
//    Lines 2041-2049: Check for yield handler
//      - If IsKw(parser, "yield"):
//        * SPEC_RULE("Parse-RaceHandler-Yield")
//        * Advance past yield
//        * Parse expression
//        * handler.kind = RaceHandlerKind::Yield
//
//    Lines 2051-2055: Otherwise return handler
//      - SPEC_RULE("Parse-RaceHandler-Return")
//      - Parse expression directly
//      - handler.kind = RaceHandlerKind::Return
//
// 3. ParseRaceArm (Lines 2058-2092)
//    ─────────────────────────────────────────────────────────────────────────
//    Lines 2059-2066: Parse async expression and arrow
//      - SPEC_RULE("Parse-RaceArm")
//      - ParseExpr for async expression
//      - Expect "->" operator
//      - Error recovery if missing
//
//    Lines 2067-2083: Parse pattern in pipes
//      - Expect "|" punctuator
//      - ParsePattern for binding pattern
//      - Expect closing "|"
//      - Error recovery for each
//
//    Lines 2084-2091: Parse handler and construct arm
//      - Advance past second "|"
//      - ParseRaceHandler for handler expression
//      - Construct RaceArm{expr, pattern, handler}
//
// 4. ParseRaceArms (Lines 2094-2104)
//    ─────────────────────────────────────────────────────────────────────────
//    Lines 2095-2098: Handle empty case (error)
//      - If immediately at "}", return empty with error
//      - SPEC_RULE("Parse-RaceArms-Empty")
//
//    Lines 2099-2103: Parse first arm and continue
//      - SPEC_RULE("Parse-RaceArms-Cons")
//      - Parse first arm
//      - Continue with ParseRaceArmsTail
//
// 5. ParseRaceArmsTail (Lines 2106-2131)
//    ─────────────────────────────────────────────────────────────────────────
//    Lines 2109-2112: Handle end of arms
//      - SkipNewlines for formatting flexibility
//      - If at "}", return accumulated arms
//      - SPEC_RULE("Parse-RaceArmsTail-End")
//
//    Lines 2114-2128: Handle comma continuation
//      - Check for ","
//      - Handle trailing comma case (with error)
//      - SPEC_RULE("Parse-RaceArmsTail-TrailingComma")
//      - Otherwise parse next arm
//      - SPEC_RULE("Parse-RaceArmsTail-Comma")
//      - Recursively continue with tail
//
// =============================================================================
// DEPENDENCIES:
// =============================================================================
// - ParseExpr: General expression parser
// - ParsePattern: Pattern parser
// - RaceExpr AST node (from ast.hpp)
// - RaceArm struct with expr, pattern, handler
// - RaceHandler struct with kind enum and value
// - RaceHandlerKind enum: Yield, Return
// - MakeExpr, SpanBetween helpers
// - Token predicates: IsKwTok, IsPunc, IsOp, IsKw
// - EmitParseSyntaxErr, SyncStmt, SkipNewlines for error recovery
// - EmitTrailingCommaErr for trailing comma diagnostics
//
// =============================================================================
// REFACTORING NOTES:
// =============================================================================
// 1. The race arm syntax `expr -> |pat| handler` uses a unique pattern-in-pipes
//    notation. Ensure this is correctly documented.
//
// 2. The source uses SkipNewlines for formatting flexibility; verify this
//    matches the spec's newline handling requirements.
//
// 3. Trailing comma handling emits an error via EmitTrailingCommaErr but still
//    accepts the code - this is consistent with spec's TrailingCommaAllowed.
//
// 4. Consider whether ParseRaceArms/ParseRaceArmsTail could use a shared
//    comma-list parsing template.
//
// 5. Error recovery in ParseRaceArm uses SyncStmt; verify this provides
//    reasonable recovery behavior.
//
// 6. The minimum 2 arms requirement is a semantic constraint, not syntactic.
//
// =============================================================================
