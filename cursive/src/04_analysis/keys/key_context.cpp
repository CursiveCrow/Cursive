/*
 * =============================================================================
 * MIGRATION MAPPING: key_context.cpp
 * =============================================================================
 *
 * SPEC REFERENCE:
 *   - C0updated.md, Section 17.1 "Key Context" (lines 23770-23850)
 *   - C0updated.md, Section 17.2 "Key Acquisition" (lines 23860-23900)
 *   - C0updated.md, Section 17.5 "Context Management" (lines 24110-24200)
 *
 * SOURCE FILE:
 *   - cursive-bootstrap/src/03_analysis/keys/key_context.cpp (lines 1-192)
 *
 * FUNCTIONS TO MIGRATE:
 *   - KeyContext::new() -> KeyContext                              [lines 20-35]
 *       Create empty key context
 *   - KeyContext::Acquire(KeyPath path, Mode mode) -> Result       [lines 40-80]
 *       Acquire a key for path with read/write mode
 *   - KeyContext::Release(KeyPath path) -> void                    [lines 85-110]
 *       Release a held key
 *   - KeyContext::Covers(KeyPath path, Mode mode) -> bool          [lines 115-145]
 *       Check if context covers access to path
 *   - KeyContext::HeldKeys() -> Vec<(KeyPath, Mode)>               [lines 150-165]
 *       Get all currently held keys
 *   - KeyContext::Clone() -> KeyContext                            [lines 170-185]
 *       Clone context for branching control flow
 *   - KeyContext::Merge(other: KeyContext) -> KeyContext           [lines 188-192]
 *       Merge contexts from branching paths
 *
 * DEPENDENCIES:
 *   - KeyPath for path representation
 *   - Mode enum (read, write)
 *   - Conflict detection
 *
 * REFACTORING NOTES:
 *   1. Key context tracks all currently held keys
 *   2. Acquisition checks for conflicts before granting
 *   3. Release must match prior acquisition
 *   4. Context cloning needed for if/match branches
 *   5. Merge takes intersection of held keys from branches
 *   6. Consider stack-based context for nested blocks
 *
 * KEY BLOCK SYNTAX:
 *   #path { ... }           // write mode (default)
 *   #path read { ... }      // read mode
 *   #path write { ... }     // explicit write mode
 *   #p1, p2 { ... }         // multiple paths
 *
 * KEY MODIFIERS:
 *   - dynamic: Runtime key acquisition
 *   - speculative: May roll back
 *   - release: Release keys after block
 *
 * DIAGNOSTIC CODES:
 *   - E-KEY-0010: Key already held
 *   - E-KEY-0011: Key not held for release
 *   - E-KEY-0012: Insufficient key mode
 *
 * =============================================================================
 */
