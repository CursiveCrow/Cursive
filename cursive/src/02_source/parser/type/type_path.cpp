// =============================================================================
// MIGRATION MAPPING: type_path.cpp
// =============================================================================
// This file should contain parsing logic for type paths: qualified names
// like MyType, module::SubModule::Type representing type references.
//
// SPEC REFERENCE: C0updated.md, Section 3.3.6.13, Lines 3786-3805
// Section 3.3.7, Lines 4818-4821 (Parse-Type-Path rule)
// =============================================================================
//
// FORMAL RULES FROM SPEC:
// -----------------------------------------------------------------------------
//
// **(Parse-TypePath)** Lines 3786-3789
// Γ ⊢ ParseIdent(P) ⇓ (P_1, id)
// Γ ⊢ ParseTypePathTail(P_1, [id]) ⇓ (P_2, path)
// ────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseTypePath(P) ⇓ (P_2, path)
//
// **(Parse-TypePathTail-End)** Lines 3797-3800
// ¬ IsOp(Tok(P), "::")
// ────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseTypePathTail(P, xs) ⇓ (P, xs)
//
// **(Parse-TypePathTail-Cons)** Lines 3802-3805
// IsOp(Tok(P), "::")
// Γ ⊢ ParseIdent(Advance(P)) ⇓ (P_1, id)
// Γ ⊢ ParseTypePathTail(P_1, xs ++ [id]) ⇓ (P_2, ys)
// ────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseTypePathTail(P, xs) ⇓ (P_2, ys)
//
// TYPE PATH AS TYPE (Parse-Type-Path) Lines 4818-4821:
// -----------------------------------------------------------------------------
// Γ ⊢ ParseTypePath(P) ⇓ (P_1, path)
// ¬ IsOp(Tok(P_1), "@")
// ¬ IsOp(Tok(P_1), "<")
// ¬ BuiltinTypePath(path)
// ────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseNonPermType(P) ⇓ (P_1, TypePath(path))
//
// SEMANTICS:
// - Type paths are qualified names referencing user-defined types
// - Simple: MyStruct, Point, Connection
// - Qualified: collections::HashMap, std::io::File
// - The "::" separator indicates module path components
// - Excludes built-in types (handled by IsPrimLexemeSet, string, bytes)
//
// =============================================================================
// SOURCE FILE: cursive-bootstrap/src/02_syntax/parser_types.cpp
// =============================================================================
//
// CONTENT TO MIGRATE:
// -----------------------------------------------------------------------------
//
// NOTE: ParseTypePath is defined in parser_decl.cpp (shared with items).
// The type_path.cpp file should either re-export or call into that shared impl.
//
// SOURCE FILE: cursive-bootstrap/src/02_syntax/parser_decl.cpp
// (Approximate location - ParseTypePath definition)
//
// 1. ParseTypePath function structure:
//    ─────────────────────────────────────────────────────────────────────────
//    ParseElemResult<TypePath> ParseTypePath(Parser parser) {
//      ParseElemResult<Identifier> first = ParseIdent(parser);
//      if (!first.elem.has_value()) {
//        // Error handling
//      }
//      return ParseTypePathTail(first.parser, {*first.elem});
//    }
//
// 2. ParseTypePathTail function:
//    ─────────────────────────────────────────────────────────────────────────
//    ParseElemResult<TypePath> ParseTypePathTail(Parser parser, TypePath path) {
//      if (!IsOp(parser, "::")) {
//        return {parser, path};
//      }
//      Parser next = parser;
//      Advance(next);  // consume ::
//      ParseElemResult<Identifier> id = ParseIdent(next);
//      path.push_back(id.elem);
//      return ParseTypePathTail(id.parser, std::move(path));
//    }
//
// 3. Type path as type (in ParseNonPermType, Lines 649-661):
//    ─────────────────────────────────────────────────────────────────────────
//    if (!after_args || (!IsOpTok(*after_args, "@") && !IsOpTok(*after_args, "<") &&
//                  !BuiltinTypePath(path.elem))) {
//      SPEC_RULE("Parse-Type-Path");
//      TypePathType ty_path;
//      ty_path.path = std::move(path.elem);
//      ty_path.generic_args = std::move(args);
//      return {cur, MakeTypeNode(SpanBetween(start, cur), ty_path)};
//    }
//
// 4. BuiltinTypePath predicate (Lines 73-82):
//    ─────────────────────────────────────────────────────────────────────────
//    bool BuiltinTypePath(const TypePath& path) {
//      if (path.size() != 1) {
//        return false;
//      }
//      const std::string_view name = path[0];
//      if (IsPrimLexemeSet(name)) {
//        return true;
//      }
//      return name == "string" || name == "bytes";
//    }
//
// PARSING FLOW:
// -----------------------------------------------------------------------------
//   1. Parse first identifier
//   2. While "::" follows:
//      a. Advance past "::"
//      b. Parse next identifier
//      c. Append to path
//   3. Return accumulated path
//
// DEPENDENCIES:
// =============================================================================
// - ParseIdent function (common identifier parsing)
// - TypePath type (std::vector<Identifier>)
// - BuiltinTypePath predicate (type_common.cpp)
// - IsPrimLexemeSet predicate (primitive_type.cpp)
// - Token predicates: IsOp
// - Parser utilities: Tok, Advance
//
// =============================================================================
// REFACTORING NOTES:
// =============================================================================
// - Type paths are shared between type references and item declarations
// - Consider placing ParseTypePath in a shared module
// - The "::" operator must not be confused with other uses
// - BuiltinTypePath excludes primitive types from being treated as user types
// - Single-element paths are common (simple type names)
// - Long paths indicate deep module nesting
// - TypePath is essentially vector<Identifier> or similar
// =============================================================================
