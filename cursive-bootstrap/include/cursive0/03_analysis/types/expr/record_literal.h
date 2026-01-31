// =================================================================
// File: 03_analysis/types/expr/record_literal.h
// Construct: Record Literal Expression Type Checking
// Spec Section: 5.2.12
// Spec Rules: T-Record-Literal, T-Modal-State-Intro, Record-FieldInit-Dup,
//             Record-Field-Unknown, Record-Field-NotVisible,
//             Record-FieldInit-Missing, Record-Field-NonBitcopy-Move
// =================================================================
#pragma once

#include "cursive0/03_analysis/types/context.h"
#include "cursive0/03_analysis/types/type_infer.h"
#include "cursive0/03_analysis/types/type_stmt.h"
#include "cursive0/02_syntax/ast.h"

namespace cursive0::analysis::expr {

// §5.2.12 Record Literal Expression Typing
ExprTypeResult TypeRecordExprImpl(const ScopeContext& ctx,
                                  const StmtTypeContext& type_ctx,
                                  const syntax::RecordExpr& expr,
                                  const TypeEnv& env);

}  // namespace cursive0::analysis::expr
