#include "cursive0/sema/arrays_slices.h"

#include <array>
#include <cstdint>
#include <optional>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "cursive0/core/assert_spec.h"
#include "cursive0/sema/scopes.h"
#include "cursive0/sema/type_equiv.h"

namespace cursive0::sema {

namespace {

static inline void SpecDefsArraysSlices() {
  SPEC_DEF("ConstIndex", "5.2.6");
  SPEC_DEF("ConstLenJudg", "5.2.1");
  SPEC_DEF("BitcopyType", "5.11");
  SPEC_DEF("StripPerm", "5.2.12");
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

static bool IsRangeExpr(const syntax::ExprPtr& expr) {
  return expr && std::holds_alternative<syntax::RangeExpr>(expr->node);
}

static constexpr std::array<std::string_view, 12> kIntTypes = {
    "i8",   "i16",  "i32",  "i64",  "i128", "u8",
    "u16",  "u32",  "u64",  "u128", "isize", "usize"};

static constexpr std::array<std::string_view, 3> kFloatTypes = {"f16",
                                                                "f32",
                                                                "f64"};

static bool IsIntTypeName(std::string_view name) {
  for (const auto& t : kIntTypes) {
    if (name == t) {
      return true;
    }
  }
  return false;
}

static bool IsFloatTypeName(std::string_view name) {
  for (const auto& t : kFloatTypes) {
    if (name == t) {
      return true;
    }
  }
  return false;
}

static bool IsPrimTypeName(std::string_view name) {
  return IsIntTypeName(name) || IsFloatTypeName(name) || name == "bool" ||
         name == "char" || name == "()" || name == "!";
}

static bool BuiltinBitcopyType(const TypeRef& type) {
  if (!type) {
    return false;
  }
  return std::visit(
      [](const auto& node) -> bool {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, TypePrim>) {
          return IsPrimTypeName(node.name);
        } else if constexpr (std::is_same_v<T, TypePtr> ||
                             std::is_same_v<T, TypeRawPtr> ||
                             std::is_same_v<T, TypeSlice> ||
                             std::is_same_v<T, TypeFunc> ||
                             std::is_same_v<T, TypeDynamic> ||
                             std::is_same_v<T, TypeRange>) {
          return true;
        } else if constexpr (std::is_same_v<T, TypeString>) {
          return node.state.has_value() && *node.state == StringState::View;
        } else if constexpr (std::is_same_v<T, TypeBytes>) {
          return node.state.has_value() && *node.state == BytesState::View;
        } else {
          return false;
        }
      },
      type->node);
}

static bool IsBitcopyClassPath(const syntax::ClassPath& path) {
  return !path.empty() && IdEq(path.back(), "Bitcopy");
}

static bool ImplementsBitcopy(const ScopeContext& ctx, const TypeRef& type) {
  const auto* path_type = std::get_if<TypePathType>(&type->node);
  if (!path_type) {
    return false;
  }

  syntax::Path syntax_path;
  syntax_path.reserve(path_type->path.size());
  for (const auto& comp : path_type->path) {
    syntax_path.push_back(comp);
  }
  const auto it = ctx.sigma.types.find(PathKeyOf(syntax_path));
  if (it == ctx.sigma.types.end()) {
    return false;
  }

  auto has_bitcopy = [](const std::vector<syntax::ClassPath>& impls) {
    for (const auto& impl : impls) {
      if (IsBitcopyClassPath(impl)) {
        return true;
      }
    }
    return false;
  };

  if (const auto* record = std::get_if<syntax::RecordDecl>(&it->second)) {
    return has_bitcopy(record->implements);
  }
  if (const auto* enum_decl = std::get_if<syntax::EnumDecl>(&it->second)) {
    return has_bitcopy(enum_decl->implements);
  }
  if (const auto* modal_decl = std::get_if<syntax::ModalDecl>(&it->second)) {
    return has_bitcopy(modal_decl->implements);
  }
  return false;
}

static bool BitcopyType(const ScopeContext& ctx, const TypeRef& type) {
  if (!type) {
    return false;
  }
  if (const auto* perm = std::get_if<TypePerm>(&type->node)) {
    if (perm->perm == Permission::Unique) {
      return false;
    }
    return BitcopyType(ctx, perm->base);
  }
  if (const auto* tuple = std::get_if<TypeTuple>(&type->node)) {
    for (const auto& elem : tuple->elements) {
      if (!BitcopyType(ctx, elem)) {
        return false;
      }
    }
    return true;
  }
  if (const auto* array = std::get_if<TypeArray>(&type->node)) {
    return BitcopyType(ctx, array->element);
  }
  return BuiltinBitcopyType(type) || ImplementsBitcopy(ctx, type);
}

static bool CheckExprUsize(const ExprTypeFn& type_expr,
                           const syntax::ExprPtr& expr,
                           std::optional<std::string_view>& diag_id,
                           std::string_view non_usize_diag) {
  if (!expr) {
    return false;
  }
  const auto type_res = type_expr(expr);
  if (!type_res.ok) {
    diag_id = type_res.diag_id;
    return false;
  }
  if (!IsPrimType(type_res.type, "usize")) {
    diag_id = non_usize_diag;
    return false;
  }
  return true;
}

static bool CheckRangeExpr(const ExprTypeFn& type_expr,
                           const syntax::RangeExpr& range,
                           std::optional<std::string_view>& diag_id,
                           std::string_view non_usize_diag) {
  switch (range.kind) {
    case syntax::RangeKind::Full:
      return true;
    case syntax::RangeKind::To:
    case syntax::RangeKind::ToInclusive:
      return CheckExprUsize(type_expr, range.rhs, diag_id, non_usize_diag);
    case syntax::RangeKind::From:
      return CheckExprUsize(type_expr, range.lhs, diag_id, non_usize_diag);
    case syntax::RangeKind::Exclusive:
    case syntax::RangeKind::Inclusive: {
      if (!CheckExprUsize(type_expr, range.lhs, diag_id, non_usize_diag)) {
        return false;
      }
      return CheckExprUsize(type_expr, range.rhs, diag_id, non_usize_diag);
    }
  }
  return false;
}

}  // namespace

ExprTypeResult TypeArrayExpr(const ScopeContext& ctx,
                             const syntax::ArrayExpr& expr,
                             const ExprTypeFn& type_expr) {
  SpecDefsArraysSlices();
  (void)ctx;
  ExprTypeResult result;
  if (expr.elements.empty()) {
    return result;
  }

  const auto first_type = type_expr(expr.elements.front());
  if (!first_type.ok) {
    result.diag_id = first_type.diag_id;
    return result;
  }

  for (std::size_t i = 1; i < expr.elements.size(); ++i) {
    const auto elem_type = type_expr(expr.elements[i]);
    if (!elem_type.ok) {
      result.diag_id = elem_type.diag_id;
      return result;
    }
    const auto equiv = TypeEquiv(first_type.type, elem_type.type);
    if (!equiv.ok) {
      result.diag_id = equiv.diag_id;
      return result;
    }
    if (!equiv.equiv) {
      return result;
    }
  }

  SPEC_RULE("T-Array-Literal-List");
  result.ok = true;
  result.type =
      MakeTypeArray(first_type.type,
                    static_cast<std::uint64_t>(expr.elements.size()));
  return result;
}

ExprTypeResult TypeIndexAccessValue(const ScopeContext& ctx,
                                    const syntax::IndexAccessExpr& expr,
                                    const ExprTypeFn& type_expr) {
  SpecDefsArraysSlices();
  ExprTypeResult result;
  if (!expr.base || !expr.index) {
    return result;
  }

  const auto base_type = type_expr(expr.base);
  if (!base_type.ok) {
    result.diag_id = base_type.diag_id;
    return result;
  }
  const auto stripped = StripPerm(base_type.type);
  if (!stripped) {
    return result;
  }

  if (const auto* array = std::get_if<TypeArray>(&stripped->node)) {
    if (IsRangeExpr(expr.index)) {
      const auto& range = std::get<syntax::RangeExpr>(expr.index->node);
      std::optional<std::string_view> diag_id;
      if (!CheckRangeExpr(type_expr, range, diag_id,
                          "Index-Array-NonUsize")) {
        if (diag_id == std::optional<std::string_view>("Index-Array-NonUsize")) {
          SPEC_RULE("Index-Array-NonUsize");
        }
        result.diag_id = diag_id;
        return result;
      }
      TypeRef out_type = MakeTypeSlice(array->element);
      if (const auto* perm = std::get_if<TypePerm>(&base_type.type->node)) {
        out_type = MakeTypePerm(perm->perm, out_type);
        if (!BitcopyType(ctx, out_type)) {
          result.diag_id = "ValueUse-NonBitcopyPlace";
          return result;
        }
        SPEC_RULE("T-Slice-From-Array-Perm");
      } else {
        if (!BitcopyType(ctx, out_type)) {
          result.diag_id = "ValueUse-NonBitcopyPlace";
          return result;
        }
        SPEC_RULE("T-Slice-From-Array");
      }
      result.ok = true;
      result.type = std::move(out_type);
      return result;
    }

    const auto index_type = type_expr(expr.index);
    if (!index_type.ok) {
      result.diag_id = index_type.diag_id;
      return result;
    }
    if (!IsPrimType(index_type.type, "usize")) {
      SPEC_RULE("Index-Array-NonUsize");
      result.diag_id = "Index-Array-NonUsize";
      return result;
    }

    const auto index_const = ConstLen(ctx, expr.index);
    if (!index_const.ok || !index_const.value.has_value()) {
      SPEC_RULE("Index-Array-NonConst-Err");
      result.diag_id = "Index-Array-NonConst-Err";
      return result;
    }

    if (*index_const.value >= array->length) {
      SPEC_RULE("Index-Array-OOB-Err");
      result.diag_id = "Index-Array-OOB-Err";
      return result;
    }

    const auto* perm = std::get_if<TypePerm>(&base_type.type->node);
    TypeRef out_type = array->element;
    if (perm) {
      out_type = MakeTypePerm(perm->perm, out_type);
      if (!BitcopyType(ctx, out_type)) {
        result.diag_id = "ValueUse-NonBitcopyPlace";
        return result;
      }
      SPEC_RULE("T-Index-Array-Perm");
    } else {
      if (!BitcopyType(ctx, out_type)) {
        result.diag_id = "ValueUse-NonBitcopyPlace";
        return result;
      }
      SPEC_RULE("T-Index-Array");
    }

    result.ok = true;
    result.type = std::move(out_type);
    return result;
  }

  if (const auto* slice = std::get_if<TypeSlice>(&stripped->node)) {
    if (IsRangeExpr(expr.index)) {
      const auto& range = std::get<syntax::RangeExpr>(expr.index->node);
      std::optional<std::string_view> diag_id;
      if (!CheckRangeExpr(type_expr, range, diag_id,
                          "Index-Slice-NonUsize")) {
        result.diag_id = diag_id;
        return result;
      }
      TypeRef out_type = MakeTypeSlice(slice->element);
      if (const auto* perm = std::get_if<TypePerm>(&base_type.type->node)) {
        out_type = MakeTypePerm(perm->perm, out_type);
        if (!BitcopyType(ctx, out_type)) {
          result.diag_id = "ValueUse-NonBitcopyPlace";
          return result;
        }
        SPEC_RULE("T-Slice-From-Slice-Perm");
      } else {
        if (!BitcopyType(ctx, out_type)) {
          result.diag_id = "ValueUse-NonBitcopyPlace";
          return result;
        }
        SPEC_RULE("T-Slice-From-Slice");
      }
      result.ok = true;
      result.type = std::move(out_type);
      return result;
    }

    const auto index_type = type_expr(expr.index);
    if (!index_type.ok) {
      result.diag_id = index_type.diag_id;
      return result;
    }
    if (IsPrimType(index_type.type, "usize")) {
      if (std::holds_alternative<TypePerm>(base_type.type->node)) {
        SPEC_RULE("Index-Slice-Perm-Direct-Err");
      } else {
        SPEC_RULE("Index-Slice-Direct-Err");
      }
      result.diag_id = "Index-Slice-Direct-Err";
      return result;
    }
    result.diag_id = "Index-Slice-NonUsize";
    return result;
  }

  SPEC_RULE("Index-NonIndexable");
  result.diag_id = "Index-NonIndexable";
  return result;
}

PlaceTypeResult TypeIndexAccessPlace(const ScopeContext& ctx,
                                     const syntax::IndexAccessExpr& expr,
                                     const PlaceTypeFn& type_place,
                                     const ExprTypeFn& type_expr) {
  SpecDefsArraysSlices();
  PlaceTypeResult result;
  if (!expr.base || !expr.index) {
    return result;
  }

  const auto base_type = type_place(expr.base);
  if (!base_type.ok) {
    result.diag_id = base_type.diag_id;
    return result;
  }
  const auto stripped = StripPerm(base_type.type);
  if (!stripped) {
    return result;
  }

  if (const auto* array = std::get_if<TypeArray>(&stripped->node)) {
    if (IsRangeExpr(expr.index)) {
      const auto& range = std::get<syntax::RangeExpr>(expr.index->node);
      std::optional<std::string_view> diag_id;
      if (!CheckRangeExpr(type_expr, range, diag_id,
                          "Index-Array-NonUsize")) {
        if (diag_id == std::optional<std::string_view>("Index-Array-NonUsize")) {
          SPEC_RULE("Index-Array-NonUsize");
        }
        result.diag_id = diag_id;
        return result;
      }
      TypeRef out_type = MakeTypeSlice(array->element);
      if (const auto* perm = std::get_if<TypePerm>(&base_type.type->node)) {
        out_type = MakeTypePerm(perm->perm, out_type);
        SPEC_RULE("P-Slice-From-Array-Perm");
      } else {
        SPEC_RULE("P-Slice-From-Array");
      }
      result.ok = true;
      result.type = std::move(out_type);
      return result;
    }

    const auto index_type = type_expr(expr.index);
    if (!index_type.ok) {
      result.diag_id = index_type.diag_id;
      return result;
    }
    if (!IsPrimType(index_type.type, "usize")) {
      SPEC_RULE("Index-Array-NonUsize");
      result.diag_id = "Index-Array-NonUsize";
      return result;
    }

    const auto index_const = ConstLen(ctx, expr.index);
    if (!index_const.ok || !index_const.value.has_value()) {
      SPEC_RULE("Index-Array-NonConst-Err");
      result.diag_id = "Index-Array-NonConst-Err";
      return result;
    }

    if (*index_const.value >= array->length) {
      SPEC_RULE("Index-Array-OOB-Err");
      result.diag_id = "Index-Array-OOB-Err";
      return result;
    }

    if (const auto* perm = std::get_if<TypePerm>(&base_type.type->node)) {
      SPEC_RULE("P-Index-Array-Perm");
      result.type = MakeTypePerm(perm->perm, array->element);
    } else {
      SPEC_RULE("P-Index-Array");
      result.type = array->element;
    }
    result.ok = true;
    return result;
  }

  if (const auto* slice = std::get_if<TypeSlice>(&stripped->node)) {
    if (IsRangeExpr(expr.index)) {
      const auto& range = std::get<syntax::RangeExpr>(expr.index->node);
      std::optional<std::string_view> diag_id;
      if (!CheckRangeExpr(type_expr, range, diag_id,
                          "Index-Slice-NonUsize")) {
        result.diag_id = diag_id;
        return result;
      }
      TypeRef out_type = MakeTypeSlice(slice->element);
      if (const auto* perm = std::get_if<TypePerm>(&base_type.type->node)) {
        out_type = MakeTypePerm(perm->perm, out_type);
        SPEC_RULE("P-Slice-From-Slice-Perm");
      } else {
        SPEC_RULE("P-Slice-From-Slice");
      }
      result.ok = true;
      result.type = std::move(out_type);
      return result;
    }

    const auto index_type = type_expr(expr.index);
    if (!index_type.ok) {
      result.diag_id = index_type.diag_id;
      return result;
    }
    if (IsPrimType(index_type.type, "usize")) {
      if (std::holds_alternative<TypePerm>(base_type.type->node)) {
        SPEC_RULE("Index-Slice-Perm-Direct-Err");
      } else {
        SPEC_RULE("Index-Slice-Direct-Err");
      }
      result.diag_id = "Index-Slice-Direct-Err";
      return result;
    }
    result.diag_id = "Index-Slice-NonUsize";
    return result;
  }

  SPEC_RULE("Index-NonIndexable");
  result.diag_id = "Index-NonIndexable";
  return result;
}

ExprTypeResult CoerceArrayToSlice(const ScopeContext& ctx,
                                  const TypeRef& type) {
  SpecDefsArraysSlices();
  (void)ctx;
  ExprTypeResult result;
  if (!type) {
    return result;
  }
  const auto* perm = std::get_if<TypePerm>(&type->node);
  if (!perm || !perm->base) {
    return result;
  }
  const auto* array = std::get_if<TypeArray>(&perm->base->node);
  if (!array) {
    return result;
  }

  SPEC_RULE("Coerce-Array-Slice");
  result.ok = true;
  result.type = MakeTypePerm(perm->perm, MakeTypeSlice(array->element));
  return result;
}

}  // namespace cursive0::sema
