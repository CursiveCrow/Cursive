#include "cursive0/03_analysis/types/type_match.h"

#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

#include "cursive0/00_core/assert_spec.h"
#include "cursive0/03_analysis/resolve/collect_toplevel.h"
#include "cursive0/03_analysis/modal/modal.h"
#include "cursive0/03_analysis/resolve/scopes.h"
#include "cursive0/03_analysis/types/type_equiv.h"
#include "cursive0/03_analysis/types/type_expr.h"
#include "cursive0/03_analysis/types/type_pattern.h"
#include "cursive0/03_analysis/types/types.h"

namespace cursive0::analysis {

namespace {

struct IntroResult {
  bool ok = false;
  std::optional<std::string_view> diag_id;
  TypeEnv env;
};

struct LocalTypeLowerResult {
  bool ok = false;
  std::optional<std::string_view> diag_id;
  TypeRef type;
};

struct AllEqResult {
  bool ok = false;
  std::optional<std::string_view> diag_id;
  bool equal = false;
};

struct ExhaustiveResult {
  bool ok = false;
  std::optional<std::string_view> diag_id;
  bool exhaustive = false;
};

static inline void SpecDefsTypeMatch() {
  SPEC_DEF("MatchJudg", "5.2.13");
  SPEC_DEF("ArmBody", "5.2.13");
  SPEC_DEF("Irrefutable", "5.2.13");
  SPEC_DEF("HasIrrefutableArm", "5.2.13");
  SPEC_DEF("ArmVariants", "5.2.13");
  SPEC_DEF("VariantNames", "5.2.13");
  SPEC_DEF("ArmStates", "5.2.13");
  SPEC_DEF("StateNames", "5.2.13");
  SPEC_DEF("UnionTypesExhaustive", "5.2.13");
  SPEC_DEF("AllEq", "5.2.13");
  SPEC_DEF("IntroAll", "5.2.11");
  SPEC_DEF("IntroAllVar", "5.2.11");
  SPEC_DEF("StripPerm", "5.2.12");
}

static std::optional<ParamMode> LowerParamMode(
    const std::optional<syntax::ParamMode>& mode) {
  if (!mode.has_value()) {
    return std::nullopt;
  }
  return ParamMode::Move;
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

static RawPtrQual LowerRawPtrQual(syntax::RawPtrQual qual) {
  switch (qual) {
    case syntax::RawPtrQual::Imm:
      return RawPtrQual::Imm;
    case syntax::RawPtrQual::Mut:
      return RawPtrQual::Mut;
  }
  return RawPtrQual::Imm;
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
          for (const auto& elem : node.types) {
            const auto lowered = LocalLowerType(ctx, elem);
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
          return {true, std::nullopt, MakeTypeString(LowerStringState(node.state))};
        } else if constexpr (std::is_same_v<T, syntax::TypeBytes>) {
          return {true, std::nullopt, MakeTypeBytes(LowerBytesState(node.state))};
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

static const syntax::EnumDecl* LookupEnumDecl(const ScopeContext& ctx,
                                              const TypePath& path) {
  syntax::TypePath syntax_path;
  syntax_path.reserve(path.size());
  for (const auto& comp : path) {
    syntax_path.push_back(comp);
  }
  const auto it = ctx.sigma.types.find(PathKeyOf(syntax_path));
  if (it == ctx.sigma.types.end()) {
    return nullptr;
  }
  return std::get_if<syntax::EnumDecl>(&it->second);
}

static bool InScope(const TypeEnv& env, const IdKey& key) {
  if (env.scopes.empty()) {
    return false;
  }
  return env.scopes.back().find(key) != env.scopes.back().end();
}

static bool InOuter(const TypeEnv& env, const IdKey& key) {
  if (env.scopes.size() < 2) {
    return false;
  }
  for (std::size_t i = 0; i + 1 < env.scopes.size(); ++i) {
    if (env.scopes[i].find(key) != env.scopes[i].end()) {
      return true;
    }
  }
  return false;
}

static IntroResult IntroBinding(const TypeEnv& env,
                                std::string_view name,
                                const TypeBinding& binding) {
  SpecDefsTypeMatch();
  if (ReservedGen(name)) {
    SPEC_RULE("Intro-Reserved-Gen-Err");
    return {false, "Intro-Reserved-Gen-Err", env};
  }
  if (ReservedCursive(name)) {
    SPEC_RULE("Intro-Reserved-Cursive-Err");
    return {false, "Intro-Reserved-Cursive-Err", env};
  }

  const auto key = IdKeyOf(name);
  if (env.scopes.empty()) {
    return {false, std::nullopt, env};
  }

  if (InScope(env, key)) {
    SPEC_RULE("Intro-Dup");
    return {false, std::nullopt, env};
  }
  if (InOuter(env, key)) {
    SPEC_RULE("Intro-Shadow-Required");
    if (std::getenv("CURSIVE0_DEBUG_SHADOW")) {
      std::cerr << "[cursivec0] shadow required for pattern `" << name << "`";
      for (std::size_t i = 1; i < env.scopes.size(); ++i) {
        if (env.scopes[i].count(key)) {
          std::cerr << " (outer scope " << i << ")";
          break;
        }
      }
      std::cerr << "\n";
    }
    return {false, "Intro-Shadow-Required", env};
  }

  TypeEnv out = env;
  out.scopes.back().emplace(key, binding);
  SPEC_RULE("Intro-Ok");
  return {true, std::nullopt, std::move(out)};
}

static IntroResult IntroAll(const TypeEnv& env,
                            const std::vector<std::pair<std::string, TypeRef>>& binds,
                            syntax::Mutability mut) {
  SpecDefsTypeMatch();
  if (binds.empty()) {
    if (mut == syntax::Mutability::Var) {
      SPEC_RULE("IntroAllVar-Empty");
    } else {
      SPEC_RULE("IntroAll-Empty");
    }
    return {true, std::nullopt, env};
  }

  TypeEnv current = env;
  for (const auto& [name, type] : binds) {
    TypeBinding binding{mut, type};
    const auto res = IntroBinding(current, name, binding);
    if (!res.ok) {
      return res;
    }
    current = res.env;
    if (mut == syntax::Mutability::Var) {
      SPEC_RULE("IntroAllVar-Cons");
    } else {
      SPEC_RULE("IntroAll-Cons");
    }
  }

  return {true, std::nullopt, std::move(current)};
}

static std::unordered_set<IdKey> VariantNames(const syntax::EnumDecl& decl) {
  SpecDefsTypeMatch();
  std::unordered_set<IdKey> out;
  for (const auto& variant : decl.variants) {
    out.insert(IdKeyOf(variant.name));
  }
  return out;
}

static std::unordered_set<IdKey> ArmVariants(
    const std::vector<syntax::MatchArm>& arms,
    const TypePath& expected_path) {
  SpecDefsTypeMatch();
  std::unordered_set<IdKey> out;
  for (const auto& arm : arms) {
    if (!arm.pattern) {
      continue;
    }
    if (const auto* enum_pat = std::get_if<syntax::EnumPattern>(&arm.pattern->node)) {
      if (enum_pat->path == expected_path) {
        out.insert(IdKeyOf(enum_pat->name));
      }
    }
  }
  return out;
}

static std::unordered_set<IdKey> ArmStates(
    const std::vector<syntax::MatchArm>& arms) {
  SpecDefsTypeMatch();
  std::unordered_set<IdKey> out;
  for (const auto& arm : arms) {
    if (!arm.pattern) {
      continue;
    }
    if (const auto* modal_pat = std::get_if<syntax::ModalPattern>(&arm.pattern->node)) {
      out.insert(IdKeyOf(modal_pat->state));
    }
  }
  return out;
}

static bool HasIrrefutableArm(const ScopeContext& ctx,
                              const std::vector<syntax::MatchArm>& arms,
                              const TypeRef& expected) {
  SpecDefsTypeMatch();
  for (const auto& arm : arms) {
    if (IrrefutablePattern(ctx, arm.pattern, expected)) {
      return true;
    }
  }
  return false;
}

static AllEqResult AllEq(const std::vector<TypeRef>& types) {
  SpecDefsTypeMatch();
  if (types.empty()) {
    return {true, std::nullopt, false};
  }
  TypeRef base = types.front();
  for (std::size_t i = 1; i < types.size(); ++i) {
    const auto equiv = TypeEquiv(base, types[i]);
    if (!equiv.ok) {
      return {false, equiv.diag_id, false};
    }
    if (!equiv.equiv) {
      return {true, std::nullopt, false};
    }
  }
  return {true, std::nullopt, true};
}

static ExhaustiveResult UnionTypesExhaustive(
    const ScopeContext& ctx,
    const std::vector<syntax::MatchArm>& arms,
    const std::vector<TypeRef>& members) {
  SpecDefsTypeMatch();
  for (const auto& member : members) {
    bool found = false;
    for (const auto& arm : arms) {
      if (!arm.pattern) {
        continue;
      }
      if (const auto* typed =
              std::get_if<syntax::TypedPattern>(&arm.pattern->node)) {
        const auto lowered = LocalLowerType(ctx, typed->type);
        if (!lowered.ok) {
          return {false, lowered.diag_id, false};
        }
        const auto equiv = TypeEquiv(lowered.type, member);
        if (!equiv.ok) {
          return {false, equiv.diag_id, false};
        }
        if (equiv.equiv) {
          found = true;
          break;
        }
      }
    }
    if (!found) {
      return {true, std::nullopt, false};
    }
  }
  return {true, std::nullopt, true};
}

static ExprTypeResult TypeArmBody(const ScopeContext& ctx,
                                  const StmtTypeContext& type_ctx,
                                  const syntax::ExprPtr& body,
                                  const TypeEnv& env) {
  SpecDefsTypeMatch();
  if (!body) {
    return {false, std::nullopt, {}};
  }

  if (const auto* block_expr = std::get_if<syntax::BlockExpr>(&body->node)) {
    if (!block_expr->block) {
      return {false, std::nullopt, {}};
    }
    TypeEnv live_env = env;
    StmtTypeContext arm_ctx = type_ctx;
    arm_ctx.env_ref = &live_env;
    auto active_env = [&]() -> const TypeEnv& {
      return arm_ctx.env_ref ? *arm_ctx.env_ref : env;
    };
    auto type_expr = [&](const syntax::ExprPtr& inner) {
      return TypeExpr(ctx, arm_ctx, inner, active_env());
    };
    auto type_ident = [&](std::string_view name) -> ExprTypeResult {
      return TypeIdentifierExpr(ctx, syntax::IdentifierExpr{std::string(name)},
                                active_env());
    };
    auto type_place = [&](const syntax::ExprPtr& inner) {
      return TypePlace(ctx, arm_ctx, inner, active_env());
    };
    const auto typed =
        TypeBlock(ctx, arm_ctx, *block_expr->block, env, type_expr,
                  type_ident, type_place, arm_ctx.env_ref);
    if (!typed.ok) {
      return typed;
    }
    SPEC_RULE("ArmBody-Block");
    return typed;
  }

  const auto typed = TypeExpr(ctx, type_ctx, body, env);
  if (!typed.ok) {
    return typed;
  }
  SPEC_RULE("ArmBody-Expr");
  return typed;
}

static CheckResult CheckArmBody(const ScopeContext& ctx,
                                const StmtTypeContext& type_ctx,
                                const syntax::ExprPtr& body,
                                const TypeEnv& env,
                                const TypeRef& expected) {
  SpecDefsTypeMatch();
  CheckResult result;
  if (!body || !expected) {
    return result;
  }

  if (const auto* block_expr = std::get_if<syntax::BlockExpr>(&body->node)) {
    if (!block_expr->block) {
      return result;
    }
    TypeEnv live_env = env;
    StmtTypeContext arm_ctx = type_ctx;
    arm_ctx.env_ref = &live_env;
    auto active_env = [&]() -> const TypeEnv& {
      return arm_ctx.env_ref ? *arm_ctx.env_ref : env;
    };
    auto type_expr = [&](const syntax::ExprPtr& inner) {
      return TypeExpr(ctx, arm_ctx, inner, active_env());
    };
    auto type_ident = [&](std::string_view name) -> ExprTypeResult {
      return TypeIdentifierExpr(ctx, syntax::IdentifierExpr{std::string(name)},
                                active_env());
    };
    auto type_place = [&](const syntax::ExprPtr& inner) {
      return TypePlace(ctx, arm_ctx, inner, active_env());
    };
    const auto check =
        CheckBlock(ctx, arm_ctx, *block_expr->block, env, expected, type_expr,
                   type_ident, type_place, arm_ctx.env_ref);
    if (!check.ok) {
      result.diag_id = check.diag_id;
      return result;
    }
    SPEC_RULE("ArmBody-Block-Chk");
    result.ok = true;
    return result;
  }

  auto type_expr = [&](const syntax::ExprPtr& inner) {
    return TypeExpr(ctx, type_ctx, inner, env);
  };
  auto type_ident = [&](std::string_view name) -> ExprTypeResult {
    return TypeIdentifierExpr(ctx, syntax::IdentifierExpr{std::string(name)},
                              env);
  };
  auto type_place = [&](const syntax::ExprPtr& inner) {
    return TypePlace(ctx, type_ctx, inner, env);
  };
  auto match_check = [&](const syntax::MatchExpr& match,
                         const TypeRef& expected_type) -> CheckResult {
    return CheckMatchExpr(ctx, type_ctx, match, env, expected_type);
  };
  const auto check =
      CheckExpr(ctx, body, expected, type_expr, type_place, type_ident, match_check);
  if (!check.ok) {
    result.diag_id = check.diag_id;
    return result;
  }
  SPEC_RULE("ArmBody-Expr-Chk");
  result.ok = true;
  return result;
}

}  // namespace

ExprTypeResult TypeMatchExpr(const ScopeContext& ctx,
                             const StmtTypeContext& type_ctx,
                             const syntax::MatchExpr& expr,
                             const TypeEnv& env) {
  SpecDefsTypeMatch();
  ExprTypeResult result;
  if (!expr.value) {
    return result;
  }

  const auto scrutinee = TypeExpr(ctx, type_ctx, expr.value, env);
  if (!scrutinee.ok) {
    result.diag_id = scrutinee.diag_id;
    return result;
  }

  const TypeRef scrutinee_base = StripPerm(scrutinee.type);
  if (!scrutinee_base) {
    return result;
  }

  const auto* union_type = std::get_if<TypeUnion>(&scrutinee_base->node);
  const auto* path_type = std::get_if<TypePathType>(&scrutinee_base->node);
  const syntax::EnumDecl* enum_decl = nullptr;
  const syntax::ModalDecl* modal_decl = nullptr;
  if (path_type) {
    enum_decl = LookupEnumDecl(ctx, path_type->path);
    if (!enum_decl) {
      modal_decl = LookupModalDecl(ctx, path_type->path);
    }
  }

  std::vector<TypeRef> arm_types;
  for (const auto& arm : expr.arms) {
    if (!arm.pattern || !arm.body) {
      return result;
    }
    const auto pat = TypeMatchPattern(ctx, arm.pattern, scrutinee.type);
    if (!pat.ok) {
      result.diag_id = pat.diag_id;
      return result;
    }
    const auto intro = IntroAll(env, pat.bindings, syntax::Mutability::Let);
    if (!intro.ok) {
      result.diag_id = intro.diag_id;
      return result;
    }
    if (arm.guard_opt) {
      const auto guard = TypeExpr(ctx, type_ctx, arm.guard_opt, intro.env);
      if (!guard.ok) {
        result.diag_id = guard.diag_id;
        return result;
      }
      if (!IsPrimType(guard.type, "bool")) {
        return result;
      }
    }
    const auto body = TypeArmBody(ctx, type_ctx, arm.body, intro.env);
    if (!body.ok) {
      result.diag_id = body.diag_id;
      return result;
    }
    arm_types.push_back(body.type);
  }

  const auto all_eq = AllEq(arm_types);
  if (!all_eq.ok) {
    result.diag_id = all_eq.diag_id;
    return result;
  }
  if (!all_eq.equal || arm_types.empty()) {
    return result;
  }

  if (enum_decl) {
    const auto arm_variants = ArmVariants(expr.arms, path_type->path);
    const auto decl_variants = VariantNames(*enum_decl);
    if (!HasIrrefutableArm(ctx, expr.arms, scrutinee.type) &&
        arm_variants != decl_variants) {
      return result;
    }
    SPEC_RULE("T-Match-Enum");
  } else if (modal_decl) {
    const auto arm_states = ArmStates(expr.arms);
    const auto decl_states = StateNameSet(*modal_decl);
    if (!HasIrrefutableArm(ctx, expr.arms, scrutinee.type) &&
        arm_states != decl_states) {
      SPEC_RULE("Match-Modal-NonExhaustive");
      result.diag_id = "Match-Modal-NonExhaustive";
      return result;
    }
    SPEC_RULE("T-Match-Modal");
  } else if (union_type) {
    const auto exhaustive = UnionTypesExhaustive(ctx, expr.arms,
                                                 union_type->members);
    if (!exhaustive.ok) {
      result.diag_id = exhaustive.diag_id;
      return result;
    }
    if (!HasIrrefutableArm(ctx, expr.arms, scrutinee.type) &&
        !exhaustive.exhaustive) {
      SPEC_RULE("Match-Union-NonExhaustive");
      result.diag_id = "Match-Union-NonExhaustive";
      return result;
    }
    SPEC_RULE("T-Match-Union");
  } else {
    if (!HasIrrefutableArm(ctx, expr.arms, scrutinee.type)) {
      return result;
    }
    SPEC_RULE("T-Match-Other");
  }

  result.ok = true;
  result.type = arm_types.front();
  return result;
}

CheckResult CheckMatchExpr(const ScopeContext& ctx,
                           const StmtTypeContext& type_ctx,
                           const syntax::MatchExpr& expr,
                           const TypeEnv& env,
                           const TypeRef& expected) {
  SpecDefsTypeMatch();
  CheckResult result;
  if (!expr.value || !expected) {
    return result;
  }

  const auto scrutinee = TypeExpr(ctx, type_ctx, expr.value, env);
  if (!scrutinee.ok) {
    result.diag_id = scrutinee.diag_id;
    return result;
  }

  const TypeRef scrutinee_base = StripPerm(scrutinee.type);
  if (!scrutinee_base) {
    return result;
  }

  const auto* union_type = std::get_if<TypeUnion>(&scrutinee_base->node);
  const auto* path_type = std::get_if<TypePathType>(&scrutinee_base->node);
  const syntax::EnumDecl* enum_decl = nullptr;
  const syntax::ModalDecl* modal_decl = nullptr;
  if (path_type) {
    enum_decl = LookupEnumDecl(ctx, path_type->path);
    if (!enum_decl) {
      modal_decl = LookupModalDecl(ctx, path_type->path);
    }
  }

  for (const auto& arm : expr.arms) {
    if (!arm.pattern || !arm.body) {
      return result;
    }
    const auto pat = TypeMatchPattern(ctx, arm.pattern, scrutinee.type);
    if (!pat.ok) {
      result.diag_id = pat.diag_id;
      return result;
    }
    const auto intro = IntroAll(env, pat.bindings, syntax::Mutability::Let);
    if (!intro.ok) {
      result.diag_id = intro.diag_id;
      return result;
    }
    if (arm.guard_opt) {
      auto type_expr = [&](const syntax::ExprPtr& inner) {
        return TypeExpr(ctx, type_ctx, inner, intro.env);
      };
      auto type_ident = [&](std::string_view name) -> ExprTypeResult {
        return TypeIdentifierExpr(ctx,
                                  syntax::IdentifierExpr{std::string(name)},
                                  intro.env);
      };
      auto type_place = [&](const syntax::ExprPtr& inner) {
        return TypePlace(ctx, type_ctx, inner, intro.env);
      };
      auto match_check = [&](const syntax::MatchExpr& match,
                             const TypeRef& expected_type) -> CheckResult {
        return CheckMatchExpr(ctx, type_ctx, match, intro.env, expected_type);
      };
      const auto check =
          CheckExpr(ctx, arm.guard_opt, MakeTypePrim("bool"), type_expr,
                    type_place, type_ident, match_check);
      if (!check.ok) {
        result.diag_id = check.diag_id;
        return result;
      }
    }

    const auto check = CheckArmBody(ctx, type_ctx, arm.body, intro.env, expected);
    if (!check.ok) {
      result.diag_id = check.diag_id;
      return result;
    }
  }

  if (enum_decl) {
    const auto arm_variants = ArmVariants(expr.arms, path_type->path);
    const auto decl_variants = VariantNames(*enum_decl);
    if (!HasIrrefutableArm(ctx, expr.arms, scrutinee.type) &&
        arm_variants != decl_variants) {
      return result;
    }
    SPEC_RULE("Chk-Match-Enum");
  } else if (modal_decl) {
    const auto arm_states = ArmStates(expr.arms);
    const auto decl_states = StateNameSet(*modal_decl);
    if (!HasIrrefutableArm(ctx, expr.arms, scrutinee.type) &&
        arm_states != decl_states) {
      SPEC_RULE("Match-Modal-NonExhaustive");
      result.diag_id = "Match-Modal-NonExhaustive";
      return result;
    }
    SPEC_RULE("Chk-Match-Modal");
  } else if (union_type) {
    const auto exhaustive = UnionTypesExhaustive(ctx, expr.arms,
                                                 union_type->members);
    if (!exhaustive.ok) {
      result.diag_id = exhaustive.diag_id;
      return result;
    }
    if (!HasIrrefutableArm(ctx, expr.arms, scrutinee.type) &&
        !exhaustive.exhaustive) {
      SPEC_RULE("Match-Union-NonExhaustive");
      result.diag_id = "Match-Union-NonExhaustive";
      return result;
    }
    SPEC_RULE("Chk-Match-Union");
  } else {
    if (!HasIrrefutableArm(ctx, expr.arms, scrutinee.type)) {
      return result;
    }
    SPEC_RULE("Chk-Match-Other");
  }

  result.ok = true;
  return result;
}

}  // namespace cursive0::analysis
