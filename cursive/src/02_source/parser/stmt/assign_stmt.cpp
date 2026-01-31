// =============================================================================
// MIGRATION MAPPING: assign_stmt.cpp
// =============================================================================
// This file should contain parsing logic for simple assignment statements.
//
// SPEC REFERENCE: C0updated.md, Section 3.3.10, Lines 6275-6278, 6406-6409
// =============================================================================
//
// FORMAL RULES FROM SPEC:
// -----------------------------------------------------------------------------
// **(Parse-Assign-Stmt)** Lines 6275-6278
// Gamma |- ParsePlace(P) => (P_1, p)
// Tok(P_1) in {Operator("="), Operator("+="), Operator("-="), Operator("*="),
//              Operator("/="), Operator("%=")}
// Gamma |- ParseExpr(Advance(P_1)) => (P_2, e)
// ────────────────────────────────────────────────────────────────────────────
// Gamma |- ParseStmtCore(P) => (P_2, AssignOrCompound(P_1, p, e))
//
// **(AssignOrCompound-Assign)** Lines 6406-6409
// Tok(P_1) = Operator("=")
// ────────────────────────────────────────────────────────────────────────────
// Gamma |- AssignOrCompound(P_1, p, e) => AssignStmt(p, e)
//
// TERMINATOR RULES (Lines 6360-6370):
// -----------------------------------------------------------------------------
// ReqTerm(s) <=> s in {..., AssignStmt(_, _), ...}
//
// **(ConsumeTerminatorOpt-Req-Yes)** Lines 6362-6365
// ReqTerm(s)    IsTerm(Tok(P))
// ────────────────────────────────────────────────────────────────────────────
// Gamma |- ConsumeTerminatorOpt(P, s) => Advance(P)
//
// SEMANTICS:
// - Assignment statement `place = expr` stores value in place expression
// - Place expression must be valid L-value (identifier, field, index, deref)
// - Simple assignment uses `=` operator only
// - Statements require terminator (`;` or newline)
//
// PLACE EXPRESSION VALIDATION (Lines 90-110):
// -----------------------------------------------------------------------------
// IsPlaceExpr(expr) returns true if:
//   - IdentifierExpr (variable name)
//   - FieldAccessExpr (record.field)
//   - TupleAccessExpr (tuple.0)
//   - IndexAccessExpr (array[i])
//   - DerefExpr (*ptr) where inner is also place expr
//
// =============================================================================
// SOURCE FILE: cursive-bootstrap/src/02_syntax/parser_stmt.cpp
// =============================================================================
//
// CONTENT TO MIGRATE:
// -----------------------------------------------------------------------------
//
// 1. IsAssignOp helper function (Lines 39-45)
//    ─────────────────────────────────────────────────────────────────────────
//    bool IsAssignOp(const Token& tok) {
//      if (tok.kind != TokenKind::Operator) {
//        return false;
//      }
//      return tok.lexeme == "=" || tok.lexeme == "+=" || tok.lexeme == "-=" ||
//             tok.lexeme == "*=" || tok.lexeme == "/=" || tok.lexeme == "%=";
//    }
//
// 2. IsPlaceExpr validation function (Lines 90-110)
//    ─────────────────────────────────────────────────────────────────────────
//    bool IsPlaceExpr(const ExprPtr& expr) {
//      if (!expr) return false;
//      if (std::holds_alternative<IdentifierExpr>(expr->node)) return true;
//      if (std::holds_alternative<FieldAccessExpr>(expr->node)) return true;
//      if (std::holds_alternative<TupleAccessExpr>(expr->node)) return true;
//      if (std::holds_alternative<IndexAccessExpr>(expr->node)) return true;
//      if (const auto* deref = std::get_if<DerefExpr>(&expr->node)) {
//        return IsPlaceExpr(deref->value);
//      }
//      return false;
//    }
//
// 3. Assignment parsing in ParseStmtCore (Lines 694-726)
//    ─────────────────────────────────────────────────────────────────────────
//    Lines 695-698: Parse left-hand side as expression
//      - ParseElemResult<ExprPtr> expr = ParseExpr(parser);
//      - (Note: we parse as expression first, then validate as place)
//
//    Lines 699-700: Check for assignment operator
//      - const Token* op = Tok(expr.parser);
//        if (op && IsAssignOp(*op))
//
//    Lines 701-707: Validate place expression
//      - SPEC_RULE("Parse-Assign-Stmt");
//        if (!IsPlaceExpr(expr.elem)) {
//          EmitParseSyntaxErr(expr.parser, TokSpan(parser));
//          Parser sync = expr.parser;
//          SyncStmt(sync);
//          return {sync, ErrorStmt{SpanBetween(start, sync)}, true};
//        }
//
//    Lines 708-710: Parse right-hand side
//      - Parser next = expr.parser;
//        Advance(next);  // consume operator
//        ParseElemResult<ExprPtr> rhs = ParseExpr(next);
//
//    Lines 719-725: Construct AssignStmt (when op is "=")
//      - SPEC_RULE("AssignOrCompound-Assign");
//        AssignStmt stmt;
//        stmt.place = expr.elem;
//        stmt.value = rhs.elem;
//        stmt.span = SpanBetween(start, rhs.parser);
//        return {rhs.parser, stmt, true};
//
// 4. RequiresTerminator check (Lines 145-158)
//    ─────────────────────────────────────────────────────────────────────────
//    Line 157: AssignStmt requires terminator
//      - return ... || std::holds_alternative<AssignStmt>(stmt) || ...;
//
// 5. ApplyStmtAttrs for attributes (Lines 194-198)
//    ─────────────────────────────────────────────────────────────────────────
//    Lines 194-198: Apply attributes to assign statement
//      - if (auto* assign = std::get_if<AssignStmt>(&stmt)) {
//          assign->place = WrapAttrExpr(attrs, assign->place);
//          assign->value = WrapAttrExpr(attrs, assign->value);
//          return;
//        }
//
// ASSIGN DATA STRUCTURE:
// =============================================================================
// struct AssignStmt {
//   ExprPtr place;       // Place expression (L-value)
//   ExprPtr value;       // Value expression (R-value)
//   core::Span span;     // Source span
// };
//
// DEPENDENCIES:
// =============================================================================
// - IsAssignOp helper function (stmt_common.cpp or here)
// - IsPlaceExpr validation function (stmt_common.cpp or here)
// - ParseExpr function (expr/*.cpp)
// - AssignStmt AST node type
// - ConsumeTerminatorOpt function (stmt_common.cpp)
// - WrapAttrExpr function (stmt_common.cpp)
// - SyncStmt recovery function (stmt_common.cpp)
//
// =============================================================================
// REFACTORING NOTES:
// =============================================================================
// - Assignment is parsed as: expr op expr, then validated
// - Simple assignment uses "=", compound uses "+=", "-=", etc.
// - Place validation happens AFTER parsing the expression
// - Error recovery via SyncStmt if place is invalid
// - Span covers from place expression to value expression
// - Attributes can be applied to both place and value
// - This file handles "=" only; compound_assign_stmt.cpp handles +=, etc.
// =============================================================================
