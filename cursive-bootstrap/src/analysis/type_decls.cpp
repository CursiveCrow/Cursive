#include "cursive0/sema/type_decls.h"

#include <algorithm>
#include <optional>
#include <string>
#include <string_view>
#include <set>
#include <unordered_set>
#include <utility>
#include <vector>

#include "cursive0/core/assert_spec.h"
#include "cursive0/core/diagnostic_messages.h"
#include "cursive0/sema/borrow_bind.h"
#include "cursive0/sema/classes.h"
#include "cursive0/sema/enums.h"
#include "cursive0/sema/regions.h"
#include "cursive0/sema/records.h"
#include "cursive0/sema/record_methods.h"
#include "cursive0/sema/scopes.h"
#include "cursive0/sema/scopes_lookup.h"
#include "cursive0/sema/subtyping.h"
#include "cursive0/sema/type_equiv.h"
#include "cursive0/sema/type_expr.h"
#include "cursive0/sema/type_infer.h"
#include "cursive0/sema/type_match.h"
#include "cursive0/sema/type_pattern.h"
#include "cursive0/sema/type_stmt.h"
#include "cursive0/sema/types.h"

namespace cursive0::sema {

namespace {

struct DiagMapEntry {
  const char* diag_id;
  const char* code;
};

#include "typecheck_diag_map.inc"

static std::optional<std::string_view> CodeForTypecheckDiag(
    std::string_view diag_id) {
  for (const auto& entry : kTypecheckDiagMap) {
    if (diag_id == entry.diag_id) {
      return std::string_view(entry.code);
    }
  }
  return std::nullopt;
}

static void EmitTypecheckDiag(core::DiagnosticStream& diags,
                              std::string_view diag_id,
                              const std::optional<core::Span>& span) {
  const auto code = CodeForTypecheckDiag(diag_id);
  if (!code.has_value()) {
    core::Diagnostic diag;
    diag.code = std::string(diag_id);
    diag.severity = core::Severity::Error;
    diag.message = "Internal error: unknown typecheck diagnostic id";
    diag.span = span;
    diags = core::Emit(diags, diag);
    return;
  }
  if (auto diag = core::MakeDiagnostic(*code, span)) {
    diags = core::Emit(diags, *diag);
    return;
  }
  core::Diagnostic diag;
  diag.code = std::string(*code);
  diag.severity = core::Severity::Error;
  diag.message = "Internal error: unknown diagnostic code";
  diag.span = span;
  diags = core::Emit(diags, diag);
}

static inline void SpecDefsDeclTyping() {
  SPEC_DEF("DeclJudg", "5.2.14");
  SPEC_DEF("DeclTyping", "5.2.14");
  SPEC_DEF("DeclTypingMod", "5.2.14");
  SPEC_DEF("DeclTypingItem", "5.2.14");
  SPEC_DEF("ProvBindCheck", "5.2.14");
  SPEC_DEF("ParamBinds", "5.2.14");
  SPEC_DEF("ProcReturn", "5.2.14");
  SPEC_DEF("ReturnAnnOk", "5.2.14");
  SPEC_DEF("StaticVisOk", "5.2.14");
  SPEC_DEF("VisRank", "5.2.14");
  SPEC_DEF("FieldVisOk", "5.2.14");
  SPEC_DEF("StateMemberVisOk", "5.2.14");
  SPEC_DEF("MainDecls", "5.2.14");
  SPEC_DEF("MainGeneric", "5.2.14");
  SPEC_DEF("MainSigOk", "5.2.14");
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
  const auto lowered = std::visit(
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
  if (!lowered.ok) {
    return lowered;
  }
  const auto wf = TypeWF(ctx, lowered.type);
  if (!wf.ok) {
    return {false, wf.diag_id, {}};
  }
  return lowered;
}

static TypeLowerResult LowerReturnType(const ScopeContext& ctx,
                                       const std::shared_ptr<syntax::Type>& type_opt) {
  if (!type_opt) {
    return {true, std::nullopt, MakeTypePrim("()")};
  }
  return LowerType(ctx, type_opt);
}

static TypePath TypePathForItem(const syntax::ModulePath& module_path,
                                std::string_view name) {
  TypePath out;
  out.reserve(module_path.size() + 1);
  for (const auto& part : module_path) {
    out.push_back(part);
  }
  out.push_back(std::string(name));
  return out;
}

static bool DistinctNames(const std::vector<IdKey>& names) {
  if (names.size() < 2) {
    return true;
  }
  std::vector<IdKey> sorted = names;
  std::sort(sorted.begin(), sorted.end());
  return std::adjacent_find(sorted.begin(), sorted.end()) == sorted.end();
}

static std::vector<IdKey> ParamNames(const std::vector<syntax::Param>& params) {
  std::vector<IdKey> names;
  names.reserve(params.size());
  for (const auto& param : params) {
    names.push_back(IdKeyOf(param.name));
  }
  return names;
}

static bool AddBinding(TypeEnv& env,
                       std::string_view name,
                       const TypeRef& type) {
  if (env.scopes.empty()) {
    env.scopes.emplace_back();
  }
  const auto key = IdKeyOf(name);
  if (env.scopes.back().find(key) != env.scopes.back().end()) {
    return false;
  }
  env.scopes.back()[key] = {syntax::Mutability::Let, type};
  return true;
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
          return type;
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
        } else {
          return type;
        }
      },
      type->node);
}

static bool StaticVisOk(syntax::Visibility vis, syntax::Mutability mut) {
  return !(vis == syntax::Visibility::Public && mut == syntax::Mutability::Var);
}

static int VisRank(syntax::Visibility vis) {
  switch (vis) {
    case syntax::Visibility::Public:
      return 3;
    case syntax::Visibility::Internal:
      return 2;
    case syntax::Visibility::Private:
    case syntax::Visibility::Protected:
      return 1;
  }
  return 1;
}

static bool FieldVisOk(const syntax::RecordDecl& record) {
  for (const auto& member : record.members) {
    const auto* field = std::get_if<syntax::FieldDecl>(&member);
    if (!field) {
      continue;
    }
    if (VisRank(field->vis) > VisRank(record.vis)) {
      return false;
    }
  }
  return true;
}

static bool StateMemberVisOk(const syntax::ModalDecl& modal) {
  for (const auto& state : modal.states) {
    for (const auto& member : state.members) {
      std::optional<syntax::Visibility> vis;
      if (const auto* field = std::get_if<syntax::StateFieldDecl>(&member)) {
        vis = field->vis;
      } else if (const auto* method = std::get_if<syntax::StateMethodDecl>(&member)) {
        vis = method->vis;
      } else if (const auto* transition =
                     std::get_if<syntax::TransitionDecl>(&member)) {
        vis = transition->vis;
      }
      if (vis.has_value() && VisRank(*vis) > VisRank(modal.vis)) {
        return false;
      }
    }
  }
  return true;
}

static ExprTypeResult TypeBlockForDecl(const ScopeContext& ctx,
                                       const syntax::Block& block,
                                       const TypeEnv& env,
                                       const TypeRef& return_type,
                                       core::DiagnosticStream& diags) {
  StmtTypeContext type_ctx;
  type_ctx.return_type = return_type;
  type_ctx.loop_flag = LoopFlag::None;
  type_ctx.in_unsafe = false;
  type_ctx.diags = &diags;

  TypeEnv live_env = env;
  type_ctx.env_ref = &live_env;
  auto active_env = [&]() -> const TypeEnv& {
    return type_ctx.env_ref ? *type_ctx.env_ref : env;
  };
  auto type_expr = [&](const syntax::ExprPtr& expr) {
    return TypeExpr(ctx, type_ctx, expr, active_env());
  };
  auto type_ident = [&](std::string_view name) -> ExprTypeResult {
    return TypeIdentifierExpr(ctx, syntax::IdentifierExpr{std::string(name)},
                              active_env());
  };
  auto type_place = [&](const syntax::ExprPtr& expr) -> PlaceTypeResult {
    return TypePlace(ctx, type_ctx, expr, active_env());
  };

  return TypeBlock(ctx, type_ctx, block, env, type_expr, type_ident, type_place,
                   &live_env);
}

