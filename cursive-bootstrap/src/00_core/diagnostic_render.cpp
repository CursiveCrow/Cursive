#include "cursive0/00_core/diagnostic_render.h"

#include "cursive0/00_core/assert_spec.h"

namespace cursive0::core {

DiagnosticStream Order(const DiagnosticStream& stream) {
  SPEC_RULE("Order");
  return stream;
}

Diagnostic ApplyNoSpanExternal(const Diagnostic& diag, DiagnosticOrigin origin) {
  SPEC_RULE("NoSpan-External");
  if (origin == DiagnosticOrigin::External) {
    Diagnostic out = diag;
    out.span.reset();
    return out;
  }
  return diag;
}

std::string Render(const Diagnostic& diag) {
  SPEC_DEF("Render", "1.6.6");
  std::string out = diag.code;
  out += " (";
  switch (diag.severity) {
    case Severity::Error:
      out += "error";
      break;
    case Severity::Warning:
      out += "warning";
      break;
    case Severity::Info:
      out += "info";
      break;
    case Severity::Panic:
      out += "panic";
      break;
  }
  out += ")";
  if (!diag.message.empty()) {
    out += ": ";
    out += diag.message;
  }
  if (diag.span.has_value()) {
    out += " @";
    out += diag.span->file;
    out += ":";
    out += std::to_string(diag.span->start_line);
    out += ":";
    out += std::to_string(diag.span->start_col);
  }
  return out;
}

}  // namespace cursive0::core
