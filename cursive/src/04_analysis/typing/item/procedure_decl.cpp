// =============================================================================
// MIGRATION MAPPING: item/procedure_decl.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 5.3.1: Procedure Declarations
//   Section 5.2.3: Function Types (T-Proc-As-Value)
//   Section 5.8: Contracts
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/types/type_decls.cpp
//   Procedure declaration typing
//
// KEY CONTENT TO MIGRATE:
//   PROCEDURE DECLARATION:
//   1. Process generic parameters (if any)
//   2. Process where clauses (type bounds)
//   3. Process parameters:
//      - Lower each parameter type
//      - Check parameter modes (move or ref)
//      - Check receiver shorthand (~, ~!, ~%)
//   4. Lower return type
//   5. Process contracts (preconditions/postconditions)
//   6. Type procedure body in parameter scope
//   7. Check body returns correct type
//
//   RECEIVER SHORTHANDS:
//   - ~ expands to (self: const Self)
//   - ~! expands to (self: unique Self)
//   - ~% expands to (self: shared Self)
//   - Must be first parameter if present
//
//   CONTRACT PROCESSING:
//   - Preconditions: |= P
//   - Postconditions: |= => Q (uses @result)
//   - Both: |= P => Q
//   - Verification mode: [[static]] or [[dynamic]]
//
//   GENERIC PROCEDURES:
//   - Type parameters in scope for body
//   - Where clauses constrain parameters
//   - Instantiation checked at call sites
//
// DEPENDENCIES:
//   - LowerType() for type lowering
//   - ProcessGenericParams()
//   - ProcessWhereClause()
//   - ContractTyping() for contracts
//   - Block typing for body
//   - IntroAll() for parameter bindings
//
// SPEC RULES:
//   - T-Proc-Decl: Procedure declaration well-formed
//   - T-Proc-As-Value: Procedure yields function type
//   - WF-Contract: Contract well-formed
//
// RESULT:
//   Procedure declaration added to environment
//   Diagnostics for any errors
//
// =============================================================================

