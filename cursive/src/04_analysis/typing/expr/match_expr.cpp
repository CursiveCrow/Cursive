// =============================================================================
// MIGRATION MAPPING: expr/match_expr.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 5.2.15: Match Expressions (search for T-Match rules)
//   Section 5.4.2: Pattern Exhaustiveness
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/types/type_match.cpp
//   Match expression typing
//
// CROSS-REFERENCE: See also ../match_check.cpp for exhaustiveness
//
// KEY CONTENT TO MIGRATE:
//   MATCH EXPRESSION TYPING:
//   1. Type the scrutinee expression
//   2. For each arm:
//      a. Check pattern against scrutinee type
//      b. Bind pattern variables in arm scope
//      c. Type guard expression if present (must be bool)
//      d. Type arm body in extended scope
//   3. Unify all arm body types
//   4. Check exhaustiveness
//   5. Return unified body type
//
//   PATTERN BINDINGS:
//   - Identifier patterns bind to their matched type
//   - Typed patterns constrain the binding type
//   - Destructuring patterns bind fields/elements
//
//   ARM GUARDS:
//   - Optional boolean expression
//   - Access to pattern bindings
//   - Does not affect type (only exhaustiveness)
//
//   TYPE UNIFICATION:
//   - All arm bodies must have compatible types
//   - Never type (!) is absorbed by other types
//   - Result is common supertype or error
//
// DEPENDENCIES:
//   - ExprTypeFn for typing
//   - PatternTypeFn for pattern checking
//   - match_check.cpp for exhaustiveness
//   - Subtyping() for type compatibility
//   - PushScope/PopScope for arm scopes
//
// SPEC RULES:
//   - T-Match: Match expression typing
//   - T-Match-Arm: Individual arm typing
//   - Exhaustiveness: All values covered
//
// RESULT TYPE:
//   ExprTypeResult { bool ok; optional<string_view> diag_id; TypeRef type; }
//
// ERROR CASES:
//   - Pattern incompatible with scrutinee
//   - Guard expression not bool
//   - Arm body types incompatible
//   - Non-exhaustive match
//
// =============================================================================

