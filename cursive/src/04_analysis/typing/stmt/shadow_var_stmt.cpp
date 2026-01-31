// =============================================================================
// MIGRATION MAPPING: stmt/shadow_var_stmt.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 5.2.10: Shadow Binding Statements (lines 9429-9447)
//   - T-ShadowVarStmt-Ann (line 9429): With type annotation
//   - T-ShadowVarStmt-Ann-Mismatch (line 9434): Annotation mismatch
//   - T-ShadowVarStmt-Infer (line 9439): Type inference
//   - T-ShadowVarStmt-Infer-Err (line 9444): Inference failure
//   - shadow_binding grammar (line 3153)
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/types/type_infer.cpp
//   Shadow var statement typing portions
//
// KEY CONTENT TO MIGRATE:
//   SHADOW VAR STATEMENT (shadow var name: T = init):
//   1. Type initializer expression
//   2. If type annotation provided:
//      a. Lower the annotation type
//      b. Check init type is subtype of annotation
//      c. Use annotation as binding type
//   3. If no annotation:
//      a. Infer type from initializer
//      b. Error if inference fails
//   4. Shadow existing binding with same name
//   5. New binding is MUTABLE
//
//   TYPING RULE (T-ShadowVarStmt-Ann):
//   ty_opt = T_a
//   Gamma; R; L |- init => T_i
//   Gamma |- T_i <: T_a
//   --------------------------------------------------
//   Gamma |- shadow var name: T_a = init
//   Gamma' = Gamma[name := unique T_a]
//
//   TYPING RULE (T-ShadowVarStmt-Infer):
//   ty_opt = bottom
//   Gamma; R; L |- init => T_i
//   Solve(C) succeeds
//   --------------------------------------------------
//   Gamma |- shadow var name = init
//   Gamma' = Gamma[name := unique T_i]
//
//   DIFFERENCE FROM SHADOW LET:
//   - Binding is mutable (can be assigned)
//   - Type includes unique permission
//   - Value can be modified after binding
//
//   EXAMPLE:
//   var x: i32 = 10
//   shadow var x: i32 = x * 2  // x is now 20, still mutable
//   x = x + 1                   // x is now 21
//
// DEPENDENCIES:
//   - ExprTypeFn for initializer typing
//   - LowerType for annotation processing
//   - Subtyping for T_i <: T_a check
//   - Environment update (shadowing with mutability)
//   - Solve for constraint solving
//
// SPEC RULES:
//   - T-ShadowVarStmt-Ann (line 9429)
//   - T-ShadowVarStmt-Ann-Mismatch (E-MOD-2402)
//   - T-ShadowVarStmt-Infer (line 9439)
//   - T-ShadowVarStmt-Infer-Err (E-TYP-1530)
//
// RESULT:
//   Updated environment with shadowed mutable binding
//   Diagnostics for mismatches or inference failures
//
// =============================================================================

