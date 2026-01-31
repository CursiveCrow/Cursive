// =============================================================================
// MIGRATION MAPPING: expr/index_access.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 5.2.6: Arrays and Slices (lines 8922-9055)
//   - T-Index-Array (lines 8931-8934): Array indexing with const index
//   - T-Index-Array-Dynamic (lines 8936-8939): Dynamic array indexing
//   - T-Index-Array-Perm (lines 8941-8944): Permission-qualified array
//   - T-Index-Slice (lines 8961-8964): Slice indexing
//   - T-Slice-From-Array (lines 8971-8974): Range indexing produces slice
//   - P-Index-* rules: Place typing variants
//   - Index-Array-OOB-Err, Index-Array-NonConst-Err, Index-NonIndexable
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/composite/arrays_slices.cpp
//   Index access typing
//
// KEY CONTENT TO MIGRATE:
//   ARRAY INDEXING:
//   1. Type base expression (must be array or slice)
//   2. Type index expression (must be usize)
//   3. For arrays with const index:
//      - Evaluate index via ConstLen
//      - Check bounds: index < array length
//   4. For dynamic index:
//      - Requires [[dynamic]] context
//      - Runtime bounds check
//   5. Result is element type (with permission)
//
//   SLICE INDEXING:
//   - No compile-time bounds check
//   - Runtime bounds check
//   - Result is element type
//
//   RANGE INDEXING (slicing):
//   - base[start..end] or base[start..=end]
//   - Produces a slice of the element type
//   - Works on arrays and slices
//
//   PERMISSION PROPAGATION:
//   - const array => const element
//   - unique array => unique element
//   - shared array => shared element
//
// DEPENDENCIES:
//   - ExprTypeFn for base and index typing
//   - ConstLen() for constant index evaluation
//   - IsInDynamicContext() for dynamic indexing
//   - MakeTypeSlice() for slice result
//   - StripPerm() for base type extraction
//
// SPEC RULES IMPLEMENTED:
//   - T-Index-Array, T-Index-Array-Dynamic
//   - T-Index-Array-Perm, T-Index-Array-Perm-Dynamic
//   - T-Index-Slice, T-Index-Slice-Perm
//   - T-Slice-From-Array, T-Slice-From-Array-Perm
//   - T-Slice-From-Slice, T-Slice-From-Slice-Perm
//   - P-Index-* for place typing
//   - Coerce-Array-Slice: Array coercion to slice
//
// ERROR CASES:
//   - Index-Array-OOB-Err: Compile-time out-of-bounds
//   - Index-Array-NonConst-Err: Non-const index outside dynamic context
//   - Index-Array-NonUsize: Index not usize
//   - Index-NonIndexable: Base not array/slice
//
// =============================================================================

