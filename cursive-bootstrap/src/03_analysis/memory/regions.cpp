#include "cursive0/03_analysis/memory/regions.h"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "cursive0/00_core/assert_spec.h"
#include "cursive0/03_analysis/resolve/collect_toplevel.h"
#include "cursive0/03_analysis/composite/function_types.h"
#include "cursive0/03_analysis/resolve/scopes.h"
#include "cursive0/03_analysis/resolve/scopes_lookup.h"
#include "cursive0/03_analysis/types/type_equiv.h"
#include "cursive0/03_analysis/types/type_expr.h"
#include "cursive0/03_analysis/types/type_infer.h"
#include "cursive0/03_analysis/types/type_pattern.h"

namespace cursive0::analysis {

namespace {

enum class ProvKind {
  Global,
  Stack,
  Heap,
  Region,
  Bottom,
  Param,
};

struct ProvTag {
  ProvKind kind = ProvKind::Bottom;
  std::size_t scope_id = 0;
  IdKey region;
  std::size_t param_index = 0;
};

struct ProvScope {
  std::size_t id = 0;
  std::unordered_map<IdKey, ProvTag> map;
};

struct RegionEntry {
  IdKey tag;
  IdKey target;
};

struct ProvEnv {
  std::vector<ProvScope> scopes;
  std::vector<RegionEntry> regions;
  std::size_t next_scope_id = 0;
};

struct ProvFlow {
  std::vector<ProvTag> results;
  std::vector<ProvTag> breaks;
  bool break_void = false;
};

struct ProvExprResult {
  bool ok = false;
  std::optional<std::string_view> diag_id;
  std::optional<core::Span> span;
  ProvTag prov;
};

struct ExprProvInfo {
  ProvTag tag;
  std::optional<IdKey> target;
};

using ExprProvTagMap = std::unordered_map<const syntax::Expr*, ExprProvInfo>;

struct ProvStmtResult {
  bool ok = false;
  std::optional<std::string_view> diag_id;
  std::optional<core::Span> span;
  ProvEnv env;
  TypeEnv gamma;
  ProvFlow flow;
};

struct ProvSeqResult {
  bool ok = false;
  std::optional<std::string_view> diag_id;
  std::optional<core::Span> span;
  ProvEnv env;
  TypeEnv gamma;
  ProvFlow flow;
};

struct BreakProvResult {
  bool ok = false;
  std::optional<std::string_view> diag_id;
  std::optional<core::Span> span;
  std::vector<ProvTag> breaks;
  bool break_void = false;
};

static inline void SpecDefsRegions() {
  SPEC_DEF("RegionActiveType", "5.2.17");
  SPEC_DEF("FreshRegion", "5.2.17");
  SPEC_DEF("RegionOptsExpr", "5.2.17");
  SPEC_DEF("RegionBind", "5.2.17");
  SPEC_DEF("InnermostActiveRegion", "5.2.17");
  SPEC_DEF("FrameBind", "5.2.17");
  SPEC_DEF("ProvPlaceJudg", "5.2.17");
  SPEC_DEF("ProvExprJudg", "5.2.17");
  SPEC_DEF("ProvStmtJudg", "5.2.17");
  SPEC_DEF("BlockProvJudg", "5.2.17");
  SPEC_DEF("JoinProv", "5.2.17");
  SPEC_DEF("JoinAllProv", "5.2.17");
  SPEC_DEF("BindProv", "5.2.17");
  SPEC_DEF("StaticBindProv", "5.2.17");
  SPEC_DEF("AssignProvOk", "5.2.17");
  SPEC_DEF("AllocTag", "5.2.17");
}

struct LocalTypeLowerResult {
  bool ok = false;
  std::optional<std::string_view> diag_id;
  TypeRef type;
};

static TypeRef StripPermOnce(const TypeRef& type) {
  if (!type) {
    return type;
  }
  if (const auto* perm = std::get_if<TypePerm>(&type->node)) {
    return perm->base;
  }
  if (const auto* refine = std::get_if<TypeRefine>(&type->node)) {
    return refine->base;
  }
  return type;
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

static LocalTypeLowerResult LocalLowerType(const ScopeContext& ctx,
                                 const std::shared_ptr<syntax::Type>& type) {
  if (!type) {
    return {false, std::nullopt, {}};
  }
  return std::visit(
      [&](const auto& node) -> LocalTypeLowerResult {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::TypePrim>) {
          return {true, std::nullopt, MakeTypePrim(node.name)};
        } else if constexpr (std::is_same_v<T, syntax::TypePermType>) {
          const auto base = LocalLowerType(ctx, node.base);
          if (!base.ok) {
            return base;
          }
          return {true, std::nullopt,
                  MakeTypePerm(LowerPermission(node.perm), base.type)};
        } else if constexpr (std::is_same_v<T, syntax::TypeUnion>) {
          std::vector<TypeRef> members;
          members.reserve(node.types.size());
          for (const auto& member : node.types) {
            const auto lowered = LocalLowerType(ctx, member);
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
            const auto lowered = LocalLowerType(ctx, param.type);
            if (!lowered.ok) {
              return lowered;
            }
            params.push_back(TypeFuncParam{LowerParamMode(param.mode),
                                           lowered.type});
          }
          const auto ret = LocalLowerType(ctx, node.ret);
          if (!ret.ok) {
            return ret;
          }
          return {true, std::nullopt, MakeTypeFunc(std::move(params), ret.type)};
        } else if constexpr (std::is_same_v<T, syntax::TypeTuple>) {
          std::vector<TypeRef> elements;
          elements.reserve(node.elements.size());
          for (const auto& elem : node.elements) {
            const auto lowered = LocalLowerType(ctx, elem);
            if (!lowered.ok) {
              return lowered;
            }
            elements.push_back(lowered.type);
          }
          return {true, std::nullopt, MakeTypeTuple(std::move(elements))};
        } else if constexpr (std::is_same_v<T, syntax::TypeArray>) {
          const auto elem = LocalLowerType(ctx, node.element);
          if (!elem.ok) {
            return elem;
          }
          const auto len = ConstLen(ctx, node.length);
          if (!len.ok || !len.value.has_value()) {
            return {false, len.diag_id, {}};
          }
          return {true, std::nullopt, MakeTypeArray(elem.type, *len.value)};
        } else if constexpr (std::is_same_v<T, syntax::TypeSlice>) {
          const auto elem = LocalLowerType(ctx, node.element);
          if (!elem.ok) {
            return elem;
          }
          return {true, std::nullopt, MakeTypeSlice(elem.type)};
        } else if constexpr (std::is_same_v<T, syntax::TypePtr>) {
          const auto elem = LocalLowerType(ctx, node.element);
          if (!elem.ok) {
            return elem;
          }
          return {true, std::nullopt,
                  MakeTypePtr(elem.type, LowerPtrState(node.state))};
        } else if constexpr (std::is_same_v<T, syntax::TypeRawPtr>) {
          const auto elem = LocalLowerType(ctx, node.element);
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
          const auto base = LocalLowerType(ctx, node.base);
          if (!base.ok) {
            return base;
          }
          return {true, std::nullopt,
                  MakeTypeRefine(base.type, node.predicate)};
        } else if constexpr (std::is_same_v<T, syntax::TypeModalState>) {
          std::vector<TypeRef> args;
          args.reserve(node.generic_args.size());
          for (const auto& arg : node.generic_args) {
            const auto lowered = LocalLowerType(ctx, arg);
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
              const auto lower_result = LocalLowerType(ctx, arg);
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

static void ParamTypeMap(const ScopeContext& ctx,
                         const std::vector<syntax::Param>& params,
                         const std::optional<BindSelfParam>& self_param,
                         TypeEnv& env) {
  TypeRef self_base;
  if (self_param.has_value()) {
    self_base = StripPermOnce(self_param->type);
    if (env.scopes.empty()) {
      env.scopes.emplace_back();
    }
    env.scopes.back()[IdKeyOf("self")] =
        TypeBinding{syntax::Mutability::Let, self_param->type};
  }
  for (const auto& param : params) {
    const auto lowered = LocalLowerType(ctx, param.type);
    if (!lowered.ok) {
      continue;
    }
    TypeRef type = lowered.type;
    if (self_base) {
      type = SubstSelfType(self_base, type);
    }
    if (env.scopes.empty()) {
      env.scopes.emplace_back();
    }
    env.scopes.back()[IdKeyOf(param.name)] =
        TypeBinding{syntax::Mutability::Let, type};
  }
}

static std::vector<std::pair<std::string, TypeRef>> PatternBindings(
    const ScopeContext& ctx,
    const syntax::PatternPtr& pat,
    const TypeRef& expected) {
  std::vector<std::pair<std::string, TypeRef>> bindings;
  if (!pat || !expected) {
    return bindings;
  }
  const auto typed = TypePattern(ctx, pat, expected);
  if (typed.ok) {
    bindings = typed.bindings;
    return bindings;
  }
  const auto names = PatNames(pat);
  bindings.reserve(names.size());
  for (const auto& name : names) {
    bindings.emplace_back(name, expected);
  }
  return bindings;
}

static void AddBindingsToTypeEnv(TypeEnv& env,
                                 const std::vector<std::pair<std::string, TypeRef>>& binds,
                                 syntax::Mutability mut,
                                 bool shadow) {
  if (env.scopes.empty()) {
    env.scopes.emplace_back();
  }
  for (const auto& [name, type] : binds) {
    const auto key = IdKeyOf(name);
    if (shadow) {
      bool updated = false;
      for (auto it = env.scopes.rbegin(); it != env.scopes.rend(); ++it) {
        const auto found = it->find(key);
        if (found != it->end()) {
          found->second = TypeBinding{mut, type};
          updated = true;
          break;
        }
      }
      if (!updated) {
        env.scopes.back()[key] = TypeBinding{mut, type};
      }
    } else {
      env.scopes.back()[key] = TypeBinding{mut, type};
    }
  }
}

static std::optional<TypeRef> BindingType(const ScopeContext& ctx,
                                          const syntax::Binding& binding,
                                          const TypeEnv& env) {
  if (binding.type_opt) {
    const auto lowered = LocalLowerType(ctx, binding.type_opt);
    if (!lowered.ok) {
      return std::nullopt;
    }
    return lowered.type;
  }

  StmtTypeContext type_ctx;
  type_ctx.return_type = {};
  type_ctx.loop_flag = LoopFlag::None;
  type_ctx.in_unsafe = false;
  type_ctx.diags = nullptr;

  auto type_expr = [&](const syntax::ExprPtr& expr) {
    return TypeExpr(ctx, type_ctx, expr, env);
  };
  auto type_ident = [&](std::string_view name) -> ExprTypeResult {
    return TypeIdentifierExpr(ctx, syntax::IdentifierExpr{std::string(name)},
                              env);
  };
  auto type_place = [&](const syntax::ExprPtr& expr) {
    return TypePlace(ctx, type_ctx, expr, env);
  };

  const auto inferred = InferExpr(ctx, binding.init, type_expr, type_place, type_ident);
  if (!inferred.ok) {
    return std::nullopt;
  }
  return inferred.type;
}

static std::optional<TypeRef> ShadowBindingType(const ScopeContext& ctx,
                                                const syntax::ExprPtr& init,
                                                const std::shared_ptr<syntax::Type>& type_opt,
                                                const TypeEnv& env) {
  if (type_opt) {
    const auto lowered = LocalLowerType(ctx, type_opt);
    if (!lowered.ok) {
      return std::nullopt;
    }
    return lowered.type;
  }

  StmtTypeContext type_ctx;
  type_ctx.return_type = {};
  type_ctx.loop_flag = LoopFlag::None;
  type_ctx.in_unsafe = false;
  type_ctx.diags = nullptr;

  auto type_expr = [&](const syntax::ExprPtr& expr) {
    return TypeExpr(ctx, type_ctx, expr, env);
  };
  auto type_ident = [&](std::string_view name) -> ExprTypeResult {
    return TypeIdentifierExpr(ctx, syntax::IdentifierExpr{std::string(name)},
                              env);
  };
  auto type_place = [&](const syntax::ExprPtr& expr) {
    return TypePlace(ctx, type_ctx, expr, env);
  };

  const auto inferred = InferExpr(ctx, init, type_expr, type_place, type_ident);
  if (!inferred.ok) {
    return std::nullopt;
  }
  return inferred.type;
}

static const syntax::ASTModule* FindModuleByPath(
    const ScopeContext& ctx,
    const syntax::ModulePath& path) {
  for (const auto& mod : ctx.sigma.mods) {
    if (PathEq(mod.path, path)) {
      return &mod;
    }
  }
  return nullptr;
}

struct StaticBindingInfo {
  std::string name;
  TypeRef type;
  syntax::Mutability mut = syntax::Mutability::Let;
};

static std::vector<StaticBindingInfo> StaticBindings(
    const ScopeContext& ctx,
    const syntax::ModulePath& module_path) {
  std::vector<StaticBindingInfo> bindings;
  const auto* module = FindModuleByPath(ctx, module_path);
  if (!module) {
    return bindings;
  }
  for (const auto& item : module->items) {
    const auto* decl = std::get_if<syntax::StaticDecl>(&item);
    if (!decl) {
      continue;
    }
    if (!decl->binding.type_opt) {
      continue;
    }
    const auto ann = LocalLowerType(ctx, decl->binding.type_opt);
    if (!ann.ok) {
      continue;
    }
    const auto pat_bindings = PatternBindings(ctx, decl->binding.pat, ann.type);
    for (const auto& [name, type] : pat_bindings) {
      bindings.push_back(StaticBindingInfo{name, type, decl->mut});
    }
  }
  return bindings;
}

static ProvTag BottomTag() {
  return ProvTag{ProvKind::Bottom, 0, IdKey{}, 0};
}

static ProvTag GlobalTag() {
  return ProvTag{ProvKind::Global, 0, IdKey{}, 0};
}

static ProvTag StackTag(std::size_t scope_id) {
  return ProvTag{ProvKind::Stack, scope_id, IdKey{}, 0};
}

static ProvTag HeapTag() {
  return ProvTag{ProvKind::Heap, 0, IdKey{}, 0};
}

static ProvTag RegionTag(const IdKey& name) {
  return ProvTag{ProvKind::Region, 0, name, 0};
}

static ProvTag ParamTag(std::size_t idx) {
  return ProvTag{ProvKind::Param, 0, IdKey{}, idx};
}

static bool ProvEq(const ProvTag& lhs, const ProvTag& rhs) {
  if (lhs.kind != rhs.kind) {
    return false;
  }
  switch (lhs.kind) {
    case ProvKind::Stack:
      return lhs.scope_id == rhs.scope_id;
    case ProvKind::Region:
      return lhs.region == rhs.region;
    case ProvKind::Param:
      return lhs.param_index == rhs.param_index;
    default:
      return true;
  }
}

static std::optional<std::size_t> RegionIndex(const ProvEnv& env,
                                              const IdKey& name) {
  for (std::size_t i = 0; i < env.regions.size(); ++i) {
    if (env.regions[i].tag == name) {
      return i;
    }
  }
  return std::nullopt;
}

static bool RegionNesting(const ProvEnv& env,
                          const IdKey& inner,
                          const IdKey& outer) {
  const auto inner_idx = RegionIndex(env, inner);
  const auto outer_idx = RegionIndex(env, outer);
  if (!inner_idx.has_value() || !outer_idx.has_value()) {
    return false;
  }
  return *inner_idx > *outer_idx;
}

static bool ProvLess(const ProvEnv& env, const ProvTag& lhs, const ProvTag& rhs) {
  if (lhs.kind == ProvKind::Param || rhs.kind == ProvKind::Param) {
    return false;
  }
  if (lhs.kind == ProvKind::Region && rhs.kind == ProvKind::Region) {
    return RegionNesting(env, lhs.region, rhs.region);
  }
  if (lhs.kind == ProvKind::Region && rhs.kind == ProvKind::Stack) {
    return true;
  }
  if (lhs.kind == ProvKind::Stack && rhs.kind == ProvKind::Heap) {
    return true;
  }
  if (lhs.kind == ProvKind::Heap && rhs.kind == ProvKind::Global) {
    return true;
  }
  if (lhs.kind == ProvKind::Global && rhs.kind == ProvKind::Bottom) {
    return true;
  }
  return false;
}

static int ProvRank(const ProvTag& tag) {
  switch (tag.kind) {
    case ProvKind::Region:
      return 0;
    case ProvKind::Stack:
      return 1;
    case ProvKind::Heap:
      return 2;
    case ProvKind::Global:
      return 3;
    case ProvKind::Bottom:
      return 4;
    case ProvKind::Param:
      return -1;
  }
  return -1;
}

static bool ProvLeq(const ProvEnv& env, const ProvTag& lhs, const ProvTag& rhs) {
  if (ProvEq(lhs, rhs)) {
    return true;
  }
  if (lhs.kind == ProvKind::Param || rhs.kind == ProvKind::Param) {
    return false;
  }
  if (lhs.kind == ProvKind::Region && rhs.kind == ProvKind::Region) {
    return RegionNesting(env, lhs.region, rhs.region);
  }
  const int lhs_rank = ProvRank(lhs);
  const int rhs_rank = ProvRank(rhs);
  if (lhs_rank < 0 || rhs_rank < 0) {
    return false;
  }
  return lhs_rank < rhs_rank;
}

static ProvTag JoinProv(const ProvEnv& env, const ProvTag& lhs,
                        const ProvTag& rhs) {
  SpecDefsRegions();
  if (ProvLeq(env, lhs, rhs)) {
    return lhs;
  }
  if (ProvLeq(env, rhs, lhs)) {
    return rhs;
  }
  return BottomTag();
}

static ProvTag JoinAllProv(const ProvEnv& env, const std::vector<ProvTag>& tags) {
  SpecDefsRegions();
  if (tags.empty()) {
    return BottomTag();
  }
  ProvTag current = tags.front();
  for (std::size_t i = 1; i < tags.size(); ++i) {
    current = JoinProv(env, current, tags[i]);
  }
  return current;
}

static ProvEnv PushScope_pi(const ProvEnv& env) {
  ProvEnv out = env;
  ProvScope scope;
  scope.id = out.next_scope_id++;
  out.scopes.push_back(std::move(scope));
  return out;
}

static ProvEnv PopScope_pi(const ProvEnv& env) {
  if (env.scopes.empty()) {
    return env;
  }
  ProvEnv out = env;
  out.scopes.pop_back();
  return out;
}

static std::optional<ProvTag> Lookup_pi(const ProvEnv& env, std::string_view name) {
  const auto key = IdKeyOf(name);
  for (auto it = env.scopes.rbegin(); it != env.scopes.rend(); ++it) {
    const auto found = it->map.find(key);
    if (found != it->map.end()) {
      return found->second;
    }
  }
  return std::nullopt;
}

static ProvEnv Intro_pi(const ProvEnv& env, std::string_view name,
                        const ProvTag& tag) {
  ProvEnv out = env;
  if (out.scopes.empty()) {
    return out;
  }
  out.scopes.back().map[IdKeyOf(name)] = tag;
  return out;
}

static ProvEnv ShadowIntro_pi(const ProvEnv& env, std::string_view name,
                              const ProvTag& tag) {
  ProvEnv out = env;
  const auto key = IdKeyOf(name);
  for (auto it = out.scopes.rbegin(); it != out.scopes.rend(); ++it) {
    const auto found = it->map.find(key);
    if (found != it->map.end()) {
      found->second = tag;
      return out;
    }
  }
  if (!out.scopes.empty()) {
    out.scopes.back().map[key] = tag;
  }
  return out;
}

static ProvEnv IntroAll_pi(const ProvEnv& env,
                           const std::vector<std::string>& names,
                           const ProvTag& tag) {
  ProvEnv current = env;
  for (const auto& name : names) {
    current = Intro_pi(current, name, tag);
  }
  return current;
}

static ProvEnv ShadowAll_pi(const ProvEnv& env,
                            const std::vector<std::string>& names,
                            const ProvTag& tag) {
  ProvEnv current = env;
  for (const auto& name : names) {
    current = ShadowIntro_pi(current, name, tag);
  }
  return current;
}

static ProvTag StackProv(const ProvEnv& env) {
  SpecDefsRegions();
  if (env.scopes.empty()) {
    return BottomTag();
  }
  return StackTag(env.scopes.back().id);
}

static ProvTag BindProv(const ProvEnv& env, const ProvTag& init) {
  SpecDefsRegions();
  if (init.kind == ProvKind::Bottom) {
    return StackProv(env);
  }
  return init;
}

static ProvEnv InitProvEnv(const std::vector<std::string>& params,
                           const std::vector<ProvTag>& tags,
                           const std::vector<RegionEntry>& regions) {
  ProvEnv env;
  ProvScope scope;
  scope.id = env.next_scope_id++;
  for (std::size_t i = 0; i < params.size() && i < tags.size(); ++i) {
    scope.map.emplace(IdKeyOf(params[i]), tags[i]);
  }
  env.scopes.push_back(std::move(scope));
  env.regions = regions;
  return env;
}

static std::optional<IdKey> AllocTag(const ProvEnv& env,
                                     const std::optional<std::string>& target) {
  SpecDefsRegions();
  if (!target.has_value()) {
    if (env.regions.empty()) {
      return std::nullopt;
    }
    return env.regions.back().tag;
  }
  const auto key = IdKeyOf(*target);
  for (auto it = env.regions.rbegin(); it != env.regions.rend(); ++it) {
    if (it->target == key) {
      return it->tag;
    }
  }
  return std::nullopt;
}

static std::optional<std::string> InnermostActiveRegionName(
    const TypeEnv& env) {
  const auto region = InnermostActiveRegion(env);
  if (!region.has_value()) {
    return std::nullopt;
  }
  return *region;
}

static ProvTag FrameProv(const TypeEnv& gamma, const ProvEnv& env) {
  SpecDefsRegions();
  const auto region = InnermostActiveRegionName(gamma);
  if (!region.has_value()) {
    return StackProv(env);
  }
  return RegionTag(IdKeyOf(*region));
}

static std::optional<AsyncSig> AsyncSigForExpr(const ScopeContext& ctx,
                                               const TypeEnv& env,
                                               const syntax::ExprPtr& expr) {
  if (!expr) {
    return std::nullopt;
  }
  StmtTypeContext type_ctx;
  type_ctx.return_type = {};
  type_ctx.loop_flag = LoopFlag::None;
  type_ctx.in_unsafe = false;
  type_ctx.diags = nullptr;

  const auto expr_type = TypeExpr(ctx, type_ctx, expr, env);
  if (!expr_type.ok || !expr_type.type) {
    return std::nullopt;
  }
  return AsyncSigOf(ctx, expr_type.type);
}

static bool AsyncCreateExpr(const ScopeContext& ctx,
                            const TypeEnv& env,
                            const syntax::ExprPtr& expr) {
  if (!expr) {
    return false;
  }
  const bool is_async_form =
      std::holds_alternative<syntax::CallExpr>(expr->node) ||
      std::holds_alternative<syntax::MethodCallExpr>(expr->node) ||
      std::holds_alternative<syntax::RaceExpr>(expr->node);
  if (!is_async_form) {
    return false;
  }
  return AsyncSigForExpr(ctx, env, expr).has_value();
}

static std::vector<syntax::ExprPtr> AsyncCaptureArgs(
    const syntax::ExprPtr& expr) {
  std::vector<syntax::ExprPtr> out;
  if (!expr) {
    return out;
  }
  std::visit(
      [&](const auto& node) {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::CallExpr>) {
          for (const auto& arg : node.args) {
            out.push_back(arg.value);
          }
        } else if constexpr (std::is_same_v<T, syntax::MethodCallExpr>) {
          out.push_back(node.receiver);
          for (const auto& arg : node.args) {
            out.push_back(arg.value);
          }
        } else if constexpr (std::is_same_v<T, syntax::RaceExpr>) {
          for (const auto& arm : node.arms) {
            out.push_back(arm.expr);
          }
        }
      },
      expr->node);
  return out;
}

static std::optional<TypeRef> LookupTypeForRegion(const ScopeContext& ctx,
                                                  const TypeEnv& env,
                                                  std::string_view name) {
  const auto binding = BindOf(env, name);
  if (binding.has_value()) {
    return binding->type;
  }
  const auto ent = ResolveValueName(ctx, name);
  if (ent.has_value() && ent->origin_opt.has_value()) {
    const auto target_name = ent->target_opt.value_or(std::string(name));
    const auto value_type = ValuePathType(ctx, *ent->origin_opt, target_name);
    if (value_type.ok && value_type.type) {
      return value_type.type;
    }
  }
  return std::nullopt;
}

static std::vector<syntax::ExprPtr> ChildrenLtr(const syntax::ExprPtr& expr) {
  std::vector<syntax::ExprPtr> out;
  if (!expr) {
    return out;
  }
  std::visit(
      [&](const auto& node) {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::QualifiedApplyExpr>) {
          if (const auto* paren = std::get_if<syntax::ParenArgs>(&node.args)) {
            for (const auto& arg : paren->args) {
              out.push_back(arg.value);
            }
          } else if (const auto* brace =
                         std::get_if<syntax::BraceArgs>(&node.args)) {
            for (const auto& field : brace->fields) {
              out.push_back(field.value);
            }
          }
        } else if constexpr (std::is_same_v<T, syntax::RangeExpr>) {
          if (node.lhs) {
            out.push_back(node.lhs);
          }
          if (node.rhs) {
            out.push_back(node.rhs);
          }
        } else if constexpr (std::is_same_v<T, syntax::BinaryExpr>) {
          out.push_back(node.lhs);
          out.push_back(node.rhs);
        } else if constexpr (std::is_same_v<T, syntax::CastExpr>) {
          out.push_back(node.value);
        } else if constexpr (std::is_same_v<T, syntax::UnaryExpr>) {
          out.push_back(node.value);
        } else if constexpr (std::is_same_v<T, syntax::DerefExpr>) {
          out.push_back(node.value);
        } else if constexpr (std::is_same_v<T, syntax::AddressOfExpr>) {
          out.push_back(node.place);
        } else if constexpr (std::is_same_v<T, syntax::MoveExpr>) {
          out.push_back(node.place);
        } else if constexpr (std::is_same_v<T, syntax::AllocExpr>) {
          out.push_back(node.value);
        } else if constexpr (std::is_same_v<T, syntax::TupleExpr>) {
          for (const auto& elem : node.elements) {
            out.push_back(elem);
          }
        } else if constexpr (std::is_same_v<T, syntax::ArrayExpr>) {
          for (const auto& elem : node.elements) {
            out.push_back(elem);
          }
        } else if constexpr (std::is_same_v<T, syntax::ArrayRepeatExpr>) {
          out.push_back(node.value);
          out.push_back(node.count);
        } else if constexpr (std::is_same_v<T, syntax::SizeofExpr>) {
          // sizeof(type) has no runtime sub-expressions
        } else if constexpr (std::is_same_v<T, syntax::AlignofExpr>) {
          // alignof(type) has no runtime sub-expressions
        } else if constexpr (std::is_same_v<T, syntax::RecordExpr>) {
          for (const auto& field : node.fields) {
            out.push_back(field.value);
          }
        } else if constexpr (std::is_same_v<T, syntax::EnumLiteralExpr>) {
          if (node.payload_opt.has_value()) {
            if (const auto* tuple =
                    std::get_if<syntax::EnumPayloadParen>(&*node.payload_opt)) {
              for (const auto& elem : tuple->elements) {
                out.push_back(elem);
              }
            } else if (const auto* rec =
                           std::get_if<syntax::EnumPayloadBrace>(&*node.payload_opt)) {
              for (const auto& field : rec->fields) {
                out.push_back(field.value);
              }
            }
          }
        } else if constexpr (std::is_same_v<T, syntax::TransmuteExpr>) {
          out.push_back(node.value);
        } else if constexpr (std::is_same_v<T, syntax::FieldAccessExpr>) {
          out.push_back(node.base);
        } else if constexpr (std::is_same_v<T, syntax::TupleAccessExpr>) {
          out.push_back(node.base);
        } else if constexpr (std::is_same_v<T, syntax::IndexAccessExpr>) {
          out.push_back(node.base);
          out.push_back(node.index);
        } else if constexpr (std::is_same_v<T, syntax::CallExpr>) {
          out.push_back(node.callee);
          for (const auto& arg : node.args) {
            out.push_back(arg.value);
          }
        } else if constexpr (std::is_same_v<T, syntax::MethodCallExpr>) {
          out.push_back(node.receiver);
          for (const auto& arg : node.args) {
            out.push_back(arg.value);
          }
        } else if constexpr (std::is_same_v<T, syntax::PropagateExpr>) {
          out.push_back(node.value);
        }
      },
      expr->node);
  return out;
}

static std::optional<IdKey> ResolveRegionTarget(const ProvEnv& env,
                                                const ProvTag& tag) {
  if (tag.kind != ProvKind::Region) {
    return std::nullopt;
  }
  for (const auto& entry : env.regions) {
    if (entry.tag == tag.region) {
      return entry.target;
    }
  }
  return std::nullopt;
}

static void RecordExprProv(const syntax::ExprPtr& expr,
                           const ProvExprResult& res,
                           const ProvEnv& env,
                           ExprProvTagMap* map) {
  if (!map || !expr) {
    return;
  }
  if (!res.ok) {
    return;
  }
  ExprProvInfo info;
  info.tag = res.prov;
  info.target = ResolveRegionTarget(env, res.prov);
  (*map)[expr.get()] = std::move(info);
}

static ProvExprResult ProvExpr(const ScopeContext& ctx,
                               const syntax::ExprPtr& expr,
                               const ProvEnv& env,
                               const TypeEnv& gamma,
                               ExprProvTagMap* expr_map);

static ProvExprResult ProvPlace(const ScopeContext& ctx,
                                const syntax::ExprPtr& place,
                                const ProvEnv& env,
                                const TypeEnv& gamma,
                                ExprProvTagMap* expr_map) {
  (void)ctx;
  (void)expr_map;
  ProvExprResult result;
  if (!place) {
    result.ok = true;
    result.prov = BottomTag();
    return result;
  }

  return std::visit(
      [&](const auto& node) -> ProvExprResult {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::IdentifierExpr>) {
          const auto lookup = Lookup_pi(env, node.name);
          result.ok = true;
          result.prov = lookup.value_or(BottomTag());
          return result;
        } else if constexpr (std::is_same_v<T, syntax::FieldAccessExpr>) {
          auto base = ProvPlace(ctx, node.base, env, gamma, expr_map);
          if (!base.ok) {
            return base;
          }
          SPEC_RULE("P-Field");
          base.prov = base.prov;
          return base;
        } else if constexpr (std::is_same_v<T, syntax::TupleAccessExpr>) {
          auto base = ProvPlace(ctx, node.base, env, gamma, expr_map);
          if (!base.ok) {
            return base;
          }
          SPEC_RULE("P-Tuple");
          base.prov = base.prov;
          return base;
        } else if constexpr (std::is_same_v<T, syntax::IndexAccessExpr>) {
          auto base = ProvPlace(ctx, node.base, env, gamma, expr_map);
          if (!base.ok) {
            return base;
          }
          SPEC_RULE("P-Index");
          base.prov = base.prov;
          return base;
        } else if constexpr (std::is_same_v<T, syntax::DerefExpr>) {
          auto inner = ProvExpr(ctx, node.value, env, gamma, expr_map);
          if (!inner.ok) {
            return inner;
          }
          SPEC_RULE("P-Deref");
          return inner;
        } else {
          ProvExprResult out;
          out.ok = true;
          out.prov = BottomTag();
          return out;
        }
      },
      place->node);
}

static ProvTag LoopProv(const ProvEnv& env,
                        const std::vector<ProvTag>& brk,
                        bool brk_void) {
  if (brk.empty()) {
    return BottomTag();
  }
  if (brk_void) {
    return BottomTag();
  }
  return JoinAllProv(env, brk);
}

static ProvExprResult BlockProv(const ScopeContext& ctx,
                                const syntax::Block& block,
                                const ProvEnv& env,
                                const TypeEnv& gamma,
                                ExprProvTagMap* expr_map);

static ProvStmtResult ProvStmt(const ScopeContext& ctx,
                               const syntax::Stmt& stmt,
                               const ProvEnv& env,
                               const TypeEnv& gamma,
                               ExprProvTagMap* expr_map);

static ProvSeqResult ProvStmtSeq(const ScopeContext& ctx,
                                 const std::vector<syntax::Stmt>& stmts,
                                 const ProvEnv& env,
                                 const TypeEnv& gamma,
                                 ExprProvTagMap* expr_map) {
  ProvSeqResult result;
  if (stmts.empty()) {
    SPEC_RULE("Prov-Seq-Empty");
    result.ok = true;
    result.env = env;
    result.gamma = gamma;
    return result;
  }

  const auto& head = stmts.front();
  std::vector<syntax::Stmt> tail(stmts.begin() + 1, stmts.end());
  const auto head_res = ProvStmt(ctx, head, env, gamma, expr_map);
  if (!head_res.ok) {
    result.ok = false;
    result.diag_id = head_res.diag_id;
    result.span = head_res.span;
    return result;
  }

  const auto tail_res =
      ProvStmtSeq(ctx, tail, head_res.env, head_res.gamma, expr_map);
  if (!tail_res.ok) {
    return tail_res;
  }

  SPEC_RULE("Prov-Seq-Cons");
  result.ok = true;
  result.env = tail_res.env;
  result.gamma = tail_res.gamma;
  result.flow.results = head_res.flow.results;
  result.flow.results.insert(result.flow.results.end(),
                             tail_res.flow.results.begin(),
                             tail_res.flow.results.end());
  result.flow.breaks = head_res.flow.breaks;
  result.flow.breaks.insert(result.flow.breaks.end(),
                            tail_res.flow.breaks.begin(),
                            tail_res.flow.breaks.end());
  result.flow.break_void = head_res.flow.break_void || tail_res.flow.break_void;
  return result;
}

static ProvExprResult BlockProv(const ScopeContext& ctx,
                                const syntax::Block& block,
                                const ProvEnv& env,
                                const TypeEnv& gamma,
                                ExprProvTagMap* expr_map) {
  ProvExprResult result;
  ProvEnv inner_env = PushScope_pi(env);
  TypeEnv inner_gamma = PushScope(gamma);
  const auto seq = ProvStmtSeq(ctx, block.stmts, inner_env, inner_gamma, expr_map);
  if (!seq.ok) {
    result.ok = false;
    result.diag_id = seq.diag_id;
    result.span = seq.span;
    return result;
  }

  std::optional<ProvTag> tail_prov;
  if (block.tail_opt) {
    const auto tail = ProvExpr(ctx, block.tail_opt, seq.env, seq.gamma, expr_map);
    if (!tail.ok) {
      return tail;
    }
    tail_prov = tail.prov;
  }

  if (!seq.flow.results.empty()) {
    SPEC_RULE("BlockProv-Res");
    result.ok = true;
    result.prov = JoinAllProv(seq.env, seq.flow.results);
    return result;
  }

  if (block.tail_opt && tail_prov.has_value()) {
    SPEC_RULE("BlockProv-Tail");
    result.ok = true;
    result.prov = *tail_prov;
    return result;
  }

  SPEC_RULE("BlockProv-Unit");
  result.ok = true;
  result.prov = BottomTag();
  return result;
}

static BreakProvResult BreakProv(const ScopeContext& ctx,
                                 const syntax::Block& body,
                                 const ProvEnv& env,
                                 const TypeEnv& gamma) {
  BreakProvResult result;
  ProvEnv inner_env = PushScope_pi(env);
  TypeEnv inner_gamma = PushScope(gamma);
  const auto seq = ProvStmtSeq(ctx, body.stmts, inner_env, inner_gamma, nullptr);
  if (!seq.ok) {
    result.ok = false;
    result.diag_id = seq.diag_id;
    result.span = seq.span;
    return result;
  }
  if (body.tail_opt) {
    const auto tail = ProvExpr(ctx, body.tail_opt, seq.env, seq.gamma, nullptr);
    if (!tail.ok) {
      result.ok = false;
      result.diag_id = tail.diag_id;
      result.span = tail.span;
      return result;
    }
  }
  result.ok = true;
  result.breaks = seq.flow.breaks;
  result.break_void = seq.flow.break_void;
  return result;
}

static ProvExprResult ProvExpr(const ScopeContext& ctx,
                               const syntax::ExprPtr& expr,
                               const ProvEnv& env,
                               const TypeEnv& gamma,
                               ExprProvTagMap* expr_map) {
  auto finish = [&](ProvExprResult res) -> ProvExprResult {
    RecordExprProv(expr, res, env, expr_map);
    return res;
  };
  ProvExprResult result;
  if (!expr) {
    result.ok = true;
    result.prov = BottomTag();
    return finish(result);
  }

  if (const auto* literal = std::get_if<syntax::LiteralExpr>(&expr->node)) {
    (void)literal;
    SPEC_RULE("P-Literal");
    result.ok = true;
    result.prov = BottomTag();
    return finish(result);
  }

  if (const auto* move = std::get_if<syntax::MoveExpr>(&expr->node)) {
    auto inner = ProvPlace(ctx, move->place, env, gamma, expr_map);
    if (!inner.ok) {
      return finish(inner);
    }
    SPEC_RULE("P-Move");
    inner.prov = inner.prov;
    return finish(inner);
  }

  if (const auto* addr = std::get_if<syntax::AddressOfExpr>(&expr->node)) {
    auto inner = ProvPlace(ctx, addr->place, env, gamma, expr_map);
    if (!inner.ok) {
      return finish(inner);
    }
    SPEC_RULE("P-AddrOf");
    inner.prov = inner.prov;
    return finish(inner);
  }

  if (const auto* alloc = std::get_if<syntax::AllocExpr>(&expr->node)) {
    auto inner = ProvExpr(ctx, alloc->value, env, gamma, expr_map);
    if (!inner.ok) {
      return finish(inner);
    }
    const auto tag = AllocTag(env, alloc->region_opt);
    SPEC_RULE("P-Alloc");
    result.ok = true;
    if (tag.has_value()) {
      result.prov = RegionTag(*tag);
      return finish(result);
    }
    result.prov = BottomTag();
    return finish(result);
  }

  if (const auto* call = std::get_if<syntax::MethodCallExpr>(&expr->node)) {
    if (IdEq(call->name, "alloc")) {
      if (const auto* recv = std::get_if<syntax::IdentifierExpr>(&call->receiver->node)) {
        const auto type = LookupTypeForRegion(ctx, gamma, recv->name);
        if (type.has_value() && RegionActiveType(*type)) {
          for (const auto& arg : call->args) {
            const auto arg_res = ProvExpr(ctx, arg.value, env, gamma, expr_map);
            if (!arg_res.ok) {
              return finish(arg_res);
            }
          }
          const auto tag = AllocTag(env, std::optional<std::string>(recv->name));
          SPEC_RULE("P-Region-Alloc-Method");
          result.ok = true;
          result.prov = tag.has_value() ? RegionTag(*tag) : BottomTag();
          return finish(result);
        }
      }
    }
  }

  if (const auto* if_expr = std::get_if<syntax::IfExpr>(&expr->node)) {
    const auto cond = ProvExpr(ctx, if_expr->cond, env, gamma, expr_map);
    if (!cond.ok) {
      return finish(cond);
    }
    const auto then_res = ProvExpr(ctx, if_expr->then_expr, env, gamma, expr_map);
    if (!then_res.ok) {
      return finish(then_res);
    }
    if (if_expr->else_expr) {
      const auto else_res = ProvExpr(ctx, if_expr->else_expr, env, gamma, expr_map);
      if (!else_res.ok) {
        return finish(else_res);
      }
      SPEC_RULE("P-If");
      result.ok = true;
      result.prov = JoinProv(env, then_res.prov, else_res.prov);
      return finish(result);
    }
    SPEC_RULE("P-If-No-Else");
    result.ok = true;
    result.prov = BottomTag();
    return finish(result);
  }

  if (const auto* match = std::get_if<syntax::MatchExpr>(&expr->node)) {
    const auto scrut = ProvExpr(ctx, match->value, env, gamma, expr_map);
    if (!scrut.ok) {
      return finish(scrut);
    }
    std::vector<ProvTag> arm_provs;
    arm_provs.reserve(match->arms.size());
    for (const auto& arm : match->arms) {
      if (!arm.pattern) {
        continue;
      }
      const auto names = PatNames(*arm.pattern);
      std::vector<std::string> bind_names;
      bind_names.reserve(names.size());
      for (const auto& name : names) {
        bind_names.push_back(name);
      }
      ProvEnv arm_env = env;
      const auto bind_pi = BindProv(arm_env, BottomTag());
      arm_env = IntroAll_pi(arm_env, bind_names, bind_pi);
      if (arm.guard_opt) {
        const auto guard = ProvExpr(ctx, arm.guard_opt, arm_env, gamma, expr_map);
        if (!guard.ok) {
          return finish(guard);
        }
      }
      const auto body = ProvExpr(ctx, arm.body, arm_env, gamma, expr_map);
      if (!body.ok) {
        return finish(body);
      }
      arm_provs.push_back(body.prov);
    }
    SPEC_RULE("P-Match");
    result.ok = true;
    result.prov = JoinAllProv(env, arm_provs);
    return finish(result);
  }

  if (const auto* block_expr = std::get_if<syntax::BlockExpr>(&expr->node)) {
    if (!block_expr->block) {
      result.ok = true;
      result.prov = BottomTag();
      return finish(result);
    }
    SPEC_RULE("P-Block");
    return finish(BlockProv(ctx, *block_expr->block, env, gamma, expr_map));
  }

  if (const auto* unsafe_expr =
          std::get_if<syntax::UnsafeBlockExpr>(&expr->node)) {
    if (!unsafe_expr->block) {
      result.ok = true;
      result.prov = BottomTag();
      return finish(result);
    }
    SPEC_RULE("P-Block");
    return finish(BlockProv(ctx, *unsafe_expr->block, env, gamma, expr_map));
  }

  if (const auto* loop_inf =
          std::get_if<syntax::LoopInfiniteExpr>(&expr->node)) {
    if (!loop_inf->body) {
      result.ok = true;
      result.prov = BottomTag();
      return finish(result);
    }
    const auto prov = BreakProv(ctx, *loop_inf->body, env, gamma);
    if (!prov.ok) {
      result.ok = false;
      result.diag_id = prov.diag_id;
      result.span = prov.span;
      return finish(result);
    }
    SPEC_RULE("P-Loop-Infinite");
    result.ok = true;
    result.prov = LoopProv(env, prov.breaks, prov.break_void);
    return finish(result);
  }

  if (const auto* loop_cond =
          std::get_if<syntax::LoopConditionalExpr>(&expr->node)) {
    const auto cond = ProvExpr(ctx, loop_cond->cond, env, gamma, expr_map);
    if (!cond.ok) {
      return finish(cond);
    }
    if (!loop_cond->body) {
      result.ok = true;
      result.prov = BottomTag();
      return finish(result);
    }
    const auto prov = BreakProv(ctx, *loop_cond->body, env, gamma);
    if (!prov.ok) {
      result.ok = false;
      result.diag_id = prov.diag_id;
      result.span = prov.span;
      return finish(result);
    }
    SPEC_RULE("P-Loop-Conditional");
    result.ok = true;
    result.prov = LoopProv(env, prov.breaks, prov.break_void);
    return finish(result);
  }

  if (const auto* loop_iter =
          std::get_if<syntax::LoopIterExpr>(&expr->node)) {
    const auto iter = ProvExpr(ctx, loop_iter->iter, env, gamma, expr_map);
    if (!iter.ok) {
      return finish(iter);
    }
    ProvEnv arm_env = env;
    std::vector<std::string> bind_names;
    if (loop_iter->pattern) {
      const auto names = PatNames(*loop_iter->pattern);
      bind_names.reserve(names.size());
      for (const auto& name : names) {
        bind_names.push_back(name);
      }
    }
    arm_env = IntroAll_pi(arm_env, bind_names, iter.prov);
    if (!loop_iter->body) {
      result.ok = true;
      result.prov = BottomTag();
      return finish(result);
    }
    const auto prov = BreakProv(ctx, *loop_iter->body, arm_env, gamma);
    if (!prov.ok) {
      result.ok = false;
      result.diag_id = prov.diag_id;
      result.span = prov.span;
      return finish(result);
    }
    SPEC_RULE("P-Loop-Iter");
    result.ok = true;
    result.prov = LoopProv(env, prov.breaks, prov.break_void);
    return finish(result);
  }

  if (AsyncCreateExpr(ctx, gamma, expr)) {
    const auto args = AsyncCaptureArgs(expr);
    const auto frame_prov = FrameProv(gamma, env);
    for (const auto& arg : args) {
      const auto arg_res = ProvExpr(ctx, arg, env, gamma, expr_map);
      if (!arg_res.ok) {
        return finish(arg_res);
      }
      if (ProvLess(env, arg_res.prov, frame_prov)) {
        SPEC_RULE("Async-Capture-Err");
        ProvExprResult err;
        err.ok = false;
        err.diag_id = "Async-Capture-Err";
        err.span = expr->span;
        return finish(err);
      }
    }
    SPEC_RULE("P-Async-Create");
    result.ok = true;
    result.prov = frame_prov;
    return finish(result);
  }

  if (IsPlaceExpr(expr)) {
    const auto place = ProvPlace(ctx, expr, env, gamma, expr_map);
    if (!place.ok) {
      return finish(place);
    }
    SPEC_RULE("P-Place-Expr");
    return finish(place);
  }

  const auto children = ChildrenLtr(expr);
  std::vector<ProvTag> child_provs;
  child_provs.reserve(children.size());
  for (const auto& child : children) {
    const auto child_res = ProvExpr(ctx, child, env, gamma, expr_map);
    if (!child_res.ok) {
      return finish(child_res);
    }
    child_provs.push_back(child_res.prov);
  }
  SPEC_RULE("P-Expr-Sub");
  result.ok = true;
  result.prov = JoinAllProv(env, child_provs);
  return finish(result);
}

static ProvStmtResult ProvStmt(const ScopeContext& ctx,
                               const syntax::Stmt& stmt,
                               const ProvEnv& env,
                               const TypeEnv& gamma,
                               ExprProvTagMap* expr_map) {
  ProvStmtResult result;
  result.ok = true;
  result.env = env;
  result.gamma = gamma;

  return std::visit(
      [&](const auto& node) -> ProvStmtResult {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::LetStmt>) {
          const auto init = ProvExpr(ctx, node.binding.init, env, gamma, expr_map);
          if (!init.ok) {
            return {false, init.diag_id, init.span, env, gamma, {}};
          }
          std::vector<std::string> names;
          const auto pat_names = PatNames(node.binding.pat);
          names.reserve(pat_names.size());
          for (const auto& name : pat_names) {
            names.push_back(name);
          }
          const auto bind_pi = BindProv(env, init.prov);
          ProvEnv out_env = IntroAll_pi(env, names, bind_pi);

          TypeEnv out_gamma = gamma;
          const auto bind_type = BindingType(ctx, node.binding, gamma);
          if (bind_type.has_value()) {
            const auto binds = PatternBindings(ctx, node.binding.pat, *bind_type);
            AddBindingsToTypeEnv(out_gamma, binds, syntax::Mutability::Let, false);
          }
          SPEC_RULE("Prov-LetVar");
          return {true, std::nullopt, std::nullopt, std::move(out_env),
                  std::move(out_gamma), {}};
        } else if constexpr (std::is_same_v<T, syntax::VarStmt>) {
          const auto init = ProvExpr(ctx, node.binding.init, env, gamma, expr_map);
          if (!init.ok) {
            return {false, init.diag_id, init.span, env, gamma, {}};
          }
          std::vector<std::string> names;
          const auto pat_names = PatNames(node.binding.pat);
          names.reserve(pat_names.size());
          for (const auto& name : pat_names) {
            names.push_back(name);
          }
          const auto bind_pi = BindProv(env, init.prov);
          ProvEnv out_env = IntroAll_pi(env, names, bind_pi);

          TypeEnv out_gamma = gamma;
          const auto bind_type = BindingType(ctx, node.binding, gamma);
          if (bind_type.has_value()) {
            const auto binds = PatternBindings(ctx, node.binding.pat, *bind_type);
            AddBindingsToTypeEnv(out_gamma, binds, syntax::Mutability::Var, false);
          }
          SPEC_RULE("Prov-LetVar");
          return {true, std::nullopt, std::nullopt, std::move(out_env),
                  std::move(out_gamma), {}};
        } else if constexpr (std::is_same_v<T, syntax::ShadowLetStmt>) {
          const auto init = ProvExpr(ctx, node.init, env, gamma, expr_map);
          if (!init.ok) {
            return {false, init.diag_id, init.span, env, gamma, {}};
          }
          const auto bind_pi = BindProv(env, init.prov);
          ProvEnv out_env =
              ShadowAll_pi(env, std::vector<std::string>{node.name}, bind_pi);
          TypeEnv out_gamma = gamma;
          const auto bind_type =
              ShadowBindingType(ctx, node.init, node.type_opt, gamma);
          if (bind_type.has_value()) {
            AddBindingsToTypeEnv(out_gamma,
                                 std::vector<std::pair<std::string, TypeRef>>{
                                     {node.name, *bind_type}},
                                 syntax::Mutability::Let, true);
          }
          SPEC_RULE("Prov-ShadowLet");
          return {true, std::nullopt, std::nullopt, std::move(out_env),
                  std::move(out_gamma), {}};
        } else if constexpr (std::is_same_v<T, syntax::ShadowVarStmt>) {
          const auto init = ProvExpr(ctx, node.init, env, gamma, expr_map);
          if (!init.ok) {
            return {false, init.diag_id, init.span, env, gamma, {}};
          }
          const auto bind_pi = BindProv(env, init.prov);
          ProvEnv out_env =
              ShadowAll_pi(env, std::vector<std::string>{node.name}, bind_pi);
          TypeEnv out_gamma = gamma;
          const auto bind_type =
              ShadowBindingType(ctx, node.init, node.type_opt, gamma);
          if (bind_type.has_value()) {
            AddBindingsToTypeEnv(out_gamma,
                                 std::vector<std::pair<std::string, TypeRef>>{
                                     {node.name, *bind_type}},
                                 syntax::Mutability::Var, true);
          }
          SPEC_RULE("Prov-ShadowVar");
          return {true, std::nullopt, std::nullopt, std::move(out_env),
                  std::move(out_gamma), {}};
        } else if constexpr (std::is_same_v<T, syntax::AssignStmt>) {
          const auto place = ProvPlace(ctx, node.place, env, gamma, expr_map);
          if (!place.ok) {
            return {false, place.diag_id, place.span, env, gamma, {}};
          }
          const auto value = ProvExpr(ctx, node.value, env, gamma, expr_map);
          if (!value.ok) {
            return {false, value.diag_id, value.span, env, gamma, {}};
          }
          if (ProvLess(env, value.prov, place.prov)) {
            if (AsyncSigForExpr(ctx, gamma, node.value).has_value()) {
              SPEC_RULE("Prov-Async-Escape-Err");
              return {false, "Prov-Async-Escape-Err", node.span, env, gamma, {}};
            }
            SPEC_RULE("Prov-Escape-Err");
            return {false, "Prov-Escape-Err", node.span, env, gamma, {}};
          }
          SPEC_RULE("Prov-Assign");
          return {true, std::nullopt, std::nullopt, env, gamma, {}};
        } else if constexpr (std::is_same_v<T, syntax::CompoundAssignStmt>) {
          const auto place = ProvPlace(ctx, node.place, env, gamma, expr_map);
          if (!place.ok) {
            return {false, place.diag_id, place.span, env, gamma, {}};
          }
          const auto value = ProvExpr(ctx, node.value, env, gamma, expr_map);
          if (!value.ok) {
            return {false, value.diag_id, value.span, env, gamma, {}};
          }
          if (ProvLess(env, value.prov, place.prov)) {
            if (AsyncSigForExpr(ctx, gamma, node.value).has_value()) {
              SPEC_RULE("Prov-Async-Escape-Err");
              return {false, "Prov-Async-Escape-Err", node.span, env, gamma, {}};
            }
            SPEC_RULE("Prov-Escape-Err");
            return {false, "Prov-Escape-Err", node.span, env, gamma, {}};
          }
          SPEC_RULE("Prov-CompoundAssign");
          return {true, std::nullopt, std::nullopt, env, gamma, {}};
        } else if constexpr (std::is_same_v<T, syntax::ExprStmt>) {
          const auto expr = ProvExpr(ctx, node.value, env, gamma, expr_map);
          if (!expr.ok) {
            return {false, expr.diag_id, expr.span, env, gamma, {}};
          }
          SPEC_RULE("Prov-ExprStmt");
          return {true, std::nullopt, std::nullopt, env, gamma, {}};
        } else if constexpr (std::is_same_v<T, syntax::DeferStmt>) {
          if (!node.body) {
            return {true, std::nullopt, std::nullopt, env, gamma, {}};
          }
          const auto body = BlockProv(ctx, *node.body, env, gamma, expr_map);
          if (!body.ok) {
            return {false, body.diag_id, body.span, env, gamma, {}};
          }
          SPEC_RULE("Prov-DeferStmt");
          return {true, std::nullopt, std::nullopt, env, gamma, {}};
        } else if constexpr (std::is_same_v<T, syntax::RegionStmt>) {
          syntax::ExprPtr opts_expr = node.opts_opt;
          if (!opts_expr) {
            auto ident = std::make_shared<syntax::Expr>();
            ident->node = syntax::IdentifierExpr{"RegionOptions"};
            syntax::CallExpr call;
            call.callee = ident;
            call.args = {};
            auto expr_ptr = std::make_shared<syntax::Expr>();
            expr_ptr->node = call;
            opts_expr = expr_ptr;
          }
          const auto opts = ProvExpr(ctx, opts_expr, env, gamma, expr_map);
          if (!opts.ok) {
            return {false, opts.diag_id, opts.span, env, gamma, {}};
          }
          std::string name =
              node.alias_opt.has_value() ? *node.alias_opt
                                         : FreshRegionName(gamma);

          ProvEnv inner_env = PushScope_pi(env);
          const auto bind_pi = BindProv(inner_env, BottomTag());
          inner_env = Intro_pi(inner_env, name, bind_pi);
          inner_env.regions.push_back(RegionEntry{IdKeyOf(name), IdKeyOf(name)});

          TypeEnv inner_gamma = PushScope(gamma);
          if (inner_gamma.scopes.empty()) {
            inner_gamma.scopes.emplace_back();
          }
          inner_gamma.scopes.back()[IdKeyOf(name)] =
              TypeBinding{syntax::Mutability::Let, RegionActiveTypeRef()};

          if (node.body) {
            const auto body =
                BlockProv(ctx, *node.body, inner_env, inner_gamma, expr_map);
            if (!body.ok) {
              return {false, body.diag_id, body.span, env, gamma, {}};
            }
          }
          SPEC_RULE("Prov-RegionStmt");
          return {true, std::nullopt, std::nullopt, env, gamma, {}};
        } else if constexpr (std::is_same_v<T, syntax::FrameStmt>) {
          std::optional<std::string> target = node.target_opt;
          if (!target.has_value()) {
            target = InnermostActiveRegionName(gamma);
          } else {
            const auto type = LookupTypeForRegion(ctx, gamma, *target);
            if (!type.has_value() || !RegionActiveType(*type)) {
              target = std::nullopt;
            }
          }
          if (!target.has_value()) {
            SPEC_RULE("Prov-FrameStmt");
            return {true, std::nullopt, std::nullopt, env, gamma, {}};
          }

          const std::string fresh = FreshRegionName(gamma);
          ProvEnv inner_env = PushScope_pi(env);
          const auto bind_pi = BindProv(inner_env, BottomTag());
          inner_env = Intro_pi(inner_env, fresh, bind_pi);
          inner_env.regions.push_back(
              RegionEntry{IdKeyOf(fresh), IdKeyOf(*target)});

          TypeEnv inner_gamma = PushScope(gamma);
          if (inner_gamma.scopes.empty()) {
            inner_gamma.scopes.emplace_back();
          }
          inner_gamma.scopes.back()[IdKeyOf(fresh)] =
              TypeBinding{syntax::Mutability::Let, RegionActiveTypeRef()};

          if (node.body) {
            const auto body =
                BlockProv(ctx, *node.body, inner_env, inner_gamma, expr_map);
            if (!body.ok) {
              return {false, body.diag_id, body.span, env, gamma, {}};
            }
          }
          SPEC_RULE("Prov-FrameStmt");
          return {true, std::nullopt, std::nullopt, env, gamma, {}};
        } else if constexpr (std::is_same_v<T, syntax::ReturnStmt>) {
          if (!node.value_opt) {
            SPEC_RULE("Prov-Return-Unit");
            return {true, std::nullopt, std::nullopt, env, gamma, {}};
          }
          const auto value = ProvExpr(ctx, node.value_opt, env, gamma, expr_map);
          if (!value.ok) {
            return {false, value.diag_id, value.span, env, gamma, {}};
          }
          SPEC_RULE("Prov-Return");
          ProvFlow flow;
          flow.results.push_back(value.prov);
          return {true, std::nullopt, std::nullopt, env, gamma, std::move(flow)};
        } else if constexpr (std::is_same_v<T, syntax::BreakStmt>) {
          if (!node.value_opt) {
            SPEC_RULE("Prov-Break-Unit");
            ProvFlow flow;
            flow.break_void = true;
            return {true, std::nullopt, std::nullopt, env, gamma, std::move(flow)};
          }
          const auto value = ProvExpr(ctx, node.value_opt, env, gamma, expr_map);
          if (!value.ok) {
            return {false, value.diag_id, value.span, env, gamma, {}};
          }
          SPEC_RULE("Prov-Break");
          ProvFlow flow;
          flow.breaks.push_back(value.prov);
          return {true, std::nullopt, std::nullopt, env, gamma, std::move(flow)};
        } else if constexpr (std::is_same_v<T, syntax::ContinueStmt>) {
          SPEC_RULE("Prov-Continue");
          return {true, std::nullopt, std::nullopt, env, gamma, {}};
        } else if constexpr (std::is_same_v<T, syntax::UnsafeBlockStmt>) {
          if (!node.body) {
            return {true, std::nullopt, std::nullopt, env, gamma, {}};
          }
          const auto body = BlockProv(ctx, *node.body, env, gamma, expr_map);
          if (!body.ok) {
            return {false, body.diag_id, body.span, env, gamma, {}};
          }
          SPEC_RULE("Prov-UnsafeStmt");
          return {true, std::nullopt, std::nullopt, env, gamma, {}};
        } else if constexpr (std::is_same_v<T, syntax::ErrorStmt>) {
          SPEC_RULE("Prov-ErrorStmt");
          return {true, std::nullopt, std::nullopt, env, gamma, {}};
        } else {
          return {true, std::nullopt, std::nullopt, env, gamma, {}};
        }
      },
      stmt);
}

}  // namespace

TypeRef RegionActiveTypeRef() {
  return MakeTypeModalState({"Region"}, "Active");
}

bool RegionActiveType(const TypeRef& type) {
  SpecDefsRegions();
  const auto stripped = StripPermOnce(type);
  if (!stripped) {
    return false;
  }
  const auto* modal = std::get_if<TypeModalState>(&stripped->node);
  if (!modal) {
    return false;
  }
  if (modal->path.size() != 1 || modal->path[0] != "Region") {
    return false;
  }
  return modal->state == "Active";
}

std::optional<std::string> InnermostActiveRegion(const TypeEnv& env) {
  SpecDefsRegions();
  for (auto it = env.scopes.rbegin(); it != env.scopes.rend(); ++it) {
    std::optional<std::string> best;
    for (const auto& [key, binding] : *it) {
      if (!RegionActiveType(binding.type)) {
        continue;
      }
      if (!best.has_value() || key < *best) {
        best = key;
      }
    }
    if (best.has_value()) {
      return best;
    }
  }
  return std::nullopt;
}

std::string FreshRegionName(const TypeEnv& env) {
  SpecDefsRegions();
  for (std::size_t i = 0;; ++i) {
    std::string name = "region$" + std::to_string(i);
    const auto key = IdKeyOf(name);
    bool used = false;
    for (const auto& scope : env.scopes) {
      if (scope.find(key) != scope.end()) {
        used = true;
        break;
      }
    }
    if (!used) {
      return name;
    }
  }
}

ProvCheckResult ProvBindCheck(const ScopeContext& ctx,
                              const syntax::ModulePath& module_path,
                              const std::vector<syntax::Param>& params,
                              const std::shared_ptr<syntax::Block>& body,
                              const std::optional<BindSelfParam>& self_param) {
  SpecDefsRegions();
  ProvCheckResult result;
  if (!body) {
    result.ok = true;
    return result;
  }

  std::vector<std::string> param_names;
  std::vector<ProvTag> param_tags;
  if (self_param.has_value()) {
    param_names.push_back("self");
    param_tags.push_back(ParamTag(param_tags.size()));
  }
  for (const auto& param : params) {
    param_names.push_back(param.name);
    param_tags.push_back(ParamTag(param_tags.size()));
  }

  ProvEnv prov_env = InitProvEnv(param_names, param_tags, {});

  const auto static_bindings = StaticBindings(ctx, module_path);
  if (!static_bindings.empty()) {
    ProvScope static_scope;
    static_scope.id = prov_env.next_scope_id++;
    for (const auto& binding : static_bindings) {
      static_scope.map.emplace(IdKeyOf(binding.name), GlobalTag());
    }
    prov_env.scopes.insert(prov_env.scopes.begin(), std::move(static_scope));
  }

  TypeEnv gamma;
  gamma.scopes.emplace_back();
  if (!static_bindings.empty()) {
    for (const auto& binding : static_bindings) {
      gamma.scopes.front()[IdKeyOf(binding.name)] =
          TypeBinding{binding.mut, binding.type};
    }
  }
  gamma.scopes.emplace_back();
  ParamTypeMap(ctx, params, self_param, gamma);

  const auto block = BlockProv(ctx, *body, prov_env, gamma, nullptr);
  if (!block.ok) {
    result.ok = false;
    result.diag_id = block.diag_id;
    result.span = block.span;
    return result;
  }

  result.ok = true;
  return result;
}

static ProvenanceKind ToPublicKind(const ProvTag& tag) {
  switch (tag.kind) {
    case ProvKind::Global:
      return ProvenanceKind::Global;
    case ProvKind::Stack:
      return ProvenanceKind::Stack;
    case ProvKind::Heap:
      return ProvenanceKind::Heap;
    case ProvKind::Region:
      return ProvenanceKind::Region;
    case ProvKind::Bottom:
      return ProvenanceKind::Bottom;
    case ProvKind::Param:
      return ProvenanceKind::Param;
  }
  return ProvenanceKind::Bottom;
}

ExprProvMapResult ComputeExprProvenanceMap(
    const ScopeContext& ctx,
    const syntax::ModulePath& module_path,
    const std::vector<syntax::Param>& params,
    const std::shared_ptr<syntax::Block>& body,
    const std::optional<BindSelfParam>& self_param) {
  SpecDefsRegions();
  ExprProvMapResult result;
  if (!body) {
    result.ok = true;
    return result;
  }

  std::vector<std::string> param_names;
  std::vector<ProvTag> param_tags;
  if (self_param.has_value()) {
    param_names.push_back("self");
    param_tags.push_back(ParamTag(param_tags.size()));
  }
  for (const auto& param : params) {
    param_names.push_back(param.name);
    param_tags.push_back(ParamTag(param_tags.size()));
  }

  ProvEnv prov_env = InitProvEnv(param_names, param_tags, {});

  const auto static_bindings = StaticBindings(ctx, module_path);
  if (!static_bindings.empty()) {
    ProvScope static_scope;
    static_scope.id = prov_env.next_scope_id++;
    for (const auto& binding : static_bindings) {
      static_scope.map.emplace(IdKeyOf(binding.name), GlobalTag());
    }
    prov_env.scopes.insert(prov_env.scopes.begin(), std::move(static_scope));
  }

  TypeEnv gamma;
  gamma.scopes.emplace_back();
  if (!static_bindings.empty()) {
    for (const auto& binding : static_bindings) {
      gamma.scopes.front()[IdKeyOf(binding.name)] =
          TypeBinding{binding.mut, binding.type};
    }
  }
  gamma.scopes.emplace_back();
  ParamTypeMap(ctx, params, self_param, gamma);

  ExprProvTagMap expr_map;
  const auto block = BlockProv(ctx, *body, prov_env, gamma, &expr_map);
  if (!block.ok) {
    result.ok = false;
    result.diag_id = block.diag_id;
    result.span = block.span;
    return result;
  }

  result.ok = true;
  result.expr_prov.reserve(expr_map.size());
  for (const auto& [expr_ptr, info] : expr_map) {
    result.expr_prov.emplace(expr_ptr, ToPublicKind(info.tag));
    if (info.tag.kind == ProvKind::Region && info.target.has_value()) {
      result.expr_region.emplace(expr_ptr, *info.target);
    }
  }
  return result;
}

}  // namespace cursive0::analysis
