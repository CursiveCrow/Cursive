#include "cursive0/sema/types.h"

#include <algorithm>
#include <type_traits>
#include <utility>

#include "cursive0/core/assert_spec.h"
#include "cursive0/project/deterministic_order.h"

namespace cursive0::sema {

namespace {

static inline void SpecDefsPermissionSets() {
  SPEC_DEF("PermSet_C0", "1.1.1");
  SPEC_DEF("S_Perms", "1.1.1");
}

static inline void SpecDefsTypeRepr() {
  SPEC_DEF("Type", "3.3.2.3");
  SPEC_DEF("TypeCtor", "1.1.1");
  SPEC_DEF("TypeConstructs", "1.1.1");
  SPEC_DEF("TypeRangeSyntax", "3.3.2.3");
  SPEC_DEF("ParamMode", "3.3.2.3");
  SPEC_DEF("Perm", "3.3.2.3");
  SPEC_DEF("Qual", "3.3.2.3");
  SPEC_DEF("PtrStateOpt", "3.3.2.3");
  SPEC_DEF("StringStateOpt", "3.3.2.3");
  SPEC_DEF("BytesStateOpt", "3.3.2.3");
}

static inline void SpecDefsTypePaths() {
  SPEC_DEF("TypePaths", "3.3.2.2");
}

static inline void SpecDefsTypeKey() {
  SPEC_DEF("PathOrderKey", "6.1.4.1");
  SPEC_DEF("ArrayLen", "6.1.4.1");
  SPEC_DEF("TagKey", "6.1.4.1");
  SPEC_DEF("PermKey", "6.1.4.1");
  SPEC_DEF("PtrStateKey", "6.1.4.1");
  SPEC_DEF("QualKey", "6.1.4.1");
  SPEC_DEF("ModeKey", "6.1.4.1");
  SPEC_DEF("StateKey", "6.1.4.1");
  SPEC_DEF("TypeKey", "6.1.4.1");
  SPEC_DEF("Key", "6.1.4.1");
  SPEC_DEF("KeyList", "6.1.4.1");
  SPEC_DEF("Sort", "6.1.4.1");
  SPEC_DEF("Sorted", "6.1.4.1");
  SPEC_DEF("LexLess", "6.1.4.1");
}

static std::string PathToString(const TypePath& path) {
  if (path.empty()) {
    return "";
  }
  std::string out = path[0];
  for (std::size_t i = 1; i < path.size(); ++i) {
    out.append("::");
    out.append(path[i]);
  }
  return out;
}

static std::string FoldPath(const TypePath& path) {
  return project::Fold(PathToString(path));
}

static bool PathLess(const TypePath& lhs, const TypePath& rhs) {
  const std::string lhs_fold = FoldPath(lhs);
  const std::string rhs_fold = FoldPath(rhs);
  if (project::Utf8LexLess(lhs_fold, rhs_fold)) {
    return true;
  }
  if (lhs_fold == rhs_fold) {
    return project::Utf8LexLess(PathToString(lhs), PathToString(rhs));
  }
  return false;
}

static TypeKey PathOrderKey(const TypePath& path) {
  TypeKey key;
  key.atoms.push_back(KeyAtom::String(FoldPath(path)));
  key.atoms.push_back(KeyAtom::String(PathToString(path)));
  return key;
}

static std::uint64_t TagKeyOf(const TypeNode& node) {
  return std::visit(
      [](const auto& value) -> std::uint64_t {
        using T = std::decay_t<decltype(value)>;
        if constexpr (std::is_same_v<T, TypePrim>) {
          return 0;
        } else if constexpr (std::is_same_v<T, TypeTuple>) {
          return 1;
        } else if constexpr (std::is_same_v<T, TypeArray>) {
          return 2;
        } else if constexpr (std::is_same_v<T, TypeSlice>) {
          return 3;
        } else if constexpr (std::is_same_v<T, TypeFunc>) {
          return 4;
        } else if constexpr (std::is_same_v<T, TypePathType>) {
          return 5;
        } else if constexpr (std::is_same_v<T, TypeModalState>) {
          return 6;
        } else if constexpr (std::is_same_v<T, TypeString>) {
          return 7;
        } else if constexpr (std::is_same_v<T, TypeBytes>) {
          return 8;
        } else if constexpr (std::is_same_v<T, TypeDynamic>) {
          return 9;
        } else if constexpr (std::is_same_v<T, TypePtr>) {
          return 10;
        } else if constexpr (std::is_same_v<T, TypeRawPtr>) {
          return 11;
        } else if constexpr (std::is_same_v<T, TypeUnion>) {
          return 12;
        } else if constexpr (std::is_same_v<T, TypePerm>) {
          return 13;
        } else if constexpr (std::is_same_v<T, TypeRange>) {
          return 14;
        } else {
          return 0;
        }
      },
      node);
}

static std::uint64_t PermKeyOf(Permission perm) {
  if (perm == Permission::Const) {
    return 0;
  }
  if (perm == Permission::Unique) {
    return 1;
  }
  SPEC_RULE("Perm-Shared-Unsupported");
  return 2;
}

static std::uint64_t PtrStateKeyOf(const std::optional<PtrState>& state) {
  if (!state.has_value()) {
    return 0;
  }
  switch (*state) {
    case PtrState::Valid:
      return 1;
    case PtrState::Null:
      return 2;
    case PtrState::Expired:
      return 3;
  }
  return 0;
}

static std::uint64_t QualKeyOf(RawPtrQual qual) {
  return qual == RawPtrQual::Imm ? 0 : 1;
}

static std::uint64_t ModeKeyOf(const std::optional<ParamMode>& mode) {
  if (!mode.has_value()) {
    return 0;
  }
  return 1;
}

template <typename State>
static std::uint64_t StateKeyOf(const std::optional<State>& state) {
  if (!state.has_value()) {
    return 2;
  }
  switch (*state) {
    case State::View:
      return 0;
    case State::Managed:
      return 1;
  }
  return 2;
}

static bool TypeKeyLessImpl(const TypeKey& lhs, const TypeKey& rhs);

static bool KeyAtomEqual(const KeyAtom& lhs, const KeyAtom& rhs) {
  if (lhs.kind != rhs.kind) {
    return false;
  }
  switch (lhs.kind) {
    case KeyAtom::Kind::Number:
      return lhs.number == rhs.number;
    case KeyAtom::Kind::String:
      return lhs.text == rhs.text;
    case KeyAtom::Kind::Key:
      if (!lhs.key || !rhs.key) {
        return lhs.key == rhs.key;
      }
      return !TypeKeyLessImpl(*lhs.key, *rhs.key) &&
             !TypeKeyLessImpl(*rhs.key, *lhs.key);
    case KeyAtom::Kind::KeyList:
      if (lhs.key_list.size() != rhs.key_list.size()) {
        return false;
      }
      for (std::size_t i = 0; i < lhs.key_list.size(); ++i) {
        const auto& l_key = lhs.key_list[i];
        const auto& r_key = rhs.key_list[i];
        if (l_key && r_key) {
          if (TypeKeyLessImpl(*l_key, *r_key) ||
              TypeKeyLessImpl(*r_key, *l_key)) {
            return false;
          }
        } else if (l_key || r_key) {
          return false;
        }
      }
      return true;
  }
  return false;
}

static bool KeyListLess(const std::vector<std::shared_ptr<TypeKey>>& lhs,
                        const std::vector<std::shared_ptr<TypeKey>>& rhs) {
  const std::size_t count = std::min(lhs.size(), rhs.size());
  for (std::size_t i = 0; i < count; ++i) {
    const auto& l_key = lhs[i];
    const auto& r_key = rhs[i];
    if (l_key && r_key) {
      if (TypeKeyLessImpl(*l_key, *r_key)) {
        return true;
      }
      if (TypeKeyLessImpl(*r_key, *l_key)) {
        return false;
      }
    } else if (!l_key && r_key) {
      return true;
    } else if (l_key && !r_key) {
      return false;
    }
  }
  return lhs.size() < rhs.size();
}

static bool KeyAtomLess(const KeyAtom& lhs, const KeyAtom& rhs) {
  if (lhs.kind != rhs.kind) {
    return static_cast<int>(lhs.kind) < static_cast<int>(rhs.kind);
  }
  switch (lhs.kind) {
    case KeyAtom::Kind::Number:
      return lhs.number < rhs.number;
    case KeyAtom::Kind::String:
      return project::Utf8LexLess(lhs.text, rhs.text);
    case KeyAtom::Kind::Key:
      if (!lhs.key || !rhs.key) {
        return !lhs.key && rhs.key;
      }
      return TypeKeyLessImpl(*lhs.key, *rhs.key);
    case KeyAtom::Kind::KeyList:
      return KeyListLess(lhs.key_list, rhs.key_list);
  }
  return false;
}

static bool TypeKeyLessImpl(const TypeKey& lhs, const TypeKey& rhs) {
  const std::size_t count = std::min(lhs.atoms.size(), rhs.atoms.size());
  for (std::size_t i = 0; i < count; ++i) {
    if (KeyAtomLess(lhs.atoms[i], rhs.atoms[i])) {
      return true;
    }
    if (KeyAtomLess(rhs.atoms[i], lhs.atoms[i])) {
      return false;
    }
  }
  return lhs.atoms.size() < rhs.atoms.size();
}

static std::vector<std::shared_ptr<TypeKey>> MakeKeyList(
    const std::vector<TypeKey>& keys) {
  std::vector<std::shared_ptr<TypeKey>> out;
  out.reserve(keys.size());
  for (const auto& key : keys) {
    out.push_back(std::make_shared<TypeKey>(key));
  }
  return out;
}

static std::string PermString(Permission perm) {
  switch (perm) {
    case Permission::Const:
      return "const";
    case Permission::Unique:
      return "unique";
    case Permission::Shared:
      return "shared";
  }
  return "const";
}

static std::string PtrStateString(PtrState state) {
  switch (state) {
    case PtrState::Valid:
      return "@Valid";
    case PtrState::Null:
      return "@Null";
    case PtrState::Expired:
      return "@Expired";
  }
  return "";
}

static std::string StringStateString(StringState state) {
  switch (state) {
    case StringState::Managed:
      return "@Managed";
    case StringState::View:
      return "@View";
  }
  return "";
}

static std::string BytesStateString(BytesState state) {
  switch (state) {
    case BytesState::Managed:
      return "@Managed";
    case BytesState::View:
      return "@View";
  }
  return "";
}

}  // namespace

bool IsPermInC0(Permission perm) {
  SpecDefsPermissionSets();
  return perm != Permission::Shared;
}

std::optional<Permission> ParsePermissionKeyword(std::string_view token) {
  SpecDefsPermissionSets();
  if (token == "const") {
    return Permission::Const;
  }
  if (token == "unique") {
    return Permission::Unique;
  }
  if (token == "shared") {
    return Permission::Shared;
  }
  return std::nullopt;
}

TypeRef MakeType(TypeNode node) {
  SpecDefsTypeRepr();
  return std::make_shared<Type>(Type{std::move(node)});
}

TypeRef MakeTypePrim(std::string name) {
  SpecDefsTypeRepr();
  return MakeType(TypePrim{std::move(name)});
}

TypeRef MakeTypeRange() {
  SpecDefsTypeRepr();
  return MakeType(TypeRange{});
}

TypeRef MakeTypePerm(Permission perm, TypeRef base) {
  SpecDefsTypeRepr();
  return MakeType(TypePerm{perm, std::move(base)});
}

TypeRef MakeTypeUnion(std::vector<TypeRef> members) {
  SpecDefsTypeRepr();
  return MakeType(TypeUnion{std::move(members)});
}

TypeRef MakeTypeFunc(std::vector<TypeFuncParam> params, TypeRef ret) {
  SpecDefsTypeRepr();
  return MakeType(TypeFunc{std::move(params), std::move(ret)});
}

TypeRef MakeTypeTuple(std::vector<TypeRef> elements) {
  SpecDefsTypeRepr();
  return MakeType(TypeTuple{std::move(elements)});
}

TypeRef MakeTypeArray(TypeRef element, std::uint64_t length) {
  SpecDefsTypeRepr();
  return MakeType(TypeArray{std::move(element), length});
}

TypeRef MakeTypeSlice(TypeRef element) {
  SpecDefsTypeRepr();
  return MakeType(TypeSlice{std::move(element)});
}

TypeRef MakeTypePtr(TypeRef element, std::optional<PtrState> state) {
  SpecDefsTypeRepr();
  return MakeType(TypePtr{std::move(element), state});
}

TypeRef MakeTypeRawPtr(RawPtrQual qual, TypeRef element) {
  SpecDefsTypeRepr();
  return MakeType(TypeRawPtr{qual, std::move(element)});
}

TypeRef MakeTypeString(std::optional<StringState> state) {
  SpecDefsTypeRepr();
  return MakeType(TypeString{state});
}

TypeRef MakeTypeBytes(std::optional<BytesState> state) {
  SpecDefsTypeRepr();
  return MakeType(TypeBytes{state});
}

TypeRef MakeTypeDynamic(TypePath path) {
  SpecDefsTypeRepr();
  return MakeType(TypeDynamic{std::move(path)});
}

TypeRef MakeTypeModalState(TypePath path, std::string state) {
  SpecDefsTypeRepr();
  return MakeType(TypeModalState{std::move(path), std::move(state)});
}

TypeRef MakeTypePath(TypePath path) {
  SpecDefsTypeRepr();
  return MakeType(TypePathType{std::move(path)});
}

static void AppendTypeString(std::string& out, const Type& type) {
  std::visit(
      [&](const auto& node) {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, TypePrim>) {
          out.append(node.name);
        } else if constexpr (std::is_same_v<T, TypeRange>) {
          out.append("TypeRange");
        } else if constexpr (std::is_same_v<T, TypePerm>) {
          out.append(PermString(node.perm));
          out.push_back(' ');
          AppendTypeString(out, *node.base);
        } else if constexpr (std::is_same_v<T, TypeUnion>) {
          for (std::size_t i = 0; i < node.members.size(); ++i) {
            if (i > 0) {
              out.append(" | ");
            }
            AppendTypeString(out, *node.members[i]);
          }
        } else if constexpr (std::is_same_v<T, TypeFunc>) {
          out.push_back('(');
          for (std::size_t i = 0; i < node.params.size(); ++i) {
            if (i > 0) {
              out.append(", ");
            }
            const auto& param = node.params[i];
            if (param.mode.has_value() && *param.mode == ParamMode::Move) {
              out.append("move ");
            }
            AppendTypeString(out, *param.type);
          }
          out.append(") -> ");
          AppendTypeString(out, *node.ret);
        } else if constexpr (std::is_same_v<T, TypeTuple>) {
          if (node.elements.empty()) {
            out.append("()");
            return;
          }
          out.push_back('(');
          if (node.elements.size() == 1) {
            AppendTypeString(out, *node.elements[0]);
            out.append(";)");
            return;
          }
          for (std::size_t i = 0; i < node.elements.size(); ++i) {
            if (i > 0) {
              out.append(", ");
            }
            AppendTypeString(out, *node.elements[i]);
          }
          out.push_back(')');
        } else if constexpr (std::is_same_v<T, TypeArray>) {
          out.push_back('[');
          AppendTypeString(out, *node.element);
          out.append("; ");
          out.append(std::to_string(node.length));
          out.push_back(']');
        } else if constexpr (std::is_same_v<T, TypeSlice>) {
          out.push_back('[');
          AppendTypeString(out, *node.element);
          out.push_back(']');
        } else if constexpr (std::is_same_v<T, TypePtr>) {
          out.append("Ptr<");
          AppendTypeString(out, *node.element);
          out.push_back('>');
          if (node.state.has_value()) {
            out.append(PtrStateString(*node.state));
          }
        } else if constexpr (std::is_same_v<T, TypeRawPtr>) {
          out.append("* ");
          out.append(node.qual == RawPtrQual::Imm ? "imm " : "mut ");
          AppendTypeString(out, *node.element);
        } else if constexpr (std::is_same_v<T, TypeString>) {
          out.append("string");
          if (node.state.has_value()) {
            out.append(StringStateString(*node.state));
          }
        } else if constexpr (std::is_same_v<T, TypeBytes>) {
          out.append("bytes");
          if (node.state.has_value()) {
            out.append(BytesStateString(*node.state));
          }
        } else if constexpr (std::is_same_v<T, TypeDynamic>) {
          out.push_back('$');
          out.append(PathToString(node.path));
        } else if constexpr (std::is_same_v<T, TypeModalState>) {
          out.append(PathToString(node.path));
          out.push_back('@');
          out.append(node.state);
        } else if constexpr (std::is_same_v<T, TypePathType>) {
          out.append(PathToString(node.path));
        }
      },
      type.node);
}

