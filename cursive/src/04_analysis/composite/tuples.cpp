// =============================================================================
// MIGRATION MAPPING: tuples.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
// - Section 5.2.5 "Tuples" (lines 8874-8920)
//   - T-Unit-Literal (lines 8876-8878)
//   - T-Tuple-Literal (lines 8880-8883)
//   - T-Tuple-Index (lines 8885-8888)
//   - T-Tuple-Index-Perm (lines 8890-8893)
//   - P-Tuple-Index (lines 8895-8898)
//   - P-Tuple-Index-Perm (lines 8900-8903)
//   - ConstTupleIndex (line 8905)
//   - TupleIndex-NonConst (lines 8907-8910)
//   - TupleIndex-OOB (lines 8912-8915)
//   - TupleAccess-NotTuple (lines 8917-8920)
// - Section 5.11 "Foundational Predicates" for BitcopyType
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/composite/tuples.cpp
// - Lines 1-421 (entire file)
//
// Key source functions to migrate:
// - TypeTupleExpr (lines 278-304): Tuple literal typing
// - TypeTupleAccessValue (lines 306-366): Tuple element access (value context)
// - TypeTupleAccessPlace (lines 368-418): Tuple element access (place context)
//
// Supporting helpers:
// - kIntSuffixes (lines 26-28): Integer literal suffixes
// - EndsWith (lines 30-35): String suffix check
// - StripIntSuffix (lines 37-48): Strip integer suffix from lexeme
// - DigitValue (lines 50-79): Parse digit value for base
// - ParseIntCore (lines 81-128): Parse integer literal core (128-bit)
// - ParseTupleIndex (lines 130-141): Parse tuple index from token
// - StripPerm (lines 143-151): Strip permission layer
// - kIntTypes/kFloatTypes (lines 153-159): Primitive type name lists
// - IsPrimTypeName (lines 179-182): Check primitive type names
// - BuiltinBitcopyType (lines 184-209): Check built-in bitcopy types
// - IsBitcopyClassPath (lines 211-213): Check Bitcopy class path
// - ImplementsBitcopy (lines 215-250): Check if type implements Bitcopy
// - BitcopyType (lines 252-274): Full bitcopy type predicate
//
// DEPENDENCIES:
// - cursive/src/04_analysis/resolve/scopes.h (ScopeContext)
// - cursive/src/00_core/int128.h (UInt128 operations)
// - cursive/src/00_core/assert_spec.h (SPEC_DEF, SPEC_RULE)
//
// REFACTORING NOTES:
// 1. Integer parsing logic is duplicated with enums.cpp - consolidate
// 2. BitcopyType and related predicates are duplicated across files
// 3. StripPerm helper is duplicated; extract to common module
// 4. The 128-bit integer parsing should use a shared utility
// 5. Tuple index must be a compile-time constant (statically resolved)
// =============================================================================

