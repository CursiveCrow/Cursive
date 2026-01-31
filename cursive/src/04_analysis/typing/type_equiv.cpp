// =============================================================================
// MIGRATION MAPPING: type_equiv.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 5.2.1: Type Equivalence (lines 8514-8592)
//   - T-Equiv-Prim (lines 8514-8517): Primitive type equivalence
//   - T-Equiv-Perm (lines 8519-8522): Permission type equivalence
//   - T-Equiv-Tuple (lines 8524-8527): Tuple type equivalence
//   - T-Equiv-Array (lines 8529-8532): Array type equivalence
//   - T-Equiv-Slice (lines 8534-8537): Slice type equivalence
//   - T-Equiv-Func (lines 8539-8542): Function type equivalence
//   - T-Equiv-Closure (lines 8544-8547): Closure type equivalence
//   - T-Equiv-Union (lines 8549-8552): Union type equivalence (unordered)
//   - T-Equiv-Path (lines 8554-8557): Named type equivalence
//   - T-Equiv-ModalState (lines 8559-8562): Modal state equivalence
//   - T-Equiv-String (lines 8564-8567): String type equivalence
//   - T-Equiv-Bytes (lines 8569-8572): Bytes type equivalence
//   - T-Equiv-Range (lines 8574-8577): Range type equivalence
//   - T-Equiv-Ptr (lines 8579-8582): Safe pointer equivalence
//   - T-Equiv-RawPtr (lines 8584-8587): Raw pointer equivalence
//   - T-Equiv-Dynamic (lines 8589-8592): Dynamic class equivalence
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/types/type_equiv.cpp
//   Lines 1-602: Full implementation
//   Key functions:
//   - TypeEquiv(Env, Type, Type) -> bool (main entry)
//   - TypeEquivImpl(Env, Type, Type) -> bool (recursive comparison)
//   - MembersEq() for union member comparison
//   - TypeKey computation for union normalization
//
// KEY CONTENT TO MIGRATE:
//   TYPE EQUIVALENCE JUDGMENT (Gamma |- T equiv U):
//   1. Primitive types: same name implies equivalence
//   2. Permission types: same permission and equivalent inner type
//   3. Tuple types: same arity, pairwise equivalent elements
//   4. Array types: same length (via ConstLen), equivalent element type
//   5. Slice types: equivalent element type
//   6. Function types: same modes, pairwise equivalent params, equiv return
//   7. Union types: UNORDERED - MembersEq checks permutation equivalence
//   8. Named types (TypePath): same path implies equivalence
//   9. Modal state types: same modal ref and state name
//   10. String/Bytes: same storage type (View/Managed)
//   11. Ptr types: same state, equivalent pointee
//   12. RawPtr types: same qualifier (imm/mut), equivalent pointee
//   13. Dynamic types: same class path
//
//   UNION NORMALIZATION:
//   - Unions are unordered: i32 | bool equiv bool | i32
//   - MembersEq computes via permutation check
//   - TypeKey provides deterministic ordering for comparisons
//
//   CONSTLEN EVALUATION:
//   - Array lengths compared via constant expression evaluation
//   - See const_len.cpp for ConstLen implementation
//
// DEPENDENCIES:
//   - ConstLen() for array length evaluation
//   - TypeKey for union normalization
//   - Type representation (TypePrim, TypeTuple, TypeArray, etc.)
//   - Env for type parameter lookups
//
// SPEC RULES:
//   - T-Equiv-*: All type equivalence rules
//   - MembersEq predicate for unions (line 8512)
//
// RESULT:
//   bool indicating whether types are equivalent
//
// NOTES:
//   - Type equivalence is reflexive, symmetric, transitive
//   - No subtyping between numeric types (i32 is NOT equiv i64)
//   - Union equivalence requires exact membership (after normalization)
//
// =============================================================================