static CheckResult CheckExprAgainstType(const ScopeContext& ctx,
                                        const syntax::ExprPtr& expr,
                                        const TypeRef& expected,
                                        const TypeEnv& env,
                                        core::DiagnosticStream& diags) {
  StmtTypeContext type_ctx;
  type_ctx.return_type = {};
  type_ctx.loop_flag = LoopFlag::None;
  type_ctx.in_unsafe = false;
  type_ctx.diags = &diags;
  TypeEnv live_env = env;
  type_ctx.env_ref = &live_env;
  auto type_expr = [&](const syntax::ExprPtr& subexpr) {
    return TypeExpr(ctx, type_ctx, subexpr, live_env);
  };
  auto type_ident = [&](std::string_view name) -> ExprTypeResult {
    return TypeIdentifierExpr(ctx, syntax::IdentifierExpr{std::string(name)},
                              live_env);
  };
  auto type_place = [&](const syntax::ExprPtr& subexpr) {
    return TypePlace(ctx, type_ctx, subexpr, live_env);
  };
  auto match_check = [&](const syntax::MatchExpr& match_expr,
                         const TypeRef& expected_type) {
    return CheckMatchExpr(ctx, type_ctx, match_expr, live_env, expected_type);
  };
  return CheckExpr(ctx, expr, expected, type_expr, type_place, type_ident, match_check);
}

static bool ReturnAnnOk(const std::shared_ptr<syntax::Type>& ret_opt) {
  return ret_opt != nullptr;
}

static bool BuiltInContextType(const syntax::Type& type) {
  const auto* path = std::get_if<syntax::TypePathType>(&type.node);
  return path && path->path.size() == 1 && IdEq(path->path[0], "Context");
}

static bool MainGeneric(const syntax::ProcedureDecl& /*decl*/) {
  return false;
}

static bool MainSigOk(const syntax::ProcedureDecl& decl) {
  if (decl.vis != syntax::Visibility::Public) {
    return false;
  }
  if (decl.params.size() != 1) {
    return false;
  }
  const auto& param = decl.params[0];
  if (param.mode.has_value() && *param.mode != syntax::ParamMode::Move) {
    return false;
  }
  if (!param.type || !BuiltInContextType(*param.type)) {
    return false;
  }
  const auto* ret = decl.return_type_opt
                        ? std::get_if<syntax::TypePrim>(&decl.return_type_opt->node)
                        : nullptr;
  return ret && ret->name == "i32";
}

static bool SubtypeReturn(const ScopeContext& ctx,
                          const TypeRef& body_type,
                          const TypeRef& return_type,
                          core::DiagnosticStream& diags,
                          const std::optional<core::Span>& span) {
  const auto sub = Subtyping(ctx, body_type, return_type);
  if (!sub.ok) {
    if (sub.diag_id.has_value()) {
      EmitTypecheckDiag(diags, *sub.diag_id, span);
    }
    return false;
  }
  if (!sub.subtype) {
    SPEC_RULE("Return-Type-Err");
    EmitTypecheckDiag(diags, "Return-Type-Err", span);
    return false;
  }
  return true;
}

// Check if a type is a primitive type with the given name.
static bool IsPrimType(const TypeRef& type, std::string_view name) {
  if (!type) {
    return false;
  }
  const auto* prim = std::get_if<TypePrim>(&type->node);
  return prim && prim->name == name;
}

// Check if a block body has an explicit return statement at the end.
// Required for procedures/methods with non-unit return types.
static bool HasExplicitReturn(const syntax::Block& block) {
  // If there's a tail expression, it's not an explicit return
  if (block.tail_opt) {
    return false;
  }
  // Check if last statement is a return
  if (!block.stmts.empty() &&
      std::holds_alternative<syntax::ReturnStmt>(block.stmts.back())) {
    return true;
  }
  return false;
}

static bool TypeAliasCycleFrom(const ScopeContext& ctx,
                               const PathKey& start,
                               std::set<PathKey>& active,
                               std::set<PathKey>& done);

static void CollectAliasDeps(const ScopeContext& ctx,
                             const std::shared_ptr<syntax::Type>& type,
                             std::vector<PathKey>& deps) {
  if (!type) {
    return;
  }
  std::visit(
      [&](const auto& node) {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::TypePathType>) {
          const auto key = PathKeyOf(node.path);
          const auto it = ctx.sigma.types.find(key);
          if (it != ctx.sigma.types.end() &&
              std::holds_alternative<syntax::TypeAliasDecl>(it->second)) {
            deps.push_back(key);
          }
        } else if constexpr (std::is_same_v<T, syntax::TypePermType>) {
          CollectAliasDeps(ctx, node.base, deps);
        } else if constexpr (std::is_same_v<T, syntax::TypeUnion>) {
          for (const auto& elem : node.types) {
            CollectAliasDeps(ctx, elem, deps);
          }
        } else if constexpr (std::is_same_v<T, syntax::TypeFunc>) {
          for (const auto& param : node.params) {
            CollectAliasDeps(ctx, param.type, deps);
          }
          CollectAliasDeps(ctx, node.ret, deps);
        } else if constexpr (std::is_same_v<T, syntax::TypeTuple>) {
          for (const auto& elem : node.elements) {
            CollectAliasDeps(ctx, elem, deps);
          }
        } else if constexpr (std::is_same_v<T, syntax::TypeArray>) {
          CollectAliasDeps(ctx, node.element, deps);
        } else if constexpr (std::is_same_v<T, syntax::TypeSlice>) {
          CollectAliasDeps(ctx, node.element, deps);
        } else if constexpr (std::is_same_v<T, syntax::TypePtr>) {
          CollectAliasDeps(ctx, node.element, deps);
        } else if constexpr (std::is_same_v<T, syntax::TypeRawPtr>) {
          CollectAliasDeps(ctx, node.element, deps);
        } else if constexpr (std::is_same_v<T, syntax::TypeDynamic>) {
          return;
        } else if constexpr (std::is_same_v<T, syntax::TypeModalState>) {
          return;
        } else if constexpr (std::is_same_v<T, syntax::TypePrim> ||
                             std::is_same_v<T, syntax::TypeString> ||
                             std::is_same_v<T, syntax::TypeBytes>) {
          return;
        }
      },
      type->node);
}

static bool TypeAliasCycleFrom(const ScopeContext& ctx,
                               const PathKey& start,
                               std::set<PathKey>& active,
                               std::set<PathKey>& done) {
  if (done.find(start) != done.end()) {
    return false;
  }
  if (!active.insert(start).second) {
    return true;
  }

  const auto it = ctx.sigma.types.find(start);
  if (it == ctx.sigma.types.end()) {
    active.erase(start);
    done.insert(start);
    return false;
  }
  const auto* alias = std::get_if<syntax::TypeAliasDecl>(&it->second);
  if (!alias) {
    active.erase(start);
    done.insert(start);
    return false;
  }

  std::vector<PathKey> deps;
  CollectAliasDeps(ctx, alias->type, deps);
  for (const auto& dep : deps) {
    if (TypeAliasCycleFrom(ctx, dep, active, done)) {
      return true;
    }
  }

  active.erase(start);
  done.insert(start);
  return false;
}

static bool TypeAliasOk(const ScopeContext& ctx, const syntax::Path& path) {
  std::set<PathKey> active;
  std::set<PathKey> done;
  return !TypeAliasCycleFrom(ctx, PathKeyOf(path), active, done);
}

static bool DistinctPaths(const std::vector<syntax::ClassPath>& paths) {
  if (paths.size() < 2) {
    return true;
  }
  std::vector<PathKey> keys;
  keys.reserve(paths.size());
  for (const auto& path : paths) {
    keys.push_back(PathKeyOf(path));
  }
  std::sort(keys.begin(), keys.end());
  return std::adjacent_find(keys.begin(), keys.end()) == keys.end();
}

