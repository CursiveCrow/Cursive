// =============================================================================
// MIGRATION MAPPING: enums.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
// - Section 5.10 "Enum Discriminant Defaults" (not shown in excerpts, referenced in source)
//   - DiscOf function
//   - DiscSeq function
//   - EnumDiscriminants function
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/composite/enums.cpp
// - Lines 1-181 (entire file)
//
// Key source functions to migrate:
// - EnumDiscriminants (lines 113-178): Compute discriminant values for enum variants
//
// Supporting helpers:
// - ParseIntCore (lines 23-78): Parse integer literal core (128-bit)
// - StripIntSuffix (lines 80-93): Strip integer suffix from lexeme
// - ParseUnsignedIntLiteral (lines 95-109): Parse unsigned integer literal
//
// DEPENDENCIES:
// - cursive/src/00_core/int128.h (UInt128 operations)
// - cursive/src/02_syntax/ast.h (EnumDecl, EnumVariant, Token)
// - cursive/src/00_core/assert_spec.h (SPEC_DEF, SPEC_RULE)
//
// Diagnostic rules implemented:
// - Enum-Disc-NotInt (line 129): Discriminant must be an integer literal
// - Enum-Disc-Negative (line 136): Discriminant must not be negative
// - Enum-Disc-Invalid (lines 143, 149, 157, 160): Invalid discriminant value
// - Enum-Disc-Dup (line 166): Duplicate discriminant value
//
// REFACTORING NOTES:
// 1. Integer parsing logic is duplicated with tuples.cpp - consolidate
// 2. Discriminant values are u64, computed incrementally from 0
// 3. Explicit discriminants set the next implicit value
// 4. Duplicate discriminant values are an error
// 5. Overflow at max_u64 is an error if not the last variant
// 6. The result includes both the discriminant list and max discriminant
// =============================================================================

