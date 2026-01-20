#include <cassert>
#include <string>
#include <utility>
#include <vector>

#include "cursive0/core/assert_spec.h"
#include "cursive0/semantics/exec.h"
#include "cursive0/semantics/state.h"
#include "cursive0/semantics/value.h"

int main() {
  SPEC_COV("BindList-Cons");
  SPEC_COV("BindList-Empty");

  cursive0::semantics::Sigma sigma;
  cursive0::semantics::PushScope(sigma, nullptr);

  cursive0::semantics::BindInfo info;
  cursive0::semantics::BindListInput empty;
  const auto empty_out = cursive0::semantics::BindList(sigma, empty, info);
  assert(empty_out.ok);
  assert(empty_out.bindings.empty());

  cursive0::semantics::BindListInput binds;
  binds.emplace_back("x", cursive0::semantics::Value{cursive0::semantics::UnitVal{}});
  const auto out = cursive0::semantics::BindList(sigma, binds, info);
  assert(out.ok);
  assert(out.bindings.size() == 1);

  return 0;
}
