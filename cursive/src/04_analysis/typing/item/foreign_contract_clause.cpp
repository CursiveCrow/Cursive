// =============================================================================
// MIGRATION MAPPING: item/foreign_contract_clause.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 18: Foreign Function Interface - Contracts
//   - @foreign_assumes syntax
//   - @foreign_ensures syntax
//   - Error and null result cases
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/types/contracts.cpp
//   Foreign contract clause typing
//
// KEY CONTENT TO MIGRATE:
//   FOREIGN CONTRACT PROCESSING:
//   1. Parse foreign contract form
//   2. Type predicates
//   3. Register for call-site checking
//
//   FOREIGN CONTRACT FORMS:
//   - |= @foreign_assumes(p1, p2, ...): preconditions
//   - |= @foreign_ensures(p): postcondition on success
//   - |= @foreign_ensures(@error: p): postcondition on error
//   - |= @foreign_ensures(@null_result: p): when result is null
//
//   @FOREIGN_ASSUMES:
//   - Predicates the foreign code expects
//   - Caller MUST ensure these hold before call
//   - Multiple predicates comma-separated
//
//   @FOREIGN_ENSURES:
//   - Predicates the foreign code guarantees
//   - Different cases for success/error/null
//   - @result available in success predicates
//
//   @ERROR CASE:
//   - For functions that indicate failure via return value
//   - Predicate describes error indicator
//   - Example: |= @foreign_ensures(@error: @result < 0)
//
//   @NULL_RESULT CASE:
//   - For functions that may return null
//   - Predicate describes when null is returned
//   - Example: |= @foreign_ensures(@null_result: @result == null)
//
//   VERIFICATION:
//   - [[static]]: caller proves assumes at compile time
//   - [[dynamic]]: runtime checks before call
//   - [[assume]]: assume true without checks
//
// DEPENDENCIES:
//   - ExprTypeFn for predicate typing
//   - ForeignContext for @result handling
//   - Verification mode handling
//
// SPEC RULES:
//   - Foreign contract syntax
//   - Verification mode semantics
//
// RESULT:
//   Typed foreign contract
//   Diagnostics for invalid predicates
//
// =============================================================================

