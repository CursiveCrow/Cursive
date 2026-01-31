// =============================================================================
// MIGRATION MAPPING: diagnostic_messages.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   - Section 1.6.3 "Diagnostics: Records and Emission" (lines 612-634)
//     - Message function: maps code to condition text
//     - Severity function: maps code to severity
//   - Section 8.0 "DiagId-Code Map" (Appendix A)
//     - Complete mapping of diagnostic IDs to codes, severities, conditions
//   - Section 1.6.6 "Diagnostic Rendering" (lines 668-718)
//     - TypeRender, PermLexeme, etc. for message formatting
//
// SOURCE FILE: cursive-bootstrap/src/00_core/diagnostic_messages.cpp
//   - Lines 1-129 (entire file)
//
// CONTENT TO MIGRATE:
//   - DiagMessageEntry struct (lines 12-16)
//     Internal struct: code, severity, message
//   - kDiagMessages table (included from diagnostic_messages_table.inc, line 18)
//     Sorted array of all diagnostic entries
//   - FindEntry(code) -> DiagMessageEntry* (lines 27-44)
//     Binary search in sorted message table
//   - MessageForCode(code, locale) -> optional<string_view> (lines 48-57)
//     Returns message template for given diagnostic code
//   - SeverityForCode(code, locale) -> optional<Severity> (lines 59-68)
//     Returns severity level for given diagnostic code
//   - FormatMessage(template, args) -> string (lines 70-108)
//     Substitutes {key} placeholders with MessageArg values
//   - MakeDiagnostic(code, span, locale) -> optional<Diagnostic> (lines 110-125)
//     Creates complete Diagnostic record from code
//
// DEPENDENCIES:
//   - cursive/include/00_core/diagnostic_messages.h (header)
//     - MessageLocale enum
//     - MessageArg struct
//     - Severity enum (from diagnostics.h)
//     - Diagnostic struct (from diagnostics.h)
//   - cursive/include/00_core/assert_spec.h
//     - SPEC_DEF macro
//   - cursive/include/00_core/span.h
//     - Span type for diagnostic locations
//   - diagnostic_messages_table.inc
//     - Generated table of all diagnostic messages (must be sorted by code)
//
// REFACTORING NOTES:
//   1. Binary search in FindEntry assumes sorted table - maintain sort invariant
//   2. FormatMessage handles {key} substitution - simple regex-free implementation
//   3. locale parameter currently unused (void cast) - reserved for i18n
//   4. SPEC_DEF traces:
//      - "DiagId-Code-Map" -> "8.0"
//      - "SeverityColumn" -> "8.0"
//      - "ConditionColumn" -> "8.0"
//      - "Message" -> "1.6.3"
//   5. Table include path may need adjustment for new directory structure
//   6. Consider string_view for FormatMessage output where lifetime permits
//
// =============================================================================
