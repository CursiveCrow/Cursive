// =============================================================================
// MIGRATION MAPPING: expr/yield_expr.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 17.3.3: Yield Expression (lines 25504+)
//   - T-Yield (line 25504): Yield typing rule
//   - yield expression forms
//   - yield release modifier
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/types/type_infer.cpp
//   Yield expression typing portions
//
// KEY CONTENT TO MIGRATE:
//   YIELD EXPRESSION (yield expr | yield release expr):
//   1. Verify inside async procedure
//   2. Type yielded expression
//   3. Check yielded type matches async Out parameter
//   4. If release: release held keys before suspend
//   5. Result type is In parameter (received on resume)
//
//   TYPING RULE (T-Yield):
//   AsyncSig(R) = (Out, In, Result, E)
//   Gamma |- expr : Out
//   --------------------------------------------------
//   Gamma |- yield expr : In
//
//   YIELD FORMS:
//   - yield expr: yield value and suspend
//   - yield release expr: release keys, yield, suspend
//
//   ASYNC CONTEXT:
//   - yield only valid in async procedures
//   - AsyncSig(R) extracts Out, In, Result, E from return type
//   - Yielded value must match Out type
//   - Resume value has In type
//
//   KEY RELEASE:
//   CRITICAL: yield MUST NOT occur while keys are held
//   UNLESS: yield release form is used
//   - E-CON-0213: yield inside key block without release
//   - yield release: explicitly releases keys before suspend
//
//   SUSPENSION POINT:
//   - yield is a suspension point
//   - Execution suspends until resume() called
//   - Keys released at suspend, reacquired after resume
//
// DEPENDENCIES:
//   - AsyncContext verification
//   - AsyncSig extraction from return type
//   - ExprTypeFn for yielded value typing
//   - Key state verification
//   - Subtyping for Out type check
//
// SPEC RULES:
//   - T-Yield (line 25504)
//   - E-CON-0213: yield inside key block
//
// RESULT TYPE:
//   In type from async signature
//
// NOTES:
//   - yield is the core async primitive
//   - Keys must be released before yield (unless release modifier)
//   - Bindings may be stale after yield release
//
// =============================================================================

