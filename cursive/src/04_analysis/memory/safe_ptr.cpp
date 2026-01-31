/*
 * =============================================================================
 * MIGRATION MAPPING: safe_ptr.cpp
 * =============================================================================
 *
 * SPEC REFERENCE:
 *   - C0updated.md, Section 5.5 "Pointer Types" (lines 13010-13150)
 *   - C0updated.md, Section 5.5.1 "Safe Pointers" (lines 13020-13100)
 *   - C0updated.md, Section 5.5.2 "Pointer States" (lines 13110-13150)
 *
 * SOURCE FILE:
 *   - cursive-bootstrap/src/03_analysis/memory/safe_ptr.cpp (lines 1-56)
 *
 * FUNCTIONS TO MIGRATE:
 *   - AsSafePtr(TypeRef ty) -> SafePtrType*                        [lines 12-25]
 *       Extract safe pointer type info if applicable
 *   - IsSafePtrType(TypeRef ty) -> bool                            [lines 27-35]
 *       Check if type is a safe pointer (Ptr<T>@State)
 *   - PtrStateOf(TypeRef ty) -> PtrState                           [lines 37-50]
 *       Get the state (@Valid, @Null, @Expired) of a pointer type
 *   - PtrElementType(TypeRef ty) -> TypeRef                        [lines 52-56]
 *       Get the element type T from Ptr<T>
 *
 * DEPENDENCIES:
 *   - TypeRef, PtrState enum
 *   - Modal state resolution for Ptr modal
 *
 * REFACTORING NOTES:
 *   1. Safe pointers are modal types: Ptr<T>@Valid, Ptr<T>@Null, Ptr<T>@Expired
 *   2. @Valid: guaranteed non-null, dereferenceable
 *   3. @Null: may be null, must check before deref
 *   4. @Expired: points to deallocated memory, cannot use
 *   5. Ptr::null() creates @Null pointer (requires type context)
 *   6. & operator creates @Valid pointer to existing binding
 *   7. Deref (*ptr) only allowed on @Valid
 *   8. Consider Ptr as built-in modal with niche optimization
 *
 * TYPE SIGNATURES:
 *   - Ptr<T>@Valid: Dereferenceable, non-null
 *   - Ptr<T>@Null: May be null, check required
 *   - Ptr<T>@Expired: Invalid, compile error to use
 *
 * DIAGNOSTIC CODES:
 *   - E-PTR-0001: Dereference of @Null pointer
 *   - E-PTR-0002: Use of @Expired pointer
 *   - E-PTR-0003: Null check required
 *
 * =============================================================================
 */
