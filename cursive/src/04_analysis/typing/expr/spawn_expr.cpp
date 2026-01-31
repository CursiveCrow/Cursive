// =============================================================================
// MIGRATION MAPPING: expr/spawn_expr.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 17.2.2: Spawn Expression (lines 25061+)
//   - T-Spawn (line 25061): Spawn typing rule
//   - spawn_option grammar (line 3231)
//   - Spawn options (line 25093)
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/types/type_infer.cpp
//   Spawn expression typing portions
//
// KEY CONTENT TO MIGRATE:
//   SPAWN EXPRESSION (spawn opts { body }):
//   1. Verify inside parallel block
//   2. Process spawn options (name, affinity, priority)
//   3. Type body expression
//   4. Result is Spawned<T> where T is body type
//
//   TYPING RULE (T-Spawn):
//   Inside parallel block
//   opts well-formed
//   Gamma |- body : T
//   --------------------------------------------------
//   Gamma |- spawn opts {body} : Spawned<T>
//
//   SPAWN OPTIONS:
//   - name: string - debugging/profiling label
//   - affinity: CpuSet - CPU core affinity hint
//   - priority: Priority (Low, Normal, High)
//
//   SPAWNED<T> MODAL TYPE:
//   States: @Pending, @Ready
//   Fields:
//     @Ready: value: T
//   Used with wait expression to retrieve result
//
//   CAPTURE RULES:
//   - const bindings: captured by reference
//   - shared bindings: captured by reference (synchronized)
//   - unique bindings: MUST use move, consumed by spawn
//
//   CONTEXT REQUIREMENTS:
//   - Must be directly inside parallel block body
//   - Cannot be nested in non-parallel control flow
//   - No spawn outside parallel context
//
// DEPENDENCIES:
//   - ParallelContext verification
//   - ExprTypeFn for body typing
//   - Spawned<T> modal type construction
//   - Option validation (CpuSet, Priority types)
//   - Capture analysis
//
// SPEC RULES:
//   - T-Spawn (line 25061)
//   - Spawn context rule (lines 24818-24819)
//
// RESULT TYPE:
//   Spawned<T> where T is the body expression type
//
// NOTES:
//   - spawn creates work item queued for execution
//   - Result retrieved via wait expression
//   - move required for unique captures
//
// =============================================================================

