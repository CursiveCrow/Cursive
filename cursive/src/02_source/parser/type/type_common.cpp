// =============================================================================
// MIGRATION MAPPING: type_common.cpp
// =============================================================================
// This file should contain common utilities, helpers, and the main ParseType
// entry point that orchestrates all type parsing.
//
// SPEC REFERENCE: C0updated.md, Section 3.3.7, Lines 4628-4641
// Lines 4630-4631 (TypeStart predicate)
// =============================================================================
//
// MAIN TYPE PARSING RULE:
// -----------------------------------------------------------------------------
//
// **(Parse-Type)** Lines 4633-4636
// Γ ⊢ ParsePermOpt(P) ⇓ (P_1, perm_opt)
// Γ ⊢ ParseNonPermType(P_1) ⇓ (P_2, base)
// Γ ⊢ ParseUnionTail(P_2) ⇓ (P_3, ts)
// base' = (base if ts = [] else TypeUnion([base] ++ ts))
// ty_0 = (base' if perm_opt = ⊥ else TypePerm(perm_opt, base'))
// Γ ⊢ ParseRefinementOpt(P_3) ⇓ (P_4, pred_opt)
// ty = (ty_0 if pred_opt = ⊥ else TypeRefine(ty_0, pred_opt))
// ────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseType(P) ⇓ (P_4, ty)
//
// **(Parse-Type-Err)** Lines 4638-4641
// Γ ⊢ ParsePermOpt(P) ⇓ (P_1, perm_opt)
// c = Code(Parse-Syntax-Err)
// Γ ⊢ Emit(c, Tok(P_1).span)
// base = TypePrim("!")
// ty = (base if perm_opt = ⊥ else TypePerm(perm_opt, base))
// ────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseType(P) ⇓ (P_1, ty)
//
// TYPE START PREDICATE (Lines 4630-4631):
// -----------------------------------------------------------------------------
// TypeStart(t) ⇔
//   IsKw(t, `const`) ∨ IsKw(t, `unique`) ∨ IsKw(t, `shared`) ∨
//   Lexeme(t) ∈ PrimTypes_C0 ∨
//   IsPunc(t, "(") ∨ IsPunc(t, "[") ∨
//   IsOp(t, "*") ∨ IsOp(t, "$") ∨
//   Lexeme(t) ∈ {`string`, `Ptr`} ∨
//   IsIdent(t)
//
// =============================================================================
// SOURCE FILE: cursive-bootstrap/src/02_syntax/parser_types.cpp
// =============================================================================
//
// CONTENT TO MIGRATE:
// -----------------------------------------------------------------------------
//
// 1. MakeTypeNode helper (Lines 30-35)
//    ─────────────────────────────────────────────────────────────────────────
//    std::shared_ptr<Type> MakeTypeNode(const core::Span& span, TypeNode node) {
//      auto ty = std::make_shared<Type>();
//      ty->span = span;
//      ty->node = std::move(node);
//      return ty;
//    }
//
// 2. MakeTypePrim helper (Lines 37-40)
//    ─────────────────────────────────────────────────────────────────────────
//    std::shared_ptr<Type> MakeTypePrim(const core::Span& span,
//                                       std::string_view name) {
//      return MakeTypeNode(span, TypePrim{Identifier{name}});
//    }
//
// 3. Token predicates (Lines 42-55)
//    ─────────────────────────────────────────────────────────────────────────
//    bool IsOp(const Parser& parser, std::string_view op) {
//      const Token* tok = Tok(parser);
//      return tok && IsOpTok(*tok, op);
//    }
//
//    bool IsPunc(const Parser& parser, std::string_view punc) {
//      const Token* tok = Tok(parser);
//      return tok && IsPuncTok(*tok, punc);
//    }
//
//    bool IsKw(const Parser& parser, std::string_view kw) {
//      const Token* tok = Tok(parser);
//      return tok && IsKwTok(*tok, kw);
//    }
//
// 4. SkipNewlines helper (Lines 23-27)
//    ─────────────────────────────────────────────────────────────────────────
//    void SkipNewlines(Parser& parser) {
//      while (Tok(parser) && Tok(parser)->kind == TokenKind::Newline) {
//        Advance(parser);
//      }
//    }
//
// 5. Main ParseType function (Lines 689-780)
//    ─────────────────────────────────────────────────────────────────────────
//    ParseElemResult<std::shared_ptr<Type>> ParseType(Parser parser) {
//      Parser start = parser;
//      PermOptResult perm = ParsePermOpt(parser);
//      Parser after_perm = perm.parser;
//      const Token* tok = Tok(after_perm);
//
//      // Error case: no valid type start
//      if (!tok) {
//        SPEC_RULE("Parse-Type-Err");
//        EmitParseSyntaxErr(after_perm, TokSpan(after_perm));
//        std::shared_ptr<Type> base =
//            MakeTypePrim(SpanBetween(start, after_perm), "!");
//        if (perm.perm.has_value()) {
//          TypePermType perm_type;
//          perm_type.perm = *perm.perm;
//          perm_type.base = base;
//          return {after_perm,
//                  MakeTypeNode(SpanBetween(start, after_perm), perm_type)};
//        }
//        return {after_perm, base};
//      }
//
//      // Check for valid non-perm type start
//      const bool non_perm_start = tok->kind == TokenKind::Identifier ||
//                                  (tok->kind == TokenKind::Punctuator &&
//                                   (tok->lexeme == "(" || tok->lexeme == "[")) ||
//                                  (tok->kind == TokenKind::Operator &&
//                                   (tok->lexeme == "*" || tok->lexeme == "$" ||
//                                    tok->lexeme == "!"));
//      if (!non_perm_start) {
//        // Error: invalid type start
//        SPEC_RULE("Parse-Type-Err");
//        EmitParseSyntaxErr(after_perm, TokSpan(after_perm));
//        // ... error recovery ...
//      }
//
//      // Parse base type
//      ParseElemResult<std::shared_ptr<Type>> base = ParseNonPermType(after_perm);
//
//      // Parse union tail
//      ParseElemResult<std::vector<std::shared_ptr<Type>>> tail =
//          ParseUnionTail(base.parser);
//
//      Parser out = tail.parser;
//
//      // Construct type
//      SPEC_RULE("Parse-Type");
//      std::shared_ptr<Type> merged = base.elem;
//
//      // Merge into union if tail is non-empty
//      if (!tail.elem.empty()) {
//        TypeUnion uni;
//        uni.types.reserve(1 + tail.elem.size());
//        uni.types.push_back(base.elem);
//        uni.types.insert(uni.types.end(), tail.elem.begin(), tail.elem.end());
//        merged = MakeTypeNode(SpanBetween(start, out), uni);
//      }
//
//      // Apply permission if present
//      if (perm.perm.has_value()) {
//        TypePermType perm_type;
//        perm_type.perm = *perm.perm;
//        perm_type.base = merged;
//        merged = MakeTypeNode(SpanBetween(start, out), perm_type);
//      }
//
//      // Check for refinement clause
//      const Token* where_tok = Tok(out);
//      if (where_tok && TypeWhereTok(*where_tok)) {
//        // ... parse refinement (see refinement_clause.cpp) ...
//      }
//
//      return {out, merged};
//    }
//
// 6. ParseNonPermType function (Lines 419-666)
//    ─────────────────────────────────────────────────────────────────────────
//    This is the main dispatch for all non-permission types.
//    It checks the current token and delegates to appropriate parsing:
//    - "(" -> tuple/unit/function type
//    - "[" -> array/slice type
//    - "!" -> never type
//    - "*" -> raw pointer type
//    - "$" -> dynamic type
//    - identifier -> prim/string/bytes/Ptr/opaque/where/path/modal
//
//    See individual type files for specific parsing logic.
//
// DEPENDENCIES:
// =============================================================================
// - All individual type parsing functions
// - ParsePermOpt (permission.cpp)
// - ParseUnionTail (union_type.cpp)
// - ParseRefinementOpt (refinement_clause.cpp)
// - Type AST node types (TypePrim, TypeUnion, TypePermType, TypeRefine, etc.)
// - Token types and predicates
// - Parser state and utilities
// - Diagnostic emission helpers
//
// =============================================================================
// REFACTORING NOTES:
// =============================================================================
// - ParseType is the main entry point, called from many places
// - The layering is: Perm -> NonPerm -> Union -> Refinement
// - Permission applies first (outermost)
// - Union is left-associative
// - Refinement applies last (innermost after union construction)
// - ParseNonPermType is a large dispatch; consider table-driven approach
// - Consider separating ParseType orchestration from ParseNonPermType dispatch
// - The TypeStart predicate could be a lookup table for performance
// - Span management is consistent: start at parser entry, end at final position
// =============================================================================
