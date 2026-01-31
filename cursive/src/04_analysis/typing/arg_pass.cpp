// =============================================================================
// MIGRATION MAPPING: arg_pass.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 5.2.4: Procedure Calls (lines 8707-8777)
//   - ArgType(a) definition (lines 8715-8717):
//     * If ArgMoved(a) = true: ExprType(MovedArg(ArgMoved(a), ArgExpr(a)))
//     * If ArgMoved(a) = false: PlaceType(ArgExpr(a))
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/memory/calls.cpp
//   (Argument passing semantics)
//
// KEY CONTENT TO MIGRATE:
//   ARGUMENT TYPE COMPUTATION:
//   - For move parameters: Type the expression as a moved value
//   - For reference parameters: Type as a place expression
//
//   MOVE ARGUMENT HANDLING:
//   - MovedArg judgment combines move flag with expression
//   - Move invalidates the source binding
//   - Type is the owned type (not a reference)
//
//   REFERENCE ARGUMENT HANDLING:
//   - PlaceType judgment for the argument expression
//   - Must be a valid place (lvalue)
//   - Type is the place type with permission preserved
//
//   PARAMETER MODE MATCHING:
//   - ParamMode = `move` requires ArgMoved = true
//   - ParamMode = none requires ArgMoved = false
//   - Mismatch errors: Call-Move-Missing, Call-Move-Unexpected
//
// DEPENDENCIES:
//   - TypeRef for types
//   - ParamMode enum (None, Move)
//   - IsPlaceExpr() for place detection
//   - ExprTypeFn for expression typing
//   - PlaceTypeFn for place typing
//   - Subtyping() for compatibility check
//
// REFACTORING NOTES:
//   1. This is closely related to args_ok.cpp but focuses on individual arg
//   2. Consider combining into a unified argument handling module
//   3. The distinction between moved and reference args is fundamental
//   4. Place typing preserves permissions while move typing transfers ownership
//
// HELPER FUNCTIONS:
//   ArgType() - Compute type for a single argument
//   CheckArgModeMatch() - Verify move flag matches parameter mode
//   TypeMovedArg() - Type an argument with move semantics
//   TypeRefArg() - Type an argument as a reference
//
// =============================================================================

