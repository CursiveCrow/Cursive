#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "cursive0/core/diagnostics.h"
#include "cursive0/core/span.h"

namespace cursive0::core {

enum class MessageLocale {
  EnUS,
};

struct MessageArg {
  std::string_view key;
  std::string_view value;
};

std::optional<std::string_view> MessageForCode(
    std::string_view code,
    MessageLocale locale = MessageLocale::EnUS);

std::optional<Severity> SeverityForCode(
    std::string_view code,
    MessageLocale locale = MessageLocale::EnUS);

std::string FormatMessage(std::string_view message_template,
                          const std::vector<MessageArg>& args);

std::optional<Diagnostic> MakeDiagnostic(
    std::string_view code,
    std::optional<Span> span = std::nullopt,
    MessageLocale locale = MessageLocale::EnUS);


}  // namespace cursive0::core
