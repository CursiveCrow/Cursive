/*
 * =============================================================================
 * MIGRATION MAPPING: key_capture.cpp
 * =============================================================================
 *
 * SPEC REFERENCE:
 *   - C0updated.md, Section 17.6 "Key Capture in Parallel" (lines 24210-24300)
 *   - C0updated.md, Section 17.7 "Dispatch Key Clause" (lines 24310-24400)
 *   - C0updated.md, Section 16.5 "Parallel Capture" (lines 24050-24150)
 *
 * SOURCE FILE:
 *   - No direct source file; content distributed across key files
 *
 * FUNCTIONS TO MIGRATE:
 *   - AnalyzeKeyCapture(SpawnBlock* block) -> CapturedKeys
 *       Analyze which keys are captured by spawn block
 *   - ValidateKeyCapture(Spawn* spawn, KeyContext* ctx) -> bool
 *       Validate captured keys don't conflict
 *   - ComputeDispatchKeys(Dispatch* disp) -> Vec<KeyPattern>
 *       Compute key patterns for dispatch iterations
 *   - ValidateDispatchKeyClause(Dispatch* disp) -> bool
 *       Validate dispatch key clause syntax and semantics
 *   - CheckKeyAcrossYield(Async* async, KeyContext* ctx) -> bool
 *       Ensure no keys held across yield points
 *
 * DEPENDENCIES:
 *   - Spawn, Dispatch AST nodes
 *   - Key context tracking
 *   - Parallel block analysis
 *
 * REFACTORING NOTES:
 *   1. Spawn blocks capture keys from surrounding context
 *   2. Captured keys must not conflict between spawn blocks
 *   3. Dispatch key clause specifies per-iteration key acquisition
 *   4. CRITICAL: Keys MUST NOT be held across yield points
 *   5. yield release explicitly releases keys before suspend
 *   6. Consider key refinement for dispatch index expressions
 *
 * CAPTURE RULES:
 *   - const data: Captured by reference, no key needed
 *   - shared data: Captured by reference, key needed
 *   - unique data: MUST use explicit move
 *
 * DISPATCH KEY CLAUSE:
 *   dispatch i in 0..100 key data[i] write {
 *       data[i] = compute(i)
 *   }
 *   - key data[i]: Per-iteration key for data[i]
 *   - Different iterations get different keys (parallel OK)
 *   - Same index expressions serialize
 *
 * DIAGNOSTIC CODES:
 *   - E-KEY-0020: Key held across yield
 *   - E-KEY-0021: Conflicting key capture in spawn
 *   - E-KEY-0022: Invalid dispatch key pattern
 *   - E-CON-0213: Key held across yield (from spec)
 *
 * =============================================================================
 */
