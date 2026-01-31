#include "cursive0/03_analysis/types/type_expr.h"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <unordered_map>
#include <utility>
#include <vector>

#include "cursive0/00_core/assert_spec.h"
#include "cursive0/00_core/diagnostic_messages.h"
#include "cursive0/03_analysis/composite/arrays_slices.h"
#include "cursive0/03_analysis/composite/classes.h"
#include "cursive0/03_analysis/generics/monomorphize.h"
#include "cursive0/03_analysis/caps/cap_filesystem.h"
#include "cursive0/03_analysis/attributes/attribute_registry.h"
#include "cursive0/03_analysis/caps/cap_heap.h"
#include "cursive0/03_analysis/caps/cap_system.h"
#include "cursive0/03_analysis/caps/cap_concurrency.h"
#include "cursive0/03_analysis/composite/enums.h"
#include "cursive0/03_analysis/composite/function_types.h"
#include "cursive0/03_analysis/contracts/contract_check.h"
#include "cursive0/03_analysis/contracts/verification.h"
#include "cursive0/03_analysis/types/literals.h"
#include "cursive0/03_analysis/modal/modal.h"
#include "cursive0/03_analysis/modal/modal_fields.h"
#include "cursive0/03_analysis/modal/modal_transitions.h"
#include "cursive0/03_analysis/modal/modal_widen.h"
#include "cursive0/03_analysis/composite/records.h"
#include "cursive0/03_analysis/composite/record_methods.h"
#include "cursive0/03_analysis/composite/tuples.h"
#include "cursive0/03_analysis/resolve/scopes.h"
#include "cursive0/03_analysis/resolve/scopes_lookup.h"
#include "cursive0/03_analysis/memory/safe_ptr.h"
#include "cursive0/03_analysis/types/subtyping.h"
#include "cursive0/03_analysis/types/type_equiv.h"
#include "cursive0/03_analysis/types/type_infer.h"
#include "cursive0/03_analysis/types/type_layout.h"
#include "cursive0/03_analysis/types/type_lookup.h"
#include "cursive0/03_analysis/types/type_lower.h"
#include "cursive0/03_analysis/types/type_match.h"
#include "cursive0/03_analysis/types/type_stmt.h"
#include "cursive0/03_analysis/types/type_predicates.h"
#include "cursive0/03_analysis/types/type_wf.h"
#include "cursive0/03_analysis/types/expr/common.h"
#include "cursive0/03_analysis/types/expr/binary.h"
#include "cursive0/03_analysis/types/expr/unary.h"
#include "cursive0/03_analysis/types/expr/cast.h"
#include "cursive0/03_analysis/types/expr/if.h"
#include "cursive0/03_analysis/types/expr/range.h"
#include "cursive0/03_analysis/types/expr/deref.h"
#include "cursive0/03_analysis/types/expr/move.h"
#include "cursive0/03_analysis/types/expr/propagate.h"
#include "cursive0/03_analysis/types/expr/transmute.h"
#include "cursive0/03_analysis/types/expr/addr_of.h"
#include "cursive0/03_analysis/types/expr/field_access.h"
#include "cursive0/03_analysis/types/expr/tuple_access.h"
#include "cursive0/03_analysis/types/expr/alloc.h"
#include "cursive0/03_analysis/types/expr/method_call.h"
#include "cursive0/03_analysis/types/expr/path.h"
#include "cursive0/03_analysis/types/expr/call.h"
#include "cursive0/03_analysis/types/expr/record_literal.h"
#include "cursive0/03_analysis/types/expr/enum_literal.h"
#include "cursive0/03_analysis/types/expr/sizeof.h"
#include "cursive0/03_analysis/types/expr/alignof.h"
#include "cursive0/03_analysis/types/expr/array_repeat.h"
#include "cursive0/03_analysis/memory/regions.h"

