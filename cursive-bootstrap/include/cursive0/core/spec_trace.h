#pragma once

#include <optional>
#include <string>
#include <string_view>

#include "cursive0/core/span.h"

namespace cursive0::core {

class SpecTrace {
 public:
  static void Init(const std::string& path, std::string_view domain);
  static void SetRoot(const std::string& root);
  static void SetPhase(std::string_view phase);
  static void Record(std::string_view rule_id,
                     const std::optional<Span>& span,
                     std::string_view payload);
  static void Record(std::string_view rule_id);
  static bool Enabled();
};

}  // namespace cursive0::core
