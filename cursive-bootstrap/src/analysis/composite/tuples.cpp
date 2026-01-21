#include "cursive0/analysis/composite/tuples.h"

#include <array>
#include <cstdint>
#include <limits>
#include <optional>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "cursive0/core/assert_spec.h"
#include "cursive0/core/int128.h"
#include "cursive0/analysis/resolve/scopes.h"

namespace cursive0::analysis {

namespace {

static inline void SpecDefsTuples() {
  SPEC_DEF("ConstTupleIndex", "5.2.5");
  SPEC_DEF("BitcopyType", "5.11");
  SPEC_DEF("StripPerm", "5.2.12");
}

static constexpr std::array<std::string_view, 12> kIntSuffixes = {
    "i128", "u128", "isize", "usize", "i64", "u64",
    "i32",  "u32",  "i16",   "u16",  "i8",  "u8"};

static bool EndsWith(std::string_view value, std::string_view suffix) {
  if (suffix.size() > value.size()) {
    return false;
  }
  return value.substr(value.size() - suffix.size()) == suffix;
}

static std::string_view StripIntSuffix(std::string_view lexeme) {
  for (const auto suffix : kIntSuffixes) {
    if (EndsWith(lexeme, suffix)) {
      const std::size_t core_len = lexeme.size() - suffix.size();
      if (core_len == 0) {
        continue;
      }
      return lexeme.substr(0, core_len);
    }
  }
  return lexeme;
}

static bool DigitValue(char c, unsigned base, unsigned* out) {
  if (c >= '0' && c <= '9') {
    unsigned digit = static_cast<unsigned>(c - '0');
    if (digit < base) {
      *out = digit;
      return true;
    }
    return false;
  }
  if (base <= 10) {
    return false;
  }
  if (c >= 'a' && c <= 'f') {
    unsigned digit = static_cast<unsigned>(10 + (c - 'a'));
    if (digit < base) {
      *out = digit;
      return true;
    }
    return false;
  }
  if (c >= 'A' && c <= 'F') {
    unsigned digit = static_cast<unsigned>(10 + (c - 'A'));
    if (digit < base) {
      *out = digit;
      return true;
    }
    return false;
  }
  return false;
}

static bool ParseIntCore(std::string_view core,
                         core::UInt128& value_out) {
  unsigned base = 10;
  std::string_view digits = core;
  if (core.size() >= 2 && core[0] == '0') {
    const char prefix = core[1];
    if (prefix == 'x' || prefix == 'X') {
      base = 16;
      digits = core.substr(2);
    } else if (prefix == 'o' || prefix == 'O') {
      base = 8;
      digits = core.substr(2);
    } else if (prefix == 'b' || prefix == 'B') {
      base = 2;
      digits = core.substr(2);
    }
  }
  if (digits.empty()) {
    return false;
  }
  core::UInt128 value = core::UInt128FromU64(0);
  const core::UInt128 max = core::UInt128Max();
  const core::UInt128 base128 = core::UInt128FromU64(base);
  bool saw_digit = false;
  for (char c : digits) {
    if (c == '_') {
      continue;
    }
    unsigned digit = 0;
    if (!DigitValue(c, base, &digit)) {
      return false;
    }
    saw_digit = true;
    const core::UInt128 digit128 = core::UInt128FromU64(digit);
    // Check for overflow: value > (max - digit) / base
    const core::UInt128 max_minus_digit = core::UInt128Sub(max, digit128);
    const core::UInt128 threshold = core::UInt128Div(max_minus_digit, base128);
    if (core::UInt128Greater(value, threshold)) {
      return false;
    }
    value = core::UInt128Add(core::UInt128Mul(value, base128), digit128);
  }
  if (!saw_digit) {
    return false;
  }
  value_out = value;
  return true;
}

static std::optional<core::UInt128> ParseTupleIndex(
    const syntax::Token& token) {
  if (token.kind != syntax::TokenKind::IntLiteral) {
    return std::nullopt;
  }
  const std::string_view core = StripIntSuffix(token.lexeme);
  core::UInt128 value = core::UInt128FromU64(0);
  if (!ParseIntCore(core, value)) {
    return std::nullopt;
  }
  return value;
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

}  // namespace

ExprTypeResult TypeTupleExpr(const ScopeContext& ctx,
                             const syntax::TupleExpr& expr,
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
                                    const syntax::TupleAccessExpr& expr,
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
  const auto* tuple = stripped ? std::get_if<TypeTuple>(&stripped->node) : nullptr;
  if (!tuple) {
    SPEC_RULE("TupleAccess-NotTuple");
    result.diag_id = "TupleAccess-NotTuple";
    return result;
  }

  const auto idx_value = ParseTupleIndex(expr.index);
  if (!idx_value.has_value()) {
    SPEC_RULE("TupleIndex-NonConst");
    result.diag_id = "TupleIndex-NonConst";
    return result;
  }

  // Check index is within bounds
  const core::UInt128 size_as_uint128 = core::UInt128FromU64(tuple->elements.size());
  if (!core::UInt128FitsU64(*idx_value) ||
      core::UInt128ToU64(*idx_value) >= tuple->elements.size()) {
    SPEC_RULE("TupleIndex-OOB");
    result.diag_id = "TupleIndex-OOB";
    return result;
  }

  const auto index = static_cast<std::size_t>(core::UInt128ToU64(*idx_value));
  const auto element = tuple->elements[index];
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
                                     const syntax::TupleAccessExpr& expr,
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
  const auto* tuple = stripped ? std::get_if<TypeTuple>(&stripped->node) : nullptr;
  if (!tuple) {
    SPEC_RULE("TupleAccess-NotTuple");
    result.diag_id = "TupleAccess-NotTuple";
    return result;
  }

  const auto idx_value = ParseTupleIndex(expr.index);
  if (!idx_value.has_value()) {
    SPEC_RULE("TupleIndex-NonConst");
    result.diag_id = "TupleIndex-NonConst";
    return result;
  }

  // Check index is within bounds
  if (!core::UInt128FitsU64(*idx_value) ||
      core::UInt128ToU64(*idx_value) >= tuple->elements.size()) {
    SPEC_RULE("TupleIndex-OOB");
    result.diag_id = "TupleIndex-OOB";
    return result;
  }

  const auto index = static_cast<std::size_t>(core::UInt128ToU64(*idx_value));
  const auto element = tuple->elements[index];
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

}  // namespace cursive0::analysis
