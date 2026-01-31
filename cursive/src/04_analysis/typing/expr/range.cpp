// =============================================================================
// MIGRATION MAPPING: expr/range.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 5.2: Static Semantics - Range Expressions
//   - T-Equiv-Range (lines 8574-8577): Range type equivalence
//   - Range expression typing
//   - Exclusive (..) and inclusive (..=)
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/types/type_infer.cpp
//   Range expression typing
//
// KEY CONTENT TO MIGRATE:
//   RANGE EXPRESSION (start..end | start..=end):
//   1. Type start expression
//   2. Type end expression
//   3. Verify start and end have compatible types
//   4. Result is Range type
//
//   TYPING:
//   Gamma |- start : T
//   Gamma |- end : T
//   T supports iteration (integer, Step class)
//   --------------------------------------------------
//   Gamma |- start..end : Range
//
//   RANGE FORMS:
//   - a..b: exclusive (a <= x < b)
//   - a..=b: inclusive (a <= x <= b)
//   - ..b: range to (0 <= x < b)
//   - a..: range from (a <= x)
//
//   SUPPORTED TYPES:
//   - Integer types (i32, i64, u32, etc.)
//   - Types implementing Step class
//
//   RANGE TYPE:
//   - TypeRange is the range type
//   - Used in loop iteration
//   - Bounds stored for iteration
//
//   USE IN LOOPS:
//   loop i in 0..10 {
//       // i iterates 0, 1, ..., 9
//   }
//
//   loop i in 0..=10 {
//       // i iterates 0, 1, ..., 10
//   }
//
// DEPENDENCIES:
//   - ExprTypeFn for bound expressions
//   - TypeEquiv for type compatibility
//   - TypeRange construction
//   - Step class check (optional)
//
// SPEC RULES:
//   - T-Equiv-Range (lines 8574-8577)
//   - Range expression semantics
//
// RESULT TYPE:
//   Range (TypeRange)
//
// =============================================================================

