// =================================================================
// File: 04_analysis/typing/expr/binary.cpp
// Construct: Binary Expression Type Checking
// Spec Section: 5.2.12
// Spec Rules: T-Arith, T-Bitwise, T-Shift, T-Compare-Eq, T-Compare-Ord, T-Logical
// =================================================================

#include "04_analysis/typing/expr/binary.h"

#include "00_core/assert_spec.h"
#include "04_analysis/generics/monomorphize.h"
#include "04_analysis/resolve/scopes_lookup.h"
#include "04_analysis/typing/expr/expr_common.h"
#include "04_analysis/typing/subtyping.h"
#include "04_analysis/typing/type_equiv.h"
#include "04_analysis/typing/type_expr.h"
#include "04_analysis/typing/type_lower.h"
#include "04_analysis/typing/type_predicates.h"
#include "04_analysis/typing/typecheck.h"
#include "04_analysis/typing/types.h"

namespace cursive::analysis::expr {

namespace {

static inline void SpecDefsBinary() {
  SPEC_DEF("T-Arith", "5.2.12");
  SPEC_DEF("T-Bitwise", "5.2.12");
  SPEC_DEF("T-Shift", "5.2.12");
  SPEC_DEF("T-Compare-Eq", "5.2.12");
  SPEC_DEF("T-Compare-Ord", "5.2.12");
  SPEC_DEF("T-Logical", "5.2.12");
}

struct PrimResolveResult {
  bool ok = true;
  std::optional<std::string_view> diag_id;
  std::optional<std::string> prim;
};

static PrimResolveResult ResolveAliasTransparentPrim(const ScopeContext& ctx,
                                                     const TypeRef& type) {
  PrimResolveResult result;
  if (!type) {
    return result;
  }

  if (const auto direct = GetPrimName(type); direct.has_value()) {
    result.prim = *direct;
    return result;
  }

  static constexpr std::array<std::string_view, 18> kPrimCandidates = {
      "i8",   "i16",   "i32",   "i64",   "i128", "isize",
      "u8",   "u16",   "u32",   "u64",   "u128", "usize",
      "f16",  "f32",   "f64",   "bool",  "char", "()"};

  for (const auto candidate : kPrimCandidates) {
    const auto prim = MakeTypePrim(std::string(candidate));
    const auto l2r = Subtyping(ctx, type, prim);
    if (!l2r.ok) {
      result.ok = false;
      result.diag_id = l2r.diag_id;
      return result;
    }
    if (!l2r.subtype) {
      continue;
    }

    const auto r2l = Subtyping(ctx, prim, type);
    if (!r2l.ok) {
      result.ok = false;
      result.diag_id = r2l.diag_id;
      return result;
    }
    if (r2l.subtype) {
      result.prim = std::string(candidate);
      return result;
    }
  }

  return result;
}

static SubtypingResult AliasTransparentEquiv(const ScopeContext& ctx,
                                             const TypeRef& lhs,
                                             const TypeRef& rhs) {
  const auto l2r = Subtyping(ctx, lhs, rhs);
  if (!l2r.ok) {
    return {false, l2r.diag_id, false};
  }
  if (!l2r.subtype) {
    return {true, std::nullopt, false};
  }

  const auto r2l = Subtyping(ctx, rhs, lhs);
  if (!r2l.ok) {
    return {false, r2l.diag_id, false};
  }
  if (!r2l.subtype) {
    return {true, std::nullopt, false};
  }

  return {true, std::nullopt, true};
}

static SubtypingResult AliasTransparentPrimEq(const ScopeContext& ctx,
                                              const TypeRef& type,
                                              std::string_view prim_name) {
  return AliasTransparentEquiv(ctx, type, MakeTypePrim(std::string(prim_name)));
}

static bool IsOrdPrim(std::string_view prim_name) {
  return prim_name == "i8" || prim_name == "i16" ||
         prim_name == "i32" || prim_name == "i64" ||
         prim_name == "i128" || prim_name == "isize" ||
         prim_name == "u8" || prim_name == "u16" ||
         prim_name == "u32" || prim_name == "u64" ||
         prim_name == "u128" || prim_name == "usize" ||
         prim_name == "f16" || prim_name == "f32" ||
         prim_name == "f64" || prim_name == "char";
}

static bool IsNullLiteralExpr(const ast::ExprPtr& expr) {
  if (!expr) {
    return false;
  }
  const auto* literal = std::get_if<ast::LiteralExpr>(&expr->node);
  return literal && literal->literal.kind == lexer::TokenKind::NullLiteral;
}

static TypeRef StripPermAndRefine(const TypeRef& type) {
  TypeRef cur = type;
  while (cur) {
    if (const auto* perm = std::get_if<TypePerm>(&cur->node)) {
      cur = perm->base;
      continue;
    }
    if (const auto* refine = std::get_if<TypeRefine>(&cur->node)) {
      cur = refine->base;
      continue;
    }
    break;
  }
  return cur;
}

struct AliasExpandResult {
  bool ok = true;
  std::optional<std::string_view> diag_id;
  TypeRef type = nullptr;
  bool expanded = false;
};

static const ast::TypeAliasDecl* LookupTypeAliasDecl(const ScopeContext& ctx,
                                                     const TypePath& path) {
  if (path.empty()) {
    return nullptr;
  }
  if (path.size() > 1) {
    ast::Path full;
    full.reserve(path.size());
    for (const auto& seg : path) {
      full.push_back(seg);
    }
    const auto it = ctx.sigma.types.find(PathKeyOf(full));
    if (it == ctx.sigma.types.end()) {
      return nullptr;
    }
    return std::get_if<ast::TypeAliasDecl>(&it->second);
  }

  const auto ent = ResolveTypeName(ctx, path[0]);
  if (!ent.has_value() || !ent->origin_opt.has_value()) {
    return nullptr;
  }

  ast::Path resolved = *ent->origin_opt;
  const std::string resolved_name =
      ent->target_opt.has_value() ? *ent->target_opt : path[0];
  resolved.push_back(resolved_name);
  const auto resolved_it = ctx.sigma.types.find(PathKeyOf(resolved));
  if (resolved_it == ctx.sigma.types.end()) {
    return nullptr;
  }
  return std::get_if<ast::TypeAliasDecl>(&resolved_it->second);
}

static AliasExpandResult ExpandTypeAliasApply(const ScopeContext& ctx,
                                              const TypePathType& applied) {
  AliasExpandResult result;
  const auto* alias = LookupTypeAliasDecl(ctx, applied.path);
  if (!alias) {
    return result;
  }

  const auto lowered = LowerType(ctx, alias->type);
  if (!lowered.ok) {
    result.ok = false;
    result.diag_id = lowered.diag_id;
    return result;
  }

  if (!alias->generic_params.has_value()) {
    if (!applied.generic_args.empty()) {
      return result;
    }
    result.type = lowered.type;
    result.expanded = true;
    return result;
  }

  const auto& params = alias->generic_params->params;
  if (applied.generic_args.size() > params.size()) {
    return result;
  }

  const auto subst = BuildSubstitution(params, applied.generic_args);
  result.type = InstantiateType(lowered.type, subst);
  result.expanded = result.type != nullptr;
  return result;
}

static AliasExpandResult NormalizeBinaryCoreType(const ScopeContext& ctx,
                                                 const TypeRef& type) {
  AliasExpandResult out;
  out.type = StripPermAndRefine(type);
  for (int i = 0; i < 16; ++i) {
    if (!out.type) {
      return out;
    }
    const auto* path = std::get_if<TypePathType>(&out.type->node);
    if (!path) {
      return out;
    }
    const auto expanded = ExpandTypeAliasApply(ctx, *path);
    if (!expanded.ok) {
      out.ok = false;
      out.diag_id = expanded.diag_id;
      return out;
    }
    if (!expanded.expanded) {
      return out;
    }
    out.type = StripPermAndRefine(expanded.type);
    out.expanded = true;
  }
  return out;
}

static bool IsRawPtrLikeType(const TypeRef& type) {
  TypeRef cur = type;
  while (cur) {
    if (const auto* perm = std::get_if<TypePerm>(&cur->node)) {
      cur = perm->base;
      continue;
    }
    if (const auto* refine = std::get_if<TypeRefine>(&cur->node)) {
      cur = refine->base;
      continue;
    }
    break;
  }
  return cur && std::holds_alternative<TypeRawPtr>(cur->node);
}

}  // namespace

// (T-Arith), (T-Bitwise), (T-Shift), (T-Compare-Eq), (T-Compare-Ord), (T-Logical)
ExprTypeResult TypeBinaryExprImpl(const ScopeContext& ctx,
                                  const StmtTypeContext& type_ctx,
                                  const ast::BinaryExpr& expr,
                                  const TypeEnv& env) {
  SpecDefsBinary();
  ExprTypeResult result;
  const std::string_view op = expr.op;

  // Null literals are check-only by default (Chk-Null-Literal), but equality
  // needs contextual typing from the opposite operand.
  if (IsEqOp(op)) {
    const bool lhs_null = IsNullLiteralExpr(expr.lhs);
    const bool rhs_null = IsNullLiteralExpr(expr.rhs);
    if (lhs_null != rhs_null) {
      const auto typed_non_null =
          TypeExpr(ctx, type_ctx, lhs_null ? expr.rhs : expr.lhs, env);
      if (!typed_non_null.ok) {
        result.diag_id = typed_non_null.diag_id;
        return result;
      }
      if (!IsRawPtrLikeType(typed_non_null.type)) {
        return result;
      }
      SPEC_RULE("T-Compare-Eq");
      result.ok = true;
      result.type = MakeTypePrim("bool");
      return result;
    }
  }

  // Type both operands
  const auto lhs = TypeExpr(ctx, type_ctx, expr.lhs, env);
  if (!lhs.ok) {
    result.diag_id = lhs.diag_id;
    return result;
  }

  const auto rhs = TypeExpr(ctx, type_ctx, expr.rhs, env);
  if (!rhs.ok) {
    result.diag_id = rhs.diag_id;
    return result;
  }

  // Binary operator typing compares value domains; permission/refinement wrappers
  // are ignored once access legality has been established by place typing.
  const auto lhs_core_norm = NormalizeBinaryCoreType(ctx, lhs.type);
  if (!lhs_core_norm.ok) {
    result.diag_id = lhs_core_norm.diag_id;
    return result;
  }
  const auto rhs_core_norm = NormalizeBinaryCoreType(ctx, rhs.type);
  if (!rhs_core_norm.ok) {
    result.diag_id = rhs_core_norm.diag_id;
    return result;
  }
  const auto lhs_core = lhs_core_norm.type;
  const auto rhs_core = rhs_core_norm.type;

  const auto lhs_name = ResolveAliasTransparentPrim(ctx, lhs_core);
  if (!lhs_name.ok) {
    result.diag_id = lhs_name.diag_id;
    return result;
  }
  const auto rhs_name = ResolveAliasTransparentPrim(ctx, rhs_core);
  if (!rhs_name.ok) {
    result.diag_id = rhs_name.diag_id;
    return result;
  }

  // Arithmetic operators: +, -, *, /, %, **
  if (IsArithOp(op)) {
    if (!lhs_name.prim.has_value() || !rhs_name.prim.has_value()) {
      return result;
    }
    const auto equiv = AliasTransparentEquiv(ctx, lhs_core, rhs_core);
    if (!equiv.ok) {
      result.diag_id = equiv.diag_id;
      return result;
    }
    if (!equiv.subtype) {
      return result;
    }
    if (!IsNumericType(*lhs_name.prim)) {
      return result;
    }
    SPEC_RULE("T-Arith");
    result.ok = true;
    result.type = MakeTypePrim(*lhs_name.prim);
    return result;
  }

  // Bitwise operators: &, |, ^
  if (IsBitOp(op)) {
    if (!lhs_name.prim.has_value() || !rhs_name.prim.has_value()) {
      return result;
    }
    const auto equiv = AliasTransparentEquiv(ctx, lhs_core, rhs_core);
    if (!equiv.ok) {
      result.diag_id = equiv.diag_id;
      return result;
    }
    if (!equiv.subtype) {
      return result;
    }
    if (!IsIntType(*lhs_name.prim)) {
      return result;
    }
    SPEC_RULE("T-Bitwise");
    result.ok = true;
    result.type = MakeTypePrim(*lhs_name.prim);
    return result;
  }

  // Shift operators: <<, >>
  if (IsShiftOp(op)) {
    if (!lhs_name.prim.has_value()) {
      return result;
    }
    if (!IsIntType(*lhs_name.prim)) {
      return result;
    }
    const auto rhs_u32 = AliasTransparentPrimEq(ctx, rhs_core, "u32");
    if (!rhs_u32.ok) {
      result.diag_id = rhs_u32.diag_id;
      return result;
    }
    if (!rhs_u32.subtype) {
      return result;
    }
    SPEC_RULE("T-Shift");
    result.ok = true;
    result.type = MakeTypePrim(*lhs_name.prim);
    return result;
  }

  // Equality operators: ==, !=
  if (IsEqOp(op)) {
    bool lhs_eq = EqType(lhs_core);
    if (!lhs_eq && lhs_name.prim.has_value()) {
      lhs_eq = true;
    }
    if (!lhs_eq) {
      return result;
    }
    const auto equiv = AliasTransparentEquiv(ctx, lhs_core, rhs_core);
    if (!equiv.ok) {
      result.diag_id = equiv.diag_id;
      return result;
    }
    if (!equiv.subtype) {
      return result;
    }
    SPEC_RULE("T-Compare-Eq");
    result.ok = true;
    result.type = MakeTypePrim("bool");
    return result;
  }

  // Ordering operators: <, <=, >, >=
  if (IsOrdOp(op)) {
    bool lhs_ord = OrdType(lhs_core);
    if (!lhs_ord && lhs_name.prim.has_value()) {
      lhs_ord = IsOrdPrim(*lhs_name.prim);
    }
    if (!lhs_ord) {
      return result;
    }
    const auto equiv = AliasTransparentEquiv(ctx, lhs_core, rhs_core);
    if (!equiv.ok) {
      result.diag_id = equiv.diag_id;
      return result;
    }
    if (!equiv.subtype) {
      return result;
    }
    SPEC_RULE("T-Compare-Ord");
    result.ok = true;
    result.type = MakeTypePrim("bool");
    return result;
  }

  // Logical operators: &&, ||
  if (IsLogicOp(op)) {
    const auto lhs_bool = AliasTransparentPrimEq(ctx, lhs_core, "bool");
    if (!lhs_bool.ok) {
      result.diag_id = lhs_bool.diag_id;
      return result;
    }
    const auto rhs_bool = AliasTransparentPrimEq(ctx, rhs_core, "bool");
    if (!rhs_bool.ok) {
      result.diag_id = rhs_bool.diag_id;
      return result;
    }
    if (!lhs_bool.subtype || !rhs_bool.subtype) {
      return result;
    }
    SPEC_RULE("T-Logical");
    result.ok = true;
    result.type = MakeTypePrim("bool");
    return result;
  }

  return result;
}

}  // namespace cursive::analysis::expr
