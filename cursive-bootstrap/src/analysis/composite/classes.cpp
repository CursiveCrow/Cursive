#include "cursive0/analysis/composite/classes.h"

#include <algorithm>
#include <unordered_map>
#include <unordered_set>

#include "cursive0/core/assert_spec.h"
#include "cursive0/analysis/resolve/scopes.h"
#include "cursive0/analysis/types/type_equiv.h"

namespace cursive0::analysis {

namespace {

static inline void SpecDefsClasses() {
  SPEC_DEF("ClassItems", "5.3.1");
  SPEC_DEF("ClassMethods", "5.3.1");
  SPEC_DEF("ClassFields", "5.3.1");
  SPEC_DEF("MethodNames", "5.3.1");
  SPEC_DEF("FieldNames", "5.3.1");
  SPEC_DEF("SelfVar", "5.3.1");
  SPEC_DEF("SubstSelf", "5.3.1");
  SPEC_DEF("RecvType", "5.3.1");
  SPEC_DEF("RecvMode", "5.3.1");
  SPEC_DEF("RecvPerm", "5.3.1");
  SPEC_DEF("ParamSig_T", "5.3.1");
  SPEC_DEF("ParamBinds_T", "5.3.1");
  SPEC_DEF("ReturnType_T", "5.3.1");
  SPEC_DEF("Sig_T", "5.3.1");
  SPEC_DEF("SigSelf", "5.3.1");
  SPEC_DEF("StripPerm", "5.2.12");
  SPEC_DEF("EffMethods", "5.3.1");
  SPEC_DEF("FirstByName", "5.3.1");
  SPEC_DEF("EffFields", "5.3.1");
  SPEC_DEF("FirstFieldByName", "5.3.1");
  SPEC_DEF("FieldSig", "5.3.1");
  SPEC_DEF("SelfOccurs", "5.3.1");
  SPEC_DEF("dispatchable", "5.3.1");
  SPEC_DEF("vtable_eligible", "5.3.1");
  SPEC_DEF("ClassMethodTable", "5.3.1");
  SPEC_DEF("ClassFieldTable", "5.3.1");
  SPEC_DEF("LookupClassMethod", "5.3.1");
  SPEC_DEF("SuperclassClosure", "5.3.1");
}

static const syntax::ClassDecl* LookupClassDecl(const ScopeContext& ctx,
                                                const syntax::ClassPath& path) {
  const auto it = ctx.sigma.classes.find(PathKeyOf(path));
  if (it == ctx.sigma.classes.end()) {
    return nullptr;
  }
  return &it->second;
}

static std::vector<const syntax::ClassMethodDecl*> ClassMethods(
    const syntax::ClassDecl& decl) {
  std::vector<const syntax::ClassMethodDecl*> out;
  for (const auto& item : decl.items) {
    if (const auto* method = std::get_if<syntax::ClassMethodDecl>(&item)) {
      out.push_back(method);
    }
  }
  return out;
}

static std::vector<const syntax::ClassFieldDecl*> ClassFields(
    const syntax::ClassDecl& decl) {
  std::vector<const syntax::ClassFieldDecl*> out;
  for (const auto& item : decl.items) {
    if (const auto* field = std::get_if<syntax::ClassFieldDecl>(&item)) {
      out.push_back(field);
    }
  }
  return out;
}

struct TypeLowerResult {
  bool ok = false;
  std::optional<std::string_view> diag_id;
  TypeRef type;
};

static Permission LowerPermission(syntax::TypePerm perm) {
  switch (perm) {
    case syntax::TypePerm::Const:
      return Permission::Const;
    case syntax::TypePerm::Unique:
      return Permission::Unique;
    case syntax::TypePerm::Shared:
      return Permission::Shared;
  }
  return Permission::Const;
}

static std::optional<ParamMode> LowerParamMode(
    const std::optional<syntax::ParamMode>& mode) {
  if (!mode.has_value()) {
    return std::nullopt;
  }
  return ParamMode::Move;
}

static RawPtrQual LowerRawPtrQual(syntax::RawPtrQual qual) {
  switch (qual) {
    case syntax::RawPtrQual::Imm:
      return RawPtrQual::Imm;
    case syntax::RawPtrQual::Mut:
      return RawPtrQual::Mut;
  }
  return RawPtrQual::Imm;
}

static std::optional<StringState> LowerStringState(
    const std::optional<syntax::StringState>& state) {
  if (!state.has_value()) {
    return std::nullopt;
  }
  switch (*state) {
    case syntax::StringState::Managed:
      return StringState::Managed;
    case syntax::StringState::View:
      return StringState::View;
  }
  return std::nullopt;
}

static std::optional<BytesState> LowerBytesState(
    const std::optional<syntax::BytesState>& state) {
  if (!state.has_value()) {
    return std::nullopt;
  }
  switch (*state) {
    case syntax::BytesState::Managed:
      return BytesState::Managed;
    case syntax::BytesState::View:
      return BytesState::View;
  }
  return std::nullopt;
}

static std::optional<PtrState> LowerPtrState(
    const std::optional<syntax::PtrState>& state) {
  if (!state.has_value()) {
    return std::nullopt;
  }
  switch (*state) {
    case syntax::PtrState::Valid:
      return PtrState::Valid;
    case syntax::PtrState::Null:
      return PtrState::Null;
    case syntax::PtrState::Expired:
      return PtrState::Expired;
  }
  return std::nullopt;
}

static TypeLowerResult LowerType(const ScopeContext& ctx,
                                 const std::shared_ptr<syntax::Type>& type) {
  if (!type) {
    return {false, std::nullopt, {}};
  }
  return std::visit(
      [&](const auto& node) -> TypeLowerResult {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::TypePrim>) {
          return {true, std::nullopt, MakeTypePrim(node.name)};
        } else if constexpr (std::is_same_v<T, syntax::TypePermType>) {
          const auto base = LowerType(ctx, node.base);
          if (!base.ok) {
            return base;
          }
          return {true, std::nullopt,
                  MakeTypePerm(LowerPermission(node.perm), base.type)};
        } else if constexpr (std::is_same_v<T, syntax::TypeUnion>) {
          std::vector<TypeRef> members;
          members.reserve(node.types.size());
          for (const auto& elem : node.types) {
            const auto lowered = LowerType(ctx, elem);
            if (!lowered.ok) {
              return lowered;
            }
            members.push_back(lowered.type);
          }
          return {true, std::nullopt, MakeTypeUnion(std::move(members))};
        } else if constexpr (std::is_same_v<T, syntax::TypeFunc>) {
          std::vector<TypeFuncParam> params;
          params.reserve(node.params.size());
          for (const auto& param : node.params) {
            const auto lowered = LowerType(ctx, param.type);
            if (!lowered.ok) {
              return lowered;
            }
            params.push_back(TypeFuncParam{LowerParamMode(param.mode),
                                           lowered.type});
          }
          const auto ret = LowerType(ctx, node.ret);
          if (!ret.ok) {
            return ret;
          }
          return {true, std::nullopt, MakeTypeFunc(std::move(params), ret.type)};
        } else if constexpr (std::is_same_v<T, syntax::TypeTuple>) {
          std::vector<TypeRef> elements;
          elements.reserve(node.elements.size());
          for (const auto& elem : node.elements) {
            const auto lowered = LowerType(ctx, elem);
            if (!lowered.ok) {
              return lowered;
            }
            elements.push_back(lowered.type);
          }
          return {true, std::nullopt, MakeTypeTuple(std::move(elements))};
        } else if constexpr (std::is_same_v<T, syntax::TypeArray>) {
          const auto elem = LowerType(ctx, node.element);
          if (!elem.ok) {
            return elem;
          }
          const auto len = ConstLen(ctx, node.length);
          if (!len.ok || !len.value.has_value()) {
            return {false, len.diag_id, {}};
          }
          return {true, std::nullopt, MakeTypeArray(elem.type, *len.value)};
        } else if constexpr (std::is_same_v<T, syntax::TypeSlice>) {
          const auto elem = LowerType(ctx, node.element);
          if (!elem.ok) {
            return elem;
          }
          return {true, std::nullopt, MakeTypeSlice(elem.type)};
        } else if constexpr (std::is_same_v<T, syntax::TypePtr>) {
          const auto elem = LowerType(ctx, node.element);
          if (!elem.ok) {
            return elem;
          }
          return {true, std::nullopt,
                  MakeTypePtr(elem.type, LowerPtrState(node.state))};
        } else if constexpr (std::is_same_v<T, syntax::TypeRawPtr>) {
          const auto elem = LowerType(ctx, node.element);
          if (!elem.ok) {
            return elem;
          }
          return {true, std::nullopt,
                  MakeTypeRawPtr(LowerRawPtrQual(node.qual), elem.type)};
        } else if constexpr (std::is_same_v<T, syntax::TypeString>) {
          return {true, std::nullopt,
                  MakeTypeString(LowerStringState(node.state))};
        } else if constexpr (std::is_same_v<T, syntax::TypeBytes>) {
          return {true, std::nullopt,
                  MakeTypeBytes(LowerBytesState(node.state))};
        } else if constexpr (std::is_same_v<T, syntax::TypeDynamic>) {
          return {true, std::nullopt, MakeTypeDynamic(node.path)};
        } else if constexpr (std::is_same_v<T, syntax::TypeOpaque>) {
          return {true, std::nullopt,
                  MakeTypeOpaque(node.path, type.get(), type->span)};
        } else if constexpr (std::is_same_v<T, syntax::TypeRefine>) {
          const auto base = LowerType(ctx, node.base);
          if (!base.ok) {
            return base;
          }
          return {true, std::nullopt,
                  MakeTypeRefine(base.type, node.predicate)};
        } else if constexpr (std::is_same_v<T, syntax::TypeModalState>) {
          std::vector<TypeRef> args;
          args.reserve(node.generic_args.size());
          for (const auto& arg : node.generic_args) {
            const auto lowered = LowerType(ctx, arg);
            if (!lowered.ok) {
              return lowered;
            }
            args.push_back(lowered.type);
          }
          return {true, std::nullopt,
                  MakeTypeModalState(node.path, node.state, std::move(args))};
        } else if constexpr (std::is_same_v<T, syntax::TypePathType>) {
          // §5.2.9, §13.1: Generic type instantiation lowering
          // Per WF-Apply (§5.2.3), type arguments MUST be preserved
          if (!node.generic_args.empty()) {
            std::vector<TypeRef> lowered_args;
            lowered_args.reserve(node.generic_args.size());
            for (const auto& arg : node.generic_args) {
              const auto lower_result = LowerType(ctx, arg);
              if (!lower_result.ok) {
                return lower_result;
              }
              lowered_args.push_back(lower_result.type);
            }
            return {true, std::nullopt,
                    MakeTypePath(node.path, std::move(lowered_args))};
          }
          return {true, std::nullopt, MakeTypePath(node.path)};
        } else {
          return {false, std::nullopt, {}};
        }
      },
      type->node);
}

static TypeLowerResult LowerReturnType(
    const ScopeContext& ctx,
    const std::shared_ptr<syntax::Type>& type_opt) {
  if (!type_opt) {
    return {true, std::nullopt, MakeTypePrim("()")};
  }
  return LowerType(ctx, type_opt);
}

static TypeRef SubstSelfType(const TypeRef& self, const TypeRef& type) {
  if (!type) {
    return type;
  }
  return std::visit(
      [&](const auto& node) -> TypeRef {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, TypePathType>) {
          if (node.path.size() == 1 && node.path[0] == "Self") {
            return self;
          }
          if (node.generic_args.empty()) {
            return type;
          }
          TypePathType out = node;
          out.generic_args.clear();
          out.generic_args.reserve(node.generic_args.size());
          for (const auto& arg : node.generic_args) {
            out.generic_args.push_back(SubstSelfType(self, arg));
          }
          return MakeType(out);
        } else if constexpr (std::is_same_v<T, TypePerm>) {
          return MakeTypePerm(node.perm, SubstSelfType(self, node.base));
        } else if constexpr (std::is_same_v<T, TypeTuple>) {
          std::vector<TypeRef> elems;
          elems.reserve(node.elements.size());
          for (const auto& elem : node.elements) {
            elems.push_back(SubstSelfType(self, elem));
          }
          return MakeTypeTuple(std::move(elems));
        } else if constexpr (std::is_same_v<T, TypeArray>) {
          return MakeTypeArray(SubstSelfType(self, node.element), node.length);
        } else if constexpr (std::is_same_v<T, TypeSlice>) {
          return MakeTypeSlice(SubstSelfType(self, node.element));
        } else if constexpr (std::is_same_v<T, TypeUnion>) {
          std::vector<TypeRef> members;
          members.reserve(node.members.size());
          for (const auto& member : node.members) {
            members.push_back(SubstSelfType(self, member));
          }
          return MakeTypeUnion(std::move(members));
        } else if constexpr (std::is_same_v<T, TypeFunc>) {
          std::vector<TypeFuncParam> params;
          params.reserve(node.params.size());
          for (const auto& param : node.params) {
            params.push_back(TypeFuncParam{param.mode,
                                           SubstSelfType(self, param.type)});
          }
          return MakeTypeFunc(std::move(params), SubstSelfType(self, node.ret));
        } else if constexpr (std::is_same_v<T, TypePtr>) {
          return MakeTypePtr(SubstSelfType(self, node.element), node.state);
        } else if constexpr (std::is_same_v<T, TypeRawPtr>) {
          return MakeTypeRawPtr(node.qual, SubstSelfType(self, node.element));
        } else if constexpr (std::is_same_v<T, TypeRefine>) {
          return MakeTypeRefine(SubstSelfType(self, node.base), node.predicate);
        } else if constexpr (std::is_same_v<T, TypeModalState>) {
          TypeModalState out = node;
          out.generic_args.clear();
          out.generic_args.reserve(node.generic_args.size());
          for (const auto& arg : node.generic_args) {
            out.generic_args.push_back(SubstSelfType(self, arg));
          }
          return MakeType(out);
        } else {
          return type;
        }
      },
      type->node);
}

