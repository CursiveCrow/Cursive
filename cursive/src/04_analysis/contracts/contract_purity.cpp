/*
 * =============================================================================
 * MIGRATION MAPPING: contract_purity.cpp
 * =============================================================================
 *
 * SPEC REFERENCE:
 *   - C0updated.md, Section 14.7 "Purity Constraints" (lines 23510-23600)
 *   - C0updated.md, Section 14.7.1 "Pure Expressions" (lines 23520-23560)
 *   - C0updated.md, Section 14.7.2 "Prohibited Operations" (lines 23570-23600)
 *
 * SOURCE FILE:
 *   - cursive-bootstrap/src/03_analysis/contracts/contract_check.cpp (lines 1-263)
 *     (purity checking integrated into contract checking)
 *
 * FUNCTIONS TO MIGRATE:
 *   - CheckContractPurity(Expr* expr) -> bool
 *       Verify contract expression is pure
 *   - IsCapabilityCall(Call* call) -> bool
 *       Check if call uses capability parameters
 *   - HasSideEffects(Expr* expr) -> bool
 *       Check for observable side effects
 *   - IsPureOperator(BinOp op, TypeRef lhs, TypeRef rhs) -> bool
 *       Check if binary operator is pure for given types
 *   - CheckMutationFree(Expr* expr) -> bool
 *       Verify no mutations in contract expression
 *
 * DEPENDENCIES:
 *   - Expression AST traversal
 *   - Capability detection
 *   - Side effect analysis
 *
 * REFACTORING NOTES:
 *   1. CRITICAL: Contract predicates MUST be pure
 *   2. MUST NOT invoke procedures accepting capability parameters
 *   3. MUST NOT mutate state observable outside expression evaluation
 *   4. Built-in operators on primitives are always pure
 *   5. Method calls must be to pure methods only
 *   6. No I/O, no allocation (unless provably temporary)
 *
 * PURE OPERATIONS:
 *   - Arithmetic: +, -, *, /, %
 *   - Comparison: ==, !=, <, <=, >, >=
 *   - Logical: &&, ||, !
 *   - Bitwise: &, |, ^, <<, >>
 *   - Field access on const receiver
 *   - Array indexing on const receiver
 *
 * IMPURE OPERATIONS (PROHIBITED):
 *   - Capability method calls (ctx.fs~>read_file, etc.)
 *   - Assignment and mutation
 *   - I/O operations
 *   - Memory allocation
 *   - Procedures with side effects
 *
 * DIAGNOSTIC CODES:
 *   - E-CON-0010: Impure contract predicate
 *   - E-CON-0011: Capability call in contract
 *   - E-CON-0012: Mutation in contract
 *   - E-CON-0013: I/O in contract
 *
 * =============================================================================
 */
