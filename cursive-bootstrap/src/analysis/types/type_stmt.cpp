#include "cursive0/analysis/types/type_stmt.h"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

#include "cursive0/core/assert_spec.h"
#include "cursive0/core/diagnostic_messages.h"
#include "cursive0/analysis/contracts/verification.h"
#include "cursive0/analysis/composite/classes.h"
#include "cursive0/analysis/resolve/scopes.h"
#include "cursive0/analysis/types/subtyping.h"
#include "cursive0/analysis/types/type_equiv.h"
#include "cursive0/analysis/types/type_infer.h"
#include "cursive0/analysis/types/type_match.h"
#include "cursive0/analysis/memory/regions.h"

namespace cursive0::analysis {

bool BitcopyType(const ScopeContext& ctx, const TypeRef& type);
ExprTypeResult TypeExpr(const ScopeContext& ctx,
                        const StmtTypeContext& type_ctx,
                        const syntax::ExprPtr& expr,
                        const TypeEnv& env);
ExprTypeResult TypeIdentifierExpr(const ScopeContext& ctx,
                                  const syntax::IdentifierExpr& expr,
                                  const TypeEnv& env);
PlaceTypeResult TypePlace(const ScopeContext& ctx,
                          const StmtTypeContext& type_ctx,
                          const syntax::ExprPtr& expr,
                          const TypeEnv& env);

namespace {

struct IntroResult {
  bool ok = false;
  std::optional<std::string_view> diag_id;
  TypeEnv env;
};

struct TypeLowerResult {
  bool ok = false;
  std::optional<std::string_view> diag_id;
  TypeRef type;
};

static inline void SpecDefsTypeStmt() {
  SPEC_DEF("MutKind", "5.2.11");
  SPEC_DEF("Bind", "5.2.11");
  SPEC_DEF("BindOf", "5.2.11");
  SPEC_DEF("MutOf", "5.2.11");
  SPEC_DEF("StmtJudg", "5.2.11");
  SPEC_DEF("LoopFlag", "5.2.11");
  SPEC_DEF("PushScope", "5.2.11");
  SPEC_DEF("PopScope", "5.2.11");
  SPEC_DEF("IntroAll", "5.2.11");
  SPEC_DEF("IntroAllVar", "5.2.11");
  SPEC_DEF("ShadowAll", "5.2.11");
  SPEC_DEF("ShadowAllVar", "5.2.11");
  SPEC_DEF("ResType", "5.2.11");
  SPEC_DEF("LoopTypeInf", "5.2.11");
  SPEC_DEF("LoopTypeFin", "5.2.11");
  SPEC_DEF("LastStmt", "5.2.11");
  SPEC_DEF("ResultNotLast", "5.2.11");
  SPEC_DEF("FirstResultSpan", "5.2.11");
  SPEC_DEF("WarnResultUnreachable", "5.2.11");
  SPEC_DEF("HasNonLocalCtrl", "5.2.11");
  SPEC_DEF("DeferSafe", "5.2.11");
  SPEC_DEF("RegionActiveType", "5.2.17");
  SPEC_DEF("RegionOptsExpr", "5.2.17");
  SPEC_DEF("RegionBind", "5.2.17");
  SPEC_DEF("InnermostActiveRegion", "5.2.17");
  SPEC_DEF("FrameBind", "5.2.17");
  SPEC_DEF("StripPerm", "5.2.12");
  SPEC_DEF("NumericTypes", "5.2.12");
}

static TypeRef StripPerm(const TypeRef& type) {
  if (!type) {
    return type;
  }
  if (const auto* perm = std::get_if<TypePerm>(&type->node)) {
    return perm->base;
  }
  return type;
}

static bool IsPrimType(const TypeRef& type, std::string_view name) {
  if (!type) {
    return false;
  }
  const auto* prim = std::get_if<TypePrim>(&type->node);
  return prim && prim->name == name;
}

static syntax::ExprPtr MakeExpr(const core::Span& span, syntax::ExprNode node) {
  auto expr = std::make_shared<syntax::Expr>();
  expr->span = span;
  expr->node = std::move(node);
  return expr;
}

static syntax::ExprPtr MakeUnitExpr(const core::Span& span) {
  syntax::TupleExpr tuple;
  return MakeExpr(span, tuple);
}

static syntax::ExprPtr SubstituteResultEntry(const syntax::ExprPtr& expr,
                                              const syntax::ExprPtr& result_expr) {
  if (!expr) {
    return expr;
  }
  if (std::holds_alternative<syntax::ResultExpr>(expr->node)) {
    return result_expr;
  }
  return std::visit(
      [&](const auto& node) -> syntax::ExprPtr {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::BinaryExpr>) {
          auto out = node;
          out.lhs = SubstituteResultEntry(node.lhs, result_expr);
          out.rhs = SubstituteResultEntry(node.rhs, result_expr);
          return MakeExpr(expr->span, out);
        } else if constexpr (std::is_same_v<T, syntax::UnaryExpr>) {
          auto out = node;
          out.value = SubstituteResultEntry(node.value, result_expr);
          return MakeExpr(expr->span, out);
        } else if constexpr (std::is_same_v<T, syntax::FieldAccessExpr>) {
          auto out = node;
          out.base = SubstituteResultEntry(node.base, result_expr);
          return MakeExpr(expr->span, out);
        } else if constexpr (std::is_same_v<T, syntax::TupleAccessExpr>) {
          auto out = node;
          out.base = SubstituteResultEntry(node.base, result_expr);
          return MakeExpr(expr->span, out);
        } else if constexpr (std::is_same_v<T, syntax::IndexAccessExpr>) {
          auto out = node;
          out.base = SubstituteResultEntry(node.base, result_expr);
          out.index = SubstituteResultEntry(node.index, result_expr);
          return MakeExpr(expr->span, out);
        } else if constexpr (std::is_same_v<T, syntax::CallExpr>) {
          auto out = node;
          out.callee = SubstituteResultEntry(node.callee, result_expr);
          for (auto& arg : out.args) {
            arg.value = SubstituteResultEntry(arg.value, result_expr);
          }
          return MakeExpr(expr->span, out);
        } else if constexpr (std::is_same_v<T, syntax::QualifiedApplyExpr>) {
          auto out = node;
          if (std::holds_alternative<syntax::ParenArgs>(node.args)) {
            auto paren = std::get<syntax::ParenArgs>(node.args);
            for (auto& arg : paren.args) {
              arg.value = SubstituteResultEntry(arg.value, result_expr);
            }
            out.args = paren;
          } else {
            auto brace = std::get<syntax::BraceArgs>(node.args);
            for (auto& field : brace.fields) {
              field.value = SubstituteResultEntry(field.value, result_expr);
            }
            out.args = brace;
          }
          return MakeExpr(expr->span, out);
        } else if constexpr (std::is_same_v<T, syntax::MethodCallExpr>) {
          auto out = node;
          out.receiver = SubstituteResultEntry(node.receiver, result_expr);
          for (auto& arg : out.args) {
            arg.value = SubstituteResultEntry(arg.value, result_expr);
          }
          return MakeExpr(expr->span, out);
        } else if constexpr (std::is_same_v<T, syntax::CastExpr>) {
          auto out = node;
          out.value = SubstituteResultEntry(node.value, result_expr);
          return MakeExpr(expr->span, out);
        } else if constexpr (std::is_same_v<T, syntax::RangeExpr>) {
          auto out = node;
          out.lhs = SubstituteResultEntry(node.lhs, result_expr);
          out.rhs = SubstituteResultEntry(node.rhs, result_expr);
          return MakeExpr(expr->span, out);
        } else if constexpr (std::is_same_v<T, syntax::DerefExpr>) {
          auto out = node;
          out.value = SubstituteResultEntry(node.value, result_expr);
          return MakeExpr(expr->span, out);
        } else if constexpr (std::is_same_v<T, syntax::AddressOfExpr>) {
          auto out = node;
          out.place = SubstituteResultEntry(node.place, result_expr);
          return MakeExpr(expr->span, out);
        } else if constexpr (std::is_same_v<T, syntax::MoveExpr>) {
          auto out = node;
          out.place = SubstituteResultEntry(node.place, result_expr);
          return MakeExpr(expr->span, out);
        } else if constexpr (std::is_same_v<T, syntax::AllocExpr>) {
          auto out = node;
          out.value = SubstituteResultEntry(node.value, result_expr);
          return MakeExpr(expr->span, out);
        } else if constexpr (std::is_same_v<T, syntax::TupleExpr>) {
          auto out = node;
          for (auto& elem : out.elements) {
            elem = SubstituteResultEntry(elem, result_expr);
          }
          return MakeExpr(expr->span, out);
        } else if constexpr (std::is_same_v<T, syntax::ArrayExpr>) {
          auto out = node;
          for (auto& elem : out.elements) {
            elem = SubstituteResultEntry(elem, result_expr);
          }
          return MakeExpr(expr->span, out);
        } else if constexpr (std::is_same_v<T, syntax::RecordExpr>) {
          auto out = node;
          for (auto& field : out.fields) {
            field.value = SubstituteResultEntry(field.value, result_expr);
          }
          return MakeExpr(expr->span, out);
        } else if constexpr (std::is_same_v<T, syntax::EnumLiteralExpr>) {
          auto out = node;
          if (out.payload_opt.has_value()) {
            if (std::holds_alternative<syntax::EnumPayloadParen>(*out.payload_opt)) {
              auto paren = std::get<syntax::EnumPayloadParen>(*out.payload_opt);
              for (auto& elem : paren.elements) {
                elem = SubstituteResultEntry(elem, result_expr);
              }
              out.payload_opt = paren;
            } else {
              auto brace = std::get<syntax::EnumPayloadBrace>(*out.payload_opt);
              for (auto& field : brace.fields) {
                field.value = SubstituteResultEntry(field.value, result_expr);
              }
              out.payload_opt = brace;
            }
          }
          return MakeExpr(expr->span, out);
        } else if constexpr (std::is_same_v<T, syntax::IfExpr>) {
          auto out = node;
          out.cond = SubstituteResultEntry(node.cond, result_expr);
          out.then_expr = SubstituteResultEntry(node.then_expr, result_expr);
          out.else_expr = SubstituteResultEntry(node.else_expr, result_expr);
          return MakeExpr(expr->span, out);
        } else if constexpr (std::is_same_v<T, syntax::MatchExpr>) {
          auto out = node;
          out.value = SubstituteResultEntry(node.value, result_expr);
          for (auto& arm : out.arms) {
            arm.guard_opt = SubstituteResultEntry(arm.guard_opt, result_expr);
            arm.body = SubstituteResultEntry(arm.body, result_expr);
          }
          return MakeExpr(expr->span, out);
        } else if constexpr (std::is_same_v<T, syntax::PropagateExpr>) {
          auto out = node;
          out.value = SubstituteResultEntry(node.value, result_expr);
          return MakeExpr(expr->span, out);
        } else if constexpr (std::is_same_v<T, syntax::EntryExpr>) {
          auto out = node;
          out.expr = SubstituteResultEntry(node.expr, result_expr);
          return MakeExpr(expr->span, out);
        } else {
          return expr;
        }
      },
      expr->node);
}

static void CollectContractFacts(const syntax::ExprPtr& expr,
                                 StaticProofContext& ctx) {
  if (!expr) {
    return;
  }
  if (const auto* binary = std::get_if<syntax::BinaryExpr>(&expr->node)) {
    if (binary->op == "&&") {
      CollectContractFacts(binary->lhs, ctx);
      CollectContractFacts(binary->rhs, ctx);
      return;
    }
  }
  AddFact(ctx, expr, expr->span);
}

static std::optional<std::string_view> VerifyPostconditionAtReturn(
    const ScopeContext& ctx,
    const StmtTypeContext& type_ctx,
    const syntax::ExprPtr& return_value) {
  if (!type_ctx.contract || !type_ctx.contract->postcondition) {
    return std::nullopt;
  }
  const auto result_expr =
      return_value ? return_value : MakeUnitExpr(type_ctx.contract->postcondition->span);
  const auto pred =
      SubstituteResultEntry(type_ctx.contract->postcondition, result_expr);
  StaticProofContext proof_ctx;
  if (type_ctx.contract->precondition) {
    CollectContractFacts(type_ctx.contract->precondition, proof_ctx);
  }
  const auto proof = StaticProof(proof_ctx, pred);
  if (!proof.provable && !type_ctx.contract_dynamic) {
    return "E-SEM-2801";
  }
  return std::nullopt;
}

static constexpr std::array<std::string_view, 12> kIntTypes = {
    "i8", "i16", "i32", "i64", "i128", "isize",
    "u8", "u16", "u32", "u64", "u128", "usize"};
static constexpr std::array<std::string_view, 3> kFloatTypes = {"f16", "f32",
                                                                "f64"};

static bool IsNumericType(const TypeRef& type) {
  const auto stripped = StripPerm(type);
  if (!stripped) {
    return false;
  }
  const auto* prim = std::get_if<TypePrim>(&stripped->node);
  if (!prim) {
    return false;
  }
  for (const auto& t : kIntTypes) {
    if (prim->name == t) {
      return true;
    }
  }
  for (const auto& t : kFloatTypes) {
    if (prim->name == t) {
      return true;
    }
  }
  return false;
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
          return {true, std::nullopt, MakeTypeString(LowerStringState(node.state))};
        } else if constexpr (std::is_same_v<T, syntax::TypeBytes>) {
          return {true, std::nullopt, MakeTypeBytes(LowerBytesState(node.state))};
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
          auto is_builtin_generic = [&](const syntax::TypePath& path) {
            if (path.size() != 1) {
              return false;
            }
            return IdEq(path[0], "Spawned") ||
                   IdEq(path[0], "Tracked") ||
                   IdEq(path[0], "Async") ||
                   IdEq(path[0], "Sequence") ||
                   IdEq(path[0], "Future") ||
                   IdEq(path[0], "Stream") ||
                   IdEq(path[0], "Pipe") ||
                   IdEq(path[0], "Exchange") ||
                   IdEq(path[0], "Ptr");
          };
          if (!node.generic_args.empty() && is_builtin_generic(node.path)) {
            std::vector<TypeRef> lowered_args;
            for (const auto& arg : node.generic_args) {
              const auto lower_result = LowerType(ctx, arg);
              if (!lower_result.ok) {
                return lower_result;
              }
              lowered_args.push_back(lower_result.type);
            }
            TypePathType result_type;
            result_type.path = node.path;
            result_type.generic_args = std::move(lowered_args);
            return {true, std::nullopt, MakeType(result_type)};
          }
          return {true, std::nullopt, MakeTypePath(node.path)};
        } else {
          return {false, std::nullopt, {}};
        }
      },
      type->node);
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
  SpecDefsTypeStmt();
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
      std::cerr << "[cursivec0] shadow required for `" << name << "`\n";
    }
    return {false, "Intro-Shadow-Required", env};
  }

  TypeEnv out = env;
  out.scopes.back().emplace(key, binding);
  SPEC_RULE("Intro-Ok");
  return {true, std::nullopt, std::move(out)};
}

