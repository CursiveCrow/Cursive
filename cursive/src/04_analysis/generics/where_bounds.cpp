/*
 * =============================================================================
 * MIGRATION MAPPING: where_bounds.cpp
 * =============================================================================
 *
 * SPEC REFERENCE:
 *   - C0updated.md, Section 13.3 "Type Bounds" (lines 22460-22520)
 *   - C0updated.md, Section 13.3.1 "Where Clauses" (lines 22470-22500)
 *   - C0updated.md, Section 13.3.2 "Predicate Syntax" (lines 22510-22520)
 *   - C0updated.md, Section 11 "Classes" (lines 22700-22900)
 *
 * SOURCE FILE:
 *   - cursive-bootstrap/src/03_analysis/generics/monomorphize.cpp (lines 1-337)
 *     (where clause handling integrated into monomorphization)
 *
 * FUNCTIONS TO MIGRATE:
 *   - ParseWhereClause(Where* where) -> Vec<Bound>
 *       Parse where clause into bounds
 *   - ValidateBound(Bound bound, TypeArg arg) -> bool
 *       Validate type argument satisfies bound
 *   - CheckPredicateBound(Pred pred, TypeRef ty) -> bool
 *       Check predicate bound (Bitcopy, Clone, etc.)
 *   - CheckClassBound(ClassRef class, TypeRef ty) -> bool
 *       Check class implementation bound
 *   - SubstituteWhere(Where* where, Subst subst) -> Where*
 *       Substitute types in where clause
 *   - InferBoundsFromUsage(Block* body, TypeParam* param) -> Vec<Bound>
 *       Infer required bounds from generic body
 *
 * DEPENDENCIES:
 *   - Where clause AST
 *   - Predicate checking (Bitcopy, Clone, Drop, FfiSafe)
 *   - Class implementation checking
 *   - Substitution
 *
 * REFACTORING NOTES:
 *   1. CRITICAL: Where clause syntax is Predicate(Type), NOT Type: Predicate
 *   2. Available predicates: Bitcopy, Clone, Drop, FfiSafe
 *   3. Class bounds use <: syntax: T <: Comparable
 *   4. Multiple where predicates separated by newlines
 *   5. Where clauses constrain what types can be used
 *   6. Bounds must be satisfiable for instantiation
 *
 * WHERE CLAUSE SYNTAX:
 *   procedure copy<T>(x: T) -> T
 *       where Bitcopy(T)              // CORRECT
 *   {
 *       return x
 *   }
 *
 *   // WRONG - Rust style
 *   where T: Bitcopy                  // INCORRECT
 *
 * PREDICATE BOUNDS:
 *   | Predicate | Meaning                           |
 *   |-----------|-----------------------------------|
 *   | Bitcopy   | Can be copied bitwise             |
 *   | Clone     | Has clone() method                |
 *   | Drop      | Has drop() cleanup                |
 *   | FfiSafe   | Can cross FFI boundary            |
 *
 * CLASS BOUNDS:
 *   procedure compare<T <: Comparable>(a: T, b: T) -> i32
 *   procedure compare<T>(a: T, b: T) -> i32
 *       where T <: Comparable
 *
 * MULTIPLE BOUNDS:
 *   procedure process<T>(x: T) -> T
 *       where Bitcopy(T)
 *       Clone(T)
 *       T <: Printable
 *   {
 *       // T must satisfy all three bounds
 *   }
 *
 * DIAGNOSTIC CODES:
 *   - E-GEN-0020: Unknown predicate in where clause
 *   - E-GEN-0021: Bound not satisfied
 *   - E-GEN-0022: Class not implemented
 *   - E-GEN-0023: Conflicting bounds
 *
 * =============================================================================
 */
