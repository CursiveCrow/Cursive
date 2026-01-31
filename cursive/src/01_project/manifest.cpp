// =============================================================================
// MIGRATION MAPPING: manifest.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md Section 2.1 (lines 779-1027)
//   - 2.1. Project Root and Manifest
//   - Manifest Parsing (Big-Step): ParseTOML, Parse-Manifest-Ok/Missing/Err
//   - Manifest Required (No Single-File Fallback)
//   - Manifest Path Resolution
//
// SOURCE FILE: cursive-bootstrap/src/01_project/manifest.cpp
//   - Lines 1-109 (entire file)
//
// =============================================================================
// CONTENT TO MIGRATE
// =============================================================================
//
// 1. StartDirForInput() helper (lines 16-32)
//    - PURPOSE: Determine starting directory for project root search
//    - REFACTORING: Consider moving to 00_core/path module
//
// 2. MakeExternalDiag() helper (lines 34-40)
//    - PURPOSE: Create diagnostic without source span
//    - REFACTORING: Common helper - consolidate into core diagnostics module
//
// 3. FindProjectRoot() (lines 44-66)
//    - PURPOSE: Search upward from input path to find Cursive.toml
//    - SPEC RULE: WF-Project-Root (line 1154-1159 in spec)
//    - DEPENDENCIES: std::filesystem, core diagnostics
//
// 4. ParseManifest() (lines 68-107)
//    - PURPOSE: Parse Cursive.toml manifest file
//    - SPEC RULES:
//      * Parse-Manifest-Ok (spec lines 785-788)
//      * Parse-Manifest-Missing (spec lines 790-793)
//      * Parse-Manifest-Err (spec lines 795-798)
//    - DIAGNOSTICS:
//      * E-PRJ-0101: Manifest file not found
//      * E-PRJ-0102: Manifest parse error
//    - DEPENDENCIES:
//      * toml++ library (toml::parse_file)
//      * core::HostPrimFail for host primitive failure tracking
//      * core::MakeDiagnostic, core::Emit
//
// =============================================================================
// DEPENDENCIES
// =============================================================================
//
// Headers required:
//   - "cursive0/01_project/manifest.h" (types: ManifestParseResult)
//   - "cursive0/00_core/assert_spec.h" (SPEC_RULE macro)
//   - "cursive0/00_core/diagnostic_messages.h" (MakeDiagnostic)
//   - "cursive0/00_core/diagnostics.h" (DiagnosticStream, Emit, HasError)
//   - "cursive0/00_core/host_primitives.h" (HostPrimFail)
//   - <filesystem>
//   - <exception>
//   - <system_error>
//   - toml++ (toml::parse_file, toml::parse_error)
//
// Types from header (manifest.h):
//   - ManifestParseResult { std::optional<toml::table> table; DiagnosticStream diags; }
//
// =============================================================================
// REFACTORING NOTES
// =============================================================================
//
// 1. MakeExternalDiag helper is duplicated across multiple files:
//    - manifest.cpp
//    - load_project.cpp
//    - module_discovery.cpp
//    - outputs.cpp
//    - link.cpp
//    - project_validate.cpp
//    - deterministic_order.cpp
//    RECOMMENDATION: Consolidate into core::MakeExternalDiag() in diagnostics module
//
// 2. Consider adding structured error context for ParseManifest:
//    - Capture TOML parse error details (line, column, message)
//    - Include file path in diagnostic
//
// 3. The spec defines:
//    - ParseTOML : Path -> TOMLTable (host primitive)
//    - The implementation wraps this with diagnostic emission
//
// =============================================================================
// SPEC RULE ANNOTATIONS (use SPEC_RULE macro)
// =============================================================================
//
// Line 75: SPEC_RULE("Parse-Manifest-Err");
// Line 84: SPEC_RULE("Parse-Manifest-Missing");
// Line 93: SPEC_RULE("Parse-Manifest-Ok");
// Line 101: SPEC_RULE("Parse-Manifest-Err");
//
// =============================================================================

