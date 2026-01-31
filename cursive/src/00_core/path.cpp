// =============================================================================
// MIGRATION MAPPING: path.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   - Section 2.1 "Project Root and Manifest" - Path Resolution (lines 956-999)
//     - WinSep = {"\\", "/"}
//     - AsciiLetter(c): c in A-Z or a-z
//     - DriveRooted(p): |p| >= 3 && AsciiLetter(p[0]) && p[1] = ":" && p[2] in WinSep
//     - UNC(p): StartsWith(p, "//") || StartsWith(p, "\\\\")
//     - RootRelative(p): starts with / or \ but not UNC or DriveRooted
//     - RootTag(p): extracts root component
//     - Tail(p): extracts path after root
//     - Segs(p): splits path into segments by separators
//     - PathComps(p): RootTag + Segs(Tail)
//     - JoinComp(comps): joins components with /
//     - Join(a, b): if b absolute return b, else join components
//     - AbsPath(p): DriveRooted || UNC || RootRelative
//     - is_relative(p): !AbsPath(p)
//     - Normalize(p): removes "." components
//     - Canon(p): Normalize, fails if ".." present
//     - prefix(p, q): PathPrefix check
//     - relative(p, base): computes relative path
//     - Basename(p): last component
//     - FileExt(p): extension including dot (lines 1001-1006)
//   - Resolve rules (lines 1008-1026)
//     - Resolve-Canonical: joins and canonicalizes
//     - Resolve-Canonical-Err: fails if Canon fails
//     - WF-RelPath: validates relative path stays under root
//
// SOURCE FILE: cursive-bootstrap/src/00_core/path.cpp
//   - Lines 1-219 (entire file)
//
// CONTENT TO MIGRATE:
//   - IsWinSep(c) -> bool (lines 9-11)
//   - IsAsciiLetter(c) -> bool (lines 13-15)
//   - StartsWith(s, prefix) -> bool (lines 17-19) [static]
//   - DriveRooted(p) -> bool (lines 21-23)
//   - UNC(p) -> bool (lines 25-27)
//   - RootRelative(p) -> bool (lines 29-31)
//   - AbsPath(p) -> bool (lines 33-35)
//   - IsRelative(p) -> bool (lines 37-39)
//   - RootTag(p) -> string (lines 41-52)
//   - Tail(p) -> string (lines 54-65)
//   - Segs(p) -> vector<string> (lines 67-83)
//   - PathComps(p) -> vector<string> (lines 85-96)
//   - JoinCompRec helper (lines 98-112) [static]
//   - JoinComp(comps) -> string (lines 114-116)
//   - Join(a, b) -> string (lines 118-126)
//   - Normalize(p) -> string (lines 128-139)
//   - Canon(p) -> optional<string> (lines 141-150)
//   - PathPrefix(path, pref) -> bool (lines 152-163)
//   - Prefix(p, q) -> bool (lines 165-167)
//   - Resolve(root, p) -> optional<ResolveResult> (lines 169-179)
//   - Relative(p, base) -> optional<string> (lines 181-197)
//   - Basename(p) -> string (lines 199-205)
//   - FileExt(p) -> string (lines 207-217)
//
// DEPENDENCIES:
//   - cursive/include/00_core/path.h (header)
//     - ResolveResult struct (root, path)
//   - cursive/include/00_core/assert_spec.h
//     - SPEC_RULE macro
//   - <string>, <string_view>, <vector>, <optional>
//
// REFACTORING NOTES:
//   1. Windows-specific path handling (drive letters, UNC paths)
//   2. SPEC_RULE traces:
//      - "Resolve-Canonical" and "Resolve-Canonical-Err" in Resolve()
//   3. RootTag returns drive letter without trailing separator (e.g., "C:")
//   4. Canon fails (returns nullopt) if any ".." component exists
//   5. FileExt returns empty string if no extension or dot at position 0
//   6. All path operations are pure functions (no filesystem access)
//   7. Consider string_view parameters where lifetimes permit
//
// =============================================================================
