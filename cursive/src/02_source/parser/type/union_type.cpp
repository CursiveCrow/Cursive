// =============================================================================
// MIGRATION MAPPING: union_type.cpp
// =============================================================================
// This file should contain parsing logic for union types: T1 | T2 | T3
// representing tagged unions (sum types).
//
// SPEC REFERENCE: C0updated.md, Section 3.3.7, Lines 4664-4672
// =============================================================================
//
// FORMAL RULES FROM SPEC:
// -----------------------------------------------------------------------------
//
// **(Parse-UnionTail-None)** Lines 4664-4667
// ¬ IsOp(Tok(P), "|")
// ────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseUnionTail(P) ⇓ (P, [])
//
// **(Parse-UnionTail-Cons)** Lines 4669-4672
// IsOp(Tok(P), "|")
// Γ ⊢ ParseNonPermType(Advance(P)) ⇓ (P_1, t_1)
// Γ ⊢ ParseUnionTail(P_1) ⇓ (P_2, ts)
// ────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseUnionTail(P) ⇓ (P_2, [t_1] ++ ts)
//
// UNION CONSTRUCTION IN Parse-Type (Lines 4633-4636):
// -----------------------------------------------------------------------------
// Γ ⊢ ParsePermOpt(P) ⇓ (P_1, perm_opt)
// Γ ⊢ ParseNonPermType(P_1) ⇓ (P_2, base)
// Γ ⊢ ParseUnionTail(P_2) ⇓ (P_3, ts)
// base' = (base if ts = [] else TypeUnion([base] ++ ts))
// ...
//
// SEMANTICS:
// - Union types are unordered: A | B is equivalent to B | A
// - No subtyping between union members
// - Used for error handling: i32 | Error
// - Used for nullable types: Ptr<T>@Valid | Ptr<T>@Null
// - Examples: i32 | bool, string@Managed | IOError
//
// =============================================================================
// SOURCE FILE: cursive-bootstrap/src/02_syntax/parser_types.cpp
// =============================================================================
//
// CONTENT TO MIGRATE:
// -----------------------------------------------------------------------------
//
// 1. ParseUnionTail function (Lines 668-685)
//    ─────────────────────────────────────────────────────────────────────────
//    ParseElemResult<std::vector<std::shared_ptr<Type>>> ParseUnionTail(
//        Parser parser) {
//      if (!IsOp(parser, "|")) {
//        SPEC_RULE("Parse-UnionTail-None");
//        return {parser, {}};
//      }
//      SPEC_RULE("Parse-UnionTail-Cons");
//      Parser next = parser;
//      Advance(next);  // consume |
//      ParseElemResult<std::shared_ptr<Type>> head = ParseNonPermType(next);
//      ParseElemResult<std::vector<std::shared_ptr<Type>>> tail =
//          ParseUnionTail(head.parser);
//      std::vector<std::shared_ptr<Type>> elems;
//      elems.reserve(1 + tail.elem.size());
//      elems.push_back(head.elem);
//      elems.insert(elems.end(), tail.elem.begin(), tail.elem.end());
//      return {tail.parser, elems};
//    }
//
// 2. Union construction in ParseType (Lines 730-743)
//    ─────────────────────────────────────────────────────────────────────────
//    ParseElemResult<std::shared_ptr<Type>> base = ParseNonPermType(after_perm);
//    ParseElemResult<std::vector<std::shared_ptr<Type>>> tail =
//        ParseUnionTail(base.parser);
//
//    Parser out = tail.parser;
//
//    SPEC_RULE("Parse-Type");
//    std::shared_ptr<Type> merged = base.elem;
//    if (!tail.elem.empty()) {
//      TypeUnion uni;
//      uni.types.reserve(1 + tail.elem.size());
//      uni.types.push_back(base.elem);
//      uni.types.insert(uni.types.end(), tail.elem.begin(), tail.elem.end());
//      merged = MakeTypeNode(SpanBetween(start, out), uni);
//    }
//
// PARSING FLOW:
// -----------------------------------------------------------------------------
//   1. After parsing first non-permission type (base)
//   2. Call ParseUnionTail to collect additional types
//   3. ParseUnionTail:
//      a. If no "|" operator, return empty list
//      b. If "|" found:
//         - Advance past "|"
//         - Parse next non-permission type
//         - Recursively call ParseUnionTail for more types
//         - Prepend current type to recursive result
//   4. If tail is non-empty, wrap base + tail in TypeUnion
//
// ERROR HANDLING:
// -----------------------------------------------------------------------------
//   - Each member type parsing handles its own errors
//   - The "|" operator disambiguates from bitwise OR in expr context
//   - In type context, "|" always means union
//
// DEPENDENCIES:
// =============================================================================
// - ParseNonPermType function (recursive calls for each member)
// - MakeTypeNode helper (type_common.cpp)
// - TypeUnion AST node type
// - IsOp predicate
// - Parser utilities: Tok, Advance, SpanBetween
//
// =============================================================================
// REFACTORING NOTES:
// =============================================================================
// - Union types are parsed with right-recursion (ParseUnionTail)
// - Order is preserved in AST but types are semantically unordered
// - The "|" operator has lower precedence than most type constructors
// - Permission qualifiers apply to the entire union, not members
// - Consider normalizing/sorting union members in semantic analysis
// - Example: const (i32 | bool) vs (const i32) | (const bool)
// - The first form is valid; permission applies to whole union
// =============================================================================
