#pragma once

#include <cstddef>
#include <utility>
#include <vector>

#include "cursive0/core/diagnostics.h"
#include "cursive0/analysis/types/context.h"
#include "cursive0/analysis/resolve/scopes_lookup.h"
#include "cursive0/syntax/ast.h"

namespace cursive0::analysis {

struct InitGraph {
  std::vector<syntax::ModulePath> modules;
  std::vector<std::pair<std::size_t, std::size_t>> type_edges;
  std::vector<std::pair<std::size_t, std::size_t>> eager_edges;
  std::vector<std::pair<std::size_t, std::size_t>> lazy_edges;
};

struct InitPlan {
  InitGraph graph;
  std::vector<syntax::ModulePath> init_order;
  bool topo_ok = false;
};

struct InitPlanResult {
  bool ok = false;
  core::DiagnosticStream diags;
  InitPlan plan;
};

InitPlanResult BuildInitPlan(const ScopeContext& ctx,
                             const NameMapTable& name_maps);

}  // namespace cursive0::analysis
