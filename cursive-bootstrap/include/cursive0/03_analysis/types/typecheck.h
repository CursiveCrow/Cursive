#pragma once

#include <optional>
#include <string_view>
#include <vector>

#include "cursive0/00_core/diagnostics.h"
#include "cursive0/03_analysis/types/context.h"
#include "cursive0/03_analysis/memory/init_planner.h"
#include "cursive0/02_syntax/ast.h"

namespace cursive0::analysis {

struct TypecheckResult {
  bool ok = false;
  core::DiagnosticStream diags;
  ExprTypeMap expr_types;
  std::optional<InitPlan> init_plan;
};

TypecheckResult TypecheckModules(ScopeContext& ctx,
                                 const std::vector<syntax::ASTModule>& modules);

}  // namespace cursive0::analysis
