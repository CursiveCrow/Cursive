// =============================================================================
// MIGRATION MAPPING: item/where_clause.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 9: Generics - Where Clauses
//   - Predicate bounds: Bitcopy(T), Clone(T), Drop(T), FfiSafe(T)
//   - Class constraints: T <: ClassName
//   - Where clause syntax
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/types/generics.cpp
//   Where clause processing
//
// KEY CONTENT TO MIGRATE:
//   WHERE CLAUSE PROCESSING:
//   1. Parse where clause bounds
//   2. For each bound:
//      a. Predicate bound: verify predicate valid
//      b. Class bound: verify class exists
//   3. Associate bounds with type parameters
//   4. Store for instantiation checking
//
//   WHERE CLAUSE SYNTAX:
//   procedure foo<T>(x: T) -> T
//       where Bitcopy(T)
//       Clone(T)
//   {
//       ...
//   }
//
//   PREDICATE BOUNDS:
//   - Bitcopy(T): T can be copied by memcpy
//   - Clone(T): T implements Clone (has clone method)
//   - Drop(T): T has destructor
//   - FfiSafe(T): T can cross FFI boundary
//
//   CLASS BOUNDS:
//   - T <: ClassName: T implements ClassName
//   - Multiple: T <: A, T <: B
//
//   BOUND CHECKING AT INSTANTIATION:
//   - When generic is instantiated with concrete type A
//   - Each predicate/class bound checked for A
//   - Error if bound not satisfied
//
//   BOUND IMPLICATIONS:
//   - Some predicates imply others
//   - Bitcopy implies Clone (trivial clone)
//   - Class inheritance implies superclass bounds
//
// DEPENDENCIES:
//   - PredicateCheck for predicate bounds
//   - ClassLookup for class bounds
//   - BoundCollection for storage
//   - InstantiationCheck for use-site verification
//
// SPEC RULES:
//   - Where clause syntax
//   - Predicate semantics
//   - Class constraint semantics
//
// RESULT:
//   Validated where clause bounds
//   Associated with generic parameters
//
// =============================================================================

