#undef NDEBUG
#include <cassert>

#include "cursive0/core/assert_spec.h"
#include "cursive0/eval/state.h"

int main() {
  SPEC_COV("LookupVal-Bind-Value");
  SPEC_COV("LookupVal-Bind-Alias");

  cursive0::eval::Sigma sigma;
  cursive0::eval::SemanticsContext ctx;

  cursive0::eval::ScopeEntry scope;
  assert(cursive0::eval::PushScope(sigma, &scope));

  cursive0::eval::BindInfo info;
  cursive0::eval::Binding binding;
  cursive0::eval::Value value{cursive0::eval::BoolVal{true}};
  assert(cursive0::eval::BindVal(sigma, "x", value, info, &binding));

  auto got = cursive0::eval::LookupVal(ctx, sigma, "x");
  assert(got.has_value());
  assert(cursive0::eval::ValueEqual(*got, value));

  const auto addr = cursive0::eval::AllocAddr(sigma);
  sigma.store[addr] = cursive0::eval::Value{cursive0::eval::CharVal{65}};

  cursive0::eval::Binding alias_binding;
  assert(cursive0::eval::BindVal(sigma,
                                      "y",
                                      cursive0::eval::Value{cursive0::eval::UnitVal{}},
                                      info,
                                      &alias_binding));
  auto* current = cursive0::eval::CurrentScope(sigma);
  assert(current);
  current->vals[alias_binding.bind_id] = cursive0::eval::Alias{addr};

  auto alias_value = cursive0::eval::LookupVal(ctx, sigma, "y");
  assert(alias_value.has_value());
  assert(cursive0::eval::ValueEqual(*alias_value, sigma.store[addr]));

  return 0;
}
