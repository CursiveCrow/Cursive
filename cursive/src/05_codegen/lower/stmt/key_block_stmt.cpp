// =============================================================================
// Key Block Statement Lowering Implementation
// =============================================================================
//
// SPEC REFERENCE: CursiveSpecification.md Section 17 (Key Blocks and Key Semantics)
//
// This lowering emits explicit runtime key operations so dynamic behavior
// matches key block scope rules and integrates with cleanup for all control
// flow exits.
//
// =============================================================================

#include "05_codegen/lower/stmt/key_block_stmt.h"

#include <algorithm>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "00_core/assert_spec.h"
#include "04_analysis/typing/types.h"
#include "05_codegen/cleanup/cleanup.h"
#include "05_codegen/intrinsics/intrinsics_interface.h"
#include "05_codegen/ir/ir_model.h"
#include "05_codegen/lower/lower_expr.h"
#include "05_codegen/lower/lower_stmt.h"

namespace cursive::codegen {
namespace {

IRValue StringImmediate(std::string_view text) {
  IRValue value;
  value.kind = IRValue::Kind::Immediate;
  value.name = "\"" + std::string(text) + "\"";
  value.bytes.assign(text.begin(), text.end());
  return value;
}

IRValue U8Immediate(std::uint8_t value) {
  IRValue out;
  out.kind = IRValue::Kind::Immediate;
  out.name = std::to_string(value);
  out.bytes = {value};
  return out;
}

std::string EncodeIndexSegment(const ast::ExprPtr& /*expr*/) {
  // Keep runtime locking conservative for dynamic indices.
  return "*";
}

std::string EncodeKeyPath(const ast::KeyPathExpr& path) {
  std::string encoded = path.root;
  for (const auto& seg : path.segs) {
    encoded += ".";
    if (const auto* field = std::get_if<ast::KeySegField>(&seg)) {
      encoded += field->marked ? "bf:" : "f:";
      encoded += field->name;
      continue;
    }
    if (const auto* index = std::get_if<ast::KeySegIndex>(&seg)) {
      encoded += index->marked ? "bi:" : "i:";
      encoded += EncodeIndexSegment(index->expr);
      continue;
    }
  }
  return encoded;
}

std::vector<std::string> CanonicalizeKeyPaths(const ast::KeyBlockStmt& stmt) {
  std::vector<std::string> paths;
  paths.reserve(stmt.paths.size());
  for (const auto& path : stmt.paths) {
    paths.push_back(EncodeKeyPath(path));
  }
  std::sort(paths.begin(), paths.end());
  paths.erase(std::unique(paths.begin(), paths.end()), paths.end());
  return paths;
}

}  // namespace

IRPtr LowerKeyBlockStmt(const ast::KeyBlockStmt& stmt, LowerCtx& ctx) {
  SPEC_RULE("Lower-Stmt-KeyBlock");

  if (!stmt.body) {
    return EmptyIR();
  }

  // Track key-scope lifetime with the standard cleanup stack so key release
  // happens on normal and non-local exits (return/break/continue/panic).
  ctx.PushScope(false, false);

  const analysis::TypeRef key_scope_type = analysis::MakeTypeRawPtr(
      analysis::RawPtrQual::Mut,
      analysis::MakeTypePrim("u8"));
  const analysis::TypeRef unit_type = analysis::MakeTypePrim("()");

  std::vector<IRPtr> setup_parts;

  IRCall key_scope_enter;
  key_scope_enter.callee.kind = IRValue::Kind::Symbol;
  key_scope_enter.callee.name = ConcurrencySymKeyScopeEnter();
  key_scope_enter.result = ctx.FreshTempValue("key_scope_enter");
  ctx.RegisterValueType(key_scope_enter.result, key_scope_type);
  IRValue key_scope_value = key_scope_enter.result;
  setup_parts.push_back(MakeIR(std::move(key_scope_enter)));

  const std::string scope_local_name =
      ctx.FreshTempValue("__c0_key_scope").name;
  IRBindVar scope_bind;
  scope_bind.name = scope_local_name;
  scope_bind.value = key_scope_value;
  scope_bind.type = key_scope_type;
  setup_parts.push_back(MakeIR(std::move(scope_bind)));

  ctx.RegisterVar(scope_local_name,
                  key_scope_type,
                  /*has_responsibility=*/false,
                  /*is_immovable=*/true,
                  analysis::ProvenanceKind::Bottom);

  IRValue scope_local;
  scope_local.kind = IRValue::Kind::Local;
  scope_local.name = scope_local_name;
  ctx.RegisterValueType(scope_local, key_scope_type);

  ctx.RegisterKeyScopeExit(scope_local_name);

  const std::uint8_t key_mode =
      stmt.mode.has_value() && *stmt.mode == ast::KeyMode::Read ? 0u : 1u;
  for (const auto& encoded_path : CanonicalizeKeyPaths(stmt)) {
    IRCall acquire;
    acquire.callee.kind = IRValue::Kind::Symbol;
    acquire.callee.name = ConcurrencySymKeyAcquire();
    acquire.args.push_back(scope_local);
    acquire.args.push_back(StringImmediate(encoded_path));
    acquire.args.push_back(U8Immediate(key_mode));
    acquire.result = ctx.FreshTempValue("key_acquire");
    ctx.RegisterValueType(acquire.result, unit_type);
    setup_parts.push_back(MakeIR(std::move(acquire)));
  }

  auto body_result = LowerBlock(*stmt.body, ctx);

  CleanupPlan cleanup_plan = ComputeCleanupPlanForCurrentScope(ctx);
  CleanupPlan remainder =
      ComputeCleanupPlanRemainder(CleanupTarget::CurrentScope, ctx);
  IRPtr cleanup_ir = EmitCleanupWithRemainder(cleanup_plan, remainder, ctx);
  ctx.PopScope();

  if (ctx.temp_sink) {
    analysis::TypeRef result_type;
    if (stmt.body->tail_opt && ctx.expr_type) {
      result_type = ctx.expr_type(*stmt.body->tail_opt);
    } else if (!stmt.body->tail_opt) {
      result_type = analysis::MakeTypePrim("()");
    }
    ctx.RegisterTempValue(body_result.value, result_type);
  }

  IRBlock block_ir;
  block_ir.setup = SeqIR(std::move(setup_parts));
  block_ir.body = SeqIR({body_result.ir, cleanup_ir});
  block_ir.value = body_result.value;
  return MakeIR(std::move(block_ir));
}

}  // namespace cursive::codegen
