// =============================================================================
// MIGRATION MAPPING: expr_stmt.cpp
// =============================================================================
// This file should contain parsing logic for expression statements.
//
// SPEC REFERENCE: C0updated.md, Section 3.3.10, Lines 6345-6348
// =============================================================================
//
// FORMAL RULES FROM SPEC:
// -----------------------------------------------------------------------------
// **(Parse-Expr-Stmt)** Lines 6345-6348
// Gamma |- ParseExpr(P) => (P_1, e)
// ────────────────────────────────────────────────────────────────────────────
// Gamma |- ParseStmtCore(P) => (P_1, ExprStmt(e))
//
// TERMINATOR RULES (Lines 6360-6380):
// -----------------------------------------------------------------------------
// ReqTerm(s) <=> s in {..., ExprStmt(_)} (but with exceptions)
//
// ExprStmt terminator is OPTIONAL if expression ends with a block:
//   - LoopInfiniteExpr, LoopConditionalExpr, LoopIterExpr
//   - MatchExpr
//   - BlockExpr
//   - UnsafeBlockExpr
//   - IfExpr (when has else branch ending with block)
//
// **(ConsumeTerminatorOpt-Opt-Yes)** Lines 6372-6375
// NOT ReqTerm(s)    IsTerm(Tok(P))
// ────────────────────────────────────────────────────────────────────────────
// Gamma |- ConsumeTerminatorOpt(P, s) => Advance(P)
//
// **(ConsumeTerminatorOpt-Opt-No)** Lines 6377-6380
// NOT ReqTerm(s)    NOT IsTerm(Tok(P))
// ────────────────────────────────────────────────────────────────────────────
// Gamma |- ConsumeTerminatorOpt(P, s) => P
//
// SEMANTICS:
// - Expression statements evaluate an expression for side effects
// - The expression's value is discarded
// - Block-ending expressions don't require explicit terminators
// - Terminator is optional for block-ending expressions but consumed if present
//
// =============================================================================
// SOURCE FILE: cursive-bootstrap/src/02_syntax/parser_stmt.cpp
// =============================================================================
//
// CONTENT TO MIGRATE:
// -----------------------------------------------------------------------------
//
// 1. EndsWithBlock helper function (Lines 122-143)
//    ─────────────────────────────────────────────────────────────────────────
//    bool EndsWithBlock(const ExprPtr& expr) {
//      if (!expr) return false;
//      if (std::holds_alternative<LoopInfiniteExpr>(expr->node) ||
//          std::holds_alternative<LoopConditionalExpr>(expr->node) ||
//          std::holds_alternative<LoopIterExpr>(expr->node) ||
//          std::holds_alternative<MatchExpr>(expr->node) ||
//          std::holds_alternative<BlockExpr>(expr->node) ||
//          std::holds_alternative<UnsafeBlockExpr>(expr->node)) {
//        return true;
//      }
//      if (const auto* if_expr = std::get_if<IfExpr>(&expr->node)) {
//        // If has else branch that ends with block
//        if (if_expr->else_expr) {
//          return EndsWithBlock(if_expr->else_expr);
//        }
//        // If without else still ends with a block (the then branch)
//        return true;
//      }
//      return false;
//    }
//
// 2. RequiresTerminator check with EndsWithBlock (Lines 145-159)
//    ─────────────────────────────────────────────────────────────────────────
//    bool RequiresTerminator(const Stmt& stmt) {
//      if (const auto* expr_stmt = std::get_if<ExprStmt>(&stmt)) {
//        // Expressions ending with blocks don't require explicit terminators
//        if (EndsWithBlock(expr_stmt->value)) {
//          return false;
//        }
//        return true;
//      }
//      // ... other statements
//    }
//
// 3. Expression statement parsing in ParseStmtCore (Lines 691-732)
//    ─────────────────────────────────────────────────────────────────────────
//    Lines 691-693: Check expression start
//      - if (!IsExprStartToken(*tok)) {
//          return {parser, ErrorStmt{TokSpan(parser)}, false};
//        }
//
//    Lines 695-698: Parse expression
//      - ParseElemResult<ExprPtr> expr = ParseExpr(parser);
//        if (expr.elem && std::holds_alternative<ErrorExpr>(expr.elem->node)) {
//          return {expr.parser, ErrorStmt{SpanBetween(start, expr.parser)}, true};
//        }
//
//    Lines 699-726: Check for assignment (handled in assign_stmt.cpp)
//
//    Lines 728-732: Construct ExprStmt (fallback case)
//      - SPEC_RULE("Parse-Expr-Stmt");
//        ExprStmt stmt;
//        stmt.value = expr.elem;
//        stmt.span = SpanBetween(start, expr.parser);
//        return {expr.parser, stmt, true};
//
// 4. IsExprStartToken helper function (Lines 64-88)
//    ─────────────────────────────────────────────────────────────────────────
//    bool IsExprStartToken(const Token& tok) {
//      if (IsIdentTok(tok) || IsLiteralToken(tok)) return true;
//      if (tok.kind == TokenKind::Punctuator) {
//        return tok.lexeme == "(" || tok.lexeme == "[" || tok.lexeme == "{";
//      }
//      if (tok.kind == TokenKind::Operator) {
//        return tok.lexeme == "!" || tok.lexeme == "-" || tok.lexeme == "&" ||
//               tok.lexeme == "*" || tok.lexeme == "^";
//      }
//      if (tok.kind == TokenKind::Keyword) {
//        return tok.lexeme == "if" || tok.lexeme == "match" ||
//               tok.lexeme == "loop" || tok.lexeme == "unsafe" ||
//               tok.lexeme == "move" || tok.lexeme == "transmute" ||
//               tok.lexeme == "widen" || tok.lexeme == "comptime" ||
//               tok.lexeme == "parallel" || tok.lexeme == "spawn" ||
//               tok.lexeme == "dispatch" || tok.lexeme == "yield" ||
//               tok.lexeme == "sync" || tok.lexeme == "race" ||
//               tok.lexeme == "all";
//      }
//      return false;
//    }
//
// 5. ApplyStmtAttrs for attributes (Lines 204-207)
//    ─────────────────────────────────────────────────────────────────────────
//    Lines 204-207: Apply attributes to expr statement
//      - if (auto* expr_stmt = std::get_if<ExprStmt>(&stmt)) {
//          expr_stmt->value = WrapAttrExpr(attrs, expr_stmt->value);
//          return;
//        }
//
// EXPR STMT DATA STRUCTURE:
// =============================================================================
// struct ExprStmt {
//   ExprPtr value;       // The expression being evaluated
//   core::Span span;     // Source span
// };
//
// DEPENDENCIES:
// =============================================================================
// - IsExprStartToken helper function (stmt_common.cpp or here)
// - EndsWithBlock helper function (stmt_common.cpp or here)
// - ParseExpr function (expr/*.cpp)
// - ExprStmt AST node type
// - ConsumeTerminatorOpt function (stmt_common.cpp)
// - WrapAttrExpr function (stmt_common.cpp)
// - RequiresTerminator function (stmt_common.cpp)
//
// =============================================================================
// REFACTORING NOTES:
// =============================================================================
// - ExprStmt is the fallback case when expression is not followed by "="
// - Expression statements evaluate for side effects (e.g., function calls)
// - Block-ending expressions have optional terminators
// - The EndsWithBlock check recurses through IfExpr else branches
// - Span covers the entire expression
// - This is parsed AFTER checking for assignment operators
// - If expression is followed by "=" or compound op, it's an assignment
// =============================================================================
