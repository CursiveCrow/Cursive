// =============================================================================
// MIGRATION MAPPING: arrays_slices.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
// - Section 5.2.6 "Arrays and Slices" (lines 8922-9054)
//   - ConstIndex definition (line 8924)
//   - T-Array-Literal-List (lines 8926-8929)
//   - T-Index-Array (lines 8931-8934)
//   - T-Index-Array-Dynamic (lines 8936-8939)
//   - T-Index-Array-Perm (lines 8941-8944)
//   - T-Index-Array-Perm-Dynamic (lines 8946-8949)
//   - Index-Array-NonConst-Err (lines 8951-8954)
//   - Index-Array-OOB-Err (lines 8956-8959)
//   - T-Index-Slice (lines 8961-8964)
//   - T-Index-Slice-Perm (lines 8966-8969)
//   - T-Slice-From-Array (lines 8971-8974)
//   - T-Slice-From-Array-Perm (lines 8976-8979)
//   - T-Slice-From-Slice (lines 8981-8984)
//   - T-Slice-From-Slice-Perm (lines 8986-8989)
//   - P-Index-Array (lines 8991-8994)
//   - P-Index-Array-Perm (lines 8996-8999)
//   - P-Index-Array-Dynamic (lines 9001-9004)
//   - P-Index-Array-Perm-Dynamic (lines 9006-9009)
//   - P-Index-Slice (lines 9011-9014)
//   - P-Index-Slice-Perm (lines 9016-9019)
//   - P-Slice-From-Array (lines 9021-9024)
//   - P-Slice-From-Array-Perm (lines 9026-9029)
//   - P-Slice-From-Slice (lines 9031-9034)
//   - P-Slice-From-Slice-Perm (lines 9036-9039)
//   - Coerce-Array-Slice (lines 9041-9044)
//   - Index-Array-NonUsize (lines 9046-9049)
//   - Index-NonIndexable (lines 9051-9054)
// - Section 5.2.1 "Type Equivalence" (lines 8492-8593)
//   - ConstLenJudg (line 8495)
//   - ConstLen-Lit (lines 8497-8500)
//   - ConstLen-Path (lines 8502-8505)
//   - ConstLen-Err (lines 8507-8510)
// - Section 5.11 "Foundational Predicates" for BitcopyType
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/composite/arrays_slices.cpp
// - Lines 1-557 (entire file)
//
// Key source functions to migrate:
// - TypeArrayExpr (lines 184-222): Array literal typing
// - TypeIndexAccessValue (lines 224-395): Array/slice index access (value context)
// - TypeIndexAccessPlace (lines 397-531): Array/slice index access (place context)
// - CoerceArrayToSlice (lines 533-554): Array to slice coercion
//
// Supporting helpers:
// - StripPermOnce (lines 27-35): Strip one layer of permission
// - IsRangeExpr (lines 37-39): Check if expression is a range
// - kIntTypes/kFloatTypes (lines 41-47): Primitive type name lists
// - IsPrimTypeName (lines 67-70): Check primitive type names
// - BuiltinBitcopyType (lines 72-97): Check built-in bitcopy types
// - IsBitcopyClassPath (lines 99-101): Check Bitcopy class path
// - ImplementsBitcopy (lines 103-138): Check if type implements Bitcopy
// - CheckExprUsize (lines 140-157): Validate usize expression
// - CheckRangeExpr (lines 159-180): Validate range expression
//
// DEPENDENCIES:
// - cursive/src/04_analysis/types/type_equiv.h (TypeEquiv)
// - cursive/src/04_analysis/types/type_expr.h (TypeRef, type constructors)
// - cursive/src/04_analysis/resolve/scopes.h (ScopeContext)
// - cursive/src/00_core/assert_spec.h (SPEC_DEF, SPEC_RULE)
//
// REFACTORING NOTES:
// 1. The source duplicates BuiltinBitcopyType and ImplementsBitcopy helpers that
//    should be consolidated into a shared predicate module
// 2. Consider extracting StripPerm helpers to a common location
// 3. The BitcopyType check pattern is repeated; unify with 05_analysis/predicates
// 4. Range expression validation could be factored out
// 5. Permission propagation logic is duplicated between value/place contexts
// =============================================================================

