// =============================================================================
// MIGRATION MAPPING: item/type_alias_decl.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 5.3.6: Type Alias Declarations
//   - Type alias grammar
//   - Generic type aliases
//   - Type refinements (where { predicate })
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/types/type_decls.cpp
//   Type alias declaration typing
//
// KEY CONTENT TO MIGRATE:
//   TYPE ALIAS DECLARATION:
//   1. Process generic parameters (if any)
//   2. Process where clauses
//   3. Lower the aliased type
//   4. Process type refinement (if any)
//   5. Register alias in environment
//
//   TYPING:
//   Generic params well-formed
//   Gamma |- aliased_type wf
//   Refinement predicate valid (if present)
//   --------------------------------------------------
//   Gamma |- type Name<params> = aliased_type where {...}
//
//   TYPE ALIAS FORMS:
//   - Simple: type Result = i32 | Error
//   - Generic: type Pair<T; U> = (T, U)
//   - Refined: type PositiveInt = i32 where { self > 0 }
//
//   TYPE REFINEMENTS:
//   - where { predicate } constrains the type
//   - predicate must be pure expression
//   - self references the value being refined
//   - Checked at construction/assignment
//
//   GENERIC ALIASES:
//   - Parameters use semicolons: <T; U>
//   - May have bounds: <T <: Comparable>
//   - May have where clauses
//
//   EXPANSION:
//   - Type aliases expand at use sites
//   - Aliases are transparent to type system
//   - Refinements preserved through alias
//
// DEPENDENCIES:
//   - ProcessGenericParams()
//   - LowerType() for aliased type
//   - RefinementTyping() for where clause
//   - TypeWellFormed() for validation
//
// SPEC RULES:
//   - Type alias well-formedness
//   - Refinement predicate typing
//
// RESULT:
//   Type alias added to environment
//   Diagnostics for any errors
//
// =============================================================================

