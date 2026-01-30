#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include <variant>

#include "cursive0/core/span.h"

namespace cursive0::syntax {
struct Type;
struct Expr;
}

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
  std::vector<TypeRef> generic_args;
};

struct TypePathType {
  TypePath path;
  std::vector<TypeRef> generic_args;  // C0X Extension: Foo<T, U>
};

struct TypeOpaque {
  TypePath class_path;
  const syntax::Type* origin = nullptr;
  core::Span origin_span;
};

struct TypeRefine {
  TypeRef base;
  std::shared_ptr<syntax::Expr> predicate;
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
                              TypeOpaque,
                              TypeRefine,
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
TypeRef MakeTypeModalState(TypePath path,
                           std::string state,
                           std::vector<TypeRef> generic_args = {});
TypeRef MakeTypePath(TypePath path);
TypeRef MakeTypePath(TypePath path, std::vector<TypeRef> generic_args);
TypeRef MakeTypeOpaque(TypePath class_path,
                       const syntax::Type* origin,
                       const core::Span& origin_span);
TypeRef MakeTypeRefine(TypeRef base,
                       std::shared_ptr<syntax::Expr> predicate);

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

// C0X Extension: Async type helpers (§19.1)

/// Returns true if the type is an async type (Async<...> or a type alias thereof).
/// This checks for TypePathType or TypeModalState with path ["Async"], or
/// type aliases Future, Sequence, Stream, Pipe, Exchange.
bool IsAsyncType(const TypeRef& type);

/// Helper to compare identifiers for equality (case-sensitive).
bool IdEq(const std::string& a, const std::string& b);

/// Async type signature components.
/// For Async<Out, In, Result, E>, holds the extracted type arguments.
struct AsyncSig {
  TypeRef out;     // Yielded output type
  TypeRef in;      // Resume input type
  TypeRef result;  // Completion result type
  TypeRef err;     // Error type (! for infallible)
};

/// Extracts the async signature from an async type.
/// Returns nullopt if the type is not an async type.
/// Handles Async<Out, In, Result, E> and aliases:
///   Future<T; E = !> = Async<(), (), T, E>
///   Sequence<T> = Async<T, (), (), !>
///   Stream<T; E> = Async<T, (), (), E>
///   Pipe<In; Out> = Async<Out, In, (), !>
///   Exchange<T> = Async<T, T, T, !>
std::optional<AsyncSig> GetAsyncSig(const TypeRef& type);

}  // namespace cursive0::analysis
