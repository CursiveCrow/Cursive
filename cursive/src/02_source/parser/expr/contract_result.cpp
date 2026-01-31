// =============================================================================
// MIGRATION MAPPING: contract_result.cpp
// =============================================================================
// This file should contain parsing logic for contract result expressions (@result).
//
// SPEC REFERENCE: C0updated.md, Section 3.3.8.6, Lines 5168-5171
// =============================================================================
//
// FORMAL RULES FROM SPEC:
// -----------------------------------------------------------------------------
// **(Parse-Contract-Result)** Lines 5168-5171
// IsOp(Tok(P), "@")    IsIdent(Tok(Advance(P)))    Lexeme(Tok(Advance(P))) = "result"
// ----------------------------------------------------------------------------
// Gamma |- ParsePrimary(P) => (Advance(Advance(P)), ContractResult)
//
// SEMANTICS:
// - @result is a contract intrinsic that references the return value
// - Valid ONLY in postcondition context (right side of => in contract)
// - Cannot be used in preconditions or general expressions
// - Type is the return type of the enclosing procedure
// - Semantic validation occurs during analysis phase, not parsing
//
// =============================================================================
// SOURCE FILE: cursive-bootstrap/src/02_syntax/parser_expr.cpp
// =============================================================================
//
// CONTENT TO MIGRATE:
// -----------------------------------------------------------------------------
//
// 1. Contract result parsing in ParsePrimary (Lines 867-879)
//    ---------------------------------------------------------------------------
//    Lines 867-868: "@" operator detection
//      - Check: tok && IsOpTok(*tok, "@")
//      - If true, enter contract intrinsic parsing branch
//
//    Lines 868-871: Setup and advance
//      - Save start position (Parser start = parser)
//      - Create next parser state
//      - Advance(next) past the "@" operator
//      - Get name_tok = Tok(next)
//
//    Lines 872-875: Validate identifier follows
//      - Check: name_tok && (IsIdentTok(*name_tok) || name_tok->kind == TokenKind::Keyword)
//      - Extract name = name_tok->lexeme
//      - Create after_name by advancing past identifier
//
//    Lines 876-879: Check for "result" keyword
//      - Check: name == "result"
//      - SPEC_RULE: "Parse-Contract-Result"
//      - Create ResultExpr node (res)
//      - Return with:
//        - Parser: after_name
//        - Expr: MakeExpr(SpanBetween(start, after_name), res)
//
// SOURCE CODE (Lines 867-879):
// -----------------------------------------------------------------------------
//   if (tok && IsOpTok(*tok, "@")) {
//     Parser start = parser;
//     Parser next = parser;
//     Advance(next);  // consume @
//     const Token* name_tok = Tok(next);
//     if (name_tok && (IsIdentTok(*name_tok) || name_tok->kind == TokenKind::Keyword)) {
//       const std::string_view name = name_tok->lexeme;
//       Parser after_name = next;
//       Advance(after_name);
//       if (name == "result") {
//         SPEC_RULE("Parse-Contract-Result");
//         ResultExpr res;
//         return {after_name, MakeExpr(SpanBetween(start, after_name), res)};
//       }
//       // ... continues with @entry handling ...
//     }
//     // ... error handling ...
//   }
//
// =============================================================================
// DEPENDENCIES:
// =============================================================================
// - MakeExpr, SpanBetween helpers (expr_common.cpp)
// - IsOpTok, IsIdentTok, Tok, Advance helpers (parser utilities)
// - ResultExpr AST node type
//
// =============================================================================
// RELATED CONTRACT INTRINSICS:
// =============================================================================
// @result - References return value in postconditions (this file)
// @entry(expr) - Captures entry/old value of expression (contract_entry.cpp)
//
// Contract syntax in procedures:
//   procedure foo(x: i32) -> i32
//       |= x > 0 => @result >= x
//   {
//       return x * 2
//   }
//
// =============================================================================
// REFACTORING NOTES:
// =============================================================================
// - @result is parsed as part of the "@" operator dispatch in ParsePrimary
// - Consider: Combine with contract_entry.cpp into contract_intrinsics.cpp
// - The "result" check comes before "entry" check in the original code
// - Span: covers from "@" to end of "result" identifier
// - No parentheses or arguments - just @result as-is
// - Semantic validation (postcondition context only) is NOT done at parse time
// - ResultExpr is a simple marker node with no fields
// =============================================================================
