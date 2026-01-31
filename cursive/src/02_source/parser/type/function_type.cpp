// =============================================================================
// MIGRATION MAPPING: function_type.cpp
// =============================================================================
// This file should contain parsing logic for function types: (T1, T2) -> R
// representing the type of a procedure pointer.
//
// SPEC REFERENCE: C0updated.md, Section 3.3.7, Lines 4713-4716
// Section 3.3.7.1 (Param Type Lists), Lines 4847-4879
// =============================================================================
//
// FORMAL RULES FROM SPEC:
// -----------------------------------------------------------------------------
//
// **(Parse-Func-Type)** Lines 4713-4716
// IsPunc(Tok(P), "(")
// Γ ⊢ ParseParamTypeList(Advance(P)) ⇓ (P_1, params)
// IsPunc(Tok(P_1), ")")
// IsOp(Tok(Advance(P_1)), "->")
// Γ ⊢ ParseType(Advance(Advance(P_1))) ⇓ (P_2, ret)
// ────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseNonPermType(P) ⇓ (P_2, TypeFunc(params, ret))
//
// PARAM TYPE LIST RULES (Lines 4849-4867):
// -----------------------------------------------------------------------------
//
// **(Parse-ParamTypeList-Empty)** Lines 4849-4852
// IsPunc(Tok(P), ")")
// ────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseParamTypeList(P) ⇓ (P, [])
//
// **(Parse-ParamTypeList-Cons)** Lines 4854-4857
// Γ ⊢ ParseParamType(P) ⇓ (P_1, pt)
// Γ ⊢ ParseParamTypeListTail(P_1, [pt]) ⇓ (P_2, pts)
// ────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseParamTypeList(P) ⇓ (P_2, pts)
//
// **(Parse-ParamTypeListTail-End)** Lines 4859-4862
// ¬ IsPunc(Tok(P), ",")
// ────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseParamTypeListTail(P, ps) ⇓ (P, ps)
//
// **(Parse-ParamTypeListTail-Comma)** Lines 4864-4867
// IsPunc(Tok(P), ",")
// Γ ⊢ ParseParamType(Advance(P)) ⇓ (P_1, pt)
// Γ ⊢ ParseParamTypeListTail(P_1, ps ++ [pt]) ⇓ (P_2, ps')
// ────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseParamTypeListTail(P, ps) ⇓ (P_2, ps')
//
// PARAM TYPE RULES (Lines 4871-4879):
// -----------------------------------------------------------------------------
//
// **(Parse-ParamType-Move)** Lines 4871-4874
// IsKw(Tok(P), `move`)    Γ ⊢ ParseType(Advance(P)) ⇓ (P_1, ty)
// ────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseParamType(P) ⇓ (P_1, ⟨`move`, ty⟩)
//
// **(Parse-ParamType-Plain)** Lines 4876-4879
// ¬ IsKw(Tok(P), `move`)    Γ ⊢ ParseType(P) ⇓ (P_1, ty)
// ────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseParamType(P) ⇓ (P_1, ⟨⊥, ty⟩)
//
// SEMANTICS:
// - Function types represent procedure pointer types
// - Parameters can have `move` modifier indicating ownership transfer
// - Examples: (i32, i32) -> i32, (move Buffer) -> ()
//
// =============================================================================
// SOURCE FILE: cursive-bootstrap/src/02_syntax/parser_types.cpp
// =============================================================================
//
// CONTENT TO MIGRATE:
// -----------------------------------------------------------------------------
//
// 1. HasFuncArrow lookahead predicate (Lines 98-124)
//    ─────────────────────────────────────────────────────────────────────────
//    bool HasFuncArrow(const Parser& parser) {
//      if (!IsPunc(parser, "(")) {
//        return false;
//      }
//      Parser cur = parser;
//      int depth = 0;
//      while (!AtEof(cur)) {
//        const Token* tok = Tok(cur);
//        if (!tok) {
//          return false;
//        }
//        if (tok->kind == TokenKind::Punctuator) {
//          if (tok->lexeme == "(") {
//            ++depth;
//          } else if (tok->lexeme == ")") {
//            --depth;
//            if (depth == 0) {
//              Parser after = cur;
//              Advance(after);
//              return IsOp(after, "->");
//            }
//          }
//        }
//        Advance(cur);
//      }
//      return false;
//    }
//
// 2. ParseFuncType function (Lines 356-380)
//    ─────────────────────────────────────────────────────────────────────────
//    ParseElemResult<std::shared_ptr<Type>> ParseFuncType(Parser parser) {
//      SPEC_RULE("Parse-Func-Type");
//      Parser start = parser;
//      Parser next = parser;
//      Advance(next);  // consume (
//      ParseElemResult<std::vector<TypeFuncParam>> params = ParseParamTypeList(next);
//      if (!IsPunc(params.parser, ")")) {
//        EmitParseSyntaxErr(params.parser, TokSpan(params.parser));
//        return {params.parser, MakeTypePrim(SpanBetween(start, params.parser), "!")};
//      }
//      Parser after_rparen = params.parser;
//      Advance(after_rparen);  // consume )
//      if (!IsOp(after_rparen, "->")) {
//        EmitParseSyntaxErr(after_rparen, TokSpan(after_rparen));
//        return {after_rparen,
//                MakeTypePrim(SpanBetween(start, after_rparen), "!")};
//      }
//      Parser after_arrow = after_rparen;
//      Advance(after_arrow);  // consume ->
//      ParseElemResult<std::shared_ptr<Type>> ret = ParseType(after_arrow);
//      TypeFunc func;
//      func.params = std::move(params.elem);
//      func.ret = ret.elem;
//      return {ret.parser, MakeTypeNode(SpanBetween(start, ret.parser), func)};
//    }
//
// 3. ParseParamTypeList function (Lines 181-192)
//    ─────────────────────────────────────────────────────────────────────────
//    ParseElemResult<std::vector<TypeFuncParam>> ParseParamTypeList(Parser parser) {
//      SkipNewlines(parser);
//      if (IsPunc(parser, ")")) {
//        SPEC_RULE("Parse-ParamTypeList-Empty");
//        return {parser, {}};
//      }
//      SPEC_RULE("Parse-ParamTypeList-Cons");
//      ParseElemResult<TypeFuncParam> first = ParseParamType(parser);
//      std::vector<TypeFuncParam> params;
//      params.push_back(first.elem);
//      return ParseParamTypeListTail(first.parser, std::move(params));
//    }
//
// 4. ParseParamTypeListTail function (Lines 157-178)
//    ─────────────────────────────────────────────────────────────────────────
//    ParseElemResult<std::vector<TypeFuncParam>> ParseParamTypeListTail(
//        Parser parser,
//        std::vector<TypeFuncParam> ps) {
//      SkipNewlines(parser);
//      if (!IsPunc(parser, ",")) {
//        SPEC_RULE("Parse-ParamTypeListTail-End");
//        return {parser, ps};
//      }
//      const TokenKindMatch end_set[] = {MatchPunct(")")};
//      Parser after = parser;
//      Advance(after);
//      SkipNewlines(after);
//      if (IsPunc(after, ")")) {
//        EmitTrailingCommaErr(parser, end_set);
//        after.diags = parser.diags;
//        return {after, ps};
//      }
//      SPEC_RULE("Parse-ParamTypeListTail-Comma");
//      ParseElemResult<TypeFuncParam> param = ParseParamType(after);
//      ps.push_back(param.elem);
//      return ParseParamTypeListTail(param.parser, std::move(ps));
//    }
//
// 5. ParseParamType function (Lines 195-212)
//    ─────────────────────────────────────────────────────────────────────────
//    ParseElemResult<TypeFuncParam> ParseParamType(Parser parser) {
//      if (IsKw(parser, "move")) {
//        SPEC_RULE("Parse-ParamType-Move");
//        Parser next = parser;
//        Advance(next);
//        ParseElemResult<std::shared_ptr<Type>> ty = ParseType(next);
//        TypeFuncParam param;
//        param.mode = ParamMode::Move;
//        param.type = ty.elem;
//        return {ty.parser, param};
//      }
//      SPEC_RULE("Parse-ParamType-Plain");
//      ParseElemResult<std::shared_ptr<Type>> ty = ParseType(parser);
//      TypeFuncParam param;
//      param.mode = std::nullopt;
//      param.type = ty.elem;
//      return {ty.parser, param};
//    }
//
// DEPENDENCIES:
// =============================================================================
// - ParseType function (recursive calls for param/return types)
// - MakeTypeNode, MakeTypePrim helpers (type_common.cpp)
// - TypeFunc, TypeFuncParam AST node types
// - ParamMode enum (Move variant)
// - Token predicates: IsPunc, IsOp, IsKw
// - Parser utilities: Tok, Advance, SkipNewlines, SpanBetween, TokSpan, AtEof
// - EmitParseSyntaxErr, EmitTrailingCommaErr diagnostic helpers
//
// =============================================================================
// REFACTORING NOTES:
// =============================================================================
// - HasFuncArrow performs O(n) lookahead to distinguish func from tuple types
// - Consider caching the lookahead result if performance is a concern
// - TypeFuncParam stores both the type and optional move mode
// - Trailing comma handling in param lists follows standard pattern
// - The arrow "->" is required; without it, error is emitted
// - Span covers from opening "(" to end of return type
// =============================================================================
