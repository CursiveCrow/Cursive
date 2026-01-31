// =============================================================================
// MIGRATION MAPPING: expr/deref.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 5.5: Memory and Pointers (search for T-Deref rules)
//   - *ptr dereference for safe and raw pointers
//   - Safe pointer must be @Valid state
//   - Raw pointer deref requires unsafe
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/memory/safe_ptr.cpp
//   Dereference typing
//
// KEY CONTENT TO MIGRATE:
//   SAFE POINTER DEREFERENCE (*ptr):
//   1. Type the pointer expression
//   2. Must be Ptr<T>@Valid
//   3. Result type is T (element type)
//   4. If Ptr<T>@Null or Ptr<T>@Expired, error
//   5. If Ptr<T> (no state), error (state unknown)
//
//   RAW POINTER DEREFERENCE (*raw_ptr):
//   1. Type the pointer expression
//   2. Must be *imm T or *mut T
//   3. Requires unsafe context
//   4. Result type is T (element type)
//   5. Permission depends on qualifier:
//      - *imm T => const T
//      - *mut T => unique T
//
//   PERMISSION PROPAGATION:
//   - Ptr<unique T>@Valid => unique T
//   - Ptr<const T>@Valid => const T
//   - Ptr<shared T>@Valid => shared T
//
// DEPENDENCIES:
//   - ExprTypeFn for pointer expression
//   - IsInUnsafeSpan() for raw pointer check
//   - StripPerm() for base type extraction
//   - TypePtr, TypeRawPtr type nodes
//   - PtrState enum
//
// SPEC RULES:
//   - T-Deref-Ptr: Safe pointer dereference
//   - T-Deref-RawPtr: Raw pointer deref (unsafe)
//   - Deref-Null-Err: Deref of null pointer type
//   - Deref-Expired-Err: Deref of expired pointer
//   - Deref-RawPtr-NotUnsafe: Raw deref outside unsafe
//
// RESULT TYPE:
//   ExprTypeResult { bool ok; optional<string_view> diag_id; TypeRef type; }
//
// PLACE TYPING:
//   Dereference can also be a place expression for assignment:
//   *ptr = value (requires unique permission)
//
// =============================================================================

