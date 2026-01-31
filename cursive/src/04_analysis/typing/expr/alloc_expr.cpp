// =============================================================================
// MIGRATION MAPPING: expr/alloc_expr.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 5.5: Memory and Pointers - Allocation
//   - T-Alloc-Explicit (line 10026): Explicit region allocation
//   - T-Alloc-Implicit (line 10031): Implicit region allocation
//   - alloc_expr grammar (line 3223)
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/types/type_infer.cpp
//   Allocation expression typing
//
// KEY CONTENT TO MIGRATE:
//   ALLOCATION EXPRESSION (^value | region ^ value):
//   1. Verify inside region context
//   2. Type the value expression
//   3. For explicit: verify region is active
//   4. Result is Ptr<T>@Valid
//
//   TYPING RULE (T-Alloc-Implicit):
//   Inside region context
//   Gamma; R; L |- value : T
//   --------------------------------------------------
//   Gamma |- ^value : Ptr<T>@Valid
//
//   TYPING RULE (T-Alloc-Explicit):
//   Gamma |- region : Region@Active
//   Gamma; R; L |- value : T
//   --------------------------------------------------
//   Gamma |- region ^ value : Ptr<T>@Valid
//
//   ALLOCATION FORMS:
//   - ^value: allocate in current region
//   - region ^ value: allocate in named region
//
//   REGION CONTEXT:
//   - Allocation only valid inside region block
//   - Error if no enclosing region
//
//   POINTER LIFETIME:
//   - Ptr valid while region is active
//   - Becomes @Expired when region freed
//   - No escape from region scope
//
//   EXAMPLE:
//   region {
//       let p: Ptr<i32>@Valid = ^42
//       *p = *p + 1
//   }
//   // p is now @Expired
//
//   region as r {
//       let p: Ptr<i32>@Valid = r ^ 100
//   }
//
// DEPENDENCIES:
//   - RegionContext verification
//   - ExprTypeFn for value typing
//   - Ptr<T>@Valid type construction
//   - Region@Active check
//
// SPEC RULES:
//   - T-Alloc-Explicit (line 10026)
//   - T-Alloc-Implicit (line 10031)
//
// RESULT TYPE:
//   Ptr<T>@Valid where T is value type
//
// =============================================================================

