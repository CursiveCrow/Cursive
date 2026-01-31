// =============================================================================
// MIGRATION MAPPING: expr/if_expr.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 5.2.14: Control Flow Expressions (search for T-If rules)
//   - T-If-Else: if cond { then_branch } else { else_branch }
//   - T-If-Unit: if cond { then_branch } (no else, result is unit)
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/types/type_expr.cpp
//   IfExpr typing
//
// KEY CONTENT TO MIGRATE:
//   IF-ELSE TYPING:
//   1. Type condition expression
//   2. Condition must be bool
//   3. Type then-branch
//   4. Type else-branch (if present)
//   5. Both branches must have compatible types
//   6. Result type is the unified branch type
//
//   IF WITHOUT ELSE:
//   - Then-branch must have unit type
//   - Result type is unit ()
//   - Otherwise, missing else is an error
//
//   TYPE UNIFICATION:
//   - If branches have same type, use it
//   - If one branch is !, use the other's type
//   - Otherwise, find common supertype or error
//
// DEPENDENCIES:
//   - ExprTypeFn for condition and branch typing
//   - TypeEquiv() for branch type comparison
//   - Subtyping() for type compatibility
//   - MakeTypePrim("()") for unit type
//
// SPEC RULES (approximate):
//   - T-If-Else: Both branches present, types unified
//   - T-If-Unit: No else, then must be unit
//
// RESULT TYPE:
//   ExprTypeResult { bool ok; optional<string_view> diag_id; TypeRef type; }
//
// ERROR CASES:
//   - Condition is not bool
//   - Branch types incompatible
//   - No else and then is not unit
//
// =============================================================================

