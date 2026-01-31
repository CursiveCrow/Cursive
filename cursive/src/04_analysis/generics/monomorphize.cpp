/*
 * =============================================================================
 * MIGRATION MAPPING: monomorphize.cpp
 * =============================================================================
 *
 * SPEC REFERENCE:
 *   - C0updated.md, Section 13.4 "Monomorphization" (line 22529)
 *   - C0updated.md, Section 13.4.1 "Type Instantiation" (lines 22540-22600)
 *   - C0updated.md, Section 13.4.2 "Substitution" (lines 22610-22700)
 *   - C0updated.md, Section 8.14 "E-GEN Errors" (lines 22300-22400)
 *
 * SOURCE FILE:
 *   - cursive-bootstrap/src/03_analysis/generics/monomorphize.cpp (lines 1-337)
 *
 * FUNCTIONS TO MIGRATE:
 *   - InstantiateType(TypeRef generic, TypeArgs args) -> TypeRef   [lines 50-120]
 *       Instantiate generic type with concrete arguments
 *   - BuildSubstitution(GenericParams* params, TypeArgs args) -> Subst [lines 125-180]
 *       Build substitution map from params to args
 *   - InferTypeArguments(Call* call, GenericParams* params) -> TypeArgs [lines 185-260]
 *       Infer type arguments from call site
 *   - SubstituteType(TypeRef ty, Subst subst) -> TypeRef           [lines 265-310]
 *       Apply substitution to type
 *   - ValidateTypeArguments(TypeArgs args, GenericParams* params) -> bool [lines 315-337]
 *       Validate arguments satisfy bounds
 *
 * DEPENDENCIES:
 *   - TypeRef for type manipulation
 *   - GenericParams, TypeArgs
 *   - Substitution map
 *   - Type inference
 *
 * REFACTORING NOTES:
 *   1. Monomorphization creates specialized copies for each type arg combination
 *   2. Type arguments can be inferred from usage context
 *   3. Substitution replaces type params with concrete types
 *   4. All bounds must be satisfied after substitution
 *   5. Const generics substituted as compile-time values
 *   6. Consider caching monomorphized instances
 *
 * MONOMORPHIZATION PROCESS:
 *   1. Collect type arguments (explicit or inferred)
 *   2. Build substitution: param -> arg
 *   3. Substitute all type params in:
 *      - Field types
 *      - Method signatures
 *      - Where clause bounds
 *   4. Validate bounds are satisfied
 *   5. Generate specialized code
 *
 * TYPE INFERENCE:
 *   procedure identity<T>(x: T) -> T
 *   identity(42)              // T inferred as i32
 *   identity<bool>(true)      // T explicit as bool
 *
 * SUBSTITUTION:
 *   record Pair<T; U> { first: T, second: U }
 *   Pair<i32, bool>
 *   => record Pair_i32_bool { first: i32, second: bool }
 *
 * DIAGNOSTIC CODES:
 *   - E-GEN-0010: Could not infer type argument
 *   - E-GEN-0011: Type argument violates bound
 *   - E-GEN-0012: Wrong number of type arguments
 *   - E-GEN-0013: Const generic value out of range
 *
 * =============================================================================
 */
