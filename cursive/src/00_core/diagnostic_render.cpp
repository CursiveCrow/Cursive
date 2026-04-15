// =============================================================================
// diagnostic_render.cpp - Diagnostic formatting and rendering
// =============================================================================
//
// SPEC REFERENCE: CursiveSpecification.md
//   - Section 1.6.6 "Diagnostic Rendering" (lines 668-718)
//     - Render rules for diagnostics (message + location)
//     - RenderRich rules for source-context diagnostics
//   - Section 1.6.3 "Diagnostics: Records and Emission" (lines 612-634)
//     - Diagnostic record structure
//
// =============================================================================

#include "00_core/diagnostic_render.h"

#include <algorithm>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>

#include "00_core/assert_spec.h"
#include "00_core/terminal.h"

namespace cursive::core {

// ============================================================================
// Ordering (C5)
// ============================================================================

DiagnosticStream Order(const DiagnosticStream& stream) {
  SPEC_RULE("Order");
  // Spec §1.6.5 defines Order(Δ) as identity-preserving over emission order.
  return stream;
}

// ============================================================================
// ApplyNoSpanExternal
// ============================================================================

Diagnostic ApplyNoSpanExternal(const Diagnostic& diag, DiagnosticOrigin origin) {
  SPEC_RULE("NoSpan-External");
  if (origin == DiagnosticOrigin::External) {
    Diagnostic out = diag;
    out.span.reset();
    return out;
  }
  return diag;
}

// ============================================================================
// Render (plain text, backward-compatible)
// ============================================================================

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
    case Severity::Note:
      out += "note";
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

// ============================================================================
// Helpers for RenderRich
// ============================================================================

namespace {

std::string_view SeverityLabel(Severity sev) {
  switch (sev) {
    case Severity::Error: return "error";
    case Severity::Warning: return "warning";
    case Severity::Note: return "note";
  }
  return "error";
}

Color SeverityColor(Severity sev) {
  switch (sev) {
    case Severity::Error: return Color::BoldRed;
    case Severity::Warning: return Color::BoldYellow;
    case Severity::Note: return Color::BoldCyan;
  }
  return Color::BoldRed;
}

std::string_view SubDiagLabel(SubDiagnosticKind kind) {
  switch (kind) {
    case SubDiagnosticKind::Note: return "note";
    case SubDiagnosticKind::Help: return "help";
    case SubDiagnosticKind::FixIt: return "help";
  }
  return "note";
}

Color SubDiagColor(SubDiagnosticKind kind) {
  switch (kind) {
    case SubDiagnosticKind::Note: return Color::BoldCyan;
    case SubDiagnosticKind::Help: return Color::BoldGreen;
    case SubDiagnosticKind::FixIt: return Color::BoldGreen;
  }
  return Color::BoldCyan;
}

// Truncate a source line to fit within max_width columns.
// gutter_width is the space taken by the gutter (line number + " | ").
// Returns the possibly-truncated line and adjusts caret_start/caret_len.
struct TruncatedLine {
  std::string text;
  std::size_t caret_start;
  std::size_t caret_len;
};

TruncatedLine TruncateSourceLine(std::string_view line,
                                 std::size_t gutter_width,
                                 std::size_t caret_start,
                                 std::size_t caret_len,
                                 int terminal_width) {
  TruncatedLine result;
  result.text = std::string(line);
  result.caret_start = caret_start;
  result.caret_len = caret_len;

  if (terminal_width <= 0) {
    return result;
  }

  const int available = terminal_width - static_cast<int>(gutter_width);
  if (available <= 6 || static_cast<int>(line.size()) <= available) {
    return result;
  }

  // If the caret is within the visible portion, truncate the right side.
  if (caret_start + caret_len <= static_cast<std::size_t>(available - 3)) {
    result.text = std::string(line.substr(0, static_cast<std::size_t>(available - 3))) + "...";
    return result;
  }

  // Shift the view to center on the caret.
  std::size_t view_start = 0;
  if (caret_start > static_cast<std::size_t>(available / 2)) {
    view_start = caret_start - static_cast<std::size_t>(available / 2);
  }
  std::size_t view_len = static_cast<std::size_t>(available - 6);  // room for "..." on both sides
  if (view_start + view_len > line.size()) {
    view_len = line.size() - view_start;
  }

  result.text = "..." + std::string(line.substr(view_start, view_len)) + "...";
  result.caret_start = caret_start - view_start + 3;  // +3 for leading "..."
  return result;
}

// Extract line N (1-based) from source text. Returns empty if out of range.
std::string_view GetSourceLine(std::string_view source, std::size_t line_number) {
  if (line_number == 0) return {};
  std::size_t current_line = 1;
  std::size_t pos = 0;
  while (pos < source.size()) {
    if (current_line == line_number) {
      std::size_t end = source.find('\n', pos);
      if (end == std::string_view::npos) {
        end = source.size();
      }
      // Strip trailing \r for Windows line endings
      std::size_t line_end = end;
      if (line_end > pos && source[line_end - 1] == '\r') {
        --line_end;
      }
      return source.substr(pos, line_end - pos);
    }
    std::size_t nl = source.find('\n', pos);
    if (nl == std::string_view::npos) break;
    pos = nl + 1;
    ++current_line;
  }
  return {};
}

std::string UnderlineForSpan(const Span& span) {
  const std::size_t spaces = span.start_col > 0 ? span.start_col - 1 : 0;
  const std::size_t carets =
      span.end_col > span.start_col ? span.end_col - span.start_col : 0;
  return std::string(spaces, ' ') + std::string(carets, '^');
}

}  // namespace

// ============================================================================
// RenderRich (C2 - source context + color)
// ============================================================================

std::string RenderRich(const Diagnostic& diag,
                       const SourceRegistry& sources,
                       const RenderOptions& opts) {
  const bool c = opts.color;
  (void)opts.context_lines;
  (void)opts.terminal_width;
  const auto sev_color = SeverityColor(diag.severity);
  const auto sev_label = SeverityLabel(diag.severity);

  std::string head;
  head += Colorize(std::string(sev_label), sev_color, c);
  head += Colorize("[", sev_color, c);
  head += Colorize(diag.code, sev_color, c);
  head += Colorize("]", sev_color, c);
  if (!diag.message.empty()) {
    head += ": ";
    head += diag.message;
  }

  if (diag.span.has_value() && sources) {
    const auto& sp = *diag.span;
    if (const auto file_content = sources(sp.file); file_content.has_value()) {
      const auto line_text = GetSourceLine(*file_content, sp.start_line);
      const auto gutter = std::to_string(sp.start_line);
      std::string out = head;
      out += "\n";
      out += Colorize("  --> ", Color::Blue, c);
      out += sp.file;
      out += ":";
      out += std::to_string(sp.start_line);
      out += ":";
      out += std::to_string(sp.start_col);
      out += "\n";
      out += Colorize(gutter, Color::Blue, c);
      out += " ";
      out += Colorize("|", Color::Blue, c);
      out += " ";
      out += std::string(line_text);
      out += "\n";
      out += Colorize(gutter, Color::Blue, c);
      out += " ";
      out += Colorize("|", Color::Blue, c);
      out += " ";
      out += Colorize(UnderlineForSpan(sp), sev_color, c);
      return out;
    }
  }
  return head;
}

// ============================================================================
// DiagnosticSummary (C6)
// ============================================================================

std::string DiagnosticSummary(const DiagnosticStream& stream, bool color) {
  int errors = 0;
  int warnings = 0;
  int notes = 0;
  for (const auto& d : stream) {
    if (d.severity == Severity::Error) {
      ++errors;
    } else if (d.severity == Severity::Warning) {
      ++warnings;
    } else if (d.severity == Severity::Note) {
      ++notes;
    }
  }
  if (errors == 0 && warnings == 0 && notes == 0) {
    return {};
  }
  std::string out;
  if (errors > 0) {
    out += Colorize(
        std::to_string(errors) + " error" + (errors != 1 ? "s" : ""),
        Color::BoldRed, color);
  }
  if (errors > 0 && warnings > 0) {
    out += ", ";
  }
  if (warnings > 0) {
    out += Colorize(
        std::to_string(warnings) + " warning" + (warnings != 1 ? "s" : ""),
        Color::BoldYellow, color);
  }
  if ((errors > 0 || warnings > 0) && notes > 0) {
    out += ", ";
  }
  if (notes > 0) {
    out += Colorize(
        std::to_string(notes) + " note" + (notes != 1 ? "s" : ""),
        Color::BoldCyan, color);
  }
  out += " emitted";
  return out;
}

}  // namespace cursive::core
