#include "cursive0/analysis/types/type_infer.h"

#include <optional>
#include <string_view>
#include <utility>
#include <vector>

#include "cursive0/core/assert_spec.h"
#include "cursive0/analysis/types/literals.h"
#include "cursive0/analysis/composite/arrays_slices.h"
#include "cursive0/analysis/modal/modal.h"
#include "cursive0/analysis/modal/modal_widen.h"
#include "cursive0/analysis/composite/records.h"
#include "cursive0/analysis/composite/record_methods.h"
#include "cursive0/analysis/resolve/scopes.h"
#include "cursive0/analysis/memory/safe_ptr.h"
#include "cursive0/analysis/contracts/verification.h"
#include "cursive0/analysis/types/subtyping.h"
#include "cursive0/analysis/types/type_expr.h"

namespace cursive0::analysis {

namespace {

static inline void SpecDefsTypeInfer() {
  SPEC_DEF("TypeInfJudg", "5.2.9");
  SPEC_DEF("Constraint", "5.2.9");
  SPEC_DEF("ConstraintSet", "5.2.9");
  SPEC_DEF("Solve", "5.2.9");
  SPEC_DEF("StripPerm", "5.2.12");
  SPEC_DEF("UnsafeSpan", "5.2.12");
  SPEC_DEF("PtrNullExpected", "5.2.9");
  SPEC_DEF("NicheCompatible", "5.7");
  SpecDefsSafePtr();
}

static bool PtrNullExpected(const TypeRef& type) {
  if (!type) {
    return false;
  }
  if (const auto* perm = std::get_if<TypePerm>(&type->node)) {
    return PtrNullExpected(perm->base);
  }
  if (const auto* refine = std::get_if<TypeRefine>(&type->node)) {
    return PtrNullExpected(refine->base);
  }
  const auto* ptr = std::get_if<TypePtr>(&type->node);
  if (!ptr) {
    return false;
  }
  if (!ptr->state.has_value()) {
    return true;
  }
  return ptr->state == PtrState::Null;
}

static syntax::ExprPtr MakeExpr(const core::Span& span, syntax::ExprNode node) {
  auto expr = std::make_shared<syntax::Expr>();
  expr->span = span;
  expr->node = std::move(node);
  return expr;
}

static syntax::ExprPtr SubstituteIdent(const syntax::ExprPtr& expr,
                                       std::string_view name,
                                       const syntax::ExprPtr& replacement) {
  if (!expr) {
    return expr;
  }
  if (const auto* ident = std::get_if<syntax::IdentifierExpr>(&expr->node)) {
    if (IdEq(ident->name, name)) {
      return replacement;
    }
    return expr;
  }
  return std::visit(
      [&](const auto& node) -> syntax::ExprPtr {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::BinaryExpr>) {
          auto out = node;
          out.lhs = SubstituteIdent(node.lhs, name, replacement);
          out.rhs = SubstituteIdent(node.rhs, name, replacement);
          return MakeExpr(expr->span, out);
        } else if constexpr (std::is_same_v<T, syntax::UnaryExpr>) {
          auto out = node;
          out.value = SubstituteIdent(node.value, name, replacement);
          return MakeExpr(expr->span, out);
        } else if constexpr (std::is_same_v<T, syntax::FieldAccessExpr>) {
          auto out = node;
          out.base = SubstituteIdent(node.base, name, replacement);
          return MakeExpr(expr->span, out);
        } else if constexpr (std::is_same_v<T, syntax::TupleAccessExpr>) {
          auto out = node;
          out.base = SubstituteIdent(node.base, name, replacement);
          return MakeExpr(expr->span, out);
        } else if constexpr (std::is_same_v<T, syntax::IndexAccessExpr>) {
          auto out = node;
          out.base = SubstituteIdent(node.base, name, replacement);
          out.index = SubstituteIdent(node.index, name, replacement);
          return MakeExpr(expr->span, out);
        } else if constexpr (std::is_same_v<T, syntax::CallExpr>) {
          auto out = node;
          out.callee = SubstituteIdent(node.callee, name, replacement);
          for (auto& arg : out.args) {
            arg.value = SubstituteIdent(arg.value, name, replacement);
          }
          return MakeExpr(expr->span, out);
        } else if constexpr (std::is_same_v<T, syntax::QualifiedApplyExpr>) {
          auto out = node;
          if (std::holds_alternative<syntax::ParenArgs>(node.args)) {
            auto paren = std::get<syntax::ParenArgs>(node.args);
            for (auto& arg : paren.args) {
              arg.value = SubstituteIdent(arg.value, name, replacement);
            }
            out.args = paren;
          } else {
            auto brace = std::get<syntax::BraceArgs>(node.args);
            for (auto& field : brace.fields) {
              field.value = SubstituteIdent(field.value, name, replacement);
            }
            out.args = brace;
          }
          return MakeExpr(expr->span, out);
        } else if constexpr (std::is_same_v<T, syntax::MethodCallExpr>) {
          auto out = node;
          out.receiver = SubstituteIdent(node.receiver, name, replacement);
          for (auto& arg : out.args) {
            arg.value = SubstituteIdent(arg.value, name, replacement);
          }
          return MakeExpr(expr->span, out);
        } else if constexpr (std::is_same_v<T, syntax::CastExpr>) {
          auto out = node;
          out.value = SubstituteIdent(node.value, name, replacement);
          return MakeExpr(expr->span, out);
        } else if constexpr (std::is_same_v<T, syntax::RangeExpr>) {
          auto out = node;
          out.lhs = SubstituteIdent(node.lhs, name, replacement);
          out.rhs = SubstituteIdent(node.rhs, name, replacement);
          return MakeExpr(expr->span, out);
        } else if constexpr (std::is_same_v<T, syntax::DerefExpr>) {
          auto out = node;
          out.value = SubstituteIdent(node.value, name, replacement);
          return MakeExpr(expr->span, out);
        } else if constexpr (std::is_same_v<T, syntax::AddressOfExpr>) {
          auto out = node;
          out.place = SubstituteIdent(node.place, name, replacement);
          return MakeExpr(expr->span, out);
        } else if constexpr (std::is_same_v<T, syntax::MoveExpr>) {
          auto out = node;
          out.place = SubstituteIdent(node.place, name, replacement);
          return MakeExpr(expr->span, out);
        } else if constexpr (std::is_same_v<T, syntax::AllocExpr>) {
          auto out = node;
          out.value = SubstituteIdent(node.value, name, replacement);
          return MakeExpr(expr->span, out);
        } else if constexpr (std::is_same_v<T, syntax::TupleExpr>) {
          auto out = node;
          for (auto& elem : out.elements) {
            elem = SubstituteIdent(elem, name, replacement);
          }
          return MakeExpr(expr->span, out);
        } else if constexpr (std::is_same_v<T, syntax::ArrayExpr>) {
          auto out = node;
          for (auto& elem : out.elements) {
            elem = SubstituteIdent(elem, name, replacement);
          }
          return MakeExpr(expr->span, out);
        } else if constexpr (std::is_same_v<T, syntax::RecordExpr>) {
          auto out = node;
          for (auto& field : out.fields) {
            field.value = SubstituteIdent(field.value, name, replacement);
          }
          return MakeExpr(expr->span, out);
        } else if constexpr (std::is_same_v<T, syntax::EnumLiteralExpr>) {
          auto out = node;
          if (out.payload_opt.has_value()) {
            if (std::holds_alternative<syntax::EnumPayloadParen>(*out.payload_opt)) {
              auto paren = std::get<syntax::EnumPayloadParen>(*out.payload_opt);
              for (auto& elem : paren.elements) {
                elem = SubstituteIdent(elem, name, replacement);
              }
              out.payload_opt = paren;
            } else {
              auto brace = std::get<syntax::EnumPayloadBrace>(*out.payload_opt);
              for (auto& field : brace.fields) {
                field.value = SubstituteIdent(field.value, name, replacement);
              }
              out.payload_opt = brace;
            }
          }
          return MakeExpr(expr->span, out);
        } else if constexpr (std::is_same_v<T, syntax::IfExpr>) {
          auto out = node;
          out.cond = SubstituteIdent(node.cond, name, replacement);
          out.then_expr = SubstituteIdent(node.then_expr, name, replacement);
          out.else_expr = SubstituteIdent(node.else_expr, name, replacement);
          return MakeExpr(expr->span, out);
        } else if constexpr (std::is_same_v<T, syntax::MatchExpr>) {
          auto out = node;
          out.value = SubstituteIdent(node.value, name, replacement);
          for (auto& arm : out.arms) {
            arm.guard_opt = SubstituteIdent(arm.guard_opt, name, replacement);
            arm.body = SubstituteIdent(arm.body, name, replacement);
          }
          return MakeExpr(expr->span, out);
        } else if constexpr (std::is_same_v<T, syntax::PropagateExpr>) {
          auto out = node;
          out.value = SubstituteIdent(node.value, name, replacement);
          return MakeExpr(expr->span, out);
        } else if constexpr (std::is_same_v<T, syntax::EntryExpr>) {
          auto out = node;
          out.expr = SubstituteIdent(node.expr, name, replacement);
          return MakeExpr(expr->span, out);
        } else {
          return expr;
        }
      },
      expr->node);
}

static bool ProveRefinePredicate(const syntax::ExprPtr& value,
                                 const TypeRefine& refine,
                                 std::optional<std::string_view>& diag_id) {
  if (!refine.predicate) {
    return false;
  }
  const auto substituted =
      SubstituteIdent(refine.predicate, "self", value);
  StaticProofContext proof_ctx;
  const auto proof = StaticProof(proof_ctx, substituted);
  if (!proof.provable) {
    diag_id = "E-TYP-1953";
    return false;
  }
  return true;
}

static bool TypePathEq(const TypePath& lhs, const TypePath& rhs) {
  if (lhs.size() != rhs.size()) {
    return false;
  }
  for (std::size_t i = 0; i < lhs.size(); ++i) {
    if (IdKeyOf(lhs[i]) != IdKeyOf(rhs[i])) {
      return false;
    }
  }
  return true;
}

static bool ModalNonNiche(const ScopeContext& ctx,
                          const TypeRef& source,
                          const TypeRef& target) {
  const auto stripped_source = StripPerm(source);
  const auto stripped_target = StripPerm(target);
  if (!stripped_source || !stripped_target) {
    return false;
  }
  const auto* modal = std::get_if<TypeModalState>(&stripped_source->node);
  const auto* path = std::get_if<TypePathType>(&stripped_target->node);
  if (!modal || !path) {
    return false;
  }
  if (!TypePathEq(modal->path, path->path)) {
    return false;
  }
  const auto* decl = LookupModalDecl(ctx, modal->path);
  if (!decl) {
    return false;
  }
  if (!HasState(*decl, modal->state)) {
    return false;
  }
  return !NicheCompatible(ctx, modal->path, modal->state);
}

}  // namespace

