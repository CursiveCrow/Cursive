#include "cursive0/analysis/types/literals.h"

#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>

#include "cursive0/core/assert_spec.h"
#include "cursive0/core/int128.h"

namespace cursive0::analysis {

namespace {

static inline void SpecDefsLiterals() {
  SPEC_DEF("IntTypes", "5.2.10");
  SPEC_DEF("FloatTypes", "5.2.10");
  SPEC_DEF("DefaultInt", "5.2.10");
  SPEC_DEF("DefaultFloat", "5.2.10");
  SPEC_DEF("StripIntSuffix", "5.2.10");
  SPEC_DEF("StripFloatSuffix", "5.2.10");
  SPEC_DEF("IntValue", "5.2.10");
  SPEC_DEF("FloatValue", "5.2.10");
  SPEC_DEF("InRange", "5.2.10");
  SPEC_DEF("RangeOf", "5.2.10");
  SPEC_DEF("NullLiteralExpected", "5.2.10");
}

static constexpr unsigned kPointerSizeBytes = 8;
static constexpr unsigned kPointerSizeBits = 8 * kPointerSizeBytes;

static constexpr std::array<std::string_view, 12> kIntSuffixes = {
    "i128", "u128", "isize", "usize", "i64", "u64",
    "i32",  "u32",  "i16",   "u16",  "i8",  "u8"};

static constexpr std::array<std::string_view, 4> kFloatSuffixes = {
    "f16", "f32", "f64", "f"};

static constexpr std::array<std::string_view, 12> kIntTypes = {
    "i8",   "i16",  "i32",  "i64",  "i128", "u8",
    "u16",  "u32",  "u64",  "u128", "isize", "usize"};

static constexpr std::array<std::string_view, 3> kFloatTypes = {
    "f16", "f32", "f64"};

static bool EndsWith(std::string_view value, std::string_view suffix) {
  if (suffix.size() > value.size()) {
    return false;
  }
  return value.substr(value.size() - suffix.size()) == suffix;
}

static std::optional<std::string_view> MatchSuffix(
    std::string_view lexeme,
    std::span<const std::string_view> suffixes) {
  for (const auto suffix : suffixes) {
    if (EndsWith(lexeme, suffix)) {
      const std::size_t core_len = lexeme.size() - suffix.size();
      if (core_len == 0) {
        continue;
      }
      return suffix;
    }
  }
  return std::nullopt;
}

static std::optional<std::string_view> IntSuffix(const syntax::Token& lit) {
  if (lit.kind != syntax::TokenKind::IntLiteral) {
    return std::nullopt;
  }
  return MatchSuffix(lit.lexeme, kIntSuffixes);
}

static std::optional<std::string_view> FloatSuffix(const syntax::Token& lit) {
  if (lit.kind != syntax::TokenKind::FloatLiteral) {
    return std::nullopt;
  }
  return MatchSuffix(lit.lexeme, kFloatSuffixes);
}

static std::string_view IntCore(const syntax::Token& lit) {
  const auto suffix = IntSuffix(lit);
  if (!suffix.has_value()) {
    return lit.lexeme;
  }
  return std::string_view(lit.lexeme).substr(0,
                                             lit.lexeme.size() - suffix->size());
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

static std::optional<core::UInt128> IntValue(const syntax::Token& lit) {
  if (lit.kind != syntax::TokenKind::IntLiteral) {
    return std::nullopt;
  }
  const auto core = IntCore(lit);
  core::UInt128 value = core::UInt128FromU64(0);
  if (!ParseIntCore(core, value)) {
    return std::nullopt;
  }
  return value;
}

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

static std::optional<unsigned> IntWidthOf(std::string_view name) {
  if (name == "i8" || name == "u8") {
    return 8;
  }
  if (name == "i16" || name == "u16") {
    return 16;
  }
  if (name == "i32" || name == "u32") {
    return 32;
  }
  if (name == "i64" || name == "u64") {
    return 64;
  }
  if (name == "i128" || name == "u128") {
    return 128;
  }
  if (name == "isize" || name == "usize") {
    return kPointerSizeBits;
  }
  return std::nullopt;
}

static bool IsUnsignedIntType(std::string_view name) {
  return name == "u8" || name == "u16" || name == "u32" ||
         name == "u64" || name == "u128" || name == "usize";
}

static bool IsSignedIntType(std::string_view name) {
  return name == "i8" || name == "i16" || name == "i32" ||
         name == "i64" || name == "i128" || name == "isize";
}

static bool InRangeUnsigned(core::UInt128 value, unsigned width) {
  if (width >= 128) {
    return true;
  }
  const core::UInt128 one = core::UInt128FromU64(1);
  const core::UInt128 max =
      core::UInt128Sub(core::UInt128ShiftLeft(one, width), one);
  return core::UInt128LessOrEqual(value, max);
}

static bool InRangeSigned(core::UInt128 value, unsigned width) {
  if (width == 0) {
    return false;
  }
  if (width >= 128) {
    const core::UInt128 one = core::UInt128FromU64(1);
    const core::UInt128 max =
        core::UInt128Sub(core::UInt128ShiftLeft(one, 127), one);
    return core::UInt128LessOrEqual(value, max);
  }
  const core::UInt128 one = core::UInt128FromU64(1);
  const core::UInt128 max =
      core::UInt128Sub(core::UInt128ShiftLeft(one, width - 1), one);
  return core::UInt128LessOrEqual(value, max);
}

static bool InRangeInt(core::UInt128 value, std::string_view name) {
  const auto width = IntWidthOf(name);
  if (!width.has_value()) {
    return false;
  }
  if (IsUnsignedIntType(name)) {
    return InRangeUnsigned(value, *width);
  }
  if (IsSignedIntType(name)) {
    return InRangeSigned(value, *width);
  }
  return false;
}

}  // namespace

ExprTypeResult TypeLiteralExpr(const ScopeContext& ctx,
                               const syntax::LiteralExpr& expr) {
  (void)ctx;
  SpecDefsLiterals();
  ExprTypeResult result;
  const auto& lit = expr.literal;
  switch (lit.kind) {
    case syntax::TokenKind::IntLiteral: {
      const auto value = IntValue(lit);
      if (!value.has_value()) {
        return result;
      }
      if (const auto suffix = IntSuffix(lit)) {
        if (!InRangeInt(*value, *suffix)) {
          return result;
        }
        SPEC_RULE("T-Int-Literal-Suffix");
        result.ok = true;
        result.type = MakeTypePrim(std::string(*suffix));
        return result;
      }
      if (!InRangeInt(*value, "i32")) {
        return result;
      }
      SPEC_RULE("T-Int-Literal-Default");
      result.ok = true;
      result.type = MakeTypePrim("i32");
      return result;
    }
    case syntax::TokenKind::FloatLiteral: {
      if (const auto suffix = FloatSuffix(lit)) {
        // Bare 'f' suffix defaults to f32
        if (*suffix == "f") {
          SPEC_RULE("T-Float-Literal-Infer");
          result.ok = true;
          result.type = MakeTypePrim("f32");
          return result;
        }
        // Explicit width suffix (f16, f32, f64)
        SPEC_RULE("T-Float-Literal-Explicit");
        result.ok = true;
        result.type = MakeTypePrim(std::string(*suffix));
        return result;
      }
      // Suffix is required, but if somehow missing, default to f32
      SPEC_RULE("T-Float-Literal-Infer");
      result.ok = true;
      result.type = MakeTypePrim("f32");
      return result;
    }
    case syntax::TokenKind::BoolLiteral:
      SPEC_RULE("T-Bool-Literal");
      result.ok = true;
      result.type = MakeTypePrim("bool");
      return result;
    case syntax::TokenKind::CharLiteral:
      SPEC_RULE("T-Char-Literal");
      result.ok = true;
      result.type = MakeTypePrim("char");
      return result;
    case syntax::TokenKind::StringLiteral:
      SPEC_RULE("T-String-Literal");
      result.ok = true;
      result.type = MakeTypeString(StringState::View);
      return result;
    case syntax::TokenKind::NullLiteral:
    case syntax::TokenKind::Identifier:
    case syntax::TokenKind::Keyword:
    case syntax::TokenKind::Operator:
    case syntax::TokenKind::Punctuator:
    case syntax::TokenKind::Newline:
    case syntax::TokenKind::Unknown:
      break;
  }
  return result;
}

bool NullLiteralExpected(const TypeRef& expected) {
  SpecDefsLiterals();
  if (!expected) {
    return false;
  }
  TypeRef cur = expected;
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

LiteralCheckResult CheckLiteralExpr(const ScopeContext& ctx,
                                    const syntax::LiteralExpr& expr,
                                    const TypeRef& expected) {
  (void)ctx;
  SpecDefsLiterals();
  LiteralCheckResult result;
  if (!expected) {
    return result;
  }
  TypeRef base = expected;
  while (base) {
    if (const auto* perm = std::get_if<TypePerm>(&base->node)) {
      base = perm->base;
      continue;
    }
    if (const auto* refine = std::get_if<TypeRefine>(&base->node)) {
      base = refine->base;
      continue;
    }
    break;
  }
  if (!base) {
    return result;
  }
  const auto& lit = expr.literal;
  if (lit.kind == syntax::TokenKind::IntLiteral) {
    const auto* prim = std::get_if<TypePrim>(&base->node);
    if (!prim || !IsIntTypeName(prim->name)) {
      return result;
    }
    const auto value = IntValue(lit);
    if (!value.has_value() || !InRangeInt(*value, prim->name)) {
      return result;
    }
    SPEC_RULE("Chk-Int-Literal");
    result.ok = true;
    return result;
  }
  if (lit.kind == syntax::TokenKind::FloatLiteral) {
    const auto* prim = std::get_if<TypePrim>(&base->node);
    if (!prim || !IsFloatTypeName(prim->name)) {
      return result;
    }
    const auto suffix = FloatSuffix(lit);
    // Bare 'f' suffix accepts any float type from context
    if (!suffix.has_value() || *suffix == "f") {
      SPEC_RULE("Chk-Float-Literal-Infer");
      result.ok = true;
      return result;
    }
    // Explicit suffix must match expected type
    if (*suffix == prim->name) {
      SPEC_RULE("Chk-Float-Literal-Explicit");
      result.ok = true;
      return result;
    }
    // Explicit suffix mismatch - error
    SPEC_RULE("Chk-Float-Literal-Mismatch-Err");
    result.diag_id = "E-TYP-1531";
    return result;
  }
  if (lit.kind == syntax::TokenKind::NullLiteral) {
    if (NullLiteralExpected(expected)) {
      SPEC_RULE("Chk-Null-Literal");
      result.ok = true;
      return result;
    }
    SPEC_RULE("Chk-Null-Literal-Err");
    result.diag_id = "NullLiteral-Infer-Err";
    return result;
  }
  return result;
}

}  // namespace cursive0::analysis
