// =============================================================================
// MIGRATION MAPPING: tuples.cpp
// =============================================================================
//
// SPEC REFERENCE: CursiveSpecification.md
// - Section 5.2.5 "Tuples" (lines 8874-8920)
//   - T-Unit-Literal (lines 8876-8878)
//   - T-Tuple-Literal (lines 8880-8883)
//   - T-Tuple-Index (lines 8885-8888)
//   - T-Tuple-Index-Perm (lines 8890-8893)
//   - P-Tuple-Index (lines 8895-8898)
//   - P-Tuple-Index-Perm (lines 8900-8903)
//   - ConstTupleIndex (line 8905)
//   - TupleIndex-NonConst (lines 8907-8910)
//   - TupleIndex-OOB (lines 8912-8915)
//   - TupleAccess-NotTuple (lines 8917-8920)
// - Section 5.11 "Foundational Predicates" for BitcopyType
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/composite/tuples.cpp
// - Lines 1-421 (entire file)
//
// Key source functions to migrate:
// - TypeTupleExpr (lines 278-304): Tuple literal typing
// - TypeTupleAccessValue (lines 306-366): Tuple element access (value context)
// - TypeTupleAccessPlace (lines 368-418): Tuple element access (place context)
//
// Supporting helpers:
// - kIntSuffixes (lines 26-28): Integer literal suffixes
// - EndsWith (lines 30-35): String suffix check
// - StripIntSuffix (lines 37-48): Strip integer suffix from lexeme
// - DigitValue (lines 50-79): Parse digit value for base
// - ParseIntCore (lines 81-128): Parse integer literal core (128-bit)
// - ParseTupleIndex (lines 130-141): Parse tuple index from token
// - StripPerm (lines 143-151): Strip permission layer
// - kIntTypes/kFloatTypes (lines 153-159): Primitive type name lists
// - IsPrimTypeName (lines 179-182): Check primitive type names
// - BuiltinBitcopyType (lines 184-209): Check built-in bitcopy types
// - IsBitcopyClassPath (lines 211-213): Check Bitcopy class path
// - ImplementsBitcopy (lines 215-250): Check if type implements Bitcopy
// - BitcopyType (lines 252-274): Full bitcopy type predicate
//
// DEPENDENCIES:
// - cursive/src/04_analysis/resolve/scopes.h (ScopeContext)
// - cursive/src/00_core/int128.h (UInt128 operations)
// - cursive/src/00_core/assert_spec.h (SPEC_DEF, SPEC_RULE)
//
// REFACTORING NOTES:
// 1. Integer parsing logic is duplicated with enums.cpp - consolidate
// 2. BitcopyType and related predicates are duplicated across files
// 3. StripPerm helper is duplicated; extract to common module
// 4. The 128-bit integer parsing should use a shared utility
// 5. Tuple index must be a compile-time constant (statically resolved)
// =============================================================================

#include "04_analysis/composite/tuples.h"

#include <array>
#include <cstdint>
#include <optional>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "00_core/assert_spec.h"
#include "00_core/int128.h"
#include "00_core/numeric_literals.h"
#include "04_analysis/generics/monomorphize.h"
#include "04_analysis/resolve/scopes.h"
#include "04_analysis/resolve/scopes_lookup.h"
#include "04_analysis/typing/type_lower.h"

