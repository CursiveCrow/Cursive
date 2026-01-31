/*
 * =============================================================================
 * MIGRATION MAPPING: generic_params.cpp
 * =============================================================================
 *
 * SPEC REFERENCE:
 *   - C0updated.md, Section 13 "Generics" (lines 22300-22600)
 *   - C0updated.md, Section 13.1 "Generic Parameters" (line 22328)
 *   - C0updated.md, Section 13.2 "Generic Arguments" (lines 22380-22450)
 *   - C0updated.md, Section 13.3 "Type Bounds" (lines 22460-22520)
 *
 * SOURCE FILE:
 *   - cursive-bootstrap/src/03_analysis/generics/monomorphize.cpp (lines 1-337)
 *
 * FUNCTIONS TO MIGRATE:
 *   - ValidateGenericParams(GenericParams* params) -> bool         [lines 30-80]
 *       Validate generic parameter declarations
 *   - CheckParamUniqueness(GenericParams* params) -> bool          [lines 85-110]
 *       Ensure no duplicate parameter names
 *   - ParseTypeParam(TypeParam* param) -> TypeParamInfo            [lines 115-160]
 *       Parse and validate type parameter
 *   - ParseConstParam(ConstParam* param) -> ConstParamInfo         [lines 165-210]
 *       Parse and validate const generic parameter
 *   - ValidateDefaultValues(GenericParams* params) -> bool         [lines 215-280]
 *       Validate default type argument values
 *   - BuildParamScope(GenericParams* params) -> Scope              [lines 285-337]
 *       Create scope for generic parameters
 *
 * DEPENDENCIES:
 *   - GenericParams, TypeParam, ConstParam AST nodes
 *   - Scope management
 *   - Type constraint system
 *
 * REFACTORING NOTES:
 *   1. CRITICAL: Generic params use SEMICOLONS: <T; U>
 *   2. CRITICAL: Generic args use COMMAS: <T, U>
 *   3. Type parameters can have bounds: <T <: Comparable>
 *   4. Type parameters can have defaults: <T; Alloc = DefaultAlloc>
 *   5. Const parameters allowed: <const N: usize>
 *   6. Parameters are in scope for bounds and defaults
 *
 * GENERIC PARAMETER SYNTAX:
 *   <T>                      // Single type param
 *   <T; U>                   // Multiple type params (SEMICOLONS)
 *   <T <: Comparable>        // Type param with bound
 *   <T; Alloc = Default>     // Type param with default
 *   <const N: usize>         // Const generic param
 *
 * GENERIC ARGUMENT SYNTAX:
 *   foo<i32>()               // Single type arg
 *   foo<i32, bool>()         // Multiple type args (COMMAS)
 *   Pair<i32, bool>          // Type instantiation
 *
 * DIAGNOSTIC CODES:
 *   - E-GEN-0001: Duplicate type parameter name
 *   - E-GEN-0002: Invalid type parameter bound
 *   - E-GEN-0003: Default after non-default parameter
 *   - E-GEN-0004: Invalid const generic type
 *
 * =============================================================================
 */
