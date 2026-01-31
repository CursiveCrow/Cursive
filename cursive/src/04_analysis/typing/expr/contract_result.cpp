// =============================================================================
// MIGRATION MAPPING: expr/contract_result.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 5.8: Contracts
//   - @result intrinsic in postconditions
//   - References the return value
//   - Used in contract predicates (right side of =>)
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/types/contracts.cpp
//   Contract result intrinsic typing
//
// KEY CONTENT TO MIGRATE:
//   @RESULT INTRINSIC (@result):
//   1. Valid only in postcondition context (right of =>)
//   2. Type is the procedure's return type
//   3. References the value about to be returned
//
//   TYPING:
//   InPostcondition = true
//   ReturnType(procedure) = T
//   --------------------------------------------------
//   Gamma |- @result : T
//
//   CONTEXT REQUIREMENTS:
//   - ONLY valid in postcondition context (right of =>)
//   - Cannot be used in preconditions (left of => or alone)
//   - References the actual return value
//
//   USE CASE:
//   procedure abs(x: i32) -> i32
//       |= => @result >= 0
//   {
//       return if x < 0 { -x } else { x }
//   }
//
//   WITH PRECONDITION:
//   procedure divide(a: i32, b: i32) -> i32
//       |= b != 0 => @result * b == a
//   {
//       return a / b
//   }
//
//   EVALUATION:
//   - @result evaluated after procedure body completes
//   - Before actual return to caller
//   - Represents the computed return value
//
// DEPENDENCIES:
//   - PostconditionContext verification
//   - ReturnType extraction from procedure signature
//   - Contract clause parsing
//
// SPEC RULES:
//   - @result intrinsic semantics
//   - Postcondition context requirement
//
// RESULT TYPE:
//   Procedure's return type
//
// NOTES:
//   - @result is the primary postcondition intrinsic
//   - Enables specification of output properties
//   - Cannot be assigned, only referenced
//
// =============================================================================

