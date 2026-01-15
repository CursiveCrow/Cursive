#include "cursive0/core/span.h"

#include <algorithm>

#include "cursive0/core/assert_spec.h"

namespace cursive0::core {

static inline void SpecDefsSpanTypes() {
  SPEC_DEF("SourceLocation", "1.6.1");
  SPEC_DEF("Span", "1.6.1");
}

std::pair<std::size_t, std::size_t> SpanRange(const Span& sp) {
  SPEC_DEF("SpanRange", "1.6.1");
  return {sp.start_offset, sp.end_offset};
}

SourceLocation Locate(const SourceFile& source, std::size_t offset) {
  SpecDefsSpanTypes();
  SPEC_RULE("WF-Location");
  const std::size_t o_prime = std::min(offset, source.byte_len);
  const auto& line_starts = source.line_starts;

  std::size_t k = 0;
  if (!line_starts.empty()) {
    auto it = std::upper_bound(line_starts.begin(), line_starts.end(), o_prime);
    if (it != line_starts.begin()) {
      k = static_cast<std::size_t>(it - line_starts.begin() - 1);
    }
  }

  const std::size_t line = k + 1;
  const std::size_t line_start = line_starts.empty() ? 0 : line_starts[k];
  const std::size_t col = (o_prime - line_start) + 1;

  return SourceLocation{source.path, o_prime, line, col};
}

std::pair<std::size_t, std::size_t> ClampSpan(
    const SourceFile& source,
    std::size_t start,
    std::size_t end) {
  SPEC_DEF("ClampSpan", "1.6.1");
  const std::size_t s = std::min(start, source.byte_len);
  const std::size_t e = std::min(std::max(end, s), source.byte_len);
  return {s, e};
}

Span SpanOf(const SourceFile& source, std::size_t start, std::size_t end) {
  SPEC_RULE("Span-Of");
  SPEC_RULE("WF-Span");
  const auto clamped = ClampSpan(source, start, end);
  const SourceLocation start_loc = Locate(source, clamped.first);
  const SourceLocation end_loc = Locate(source, clamped.second);

  Span sp;
  sp.file = source.path;
  sp.start_offset = clamped.first;
  sp.end_offset = clamped.second;
  sp.start_line = start_loc.line;
  sp.start_col = start_loc.column;
  sp.end_line = end_loc.line;
  sp.end_col = end_loc.column;
  return sp;
}

}  // namespace cursive0::core