static bool ClassPathOk(const ScopeContext& ctx,
                        const syntax::ClassPath& path,
                        core::DiagnosticStream& diags,
                        const core::Span& span) {
  if (ctx.sigma.classes.find(PathKeyOf(path)) == ctx.sigma.classes.end()) {
    SPEC_RULE("WF-ClassPath-Err");
    EmitTypecheckDiag(diags, "Superclass-Undefined", span);
    return false;
  }
  SPEC_RULE("WF-ClassPath");
  return true;
}

struct MethodSig {
  bool ok = false;
  TypeRef recv_type;
  std::vector<TypeFuncParam> params;
  TypeRef ret;
};

static std::optional<MethodSig> BuildMethodSig(const ScopeContext& ctx,
                                               const syntax::Receiver& receiver,
                                               const std::vector<syntax::Param>& params,
                                               const std::shared_ptr<syntax::Type>& return_type_opt,
                                               const TypeRef& self_type,
                                               std::optional<std::string_view>& diag_id) {
  MethodSig sig;
  auto lower_type = [&](const std::shared_ptr<syntax::Type>& type) -> LowerTypeResult {
    const auto lowered = LowerType(ctx, type);
    if (!lowered.ok) {
      return {false, lowered.diag_id, {}};
    }
    return {true, std::nullopt, lowered.type};
  };
  const auto recv = RecvTypeForReceiver(ctx, self_type, receiver, lower_type);
  if (!recv.ok) {
    diag_id = recv.diag_id;
    return std::nullopt;
  }
  sig.recv_type = recv.type;

  for (const auto& param : params) {
    const auto lowered = LowerType(ctx, param.type);
    if (!lowered.ok) {
      diag_id = lowered.diag_id;
      return std::nullopt;
    }
    sig.params.push_back(TypeFuncParam{LowerParamMode(param.mode),
                                       SubstSelfType(self_type, lowered.type)});
  }

  const auto ret = LowerReturnType(ctx, return_type_opt);
  if (!ret.ok) {
    diag_id = ret.diag_id;
    return std::nullopt;
  }
  sig.ret = SubstSelfType(self_type, ret.type);
  sig.ok = true;
  return sig;
}

