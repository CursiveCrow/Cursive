#include <cassert>
#include <string>

#include "cursive0/core/diagnostic_render.h"
#include "cursive0/core/diagnostics.h"

namespace {

using cursive0::core::Diagnostic;
using cursive0::core::Render;
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

}  // namespace

int main() {
  Span sp;
  sp.file = "render.cursive";
  sp.start_offset = 0;
  sp.end_offset = 1;
  sp.start_line = 2;
  sp.start_col = 5;
  sp.end_line = 2;
  sp.end_col = 6;

  Diagnostic with_span = MakeDiag("E-TEST-0200", Severity::Error, "boom");
  with_span.span = sp;
  assert(Render(with_span) == "E-TEST-0200 (error): boom @render.cursive:2:5");

  Diagnostic no_span = MakeDiag("W-TEST-0201", Severity::Warning, "warn");
  assert(Render(no_span) == "W-TEST-0201 (warning): warn");

  Diagnostic empty_msg = MakeDiag("E-TEST-0202", Severity::Error, "");
  empty_msg.span = sp;
  assert(Render(empty_msg) == "E-TEST-0202 (error) @render.cursive:2:5");

  Diagnostic empty_msg_no_span = MakeDiag("W-TEST-0203", Severity::Warning, "");
  assert(Render(empty_msg_no_span) == "W-TEST-0203 (warning)");

  return 0;
}