static TypeRef StripPerm(const TypeRef& type) {
  SpecDefsClasses();
  if (!type) {
    return type;
  }
  if (const auto* perm = std::get_if<TypePerm>(&type->node)) {
    return StripPerm(perm->base);
  }
  if (const auto* refine = std::get_if<TypeRefine>(&type->node)) {
    return StripPerm(refine->base);
  }
  return type;
}


static Permission LowerReceiverPerm(syntax::ReceiverPerm perm) {
  switch (perm) {
    case syntax::ReceiverPerm::Const:
      return Permission::Const;
    case syntax::ReceiverPerm::Unique:
      return Permission::Unique;
    case syntax::ReceiverPerm::Shared:
      return Permission::Shared;
  }
  return Permission::Const;
}

struct MethodSig {
  bool ok = false;
  TypeRef recv_type;
  std::vector<TypeFuncParam> params;
  TypeRef ret;
};

static MethodSig MethodSigSelf(const ScopeContext& ctx,
                               const syntax::ClassMethodDecl& method) {
  MethodSig sig;
  const auto self = MakeTypePath({"Self"});

  if (const auto* shorthand =
          std::get_if<syntax::ReceiverShorthand>(&method.receiver)) {
    sig.recv_type = MakeTypePerm(LowerReceiverPerm(shorthand->perm), self);
  } else if (const auto* explicit_recv =
                 std::get_if<syntax::ReceiverExplicit>(&method.receiver)) {
    const auto lowered = LowerType(ctx, explicit_recv->type);
    if (!lowered.ok) {
      return sig;
    }
    sig.recv_type = SubstSelfType(self, lowered.type);
  }

  for (const auto& param : method.params) {
    const auto lowered = LowerType(ctx, param.type);
    if (!lowered.ok) {
      return sig;
    }
    sig.params.push_back(TypeFuncParam{LowerParamMode(param.mode),
                                       SubstSelfType(self, lowered.type)});
  }

  const auto ret = LowerReturnType(ctx, method.return_type_opt);
  if (!ret.ok) {
    return sig;
  }
  sig.ret = SubstSelfType(self, ret.type);
  sig.ok = true;
  return sig;
}

