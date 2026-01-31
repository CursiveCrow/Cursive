// =============================================================================
// MIGRATION MAPPING: expr/null_ptr.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 5.2.9: Type Inference (lines 9157-9172)
//   - Chk-Null-Ptr (lines 9157-9160): Ptr::null() with expected Ptr<U, s> where s in {Null, none}
//   - PtrNullExpected (line 9162): T = TypePtr(U, s) and s in {Null, none}
//   - Syn-PtrNull-Err (lines 9164-9167): Cannot infer Ptr::null() type
//   - Chk-PtrNull-Err (lines 9169-9172): Expected type not compatible with null
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/types/type_infer.cpp
//   - PtrNullExpected() helper (lines 40-58)
//   - InferExprImpl() - PtrNullExpr case (lines 338-342)
//   - CheckExprImpl() - PtrNullExpr case (lines 486-495)
//
// KEY CONTENT TO MIGRATE:
//   PTR::NULL() CHECKING:
//   - Synthesis: Error - cannot infer type without context
//   - Checking: OK if expected type is Ptr<T>@Null or Ptr<T> (no state)
//
//   PtrNullExpected PREDICATE:
//   - Strip permission qualifiers (TypePerm)
//   - Strip refinement types (TypeRefine)
//   - Check if result is TypePtr with state Null or no state
//
//   ERROR HANDLING:
//   - "PtrNull-Infer-Err" when synthesizing without context
//   - "PtrNull-Infer-Err" when checking against incompatible type
//
// DEPENDENCIES:
//   - TypeRef for types
//   - TypePerm, TypeRefine, TypePtr type nodes
//   - PtrState enum (Valid, Null, Expired)
//
// SPEC RULES IMPLEMENTED:
//   - Chk-Null-Ptr: Check Ptr::null() against expected Ptr type
//   - Syn-PtrNull-Err: Error on synthesizing Ptr::null()
//   - Chk-PtrNull-Err: Error when expected type doesn't allow null
//
// HELPER FUNCTIONS:
//   bool PtrNullExpected(const TypeRef& type) {
//     // Strip TypePerm and TypeRefine wrappers
//     // Return true if TypePtr with state Null or no state
//   }
//
// NOTE: Distinct from null literal (for raw pointers) - see NullLiteralExpected
//
// =============================================================================

