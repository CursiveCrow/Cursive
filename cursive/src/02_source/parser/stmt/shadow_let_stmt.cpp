// =============================================================================
// MIGRATION MAPPING: shadow_let_stmt.cpp
// =============================================================================
// This file should contain parsing logic for shadow let binding statements.
//
// SPEC REFERENCE: C0updated.md, Section 3.3.10, Lines 6270-6273, 4098-4101
// =============================================================================
//
// FORMAL RULES FROM SPEC:
// -----------------------------------------------------------------------------
// **(Parse-Shadow-Stmt)** Lines 6270-6273
// IsKw(Tok(P), `shadow`)
// Tok(Advance(P)) in {Keyword(`let`), Keyword(`var`)}
// Gamma |- ParseShadowBinding(Advance(P)) => (P_1, s)
// ────────────────────────────────────────────────────────────────────────────
// Gamma |- ParseStmtCore(P) => (P_1, s)
//
// **(Parse-ShadowBinding)** Lines 4098-4101
// Tok(P) = kw in {Keyword(`let`), Keyword(`var`)}
// Gamma |- ParseIdent(Advance(P)) => (P_1, name)
// Gamma |- ParseTypeAnnotOpt(P_1) => (P_2, ty_opt)
// IsOp(Tok(P_2), "=")
// Gamma |- ParseExpr(Advance(P_2)) => (P_3, init)
// s = (ShadowLetStmt(name, ty_opt, init) if kw = Keyword(`let`) else ShadowVarStmt(...))
// ────────────────────────────────────────────────────────────────────────────
// Gamma |- ParseShadowBinding(P) => (P_3, s)
//
// TERMINATOR RULES (Lines 6360-6370):
// -----------------------------------------------------------------------------
// ReqTerm(s) <=> s in {..., ShadowLetStmt(_), ...}
//
// **(ConsumeTerminatorOpt-Req-Yes)** Lines 6362-6365
// ReqTerm(s)    IsTerm(Tok(P))
// ────────────────────────────────────────────────────────────────────────────
// Gamma |- ConsumeTerminatorOpt(P, s) => Advance(P)
//
// SEMANTICS:
// - `shadow let` rebinds an existing name with an immutable binding
// - Unlike regular let, shadow binding uses ONLY `=` operator (not `:=`)
// - Shadow bindings use identifier only, NOT full pattern matching
// - Statements require terminator (`;` or newline)
//
// =============================================================================
// SOURCE FILE: cursive-bootstrap/src/02_syntax/parser_stmt.cpp
// =============================================================================
//
// CONTENT TO MIGRATE:
// -----------------------------------------------------------------------------
//
// 1. ParseShadowBinding helper (in parser_binding.cpp or this file)
//    ─────────────────────────────────────────────────────────────────────────
//    Expected signature:
//      ParseElemResult<Stmt> ParseShadowBinding(Parser parser);
//
//    Logic:
//      - Check for `let` or `var` keyword (already at this position)
//      - Parse identifier (name)
//      - Parse optional type annotation
//      - Expect `=` operator (NOT `:=`)
//      - Parse initializer expression
//      - Return ShadowLetStmt or ShadowVarStmt based on keyword
//
// 2. Shadow statement detection in ParseStmtCore (Lines 401-409)
//    ─────────────────────────────────────────────────────────────────────────
//    Lines 401-402: Check for shadow keyword
//      - if (IsKw(parser, "shadow"))
//
//    Lines 403-404: Check for let/var following shadow
//      - Parser after_shadow = parser;
//        Advance(after_shadow);
//        if (IsKw(after_shadow, "let") || IsKw(after_shadow, "var"))
//
//    Lines 405-408: Parse shadow binding
//      - SPEC_RULE("Parse-Shadow-Stmt");
//        ParseElemResult<Stmt> stmt = ParseShadowBinding(after_shadow);
//        return {stmt.parser, std::move(stmt.elem), true};
//
// 3. RequiresTerminator check (Lines 145-158)
//    ─────────────────────────────────────────────────────────────────────────
//    Line 155: ShadowLetStmt requires terminator
//      - return ... || std::holds_alternative<ShadowLetStmt>(stmt) || ...;
//
// 4. ApplyStmtAttrs for attributes (Lines 186-189)
//    ─────────────────────────────────────────────────────────────────────────
//    Lines 186-189: Apply attributes to shadow let statement
//      - if (auto* shadow_let = std::get_if<ShadowLetStmt>(&stmt)) {
//          shadow_let->init = WrapAttrExpr(attrs, shadow_let->init);
//          return;
//        }
//
// SHADOW LET DATA STRUCTURE:
// =============================================================================
// struct ShadowLetStmt {
//   Identifier name;            // The name being rebound (identifier only)
//   std::optional<Type> type;   // Optional type annotation
//   ExprPtr init;               // Initializer expression
//   core::Span span;            // Source span
// };
//
// DEPENDENCIES:
// =============================================================================
// - ParseShadowBinding function (binding.cpp or here)
// - ParseIdent function (identifier.cpp)
// - ParseTypeAnnotOpt function (type_annot.cpp)
// - ParseExpr function (expr/*.cpp)
// - ShadowLetStmt AST node type
// - ConsumeTerminatorOpt function (stmt_common.cpp)
// - WrapAttrExpr function (stmt_common.cpp)
// - IsKw helper function
//
// =============================================================================
// REFACTORING NOTES:
// =============================================================================
// - Shadow bindings differ from regular bindings:
//   1. Use identifier only (not full pattern)
//   2. Use only `=` operator (not `:=`)
// - The `shadow` keyword is consumed before calling ParseShadowBinding
// - ParseShadowBinding expects parser at `let` or `var` keyword
// - ShadowLetStmt and ShadowVarStmt share parsing via ParseShadowBinding
// - The keyword (let vs var) determines which statement type to produce
// - Attributes are applied to the initializer expression
// =============================================================================