static bool SigEqual(const MethodSig& lhs, const MethodSig& rhs) {
  if (!lhs.ok || !rhs.ok) {
    return false;
  }
  const auto recv_eq = TypeEquiv(lhs.recv_type, rhs.recv_type);
  if (!recv_eq.ok || !recv_eq.equiv) {
    return false;
  }
  if (lhs.params.size() != rhs.params.size()) {
    return false;
  }
  for (std::size_t i = 0; i < lhs.params.size(); ++i) {
    if (lhs.params[i].mode != rhs.params[i].mode) {
      return false;
    }
    const auto param_eq = TypeEquiv(lhs.params[i].type, rhs.params[i].type);
    if (!param_eq.ok || !param_eq.equiv) {
      return false;
    }
  }
  const auto ret_eq = TypeEquiv(lhs.ret, rhs.ret);
  if (!ret_eq.ok || !ret_eq.equiv) {
    return false;
  }
  return true;
}

static bool SelfOccurs(const std::shared_ptr<syntax::Type>& type) {
  if (!type) {
    return false;
  }
  return std::visit(
      [&](const auto& node) -> bool {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::TypePathType>) {
          return node.path.size() == 1 && IdEq(node.path[0], "Self");
        } else if constexpr (std::is_same_v<T, syntax::TypePermType>) {
          return SelfOccurs(node.base);
        } else if constexpr (std::is_same_v<T, syntax::TypeTuple>) {
          for (const auto& elem : node.elements) {
            if (SelfOccurs(elem)) {
              return true;
            }
          }
          return false;
        } else if constexpr (std::is_same_v<T, syntax::TypeArray>) {
          return SelfOccurs(node.element);
        } else if constexpr (std::is_same_v<T, syntax::TypeSlice>) {
          return SelfOccurs(node.element);
        } else if constexpr (std::is_same_v<T, syntax::TypeUnion>) {
          for (const auto& elem : node.types) {
            if (SelfOccurs(elem)) {
              return true;
            }
          }
          return false;
        } else if constexpr (std::is_same_v<T, syntax::TypeFunc>) {
          for (const auto& param : node.params) {
            if (SelfOccurs(param.type)) {
              return true;
            }
          }
          return SelfOccurs(node.ret);
        } else if constexpr (std::is_same_v<T, syntax::TypePtr>) {
          return SelfOccurs(node.element);
        } else if constexpr (std::is_same_v<T, syntax::TypeRawPtr>) {
          return SelfOccurs(node.element);
        } else if constexpr (std::is_same_v<T, syntax::TypeRefine>) {
          return SelfOccurs(node.base);
        } else {
          return false;
        }
      },
      type->node);
}

