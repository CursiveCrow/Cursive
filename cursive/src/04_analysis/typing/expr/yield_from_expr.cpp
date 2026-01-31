// =============================================================================
// MIGRATION MAPPING: expr/yield_from_expr.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 17.3.3: Yield From Expression (lines 25542+)
//   - T-Yield-From (line 25542): Yield from typing rule
//   - yield from delegation semantics
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/types/type_infer.cpp
//   Yield from expression typing portions
//
// KEY CONTENT TO MIGRATE:
//   YIELD FROM EXPRESSION (yield from expr | yield release from expr):
//   1. Verify inside async procedure
//   2. Type the delegated async expression
//   3. Check delegated async is compatible with enclosing async
//   4. Delegate all yielded values from inner async
//   5. Result is the inner async's Result type
//
//   TYPING RULE (T-Yield-From):
//   AsyncSig(R) = (Out_r, In_r, Result_r, E_r)
//   Gamma |- expr : Async<Out_i, In_i, Result_i, E_i>
//   Out_i <: Out_r
//   In_r <: In_i
//   E_i <: E_r
//   --------------------------------------------------
//   Gamma |- yield from expr : Result_i
//
//   YIELD FROM FORMS:
//   - yield from expr: delegate to inner async
//   - yield release from expr: release keys, then delegate
//
//   DELEGATION SEMANTICS:
//   - All values from inner async yielded to outer
//   - All inputs from outer async forwarded to inner
//   - Errors from inner propagated to outer
//   - Result returned when inner completes
//
//   TYPE COMPATIBILITY:
//   - Inner Out must be subtype of outer Out
//   - Outer In must be subtype of inner In (contravariant)
//   - Inner E must be subtype of outer E
//
//   KEY RELEASE:
//   Same rules as yield:
//   - yield from MUST NOT occur while keys held (unless release)
//   - E-CON-0223: yield from inside sync expression
//
// DEPENDENCIES:
//   - AsyncContext verification
//   - AsyncSig extraction
//   - ExprTypeFn for delegated async typing
//   - Subtyping for compatibility checks
//   - Key state verification
//
// SPEC RULES:
//   - T-Yield-From (line 25542)
//   - E-CON-0223: yield from inside sync
//
// RESULT TYPE:
//   Result_i from delegated async
//
// NOTES:
//   - yield from is async delegation/composition
//   - Similar to Python's yield from
//   - Enables async pipeline composition
//
// =============================================================================

