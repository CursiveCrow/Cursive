// =============================================================================
// MIGRATION MAPPING: primitive_type.cpp
// =============================================================================
// This file should contain parsing logic for primitive types (i32, u64, bool,
// char, unit type "()", and never type "!").
//
// SPEC REFERENCE: C0updated.md, Section 3.3.7, Lines 4695-4711
// =============================================================================
//
// PREDICATE DEFINITIONS (Lines 4695-4696):
// -----------------------------------------------------------------------------
//   PrimLexemeSet = PrimTypes_C0 \ {"()", "!"}
//     = {i8, i16, i32, i64, i128, u8, u16, u32, u64, u128, isize, usize,
//        f16, f32, f64, bool, char}
//
//   BuiltinTypePath(path) ⇔
//     (|path| = 1 ∧ path[0] ∈ PrimLexemeSet) ∨
//     path = ["string"] ∨
//     path = ["bytes"]
//
// FORMAL RULES FROM SPEC:
// -----------------------------------------------------------------------------
//
// **(Parse-Prim-Type)** Lines 4698-4701
// IsIdent(Tok(P))    Lexeme(Tok(P)) ∈ PrimLexemeSet
// ────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseNonPermType(P) ⇓ (Advance(P), TypePrim(Lexeme(Tok(P))))
//
// **(Parse-Unit-Type)** Lines 4703-4706
// IsPunc(Tok(P), "(")    IsPunc(Tok(Advance(P)), ")")
// ────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseNonPermType(P) ⇓ (Advance(Advance(P)), TypePrim("()"))
//
// **(Parse-Never-Type)** Lines 4708-4711
// IsOp(Tok(P), "!")
// ────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseNonPermType(P) ⇓ (Advance(P), TypePrim("!"))
//
// SEMANTICS:
// - Primitive types are the basic scalar types of the language
// - Unit type "()" represents absence of value / void-like
// - Never type "!" represents computations that don't return
// - All primitive type names are identifiers (not keywords)
//
// =============================================================================
// SOURCE FILE: cursive-bootstrap/src/02_syntax/parser_types.cpp
// =============================================================================
//
// CONTENT TO MIGRATE:
// -----------------------------------------------------------------------------
//
// 1. IsIntTypeLexeme predicate (Lines 57-62)
//    ─────────────────────────────────────────────────────────────────────────
//    bool IsIntTypeLexeme(std::string_view lexeme) {
//      return lexeme == "i8" || lexeme == "i16" || lexeme == "i32" ||
//             lexeme == "i64" || lexeme == "i128" || lexeme == "u8" ||
//             lexeme == "u16" || lexeme == "u32" || lexeme == "u64" ||
//             lexeme == "u128" || lexeme == "isize" || lexeme == "usize";
//    }
//
// 2. IsFloatTypeLexeme predicate (Lines 64-66)
//    ─────────────────────────────────────────────────────────────────────────
//    bool IsFloatTypeLexeme(std::string_view lexeme) {
//      return lexeme == "f16" || lexeme == "f32" || lexeme == "f64";
//    }
//
// 3. IsPrimLexemeSet predicate (Lines 68-71)
//    ─────────────────────────────────────────────────────────────────────────
//    bool IsPrimLexemeSet(std::string_view lexeme) {
//      return IsIntTypeLexeme(lexeme) || IsFloatTypeLexeme(lexeme) ||
//             lexeme == "bool" || lexeme == "char";
//    }
//
// 4. Primitive type parsing in ParseNonPermType (Lines 556-564)
//    ─────────────────────────────────────────────────────────────────────────
//    if (IsPrimLexemeSet(lexeme)) {
//      SPEC_RULE("Parse-Prim-Type");
//      Parser next = parser;
//      Advance(next);
//      return {next,
//              MakeTypeNode(SpanBetween(parser, next),
//                           TypePrim{Identifier{lexeme}})};
//    }
//
// 5. Unit type parsing in ParseNonPermType (Lines 434-446)
//    ─────────────────────────────────────────────────────────────────────────
//    if (elems.elem.empty()) {
//      if (!IsPunc(elems.parser, ")")) {
//        EmitParseSyntaxErr(elems.parser, TokSpan(elems.parser));
//        return {elems.parser,
//                MakeTypePrim(SpanBetween(parser, elems.parser), "!")};
//      }
//      SPEC_RULE("Parse-Unit-Type");
//      Parser after_r = elems.parser;
//      Advance(after_r);
//      return {after_r,
//              MakeTypeNode(SpanBetween(parser, after_r),
//                           TypePrim{Identifier{"()"}})};
//    }
//
// 6. Never type parsing in ParseNonPermType (Lines 493-500)
//    ─────────────────────────────────────────────────────────────────────────
//    if (IsOpTok(*tok, "!")) {
//      SPEC_RULE("Parse-Never-Type");
//      Parser next = parser;
//      Advance(next);
//      return {next,
//              MakeTypeNode(SpanBetween(parser, next),
//                           TypePrim{Identifier{"!"}})};
//    }
//
// 7. MakeTypePrim helper (Lines 37-40)
//    ─────────────────────────────────────────────────────────────────────────
//    std::shared_ptr<Type> MakeTypePrim(const core::Span& span,
//                                       std::string_view name) {
//      return MakeTypeNode(span, TypePrim{Identifier{name}});
//    }
//
// DEPENDENCIES:
// =============================================================================
// - MakeTypeNode helper (type_common.cpp)
// - TypePrim AST node type
// - Identifier type
// - Token predicates: IsIdentTok, IsPuncTok, IsOpTok
// - Parser utilities: Tok, Advance, TokSpan, SpanBetween
//
// =============================================================================
// REFACTORING NOTES:
// =============================================================================
// - Unit type is parsed in the tuple type logic (empty parentheses case)
// - Never type uses the "!" operator token, not an identifier
// - Consider consolidating primitive type checks into a single lookup table
// - MakeTypePrim is a convenience wrapper over MakeTypeNode
// - The predicates (IsIntTypeLexeme, etc.) could be moved to type_common.cpp
// =============================================================================
