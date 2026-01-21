#include <cassert>
#include <string>
#include <utility>
#include <vector>

#include "cursive0/core/assert_spec.h"
#include "cursive0/eval/exec.h"
#include "cursive0/eval/state.h"
#include "cursive0/eval/value.h"

int main() {
  SPEC_COV("BindList-Cons");
  SPEC_COV("BindList-Empty");

  cursive0::eval::Sigma sigma;
  cursive0::eval::PushScope(sigma, nullptr);

  cursive0::eval::BindInfo info;
  cursive0::eval::BindListInput empty;
  const auto empty_out = cursive0::eval::BindList(sigma, empty, info);
  assert(empty_out.ok);
  assert(empty_out.bindings.empty());

  cursive0::eval::BindListInput binds;
  binds.emplace_back("x", cursive0::eval::Value{cursive0::eval::UnitVal{}});
  const auto out = cursive0::eval::BindList(sigma, binds, info);
  assert(out.ok);
  assert(out.bindings.size() == 1);

  return 0;
}
