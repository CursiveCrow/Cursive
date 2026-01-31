/*
 * =============================================================================
 * MIGRATION MAPPING: async.cpp
 * =============================================================================
 *
 * SPEC REFERENCE:
 *   - C0updated.md, Section 15 "Asynchronous Operations" (lines 23800-24000)
 *   - C0updated.md, Section 15.1 "Async Types" (lines 23810-23820)
 *   - C0updated.md, Section 15.7 "Async Combinators" (lines 24140-24200)
 *   - C0updated.md, Section 5.4.8 "Built-in Modals - Async" (lines 13010-13100)
 *
 * SOURCE FILE:
 *   - cursive-bootstrap/src/03_analysis/caps/cap_concurrency.cpp (lines 1-717)
 *
 * FUNCTIONS TO MIGRATE:
 *   - IsAsyncType(TypeRef ty) -> bool
 *       Check if type is Async or alias
 *   - GetAsyncParams(TypeRef ty) -> (Out, In, Result, E)
 *       Extract type parameters from Async type
 *   - ValidateAsyncProcedure(ProcDecl* proc) -> bool
 *       Validate procedure returning async type
 *   - CheckAsyncReturnType(ProcDecl* proc) -> bool
 *       Ensure async procedures return Async<...>
 *   - ValidateAsyncCombinator(MethodCall* call) -> bool
 *       Validate async combinator methods (map, filter, etc.)
 *   - InferAsyncBodyType(Block* body, AsyncType* ty) -> bool
 *       Check body is consistent with async type params
 *
 * DEPENDENCIES:
 *   - Async<Out, In, Result, E> modal type
 *   - Type aliases (Future, Sequence, Stream, etc.)
 *   - Combinator method signatures
 *
 * REFACTORING NOTES:
 *   1. Async<Out, In, Result, E> is the core async modal type
 *   2. States: @Suspended, @Completed, @Failed
 *   3. Type aliases simplify common patterns
 *   4. Procedure is async if return type is Async or alias
 *   5. Async procedures can use yield, return, handle errors
 *   6. Combinators transform async values
 *
 * ASYNC TYPE:
 *   Async<Out, In, Result, E>
 *   - Out: Type of yielded values
 *   - In: Type of resume input
 *   - Result: Type of final result
 *   - E: Error type (! for infallible)
 *
 * TYPE ALIASES:
 *   type Sequence<T> = Async<T, (), (), !>
 *   type Future<T; E = !> = Async<(), (), T, E>
 *   type Pipe<In; Out> = Async<Out, In, (), !>
 *   type Exchange<T> = Async<T, T, T, !>
 *   type Stream<T; E> = Async<T, (), (), E>
 *
 * ASYNC STATES:
 *   @Suspended:
 *     - output: Out (yielded value)
 *     - resume(~!, input: In) -> Async@Suspended | @Completed | @Failed
 *   @Completed:
 *     - value: Result
 *   @Failed:
 *     - error: E
 *
 * COMBINATORS:
 *   - map(f): Transform yielded values
 *   - filter(p): Filter yielded values
 *   - take(n): Take first n values
 *   - fold(init, f): Fold to single result
 *   - chain(f): Chain futures
 *
 * DIAGNOSTIC CODES:
 *   - E-CON-0090: Invalid async type parameters
 *   - E-CON-0091: Yield in non-async procedure
 *   - E-CON-0092: Return type mismatch in async
 *   - E-CON-0093: Invalid combinator usage
 *
 * =============================================================================
 */
