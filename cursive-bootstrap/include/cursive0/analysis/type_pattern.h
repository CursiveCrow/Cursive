#pragma once

#include "cursive0/sema/context.h"
#include "cursive0/sema/type_stmt.h"
#include "cursive0/sema/types.h"
#include "cursive0/syntax/ast.h"

namespace cursive0::sema {

PatternTypeResult TypeMatchPattern(const ScopeContext& ctx,
                                   const syntax::PatternPtr& pattern,
                                   const TypeRef& expected);

bool IrrefutablePattern(const ScopeContext& ctx,
                        const syntax::PatternPtr& pattern,
                        const TypeRef& expected);

}  // namespace cursive0::sema
