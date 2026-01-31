/*
 * =============================================================================
 * MIGRATION MAPPING: contract_check.cpp
 * =============================================================================
 *
 * SPEC REFERENCE:
 *   - C0updated.md, Section 14 "Contracts" (line 23181)
 *   - C0updated.md, Section 14.4 "Contract Syntax" (line 23183)
 *   - C0updated.md, Section 14.5 "Preconditions and Postconditions" (line 23279)
 *   - C0updated.md, Section 14.6 "Contract Well-Formedness" (lines 23400-23500)
 *
 * SOURCE FILE:
 *   - cursive-bootstrap/src/03_analysis/contracts/contract_check.cpp (lines 1-263)
 *
 * FUNCTIONS TO MIGRATE:
 *   - CheckContractWellFormed(Contract* contract) -> bool          [lines 30-80]
 *       Validate contract syntax and semantics
 *   - CheckPrecondition(Precond* pre, ProcDecl* proc) -> bool      [lines 85-140]
 *       Validate precondition references valid parameters
 *   - CheckPostcondition(Postcond* post, ProcDecl* proc) -> bool   [lines 145-200]
 *       Validate postcondition with @result and @entry
 *   - ValidateContractExpression(Expr* expr) -> bool               [lines 205-250]
 *       Ensure contract expression is well-typed boolean
 *   - CheckContractScope(Contract* contract, ProcDecl* proc) -> bool [lines 253-263]
 *       Validate contract only references in-scope names
 *
 * DEPENDENCIES:
 *   - Contract, Precondition, Postcondition AST nodes
 *   - Expression type checking
 *   - Name resolution
 *
 * REFACTORING NOTES:
 *   1. Contract syntax: |= P (precond), |= P => Q (pre+post), |= => Q (postcond only)
 *   2. @result references return value (postcondition only)
 *   3. @entry(expr) captures entry/old value of expression
 *   4. @entry requires BitcopyType or CloneType
 *   5. Contracts must be boolean expressions
 *   6. Contract predicates must be PURE (see contract_purity.cpp)
 *
 * CONTRACT FORMS:
 *   |= precondition
 *   |= precondition => postcondition
 *   |= => postcondition
 *
 * INTRINSICS:
 *   - @result: Return value (postcondition context only)
 *   - @entry(expr): Captured entry value
 *
 * DIAGNOSTIC CODES:
 *   - E-CON-0001: Invalid contract syntax
 *   - E-CON-0002: @result outside postcondition
 *   - E-CON-0003: @entry with non-copyable type
 *   - E-CON-0004: Contract not boolean
 *   - E-CON-0005: Undefined name in contract
 *
 * =============================================================================
 */
