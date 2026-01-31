// =============================================================================
// MIGRATION MAPPING: pattern/literal_pattern.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 5.4: Pattern Matching
//   - P-Literal (line 11464): Literal pattern typing
//   - Literal patterns are refutable
//   - Only valid in match expressions
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/types/type_pattern.cpp
//   Literal pattern typing
//
// KEY CONTENT TO MIGRATE:
//   LITERAL PATTERN (42, true, 'a', "str"):
//   1. Determine literal type
//   2. Check scrutinee type compatible with literal type
//   3. Pattern is refutable (may not match)
//   4. No bindings produced
//
//   TYPING RULE (P-Literal):
//   Gamma |- literal : T_lit
//   Gamma |- T_s <: T_lit or T_lit <: T_s
//   --------------------------------------------------
//   Gamma |- literal <= T_s => ok, bindings = []
//
//   LITERAL TYPES:
//   - Integer: i32, i64, etc. (inferred from context)
//   - Boolean: bool
//   - Character: char
//   - String: string@View
//
//   REFUTABILITY:
//   - Literal patterns are ALWAYS refutable
//   - Cannot be used in let/var bindings
//   - Must be in match expression
//   - Other arms must cover remaining cases
//
//   LET-REFUTABLE-PATTERN-ERR:
//   - Error if literal pattern used in let/var
//   - See line 9773-9776
//
//   EXAMPLE:
//   match x {
//       0 => { "zero" },
//       1 => { "one" },
//       n: i32 => { "other" }
//   }
//
// DEPENDENCIES:
//   - LiteralTypeFn for literal type
//   - Subtyping for compatibility check
//   - RefutableContext verification
//
// SPEC RULES:
//   - P-Literal (line 11464)
//   - Let-Refutable-Pattern-Err (line 9773)
//
// RESULT TYPE:
//   PatternResult { ok: true, bindings: [] }
//
// NOTES:
//   - No bindings - literal matches value exactly
//   - Refutable - other cases must be handled
//
// =============================================================================

