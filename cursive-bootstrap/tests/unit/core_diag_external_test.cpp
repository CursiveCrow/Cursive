#include <cassert>
#include <string>

#include "cursive0/core/assert_spec.h"
#include "cursive0/core/diagnostic_render.h"
#include "cursive0/core/diagnostics.h"

namespace {

using cursive0::core::ApplyNoSpanExternal;
using cursive0::core::Diagnostic;
using cursive0::core::DiagnosticOrigin;
using cursive0::core::Severity;
using cursive0::core::Span;

static Diagnostic MakeDiag(const std::string& code,
                           Severity severity,
                           const std::string& message) {
  Diagnostic diag;
  diag.code = code;
  diag.severity = severity;
  diag.message = message;
  return diag;
}

static void ExpectSpanEq(const Span& a, const Span& b) {
  assert(a.file == b.file);
  assert(a.start_offset == b.start_offset);
  assert(a.end_offset == b.end_offset);
  assert(a.start_line == b.start_line);
  assert(a.start_col == b.start_col);
  assert(a.end_line == b.end_line);
  assert(a.end_col == b.end_col);
}

}  // namespace

int main() {
  SPEC_COV("NoSpan-External");

  Span sp;
  sp.file = "external.cursive";
  sp.start_offset = 2;
  sp.end_offset = 6;
  sp.start_line = 3;
  sp.start_col = 4;
  sp.end_line = 3;
  sp.end_col = 8;

  Diagnostic base = MakeDiag("E-TEST-0100", Severity::Error, "external");
  base.span = sp;

  Diagnostic external = ApplyNoSpanExternal(base, DiagnosticOrigin::External);
  assert(!external.span.has_value());

  Diagnostic internal = ApplyNoSpanExternal(base, DiagnosticOrigin::Internal);
  assert(internal.span.has_value());
  ExpectSpanEq(*internal.span, sp);

  return 0;
}
