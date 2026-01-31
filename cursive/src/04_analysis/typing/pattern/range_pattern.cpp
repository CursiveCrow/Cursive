// =============================================================================
// MIGRATION MAPPING: pattern/range_pattern.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 5.4: Pattern Matching
//   - Range pattern typing
//   - Exclusive (0..10) and inclusive (0..=10)
//   - Let-Refutable-Pattern-Err (line 9773)
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/types/type_pattern.cpp
//   Range pattern typing
//
// KEY CONTENT TO MIGRATE:
//   RANGE PATTERN (start..end | start..=end):
//   1. Type start expression (must be constant)
//   2. Type end expression (must be constant)
//   3. Verify start and end have same type
//   4. Check scrutinee type matches
//   5. Pattern is refutable (value may not be in range)
//   6. No bindings produced
//
//   TYPING:
//   Gamma |- start : T
//   Gamma |- end : T
//   T is integer or char type
//   IsConstExpr(start) and IsConstExpr(end)
//   --------------------------------------------------
//   Gamma |- start..end <= T => ok, bindings = []
//
//   RANGE FORMS:
//   - a..b: exclusive range (a <= x < b)
//   - a..=b: inclusive range (a <= x <= b)
//
//   SUPPORTED TYPES:
//   - Integer types: i8, i16, i32, i64, etc.
//   - Unsigned: u8, u16, u32, u64, etc.
//   - Character: char
//
//   CONSTANT REQUIREMENT:
//   - Both start and end must be constant expressions
//   - Evaluated at compile time
//   - Enables exhaustiveness checking
//
//   REFUTABILITY:
//   - Range patterns are refutable
//   - Cannot be used in let/var directly
//   - Must be in match expression
//
//   EXAMPLE:
//   match code {
//       0..10 => { "single digit" },
//       10..100 => { "double digit" },
//       _ => { "large" }
//   }
//
//   match ch {
//       'a'..='z' => { "lowercase" },
//       'A'..='Z' => { "uppercase" },
//       '0'..='9' => { "digit" },
//       _ => { "other" }
//   }
//
// DEPENDENCIES:
//   - ExprTypeFn for bound typing
//   - ConstEval for constant evaluation
//   - Subtyping for type compatibility
//   - RefutableContext verification
//
// SPEC RULES:
//   - Range pattern semantics
//   - Let-Refutable-Pattern-Err (line 9773)
//
// RESULT TYPE:
//   PatternResult { ok: true, bindings: [] }
//
// NOTES:
//   - No bindings - just tests membership
//   - Bounds must be constants
//   - Useful for numeric/char classification
//
// =============================================================================

