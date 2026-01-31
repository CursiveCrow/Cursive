// =============================================================================
// MIGRATION MAPPING: contract_entry.cpp
// =============================================================================
// This file should contain parsing logic for contract entry expressions (@entry(expr)).
//
// SPEC REFERENCE: C0updated.md, Section 3.3.8.6, Lines 5173-5176
// =============================================================================
//
// FORMAL RULES FROM SPEC:
// -----------------------------------------------------------------------------
// **(Parse-Contract-Entry)** Lines 5173-5176
// IsOp(Tok(P), "@")    IsIdent(Tok(Advance(P)))    Lexeme(Tok(Advance(P))) = "entry"
// IsPunc(Tok(Advance(Advance(P))), "(")
// Gamma |- ParseExpr(Advance(Advance(Advance(P)))) => (P_1, e)
// IsPunc(Tok(P_1), ")")
// ----------------------------------------------------------------------------
// Gamma |- ParsePrimary(P) => (Advance(P_1), ContractEntry(e))
//
// SEMANTICS:
// - @entry(expr) captures the entry/old value of an expression at procedure entry
// - Valid ONLY in postcondition context (right side of => in contract)
// - The expression type MUST satisfy BitcopyType or CloneType
// - Allows comparing post-state values against entry-state values
// - Semantic validation occurs during analysis phase, not parsing
//
// =============================================================================
// SOURCE FILE: cursive-bootstrap/src/02_syntax/parser_expr.cpp
// =============================================================================
//
// CONTENT TO MIGRATE:
// -----------------------------------------------------------------------------
//
// 1. Contract entry parsing in ParsePrimary (Lines 881-902)
//    ---------------------------------------------------------------------------
//    Lines 881-882: Check for "entry" keyword
//      - Check: name == "entry" (after "@" was consumed and identifier extracted)
//      - SPEC_RULE: "Parse-Contract-Entry"
//
//    Lines 883-888: Expect opening parenthesis
//      - Check: IsPunc(after_name, "(")
//      - If missing, emit syntax error
//      - Call SyncStmt for error recovery
//      - Return ErrorExpr on failure
//
//    Lines 889-891: Parse inner expression
//      - Advance past "("
//      - Call ParseExpr(after_l) for the captured expression
//      - Store result in `expr`
//
//    Lines 892-897: Expect closing parenthesis
//      - Check: IsPunc(expr.parser, ")")
//      - If missing, emit syntax error
//      - Call SyncStmt for error recovery
//      - Return ErrorExpr on failure
//
//    Lines 898-902: Build EntryExpr
//      - Advance past ")"
//      - Create EntryExpr node (entry)
//      - Set entry.expr = expr.elem
//      - Return with:
//        - Parser: after_r
//        - Expr: MakeExpr(SpanBetween(start, after_r), entry)
//
// SOURCE CODE (Lines 881-902):
// -----------------------------------------------------------------------------
//       if (name == "entry") {
//         SPEC_RULE("Parse-Contract-Entry");
//         if (!IsPunc(after_name, "(")) {
//           EmitParseSyntaxErr(after_name, TokSpan(after_name));
//           Parser sync = after_name;
//           SyncStmt(sync);
//           return {sync, MakeExpr(SpanBetween(start, sync), ErrorExpr{})};
//         }
//         Parser after_l = after_name;
//         Advance(after_l);
//         ParseElemResult<ExprPtr> expr = ParseExpr(after_l);
//         if (!IsPunc(expr.parser, ")")) {
//           EmitParseSyntaxErr(expr.parser, TokSpan(expr.parser));
//           Parser sync = expr.parser;
//           SyncStmt(sync);
//           return {sync, MakeExpr(SpanBetween(start, sync), ErrorExpr{})};
//         }
//         Parser after_r = expr.parser;
//         Advance(after_r);
//         EntryExpr entry;
//         entry.expr = expr.elem;
//         return {after_r, MakeExpr(SpanBetween(start, after_r), entry)};
//       }
//
// 2. Error handling for invalid @ constructs (Lines 905-908)
//    ---------------------------------------------------------------------------
//    Lines 905-908: Unknown @ intrinsic
//      - If name is neither "result" nor "entry", emit syntax error
//      - Call SyncStmt for error recovery
//      - Return ErrorExpr
//
// SOURCE CODE (Lines 905-908):
// -----------------------------------------------------------------------------
//     EmitParseSyntaxErr(next, TokSpan(next));
//     Parser sync = next;
//     SyncStmt(sync);
//     return {sync, MakeExpr(SpanBetween(start, sync), ErrorExpr{})};
//
// =============================================================================
// DEPENDENCIES:
// =============================================================================
// - ParseExpr function (expr.cpp) - for parsing the captured expression
// - MakeExpr, SpanBetween helpers (expr_common.cpp)
// - IsOpTok, IsIdentTok, IsPunc, Tok, Advance helpers (parser utilities)
// - EmitParseSyntaxErr, SyncStmt for error handling
// - EntryExpr, ErrorExpr AST node types
//
// =============================================================================
// RELATED CONTRACT INTRINSICS:
// =============================================================================
// @result - References return value in postconditions (contract_result.cpp)
// @entry(expr) - Captures entry/old value of expression (this file)
//
// Contract syntax in procedures:
//   procedure increment(~!) -> i32
//       |= self.value >= 0 => @result > @entry(self.value)
//   {
//       self.value = self.value + 1
//       return self.value
//   }
//
// =============================================================================
// TYPE CONSTRAINTS (Spec Reference):
// =============================================================================
// From the specification, @entry(expr) requires:
// - Expression must have type satisfying BitcopyType OR CloneType
// - This ensures the entry value can be preserved across the procedure call
// - Validation occurs during semantic analysis, not parsing
//
// =============================================================================
// REFACTORING NOTES:
// =============================================================================
// - @entry is parsed as part of the "@" operator dispatch in ParsePrimary
// - Consider: Combine with contract_result.cpp into contract_intrinsics.cpp
// - The "entry" check comes after "result" check in the original code
// - Both checks share the "@" operator detection and identifier extraction
// - Error handling is duplicated for missing "(" and ")"
// - Span: covers from "@" to closing ")"
// - EntryExpr has one field: the captured expression
// - Semantic validation (postcondition context, type constraints) NOT at parse time
// =============================================================================
