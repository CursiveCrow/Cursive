// =============================================================================
// MIGRATION MAPPING: raw_ptr_type.cpp
// =============================================================================
// This file should contain parsing logic for raw pointer types: *imm T and
// *mut T representing unsafe, untracked pointers.
//
// SPEC REFERENCE: C0updated.md, Section 3.3.7, Lines 4783-4786
// =============================================================================
//
// FORMAL RULES FROM SPEC:
// -----------------------------------------------------------------------------
//
// **(Parse-Raw-Pointer-Type)** Lines 4783-4786
// IsOp(Tok(P), "*")
// IsKw(Tok(Advance(P)), q)    q ∈ {`imm`, `mut`}
// Γ ⊢ ParseType(Advance(Advance(P))) ⇓ (P_1, t)
// ────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseNonPermType(P) ⇓ (P_1, TypeRawPtr(q, t))
//
// SEMANTICS:
// - Raw pointers are unsafe, untracked memory addresses
// - *imm T: immutable raw pointer (cannot write through it)
// - *mut T: mutable raw pointer (can read and write)
// - Usage requires unsafe block
// - Examples: *imm u8 (C's const char*), *mut i32 (C's int*)
//
// =============================================================================
// SOURCE FILE: cursive-bootstrap/src/02_syntax/parser_types.cpp
// =============================================================================
//
// CONTENT TO MIGRATE:
// -----------------------------------------------------------------------------
//
// 1. Raw pointer type parsing in ParseNonPermType (Lines 502-521)
//    ─────────────────────────────────────────────────────────────────────────
//    if (IsOpTok(*tok, "*")) {
//      Parser start = parser;
//      Parser next = parser;
//      Advance(next);  // consume *
//      const Token* qual = Tok(next);
//      if (qual && qual->kind == TokenKind::Keyword &&
//          (qual->lexeme == "imm" || qual->lexeme == "mut")) {
//        SPEC_RULE("Parse-Raw-Pointer-Type");
//        const RawPtrQual q =
//            qual->lexeme == "imm" ? RawPtrQual::Imm : RawPtrQual::Mut;
//        Advance(next);  // consume imm/mut
//        ParseElemResult<std::shared_ptr<Type>> elem = ParseType(next);
//        TypeRawPtr ptr;
//        ptr.qual = q;
//        ptr.element = elem.elem;
//        return {elem.parser, MakeTypeNode(SpanBetween(start, elem.parser), ptr)};
//      }
//      EmitParseSyntaxErr(next, TokSpan(next));
//      return {next, MakeTypePrim(SpanBetween(start, next), "!")};
//    }
//
// PARSING FLOW:
// -----------------------------------------------------------------------------
//   1. Match "*" operator
//   2. Advance past "*"
//   3. Expect "imm" or "mut" keyword
//   4. If found:
//      a. Determine qualifier (RawPtrQual::Imm or RawPtrQual::Mut)
//      b. Advance past qualifier
//      c. Parse element type via ParseType
//      d. Construct TypeRawPtr node
//   5. If qualifier missing:
//      a. Emit syntax error
//      b. Return error type
//
// ERROR HANDLING:
// -----------------------------------------------------------------------------
//   - If "*" is not followed by "imm" or "mut":
//     - Emit syntax error diagnostic
//     - Return error type (TypePrim with "!")
//   - Note: "*" alone (dereference operator) is handled in expression parsing
//
// DEPENDENCIES:
// =============================================================================
// - ParseType function (recursive call for element type)
// - MakeTypeNode, MakeTypePrim helpers (type_common.cpp)
// - TypeRawPtr AST node type
// - RawPtrQual enum (Imm, Mut)
// - Token predicates: IsOpTok
// - Parser utilities: Tok, Advance, SpanBetween, TokSpan
// - EmitParseSyntaxErr diagnostic helper
//
// =============================================================================
// REFACTORING NOTES:
// =============================================================================
// - Raw pointers are primarily for FFI interop with C
// - The "imm" and "mut" qualifiers are keywords, not identifiers
// - "*" as a type prefix is distinct from "*" as dereference operator (expr)
// - No state tracking unlike Ptr<T> safe pointers
// - Consider adding a note about FFI requirements (FfiSafe types)
// - Span covers from "*" to end of element type
// =============================================================================