static CheckResult CheckExprImpl(const ScopeContext& ctx,
                      const syntax::ExprPtr& expr,
                      const TypeRef& expected,
                      const ExprTypeFn& type_expr,
                      const PlaceTypeFn* type_place,
                      const IdentTypeFn& type_ident,
                      const MatchCheckFn* match_check);

static ExprTypeResult InferExprImpl(const ScopeContext& ctx,
                         const syntax::ExprPtr& expr,
                         const ExprTypeFn& type_expr,
                         const PlaceTypeFn* type_place,
                         const IdentTypeFn& type_ident,
                         const MatchCheckFn* match_check) {
  SpecDefsTypeInfer();
  ExprTypeResult result;
  struct ExprTypeRecorder {
    const ScopeContext& ctx;
    const syntax::ExprPtr& expr;
    ExprTypeResult& result;
    ~ExprTypeRecorder() {
      if (result.ok && ctx.expr_types && expr) {
        (*ctx.expr_types)[expr.get()] = result.type;
      }
    }
  } recorder{ctx, expr, result};
  if (!expr) {
    return result;
  }

  if (std::holds_alternative<syntax::PtrNullExpr>(expr->node)) {
    SPEC_RULE("Syn-PtrNull-Err");
    result.diag_id = "PtrNull-Infer-Err";
    return result;
  }


  if (const auto* literal = std::get_if<syntax::LiteralExpr>(&expr->node)) {
    if (literal->literal.kind == syntax::TokenKind::NullLiteral) {
      SPEC_RULE("Syn-Null-Literal-Err");
      result.diag_id = "NullLiteral-Infer-Err";
      return result;
    }
    const auto typed = TypeLiteralExpr(ctx, *literal);
    if (!typed.ok) {
      result.diag_id = typed.diag_id;
      return result;
    }
    SPEC_RULE("Syn-Literal");
    result.ok = true;
    result.type = typed.type;
    return result;
  }

  if (const auto* ident = std::get_if<syntax::IdentifierExpr>(&expr->node)) {
    const auto ident_type = type_ident(ident->name);
    if (!ident_type.ok) {
      result.diag_id = ident_type.diag_id;
      return result;
    }
    SPEC_RULE("Syn-Ident");
    result.ok = true;
    result.type = ident_type.type;
    return result;
  }

  if (const auto* tuple = std::get_if<syntax::TupleExpr>(&expr->node)) {
    if (tuple->elements.empty()) {
      SPEC_RULE("Syn-Unit");
      result.ok = true;
      result.type = MakeTypePrim("()");
      return result;
    }
    std::vector<TypeRef> elements;
    elements.reserve(tuple->elements.size());
    for (const auto& elem : tuple->elements) {
      const auto elem_type =
          InferExprImpl(ctx, elem, type_expr, type_place, type_ident, match_check);
      if (!elem_type.ok) {
        result.diag_id = elem_type.diag_id;
        return result;
      }
      elements.push_back(elem_type.type);
    }
    SPEC_RULE("Syn-Tuple");
    result.ok = true;
    result.type = MakeTypeTuple(std::move(elements));
    return result;
  }

  if (const auto* call = std::get_if<syntax::CallExpr>(&expr->node)) {
    if (!call->callee) {
      return result;
    }
    if (call->args.empty()) {
      if (const auto* ident =
              std::get_if<syntax::IdentifierExpr>(&call->callee->node);
          ident && IdEq(ident->name, "RegionOptions")) {
        SPEC_RULE("T-Record-Default");
        result.ok = true;
        result.type = MakeTypePath({"RegionOptions"});
        return result;
      }
    }
    const auto record =
        TypeRecordDefaultCall(ctx, call->callee, call->args, type_expr);
    if (record.ok) {
      result.ok = true;
      result.type = record.type;
      return result;
    }
    if (record.diag_id.has_value()) {
      result.diag_id = record.diag_id;
      return result;
    }
    const auto call_type =
        TypeCall(ctx, call->callee, call->args, type_expr, type_place);
    if (!call_type.ok) {
      if (call_type.diag_id.has_value()) {
        SPEC_RULE("Syn-Call-Err");
        result.diag_id = call_type.diag_id;
      }
      return result;
    }
    SPEC_RULE("Syn-Call");
    result.ok = true;
    result.type = call_type.type;
    return result;
  }

  const auto fallback = type_expr(expr);
  if (!fallback.ok) {
    result.diag_id = fallback.diag_id;
    return result;
  }
  SPEC_RULE("Syn-Expr");
  result.ok = true;
  result.type = fallback.type;
  return result;
}

