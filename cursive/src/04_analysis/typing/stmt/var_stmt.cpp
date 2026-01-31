// =============================================================================
// MIGRATION MAPPING: stmt/var_stmt.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 5.2.11: Statement Typing (lines 9389-9407)
//   - T-VarStmt-Ann (lines 9389-9392): Var with type annotation
//   - T-VarStmt-Ann-Mismatch (lines 9394-9397): Type mismatch error
//   - T-VarStmt-Infer (lines 9399-9402): Var with inference
//   - T-VarStmt-Infer-Err (lines 9404-9407): Inference failure
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/types/type_stmt.cpp
//   Var statement typing
//
// KEY CONTENT TO MIGRATE:
//   VAR WITH ANNOTATION (var pat: T = init):
//   1. Lower type annotation to TypeRef
//   2. Check init expression against annotated type
//   3. Match pattern against annotated type
//   4. Check pattern names are distinct
//   5. Introduce bindings with 'var' mutability
//
//   VAR WITH INFERENCE (var pat = init):
//   1. Infer type of init expression
//   2. Solve any constraints
//   3. Match pattern against inferred type
//   4. Check pattern names are distinct
//   5. Introduce bindings with 'var' mutability
//
//   PATTERN BINDING:
//   - IntroAllVar introduces bindings as mutable (var)
//   - Each pattern variable gets (name, type) entry with var flag
//   - Mutable bindings can be assigned later
//
//   BINDING OPERATOR:
//   - var x = v: mutable, movable
//   - var x := v: mutable, immovable
//   - Operator determines moveability
//
// DEPENDENCIES:
//   - Same as let_stmt.cpp plus:
//   - IntroAllVar() for mutable binding introduction
//
// SPEC RULES IMPLEMENTED:
//   - T-VarStmt-Ann: Annotated var
//   - T-VarStmt-Ann-Mismatch: Type error
//   - T-VarStmt-Infer: Inferred var
//   - T-VarStmt-Infer-Err: Inference error
//
// RESULT TYPE:
//   StmtResult { Gamma', Res=[], Brk=[], BrkVoid=false }
//
// =============================================================================

