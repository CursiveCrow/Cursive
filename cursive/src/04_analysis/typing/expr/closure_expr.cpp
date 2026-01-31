// =============================================================================
// MIGRATION MAPPING: expr/closure_expr.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   NOTE: Closures are NOT SUPPORTED in Cursive0
//   - T-Equiv-Closure (lines 8544-8547): Closure type equivalence
//   - TypeClosure type constructor exists for future use
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/types/type_infer.cpp
//   Closure expression typing (if implemented)
//
// KEY CONTENT TO MIGRATE:
//   NOTE: CLOSURES ARE UNSUPPORTED IN CURSIVE0
//   This file exists as a placeholder for future closure support.
//
//   CURSIVE0 GUIDANCE:
//   - Use named procedures instead of closures
//   - |x| x + 1 syntax is NOT valid
//   - Define procedure add_one(x: i32) -> i32 { return x + 1 }
//
//   FUTURE CLOSURE DESIGN (if implemented):
//   - TypeClosure([params], return, disposition)
//   - Disposition: capturing vs non-capturing
//   - Capture semantics follow permission rules
//   - No escaping closures in spawn expressions
//
//   SPEC NOTES:
//   - T-Equiv-Closure defined for type equivalence
//   - Closure types have TypeClosure representation
//   - Escaping closures cannot be used in spawn (line 24263)
//
// DEPENDENCIES:
//   - N/A for Cursive0
//
// SPEC RULES:
//   - T-Equiv-Closure (type equivalence only)
//
// RESULT TYPE:
//   N/A - unsupported in Cursive0
//
// NOTES:
//   - This file should emit error for closure expressions
//   - Guide users to use named procedures
//   - Future support may add closure typing here
//
// =============================================================================

