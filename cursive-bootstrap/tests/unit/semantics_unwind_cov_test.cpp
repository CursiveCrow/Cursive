#include <cassert>
#include <vector>

#include "cursive0/core/assert_spec.h"
#include "cursive0/semantics/cleanup.h"
#include "cursive0/semantics/state.h"

namespace {

using cursive0::semantics::CleanupStatus;
using cursive0::semantics::ScopeEntry;
using cursive0::semantics::Sigma;
using cursive0::semantics::SemanticsContext;

}  // namespace

namespace cursive0::semantics {
CleanupStatus Unwind(const SemanticsContext& ctx,
                     const std::vector<ScopeEntry>& frames,
                     Sigma& sigma);
}

int main() {
  SPEC_COV("Unwind-Abort");
  SPEC_COV("Unwind-Step");

  {
    Sigma sigma;
    cursive0::semantics::PushScope(sigma, nullptr);
    SemanticsContext ctx;
    const auto* scope = cursive0::semantics::CurrentScope(sigma);
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