std::string TypeToString(const Type& type) {
  SpecDefsTypeRepr();
  std::string out;
  AppendTypeString(out, type);
  return out;
}

std::string TypeToString(const TypeRef& type) {
  SpecDefsTypeRepr();
  if (!type) {
    return "";
  }
  return TypeToString(*type);
}

KeyAtom KeyAtom::Number(std::uint64_t value) {
  KeyAtom atom;
  atom.kind = Kind::Number;
  atom.number = value;
  return atom;
}

KeyAtom KeyAtom::String(std::string value) {
  KeyAtom atom;
  atom.kind = Kind::String;
  atom.text = std::move(value);
  return atom;
}

KeyAtom KeyAtom::Key(std::shared_ptr<TypeKey> value) {
  KeyAtom atom;
  atom.kind = Kind::Key;
  atom.key = std::move(value);
  return atom;
}

KeyAtom KeyAtom::KeyList(std::vector<std::shared_ptr<TypeKey>> value) {
  KeyAtom atom;
  atom.kind = Kind::KeyList;
  atom.key_list = std::move(value);
  return atom;
}

TypeKey TypeKeyOf(const Type& type) {
  SpecDefsTypeKey();
  return std::visit(
      [&](const auto& node) -> TypeKey {
        using T = std::decay_t<decltype(node)>;
        TypeKey key;
        if constexpr (std::is_same_v<T, TypePrim>) {
          key.atoms.push_back(KeyAtom::Number(TagKeyOf(type.node)));
          key.atoms.push_back(KeyAtom::String(node.name));
          return key;
        } else if constexpr (std::is_same_v<T, TypeRange>) {
          key.atoms.push_back(KeyAtom::Number(TagKeyOf(type.node)));
          return key;
        } else if constexpr (std::is_same_v<T, TypeTuple>) {
          key.atoms.push_back(KeyAtom::Number(TagKeyOf(type.node)));
          key.atoms.push_back(KeyAtom::Number(node.elements.size()));
          for (const auto& elem : node.elements) {
            key.atoms.push_back(
                KeyAtom::Key(std::make_shared<TypeKey>(TypeKeyOf(*elem))));
          }
          return key;
        } else if constexpr (std::is_same_v<T, TypeArray>) {
          key.atoms.push_back(KeyAtom::Number(TagKeyOf(type.node)));
          key.atoms.push_back(
              KeyAtom::Key(std::make_shared<TypeKey>(TypeKeyOf(*node.element))));
          key.atoms.push_back(KeyAtom::Number(node.length));
          return key;
        } else if constexpr (std::is_same_v<T, TypeSlice>) {
          key.atoms.push_back(KeyAtom::Number(TagKeyOf(type.node)));
          key.atoms.push_back(
              KeyAtom::Key(std::make_shared<TypeKey>(TypeKeyOf(*node.element))));
          return key;
        } else if constexpr (std::is_same_v<T, TypeFunc>) {
          key.atoms.push_back(KeyAtom::Number(TagKeyOf(type.node)));
          key.atoms.push_back(KeyAtom::Number(node.params.size()));
          for (const auto& param : node.params) {
            key.atoms.push_back(KeyAtom::Number(ModeKeyOf(param.mode)));
            key.atoms.push_back(KeyAtom::Key(
                std::make_shared<TypeKey>(TypeKeyOf(*param.type))));
          }
          key.atoms.push_back(
              KeyAtom::Key(std::make_shared<TypeKey>(TypeKeyOf(*node.ret))));
          return key;
        } else if constexpr (std::is_same_v<T, TypePathType>) {
          key.atoms.push_back(KeyAtom::Number(TagKeyOf(type.node)));
          key.atoms.push_back(
              KeyAtom::Key(std::make_shared<TypeKey>(PathOrderKey(node.path))));
          return key;
        } else if constexpr (std::is_same_v<T, TypeModalState>) {
          key.atoms.push_back(KeyAtom::Number(TagKeyOf(type.node)));
          key.atoms.push_back(
              KeyAtom::Key(std::make_shared<TypeKey>(PathOrderKey(node.path))));
          key.atoms.push_back(KeyAtom::String(node.state));
          return key;
        } else if constexpr (std::is_same_v<T, TypeString>) {
          key.atoms.push_back(KeyAtom::Number(TagKeyOf(type.node)));
          key.atoms.push_back(
              KeyAtom::Number(StateKeyOf<StringState>(node.state)));
          return key;
        } else if constexpr (std::is_same_v<T, TypeBytes>) {
          key.atoms.push_back(KeyAtom::Number(TagKeyOf(type.node)));
          key.atoms.push_back(
              KeyAtom::Number(StateKeyOf<BytesState>(node.state)));
          return key;
        } else if constexpr (std::is_same_v<T, TypeDynamic>) {
          key.atoms.push_back(KeyAtom::Number(TagKeyOf(type.node)));
          key.atoms.push_back(
              KeyAtom::Key(std::make_shared<TypeKey>(PathOrderKey(node.path))));
          return key;
        } else if constexpr (std::is_same_v<T, TypePtr>) {
          key.atoms.push_back(KeyAtom::Number(TagKeyOf(type.node)));
          key.atoms.push_back(KeyAtom::Number(PtrStateKeyOf(node.state)));
          key.atoms.push_back(
              KeyAtom::Key(std::make_shared<TypeKey>(TypeKeyOf(*node.element))));
          return key;
        } else if constexpr (std::is_same_v<T, TypeRawPtr>) {
          key.atoms.push_back(KeyAtom::Number(TagKeyOf(type.node)));
          key.atoms.push_back(KeyAtom::Number(QualKeyOf(node.qual)));
          key.atoms.push_back(
              KeyAtom::Key(std::make_shared<TypeKey>(TypeKeyOf(*node.element))));
          return key;
        } else if constexpr (std::is_same_v<T, TypeUnion>) {
          key.atoms.push_back(KeyAtom::Number(TagKeyOf(type.node)));
          std::vector<TypeKey> member_keys;
          member_keys.reserve(node.members.size());
          for (const auto& member : node.members) {
            member_keys.push_back(TypeKeyOf(*member));
          }
          std::stable_sort(member_keys.begin(), member_keys.end(),
                           TypeKeyLessImpl);
          key.atoms.push_back(KeyAtom::KeyList(MakeKeyList(member_keys)));
          return key;
        } else if constexpr (std::is_same_v<T, TypePerm>) {
          key.atoms.push_back(KeyAtom::Number(TagKeyOf(type.node)));
          key.atoms.push_back(KeyAtom::Number(PermKeyOf(node.perm)));
          key.atoms.push_back(
              KeyAtom::Key(std::make_shared<TypeKey>(TypeKeyOf(*node.base))));
          return key;
        } else {
          key.atoms.push_back(KeyAtom::Number(TagKeyOf(type.node)));
          return key;
        }
      },
      type.node);
}

