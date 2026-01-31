/*
 * =============================================================================
 * MIGRATION MAPPING: key_conflict.cpp
 * =============================================================================
 *
 * SPEC REFERENCE:
 *   - C0updated.md, Section 17 "Key System" (line 23759)
 *   - C0updated.md, Section 17.3 "Key Conflicts" (lines 23900-24000)
 *   - C0updated.md, Section 17.4 "Conflict Detection" (lines 24010-24100)
 *   - C0updated.md, Section 8.13 "E-KEY Errors" (lines 22200-22300)
 *
 * SOURCE FILE:
 *   - cursive-bootstrap/src/03_analysis/keys/key_conflict.cpp (lines 1-158)
 *
 * FUNCTIONS TO MIGRATE:
 *   - KeysConflict(KeyPath a, KeyPath b) -> bool                   [lines 25-70]
 *       Determine if two key paths conflict
 *   - CheckAcquisitionConflict(KeyAcq* acq, KeyContext* ctx) -> bool [lines 75-120]
 *       Check if acquisition conflicts with held keys
 *   - PathOverlaps(KeyPath a, KeyPath b) -> bool                   [lines 125-145]
 *       Check if paths access overlapping memory
 *   - ConflictMode(KeyPath a, KeyPath b) -> ConflictKind           [lines 148-158]
 *       Determine kind of conflict (read-write, write-write)
 *
 * DEPENDENCIES:
 *   - KeyPath for path representation
 *   - KeyContext for held keys tracking
 *   - KeyAcquisition AST nodes
 *
 * REFACTORING NOTES:
 *   1. Read-read access does not conflict
 *   2. Read-write or write-write on overlapping paths conflicts
 *   3. Path prefix relationship means overlap
 *   4. Key boundaries (#field) stop conflict propagation
 *   5. Different roots never conflict
 *   6. Consider caching conflict checks for repeated paths
 *
 * CONFLICT RULES:
 *   | Held Key | New Key | Conflict? |
 *   |----------|---------|-----------|
 *   | read     | read    | No        |
 *   | read     | write   | Yes       |
 *   | write    | read    | Yes       |
 *   | write    | write   | Yes       |
 *
 * PATH OVERLAP:
 *   - a.b overlaps a.b.c (prefix)
 *   - a.b overlaps a.b (same)
 *   - a.b does not overlap a.c (different field)
 *   - #boundary stops overlap propagation
 *
 * DIAGNOSTIC CODES:
 *   - E-KEY-0001: Key conflict detected
 *   - E-KEY-0002: Write conflict with held read key
 *   - E-KEY-0003: Read conflict with held write key
 *
 * =============================================================================
 */