namespace cursive0::analysis {

namespace {

// §5.2.12 definitions
static inline void SpecDefsTypeExpr() {
  SPEC_DEF("ExprJudg", "5.2.12");
  SPEC_DEF("UnresolvedExpr", "5.2.12");
  SPEC_DEF("SignedIntTypes", "5.2.12");
  SPEC_DEF("NumericTypes", "5.2.12");
  SPEC_DEF("IntTypes", "5.2.12");
  SPEC_DEF("FloatTypes", "5.2.12");
  SPEC_DEF("ValuePathType", "5.2.12");
  SPEC_DEF("UnsafeSpan", "5.2.12");
  SPEC_DEF("ArithOps", "5.2.12");
  SPEC_DEF("BitOps", "5.2.12");
  SPEC_DEF("ShiftOps", "5.2.12");
  SPEC_DEF("CmpOps", "5.2.12");
  SPEC_DEF("LogicOps", "5.2.12");
  SPEC_DEF("AddrOfOk", "5.2.12");
  SPEC_DEF("SuccessMember", "5.2.12");
  SPEC_DEF("FieldVis", "5.2.12");
  SPEC_DEF("FieldVisible", "5.2.12");
  SPEC_DEF("FieldNames", "5.2.12");
  SPEC_DEF("FieldInitNames", "5.2.12");
  SPEC_DEF("FieldNameSet", "5.2.12");
  SPEC_DEF("FieldInitSet", "5.2.12");
  SPEC_DEF("RegionActiveType", "5.2.17");
  SPEC_DEF("InnermostActiveRegion", "5.2.17");
  SpecDefsSafePtr();
}

// Parse a tuple index from an IntLiteral token
static std::optional<std::size_t> ParseTupleIndex(const syntax::Token& token) {
  if (token.kind != syntax::TokenKind::IntLiteral) {
    return std::nullopt;
  }
  // Simple decimal parsing (tuple indices are always small decimal numbers)
  std::size_t value = 0;
  for (char c : token.lexeme) {
    if (c == '_') continue;
    if (c < '0' || c > '9') return std::nullopt;
    value = value * 10 + static_cast<std::size_t>(c - '0');
  }
  return value;
}

// Forward declarations for mutual recursion
static ExprTypeResult TypeExprImpl(const ScopeContext& ctx,
                                   const StmtTypeContext& type_ctx,
                                   const syntax::ExprPtr& expr,
                                   const TypeEnv& env);

static PlaceTypeResult TypePlaceImpl(const ScopeContext& ctx,
                                     const StmtTypeContext& type_ctx,
                                     const syntax::ExprPtr& expr,
                                     const TypeEnv& env);

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
        } else if constexpr (std::is_same_v<T, syntax::ArrayRepeatExpr>) {
          auto out = node;
          out.value = SubstituteIdent(node.value, name, replacement);
          out.count = SubstituteIdent(node.count, name, replacement);
          return MakeExpr(expr->span, out);
        } else if constexpr (std::is_same_v<T, syntax::SizeofExpr>) {
          // sizeof(type) has no identifier substitution in type argument
          return expr;
        } else if constexpr (std::is_same_v<T, syntax::AlignofExpr>) {
          // alignof(type) has no identifier substitution in type argument
          return expr;
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

static bool ExprUsesOnlyEnvBindings(const syntax::ExprPtr& expr,
                                    const TypeEnv& env) {
  if (!expr) {
    return true;
  }
  if (const auto* ident = std::get_if<syntax::IdentifierExpr>(&expr->node)) {
    return BindOf(env, ident->name).has_value();
  }
  if (std::holds_alternative<syntax::ResultExpr>(expr->node)) {
    return false;
  }
  return std::visit(
      [&](const auto& node) -> bool {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::BinaryExpr>) {
          return ExprUsesOnlyEnvBindings(node.lhs, env) &&
                 ExprUsesOnlyEnvBindings(node.rhs, env);
        } else if constexpr (std::is_same_v<T, syntax::UnaryExpr>) {
          return ExprUsesOnlyEnvBindings(node.value, env);
        } else if constexpr (std::is_same_v<T, syntax::FieldAccessExpr>) {
          return ExprUsesOnlyEnvBindings(node.base, env);
        } else if constexpr (std::is_same_v<T, syntax::TupleAccessExpr>) {
          return ExprUsesOnlyEnvBindings(node.base, env);
        } else if constexpr (std::is_same_v<T, syntax::IndexAccessExpr>) {
          return ExprUsesOnlyEnvBindings(node.base, env) &&
                 ExprUsesOnlyEnvBindings(node.index, env);
        } else if constexpr (std::is_same_v<T, syntax::CallExpr>) {
          if (!ExprUsesOnlyEnvBindings(node.callee, env)) {
            return false;
          }
          for (const auto& arg : node.args) {
            if (!ExprUsesOnlyEnvBindings(arg.value, env)) {
              return false;
            }
          }
          return true;
        } else if constexpr (std::is_same_v<T, syntax::QualifiedApplyExpr>) {
          return false;
        } else if constexpr (std::is_same_v<T, syntax::QualifiedNameExpr>) {
          return false;
        } else if constexpr (std::is_same_v<T, syntax::PathExpr>) {
          return false;
        } else if constexpr (std::is_same_v<T, syntax::MethodCallExpr>) {
          if (!ExprUsesOnlyEnvBindings(node.receiver, env)) {
            return false;
          }
          for (const auto& arg : node.args) {
            if (!ExprUsesOnlyEnvBindings(arg.value, env)) {
              return false;
            }
          }
          return true;
        } else if constexpr (std::is_same_v<T, syntax::CastExpr>) {
          return ExprUsesOnlyEnvBindings(node.value, env);
        } else if constexpr (std::is_same_v<T, syntax::RangeExpr>) {
          return ExprUsesOnlyEnvBindings(node.lhs, env) &&
                 ExprUsesOnlyEnvBindings(node.rhs, env);
        } else if constexpr (std::is_same_v<T, syntax::DerefExpr>) {
          return ExprUsesOnlyEnvBindings(node.value, env);
        } else if constexpr (std::is_same_v<T, syntax::AddressOfExpr>) {
          return ExprUsesOnlyEnvBindings(node.place, env);
        } else if constexpr (std::is_same_v<T, syntax::MoveExpr>) {
          return ExprUsesOnlyEnvBindings(node.place, env);
        } else if constexpr (std::is_same_v<T, syntax::AllocExpr>) {
          return ExprUsesOnlyEnvBindings(node.value, env);
        } else if constexpr (std::is_same_v<T, syntax::TupleExpr>) {
          for (const auto& elem : node.elements) {
            if (!ExprUsesOnlyEnvBindings(elem, env)) {
              return false;
            }
          }
          return true;
        } else if constexpr (std::is_same_v<T, syntax::ArrayExpr>) {
          for (const auto& elem : node.elements) {
            if (!ExprUsesOnlyEnvBindings(elem, env)) {
              return false;
            }
          }
          return true;
        } else if constexpr (std::is_same_v<T, syntax::ArrayRepeatExpr>) {
          return ExprUsesOnlyEnvBindings(node.value, env) &&
                 ExprUsesOnlyEnvBindings(node.count, env);
        } else if constexpr (std::is_same_v<T, syntax::SizeofExpr>) {
          // sizeof(type) references no runtime bindings
          return true;
        } else if constexpr (std::is_same_v<T, syntax::AlignofExpr>) {
          // alignof(type) references no runtime bindings
          return true;
        } else if constexpr (std::is_same_v<T, syntax::RecordExpr>) {
          for (const auto& field : node.fields) {
            if (!ExprUsesOnlyEnvBindings(field.value, env)) {
              return false;
            }
          }
          return true;
        } else if constexpr (std::is_same_v<T, syntax::EnumLiteralExpr>) {
          if (!node.payload_opt.has_value()) {
            return true;
          }
          if (std::holds_alternative<syntax::EnumPayloadParen>(*node.payload_opt)) {
            const auto& paren = std::get<syntax::EnumPayloadParen>(*node.payload_opt);
            for (const auto& elem : paren.elements) {
              if (!ExprUsesOnlyEnvBindings(elem, env)) {
                return false;
              }
            }
            return true;
          }
          const auto& brace = std::get<syntax::EnumPayloadBrace>(*node.payload_opt);
          for (const auto& field : brace.fields) {
            if (!ExprUsesOnlyEnvBindings(field.value, env)) {
              return false;
            }
          }
          return true;
        } else if constexpr (std::is_same_v<T, syntax::IfExpr>) {
          return ExprUsesOnlyEnvBindings(node.cond, env) &&
                 ExprUsesOnlyEnvBindings(node.then_expr, env) &&
                 ExprUsesOnlyEnvBindings(node.else_expr, env);
        } else if constexpr (std::is_same_v<T, syntax::MatchExpr>) {
          if (!ExprUsesOnlyEnvBindings(node.value, env)) {
            return false;
          }
          for (const auto& arm : node.arms) {
            if (!ExprUsesOnlyEnvBindings(arm.guard_opt, env)) {
              return false;
            }
            if (!ExprUsesOnlyEnvBindings(arm.body, env)) {
              return false;
            }
          }
          return true;
        } else if constexpr (std::is_same_v<T, syntax::PropagateExpr>) {
          return ExprUsesOnlyEnvBindings(node.value, env);
        } else if constexpr (std::is_same_v<T, syntax::EntryExpr>) {
          return ExprUsesOnlyEnvBindings(node.expr, env);
        } else {
          return true;
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

}  // namespace

Permission PermOfType(const TypeRef& type) {
  if (!type) {
    return Permission::Const;
  }
  if (const auto* perm = std::get_if<TypePerm>(&type->node)) {
    return perm->perm;
  }
  return Permission::Const;
}

bool IsPrimType(const TypeRef& type, std::string_view name) {
  if (!type) {
    return false;
  }
  const auto* prim = std::get_if<TypePrim>(&type->node);
  return prim && prim->name == name;
}

bool IsCapabilityType(const TypeRef& type) {
  const auto stripped = StripPerm(type);
  if (!stripped) {
    return false;
  }
  const auto* dyn = std::get_if<TypeDynamic>(&stripped->node);
  if (!dyn) {
    return false;
  }
  return IsFileSystemClassPath(dyn->path) ||
         IsHeapAllocatorClassPath(dyn->path) ||
         IsReactorClassPath(dyn->path) ||
         IsExecutionDomainClassPath(dyn->path);
}

bool IsImpureType(const TypeRef& type) {
  if (!type) {
    return false;
  }
  if (PermOfType(type) == Permission::Unique) {
    return true;
  }
  if (IsCapabilityType(type)) {
    return true;
  }
  return false;
}

bool ParamsPure(const ScopeContext& ctx,
                const std::vector<TypeFuncParam>& params) {
  (void)ctx;
  for (const auto& param : params) {
    if (IsImpureType(param.type)) {
      return false;
    }
  }
  return true;
}

bool ParamsPure(const ScopeContext& ctx,
                const std::vector<syntax::Param>& params,
                const std::function<LowerTypeResult(
                    const std::shared_ptr<syntax::Type>&)>& lower_type) {
  for (const auto& param : params) {
    const auto lowered = lower_type(param.type);
    if (!lowered.ok) {
      return true;
    }
    if (IsImpureType(lowered.type)) {
      return false;
    }
  }
  return true;
}

TypeRef SubstSelfType(const TypeRef& self, const TypeRef& type) {
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
            params.push_back(
                TypeFuncParam{param.mode, SubstSelfType(self, param.type)});
          }
          return MakeTypeFunc(std::move(params), SubstSelfType(self, node.ret));
        } else if constexpr (std::is_same_v<T, TypePtr>) {
          return MakeTypePtr(SubstSelfType(self, node.element), node.state);
        } else if constexpr (std::is_same_v<T, TypeRawPtr>) {
          return MakeTypeRawPtr(node.qual, SubstSelfType(self, node.element));
        } else if constexpr (std::is_same_v<T, TypeRefine>) {
          return MakeTypeRefine(SubstSelfType(self, node.base), node.predicate);
        } else if constexpr (std::is_same_v<T, TypeModalState>) {
          std::vector<TypeRef> args;
          args.reserve(node.generic_args.size());
          for (const auto& arg : node.generic_args) {
            args.push_back(SubstSelfType(self, arg));
          }
          return MakeTypeModalState(node.path, node.state, std::move(args));
        } else {
          return type;
        }
      },
      type->node);
}