TypeKey TypeKeyOf(const TypeRef& type) {
  SpecDefsTypeKey();
  if (!type) {
    return TypeKey{};
  }
  return TypeKeyOf(*type);
}

bool TypeKeyEqual(const TypeKey& lhs, const TypeKey& rhs) {
  SpecDefsTypeKey();
  if (lhs.atoms.size() != rhs.atoms.size()) {
    return false;
  }
  for (std::size_t i = 0; i < lhs.atoms.size(); ++i) {
    if (!KeyAtomEqual(lhs.atoms[i], rhs.atoms[i])) {
      return false;
    }
  }
  return true;
}

bool TypeKeyLess(const TypeKey& lhs, const TypeKey& rhs) {
  SpecDefsTypeKey();
  return TypeKeyLessImpl(lhs, rhs);
}

std::vector<TypeRef> SortUnionMembers(const std::vector<TypeRef>& members) {
  SpecDefsTypeKey();
  struct Entry {
    TypeRef member;
    TypeKey key;
  };
  std::vector<Entry> entries;
  entries.reserve(members.size());
  for (const auto& member : members) {
    entries.push_back(Entry{member, TypeKeyOf(*member)});
  }
  std::stable_sort(entries.begin(), entries.end(),
                   [](const Entry& lhs, const Entry& rhs) {
                     return TypeKeyLessImpl(lhs.key, rhs.key);
                   });
  std::vector<TypeRef> sorted;
  sorted.reserve(entries.size());
  for (auto& entry : entries) {
    sorted.push_back(std::move(entry.member));
  }
  return sorted;
}

