// =============================================================================
// MIGRATION MAPPING: stmt/compound_assign_stmt.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 5.2.9: Assignment Statements
//   - Compound assignment operators: +=, -=, *=, /=, %=, &=, |=, ^=, <<=, >>=
//   - Desugaring to binary operation + assignment
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/types/type_infer.cpp
//   Compound assignment statement typing
//
// KEY CONTENT TO MIGRATE:
//   COMPOUND ASSIGNMENT (place op= expr):
//   1. Type the place expression (LHS)
//   2. Verify place is assignable (mutable binding or field)
//   3. Type the value expression (RHS)
//   4. Check binary operation (place op expr) is valid
//   5. Check result type is assignable to place type
//   6. Desugar: place = place op expr
//
//   TYPING:
//   Gamma; R; L |- place :place T_p
//   place is mutable
//   Gamma; R; L |- expr : T_e
//   T_p op T_e => T_r
//   Gamma |- T_r <: T_p
//   --------------------------------------------------
//   Gamma |- place op= expr
//
//   COMPOUND OPERATORS:
//   - Arithmetic: +=, -=, *=, /=, %=
//   - Bitwise: &=, |=, ^=
//   - Shift: <<=, >>=
//
//   DESUGARING:
//   x += 1  =>  x = x + 1
//   a[i] *= 2  =>  a[i] = a[i] * 2
//
//   PLACE REQUIREMENTS:
//   - Must be assignable (mutable)
//   - Types must support the binary operator
//   - Result type must be compatible with place type
//
//   EVALUATION ORDER:
//   - Place evaluated once (not twice as in naive desugaring)
//   - RHS evaluated after place
//   - Result assigned to place
//
// DEPENDENCIES:
//   - PlaceTypeFn for LHS typing
//   - ExprTypeFn for RHS typing
//   - BinaryOpTyping for operation check
//   - Subtyping for result assignability
//   - Mutability checking
//
// SPEC RULES:
//   - Compound assignment semantics
//   - Binary operator typing rules
//
// RESULT:
//   Statement effect (assignment)
//   Diagnostics for type mismatches
//
// =============================================================================

