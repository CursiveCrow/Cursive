#undef NDEBUG
#include <cassert>

#include "cursive0/core/assert_spec.h"
#include "cursive0/semantics/state.h"

int main() {
  SPEC_COV("LookupVal-Bind-Value");
  SPEC_COV("LookupVal-Bind-Alias");

  cursive0::semantics::Sigma sigma;
  cursive0::semantics::SemanticsContext ctx;

  cursive0::semantics::ScopeEntry scope;
  assert(cursive0::semantics::PushScope(sigma, &scope));

  cursive0::semantics::BindInfo info;
  cursive0::semantics::Binding binding;
  cursive0::semantics::Value value{cursive0::semantics::BoolVal{true}};
  assert(cursive0::semantics::BindVal(sigma, "x", value, info, &binding));

  auto got = cursive0::semantics::LookupVal(ctx, sigma, "x");
  assert(got.has_value());
  assert(cursive0::semantics::ValueEqual(*got, value));

  const auto addr = cursive0::semantics::AllocAddr(sigma);
  sigma.store[addr] = cursive0::semantics::Value{cursive0::semantics::CharVal{65}};

  cursive0::semantics::Binding alias_binding;
  assert(cursive0::semantics::BindVal(sigma,
                                      "y",
                                      cursive0::semantics::Value{cursive0::semantics::UnitVal{}},
                                      info,
                                      &alias_binding));
  auto* current = cursive0::semantics::CurrentScope(sigma);
  assert(current);
  current->vals[alias_binding.bind_id] = cursive0::semantics::Alias{addr};

  auto alias_value = cursive0::semantics::LookupVal(ctx, sigma, "y");
  assert(alias_value.has_value());
  assert(cursive0::semantics::ValueEqual(*alias_value, sigma.store[addr]));

  return 0;
}
