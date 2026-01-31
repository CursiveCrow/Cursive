#include "cursive0/03_analysis/memory/string_bytes.h"

#include <array>
#include <utility>
#include <vector>

#include "cursive0/00_core/assert_spec.h"
#include "cursive0/03_analysis/resolve/scopes.h"

namespace cursive0::analysis {

namespace {

static inline void SpecDefsStringBytes() {
  SPEC_DEF("StringBytesBuiltinTable", "5.8");
  SPEC_DEF("StringBytesBuiltinSig", "5.8");
  SPEC_DEF("StringBytesJudg", "5.8");
}

static inline void TouchStringBytesRules() {
  SPEC_RULE("StringFrom-Ok");
  SPEC_RULE("StringFrom-Err");
  SPEC_RULE("StringAsView-Ok");
  SPEC_RULE("StringToManaged-Ok");
  SPEC_RULE("StringToManaged-Err");
  SPEC_RULE("StringCloneWith-Ok");
  SPEC_RULE("StringCloneWith-Err");
  SPEC_RULE("StringAppend-Ok");
  SPEC_RULE("StringAppend-Err");
  SPEC_RULE("StringLength");
  SPEC_RULE("StringIsEmpty");
  SPEC_RULE("BytesWithCapacity-Ok");
  SPEC_RULE("BytesWithCapacity-Err");
  SPEC_RULE("BytesFromSlice-Ok");
  SPEC_RULE("BytesFromSlice-Err");
  SPEC_RULE("BytesAsView-Ok");
  SPEC_RULE("BytesToManaged-Ok");
  SPEC_RULE("BytesToManaged-Err");
  SPEC_RULE("BytesView-Ok");
  SPEC_RULE("BytesViewString-Ok");
  SPEC_RULE("BytesAsSlice-Ok");
  SPEC_RULE("BytesAppend-Ok");
  SPEC_RULE("BytesAppend-Err");
  SPEC_RULE("BytesLength");
  SPEC_RULE("BytesIsEmpty");
}

static bool IsStringPath(const syntax::ModulePath& path) {
  return path.size() == 1 && IdEq(path[0], "string");
}

static bool IsBytesPath(const syntax::ModulePath& path) {
  return path.size() == 1 && IdEq(path[0], "bytes");
}

static TypeRef AllocErrorType() {
  return MakeTypePath({"AllocationError"});
}

static TypeRef HeapAllocatorType() {
  return MakeTypeDynamic({"HeapAllocator"});
}

static TypeRef StringViewType() {
  return MakeTypeString(StringState::View);
}

static TypeRef StringManagedType() {
  return MakeTypeString(StringState::Managed);
}

static TypeRef BytesViewType() {
  return MakeTypeBytes(BytesState::View);
}

static TypeRef BytesManagedType() {
  return MakeTypeBytes(BytesState::Managed);
}

static TypeRef ConstType(const TypeRef& base) {
  return MakeTypePerm(Permission::Const, base);
}

static TypeRef UniqueType(const TypeRef& base) {
  return MakeTypePerm(Permission::Unique, base);
}

static TypeRef SliceU8Type() {
  return MakeTypeSlice(MakeTypePrim("u8"));
}

static TypeRef Union2(TypeRef a, TypeRef b) {
  std::vector<TypeRef> members;
  members.reserve(2);
  members.push_back(std::move(a));
  members.push_back(std::move(b));
  return MakeTypeUnion(std::move(members));
}

static TypeRef MakeFunc(std::vector<TypeFuncParam> params, TypeRef ret) {
  return MakeTypeFunc(std::move(params), std::move(ret));
}

static std::optional<TypeRef> StringBuiltinType(std::string_view name) {
  if (IdEq(name, "from")) {
    std::vector<TypeFuncParam> params = {
        {std::nullopt, StringViewType()},
        {std::nullopt, HeapAllocatorType()},
    };
    return MakeFunc(std::move(params),
                    Union2(StringManagedType(), AllocErrorType()));
  }
  if (IdEq(name, "as_view")) {
    std::vector<TypeFuncParam> params = {
        {std::nullopt, ConstType(StringManagedType())},
    };
    return MakeFunc(std::move(params), StringViewType());
  }
  if (IdEq(name, "to_managed")) {
    std::vector<TypeFuncParam> params = {
        {std::nullopt, ConstType(StringViewType())},
        {std::nullopt, HeapAllocatorType()},
    };
    return MakeFunc(std::move(params),
                    Union2(StringManagedType(), AllocErrorType()));
  }
  if (IdEq(name, "clone_with")) {
    std::vector<TypeFuncParam> params = {
        {std::nullopt, ConstType(StringManagedType())},
        {std::nullopt, HeapAllocatorType()},
    };
    return MakeFunc(std::move(params),
                    Union2(StringManagedType(), AllocErrorType()));
  }
  if (IdEq(name, "append")) {
    std::vector<TypeFuncParam> params = {
        {std::nullopt, UniqueType(StringManagedType())},
        {std::nullopt, StringViewType()},
        {std::nullopt, HeapAllocatorType()},
    };
    return MakeFunc(std::move(params),
                    Union2(MakeTypePrim("()"), AllocErrorType()));
  }
  if (IdEq(name, "length")) {
    std::vector<TypeFuncParam> params = {
        {std::nullopt, ConstType(StringViewType())},
    };
    return MakeFunc(std::move(params), MakeTypePrim("usize"));
  }
  if (IdEq(name, "is_empty")) {
    std::vector<TypeFuncParam> params = {
        {std::nullopt, ConstType(StringViewType())},
    };
    return MakeFunc(std::move(params), MakeTypePrim("bool"));
  }
  return std::nullopt;
}

static std::optional<TypeRef> BytesBuiltinType(std::string_view name) {
  if (IdEq(name, "with_capacity")) {
    std::vector<TypeFuncParam> params = {
        {std::nullopt, MakeTypePrim("usize")},
        {std::nullopt, HeapAllocatorType()},
    };
    return MakeFunc(std::move(params),
                    Union2(BytesManagedType(), AllocErrorType()));
  }
  if (IdEq(name, "from_slice")) {
    std::vector<TypeFuncParam> params = {
        {std::nullopt, ConstType(SliceU8Type())},
        {std::nullopt, HeapAllocatorType()},
    };
    return MakeFunc(std::move(params),
                    Union2(BytesManagedType(), AllocErrorType()));
  }
  if (IdEq(name, "as_view")) {
    std::vector<TypeFuncParam> params = {
        {std::nullopt, ConstType(BytesManagedType())},
    };
    return MakeFunc(std::move(params), BytesViewType());
  }
  if (IdEq(name, "to_managed")) {
    std::vector<TypeFuncParam> params = {
        {std::nullopt, ConstType(BytesViewType())},
        {std::nullopt, HeapAllocatorType()},
    };
    return MakeFunc(std::move(params),
                    Union2(BytesManagedType(), AllocErrorType()));
  }
  if (IdEq(name, "view")) {
    std::vector<TypeFuncParam> params = {
        {std::nullopt, ConstType(SliceU8Type())},
    };
    return MakeFunc(std::move(params), BytesViewType());
  }
  if (IdEq(name, "view_string")) {
    std::vector<TypeFuncParam> params = {
        {std::nullopt, StringViewType()},
    };
    return MakeFunc(std::move(params), BytesViewType());
  }
  if (IdEq(name, "as_slice")) {
    std::vector<TypeFuncParam> params = {
        {std::nullopt, ConstType(BytesViewType())},
    };
    return MakeFunc(std::move(params), ConstType(SliceU8Type()));
  }
  if (IdEq(name, "append")) {
    std::vector<TypeFuncParam> params = {
        {std::nullopt, UniqueType(BytesManagedType())},
        {std::nullopt, BytesViewType()},
        {std::nullopt, HeapAllocatorType()},
    };
    return MakeFunc(std::move(params),
                    Union2(MakeTypePrim("()"), AllocErrorType()));
  }
  if (IdEq(name, "length")) {
    std::vector<TypeFuncParam> params = {
        {std::nullopt, ConstType(BytesViewType())},
    };
    return MakeFunc(std::move(params), MakeTypePrim("usize"));
  }
  if (IdEq(name, "is_empty")) {
    std::vector<TypeFuncParam> params = {
        {std::nullopt, ConstType(BytesViewType())},
    };
    return MakeFunc(std::move(params), MakeTypePrim("bool"));
  }
  return std::nullopt;
}

}  // namespace

bool IsStringBytesBuiltinPath(const syntax::ModulePath& path) {
  SpecDefsStringBytes();
  return IsStringPath(path) || IsBytesPath(path);
}

bool IsStringBuiltinName(std::string_view name) {
  SpecDefsStringBytes();
  static constexpr std::array<std::string_view, 7> names = {
      "from", "as_view", "to_managed", "clone_with",
      "append", "length", "is_empty",
  };
  for (const auto& entry : names) {
    if (IdEq(name, entry)) {
      return true;
    }
  }
  return false;
}

bool IsBytesBuiltinName(std::string_view name) {
  SpecDefsStringBytes();
  static constexpr std::array<std::string_view, 10> names = {
      "with_capacity", "from_slice", "as_view", "to_managed",
      "view", "view_string", "as_slice", "append", "length", "is_empty",
  };
  for (const auto& entry : names) {
    if (IdEq(name, entry)) {
      return true;
    }
  }
  return false;
}

std::optional<TypeRef> LookupStringBytesBuiltinType(
    const syntax::ModulePath& path,
    std::string_view name) {
  SpecDefsStringBytes();
  if (!IsStringBytesBuiltinPath(path)) {
    return std::nullopt;
  }
  TouchStringBytesRules();
  if (IsStringPath(path)) {
    return StringBuiltinType(name);
  }
  if (IsBytesPath(path)) {
    return BytesBuiltinType(name);
  }
  return std::nullopt;
}

}  // namespace cursive0::analysis
