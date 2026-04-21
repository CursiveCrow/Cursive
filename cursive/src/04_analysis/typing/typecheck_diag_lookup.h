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

inline std::optional<std::string_view> LegacyTypecheckRuleDiagCode(
    std::string_view diag_id) {
  if (diag_id == "Main-Missing") {
    return "E-MOD-2434";
  }
  if (diag_id == "Main-Multiple") {
    return "E-MOD-2430";
  }
  if (diag_id == "Main-Generic-Err") {
    return "E-MOD-2432";
  }
  if (diag_id == "Main-Signature-Err") {
    return "E-MOD-2431";
  }
	  if (diag_id == "Call-Arg-NotPlace") {
	    return "E-TYP-1603";
	  }
  if (diag_id == "Assign-NotPlace") {
    return "E-SEM-3131";
  }
  if (diag_id == "Assign-Const-Err") {
    return "E-SEM-3132";
  }
  if (diag_id == "Assign-Type-Err") {
    return "E-SEM-3133";
  }
  if (diag_id == "Assign-Immutable-Err") {
    return "E-MOD-2401";
  }
  if (diag_id == "B-Place-Unique-Err") {
    return "E-TYP-1602";
  }
  if (diag_id == "MethodCall-RecvPerm-Err") {
    return "E-TYP-1605";
  }
  if (diag_id == "GpuIntrinsic-Outside-Err") {
    return "E-CON-0154";
  }
	  if (diag_id == "Call-ArgCount-Err" ||
	      diag_id == "Generic-Call-ArgCount-Err") {
	    return "E-SEM-2532";
  }
  if (diag_id == "Call-ArgType-Err") {
    return "E-SEM-2533";
  }
  if (diag_id == "Call-Move-Missing") {
    return "E-SEM-2534";
  }
  if (diag_id == "Call-Move-Unexpected") {
    return "E-SEM-2535";
  }
  if (diag_id == "Method-Context-Err") {
    return "E-SEM-3011";
  }
  if (diag_id == "Impl-Missing-Method") {
    return "E-TYP-2503";
  }
  if (diag_id == "Impl-Sig-Err") {
    return "E-TYP-2503";
  }
  if (diag_id == "Impl-AssocType-Missing") {
    return "E-TYP-2503";
  }
  if (diag_id == "Alloc-Implicit-NoRegion-Err") {
    return "E-MEM-3021";
  }
  if (diag_id == "Alloc-Region-NotFound-Err") {
    return "E-MEM-1206";
  }
  if (diag_id == "Region-Unchecked-Unsafe-Err") {
    return "E-MEM-3030";
  }
  if (diag_id == "AllocRaw-Unsafe-Err") {
    return "E-MEM-3030";
  }
  if (diag_id == "DeallocRaw-Unsafe-Err") {
    return "E-MEM-3030";
  }
  if (diag_id == "Export-Vis-Err") {
    return "E-SYS-3353";
  }
  if (diag_id == "ExternAbi-Unknown-Err") {
    return "E-SYS-3352";
  }
  if (diag_id == "ExternProc-Generic-Err") {
    return "E-TYP-2306";
  }
  if (diag_id == "Packed-Field-Unsafe-Err") {
    return "E-TYP-2105";
  }
  if (diag_id == "FieldAccess-Unknown") {
    return "E-TYP-1904";
  }
  if (diag_id == "Return-Type-Err") {
    return "E-SEM-3161";
  }
  if (diag_id == "Return-Unit-Err") {
    return "E-SEM-3161";
  }
  if (diag_id == "Enum-Disc-Invalid") {
    return "E-TYP-1921";
  }
  if (diag_id == "Enum-Disc-Dup") {
    return "E-TYP-1923";
  }
  if (diag_id == "Superclass-Cycle") {
    return "E-TYP-2508";
  }
  if (diag_id == "Modal-NoStates-Err") {
    return "E-TYP-2050";
  }
  if (diag_id == "Modal-DupState-Err") {
    return "E-TYP-2051";
  }
  if (diag_id == "Modal-StateName-Err") {
    return "E-TYP-2054";
  }
  if (diag_id == "Modal-Field-Missing") {
    return "E-TYP-2052";
  }
  if (diag_id == "Modal-Field-General-Err") {
    return "E-TYP-2057";
  }
  if (diag_id == "Modal-Field-NotVisible") {
    return "E-TYP-2064";
  }
  if (diag_id == "Modal-Method-NotVisible") {
    return "E-TYP-2064";
  }
  if (diag_id == "Modal-Method-NotFound") {
    return "E-TYP-2053";
  }
  if (diag_id == "Transition-NotVisible") {
    return "E-TYP-2064";
  }
  if (diag_id == "Transition-Body-Err") {
    return "E-TYP-2055";
  }
  if (diag_id == "Transition-Source-Err") {
    return "E-TYP-2056";
  }
  if (diag_id == "Transition-Target-Err") {
    return "E-TYP-2059";
  }
  if (diag_id == "Modal-Payload-DupField") {
    return "E-TYP-2058";
  }

  return std::nullopt;
}

inline bool IsDiagnosticCode(std::string_view diag_id) {
  return diag_id.size() > 2 &&
         (diag_id[0] == 'E' || diag_id[0] == 'W' || diag_id[0] == 'I' ||
          diag_id[0] == 'P') &&
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

  if (const auto code = LegacyTypecheckRuleDiagCode(diag_id);
      code.has_value()) {
    return std::string(*code);
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
