// =============================================================================
// MIGRATION MAPPING: item/static_decl.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 5.3.7: Static Declarations
//   - WF-StaticDecl-Ann-Mismatch (line 21477): Type mismatch
//   - Static let/var declarations
//   - Module-level bindings
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/types/type_decls.cpp
//   Static declaration typing
//
// KEY CONTENT TO MIGRATE:
//   STATIC DECLARATION:
//   1. Lower type annotation
//   2. Type initializer expression
//   3. Check initializer type matches annotation
//   4. Verify initializer is const-evaluable (for let)
//   5. Register static in environment
//
//   STATIC LET:
//   Gamma |- type wf
//   Gamma |- init : T_i
//   Gamma |- T_i <: annotation
//   IsConstExpr(init)
//   --------------------------------------------------
//   Gamma |- public let NAME: annotation = init
//
//   STATIC VAR:
//   Gamma |- type wf
//   Gamma |- init : T_i
//   Gamma |- T_i <: annotation
//   --------------------------------------------------
//   Gamma |- public var NAME: annotation = init
//
//   STATIC LET VS VAR:
//   - let: immutable, must be const-evaluable
//   - var: mutable, runtime initialization allowed
//
//   CONST EVALUATION:
//   - Literals: allowed
//   - Arithmetic on literals: allowed
//   - Function calls: generally not allowed
//   - Complex expressions: depends on context
//
//   VISIBILITY:
//   - public: visible everywhere
//   - internal: visible in assembly (default)
//   - private: visible in module only
//
// DEPENDENCIES:
//   - LowerType() for annotation
//   - ExprTypeFn for initializer
//   - Subtyping for type check
//   - ConstEval() for let statics
//   - VisibilityCheck()
//
// SPEC RULES:
//   - WF-StaticDecl-Ann-Mismatch (E-MOD-2402)
//   - Static declaration semantics
//
// RESULT:
//   Static declaration added to environment
//   Diagnostics for any errors
//
// =============================================================================

