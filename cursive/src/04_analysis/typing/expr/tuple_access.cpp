// =============================================================================
// MIGRATION MAPPING: expr/tuple_access.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 5.2: Static Semantics - Tuple Index
//   - P-Tuple-Index (line 8895): Tuple index typing
//   - P-Tuple-Index-Perm (line 8900): With permission
//   - Numeric index syntax (tuple.0, tuple.1)
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/types/type_infer.cpp
//   Tuple access expression typing
//
// KEY CONTENT TO MIGRATE:
//   TUPLE ACCESS (tuple.index):
//   1. Type tuple expression
//   2. Verify tuple has enough elements
//   3. Extract element type at index
//   4. Preserve/propagate permission
//
//   TYPING RULE (P-Tuple-Index):
//   Gamma |- tuple : TypeTuple([T_0, T_1, ..., T_n])
//   0 <= index < n
//   --------------------------------------------------
//   Gamma |- tuple.index : T_index
//
//   TYPING RULE (P-Tuple-Index-Perm):
//   Gamma |- tuple : TypePerm(p, TypeTuple([T_0, ..., T_n]))
//   0 <= index < n
//   --------------------------------------------------
//   Gamma |- tuple.index : TypePerm(p, T_index)
//
//   INDEX SYNTAX:
//   - .0, .1, .2, etc. - numeric literals
//   - Must be valid index (0 <= i < arity)
//
//   PERMISSION PRESERVATION:
//   - If tuple has permission, element inherits it
//   - const tuple.0 -> const element
//   - unique tuple.0 -> unique element
//
//   PLACE EXPRESSION:
//   - tuple.index is a place expression
//   - Can be assigned if tuple is mutable
//   - Can take address of element
//
//   EXAMPLE:
//   let pair: (i32, bool) = (42, true)
//   let first: i32 = pair.0
//   let second: bool = pair.1
//
//   var mutable: (i32, i32) = (1, 2)
//   mutable.0 = 10  // assign to element
//
// DEPENDENCIES:
//   - ExprTypeFn for tuple typing
//   - TypeTuple for arity check
//   - PermissionPropagation
//   - PlaceExprCheck
//
// SPEC RULES:
//   - P-Tuple-Index (line 8895)
//   - P-Tuple-Index-Perm (line 8900)
//
// RESULT TYPE:
//   Element type at index (with permission if applicable)
//
// =============================================================================