static CheckResult CheckExprImpl(const ScopeContext& ctx,
                      const syntax::ExprPtr& expr,
                      const TypeRef& expected,
                      const ExprTypeFn& type_expr,
                      const PlaceTypeFn* type_place,
                      const IdentTypeFn& type_ident,
                      const MatchCheckFn* match_check) {
  SpecDefsTypeInfer();
  CheckResult result;
  struct CheckExprRecorder {
    const ScopeContext& ctx;
    const syntax::ExprPtr& expr;
    const TypeRef& expected;
    CheckResult& result;
    ~CheckExprRecorder() {
      if (result.ok && ctx.expr_types && expr && expected) {
        (*ctx.expr_types)[expr.get()] = expected;
      }
    }
  } recorder{ctx, expr, expected, result};
  if (!expr || !expected) {
    return result;
  }

  if (match_check) {
    if (const auto* match = std::get_if<syntax::MatchExpr>(&expr->node)) {
      result = (*match_check)(*match, expected);
      return result;
    }
  }

  if (std::holds_alternative<syntax::PtrNullExpr>(expr->node)) {
    if (PtrNullExpected(expected)) {
      SPEC_RULE("Chk-Null-Ptr");
      result.ok = true;
      return result;
    }
    SPEC_RULE("Chk-PtrNull-Err");
    result.diag_id = "PtrNull-Infer-Err";
    return result;
  }

  if (const auto* literal = std::get_if<syntax::LiteralExpr>(&expr->node)) {
    const auto check = CheckLiteralExpr(ctx, *literal, expected);
    if (check.ok) {
      result.ok = true;
      return result;
    }
    if (check.diag_id.has_value()) {
      result.diag_id = check.diag_id;
      return result;
    }
  }

  if (const auto* method = std::get_if<syntax::MethodCallExpr>(&expr->node)) {
    if (IdEq(method->name, "alloc_raw")) {
      const auto expected_strip = StripPerm(expected);
      const auto* expected_raw = expected_strip
                                    ? std::get_if<TypeRawPtr>(&expected_strip->node)
                                    : nullptr;
      if (expected_raw && expected_raw->qual == RawPtrQual::Mut) {
        auto recv_type = type_expr(method->receiver);
        if (!recv_type.ok && recv_type.diag_id.has_value() &&
            *recv_type.diag_id == "ValueUse-NonBitcopyPlace") {
          auto move_expr = std::make_shared<syntax::Expr>();
          move_expr->span = method->receiver ? method->receiver->span : core::Span{};
          move_expr->node = syntax::MoveExpr{method->receiver};
          recv_type = type_expr(move_expr);
        }
        if (!recv_type.ok) {
          result.diag_id = recv_type.diag_id;
          return result;
        }
        const auto recv_strip = StripPerm(recv_type.type);
        const auto* recv_dyn = recv_strip
                                   ? std::get_if<TypeDynamic>(&recv_strip->node)
                                   : nullptr;
        if (recv_dyn && recv_dyn->path.size() == 1 &&
            IdEq(recv_dyn->path[0], "HeapAllocator")) {
          if (!IsInUnsafeSpan(ctx, expr->span)) {
            SPEC_RULE("AllocRaw-Unsafe-Err");
            result.diag_id = "AllocRaw-Unsafe-Err";
            return result;
          }
          const auto required =
              MakeTypePerm(Permission::Const, MakeTypeDynamic({"HeapAllocator"}));
          const auto recv_sub = Subtyping(ctx, recv_type.type, required);
          if (!recv_sub.ok) {
            result.diag_id = recv_sub.diag_id;
            return result;
          }
          if (!recv_sub.subtype) {
            SPEC_RULE("MethodCall-RecvPerm-Err");
            result.diag_id = "MethodCall-RecvPerm-Err";
            return result;
          }
          const auto recv_arg =
              RecvArgOk(method->receiver, std::nullopt, type_expr);
          if (!recv_arg.ok) {
            result.diag_id = recv_arg.diag_id;
            return result;
          }
          if (method->args.size() != 1) {
            SPEC_RULE("Call-ArgCount-Err");
            result.diag_id = "Call-ArgCount-Err";
            return result;
          }
          const auto& arg = method->args[0];
          if (arg.moved) {
            SPEC_RULE("Call-Move-Unexpected");
            result.diag_id = "Call-Move-Unexpected";
            return result;
          }
          if (!IsPlaceExpr(arg.value)) {
            SPEC_RULE("Call-Arg-NotPlace");
            result.diag_id = "Call-Arg-NotPlace";
            return result;
          }
          const auto arg_type = type_expr(arg.value);
          if (!arg_type.ok) {
            result.diag_id = arg_type.diag_id;
            return result;
          }
          const auto count_sub =
              Subtyping(ctx, arg_type.type, MakeTypePrim("usize"));
          if (!count_sub.ok) {
            result.diag_id = count_sub.diag_id;
            return result;
          }
          if (!count_sub.subtype) {
            SPEC_RULE("Call-ArgType-Err");
            result.diag_id = "Call-ArgType-Err";
            return result;
          }
          SPEC_RULE("Chk-Subsumption");
          result.ok = true;
          return result;
        }
      }
    }
  }

  const auto inferred =
      InferExprImpl(ctx, expr, type_expr, type_place, type_ident, match_check);
  if (!inferred.ok) {
    result.diag_id = inferred.diag_id;
    return result;
  }

  if (ModalNonNiche(ctx, inferred.type, expected)) {
    SPEC_RULE("Chk-Subsumption-Modal-NonNiche");
    result.diag_id = "Chk-Subsumption-Modal-NonNiche";
    return result;
  }

  auto try_coerce = [&](const TypeRef& source) -> bool {
    const auto coerced = CoerceArrayToSlice(ctx, source);
    if (!coerced.ok) {
      return false;
    }
    const auto sub = Subtyping(ctx, coerced.type, expected);
    if (!sub.ok) {
      result.diag_id = sub.diag_id;
      return true;
    }
    if (sub.subtype) {
      result.ok = true;
      return true;
    }
    return false;
  };

  if (try_coerce(inferred.type)) {
    return result;
  }
  if (std::holds_alternative<TypeArray>(inferred.type->node)) {
    const auto perm_wrapped =
        MakeTypePerm(Permission::Const, inferred.type);
    if (try_coerce(perm_wrapped)) {
      return result;
    }
  }

  const auto sub = Subtyping(ctx, inferred.type, expected);
  if (!sub.ok) {
    result.diag_id = sub.diag_id;
    return result;
  }
  if (!sub.subtype) {
    if (const auto* refine = std::get_if<TypeRefine>(&expected->node)) {
      const auto base_check =
          CheckExprImpl(ctx, expr, refine->base, type_expr, type_place,
                        type_ident, match_check);
      if (!base_check.ok) {
        result.diag_id = base_check.diag_id;
        return result;
      }
      if (!ProveRefinePredicate(expr, *refine, result.diag_id)) {
        return result;
      }
      SPEC_RULE("T-Refine-Intro");
      result.ok = true;
      return result;
    }
    return result;
  }

  SPEC_RULE("Chk-Subsumption");
  result.ok = true;
  return result;
}

