// =============================================================================
// MIGRATION MAPPING: stmt/let_stmt.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 5.2.11: Statement Typing (lines 9365-9387)
//   - T-LetStmt-Ann (lines 9369-9372): Let with type annotation
//   - T-LetStmt-Ann-Mismatch (lines 9374-9377): Type mismatch error
//   - T-LetStmt-Infer (lines 9379-9382): Let with inference
//   - T-LetStmt-Infer-Err (lines 9384-9387): Inference failure
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/types/type_stmt.cpp
//   Let statement typing
//
// KEY CONTENT TO MIGRATE:
//   LET WITH ANNOTATION (let pat: T = init):
//   1. Lower type annotation to TypeRef
//   2. Check init expression against annotated type
//   3. Match pattern against annotated type
//   4. Check pattern names are distinct
//   5. Introduce bindings with 'let' mutability
//
//   LET WITH INFERENCE (let pat = init):
//   1. Infer type of init expression
//   2. Solve any constraints
//   3. Match pattern against inferred type
//   4. Check pattern names are distinct
//   5. Introduce bindings with 'let' mutability
//
//   PATTERN BINDING:
//   - IntroAll introduces bindings as immutable (let)
//   - Each pattern variable gets (name, type) entry
//   - Distinct check prevents duplicate bindings
//
//   BINDING OPERATOR:
//   - let x = v: immutable, movable
//   - let x := v: immutable, immovable
//   - Operator determines moveability, not mutability
//
// DEPENDENCIES:
//   - ExprTypeFn for init typing
//   - CheckExpr() for annotation checking
//   - InferExpr() for inference
//   - PatternType() for pattern matching
//   - PatNames() for name extraction
//   - Distinct() for uniqueness check
//   - IntroAll() for binding introduction
//   - LowerType() for annotation
//
// SPEC RULES IMPLEMENTED:
//   - T-LetStmt-Ann: Annotated let
//   - T-LetStmt-Ann-Mismatch: Type error
//   - T-LetStmt-Infer: Inferred let
//   - T-LetStmt-Infer-Err: Inference error
//
// RESULT TYPE:
//   StmtResult { Gamma', Res=[], Brk=[], BrkVoid=false }
//
// =============================================================================

