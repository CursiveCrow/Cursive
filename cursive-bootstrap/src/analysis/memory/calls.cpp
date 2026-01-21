#include "cursive0/analysis/memory/calls.h"

#include <optional>
#include <string_view>
#include <utility>
#include <vector>

#include "cursive0/core/assert_spec.h"
#include "cursive0/analysis/resolve/scopes.h"
#include "cursive0/analysis/resolve/scopes_lookup.h"
#include "cursive0/analysis/types/subtyping.h"

namespace cursive0::analysis {

namespace {

static inline void SpecDefsCalls() {
  SPEC_DEF("ArgsOkTJudg", "5.2.4");
  SPEC_DEF("ParamMode", "5.2.4");
  SPEC_DEF("ParamType", "5.2.4");
  SPEC_DEF("ArgMoved", "5.2.4");
  SPEC_DEF("ArgExpr", "5.2.4");
  SPEC_DEF("ArgType", "5.2.4");
  SPEC_DEF("MovedArg", "3.3.2.4");
  SPEC_DEF("IsPlace", "3.3.3");
  SPEC_DEF("RecordCallee", "5.2.8");
}

syntax::ExprPtr MakeExpr(const core::Span& span, syntax::ExprNode node) {
  auto expr = std::make_shared<syntax::Expr>();
  expr->span = span;
  expr->node = std::move(node);
  return expr;
}

bool IsPlaceExpr(const syntax::ExprPtr& expr) {
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

syntax::ExprPtr MovedArgExpr(const syntax::Arg& arg) {
  if (!arg.moved || !IsPlaceExpr(arg.value)) {
    return arg.value;
  }
  core::Span span = arg.span;
  if (!span.file.empty()) {
    return MakeExpr(span, syntax::MoveExpr{arg.value});
  }
  if (arg.value) {
    return MakeExpr(arg.value->span, syntax::MoveExpr{arg.value});
  }
  return MakeExpr(core::Span{}, syntax::MoveExpr{arg.value});
}

TypeRef StripPerm(const TypeRef& type) {
  if (!type) {
    return type;
  }
  if (const auto* perm = std::get_if<TypePerm>(&type->node)) {
    return perm->base;
  }
  return type;
}

bool IsPrimType(const TypeRef& type, std::string_view name) {
  if (!type) {
    return false;
  }
  const auto* prim = std::get_if<TypePrim>(&type->node);
  return prim && prim->name == name;
}

struct AddrOfOkResult {
  bool ok = false;
  std::optional<std::string_view> diag_id;
};

AddrOfOkResult AddrOfOk(const syntax::ExprPtr& expr,
                        const ExprTypeFn& type_expr) {
  if (!IsPlaceExpr(expr)) {
    return {false, std::nullopt};
  }
  const auto* index = std::get_if<syntax::IndexAccessExpr>(&expr->node);
  if (!index) {
    return {true, std::nullopt};
  }
  const auto idx_type = type_expr(index->index);
  if (!idx_type.ok) {
    return {false, idx_type.diag_id};
  }
  const auto idx_stripped = StripPerm(idx_type.type);
  if (IsPrimType(idx_stripped, "usize")) {
    return {true, std::nullopt};
  }

  const auto base_type = type_expr(index->base);
  if (!base_type.ok) {
    return {false, base_type.diag_id};
  }
  const auto stripped = StripPerm(base_type.type);
  if (stripped) {
    if (std::holds_alternative<TypeArray>(stripped->node)) {
      return {false, "Index-Array-NonUsize"};
    }
    if (std::holds_alternative<TypeSlice>(stripped->node)) {
      return {false, "Index-Slice-NonUsize"};
    }
  }
  return {false, "Index-NonIndexable"};
}

syntax::Path FullPath(const syntax::ModulePath& path, std::string_view name) {
  syntax::Path out = path;
  out.emplace_back(name);
  return out;
}

const syntax::RecordDecl* LookupRecordDecl(const ScopeContext& ctx,
                                           const syntax::TypePath& path) {
  const auto it = ctx.sigma.types.find(PathKeyOf(path));
  if (it == ctx.sigma.types.end()) {
    return nullptr;
  }
  return std::get_if<syntax::RecordDecl>(&it->second);
}

const syntax::RecordDecl* RecordCalleeDecl(const ScopeContext& ctx,
                                           const syntax::ExprPtr& callee,
                                           const std::vector<syntax::Arg>& args) {
  if (!callee || !args.empty()) {
    return nullptr;
  }
  return std::visit(
      [&](const auto& node) -> const syntax::RecordDecl* {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::IdentifierExpr>) {
          const auto ent = ResolveTypeName(ctx, node.name);
          if (!ent.has_value() || !ent->origin_opt.has_value()) {
            return nullptr;
          }
          const auto name = ent->target_opt.value_or(node.name);
          const auto full = FullPath(*ent->origin_opt, name);
          return LookupRecordDecl(ctx, full);
        } else if constexpr (std::is_same_v<T, syntax::PathExpr>) {
          const auto full = FullPath(node.path, node.name);
          return LookupRecordDecl(ctx, full);
        }
        return nullptr;
      },
      callee->node);
}

}  // namespace

bool IsRecordCallee(const ScopeContext& ctx,
                    const syntax::ExprPtr& callee,
                    const std::vector<syntax::Arg>& args) {
  SpecDefsCalls();
  return RecordCalleeDecl(ctx, callee, args) != nullptr;
}

CallTypeResult TypeCall(const ScopeContext& ctx,
                        const syntax::ExprPtr& callee,
                        const std::vector<syntax::Arg>& args,
                        const ExprTypeFn& type_expr,
                        const PlaceTypeFn* type_place) {
  SpecDefsCalls();
  CallTypeResult result;
  if (!callee) {
    return result;
  }
  const auto callee_type = type_expr(callee);
  if (!callee_type.ok) {
    result.diag_id = callee_type.diag_id;
    return result;
  }

  const auto* func = std::get_if<TypeFunc>(&callee_type.type->node);
  if (!func) {
    if (RecordCalleeDecl(ctx, callee, args)) {
      result.record_callee = true;
      return result;
    }
    SPEC_RULE("Call-Callee-NotFunc");
    result.diag_id = "Call-Callee-NotFunc";
    return result;
  }

  const auto& params = func->params;
  if (params.size() != args.size()) {
    SPEC_RULE("Call-ArgCount-Err");
    result.diag_id = "Call-ArgCount-Err";
    return result;
  }

  for (std::size_t i = 0; i < args.size(); ++i) {
    if (params[i].mode == ParamMode::Move && !args[i].moved) {
      SPEC_RULE("Call-Move-Missing");
      result.diag_id = "Call-Move-Missing";
      return result;
    }
  }

  for (std::size_t i = 0; i < args.size(); ++i) {
    if (!params[i].mode.has_value() && args[i].moved) {
      SPEC_RULE("Call-Move-Unexpected");
      result.diag_id = "Call-Move-Unexpected";
      return result;
    }
  }

  std::vector<TypeRef> arg_types;
  arg_types.reserve(args.size());
  for (std::size_t i = 0; i < args.size(); ++i) {
    const auto& arg = args[i];
    if (!params[i].mode.has_value() && type_place &&
        IsPlaceExpr(arg.value)) {
      const auto place_type = (*type_place)(arg.value);
      if (!place_type.ok) {
        result.diag_id = place_type.diag_id;
        return result;
      }
      arg_types.push_back(place_type.type);
      continue;
    }
    const auto arg_expr = MovedArgExpr(arg);
    const auto arg_type = type_expr(arg_expr);
    if (!arg_type.ok) {
      result.diag_id = arg_type.diag_id;
      return result;
    }
    arg_types.push_back(arg_type.type);
  }

  for (std::size_t i = 0; i < args.size(); ++i) {
    const auto sub = Subtyping(ctx, arg_types[i], params[i].type);
    if (!sub.ok) {
      result.diag_id = sub.diag_id;
      return result;
    }
    if (!sub.subtype) {
      SPEC_RULE("Call-ArgType-Err");
      result.diag_id = "Call-ArgType-Err";
      return result;
    }
  }

  for (std::size_t i = 0; i < args.size(); ++i) {
    if (!params[i].mode.has_value() && !IsPlaceExpr(args[i].value)) {
      SPEC_RULE("Call-Arg-NotPlace");
      result.diag_id = "Call-Arg-NotPlace";
      return result;
    }
  }

  if (params.empty()) {
    SPEC_RULE("ArgsT-Empty");
  } else {
    for (std::size_t i = 0; i < params.size(); ++i) {
      if (params[i].mode == ParamMode::Move) {
        const auto moved = MovedArgExpr(args[i]);
        const auto moved_type = type_expr(moved);
        if (!moved_type.ok) {
          result.diag_id = moved_type.diag_id;
          return result;
        }
        const auto sub = Subtyping(ctx, moved_type.type, params[i].type);
        if (!sub.ok) {
          result.diag_id = sub.diag_id;
          return result;
        }
        if (!sub.subtype) {
          SPEC_RULE("Call-ArgType-Err");
          result.diag_id = "Call-ArgType-Err";
          return result;
        }
        SPEC_RULE("ArgsT-Cons");
        continue;
      }
      const auto addr_ok = AddrOfOk(args[i].value, type_expr);
      if (!addr_ok.ok) {
        result.diag_id = addr_ok.diag_id;
        return result;
      }
      SPEC_RULE("ArgsT-Cons-Ref");
    }
  }

  SPEC_RULE("T-Call");
  result.ok = true;
  result.type = func->ret;
  return result;
}

}  // namespace cursive0::analysis