static bool MethodSigEqual(const MethodSig& lhs, const MethodSig& rhs) {
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

static const syntax::MethodDecl* FindRecordMethod(const syntax::RecordDecl& record,
                                                  std::string_view name) {
  for (const auto& member : record.members) {
    const auto* method = std::get_if<syntax::MethodDecl>(&member);
    if (!method) {
      continue;
    }
    if (IdEq(method->name, name)) {
      return method;
    }
  }
  return nullptr;
}

static const syntax::FieldDecl* FindRecordField(const syntax::RecordDecl& record,
                                                std::string_view name) {
  for (const auto& member : record.members) {
    const auto* field = std::get_if<syntax::FieldDecl>(&member);
    if (!field) {
      continue;
    }
    if (IdEq(field->name, name)) {
      return field;
    }
  }
  return nullptr;
}

static std::vector<const syntax::MethodDecl*> RecordMethods(
    const syntax::RecordDecl& record) {
  std::vector<const syntax::MethodDecl*> methods;
  for (const auto& member : record.members) {
    if (const auto* method = std::get_if<syntax::MethodDecl>(&member)) {
      methods.push_back(method);
    }
  }
  return methods;
}

static bool ImplementsOk(const ScopeContext& ctx,
                         const syntax::ModulePath& module_path,
                         const syntax::Path& type_path,
                         const std::vector<syntax::ClassPath>& impls,
                         const syntax::RecordDecl* record,
                         core::DiagnosticStream& diags,
                         const core::Span& span) {
  SpecDefsDeclTyping();

  if (!DistinctPaths(impls)) {
    SPEC_RULE("Impl-Duplicate-Class-Err");
    SPEC_RULE("Impl-Dup");
    EmitTypecheckDiag(diags, "Impl-Duplicate-Class-Err", span);
    return false;
  }

  auto has_impl = [&](std::string_view name) {
    for (const auto& impl : impls) {
      if (impl.size() == 1 && IdEq(impl[0], name)) {
        return true;
      }
    }
    return false;
  };

  const bool has_bitcopy = has_impl("Bitcopy");
  const bool has_drop = has_impl("Drop");
  const bool has_clone = has_impl("Clone");

  if (has_bitcopy && has_drop) {
    SPEC_RULE("BitcopyDrop-Conflict");
    EmitTypecheckDiag(diags, "BitcopyDrop-Conflict", span);
    return false;
  }

  if (has_bitcopy && !has_clone) {
    SPEC_RULE("Bitcopy-Clone-Missing");
    EmitTypecheckDiag(diags, "Bitcopy-Clone-Missing", span);
    return false;
  }

  if (has_bitcopy && record) {
    for (const auto& member : record->members) {
      const auto* field = std::get_if<syntax::FieldDecl>(&member);
      if (!field) {
        continue;
      }
      const auto lowered = LowerType(ctx, field->type);
      if (!lowered.ok) {
        if (lowered.diag_id.has_value()) {
          EmitTypecheckDiag(diags, *lowered.diag_id, field->span);
        }
        return false;
      }
      if (!BitcopyType(ctx, lowered.type)) {
        SPEC_RULE("Bitcopy-Field-NonBitcopy");
        EmitTypecheckDiag(diags, "Bitcopy-Field-NonBitcopy", field->span);
        return false;
      }
    }
  }

  SPEC_RULE("BitcopyDrop-Ok");

  std::vector<const syntax::MethodDecl*> record_methods;
  if (record) {
    record_methods = RecordMethods(*record);
  }

  std::unordered_set<IdKey> concrete_defaults;
  for (const auto& cls_path : impls) {
    const auto cls_it = ctx.sigma.classes.find(PathKeyOf(cls_path));
    if (cls_it == ctx.sigma.classes.end()) {
      SPEC_RULE("Superclass-Undefined");
      EmitTypecheckDiag(diags, "Superclass-Undefined", span);
      return false;
    }

    const auto method_table = ClassMethodTable(ctx, cls_path);
    if (!method_table.ok) {
      if (method_table.diag_id.has_value()) {
        EmitTypecheckDiag(diags, *method_table.diag_id, span);
      }
      return false;
    }
    const auto field_table = ClassFieldTable(ctx, cls_path);
    if (!field_table.ok) {
      if (field_table.diag_id.has_value()) {
        EmitTypecheckDiag(diags, *field_table.diag_id, span);
      }
      return false;
    }

    for (const auto& entry : method_table.methods) {
      const auto* method = entry.method;
      if (!method) {
        continue;
      }
      const auto name = IdKeyOf(method->name);
      if (method->body_opt) {
        concrete_defaults.insert(name);
      }

      if (!record) {
        if (!method->body_opt) {
          SPEC_RULE("Impl-Missing-Method");
          EmitTypecheckDiag(diags, "Impl-Missing-Method", span);
          return false;
        }
        if (method->body_opt) {
          SPEC_RULE("Impl-Concrete-Default");
        }
        continue;
      }

      const auto* impl_method = FindRecordMethod(*record, method->name);
      if (!method->body_opt) {
        if (!impl_method) {
          SPEC_RULE("Impl-Missing-Method");
          EmitTypecheckDiag(diags, "Impl-Missing-Method", span);
          return false;
        }
        std::optional<std::string_view> diag_id;
        const auto self_type = MakeTypePath(type_path);
        const auto class_sig = BuildMethodSig(ctx, method->receiver, method->params,
                                              method->return_type_opt, self_type, diag_id);
        const auto impl_sig = BuildMethodSig(ctx, impl_method->receiver,
                                             impl_method->params,
                                             impl_method->return_type_opt,
                                             self_type, diag_id);
        if (!class_sig.has_value() || !impl_sig.has_value() ||
            !MethodSigEqual(*class_sig, *impl_sig)) {
          SPEC_RULE("Impl-Sig-Err");
          EmitTypecheckDiag(diags, "Impl-Sig-Err", impl_method->span);
          return false;
        }
        if (impl_method->override_flag) {
          SPEC_RULE("Override-Abstract-Err");
          EmitTypecheckDiag(diags, "Override-Abstract-Err", impl_method->span);
          return false;
        }
        SPEC_RULE("Impl-Abstract-Method");
      } else {
        if (!impl_method) {
          SPEC_RULE("Impl-Concrete-Default");
          continue;
        }
        std::optional<std::string_view> diag_id;
        const auto self_type = MakeTypePath(type_path);
        const auto class_sig = BuildMethodSig(ctx, method->receiver, method->params,
                                              method->return_type_opt, self_type, diag_id);
        const auto impl_sig = BuildMethodSig(ctx, impl_method->receiver,
                                             impl_method->params,
                                             impl_method->return_type_opt,
                                             self_type, diag_id);
        if (!class_sig.has_value() || !impl_sig.has_value() ||
            !MethodSigEqual(*class_sig, *impl_sig)) {
          SPEC_RULE("Impl-Sig-Err-Concrete");
          EmitTypecheckDiag(diags, "Impl-Sig-Err-Concrete", impl_method->span);
          return false;
        }
        if (!impl_method->override_flag) {
          SPEC_RULE("Override-Missing-Err");
          EmitTypecheckDiag(diags, "Override-Missing-Err", impl_method->span);
          return false;
        }
        SPEC_RULE("Impl-Concrete-Override");
      }
    }

    const auto self_type = MakeTypePath(type_path);
    for (const auto* field : field_table.fields) {
      if (!record) {
        SPEC_RULE("Impl-Field-Missing");
        EmitTypecheckDiag(diags, "Impl-Field-Missing", span);
        return false;
      }
      const auto* impl_field = FindRecordField(*record, field->name);
      if (!impl_field) {
        SPEC_RULE("Impl-Field-Missing");
        EmitTypecheckDiag(diags, "Impl-Field-Missing", span);
        return false;
      }
      const auto class_field_type = LowerType(ctx, field->type);
      if (!class_field_type.ok) {
        if (class_field_type.diag_id.has_value()) {
          EmitTypecheckDiag(diags, *class_field_type.diag_id, field->span);
        }
        return false;
      }
      const auto impl_field_type = LowerType(ctx, impl_field->type);
      if (!impl_field_type.ok) {
        if (impl_field_type.diag_id.has_value()) {
          EmitTypecheckDiag(diags, *impl_field_type.diag_id, impl_field->span);
        }
        return false;
      }
      const auto field_type = SubstSelfType(self_type, class_field_type.type);
      const auto sub = Subtyping(ctx, impl_field_type.type, field_type);
      if (!sub.ok) {
        if (sub.diag_id.has_value()) {
          EmitTypecheckDiag(diags, *sub.diag_id, impl_field->span);
        }
        return false;
      }
      if (!sub.subtype) {
        SPEC_RULE("Impl-Field-Type-Err");
        EmitTypecheckDiag(diags, "Impl-Field-Type-Err", impl_field->span);
        return false;
      }
      SPEC_RULE("Impl-Field");
    }
  }

  if (record) {
    for (const auto* method : record_methods) {
      if (!method->override_flag) {
        continue;
      }
      if (concrete_defaults.find(IdKeyOf(method->name)) == concrete_defaults.end()) {
        SPEC_RULE("Override-NoConcrete");
        EmitTypecheckDiag(diags, "Override-NoConcrete", method->span);
        return false;
      }
    }
  }

  SPEC_RULE("WF-Impl");
  return true;
}

static bool BindCheckStub(const ScopeContext& ctx,
                          const syntax::ModulePath& module_path,
                          const std::vector<syntax::Param>& params,
                          const std::shared_ptr<syntax::Block>& body,
                          const std::optional<BindSelfParam>& self_param,
                          core::DiagnosticStream& diags) {
  if (!body) {
    return true;
  }
  const auto result = BindCheckBody(ctx, module_path, params, body, self_param);
  if (!result.ok) {
    if (result.diag_id.has_value()) {
      EmitTypecheckDiag(diags, *result.diag_id, result.span);
    }
    return false;
  }
  return true;
}

static bool ProvBindCheckBody(const ScopeContext& ctx,
                              const syntax::ModulePath& module_path,
                              const std::vector<syntax::Param>& params,
                              const std::shared_ptr<syntax::Block>& body,
                              const std::optional<BindSelfParam>& self_param,
                              core::DiagnosticStream& diags) {
  if (!body) {
    return true;
  }
  const auto result = ProvBindCheck(ctx, module_path, params, body, self_param);
  if (!result.ok) {
    if (result.diag_id.has_value()) {
      EmitTypecheckDiag(diags, *result.diag_id, result.span);
    }
    return false;
  }
  return true;
}

static bool CheckParamTypes(const ScopeContext& ctx,
                            const std::vector<syntax::Param>& params,
                            const TypeRef& self,
                            std::vector<std::pair<std::string, TypeRef>>& binds,
                            core::DiagnosticStream& diags,
                            const std::optional<core::Span>& span) {
  for (const auto& param : params) {
    const auto lowered = LowerType(ctx, param.type);
    if (!lowered.ok) {
      if (lowered.diag_id.has_value()) {
        EmitTypecheckDiag(diags, *lowered.diag_id, param.span);
      }
      return false;
    }
    const auto subst = self ? SubstSelfType(self, lowered.type) : lowered.type;
    binds.emplace_back(param.name, subst);
  }
  (void)span;
  return true;
}

static bool CheckProcedureDecl(const ScopeContext& ctx,
                               const syntax::ProcedureDecl& decl,
                               core::DiagnosticStream& diags) {
  SpecDefsDeclTyping();
  if (!ReturnAnnOk(decl.return_type_opt)) {
    SPEC_RULE("WF-ProcedureDecl-MissingReturnType");
    EmitTypecheckDiag(diags, "WF-ProcedureDecl-MissingReturnType", decl.span);
    return false;
  }

  const auto names = ParamNames(decl.params);
  if (!DistinctNames(names)) {
    EmitTypecheckDiag(diags, "Collect-Dup", decl.span);
    return false;
  }

  std::vector<std::pair<std::string, TypeRef>> binds;
  if (!CheckParamTypes(ctx, decl.params, {}, binds, diags, decl.span)) {
    return false;
  }

  const auto ret = LowerType(ctx, decl.return_type_opt);
  if (!ret.ok) {
    if (ret.diag_id.has_value()) {
      EmitTypecheckDiag(diags, *ret.diag_id,
                        decl.return_type_opt->span);
    }
    return false;
  }

  TypeEnv env;
  env.scopes.emplace_back();
  for (const auto& [name, type] : binds) {
    (void)AddBinding(env, name, type);
  }

  if (decl.body) {
    const auto typed = TypeBlockForDecl(ctx, *decl.body, env, ret.type, diags);
    if (!typed.ok) {
      if (typed.diag_id.has_value()) {
        EmitTypecheckDiag(diags, *typed.diag_id, decl.body->span);
      }
      return false;
    }
    // Check explicit return requirement for non-unit return types
    if (!IsPrimType(ret.type, "()") && !HasExplicitReturn(*decl.body)) {
      SPEC_RULE("WF-ProcBody-ExplicitReturn-Err");
      EmitTypecheckDiag(diags, "WF-ProcBody-ExplicitReturn-Err", decl.body->span);
      return false;
    }
    if (!SubtypeReturn(ctx, typed.type, ret.type, diags, decl.body->span)) {
      return false;
    }
  }

  if (!BindCheckStub(ctx, ctx.current_module, decl.params, decl.body,
                     std::nullopt, diags)) {
    return false;
  }
  if (!ProvBindCheckBody(ctx, ctx.current_module, decl.params, decl.body, std::nullopt, diags)) {
    return false;
  }

  SPEC_RULE("WF-ProcedureDecl");
  return true;
}

static bool CheckStaticDecl(const ScopeContext& ctx,
                            const syntax::StaticDecl& decl,
                            core::DiagnosticStream& diags) {
  SpecDefsDeclTyping();
  if (!StaticVisOk(decl.vis, decl.mut)) {
    SPEC_RULE("StaticVisOk-Err");
    EmitTypecheckDiag(diags, "StaticVisOk-Err", decl.span);
    return false;
  }

  if (!decl.binding.type_opt) {
    SPEC_RULE("WF-StaticDecl-MissingType");
    EmitTypecheckDiag(diags, "WF-StaticDecl-MissingType", decl.span);
    return false;
  }

  const auto ann = LowerType(ctx, decl.binding.type_opt);
  if (!ann.ok) {
    if (ann.diag_id.has_value()) {
      EmitTypecheckDiag(diags, *ann.diag_id, decl.binding.type_opt->span);
    }
    return false;
  }

  TypeEnv env;
  env.scopes.emplace_back();
  const auto check = CheckExprAgainstType(ctx, decl.binding.init, ann.type,
                                          env, diags);
  if (!check.ok) {
    if (check.diag_id.has_value()) {
      EmitTypecheckDiag(diags, *check.diag_id, decl.binding.init->span);
      return false;
    }
    SPEC_RULE("WF-StaticDecl-Ann-Mismatch");
    EmitTypecheckDiag(diags, "WF-StaticDecl-Ann-Mismatch", decl.span);
    return false;
  }

  const auto pat = TypeMatchPattern(ctx, decl.binding.pat, ann.type);
  if (!pat.ok) {
    if (pat.diag_id.has_value()) {
      EmitTypecheckDiag(diags, *pat.diag_id, decl.binding.span);
      return false;
    }
    return false;
  }

  SPEC_RULE("WF-StaticDecl");
  return true;
}

static bool CheckRecordMethod(const ScopeContext& ctx,
                              const syntax::ModulePath& module_path,
                              const syntax::RecordDecl& record,
                              const syntax::MethodDecl& method,
                              core::DiagnosticStream& diags) {
  auto lower_type = [&](const std::shared_ptr<syntax::Type>& type) -> LowerTypeResult {
    const auto lowered = LowerType(ctx, type);
    if (!lowered.ok) {
      return {false, lowered.diag_id, {}};
    }
    return {true, std::nullopt, lowered.type};
  };
  const auto base = MakeTypePath(TypePathForItem(module_path, record.name));
  TypeRef recv_type;
  if (const auto* explicit_recv = std::get_if<syntax::ReceiverExplicit>(&method.receiver)) {
    const auto lowered = LowerType(ctx, explicit_recv->type);
    if (!lowered.ok) {
      if (lowered.diag_id.has_value()) {
        EmitTypecheckDiag(diags, *lowered.diag_id, method.span);
      }
      return false;
    }
    bool is_self = false;
    if (const auto* path = std::get_if<TypePathType>(&lowered.type->node)) {
      is_self = path->path.size() == 1 && IdEq(path->path[0], "Self");
      if (!is_self) {
        is_self = path->path == TypePathForItem(module_path, record.name);
      }
    } else if (const auto* perm = std::get_if<TypePerm>(&lowered.type->node)) {
      if (const auto* path = std::get_if<TypePathType>(&perm->base->node)) {
        is_self = path->path.size() == 1 && IdEq(path->path[0], "Self");
        if (!is_self) {
          is_self = path->path == TypePathForItem(module_path, record.name);
        }
      }
    }
    if (!is_self) {
      SPEC_RULE("Record-Method-RecvSelf-Err");
      EmitTypecheckDiag(diags, "Record-Method-RecvSelf-Err", method.span);
      return false;
    }
    SPEC_RULE("Recv-Explicit");
    recv_type = SubstSelfType(base, lowered.type);
  } else {
    const auto* shorthand = std::get_if<syntax::ReceiverShorthand>(&method.receiver);
    if (shorthand) {
      if (shorthand->perm == syntax::ReceiverPerm::Const) {
        SPEC_RULE("Recv-Const");
      } else if (shorthand->perm == syntax::ReceiverPerm::Unique) {
        SPEC_RULE("Recv-Unique");
      }
    }
    const auto recv = RecvTypeForReceiver(ctx, base, method.receiver, lower_type);
    if (!recv.ok) {
      if (recv.diag_id.has_value()) {
        EmitTypecheckDiag(diags, *recv.diag_id, method.span);
      }
      return false;
    }
    recv_type = recv.type;
  }

  const auto names = ParamNames(method.params);
  if (!DistinctNames(names) || std::find(names.begin(), names.end(), IdKeyOf("self")) != names.end()) {
    EmitTypecheckDiag(diags, "Collect-Dup", method.span);
    return false;
  }

  std::vector<std::pair<std::string, TypeRef>> binds;
  binds.emplace_back("self", recv_type);
  if (!CheckParamTypes(ctx, method.params, base, binds, diags, method.span)) {
    return false;
  }

  const auto ret = LowerReturnType(ctx, method.return_type_opt);
  if (!ret.ok) {
    if (ret.diag_id.has_value()) {
      EmitTypecheckDiag(diags, *ret.diag_id, method.span);
    }
    return false;
  }
  const auto ret_type = SubstSelfType(base, ret.type);

  SPEC_RULE("WF-Record-Method");

  TypeEnv env;
  env.scopes.emplace_back();
  for (const auto& [name, type] : binds) {
    (void)AddBinding(env, name, type);
  }

  if (method.body) {
    const auto typed = TypeBlockForDecl(ctx, *method.body, env, ret_type, diags);
    if (!typed.ok) {
      if (typed.diag_id.has_value()) {
        EmitTypecheckDiag(diags, *typed.diag_id, method.body->span);
      }
      return false;
    }
    // Check explicit return requirement for non-unit return types
    if (!IsPrimType(ret_type, "()") && !HasExplicitReturn(*method.body)) {
      SPEC_RULE("WF-ProcBody-ExplicitReturn-Err");
      EmitTypecheckDiag(diags, "WF-ProcBody-ExplicitReturn-Err", method.body->span);
      return false;
    }
    if (!SubtypeReturn(ctx, typed.type, ret_type, diags, method.body->span)) {
      return false;
    }
    SPEC_RULE("T-Record-Method-Body");
  }

  std::optional<ParamMode> recv_mode;
  if (const auto* explicit_recv = std::get_if<syntax::ReceiverExplicit>(&method.receiver)) {
    recv_mode = LowerParamMode(explicit_recv->mode_opt);
  }
  const BindSelfParam self_param{recv_type, recv_mode};
  if (!BindCheckStub(ctx, module_path, method.params, method.body, self_param, diags)) {
    return false;
  }
  if (!ProvBindCheckBody(ctx, module_path, method.params, method.body, self_param, diags)) {
    return false;
  }

  return true;
}

static bool CheckRecordDecl(const ScopeContext& ctx,
                            const syntax::ModulePath& module_path,
                            const syntax::RecordDecl& record,
                            core::DiagnosticStream& diags) {
  SpecDefsDeclTyping();
  for (const auto& member : record.members) {
    const auto* field = std::get_if<syntax::FieldDecl>(&member);
    if (!field) {
      continue;
    }
    const auto lowered = LowerType(ctx, field->type);
    if (!lowered.ok) {
      if (lowered.diag_id.has_value()) {
        EmitTypecheckDiag(diags, *lowered.diag_id, field->span);
      }
      return false;
    }
  }

  if (!FieldVisOk(record)) {
    SPEC_RULE("FieldVisOk-Err");
    EmitTypecheckDiag(diags, "FieldVisOk-Err", record.span);
    return false;
  }

  auto type_ctx = StmtTypeContext{};
  type_ctx.diags = &diags;
  auto type_expr = [&](const syntax::ExprPtr& expr) {
    return TypeExpr(ctx, type_ctx, expr, TypeEnv{});
  };
  const auto wf = CheckRecordWf(ctx, record, type_expr);
  if (!wf.ok) {
    if (wf.diag_id.has_value()) {
      EmitTypecheckDiag(diags, *wf.diag_id, record.span);
    }
    return false;
  }

  std::unordered_set<IdKey> method_names;
  for (const auto& member : record.members) {
    const auto* method = std::get_if<syntax::MethodDecl>(&member);
    if (!method) {
      continue;
    }
    const auto key = IdKeyOf(method->name);
    if (!method_names.insert(key).second) {
      SPEC_RULE("Record-Method-Dup");
      EmitTypecheckDiag(diags, "Record-Method-Dup", method->span);
      return false;
    }
  }

  for (const auto& member : record.members) {
    const auto* method = std::get_if<syntax::MethodDecl>(&member);
    if (!method) {
      continue;
    }
    if (!CheckRecordMethod(ctx, module_path, record, *method, diags)) {
      return false;
    }
  }

  SPEC_RULE("WF-Record-Methods");

  const auto record_path = TypePathForItem(module_path, record.name);
  if (!ImplementsOk(ctx, module_path, record_path, record.implements, &record,
                    diags, record.span)) {
    return false;
  }

  SPEC_RULE("WF-RecordDecl");
  return true;
}

static bool CheckEnumDecl(const ScopeContext& ctx,
                          const syntax::ModulePath& module_path,
                          const syntax::EnumDecl& decl,
                          core::DiagnosticStream& diags) {
  SpecDefsDeclTyping();
  if (decl.variants.empty()) {
    EmitTypecheckDiag(diags, "Collect-Dup", decl.span);
    return false;
  }

  std::unordered_set<IdKey> variant_names;
  for (const auto& variant : decl.variants) {
    const auto key = IdKeyOf(variant.name);
    if (!variant_names.insert(key).second) {
      EmitTypecheckDiag(diags, "Collect-Dup", variant.span);
      return false;
    }
    if (!variant.payload_opt.has_value()) {
      continue;
    }
    if (const auto* tuple =
            std::get_if<syntax::VariantPayloadTuple>(&*variant.payload_opt)) {
      for (const auto& elem : tuple->elements) {
        const auto lowered = LowerType(ctx, elem);
        if (!lowered.ok) {
          if (lowered.diag_id.has_value()) {
            EmitTypecheckDiag(diags, *lowered.diag_id, variant.span);
          }
          return false;
        }
      }
    } else if (const auto* rec =
                   std::get_if<syntax::VariantPayloadRecord>(&*variant.payload_opt)) {
      std::unordered_set<IdKey> field_names;
      for (const auto& field : rec->fields) {
        const auto field_key = IdKeyOf(field.name);
        if (!field_names.insert(field_key).second) {
          EmitTypecheckDiag(diags, "Collect-Dup", field.span);
          return false;
        }
        const auto lowered = LowerType(ctx, field.type);
        if (!lowered.ok) {
          if (lowered.diag_id.has_value()) {
            EmitTypecheckDiag(diags, *lowered.diag_id, field.span);
          }
          return false;
        }
      }
    }
  }

  const auto discs = EnumDiscriminants(decl);
  if (!discs.ok) {
    if (discs.diag_id.has_value()) {
      EmitTypecheckDiag(diags, *discs.diag_id, discs.span.value_or(decl.span));
    }
    return false;
  }

  const auto enum_path = TypePathForItem(module_path, decl.name);
  if (!ImplementsOk(ctx, module_path, enum_path, decl.implements, nullptr, diags,
                    decl.span)) {
    return false;
  }

  SPEC_RULE("WF-EnumDecl");
  return true;
}

static bool CheckStatePayload(const ScopeContext& ctx,
                              const syntax::StateBlock& state,
                              core::DiagnosticStream& diags) {
  std::unordered_set<IdKey> names;
  for (const auto& member : state.members) {
    const auto* field = std::get_if<syntax::StateFieldDecl>(&member);
    if (!field) {
      continue;
    }
    const auto key = IdKeyOf(field->name);
    if (!names.insert(key).second) {
      SPEC_RULE("Modal-Payload-DupField");
      EmitTypecheckDiag(diags, "Modal-Payload-DupField", field->span);
      return false;
    }
    const auto lowered = LowerType(ctx, field->type);
    if (!lowered.ok) {
      if (lowered.diag_id.has_value()) {
        EmitTypecheckDiag(diags, *lowered.diag_id, field->span);
      }
      return false;
    }
  }
  SPEC_RULE("WF-Modal-Payload");
  return true;
}

static bool CheckStateMethod(const ScopeContext& ctx,
                             const TypePath& modal_path,
                             const syntax::StateBlock& state,
                             const syntax::StateMethodDecl& method,
                             core::DiagnosticStream& diags) {
  const auto names = ParamNames(method.params);
  if (!DistinctNames(names) || std::find(names.begin(), names.end(), IdKeyOf("self")) != names.end()) {
    EmitTypecheckDiag(diags, "Collect-Dup", method.span);
    return false;
  }

  const auto self_type = MakeTypePerm(
      Permission::Const, MakeTypeModalState(modal_path, state.name));
  std::vector<std::pair<std::string, TypeRef>> binds;
  binds.emplace_back("self", self_type);
  if (!CheckParamTypes(ctx, method.params, {}, binds, diags, method.span)) {
    return false;
  }

  const auto ret = LowerReturnType(ctx, method.return_type_opt);
  if (!ret.ok) {
    if (ret.diag_id.has_value()) {
      EmitTypecheckDiag(diags, *ret.diag_id, method.span);
    }
    return false;
  }

  SPEC_RULE("WF-State-Method");

  TypeEnv env;
  env.scopes.emplace_back();
  for (const auto& [name, type] : binds) {
    (void)AddBinding(env, name, type);
  }

  if (method.body) {
    const auto typed = TypeBlockForDecl(ctx, *method.body, env, ret.type, diags);
    if (!typed.ok) {
      if (typed.diag_id.has_value()) {
        EmitTypecheckDiag(diags, *typed.diag_id, method.body->span);
      }
      return false;
    }
    // Check explicit return requirement for non-unit return types
    if (!IsPrimType(ret.type, "()") && !HasExplicitReturn(*method.body)) {
      SPEC_RULE("WF-ProcBody-ExplicitReturn-Err");
      EmitTypecheckDiag(diags, "WF-ProcBody-ExplicitReturn-Err", method.body->span);
      return false;
    }
    if (!SubtypeReturn(ctx, typed.type, ret.type, diags, method.body->span)) {
      return false;
    }
    SPEC_RULE("T-Modal-Method-Body");
  }

  const BindSelfParam self_param{self_type, std::nullopt};
  if (!BindCheckStub(ctx, ctx.current_module, method.params, method.body,
                     self_param, diags)) {
    return false;
  }
  if (!ProvBindCheckBody(ctx, ctx.current_module, method.params, method.body, self_param, diags)) {
    return false;
  }

  return true;
}

static bool CheckTransition(const ScopeContext& ctx,
                            const syntax::ModalDecl& modal,
                            const TypePath& modal_path,
                            const syntax::StateBlock& state,
                            const syntax::TransitionDecl& transition,
                            core::DiagnosticStream& diags) {
  const auto names = ParamNames(transition.params);
  if (!DistinctNames(names) || std::find(names.begin(), names.end(), IdKeyOf("self")) != names.end()) {
    EmitTypecheckDiag(diags, "Collect-Dup", transition.span);
    return false;
  }

  const auto self_type = MakeTypePerm(
      Permission::Unique, MakeTypeModalState(modal_path, state.name));
  std::vector<std::pair<std::string, TypeRef>> binds;
  binds.emplace_back("self", self_type);
  if (!CheckParamTypes(ctx, transition.params, {}, binds, diags, transition.span)) {
    return false;
  }

  bool target_ok = false;
  for (const auto& st : modal.states) {
    if (IdEq(st.name, transition.target_state)) {
      target_ok = true;
      break;
    }
  }
  if (!target_ok) {
    SPEC_RULE("Transition-Target-Err");
    EmitTypecheckDiag(diags, "Transition-Target-Err", transition.span);
    return false;
  }

  SPEC_RULE("WF-Transition");

  TypeEnv env;
  env.scopes.emplace_back();
  for (const auto& [name, type] : binds) {
    (void)AddBinding(env, name, type);
  }

  const auto ret_type = MakeTypeModalState(modal_path, transition.target_state);
  if (transition.body) {
    const auto typed =
        TypeBlockForDecl(ctx, *transition.body, env, ret_type, diags);
    if (!typed.ok) {
      if (typed.diag_id.has_value()) {
        EmitTypecheckDiag(diags, *typed.diag_id, transition.body->span);
      }
      return false;
    }
    const auto sub = Subtyping(ctx, typed.type, ret_type);
    if (!sub.ok) {
      if (sub.diag_id.has_value()) {
        EmitTypecheckDiag(diags, *sub.diag_id, transition.body->span);
      }
      return false;
    }
    if (!sub.subtype) {
      SPEC_RULE("Transition-Body-Err");
      EmitTypecheckDiag(diags, "Transition-Body-Err", transition.body->span);
      return false;
    }
    SPEC_RULE("T-Modal-Transition-Body");
  }

  const BindSelfParam self_param{self_type, ParamMode::Move};
  if (!BindCheckStub(ctx, ctx.current_module, transition.params, transition.body,
                     self_param, diags)) {
    return false;
  }
  if (!ProvBindCheckBody(ctx, ctx.current_module, transition.params, transition.body, self_param, diags)) {
    return false;
  }

  return true;
}

static bool CheckModalDecl(const ScopeContext& ctx,
                           const syntax::ModulePath& module_path,
                           const syntax::ModalDecl& decl,
                           core::DiagnosticStream& diags) {
  SpecDefsDeclTyping();
  if (decl.states.empty()) {
    SPEC_RULE("Modal-NoStates-Err");
    EmitTypecheckDiag(diags, "Modal-NoStates-Err", decl.span);
    return false;
  }

  std::unordered_set<IdKey> state_names;
  for (const auto& state : decl.states) {
    const auto key = IdKeyOf(state.name);
    if (!state_names.insert(key).second) {
      SPEC_RULE("Modal-DupState-Err");
      EmitTypecheckDiag(diags, "Modal-DupState-Err", state.span);
      return false;
    }
    if (IdEq(state.name, decl.name)) {
      SPEC_RULE("Modal-StateName-Err");
      EmitTypecheckDiag(diags, "Modal-StateName-Err", state.span);
      return false;
    }
  }

  for (const auto& state : decl.states) {
    if (!CheckStatePayload(ctx, state, diags)) {
      return false;
    }
  }

  if (!StateMemberVisOk(decl)) {
    SPEC_RULE("StateMemberVisOk-Err");
    EmitTypecheckDiag(diags, "StateMemberVisOk-Err", decl.span);
    return false;
  }

  for (const auto& state : decl.states) {
    std::unordered_set<IdKey> method_names;
    std::unordered_set<IdKey> transition_names;
    for (const auto& member : state.members) {
      if (const auto* method = std::get_if<syntax::StateMethodDecl>(&member)) {
        const auto key = IdKeyOf(method->name);
        if (!method_names.insert(key).second) {
          SPEC_RULE("StateMethod-Dup");
          EmitTypecheckDiag(diags, "StateMethod-Dup", method->span);
          return false;
        }
      } else if (const auto* transition =
                     std::get_if<syntax::TransitionDecl>(&member)) {
        const auto key = IdKeyOf(transition->name);
        if (!transition_names.insert(key).second) {
          SPEC_RULE("Transition-Dup");
          EmitTypecheckDiag(diags, "Transition-Dup", transition->span);
          return false;
        }
      }
    }
  }

  const auto modal_path = TypePathForItem(module_path, decl.name);
  for (const auto& state : decl.states) {
    for (const auto& member : state.members) {
      if (const auto* method = std::get_if<syntax::StateMethodDecl>(&member)) {
        if (!CheckStateMethod(ctx, modal_path, state, *method, diags)) {
          return false;
        }
      } else if (const auto* transition =
                     std::get_if<syntax::TransitionDecl>(&member)) {
        if (!CheckTransition(ctx, decl, modal_path, state, *transition, diags)) {
          return false;
        }
      }
    }
  }

  if (!ImplementsOk(ctx, module_path, modal_path, decl.implements, nullptr, diags,
                    decl.span)) {
    return false;
  }

  SPEC_RULE("Modal-WF");
  SPEC_RULE("WF-ModalDecl");
  return true;
}

static bool CheckClassMethod(const ScopeContext& ctx,
                             const syntax::ModulePath& module_path,
                             const syntax::ClassDecl& decl,
                             const syntax::ClassMethodDecl& method,
                             core::DiagnosticStream& diags) {
  const auto self_type = MakeTypePath({"Self"});
  if (const auto* explicit_recv =
          std::get_if<syntax::ReceiverExplicit>(&method.receiver)) {
    const auto lowered = LowerType(ctx, explicit_recv->type);
    if (!lowered.ok) {
      if (lowered.diag_id.has_value()) {
        EmitTypecheckDiag(diags, *lowered.diag_id, method.span);
      }
      return false;
    }
    bool is_self = false;
    if (const auto* path = std::get_if<TypePathType>(&lowered.type->node)) {
      is_self = path->path.size() == 1 && IdEq(path->path[0], "Self");
    } else if (const auto* perm = std::get_if<TypePerm>(&lowered.type->node)) {
      if (const auto* path = std::get_if<TypePathType>(&perm->base->node)) {
        is_self = path->path.size() == 1 && IdEq(path->path[0], "Self");
      }
    }
    if (!is_self) {
      EmitTypecheckDiag(diags, "Record-Method-RecvSelf-Err", method.span);
      return false;
    }
  }

  auto lower_type = [&](const std::shared_ptr<syntax::Type>& type) -> LowerTypeResult {
    const auto lowered = LowerType(ctx, type);
    if (!lowered.ok) {
      return {false, lowered.diag_id, {}};
    }
    return {true, std::nullopt, lowered.type};
  };
  const auto recv = RecvTypeForReceiver(ctx, self_type, method.receiver, lower_type);
  if (!recv.ok) {
    if (recv.diag_id.has_value()) {
      EmitTypecheckDiag(diags, *recv.diag_id, method.span);
    }
    return false;
  }

  const auto names = ParamNames(method.params);
  if (!DistinctNames(names) || std::find(names.begin(), names.end(), IdKeyOf("self")) != names.end()) {
    EmitTypecheckDiag(diags, "Collect-Dup", method.span);
    return false;
  }

  std::vector<std::pair<std::string, TypeRef>> binds;
  binds.emplace_back("self", recv.type);
  if (!CheckParamTypes(ctx, method.params, self_type, binds, diags, method.span)) {
    return false;
  }

  const auto ret = LowerReturnType(ctx, method.return_type_opt);
  if (!ret.ok) {
    if (ret.diag_id.has_value()) {
      EmitTypecheckDiag(diags, *ret.diag_id, method.span);
    }
    return false;
  }

  TypeEnv env;
  env.scopes.emplace_back();
  for (const auto& [name, type] : binds) {
    (void)AddBinding(env, name, type);
  }

  SPEC_RULE("WF-Class-Method");

  if (!method.body_opt) {
    SPEC_RULE("T-Class-Method-Body-Abstract");
    return true;
  }

  const auto typed =
      TypeBlockForDecl(ctx, *method.body_opt, env, ret.type, diags);
  if (!typed.ok) {
    if (typed.diag_id.has_value()) {
      EmitTypecheckDiag(diags, *typed.diag_id, method.body_opt->span);
    }
    return false;
  }
  // Check explicit return requirement for non-unit return types
  if (!IsPrimType(ret.type, "()") && !HasExplicitReturn(*method.body_opt)) {
    SPEC_RULE("WF-ProcBody-ExplicitReturn-Err");
    EmitTypecheckDiag(diags, "WF-ProcBody-ExplicitReturn-Err", method.body_opt->span);
    return false;
  }
  if (!SubtypeReturn(ctx, typed.type, ret.type, diags, method.body_opt->span)) {
    return false;
  }
  SPEC_RULE("T-Class-Method-Body");
  std::optional<ParamMode> recv_mode;
  if (const auto* explicit_recv = std::get_if<syntax::ReceiverExplicit>(&method.receiver)) {
    recv_mode = LowerParamMode(explicit_recv->mode_opt);
  }
  const BindSelfParam self_param{recv.type, recv_mode};
  if (!BindCheckStub(ctx, module_path, method.params, method.body_opt,
                     self_param, diags)) {
    return false;
  }
  if (!ProvBindCheckBody(ctx, module_path, method.params, method.body_opt, self_param, diags)) {
    return false;
  }
  return true;
}

static bool CheckClassDecl(const ScopeContext& ctx,
                           const syntax::ModulePath& module_path,
                           const syntax::ClassDecl& decl,
                           core::DiagnosticStream& diags) {
  SpecDefsDeclTyping();
  std::unordered_set<IdKey> method_names;
  std::unordered_set<IdKey> field_names;
  bool method_dup = false;
  bool field_dup = false;
  for (const auto& item : decl.items) {
    if (const auto* method = std::get_if<syntax::ClassMethodDecl>(&item)) {
      if (!method_names.insert(IdKeyOf(method->name)).second) {
        method_dup = true;
      }
    } else if (const auto* field = std::get_if<syntax::ClassFieldDecl>(&item)) {
      if (!field_names.insert(IdKeyOf(field->name)).second) {
        field_dup = true;
      }
      const auto lowered = LowerType(ctx, field->type);
      if (!lowered.ok) {
        if (lowered.diag_id.has_value()) {
          EmitTypecheckDiag(diags, *lowered.diag_id, field->span);
        }
        return false;
      }
    }
  }

  if (method_dup) {
    SPEC_RULE("Class-Method-Dup");
    EmitTypecheckDiag(diags, "Class-Method-Dup", decl.span);
    return false;
  }
  if (field_dup) {
    SPEC_RULE("Class-AbstractField-Dup");
    EmitTypecheckDiag(diags, "Class-AbstractField-Dup", decl.span);
    return false;
  }

  for (const auto& name : method_names) {
    if (field_names.find(name) != field_names.end()) {
      SPEC_RULE("Class-Name-Conflict");
      EmitTypecheckDiag(diags, "Class-Name-Conflict", decl.span);
      return false;
    }
  }

  if (!DistinctPaths(decl.supers)) {
    SPEC_RULE("Impl-Duplicate-Class-Err");
    EmitTypecheckDiag(diags, "Impl-Duplicate-Class-Err", decl.span);
    return false;
  }

  for (const auto& super : decl.supers) {
    if (!ClassPathOk(ctx, super, diags, decl.span)) {
      SPEC_RULE("Superclass-Undefined");
      return false;
    }
  }

  for (const auto& item : decl.items) {
    const auto* method = std::get_if<syntax::ClassMethodDecl>(&item);
    if (!method) {
      continue;
    }
    if (!CheckClassMethod(ctx, module_path, decl, *method, diags)) {
      return false;
    }
  }

  const auto class_path = TypePathForItem(module_path, decl.name);
  const auto linearized = LinearizeClass(ctx, class_path);
  if (!linearized.ok) {
    SPEC_RULE("Superclass-Cycle");
    EmitTypecheckDiag(diags, "Superclass-Cycle", decl.span);
    return false;
  }

  const auto method_table = ClassMethodTable(ctx, class_path);
  if (!method_table.ok) {
    if (method_table.diag_id.has_value()) {
      EmitTypecheckDiag(diags, *method_table.diag_id, decl.span);
    }
    return false;
  }

  const auto field_table = ClassFieldTable(ctx, class_path);
  if (!field_table.ok) {
    if (field_table.diag_id.has_value()) {
      EmitTypecheckDiag(diags, *field_table.diag_id, decl.span);
    }
    return false;
  }

  SPEC_RULE("WF-Class");
  SPEC_RULE("WF-ClassDecl");
  return true;
}

static bool DeclTypingItem(const ScopeContext& ctx,
                           const syntax::ModulePath& module_path,
                           const syntax::ASTItem& item,
                           core::DiagnosticStream& diags) {
  SpecDefsDeclTyping();
  return std::visit(
      [&](const auto& node) -> bool {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::UsingDecl>) {
          return true;
        } else if constexpr (std::is_same_v<T, syntax::StaticDecl>) {
          return CheckStaticDecl(ctx, node, diags);
        } else if constexpr (std::is_same_v<T, syntax::TypeAliasDecl>) {
          if (!TypeAliasOk(ctx, TypePathForItem(module_path, node.name))) {
            SPEC_RULE("TypeAlias-Recursive-Err");
            EmitTypecheckDiag(diags, "TypeAlias-Recursive-Err", node.span);
            return false;
          }
          SPEC_RULE("TypeAlias-Ok");
          return true;
        } else if constexpr (std::is_same_v<T, syntax::ProcedureDecl>) {
          return CheckProcedureDecl(ctx, node, diags);
        } else if constexpr (std::is_same_v<T, syntax::RecordDecl>) {
          return CheckRecordDecl(ctx, module_path, node, diags);
        } else if constexpr (std::is_same_v<T, syntax::EnumDecl>) {
          return CheckEnumDecl(ctx, module_path, node, diags);
        } else if constexpr (std::is_same_v<T, syntax::ModalDecl>) {
          return CheckModalDecl(ctx, module_path, node, diags);
        } else if constexpr (std::is_same_v<T, syntax::ClassDecl>) {
          return CheckClassDecl(ctx, module_path, node, diags);
        } else {
          return true;
        }
      },
      item);
}

}  // namespace

