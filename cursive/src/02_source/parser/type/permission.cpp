// =============================================================================
// MIGRATION MAPPING: permission.cpp
// =============================================================================
// This file should contain parsing logic for permission qualifiers: const,
// unique, and shared that modify type access patterns.
//
// SPEC REFERENCE: C0updated.md, Section 3.3.7, Lines 4644-4662
// =============================================================================
//
// FORMAL RULES FROM SPEC:
// -----------------------------------------------------------------------------
//
// **(Parse-Perm-Const)** Lines 4644-4647
// IsKw(Tok(P), `const`)
// ────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParsePermOpt(P) ⇓ (Advance(P), `const`)
//
// **(Parse-Perm-Unique)** Lines 4649-4652
// IsKw(Tok(P), `unique`)
// ────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParsePermOpt(P) ⇓ (Advance(P), `unique`)
//
// **(Parse-Perm-Shared)** Lines 4654-4657
// IsKw(Tok(P), `shared`)
// ────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParsePermOpt(P) ⇓ (Advance(P), `shared`)
//
// **(Parse-Perm-None)** Lines 4659-4662
// ¬ (IsKw(Tok(P), `const`) ∨ IsKw(Tok(P), `unique`) ∨ IsKw(Tok(P), `shared`))
// ────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParsePermOpt(P) ⇓ (P, ⊥)
//
// PERMISSION LATTICE:
// -----------------------------------------------------------------------------
//   unique <: shared <: const
//   - unique: exclusive mutable access (no aliases)
//   - shared: synchronized shared access (requires key system)
//   - const: read-only access (unlimited aliases)
//
// SEMANTICS:
// - Permissions modify access patterns for types
// - Applied as prefix: const i32, unique Buffer, shared DataStore
// - Default (no permission) depends on context
// - Permission applies to the entire following type expression
//
// =============================================================================
// SOURCE FILE: cursive-bootstrap/src/02_syntax/parser_types.cpp
// =============================================================================
//
// CONTENT TO MIGRATE:
// -----------------------------------------------------------------------------
//
// 1. PermOptResult structure (Lines 126-129)
//    ─────────────────────────────────────────────────────────────────────────
//    struct PermOptResult {
//      Parser parser;
//      std::optional<TypePerm> perm;
//    };
//
// 2. ParsePermOpt function (Lines 131-153)
//    ─────────────────────────────────────────────────────────────────────────
//    PermOptResult ParsePermOpt(Parser parser) {
//      const Token* tok = Tok(parser);
//      if (tok && IsKwTok(*tok, "const")) {
//        SPEC_RULE("Parse-Perm-Const");
//        Parser next = parser;
//        Advance(next);
//        return {next, TypePerm::Const};
//      }
//      if (tok && IsKwTok(*tok, "unique")) {
//        SPEC_RULE("Parse-Perm-Unique");
//        Parser next = parser;
//        Advance(next);
//        return {next, TypePerm::Unique};
//      }
//      if (tok && IsKwTok(*tok, "shared")) {
//        SPEC_RULE("Parse-Perm-Shared");
//        Parser next = parser;
//        Advance(next);
//        return {next, TypePerm::Shared};
//      }
//      SPEC_RULE("Parse-Perm-None");
//      return {parser, std::nullopt};
//    }
//
// 3. Permission application in ParseType (Lines 689-750)
//    ─────────────────────────────────────────────────────────────────────────
//    ParseElemResult<std::shared_ptr<Type>> ParseType(Parser parser) {
//      Parser start = parser;
//      PermOptResult perm = ParsePermOpt(parser);
//      Parser after_perm = perm.parser;
//      // ... parse base type ...
//      if (perm.perm.has_value()) {
//        TypePermType perm_type;
//        perm_type.perm = *perm.perm;
//        perm_type.base = merged;
//        merged = MakeTypeNode(SpanBetween(start, out), perm_type);
//      }
//      return {out, merged};
//    }
//
// 4. IsKw helper (Lines 52-55)
//    ─────────────────────────────────────────────────────────────────────────
//    bool IsKw(const Parser& parser, std::string_view kw) {
//      const Token* tok = Tok(parser);
//      return tok && IsKwTok(*tok, kw);
//    }
//
// PARSING FLOW:
// -----------------------------------------------------------------------------
//   1. At start of type parsing, call ParsePermOpt
//   2. Check for const/unique/shared keywords
//   3. If found:
//      a. Record which permission
//      b. Advance past keyword
//   4. If not found:
//      a. Return nullopt (no permission)
//      b. Parser position unchanged
//   5. After parsing base type, wrap in TypePermType if permission present
//
// DEPENDENCIES:
// =============================================================================
// - TypePerm enum (Const, Unique, Shared)
// - TypePermType AST node type
// - MakeTypeNode helper (type_common.cpp)
// - IsKwTok predicate
// - Parser utilities: Tok, Advance
//
// =============================================================================
// REFACTORING NOTES:
// =============================================================================
// - const, unique, shared are keywords (not identifiers)
// - Permission is optional; absence means context-dependent default
// - Permission wraps the entire type: const (i32 | bool) not (const i32) | bool
// - The permission lattice affects subtyping in semantic analysis
// - Method receivers use shorthand: ~ (const), ~! (unique), ~% (shared)
// - Consider whether PermOptResult should be a standard ParseElemResult
// =============================================================================
