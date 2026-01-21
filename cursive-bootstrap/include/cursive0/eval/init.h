#pragma once

#include <set>

#include "cursive0/eval/state.h"
#include "cursive0/eval/cleanup.h"

namespace cursive0::eval {

struct InitResult {
  bool ok = false;
  std::set<analysis::PathKey> poisoned;
};

InitResult Init(const SemanticsContext& ctx, Sigma& sigma);
CleanupStatus Deinit(const SemanticsContext& ctx, Sigma& sigma);

}  // namespace cursive0::eval
