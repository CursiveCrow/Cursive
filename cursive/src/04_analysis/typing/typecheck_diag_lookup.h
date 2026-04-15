#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "00_core/behavior_model.h"
#include "00_core/diagnostic_codes.h"
#include "00_core/diagnostic_messages.h"
#include "00_core/diagnostics.h"

namespace cursive::analysis {

inline bool IsDiagnosticCode(std::string_view diag_id) {
  return diag_id.size() > 2 &&
         (diag_id[0] == 'E' || diag_id[0] == 'W' || diag_id[0] == 'I') &&
         diag_id[1] == '-';
}

inline core::Diagnostic MakeInternalTypecheckDiagnostic(
    core::Severity severity,
    const std::optional<core::Span>& span,
    const std::string& message) {
  core::Diagnostic diag;
  diag.severity = severity;
  diag.span = span;
  diag.message = message;
  return diag;
}

inline std::optional<std::string> LookupTypecheckDiagCode(std::string_view diag_id) {
  if (const auto code = core::ResolveDiagCode(std::string(diag_id));
      code.has_value()) {
    return *code;
  }

  if (const auto code =
          core::StaticUndefinedCodeForRule(core::SpecDiagCodeMap(),
                                           core::C0DiagCodeMap(), diag_id);
      code.has_value()) {
    return *code;
  }

  if (IsDiagnosticCode(diag_id)) {
    return std::string(diag_id);
  }

  return std::nullopt;
}

inline std::optional<core::Diagnostic> BuildRuleFallbackTypecheckDiagnostic(
    std::string_view diag_id,
    const std::optional<core::Span>& span) {
  if (diag_id == "T-Cast-Invalid") {
    core::Diagnostic diag;
    diag.code = "T-Cast-Invalid";
    diag.severity = core::Severity::Error;
    diag.span = span;
    diag.message = "Invalid cast";
    return diag;
  }
  return std::nullopt;
}

inline std::optional<core::Diagnostic> BuildResolvedTypecheckDiagnostic(
    std::string_view diag_id,
    const std::optional<core::Span>& span) {
  if (const auto code = LookupTypecheckDiagCode(diag_id); code.has_value()) {
    if (auto diag = core::MakeDiagnosticById(*code, span)) {
      return diag;
    }
    return MakeInternalTypecheckDiagnostic(
        core::Severity::Error, span,
        "Internal error: unresolved diagnostic code '" + *code + "'");
  }

  if (auto fallback = BuildRuleFallbackTypecheckDiagnostic(diag_id, span);
      fallback.has_value()) {
    return fallback;
  }

  return MakeInternalTypecheckDiagnostic(
      core::Severity::Error, span,
      "Internal error: unknown diagnostic id '" + std::string(diag_id) + "'");
}

inline void EmitResolvedTypecheckDiagnostic(
    core::DiagnosticStream& diags,
    std::string_view diag_id,
    const std::optional<core::Span>& span,
    const std::string& detail = {}) {
  auto diag = BuildResolvedTypecheckDiagnostic(diag_id, span);
  if (!diag.has_value()) {
    return;
  }
  if (!detail.empty()) {
    core::SubDiagnostic note;
    note.kind = core::SubDiagnosticKind::Note;
    note.message = detail;
    diag->children.push_back(std::move(note));
  }
  core::Emit(diags, *diag);
}

}  // namespace cursive::analysis
