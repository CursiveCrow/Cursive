#include <cassert>
#include <string>
#include <vector>

#include "cursive0/core/assert_spec.h"
#include "cursive0/sema/context.h"
#include "cursive0/sema/scopes.h"
#include "cursive0/sema/type_expr.h"
#include "cursive0/sema/types.h"
#include "cursive0/syntax/ast.h"

namespace {

using cursive0::sema::MakeTypeArray;
using cursive0::sema::MakeTypeBytes;
using cursive0::sema::MakeTypeDynamic;
using cursive0::sema::MakeTypeFunc;
using cursive0::sema::MakeTypeModalState;
using cursive0::sema::MakeTypePath;
using cursive0::sema::MakeTypePerm;
using cursive0::sema::MakeTypePrim;
using cursive0::sema::MakeTypePtr;
using cursive0::sema::MakeTypeRawPtr;
using cursive0::sema::MakeTypeSlice;
using cursive0::sema::MakeTypeString;
using cursive0::sema::MakeTypeTuple;
using cursive0::sema::MakeTypeUnion;
using cursive0::sema::PathKeyOf;
using cursive0::sema::Permission;
using cursive0::sema::PtrState;
using cursive0::sema::RawPtrQual;
using cursive0::sema::ScopeContext;
using cursive0::sema::TypeFuncParam;
using cursive0::sema::TypePath;
using cursive0::sema::TypeWF;
using cursive0::sema::TypeWfResult;

using cursive0::syntax::ClassDecl;
using cursive0::syntax::ModalDecl;
using cursive0::syntax::RecordDecl;
using cursive0::syntax::StateBlock;
using cursive0::syntax::Visibility;

ScopeContext MakeCtx() {
  ScopeContext ctx;
  ctx.scopes = {cursive0::sema::Scope{}, cursive0::sema::Scope{},
                cursive0::sema::Scope{}};
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
