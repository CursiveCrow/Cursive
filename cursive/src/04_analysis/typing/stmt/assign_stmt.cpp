// =============================================================================
// MIGRATION MAPPING: stmt/assign_stmt.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 5.2.11: Statement Typing - Assignments (lines 9449-9488)
//   - PlaceRoot definition (lines 9451-9455)
//   - T-Assign (lines 9457-9460): Basic assignment
//   - T-CompoundAssign (lines 9462-9465): Compound assignment (+=, etc.)
//   - Assign-NotPlace (lines 9467-9470): Not a place error
//   - Assign-Immutable-Err (lines 9472-9475): Immutable binding error
//   - Assign-Type-Err (lines 9477-9480): Type mismatch error
//   - Assign-Const-Err (lines 9484-9487): Const-qualified error
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/types/type_stmt.cpp
//   Assignment statement typing
//
// KEY CONTENT TO MIGRATE:
//   SIMPLE ASSIGNMENT (place = expr):
//   1. Check place is an lvalue
//   2. Find place root variable
//   3. Check root is mutable (var, not let)
//   4. Type place as place expression
//   5. Check expr type against place type
//   6. No const-qualified place
//
//   COMPOUND ASSIGNMENT (place op= expr):
//   1. All checks from simple assignment
//   2. Place type must be numeric
//   3. Expr type must be subtype of place type's primitive
//   4. Operators: +=, -=, *=, /=, %=, &=, |=, ^=, <<=, >>=
//
//   PLACE ROOT:
//   - Identifier: root is the identifier
//   - FieldAccess: recurse on base
//   - TupleAccess: recurse on base
//   - IndexAccess: recurse on base
//   - Deref: recurse on pointer
//
// DEPENDENCIES:
//   - PlaceTypeFn for place typing
//   - ExprTypeFn for value typing
//   - IsPlaceExpr() for lvalue check
//   - PlaceRoot() for root extraction
//   - MutOf() for mutability check
//   - Subtyping() for type compatibility
//   - StripPerm() for permission check
//
// SPEC RULES IMPLEMENTED:
//   - T-Assign, T-CompoundAssign
//   - Assign-NotPlace, Assign-Immutable-Err
//   - Assign-Type-Err, Assign-Const-Err
//
// RESULT TYPE:
//   StmtResult { Gamma (unchanged), Res=[], Brk=[], BrkVoid=false }
//
// =============================================================================

