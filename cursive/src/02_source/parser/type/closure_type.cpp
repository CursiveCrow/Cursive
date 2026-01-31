// =============================================================================
// MIGRATION MAPPING: closure_type.cpp
// =============================================================================
// This file should contain parsing logic for closure types: |T1, T2| -> R
// representing anonymous function types with optional captured dependencies.
//
// SPEC REFERENCE: C0updated.md, Section 3.3.7, Lines 4718-4756
// =============================================================================
//
// FORMAL RULES FROM SPEC:
// -----------------------------------------------------------------------------
//
// **(Parse-Closure-Type)** Lines 4718-4721
// IsOp(Tok(P), "|")
// Γ ⊢ ParseParamTypeList(Advance(P)) ⇓ (P_1, params)
// IsOp(Tok(P_1), "|")
// IsOp(Tok(Advance(P_1)), "->")
// Γ ⊢ ParseType(Advance(Advance(P_1))) ⇓ (P_2, ret)
// Γ ⊢ ParseClosureDepsOpt(P_2) ⇓ (P_3, deps_opt)
// ────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseNonPermType(P) ⇓ (P_3, TypeClosure(params, ret, deps_opt))
//
// **(Parse-Closure-Type-Empty)** Lines 4723-4726
// IsOp(Tok(P), "|")
// IsOp(Tok(Advance(P)), "|")
// IsOp(Tok(Advance(Advance(P))), "->")
// Γ ⊢ ParseType(Advance(Advance(Advance(P)))) ⇓ (P_1, ret)
// Γ ⊢ ParseClosureDepsOpt(P_1) ⇓ (P_2, deps_opt)
// ────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseNonPermType(P) ⇓ (P_2, TypeClosure([], ret, deps_opt))
//
// CLOSURE DEPENDENCY RULES (Lines 4728-4756):
// -----------------------------------------------------------------------------
//
// **(Parse-ClosureDepsOpt-None)** Lines 4728-4731
// ¬ IsPunc(Tok(P), "[")
// ────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseClosureDepsOpt(P) ⇓ (P, ⊥)
//
// **(Parse-ClosureDepsOpt-Some)** Lines 4733-4736
// IsPunc(Tok(P), "[")
// IsIdent(Tok(Advance(P)))
// Lexeme(Tok(Advance(P))) = `shared`
// IsPunc(Tok(Advance(Advance(P))), ":")
// IsPunc(Tok(Advance(Advance(Advance(P)))), "{")
// Γ ⊢ ParseSharedDepList(Advance(Advance(Advance(Advance(P))))) ⇓ (P_1, deps)
// IsPunc(Tok(P_1), "}")
// IsPunc(Tok(Advance(P_1)), "]")
// ────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseClosureDepsOpt(P) ⇓ (Advance(Advance(P_1)), ⟨deps⟩)
//
// **(Parse-SharedDepList-Empty)** Lines 4738-4741
// IsPunc(Tok(P), "}")
// ────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseSharedDepList(P) ⇓ (P, [])
//
// **(Parse-SharedDepList-Single)** Lines 4743-4746
// Γ ⊢ ParseSharedDep(P) ⇓ (P_1, dep)    ¬ IsPunc(Tok(P_1), ",")
// ────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseSharedDepList(P) ⇓ (P_1, [dep])
//
// **(Parse-SharedDepList-Cons)** Lines 4748-4751
// Γ ⊢ ParseSharedDep(P) ⇓ (P_1, dep)
// IsPunc(Tok(P_1), ",")
// Γ ⊢ ParseSharedDepList(Advance(P_1)) ⇓ (P_2, deps)
// ────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseSharedDepList(P) ⇓ (P_2, [dep] ++ deps)
//
// **(Parse-SharedDep)** Lines 4753-4756
// IsIdent(Tok(P))    name = Lexeme(Tok(P))
// IsPunc(Tok(Advance(P)), ":")
// Γ ⊢ ParseType(Advance(Advance(P))) ⇓ (P_1, ty)
// ────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseSharedDep(P) ⇓ (P_1, ⟨name, ty⟩)
//
// SEMANTICS:
// - Closure types use | instead of ( for parameters
// - Optional dependency clause: [shared: {name: Type, ...}]
// - Empty params: || -> R
// - With params: |i32, bool| -> string@View
// - With deps: |i32| -> () [shared: {data: DataStore}]
//
// =============================================================================
// SOURCE FILE: cursive-bootstrap/src/02_syntax/parser_types.cpp
// =============================================================================
//
// CONTENT TO MIGRATE:
// -----------------------------------------------------------------------------
//
// NOTE: Closure types are NOT implemented in the bootstrap parser (C0 subset).
// The spec defines them but they are marked as unsupported in C0.
// The following is the expected implementation based on the spec rules.
//
// 1. ParseClosureType function (NOT IN SOURCE - to be implemented)
//    ─────────────────────────────────────────────────────────────────────────
//    ParseElemResult<std::shared_ptr<Type>> ParseClosureType(Parser parser) {
//      // Check for || (empty params)
//      if (IsOp(parser, "|") && IsOp(Advance(parser), "|")) {
//        // Parse-Closure-Type-Empty
//        Parser after_pipes = Advance(Advance(parser));
//        if (!IsOp(after_pipes, "->")) {
//          // Error
//        }
//        Parser after_arrow = Advance(after_pipes);
//        auto ret = ParseType(after_arrow);
//        auto deps = ParseClosureDepsOpt(ret.parser);
//        // Construct TypeClosure with empty params
//      }
//      // Parse-Closure-Type with params
//      Parser after_pipe = Advance(parser);
//      auto params = ParseParamTypeList(after_pipe);
//      // ... rest of parsing
//    }
//
// 2. ParseClosureDepsOpt function (NOT IN SOURCE - to be implemented)
//    ─────────────────────────────────────────────────────────────────────────
//    // Parse optional [shared: {dep1: T1, dep2: T2}]
//
// 3. ParseSharedDepList function (NOT IN SOURCE - to be implemented)
//    ─────────────────────────────────────────────────────────────────────────
//    // Parse comma-separated list of name: Type pairs
//
// 4. ParseSharedDep function (NOT IN SOURCE - to be implemented)
//    ─────────────────────────────────────────────────────────────────────────
//    // Parse single name: Type pair
//
// DEPENDENCIES:
// =============================================================================
// - ParseType function (for param and return types)
// - ParseParamTypeList function (shared with function_type.cpp)
// - MakeTypeNode helper (type_common.cpp)
// - TypeClosure AST node type
// - SharedDep AST node type (name + type pair)
// - Token predicates: IsOp, IsPunc, IsIdent, Lexeme
// - Parser utilities: Tok, Advance, SpanBetween
//
// =============================================================================
// REFACTORING NOTES:
// =============================================================================
// - Closure types are NOT supported in C0 - emit E-UNS-0101 if encountered
// - The | character is used as an operator, not punctuator
// - Must distinguish || (empty closure params) from | (bitwise or)
// - Dependency syntax: [shared: {name: Type}] is verbose but explicit
// - Consider whether closure types should share param parsing with func types
// - When implementing, ensure proper error handling for malformed syntax
// =============================================================================
