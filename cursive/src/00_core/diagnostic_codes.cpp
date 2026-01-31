// =============================================================================
// MIGRATION MAPPING: diagnostic_codes.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   - Section 1.6.4 "Diagnostic Code Selection" (lines 636-660)
//     - SpecCode function: maps DiagId to DiagCode via spec-defined codes
//     - C0Code function: maps DiagId to DiagCode via C0-specific codes
//     - Code function: prioritizes SpecCode over C0Code
//   - Rules: Code-Spec, Code-C0 (lines 642-650)
//   - Section 0.4 "Document Conventions" (lines 220-227)
//     - DiagCode format: DiagPrefix-DiagCategory-DiagDigits
//     - DiagPrefix = {E, W, I}
//     - DiagCategory = [A-Z]^3
//     - DiagDigits = [0-9]^4
//   - Section 1.2 "Behavior Types" (lines 392-421)
//     - DiagId, DiagIdOf definitions
//
// SOURCE FILE: cursive-bootstrap/src/00_core/diagnostic_codes.cpp
//   - Lines 1-45 (entire file)
//
// CONTENT TO MIGRATE:
//   - SpecCode(spec_map, id) -> optional<DiagCode> (lines 12-19)
//     Maps DiagId to DiagCode using spec-defined map
//   - C0Code(c0_map, id) -> optional<DiagCode> (lines 21-28)
//     Maps DiagId to DiagCode using C0-specific map
//   - Code(spec_map, c0_map, id) -> optional<DiagCode> (lines 30-43)
//     Unified lookup: spec code takes priority over C0 code
//
// DEPENDENCIES:
//   - cursive/include/00_core/diagnostic_codes.h (header)
//     - DiagCode type (string alias)
//     - DiagId type (string alias)
//     - DiagCodeMap type (unordered_map<DiagId, DiagCode>)
//   - cursive/include/00_core/assert_spec.h
//     - SPEC_DEF macro for tracing spec definitions
//     - SPEC_RULE macro for tracing rule applications
//
// REFACTORING NOTES:
//   1. Keep interface identical - these are pure lookup functions
//   2. SPEC_DEF and SPEC_RULE macros should trace to spec sections:
//      - "DiagId" -> "1.2"
//      - "DiagCode" -> "0.4"
//      - "Code-Spec" rule -> "1.6.4"
//      - "Code-C0" rule -> "1.6.4"
//   3. Consider adding constexpr where possible for compile-time evaluation
//   4. The maps are passed by const reference - maintain this pattern
//   5. Return type is std::optional<DiagCode> for missing entries
//
// =============================================================================
