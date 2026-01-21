#pragma once

#include "cursive0/analysis/types/context.h"
#include "cursive0/analysis/types/type_stmt.h"
#include "cursive0/analysis/types/types.h"
#include "cursive0/syntax/ast.h"

namespace cursive0::analysis {

PatternTypeResult TypeMatchPattern(const ScopeContext& ctx,
                                   const syntax::PatternPtr& pattern,
                                   const TypeRef& expected);

bool IrrefutablePattern(const ScopeContext& ctx,
                        const syntax::PatternPtr& pattern,
                        const TypeRef& expected);

}  // namespace cursive0::analysis
