// =============================================================================
// MIGRATION MAPPING: expr/propagate_expr.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 5.2: Static Semantics - Union Propagation
//   - ? operator for error/union propagation
//   - Early return semantics
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/types/type_infer.cpp
//   Propagation expression typing
//
// KEY CONTENT TO MIGRATE:
//   PROPAGATION EXPRESSION (expr?):
//   1. Type the inner expression
//   2. Check it's a union type
//   3. Identify "success" and "failure" variants
//   4. If failure: early return with failure
//   5. If success: unwrap to success type
//
//   TYPING:
//   Gamma |- expr : Success | Failure
//   Return type includes Failure
//   --------------------------------------------------
//   Gamma |- expr? : Success
//   (early returns Failure if not success)
//
//   UNION PROPAGATION SEMANTICS:
//   - Evaluates expr
//   - If failure variant: returns from function
//   - If success variant: unwraps to success type
//
//   RETURN TYPE COMPATIBILITY:
//   - Enclosing function return type must include Failure
//   - Or Failure must be subtype of return error type
//
//   EXAMPLE:
//   procedure read_config() -> Config | IOError {
//       let file: File = open("config.txt")?  // propagates IOError
//       let content: string@Managed = file~>read_all()?
//       return parse(content)
//   }
//
//   CHAINED PROPAGATION:
//   a()?.method()?.field?
//   - Each ? can trigger early return
//   - Chains unwrapped values
//
// DEPENDENCIES:
//   - ExprTypeFn for inner expression
//   - UnionType analysis for success/failure
//   - ReturnType compatibility check
//
// SPEC RULES:
//   - Union propagation semantics
//   - Return type inclusion
//
// RESULT TYPE:
//   Success type (failure propagated to caller)
//
// =============================================================================

