#include "cursive0/analysis/memory/borrow_bind.h"

#include <algorithm>
#include <functional>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "cursive0/core/assert_spec.h"
#include "cursive0/analysis/composite/classes.h"
#include "cursive0/analysis/resolve/scopes.h"
#include "cursive0/analysis/types/type_equiv.h"
#include "cursive0/analysis/types/type_expr.h"
#include "cursive0/analysis/types/type_infer.h"
#include "cursive0/analysis/types/type_match.h"
#include "cursive0/analysis/types/type_pattern.h"
#include "cursive0/analysis/types/type_stmt.h"
#include "cursive0/analysis/memory/regions.h"
#include "cursive0/analysis/types/types.h"

namespace cursive0::analysis {

namespace {

static inline void SpecDefsBorrowBind() {
  SPEC_DEF("BindingState", "5.2.15");
  SPEC_DEF("Movability", "5.2.15");
  SPEC_DEF("Responsibility", "5.2.15");
  SPEC_DEF("BindInfo", "5.2.15");
  SPEC_DEF("BindScope", "5.2.15");
  SPEC_DEF("PermOf", "5.2.15");
  SPEC_DEF("ActiveState", "5.2.15");
  SPEC_DEF("PermKey", "5.2.15");
  SPEC_DEF("PermScope", "5.2.15");
  SPEC_DEF("FieldPath", "5.2.15");
  SPEC_DEF("FieldPathOf", "5.2.15");
  SPEC_DEF("FieldHead", "5.2.15");
  SPEC_DEF("AccessStateOk", "5.2.15");
  SPEC_DEF("AccessOk", "5.2.15");
  SPEC_DEF("BindInfoMap", "5.2.15");
  SPEC_DEF("MovEff", "5.2.15");
  SPEC_DEF("ParamBindMap", "5.2.15");
  SPEC_DEF("ParamTypeMap", "5.2.15");
  SPEC_DEF("ParamMov", "5.2.15");
  SPEC_DEF("ParamResp", "5.2.15");
  SPEC_DEF("JoinState", "5.2.15");
  SPEC_DEF("JoinBindInfo", "5.2.15");
  SPEC_DEF("JoinScope_B", "5.2.15");
  SPEC_DEF("Join_B", "5.2.15");
  SPEC_DEF("JoinPerm", "5.2.15");
  SPEC_DEF("JoinPermState", "5.2.15");
  SPEC_DEF("JoinAll_B", "5.2.15");
  SPEC_DEF("JoinAllPerm", "5.2.15");
  SPEC_DEF("LoopFix", "5.2.15");
}

static inline void SpecRuleTransitionAnchor() {
  SPEC_RULE("B-Transition");
}

enum class BindStateKind {
  Valid,
  Moved,
  PartiallyMoved,
};

struct BindState {
  BindStateKind kind = BindStateKind::Valid;
  std::set<IdKey> fields;
};

enum class Movability {
  Mov,
  Immov,
};

enum class Responsibility {
  Resp,
  Alias,
};

struct BindInfo {
  BindState state;
  Movability mov = Movability::Mov;
  syntax::Mutability mut = syntax::Mutability::Let;
  Responsibility resp = Responsibility::Alias;
};

using BindScope = std::map<IdKey, BindInfo>;
using BindEnv = std::vector<BindScope>;

enum class ActiveState {
  Active,
  Inactive,
};

struct PermKey {
  IdKey root;
  std::vector<IdKey> path;
};

struct PermKeyLess {
  bool operator()(const PermKey& lhs, const PermKey& rhs) const {
    if (lhs.root != rhs.root) {
      return lhs.root < rhs.root;
    }
    const auto& a = lhs.path;
    const auto& b = rhs.path;
    const std::size_t min_len = std::min(a.size(), b.size());
    for (std::size_t i = 0; i < min_len; ++i) {
      if (a[i] != b[i]) {
        return a[i] < b[i];
      }
    }
    return a.size() < b.size();
  }
};

using PermScope = std::map<PermKey, ActiveState, PermKeyLess>;
using PermEnv = std::vector<PermScope>;

struct BindStateBundle {
  BindEnv binds;
  PermEnv perms;
  TypeEnv env;
};

struct BindResult {
  bool ok = false;
  std::optional<std::string_view> diag_id;
  std::optional<core::Span> span;
  BindStateBundle state;
};

struct ArgPassResult {
  bool ok = false;
  std::optional<std::string_view> diag_id;
  std::optional<core::Span> span;
  BindStateBundle state;
  std::set<PermKey, PermKeyLess> roots;
};

struct ParamInfo {
  std::optional<ParamMode> mode;
};

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
          return {true, std::nullopt, MakeTypePath(node.path)};
        } else {
          return {false, std::nullopt, {}};
        }
      },
      type->node);
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

static Permission PermOfType(const TypeRef& type) {
  if (!type) {
    return Permission::Const;
  }
  if (const auto* perm = std::get_if<TypePerm>(&type->node)) {
    return perm->perm;
  }
  return Permission::Const;
}

static StmtTypeContext MakeTypeCtx() {
  StmtTypeContext ctx;
  ctx.return_type = MakeTypePrim("()");
  ctx.loop_flag = LoopFlag::None;
  ctx.in_unsafe = false;
  ctx.diags = nullptr;
  ctx.env_ref = nullptr;
  return ctx;
}

static std::optional<TypeRef> ExprTypeOf(const ScopeContext& ctx,
                                        const TypeEnv& env,
                                        const syntax::ExprPtr& expr) {
  if (!expr) {
    return std::nullopt;
  }
  auto type_ctx = MakeTypeCtx();
  if (IsPlaceExpr(expr)) {
    const auto place = TypePlace(ctx, type_ctx, expr, env);
    if (place.ok) {
      return place.type;
    }
  }
  const auto typed = TypeExpr(ctx, type_ctx, expr, env);
  if (!typed.ok) {
    return std::nullopt;
  }
  return typed.type;
}

static std::optional<TypeRef> PlaceTypeOf(const ScopeContext& ctx,
                                         const TypeEnv& env,
                                         const syntax::ExprPtr& expr) {
  if (!expr) {
    return std::nullopt;
  }
  auto type_ctx = MakeTypeCtx();
  const auto place = TypePlace(ctx, type_ctx, expr, env);
  if (!place.ok) {
    return std::nullopt;
  }
  return place.type;
}

static bool BindStateEqual(const BindState& lhs, const BindState& rhs) {
  if (lhs.kind != rhs.kind) {
    return false;
  }
  if (lhs.kind != BindStateKind::PartiallyMoved) {
    return true;
  }
  return lhs.fields == rhs.fields;
}

static bool BindInfoEqual(const BindInfo& lhs, const BindInfo& rhs) {
  return lhs.mov == rhs.mov && lhs.mut == rhs.mut && lhs.resp == rhs.resp &&
         BindStateEqual(lhs.state, rhs.state);
}

static bool BindScopeEqual(const BindScope& lhs, const BindScope& rhs) {
  if (lhs.size() != rhs.size()) {
    return false;
  }
  auto it_l = lhs.begin();
  auto it_r = rhs.begin();
  for (; it_l != lhs.end(); ++it_l, ++it_r) {
    if (it_l->first != it_r->first) {
      return false;
    }
    if (!BindInfoEqual(it_l->second, it_r->second)) {
      return false;
    }
  }
  return true;
}

static bool BindEnvEqual(const BindEnv& lhs, const BindEnv& rhs) {
  if (lhs.size() != rhs.size()) {
    return false;
  }
  for (std::size_t i = 0; i < lhs.size(); ++i) {
    if (!BindScopeEqual(lhs[i], rhs[i])) {
      return false;
    }
  }
  return true;
}

static bool PermScopeEqual(const PermScope& lhs, const PermScope& rhs) {
  if (lhs.size() != rhs.size()) {
    return false;
  }
  auto it_l = lhs.begin();
  auto it_r = rhs.begin();
  for (; it_l != lhs.end(); ++it_l, ++it_r) {
    if (!PermKeyLess{}(it_l->first, it_r->first) &&
        !PermKeyLess{}(it_r->first, it_l->first)) {
      if (it_l->second != it_r->second) {
        return false;
      }
      continue;
    }
    return false;
  }
  return true;
}

static bool PermEnvEqual(const PermEnv& lhs, const PermEnv& rhs) {
  if (lhs.size() != rhs.size()) {
    return false;
  }
  for (std::size_t i = 0; i < lhs.size(); ++i) {
    if (!PermScopeEqual(lhs[i], rhs[i])) {
      return false;
    }
  }
  return true;
}

static BindEnv PushScope_B(const BindEnv& env) {
  BindEnv out = env;
  out.emplace_back();
  return out;
}

static BindEnv PopScope_B(const BindEnv& env) {
  if (env.empty()) {
    return env;
  }
  BindEnv out = env;
  out.pop_back();
  return out;
}

static std::optional<BindInfo> Lookup_B(const BindEnv& env, std::string_view name) {
  const auto key = IdKeyOf(name);
  for (auto it = env.rbegin(); it != env.rend(); ++it) {
    const auto found = it->find(key);
    if (found != it->end()) {
      return found->second;
    }
  }
  return std::nullopt;
}

static std::optional<BindEnv> Update_B(const BindEnv& env,
                                      std::string_view name,
                                      const BindInfo& info) {
  const auto key = IdKeyOf(name);
  BindEnv out = env;
  for (auto it = out.rbegin(); it != out.rend(); ++it) {
    const auto found = it->find(key);
    if (found != it->end()) {
      found->second = info;
      return out;
    }
  }
  return std::nullopt;
}

static BindEnv Intro_B(const BindEnv& env,
                       std::string_view name,
                       const BindInfo& info) {
  BindEnv out = env;
  if (out.empty()) {
    out.emplace_back();
  }
  out.back()[IdKeyOf(name)] = info;
  return out;
}

