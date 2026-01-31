// =============================================================================
// MIGRATION MAPPING: record_literal.cpp
// =============================================================================
// This file should contain parsing logic for record literal expressions,
// including simple record literals and modal state record literals.
//
// SPEC REFERENCE: C0updated.md
// -----------------------------------------------------------------------------
// **(Parse-Record-Literal)** Lines 5224-5227
//   Γ ⊢ ParseTypePath(P) ⇓ (P_1, path)    |path| = 1
//   IsPunc(Tok(P_1), "{")
//   Γ ⊢ ParseFieldInitList(Advance(P_1)) ⇓ (P_2, fields)    IsPunc(Tok(P_2), "}")
//   ────────────────────────────────────────────────────────────────────────────
//   Γ ⊢ ParsePrimary(P) ⇓ (Advance(P_2), RecordExpr(TypePath(path), fields))
//
// **(Parse-Record-Literal-ModalState)** Lines 5219-5222
//   Γ ⊢ ParseModalTypeRef(P) ⇓ (P_1, modal_ref)    IsOp(Tok(P_1), "@")
//   Γ ⊢ ParseIdent(Advance(P_1)) ⇓ (P_2, state)    IsPunc(Tok(P_2), "{")
//   Γ ⊢ ParseFieldInitList(Advance(P_2)) ⇓ (P_3, fields)    IsPunc(Tok(P_3), "}")
//   ────────────────────────────────────────────────────────────────────────────
//   Γ ⊢ ParsePrimary(P) ⇓ (Advance(P_3), RecordExpr(ModalStateRef(modal_ref, state), fields))
//
// **Field Initializer Rules** (Lines 5878-5988)
//
// **(Parse-FieldInitList-Empty)** Lines 5880-5883
//   IsPunc(Tok(P), "}")    Γ ⊢ Emit(Code(Unsupported-Construct))
//   ────────────────────────────────────────────────────────────────────────────
//   Γ ⊢ ParseFieldInitList(P) ⇓ (P, [])
//
// **(Parse-FieldInitList-Cons)** Lines 5885-5888
//   Γ ⊢ ParseFieldInit(P) ⇓ (P_1, f)
//   Γ ⊢ ParseFieldInitTail(P_1, [f]) ⇓ (P_2, fs)
//   ────────────────────────────────────────────────────────────────────────────
//   Γ ⊢ ParseFieldInitList(P) ⇓ (P_2, fs)
//
// **(Parse-FieldInit-Explicit)** Lines 5891-5894
//   Γ ⊢ ParseIdent(P) ⇓ (P_1, name)    IsPunc(Tok(P_1), ":")
//   Γ ⊢ ParseExpr(Advance(P_1)) ⇓ (P_2, e)
//   ────────────────────────────────────────────────────────────────────────────
//   Γ ⊢ ParseFieldInit(P) ⇓ (P_2, ⟨name, e⟩)
//
// **(Parse-FieldInit-Shorthand)** Lines 5896-5899
//   Γ ⊢ ParseIdent(P) ⇓ (P_1, name)    ¬ IsPunc(Tok(P_1), ":")
//   ────────────────────────────────────────────────────────────────────────────
//   Γ ⊢ ParseFieldInit(P) ⇓ (P_1, ⟨name, Identifier(name)⟩)
//
// **(Parse-FieldInitTail-End)** Lines 5975-5978
//   IsPunc(Tok(P), "}")
//   ────────────────────────────────────────────────────────────────────────────
//   Γ ⊢ ParseFieldInitTail(P, xs) ⇓ (P, xs)
//
// **(Parse-FieldInitTail-TrailingComma)** Lines 5980-5983
//   IsPunc(Tok(P), ",")    IsPunc(Tok(Advance(P)), "}")
//   TrailingCommaAllowed(P_0, P, {Punctuator("}")})
//   ────────────────────────────────────────────────────────────────────────────
//   Γ ⊢ ParseFieldInitTail(P, xs) ⇓ (Advance(P), xs)
//
// **(Parse-FieldInitTail-Comma)** Lines 5985-5988
//   IsPunc(Tok(P), ",")    Γ ⊢ ParseFieldInit(Advance(P)) ⇓ (P_1, f)
//   Γ ⊢ ParseFieldInitTail(P_1, xs ++ [f]) ⇓ (P_2, ys)
//   ────────────────────────────────────────────────────────────────────────────
//   Γ ⊢ ParseFieldInitTail(P, xs) ⇓ (P_2, ys)
//
// SEMANTICS:
// - RecordName{ field: value, ... } creates a record literal
// - RecordName{ field } is shorthand for RecordName{ field: field }
// - ModalType@State{ fields } creates a modal state literal
// - Empty field lists {} emit Unsupported-Construct in bootstrap
// - Trailing comma allowed only on multi-line
//
// SOURCE FILE: cursive-bootstrap/src/02_syntax/parser_expr.cpp
//
// CONTENT TO MIGRATE:
// -----------------------------------------------------------------------------
// 1. Simple record literal parsing
//    Source: parser_expr.cpp, lines 1386-1410
//    Purpose: Parse TypeName{ field: value, ... }
//    SPEC_RULE: "Parse-Record-Literal"
//
// 2. Modal state record literal parsing
//    Source: parser_expr.cpp, lines 1355-1384
//    Purpose: Parse ModalType@State{ fields }
//    SPEC_RULE: "Parse-Record-Literal-ModalState"
//
// 3. Generic record literal parsing (with type args)
//    Source: parser_expr.cpp, lines 1327-1353
//    Purpose: Parse GenericType<T>{ fields } or GenericType<T>@State{ fields }
//    Note: Generic args parsing interleaved with modal/record dispatch
//
// 4. ParseFieldInitList function
//    Source: parser_expr.cpp, lines 2220-2232
//    Purpose: Parse comma-separated field initializer list
//    SPEC_RULE: "Parse-FieldInitList-Empty", "Parse-FieldInitList-Cons"
//
// 5. ParseFieldInit function
//    Source: parser_expr.cpp, lines 2234-2257
//    Purpose: Parse individual field: expr or shorthand field
//    SPEC_RULE: "Parse-FieldInit-Explicit", "Parse-FieldInit-Shorthand"
//
// 6. ParseFieldInitTail function
//    Source: parser_expr.cpp, lines 2259-2284
//    Purpose: Parse tail of field initializer list
//    SPEC_RULE: "Parse-FieldInitTail-End", "Parse-FieldInitTail-TrailingComma",
//               "Parse-FieldInitTail-Comma"
//
// DEPENDENCIES:
// - Requires: ParseTypePath (from parser_paths.cpp)
// - Requires: ParseIdent (from parser_paths.cpp)
// - Requires: ParseExpr (recursive expression parsing)
// - Requires: ParseType (for generic arguments)
// - Requires: RecordExpr, FieldInit AST node types
// - Requires: TypePath, ModalStateRef, GenericTypeRef variant types
// - Requires: IsPunc, IsOp, Tok, Advance helpers
// - Requires: SkipNewlines for multi-line handling
// - Requires: EmitTrailingCommaErr for single-line trailing comma error
// - Requires: EmitUnsupportedConstruct for empty field list
// - Requires: MakeExpr, SpanBetween helpers
// - Requires: allow_brace parameter (only parse when allow_brace=true)
//
// REFACTORING NOTES:
// - Bootstrap handles generic args inline with record literal parsing
// - Consider separating generic type ref parsing from record literal
// - FieldInit has both explicit (name: expr) and shorthand (name) forms
// - Empty field lists are currently unsupported (bootstrap restriction)
// - Modal state refs can have generic args: ModalType<T>@State{ fields }
// - The allow_brace parameter prevents parsing in no-brace contexts
// =============================================================================