static bool SelfOccurs(const syntax::ClassMethodDecl& method) {
  if (method.return_type_opt && SelfOccurs(method.return_type_opt)) {
    return true;
  }
  for (const auto& param : method.params) {
    if (SelfOccurs(param.type)) {
      return true;
    }
  }
  return false;
}

static bool VtableEligible(const syntax::ClassMethodDecl& method) {
  return !SelfOccurs(method);
}

}  // namespace

ClassMethodTableResult ClassMethodTable(const ScopeContext& ctx,
                                        const syntax::ClassPath& path) {
  SpecDefsClasses();
  ClassMethodTableResult result;
  const auto lin = LinearizeClass(ctx, path);
  if (!lin.ok) {
    result.diag_id = lin.diag_id;
    return result;
  }

  std::unordered_map<IdKey, MethodSig> seen;
  for (const auto& cls_path : lin.order) {
    const auto* decl = LookupClassDecl(ctx, cls_path);
    if (!decl) {
      result.diag_id = "Superclass-Undefined";
      return result;
    }
    const auto methods = ClassMethods(*decl);
    for (const auto* method : methods) {
      const auto key = IdKeyOf(method->name);
      const auto sig = MethodSigSelf(ctx, *method);
      if (!sig.ok) {
        continue;
      }
      const auto it = seen.find(key);
      if (it == seen.end()) {
        seen.emplace(key, sig);
        result.methods.push_back(ClassMethodTableResult::Entry{method, cls_path});
        continue;
      }
      if (SigEqual(it->second, sig)) {
        continue;
      }
      SPEC_RULE("EffMethods-Conflict");
      result.diag_id = "EffMethods-Conflict";
      return result;
    }
  }

  result.ok = true;
  return result;
}

