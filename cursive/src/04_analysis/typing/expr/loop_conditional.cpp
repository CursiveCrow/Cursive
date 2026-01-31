// =============================================================================
// MIGRATION MAPPING: expr/loop_conditional.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 5.2.11: Loop Expressions
//   - T-Loop-Conditional (lines 9705-9708): Conditional loop typing
//   - loop_expression grammar (line 23432)
//   - Loop invariant (line 23427)
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/types/type_infer.cpp
//   Loop expression typing portions
//
// KEY CONTENT TO MIGRATE:
//   CONDITIONAL LOOP (loop condition { body }):
//   1. Type condition expression
//   2. Check condition has type bool
//   3. Type body in loop context (L = `loop`)
//   4. Extract BlockInfo: (T_body, Brk, BrkVoid)
//   5. Compute result type via LoopTypeFin(Brk, BrkVoid)
//
//   TYPING RULE (T-Loop-Conditional):
//   Gamma; R; L |- cond : bool
//   Gamma; R; `loop` |- BlockInfo(body) => (T_b, Brk, BrkVoid)
//   LoopTypeFin(Brk, BrkVoid) = T
//   --------------------------------------------------
//   Gamma |- LoopConditional(cond, body) : T
//
//   LOOP TYPE COMPUTATION:
//   LoopTypeFin(Brk, BrkVoid):
//   - If Brk is empty and BrkVoid: type is ()
//   - If Brk has single type T: type is T | ()
//   - If Brk has multiple types: union of all | ()
//
//   LOOP INVARIANT:
//   - Optional where { predicate } clause
//   - Checked at loop entry and each iteration
//   - See spec line 23427
//
// DEPENDENCIES:
//   - ExprTypeFn for condition typing
//   - BlockTypeFn for body typing
//   - LoopTypeFin helper for result type
//   - BlockInfo extraction
//
// SPEC RULES:
//   - T-Loop-Conditional (lines 9705-9708)
//   - LoopTypeFin predicate
//
// RESULT TYPE:
//   Type computed via LoopTypeFin (body type or () if exits normally)
//
// =============================================================================