bool PermSub(Permission lhs, Permission rhs) {
  if (lhs == Permission::Const && rhs == Permission::Const) {
    SPEC_RULE("Perm-Const");
    return true;
  }
  if (lhs == Permission::Unique && rhs == Permission::Unique) {
    SPEC_RULE("Perm-Unique");
    return true;
  }
  if (lhs == Permission::Unique && rhs == Permission::Const) {
    SPEC_RULE("Perm-Unique-Const");
    return true;
  }
  return false;
}

namespace {

// Record field lookup helpers
static syntax::Path FullPath(const syntax::ModulePath& path,
                             std::string_view name) {
  syntax::Path out = path;
  out.emplace_back(name);
  return out;
}

// NOTE: LookupRecordDecl, LookupEnumDecl, FieldExists, FieldVis,
// FieldVisible, and FieldType are now in type_lookup.h/cpp
// NOTE: Operator classification (IsArithOp, IsBitOp, etc.) moved to expr/common.h

// (T-ErrorExpr)
ExprTypeResult TypeErrorExprImpl(const ScopeContext& /*ctx*/,
                                 const syntax::ErrorExpr& /*expr*/) {
  SpecDefsTypeExpr();
  ExprTypeResult result;
  SPEC_RULE("T-ErrorExpr");
  result.ok = true;
  result.type = MakeTypePrim("!");
  return result;
}

// Main expression typing dispatcher
static ExprTypeResult TypeExprImpl(const ScopeContext& ctx,
                                   const StmtTypeContext& type_ctx,
                                   const syntax::ExprPtr& expr,
                                   const TypeEnv& env) {
  SpecDefsTypeExpr();
  ExprTypeResult result;

  if (!expr) {
    return result;
  }

  auto type_expr = [&](const syntax::ExprPtr& inner) {
    return TypeExpr(ctx, type_ctx, inner, env);
  };
  PlaceTypeFn type_place = [&](const syntax::ExprPtr& inner) {
    return TypePlace(ctx, type_ctx, inner, env);
  };
  auto type_ident = [&](std::string_view name) -> ExprTypeResult {
    return expr::TypeIdentifierExprImpl(ctx, syntax::IdentifierExpr{std::string(name)},
                                  env);
  };

  return std::visit(
      [&](const auto& node) -> ExprTypeResult {
        using T = std::decay_t<decltype(node)>;

        if constexpr (std::is_same_v<T, syntax::ErrorExpr>) {
          return TypeErrorExprImpl(ctx, node);
        } else if constexpr (std::is_same_v<T, syntax::AttributedExpr>) {
          StmtTypeContext inner_ctx = type_ctx;
          if (HasAttribute(node.attrs, attrs::kDynamic)) {
            inner_ctx.contract_dynamic = true;
          }
          return TypeExpr(ctx, inner_ctx, node.expr, env);
        } else if constexpr (std::is_same_v<T, syntax::LiteralExpr>) {
          return TypeLiteralExpr(ctx, node);
        } else if constexpr (std::is_same_v<T, syntax::PtrNullExpr>) {
          SPEC_RULE("Syn-PtrNull-Err");
          ExprTypeResult r;
          r.diag_id = "PtrNull-Infer-Err";
          return r;
        } else if constexpr (std::is_same_v<T, syntax::IdentifierExpr>) {
          return expr::TypeIdentifierExprImpl(ctx, node, env);
        } else if constexpr (std::is_same_v<T, syntax::PathExpr>) {
          return expr::TypePathExprImpl(ctx, node);
        } else if constexpr (std::is_same_v<T, syntax::FieldAccessExpr>) {
          return expr::TypeFieldAccessExprImpl(ctx, type_ctx, node, env);
        } else if constexpr (std::is_same_v<T, syntax::TupleAccessExpr>) {
          return expr::TypeTupleAccessExprImpl(ctx, type_ctx, node, env);
        } else if constexpr (std::is_same_v<T, syntax::IndexAccessExpr>) {
          return TypeIndexAccessValue(ctx, node, type_expr, type_ctx.contract_dynamic);
        } else if constexpr (std::is_same_v<T, syntax::BinaryExpr>) {
          return expr::TypeBinaryExprImpl(ctx, type_ctx, node, env);
        } else if constexpr (std::is_same_v<T, syntax::UnaryExpr>) {
          return expr::TypeUnaryExprImpl(ctx, type_ctx, node, env, expr->span);
        } else if constexpr (std::is_same_v<T, syntax::CastExpr>) {
          return expr::TypeCastExprImpl(ctx, type_ctx, node, env);
        } else if constexpr (std::is_same_v<T, syntax::IfExpr>) {
          return expr::TypeIfExprImpl(ctx, type_ctx, node, env);
        } else if constexpr (std::is_same_v<T, syntax::MatchExpr>) {
          return TypeMatchExpr(ctx, type_ctx, node, env);
        } else if constexpr (std::is_same_v<T, syntax::RangeExpr>) {
          return expr::TypeRangeExprImpl(ctx, type_ctx, node, env);
        } else if constexpr (std::is_same_v<T, syntax::AddressOfExpr>) {
          return expr::TypeAddressOfExprImpl(ctx, type_ctx, node, env);
        } else if constexpr (std::is_same_v<T, syntax::DerefExpr>) {
          return expr::TypeDerefExprImpl(ctx, type_ctx, node, env, expr->span);
        } else if constexpr (std::is_same_v<T, syntax::MoveExpr>) {
          return expr::TypeMoveExprImpl(ctx, type_ctx, node, env);
        } else if constexpr (std::is_same_v<T, syntax::PropagateExpr>) {
          return expr::TypePropagateExprImpl(ctx, type_ctx, node, env);
        } else if constexpr (std::is_same_v<T, syntax::ResultExpr>) {
          if (type_ctx.contract_phase != ContractPhase::Postcondition) {
            ExprTypeResult r;
            r.diag_id = "E-SEM-2806";
            return r;
          }
          ExprTypeResult r;
          r.ok = true;
          r.type = type_ctx.return_type ? type_ctx.return_type : MakeTypePrim("()");
          return r;
        } else if constexpr (std::is_same_v<T, syntax::EntryExpr>) {
          if (type_ctx.contract_phase != ContractPhase::Postcondition) {
            ExprTypeResult r;
            r.diag_id = "E-SEM-2852";
            return r;
          }
          if (!node.expr) {
            return ExprTypeResult{};
          }
          if (!ExprUsesOnlyEnvBindings(node.expr, env)) {
            ExprTypeResult r;
            r.diag_id = "E-SEM-2852";
            return r;
          }
          const auto typed = TypeExpr(ctx, type_ctx, node.expr, env);
          if (!typed.ok) {
            ExprTypeResult r;
            r.diag_id = typed.diag_id;
            return r;
          }
          if (!BitcopyType(ctx, typed.type) && !CloneType(ctx, typed.type)) {
            ExprTypeResult r;
            r.diag_id = "E-SEM-2805";
            return r;
          }
          ExprTypeResult r;
          r.ok = true;
          r.type = typed.type;
          return r;
        } else if constexpr (std::is_same_v<T, syntax::RecordExpr>) {
          return expr::TypeRecordExprImpl(ctx, type_ctx, node, env);
        } else if constexpr (std::is_same_v<T, syntax::EnumLiteralExpr>) {
          return expr::TypeEnumLiteralExprImpl(ctx, type_ctx, node, env);
        } else if constexpr (std::is_same_v<T, syntax::TupleExpr>) {
          return TypeTupleExpr(ctx, node, type_expr);
        } else if constexpr (std::is_same_v<T, syntax::ArrayExpr>) {
          return TypeArrayExpr(ctx, node, type_expr);
        } else if constexpr (std::is_same_v<T, syntax::ArrayRepeatExpr>) {
          return expr::TypeArrayRepeatExprImpl(ctx, node, type_expr);
        } else if constexpr (std::is_same_v<T, syntax::SizeofExpr>) {
          return expr::TypeSizeofExprImpl(ctx, node);
        } else if constexpr (std::is_same_v<T, syntax::AlignofExpr>) {
          return expr::TypeAlignofExprImpl(ctx, node);
        } else if constexpr (std::is_same_v<T, syntax::CallExpr>) {
          return expr::TypeCallExprImpl(ctx, type_ctx, node, env);
        } else if constexpr (std::is_same_v<T, syntax::MethodCallExpr>) {
          return expr::TypeMethodCallExprImpl(ctx, type_ctx, node, env, expr->span);
        } else if constexpr (std::is_same_v<T, syntax::BlockExpr>) {
          return TypeBlockExpr(ctx, type_ctx, node, env, type_expr, type_ident,
                               type_place);
        } else if constexpr (std::is_same_v<T, syntax::UnsafeBlockExpr>) {
          return TypeUnsafeBlockExpr(ctx, type_ctx, node, env, type_expr,
                                     type_ident, type_place);
        } else if constexpr (std::is_same_v<T, syntax::LoopInfiniteExpr>) {
          return TypeLoopInfiniteExpr(ctx, type_ctx, node, env, type_expr,
                                      type_ident, type_place);
        } else if constexpr (std::is_same_v<T, syntax::LoopConditionalExpr>) {
          return TypeLoopConditionalExpr(ctx, type_ctx, node, env, type_expr,
                                         type_ident, type_place);
        } else if constexpr (std::is_same_v<T, syntax::LoopIterExpr>) {
          return TypeLoopIterExpr(ctx, type_ctx, node, env, type_expr,
                                  type_ident, type_place);
        } else if constexpr (std::is_same_v<T, syntax::AllocExpr>) {
          return expr::TypeAllocExprImpl(ctx, type_ctx, node, env);
        } else if constexpr (std::is_same_v<T, syntax::TransmuteExpr>) {
          return expr::TypeTransmuteExprImpl(ctx, type_ctx, node, env, expr->span);
        // C0X Extension: Async expressions (§19)
        } else if constexpr (std::is_same_v<T, syntax::YieldExpr>) {
          return TypeYieldExpr(ctx, type_ctx, node, env, type_expr);
        } else if constexpr (std::is_same_v<T, syntax::YieldFromExpr>) {
          return TypeYieldFromExpr(ctx, type_ctx, node, env, type_expr);
        } else if constexpr (std::is_same_v<T, syntax::SyncExpr>) {
          return TypeSyncExpr(ctx, type_ctx, node, env, type_expr);
        } else if constexpr (std::is_same_v<T, syntax::RaceExpr>) {
          return TypeRaceExpr(ctx, type_ctx, node, env, type_expr, type_ident,
                              type_place);
        } else if constexpr (std::is_same_v<T, syntax::AllExpr>) {
          return TypeAllExpr(ctx, type_ctx, node, env, type_expr);
        } else if constexpr (std::is_same_v<T, syntax::QualifiedNameExpr>) {
          SPEC_RULE("Expr-Unresolved-Err");
          ExprTypeResult r;
          r.diag_id = "ResolveExpr-Ident-Err";
          return r;
        } else if constexpr (std::is_same_v<T, syntax::QualifiedApplyExpr>) {
          SPEC_RULE("Expr-Unresolved-Err");
          ExprTypeResult r;
          r.diag_id = "ResolveExpr-Ident-Err";
          return r;
        // C0X Extension: Structured Concurrency (§10, §18)
        } else if constexpr (std::is_same_v<T, syntax::ParallelExpr>) {
          return TypeParallelExpr(ctx, type_ctx, node, env, type_expr,
                                  type_ident, type_place);
        } else if constexpr (std::is_same_v<T, syntax::SpawnExpr>) {
          return TypeSpawnExpr(ctx, type_ctx, node, env, type_expr,
                               type_ident, type_place);
        } else if constexpr (std::is_same_v<T, syntax::WaitExpr>) {
          return TypeWaitExpr(ctx, type_ctx, node, env, type_expr, type_place);
        } else if constexpr (std::is_same_v<T, syntax::DispatchExpr>) {
          return TypeDispatchExpr(ctx, type_ctx, node, env, type_expr,
                                  type_ident, type_place);
        } else {
          return ExprTypeResult{};
        }
      },
      expr->node);
}

// Main place typing dispatcher
static PlaceTypeResult TypePlaceImpl(const ScopeContext& ctx,
                                     const StmtTypeContext& type_ctx,
                                     const syntax::ExprPtr& expr,
                                     const TypeEnv& env) {
  SpecDefsTypeExpr();
  PlaceTypeResult result;
  struct PlaceTypeRecorder {
    const ScopeContext& ctx;
    const syntax::ExprPtr& expr;
    PlaceTypeResult& result;
    ~PlaceTypeRecorder() {
      if (result.ok && ctx.expr_types && expr) {
        (*ctx.expr_types)[expr.get()] = result.type;
      }
    }
  } recorder{ctx, expr, result};

  if (!expr) {
    return result;
  }

  PlaceTypeFn type_place = [&](const syntax::ExprPtr& inner) {
    return TypePlace(ctx, type_ctx, inner, env);
  };
  auto type_expr = [&](const syntax::ExprPtr& inner) {
    return TypeExpr(ctx, type_ctx, inner, env);
  };

  return std::visit(
      [&](const auto& node) -> PlaceTypeResult {
        using T = std::decay_t<decltype(node)>;

        if constexpr (std::is_same_v<T, syntax::IdentifierExpr>) {
          return expr::TypeIdentifierPlaceImpl(ctx, node, env);
        } else if constexpr (std::is_same_v<T, syntax::AttributedExpr>) {
          StmtTypeContext inner_ctx = type_ctx;
          if (HasAttribute(node.attrs, attrs::kDynamic)) {
            inner_ctx.contract_dynamic = true;
          }
          return TypePlace(ctx, inner_ctx, node.expr, env);
        } else if constexpr (std::is_same_v<T, syntax::FieldAccessExpr>) {
          return expr::TypeFieldAccessPlaceImpl(ctx, type_ctx, node, env);
        } else if constexpr (std::is_same_v<T, syntax::TupleAccessExpr>) {
          return expr::TypeTupleAccessPlaceImpl(ctx, type_ctx, node, env);
        } else if constexpr (std::is_same_v<T, syntax::DerefExpr>) {
          return expr::TypeDerefPlaceImpl(ctx, type_ctx, node, env, expr->span);
        } else if constexpr (std::is_same_v<T, syntax::IndexAccessExpr>) {
          return TypeIndexAccessPlace(ctx, node, type_place, type_expr, type_ctx.contract_dynamic);
        } else {
          return PlaceTypeResult{};
        }
      },
      expr->node);
}

}  // namespace

