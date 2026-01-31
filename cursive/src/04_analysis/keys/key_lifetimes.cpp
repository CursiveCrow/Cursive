/*
 * =============================================================================
 * MIGRATION MAPPING: key_lifetimes.cpp
 * =============================================================================
 *
 * SPEC REFERENCE:
 *   - C0updated.md, Section 17.11 "Key Lifetimes" (lines 24710-24800)
 *   - C0updated.md, Section 17.12 "Key Release" (lines 24810-24900)
 *   - C0updated.md, Section 16.8 "Async-Key Integration" (lines 24150-24200)
 *
 * SOURCE FILE:
 *   - cursive-bootstrap/src/03_analysis/keys/key_context.cpp (lines 1-192)
 *     (lifetime handling integrated into context)
 *
 * FUNCTIONS TO MIGRATE:
 *   - ComputeKeyLifetime(KeyBlock* block) -> Lifetime
 *       Compute lifetime of keys in block
 *   - ValidateKeyRelease(KeyBlock* block) -> bool
 *       Validate keys are properly released
 *   - CheckKeyLiveness(KeyPath path, Stmt* stmt) -> bool
 *       Check if key is live at statement
 *   - TrackKeyScope(KeyBlock* block) -> Scope
 *       Track scope boundaries for key lifetime
 *   - ValidateYieldRelease(Yield* yield, KeyContext* ctx) -> bool
 *       Validate yield release modifier with held keys
 *   - PropagateKeyLifetime(Block* block) -> void
 *       Propagate key lifetime through control flow
 *
 * DEPENDENCIES:
 *   - Key context tracking
 *   - Control flow analysis
 *   - Yield expression handling
 *
 * REFACTORING NOTES:
 *   1. Keys are held for duration of # block
 *   2. Keys released when block exits (normal or abnormal)
 *   3. CRITICAL: Keys cannot be held across yield points
 *   4. yield release modifier releases keys before suspend
 *   5. After yield release, shared data may be stale
 *   6. Consider defer-style key release for early returns
 *
 * KEY LIFETIME RULES:
 *   - Acquired: At # block entry
 *   - Held: Within # block body
 *   - Released: At # block exit (including break/return)
 *
 * YIELD AND KEYS:
 *   // WRONG - key held across yield
 *   #shared_data {
 *       yield value        // ERROR: E-CON-0213
 *   }
 *
 *   // CORRECT - yield release
 *   #shared_data {
 *       yield release value  // OK: keys released before suspend
 *   }
 *
 * STALE DATA WARNING:
 *   After yield release, bindings derived from shared data may be stale.
 *   Re-acquire keys and re-read data after resume.
 *
 * DIAGNOSTIC CODES:
 *   - E-KEY-0040: Key lifetime exceeds scope
 *   - E-KEY-0041: Key not released on all paths
 *   - E-KEY-0042: Key access after release
 *   - W-KEY-0001: Data may be stale after yield release
 *
 * =============================================================================
 */
