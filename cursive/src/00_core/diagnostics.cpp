// =============================================================================
// MIGRATION MAPPING: diagnostics.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   - Section 1.6.3 "Diagnostics: Records and Emission" (lines 612-634)
//     - Severity = {Error, Warning} (line 616)
//     - Diagnostic stream: Delta = [d_1, ..., d_n] (lines 618-619)
//     - Emit-Append rule: appends diagnostic to stream (lines 621-623)
//     - CompileStatus(Delta): fail if HasError(Delta), ok otherwise (lines 632-634)
//
// SOURCE FILE: cursive-bootstrap/src/00_core/diagnostics.cpp
//   - Lines 1-67 (entire file)
//
// CONTENT TO MIGRATE:
//   - SeverityLabel(severity) -> string (lines 13-25)
//     Internal helper: maps Severity enum to string label
//   - DiagPayload(diag) -> string (lines 27-37)
//     Internal helper: formats diagnostic for spec tracing
//   - Emit(stream, diag) -> DiagnosticStream (lines 39-49)
//     Appends diagnostic to stream, records spec trace
//     Implements Emit-Append rule from spec
//   - HasError(stream) -> bool (lines 51-60)
//     Returns true if stream contains Error or Panic severity
//   - CompileStatus(stream) -> CompileStatusResult (lines 62-65)
//     Returns Fail if HasError, Ok otherwise
//
// DEPENDENCIES:
//   - cursive/include/00_core/diagnostics.h (header)
//     - Severity enum (Error, Warning, Info, Panic)
//     - Diagnostic struct (code, severity, message, span)
//     - DiagnosticStream type (vector<Diagnostic>)
//     - CompileStatusResult enum (Ok, Fail)
//   - cursive/include/00_core/assert_spec.h
//     - SPEC_RULE macro
//     - SPEC_DEF macro
//   - cursive/include/00_core/spec_trace.h
//     - SpecTrace::Enabled()
//     - SpecTrace::Record()
//
// REFACTORING NOTES:
//   1. Emit is append-only, returns new stream (functional style)
//   2. Spec defines Severity as {Error, Warning} but implementation
//      extends with {Info, Panic} for practical use
//   3. HasError checks both Error AND Panic severities
//   4. SPEC_RULE traces:
//      - "Emit-Append" -> section 1.6.3
//   5. SPEC_DEF traces:
//      - "Severity" -> "1.6.3"
//      - "Diagnostic" -> "1.6.3"
//      - "DiagnosticStream" -> "1.6.3"
//      - "HasError" -> "1.6.3"
//      - "CompileStatus" -> "1.6.3"
//   6. SpecTrace integration for conformance testing
//   7. Consider making DiagPayload format match spec observable format
//
// =============================================================================
