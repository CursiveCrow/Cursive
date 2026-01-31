// =============================================================================
// MIGRATION MAPPING: dynamic_type.cpp
// =============================================================================
// This file should contain parsing logic for dynamic/capability types: $ClassName
// representing runtime-polymorphic class objects.
//
// SPEC REFERENCE: C0updated.md, Section 3.3.7, Lines 4798-4801
// =============================================================================
//
// FORMAL RULES FROM SPEC:
// -----------------------------------------------------------------------------
//
// **(Parse-Dynamic-Type)** Lines 4798-4801
// IsOp(Tok(P), "$")
// Γ ⊢ ParseTypePath(Advance(P)) ⇓ (P_1, path)
// ────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseNonPermType(P) ⇓ (P_1, TypeDynamic(path))
//
// SEMANTICS:
// - Dynamic types are runtime-polymorphic class objects
// - Used for capability types: $FileSystem, $HeapAllocator, $ExecutionDomain
// - The $ prefix indicates dynamic dispatch
// - Type path refers to a class declaration
// - Examples: $FileSystem, $HeapAllocator, $Comparable
//
// =============================================================================
// SOURCE FILE: cursive-bootstrap/src/02_syntax/parser_types.cpp
// =============================================================================
//
// CONTENT TO MIGRATE:
// -----------------------------------------------------------------------------
//
// 1. Dynamic type parsing in ParseNonPermType (Lines 523-532)
//    ─────────────────────────────────────────────────────────────────────────
//    if (IsOpTok(*tok, "$")) {
//      SPEC_RULE("Parse-Dynamic-Type");
//      Parser start = parser;
//      Parser next = parser;
//      Advance(next);  // consume $
//      ParseElemResult<TypePath> path = ParseTypePath(next);
//      TypeDynamic dyn;
//      dyn.path = std::move(path.elem);
//      return {path.parser, MakeTypeNode(SpanBetween(start, path.parser), dyn)};
//    }
//
// PARSING FLOW:
// -----------------------------------------------------------------------------
//   1. Match "$" operator
//   2. Advance past "$"
//   3. Parse type path via ParseTypePath (e.g., FileSystem, pkg::SomeClass)
//   4. Construct TypeDynamic node with the path
//
// ERROR HANDLING:
// -----------------------------------------------------------------------------
//   - Errors in type path parsing are handled by ParseTypePath
//   - The "$" must be immediately followed by a valid type path
//
// DEPENDENCIES:
// =============================================================================
// - ParseTypePath function (type_path.cpp)
// - MakeTypeNode helper (type_common.cpp)
// - TypeDynamic AST node type
// - TypePath type (vector of identifiers)
// - Token predicates: IsOpTok
// - Parser utilities: Tok, Advance, SpanBetween
//
// =============================================================================
// REFACTORING NOTES:
// =============================================================================
// - The "$" prefix is unique to dynamic types; no ambiguity with other uses
// - Dynamic types are used for capability-based security model
// - Capability types from Context: $FileSystem, $HeapAllocator, $ExecutionDomain
// - Dynamic types are NOT FfiSafe (cannot cross FFI boundary)
// - The path could include module qualification: $mymodule::MyClass
// - Span covers from "$" to end of type path
// =============================================================================
