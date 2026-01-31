// =============================================================================
// MIGRATION MAPPING: expr/addr_of.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 5.5: Memory and Pointers (search for T-AddrOf rules)
//   - &place creates safe pointer
//   - Place must be a valid lvalue
//   - Result is Ptr<T>@Valid
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/memory/safe_ptr.cpp
//   Address-of typing
//
// KEY CONTENT TO MIGRATE:
//   ADDRESS-OF (&place):
//   1. Type the place expression
//   2. Place must be an lvalue (variable, field, deref, index)
//   3. Check AddrOfOk predicate
//   4. Result is Ptr<T>@Valid where T is place type
//   5. Permission of pointer element matches place permission
//
//   ADDR_OF_OK PREDICATE:
//   - Place is a local variable: OK
//   - Place is a field access of OK place: OK
//   - Place is tuple access of OK place: OK
//   - Place is array/slice index of OK place: OK
//   - Place is dereference of valid pointer: OK
//   - Temporary values: NOT OK
//
//   PERMISSION RULES:
//   - &var where var: unique T => Ptr<unique T>@Valid
//   - &var where var: const T => Ptr<const T>@Valid
//   - &var where var: shared T => Ptr<shared T>@Valid
//
//   PACKED FIELD HANDLING:
//   - Address of packed struct field requires unsafe
//   - Alignment may not be correct for field type
//
// DEPENDENCIES:
//   - PlaceTypeFn for place typing
//   - IsPlaceExpr() for lvalue check
//   - AddrOfOk() predicate
//   - IsPackedField() for packed check
//   - IsInUnsafeSpan() for unsafe context
//   - MakeTypePtr() for result construction
//
// SPEC RULES:
//   - T-AddrOf: Address-of typing
//   - AddrOf-NotPlace-Err: Not an lvalue
//   - Packed-Field-Unsafe-Err: Packed field outside unsafe
//
// RESULT TYPE:
//   ExprTypeResult { bool ok; optional<string_view> diag_id; TypeRef type; }
//
// NOTE: Raw pointer creation uses different syntax/rules (unsafe)
//
// =============================================================================

