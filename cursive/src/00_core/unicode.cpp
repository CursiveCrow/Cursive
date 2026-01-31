// =============================================================================
// MIGRATION MAPPING: unicode.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   - Section 3.1.6 "NFC Normalization for Identifiers and Module Paths" (lines 1838-1851)
//     - NFC(s) = UnicodeNFC_{15.0.0}(s)
//     - CaseFold(s) = UnicodeCaseFold_{15.0.0}(s)
//     - Totality: NFC and CaseFold are total on Unicode scalar sequences
//     - IdKey(s) = NFC(s)
//     - IdEq(s1, s2) iff IdKey(s1) = IdKey(s2)
//   - Section 3.2.2 "Identifier Lexing" (referenced in SpecDefsIdentChars)
//     - IdentStart: '_' or XID_Start
//     - IdentContinue: '_' or XID_Continue
//     - XID_Start, XID_Continue: Unicode properties
//     - NonCharacter: U+FDD0-U+FDEF, U+nFFFE, U+nFFFF
//     - Sensitive: bidirectional controls and zero-width joiners
//   - Section 3.1.5 "Prohibited Code Points" (lines 1827-1836)
//     - Prohibited(c): Cc category except {U+0009, U+000A, U+000C, U+000D}
//     - WF-Prohibited rule for source files
//   - Section 3.1.2 "Lexically Sensitive Unicode" (lines 1743-1746)
//     - Sensitive characters: U+200E-U+200F, U+202A-U+202E, U+2066-U+2069, U+200C-U+200D
//
// SOURCE FILE: cursive-bootstrap/src/00_core/unicode.cpp
//   - Lines 1-335 (entire file)
//
// CONTENT TO MIGRATE:
//   - SpecDefsUnicode() (lines 21-24) [static inline]
//     Traces NFC and CaseFold to spec section 3.1.6
//   - NFC(s) -> string (lines 26-45)
//     Unicode NFC normalization via ICU, targeting Unicode 15.0
//   - CaseFold(s) -> string (lines 47-59)
//     Unicode case folding via ICU, targeting Unicode 15.0
//   - SpecDefsIdentChars() (lines 61-68) [static inline]
//     Traces identifier character definitions to spec section 3.2.2
//   - IsXidStart(c) -> bool (lines 70-76)
//     Checks XID_Start Unicode property
//   - IsXidContinue(c) -> bool (lines 78-84)
//     Checks XID_Continue Unicode property
//   - IsNonCharacter(c) -> bool (lines 86-96)
//     Checks for Unicode noncharacters (FDD0-FDEF, xFFFE, xFFFF)
//   - IsIdentStart(c) -> bool (lines 98-101)
//     Returns '_' || IsXidStart(c)
//   - IsIdentContinue(c) -> bool (lines 103-106)
//     Returns '_' || IsXidContinue(c)
//   - IsSensitive(c) -> bool (lines 108-112)
//     Checks for bidirectional controls and zero-width joiners
//   - [Internal helpers] (lines 114-302)
//     - ByteSpan struct, IsHexDigit, HexValue, IsUnicodeScalarValue
//     - IsStringChar, IsCharContent
//     - ScanEscape, ScanStringLiteral, ScanCharLiteral
//     - LiteralByteSpans: identifies string/char literal ranges
//     - ByteInLiteralSpan: checks if offset is inside a literal
//   - IsProhibited(c) -> bool (lines 305-311)
//     Checks for prohibited control characters
//   - FirstProhibitedOutsideLiteral(scalars) -> optional<size_t> (lines 313-328)
//     Finds first prohibited character outside string/char literals
//   - NoProhibited(scalars) -> bool (lines 330-333)
//     Returns true if no prohibited characters outside literals
//
// DEPENDENCIES:
//   - cursive/include/00_core/unicode.h (header)
//     - UnicodeScalar type
//     - kBOM, kCR, kLF constants
//     - Utf8Offsets() function
//   - cursive/include/00_core/assert_spec.h
//     - SPEC_DEF macro
//     - SPEC_RULE macro
//   - ICU headers:
//     - <unicode/normalizer2.h> for NFC
//     - <unicode/stringpiece.h> for StringPiece
//     - <unicode/uchar.h> for u_hasBinaryProperty, UCHAR_XID_START/CONTINUE
//     - <unicode/unistr.h> for UnicodeString
//     - <unicode/utypes.h> for UErrorCode
//     - <unicode/uversion.h> for U_UNICODE_VERSION
//
// REFACTORING NOTES:
//   1. ICU dependency: requires ICU library with Unicode 15.0 support
//   2. static_assert verifies U_UNICODE_VERSION == "15.0" for spec compliance
//   3. SPEC_DEF traces:
//      - "NFC", "CaseFold" -> "3.1.6"
//      - "IdentStart", "IdentContinue", "XID_Start", "XID_Continue" -> "3.2.2"
//      - "NonCharacter", "Sensitive" -> "3.2.2"
//   4. SPEC_RULE traces:
//      - "WF-Prohibited" in NoProhibited()
//   5. Literal scanning handles escape sequences for accurate span detection
//   6. Prohibited check excludes content inside string/char literals
//   7. std::abort() on ICU failure - consider error handling improvement
//   8. UnicodeScalar range check (> 0x10FFFF) in XID functions
//
// =============================================================================
