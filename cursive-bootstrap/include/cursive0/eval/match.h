#pragma once

#include <map>
#include <optional>
#include <string>
#include <vector>

#include "cursive0/eval/state.h"
#include "cursive0/syntax/ast.h"

namespace cursive0::eval {

using BindEnv = std::map<std::string, Value>;

std::optional<BindEnv> MatchPattern(const SemanticsContext& ctx,
                                    const syntax::Pattern& pattern,
                                    const Value& value);

std::optional<BindEnv> MatchRecord(const SemanticsContext& ctx,
                                   const std::vector<syntax::FieldPattern>& fields,
                                   const Value& value);

std::optional<BindEnv> MatchModal(const SemanticsContext& ctx,
                                  const syntax::ModalPattern& pattern,
                                  const Value& value);

}  // namespace cursive0::eval
