#pragma once

#include <optional>
#include <string_view>
#include <vector>

#include "cursive0/analysis/types/types.h"
#include "cursive0/eval/value.h"
#include "cursive0/eval/state.h"

namespace cursive0::eval {

bool IsRegionProcName(std::string_view name);

std::optional<Value> BuiltinCall(const analysis::TypePath& module_path,
                                 std::string_view name,
                                 const std::vector<BindingValue>& args,
                                 Sigma& sigma);

std::optional<Value> PrimCall(const analysis::TypePath& owner,
                              std::string_view name,
                              const Value& self,
                              const std::vector<Value>& args,
                              Sigma& sigma);

}  // namespace cursive0::eval
