#pragma once

#include <vector>

#include "cursive0/core/diagnostics.h"
#include "cursive0/analysis/resolve/collect_toplevel.h"
#include "cursive0/analysis/types/context.h"
#include "cursive0/syntax/ast.h"

namespace cursive0::analysis {

struct DeclTypingResult {
  bool ok = false;
  core::DiagnosticStream diags;
};

DeclTypingResult DeclTypingModules(ScopeContext& ctx,
                                   const std::vector<syntax::ASTModule>& modules,
                                   const NameMapTable& name_maps);

DeclTypingResult MainCheckProject(ScopeContext& ctx,
                                  const std::vector<syntax::ASTModule>& modules);

}  // namespace cursive0::analysis
