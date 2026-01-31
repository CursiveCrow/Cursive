// =============================================================================
// MIGRATION MAPPING: type_infer.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 5.2.12: Expression Typing (lines 9778-9798)
//   Section 9.4: Type Inference (referenced at line 22083, 22440)
//   - ExprJudg = {Gamma; R; L |- e : T, Gamma; R; L |- e <= T, ...}
//   - Lift-Expr (lines 9789-9792): Lifting simple judgments
//   - Bidirectional type inference: synthesis (=>) and checking (<=)
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/types/type_infer.cpp
//   Lines 1-729: Full implementation
//   Key functions:
//   - InferExpr(Env, Expr) -> Type (synthesis mode)
//   - CheckExpr(Env, Expr, Type) -> bool (checking mode)
//   - UnifyTypes(Env, Type, Type) -> Type (constraint solving)
//   - Solve(Constraints) -> Substitution
//
// KEY CONTENT TO MIGRATE:
//   BIDIRECTIONAL TYPE INFERENCE:
//   Synthesis (=>): Derive type from expression
//   Checking (<=): Verify expression against expected type
//
//   SYNTHESIS MODE (InferExpr):
//   - Literals: known types (integers, floats, bools, chars, strings)
//   - Identifiers: lookup in environment
//   - Field access: receiver type determines field type
//   - Method calls: receiver + method signature
//   - Function calls: callee type determines result
//   - Binary/unary ops: operand types determine result
//   - If/match: branch type unification
//   - Block: final expression or unit
//
//   CHECKING MODE (CheckExpr):
//   - Verify synthesized type is subtype of expected
//   - Propagate expected type for better inference
//   - Handle type annotation contexts
//
//   CONSTRAINT GENERATION:
//   - Type variables for unknowns
//   - Equality constraints from unification
//   - Subtyping constraints from coercion points
//
//   CONSTRAINT SOLVING (Solve):
//   - Unification algorithm
//   - Substitution application
//   - Error on unsatisfiable constraints
//
// DEPENDENCIES:
//   - TypeEquiv() for type comparison
//   - Subtyping() for subtype checks
//   - LowerType() for type annotation processing
//   - Environment for binding lookups
//   - Constraint representation
//
// SPEC RULES:
//   - Lift-Expr: Context lifting for simple judgments
//   - All T-* expression rules use inference
//   - T-LetStmt-Infer-Err, T-VarStmt-Infer-Err for failures
//
// RESULT:
//   Type (for synthesis) or bool (for checking)
//   Diagnostics for inference failures
//
// NOTES:
//   - Inference flows bidirectionally
//   - Some expressions require checking mode (e.g., Ptr::null())
//   - Type variables resolved by Solve()
//
// =============================================================================

