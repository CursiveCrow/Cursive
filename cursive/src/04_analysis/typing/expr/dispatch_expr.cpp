// =============================================================================
// MIGRATION MAPPING: expr/dispatch_expr.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 17.2.3: Dispatch Expression (lines 25136+)
//   - T-Dispatch (line 25166): Basic dispatch typing
//   - T-Dispatch-Reduce (line 25172): Dispatch with reduction
//   - dispatch_expr grammar (line 3244)
//   - dispatch_option grammar (line 3247)
//   - Key-based parallelism (key clause)
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/types/type_infer.cpp
//   Dispatch expression typing portions
//
// KEY CONTENT TO MIGRATE:
//   DISPATCH EXPRESSION (dispatch pat in range key? opts { body }):
//   1. Verify inside parallel block
//   2. Type range expression (must be range type)
//   3. Bind pattern to iteration index
//   4. Process key clause if present
//   5. Process options (reduce, ordered, chunk)
//   6. Type body expression
//   7. Compute result type based on options
//
//   TYPING RULE (T-Dispatch):
//   Inside parallel block
//   range : Range
//   pat binds index variable
//   body : T
//   --------------------------------------------------
//   Gamma |- dispatch pat in range {body} : ()
//
//   TYPING RULE (T-Dispatch-Reduce):
//   Inside parallel block
//   range : Range
//   reduce operator: +, *, min, max, and, or, or custom
//   body : T
//   --------------------------------------------------
//   Gamma |- dispatch pat in range [reduce: op] {body} : T
//
//   DISPATCH OPTIONS:
//   - reduce: op - reduction operation (+, *, min, max, and, or)
//   - ordered - preserve iteration order for side effects
//   - chunk: expr - set chunk size for work distribution
//
//   KEY CLAUSE:
//   - key path mode - acquire keys for parallel access
//   - Enables safe parallel mutation
//   - Key pattern determines parallelism
//
//   KEY-BASED PARALLELISM:
//   | Pattern     | Keys Generated    | Parallelism           |
//   |-------------|-------------------|-----------------------|
//   | data[i]     | n distinct keys   | Full parallel         |
//   | data[i / 2] | n/2 distinct keys | Pairs serialize       |
//   | data[i % k] | k distinct keys   | k-way parallel        |
//
// DEPENDENCIES:
//   - ParallelContext verification
//   - RangeTypeFn for range typing
//   - PatternTypeFn for index pattern
//   - ExprTypeFn for body typing
//   - KeyClause processing
//   - Reduction operator validation
//
// SPEC RULES:
//   - T-Dispatch (line 25166)
//   - T-Dispatch-Reduce (line 25172)
//
// RESULT TYPE:
//   () for basic dispatch, T for dispatch with reduce
//
// NOTES:
//   - dispatch creates multiple parallel work items
//   - Key clause enables safe parallel mutation
//   - Reduction combines results from all iterations
//
// =============================================================================

