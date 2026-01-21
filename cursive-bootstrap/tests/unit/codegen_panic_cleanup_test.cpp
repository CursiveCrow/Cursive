#include <cassert>

#include "cursive0/codegen/checks.h"
#include "cursive0/codegen/cleanup.h"
#include "cursive0/codegen/lower/lower_expr.h"
#include "cursive0/core/assert_spec.h"
#include "cursive0/analysis/types/types.h"
#include "cursive0/syntax/ast.h"

namespace {

using cursive0::codegen::IRLowerPanic;
using cursive0::codegen::IRPanicCheck;
using cursive0::codegen::LowerCtx;
using cursive0::codegen::LowerPanic;
using cursive0::codegen::PanicCheck;
using cursive0::codegen::PanicReason;
using cursive0::codegen::BindValidity;
using cursive0::codegen::BindingState;
using cursive0::codegen::DropOnAssign;
using cursive0::codegen::EmitDropGlue;
using cursive0::codegen::GetBindValidity;
using cursive0::codegen::IRPlace;
using cursive0::codegen::IRStoreVar;
using cursive0::codegen::IRStoreVarNoDrop;
using cursive0::codegen::LowerWritePlace;
using cursive0::codegen::LowerWritePlaceSub;
using cursive0::analysis::MakeTypeArray;
using cursive0::analysis::MakeTypePath;
using cursive0::analysis::MakeTypeString;
using cursive0::analysis::MakeTypeTuple;
using cursive0::analysis::MakeTypePrim;
using cursive0::analysis::StringState;
using cursive0::syntax::Expr;
using cursive0::syntax::IdentifierExpr;

}  // namespace

namespace {

Expr MakeIdentExpr(const char* name) {
  Expr expr;
  IdentifierExpr ident;
  ident.name = std::string(name);
  expr.node = ident;
  return expr;
}

cursive0::codegen::IRValue MakeImmI32(std::int32_t value) {
  cursive0::codegen::IRValue v;
  v.kind = cursive0::codegen::IRValue::Kind::Immediate;
  v.bytes = {
      static_cast<std::uint8_t>(value & 0xFF),
      static_cast<std::uint8_t>((value >> 8) & 0xFF),
      static_cast<std::uint8_t>((value >> 16) & 0xFF),
      static_cast<std::uint8_t>((value >> 24) & 0xFF),
  };
  return v;
}

}  // namespace

int main() {
  SPEC_COV("LowerPanic");
  SPEC_COV("PanicCheck");

  LowerCtx ctx;
  ctx.PushScope(false, false);
  ctx.RegisterVar("s", MakeTypeString(StringState::Managed), true, false);

  auto panic_check_ir = PanicCheck(ctx);
  auto* check = std::get_if<IRPanicCheck>(&panic_check_ir->node);
  assert(check != nullptr);
  assert(check->cleanup_ir != nullptr);

  auto panic_ir = LowerPanic(PanicReason::Bounds, ctx);
  auto* panic = std::get_if<IRLowerPanic>(&panic_ir->node);
  assert(panic != nullptr);
  assert(panic->cleanup_ir != nullptr);

  {
    SPEC_COV("BindValid-Sigma");
    SPEC_COV("BindValid-Err");
    LowerCtx ctx;
    ctx.RegisterVar("present", MakeTypePrim("i32"), true, false);
    const auto validity = GetBindValidity("missing", ctx);
    assert(validity == BindValidity::Valid);
    assert(ctx.codegen_failed);
  }

  {
    SPEC_COV("UpdateValid-Err");
    LowerCtx ctx;
    ctx.MarkMoved("missing");
    assert(ctx.codegen_failed);
  }

  {
    SPEC_COV("UpdateValid-MoveRoot");
    SPEC_COV("UpdateValid-PartialMove-Init");
    SPEC_COV("UpdateValid-PartialMove-Step");
    LowerCtx ctx;
    ctx.RegisterVar("moved", MakeTypePrim("i32"), true, false);
    ctx.MarkMoved("moved");
    ctx.RegisterVar("partial", MakeTypePrim("i32"), true, false);
    ctx.MarkFieldMoved("partial", "field");
    assert(ctx.GetBindingState("moved")->is_moved);
    assert(!ctx.GetBindingState("partial")->moved_fields.empty());
  }

  {
    SPEC_COV("DropOnAssign-NotApplicable");
    LowerCtx ctx;
    ctx.RegisterVar("x", MakeTypePrim("i32"), true, false);
    DropOnAssign("x", IRPlace{}, ctx);
  }

  {
    SPEC_COV("DropOnAssign-Err");
    LowerCtx ctx;
    BindingState state;
    state.type = nullptr;
    state.has_responsibility = true;
    state.is_immovable = true;
    ctx.binding_states["bad"].push_back(state);
    DropOnAssign("bad", IRPlace{}, ctx);
    assert(ctx.codegen_failed);
  }

  {
    SPEC_COV("DropOnAssign-Record-Moved");
    LowerCtx ctx;
    auto rec = MakeTypePath({"Rec"});
    ctx.RegisterVar("rec", rec, true, true);
    ctx.MarkMoved("rec");
    DropOnAssign("rec", IRPlace{}, ctx);
  }

  {
    SPEC_COV("DropOnAssign-Record-Partial");
    LowerCtx ctx;
    auto rec = MakeTypePath({"Rec"});
    ctx.RegisterVar("rec_partial", rec, true, true);
    ctx.MarkFieldMoved("rec_partial", "field");
    DropOnAssign("rec_partial", IRPlace{}, ctx);
  }

  {
    SPEC_COV("DropOnAssign-Record-Valid");
    LowerCtx ctx;
    auto rec = MakeTypePath({"Rec"});
    ctx.RegisterVar("rec_valid", rec, true, true);
    DropOnAssign("rec_valid", IRPlace{}, ctx);
  }

  {
    SPEC_COV("DropOnAssign-Aggregate-Moved");
    LowerCtx ctx;
    auto agg = MakeTypeTuple({MakeTypePrim("i32"), MakeTypePrim("i32")});
    ctx.RegisterVar("agg", agg, true, true);
    ctx.MarkMoved("agg");
    DropOnAssign("agg", IRPlace{}, ctx);
  }

  {
    SPEC_COV("DropOnAssign-Aggregate-Ok");
    LowerCtx ctx;
    auto agg = MakeTypeArray(MakeTypePrim("u8"), 4);
    ctx.RegisterVar("agg_ok", agg, true, true);
    DropOnAssign("agg_ok", IRPlace{}, ctx);
  }

  {
    SPEC_COV("UpdateValid-StoreVar");
    SPEC_COV("UpdateValid-StoreVarNoDrop");
    LowerCtx ctx;
    ctx.RegisterVar("x", MakeTypePrim("i32"), true, false);
    Expr place = MakeIdentExpr("x");
    auto value = MakeImmI32(1);
    auto ir_store = LowerWritePlace(place, value, ctx);
    auto ir_store_nodrop = LowerWritePlaceSub(place, value, ctx);
    assert(std::get_if<IRStoreVar>(&ir_store->node) != nullptr);
    assert(std::get_if<IRStoreVarNoDrop>(&ir_store_nodrop->node) != nullptr);
  }

  {
    SPEC_COV("EmitDropGlue-Decl");
    LowerCtx ctx;
    auto ir = EmitDropGlue(MakeTypePrim("i32"), ctx);
    assert(ir != nullptr);
  }

  return 0;
}
