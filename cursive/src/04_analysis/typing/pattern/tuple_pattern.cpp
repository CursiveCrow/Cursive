// =============================================================================
// MIGRATION MAPPING: pattern/tuple_pattern.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 5.4: Pattern Matching
//   - Pat-Tuple (lines 9752-9755): Tuple pattern typing
//   - Pat-Tuple-Arity-Err (lines 9747-9750): Arity mismatch
//   - P-Tuple (line 11440): Provenance
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/types/type_pattern.cpp
//   Tuple pattern typing
//
// KEY CONTENT TO MIGRATE:
//   TUPLE PATTERN ((p1, p2, ...)):
//   1. Check scrutinee is tuple type
//   2. Verify arity matches (error if not)
//   3. Recursively check each sub-pattern
//   4. Collect bindings from all sub-patterns
//   5. Irrefutable if all sub-patterns irrefutable
//
//   TYPING RULE (Pat-Tuple):
//   T = TypeTuple([T_1, ..., T_n])
//   For each i: Gamma |- p_i <= T_i => B_i
//   B = union of all B_i
//   --------------------------------------------------
//   Gamma |- (p_1, ..., p_n) <= T => ok, bindings = B
//
//   TYPING RULE (Pat-Tuple-Arity-Err):
//   T = TypeTuple([T_1, ..., T_n])
//   patterns = [p_1, ..., p_m]
//   m != n
//   --------------------------------------------------
//   Gamma |- (p_1, ..., p_m) <= T => error
//
//   SINGLE-ELEMENT TUPLE:
//   - (x;) for single-element tuple pattern
//   - Matches (T;) single-element tuple type
//
//   NESTED PATTERNS:
//   - Each p_i can be any pattern
//   - Recursive binding collection
//   - Nested wildcards, identifiers, etc.
//
//   EXAMPLE:
//   let (x, y) = pair          // destructure 2-tuple
//   let (a, (b, c)) = nested   // nested destructuring
//   let (first, _) = pair      // partial destructuring
//
// DEPENDENCIES:
//   - PatternTypeFn for sub-pattern typing
//   - TypeTuple for scrutinee check
//   - BindingMerge for collecting bindings
//
// SPEC RULES:
//   - Pat-Tuple (lines 9752-9755)
//   - Pat-Tuple-Arity-Err (lines 9747-9750)
//
// RESULT TYPE:
//   PatternResult { ok: true/false, bindings: merged }
//
// NOTES:
//   - Arity must match exactly
//   - Bindings from all sub-patterns combined
//   - Irrefutable if all sub-patterns irrefutable
//
// =============================================================================