static BindEnv IntroAll_B(const BindEnv& env, const BindScope& scope) {
  BindEnv out = env;
  if (out.empty()) {
    out.emplace_back();
  }
  BindScope merged = out.back();
  for (const auto& [name, info] : scope) {
    merged[name] = info;
  }
  out.back() = std::move(merged);
  return out;
}

static std::optional<BindEnv> ShadowAll_B(const BindEnv& env,
                                          const BindScope& scope) {
  BindEnv current = env;
  for (const auto& [name, info] : scope) {
    const auto updated = Update_B(current, name, info);
    if (!updated.has_value()) {
      return std::nullopt;
    }
    current = *updated;
  }
  return current;
}

static BindState JoinState(const BindState& lhs, const BindState& rhs) {
  if (lhs.kind == BindStateKind::Moved || rhs.kind == BindStateKind::Moved) {
    return BindState{BindStateKind::Moved, {}};
  }
  if (lhs.kind == BindStateKind::PartiallyMoved &&
      rhs.kind == BindStateKind::PartiallyMoved) {
    std::set<IdKey> merged = lhs.fields;
    merged.insert(rhs.fields.begin(), rhs.fields.end());
    return BindState{BindStateKind::PartiallyMoved, std::move(merged)};
  }
  if (lhs.kind == BindStateKind::PartiallyMoved &&
      rhs.kind == BindStateKind::Valid) {
    return lhs;
  }
  if (lhs.kind == BindStateKind::Valid &&
      rhs.kind == BindStateKind::PartiallyMoved) {
    return rhs;
  }
  return BindState{BindStateKind::Valid, {}};
}

static std::optional<BindInfo> JoinBindInfo(const BindInfo& lhs,
                                            const BindInfo& rhs) {
  if (lhs.mov != rhs.mov || lhs.mut != rhs.mut || lhs.resp != rhs.resp) {
    return std::nullopt;
  }
  BindInfo out = lhs;
  out.state = JoinState(lhs.state, rhs.state);
  return out;
}

static std::optional<BindScope> JoinScope_B(const BindScope& lhs,
                                            const BindScope& rhs) {
  if (lhs.size() != rhs.size()) {
    return std::nullopt;
  }
  BindScope out;
  auto it_l = lhs.begin();
  auto it_r = rhs.begin();
  for (; it_l != lhs.end(); ++it_l, ++it_r) {
    if (it_l->first != it_r->first) {
      return std::nullopt;
    }
    const auto joined = JoinBindInfo(it_l->second, it_r->second);
    if (!joined.has_value()) {
      return std::nullopt;
    }
    out.emplace(it_l->first, *joined);
  }
  return out;
}

static std::optional<BindEnv> Join_B(const BindEnv& lhs, const BindEnv& rhs) {
  if (lhs.size() != rhs.size()) {
    return std::nullopt;
  }
  BindEnv out;
  out.reserve(lhs.size());
  for (std::size_t i = 0; i < lhs.size(); ++i) {
    const auto scope = JoinScope_B(lhs[i], rhs[i]);
    if (!scope.has_value()) {
      return std::nullopt;
    }
    out.push_back(*scope);
  }
  return out;
}

static ActiveState JoinPermState(ActiveState lhs, ActiveState rhs) {
  if (lhs == ActiveState::Active && rhs == ActiveState::Active) {
    return ActiveState::Active;
  }
  return ActiveState::Inactive;
}

static ActiveState PermAt(const PermScope& scope, const PermKey& key) {
  const auto it = scope.find(key);
  if (it == scope.end()) {
    return ActiveState::Active;
  }
  return it->second;
}

static PermScope JoinScope_Pi(const PermScope& lhs, const PermScope& rhs) {
  PermScope out;
  auto it_l = lhs.begin();
  auto it_r = rhs.begin();
  std::set<PermKey, PermKeyLess> keys;
  for (const auto& [key, _] : lhs) {
    keys.insert(key);
  }
  for (const auto& [key, _] : rhs) {
    keys.insert(key);
  }
  for (const auto& key : keys) {
    out.emplace(key, JoinPermState(PermAt(lhs, key), PermAt(rhs, key)));
  }
  return out;
}

static std::optional<PermEnv> JoinPerm(const PermEnv& lhs, const PermEnv& rhs) {
  if (lhs.size() != rhs.size()) {
    return std::nullopt;
  }
  PermEnv out;
  out.reserve(lhs.size());
  for (std::size_t i = 0; i < lhs.size(); ++i) {
    out.push_back(JoinScope_Pi(lhs[i], rhs[i]));
  }
  return out;
}

static std::optional<BindEnv> JoinAll_B(const std::vector<BindEnv>& envs) {
  if (envs.empty()) {
    return std::nullopt;
  }
  BindEnv current = envs.front();
  for (std::size_t i = 1; i < envs.size(); ++i) {
    const auto joined = Join_B(current, envs[i]);
    if (!joined.has_value()) {
      return std::nullopt;
    }
    current = *joined;
  }
  return current;
}

static std::optional<PermEnv> JoinAllPerm(const std::vector<PermEnv>& envs) {
  if (envs.empty()) {
    return std::nullopt;
  }
  PermEnv current = envs.front();
  for (std::size_t i = 1; i < envs.size(); ++i) {
    const auto joined = JoinPerm(current, envs[i]);
    if (!joined.has_value()) {
      return std::nullopt;
    }
    current = *joined;
  }
  return current;
}

static PermEnv PushScope_Pi(const PermEnv& env) {
  PermEnv out = env;
  out.emplace_back();
  return out;
}

static PermEnv PopScope_Pi(const PermEnv& env) {
  if (env.empty()) {
    return env;
  }
  PermEnv out = env;
  out.pop_back();
  return out;
}

static ActiveState Lookup_Pi(const PermEnv& env, const PermKey& key) {
  for (auto it = env.rbegin(); it != env.rend(); ++it) {
    const auto found = it->find(key);
    if (found != it->end() && found->second == ActiveState::Inactive) {
      return ActiveState::Inactive;
    }
  }
  return ActiveState::Active;
}

static const PermScope& TopPerm(const PermEnv& env) {
  return env.back();
}

static PermEnv SetTopPerm(const PermEnv& env, const PermScope& scope) {
  PermEnv out = env;
  if (out.empty()) {
    out.push_back(scope);
  } else {
    out.back() = scope;
  }
  return out;
}

static PermScope InactivateScope(const PermScope& scope,
                                 const std::set<PermKey, PermKeyLess>& keys) {
  PermScope out = scope;
  for (const auto& key : keys) {
    out[key] = ActiveState::Inactive;
  }
  return out;
}

static std::set<PermKey, PermKeyLess> Roots(const PermEnv& after,
                                            const PermEnv& before) {
  std::set<PermKey, PermKeyLess> roots;
  if (after.empty()) {
    return roots;
  }
  const auto& top = TopPerm(after);
  for (const auto& [key, state] : top) {
    if (state == ActiveState::Inactive &&
        Lookup_Pi(before, key) == ActiveState::Active) {
      roots.insert(key);
    }
  }
  return roots;
}

static PermScope RemoveKeys(const PermScope& scope,
                            const std::set<PermKey, PermKeyLess>& keys) {
  PermScope out;
  for (const auto& [key, state] : scope) {
    if (keys.find(key) == keys.end()) {
      out.emplace(key, state);
    }
  }
  return out;
}

static PermEnv Reactivate(const PermEnv& env,
                          const std::set<PermKey, PermKeyLess>& keys) {
  if (env.empty()) {
    return env;
  }
  return SetTopPerm(env, RemoveKeys(TopPerm(env), keys));
}

static std::optional<IdKey> PlaceRoot(const syntax::ExprPtr& expr) {
  if (!expr) {
    return std::nullopt;
  }
  if (const auto* ident = std::get_if<syntax::IdentifierExpr>(&expr->node)) {
    return IdKeyOf(ident->name);
  }
  if (const auto* field = std::get_if<syntax::FieldAccessExpr>(&expr->node)) {
    return PlaceRoot(field->base);
  }
  if (const auto* tuple = std::get_if<syntax::TupleAccessExpr>(&expr->node)) {
    return PlaceRoot(tuple->base);
  }
  if (const auto* index = std::get_if<syntax::IndexAccessExpr>(&expr->node)) {
    return PlaceRoot(index->base);
  }
  if (const auto* deref = std::get_if<syntax::DerefExpr>(&expr->node)) {
    return PlaceRoot(deref->value);
  }
  return std::nullopt;
}

static std::optional<IdKey> FieldHead(const syntax::ExprPtr& expr) {
  if (!expr) {
    return std::nullopt;
  }
  if (std::holds_alternative<syntax::IdentifierExpr>(expr->node)) {
    return std::nullopt;
  }
  if (const auto* field = std::get_if<syntax::FieldAccessExpr>(&expr->node)) {
    const auto inner = FieldHead(field->base);
    if (!inner.has_value()) {
      return IdKeyOf(field->name);
    }
    return inner;
  }
  if (const auto* tuple = std::get_if<syntax::TupleAccessExpr>(&expr->node)) {
    return FieldHead(tuple->base);
  }
  if (const auto* index = std::get_if<syntax::IndexAccessExpr>(&expr->node)) {
    return FieldHead(index->base);
  }
  if (std::holds_alternative<syntax::DerefExpr>(expr->node)) {
    return std::nullopt;
  }
  return std::nullopt;
}

