// =============================================================================
// Defer Statement Lowering Implementation
// =============================================================================
//
// SPEC REFERENCE: CursiveSpecification.md Lines 16655-16657 (Lower-Stmt-Defer)
//   - DeferIR(block) - deferred block stored for cleanup
//
// MIGRATED FROM:
//   - cursive-bootstrap/src/04_codegen/lower/lower_stmt.cpp
//   - Lines 695-701: DeferStmt case in LowerStmt dispatch
//
// =============================================================================

#include "05_codegen/lower/stmt/defer_stmt.h"

#include "00_core/assert_spec.h"
#include "05_codegen/ir/ir_model.h"
#include "05_codegen/lower/lower_expr.h"
#include "05_codegen/lower/lower_stmt.h"

namespace cursive::codegen {

// ============================================================================
// Lower-Stmt-Defer
// ============================================================================
//
// Per the spec (Lines 16655-16657):
//   DeferIR(block) - stores the deferred block for execution at scope exit
//
// The implementation:
//   - Creates an IRDefer node containing the block
//   - Registers the defer with the context for cleanup tracking
//   - Defers are executed in reverse order at scope exit
//
IRPtr LowerDeferStmt(const ast::DeferStmt& stmt, LowerCtx& ctx) {
  SPEC_RULE("Lower-Stmt-Defer");

  // Lower the deferred block now and register its IR for execution during
  // cleanup at scope exit.
  LowerResult deferred_body = LowerBlock(*stmt.body, ctx);
  ctx.RegisterDefer(deferred_body.ir);

  // Keep a DeferIR marker in the stream for IR-model fidelity. Runtime
  // behavior is driven by cleanup expansion, not immediate execution here.
  IRDefer defer;
  defer.block = stmt.body;
  IRPtr defer_ir = MakeIR(std::move(defer));
  return defer_ir;
}

}  // namespace cursive::codegen
