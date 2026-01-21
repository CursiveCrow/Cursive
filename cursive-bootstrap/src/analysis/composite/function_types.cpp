#include "cursive0/analysis/composite/function_types.h"

#include <optional>
#include <string_view>
#include <utility>
#include <vector>

#include "cursive0/core/assert_spec.h"
#include "cursive0/core/symbols.h"
#include "cursive0/analysis/resolve/collect_toplevel.h"
#include "cursive0/analysis/resolve/scopes.h"
#include "cursive0/analysis/resolve/scopes_lookup.h"
#include "cursive0/analysis/types/type_equiv.h"
#include "cursive0/analysis/memory/string_bytes.h"
#include "cursive0/analysis/resolve/visibility.h"

namespace cursive0::analysis {

namespace {

struct TypeLowerResult {
  bool ok = false;
  std::optional<std::string_view> diag_id;
  TypeRef type;
};

static inline void SpecDefsFunctionTypes() {
  SPEC_DEF("ValuePathType", "5.2.12");
  SPEC_DEF("ProcReturn", "5.2.14");
}

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
          return {true, std::nullopt,
                  MakeTypeArray(elem.type, *len.value)};
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
        } else if constexpr (std::is_same_v<T, syntax::TypeModalState>) {
          return {true, std::nullopt,
                  MakeTypeModalState(node.path, node.state)};
        } else if constexpr (std::is_same_v<T, syntax::TypePathType>) {
          return {true, std::nullopt, MakeTypePath(node.path)};
        } else {
          return {false, std::nullopt, {}};
        }
      },
      type->node);
}

static TypeLowerResult ProcReturn(const ScopeContext& ctx,
                                  const std::shared_ptr<syntax::Type>& ret_opt) {
  SpecDefsFunctionTypes();
  if (!ret_opt) {
    return {true, std::nullopt, MakeTypePrim("()")};
  }
  return LowerType(ctx, ret_opt);
}

static const syntax::ASTModule* FindModule(const ScopeContext& ctx,
                                           const syntax::ModulePath& path) {
  for (const auto& module : ctx.sigma.mods) {
    if (PathEq(module.path, path)) {
      return &module;
    }
  }
  return nullptr;
}

static ModuleNames ModuleNamesForContext(const ScopeContext& ctx) {
  if (ctx.project) {
    return ModuleNamesOf(*ctx.project);
  }
  ModuleNames names;
  names.reserve(ctx.sigma.mods.size());
  for (const auto& mod : ctx.sigma.mods) {
    names.push_back(core::StringOfPath(mod.path));
  }
  return names;
}

static TypeLowerResult StaticTypeOf(const ScopeContext& ctx,
                                    const syntax::ASTModule& module,
                                    std::string_view name) {
  for (const auto& item : module.items) {
    const auto* decl = std::get_if<syntax::StaticDecl>(&item);
    if (!decl || !decl->binding.pat) {
      continue;
    }
    const auto& pat = *decl->binding.pat;
    if (!decl->binding.type_opt) {
      continue;
    }
    if (const auto* ident = std::get_if<syntax::IdentifierPattern>(&pat.node)) {
      if (IdEq(ident->name, name)) {
        return LowerType(ctx, decl->binding.type_opt);
      }
    } else if (const auto* typed =
                   std::get_if<syntax::TypedPattern>(&pat.node)) {
      if (IdEq(typed->name, name)) {
        return LowerType(ctx, decl->binding.type_opt);
      }
    }
  }
  return {true, std::nullopt, {}};
}

static const syntax::ProcedureDecl* FindProcedure(
    const syntax::ASTModule& module,
    std::string_view name) {
  for (const auto& item : module.items) {
    const auto* decl = std::get_if<syntax::ProcedureDecl>(&item);
    if (!decl) {
      continue;
    }
    if (IdEq(decl->name, name)) {
      return decl;
    }
  }
  return nullptr;
}

}  // namespace

ValuePathTypeResult ProcType(const ScopeContext& ctx,
                             const syntax::ProcedureDecl& decl) {
  SpecDefsFunctionTypes();
  std::vector<TypeFuncParam> params;
  params.reserve(decl.params.size());
  for (const auto& param : decl.params) {
    const auto lowered = LowerType(ctx, param.type);
    if (!lowered.ok) {
      return {false, lowered.diag_id, {}};
    }
    params.push_back(TypeFuncParam{LowerParamMode(param.mode), lowered.type});
  }
  const auto ret = ProcReturn(ctx, decl.return_type_opt);
  if (!ret.ok) {
    return {false, ret.diag_id, {}};
  }
  SPEC_RULE("T-Proc-As-Value");
  return {true, std::nullopt, MakeTypeFunc(std::move(params), ret.type)};
}

ValuePathTypeResult ValuePathType(const ScopeContext& ctx,
                                  const syntax::ModulePath& path,
                                  std::string_view name) {
  SpecDefsFunctionTypes();
  if (const auto builtin = LookupStringBytesBuiltinType(path, name)) {
    return {true, std::nullopt, *builtin};
  }
  ScopeContext local = ctx;
  const auto current_module = local.current_module;
  const auto name_maps = CollectNameMaps(local);
  local.current_module = current_module;
  const auto module_names = ModuleNamesForContext(local);
  const auto resolved = ResolveQualified(
      local, name_maps.name_maps, module_names, path, name, EntityKind::Value,
      CanAccess);
  if (!resolved.ok) {
    return {false, resolved.diag_id, {}};
  }
  if (!resolved.entity || !resolved.entity->origin_opt) {
    return {true, std::nullopt, {}};
  }
  const auto* module = FindModule(local, *resolved.entity->origin_opt);
  if (!module) {
    return {true, std::nullopt, {}};
  }
  const auto static_type = StaticTypeOf(local, *module, name);
  if (!static_type.ok) {
    return {false, static_type.diag_id, {}};
  }
  if (static_type.type) {
    return {true, std::nullopt, static_type.type};
  }
  const auto* proc = FindProcedure(*module, name);
  if (!proc) {
    return {true, std::nullopt, {}};
  }
  return ProcType(local, *proc);
}

}  // namespace cursive0::analysis
