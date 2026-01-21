#pragma once

#include <optional>
#include <string_view>
#include <vector>

#include "cursive0/analysis/memory/calls.h"
#include "cursive0/analysis/types/context.h"
#include "cursive0/analysis/types/types.h"
#include "cursive0/syntax/ast.h"

namespace cursive0::analysis {

struct RecordWfResult {
  bool ok = false;
  std::optional<std::string_view> diag_id;
};

RecordWfResult CheckRecordWf(const ScopeContext& ctx,
                             const syntax::RecordDecl& record,
                             const ExprTypeFn& type_expr);

ExprTypeResult TypeRecordDefaultCall(const ScopeContext& ctx,
                                     const syntax::ExprPtr& callee,
                                     const std::vector<syntax::Arg>& args,
                                     const ExprTypeFn& type_expr);

}  // namespace cursive0::analysis