static std::vector<IdKey> FieldPathOf(const syntax::ExprPtr& expr) {
  if (!expr) {
    return {};
  }
  if (std::holds_alternative<syntax::IdentifierExpr>(expr->node)) {
    return {};
  }
  if (const auto* field = std::get_if<syntax::FieldAccessExpr>(&expr->node)) {
    auto path = FieldPathOf(field->base);
    path.push_back(IdKeyOf(field->name));
    return path;
  }
  if (const auto* tuple = std::get_if<syntax::TupleAccessExpr>(&expr->node)) {
    return FieldPathOf(tuple->base);
  }
  if (const auto* index = std::get_if<syntax::IndexAccessExpr>(&expr->node)) {
    return FieldPathOf(index->base);
  }
  if (std::holds_alternative<syntax::DerefExpr>(expr->node)) {
    return {};
  }
  return {};
}

static std::vector<std::vector<IdKey>> Prefixes(
    const std::vector<IdKey>& path) {
  std::vector<std::vector<IdKey>> out;
  out.emplace_back();
  if (path.empty()) {
    return out;
  }
  for (std::size_t i = 0; i < path.size(); ++i) {
    std::vector<IdKey> prefix;
    prefix.reserve(i + 1);
    for (std::size_t j = 0; j <= i; ++j) {
      prefix.push_back(path[j]);
    }
    out.push_back(std::move(prefix));
  }
  return out;
}

static std::set<PermKey, PermKeyLess> AncPaths(const syntax::ExprPtr& expr) {
  std::set<PermKey, PermKeyLess> keys;
  const auto root = PlaceRoot(expr);
  if (!root.has_value()) {
    return keys;
  }
  const auto path = FieldPathOf(expr);
  const auto prefixes = Prefixes(path);
  for (const auto& prefix : prefixes) {
    keys.insert(PermKey{*root, prefix});
  }
  return keys;
}

static bool AccessPathOk(const PermEnv& env, const syntax::ExprPtr& expr) {
  const auto keys = AncPaths(expr);
  for (const auto& key : keys) {
    if (Lookup_Pi(env, key) != ActiveState::Active) {
      return false;
    }
  }
  return true;
}

static bool AccessStateOk(const BindState& state,
                          const syntax::ExprPtr& expr) {
  if (state.kind == BindStateKind::Valid) {
    return true;
  }
  if (state.kind == BindStateKind::Moved) {
    return false;
  }
  const auto head = FieldHead(expr);
  if (!head.has_value()) {
    return false;
  }
  return state.fields.find(*head) == state.fields.end();
}

static bool AccessOk_B(const BindEnv& env, const syntax::ExprPtr& expr) {
  const auto root = PlaceRoot(expr);
  if (!root.has_value()) {
    return false;
  }
  const auto info = Lookup_B(env, *root);
  if (!info.has_value()) {
    return false;
  }
  return AccessStateOk(info->state, expr);
}

static bool AccessOk_Pi(const ScopeContext& ctx,
                        const TypeEnv& env,
                        const PermEnv& perms,
                        const syntax::ExprPtr& expr) {
  const auto place_type = PlaceTypeOf(ctx, env, expr);
  if (!place_type.has_value()) {
    return true;
  }
  if (PermOfType(*place_type) != Permission::Unique) {
    return true;
  }
  return AccessPathOk(perms, expr);
}

static bool AccessOk(const ScopeContext& ctx,
                     const TypeEnv& env,
                     const BindEnv& binds,
                     const PermEnv& perms,
                     const syntax::ExprPtr& expr) {
  return AccessOk_B(binds, expr) && AccessOk_Pi(ctx, env, perms, expr);
}

static Movability MovOf(const syntax::Token& op) {
  if (op.lexeme == ":=") {
    return Movability::Immov;
  }
  return Movability::Mov;
}

static bool IsMoveExpr(const syntax::ExprPtr& expr) {
  return expr && std::holds_alternative<syntax::MoveExpr>(expr->node);
}

static syntax::ExprPtr MoveInner(const syntax::ExprPtr& expr) {
  if (!expr) {
    return nullptr;
  }
  if (const auto* move = std::get_if<syntax::MoveExpr>(&expr->node)) {
    return move->place;
  }
  return nullptr;
}

static Responsibility RespOfInit(const syntax::ExprPtr& init) {
  if (!IsPlaceExpr(init)) {
    return Responsibility::Resp;
  }
  if (IsMoveExpr(init)) {
    return Responsibility::Resp;
  }
  return Responsibility::Alias;
}

static Movability MovEff(Movability mv, Responsibility resp) {
  if (resp == Responsibility::Alias) {
    return Movability::Immov;
  }
  return mv;
}

static BindScope BindInfoMap(const std::map<IdKey, TypeRef>& types,
                             Responsibility resp,
                             Movability mv,
                             syntax::Mutability mut) {
  BindScope out;
  for (const auto& [name, _] : types) {
    BindInfo info;
    info.state = BindState{BindStateKind::Valid, {}};
    info.mov = MovEff(mv, resp);
    info.mut = mut;
    info.resp = resp;
    out.emplace(name, info);
  }
  return out;
}

static BindState PM(const BindState& state, const IdKey& field) {
  if (state.kind == BindStateKind::Valid) {
    return BindState{BindStateKind::PartiallyMoved, {field}};
  }
  if (state.kind == BindStateKind::PartiallyMoved) {
    BindState out = state;
    out.fields.insert(field);
    return out;
  }
  return BindState{BindStateKind::Moved, {}};
}

static BindEnv ConsumeOnMove(const BindEnv& env, const syntax::ExprPtr& expr) {
  if (!IsMoveExpr(expr)) {
    return env;
  }
  const auto inner = MoveInner(expr);
  const auto root = PlaceRoot(inner);
  if (!root.has_value()) {
    return env;
  }
  const auto info = Lookup_B(env, *root);
  if (!info.has_value()) {
    return env;
  }
  BindInfo updated = *info;
  updated.state = BindState{BindStateKind::Moved, {}};
  const auto res = Update_B(env, *root, updated);
  return res.value_or(env);
}

static PermEnv DowngradeUniquePath(const ScopeContext& ctx,
                                   const TypeEnv& env,
                                   const PermEnv& perms,
                                   const std::optional<ParamMode>& mode,
                                   const syntax::ExprPtr& expr) {
  if (mode.has_value()) {
    return perms;
  }
  if (!IsPlaceExpr(expr)) {
    return perms;
  }
  const auto place_type = PlaceTypeOf(ctx, env, expr);
  if (!place_type.has_value()) {
    return perms;
  }
  if (PermOfType(*place_type) != Permission::Unique) {
    return perms;
  }
  PermEnv out = perms;
  if (out.empty()) {
    out.emplace_back();
  }
  const auto keys = AncPaths(expr);
  const auto top = InactivateScope(out.back(), keys);
  out.back() = top;
  return out;
}

static PermEnv DowngradeUnique(const ScopeContext& ctx,
                               const TypeEnv& env,
                               const PermEnv& perms,
                               const std::optional<ParamMode>& mode,
                               const syntax::ExprPtr& expr) {
  if (!IsPlaceExpr(expr)) {
    return perms;
  }
  return DowngradeUniquePath(ctx, env, perms, mode, expr);
}

static PermEnv DowngradeUniqueBind(const ScopeContext& ctx,
                                   const TypeEnv& env,
                                   const PermEnv& perms,
                                   const syntax::ExprPtr& init,
                                   const TypeRef& bind_type) {
  if (!IsPlaceExpr(init)) {
    return perms;
  }
  const auto init_type = PlaceTypeOf(ctx, env, init);
  if (!init_type.has_value()) {
    return perms;
  }
  if (PermOfType(*init_type) != Permission::Unique) {
    return perms;
  }
  // C0X Extension: unique -> shared or unique -> const both trigger downgrade
  // Permission lattice: unique <: shared <: const
  const auto bind_perm = PermOfType(bind_type);
  if (bind_perm != Permission::Const && bind_perm != Permission::Shared) {
    return perms;
  }
  return DowngradeUniquePath(ctx, env, perms, std::nullopt, init);
}

static std::optional<TypeRef> InferBindType(const ScopeContext& ctx,
                                            const TypeEnv& env,
                                            const syntax::ExprPtr& init,
                                            std::optional<std::string_view>& diag_id) {
  auto type_ctx = MakeTypeCtx();
  auto type_expr = [&](const syntax::ExprPtr& expr) {
    return TypeExpr(ctx, type_ctx, expr, env);
  };
  auto type_ident = [&](std::string_view name) -> ExprTypeResult {
    return TypeIdentifierExpr(ctx, syntax::IdentifierExpr{std::string(name)}, env);
  };
  auto type_place = [&](const syntax::ExprPtr& expr) {
    return TypePlace(ctx, type_ctx, expr, env);
  };
  auto match_check = [&](const syntax::MatchExpr& match,
                         const TypeRef& expected) -> CheckResult {
    return CheckMatchExpr(ctx, type_ctx, match, env, expected);
  };
  const auto inferred = InferExpr(ctx, init, type_expr, type_place, type_ident, match_check);
  if (!inferred.ok) {
    diag_id = inferred.diag_id;
    return std::nullopt;
  }
  return inferred.type;
}

static std::optional<TypeRef> BindTypeForBinding(const ScopeContext& ctx,
                                                 const TypeEnv& env,
                                                 const syntax::Binding& binding,
                                                 std::optional<std::string_view>& diag_id) {
  if (binding.type_opt) {
    const auto lowered = LowerType(ctx, binding.type_opt);
    if (!lowered.ok) {
      diag_id = lowered.diag_id;
      return std::nullopt;
    }
    return lowered.type;
  }
  return InferBindType(ctx, env, binding.init, diag_id);
}

