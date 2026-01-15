#pragma once

#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

#include "cursive0/core/diagnostics.h"
#include "cursive0/core/source_text.h"

namespace cursive0::core {

struct SourceLoadResult {
  std::optional<SourceFile> source;
  DiagnosticStream diags;
};

SourceLoadResult LoadSource(std::string_view path,
                            const std::vector<std::uint8_t>& bytes);

}  // namespace cursive0::core
