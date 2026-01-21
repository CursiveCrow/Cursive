#include <cassert>
#include <optional>
#include <vector>

#include "cursive0/core/assert_spec.h"
#include "cursive0/analysis/resolve/scopes.h"
#include "cursive0/analysis/types/subtyping.h"
#include "cursive0/analysis/types/types.h"
#include "cursive0/syntax/ast.h"

namespace {

using cursive0::analysis::MakeTypeArray;
using cursive0::analysis::MakeTypeFunc;
using cursive0::analysis::MakeTypeModalState;
using cursive0::analysis::MakeTypePath;
using cursive0::analysis::MakeTypePerm;
using cursive0::analysis::MakeTypePrim;
using cursive0::analysis::MakeTypePtr;
using cursive0::analysis::MakeTypeSlice;
using cursive0::analysis::MakeTypeTuple;
using cursive0::analysis::MakeTypeUnion;
using cursive0::analysis::ParamMode;
using cursive0::analysis::PathKeyOf;
using cursive0::analysis::Permission;
using cursive0::analysis::PtrState;
using cursive0::analysis::ScopeContext;
using cursive0::analysis::Subtyping;
using cursive0::analysis::TypeFuncParam;
using cursive0::analysis::TypePath;
using cursive0::analysis::TypeRef;
using cursive0::syntax::ModalDecl;
using cursive0::syntax::StateBlock;
using cursive0::syntax::Path;

TypeRef Prim(const char* name) { return MakeTypePrim(name); }

}  // namespace

int main() {
  ScopeContext ctx;

  {
    const auto res = Subtyping(ctx, Prim("i32"), Prim("i32"));
    assert(res.ok && res.subtype);
  }
  {
    const auto res = Subtyping(ctx, Prim("i32"), Prim("i64"));
    assert(res.ok && !res.subtype);
  }
  {
    SPEC_COV("Sub-Never");
    const auto res = Subtyping(ctx, Prim("!"), Prim("bool"));
    assert(res.ok && res.subtype);
  }
  {
    SPEC_COV("Sub-Perm");
    SPEC_COV("Perm-Const");
    const auto lhs = MakeTypePerm(Permission::Const, Prim("i32"));
    const auto rhs = MakeTypePerm(Permission::Const, Prim("i32"));
    const auto res = Subtyping(ctx, lhs, rhs);
    assert(res.ok && res.subtype);
  }
  {
    SPEC_COV("Sub-Perm");
    SPEC_COV("Perm-Unique-Const");
    const auto lhs = MakeTypePerm(Permission::Unique, Prim("i32"));
    const auto rhs = MakeTypePerm(Permission::Const, Prim("i32"));
    const auto res = Subtyping(ctx, lhs, rhs);
    assert(res.ok && res.subtype);
  }
  {
    SPEC_COV("Sub-Perm");
    SPEC_COV("Perm-Unique");
    const auto lhs = MakeTypePerm(Permission::Unique, Prim("i32"));
    const auto rhs = MakeTypePerm(Permission::Unique, Prim("i32"));
    const auto res = Subtyping(ctx, lhs, rhs);
    assert(res.ok && res.subtype);
  }
  {
    SPEC_COV("Sub-Tuple");
    const auto lhs = MakeTypeTuple({Prim("!"), Prim("bool")});
    const auto rhs = MakeTypeTuple({Prim("i32"), Prim("bool")});
    const auto res = Subtyping(ctx, lhs, rhs);
    assert(res.ok && res.subtype);
  }
  {
    SPEC_COV("Sub-Array");
    const auto lhs = MakeTypeArray(Prim("!"), 4);
    const auto rhs = MakeTypeArray(Prim("i32"), 4);
    const auto res = Subtyping(ctx, lhs, rhs);
    assert(res.ok && res.subtype);
  }
  {
    SPEC_COV("Sub-Slice");
    const auto lhs = MakeTypeSlice(Prim("!"));
    const auto rhs = MakeTypeSlice(Prim("i32"));
    const auto res = Subtyping(ctx, lhs, rhs);
    assert(res.ok && res.subtype);
  }
  {
    SPEC_COV("Sub-Ptr-State");
    const auto lhs = MakeTypePtr(Prim("i32"), PtrState::Valid);
    const auto rhs = MakeTypePtr(Prim("i32"), std::nullopt);
    const auto res = Subtyping(ctx, lhs, rhs);
    assert(res.ok && res.subtype);
  }
  {
    SPEC_COV("Sub-Func");
    std::vector<TypeFuncParam> lhs_params;
    lhs_params.push_back(TypeFuncParam{ParamMode::Move, Prim("i32")});
    std::vector<TypeFuncParam> rhs_params;
    rhs_params.push_back(TypeFuncParam{ParamMode::Move, Prim("!")});
    const auto lhs = MakeTypeFunc(std::move(lhs_params), Prim("i32"));
    const auto rhs = MakeTypeFunc(std::move(rhs_params), Prim("i32"));
    const auto res = Subtyping(ctx, lhs, rhs);
    assert(res.ok && res.subtype);
  }
  {
    SPEC_COV("Sub-Member-Union");
    const auto uni = MakeTypeUnion({Prim("i32"), Prim("bool")});
    const auto res = Subtyping(ctx, Prim("bool"), uni);
    assert(res.ok && res.subtype);
  }
  {
    SPEC_COV("Sub-Union-Width");
    const auto lhs = MakeTypeUnion({Prim("i32"), Prim("bool")});
    const auto rhs = MakeTypeUnion({Prim("char"), Prim("bool"), Prim("i32")});
    const auto res = Subtyping(ctx, lhs, rhs);
    assert(res.ok && res.subtype);
  }
  {
    SPEC_COV("Modal-Incomparable");
    ModalDecl modal{};
    modal.name = "Modal";
    StateBlock state_a{};
    state_a.name = "A";
    modal.states.push_back(state_a);
    StateBlock state_b{};
    state_b.name = "B";
    modal.states.push_back(state_b);

    Path modal_path = {"Modal"};
    ctx.sigma.types[PathKeyOf(modal_path)] = modal;

    TypePath tpath = {"Modal"};
    const auto lhs = MakeTypeModalState(tpath, "A");
    const auto rhs = MakeTypeModalState(tpath, "B");
    const auto res = Subtyping(ctx, lhs, rhs);
    assert(res.ok && !res.subtype);
  }
  {
    SPEC_COV("Sub-Modal-Niche");
    ModalDecl modal{};
    modal.name = "Modal";
    StateBlock state{};
    state.name = "State";
    modal.states.push_back(state);

    Path modal_path = {"Modal"};
    ctx.sigma.types[PathKeyOf(modal_path)] = modal;

    TypePath tpath = {"Modal"};
    const auto lhs = MakeTypeModalState(tpath, "State");
    const auto rhs = MakeTypePath(tpath);
    const auto res = Subtyping(ctx, lhs, rhs);
    assert(res.ok && !res.subtype);
  }
  {
    SPEC_COV("Sub-Range");
    const auto lhs = cursive0::analysis::MakeTypeRange();
    const auto rhs = cursive0::analysis::MakeTypeRange();
    const auto res = Subtyping(ctx, lhs, rhs);
    assert(res.ok && res.subtype);
  }

  return 0;
}