//=============================================================================
// Public API
//=============================================================================

// NOTE: SizeOf and AlignOf are now defined in type_layout.cpp

bool IsPlaceExpr(const syntax::ExprPtr& expr) {
  SpecDefsTypeExpr();
  if (!expr) {
    return false;
  }
  return std::visit(
      [](const auto& node) -> bool {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::IdentifierExpr>) {
          return true;
        } else if constexpr (std::is_same_v<T, syntax::FieldAccessExpr>) {
          return true;
        } else if constexpr (std::is_same_v<T, syntax::TupleAccessExpr>) {
          return true;
        } else if constexpr (std::is_same_v<T, syntax::IndexAccessExpr>) {
          return true;
        } else if constexpr (std::is_same_v<T, syntax::DerefExpr>) {
          return IsPlaceExpr(node.value);
        } else {
          return false;
        }
      },
      expr->node);
}

bool IsInUnsafeSpan(const ScopeContext& ctx, const core::Span& span) {
  SpecDefsTypeExpr();
  if (span.file.empty()) {
    return false;
  }

  const auto range = core::SpanRange(span);
  for (const auto& module : ctx.sigma.mods) {
    for (const auto& file_spans : module.unsafe_spans) {
      if (file_spans.path != span.file) {
        continue;
      }
      for (const auto& sp : file_spans.spans) {
        const auto sp_range = core::SpanRange(sp);
        if (range.first >= sp_range.first && range.second <= sp_range.second) {
          return true;
        }
      }
    }
  }

  return false;
}


