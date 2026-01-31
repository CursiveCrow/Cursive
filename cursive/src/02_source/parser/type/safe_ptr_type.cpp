// =============================================================================
// MIGRATION MAPPING: safe_ptr_type.cpp
// =============================================================================
// This file should contain parsing logic for safe pointer types: Ptr<T>@State
// where State can be Valid, Null, or Expired.
//
// SPEC REFERENCE: C0updated.md, Section 3.3.7, Lines 4773-4781
// Section 3.3.7.1 (Pointer State), Lines 4915-4935
// =============================================================================
//
// FORMAL RULES FROM SPEC:
// -----------------------------------------------------------------------------
//
// **(Parse-Safe-Pointer-Type-ShiftSplit)** Lines 4773-4776
// IsIdent(Tok(P))    Lexeme(Tok(P)) = `Ptr`
// IsOp(Tok(Advance(P)), "<")
// Γ ⊢ ParseType(Advance(Advance(P))) ⇓ (P_1, t)
// IsOp(Tok(P_1), ">>")
// P_1' = SplitShiftR(P_1)
// Γ ⊢ ParsePtrState(Advance(P_1')) ⇓ (P_2, st_opt)
// ────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseNonPermType(P) ⇓ (P_2, TypePtr(t, st_opt))
//
// **(Parse-Safe-Pointer-Type)** Lines 4778-4781
// IsIdent(Tok(P))    Lexeme(Tok(P)) = `Ptr`
// IsOp(Tok(Advance(P)), "<")
// Γ ⊢ ParseType(Advance(Advance(P))) ⇓ (P_1, t)
// IsOp(Tok(P_1), ">")
// Γ ⊢ ParsePtrState(Advance(P_1)) ⇓ (P_2, st_opt)
// ────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseNonPermType(P) ⇓ (P_2, TypePtr(t, st_opt))
//
// POINTER STATE RULES (Lines 4917-4935):
// -----------------------------------------------------------------------------
//
// **(Parse-PtrState-None)** Lines 4917-4920
// ¬ IsOp(Tok(P), "@")
// ────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParsePtrState(P) ⇓ (P, ⊥)
//
// **(Parse-PtrState-Valid)** Lines 4922-4925
// IsOp(Tok(P), "@")    IsIdent(Tok(Advance(P)))    Lexeme(Tok(Advance(P))) = `Valid`
// ────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParsePtrState(P) ⇓ (Advance(Advance(P)), `Valid`)
//
// **(Parse-PtrState-Null)** Lines 4927-4930
// IsOp(Tok(P), "@")    IsIdent(Tok(Advance(P)))    Lexeme(Tok(Advance(P))) = `Null`
// ────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParsePtrState(P) ⇓ (Advance(Advance(P)), `Null`)
//
// **(Parse-PtrState-Expired)** Lines 4932-4935
// IsOp(Tok(P), "@")    IsIdent(Tok(Advance(P)))    Lexeme(Tok(Advance(P))) = `Expired`
// ────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParsePtrState(P) ⇓ (Advance(Advance(P)), `Expired`)
//
// SEMANTICS:
// - Safe pointers have tracked validity states: Valid, Null, Expired
// - Syntax: Ptr<T>, Ptr<T>@Valid, Ptr<T>@Null, Ptr<T>@Expired
// - Without @State annotation, the state is unspecified (any of the three)
// - The >> token must be split for nested generics: Ptr<Ptr<T>>
// - Examples: Ptr<i32>@Valid, Ptr<Buffer>@Null
//
// =============================================================================
// SOURCE FILE: cursive-bootstrap/src/02_syntax/parser_types.cpp
// =============================================================================
//
// CONTENT TO MIGRATE:
// -----------------------------------------------------------------------------
//
// 1. ParsePtrState function (Lines 258-283)
//    ─────────────────────────────────────────────────────────────────────────
//    ParseElemResult<std::optional<PtrState>> ParsePtrState(Parser parser) {
//      if (!IsOp(parser, "@")) {
//        SPEC_RULE("Parse-PtrState-None");
//        return {parser, std::nullopt};
//      }
//      Parser next = parser;
//      Advance(next);
//      const Token* tok = Tok(next);
//      if (tok && IsIdentTok(*tok) && tok->lexeme == "Valid") {
//        SPEC_RULE("Parse-PtrState-Valid");
//        Advance(next);
//        return {next, PtrState::Valid};
//      }
//      if (tok && IsIdentTok(*tok) && tok->lexeme == "Null") {
//        SPEC_RULE("Parse-PtrState-Null");
//        Advance(next);
//        return {next, PtrState::Null};
//      }
//      if (tok && IsIdentTok(*tok) && tok->lexeme == "Expired") {
//        SPEC_RULE("Parse-PtrState-Expired");
//        Advance(next);
//        return {next, PtrState::Expired};
//      }
//      EmitParseSyntaxErr(next, TokSpan(next));
//      return {next, std::nullopt};
//    }
//
// 2. ParseSafePointerType function (Lines 382-417)
//    ─────────────────────────────────────────────────────────────────────────
//    ParseElemResult<std::shared_ptr<Type>> ParseSafePointerType(Parser parser) {
//      Parser start = parser;
//      Parser after_ident = parser;
//      Advance(after_ident);  // consume Ptr
//      if (!IsOp(after_ident, "<")) {
//        EmitParseSyntaxErr(after_ident, TokSpan(after_ident));
//        return {after_ident, MakeTypePrim(SpanBetween(start, after_ident), "!")};
//      }
//      Parser after_lt = after_ident;
//      Advance(after_lt);  // consume <
//      ParseElemResult<std::shared_ptr<Type>> elem = ParseType(after_lt);
//      const Token* close = Tok(elem.parser);
//
//      // Handle >> case (nested generics)
//      if (close && IsOpTok(*close, ">>")) {
//        SPEC_RULE("Parse-Safe-Pointer-Type-ShiftSplit");
//        Parser split = SplitShiftR(elem.parser);
//        Parser after_left = split;
//        Advance(after_left);
//        ParseElemResult<std::optional<PtrState>> st = ParsePtrState(after_left);
//        TypePtr ptr;
//        ptr.element = elem.elem;
//        ptr.state = st.elem;
//        return {st.parser, MakeTypeNode(SpanBetween(start, st.parser), ptr)};
//      }
//
//      // Handle > case (normal)
//      if (close && IsOpTok(*close, ">")) {
//        SPEC_RULE("Parse-Safe-Pointer-Type");
//        Parser after_gt = elem.parser;
//        Advance(after_gt);
//        ParseElemResult<std::optional<PtrState>> st = ParsePtrState(after_gt);
//        TypePtr ptr;
//        ptr.element = elem.elem;
//        ptr.state = st.elem;
//        return {st.parser, MakeTypeNode(SpanBetween(start, st.parser), ptr)};
//      }
//
//      EmitParseSyntaxErr(elem.parser, TokSpan(elem.parser));
//      return {elem.parser, MakeTypePrim(SpanBetween(start, elem.parser), "!")};
//    }
//
// 3. Ptr type detection in ParseNonPermType (Lines 585-591)
//    ─────────────────────────────────────────────────────────────────────────
//    if (lexeme == "Ptr") {
//      Parser after_ident = parser;
//      Advance(after_ident);
//      if (IsOp(after_ident, "<")) {
//        return ParseSafePointerType(parser);
//      }
//    }
//
// DEPENDENCIES:
// =============================================================================
// - ParseType function (recursive call for element type)
// - SplitShiftR function (lexer utility for >> splitting)
// - MakeTypeNode, MakeTypePrim helpers (type_common.cpp)
// - TypePtr AST node type
// - PtrState enum (Valid, Null, Expired)
// - Token predicates: IsOp, IsOpTok, IsIdentTok
// - Parser utilities: Tok, Advance, SpanBetween, TokSpan
// - EmitParseSyntaxErr diagnostic helper
//
// =============================================================================
// REFACTORING NOTES:
// =============================================================================
// - The >> token splitting is necessary for nested generics like Ptr<Ptr<T>>
// - SplitShiftR modifies the token stream, replacing >> with two > tokens
// - State annotation is optional; absence means "any state" (type checking)
// - The Ptr identifier is NOT a keyword, just a special type name
// - Consider caching whether < follows Ptr to avoid redundant lookahead
// - Span covers from "Ptr" to end of state annotation (or ">" if no state)
// =============================================================================
