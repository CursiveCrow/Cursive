// =============================================================================
// MIGRATION MAPPING: item/contract_clause.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 5.8: Contracts
//   - Contract syntax: |= P, |= P => Q, |= => Q
//   - Precondition context
//   - Postcondition context
//   - @result, @entry intrinsics
//   - Purity constraints
//   - Verification modes (static, [[dynamic]])
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/types/contracts.cpp
//   Contract clause typing
//
// KEY CONTENT TO MIGRATE:
//   CONTRACT CLAUSE PROCESSING:
//   1. Parse contract form
//   2. Type precondition (if present)
//   3. Type postcondition (if present)
//   4. Verify purity constraints
//   5. Check verification mode
//
//   CONTRACT FORMS:
//   - |= P: precondition only
//   - |= P => Q: precondition and postcondition
//   - |= => Q: postcondition only (pre is true)
//
//   PRECONDITION TYPING:
//   - Context: parameters at entry state
//   - Must be pure expression
//   - Must have type bool
//   - Cannot use @result
//
//   POSTCONDITION TYPING:
//   - Context: parameters at post-state, @result, @entry
//   - Must be pure expression
//   - Must have type bool
//   - @result references return value
//   - @entry(e) references entry value
//
//   PURITY CONSTRAINTS:
//   - MUST NOT invoke capability-bearing procedures
//   - MUST NOT mutate observable state
//   - Built-in operators always pure
//
//   VERIFICATION MODES:
//   - Default: static verification required
//   - [[dynamic]]: runtime checks allowed
//   - [[assume]]: assume true (optimization)
//
//   @ENTRY REQUIREMENTS:
//   - Expression type must be BitcopyType or CloneType
//   - Ensures value can be captured at entry
//
// DEPENDENCIES:
//   - ExprTypeFn for predicate typing
//   - PurityCheck for constraint verification
//   - PreContext for precondition environment
//   - PostContext for postcondition environment
//   - @result, @entry handling
//
// SPEC RULES:
//   - WF-Contract
//   - Purity constraints
//   - @entry type requirements
//
// RESULT:
//   Typed contract clause
//   Diagnostics for purity/type violations
//
// =============================================================================