ExprTypeResult TypeExpr(const ScopeContext& ctx,
                        const StmtTypeContext& type_ctx,
                        const syntax::ExprPtr& expr,
                        const TypeEnv& env) {
  SpecDefsTypeExpr();
  auto result = TypeExprImpl(ctx, type_ctx, expr, env);
  if (result.ok) {
    if (ctx.expr_types && expr) {
      (*ctx.expr_types)[expr.get()] = result.type;
    }
    SPEC_RULE("Lift-Expr");
  }
  return result;
}

CheckResult CheckExprAgainst(const ScopeContext& ctx,
                             const StmtTypeContext& type_ctx,
                             const syntax::ExprPtr& expr,
                             const TypeRef& expected,
                             const TypeEnv& env) {
  SpecDefsTypeExpr();
  CheckResult result;

  if (!expr || !expected) {
    return result;
  }

  if (const auto* if_expr = std::get_if<syntax::IfExpr>(&expr->node)) {
    return expr::CheckIfExprImpl(ctx, type_ctx, *if_expr, expected, env);
  }

  auto type_expr = [&](const syntax::ExprPtr& inner) {
    return TypeExpr(ctx, type_ctx, inner, env);
  };
  auto type_ident = [&](std::string_view name) -> ExprTypeResult {
    return expr::TypeIdentifierExprImpl(ctx, syntax::IdentifierExpr{std::string(name)},
                                  env);
  };
  PlaceTypeFn type_place = [&](const syntax::ExprPtr& inner) {
    return TypePlace(ctx, type_ctx, inner, env);
  };
  auto match_check = [&](const syntax::MatchExpr& match,
                         const TypeRef& expected_type) -> CheckResult {
    return CheckMatchExpr(ctx, type_ctx, match, env, expected_type);
  };

  const auto check =
      CheckExpr(ctx, expr, expected, type_expr, type_place, type_ident, match_check);
  if (!check.ok) {
    result.diag_id = check.diag_id;
    return result;
  }

  result.ok = true;
  return result;
}


