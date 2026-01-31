// =============================================================================
// MIGRATION MAPPING: deterministic_order.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md Section 2.3 (lines 1168-1207)
//   - 2.3. Deterministic Ordering and Case Folding
//   - 2.3.1. Module File Processing Order
//   - 2.3.2. Module Path Case-Folding Algorithm
//   - 2.3.3. Directory Enumeration Order
//
// SOURCE FILE: cursive-bootstrap/src/01_project/deterministic_order.cpp
//   - Lines 1-171 (entire file)
//
// =============================================================================
// CONTENT TO MIGRATE
// =============================================================================
//
// 1. EmitExternal() helper (lines 16-22)
//    - PURPOSE: Emit diagnostic without source span
//    - REFACTORING: Consolidate with other MakeExternalDiag helpers
//
// 2. SplitModulePath() helper (lines 24-37)
//    - PURPOSE: Split module path on "::" delimiter
//    - EXAMPLE: "foo::bar::baz" -> ["foo", "bar", "baz"]
//
// 3. JoinModulePath() helper (lines 39-49)
//    - PURPOSE: Join module path components with "::"
//    - EXAMPLE: ["foo", "bar", "baz"] -> "foo::bar::baz"
//
// 4. FoldPath() (lines 53-61)
//    - PURPOSE: Case-fold filesystem path for comparison
//    - SPEC RULE: FoldPath(r) = JoinComp([CaseFold(NFC(c)) | c in PathComps(r)])
//    - SPEC REF: Line 1171
//    - DEPENDENCIES: core::PathComps, core::CaseFold, core::NFC, core::JoinComp
//
// 5. Fold() (lines 63-71)
//    - PURPOSE: Case-fold module path for comparison
//    - SPEC RULE: Fold(p) = [CaseFold(NFC(c)) | c in p]
//    - SPEC REF: Lines 1186-1187
//    - DEPENDENCIES: SplitModulePath, core::CaseFold, core::NFC
//
// 6. Utf8LexLess() (lines 73-77)
//    - PURPOSE: UTF-8 lexicographic comparison for sorting
//    - SPEC RULE: Utf8LexLess(a, b) <=> LexBytes(Utf8(a), Utf8(b))
//    - SPEC REF: Line 1374
//
// 7. KeyLess() (lines 79-87)
//    - PURPOSE: Compare OrderKey pairs for sorting
//    - SPEC RULE: f_1 <_file f_2 <=> Utf8LexLess(FileKey(f_1, d), FileKey(f_2, d))
//    - SPEC REF: Line 1177
//
// 8. FileKey() (lines 89-101)
//    - PURPOSE: Compute file ordering key
//    - SPEC RULE: FileKey(f, d) = <FoldPath(rel), rel> if relative(f, d) ok
//    - SPEC RULE: FileKey(f, d) = <bottom, Basename(f)> if relative fails
//    - SPEC REF: Lines 1173-1175
//    - DIAGNOSTIC: E-PRJ-0303 (FileOrder-Rel-Fail)
//
// 9. DirKey() (lines 103-115)
//    - PURPOSE: Compute directory ordering key
//    - SPEC RULE: DirKey(d, S) = <FoldPath(rel), rel> if relative(d, S) ok
//    - SPEC RULE: DirKey(d, S) = <bottom, Basename(d)> if relative fails
//    - SPEC REF: Lines 1191-1194
//    - DIAGNOSTIC: E-PRJ-0303 (DirSeq-Rel-Fail)
//
// 10. DirSeqFrom() (lines 117-138)
//     - PURPOSE: Sort directory list by deterministic order
//     - SPEC RULE: DirSeq(S) = sort_{<_dir}(Dirs(S))
//     - SPEC REF: Line 1197
//
// 11. DirSeq() (lines 140-168)
//     - PURPOSE: Enumerate and sort directories
//     - SPEC RULE: DirSeq-Read-Err
//     - SPEC REF: Lines 1199-1202
//     - DIAGNOSTIC: E-PRJ-0305
//
// =============================================================================
// DEPENDENCIES
// =============================================================================
//
// Headers required:
//   - "cursive0/01_project/deterministic_order.h"
//   - "cursive0/00_core/assert_spec.h" (SPEC_RULE macro)
//   - "cursive0/00_core/diagnostic_messages.h" (MakeDiagnostic)
//   - "cursive0/00_core/path.h" (PathComps, Basename, Relative, JoinComp)
//   - "cursive0/00_core/unicode.h" (CaseFold, NFC)
//   - <algorithm>
//   - <optional>
//   - <system_error>
//   - <filesystem>
//   - <vector>
//   - <string>
//
// Types from header (deterministic_order.h):
//   - OrderKey { std::string folded; std::string raw; }
//   - DirSeqResult { std::vector<std::filesystem::path> dirs; DiagnosticStream diags; }
//
// =============================================================================
// REFACTORING NOTES
// =============================================================================
//
// 1. SplitModulePath/JoinModulePath also appear in module_discovery.cpp
//    RECOMMENDATION: Move to a shared module_path utility
//
// 2. Consider using std::ranges for sorting operations in C++20
//
// 3. The OrderKey type could benefit from comparison operators:
//    bool operator<(const OrderKey& a, const OrderKey& b) { return KeyLess(a, b); }
//
// =============================================================================
// SPEC RULE ANNOTATIONS (use SPEC_RULE macro)
// =============================================================================
//
// Line 96: SPEC_RULE("FileOrder-Rel-Fail");
// Line 110: SPEC_RULE("DirSeq-Rel-Fail");
// Line 145: SPEC_RULE("DirSeq-Read-Err");
// Line 153: SPEC_RULE("DirSeq-Read-Err");
// Line 160: SPEC_RULE("DirSeq-Read-Err");
//
// =============================================================================