static IntroResult ShadowIntroBinding(const TypeEnv& env,
                                      std::string_view name,
                                      const TypeBinding& binding) {
  SpecDefsTypeStmt();
  if (ReservedGen(name)) {
    SPEC_RULE("Shadow-Reserved-Gen-Err");
    return {false, "Shadow-Reserved-Gen-Err", env};
  }
  if (ReservedCursive(name)) {
    SPEC_RULE("Shadow-Reserved-Cursive-Err");
    return {false, "Shadow-Reserved-Cursive-Err", env};
  }

  const auto key = IdKeyOf(name);
  if (env.scopes.empty()) {
    return {false, std::nullopt, env};
  }
  if (InScope(env, key)) {
    return {false, std::nullopt, env};
  }
  if (!InOuter(env, key)) {
    SPEC_RULE("Shadow-Unnecessary");
    return {false, "Shadow-Unnecessary", env};
  }

  TypeEnv out = env;
  out.scopes.back().emplace(key, binding);
  SPEC_RULE("Shadow-Ok");
  return {true, std::nullopt, std::move(out)};
}

static IntroResult IntroAll(const TypeEnv& env,
                            const std::vector<std::pair<std::string, TypeRef>>& binds,
                            syntax::Mutability mut,
                            bool shadow) {
  SpecDefsTypeStmt();
  if (binds.empty()) {
    if (shadow) {
      if (mut == syntax::Mutability::Var) {
        SPEC_RULE("ShadowAllVar-Empty");
      } else {
        SPEC_RULE("ShadowAll-Empty");
      }
    } else {
      if (mut == syntax::Mutability::Var) {
        SPEC_RULE("IntroAllVar-Empty");
      } else {
        SPEC_RULE("IntroAll-Empty");
      }
    }
    return {true, std::nullopt, env};
  }

  TypeEnv current = env;
  for (const auto& [name, type] : binds) {
    TypeBinding binding{mut, type};
    IntroResult res = shadow ? ShadowIntroBinding(current, name, binding)
                             : IntroBinding(current, name, binding);
    if (!res.ok) {
      return res;
    }
    current = std::move(res.env);
    if (shadow) {
      if (mut == syntax::Mutability::Var) {
        SPEC_RULE("ShadowAllVar-Cons");
      } else {
        SPEC_RULE("ShadowAll-Cons");
      }
    } else {
      if (mut == syntax::Mutability::Var) {
        SPEC_RULE("IntroAllVar-Cons");
      } else {
        SPEC_RULE("IntroAll-Cons");
      }
    }
  }

  return {true, std::nullopt, std::move(current)};
}

static void CollectFieldPatNames(const syntax::FieldPattern& field,
                                 std::vector<IdKey>& out);

static void CollectPatNames(const syntax::Pattern& pat,
                            std::vector<IdKey>& out) {
  std::visit(
      [&](const auto& node) {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::IdentifierPattern>) {
          out.push_back(IdKeyOf(node.name));
        } else if constexpr (std::is_same_v<T, syntax::TypedPattern>) {
          out.push_back(IdKeyOf(node.name));
        } else if constexpr (std::is_same_v<T, syntax::TuplePattern>) {
          for (const auto& elem : node.elements) {
            if (elem) {
              CollectPatNames(*elem, out);
            }
          }
        } else if constexpr (std::is_same_v<T, syntax::RecordPattern>) {
          for (const auto& field : node.fields) {
            CollectFieldPatNames(field, out);
          }
        } else if constexpr (std::is_same_v<T, syntax::EnumPattern>) {
          if (!node.payload_opt.has_value()) {
            return;
          }
          std::visit(
              [&](const auto& payload) {
                using P = std::decay_t<decltype(payload)>;
                if constexpr (std::is_same_v<P, syntax::TuplePayloadPattern>) {
                  for (const auto& elem : payload.elements) {
                    if (elem) {
                      CollectPatNames(*elem, out);
                    }
                  }
                } else {
                  for (const auto& field : payload.fields) {
                    CollectFieldPatNames(field, out);
                  }
                }
              },
              *node.payload_opt);
        } else if constexpr (std::is_same_v<T, syntax::ModalPattern>) {
          if (!node.fields_opt.has_value()) {
            return;
          }
          for (const auto& field : node.fields_opt->fields) {
            CollectFieldPatNames(field, out);
          }
        } else if constexpr (std::is_same_v<T, syntax::RangePattern>) {
          if (node.lo) {
            CollectPatNames(*node.lo, out);
          }
          if (node.hi) {
            CollectPatNames(*node.hi, out);
          }
        } else {
          return;
        }
      },
      pat.node);
}

static void CollectFieldPatNames(const syntax::FieldPattern& field,
                                 std::vector<IdKey>& out) {
  if (field.pattern_opt) {
    CollectPatNames(*field.pattern_opt, out);
    return;
  }
  out.push_back(IdKeyOf(field.name));
}

static bool DistinctNames(const std::vector<IdKey>& names) {
  std::unordered_set<IdKey> seen;
  for (const auto& name : names) {
    if (!seen.insert(name).second) {
      return false;
    }
  }
  return true;
}

static const syntax::RecordDecl* LookupRecordDecl(const ScopeContext& ctx,
                                                  const syntax::TypePath& path) {
  const auto it = ctx.sigma.types.find(PathKeyOf(path));
  if (it == ctx.sigma.types.end()) {
    return nullptr;
  }
  return std::get_if<syntax::RecordDecl>(&it->second);
}

static bool FieldVisible(const syntax::ModulePath& current,
                         const syntax::TypePath& record_path,
                         const syntax::FieldDecl& field) {
  if (field.vis == syntax::Visibility::Public ||
      field.vis == syntax::Visibility::Internal) {
    return true;
  }
  if (record_path.empty()) {
    return false;
  }
  syntax::ModulePath module_path = record_path;
  module_path.pop_back();
  return module_path == current;
}

static const syntax::FieldDecl* LookupFieldDecl(
    const syntax::RecordDecl& record,
    std::string_view name) {
  for (const auto& member : record.members) {
    const auto* field = std::get_if<syntax::FieldDecl>(&member);
    if (!field) {
      continue;
    }
    if (IdKeyOf(field->name) == IdKeyOf(name)) {
      return field;
    }
  }
  return nullptr;
}

static bool IsPlaceExpr(const syntax::ExprPtr& expr) {
  if (!expr) {
    return false;
  }
  if (std::holds_alternative<syntax::IdentifierExpr>(expr->node)) {
    return true;
  }
  if (std::holds_alternative<syntax::FieldAccessExpr>(expr->node)) {
    return true;
  }
  if (std::holds_alternative<syntax::TupleAccessExpr>(expr->node)) {
    return true;
  }
  if (std::holds_alternative<syntax::IndexAccessExpr>(expr->node)) {
    return true;
  }
  if (const auto* deref = std::get_if<syntax::DerefExpr>(&expr->node)) {
    return IsPlaceExpr(deref->value);
  }
  return false;
}

static std::optional<std::string_view> PlaceRootName(
    const syntax::ExprPtr& expr) {
  if (!expr) {
    return std::nullopt;
  }
  if (const auto* ident = std::get_if<syntax::IdentifierExpr>(&expr->node)) {
    return ident->name;
  }
  if (const auto* field = std::get_if<syntax::FieldAccessExpr>(&expr->node)) {
    return PlaceRootName(field->base);
  }
  if (const auto* tup = std::get_if<syntax::TupleAccessExpr>(&expr->node)) {
    return PlaceRootName(tup->base);
  }
  if (const auto* idx = std::get_if<syntax::IndexAccessExpr>(&expr->node)) {
    return PlaceRootName(idx->base);
  }
  if (const auto* deref = std::get_if<syntax::DerefExpr>(&expr->node)) {
    return PlaceRootName(deref->value);
  }
  return std::nullopt;
}

static ExprTypeResult TypeIdentExpr(const ScopeContext& ctx,
                                    const TypeEnv& env,
                                    std::string_view name) {
  const auto binding = BindOf(env, name);
  if (!binding.has_value()) {
    return {false, "ResolveExpr-Ident-Err", {}};
  }
  if (!BitcopyType(ctx, binding->type)) {
    SPEC_RULE("ValueUse-NonBitcopyPlace");
    return {false, "ValueUse-NonBitcopyPlace", {}};
  }
  return {true, std::nullopt, binding->type};
}

static ExprTypeResult TypeExprWithEnv(const ScopeContext& ctx,
                                      const TypeEnv& env,
                                      const ExprTypeFn& type_expr,
                                      const syntax::ExprPtr& expr) {
  if (!expr) {
    return {false, std::nullopt, {}};
  }
  if (const auto* ident = std::get_if<syntax::IdentifierExpr>(&expr->node)) {
    const auto binding = BindOf(env, ident->name);
    if (binding.has_value()) {
      return TypeIdentExpr(ctx, env, ident->name);
    }
  }
  return type_expr(expr);
}

static PlaceTypeResult TypePlaceWithEnv(const TypeEnv& env,
                                        const PlaceTypeFn& type_place,
                                        const syntax::ExprPtr& expr) {
  if (!expr) {
    return {false, std::nullopt, {}};
  }
  if (const auto* ident = std::get_if<syntax::IdentifierExpr>(&expr->node)) {
    const auto binding = BindOf(env, ident->name);
    if (binding.has_value()) {
      return {true, std::nullopt, binding->type};
    }
  }
  return type_place(expr);
}

