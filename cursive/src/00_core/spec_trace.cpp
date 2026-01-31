// =============================================================================
// MIGRATION MAPPING: spec_trace.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   - Section 0.3.2 "Observable Behavior Equivalence for Bootstrap" (lines 181-207)
//     - DiagObs(d) = <d.code, d.severity, d.message, d.span>
//     - DiagStream(C, P) = [DiagObs(d_1), ..., DiagObs(d_k)]
//     - ObsComp(C, P) for compiler observable behavior
//   - Section 0.3.1 "Bootstrap Milestones" (lines 155-179)
//     - Bootstrap equivalence verification requirements
//   - General: Spec tracing is implementation infrastructure for conformance
//     testing, not directly specified but supports verification of spec rules
//
// SOURCE FILE: cursive-bootstrap/src/00_core/spec_trace.cpp
//   - Lines 1-134 (entire file)
//
// CONTENT TO MIGRATE:
//   - TraceState struct (lines 13-20) [internal]
//     Thread-safe state: mutex, output stream, domain, phase, root, enabled
//   - State() -> TraceState& (lines 22-25) [internal]
//     Singleton accessor for global trace state
//   - EncodePayload(payload) -> string (lines 27-53) [internal]
//     URL-encodes special characters: \t->%09, \n->%0A, %->%25, ;->%3B, =->%3D
//   - RelPath(path, root) -> string (lines 55-65) [internal]
//     Computes relative path for trace output
//   - SpecTrace::Init(path, domain) (lines 69-84)
//     Initializes trace output file, writes header "spec_trace_v1\n"
//   - SpecTrace::SetRoot(root) (lines 86-90)
//     Sets project root for relative path computation
//   - SpecTrace::SetPhase(phase) (lines 92-96)
//     Sets current compilation phase for trace context
//   - SpecTrace::Record(rule_id, span, payload) (lines 98-124)
//     Records trace entry: domain, phase, rule_id, file, line:col range, payload
//     Format: tab-separated fields, URL-encoded payload
//   - SpecTrace::Record(rule_id) (lines 126-128)
//     Convenience overload with no span or payload
//   - SpecTrace::Enabled() -> bool (lines 130-132)
//     Returns whether tracing is active
//
// DEPENDENCIES:
//   - cursive/include/00_core/spec_trace.h (header)
//     - SpecTrace class with static methods
//   - cursive/include/00_core/span.h
//     - Span struct (optional span parameter)
//   - cursive/include/00_core/path.h
//     - Normalize(), Relative() for path handling
//   - <fstream> for file output
//   - <mutex> for thread safety
//   - <sstream> for string building
//
// REFACTORING NOTES:
//   1. Thread-safe via mutex lock on all operations
//   2. Output format: tab-separated, one record per line
//     domain \t phase \t rule_id \t file \t start_line \t start_col \t end_line \t end_col \t payload
//   3. File header "spec_trace_v1\n" for format versioning
//   4. Phase defaults to "-" if not set
//   5. File defaults to "-" if span is nullopt
//   6. Payload encoding prevents field delimiter conflicts
//   7. This is infrastructure code - not directly from spec but supports
//      conformance testing via SPEC_DEF and SPEC_RULE macros
//   8. Consider buffering for performance if trace volume is high
//
// =============================================================================
