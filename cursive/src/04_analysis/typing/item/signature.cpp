// =============================================================================
// MIGRATION MAPPING: item/signature.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 5.3.1: Procedure Declarations - Signatures
//   - Parameter list processing
//   - Receiver shorthands (~, ~!, ~%)
//   - Return type
//   - Function type construction
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/types/type_decls.cpp
//   Procedure signature typing
//
// KEY CONTENT TO MIGRATE:
//   SIGNATURE PROCESSING:
//   1. Process receiver (if any)
//   2. Process parameter list
//   3. Lower return type
//   4. Construct function type
//
//   RECEIVER SHORTHANDS:
//   - ~ expands to (self: const Self)
//   - ~! expands to (self: unique Self)
//   - ~% expands to (self: shared Self)
//   - Must be first parameter if present
//
//   PARAMETER PROCESSING:
//   For each parameter:
//   - Extract name, type annotation, mode
//   - Lower type annotation
//   - Handle move mode for consuming params
//   - Build parameter list
//
//   PARAMETER MODES:
//   - Default: caller retains responsibility
//   - move: callee takes responsibility
//
//   FUNCTION TYPE CONSTRUCTION:
//   TypeFunc([<mode1, T1>, ..., <modeN, TN>], ReturnType)
//
//   VALIDATION:
//   - Parameter names distinct
//   - Types well-formed
//   - Receiver shorthand only first
//   - Return type well-formed
//
// DEPENDENCIES:
//   - LowerType() for type lowering
//   - ReceiverExpansion for shorthands
//   - TypeFunc construction
//   - ParameterMode handling
//
// SPEC RULES:
//   - Receiver shorthand expansion
//   - Parameter mode semantics
//   - Function type construction
//
// RESULT:
//   Function type for procedure
//   Parameter bindings for body typing
//
// =============================================================================

