// =================================================================
// File: 03_analysis/types/expr/deref.h
// Construct: Dereference Expression Type Checking
// Spec Section: 5.2.12
// Spec Rules: T-Deref-Ptr, T-Deref-Raw, P-Deref-Ptr, P-Deref-Raw-Imm,
//             P-Deref-Raw-Mut, Deref-Null, Deref-Expired, Deref-Raw-Unsafe
// =================================================================
#pragma once

#include "cursive0/00_core/span.h"
#include "cursive0/03_analysis/types/context.h"
#include "cursive0/03_analysis/types/place_types.h"
#include "cursive0/03_analysis/types/type_infer.h"
#include "cursive0/03_analysis/types/type_stmt.h"
#include "cursive0/02_syntax/ast.h"

namespace cursive0::analysis::expr {

// §5.2.12 Dereference Expression Typing (value context)
ExprTypeResult TypeDerefExprImpl(const ScopeContext& ctx,
                                 const StmtTypeContext& type_ctx,
                                 const syntax::DerefExpr& expr,
                                 const TypeEnv& env,
                                 const core::Span& span);

// §5.2.12 Dereference Place Typing (place context)
PlaceTypeResult TypeDerefPlaceImpl(const ScopeContext& ctx,
                                   const StmtTypeContext& type_ctx,
                                   const syntax::DerefExpr& expr,
                                   const TypeEnv& env,
                                   const core::Span& span);

}  // namespace cursive0::analysis::expr
