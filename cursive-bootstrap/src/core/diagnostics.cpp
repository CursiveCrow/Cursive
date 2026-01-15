#include "cursive0/core/diagnostics.h"

#include "cursive0/core/assert_spec.h"

namespace cursive0::core {

static inline void SpecDefsDiagnosticTypes() {
  SPEC_DEF("Severity", "1.6.3");
  SPEC_DEF("Diagnostic", "1.6.3");
  SPEC_DEF("DiagnosticStream", "1.6.3");
}

DiagnosticStream Emit(const DiagnosticStream& stream, const Diagnostic& diag) {
  SPEC_RULE("Emit-Append");
  SpecDefsDiagnosticTypes();
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
