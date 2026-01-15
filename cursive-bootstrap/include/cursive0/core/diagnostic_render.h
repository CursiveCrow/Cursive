#pragma once

#include <string>

#include "cursive0/core/diagnostics.h"

namespace cursive0::core {

enum class DiagnosticOrigin {
  Internal,
  External,
};

DiagnosticStream Order(const DiagnosticStream& stream);

Diagnostic ApplyNoSpanExternal(const Diagnostic& diag, DiagnosticOrigin origin);

std::string Render(const Diagnostic& diag);

}  // namespace cursive0::core
