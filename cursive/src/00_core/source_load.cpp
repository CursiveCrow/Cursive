// =============================================================================
// MIGRATION MAPPING: source_load.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   - Section 3.1 "Source Loading and Normalization" (lines 1703-1999)
//   - Section 3.1.1 UTF-8 encoding/decoding (lines 1715-1736)
//     - UnicodeScalar: 0 <= u <= 0x10FFFF, excluding surrogates [0xD800, 0xDFFF]
//     - Utf8Len, EncodeUTF8, DecodeUTF8 algorithms
//   - Section 3.1.3 UTF-8 Decoding and BOM Handling (lines 1747-1781)
//     - Decode-Ok, Decode-Err rules
//     - StripBOM rules: Empty, None, Start, Embedded
//     - BOM = U+FEFF
//   - Section 3.1.4 Line Ending Normalization (lines 1783-1818)
//     - CR = U+000D, LF = U+000A
//     - Norm rules: Empty, CRLF, CR, LF, Other
//     - LineStarts, Utf8Offsets computation
//   - Section 3.1.5 Prohibited Code Points (lines 1827-1836)
//     - Prohibited(c): Cc category except {U+0009, U+000A, U+000C, U+000D}
//     - WF-Prohibited rule
//   - Section 3.1.8 Source Loading Pipeline (lines 1893-1956)
//     - Small-step: Start -> Sized -> Decoded -> BomStripped -> Normalized -> LineMapped -> Validated
//     - Big-step: LoadSource-Ok, LoadSource-Err
//   - Section 3.1.9 Diagnostic Spans (lines 1958-1993)
//     - SpanAtIndex, SpanAtLineStart for error locations
//     - Diagnostic codes: E-SRC-0101 (decode), E-SRC-0103 (embedded BOM), E-SRC-0104 (prohibited)
//     - Warning: W-SRC-0101 (leading BOM)
//
// SOURCE FILE: cursive-bootstrap/src/00_core/source_load.cpp
//   - Lines 1-295 (entire file)
//
// CONTENT TO MIGRATE:
//   - IsContinuation(byte) -> bool (lines 21-23) [internal]
//     Checks UTF-8 continuation byte pattern (10xxxxxx)
//   - IsSurrogate(u) -> bool (lines 25-27) [internal]
//     Checks if codepoint is surrogate (D800-DFFF)
//   - DecodeUtf8Internal(bytes) -> optional<vector<UnicodeScalar>> (lines 29-106)
//     Full UTF-8 decoder with validation
//   - BuildSpanSource helper (lines 109-121) [internal]
//     Constructs SourceFile from decoded scalars
//   - FirstBomIndex(scalars) -> optional<size_t> (lines 123-131) [internal]
//     Finds first BOM occurrence in scalar sequence
//   - SpanAtIndex helper (lines 133-139) [internal]
//     Creates span for scalar at given index
//   - Decode(bytes) -> DecodeResult (lines 143-154)
//     Public decoder: returns scalars and ok flag
//   - StripBOM(scalars) -> StripBOMResult (lines 156-185)
//     Removes leading BOM, detects embedded BOMs
//   - NormalizeLF(scalars) -> vector<UnicodeScalar> (lines 187-223)
//     Converts CR, CRLF to LF
//   - LoadSource(path, bytes) -> SourceLoadResult (lines 225-293)
//     Complete pipeline: decode -> strip BOM -> normalize -> validate
//
// DEPENDENCIES:
//   - cursive/include/00_core/source_load.h (header)
//     - DecodeResult, StripBOMResult, SourceLoadResult structs
//   - cursive/include/00_core/source_text.h
//     - SourceFile struct, LineStarts(), EncodeUtf8(), Utf8Offsets()
//   - cursive/include/00_core/span.h
//     - Span struct, SpanOf()
//   - cursive/include/00_core/unicode.h
//     - UnicodeScalar type, kBOM, kCR, kLF constants
//     - NoProhibited(), FirstProhibitedOutsideLiteral()
//   - cursive/include/00_core/diagnostics.h
//     - DiagnosticStream, Emit()
//   - cursive/include/00_core/diagnostic_messages.h
//     - MakeDiagnostic()
//   - cursive/include/00_core/assert_spec.h
//     - SPEC_RULE macro
//
// REFACTORING NOTES:
//   1. SPEC_RULE traces map to spec sections:
//      - "Decode-Ok", "Decode-Err" -> 3.1.3
//      - "StripBOM-*" rules -> 3.1.3
//      - "Norm-*" rules -> 3.1.4
//      - "Step-*" rules -> 3.1.8
//      - "Span-*" rules -> 3.1.9
//      - "LoadSource-Ok", "LoadSource-Err" -> 3.1.8
//   2. UTF-8 decoder validates:
//      - Proper continuation bytes
//      - No overlong encodings
//      - No surrogates
//      - Valid codepoint range (0-0x10FFFF)
//   3. LoadSource emits diagnostics for all error conditions
//   4. W-SRC-0101 emitted even if LoadSource ultimately fails
//   5. NoProhibited check excludes literals (see unicode.cpp)
//
// =============================================================================
