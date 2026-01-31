// =============================================================================
// MIGRATION MAPPING: symbols.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   - Section 2.5 "Output Artifacts and Linking" - Object File Naming (lines 1349-1357)
//     - PathToPrefix(s) = Concat([BMap(b) | b in Utf8(NFC(s))])
//     - BMap(b): b in [0-9A-Za-z] -> chr(b), otherwise -> "_x" ++ Hex2(b)
//     - mangle(s) = PathToPrefix(s)
//     - MangleModulePath(p) = mangle(PathString(PathKey(p)))
//   - Section 6.3 "Symbols, Mangling, and Linkage" (referenced in hash.cpp)
//     - Symbol generation and naming conventions
//   - Section 3.1.6 "NFC Normalization" (lines 1838-1851)
//     - NFC(s) = UnicodeNFC_{15.0.0}(s)
//     - IdKey(s) = NFC(s)
//     - PathKey(p) = [NFC(c_1), ..., NFC(c_n)]
//
// SOURCE FILE: cursive-bootstrap/src/00_core/symbols.cpp
//   - Lines 1-105 (entire file)
//
// CONTENT TO MIGRATE:
//   - IsAsciiAlnum(c) -> bool (lines 11-14) [internal]
//     Checks if character is ASCII alphanumeric [0-9A-Za-z]
//   - HexDigit(value) -> char (lines 16-22) [internal]
//     Converts 0-15 to hex digit '0'-'9' or 'a'-'f' (lowercase)
//   - JoinWithDoubleColon(comps) -> string (lines 24-34) [internal]
//     Joins path components with "::" separator
//   - SplitModulePath(path) -> vector<string> (lines 36-49) [internal]
//     Splits module path on "::" delimiters
//   - StringOfPath(comps) -> string (lines 53-55)
//     Joins vector<string> with "::" via JoinWithDoubleColon
//   - StringOfPath(comps) -> string (lines 57-71)
//     Overload for initializer_list<string_view>
//   - PathToPrefix(s) -> string (lines 73-87)
//     Encodes string for symbol name: alphanumeric pass-through, else "_xHH"
//   - Mangle(s) -> string (lines 89-91)
//     Alias for PathToPrefix
//   - PathSig(comps) -> string (lines 93-95)
//     Mangles joined path: Mangle(StringOfPath(comps))
//   - MangleModulePath(module_path) -> string (lines 97-103)
//     Splits on "::", NFC-normalizes each part, mangles result
//
// DEPENDENCIES:
//   - cursive/include/00_core/symbols.h (header)
//     - Function declarations
//   - cursive/include/00_core/unicode.h
//     - NFC() for Unicode normalization
//   - <string>, <string_view>, <vector>, <initializer_list>
//
// REFACTORING NOTES:
//   1. PathToPrefix uses lowercase hex (_xhh) per BMap spec
//   2. Mangle is direct alias for PathToPrefix
//   3. NFC normalization applied per-component in MangleModulePath
//   4. "::" is the module path separator in Cursive
//   5. StringOfPath overloads handle both vector and initializer_list
//   6. No SPEC_DEF/SPEC_RULE macros in source - consider adding:
//      - "PathToPrefix" -> "2.5"
//      - "mangle" -> "2.5"
//      - "MangleModulePath" -> "2.5"
//   7. Consider constexpr where possible for compile-time evaluation
//
// =============================================================================
