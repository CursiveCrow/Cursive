#include <cassert>
#include <optional>
#include <vector>

#include "cursive0/sema/scopes.h"
#include "cursive0/sema/subtyping.h"
#include "cursive0/sema/types.h"
#include "cursive0/syntax/ast.h"

namespace {

using cursive0::sema::MakeTypeArray;
using cursive0::sema::MakeTypeFunc;
using cursive0::sema::MakeTypeModalState;
using cursive0::sema::MakeTypePath;
using cursive0::sema::MakeTypePerm;
using cursive0::sema::MakeTypePrim;
using cursive0::sema::MakeTypePtr;
using cursive0::sema::MakeTypeSlice;
using cursive0::sema::MakeTypeTuple;
using cursive0::sema::MakeTypeUnion;
using cursive0::sema::ParamMode;
using cursive0::sema::PathKeyOf;
using cursive0::sema::Permission;
using cursive0::sema::PtrState;
using cursive0::sema::ScopeContext;
using cursive0::sema::Subtyping;
using cursive0::sema::TypeFuncParam;
using cursive0::sema::TypePath;
using cursive0::sema::TypeRef;
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
    const auto res = Subtyping(ctx, Prim("!"), Prim("bool"));
    assert(res.ok && res.subtype);
  }
  {
    const auto lhs = MakeTypePerm(Permission::Unique, Prim("i32"));
    const auto rhs = MakeTypePerm(Permission::Const, Prim("i32"));
    const auto res = Subtyping(ctx, lhs, rhs);
    assert(res.ok && res.subtype);
  }
  {
    const auto lhs = MakeTypeTuple({Prim("!"), Prim("bool")});
    const auto rhs = MakeTypeTuple({Prim("i32"), Prim("bool")});
    const auto res = Subtyping(ctx, lhs, rhs);
    assert(res.ok && res.subtype);
  }
  {
    const auto lhs = MakeTypeArray(Prim("!"), 4);
    const auto rhs = MakeTypeArray(Prim("i32"), 4);
    const auto res = Subtyping(ctx, lhs, rhs);
    assert(res.ok && res.subtype);
  }
  {
    const auto lhs = MakeTypeSlice(Prim("!"));
    const auto rhs = MakeTypeSlice(Prim("i32"));
    const auto res = Subtyping(ctx, lhs, rhs);
    assert(res.ok && res.subtype);
  }
  {
    const auto lhs = MakeTypePtr(Prim("i32"), PtrState::Valid);
    const auto rhs = MakeTypePtr(Prim("i32"), std::nullopt);
    const auto res = Subtyping(ctx, lhs, rhs);
    assert(res.ok && res.subtype);
  }
  {
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
    const auto uni = MakeTypeUnion({Prim("i32"), Prim("bool")});
    const auto res = Subtyping(ctx, Prim("bool"), uni);
    assert(res.ok && res.subtype);
  }
  {
    const auto lhs = MakeTypeUnion({Prim("i32"), Prim("bool")});
    const auto rhs = MakeTypeUnion({Prim("char"), Prim("bool"), Prim("i32")});
    const auto res = Subtyping(ctx, lhs, rhs);
    assert(res.ok && res.subtype);
  }
  {
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

  return 0;
}