CheckResult CheckPlaceAgainst(const ScopeContext& ctx,
                              const StmtTypeContext& type_ctx,
                              const syntax::ExprPtr& expr,
                              const TypeRef& expected,
                              const TypeEnv& env) {
  SpecDefsTypeExpr();
  CheckResult result;

  if (!expr || !expected) {
    return result;
  }

  const auto place = TypePlace(ctx, type_ctx, expr, env);
  if (!place.ok) {
    result.diag_id = place.diag_id;
    return result;
  }

  const auto sub = Subtyping(ctx, place.type, expected);
  if (!sub.ok) {
    result.diag_id = sub.diag_id;
    return result;
  }
  if (!sub.subtype) {
    return result;
  }

  SPEC_RULE("Place-Check");
  result.ok = true;
  return result;
}

PlaceTypeResult TypePlace(const ScopeContext& ctx,
                          const StmtTypeContext& type_ctx,
                          const syntax::ExprPtr& expr,
                          const TypeEnv& env) {
  auto result = TypePlaceImpl(ctx, type_ctx, expr, env);
  if (result.ok) {
    if (ctx.expr_types && expr) {
      (*ctx.expr_types)[expr.get()] = result.type;
    }
  }
  return result;
}

// Individual expression form handlers (public wrappers)
ExprTypeResult TypeIdentifierExpr(const ScopeContext& ctx,
                                  const syntax::IdentifierExpr& expr,
                                  const TypeEnv& env) {
  return expr::TypeIdentifierExprImpl(ctx, expr, env);
}

