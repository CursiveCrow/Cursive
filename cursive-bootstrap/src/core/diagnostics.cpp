#include "cursive0/core/diagnostics.h"

#include "cursive0/core/assert_spec.h"

namespace cursive0::core {

static inline void SpecDefsDiagnosticTypes() {
  SPEC_DEF("Severity", "1.6.3");
  SPEC_DEF("Diagnostic", "1.6.3");
  SPEC_DEF("DiagnosticStream", "1.6.3");
}

static std::string SeverityLabel(Severity severity) {
  if (severity == Severity::Warning) {
    return "warning";
  }
  return "error";
}

static std::string DiagPayload(const Diagnostic& diag) {
  std::string payload;
  payload.reserve(diag.code.size() + diag.message.size() + 32);
  payload += "code=";
  payload += diag.code;
  payload += ";severity=";
  payload += SeverityLabel(diag.severity);
  payload += ";message=";
  payload += diag.message;
  return payload;
}

DiagnosticStream Emit(const DiagnosticStream& stream, const Diagnostic& diag) {
  SPEC_RULE("Emit-Append");
  SpecDefsDiagnosticTypes();
  if (SpecTrace::Enabled()) {
    const std::string payload = DiagPayload(diag);
    SpecTrace::Record("Diag-Emit", diag.span, payload);
  }
  DiagnosticStream out = stream;
  out.push_back(diag);
  return out;
}

bool HasError(const DiagnosticStream& stream) {
  SPEC_DEF("HasError", "1.6.3");
  SpecDefsDiagnosticTypes();
  for (const auto& diag : stream) {
    if (diag.severity == Severity::Error) {
      return true;
    }
  }
  return false;
}

CompileStatusResult CompileStatus(const DiagnosticStream& stream) {
  SPEC_DEF("CompileStatus", "1.6.3");
  return HasError(stream) ? CompileStatusResult::Fail : CompileStatusResult::Ok;
}

}  // namespace cursive0::core
