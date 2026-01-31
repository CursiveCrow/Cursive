// =============================================================================
// MIGRATION MAPPING: pattern/wildcard_pattern.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 5.4: Pattern Matching
//   - Pat-Wildcard (line 9734-9736): Wildcard pattern typing
//   - Always matches, no bindings
//   - Irrefutable
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/types/type_pattern.cpp
//   Wildcard pattern typing
//
// KEY CONTENT TO MIGRATE:
//   WILDCARD PATTERN (_):
//   1. Always matches any value
//   2. No bindings produced
//   3. Irrefutable - always succeeds
//   4. Value is discarded
//
//   TYPING RULE (Pat-Wildcard):
//   --------------------------------------------------
//   Gamma |- _ <= T => ok, bindings = []
//
//   USES:
//   - Catch-all in match: _ => { default_case }
//   - Ignore values: let _ = compute()
//   - Partial destructuring: let (x, _) = pair
//
//   IRREFUTABILITY:
//   - Wildcard is irrefutable
//   - Can be used in let/var bindings
//   - No type constraints
//
//   EXAMPLES:
//   let _ = side_effect()  // discard result
//
//   match x {
//       Some(v) => { use(v) },
//       _ => { default }     // catch-all
//   }
//
//   let (first, _) = pair   // ignore second
//
// DEPENDENCIES:
//   - None - simplest pattern
//
// SPEC RULES:
//   - Pat-Wildcard (lines 9734-9736)
//
// RESULT TYPE:
//   PatternResult { ok: true, bindings: [] }
//
// NOTES:
//   - Wildcard matches anything
//   - No binding created
//   - Often used as last match arm
//
// =============================================================================

