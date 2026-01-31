// =============================================================================
// MIGRATION MAPPING: expr/call_type_args.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 9.4: Type Inference and Instantiation (line 22436+)
//   - CallTypeArgs elaboration to monomorphic Call
//   - Explicit type argument syntax: f<T, U>(args)
//   - Type argument inference when omitted
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/types/type_infer.cpp
//   Generic call typing with explicit type arguments
//
// KEY CONTENT TO MIGRATE:
//   CALL WITH TYPE ARGUMENTS (callee<T1, T2>(args)):
//   1. Resolve callee to generic procedure
//   2. Extract generic parameters from procedure signature
//   3. Process explicit type arguments
//   4. Check type argument count matches parameter count
//   5. Verify type arguments satisfy bounds (where clauses)
//   6. Substitute type arguments into signature
//   7. Type arguments against instantiated parameter types
//   8. Result is instantiated return type
//
//   TYPING:
//   Gamma |- callee : forall <T_1, ..., T_n>. (P_1, ..., P_m) -> R
//   Type arguments: [A_1, ..., A_n]
//   All A_i satisfy bounds for T_i
//   Instantiate: (P_1', ..., P_m') -> R' = [A_i/T_i](signature)
//   ArgsOk(args, [P_1', ..., P_m'])
//   --------------------------------------------------
//   Gamma |- callee<A_1, ..., A_n>(args) : R'
//
//   TYPE ARGUMENT SYNTAX:
//   - Generic params use semicolons: <T; U> in declarations
//   - Generic args use commas: <T, U> in calls
//
//   TYPE INFERENCE (when args omitted):
//   - Infer from argument types (line 22440)
//   - Infer from expected return type (checking mode)
//   - Combine constraints via unification
//   - Error if cannot infer all type arguments
//
//   BOUND CHECKING:
//   - Class bounds: T <: ClassName
//   - Predicate bounds: Bitcopy(T), Clone(T), Drop(T), FfiSafe(T)
//   - Where clause predicates
//
// DEPENDENCIES:
//   - GenericResolution for callee lookup
//   - TypeBoundCheck for bound verification
//   - TypeSubstitution for instantiation
//   - ArgsOk for argument typing
//   - ExprTypeFn for argument expressions
//
// SPEC RULES:
//   - CallTypeArgs elaboration (line 22436)
//   - Type inference sources (line 22440)
//
// RESULT TYPE:
//   Instantiated return type R'
//
// NOTES:
//   - Elaborated to monomorphic Call for later phases
//   - Type arguments stored for codegen
//   - Inference may fail (T-*-Infer-Err rules)
//
// =============================================================================

