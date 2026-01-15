#include <cassert>
#include <string>
#include <vector>

#include "cursive0/core/assert_spec.h"
#include "cursive0/core/diagnostic_render.h"
#include "cursive0/core/diagnostics.h"

namespace {

using cursive0::core::Diagnostic;
using cursive0::core::DiagnosticStream;
using cursive0::core::Order;
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

static void ExpectDiagEq(const Diagnostic& a, const Diagnostic& b) {
  assert(a.code == b.code);
  assert(a.severity == b.severity);
  assert(a.message == b.message);
  assert(a.span.has_value() == b.span.has_value());
  if (a.span && b.span) {
    ExpectSpanEq(*a.span, *b.span);
  }
}

}  // namespace

int main() {
  SPEC_COV("Order");

  Span sp;
  sp.file = "order.cursive";
  sp.start_offset = 0;
  sp.end_offset = 4;
  sp.start_line = 1;
  sp.start_col = 1;
  sp.end_line = 1;
  sp.end_col = 5;

  Diagnostic d1 = MakeDiag("E-TEST-0001", Severity::Error, "first");
  Diagnostic d2 = MakeDiag("W-TEST-0002", Severity::Warning, "second");
  d2.span = sp;
  Diagnostic d3 = MakeDiag("E-TEST-0003", Severity::Error, "third");

  DiagnosticStream stream = {d1, d2, d3};
  DiagnosticStream ordered = Order(stream);

  assert(stream.size() == 3);
  assert(ordered.size() == 3);

  ExpectDiagEq(ordered[0], d1);
  ExpectDiagEq(ordered[1], d2);
  ExpectDiagEq(ordered[2], d3);

  return 0;
}