static IdentTypeFn IdentTypeFnWithEnv(const ScopeContext& ctx,
                                      const TypeEnv& env,
                                      const IdentTypeFn& type_ident) {
  return [&](std::string_view name) -> ExprTypeResult {
    const auto binding = BindOf(env, name);
    if (binding.has_value()) {
      return TypeIdentExpr(ctx, env, name);
    }
    return type_ident(name);
  };
}

static MatchCheckFn MakeMatchCheck(const ScopeContext& ctx,
                                   const StmtTypeContext& type_ctx,
                                   const TypeEnv& env) {
  return [&](const syntax::MatchExpr& match,
             const TypeRef& expected) -> CheckResult {
    return CheckMatchExpr(ctx, type_ctx, match, env, expected);
  };
}

static std::optional<TypeRef> ResType(const ScopeContext& ctx,
                                      const std::vector<TypeRef>& types) {
  SpecDefsTypeStmt();
  if (types.empty()) {
    return std::nullopt;
  }
  TypeRef base = types.front();
  for (std::size_t i = 1; i < types.size(); ++i) {
    const auto equiv = TypeEquiv(base, types[i]);
    if (!equiv.ok || !equiv.equiv) {
      return std::nullopt;
    }
  }
  return base;
}

static std::optional<TypeRef> LoopTypeInf(const ScopeContext& ctx,
                                          const std::vector<TypeRef>& breaks,
                                          bool break_void) {
  SpecDefsTypeStmt();
  if (breaks.empty() && !break_void) {
    return MakeTypePrim("!");
  }
  if (breaks.empty() && break_void) {
    return MakeTypePrim("()");
  }
  if (!break_void) {
    const auto res = ResType(ctx, breaks);
    if (res.has_value()) {
      return res;
    }
  }
  return std::nullopt;
}

static std::optional<TypeRef> LoopTypeFin(const ScopeContext& ctx,
                                          const std::vector<TypeRef>& breaks,
                                          bool break_void) {
  SpecDefsTypeStmt();
  if (breaks.empty()) {
    return MakeTypePrim("()");
  }
  if (!break_void) {
    const auto res = ResType(ctx, breaks);
    if (res.has_value()) {
      return res;
    }
  }
  return std::nullopt;
}

static bool ResultNotLast(const std::vector<syntax::Stmt>& stmts) {
  for (std::size_t i = 0; i + 1 < stmts.size(); ++i) {
    if (std::holds_alternative<syntax::ResultStmt>(stmts[i])) {
      return true;
    }
  }
  return false;
}

static std::optional<core::Span> FirstResultSpan(
    const std::vector<syntax::Stmt>& stmts) {
  for (const auto& stmt : stmts) {
    if (const auto* res = std::get_if<syntax::ResultStmt>(&stmt)) {
      return res->span;
    }
  }
  return std::nullopt;
}

static void WarnResultUnreachable(const ScopeContext& ctx,
                                  const StmtTypeContext& type_ctx,
                                  const std::vector<syntax::Stmt>& stmts) {
  (void)ctx;
  if (ResultNotLast(stmts)) {
    SPEC_RULE("Warn-Result-Unreachable");
    if (!type_ctx.diags) {
      return;
    }
    const auto span = FirstResultSpan(stmts);
    if (!span.has_value()) {
      return;
    }
    if (auto diag = core::MakeDiagnostic("W-SEM-1001", *span)) {
      *type_ctx.diags = core::Emit(*type_ctx.diags, *diag);
    }
    return;
  }
  SPEC_RULE("Warn-Result-Ok");
}

static bool HasNonLocalCtrlExpr(const syntax::ExprPtr& expr, bool in_loop);

static bool HasNonLocalCtrlBlock(const syntax::Block& block, bool in_loop);

static bool HasNonLocalCtrlStmt(const syntax::Stmt& stmt, bool in_loop) {
  return std::visit(
      [&](const auto& node) -> bool {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::ReturnStmt>) {
          SPEC_RULE("HasNonLocalCtrl-Return");
          return true;
        } else if constexpr (std::is_same_v<T, syntax::ResultStmt>) {
          SPEC_RULE("HasNonLocalCtrl-Result");
          return true;
        } else if constexpr (std::is_same_v<T, syntax::BreakStmt>) {
          if (!in_loop) {
            SPEC_RULE("HasNonLocalCtrl-Break");
            return true;
          }
          return false;
        } else if constexpr (std::is_same_v<T, syntax::ContinueStmt>) {
          if (!in_loop) {
            SPEC_RULE("HasNonLocalCtrl-Continue");
            return true;
          }
          return false;
        } else if constexpr (std::is_same_v<T, syntax::LetStmt>) {
          if (HasNonLocalCtrlExpr(node.binding.init, in_loop)) {
            SPEC_RULE("HasNonLocalCtrl-Child");
            return true;
          }
          return false;
        } else if constexpr (std::is_same_v<T, syntax::VarStmt>) {
          if (HasNonLocalCtrlExpr(node.binding.init, in_loop)) {
            SPEC_RULE("HasNonLocalCtrl-Child");
            return true;
          }
          return false;
        } else if constexpr (std::is_same_v<T, syntax::ShadowLetStmt> ||
                             std::is_same_v<T, syntax::ShadowVarStmt>) {
          if (HasNonLocalCtrlExpr(node.init, in_loop)) {
            SPEC_RULE("HasNonLocalCtrl-Child");
            return true;
          }
          return false;
        } else if constexpr (std::is_same_v<T, syntax::AssignStmt>) {
          if (HasNonLocalCtrlExpr(node.place, in_loop) ||
              HasNonLocalCtrlExpr(node.value, in_loop)) {
            SPEC_RULE("HasNonLocalCtrl-Child");
            return true;
          }
          return false;
        } else if constexpr (std::is_same_v<T, syntax::CompoundAssignStmt>) {
          if (HasNonLocalCtrlExpr(node.place, in_loop) ||
              HasNonLocalCtrlExpr(node.value, in_loop)) {
            SPEC_RULE("HasNonLocalCtrl-Child");
            return true;
          }
          return false;
        } else if constexpr (std::is_same_v<T, syntax::ExprStmt>) {
          if (HasNonLocalCtrlExpr(node.value, in_loop)) {
            SPEC_RULE("HasNonLocalCtrl-Child");
            return true;
          }
          return false;
        } else if constexpr (std::is_same_v<T, syntax::DeferStmt>) {
          if (node.body && HasNonLocalCtrlBlock(*node.body, in_loop)) {
            SPEC_RULE("HasNonLocalCtrl-Child");
            return true;
          }
          return false;
        } else if constexpr (std::is_same_v<T, syntax::RegionStmt>) {
          if (HasNonLocalCtrlExpr(node.opts_opt, in_loop)) {
            SPEC_RULE("HasNonLocalCtrl-Child");
            return true;
          }
          if (node.body && HasNonLocalCtrlBlock(*node.body, in_loop)) {
            SPEC_RULE("HasNonLocalCtrl-Child");
            return true;
          }
          return false;
        } else if constexpr (std::is_same_v<T, syntax::FrameStmt>) {
          if (node.body && HasNonLocalCtrlBlock(*node.body, in_loop)) {
            SPEC_RULE("HasNonLocalCtrl-Child");
            return true;
          }
          return false;
        } else if constexpr (std::is_same_v<T, syntax::UnsafeBlockStmt>) {
          if (node.body && HasNonLocalCtrlBlock(*node.body, in_loop)) {
            SPEC_RULE("HasNonLocalCtrl-Child");
            return true;
          }
          return false;
        } else if constexpr (std::is_same_v<T, syntax::KeyBlockStmt>) {
          // C0X Extension: Key block statement
          if (node.body && HasNonLocalCtrlBlock(*node.body, in_loop)) {
            SPEC_RULE("HasNonLocalCtrl-Child");
            return true;
          }
          return false;
        } else {
          return false;
        }
      },
      stmt);
}

static bool HasNonLocalCtrlBlock(const syntax::Block& block, bool in_loop) {
  for (const auto& stmt : block.stmts) {
    if (HasNonLocalCtrlStmt(stmt, in_loop)) {
      return true;
    }
  }
  if (block.tail_opt && HasNonLocalCtrlExpr(block.tail_opt, in_loop)) {
    SPEC_RULE("HasNonLocalCtrl-Child");
    return true;
  }
  return false;
}

