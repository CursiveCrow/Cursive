/*
 * =============================================================================
 * MIGRATION MAPPING: verification.cpp
 * =============================================================================
 *
 * SPEC REFERENCE:
 *   - C0updated.md, Section 14.8 "Static Verification" (lines 23610-23750)
 *   - C0updated.md, Section 14.8.1 "Proof Obligations" (lines 23620-23680)
 *   - C0updated.md, Section 14.8.2 "Verification Modes" (lines 23690-23750)
 *   - C0updated.md, Section 8.11 "E-VER Errors" (lines 22000-22100)
 *
 * SOURCE FILE:
 *   - cursive-bootstrap/src/03_analysis/contracts/verification.cpp (lines 1-1109)
 *
 * FUNCTIONS TO MIGRATE:
 *   - StaticProof(Contract* contract, Context* ctx) -> ProofResult [lines 50-150]
 *       Attempt to statically prove contract holds
 *   - EntTrue(Expr* expr, Context* ctx) -> bool                    [lines 155-250]
 *       Entailment: context entails expression is true
 *   - EntFact(Fact* fact, Context* ctx) -> bool                    [lines 255-350]
 *       Add fact to verification context
 *   - EntAnd(Expr* lhs, Expr* rhs, Context* ctx) -> bool           [lines 355-450]
 *       Entailment for conjunction
 *   - EntOr(Expr* lhs, Expr* rhs, Context* ctx) -> bool            [lines 455-550]
 *       Entailment for disjunction
 *   - EntLinear(Expr* linear, Context* ctx) -> bool                [lines 555-700]
 *       Linear arithmetic entailment
 *   - BuildVerificationContext(Proc* proc) -> Context              [lines 705-850]
 *       Build initial context from preconditions
 *   - PropagateAssumptions(Stmt* stmt, Context* ctx) -> Context    [lines 855-1000]
 *       Propagate assumptions through control flow
 *   - CheckPostconditionProof(Proc* proc, Context* ctx) -> bool    [lines 1005-1109]
 *       Verify postcondition holds at all return points
 *
 * DEPENDENCIES:
 *   - Contract AST nodes
 *   - Verification context
 *   - Linear arithmetic solver
 *   - Control flow graph
 *
 * REFACTORING NOTES:
 *   1. STATIC VERIFICATION IS DEFAULT - contracts must be provable
 *   2. If contract cannot be statically proven, program is ILL-FORMED
 *   3. [[dynamic]] attribute enables runtime verification instead
 *   4. Verification context accumulates known facts
 *   5. Control flow (if/match) creates branching contexts
 *   6. Linear arithmetic (comparisons, bounds) has decision procedure
 *   7. Consider SMT solver integration for complex contracts
 *   8. This is a large file - consider splitting:
 *      - entailment.cpp: Core entailment logic
 *      - linear.cpp: Linear arithmetic
 *      - context.cpp: Verification context management
 *      - proof.cpp: Proof search and generation
 *
 * VERIFICATION MODES:
 *   - [[static]] (default): Must prove at compile time
 *   - [[dynamic]]: Generate runtime checks
 *   - [[assume]]: Assume true without proof (dangerous)
 *
 * DIAGNOSTIC CODES:
 *   - E-VER-0001: Contract cannot be statically verified
 *   - E-VER-0002: Postcondition not established
 *   - E-VER-0003: Precondition may not hold at call site
 *   - W-VER-0001: Using [[dynamic]] verification
 *
 * =============================================================================
 */
