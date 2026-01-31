// =============================================================================
// MIGRATION MAPPING: type_bounds.cpp
// =============================================================================
// This file should contain parsing logic for type bounds: <: Bound1, Bound2
// used in generic parameter declarations.
//
// SPEC REFERENCE: C0updated.md, Section 3.3.6.13, Lines 3976-4009
// =============================================================================
//
// FORMAL RULES FROM SPEC:
// -----------------------------------------------------------------------------
//
// **(Parse-TypeParam)** Lines 3976-3979
// Γ ⊢ ParseIdent(P) ⇓ (P_1, name)
// Γ ⊢ ParseTypeBoundsOpt(P_1) ⇓ (P_2, bounds)
// Γ ⊢ ParseTypeDefaultOpt(P_2) ⇓ (P_3, default_opt)
// ────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseTypeParam(P) ⇓ (P_3, ⟨name, bounds, default_opt, ⊥⟩)
//
// **(Parse-TypeBoundsOpt-None)** Lines 3981-3984
// ¬ IsOp(Tok(P), "<:")
// ────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseTypeBoundsOpt(P) ⇓ (P, [])
//
// **(Parse-TypeBoundsOpt-Yes)** Lines 3986-3989
// IsOp(Tok(P), "<:")
// Γ ⊢ ParseClassBoundList(Advance(P)) ⇓ (P_1, bounds)
// ────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseTypeBoundsOpt(P) ⇓ (P_1, bounds)
//
// CLASS BOUND LIST RULES (Lines 3991-4009):
// -----------------------------------------------------------------------------
//
// **(Parse-ClassBoundList-Cons)** Lines 3991-3994
// Γ ⊢ ParseClassBound(P) ⇓ (P_1, b_1)
// Γ ⊢ ParseClassBoundListTail(P_1, [b_1]) ⇓ (P_2, bs)
// ────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseClassBoundList(P) ⇓ (P_2, bs)
//
// **(Parse-ClassBoundListTail-End)** Lines 3996-3999
// ¬ IsPunc(Tok(P), ",")
// ────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseClassBoundListTail(P, bs) ⇓ (P, bs)
//
// **(Parse-ClassBoundListTail-Cons)** Lines 4001-4004
// IsPunc(Tok(P), ",")
// Γ ⊢ ParseClassBound(Advance(P)) ⇓ (P_1, b)
// Γ ⊢ ParseClassBoundListTail(P_1, bs ++ [b]) ⇓ (P_2, bs')
// ────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseClassBoundListTail(P, bs) ⇓ (P_2, bs')
//
// **(Parse-ClassBound)** Lines 4006-4009
// Γ ⊢ ParseTypePath(P) ⇓ (P_1, path)
// Γ ⊢ ParseGenericArgsOpt(P_1) ⇓ (P_2, args_opt)
// ────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseClassBound(P) ⇓ (P_2, ⟨path, args_opt⟩)
//
// TYPE DEFAULT RULES (Lines 4011-4019):
// -----------------------------------------------------------------------------
//
// **(Parse-TypeDefaultOpt-None)** Lines 4011-4014
// ¬ IsOp(Tok(P), "=")
// ────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseTypeDefaultOpt(P) ⇓ (P, ⊥)
//
// **(Parse-TypeDefaultOpt-Yes)** Lines 4016-4019
// IsOp(Tok(P), "=")
// Γ ⊢ ParseType(Advance(P)) ⇓ (P_1, ty)
// ────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseTypeDefaultOpt(P) ⇓ (P_1, ty)
//
// SEMANTICS:
// - Type bounds constrain generic parameters: <T <: Comparable>
// - Multiple bounds separated by commas: <T <: Printable, Comparable>
// - Bounds are class paths with optional generic args
// - Type defaults provide fallback: <T = i32>
// - Examples: <T <: Clone>, <K <: Hash, Eq; V>
//
// =============================================================================
// SOURCE FILE: cursive-bootstrap/src/02_syntax/parser_decl.cpp
// =============================================================================
//
// CONTENT TO MIGRATE:
// -----------------------------------------------------------------------------
//
// NOTE: Type bounds parsing is in parser_decl.cpp as part of generic params.
//
// 1. ParseTypeBoundsOpt function (approximate):
//    ─────────────────────────────────────────────────────────────────────────
//    ParseElemResult<std::vector<ClassBound>> ParseTypeBoundsOpt(Parser parser) {
//      if (!IsOp(parser, "<:")) {
//        return {parser, {}};
//      }
//      Parser next = parser;
//      Advance(next);  // consume <:
//      return ParseClassBoundList(next);
//    }
//
// 2. ParseClassBoundList function (approximate):
//    ─────────────────────────────────────────────────────────────────────────
//    ParseElemResult<std::vector<ClassBound>> ParseClassBoundList(Parser parser) {
//      ParseElemResult<ClassBound> first = ParseClassBound(parser);
//      std::vector<ClassBound> bounds;
//      bounds.push_back(first.elem);
//      return ParseClassBoundListTail(first.parser, std::move(bounds));
//    }
//
// 3. ParseClassBoundListTail function (approximate):
//    ─────────────────────────────────────────────────────────────────────────
//    ParseElemResult<std::vector<ClassBound>> ParseClassBoundListTail(
//        Parser parser, std::vector<ClassBound> bounds) {
//      if (!IsPunc(parser, ",")) {
//        return {parser, bounds};
//      }
//      Parser next = parser;
//      Advance(next);  // consume ,
//      ParseElemResult<ClassBound> bound = ParseClassBound(next);
//      bounds.push_back(bound.elem);
//      return ParseClassBoundListTail(bound.parser, std::move(bounds));
//    }
//
// 4. ParseClassBound function (approximate):
//    ─────────────────────────────────────────────────────────────────────────
//    ParseElemResult<ClassBound> ParseClassBound(Parser parser) {
//      ParseElemResult<TypePath> path = ParseTypePath(parser);
//      ParseElemResult<std::optional<std::vector<Type>>> args =
//          ParseGenericArgsOpt(path.parser);
//      ClassBound bound;
//      bound.path = std::move(path.elem);
//      bound.args = std::move(args.elem);
//      return {args.parser, bound};
//    }
//
// 5. ParseTypeDefaultOpt function (approximate):
//    ─────────────────────────────────────────────────────────────────────────
//    ParseElemResult<std::optional<std::shared_ptr<Type>>> ParseTypeDefaultOpt(
//        Parser parser) {
//      if (!IsOp(parser, "=")) {
//        return {parser, std::nullopt};
//      }
//      Parser next = parser;
//      Advance(next);  // consume =
//      ParseElemResult<std::shared_ptr<Type>> ty = ParseType(next);
//      return {ty.parser, ty.elem};
//    }
//
// PARSING FLOW:
// -----------------------------------------------------------------------------
//   1. Parse type parameter name (identifier)
//   2. Check for "<:" operator
//   3. If found:
//      a. Advance past "<:"
//      b. Parse first class bound
//      c. While "," found:
//         - Advance past ","
//         - Parse next class bound
//   4. Check for "=" operator (default)
//   5. If found:
//      a. Advance past "="
//      b. Parse default type
//
// DEPENDENCIES:
// =============================================================================
// - ParseTypePath function (type_path.cpp)
// - ParseGenericArgsOpt function (type_args.cpp)
// - ParseType function (recursive for defaults)
// - ParseIdent function (common)
// - ClassBound AST node type
// - Token predicates: IsOp, IsPunc
// - Parser utilities: Tok, Advance
//
// =============================================================================
// REFACTORING NOTES:
// =============================================================================
// - The "<:" operator is specific to type bounds (subtype constraint)
// - Bounds are class references, not arbitrary types
// - Multiple bounds represent intersection (must satisfy all)
// - Type defaults are used when no argument is provided at instantiation
// - Consider whether bounds should allow non-class types in future
// - This is part of generic parameter parsing, may belong in items/generics.cpp
// =============================================================================
