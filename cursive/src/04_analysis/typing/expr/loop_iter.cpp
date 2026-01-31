// =============================================================================
// MIGRATION MAPPING: expr/loop_iter.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 5.2.11: Loop Expressions
//   - T-Loop-Iter (lines 9710-9713): Iterator loop typing
//   - T-Loop-Iter-Async (lines 9715-9718): Async iterator loop
//   - Loop-Async-Err (lines 9720-9723): Async loop error
//   - Iterator class (line 23148)
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/types/type_infer.cpp
//   Iterator loop typing portions
//
// KEY CONTENT TO MIGRATE:
//   ITERATOR LOOP (loop pat in iter { body }):
//   1. Type iterator expression
//   2. Check iter is iterable (Slice, Array, or Iterator)
//   3. Determine element type T
//   4. If type annotation: check T <: annotation
//   5. Check pattern against element type
//   6. Verify pattern names are distinct
//   7. Push scope, introduce pattern bindings
//   8. Type body in loop context
//   9. Compute result type via LoopTypeFin
//
//   TYPING RULE (T-Loop-Iter):
//   iter : TypeSlice(T) | TypeArray(T, n) | TypePerm(p, ...)
//   (ty_opt = bottom => T_p = T)
//   (ty_opt = T_a => T <: T_a, T_p = T_a)
//   pat <= T_p => B
//   Distinct(PatNames(pat))
//   Gamma_1 = IntroAll(PushScope(Gamma), B)
//   BlockInfo(body) in Gamma_1 => (T_b, Brk, BrkVoid)
//   LoopTypeFin(Brk, BrkVoid) = T_r
//   --------------------------------------------------
//   Gamma |- LoopIter(pat, ty_opt, iter, body) : T_r
//
//   ASYNC ITERATOR (T-Loop-Iter-Async):
//   - iter must be Async type with In = ()
//   - Only allowed in async procedure (AsyncSig(R) defined)
//   - Yields Out type as element
//   - Error E must be subtype of enclosing error type
//
//   ITERABLE TYPES:
//   - Slices: [T]
//   - Arrays: [T; n]
//   - Permission-wrapped: const [T], unique [T]
//   - Async types: Async<T, (), R, E>
//
// DEPENDENCIES:
//   - ExprTypeFn for iterator typing
//   - PatternTypeFn for pattern checking
//   - BlockTypeFn for body typing
//   - IntroAll for binding introduction
//   - AsyncSig extraction
//   - LoopTypeFin helper
//
// SPEC RULES:
//   - T-Loop-Iter (lines 9710-9713)
//   - T-Loop-Iter-Async (lines 9715-9718)
//   - Loop-Async-Err (E-CON-0240)
//
// RESULT TYPE:
//   Type computed via LoopTypeFin
//
// NOTES:
//   - Async iteration requires async context
//   - Pattern bindings scoped to loop body
//
// =============================================================================

