// =============================================================================
// MIGRATION MAPPING: stmt/shadow_let_stmt.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 5.2.10: Shadow Binding Statements (lines 9409-9427)
//   - T-ShadowLetStmt-Ann (line 9409): With type annotation
//   - T-ShadowLetStmt-Ann-Mismatch (line 9414): Annotation mismatch
//   - T-ShadowLetStmt-Infer (line 9419): Type inference
//   - T-ShadowLetStmt-Infer-Err (line 9424): Inference failure
//   - shadow_binding grammar (line 3153)
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/types/type_infer.cpp
//   Shadow let statement typing portions
//
// KEY CONTENT TO MIGRATE:
//   SHADOW LET STATEMENT (shadow let name: T = init):
//   1. Type initializer expression
//   2. If type annotation provided:
//      a. Lower the annotation type
//      b. Check init type is subtype of annotation
//      c. Use annotation as binding type
//   3. If no annotation:
//      a. Infer type from initializer
//      b. Error if inference fails
//   4. Shadow existing binding with same name
//   5. New binding is immutable
//
//   TYPING RULE (T-ShadowLetStmt-Ann):
//   ty_opt = T_a
//   Gamma; R; L |- init => T_i
//   Gamma |- T_i <: T_a
//   --------------------------------------------------
//   Gamma |- shadow let name: T_a = init
//   Gamma' = Gamma[name := T_a]
//
//   TYPING RULE (T-ShadowLetStmt-Infer):
//   ty_opt = bottom
//   Gamma; R; L |- init => T_i
//   Solve(C) succeeds
//   --------------------------------------------------
//   Gamma |- shadow let name = init
//   Gamma' = Gamma[name := T_i]
//
//   SHADOWING SEMANTICS:
//   - Previous binding with same name becomes inaccessible
//   - New binding has fresh type (may differ from old)
//   - Used for progressive refinement
//
//   EXAMPLE:
//   let x: i32 = 10
//   shadow let x: i32 = x + 1  // x is now 11
//
// DEPENDENCIES:
//   - ExprTypeFn for initializer typing
//   - LowerType for annotation processing
//   - Subtyping for T_i <: T_a check
//   - Environment update (shadowing)
//   - Solve for constraint solving
//
// SPEC RULES:
//   - T-ShadowLetStmt-Ann (line 9409)
//   - T-ShadowLetStmt-Ann-Mismatch (E-MOD-2402)
//   - T-ShadowLetStmt-Infer (line 9419)
//   - T-ShadowLetStmt-Infer-Err (E-TYP-1530)
//
// RESULT:
//   Updated environment with shadowed binding
//   Diagnostics for mismatches or inference failures
//
// =============================================================================