static bool HasNonLocalCtrlExpr(const syntax::ExprPtr& expr, bool in_loop) {
  if (!expr) {
    return false;
  }
  return std::visit(
      [&](const auto& node) -> bool {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::LoopInfiniteExpr>) {
          if (node.body && HasNonLocalCtrlBlock(*node.body, true)) {
            SPEC_RULE("HasNonLocalCtrl-LoopInfinite");
            return true;
          }
          return false;
        } else if constexpr (std::is_same_v<T, syntax::LoopConditionalExpr>) {
          if (HasNonLocalCtrlExpr(node.cond, in_loop)) {
            SPEC_RULE("HasNonLocalCtrl-Child");
            return true;
          }
          if (node.body && HasNonLocalCtrlBlock(*node.body, true)) {
            SPEC_RULE("HasNonLocalCtrl-LoopConditional");
            return true;
          }
          return false;
        } else if constexpr (std::is_same_v<T, syntax::LoopIterExpr>) {
          if (HasNonLocalCtrlExpr(node.iter, in_loop)) {
            SPEC_RULE("HasNonLocalCtrl-Child");
            return true;
          }
          if (node.body && HasNonLocalCtrlBlock(*node.body, true)) {
            SPEC_RULE("HasNonLocalCtrl-LoopIter");
            return true;
          }
          return false;
        } else if constexpr (std::is_same_v<T, syntax::BlockExpr>) {
          if (node.block && HasNonLocalCtrlBlock(*node.block, in_loop)) {
            SPEC_RULE("HasNonLocalCtrl-Child");
            return true;
          }
          return false;
        } else if constexpr (std::is_same_v<T, syntax::UnsafeBlockExpr>) {
          if (node.block && HasNonLocalCtrlBlock(*node.block, in_loop)) {
            SPEC_RULE("HasNonLocalCtrl-Child");
            return true;
          }
          return false;
        } else if constexpr (std::is_same_v<T, syntax::IfExpr>) {
          if (HasNonLocalCtrlExpr(node.cond, in_loop) ||
              HasNonLocalCtrlExpr(node.then_expr, in_loop) ||
              HasNonLocalCtrlExpr(node.else_expr, in_loop)) {
            SPEC_RULE("HasNonLocalCtrl-Child");
            return true;
          }
          return false;
        } else if constexpr (std::is_same_v<T, syntax::MatchExpr>) {
          if (HasNonLocalCtrlExpr(node.value, in_loop)) {
            SPEC_RULE("HasNonLocalCtrl-Child");
            return true;
          }
          for (const auto& arm : node.arms) {
            if (HasNonLocalCtrlExpr(arm.guard_opt, in_loop) ||
                HasNonLocalCtrlExpr(arm.body, in_loop)) {
              SPEC_RULE("HasNonLocalCtrl-Child");
              return true;
            }
          }
          return false;
        } else if constexpr (std::is_same_v<T, syntax::QualifiedApplyExpr>) {
          return std::visit(
              [&](const auto& args) -> bool {
                using A = std::decay_t<decltype(args)>;
                if constexpr (std::is_same_v<A, syntax::ParenArgs>) {
                  for (const auto& arg : args.args) {
                    if (HasNonLocalCtrlExpr(arg.value, in_loop)) {
                      SPEC_RULE("HasNonLocalCtrl-Child");
                      return true;
                    }
                  }
                } else {
                  for (const auto& field : args.fields) {
                    if (HasNonLocalCtrlExpr(field.value, in_loop)) {
                      SPEC_RULE("HasNonLocalCtrl-Child");
                      return true;
                    }
                  }
                }
                return false;
              },
              node.args);
        } else if constexpr (std::is_same_v<T, syntax::EnumLiteralExpr>) {
          if (!node.payload_opt.has_value()) {
            return false;
          }
          return std::visit(
              [&](const auto& payload) -> bool {
                using P = std::decay_t<decltype(payload)>;
                if constexpr (std::is_same_v<P, syntax::EnumPayloadParen>) {
                  for (const auto& elem : payload.elements) {
                    if (HasNonLocalCtrlExpr(elem, in_loop)) {
                      SPEC_RULE("HasNonLocalCtrl-Child");
                      return true;
                    }
                  }
                  return false;
                } else {
                  for (const auto& field : payload.fields) {
                    if (HasNonLocalCtrlExpr(field.value, in_loop)) {
                      SPEC_RULE("HasNonLocalCtrl-Child");
                      return true;
                    }
                  }
                  return false;
                }
              },
              *node.payload_opt);
        } else if constexpr (std::is_same_v<T, syntax::RangeExpr>) {
          if (HasNonLocalCtrlExpr(node.lhs, in_loop) ||
              HasNonLocalCtrlExpr(node.rhs, in_loop)) {
            SPEC_RULE("HasNonLocalCtrl-Child");
            return true;
          }
          return false;
        } else if constexpr (std::is_same_v<T, syntax::BinaryExpr>) {
          if (HasNonLocalCtrlExpr(node.lhs, in_loop) ||
              HasNonLocalCtrlExpr(node.rhs, in_loop)) {
            SPEC_RULE("HasNonLocalCtrl-Child");
            return true;
          }
          return false;
        } else if constexpr (std::is_same_v<T, syntax::CastExpr>) {
          if (HasNonLocalCtrlExpr(node.value, in_loop)) {
            SPEC_RULE("HasNonLocalCtrl-Child");
            return true;
          }
          return false;
        } else if constexpr (std::is_same_v<T, syntax::UnaryExpr>) {
          if (HasNonLocalCtrlExpr(node.value, in_loop)) {
            SPEC_RULE("HasNonLocalCtrl-Child");
            return true;
          }
          return false;
        } else if constexpr (std::is_same_v<T, syntax::DerefExpr>) {
          if (HasNonLocalCtrlExpr(node.value, in_loop)) {
            SPEC_RULE("HasNonLocalCtrl-Child");
            return true;
          }
          return false;
        } else if constexpr (std::is_same_v<T, syntax::AddressOfExpr>) {
          if (HasNonLocalCtrlExpr(node.place, in_loop)) {
            SPEC_RULE("HasNonLocalCtrl-Child");
            return true;
          }
          return false;
        } else if constexpr (std::is_same_v<T, syntax::MoveExpr>) {
          if (HasNonLocalCtrlExpr(node.place, in_loop)) {
            SPEC_RULE("HasNonLocalCtrl-Child");
            return true;
          }
          return false;
        } else if constexpr (std::is_same_v<T, syntax::AllocExpr>) {
          if (HasNonLocalCtrlExpr(node.value, in_loop)) {
            SPEC_RULE("HasNonLocalCtrl-Child");
            return true;
          }
          return false;
        } else if constexpr (std::is_same_v<T, syntax::TupleExpr>) {
          for (const auto& elem : node.elements) {
            if (HasNonLocalCtrlExpr(elem, in_loop)) {
              SPEC_RULE("HasNonLocalCtrl-Child");
              return true;
            }
          }
          return false;
        } else if constexpr (std::is_same_v<T, syntax::ArrayExpr>) {
          for (const auto& elem : node.elements) {
            if (HasNonLocalCtrlExpr(elem, in_loop)) {
              SPEC_RULE("HasNonLocalCtrl-Child");
              return true;
            }
          }
          return false;
        } else if constexpr (std::is_same_v<T, syntax::RecordExpr>) {
          for (const auto& field : node.fields) {
            if (HasNonLocalCtrlExpr(field.value, in_loop)) {
              SPEC_RULE("HasNonLocalCtrl-Child");
              return true;
            }
          }
          return false;
        } else if constexpr (std::is_same_v<T, syntax::TransmuteExpr>) {
          if (HasNonLocalCtrlExpr(node.value, in_loop)) {
            SPEC_RULE("HasNonLocalCtrl-Child");
            return true;
          }
          return false;
        } else if constexpr (std::is_same_v<T, syntax::FieldAccessExpr>) {
          if (HasNonLocalCtrlExpr(node.base, in_loop)) {
            SPEC_RULE("HasNonLocalCtrl-Child");
            return true;
          }
          return false;
        } else if constexpr (std::is_same_v<T, syntax::TupleAccessExpr>) {
          if (HasNonLocalCtrlExpr(node.base, in_loop)) {
            SPEC_RULE("HasNonLocalCtrl-Child");
            return true;
          }
          return false;
        } else if constexpr (std::is_same_v<T, syntax::IndexAccessExpr>) {
          if (HasNonLocalCtrlExpr(node.base, in_loop) ||
              HasNonLocalCtrlExpr(node.index, in_loop)) {
            SPEC_RULE("HasNonLocalCtrl-Child");
            return true;
          }
          return false;
        } else if constexpr (std::is_same_v<T, syntax::CallExpr>) {
          if (HasNonLocalCtrlExpr(node.callee, in_loop)) {
            SPEC_RULE("HasNonLocalCtrl-Child");
            return true;
          }
          for (const auto& arg : node.args) {
            if (HasNonLocalCtrlExpr(arg.value, in_loop)) {
              SPEC_RULE("HasNonLocalCtrl-Child");
              return true;
            }
          }
          return false;
        } else if constexpr (std::is_same_v<T, syntax::MethodCallExpr>) {
          if (HasNonLocalCtrlExpr(node.receiver, in_loop)) {
            SPEC_RULE("HasNonLocalCtrl-Child");
            return true;
          }
          for (const auto& arg : node.args) {
            if (HasNonLocalCtrlExpr(arg.value, in_loop)) {
              SPEC_RULE("HasNonLocalCtrl-Child");
              return true;
            }
          }
          return false;
        } else if constexpr (std::is_same_v<T, syntax::PropagateExpr>) {
          if (HasNonLocalCtrlExpr(node.value, in_loop)) {
            SPEC_RULE("HasNonLocalCtrl-Child");
            return true;
          }
          return false;
        } else {
          return false;
        }
      },
      expr->node);
}

static bool DeferSafe(const syntax::Block& block) {
  return !HasNonLocalCtrlBlock(block, false);
}

static IntroResult RegionBind(const TypeEnv& env,
                              const std::optional<std::string>& alias_opt) {
  std::string name = alias_opt.has_value() ? *alias_opt : FreshRegionName(env);
  std::vector<std::pair<std::string, TypeRef>> binds;
  binds.emplace_back(name, RegionActiveTypeRef());
  return IntroAll(env, binds, syntax::Mutability::Let, false);
}

static IntroResult FrameBind(const TypeEnv& env,
                             const std::optional<std::string>& target_opt,
                             std::optional<std::string_view>& diag_id) {
  std::string target;
  if (!target_opt.has_value()) {
    const auto inner = InnermostActiveRegion(env);
    if (!inner.has_value()) {
      diag_id = "Frame-NoActiveRegion-Err";
      SPEC_RULE("Frame-NoActiveRegion-Err");
      return {false, diag_id, env};
    }
    target = *inner;
  } else {
    target = *target_opt;
    const auto binding = BindOf(env, target);
    if (!binding.has_value()) {
      diag_id = "ResolveExpr-Ident-Err";
      return {false, diag_id, env};
    }
    if (!RegionActiveType(binding->type)) {
      diag_id = "Frame-Target-NotActive-Err";
      SPEC_RULE("Frame-Target-NotActive-Err");
      return {false, diag_id, env};
    }
  }

  (void)target;
  const std::string fresh = FreshRegionName(env);
  std::vector<std::pair<std::string, TypeRef>> binds;
  binds.emplace_back(fresh, RegionActiveTypeRef());
  return IntroAll(env, binds, syntax::Mutability::Let, false);
}

static syntax::ExprPtr MakeRegionOptsExpr() {
  auto ident = std::make_shared<syntax::Expr>();
  ident->node = syntax::IdentifierExpr{"RegionOptions"};
  syntax::CallExpr call;
  call.callee = ident;
  call.args = {};
  auto expr = std::make_shared<syntax::Expr>();
  expr->node = std::move(call);
  return expr;
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

}  // namespace

TypeEnv PushScope(const TypeEnv& env) {
  SpecDefsTypeStmt();
  TypeEnv out = env;
  out.scopes.emplace_back();
  return out;
}

TypeEnv PopScope(const TypeEnv& env) {
  SpecDefsTypeStmt();
  if (env.scopes.empty()) {
    return env;
  }
  TypeEnv out = env;
  out.scopes.pop_back();
  return out;
}

std::optional<TypeBinding> BindOf(const TypeEnv& env, std::string_view name) {
  SpecDefsTypeStmt();
  const auto key = IdKeyOf(name);
  for (auto it = env.scopes.rbegin(); it != env.scopes.rend(); ++it) {
    const auto found = it->find(key);
    if (found != it->end()) {
      return found->second;
    }
  }
  return std::nullopt;
}

std::optional<syntax::Mutability> MutOf(const TypeEnv& env,
                                        std::string_view name) {
  SpecDefsTypeStmt();
  const auto binding = BindOf(env, name);
  if (!binding.has_value()) {
    return std::nullopt;
  }
  return binding->mut;
}

static PatternTypeResult TypePatternBase(const ScopeContext& ctx,
                                         const syntax::PatternPtr& pattern,
                                         const TypeRef& expected_base) {
  SpecDefsTypeStmt();
  PatternTypeResult result;
  if (!pattern || !expected_base) {
    return result;
  }

  std::vector<IdKey> names;
  CollectPatNames(*pattern, names);
  if (!DistinctNames(names)) {
    SPEC_RULE("Pat-Dup-Err");
    result.diag_id = "Pat-Dup-Err";
    return result;
  }

  return std::visit(
      [&](const auto& node) -> PatternTypeResult {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::WildcardPattern>) {
          SPEC_RULE("Pat-Wildcard");
          return {true, std::nullopt, {}};
        } else if constexpr (std::is_same_v<T, syntax::IdentifierPattern>) {
          SPEC_RULE("Pat-Ident");
          return {true, std::nullopt, {{std::string(node.name), expected_base}}};
        } else if constexpr (std::is_same_v<T, syntax::TuplePattern>) {
          if (node.elements.empty()) {
            if (IsPrimType(expected_base, "()")) {
              SPEC_RULE("Pat-Unit");
              return {true, std::nullopt, {}};
            }
            return {false, std::nullopt, {}};
          }
          const auto* tuple = std::get_if<TypeTuple>(&expected_base->node);
          if (!tuple) {
            return {false, std::nullopt, {}};
          }
          if (tuple->elements.size() != node.elements.size()) {
            SPEC_RULE("Pat-Tuple-Arity-Err");
            return {false, "Pat-Tuple-Arity-Err", {}};
          }
          std::vector<std::pair<std::string, TypeRef>> binds;
          for (std::size_t i = 0; i < node.elements.size(); ++i) {
            const auto sub =
                TypePatternBase(ctx, node.elements[i], tuple->elements[i]);
            if (!sub.ok) {
              return sub;
            }
            binds.insert(binds.end(), sub.bindings.begin(), sub.bindings.end());
          }
          SPEC_RULE("Pat-Tuple");
          return {true, std::nullopt, std::move(binds)};
        } else if constexpr (std::is_same_v<T, syntax::RecordPattern>) {
          const auto* type_path = std::get_if<TypePathType>(&expected_base->node);
          if (!type_path || type_path->path != node.path) {
            return {false, std::nullopt, {}};
          }
          const auto* record = LookupRecordDecl(ctx, node.path);
          if (!record) {
            return {false, std::nullopt, {}};
          }
          std::vector<std::pair<std::string, TypeRef>> binds;
          for (const auto& field : node.fields) {
            const auto* decl = LookupFieldDecl(*record, field.name);
            if (!decl) {
              SPEC_RULE("RecordPattern-UnknownField-Bind");
              return {false, "RecordPattern-UnknownField", {}};
            }
            if (!FieldVisible(ctx.current_module, node.path, *decl)) {
              return {false, std::nullopt, {}};
            }
            const auto lowered = LowerType(ctx, decl->type);
            if (!lowered.ok) {
              return {false, lowered.diag_id, {}};
            }
            syntax::PatternPtr pat = field.pattern_opt;
            if (!pat) {
              auto implicit = std::make_shared<syntax::Pattern>();
              implicit->node = syntax::IdentifierPattern{field.name};
              pat = implicit;
            }
            const auto sub = TypePatternBase(ctx, pat, lowered.type);
            if (!sub.ok) {
              return sub;
            }
            binds.insert(binds.end(), sub.bindings.begin(), sub.bindings.end());
          }
          SPEC_RULE("Pat-Record");
          return {true, std::nullopt, std::move(binds)};
        } else if constexpr (std::is_same_v<T, syntax::LiteralPattern> ||
                             std::is_same_v<T, syntax::TypedPattern> ||
                             std::is_same_v<T, syntax::EnumPattern> ||
                             std::is_same_v<T, syntax::ModalPattern> ||
                             std::is_same_v<T, syntax::RangePattern>) {
          SPEC_RULE("Let-Refutable-Pattern-Err");
          return {false, "Let-Refutable-Pattern-Err", {}};
        } else {
          return {false, std::nullopt, {}};
        }
      },
      pattern->node);
}

