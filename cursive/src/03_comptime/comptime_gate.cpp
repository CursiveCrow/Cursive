// =============================================================================
// MIGRATION MAPPING: comptime_gate.cpp
// =============================================================================
//
// SPEC REFERENCE:
//   C0updated.md §4 (lines 6650-6653)
//   "Phase 2: Compile-Time Execution (Deferred in Cursive0)"
//   ComptimeForm = {`comptime`}
//   ComptimeForm ⊆ UnsupportedForm
//
//   C0updated.md §3.3.8.6 (lines 5153-5156)
//   "(Parse-Comptime-Unsupported)" rule
//   Comptime blocks are rejected during parsing with Unsupported-Construct diagnostic.
//
//   C0updated.md §5.13.1 (line 13920)
//   "Deferred attributes. `reflect`, `derive`, `emit`, and `files` require compile-time
//    execution and are deferred in Cursive0 (Phase 2). Their use is rejected as
//    Unsupported-Construct."
//
// SOURCE FILE: NEW (deferred functionality)
//   No direct source in cursive-bootstrap. This file provides a gate for
//   compile-time execution features that are deferred in Cursive0.
//
// DEPENDENCIES:
//   - cursive/include/00_core/diagnostics.h (DiagnosticStream, Emit, Severity)
//   - cursive/include/00_core/diagnostic_codes.h (diagnostic code definitions)
//   - cursive/include/02_source/ast/ast.h (Expr, ExprKind)
//   - cursive/include/00_core/span.h (Span)
//
// PURPOSE:
//   This module implements the compile-time execution gate for Cursive0.
//   In Cursive0, compile-time execution (Phase 2) is deferred, meaning:
//   1. `comptime { ... }` blocks are rejected during parsing
//   2. Deferred attributes (reflect, derive, emit, files) are rejected
//   3. Any other compile-time execution features are gated off
//
//   The gate functions provide a centralized location for:
//   - Detecting comptime constructs
//   - Emitting appropriate diagnostics
//   - Ensuring consistent behavior across the compiler
//
// REFACTORING NOTES:
//   - In future Cursive versions, this module will be replaced by actual
//     compile-time execution infrastructure
//   - The gate pattern allows easy feature flagging for gradual enablement
//   - Consider adding a compiler flag (--enable-comptime) for experimental support
//
// IMPLEMENTATION REQUIREMENTS:
//   1. IsComptimeForm(token) -> bool
//      Returns true if the token introduces a comptime construct.
//      Per spec: ComptimeForm = {`comptime`}
//
//   2. CheckComptimeGate(expr, diags) -> void
//      Emits E-UNS diagnostic if expr is a comptime block.
//      The diagnostic code should be from the Unsupported-Construct category.
//
//   3. ComptimeDeferred() -> bool
//      Returns true (constant) in Cursive0 to indicate Phase 2 is deferred.
//
// DIAGNOSTIC CODES (from spec §8.7):
//   E-UNS-0101: Unsupported construct (generic)
//   Specific codes for comptime may be added as needed.
//
// =============================================================================

// TODO: Implement comptime gate functionality
// This is a deferred feature in Cursive0 - the gate rejects all comptime usage.

