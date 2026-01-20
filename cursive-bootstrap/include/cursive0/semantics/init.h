#pragma once

#include <set>

#include "cursive0/semantics/state.h"
#include "cursive0/semantics/cleanup.h"

namespace cursive0::semantics {

struct InitResult {
  bool ok = false;
  std::set<sema::PathKey> poisoned;
};

InitResult Init(const SemanticsContext& ctx, Sigma& sigma);
CleanupStatus Deinit(const SemanticsContext& ctx, Sigma& sigma);

}  // namespace cursive0::semantics