PatternTypeResult TypePattern(const ScopeContext& ctx,
                              const syntax::PatternPtr& pattern,
                              const TypeRef& expected) {
  SpecDefsTypeStmt();
  PatternTypeResult result;
  if (!pattern || !expected) {
    return result;
  }

  const auto expected_base = StripPerm(expected);
  if (!expected_base) {
    return result;
  }

  result = TypePatternBase(ctx, pattern, expected_base);
  if (result.ok) {
    if (const auto* perm = std::get_if<TypePerm>(&expected->node)) {
      for (auto& [name, type] : result.bindings) {
        type = MakeTypePerm(perm->perm, type);
      }
      SPEC_RULE("Pat-StripPerm");
    }
  }
  return result;
}

StmtSeqResult TypeStmtSeq(const ScopeContext& ctx,
                          const StmtTypeContext& type_ctx,
                          const std::vector<syntax::Stmt>& stmts,
                          const TypeEnv& env,
                          const ExprTypeFn& type_expr,
                          const IdentTypeFn& type_ident,
                          const PlaceTypeFn& type_place,
                          TypeEnv* env_ref) {
  SpecDefsTypeStmt();
  StmtSeqResult result;
  if (stmts.empty()) {
    SPEC_RULE("StmtSeq-Empty");
    result.ok = true;
    result.env = env;
    result.flow = {};
    if (type_ctx.env_ref) {
      *type_ctx.env_ref = env;
    }
    if (env_ref) {
      *env_ref = env;
    }
    return result;
  }

  TypeEnv current = env;
  if (type_ctx.env_ref) {
    *type_ctx.env_ref = current;
  }
  if (env_ref) {
    *env_ref = current;
  }
  std::vector<TypeRef> res;
  std::vector<TypeRef> brk;
  bool brk_void = false;

  for (const auto& stmt : stmts) {
    if (type_ctx.env_ref) {
      *type_ctx.env_ref = current;
    }
    if (env_ref) {
      *env_ref = current;
    }
    const auto typed = TypeStmt(ctx, type_ctx, stmt, current, type_expr,
                                type_ident, type_place, env_ref);
    if (!typed.ok) {
      result.diag_id = typed.diag_id;
      if (env_ref) {
        *env_ref = current;
      }
      return result;
    }
    current = typed.env;
    if (type_ctx.env_ref) {
      *type_ctx.env_ref = current;
    }
    if (env_ref) {
      *env_ref = current;
    }
    res.insert(res.end(), typed.flow.results.begin(), typed.flow.results.end());
    brk.insert(brk.end(), typed.flow.breaks.begin(), typed.flow.breaks.end());
    brk_void = brk_void || typed.flow.break_void;
    SPEC_RULE("StmtSeq-Cons");
  }

  result.ok = true;
  result.env = std::move(current);
  result.flow.results = std::move(res);
  result.flow.breaks = std::move(brk);
  result.flow.break_void = brk_void;
  if (type_ctx.env_ref) {
    *type_ctx.env_ref = result.env;
  }
  if (env_ref) {
    *env_ref = result.env;
  }
  return result;
}

BlockInfoResult TypeBlockInfo(const ScopeContext& ctx,
                              const StmtTypeContext& type_ctx,
                              const syntax::Block& block,
                              const TypeEnv& env,
                              const ExprTypeFn& type_expr,
                              const IdentTypeFn& type_ident,
                              const PlaceTypeFn& type_place,
                              TypeEnv* env_ref) {
  SpecDefsTypeStmt();
  BlockInfoResult result;
  const TypeEnv pushed = PushScope(env);
  const auto stmts_typed =
      TypeStmtSeq(ctx, type_ctx, block.stmts, pushed, type_expr,
                  type_ident, type_place, env_ref);
  if (!stmts_typed.ok) {
    result.diag_id = stmts_typed.diag_id;
    return result;
  }

  WarnResultUnreachable(ctx, type_ctx, block.stmts);

  const auto res_type = ResType(ctx, stmts_typed.flow.results);
  std::optional<ExprTypeResult> tail_type;
  if (block.tail_opt) {
    if (type_ctx.env_ref) {
      *type_ctx.env_ref = stmts_typed.env;
    }
    if (env_ref) {
      *env_ref = stmts_typed.env;
    }
    const auto typed =
        TypeExprWithEnv(ctx, stmts_typed.env, type_expr, block.tail_opt);
    if (!typed.ok) {
      result.diag_id = typed.diag_id;
      return result;
    }
    tail_type = typed;
  }

  if (res_type.has_value()) {
    SPEC_RULE("BlockInfo-Res");
    result.ok = true;
    result.type = *res_type;
    result.breaks = std::move(stmts_typed.flow.breaks);
    result.break_void = stmts_typed.flow.break_void;
    return result;
  }

  if (!stmts_typed.flow.results.empty()) {
    SPEC_RULE("BlockInfo-Res-Err");
    result.diag_id = "BlockInfo-Res-Err";
    return result;
  }

  if (block.tail_opt) {
    if (!tail_type.has_value()) {
      const auto typed =
          TypeExprWithEnv(ctx, stmts_typed.env, type_expr, block.tail_opt);
      if (!typed.ok) {
        result.diag_id = typed.diag_id;
        return result;
      }
      tail_type = typed;
    }
    SPEC_RULE("BlockInfo-Tail");
    result.ok = true;
    result.type = tail_type->type;
    result.breaks = std::move(stmts_typed.flow.breaks);
    result.break_void = stmts_typed.flow.break_void;
    return result;
  }

  if (!block.stmts.empty() &&
      std::holds_alternative<syntax::ReturnStmt>(block.stmts.back())) {
    SPEC_RULE("BlockInfo-ReturnTail");
    result.ok = true;
    result.type = MakeTypePrim("!");
    result.breaks = std::move(stmts_typed.flow.breaks);
    result.break_void = stmts_typed.flow.break_void;
    return result;
  }

  SPEC_RULE("BlockInfo-Unit");
  result.ok = true;
  result.type = MakeTypePrim("()");
  result.breaks = std::move(stmts_typed.flow.breaks);
  result.break_void = stmts_typed.flow.break_void;
  return result;
}

ExprTypeResult TypeBlock(const ScopeContext& ctx,
                         const StmtTypeContext& type_ctx,
                         const syntax::Block& block,
                         const TypeEnv& env,
                         const ExprTypeFn& type_expr,
                         const IdentTypeFn& type_ident,
                         const PlaceTypeFn& type_place,
                         TypeEnv* env_ref) {
  SpecDefsTypeStmt();
  ExprTypeResult result;
  const auto info = TypeBlockInfo(ctx, type_ctx, block, env, type_expr,
                                  type_ident, type_place, env_ref);
  if (!info.ok) {
    result.diag_id = info.diag_id;
    return result;
  }
  SPEC_RULE("T-Block");
  result.ok = true;
  result.type = info.type;
  return result;
}

CheckResult CheckBlock(const ScopeContext& ctx,
                       const StmtTypeContext& type_ctx,
                       const syntax::Block& block,
                       const TypeEnv& env,
                       const TypeRef& expected,
                       const ExprTypeFn& type_expr,
                       const IdentTypeFn& type_ident,
                       const PlaceTypeFn& type_place,
                       TypeEnv* env_ref) {
  SpecDefsTypeStmt();
  CheckResult result;
  if (!expected) {
    return result;
  }

  const TypeEnv pushed = PushScope(env);
  const auto stmts_typed =
      TypeStmtSeq(ctx, type_ctx, block.stmts, pushed, type_expr,
                  type_ident, type_place, env_ref);
  if (!stmts_typed.ok) {
    result.diag_id = stmts_typed.diag_id;
    return result;
  }

  WarnResultUnreachable(ctx, type_ctx, block.stmts);

  const auto res_type = ResType(ctx, stmts_typed.flow.results);
  if (res_type.has_value()) {
    return result;
  }

  if (block.tail_opt) {
    if (type_ctx.env_ref) {
      *type_ctx.env_ref = stmts_typed.env;
    }
    if (env_ref) {
      *env_ref = stmts_typed.env;
    }
    const auto match_check = MakeMatchCheck(ctx, type_ctx, stmts_typed.env);
    auto place_fn = [&](const syntax::ExprPtr& expr) {
      return TypePlaceWithEnv(stmts_typed.env, type_place, expr);
    };
    const auto check =
        CheckExpr(ctx, block.tail_opt, expected, type_expr, place_fn,
                  IdentTypeFnWithEnv(ctx, stmts_typed.env, type_ident),
                  match_check);
    if (check.ok) {
      SPEC_RULE("Chk-Block-Tail");
      result.ok = true;
      return result;
    }
    result.diag_id = check.diag_id;
    return result;
  }

  if (!block.stmts.empty() &&
      std::holds_alternative<syntax::ReturnStmt>(block.stmts.back())) {
    SPEC_RULE("Chk-Block-Return");
    result.ok = true;
    return result;
  }

  if (IsPrimType(expected, "()")) {
    SPEC_RULE("Chk-Block-Unit");
    result.ok = true;
    return result;
  }
  return result;
}

ExprTypeResult TypeBlockExpr(const ScopeContext& ctx,
                             const StmtTypeContext& type_ctx,
                             const syntax::BlockExpr& expr,
                             const TypeEnv& env,
                             const ExprTypeFn& type_expr,
                             const IdentTypeFn& type_ident,
                             const PlaceTypeFn& type_place) {
  if (type_ctx.require_pure) {
    return {false, "E-SEM-2802", {}};
  }
  if (!expr.block) {
    return {false, std::nullopt, {}};
  }
  return TypeBlock(ctx, type_ctx, *expr.block, env, type_expr,
                   type_ident, type_place, type_ctx.env_ref);
}

CheckResult CheckBlockExpr(const ScopeContext& ctx,
                           const StmtTypeContext& type_ctx,
                           const syntax::BlockExpr& expr,
                           const TypeEnv& env,
                           const TypeRef& expected,
                           const ExprTypeFn& type_expr,
                           const IdentTypeFn& type_ident,
                           const PlaceTypeFn& type_place) {
  if (!expr.block) {
    return {};
  }
  return CheckBlock(ctx, type_ctx, *expr.block, env, expected, type_expr,
                    type_ident, type_place, type_ctx.env_ref);
}

ExprTypeResult TypeUnsafeBlockExpr(const ScopeContext& ctx,
                                   const StmtTypeContext& type_ctx,
                                   const syntax::UnsafeBlockExpr& expr,
                                   const TypeEnv& env,
                                   const ExprTypeFn& type_expr,
                                   const IdentTypeFn& type_ident,
                                   const PlaceTypeFn& type_place) {
  ExprTypeResult result;
  if (type_ctx.require_pure) {
    result.diag_id = "E-SEM-2802";
    return result;
  }
  if (!expr.block) {
    return result;
  }
  StmtTypeContext unsafe_ctx = type_ctx;
  unsafe_ctx.in_unsafe = true;
  const auto block = TypeBlock(ctx, unsafe_ctx, *expr.block, env, type_expr,
                               type_ident, type_place, unsafe_ctx.env_ref);
  if (!block.ok) {
    result.diag_id = block.diag_id;
    return result;
  }
  SPEC_RULE("T-Unsafe-Expr");
  result.ok = true;
  result.type = block.type;
  return result;
}

