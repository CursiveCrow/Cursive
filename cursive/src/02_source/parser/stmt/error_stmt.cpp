// =============================================================================
// MIGRATION MAPPING: error_stmt.cpp
// =============================================================================
// This file should contain the error statement type for parse error recovery.
//
// SPEC REFERENCE: C0updated.md, Section 3.3.10, Lines 6259-6262
// SPEC REFERENCE: C0updated.md, Section 3.3.12, Lines 6452-6479
// =============================================================================
//
// FORMAL RULES FROM SPEC:
// -----------------------------------------------------------------------------
// **(Parse-Statement-Err)** Lines 6259-6262
// c = Code(Parse-Syntax-Err)
// Gamma |- Emit(c, Tok(P).span)
// P_1 = AdvanceOrEOF(P)
// Gamma |- SyncStmt(P_1) => P_2
// ────────────────────────────────────────────────────────────────────────────
// Gamma |- ParseStmt(P) => (P_2, ErrorStmt(SpanBetween(P, P_2)))
//
// STATEMENT SYNCHRONIZATION (Lines 6454-6479):
// -----------------------------------------------------------------------------
// SyncStmt = {Punctuator(";"), Newline, Punctuator("}"), EOF}
//
// **(Sync-Stmt-Stop)** Lines 6466-6469
// Tok(P) IN {Punctuator("}"), EOF}
// ────────────────────────────────────────────────────────────────────────────
// Gamma |- SyncStmt(P) => P
//
// **(Sync-Stmt-Consume)** Lines 6471-6474
// Tok(P) IN {Punctuator(";"), Newline}
// ────────────────────────────────────────────────────────────────────────────
// Gamma |- SyncStmt(P) => Advance(P)
//
// **(Sync-Stmt-Advance)** Lines 6476-6479
// Tok(P) NOT IN SyncStmt
// ────────────────────────────────────────────────────────────────────────────
// Gamma |- SyncStmt(P) => SyncStmt(Advance(P))
//
// SEMANTICS:
// - ErrorStmt represents a malformed statement
// - Created during error recovery when parsing fails
// - Allows parser to continue after encountering errors
// - Span captures the problematic source region
// - Error diagnostic is emitted when ErrorStmt is created
//
// =============================================================================
// SOURCE FILE: cursive-bootstrap/src/02_syntax/parser_stmt.cpp
// =============================================================================
//
// CONTENT TO MIGRATE:
// -----------------------------------------------------------------------------
//
// 1. ErrorStmt creation in ParseStmt (Lines 870-900)
//    ─────────────────────────────────────────────────────────────────────────
//    Lines 879-887: Error case when core parsing fails
//      - ParseStmtCoreResult core = ParseStmtCore(parser);
//        if (!core.matched) {
//          SPEC_RULE("Parse-Statement-Err");
//          EmitParseSyntaxErr(parser, TokSpan(parser));
//          Parser next = AdvanceOrEOF(parser);
//          Parser sync = next;
//          SyncStmt(sync);
//          return {sync, ErrorStmt{SpanBetween(parser, sync)}};
//        }
//
// 2. ErrorStmt creation from expression errors (Lines 696-698)
//    ─────────────────────────────────────────────────────────────────────────
//    Lines 696-698: Expression parsing error
//      - ParseElemResult<ExprPtr> expr = ParseExpr(parser);
//        if (expr.elem && std::holds_alternative<ErrorExpr>(expr.elem->node)) {
//          return {expr.parser, ErrorStmt{SpanBetween(start, expr.parser)}, true};
//        }
//
// 3. ErrorStmt creation from assignment validation (Lines 701-707)
//    ─────────────────────────────────────────────────────────────────────────
//    Lines 701-707: Invalid place expression
//      - SPEC_RULE("Parse-Assign-Stmt");
//        if (!IsPlaceExpr(expr.elem)) {
//          EmitParseSyntaxErr(expr.parser, TokSpan(parser));
//          Parser sync = expr.parser;
//          SyncStmt(sync);
//          return {sync, ErrorStmt{SpanBetween(start, sync)}, true};
//        }
//
// 4. SyncStmt recovery function (shared across statement parsing)
//    ─────────────────────────────────────────────────────────────────────────
//    Implementation pattern:
//      void SyncStmt(Parser& parser) {
//        while (!AtEof(parser)) {
//          const Token* tok = Tok(parser);
//          if (tok->kind == TokenKind::Punctuator && tok->lexeme == "}") {
//            return;  // Stop at block end
//          }
//          if (tok->kind == TokenKind::Newline ||
//              (tok->kind == TokenKind::Punctuator && tok->lexeme == ";")) {
//            Advance(parser);  // Consume terminator
//            return;
//          }
//          Advance(parser);  // Skip other tokens
//        }
//      }
//
// ERROR STMT DATA STRUCTURE:
// =============================================================================
// struct ErrorStmt {
//   core::Span span;       // Span of the malformed statement
// };
//
// NOTE: ErrorStmt has only span, no other fields
// - The diagnostic has already been emitted
// - ErrorStmt is a placeholder in the AST
// - Allows type checking and other passes to skip the error region
//
// DEPENDENCIES:
// =============================================================================
// - SyncStmt recovery function (stmt_common.cpp)
// - EmitParseSyntaxErr function (diagnostics)
// - AdvanceOrEOF helper function
// - SpanBetween helper function
// - ErrorStmt AST node type
//
// =============================================================================
// REFACTORING NOTES:
// =============================================================================
// - ErrorStmt is created in multiple places during error recovery
// - Always paired with diagnostic emission
// - SyncStmt advances to a recovery point (terminator or block end)
// - Error recovery allows parsing to continue with subsequent statements
// - ErrorStmt span should capture as much of the error region as practical
// - Later passes should gracefully handle ErrorStmt nodes
// =============================================================================
