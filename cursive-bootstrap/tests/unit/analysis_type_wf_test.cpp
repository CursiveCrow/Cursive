#include <cassert>
#include <string>
#include <vector>

#include "cursive0/core/assert_spec.h"
#include "cursive0/analysis/types/context.h"
#include "cursive0/analysis/resolve/scopes.h"
#include "cursive0/analysis/types/type_expr.h"
#include "cursive0/analysis/types/types.h"
#include "cursive0/syntax/ast.h"

namespace {

using cursive0::analysis::MakeTypeArray;
using cursive0::analysis::MakeTypeBytes;
using cursive0::analysis::MakeTypeDynamic;
using cursive0::analysis::MakeTypeFunc;
using cursive0::analysis::MakeTypeModalState;
using cursive0::analysis::MakeTypePath;
using cursive0::analysis::MakeTypePerm;
using cursive0::analysis::MakeTypePrim;
using cursive0::analysis::MakeTypePtr;
using cursive0::analysis::MakeTypeRawPtr;
using cursive0::analysis::MakeTypeSlice;
using cursive0::analysis::MakeTypeString;
using cursive0::analysis::MakeTypeTuple;
using cursive0::analysis::MakeTypeUnion;
using cursive0::analysis::PathKeyOf;
using cursive0::analysis::Permission;
using cursive0::analysis::PtrState;
using cursive0::analysis::RawPtrQual;
using cursive0::analysis::ScopeContext;
using cursive0::analysis::TypeFuncParam;
using cursive0::analysis::TypePath;
using cursive0::analysis::TypeWF;
using cursive0::analysis::TypeWfResult;

using cursive0::syntax::ClassDecl;
using cursive0::syntax::ModalDecl;
using cursive0::syntax::RecordDecl;
using cursive0::syntax::StateBlock;
using cursive0::syntax::Visibility;

ScopeContext MakeCtx() {
  ScopeContext ctx;
  ctx.scopes = {cursive0::analysis::Scope{}, cursive0::analysis::Scope{},
                cursive0::analysis::Scope{}};
  return ctx;
}

}  // namespace

int main() {
  ScopeContext ctx = MakeCtx();

  {
    SPEC_COV("WF-Prim");
    const TypeWfResult wf = TypeWF(ctx, MakeTypePrim("i32"));
    assert(wf.ok);
  }

  {
    SPEC_COV("WF-Perm");
    const TypeWfResult wf = TypeWF(ctx, MakeTypePerm(Permission::Const,
                                                     MakeTypePrim("i8")));
    assert(wf.ok);
  }

  {
    SPEC_COV("WF-Tuple");
    const TypeWfResult wf =
        TypeWF(ctx, MakeTypeTuple({MakeTypePrim("i8"),
                                   MakeTypePrim("u8")}));
    assert(wf.ok);
  }

  {
    SPEC_COV("WF-Array");
    const TypeWfResult wf = TypeWF(ctx, MakeTypeArray(MakeTypePrim("i8"), 4));
    assert(wf.ok);
  }

  {
    SPEC_COV("WF-Slice");
    const TypeWfResult wf = TypeWF(ctx, MakeTypeSlice(MakeTypePrim("u8")));
    assert(wf.ok);
  }

  {
    SPEC_COV("WF-Union");
    const TypeWfResult wf =
        TypeWF(ctx, MakeTypeUnion({MakeTypePrim("i8"),
                                   MakeTypePrim("u8")}));
    assert(wf.ok);
  }

  {
    SPEC_COV("WF-Union-TooFew");
    const TypeWfResult wf = TypeWF(ctx, MakeTypeUnion({MakeTypePrim("i8")}));
    assert(!wf.ok);
    assert(wf.diag_id.has_value());
    assert(*wf.diag_id == "WF-Union-TooFew");
  }

  {
    SPEC_COV("WF-Func");
    std::vector<TypeFuncParam> params;
    params.push_back(TypeFuncParam{std::nullopt, MakeTypePrim("i8")});
    const TypeWfResult wf = TypeWF(ctx, MakeTypeFunc(params, MakeTypePrim("u8")));
    assert(wf.ok);
  }

  {
    SPEC_COV("WF-Path");
    RecordDecl record;
    record.vis = Visibility::Internal;
    record.name = "R";
    ctx.sigma.types.emplace(PathKeyOf({"R"}), record);
    const TypeWfResult wf = TypeWF(ctx, MakeTypePath(TypePath{"R"}));
    assert(wf.ok);
  }

  {
    const TypeWfResult wf = TypeWF(ctx, MakeTypePath(TypePath{"Self"}));
    assert(wf.ok);
  }

  {
    SPEC_COV("WF-Dynamic");
    ClassDecl cls;
    cls.vis = Visibility::Internal;
    cls.name = "C";
    ctx.sigma.classes.emplace(PathKeyOf({"C"}), cls);
    const TypeWfResult wf = TypeWF(ctx, MakeTypeDynamic(TypePath{"C"}));
    assert(wf.ok);
  }

  {
    const TypeWfResult wf = TypeWF(ctx, MakeTypeDynamic(TypePath{"FileSystem"}));
    assert(wf.ok);
  }

  {
    const TypeWfResult wf = TypeWF(ctx, MakeTypeDynamic(TypePath{"HeapAllocator"}));
    assert(wf.ok);
  }

  {
    SPEC_COV("WF-Dynamic-Err");
    const TypeWfResult wf = TypeWF(ctx, MakeTypeDynamic(TypePath{"Missing"}));
    assert(!wf.ok);
    assert(wf.diag_id.has_value());
    assert(*wf.diag_id == "Superclass-Undefined");
  }

  {
    SPEC_COV("WF-String");
    const TypeWfResult wf = TypeWF(ctx, MakeTypeString(std::nullopt));
    assert(wf.ok);
  }

  {
    SPEC_COV("WF-Bytes");
    const TypeWfResult wf = TypeWF(ctx, MakeTypeBytes(std::nullopt));
    assert(wf.ok);
  }

  {
    SPEC_COV("WF-Ptr");
    const TypeWfResult wf =
        TypeWF(ctx, MakeTypePtr(MakeTypePrim("i8"), PtrState::Valid));
    assert(wf.ok);
  }

  {
    SPEC_COV("WF-RawPtr");
    const TypeWfResult wf =
        TypeWF(ctx, MakeTypeRawPtr(RawPtrQual::Imm, MakeTypePrim("i8")));
    assert(wf.ok);
  }

  {
    SPEC_COV("WF-ModalState");
    ModalDecl modal;
    modal.vis = Visibility::Internal;
    modal.name = "M";
    StateBlock state;
    state.name = "S";
    modal.states.push_back(state);
    ctx.sigma.types.emplace(PathKeyOf({"M"}), modal);

    const TypeWfResult wf =
        TypeWF(ctx, MakeTypeModalState(TypePath{"M"}, "S"));
    assert(wf.ok);
  }

  return 0;
}