ExprTypeResult TypePathExpr(const ScopeContext& ctx,
                            const syntax::PathExpr& expr) {
  return expr::TypePathExprImpl(ctx, expr);
}

ExprTypeResult TypeFieldAccessExpr(const ScopeContext& ctx,
                                   const StmtTypeContext& type_ctx,
                                   const syntax::FieldAccessExpr& expr,
                                   const TypeEnv& env) {
  return expr::TypeFieldAccessExprImpl(ctx, type_ctx, expr, env);
}

PlaceTypeResult TypeFieldAccessPlace(const ScopeContext& ctx,
                                     const StmtTypeContext& type_ctx,
                                     const syntax::FieldAccessExpr& expr,
                                     const TypeEnv& env) {
  return expr::TypeFieldAccessPlaceImpl(ctx, type_ctx, expr, env);
}

ExprTypeResult TypeTupleAccessExpr(const ScopeContext& ctx,
                                   const StmtTypeContext& type_ctx,
                                   const syntax::TupleAccessExpr& expr,
                                   const TypeEnv& env) {
  return expr::TypeTupleAccessExprImpl(ctx, type_ctx, expr, env);
}

PlaceTypeResult TypeTupleAccessPlace(const ScopeContext& ctx,
                                     const StmtTypeContext& type_ctx,
                                     const syntax::TupleAccessExpr& expr,
                                     const TypeEnv& env) {
  return expr::TypeTupleAccessPlaceImpl(ctx, type_ctx, expr, env);
}

