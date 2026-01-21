#pragma once

#include <optional>
#include <string_view>
#include <vector>

#include "cursive0/sema/types.h"
#include "cursive0/semantics/value.h"
#include "cursive0/semantics/state.h"

namespace cursive0::semantics {

bool IsRegionProcName(std::string_view name);

std::optional<Value> BuiltinCall(const sema::TypePath& module_path,
                                 std::string_view name,
                                 const std::vector<BindingValue>& args,
                                 Sigma& sigma);

std::optional<Value> PrimCall(const sema::TypePath& owner,
                              std::string_view name,
                              const Value& self,
                              const std::vector<Value>& args,
                              Sigma& sigma);

}  // namespace cursive0::semantics
