// =============================================================================
// MIGRATION MAPPING: expr/pipeline_expr.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   NOTE: Pipelines are NOT SUPPORTED in Cursive0
//
// SOURCE FILE: N/A
//   Pipeline expression typing (not implemented)
//
// KEY CONTENT TO MIGRATE:
//   NOTE: PIPELINES ARE UNSUPPORTED IN CURSIVE0
//   This file exists as a placeholder for future pipeline support.
//
//   CURSIVE0 GUIDANCE:
//   - Use explicit method chains: a~>method1()~>method2()
//   - Use intermediate bindings for complex transformations
//   - Pipeline operator (|>) not available
//
//   EXAMPLE WORKAROUND:
//   Instead of: data |> transform |> filter |> reduce
//   Use:
//     let t1 = transform(data)
//     let t2 = filter(t1)
//     let result = reduce(t2)
//   Or:
//     data~>transform()~>filter()~>reduce()
//
// DEPENDENCIES:
//   - N/A for Cursive0
//
// SPEC RULES:
//   - N/A - unsupported feature
//
// RESULT TYPE:
//   N/A - unsupported in Cursive0
//
// NOTES:
//   - This file should emit error for pipeline expressions
//   - Guide users to use method chains or intermediate bindings
//   - Future support may add pipeline typing here
//
// =============================================================================

