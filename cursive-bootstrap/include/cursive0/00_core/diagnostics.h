#pragma once

#include <optional>
#include <string>
#include <vector>

#include "cursive0/00_core/span.h"

namespace cursive0::core {

using DiagCode = std::string;

enum class Severity {
  Error,
  Warning,
  Info,
  Panic,
};

struct Diagnostic {
  DiagCode code;
  Severity severity = Severity::Error;
  std::string message;
  std::optional<Span> span;
};

using DiagnosticStream = std::vector<Diagnostic>;

enum class CompileStatusResult {
  Ok,
  Fail,
};

DiagnosticStream Emit(const DiagnosticStream& stream, const Diagnostic& diag);

bool HasError(const DiagnosticStream& stream);

CompileStatusResult CompileStatus(const DiagnosticStream& stream);

}  // namespace cursive0::core
