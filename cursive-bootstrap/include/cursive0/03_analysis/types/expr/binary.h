// =================================================================
// File: 03_analysis/types/expr/binary.h
// Construct: Binary Expression Type Checking
// Spec Section: 5.2.12
// Spec Rules: T-Arith, T-Bitwise, T-Shift, T-Compare-Eq, T-Compare-Ord, T-Logical
// =================================================================
#pragma once

#include "cursive0/03_analysis/types/context.h"
#include "cursive0/03_analysis/types/type_infer.h"
#include "cursive0/03_analysis/types/type_stmt.h"
#include "cursive0/02_syntax/ast.h"

namespace cursive0::analysis::expr {

// §5.2.12 Binary Expression Typing
// Handles: +, -, *, /, %, **, &, |, ^, <<, >>, ==, !=, <, <=, >, >=, &&, ||
ExprTypeResult TypeBinaryExprImpl(const ScopeContext& ctx,
                                  const StmtTypeContext& type_ctx,
                                  const syntax::BinaryExpr& expr,
                                  const TypeEnv& env);

}  // namespace cursive0::analysis::expr
