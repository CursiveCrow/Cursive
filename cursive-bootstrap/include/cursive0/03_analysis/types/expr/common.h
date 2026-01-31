// =================================================================
// File: 03_analysis/types/expr/common.h
// Construct: Common utilities for expression type checking
// Spec Section: 5.2.12
// =================================================================
#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "cursive0/03_analysis/types/context.h"
#include "cursive0/03_analysis/types/place_types.h"
#include "cursive0/03_analysis/types/type_infer.h"
#include "cursive0/03_analysis/types/type_stmt.h"
#include "cursive0/03_analysis/types/types.h"
#include "cursive0/02_syntax/ast.h"

namespace cursive0::analysis::expr {

// Type sets for numeric type checking (§5.2.12)
extern const std::array<std::string_view, 12> kIntTypes;
extern const std::array<std::string_view, 6> kSignedIntTypes;
extern const std::array<std::string_view, 3> kFloatTypes;

// Type predicates
bool IsIntType(std::string_view name);
bool IsSignedIntType(std::string_view name);
bool IsFloatType(std::string_view name);
bool IsNumericType(std::string_view name);
bool IsPrimTypeName(std::string_view name);

// Operator classification (§5.2.12)
bool IsArithOp(std::string_view op);
bool IsBitOp(std::string_view op);
bool IsShiftOp(std::string_view op);
bool IsEqOp(std::string_view op);
bool IsOrdOp(std::string_view op);
bool IsLogicOp(std::string_view op);

// Helper to extract primitive type name
std::optional<std::string> GetPrimName(const TypeRef& type);

// Type function aliases for expression typing callbacks
using TypeExprFn = std::function<ExprTypeResult(const syntax::ExprPtr&)>;
using PlaceTypeFn = std::function<PlaceTypeResult(const syntax::ExprPtr&)>;
using TypeIdentFn = std::function<ExprTypeResult(std::string_view)>;

}  // namespace cursive0::analysis::expr