namespace cursive::analysis {

namespace {

static inline void SpecDefsTuples() {
  SPEC_DEF("ConstTupleIndex", "5.2.5");
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

static AliasExpandResult NormalizeTupleBaseType(const ScopeContext& ctx,
                                                const TypeRef& type) {
  AliasExpandResult out;
  out.type = type;
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
    out.type = expanded.type;
    out.expanded = true;
  }
  return out;
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
                             std::is_same_v<T, TypeRange> ||
                             std::is_same_v<T, TypeRangeInclusive> ||
                             std::is_same_v<T, TypeRangeFrom> ||
                             std::is_same_v<T, TypeRangeTo> ||
                             std::is_same_v<T, TypeRangeToInclusive> ||
                             std::is_same_v<T, TypeRangeFull>) {
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

static bool IsBitcopyClassPath(const ast::ClassPath& path) {
  return !path.empty() && IdEq(path.back(), "Bitcopy");
}

static bool ImplementsBitcopy(const ScopeContext& ctx, const TypeRef& type) {
  const auto* path_type = std::get_if<TypePathType>(&type->node);
  if (!path_type) {
    return false;
  }

  ast::Path ast_path;
  ast_path.reserve(path_type->path.size());
  for (const auto& comp : path_type->path) {
    ast_path.push_back(comp);
  }
  const auto it = ctx.sigma.types.find(PathKeyOf(ast_path));
  if (it == ctx.sigma.types.end()) {
    return false;
  }

  auto has_bitcopy = [](const std::vector<ast::ClassPath>& impls) {
    for (const auto& impl : impls) {
      if (IsBitcopyClassPath(impl)) {
        return true;
      }
    }
    return false;
  };

  if (const auto* record = std::get_if<ast::RecordDecl>(&it->second)) {
    return has_bitcopy(record->implements);
  }
  if (const auto* enum_decl = std::get_if<ast::EnumDecl>(&it->second)) {
    return has_bitcopy(enum_decl->implements);
  }
  if (const auto* modal_decl = std::get_if<ast::ModalDecl>(&it->second)) {
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

}  // namespace

ExprTypeResult TypeTupleExpr(const ScopeContext& ctx,
                             const ast::TupleExpr& expr,
                             const ExprTypeFn& type_expr) {
  SpecDefsTuples();
  (void)ctx;
  ExprTypeResult result;
  if (expr.elements.empty()) {
    SPEC_RULE("T-Unit-Literal");
    result.ok = true;
    result.type = MakeTypePrim("()");
    return result;
  }
  std::vector<TypeRef> elements;
  elements.reserve(expr.elements.size());
  for (const auto& element : expr.elements) {
    const auto elem_type = type_expr(element);
    if (!elem_type.ok) {
      result.diag_id = elem_type.diag_id;
      return result;
    }
    elements.push_back(elem_type.type);
  }
  SPEC_RULE("T-Tuple-Literal");
  result.ok = true;
  result.type = MakeTypeTuple(std::move(elements));
  return result;
}

ExprTypeResult TypeTupleAccessValue(const ScopeContext& ctx,
                                    const ast::TupleAccessExpr& expr,
                                    const ExprTypeFn& type_expr) {
  SpecDefsTuples();
  ExprTypeResult result;
  if (!expr.base) {
    return result;
  }

  const auto base_type = type_expr(expr.base);
  if (!base_type.ok) {
    result.diag_id = base_type.diag_id;
    return result;
  }
  const auto stripped = StripPerm(base_type.type);
  const auto normalized = NormalizeTupleBaseType(ctx, stripped);
  if (!normalized.ok) {
    result.diag_id = normalized.diag_id;
    return result;
  }
  const auto* tuple =
      normalized.type ? std::get_if<TypeTuple>(&normalized.type->node) : nullptr;
  if (!tuple) {
    if (normalized.type && std::holds_alternative<TypeUnion>(normalized.type->node)) {
      SPEC_RULE("Union-DirectAccess-Err");
      result.diag_id = "Union-DirectAccess-Err";
      return result;
    }
    SPEC_RULE("TupleAccess-NotTuple");
    result.diag_id = "TupleAccess-NotTuple";
    return result;
  }

  const auto index = ast::TupleIndexToSize(expr.index);
  if (!index.has_value() || *index >= tuple->elements.size()) {
    SPEC_RULE("TupleIndex-OOB");
    result.diag_id = "TupleIndex-OOB";
    return result;
  }

  const auto element = tuple->elements[*index];
  const auto* perm = std::get_if<TypePerm>(&base_type.type->node);
  TypeRef out_type = element;
  if (perm) {
    out_type = MakeTypePerm(perm->perm, element);
    if (!BitcopyType(ctx, out_type)) {
      result.diag_id = "ValueUse-NonBitcopyPlace";
      return result;
    }
    SPEC_RULE("T-Tuple-Index-Perm");
  } else {
    if (!BitcopyType(ctx, out_type)) {
      result.diag_id = "ValueUse-NonBitcopyPlace";
      return result;
    }
    SPEC_RULE("T-Tuple-Index");
  }

  result.ok = true;
  result.type = std::move(out_type);
  return result;
}

PlaceTypeResult TypeTupleAccessPlace(const ScopeContext& ctx,
                                     const ast::TupleAccessExpr& expr,
                                     const PlaceTypeFn& type_place) {
  SpecDefsTuples();
  (void)ctx;
  PlaceTypeResult result;
  if (!expr.base) {
    return result;
  }

  const auto base_type = type_place(expr.base);
  if (!base_type.ok) {
    result.diag_id = base_type.diag_id;
    return result;
  }
  const auto stripped = StripPerm(base_type.type);
  const auto normalized = NormalizeTupleBaseType(ctx, stripped);
  if (!normalized.ok) {
    result.diag_id = normalized.diag_id;
    return result;
  }
  const auto* tuple =
      normalized.type ? std::get_if<TypeTuple>(&normalized.type->node) : nullptr;
  if (!tuple) {
    if (normalized.type && std::holds_alternative<TypeUnion>(normalized.type->node)) {
      SPEC_RULE("Union-DirectAccess-Err");
      result.diag_id = "Union-DirectAccess-Err";
      return result;
    }
    SPEC_RULE("TupleAccess-NotTuple");
    result.diag_id = "TupleAccess-NotTuple";
    return result;
  }

  const auto index = ast::TupleIndexToSize(expr.index);
  if (!index.has_value() || *index >= tuple->elements.size()) {
    SPEC_RULE("TupleIndex-OOB");
    result.diag_id = "TupleIndex-OOB";
    return result;
  }

  const auto element = tuple->elements[*index];
  const auto* perm = std::get_if<TypePerm>(&base_type.type->node);
  if (perm) {
    SPEC_RULE("P-Tuple-Index-Perm");
    result.type = MakeTypePerm(perm->perm, element);
  } else {
    SPEC_RULE("P-Tuple-Index");
    result.type = element;
  }
  result.ok = true;
  return result;
}

}  // namespace cursive::analysis