ExprTypeResult InferExpr(const ScopeContext& ctx,
                         const syntax::ExprPtr& expr,
                         const ExprTypeFn& type_expr,
                         const PlaceTypeFn& type_place,
                         const IdentTypeFn& type_ident) {
  return InferExprImpl(ctx, expr, type_expr, &type_place, type_ident, nullptr);
}

ExprTypeResult InferExpr(const ScopeContext& ctx,
                         const syntax::ExprPtr& expr,
                         const ExprTypeFn& type_expr,
                         const PlaceTypeFn& type_place,
                         const IdentTypeFn& type_ident,
                         const MatchCheckFn& match_check) {
  return InferExprImpl(ctx, expr, type_expr, &type_place, type_ident,
                       &match_check);
}

CheckResult CheckExpr(const ScopeContext& ctx,
                      const syntax::ExprPtr& expr,
                      const TypeRef& expected,
                      const ExprTypeFn& type_expr,
                      const PlaceTypeFn& type_place,
                      const IdentTypeFn& type_ident) {
  return CheckExprImpl(ctx, expr, expected, type_expr, &type_place, type_ident,
                       nullptr);
}

CheckResult CheckExpr(const ScopeContext& ctx,
                      const syntax::ExprPtr& expr,
                      const TypeRef& expected,
                      const ExprTypeFn& type_expr,
                      const PlaceTypeFn& type_place,
                      const IdentTypeFn& type_ident,
                      const MatchCheckFn& match_check) {
  return CheckExprImpl(ctx, expr, expected, type_expr, &type_place, type_ident,
                       &match_check);
}

}  // namespace cursive0::analysis