CheckResult CheckUnsafeBlockExpr(const ScopeContext& ctx,
                                 const StmtTypeContext& type_ctx,
                                 const syntax::UnsafeBlockExpr& expr,
                                 const TypeEnv& env,
                                 const TypeRef& expected,
                                 const ExprTypeFn& type_expr,
                                 const IdentTypeFn& type_ident,
                                 const PlaceTypeFn& type_place) {
  CheckResult result;
  if (!expr.block) {
    return result;
  }
  StmtTypeContext unsafe_ctx = type_ctx;
  unsafe_ctx.in_unsafe = true;
  const auto check = CheckBlock(ctx, unsafe_ctx, *expr.block, env, expected,
                                type_expr, type_ident, type_place,
                                unsafe_ctx.env_ref);
  if (!check.ok) {
    result.diag_id = check.diag_id;
    return result;
  }
  SPEC_RULE("Chk-Unsafe-Expr");
  result.ok = true;
  return result;
}

static std::optional<std::string_view> CheckLoopInvariantPredicate(
    const ScopeContext& ctx,
    const StmtTypeContext& type_ctx,
    const syntax::LoopInvariant& invariant,
    const TypeEnv& env) {
  StmtTypeContext inv_ctx = type_ctx;
  inv_ctx.contract_phase = ContractPhase::Precondition;
  inv_ctx.require_pure = true;
  auto inv_expr = [&](const syntax::ExprPtr& expr) {
    return TypeExpr(ctx, inv_ctx, expr, env);
  };
  auto inv_ident = [&](std::string_view name) -> ExprTypeResult {
    return TypeIdentifierExpr(ctx, syntax::IdentifierExpr{std::string(name)}, env);
  };
  auto inv_place = [&](const syntax::ExprPtr& expr) -> PlaceTypeResult {
    return TypePlace(ctx, inv_ctx, expr, env);
  };
  const auto check = CheckExpr(ctx, invariant.predicate, MakeTypePrim("bool"),
                               inv_expr, inv_place, inv_ident);
  if (!check.ok) {
    return check.diag_id;
  }
  return std::nullopt;
}

ExprTypeResult TypeLoopInfiniteExpr(const ScopeContext& ctx,
                                    const StmtTypeContext& type_ctx,
                                    const syntax::LoopInfiniteExpr& expr,
                                    const TypeEnv& env,
                                    const ExprTypeFn& type_expr,
                                    const IdentTypeFn& type_ident,
                                    const PlaceTypeFn& type_place) {
  ExprTypeResult result;
  if (type_ctx.require_pure) {
    result.diag_id = "E-SEM-2802";
    return result;
  }
  if (expr.invariant_opt) {
    const auto diag =
        CheckLoopInvariantPredicate(ctx, type_ctx, *expr.invariant_opt, env);
    if (diag.has_value()) {
      result.diag_id = diag;
      return result;
    }
  }
  if (!expr.body) {
    return result;
  }
  StmtTypeContext loop_ctx = type_ctx;
  loop_ctx.loop_flag = LoopFlag::Loop;
  auto active_env = [&]() -> const TypeEnv& {
    return loop_ctx.env_ref ? *loop_ctx.env_ref : env;
  };
  auto loop_type_expr = [&](const syntax::ExprPtr& inner) {
    return TypeExpr(ctx, loop_ctx, inner, active_env());
  };
  auto loop_type_ident = [&](std::string_view name) -> ExprTypeResult {
    return TypeIdentifierExpr(ctx, syntax::IdentifierExpr{std::string(name)},
                              active_env());
  };
  auto loop_type_place = [&](const syntax::ExprPtr& inner) -> PlaceTypeResult {
    return TypePlace(ctx, loop_ctx, inner, active_env());
  };
  const auto info = TypeBlockInfo(ctx, loop_ctx, *expr.body, env,
                                  loop_type_expr, loop_type_ident,
                                  loop_type_place, loop_ctx.env_ref);
  if (!info.ok) {
    result.diag_id = info.diag_id;
    return result;
  }
  const auto loop_type = LoopTypeInf(ctx, info.breaks, info.break_void);
  if (!loop_type.has_value()) {
    return result;
  }
  SPEC_RULE("T-Loop-Infinite");
  result.ok = true;
  result.type = *loop_type;
  return result;
}

ExprTypeResult TypeLoopConditionalExpr(const ScopeContext& ctx,
                                       const StmtTypeContext& type_ctx,
                                       const syntax::LoopConditionalExpr& expr,
                                       const TypeEnv& env,
                                       const ExprTypeFn& type_expr,
                                       const IdentTypeFn& type_ident,
                                       const PlaceTypeFn& type_place) {
  ExprTypeResult result;
  if (type_ctx.require_pure) {
    result.diag_id = "E-SEM-2802";
    return result;
  }
  if (expr.invariant_opt) {
    const auto diag =
        CheckLoopInvariantPredicate(ctx, type_ctx, *expr.invariant_opt, env);
    if (diag.has_value()) {
      result.diag_id = diag;
      return result;
    }
  }
  const auto cond_type = TypeExprWithEnv(ctx, env, type_expr, expr.cond);
  if (!cond_type.ok) {
    result.diag_id = cond_type.diag_id;
    return result;
  }
  if (!IsPrimType(cond_type.type, "bool")) {
    return result;
  }

  if (!expr.body) {
    return result;
  }
  StmtTypeContext loop_ctx = type_ctx;
  loop_ctx.loop_flag = LoopFlag::Loop;
  auto active_env = [&]() -> const TypeEnv& {
    return loop_ctx.env_ref ? *loop_ctx.env_ref : env;
  };
  auto loop_type_expr = [&](const syntax::ExprPtr& inner) {
    return TypeExpr(ctx, loop_ctx, inner, active_env());
  };
  auto loop_type_ident = [&](std::string_view name) -> ExprTypeResult {
    return TypeIdentifierExpr(ctx, syntax::IdentifierExpr{std::string(name)},
                              active_env());
  };
  auto loop_type_place = [&](const syntax::ExprPtr& inner) -> PlaceTypeResult {
    return TypePlace(ctx, loop_ctx, inner, active_env());
  };
  const auto info = TypeBlockInfo(ctx, loop_ctx, *expr.body, env,
                                  loop_type_expr, loop_type_ident,
                                  loop_type_place, loop_ctx.env_ref);
  if (!info.ok) {
    result.diag_id = info.diag_id;
    return result;
  }
  const auto loop_type = LoopTypeFin(ctx, info.breaks, info.break_void);
  if (!loop_type.has_value()) {
    return result;
  }
  SPEC_RULE("T-Loop-Conditional");
  result.ok = true;
  result.type = *loop_type;
  return result;
}

ExprTypeResult TypeLoopIterExpr(const ScopeContext& ctx,
                                const StmtTypeContext& type_ctx,
                                const syntax::LoopIterExpr& expr,
                                const TypeEnv& env,
                                const ExprTypeFn& type_expr,
                                const IdentTypeFn& type_ident,
                                const PlaceTypeFn& type_place) {
  ExprTypeResult result;
  if (type_ctx.require_pure) {
    result.diag_id = "E-SEM-2802";
    return result;
  }
  if (expr.invariant_opt) {
    const auto diag =
        CheckLoopInvariantPredicate(ctx, type_ctx, *expr.invariant_opt, env);
    if (diag.has_value()) {
      result.diag_id = diag;
      return result;
    }
  }
  if (!expr.pattern) {
    return result;
  }
  const auto iter_type = TypeExprWithEnv(ctx, env, type_expr, expr.iter);
  if (!iter_type.ok) {
    result.diag_id = iter_type.diag_id;
    return result;
  }

  if (const auto async_iter = AsyncSigOf(ctx, iter_type.type)) {
    const auto async_ret = AsyncSigOf(ctx, type_ctx.return_type);
    if (!async_ret.has_value() || !IsPrimType(async_iter->in, "()")) {
      result.diag_id = "E-CON-0240";
      return result;
    }

    const auto err_sub = Subtyping(ctx, async_iter->err, async_ret->err);
    if (!err_sub.ok) {
      result.diag_id = err_sub.diag_id;
      return result;
    }
    if (!err_sub.subtype) {
      return result;
    }

    TypeRef pat_type = async_iter->out;
    if (expr.type_opt) {
      const auto lowered = LowerType(ctx, expr.type_opt);
      if (!lowered.ok) {
        result.diag_id = lowered.diag_id;
        return result;
      }
      const auto sub = Subtyping(ctx, async_iter->out, lowered.type);
      if (!sub.ok) {
        result.diag_id = sub.diag_id;
        return result;
      }
      if (!sub.subtype) {
        return result;
      }
      pat_type = lowered.type;
    }

    const auto pat = TypePattern(ctx, expr.pattern, pat_type);
    if (!pat.ok) {
      result.diag_id = pat.diag_id;
      return result;
    }
    std::vector<IdKey> names;
    CollectPatNames(*expr.pattern, names);
    if (!DistinctNames(names)) {
      SPEC_RULE("Pat-Dup-Err");
      result.diag_id = "Pat-Dup-Err";
      return result;
    }

    TypeEnv inner = PushScope(env);
    const auto intro = IntroAll(inner, pat.bindings, syntax::Mutability::Let,
                                false);
    if (!intro.ok) {
      result.diag_id = intro.diag_id;
      return result;
    }

    if (!expr.body) {
      return result;
    }
    StmtTypeContext loop_ctx = type_ctx;
    loop_ctx.loop_flag = LoopFlag::Loop;
    auto active_env = [&]() -> const TypeEnv& {
      return loop_ctx.env_ref ? *loop_ctx.env_ref : intro.env;
    };
    auto loop_type_expr = [&](const syntax::ExprPtr& inner_expr) {
      return TypeExpr(ctx, loop_ctx, inner_expr, active_env());
    };
    auto loop_type_ident = [&](std::string_view name) -> ExprTypeResult {
      return TypeIdentifierExpr(ctx, syntax::IdentifierExpr{std::string(name)},
                                active_env());
    };
    auto loop_type_place = [&](const syntax::ExprPtr& inner_expr) -> PlaceTypeResult {
      return TypePlace(ctx, loop_ctx, inner_expr, active_env());
    };
    const auto info = TypeBlockInfo(ctx, loop_ctx, *expr.body, intro.env,
                                    loop_type_expr, loop_type_ident,
                                    loop_type_place, loop_ctx.env_ref);
    if (!info.ok) {
      result.diag_id = info.diag_id;
      return result;
    }
    const auto loop_type = LoopTypeFin(ctx, info.breaks, info.break_void);
    if (!loop_type.has_value()) {
      return result;
    }
    SPEC_RULE("T-Loop-Iter-Async");
    result.ok = true;
    result.type = *loop_type;
    return result;
  }

  const auto elem = IterElementType(iter_type.type);
  if (!elem.has_value()) {
    return result;
  }

  TypeRef pat_type = *elem;
  if (expr.type_opt) {
    const auto lowered = LowerType(ctx, expr.type_opt);
    if (!lowered.ok) {
      result.diag_id = lowered.diag_id;
      return result;
    }
    const auto sub = Subtyping(ctx, *elem, lowered.type);
    if (!sub.ok) {
      result.diag_id = sub.diag_id;
      return result;
    }
    if (!sub.subtype) {
      return result;
    }
    pat_type = lowered.type;
  }

  const auto pat = TypePattern(ctx, expr.pattern, pat_type);
  if (!pat.ok) {
    result.diag_id = pat.diag_id;
    return result;
  }
  std::vector<IdKey> names;
  CollectPatNames(*expr.pattern, names);
  if (!DistinctNames(names)) {
    SPEC_RULE("Pat-Dup-Err");
    result.diag_id = "Pat-Dup-Err";
    return result;
  }

  TypeEnv inner = PushScope(env);
  const auto intro = IntroAll(inner, pat.bindings, syntax::Mutability::Let,
                              false);
  if (!intro.ok) {
    result.diag_id = intro.diag_id;
    return result;
  }

  if (!expr.body) {
    return result;
  }
  StmtTypeContext loop_ctx = type_ctx;
  loop_ctx.loop_flag = LoopFlag::Loop;
  auto active_env = [&]() -> const TypeEnv& {
    return loop_ctx.env_ref ? *loop_ctx.env_ref : intro.env;
  };
  auto loop_type_expr = [&](const syntax::ExprPtr& inner) {
    return TypeExpr(ctx, loop_ctx, inner, active_env());
  };
  auto loop_type_ident = [&](std::string_view name) -> ExprTypeResult {
    return TypeIdentifierExpr(ctx, syntax::IdentifierExpr{std::string(name)},
                              active_env());
  };
  auto loop_type_place = [&](const syntax::ExprPtr& inner) -> PlaceTypeResult {
    return TypePlace(ctx, loop_ctx, inner, active_env());
  };
  const auto info = TypeBlockInfo(ctx, loop_ctx, *expr.body, intro.env,
                                  loop_type_expr, loop_type_ident,
                                  loop_type_place, loop_ctx.env_ref);
  if (!info.ok) {
    result.diag_id = info.diag_id;
    return result;
  }
  const auto loop_type = LoopTypeFin(ctx, info.breaks, info.break_void);
  if (!loop_type.has_value()) {
    return result;
  }
  SPEC_RULE("T-Loop-Iter");
  result.ok = true;
  result.type = *loop_type;
  return result;
}