ClassFieldTableResult ClassFieldTable(const ScopeContext& ctx,
                                      const syntax::ClassPath& path) {
  SpecDefsClasses();
  ClassFieldTableResult result;
  const auto lin = LinearizeClass(ctx, path);
  if (!lin.ok) {
    result.diag_id = lin.diag_id;
    return result;
  }

  std::vector<const syntax::ClassFieldDecl*> all;
  for (const auto& cls_path : lin.order) {
    const auto* decl = LookupClassDecl(ctx, cls_path);
    if (!decl) {
      result.diag_id = "Superclass-Undefined";
      return result;
    }
    const auto fields = ClassFields(*decl);
    all.insert(all.end(), fields.begin(), fields.end());
  }

  std::unordered_map<IdKey, TypeRef> seen;
  for (const auto* field : all) {
    const auto key = IdKeyOf(field->name);
    const auto lowered = LowerType(ctx, field->type);
    if (!lowered.ok) {
      continue;
    }
    const auto it = seen.find(key);
    if (it == seen.end()) {
      seen.emplace(key, lowered.type);
      result.fields.push_back(field);
      continue;
    }
    const auto eq = TypeEquiv(it->second, lowered.type);
    if (eq.ok && eq.equiv) {
      continue;
    }
    SPEC_RULE("EffFields-Conflict");
    result.diag_id = "EffFields-Conflict";
    return result;
  }

  result.ok = true;
  return result;
}

