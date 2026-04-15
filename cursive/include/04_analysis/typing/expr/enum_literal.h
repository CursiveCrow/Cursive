// =================================================================
// File: 03_analysis/types/expr/enum_literal.h
// Construct: Enum Literal Expression Type Checking
// Spec Section: 5.2.12
// Spec Rules: T-Enum-Lit-Unit, T-Enum-Lit-Tuple, T-Enum-Lit-Record
// =================================================================
#pragma once

#include "04_analysis/typing/context.h"
#include "04_analysis/typing/type_infer.h"
#include "04_analysis/typing/type_stmt.h"
#include "02_source/ast/ast.h"

namespace cursive::analysis::expr {

// Type check an enum literal expression
ExprTypeResult TypeEnumLiteralExprImpl(const ScopeContext& ctx,
                                       const StmtTypeContext& type_ctx,
                                       const ast::EnumLiteralExpr& expr,
                                       const TypeEnv& env);

}  // namespace cursive::analysis::expr
