#include "cursive0/00_core/diagnostic_messages.h"

#include <algorithm>
#include <string>

#include "cursive0/00_core/assert_spec.h"

namespace cursive0::core {

namespace {

struct DiagMessageEntry {
  const char* code;
  Severity severity;
  const char* message;
};

#include "diagnostic_messages_table.inc"

static void SpecDefsDiagMessages() {
  SPEC_DEF("DiagId-Code-Map", "8.0");
  SPEC_DEF("SeverityColumn", "8.0");
  SPEC_DEF("ConditionColumn", "8.0");
  SPEC_DEF("Message", "1.6.3");
}

static const DiagMessageEntry* FindEntry(std::string_view code) {
  const auto begin = std::begin(kDiagMessages);
  const auto end = std::end(kDiagMessages);
  auto it = std::lower_bound(
      begin,
      end,
      code,
      [](const DiagMessageEntry& entry, std::string_view value) {
        return std::string_view(entry.code) < value;
      });
  if (it == end) {
    return nullptr;
  }
  if (code == std::string_view(it->code)) {
    return &(*it);
  }
  return nullptr;
}

}  // namespace

std::optional<std::string_view> MessageForCode(std::string_view code,
                                               MessageLocale locale) {
  SpecDefsDiagMessages();
  (void)locale;
  const auto* entry = FindEntry(code);
  if (!entry) {
    return std::nullopt;
  }
  return std::string_view(entry->message);
}

std::optional<Severity> SeverityForCode(std::string_view code,
                                        MessageLocale locale) {
  SpecDefsDiagMessages();
  (void)locale;
  const auto* entry = FindEntry(code);
  if (!entry) {
    return std::nullopt;
  }
  return entry->severity;
}

std::string FormatMessage(std::string_view message_template,
                          const std::vector<MessageArg>& args) {
  if (message_template.find('{') == std::string_view::npos) {
    return std::string(message_template);
  }

  std::string out;
  out.reserve(message_template.size());

  size_t i = 0;
  while (i < message_template.size()) {
    if (message_template[i] != '{') {
      out.push_back(message_template[i]);
      ++i;
      continue;
    }

    const size_t close = message_template.find('}', i + 1);
    if (close == std::string_view::npos) {
      out.append(message_template.substr(i));
      break;
    }

    const std::string_view key =
        message_template.substr(i + 1, close - i - 1);
    const auto it = std::find_if(args.begin(), args.end(),
                                 [key](const MessageArg& arg) {
                                   return arg.key == key;
                                 });
    if (it == args.end()) {
      out.append(message_template.substr(i, close - i + 1));
    } else {
      out.append(it->value);
    }
    i = close + 1;
  }

  return out;
}

std::optional<Diagnostic> MakeDiagnostic(std::string_view code,
                                         std::optional<Span> span,
                                         MessageLocale locale) {
  SpecDefsDiagMessages();
  (void)locale;
  const auto* entry = FindEntry(code);
  if (!entry) {
    return std::nullopt;
  }
  Diagnostic diag;
  diag.code = std::string(entry->code);
  diag.severity = entry->severity;
  diag.message = entry->message;
  diag.span = span;
  return diag;
}



}  // namespace cursive0::core