const syntax::ClassMethodDecl* LookupClassMethod(const ScopeContext& ctx,
                                                 const syntax::ClassPath& path,
                                                 std::string_view name) {
  SpecDefsClasses();
  const auto table = ClassMethodTable(ctx, path);
  if (!table.ok) {
    return nullptr;
  }
  for (const auto& entry : table.methods) {
    if (entry.method && IdEq(entry.method->name, name)) {
      return entry.method;
    }
  }
  return nullptr;
}

bool ClassDispatchable(const ScopeContext& ctx, const syntax::ClassPath& path) {
  SpecDefsClasses();
  const auto table = ClassMethodTable(ctx, path);
  if (!table.ok) {
    return false;
  }
  for (const auto& entry : table.methods) {
    if (!entry.method || !VtableEligible(*entry.method)) {
      return false;
    }
  }
  return true;
}

bool ClassSubtypes(const ScopeContext& ctx,
                   const syntax::ClassPath& sub,
                   const syntax::ClassPath& sup) {
  SpecDefsClasses();
  if (PathEq(sub, sup)) {
    return true;
  }
  const auto lin = LinearizeClass(ctx, sub);
  if (!lin.ok) {
    return false;
  }
  for (const auto& entry : lin.order) {
    if (PathEq(entry, sup) && !PathEq(entry, sub)) {
      return true;
    }
  }
  return false;
}

bool TypeImplementsClass(const ScopeContext& ctx,
                         const TypeRef& type,
                         const syntax::ClassPath& path) {
  SpecDefsClasses();
  if (!type) {
    return false;
  }
  const auto stripped = StripPerm(type);
  if (!stripped) {
    return false;
  }
  const auto* path_type = std::get_if<TypePathType>(&stripped->node);
  if (!path_type) {
    return false;
  }

  syntax::Path syntax_path;
  syntax_path.reserve(path_type->path.size());
  for (const auto& comp : path_type->path) {
    syntax_path.push_back(comp);
  }
  const auto it = ctx.sigma.types.find(PathKeyOf(syntax_path));
  if (it == ctx.sigma.types.end()) {
    return false;
  }

  const std::vector<syntax::ClassPath>* impls = nullptr;
  if (const auto* record = std::get_if<syntax::RecordDecl>(&it->second)) {
    impls = &record->implements;
  } else if (const auto* enum_decl = std::get_if<syntax::EnumDecl>(&it->second)) {
    impls = &enum_decl->implements;
  } else if (const auto* modal_decl =
                 std::get_if<syntax::ModalDecl>(&it->second)) {
    impls = &modal_decl->implements;
  }
  if (!impls) {
    return false;
  }

  for (const auto& impl : *impls) {
    if (PathEq(impl, path)) {
      return true;
    }
    if (ClassSubtypes(ctx, impl, path)) {
      return true;
    }
  }

  return false;
}

// C0X Extension: Associated types

std::vector<const syntax::AssociatedTypeDecl*> ClassAssociatedTypes(
    const syntax::ClassDecl& decl) {
  std::vector<const syntax::AssociatedTypeDecl*> out;
  for (const auto& item : decl.items) {
    if (const auto* assoc = std::get_if<syntax::AssociatedTypeDecl>(&item)) {
      out.push_back(assoc);
    }
  }
  return out;
}

// C0X Extension: Abstract states for modal classes

std::vector<const syntax::AbstractStateDecl*> ClassAbstractStates(
    const syntax::ClassDecl& decl) {
  std::vector<const syntax::AbstractStateDecl*> out;
  for (const auto& item : decl.items) {
    if (const auto* state = std::get_if<syntax::AbstractStateDecl>(&item)) {
      out.push_back(state);
    }
  }
  return out;
}

// C0X Extension: Check if class is a modal class
bool IsModalClass(const syntax::ClassDecl& decl) {
  SPEC_RULE("T-Modal-Class");
  return decl.modal || !ClassAbstractStates(decl).empty();
}

