#pragma once

#include <optional>
#include <string>
#include <unordered_map>

#include "cursive0/core/diagnostics.h"

namespace cursive0::core {

using DiagId = std::string;
using DiagCodeMap = std::unordered_map<DiagId, DiagCode>;

std::optional<DiagCode> SpecCode(const DiagCodeMap& spec_map, const DiagId& id);
std::optional<DiagCode> C0Code(const DiagCodeMap& c0_map, const DiagId& id);
std::optional<DiagCode> Code(const DiagCodeMap& spec_map,
                             const DiagCodeMap& c0_map,
                             const DiagId& id);

}  // namespace cursive0::core
