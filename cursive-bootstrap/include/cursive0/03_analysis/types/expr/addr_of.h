// =================================================================
// File: 03_analysis/types/expr/addr_of.h
// Construct: Address-Of Expression Type Checking
// Spec Section: 5.2.12
// Spec Rules: T-AddrOf, AddrOf-NonPlace, AddrOf-Index-Array-NonUsize,
//             AddrOf-Index-Slice-NonUsize
// =================================================================
#pragma once

#include "cursive0/03_analysis/types/context.h"
#include "cursive0/03_analysis/types/type_infer.h"
#include "cursive0/03_analysis/types/type_stmt.h"
#include "cursive0/02_syntax/ast.h"

namespace cursive0::analysis::expr {

// §5.2.12 Address-Of Expression Typing
ExprTypeResult TypeAddressOfExprImpl(const ScopeContext& ctx,
                                     const StmtTypeContext& type_ctx,
                                     const syntax::AddressOfExpr& expr,
                                     const TypeEnv& env);

}  // namespace cursive0::analysis::expr
