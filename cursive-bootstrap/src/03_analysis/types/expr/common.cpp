// =================================================================
// File: 03_analysis/types/expr/common.cpp
// Construct: Common utilities for expression type checking
// Spec Section: 5.2.12
// =================================================================
#include "cursive0/03_analysis/types/expr/common.h"

#include <algorithm>

namespace cursive0::analysis::expr {

// Forward declaration for StripPerm (defined in type_expr.cpp, declared in type_expr.h)
// We use the one from analysis namespace
namespace {
TypeRef StripPermLocal(const TypeRef& type) {
  if (!type) {
    return nullptr;
  }
  if (const auto* perm = std::get_if<TypePerm>(&type->node)) {
    return perm->base;
  }
  return type;
}
}  // namespace

// Type sets (§5.2.12)
const std::array<std::string_view, 12> kIntTypes = {
    "i8", "i16", "i32", "i64", "i128", "isize",
    "u8", "u16", "u32", "u64", "u128", "usize"};

const std::array<std::string_view, 6> kSignedIntTypes = {
    "i8", "i16", "i32", "i64", "i128", "isize"};

const std::array<std::string_view, 3> kFloatTypes = {"f16", "f32", "f64"};

bool IsIntType(std::string_view name) {
  return std::find(kIntTypes.begin(), kIntTypes.end(), name) != kIntTypes.end();
}

bool IsSignedIntType(std::string_view name) {
  return std::find(kSignedIntTypes.begin(), kSignedIntTypes.end(), name) !=
         kSignedIntTypes.end();
}

bool IsFloatType(std::string_view name) {
  return std::find(kFloatTypes.begin(), kFloatTypes.end(), name) !=
         kFloatTypes.end();
}

bool IsNumericType(std::string_view name) {
  return IsIntType(name) || IsFloatType(name);
}

bool IsPrimTypeName(std::string_view name) {
  return IsIntType(name) || IsFloatType(name) || name == "bool" ||
         name == "char" || name == "()" || name == "!";
}

// Operator classification (§5.2.12)
bool IsArithOp(std::string_view op) {
  return op == "+" || op == "-" || op == "*" || op == "/" || op == "%" ||
         op == "**";
}

bool IsBitOp(std::string_view op) {
  return op == "&" || op == "|" || op == "^";
}

bool IsShiftOp(std::string_view op) {
  return op == "<<" || op == ">>";
}

bool IsEqOp(std::string_view op) { return op == "==" || op == "!="; }

bool IsOrdOp(std::string_view op) {
  return op == "<" || op == "<=" || op == ">" || op == ">=";
}

bool IsLogicOp(std::string_view op) { return op == "&&" || op == "||"; }

// Helper to extract primitive type name
std::optional<std::string> GetPrimName(const TypeRef& type) {
  if (!type) {
    return std::nullopt;
  }
  const auto stripped = StripPermLocal(type);
  if (!stripped) {
    return std::nullopt;
  }
  const auto* prim = std::get_if<TypePrim>(&stripped->node);
  if (!prim) {
    return std::nullopt;
  }
  return prim->name;
}

}  // namespace cursive0::analysis::expr
