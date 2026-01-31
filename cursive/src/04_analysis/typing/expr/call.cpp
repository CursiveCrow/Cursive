// =============================================================================
// MIGRATION MAPPING: expr/call.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 5.2.4: Procedure Calls (lines 8707-8777)
//   - T-Call (lines 8733-8736): Call typing rule
//   - Syn-Call (lines 9137-9140): Call synthesis
//   - Syn-Call-Err (lines 9142-9145): Call error propagation
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/types/type_infer.cpp
//   - InferExprImpl() - CallExpr case (lines 398-442)
//   - TypeCall() from memory/calls.cpp
//   - TypeRecordDefaultCall() from composite/records.cpp
//
// KEY CONTENT TO MIGRATE:
//   CALL TYPING FLOW:
//   1. Check for special cases first:
//      - RegionOptions() default construction
//      - Record default construction (no args)
//   2. Type the callee expression
//   3. Verify callee is callable (TypeFunc or TypeClosure)
//   4. Check argument count matches parameter count
//   5. Check each argument via ArgsOk_T judgment
//   6. Return the function's return type
//
//   SPECIAL CASES:
//   - Generic calls: Handle type argument substitution
//   - Record default: RecordCallee() check, then default construction
//   - Extern calls: Require unsafe context
//
//   CALLEE TYPES:
//   - TypeFunc: Regular function
//   - TypeClosure: Closure with captures (C0X extension)
//   - Procedure reference: Resolve to TypeFunc
//
// DEPENDENCIES:
//   - ExprTypeFn for callee and argument typing
//   - PlaceTypeFn for reference argument typing
//   - ArgsOk() from args_ok.cpp
//   - TypeRecordDefaultCall() for record defaults
//   - TypeCall() for general calls
//   - Monomorphize() for generic instantiation
//   - IsInUnsafeSpan() for extern call check
//
// SPEC RULES IMPLEMENTED:
//   - T-Call: Main call typing rule
//   - Syn-Call, Syn-Call-Err: Synthesis rules
//   - T-Record-Default: Default record construction
//   - Call-Extern-Unsafe-Err: Extern requires unsafe
//   - Call-Callee-NotFunc: Not callable error
//   - Call-ArgCount-Err, Call-ArgType-Err: Argument errors
//
// RESULT TYPE:
//   ExprTypeResult { bool ok; optional<string_view> diag_id; TypeRef type; }
//
// ERROR CASES:
//   - Callee is not callable
//   - Argument count mismatch
//   - Argument type mismatch
//   - Move/reference mode mismatch
//   - Extern call outside unsafe
//   - Generic argument errors
//
// =============================================================================