DeclTypingResult DeclTypingModules(ScopeContext& ctx,
                                   const std::vector<syntax::ASTModule>& modules,
                                   const NameMapTable& name_maps) {
  SpecDefsDeclTyping();
  DeclTypingResult result;
  for (const auto& module : modules) {
    ctx.current_module = module.path;
    Scope module_scope;
    const auto map_it = name_maps.find(PathKeyOf(module.path));
    if (map_it != name_maps.end()) {
      module_scope = map_it->second;
    }
    ctx.scopes = {Scope{}, std::move(module_scope), UniverseBindings()};
    for (const auto& item : module.items) {
      (void)DeclTypingItem(ctx, module.path, item, result.diags);
    }
  }
  result.ok = !core::HasError(result.diags);
  return result;
}

DeclTypingResult MainCheckProject(ScopeContext& ctx,
                                  const std::vector<syntax::ASTModule>& modules) {
  (void)ctx;
  SpecDefsDeclTyping();
  DeclTypingResult result;
  std::vector<const syntax::ProcedureDecl*> mains;
  for (const auto& module : modules) {
    for (const auto& item : module.items) {
      if (const auto* proc = std::get_if<syntax::ProcedureDecl>(&item)) {
        if (IdEq(proc->name, "main")) {
          mains.push_back(proc);
        }
      }
    }
  }

  if (mains.empty()) {
    SPEC_RULE("Main-Missing");
    EmitTypecheckDiag(result.diags, "Main-Missing", std::nullopt);
    result.ok = false;
    return result;
  }
  if (mains.size() > 1) {
    SPEC_RULE("Main-Multiple");
    EmitTypecheckDiag(result.diags, "Main-Multiple", mains.front()->span);
    result.ok = false;
    return result;
  }

  const auto* main_decl = mains.front();
  if (MainGeneric(*main_decl)) {
    SPEC_RULE("Main-Generic-Err");
    EmitTypecheckDiag(result.diags, "Main-Generic-Err", main_decl->span);
    result.ok = false;
    return result;
  }
  if (!MainSigOk(*main_decl)) {
    SPEC_RULE("Main-Signature-Err");
    EmitTypecheckDiag(result.diags, "Main-Signature-Err", main_decl->span);
    result.ok = false;
    return result;
  }

  SPEC_RULE("Main-Ok");
  result.ok = true;
  return result;
}

}  // namespace cursive0::sema
