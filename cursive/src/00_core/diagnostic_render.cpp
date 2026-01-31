// =============================================================================
// MIGRATION MAPPING: diagnostic_render.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   - Section 1.6.5 "Diagnostic Ordering" (lines 661-666)
//     - Order rule: diagnostics emitted in emission order
//   - Section 1.6.6 "Diagnostic Rendering" (lines 668-718)
//     - Render(d) formula for diagnostic string format:
//       code ++ " (" ++ sev ++ ")" ++ msg ++ " @" ++ loc
//     - sev mapping: Error->"error", Warning->"warning"
//     - loc format: file:line:col
//   - Section 1.6.7 "Diagnostics without Source Spans" (lines 720-725)
//     - NoSpan-External rule: external-origin diagnostics have no span
//
// SOURCE FILE: cursive-bootstrap/src/00_core/diagnostic_render.cpp
//   - Lines 1-56 (entire file)
//
// CONTENT TO MIGRATE:
//   - Order(stream) -> DiagnosticStream (lines 7-10)
//     Returns stream unchanged (preserves emission order)
//   - ApplyNoSpanExternal(diag, origin) -> Diagnostic (lines 12-20)
//     Clears span for external-origin diagnostics
//   - Render(diag) -> string (lines 22-54)
//     Formats diagnostic for human-readable output:
//     - Format: "CODE (severity): message @file:line:col"
//     - Severity labels: error, warning, info, panic
//     - Omits location if span is nullopt
//
// DEPENDENCIES:
//   - cursive/include/00_core/diagnostic_render.h (header)
//     - DiagnosticOrigin enum (Internal, External)
//   - cursive/include/00_core/diagnostics.h
//     - DiagnosticStream type (vector<Diagnostic>)
//     - Diagnostic struct
//     - Severity enum
//   - cursive/include/00_core/assert_spec.h
//     - SPEC_RULE macro
//     - SPEC_DEF macro
//
// REFACTORING NOTES:
//   1. Order function is identity - spec says preserve emission order
//   2. Render format must match spec exactly:
//      - With span: "CODE (sev): msg @file:line:col"
//      - Without span: "CODE (sev): msg"
//   3. SPEC_RULE traces:
//      - "Order" -> section 1.6.5
//      - "NoSpan-External" -> section 1.6.7
//   4. SPEC_DEF trace:
//      - "Render" -> "1.6.6"
//   5. Added Severity::Panic handling beyond spec's {Error, Warning}
//   6. Consider using string_view where possible for efficiency
//   7. std::to_string for line/col numbers - keep this approach
//
// =============================================================================