// C0X Extension: VTable eligibility check
// A method is vtable-eligible if:
// 1. Has a receiver (not static)
// 2. No generic params on the method itself
// 3. Does not use Self by value (except through pointers)
bool VTableEligible(const syntax::ClassMethodDecl& method) {
  SpecDefsClasses();
  SPEC_RULE("vtable_eligible");
  
  // Must have receiver
  if (std::holds_alternative<syntax::ReceiverShorthand>(method.receiver)) {
    // Shorthand always has receiver
  } else if (const auto* explicit_recv = 
                 std::get_if<syntax::ReceiverExplicit>(&method.receiver)) {
    // Explicit receiver exists
  } else {
    return false;
  }
  
  // Check if Self appears by value (not through pointer)
  // For parameters
  for (const auto& param : method.params) {
    if (SelfOccurs(param.type)) {
      // Check if it's through a pointer
      if (const auto* type = param.type.get()) {
        if (std::holds_alternative<syntax::TypePathType>(type->node)) {
          // Direct Self by value - not vtable eligible
          const auto* path = std::get_if<syntax::TypePathType>(&type->node);
          if (path && path->path.size() == 1 && path->path[0] == "Self") {
            return false;
          }
        }
      }
    }
  }
  
  return true;
}

// C0X Extension: Dispatchability check
// A class is dispatchable if all procedures are either:
// - VTable eligible, or
// - Marked with [[static_dispatch_only]]
bool Dispatchable(const ScopeContext& ctx, const syntax::ClassDecl& decl) {
  SpecDefsClasses();
  SPEC_RULE("dispatchable");
  
  for (const auto& item : decl.items) {
    if (const auto* method = std::get_if<syntax::ClassMethodDecl>(&item)) {
      if (!VTableEligible(*method) && !method->static_dispatch_only) {
        return false;
      }
    }
  }
  
  return true;
}

// C0X Extension: Implementation completeness check
// (CompletenessResult is declared in the header)

CompletenessResult CheckImplCompleteness(
    const ScopeContext& ctx,
    const syntax::ClassPath& class_path,
    const syntax::RecordDecl& impl) {
  CompletenessResult result;
  
  const auto* class_decl = LookupClassDecl(ctx, class_path);
  if (!class_decl) {
    result.ok = false;
    return result;
  }
  
  // Check methods
  for (const auto* class_method : ClassMethods(*class_decl)) {
    if (!class_method->body_opt) {
      // Abstract method - must be implemented
      bool found = false;
      for (const auto& member : impl.members) {
        if (const auto* method = std::get_if<syntax::MethodDecl>(&member)) {
          if (IdEq(method->name, class_method->name)) {
            found = true;
            break;
          }
        }
      }
      if (!found) {
        result.missing_methods.push_back(class_method->name);
      }
    }
  }
  
  // Check associated types
  for (const auto* assoc_type : ClassAssociatedTypes(*class_decl)) {
    if (!assoc_type->default_type) {
      // Abstract associated type - must be provided
      // Implementation would provide this via a type alias member
      // (Simplified check - full impl needs type member lookup)
    }
  }
  
  result.ok = result.missing_methods.empty() && 
              result.missing_types.empty() &&
              result.missing_states.empty();
  
  if (!result.ok) {
    result.diag_id = "E-TYP-IMPL-INCOMPLETE";
  }
  
  return result;
}

// C0X Extension: Orphan rule check
// At least one of T or Cl must be defined in the current assembly
bool CheckOrphanRule(const ScopeContext& ctx,
                     const TypePath& type_path,
                     const syntax::ClassPath& class_path,
                     const syntax::ModulePath& current_module) {
  SPEC_RULE("T-Orphan-Rule");
  
  // Check if type is defined in current assembly
  // (Simplified - full impl needs assembly tracking)
  bool type_local = false;
  for (const auto& [key, decl] : ctx.sigma.types) {
    // Check if type path matches and is in current module prefix
    if (PathKeyOf(type_path) == key) {
      type_local = true;
      break;
    }
  }
  
  // Check if class is defined in current assembly
  bool class_local = ctx.sigma.classes.find(PathKeyOf(class_path)) 
                     != ctx.sigma.classes.end();
  
  return type_local || class_local;
}

}  // namespace cursive0::analysis
