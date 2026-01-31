// =============================================================================
// MIGRATION MAPPING: item/enum_decl.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 5.3.3: Enum Declarations
//   - WF-EnumDecl (line 10540): Enum well-formedness
//   - enum_decl grammar (line 3102)
//   - Explicit discriminants (line 13948)
//   - Type invariants on enums (line 23390)
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/types/type_decls.cpp
//   Enum declaration typing
//
// KEY CONTENT TO MIGRATE:
//   ENUM DECLARATION:
//   1. Process generic parameters (if any)
//   2. Process class implementations (<:)
//   3. Process where clauses
//   4. Process variants:
//      a. Check variant names are distinct
//      b. For each variant: process payload types
//      c. For discriminants: evaluate constant expressions
//   5. Process type invariant (if any)
//   6. Register enum in environment
//
//   TYPING RULE (WF-EnumDecl):
//   Distinct(VariantNames(variants))
//   All payload types well-formed
//   Discriminants are constant i32/i64
//   --------------------------------------------------
//   Gamma |- enum Name<params> <: classes where {...} { variants }
//
//   VARIANT FORMS:
//   - Unit variant: Name
//   - Tuple variant: Name(T1, T2, ...)
//   - Discriminant: Name = const_expr
//
//   DISCRIMINANT RULES (line 13948):
//   - Must be constant integer expression
//   - Type: i32 or i64
//   - Must be unique within enum
//   - Auto-increment from last explicit value
//
//   TYPE INVARIANT:
//   - Optional where { predicate } clause
//   - Applied to all instances of enum
//   - Checked at construction
//
//   CLASS IMPLEMENTATIONS:
//   - Enum may implement classes (<: Eq, Comparable)
//   - Must provide required methods
//   - Methods via impl-style or inline
//
// DEPENDENCIES:
//   - ProcessGenericParams()
//   - LowerType() for payload types
//   - ConstEval() for discriminants
//   - ProcessClassImpl() for class checking
//   - TypeInvariantCheck()
//
// SPEC RULES:
//   - WF-EnumDecl (line 10540)
//   - Discriminant rules (line 13948)
//
// RESULT:
//   Enum declaration added to environment
//   Diagnostics for any errors
//
// =============================================================================