static std::optional<TypeRef> BindTypeForShadow(const ScopeContext& ctx,
                                                const TypeEnv& env,
                                                const syntax::ShadowLetStmt& stmt,
                                                std::optional<std::string_view>& diag_id) {
  if (stmt.type_opt) {
    const auto lowered = LowerType(ctx, stmt.type_opt);
    if (!lowered.ok) {
      diag_id = lowered.diag_id;
      return std::nullopt;
    }
    return lowered.type;
  }
  return InferBindType(ctx, env, stmt.init, diag_id);
}

static std::optional<TypeRef> BindTypeForShadow(const ScopeContext& ctx,
                                                const TypeEnv& env,
                                                const syntax::ShadowVarStmt& stmt,
                                                std::optional<std::string_view>& diag_id) {
  if (stmt.type_opt) {
    const auto lowered = LowerType(ctx, stmt.type_opt);
    if (!lowered.ok) {
      diag_id = lowered.diag_id;
      return std::nullopt;
    }
    return lowered.type;
  }
  return InferBindType(ctx, env, stmt.init, diag_id);
}

static std::map<IdKey, TypeRef> BindTypeMapFromBindings(
    const std::vector<std::pair<std::string, TypeRef>>& bindings) {
  std::map<IdKey, TypeRef> out;
  for (const auto& [name, type] : bindings) {
    out.emplace(IdKeyOf(name), type);
  }
  return out;
}

static std::optional<IdKey> FieldHeadName(const syntax::ExprPtr& expr) {
  return FieldHead(expr);
}

static bool BindingMovedErrCond(const BindInfo& info,
                                const syntax::ExprPtr& expr) {
  if (info.state.kind == BindStateKind::Moved) {
    return true;
  }
  if (info.state.kind == BindStateKind::PartiallyMoved) {
    const auto head = FieldHeadName(expr);
    if (!head.has_value()) {
      return true;
    }
    return info.state.fields.find(*head) != info.state.fields.end();
  }
  return false;
}

static BindResult ErrorResult(std::string_view diag_id,
                              const std::optional<core::Span>& span) {
  BindResult result;
  result.ok = false;
  result.diag_id = diag_id;
  result.span = span;
  return result;
}

static BindResult ErrorResult(std::optional<std::string_view> diag_id,
                              const std::optional<core::Span>& span) {
  BindResult result;
  result.ok = false;
  result.diag_id = diag_id;
  result.span = span;
  return result;
}

static BindResult OkResult(const BindStateBundle& state) {
  BindResult result;
  result.ok = true;
  result.state = state;
  return result;
}

static ArgPassResult ArgError(std::optional<std::string_view> diag_id,
                              const std::optional<core::Span>& span) {
  ArgPassResult result;
  result.ok = false;
  result.diag_id = diag_id;
  result.span = span;
  return result;
}

