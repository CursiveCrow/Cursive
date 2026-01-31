// =============================================================================
// MIGRATION MAPPING: stmt/break_stmt.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 5.2.11: Loop Control Statements
//   - T-Break-Value (line 9609): Break with value
//   - T-Break-Unit (line 9614): Break without value
//   - break_stmt grammar (line 3163)
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/types/type_infer.cpp
//   Break statement typing portions
//
// KEY CONTENT TO MIGRATE:
//   BREAK STATEMENT (break | break expr):
//   1. Verify inside loop context (L = `loop`)
//   2. If break has value:
//      a. Type the value expression
//      b. Add type to loop's Brk set
//   3. If break has no value:
//      a. Set BrkVoid = true for loop
//   4. Control transfers to after loop
//
//   TYPING RULE (T-Break-Value):
//   L = `loop`
//   Gamma; R; L |- expr : T
//   --------------------------------------------------
//   Gamma |- break expr : !
//   Brk += T
//
//   TYPING RULE (T-Break-Unit):
//   L = `loop`
//   --------------------------------------------------
//   Gamma |- break : !
//   BrkVoid = true
//
//   LOOP CONTEXT:
//   - break only valid inside loops
//   - L tracks current loop/non-loop context
//   - Error if L != `loop`
//
//   BREAK VALUE:
//   - break 42 - loop result is 42
//   - break - loop result is ()
//   - Multiple breaks unify via Brk set
//
//   RESULT TYPE:
//   - break itself has type ! (never)
//   - Loop result determined by LoopTypeInf/LoopTypeFin
//
// DEPENDENCIES:
//   - LoopContext verification
//   - ExprTypeFn for value typing (if present)
//   - Brk set accumulation
//   - BrkVoid flag
//   - Never type (!)
//
// SPEC RULES:
//   - T-Break-Value (line 9609)
//   - T-Break-Unit (line 9614)
//
// RESULT TYPE:
//   ! (never - control does not continue)
//
// NOTES:
//   - break transfers control out of loop
//   - Value becomes loop result
//   - Multiple breaks' types unified
//
// =============================================================================

