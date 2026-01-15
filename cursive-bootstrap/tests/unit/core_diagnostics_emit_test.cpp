#include <cassert>
#include <string>
#include <vector>

#include "cursive0/core/assert_spec.h"
#include "cursive0/core/diagnostics.h"

namespace {

using cursive0::core::CompileStatus;
using cursive0::core::CompileStatusResult;
using cursive0::core::Diagnostic;
using cursive0::core::DiagnosticStream;
using cursive0::core::Emit;
using cursive0::core::HasError;
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
  SPEC_COV("Emit-Append");

  Span sp;
  sp.file = "diag.cursive";
  sp.start_offset = 0;
  sp.end_offset = 3;
  sp.start_line = 1;
  sp.start_col = 1;
  sp.end_line = 1;
  sp.end_col = 4;

  Diagnostic d1 = MakeDiag("E-TEST-0001", Severity::Warning, "first");
  Diagnostic d2 = MakeDiag("E-TEST-0002", Severity::Error, "second");
  d2.span = sp;
  Diagnostic d3 = MakeDiag("E-TEST-0003", Severity::Warning, "third");

  DiagnosticStream stream = {d1, d2};
  const DiagnosticStream appended = Emit(stream, d3);

  assert(stream.size() == 2);
  assert(appended.size() == 3);

  ExpectDiagEq(stream[0], d1);
  ExpectDiagEq(stream[1], d2);

  ExpectDiagEq(appended[0], d1);
  ExpectDiagEq(appended[1], d2);
  ExpectDiagEq(appended[2], d3);

  assert(HasError(appended));
  assert(CompileStatus(appended) == CompileStatusResult::Fail);

  DiagnosticStream warnings = {d1, d3};
  assert(!HasError(warnings));
  assert(CompileStatus(warnings) == CompileStatusResult::Ok);

  return 0;
}
