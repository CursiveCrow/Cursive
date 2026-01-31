// =============================================================================
// MIGRATION MAPPING: expr/loop_infinite.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 5.2.11: Loop Expressions
//   - T-Loop-Infinite (lines 9700-9703): Infinite loop typing
//   - loop_expression grammar (line 23432)
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/types/type_infer.cpp
//   Loop expression typing portions
//
// KEY CONTENT TO MIGRATE:
//   INFINITE LOOP (loop { body }):
//   1. Type body in loop context (L = `loop`)
//   2. Extract BlockInfo: (T_body, Brk, BrkVoid)
//   3. Compute result type via LoopTypeInf(Brk, BrkVoid)
//
//   TYPING RULE (T-Loop-Infinite):
//   Gamma; R; `loop` |- BlockInfo(body) => (T_b, Brk, BrkVoid)
//   LoopTypeInf(Brk, BrkVoid) = T
//   --------------------------------------------------
//   Gamma |- LoopInfinite(body) : T
//
//   LOOP TYPE COMPUTATION:
//   LoopTypeInf(Brk, BrkVoid):
//   - If Brk is empty: type is ! (never - infinite loop)
//   - If Brk has single type T: type is T
//   - If Brk has multiple types: union of all types
//   - BrkVoid affects whether () is included
//
//   NEVER TYPE (!):
//   - Infinite loop with no break has type !
//   - Subtype of all types
//   - Enables type unification in branches
//
// DEPENDENCIES:
//   - BlockTypeFn for body typing
//   - LoopTypeInf helper for result type
//   - BlockInfo extraction
//   - Never type (!) handling
//
// SPEC RULES:
//   - T-Loop-Infinite (lines 9700-9703)
//   - LoopTypeInf predicate
//
// RESULT TYPE:
//   Type computed via LoopTypeInf (! if no breaks, otherwise break type)
//
// =============================================================================

