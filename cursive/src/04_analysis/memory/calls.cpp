/*
 * =============================================================================
 * MIGRATION MAPPING: calls.cpp
 * =============================================================================
 *
 * SPEC REFERENCE:
 *   - C0updated.md, Section 6.8 "Procedure Calls" (lines 16500-16700)
 *   - C0updated.md, Section 6.8.1 "Argument Passing" (lines 16510-16600)
 *   - C0updated.md, Section 6.8.2 "Move Parameters" (lines 16610-16700)
 *   - C0updated.md, Section 10.3 "Move Semantics" (lines 22310-22400)
 *
 * SOURCE FILE:
 *   - cursive-bootstrap/src/03_analysis/memory/calls.cpp (if exists)
 *   - Related content may be in borrow_bind.cpp
 *
 * FUNCTIONS TO MIGRATE:
 *   - ValidateCallArguments(Call* call) -> bool
 *       Validate argument types and permissions match parameters
 *   - CheckMoveArgument(Arg* arg, Param* param) -> bool
 *       Validate move argument transfers ownership
 *   - AnalyzeCallOwnership(Call* call) -> OwnershipEffect
 *       Analyze ownership effects of a procedure call
 *   - ValidateReceiverPermission(MethodCall* call) -> bool
 *       Validate receiver permission matches method requirement
 *   - CheckArgumentEvalOrder(Call* call) -> EvalOrder
 *       Determine argument evaluation order
 *   - PropagateCallEffects(Call* call) -> void
 *       Propagate ownership/state effects from call
 *
 * DEPENDENCIES:
 *   - Call, MethodCall AST nodes
 *   - Parameter, Argument matching
 *   - Permission system
 *   - Move semantics
 *
 * REFACTORING NOTES:
 *   1. Non-move parameters: caller retains ownership
 *   2. Move parameters: callee takes ownership (move keyword required)
 *   3. Receiver shorthands: ~ (const), ~! (unique), ~% (shared)
 *   4. Argument evaluation order is left-to-right
 *   5. Permission compatibility: unique can satisfy const/shared, shared can satisfy const
 *   6. After move argument, binding is Moved state
 *   7. Consider tracking which arguments were moved for error recovery
 *
 * CALL SEMANTICS:
 *   - procedure foo(x: T) -> ()           // x borrowed, caller keeps
 *   - procedure bar(move x: T) -> ()      // x consumed, caller loses
 *   - procedure baz(~) -> ()              // receiver borrowed (const)
 *   - procedure qux(~!) -> ()             // receiver borrowed (unique)
 *   - procedure quux(~%) -> ()            // receiver borrowed (shared)
 *
 * DIAGNOSTIC CODES:
 *   - E-CALL-0001: Argument type mismatch
 *   - E-CALL-0002: Permission insufficient
 *   - E-CALL-0003: Move required for consuming parameter
 *   - E-CALL-0004: Receiver permission mismatch
 *   - E-CALL-0005: Cannot move borrowed value
 *
 * =============================================================================
 */