ExprTypeResult TypeBinaryExpr(const ScopeContext& ctx,
                              const StmtTypeContext& type_ctx,
                              const syntax::BinaryExpr& expr,
                              const TypeEnv& env) {
  return expr::TypeBinaryExprImpl(ctx, type_ctx, expr, env);
}

ExprTypeResult TypeUnaryExpr(const ScopeContext& ctx,
                             const StmtTypeContext& type_ctx,
                             const syntax::UnaryExpr& expr,
                             const TypeEnv& env,
                             const core::Span& span) {
  return expr::TypeUnaryExprImpl(ctx, type_ctx, expr, env, span);
}

ExprTypeResult TypeCastExpr(const ScopeContext& ctx,
                            const StmtTypeContext& type_ctx,
                            const syntax::CastExpr& expr,
                            const TypeEnv& env) {
  return expr::TypeCastExprImpl(ctx, type_ctx, expr, env);
}

ExprTypeResult TypeIfExpr(const ScopeContext& ctx,
                          const StmtTypeContext& type_ctx,
                          const syntax::IfExpr& expr,
                          const TypeEnv& env) {
  return expr::TypeIfExprImpl(ctx, type_ctx, expr, env);
}

CheckResult CheckIfExpr(const ScopeContext& ctx,
                        const StmtTypeContext& type_ctx,
                        const syntax::IfExpr& expr,
                        const TypeRef& expected,
                        const TypeEnv& env) {
  return expr::CheckIfExprImpl(ctx, type_ctx, expr, expected, env);
}

ExprTypeResult TypeRangeExpr(const ScopeContext& ctx,
                             const StmtTypeContext& type_ctx,
                             const syntax::RangeExpr& expr,
                             const TypeEnv& env) {
  return expr::TypeRangeExprImpl(ctx, type_ctx, expr, env);
}

ExprTypeResult TypeAddressOfExpr(const ScopeContext& ctx,
                                 const StmtTypeContext& type_ctx,
                                 const syntax::AddressOfExpr& expr,
                                 const TypeEnv& env) {
  return expr::TypeAddressOfExprImpl(ctx, type_ctx, expr, env);
}

ExprTypeResult TypeDerefExpr(const ScopeContext& ctx,
                             const StmtTypeContext& type_ctx,
                             const syntax::DerefExpr& expr,
                             const TypeEnv& env) {
  return expr::TypeDerefExprImpl(ctx, type_ctx, expr, env, core::Span{});
}

PlaceTypeResult TypeDerefPlace(const ScopeContext& ctx,
                               const StmtTypeContext& type_ctx,
                               const syntax::DerefExpr& expr,
                               const TypeEnv& env) {
  return expr::TypeDerefPlaceImpl(ctx, type_ctx, expr, env, core::Span{});
}

ExprTypeResult TypeMoveExpr(const ScopeContext& ctx,
                            const StmtTypeContext& type_ctx,
                            const syntax::MoveExpr& expr,
                            const TypeEnv& env) {
  return expr::TypeMoveExprImpl(ctx, type_ctx, expr, env);
}

ExprTypeResult TypeAllocExpr(const ScopeContext& ctx,
                             const StmtTypeContext& type_ctx,
                             const syntax::AllocExpr& expr,
                             const TypeEnv& env) {
  return expr::TypeAllocExprImpl(ctx, type_ctx, expr, env);
}




ExprTypeResult TypeTransmuteExpr(const ScopeContext& ctx,
                                 const StmtTypeContext& type_ctx,
                                 const syntax::TransmuteExpr& expr,
                                 const TypeEnv& env) {
  return expr::TypeTransmuteExprImpl(ctx, type_ctx, expr, env, core::Span{});
}

ExprTypeResult TypePropagateExpr(const ScopeContext& ctx,
                                 const StmtTypeContext& type_ctx,
                                 const syntax::PropagateExpr& expr,
                                 const TypeEnv& env) {
  return expr::TypePropagateExprImpl(ctx, type_ctx, expr, env);
}

ExprTypeResult TypeRecordExpr(const ScopeContext& ctx,
                              const StmtTypeContext& type_ctx,
                              const syntax::RecordExpr& expr,
                              const TypeEnv& env) {
  return expr::TypeRecordExprImpl(ctx, type_ctx, expr, env);
}

ExprTypeResult TypeMethodCallExpr(const ScopeContext& ctx,
                                  const StmtTypeContext& type_ctx,
                                  const syntax::MethodCallExpr& expr,
                                  const TypeEnv& env,
                                  const core::Span& span) {
  return expr::TypeMethodCallExprImpl(ctx, type_ctx, expr, env, span);
}

ExprTypeResult TypeEnumLiteralExpr(const ScopeContext& ctx,
                                   const StmtTypeContext& type_ctx,
                                   const syntax::EnumLiteralExpr& expr,
                                   const TypeEnv& env) {
  return expr::TypeEnumLiteralExprImpl(ctx, type_ctx, expr, env);
}

ExprTypeResult TypeErrorExpr(const ScopeContext& ctx,
                             const syntax::ErrorExpr& expr) {
  return TypeErrorExprImpl(ctx, expr);
}

}  // namespace cursive0::analysis
