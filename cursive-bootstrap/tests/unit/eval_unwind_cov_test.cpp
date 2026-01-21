#include <cassert>
#include <vector>

#include "cursive0/core/assert_spec.h"
#include "cursive0/eval/cleanup.h"
#include "cursive0/eval/state.h"

namespace {

using cursive0::eval::CleanupStatus;
using cursive0::eval::ScopeEntry;
using cursive0::eval::Sigma;
using cursive0::eval::SemanticsContext;

}  // namespace

namespace cursive0::eval {
CleanupStatus Unwind(const SemanticsContext& ctx,
                     const std::vector<ScopeEntry>& frames,
                     Sigma& sigma);
}

int main() {
  SPEC_COV("Unwind-Abort");
  SPEC_COV("Unwind-Step");

  {
    Sigma sigma;
    cursive0::eval::PushScope(sigma, nullptr);
    SemanticsContext ctx;
    const auto* scope = cursive0::eval::CurrentScope(sigma);
    assert(scope);
    std::vector<ScopeEntry> frames{*scope};
    const auto status = Unwind(ctx, frames, sigma);
    assert(status == CleanupStatus::Ok);
  }

  {
    Sigma sigma;
    SemanticsContext ctx;
    ScopeEntry fake;
    fake.id = 999;
    std::vector<ScopeEntry> frames{fake};
    const auto status = Unwind(ctx, frames, sigma);
    assert(status == CleanupStatus::Abort);
  }

  return 0;
}
