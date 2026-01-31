// =============================================================================
// MIGRATION MAPPING: expr/contract_entry.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 5.8: Contracts
//   - @entry(expr) intrinsic in postconditions
//   - Captures entry/old value for comparison
//   - Used in contract predicates (right side of =>)
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/types/contracts.cpp
//   Contract entry intrinsic typing
//
// KEY CONTENT TO MIGRATE:
//   @ENTRY INTRINSIC (@entry(expr)):
//   1. Valid only in postcondition context (right of =>)
//   2. Type the inner expression at entry state
//   3. Result type must satisfy BitcopyType or CloneType
//   4. Value captured at procedure entry
//   5. Available for comparison in postcondition
//
//   TYPING:
//   InPostcondition = true
//   Gamma_entry |- expr : T
//   BitcopyType(T) or CloneType(T)
//   --------------------------------------------------
//   Gamma |- @entry(expr) : T
//
//   CONTEXT REQUIREMENTS:
//   - ONLY valid in postcondition context
//   - expr evaluated in entry environment (parameters at call time)
//   - Cannot reference @result
//   - Cannot reference post-state bindings
//
//   TYPE CONSTRAINTS:
//   - Result type must be BitcopyType or CloneType
//   - This ensures value can be preserved across procedure body
//   - Prevents capturing unique/move-only types
//
//   USE CASE:
//   procedure increment(~!) -> i32
//       |= self.value >= 0 => @result > @entry(self.value)
//   {
//       self.value = self.value + 1
//       return self.value
//   }
//
// DEPENDENCIES:
//   - PostconditionContext verification
//   - EntryEnvironment for typing expr
//   - BitcopyType/CloneType predicates
//   - ExprTypeFn for inner expression
//
// SPEC RULES:
//   - @entry intrinsic semantics
//   - BitcopyType/CloneType requirements
//
// RESULT TYPE:
//   Type of inner expression (must be Bitcopy or Clone)
//
// NOTES:
//   - @entry captures snapshot at procedure entry
//   - Enables "old value" comparisons in postconditions
//   - Cannot be used in preconditions
//
// =============================================================================

