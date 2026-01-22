#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include <variant>

namespace cursive0::analysis {

enum class Permission {
  Const,
  Unique,
  Shared,
};

bool IsPermInC0(Permission perm);

std::optional<Permission> ParsePermissionKeyword(std::string_view token);

enum class ParamMode {
  Move,
};

enum class RawPtrQual {
  Imm,
  Mut,
};

enum class StringState {
  Managed,
  View,
};

enum class BytesState {
  Managed,
  View,
};

enum class PtrState {
  Valid,
  Null,
  Expired,
};

using TypePath = std::vector<std::string>;

struct Type;
using TypeRef = std::shared_ptr<Type>;

struct TypePrim {
  std::string name;
};

struct TypeRange {};

struct TypePerm {
  Permission perm;
  TypeRef base;
};

struct TypeUnion {
  std::vector<TypeRef> members;
};

struct TypeFuncParam {
  std::optional<ParamMode> mode;
  TypeRef type;
};

struct TypeFunc {
  std::vector<TypeFuncParam> params;
  TypeRef ret;
};

struct TypeTuple {
  std::vector<TypeRef> elements;
};

struct TypeArray {
  TypeRef element;
  std::uint64_t length = 0;
};

struct TypeSlice {
  TypeRef element;
};

struct TypePtr {
  TypeRef element;
  std::optional<PtrState> state;
};

struct TypeRawPtr {
  RawPtrQual qual;
  TypeRef element;
};

struct TypeString {
  std::optional<StringState> state;
};

struct TypeBytes {
  std::optional<BytesState> state;
};

struct TypeDynamic {
  TypePath path;
};

struct TypeModalState {
  TypePath path;
  std::string state;
};

struct TypePathType {
  TypePath path;
  std::vector<TypeRef> generic_args;  // C0X Extension: Foo<T, U>
};

using TypeNode = std::variant<TypePrim,
                              TypePerm,
                              TypeUnion,
                              TypeFunc,
                              TypeTuple,
                              TypeArray,
                              TypeSlice,
                              TypePtr,
                              TypeRawPtr,
                              TypeString,
                              TypeBytes,
                              TypeDynamic,
                              TypeModalState,
                              TypePathType,
                              TypeRange>;

struct Type {
  TypeNode node;
};

TypeRef MakeType(TypeNode node);
TypeRef MakeTypePrim(std::string name);
TypeRef MakeTypeRange();
TypeRef MakeTypePerm(Permission perm, TypeRef base);
TypeRef MakeTypeUnion(std::vector<TypeRef> members);
TypeRef MakeTypeFunc(std::vector<TypeFuncParam> params, TypeRef ret);
TypeRef MakeTypeTuple(std::vector<TypeRef> elements);
TypeRef MakeTypeArray(TypeRef element, std::uint64_t length);
TypeRef MakeTypeSlice(TypeRef element);
TypeRef MakeTypePtr(TypeRef element, std::optional<PtrState> state);
TypeRef MakeTypeRawPtr(RawPtrQual qual, TypeRef element);
TypeRef MakeTypeString(std::optional<StringState> state);
TypeRef MakeTypeBytes(std::optional<BytesState> state);
TypeRef MakeTypeDynamic(TypePath path);
TypeRef MakeTypeModalState(TypePath path, std::string state);
TypeRef MakeTypePath(TypePath path);

std::string TypeToString(const Type& type);
std::string TypeToString(const TypeRef& type);

struct TypeKey;

struct KeyAtom {
  enum class Kind {
    Number,
    String,
    Key,
    KeyList,
  };

  Kind kind = Kind::Number;
  std::uint64_t number = 0;
  std::string text;
  std::shared_ptr<TypeKey> key;
  std::vector<std::shared_ptr<TypeKey>> key_list;

  static KeyAtom Number(std::uint64_t value);
  static KeyAtom String(std::string value);
  static KeyAtom Key(std::shared_ptr<TypeKey> value);
  static KeyAtom KeyList(std::vector<std::shared_ptr<TypeKey>> value);
};

struct TypeKey {
  std::vector<KeyAtom> atoms;
};

TypeKey TypeKeyOf(const Type& type);
TypeKey TypeKeyOf(const TypeRef& type);
bool TypeKeyEqual(const TypeKey& lhs, const TypeKey& rhs);
bool TypeKeyLess(const TypeKey& lhs, const TypeKey& rhs);

std::vector<TypeRef> SortUnionMembers(const std::vector<TypeRef>& members);

std::vector<TypePath> TypePaths(const Type& type);
std::vector<TypePath> TypePaths(const TypeRef& type);

}  // namespace cursive0::analysis