static ArgPassResult ArgOk(const BindStateBundle& state,
                           std::set<PermKey, PermKeyLess> roots = {}) {
  ArgPassResult result;
  result.ok = true;
  result.state = state;
  result.roots = std::move(roots);
  return result;
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
          } else if (const auto* brace = std::get_if<syntax::BraceArgs>(&node.args)) {
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
        } else if constexpr (std::is_same_v<T, syntax::RecordExpr>) {
          for (const auto& field : node.fields) {
            out.push_back(field.value);
          }
        } else if constexpr (std::is_same_v<T, syntax::EnumLiteralExpr>) {
          if (node.payload_opt.has_value()) {
            if (const auto* tuple = std::get_if<syntax::EnumPayloadParen>(&*node.payload_opt)) {
              for (const auto& elem : tuple->elements) {
                out.push_back(elem);
              }
            } else if (const auto* rec = std::get_if<syntax::EnumPayloadBrace>(&*node.payload_opt)) {
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

static BindResult BindExpr(const ScopeContext& ctx,
                           const syntax::ExprPtr& expr,
                           const BindStateBundle& in);

static BindResult BindStmt(const ScopeContext& ctx,
                           const syntax::Stmt& stmt,
                           const BindStateBundle& in);

static BindResult BindStmtSeq(const ScopeContext& ctx,
                              const std::vector<syntax::Stmt>& stmts,
                              const BindStateBundle& in) {
  if (stmts.empty()) {
    SPEC_RULE("B-Seq-Empty");
    return OkResult(in);
  }
  BindStateBundle current = in;
  for (const auto& stmt : stmts) {
    const auto res = BindStmt(ctx, stmt, current);
    if (!res.ok) {
      return res;
    }
    current = res.state;
  }
  SPEC_RULE("B-Seq-Cons");
  return OkResult(current);
}

static BindResult BindBlock(const ScopeContext& ctx,
                            const syntax::Block& block,
                            const BindStateBundle& in) {
  BindStateBundle scoped = in;
  scoped.binds = PushScope_B(scoped.binds);
  scoped.perms = PushScope_Pi(scoped.perms);
  scoped.env = PushScope(scoped.env);

  auto res = BindStmtSeq(ctx, block.stmts, scoped);
  if (!res.ok) {
    return res;
  }

  BindStateBundle current = res.state;
  if (block.tail_opt) {
    const auto tail = BindExpr(ctx, block.tail_opt, current);
    if (!tail.ok) {
      return tail;
    }
    current = tail.state;
  }

  current.binds = PopScope_B(current.binds);
  current.perms = PopScope_Pi(current.perms);
  current.env = PopScope(current.env);
  SPEC_RULE("B-Block");
  return OkResult(current);
}

static bool IsMoveMissing(const syntax::ExprPtr& expr) {
  if (!expr) {
    return true;
  }
  return !std::holds_alternative<syntax::MoveExpr>(expr->node);
}

static syntax::ExprPtr WrapMoveExpr(const syntax::Arg& arg) {
  if (!arg.value) {
    return arg.value;
  }
  if (std::holds_alternative<syntax::MoveExpr>(arg.value->node)) {
    return arg.value;
  }
  auto expr = std::make_shared<syntax::Expr>();
  expr->node = syntax::MoveExpr{arg.value};
  expr->span = arg.span.file.empty() ? arg.value->span : arg.span;
  return expr;
}

static ArgPassResult ArgPass(const ScopeContext& ctx,
                             const std::vector<ParamInfo>& params,
                             const std::vector<syntax::Arg>& args,
                             const BindStateBundle& in,
                             std::size_t idx = 0) {
  if (idx >= params.size() || idx >= args.size()) {
    SPEC_RULE("B-ArgPass-Empty");
    return ArgOk(in);
  }

  const auto& param = params[idx];
  const auto& arg = args[idx];
  if (param.mode.has_value() && !arg.moved && IsMoveMissing(arg.value)) {
    SPEC_RULE("B-ArgPass-Move-Missing");
    return ArgError("B-ArgPass-Move-Missing", arg.span);
  }

  syntax::ExprPtr eval_expr = arg.value;
  if (param.mode.has_value() && arg.moved) {
    eval_expr = WrapMoveExpr(arg);
  }

  const auto eval = BindExpr(ctx, eval_expr, in);
  if (!eval.ok) {
    return ArgError(eval.diag_id, eval.span);
  }

  if (!param.mode.has_value() && !IsPlaceExpr(arg.value)) {
    return ArgPass(ctx, params, args, eval.state, idx + 1);
  }

  const auto perms_before = eval.state.perms;
  const auto perms_after =
      DowngradeUnique(ctx, eval.state.env, perms_before, param.mode, arg.value);
  BindStateBundle next = eval.state;
  next.perms = perms_after;

  const auto rest = ArgPass(ctx, params, args, next, idx + 1);
  if (!rest.ok) {
    return rest;
  }

  std::set<PermKey, PermKeyLess> roots = rest.roots;
  const auto new_roots = Roots(perms_after, perms_before);
  roots.insert(new_roots.begin(), new_roots.end());

  SPEC_RULE("B-ArgPass-Cons");
  return ArgOk(rest.state, std::move(roots));
}

static BindResult BindMoveExpr(const ScopeContext& ctx,
                               const syntax::MoveExpr& move,
                               const BindStateBundle& in) {
  const auto place = move.place;
  if (!IsPlaceExpr(place)) {
    return OkResult(in);
  }

  const auto place_type = PlaceTypeOf(ctx, in.env, place);
  if (place_type.has_value() &&
      PermOfType(*place_type) == Permission::Unique &&
      !AccessPathOk(in.perms, place)) {
    SPEC_RULE("B-Move-Unique-Err");
    return ErrorResult(std::string_view("B-Place-Unique-Err"), std::optional<core::Span>(place->span));
  }

  const auto root = PlaceRoot(place);
  if (!root.has_value()) {
    return OkResult(in);
  }
  const auto info = Lookup_B(in.binds, *root);
  if (!info.has_value()) {
    return OkResult(in);
  }

  const auto head = FieldHeadName(place);
  if (info->mov == Movability::Immov) {
    if (head.has_value()) {
      SPEC_RULE("B-Move-Field-Immovable-Err");
      return ErrorResult(std::string_view("B-Move-Field-Immovable-Err"), std::optional<core::Span>(place->span));
    }
    SPEC_RULE("B-Move-Whole-Immovable-Err");
    return ErrorResult(std::string_view("B-Move-Whole-Immovable-Err"), std::optional<core::Span>(place->span));
  }

  if (!head.has_value()) {
    if (info->state.kind != BindStateKind::Valid) {
      SPEC_RULE("B-Move-Whole-Moved-Err");
      return ErrorResult(std::string_view("B-Move-Whole-Moved-Err"), std::optional<core::Span>(place->span));
    }
    BindInfo updated = *info;
    updated.state = BindState{BindStateKind::Moved, {}};
    auto binds = Update_B(in.binds, *root, updated).value_or(in.binds);
    BindStateBundle out = in;
    out.binds = std::move(binds);
    SPEC_RULE("B-Move-Whole");
    return OkResult(out);
  }

  const auto place_perm = place_type.has_value() ? PermOfType(*place_type)
                                                 : Permission::Const;
  if (place_perm != Permission::Unique) {
    SPEC_RULE("B-Move-Field-NonUnique-Err");
    return ErrorResult(std::string_view("B-Move-Field-NonUnique-Err"), std::optional<core::Span>(place->span));
  }
  if (info->state.kind == BindStateKind::Moved ||
      (info->state.kind == BindStateKind::PartiallyMoved &&
       info->state.fields.find(*head) != info->state.fields.end())) {
    SPEC_RULE("B-Move-Field-Moved-Err");
    return ErrorResult(std::string_view("B-Move-Field-Moved-Err"), std::optional<core::Span>(place->span));
  }

  BindInfo updated = *info;
  updated.state = PM(info->state, *head);
  auto binds = Update_B(in.binds, *root, updated).value_or(in.binds);
  BindStateBundle out = in;
  out.binds = std::move(binds);
  SPEC_RULE("B-Move-Field");
  return OkResult(out);
}

static BindResult BindPlaceExpr(const ScopeContext& ctx,
                                const syntax::ExprPtr& expr,
                                const BindStateBundle& in) {
  if (AccessOk(ctx, in.env, in.binds, in.perms, expr)) {
    SPEC_RULE("B-Place");
    return OkResult(in);
  }

  const auto place_type = PlaceTypeOf(ctx, in.env, expr);
  if (place_type.has_value() &&
      PermOfType(*place_type) == Permission::Unique &&
      !AccessPathOk(in.perms, expr)) {
    SPEC_RULE("B-Place-Unique-Err");
    return ErrorResult(std::string_view("B-Place-Unique-Err"), std::optional<core::Span>(expr->span));
  }

  const auto root = PlaceRoot(expr);
  if (!root.has_value()) {
    return ErrorResult(std::nullopt, expr->span);
  }
  const auto info = Lookup_B(in.binds, *root);
  if (!info.has_value()) {
    return ErrorResult(std::nullopt, expr->span);
  }
  if (BindingMovedErrCond(*info, expr)) {
    SPEC_RULE("B-Place-Moved-Err");
    return ErrorResult(std::string_view("B-Place-Moved-Err"), std::optional<core::Span>(expr->span));
  }

  return ErrorResult(std::nullopt, expr->span);
}

static std::vector<ParamInfo> ParamInfosFromFunc(const TypeFunc& func) {
  std::vector<ParamInfo> params;
  params.reserve(func.params.size());
  for (const auto& param : func.params) {
    params.push_back(ParamInfo{param.mode});
  }
  return params;
}

static std::optional<std::vector<ParamInfo>> ParamsForCall(
    const ScopeContext& ctx,
    const TypeEnv& env,
    const syntax::ExprPtr& callee) {
  const auto type = ExprTypeOf(ctx, env, callee);
  if (!type.has_value() || !*type) {
    return std::nullopt;
  }
  if (const auto* func = std::get_if<TypeFunc>(&(*type)->node)) {
    return ParamInfosFromFunc(*func);
  }
  return std::nullopt;
}

static std::optional<std::vector<ParamInfo>> ParamsForMethod(
    const ScopeContext& ctx,
    const TypeEnv& env,
    const syntax::ExprPtr& receiver,
    std::string_view name,
    std::optional<ParamMode>& recv_mode) {
  (void)env;
  const auto recv_type = ExprTypeOf(ctx, env, receiver);
  if (!recv_type.has_value() || !*recv_type) {
    return std::nullopt;
  }

  TypeRef base = StripPerm(*recv_type);
  if (!base) {
    return std::nullopt;
  }

  if (const auto* dyn = std::get_if<TypeDynamic>(&base->node)) {
    const auto* method = LookupClassMethod(ctx, dyn->path, name);
    if (!method) {
      return std::nullopt;
    }
    if (const auto* explicit_recv =
            std::get_if<syntax::ReceiverExplicit>(&method->receiver)) {
      recv_mode = LowerParamMode(explicit_recv->mode_opt);
    }
    std::vector<ParamInfo> params;
    params.reserve(method->params.size());
    for (const auto& param : method->params) {
      params.push_back(ParamInfo{LowerParamMode(param.mode)});
    }
    return params;
  }

  const auto* path_type = std::get_if<TypePathType>(&base->node);
  if (!path_type) {
    return std::nullopt;
  }

  const syntax::RecordDecl* record = nullptr;
  std::vector<syntax::ClassPath> implements;
  const auto it = ctx.sigma.types.find(PathKeyOf(path_type->path));
  if (it != ctx.sigma.types.end()) {
    if (const auto* record_decl = std::get_if<syntax::RecordDecl>(&it->second)) {
      record = record_decl;
      implements = record_decl->implements;
    } else if (const auto* enum_decl =
                   std::get_if<syntax::EnumDecl>(&it->second)) {
      implements = enum_decl->implements;
    } else if (const auto* modal_decl =
                   std::get_if<syntax::ModalDecl>(&it->second)) {
      implements = modal_decl->implements;
    }
  }

  if (record) {
    for (const auto& member : record->members) {
      const auto* method = std::get_if<syntax::MethodDecl>(&member);
      if (!method) {
        continue;
      }
      if (!IdEq(method->name, name)) {
        continue;
      }
      if (const auto* explicit_recv =
              std::get_if<syntax::ReceiverExplicit>(&method->receiver)) {
        recv_mode = LowerParamMode(explicit_recv->mode_opt);
      }
      std::vector<ParamInfo> params;
      params.reserve(method->params.size());
      for (const auto& param : method->params) {
        params.push_back(ParamInfo{LowerParamMode(param.mode)});
      }
      return params;
    }
  }

  const syntax::ClassMethodDecl* default_method = nullptr;
  for (const auto& class_path : implements) {
    const auto table = ClassMethodTable(ctx, class_path);
    if (!table.ok) {
      continue;
    }
    for (const auto& entry : table.methods) {
      const auto* method = entry.method;
      if (!method || !method->body_opt) {
        continue;
      }
      if (!IdEq(method->name, name)) {
        continue;
      }
      if (default_method && default_method != method) {
        return std::nullopt;
      }
      default_method = method;
    }
  }

  if (default_method) {
    if (const auto* explicit_recv =
            std::get_if<syntax::ReceiverExplicit>(&default_method->receiver)) {
      recv_mode = LowerParamMode(explicit_recv->mode_opt);
    }
    std::vector<ParamInfo> params;
    params.reserve(default_method->params.size());
    for (const auto& param : default_method->params) {
      params.push_back(ParamInfo{LowerParamMode(param.mode)});
    }
    return params;
  }

  return std::nullopt;
}

static BindResult BindCallExpr(const ScopeContext& ctx,
                               const syntax::CallExpr& call,
                               const BindStateBundle& in) {
  auto callee_res = BindExpr(ctx, call.callee, in);
  if (!callee_res.ok) {
    return callee_res;
  }

  auto params = ParamsForCall(ctx, callee_res.state.env, call.callee);
  if (!params.has_value()) {
    BindStateBundle current = callee_res.state;
    for (const auto& arg : call.args) {
      const auto eval = BindExpr(ctx, arg.value, current);
      if (!eval.ok) {
        return eval;
      }
      current = eval.state;
    }
    SPEC_RULE("B-Expr-Sub");
    return OkResult(current);
  }

  const auto args_res =
      ArgPass(ctx, *params, call.args, callee_res.state);
  if (!args_res.ok) {
    return ErrorResult(args_res.diag_id, args_res.span);
  }

  BindStateBundle out = args_res.state;
  out.perms = Reactivate(out.perms, args_res.roots);
  SPEC_RULE("B-Call");
  return OkResult(out);
}

static BindResult BindMethodCallExpr(const ScopeContext& ctx,
                                     const syntax::MethodCallExpr& call,
                                     const BindStateBundle& in) {
  std::optional<ParamMode> recv_mode;
  auto params = ParamsForMethod(ctx, in.env, call.receiver, call.name, recv_mode);
  if (!params.has_value()) {
    BindStateBundle current = in;
    const auto base = BindExpr(ctx, call.receiver, current);
    if (!base.ok) {
      return base;
    }
    current = base.state;
    for (const auto& arg : call.args) {
      const auto eval = BindExpr(ctx, arg.value, current);
      if (!eval.ok) {
        return eval;
      }
      current = eval.state;
    }
    SPEC_RULE("B-Expr-Sub");
    return OkResult(current);
  }

  if (recv_mode.has_value() && IsMoveMissing(call.receiver)) {
    SPEC_RULE("B-ArgPass-Move-Missing");
    return ErrorResult(std::string_view("B-ArgPass-Move-Missing"), std::optional<core::Span>(call.receiver->span));
  }

  const auto recv_res = BindExpr(ctx, call.receiver, in);
  if (!recv_res.ok) {
    return recv_res;
  }

  const auto perms_before = recv_res.state.perms;
  const auto perms_after =
      DowngradeUnique(ctx, recv_res.state.env, perms_before, recv_mode,
                      call.receiver);
  BindStateBundle recv_state = recv_res.state;
  recv_state.perms = perms_after;

  const auto recv_roots = Roots(perms_after, perms_before);

  const auto args_res = ArgPass(ctx, *params, call.args, recv_state);
  if (!args_res.ok) {
    return ErrorResult(args_res.diag_id, args_res.span);
  }

  std::set<PermKey, PermKeyLess> all_roots = recv_roots;
  all_roots.insert(args_res.roots.begin(), args_res.roots.end());

  BindStateBundle out = args_res.state;
  out.perms = Reactivate(out.perms, all_roots);
  SPEC_RULE("B-MethodCall");
  return OkResult(out);
}

static BindResult BindIfExpr(const ScopeContext& ctx,
                             const syntax::IfExpr& expr,
                             const BindStateBundle& in) {
  const auto cond = BindExpr(ctx, expr.cond, in);
  if (!cond.ok) {
    return cond;
  }

  const auto then_res = BindExpr(ctx, expr.then_expr, cond.state);
  if (!then_res.ok) {
    return then_res;
  }
  const auto else_res = BindExpr(ctx, expr.else_expr, cond.state);
  if (!else_res.ok) {
    return else_res;
  }

  const auto joined_b = Join_B(then_res.state.binds, else_res.state.binds);
  const auto joined_p = JoinPerm(then_res.state.perms, else_res.state.perms);
  if (!joined_b.has_value() || !joined_p.has_value()) {
    return ErrorResult(std::nullopt, expr.cond ? expr.cond->span : expr.else_expr->span);
  }

  BindStateBundle out = cond.state;
  out.binds = *joined_b;
  out.perms = *joined_p;
  SPEC_RULE("B-If");
  return OkResult(out);
}

static BindResult BindMatchExpr(const ScopeContext& ctx,
                                const syntax::MatchExpr& expr,
                                const BindStateBundle& in) {
  const auto scrutinee = BindExpr(ctx, expr.value, in);
  if (!scrutinee.ok) {
    return scrutinee;
  }

  const auto scrut_type = ExprTypeOf(ctx, scrutinee.state.env, expr.value);
  if (!scrut_type.has_value()) {
    return ErrorResult(std::nullopt, expr.value ? std::optional<core::Span>(expr.value->span) : std::nullopt);
  }

  const bool moved = IsMoveExpr(expr.value);
  BindStateBundle base = scrutinee.state;
  base.binds = ConsumeOnMove(base.binds, expr.value);

  std::vector<BindEnv> bind_envs;
  std::vector<PermEnv> perm_envs;
  for (const auto& arm : expr.arms) {
    if (!arm.pattern || !arm.body) {
      return ErrorResult(std::nullopt, arm.span);
    }
    const auto pat = TypeMatchPattern(ctx, arm.pattern, *scrut_type);
    if (!pat.ok) {
      return ErrorResult(pat.diag_id, arm.pattern->span);
    }
    const auto type_map = BindTypeMapFromBindings(pat.bindings);
    const Responsibility resp = moved ? Responsibility::Resp
                                      : Responsibility::Alias;

    BindStateBundle arm_state = base;
    arm_state.binds = PushScope_B(arm_state.binds);
    arm_state.perms = PushScope_Pi(arm_state.perms);
    arm_state.env = PushScope(arm_state.env);

    arm_state.binds = IntroAll_B(
        arm_state.binds, BindInfoMap(type_map, resp, Movability::Mov,
                                     syntax::Mutability::Let));
    for (const auto& [name, type] : pat.bindings) {
      arm_state.env.scopes.back()[IdKeyOf(name)] =
          TypeBinding{syntax::Mutability::Let, type};
    }

    if (arm.guard_opt) {
      const auto guard = BindExpr(ctx, arm.guard_opt, arm_state);
      if (!guard.ok) {
        return guard;
      }
      arm_state = guard.state;
    }

    const auto body = BindExpr(ctx, arm.body, arm_state);
    if (!body.ok) {
      return body;
    }
    arm_state = body.state;

    SPEC_RULE("B-Arm");

    arm_state.binds = PopScope_B(arm_state.binds);
    arm_state.perms = PopScope_Pi(arm_state.perms);
    arm_state.env = PopScope(arm_state.env);

    bind_envs.push_back(arm_state.binds);
    perm_envs.push_back(arm_state.perms);
  }

  const auto joined_b = JoinAll_B(bind_envs);
  const auto joined_p = JoinAllPerm(perm_envs);
  if (!joined_b.has_value() || !joined_p.has_value()) {
    return ErrorResult(std::nullopt, expr.value ? std::optional<core::Span>(expr.value->span) : std::nullopt);
  }

  BindStateBundle out = scrutinee.state;
  out.binds = *joined_b;
  out.perms = *joined_p;
  SPEC_RULE("B-Match");
  return OkResult(out);
}

static std::optional<TypeRef> IterElementType(const TypeRef& iter_type) {
  const auto stripped = StripPerm(iter_type);
  if (!stripped) {
    return std::nullopt;
  }
  if (const auto* slice = std::get_if<TypeSlice>(&stripped->node)) {
    return slice->element;
  }
  if (const auto* array = std::get_if<TypeArray>(&stripped->node)) {
    return array->element;
  }
  return std::nullopt;
}

static BindResult LoopFix(const ScopeContext& ctx,
                          const BindStateBundle& init,
                          const std::function<BindResult(const BindStateBundle&)>& step) {
  BindStateBundle current = init;
  for (std::size_t iter = 0; iter < 128; ++iter) {
    const auto body = step(current);
    if (!body.ok) {
      return body;
    }
    const auto joined_b = Join_B(init.binds, body.state.binds);
    const auto joined_p = JoinPerm(init.perms, body.state.perms);
    if (!joined_b.has_value() || !joined_p.has_value()) {
      return ErrorResult(std::nullopt, std::nullopt);
    }
    BindStateBundle next = current;
    next.binds = *joined_b;
    next.perms = *joined_p;
    if (BindEnvEqual(next.binds, current.binds) &&
        PermEnvEqual(next.perms, current.perms)) {
      return OkResult(next);
    }
    current = next;
  }
  return ErrorResult(std::nullopt, std::nullopt);
}

static BindResult BindLoopInfinite(const ScopeContext& ctx,
                                   const syntax::LoopInfiniteExpr& loop,
                                   const BindStateBundle& in) {
  auto step = [&](const BindStateBundle& state) {
    return BindBlock(ctx, *loop.body, state);
  };
  const auto fixed = LoopFix(ctx, in, step);
  if (!fixed.ok) {
    return fixed;
  }
  SPEC_RULE("B-Loop-Infinite");
  return fixed;
}

static BindResult BindLoopConditional(const ScopeContext& ctx,
                                      const syntax::LoopConditionalExpr& loop,
                                      const BindStateBundle& in) {
  auto step = [&](const BindStateBundle& state) {
    const auto cond = BindExpr(ctx, loop.cond, state);
    if (!cond.ok) {
      return cond;
    }
    return BindBlock(ctx, *loop.body, cond.state);
  };
  const auto fixed = LoopFix(ctx, in, step);
  if (!fixed.ok) {
    return fixed;
  }
  SPEC_RULE("B-Loop-Conditional");
  return fixed;
}

static BindResult BindLoopIter(const ScopeContext& ctx,
                               const syntax::LoopIterExpr& loop,
                               const BindStateBundle& in) {
  const auto iter_eval = BindExpr(ctx, loop.iter, in);
  if (!iter_eval.ok) {
    return iter_eval;
  }

  TypeRef pat_type;
  if (loop.type_opt) {
    const auto lowered = LowerType(ctx, loop.type_opt);
    if (!lowered.ok) {
      return ErrorResult(lowered.diag_id, loop.type_opt->span);
    }
    pat_type = lowered.type;
  } else {
    const auto iter_type = ExprTypeOf(ctx, iter_eval.state.env, loop.iter);
    if (!iter_type.has_value()) {
      return ErrorResult(std::nullopt, loop.iter ? std::optional<core::Span>(loop.iter->span) : std::nullopt);
    }
    const auto elem = IterElementType(*iter_type);
    if (!elem.has_value()) {
      return ErrorResult(std::nullopt, loop.iter ? std::optional<core::Span>(loop.iter->span) : std::nullopt);
    }
    pat_type = *elem;
  }

  const auto pat = TypePattern(ctx, loop.pattern, pat_type);
  if (!pat.ok) {
    return ErrorResult(pat.diag_id, loop.pattern->span);
  }

  const auto type_map = BindTypeMapFromBindings(pat.bindings);

  auto step = [&](const BindStateBundle& state) {
    BindStateBundle scoped = state;
    scoped.binds = PushScope_B(scoped.binds);
    scoped.perms = PushScope_Pi(scoped.perms);
    scoped.env = PushScope(scoped.env);

    scoped.binds = IntroAll_B(
        scoped.binds, BindInfoMap(type_map, Responsibility::Resp,
                                  Movability::Mov, syntax::Mutability::Let));
    for (const auto& [name, type] : pat.bindings) {
      scoped.env.scopes.back()[IdKeyOf(name)] =
          TypeBinding{syntax::Mutability::Let, type};
    }

    const auto body = BindBlock(ctx, *loop.body, scoped);
    if (!body.ok) {
      return body;
    }
    BindStateBundle out = body.state;
    out.binds = PopScope_B(out.binds);
    out.perms = PopScope_Pi(out.perms);
    out.env = PopScope(out.env);
    return OkResult(out);
  };

  const auto fixed = LoopFix(ctx, iter_eval.state, step);
  if (!fixed.ok) {
    return fixed;
  }

  SPEC_RULE("B-Loop-Iter");
  return fixed;
}

static BindResult BindExpr(const ScopeContext& ctx,
                           const syntax::ExprPtr& expr,
                           const BindStateBundle& in) {
  if (!expr) {
    return OkResult(in);
  }

  if (const auto* move = std::get_if<syntax::MoveExpr>(&expr->node)) {
    return BindMoveExpr(ctx, *move, in);
  }

  if (IsPlaceExpr(expr)) {
    return BindPlaceExpr(ctx, expr, in);
  }

  if (const auto* call = std::get_if<syntax::CallExpr>(&expr->node)) {
    return BindCallExpr(ctx, *call, in);
  }

  if (const auto* method = std::get_if<syntax::MethodCallExpr>(&expr->node)) {
    return BindMethodCallExpr(ctx, *method, in);
  }

  if (const auto* ifexpr = std::get_if<syntax::IfExpr>(&expr->node)) {
    return BindIfExpr(ctx, *ifexpr, in);
  }

  if (const auto* match = std::get_if<syntax::MatchExpr>(&expr->node)) {
    return BindMatchExpr(ctx, *match, in);
  }

  if (const auto* block = std::get_if<syntax::BlockExpr>(&expr->node)) {
    return BindBlock(ctx, *block->block, in);
  }

  if (const auto* unsafe_block =
          std::get_if<syntax::UnsafeBlockExpr>(&expr->node)) {
    return BindBlock(ctx, *unsafe_block->block, in);
  }

  if (const auto* loop = std::get_if<syntax::LoopInfiniteExpr>(&expr->node)) {
    return BindLoopInfinite(ctx, *loop, in);
  }

  if (const auto* loop =
          std::get_if<syntax::LoopConditionalExpr>(&expr->node)) {
    return BindLoopConditional(ctx, *loop, in);
  }

  if (const auto* loop = std::get_if<syntax::LoopIterExpr>(&expr->node)) {
    return BindLoopIter(ctx, *loop, in);
  }

  BindStateBundle current = in;
  for (const auto& child : ChildrenLtr(expr)) {
    const auto res = BindExpr(ctx, child, current);
    if (!res.ok) {
      return res;
    }
    current = res.state;
  }
  SPEC_RULE("B-Expr-Sub");
  return OkResult(current);
}

static bool PlaceConstPerm(const ScopeContext& ctx,
                           const TypeEnv& env,
                           const syntax::ExprPtr& place) {
  const auto type = PlaceTypeOf(ctx, env, place);
  if (!type.has_value()) {
    return false;
  }
  if (const auto* perm = std::get_if<TypePerm>(&(*type)->node)) {
    return perm->perm == Permission::Const;
  }
  return false;
}

static BindResult BindStmt(const ScopeContext& ctx,
                           const syntax::Stmt& stmt,
                           const BindStateBundle& in) {
  return std::visit(
      [&](const auto& node) -> BindResult {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::LetStmt> ||
                      std::is_same_v<T, syntax::VarStmt>) {
          const auto& binding = node.binding;
          std::optional<std::string_view> diag_id;
          const auto bind_type = BindTypeForBinding(ctx, in.env, binding, diag_id);
          if (!bind_type.has_value()) {
            return ErrorResult(diag_id, binding.span);
          }

          if (PermOfType(*bind_type) == Permission::Unique &&
              IsPlaceExpr(binding.init) && !IsMoveExpr(binding.init)) {
            SPEC_RULE("B-LetVar-UniqueNonMove-Err");
            return ErrorResult(std::string_view("B-LetVar-UniqueNonMove-Err"), std::optional<core::Span>(binding.init->span));
          }

          const auto init_res = BindExpr(ctx, binding.init, in);
          if (!init_res.ok) {
            return init_res;
          }

          BindStateBundle current = init_res.state;
          current.perms = DowngradeUniqueBind(ctx, current.env, current.perms,
                                              binding.init, *bind_type);
          current.binds = ConsumeOnMove(current.binds, binding.init);

          const auto pat = TypePattern(ctx, binding.pat, *bind_type);
          if (!pat.ok) {
            return ErrorResult(pat.diag_id, binding.pat->span);
          }
          const auto type_map = BindTypeMapFromBindings(pat.bindings);
          const Responsibility resp = RespOfInit(binding.init);
          const Movability mv = MovOf(binding.op);
          const auto mut = std::is_same_v<T, syntax::LetStmt>
                               ? syntax::Mutability::Let
                               : syntax::Mutability::Var;

          current.binds = IntroAll_B(
              current.binds, BindInfoMap(type_map, resp, mv, mut));
          for (const auto& [name, type] : pat.bindings) {
            if (current.env.scopes.empty()) {
              current.env.scopes.emplace_back();
            }
            current.env.scopes.back()[IdKeyOf(name)] = TypeBinding{mut, type};
          }
          SPEC_RULE("B-LetVar");
          return OkResult(current);
        } else if constexpr (std::is_same_v<T, syntax::ShadowLetStmt>) {
          std::optional<std::string_view> diag_id;
          const auto bind_type = BindTypeForShadow(ctx, in.env, node, diag_id);
          if (!bind_type.has_value()) {
            return ErrorResult(diag_id, node.span);
          }
          if (PermOfType(*bind_type) == Permission::Unique &&
              IsPlaceExpr(node.init) && !IsMoveExpr(node.init)) {
            SPEC_RULE("B-ShadowLet-UniqueNonMove-Err");
            return ErrorResult(std::string_view("B-ShadowLet-UniqueNonMove-Err"), std::optional<core::Span>(node.init->span));
          }
          const auto init_res = BindExpr(ctx, node.init, in);
          if (!init_res.ok) {
            return init_res;
          }
          BindStateBundle current = init_res.state;
          current.perms = DowngradeUniqueBind(ctx, current.env, current.perms,
                                              node.init, *bind_type);
          current.binds = ConsumeOnMove(current.binds, node.init);

          std::map<IdKey, TypeRef> type_map;
          type_map.emplace(IdKeyOf(node.name), *bind_type);
          const auto info =
              BindInfoMap(type_map, RespOfInit(node.init), Movability::Mov,
                          syntax::Mutability::Let);
          const auto updated = ShadowAll_B(current.binds, info);
          if (!updated.has_value()) {
            return ErrorResult(std::nullopt, node.span);
          }
          current.binds = *updated;

          const auto key = IdKeyOf(node.name);
          for (auto it = current.env.scopes.rbegin(); it != current.env.scopes.rend(); ++it) {
            const auto found = it->find(key);
            if (found != it->end()) {
              found->second = TypeBinding{syntax::Mutability::Let, *bind_type};
              break;
            }
          }
          SPEC_RULE("B-ShadowLet");
          return OkResult(current);
        } else if constexpr (std::is_same_v<T, syntax::ShadowVarStmt>) {
          std::optional<std::string_view> diag_id;
          const auto bind_type = BindTypeForShadow(ctx, in.env, node, diag_id);
          if (!bind_type.has_value()) {
            return ErrorResult(diag_id, node.span);
          }
          if (PermOfType(*bind_type) == Permission::Unique &&
              IsPlaceExpr(node.init) && !IsMoveExpr(node.init)) {
            SPEC_RULE("B-ShadowVar-UniqueNonMove-Err");
            return ErrorResult(std::string_view("B-ShadowVar-UniqueNonMove-Err"), std::optional<core::Span>(node.init->span));
          }
          const auto init_res = BindExpr(ctx, node.init, in);
          if (!init_res.ok) {
            return init_res;
          }
          BindStateBundle current = init_res.state;
          current.perms = DowngradeUniqueBind(ctx, current.env, current.perms,
                                              node.init, *bind_type);
          current.binds = ConsumeOnMove(current.binds, node.init);

          std::map<IdKey, TypeRef> type_map;
          type_map.emplace(IdKeyOf(node.name), *bind_type);
          const auto info =
              BindInfoMap(type_map, RespOfInit(node.init), Movability::Mov,
                          syntax::Mutability::Var);
          const auto updated = ShadowAll_B(current.binds, info);
          if (!updated.has_value()) {
            return ErrorResult(std::nullopt, node.span);
          }
          current.binds = *updated;

          const auto key = IdKeyOf(node.name);
          for (auto it = current.env.scopes.rbegin(); it != current.env.scopes.rend(); ++it) {
            const auto found = it->find(key);
            if (found != it->end()) {
              found->second = TypeBinding{syntax::Mutability::Var, *bind_type};
              break;
            }
          }
          SPEC_RULE("B-ShadowVar");
          return OkResult(current);
        } else if constexpr (std::is_same_v<T, syntax::AssignStmt> ||
                             std::is_same_v<T, syntax::CompoundAssignStmt>) {
          const auto place = node.place;
          if (IsPlaceExpr(place)) {
            const auto root = PlaceRoot(place);
            if (root.has_value()) {
              const auto info = Lookup_B(in.binds, *root);
              if (info.has_value() && info->mut == syntax::Mutability::Let) {
                SPEC_RULE("B-Assign-Immutable-Err");
                return ErrorResult(std::string_view("B-Assign-Immutable-Err"), std::optional<core::Span>(node.span));
              }
            }
            if (PlaceConstPerm(ctx, in.env, place)) {
              SPEC_RULE("B-Assign-Const-Err");
              return ErrorResult(std::string_view("B-Assign-Const-Err"), std::optional<core::Span>(node.span));
            }
          }

          BindStateBundle current = in;
          if constexpr (std::is_same_v<T, syntax::AssignStmt>) {
            const auto val = BindExpr(ctx, node.value, current);
            if (!val.ok) {
              return val;
            }
            current = val.state;
          } else {
            const auto lhs = BindExpr(ctx, node.place, current);
            if (!lhs.ok) {
              return lhs;
            }
            const auto rhs = BindExpr(ctx, node.value, lhs.state);
            if (!rhs.ok) {
              return rhs;
            }
            current = rhs.state;
          }

          if (IsPlaceExpr(place)) {
            const auto root = PlaceRoot(place);
            if (root.has_value()) {
              const auto info = Lookup_B(current.binds, *root);
              if (info.has_value() && info->mut == syntax::Mutability::Var) {
                BindInfo updated = *info;
                updated.state = BindState{BindStateKind::Valid, {}};
                const auto res = Update_B(current.binds, *root, updated);
                if (res.has_value()) {
                  current.binds = *res;
                }
              }
            }
          }
          SPEC_RULE("B-Assign");
          return OkResult(current);
        } else if constexpr (std::is_same_v<T, syntax::ExprStmt>) {
          const auto res = BindExpr(ctx, node.value, in);
          if (!res.ok) {
            return res;
          }
          SPEC_RULE("B-ExprStmt");
          return res;
        } else if constexpr (std::is_same_v<T, syntax::ReturnStmt>) {
          if (!node.value_opt) {
            SPEC_RULE("B-Return-Unit");
            return OkResult(in);
          }
          const auto res = BindExpr(ctx, node.value_opt, in);
          if (!res.ok) {
            return res;
          }
          SPEC_RULE("B-Return");
          return res;
        } else if constexpr (std::is_same_v<T, syntax::BreakStmt>) {
          if (!node.value_opt) {
            SPEC_RULE("B-Break-Unit");
            return OkResult(in);
          }
          const auto res = BindExpr(ctx, node.value_opt, in);
          if (!res.ok) {
            return res;
          }
          SPEC_RULE("B-Break");
          return res;
        } else if constexpr (std::is_same_v<T, syntax::ContinueStmt>) {
          SPEC_RULE("B-Continue");
          return OkResult(in);
        } else if constexpr (std::is_same_v<T, syntax::UnsafeBlockStmt>) {
          const auto res = BindBlock(ctx, *node.body, in);
          if (!res.ok) {
            return res;
          }
          SPEC_RULE("B-UnsafeStmt");
          return res;
        } else if constexpr (std::is_same_v<T, syntax::DeferStmt>) {
          const auto res = BindBlock(ctx, *node.body, in);
          if (!res.ok) {
            return res;
          }
          if (!BindEnvEqual(res.state.binds, in.binds) ||
              !PermEnvEqual(res.state.perms, in.perms)) {
            return ErrorResult(std::nullopt, node.span);
          }
          SPEC_RULE("B-Defer");
          return OkResult(in);
        } else if constexpr (std::is_same_v<T, syntax::RegionStmt>) {
          BindStateBundle current = in;
          if (node.opts_opt) {
            const auto opts_res = BindExpr(ctx, node.opts_opt, current);
            if (!opts_res.ok) {
              return opts_res;
            }
            current = opts_res.state;
          }

          std::string name = node.alias_opt.has_value()
                                 ? *node.alias_opt
                                 : FreshRegionName(current.env);

          const auto region_type = MakeTypeModalState({"Region"}, "Active");
          std::map<IdKey, TypeRef> type_map;
          type_map.emplace(IdKeyOf(name), region_type);

          BindStateBundle scoped = current;
          scoped.binds = PushScope_B(scoped.binds);
          scoped.perms = PushScope_Pi(scoped.perms);
          scoped.env = PushScope(scoped.env);

          scoped.binds = IntroAll_B(
              scoped.binds,
              BindInfoMap(type_map, Responsibility::Resp, Movability::Mov,
                          syntax::Mutability::Let));
          scoped.env.scopes.back()[IdKeyOf(name)] =
              TypeBinding{syntax::Mutability::Let, region_type};

          const auto body = BindBlock(ctx, *node.body, scoped);
          if (!body.ok) {
            return body;
          }

          BindStateBundle out = body.state;
          out.binds = PopScope_B(out.binds);
          out.perms = PopScope_Pi(out.perms);
          out.env = PopScope(out.env);
          SPEC_RULE("B-RegionStmt");
          return OkResult(out);
        } else if constexpr (std::is_same_v<T, syntax::FrameStmt>) {
          BindStateBundle scoped = in;
          scoped.binds = PushScope_B(scoped.binds);
          scoped.perms = PushScope_Pi(scoped.perms);
          scoped.env = PushScope(scoped.env);

          const std::string name = FreshRegionName(scoped.env);

          const auto region_type = MakeTypeModalState({"Region"}, "Active");
          std::map<IdKey, TypeRef> type_map;
          type_map.emplace(IdKeyOf(name), region_type);

          scoped.binds = IntroAll_B(
              scoped.binds,
              BindInfoMap(type_map, Responsibility::Resp, Movability::Mov,
                          syntax::Mutability::Let));
          scoped.env.scopes.back()[IdKeyOf(name)] =
              TypeBinding{syntax::Mutability::Let, region_type};

          const auto body = BindBlock(ctx, *node.body, scoped);
          if (!body.ok) {
            return body;
          }

          BindStateBundle out = body.state;
          out.binds = PopScope_B(out.binds);
          out.perms = PopScope_Pi(out.perms);
          out.env = PopScope(out.env);
          SPEC_RULE("B-FrameStmt");
          return OkResult(out);
        } else if constexpr (std::is_same_v<T, syntax::KeyBlockStmt>) {
          // C0X Extension: Key block statement
          // Key blocks introduce a new scope for permission tracking
          SPEC_RULE("B-KeyBlockStmt");
          
          BindStateBundle scoped = in;
          scoped.binds = PushScope_B(scoped.binds);
          scoped.perms = PushScope_Pi(scoped.perms);
          scoped.env = PushScope(scoped.env);
          
          // Process the body with the key scope
          const auto body = BindBlock(ctx, *node.body, scoped);
          if (!body.ok) {
            return body;
          }
          
          BindStateBundle out = body.state;
          out.binds = PopScope_B(out.binds);
          out.perms = PopScope_Pi(out.perms);
          out.env = PopScope(out.env);
          return OkResult(out);
        } else if constexpr (std::is_same_v<T, syntax::ErrorStmt>) {
          return OkResult(in);
        } else {
          return OkResult(in);
        }
      },
      stmt);
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

static BindScope StaticBindInfo(const ScopeContext& ctx,
                                const syntax::StaticDecl& decl,
                                TypeEnv& env) {
  BindScope out;
  if (!decl.binding.type_opt) {
    return out;
  }
  const auto ann = LowerType(ctx, decl.binding.type_opt);
  if (!ann.ok) {
    return out;
  }
  const auto pat = TypeMatchPattern(ctx, decl.binding.pat, ann.type);
  if (!pat.ok) {
    return out;
  }
  for (const auto& [name, type] : pat.bindings) {
    if (env.scopes.empty()) {
      env.scopes.emplace_back();
    }
    env.scopes.front()[IdKeyOf(name)] = TypeBinding{decl.mut, type};
  }
  const auto type_map = BindTypeMapFromBindings(pat.bindings);
  const auto resp = RespOfInit(decl.binding.init);
  const auto mv = MovOf(decl.binding.op);
  const auto info = BindInfoMap(type_map, resp, mv, decl.mut);
  out.insert(info.begin(), info.end());
  return out;
}

static BindScope StaticBindMap(const ScopeContext& ctx,
                               const syntax::ModulePath& module_path,
                               TypeEnv& env) {
  BindScope out;
  const auto* module = FindModuleByPath(ctx, module_path);
  if (!module) {
    return out;
  }
  for (const auto& item : module->items) {
    const auto* decl = std::get_if<syntax::StaticDecl>(&item);
    if (!decl) {
      continue;
    }
    const auto info = StaticBindInfo(ctx, *decl, env);
    out.insert(info.begin(), info.end());
  }
  return out;
}

static BindScope ParamBindMap(const std::vector<syntax::Param>& params,
                              const std::optional<BindSelfParam>& self_param) {
  BindScope out;
  if (self_param.has_value()) {
    BindInfo info;
    info.state = BindState{BindStateKind::Valid, {}};
    info.mov = self_param->mode.has_value() ? Movability::Mov : Movability::Immov;
    info.mut = syntax::Mutability::Let;
    info.resp = self_param->mode.has_value() ? Responsibility::Resp
                                             : Responsibility::Alias;
    out.emplace(IdKeyOf("self"), info);
  }
  for (const auto& param : params) {
    BindInfo info;
    info.state = BindState{BindStateKind::Valid, {}};
    info.mov = param.mode.has_value() ? Movability::Mov : Movability::Immov;
    info.mut = syntax::Mutability::Let;
    info.resp = param.mode.has_value() ? Responsibility::Resp
                                       : Responsibility::Alias;
    out.emplace(IdKeyOf(param.name), info);
  }
  return out;
}

static void ParamTypeMap(const ScopeContext& ctx,
                         const std::vector<syntax::Param>& params,
                         const std::optional<BindSelfParam>& self_param,
                         TypeEnv& env) {
  TypeRef self_base;
  if (self_param.has_value()) {
    self_base = StripPerm(self_param->type);
    if (env.scopes.empty()) {
      env.scopes.emplace_back();
    }
    env.scopes.back()[IdKeyOf("self")] =
        TypeBinding{syntax::Mutability::Let, self_param->type};
  }
  for (const auto& param : params) {
    const auto lowered = LowerType(ctx, param.type);
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

}  // namespace

BindCheckResult BindCheckBody(const ScopeContext& ctx,
                              const syntax::ModulePath& module_path,
                              const std::vector<syntax::Param>& params,
                              const std::shared_ptr<syntax::Block>& body,
                              const std::optional<BindSelfParam>& self_param) {
  SpecDefsBorrowBind();
  SpecRuleTransitionAnchor();
  BindCheckResult result;
  if (!body) {
    result.ok = true;
    return result;
  }

  BindEnv binds;
  PermEnv perms;
  TypeEnv env;

  env.scopes.emplace_back();
  const auto static_info = StaticBindMap(ctx, module_path, env);
  binds = PushScope_B(binds);
  binds = IntroAll_B(binds, static_info);

  perms.emplace_back();
  perms.emplace_back();

  env.scopes.emplace_back();
  const auto param_info = ParamBindMap(params, self_param);
  binds = PushScope_B(binds);
  binds = IntroAll_B(binds, param_info);
  ParamTypeMap(ctx, params, self_param, env);

  BindStateBundle state{binds, perms, env};
  const auto checked = BindBlock(ctx, *body, state);
  if (!checked.ok) {
    result.ok = false;
    result.diag_id = checked.diag_id;
    result.span = checked.span;
    return result;
  }

  result.ok = true;
  return result;
}

}  // namespace cursive0::analysis
