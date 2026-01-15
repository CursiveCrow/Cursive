#include <cassert>

#include "cursive0/sema/types.h"

namespace {

using cursive0::sema::BytesState;
using cursive0::sema::MakeTypeArray;
using cursive0::sema::MakeTypeBytes;
using cursive0::sema::MakeTypeDynamic;
using cursive0::sema::MakeTypeFunc;
using cursive0::sema::MakeTypeModalState;
using cursive0::sema::MakeTypePath;
using cursive0::sema::MakeTypePrim;
using cursive0::sema::MakeTypePtr;
using cursive0::sema::MakeTypeRange;
using cursive0::sema::MakeTypeRawPtr;
using cursive0::sema::MakeTypeSlice;
using cursive0::sema::MakeTypeString;
using cursive0::sema::MakeTypeTuple;
using cursive0::sema::MakeTypeUnion;
using cursive0::sema::ParamMode;
using cursive0::sema::PtrState;
using cursive0::sema::RawPtrQual;
using cursive0::sema::StringState;
using cursive0::sema::TypeFuncParam;
using cursive0::sema::TypeKeyEqual;
using cursive0::sema::TypeKeyLess;
using cursive0::sema::TypeKeyOf;
using cursive0::sema::TypePath;
using cursive0::sema::TypeToString;
using cursive0::sema::TypePaths;
using cursive0::sema::TypeRef;

TypeRef Prim(const char* name) {
  return MakeTypePrim(name);
}

}  // namespace

int main() {
  {
    const auto t = Prim("i32");
    assert(TypeToString(t) == "i32");
    const auto unit = Prim("()");
    assert(TypeToString(unit) == "()");
    const auto never = Prim("!");
    assert(TypeToString(never) == "!");
  }

  {
    const auto tuple_one = MakeTypeTuple({Prim("i32")});
    assert(TypeToString(tuple_one) == "(i32;)");
    const auto tuple_two = MakeTypeTuple({Prim("i32"), Prim("bool")});
    assert(TypeToString(tuple_two) == "(i32, bool)");
  }

  {
    const auto arr = MakeTypeArray(Prim("u8"), 4);
    assert(TypeToString(arr) == "[u8; 4]");
    const auto slice = MakeTypeSlice(Prim("u8"));
    assert(TypeToString(slice) == "[u8]");
  }

  {
    const auto ptr = MakeTypePtr(Prim("i32"), PtrState::Valid);
    assert(TypeToString(ptr) == "Ptr<i32>@Valid");
    const auto raw = MakeTypeRawPtr(RawPtrQual::Imm, Prim("i32"));
    assert(TypeToString(raw) == "* imm i32");
  }

  {
    std::vector<TypeFuncParam> params;
    params.push_back(TypeFuncParam{ParamMode::Move, Prim("i32")});
    params.push_back(TypeFuncParam{std::nullopt, Prim("bool")});
    const auto func = MakeTypeFunc(std::move(params), Prim("()"));
    assert(TypeToString(func) == "(move i32, bool) -> ()");
  }

  {
    const auto view_str = MakeTypeString(StringState::View);
    assert(TypeToString(view_str) == "string@View");
    const auto bytes_any = MakeTypeBytes(std::nullopt);
    assert(TypeToString(bytes_any) == "bytes");
  }

  {
    TypePath path = {"Foo", "Bar"};
    const auto dyn = MakeTypeDynamic(path);
    assert(TypeToString(dyn) == "$Foo::Bar");
    const auto modal = MakeTypeModalState(path, "Ready");
    assert(TypeToString(modal) == "Foo::Bar@Ready");
    const auto path_ty = MakeTypePath(path);
    assert(TypeToString(path_ty) == "Foo::Bar");
  }

  {
    const auto u1 = MakeTypeUnion({Prim("i32"), Prim("bool")});
    const auto u2 = MakeTypeUnion({Prim("bool"), Prim("i32")});
    const auto k1 = TypeKeyOf(u1);
    const auto k2 = TypeKeyOf(u2);
    assert(TypeKeyEqual(k1, k2));
    assert(!TypeKeyLess(k1, k2));
    assert(!TypeKeyLess(k2, k1));
  }

  {
    const auto u1 = MakeTypeUnion({Prim("i32"), Prim("bool"), Prim("i32")});
    const auto u2 = MakeTypeUnion({Prim("i32"), Prim("bool")});
    assert(!TypeKeyEqual(TypeKeyOf(u1), TypeKeyOf(u2)));
  }

  {
    const auto t1 = MakeTypeTuple({Prim("i32"), Prim("bool")});
    const auto t2 = MakeTypeTuple({Prim("bool"), Prim("i32")});
    assert(!TypeKeyEqual(TypeKeyOf(t1), TypeKeyOf(t2)));
  }

  {
    std::vector<TypeFuncParam> params_a;
    params_a.push_back(TypeFuncParam{std::nullopt, Prim("i32")});
    const auto f1 = MakeTypeFunc(std::move(params_a), Prim("i32"));
    std::vector<TypeFuncParam> params_b;
    params_b.push_back(TypeFuncParam{ParamMode::Move, Prim("i32")});
    const auto f2 = MakeTypeFunc(std::move(params_b), Prim("i32"));
    assert(!TypeKeyEqual(TypeKeyOf(f1), TypeKeyOf(f2)));
  }

  {
    const auto p1 = MakeTypePtr(Prim("i32"), PtrState::Valid);
    const auto p2 = MakeTypePtr(Prim("i32"), PtrState::Null);
    assert(!TypeKeyEqual(TypeKeyOf(p1), TypeKeyOf(p2)));
  }

  {
    const auto s1 = MakeTypeString(StringState::View);
    const auto s2 = MakeTypeString(StringState::Managed);
    const auto s3 = MakeTypeString(std::nullopt);
    assert(!TypeKeyEqual(TypeKeyOf(s1), TypeKeyOf(s2)));
    assert(!TypeKeyEqual(TypeKeyOf(s1), TypeKeyOf(s3)));
  }

  {
    const auto r = MakeTypeRange();
    assert(TypeToString(r) == "TypeRange");
    const auto k1 = TypeKeyOf(r);
    const auto k2 = TypeKeyOf(r);
    assert(TypeKeyEqual(k1, k2));
  }

  {
    TypePath path_a = {"A"};
    TypePath path_b = {"B"};
    const auto ty = MakeTypeUnion(
        {MakeTypePath(path_b), MakeTypeDynamic(path_a),
         MakeTypeModalState(path_b, "State")});
    const auto paths = TypePaths(*ty);
    assert(paths.size() == 2);
    assert(paths[0] == path_a);
    assert(paths[1] == path_b);
  }

  return 0;
}
