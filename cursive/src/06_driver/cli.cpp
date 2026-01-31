// =============================================================================
// MIGRATION MAPPING: cli.cpp
// =============================================================================
//
// SPEC REFERENCE:
//   C0updated.md §0.3 (lines 186-204)
//   "Observable Compiler Behavior"
//   Status(C, P) = ok ⇔ ∀ d ∈ DiagStream(C, P). d.severity ≠ Error
//   Status(C, P) = fail ⇔ ∃ d ∈ DiagStream(C, P). d.severity = Error
//   ExitCode(C, P) = 0 ⇔ Status(C, P) = ok
//   ExitCode(C, P) = 1 ⇔ Status(C, P) = fail
//
//   C0updated.md §2 (lines 766-777)
//   "Command-Line Output"
//   DumpProject(P, dump) = ProjectSummary(P) ++ OutputSummary(P) ...
//
// SOURCE FILE: cursive-bootstrap/src/06_driver/main.cpp
//   Lines 74-85: CliOptions struct
//   Lines 103-187: ParseArgs function
//   Lines 189-276: JSON formatting helpers (EscapeJson, SeverityString, DiagnosticToJson)
//   Lines 627-633: Help message and usage string
//
// DEPENDENCIES:
//   - cursive/include/00_core/diagnostics.h (DiagnosticStream, Severity)
//   - cursive/include/00_core/diagnostic_render.h (Render)
//   - <string>, <string_view>, <optional>, <vector>
//
// PURPOSE:
//   This module handles command-line interface parsing and output formatting.
//   Responsibilities include:
//   1. Parsing command-line arguments
//   2. Validating CLI options
//   3. Formatting diagnostic output (text and JSON)
//   4. Help message generation
//
// REFACTORING NOTES:
//   - Extract from monolithic main.cpp into focused CLI module
//   - Consider using a CLI parsing library for future extensibility
//   - The JSON output format should be documented for tooling integration
//   - Internal flags (CURSIVE0_INTERNAL_FLAGS) gating should be preserved
//
// IMPLEMENTATION REQUIREMENTS:
//
// 1. CliOptions struct (SOURCE: lines 74-85)
//    struct CliOptions {
//      bool diag_json = false;      // --diag-json
//      bool show_help = false;      // --help, -h
//      bool phase1_only = false;    // --phase1-only (internal)
//      bool no_output = false;      // --no-output (internal)
//      bool dump_project = false;   // --dump
//      bool dump_ast = false;       // --dump-ast (internal)
//      std::optional<std::string> spec_trace_path;  // --spec-trace
//      std::optional<std::string> assembly_target;  // --assembly
//      std::string input_path;
//      bool emit_ir = false;        // --emit-ir (internal)
//    };
//
// 2. ParseArgs(argc, argv) -> std::optional<CliOptions> (SOURCE: lines 103-187)
//    Parse command-line arguments and return options.
//    Returns std::nullopt on parse error.
//    Supported flags:
//      --help, -h           Show help message
//      --diag-json          Output diagnostics as JSON
//      --spec-trace <path>  Enable spec tracing
//      --spec-trace=<path>  (alternate form)
//      --dump               Dump project structure
//      --dump-ast           Dump AST (internal)
//      --phase1-only        Stop after Phase 1 (internal)
//      --no-output          Skip output generation (internal)
//      --assembly <name>    Select assembly target
//      --assembly=<name>    (alternate form)
//      --emit-ir            Emit LLVM IR (internal)
//      build                Subcommand (ignored)
//      <path>               Input file/project path
//
// 3. InternalFlagsEnabled() -> bool (SOURCE: lines 93-96)
//    Returns true if CURSIVE0_INTERNAL_FLAGS environment variable is set.
//    Internal flags require this to be enabled.
//
// 4. EscapeJson(value: std::string_view) -> std::string (SOURCE: lines 189-225)
//    Escape a string for JSON output.
//    Handles: \\ \" \n \r \t and control characters.
//
// 5. SeverityString(severity: Severity) -> std::string (SOURCE: lines 227-239)
//    Convert severity enum to JSON string.
//    Mappings: Error->"error", Warning->"warning", Info->"info", Panic->"panic"
//
// 6. DiagnosticToJson(diag: Diagnostic) -> std::string (SOURCE: lines 241-261)
//    Format a single diagnostic as JSON object.
//    Fields: code, severity, message, span (nullable)
//
// 7. DiagnosticStreamToJson(stream: DiagnosticStream) -> std::string (SOURCE: lines 263-276)
//    Format entire diagnostic stream as JSON.
//    Format: {"diagnostics": [...]}
//
// 8. PrintUsage() -> void
//    Print usage message to stderr.
//    Format: "usage: cursivec0 build <file> [--assembly <name>] [--diag-json] [--dump] [--spec-trace <path>]"
//
// 9. PrintHelp() -> void
//    Print help message to stdout (same as usage for now).
//
// HELPER FUNCTIONS (SOURCE):
// 10. StartsWith(value, prefix) -> bool (SOURCE: lines 98-100)
//     String prefix check utility.
//
// EXIT CODE SEMANTICS (from spec §0.3):
//   - Exit code 0: Compilation succeeded (no errors in diagnostic stream)
//   - Exit code 1: Compilation failed (at least one error diagnostic)
//   - Exit code 2: CLI parse error (invalid arguments)
//
// =============================================================================

// TODO: Migrate CLI parsing and output formatting from bootstrap main.cpp

