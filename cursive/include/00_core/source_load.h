#pragma once

#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

#include "00_core/diagnostics.h"
#include "00_core/source_text.h"

namespace cursive::core {

struct SourceLoadResult {
  std::optional<SourceFile> source;
  DiagnosticStream diags;
};

SourceLoadResult LoadSource(std::string_view path,
                            const std::vector<std::uint8_t>& bytes);

}  // namespace cursive::core