static void CollectTypePaths(const Type& type, std::vector<TypePath>& out) {
  std::visit(
      [&](const auto& node) {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, TypePrim>) {
          return;
        } else if constexpr (std::is_same_v<T, TypeRange>) {
          return;
        } else if constexpr (std::is_same_v<T, TypePerm>) {
          CollectTypePaths(*node.base, out);
        } else if constexpr (std::is_same_v<T, TypeTuple>) {
          for (const auto& elem : node.elements) {
            CollectTypePaths(*elem, out);
          }
        } else if constexpr (std::is_same_v<T, TypeArray>) {
          CollectTypePaths(*node.element, out);
        } else if constexpr (std::is_same_v<T, TypeSlice>) {
          CollectTypePaths(*node.element, out);
        } else if constexpr (std::is_same_v<T, TypeUnion>) {
          for (const auto& member : node.members) {
            CollectTypePaths(*member, out);
          }
        } else if constexpr (std::is_same_v<T, TypeFunc>) {
          for (const auto& param : node.params) {
            CollectTypePaths(*param.type, out);
          }
          CollectTypePaths(*node.ret, out);
        } else if constexpr (std::is_same_v<T, TypePtr>) {
          CollectTypePaths(*node.element, out);
        } else if constexpr (std::is_same_v<T, TypeRawPtr>) {
          CollectTypePaths(*node.element, out);
        } else if constexpr (std::is_same_v<T, TypeString>) {
          return;
        } else if constexpr (std::is_same_v<T, TypeBytes>) {
          return;
        } else if constexpr (std::is_same_v<T, TypeDynamic>) {
          out.push_back(node.path);
        } else if constexpr (std::is_same_v<T, TypeModalState>) {
          out.push_back(node.path);
        } else if constexpr (std::is_same_v<T, TypePathType>) {
          out.push_back(node.path);
        }
      },
      type.node);
}

std::vector<TypePath> TypePaths(const Type& type) {
  SpecDefsTypePaths();
  std::vector<TypePath> out;
  CollectTypePaths(type, out);
  std::stable_sort(out.begin(), out.end(), PathLess);
  out.erase(std::unique(out.begin(), out.end()), out.end());
  return out;
}

std::vector<TypePath> TypePaths(const TypeRef& type) {
  SpecDefsTypePaths();
  if (!type) {
    return {};
  }
  return TypePaths(*type);
}

}  // namespace cursive0::sema
