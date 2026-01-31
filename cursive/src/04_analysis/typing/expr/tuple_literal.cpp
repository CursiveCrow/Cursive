// =============================================================================
// MIGRATION MAPPING: expr/tuple_literal.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 5.2.5: Tuples (lines 8874-8921)
//   - T-Unit-Literal (lines 8876-8878): TupleExpr([]) : TypePrim("()")
//   - T-Tuple-Literal (lines 8880-8883):
//     n >= 1, all ei : Ti => TupleExpr([e1...en]) : TypeTuple([T1...Tn])
//   - Syn-Unit (lines 9128-9130): TupleExpr([]) => TypePrim("()")
//   - Syn-Tuple (lines 9132-9135): Tuple synthesis
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/types/type_infer.cpp
//   InferExprImpl() - TupleExpr case (lines 374-396)
//
// KEY CONTENT TO MIGRATE:
//   UNIT LITERAL:
//   - Empty tuple () has type TypePrim("()")
//   - Special case: unit is both tuple and primitive
//
//   TUPLE LITERAL:
//   - Type each element expression
//   - Result type is TypeTuple([T1, T2, ..., Tn])
//   - Single-element tuple requires (T;) syntax (handled by parser)
//
//   SYNTHESIS vs CHECKING:
//   - Synthesis: Infer each element type
//   - Checking: Check each element against expected tuple type
//
// DEPENDENCIES:
//   - ExprTypeFn for element typing
//   - MakeTypePrim("()") for unit
//   - MakeTypeTuple() for tuple construction
//
// SPEC RULES IMPLEMENTED:
//   - T-Unit-Literal: Empty tuple is unit
//   - T-Tuple-Literal: Non-empty tuple type
//   - Syn-Unit, Syn-Tuple: Synthesis rules
//
// RESULT TYPE:
//   ExprTypeResult { bool ok; optional<string_view> diag_id; TypeRef type; }
//
// =============================================================================

