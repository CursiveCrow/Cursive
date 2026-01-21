#include "cursive0/sema/unions.h"

#include <optional>
#include <string_view>

#include "cursive0/core/assert_spec.h"
#include "cursive0/sema/type_equiv.h"

namespace cursive0::sema {

namespace {

static inline void SpecDefsUnions() {
  SPEC_DEF("Members", "5.2.7");
  SPEC_DEF("DistinctMembers", "5.2.7");
  SPEC_DEF("SetMembers", "5.2.7");
  SPEC_DEF("Member", "5.2.2");
  SPEC_DEF("StripPerm", "5.2.12");
}

struct UnionMemberResult {
  bool ok = false;
  std::optional<std::string_view> diag_id;
  bool member = false;
};

static TypeRef StripPerm(const TypeRef& type) {
  if (!type) {
    return type;
  }
  if (const auto* perm = std::get_if<TypePerm>(&type->node)) {
    return perm->base;
  }
  return type;
}

static UnionMemberResult UnionMember(const TypeRef& type,
                                     const TypeUnion& uni) {
  SpecDefsUnions();
  for (const auto& member : uni.members) {
    const auto equiv = TypeEquiv(type, member);
    if (!equiv.ok) {
      return {false, equiv.diag_id, false};
    }
    if (equiv.equiv) {
      return {true, std::nullopt, true};
    }
  }
  return {true, std::nullopt, false};
}

}  // namespace

ExprTypeResult TypeUnionIntro(const ScopeContext& ctx,
                              const TypeRef& value_type,
                              const TypeRef& union_type) {
  (void)ctx;
  SpecDefsUnions();
  ExprTypeResult result;
  if (!value_type || !union_type) {
    return result;
  }

  if (const auto* perm_union = std::get_if<TypePerm>(&union_type->node)) {
    if (!perm_union->base) {
      return result;
    }
    const auto* base_union = std::get_if<TypeUnion>(&perm_union->base->node);
    if (!base_union) {
      return result;
    }
    const auto* perm_value = std::get_if<TypePerm>(&value_type->node);
    if (!perm_value || perm_value->perm != perm_union->perm) {
      return result;
    }
    const auto member = UnionMember(perm_value->base, *base_union);
    if (!member.ok) {
      result.diag_id = member.diag_id;
      return result;
    }
    if (!member.member) {
      return result;
    }
    SPEC_RULE("T-Union-Intro");
    result.ok = true;
    result.type = union_type;
    return result;
  }

  const auto* base_union = std::get_if<TypeUnion>(&union_type->node);
  if (!base_union) {
    return result;
  }

  const auto member = UnionMember(value_type, *base_union);
  if (!member.ok) {
    result.diag_id = member.diag_id;
    return result;
  }
  if (!member.member) {
    return result;
  }

  SPEC_RULE("T-Union-Intro");
  result.ok = true;
  result.type = union_type;
  return result;
}

UnionAccessResult CheckUnionDirectAccess(const ScopeContext& ctx,
                                         const syntax::FieldAccessExpr& expr,
                                         const ExprTypeFn& type_expr) {
  (void)ctx;
  SpecDefsUnions();
  UnionAccessResult result;
  if (!expr.base) {
    return result;
  }
  const auto base_type = type_expr(expr.base);
  if (!base_type.ok) {
    result.diag_id = base_type.diag_id;
    return result;
  }
  const auto stripped = StripPerm(base_type.type);
  if (stripped && std::holds_alternative<TypeUnion>(stripped->node)) {
    SPEC_RULE("Union-DirectAccess-Err");
    result.diag_id = "Union-DirectAccess-Err";
    return result;
  }
  result.ok = true;
  return result;
}

}  // namespace cursive0::sema
