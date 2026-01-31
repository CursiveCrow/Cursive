// =============================================================================
// MIGRATION MAPPING: stmt/continue_stmt.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 5.2.11: Loop Control Statements
//   - T-Continue (line 9624): Continue statement typing
//   - continue_stmt grammar (line 3164)
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/types/type_infer.cpp
//   Continue statement typing portions
//
// KEY CONTENT TO MIGRATE:
//   CONTINUE STATEMENT (continue):
//   1. Verify inside loop context (L = `loop`)
//   2. Control transfers to next iteration
//   3. Statement has type ! (never)
//
//   TYPING RULE (T-Continue):
//   L = `loop`
//   --------------------------------------------------
//   Gamma |- continue : !
//
//   LOOP CONTEXT:
//   - continue only valid inside loops
//   - L tracks current context
//   - Error if L != `loop`
//
//   CONTROL FLOW:
//   - Skips remaining statements in current iteration
//   - Jumps to loop condition check (conditional loop)
//   - Jumps to next iteration (iterator loop)
//   - Jumps to top of body (infinite loop)
//
//   RESULT TYPE:
//   - continue has type ! (never)
//   - Control does not proceed past continue
//
// DEPENDENCIES:
//   - LoopContext verification
//   - Never type (!)
//
// SPEC RULES:
//   - T-Continue (line 9624)
//
// RESULT TYPE:
//   ! (never - control does not continue linearly)
//
// NOTES:
//   - continue skips to next iteration
//   - Unlike break, does not exit loop
//   - No value associated with continue
//
// =============================================================================