StmtTypeResult TypeStmt(const ScopeContext& ctx,
                        const StmtTypeContext& type_ctx,
                        const syntax::Stmt& stmt,
                        const TypeEnv& env,
                        const ExprTypeFn& type_expr,
                        const IdentTypeFn& type_ident,
                        const PlaceTypeFn& type_place,
                        TypeEnv* env_ref) {
  SpecDefsTypeStmt();
  auto ident_type = IdentTypeFnWithEnv(ctx, env, type_ident);
  auto match_check = MakeMatchCheck(ctx, type_ctx, env);
  auto place_fn = [&](const syntax::ExprPtr& expr) {
    return TypePlaceWithEnv(env, type_place, expr);
  };

  return std::visit(
      [&](const auto& node) -> StmtTypeResult {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::LetStmt>) {
          const auto& binding = node.binding;
          if (binding.type_opt) {
            const auto ann = LowerType(ctx, binding.type_opt);
            if (!ann.ok) {
              return {false, ann.diag_id, {}, {}};
            }
            const auto check =
                CheckExpr(ctx, binding.init, ann.type, type_expr, place_fn, ident_type, match_check);
            if (!check.ok) {
              if (!check.diag_id.has_value()) {
                SPEC_RULE("T-LetStmt-Ann-Mismatch");
                return {false, "T-LetStmt-Ann-Mismatch", {}, {}};
              }
              return {false, check.diag_id, {}, {}};
            }
            const auto pat = TypePattern(ctx, binding.pat, ann.type);
            if (!pat.ok) {
              return {false, pat.diag_id, {}, {}};
            }
            std::vector<IdKey> names;
            CollectPatNames(*binding.pat, names);
            if (!DistinctNames(names)) {
              SPEC_RULE("Pat-Dup-Err");
              return {false, "Pat-Dup-Err", {}, {}};
            }
            const auto intro = IntroAll(env, pat.bindings,
                                        syntax::Mutability::Let, false);
            if (!intro.ok) {
              return {false, intro.diag_id, {}, {}};
            }
            SPEC_RULE("T-LetStmt-Ann");
            return {true, std::nullopt, std::move(intro.env), {}};
          }

          const auto inferred =
              InferExpr(ctx, binding.init, type_expr, place_fn, ident_type, match_check);
          if (!inferred.ok) {
            if (inferred.diag_id.has_value()) {
              return {false, inferred.diag_id, {}, {}};
            }
            SPEC_RULE("T-LetStmt-Infer-Err");
            return {false, "T-LetStmt-Infer-Err", {}, {}};
          }
          const auto pat = TypePattern(ctx, binding.pat, inferred.type);
          if (!pat.ok) {
            return {false, pat.diag_id, {}, {}};
          }
          std::vector<IdKey> names;
          CollectPatNames(*binding.pat, names);
          if (!DistinctNames(names)) {
            SPEC_RULE("Pat-Dup-Err");
            return {false, "Pat-Dup-Err", {}, {}};
          }
          const auto intro = IntroAll(env, pat.bindings, syntax::Mutability::Let,
                                      false);
          if (!intro.ok) {
            return {false, intro.diag_id, {}, {}};
          }
          SPEC_RULE("T-LetStmt-Infer");
          return {true, std::nullopt, std::move(intro.env), {}};
        } else if constexpr (std::is_same_v<T, syntax::VarStmt>) {
          const auto& binding = node.binding;
          if (binding.type_opt) {
            const auto ann = LowerType(ctx, binding.type_opt);
            if (!ann.ok) {
              return {false, ann.diag_id, {}, {}};
            }
            const auto check =
                CheckExpr(ctx, binding.init, ann.type, type_expr, place_fn, ident_type, match_check);
            if (!check.ok) {
              if (!check.diag_id.has_value()) {
                SPEC_RULE("T-VarStmt-Ann-Mismatch");
                return {false, "T-VarStmt-Ann-Mismatch", {}, {}};
              }
              return {false, check.diag_id, {}, {}};
            }
            const auto pat = TypePattern(ctx, binding.pat, ann.type);
            if (!pat.ok) {
              return {false, pat.diag_id, {}, {}};
            }
            std::vector<IdKey> names;
            CollectPatNames(*binding.pat, names);
            if (!DistinctNames(names)) {
              SPEC_RULE("Pat-Dup-Err");
              return {false, "Pat-Dup-Err", {}, {}};
            }
            const auto intro = IntroAll(env, pat.bindings,
                                        syntax::Mutability::Var, false);
            if (!intro.ok) {
              return {false, intro.diag_id, {}, {}};
            }
            SPEC_RULE("T-VarStmt-Ann");
            return {true, std::nullopt, std::move(intro.env), {}};
          }

          const auto inferred =
              InferExpr(ctx, binding.init, type_expr, place_fn, ident_type, match_check);
          if (!inferred.ok) {
            if (inferred.diag_id.has_value()) {
              return {false, inferred.diag_id, {}, {}};
            }
            SPEC_RULE("T-VarStmt-Infer-Err");
            return {false, "T-VarStmt-Infer-Err", {}, {}};
          }
          const auto pat = TypePattern(ctx, binding.pat, inferred.type);
          if (!pat.ok) {
            return {false, pat.diag_id, {}, {}};
          }
          std::vector<IdKey> names;
          CollectPatNames(*binding.pat, names);
          if (!DistinctNames(names)) {
            SPEC_RULE("Pat-Dup-Err");
            return {false, "Pat-Dup-Err", {}, {}};
          }
          const auto intro = IntroAll(env, pat.bindings, syntax::Mutability::Var,
                                      false);
          if (!intro.ok) {
            return {false, intro.diag_id, {}, {}};
          }
          SPEC_RULE("T-VarStmt-Infer");
          return {true, std::nullopt, std::move(intro.env), {}};
        } else if constexpr (std::is_same_v<T, syntax::ShadowLetStmt>) {
          if (node.type_opt) {
            const auto ann = LowerType(ctx, node.type_opt);
            if (!ann.ok) {
              return {false, ann.diag_id, {}, {}};
            }
            const auto check =
                CheckExpr(ctx, node.init, ann.type, type_expr, place_fn, ident_type, match_check);
            if (!check.ok) {
              if (!check.diag_id.has_value()) {
                SPEC_RULE("T-ShadowLetStmt-Ann-Mismatch");
                return {false, "T-ShadowLetStmt-Ann-Mismatch", {}, {}};
              }
              return {false, check.diag_id, {}, {}};
            }
            std::vector<std::pair<std::string, TypeRef>> binds;
            binds.emplace_back(node.name, ann.type);
            const auto intro =
                IntroAll(env, binds, syntax::Mutability::Let, true);
            if (!intro.ok) {
              return {false, intro.diag_id, {}, {}};
            }
            SPEC_RULE("T-ShadowLetStmt-Ann");
            return {true, std::nullopt, std::move(intro.env), {}};
          }

          const auto inferred = InferExpr(ctx, node.init, type_expr, place_fn, ident_type, match_check);
          if (!inferred.ok) {
            if (inferred.diag_id.has_value()) {
              return {false, inferred.diag_id, {}, {}};
            }
            SPEC_RULE("T-ShadowLetStmt-Infer-Err");
            return {false, "T-ShadowLetStmt-Infer-Err", {}, {}};
          }
          std::vector<std::pair<std::string, TypeRef>> binds;
          binds.emplace_back(node.name, inferred.type);
          const auto intro =
              IntroAll(env, binds, syntax::Mutability::Let, true);
          if (!intro.ok) {
            return {false, intro.diag_id, {}, {}};
          }
          SPEC_RULE("T-ShadowLetStmt-Infer");
          return {true, std::nullopt, std::move(intro.env), {}};
        } else if constexpr (std::is_same_v<T, syntax::ShadowVarStmt>) {
          if (node.type_opt) {
            const auto ann = LowerType(ctx, node.type_opt);
            if (!ann.ok) {
              return {false, ann.diag_id, {}, {}};
            }
            const auto check =
                CheckExpr(ctx, node.init, ann.type, type_expr, place_fn, ident_type, match_check);
            if (!check.ok) {
              if (!check.diag_id.has_value()) {
                SPEC_RULE("T-ShadowVarStmt-Ann-Mismatch");
                return {false, "T-ShadowVarStmt-Ann-Mismatch", {}, {}};
              }
              return {false, check.diag_id, {}, {}};
            }
            std::vector<std::pair<std::string, TypeRef>> binds;
            binds.emplace_back(node.name, ann.type);
            const auto intro =
                IntroAll(env, binds, syntax::Mutability::Var, true);
            if (!intro.ok) {
              return {false, intro.diag_id, {}, {}};
            }
            SPEC_RULE("T-ShadowVarStmt-Ann");
            return {true, std::nullopt, std::move(intro.env), {}};
          }

          const auto inferred = InferExpr(ctx, node.init, type_expr, place_fn, ident_type, match_check);
          if (!inferred.ok) {
            if (inferred.diag_id.has_value()) {
              return {false, inferred.diag_id, {}, {}};
            }
            SPEC_RULE("T-ShadowVarStmt-Infer-Err");
            return {false, "T-ShadowVarStmt-Infer-Err", {}, {}};
          }
          std::vector<std::pair<std::string, TypeRef>> binds;
          binds.emplace_back(node.name, inferred.type);
          const auto intro =
              IntroAll(env, binds, syntax::Mutability::Var, true);
          if (!intro.ok) {
            return {false, intro.diag_id, {}, {}};
          }
          SPEC_RULE("T-ShadowVarStmt-Infer");
          return {true, std::nullopt, std::move(intro.env), {}};
        } else if constexpr (std::is_same_v<T, syntax::AssignStmt>) {
          if (!IsPlaceExpr(node.place)) {
            SPEC_RULE("Assign-NotPlace");
            return {false, "Assign-NotPlace", {}, {}};
          }
          const auto root = PlaceRootName(node.place);
          if (root.has_value()) {
            const auto mut = MutOf(env, *root);
            if (mut.has_value() && *mut == syntax::Mutability::Let) {
              SPEC_RULE("Assign-Immutable-Err");
              return {false, "Assign-Immutable-Err", {}, {}};
            }
          }
          const auto place_type = TypePlaceWithEnv(env, type_place, node.place);
          if (!place_type.ok) {
            return {false, place_type.diag_id, {}, {}};
          }
          if (const auto* perm = std::get_if<TypePerm>(&place_type.type->node)) {
            if (perm->perm == Permission::Const) {
              SPEC_RULE("Assign-Const-Err");
              return {false, "Assign-Const-Err", {}, {}};
            }
          }
          const auto check = CheckExpr(ctx, node.value, place_type.type,
                                       type_expr, place_fn, ident_type, match_check);
          if (!check.ok) {
            if (!check.diag_id.has_value()) {
              SPEC_RULE("Assign-Type-Err");
              return {false, "Assign-Type-Err", {}, {}};
            }
            return {false, check.diag_id, {}, {}};
          }
          SPEC_RULE("T-Assign");
          return {true, std::nullopt, env, {}};
        } else if constexpr (std::is_same_v<T, syntax::CompoundAssignStmt>) {
          if (!IsPlaceExpr(node.place)) {
            SPEC_RULE("Assign-NotPlace");
            return {false, "Assign-NotPlace", {}, {}};
          }
          const auto root = PlaceRootName(node.place);
          if (root.has_value()) {
            const auto mut = MutOf(env, *root);
            if (mut.has_value() && *mut == syntax::Mutability::Let) {
              SPEC_RULE("Assign-Immutable-Err");
              return {false, "Assign-Immutable-Err", {}, {}};
            }
          }
          const auto place_type = TypePlaceWithEnv(env, type_place, node.place);
          if (!place_type.ok) {
            return {false, place_type.diag_id, {}, {}};
          }
          if (const auto* perm = std::get_if<TypePerm>(&place_type.type->node)) {
            if (perm->perm == Permission::Const) {
              SPEC_RULE("Assign-Const-Err");
              return {false, "Assign-Const-Err", {}, {}};
            }
          }
          if (!IsNumericType(place_type.type)) {
            SPEC_RULE("Assign-Type-Err");
            return {false, "Assign-Type-Err", {}, {}};
          }
          const auto rhs_type = TypeExprWithEnv(ctx, env, type_expr, node.value);
          if (!rhs_type.ok) {
            return {false, rhs_type.diag_id, {}, {}};
          }
          const auto sub = Subtyping(ctx, rhs_type.type, StripPerm(place_type.type));
          if (!sub.ok) {
            return {false, sub.diag_id, {}, {}};
          }
          if (!sub.subtype) {
            SPEC_RULE("Assign-Type-Err");
            return {false, "Assign-Type-Err", {}, {}};
          }
          SPEC_RULE("T-CompoundAssign");
          return {true, std::nullopt, env, {}};
        } else if constexpr (std::is_same_v<T, syntax::ExprStmt>) {
          const auto typed = TypeExprWithEnv(ctx, env, type_expr, node.value);
          if (!typed.ok) {
            return {false, typed.diag_id, {}, {}};
          }
          SPEC_RULE("T-ExprStmt");
          return {true, std::nullopt, env, {}};
        } else if constexpr (std::is_same_v<T, syntax::UnsafeBlockStmt>) {
          if (!node.body) {
            return {false, std::nullopt, {}, {}};
          }
          StmtTypeContext unsafe_ctx = type_ctx;
          unsafe_ctx.in_unsafe = true;
          const auto typed = TypeBlock(ctx, unsafe_ctx, *node.body, env, type_expr,
                                       type_ident, type_place,
                                       env_ref ? env_ref : unsafe_ctx.env_ref);
          if (!typed.ok) {
            return {false, typed.diag_id, {}, {}};
          }
          SPEC_RULE("T-UnsafeStmt");
          return {true, std::nullopt, env, {}};
        } else if constexpr (std::is_same_v<T, syntax::DeferStmt>) {
          if (!node.body) {
            return {false, std::nullopt, {}, {}};
          }
          const auto check = CheckBlock(ctx, type_ctx, *node.body, env,
                                        MakeTypePrim("()"), type_expr,
                                        type_ident, type_place,
                                        env_ref ? env_ref : type_ctx.env_ref);
          if (!check.ok) {
            if (!check.diag_id.has_value()) {
              SPEC_RULE("Defer-NonUnit-Err");
              return {false, "Defer-NonUnit-Err", {}, {}};
            }
            return {false, check.diag_id, {}, {}};
          }
          if (!DeferSafe(*node.body)) {
            SPEC_RULE("Defer-NonLocal-Err");
            return {false, "Defer-NonLocal-Err", {}, {}};
          }
          SPEC_RULE("T-DeferStmt");
          return {true, std::nullopt, env, {}};
        } else if constexpr (std::is_same_v<T, syntax::RegionStmt>) {
          const syntax::ExprPtr opts_expr =
              node.opts_opt ? node.opts_opt : MakeRegionOptsExpr();
          const auto check =
              CheckExpr(ctx, opts_expr, MakeTypePath({"RegionOptions"}),
                        type_expr, place_fn, ident_type, match_check);
          if (!check.ok) {
            return {false, check.diag_id, {}, {}};
          }

          const auto intro = RegionBind(env, node.alias_opt);
          if (!intro.ok) {
            return {false, intro.diag_id, {}, {}};
          }

          if (!node.body) {
            return {false, std::nullopt, {}, {}};
          }
          const auto typed = TypeBlock(ctx, type_ctx, *node.body, intro.env,
                                       type_expr, type_ident, type_place,
                                       env_ref ? env_ref : type_ctx.env_ref);
          if (!typed.ok) {
            return {false, typed.diag_id, {}, {}};
          }
          SPEC_RULE("T-RegionStmt");
          return {true, std::nullopt, env, {}};
        } else if constexpr (std::is_same_v<T, syntax::FrameStmt>) {
          std::optional<std::string_view> diag_id;
          const auto intro = FrameBind(env, node.target_opt, diag_id);
          if (!intro.ok) {
            return {false, diag_id, {}, {}};
          }
          if (!node.body) {
            return {false, std::nullopt, {}, {}};
          }
          const auto typed = TypeBlock(ctx, type_ctx, *node.body, intro.env,
                                       type_expr, type_ident, type_place,
                                       env_ref ? env_ref : type_ctx.env_ref);
          if (!typed.ok) {
            return {false, typed.diag_id, {}, {}};
          }
          if (node.target_opt.has_value()) {
            SPEC_RULE("T-FrameStmt-Explicit");
          } else {
            SPEC_RULE("T-FrameStmt-Implicit");
          }
          return {true, std::nullopt, env, {}};
        } else if constexpr (std::is_same_v<T, syntax::ReturnStmt>) {
          const auto verify_post = [&](const syntax::ExprPtr& value)
              -> std::optional<std::string_view> {
            return VerifyPostconditionAtReturn(ctx, type_ctx, value);
          };
          const syntax::ExprPtr ret_value =
              node.value_opt ? node.value_opt : syntax::ExprPtr{};
          const auto async_sig = AsyncSigOf(ctx, type_ctx.return_type);
          if (async_sig.has_value()) {
            if (node.value_opt) {
              const auto check =
                  CheckExpr(ctx, node.value_opt, async_sig->result, type_expr,
                            place_fn, ident_type, match_check);
              if (!check.ok) {
                if (!check.diag_id.has_value()) {
                  SPEC_RULE("Return-Async-Type-Err");
                  return {false, "E-CON-0203", {}, {}};
                }
                return {false, check.diag_id, {}, {}};
              }
              if (const auto diag = verify_post(ret_value); diag.has_value()) {
                return {false, *diag, {}, {}};
              }
              SPEC_RULE("T-Return-Value");
              return {true, std::nullopt, env, {}};
            }
            if (!IsPrimType(async_sig->result, "()")) {
              SPEC_RULE("Return-Async-Unit-Err");
              return {false, "E-CON-0203", {}, {}};
            }
            if (const auto diag = verify_post(ret_value); diag.has_value()) {
              return {false, *diag, {}, {}};
            }
            SPEC_RULE("T-Return-Unit");
            return {true, std::nullopt, env, {}};
          }
          if (node.value_opt) {
            if (type_ctx.opaque_return) {
              const auto typed =
                  TypeExprWithEnv(ctx, env, type_expr, node.value_opt);
              if (!typed.ok) {
                return {false, typed.diag_id, {}, {}};
              }
              if (!TypeImplementsClass(ctx, typed.type,
                                       type_ctx.opaque_return->class_path)) {
                SPEC_RULE("T-Opaque-Return");
                return {false, "E-TYP-2511", {}, {}};
              }
              if (type_ctx.opaque_return->underlying) {
                const auto equiv =
                    TypeEquiv(type_ctx.opaque_return->underlying, typed.type);
                if (!equiv.ok) {
                  return {false, equiv.diag_id, {}, {}};
                }
                if (!equiv.equiv) {
                  SPEC_RULE("T-Opaque-Return");
                  return {false, "Opaque-Type-Mismatch", {}, {}};
                }
              } else {
                type_ctx.opaque_return->underlying = typed.type;
              }
              if (const auto diag = verify_post(ret_value); diag.has_value()) {
                return {false, *diag, {}, {}};
              }
              SPEC_RULE("T-Opaque-Return");
              SPEC_RULE("T-Return-Value");
              return {true, std::nullopt, env, {}};
            }
            const auto check = CheckExpr(ctx, node.value_opt, type_ctx.return_type,
                                         type_expr, place_fn, ident_type, match_check);
            if (!check.ok) {
              if (!check.diag_id.has_value()) {
                SPEC_RULE("Return-Type-Err");
                return {false, "Return-Type-Err", {}, {}};
              }
              return {false, check.diag_id, {}, {}};
            }
            if (const auto diag = verify_post(ret_value); diag.has_value()) {
              return {false, *diag, {}, {}};
            }
            SPEC_RULE("T-Return-Value");
            return {true, std::nullopt, env, {}};
          }
          if (!IsPrimType(type_ctx.return_type, "()")) {
            SPEC_RULE("Return-Unit-Err");
            return {false, "Return-Type-Err", {}, {}};
          }
          if (const auto diag = verify_post(ret_value); diag.has_value()) {
            return {false, *diag, {}, {}};
          }
          SPEC_RULE("T-Return-Unit");
          return {true, std::nullopt, env, {}};
        } else if constexpr (std::is_same_v<T, syntax::ResultStmt>) {
          const auto typed = TypeExprWithEnv(ctx, env, type_expr, node.value);
          if (!typed.ok) {
            return {false, typed.diag_id, {}, {}};
          }
          SPEC_RULE("T-ResultStmt");
          FlowInfo flow;
          flow.results.push_back(typed.type);
          return {true, std::nullopt, env, std::move(flow)};
        } else if constexpr (std::is_same_v<T, syntax::BreakStmt>) {
          if (type_ctx.loop_flag != LoopFlag::Loop) {
            SPEC_RULE("Break-Outside-Loop");
            return {false, "Break-Outside-Loop", {}, {}};
          }
          FlowInfo flow;
          if (node.value_opt) {
            const auto typed = TypeExprWithEnv(ctx, env, type_expr, node.value_opt);
            if (!typed.ok) {
              return {false, typed.diag_id, {}, {}};
            }
            SPEC_RULE("T-Break-Value");
            flow.breaks.push_back(typed.type);
          } else {
            SPEC_RULE("T-Break-Unit");
            flow.break_void = true;
          }
          return {true, std::nullopt, env, std::move(flow)};
        } else if constexpr (std::is_same_v<T, syntax::ContinueStmt>) {
          if (type_ctx.loop_flag != LoopFlag::Loop) {
            SPEC_RULE("Continue-Outside-Loop");
            return {false, "Continue-Outside-Loop", {}, {}};
          }
          SPEC_RULE("T-Continue");
          return {true, std::nullopt, env, {}};
        } else if constexpr (std::is_same_v<T, syntax::KeyBlockStmt>) {
          // C0X Extension: Key block statement type checking
          if (!node.body) {
            return {false, std::nullopt, {}, {}};
          }
          StmtTypeContext key_ctx = type_ctx;
          key_ctx.keys_held = true;
          // Key blocks use the same type checking as regular blocks
          const auto typed = TypeBlock(ctx, key_ctx, *node.body, env, type_expr,
                                       type_ident, type_place,
                                       env_ref ? env_ref : key_ctx.env_ref);
          if (!typed.ok) {
            return {false, typed.diag_id, {}, {}};
          }
          SPEC_RULE("T-KeyBlockStmt");
          return {true, std::nullopt, env, {}};
        } else if constexpr (std::is_same_v<T, syntax::ErrorStmt>) {
          SPEC_RULE("T-ErrorStmt");
          return {true, std::nullopt, env, {}};
        } else {
          return {false, std::nullopt, env, {}};
        }
      },
      stmt);
}

}  // namespace cursive0::analysis
