// ===========================================================================
// MIGRATION MAPPING: source_normalize.cpp
// ===========================================================================
//
// PURPOSE:
//   Source text normalization functions: UTF-8 decoding, BOM handling,
//   line ending normalization (CR/CRLF to LF), prohibited character checking.
//
// ---------------------------------------------------------------------------
// SPEC REFERENCE: C:\Dev\Cursive\C0updated.md
// ---------------------------------------------------------------------------
//   Section 3.1 - Source Loading and Normalization
//   Lines 1705-1994 (entire subsection)
//
//   Key definitions:
//   - Lines 1715-1735: Unicode scalars and UTF-8 encoding/decoding functions
//     * Utf8Len(u), EncodeUTF8(u), DecodeUTF8(B), Utf8Valid(B)
//   - Lines 1738-1741: NormalizeOutsideIdentifiers (identity in C0)
//   - Lines 1743-1746: LexSecure enforcement (outside scope, see lexer)
//   - Lines 1747-1781: UTF-8 Decoding and BOM Handling
//     * Decode-Ok, Decode-Err rules (1750-1758)
//     * StripLeadBOM function (1760-1762)
//     * StripBOM-Empty, StripBOM-None, StripBOM-Start, StripBOM-Embedded (1764-1781)
//   - Lines 1783-1825: Line Ending Normalization
//     * CR = U+000D, LF = U+000A (1785-1786)
//     * Norm-Empty, Norm-CRLF, Norm-CR, Norm-LF, Norm-Other rules (1788-1810)
//     * Utf8Offsets, LineStarts, LineCount functions (1814-1818)
//     * Locate function (1820-1825)
//   - Lines 1827-1836: Prohibited Code Points
//     * Prohibited(c) predicate (1829)
//     * LiteralSpan(T) computation (1831)
//     * WF-Prohibited rule (1833-1836)
//
// ---------------------------------------------------------------------------
// SOURCE FILE: C:\Dev\Cursive\cursive-bootstrap\src\00_core\source_load.cpp
// ---------------------------------------------------------------------------
//   Lines 17-141: Internal helpers
//     - IsContinuation (21-23): UTF-8 continuation byte check
//     - IsSurrogate (25-27): Surrogate codepoint check
//     - DecodeUtf8Internal (29-106): Main UTF-8 decoding implementation
//       * 1-byte sequences (37-41)
//       * 2-byte sequences (43-59)
//       * 3-byte sequences (61-78), includes surrogate check
//       * 4-byte sequences (80-100)
//     - BuildSpanSource (109-121): Constructs SourceFile from components
//     - FirstBomIndex (123-131): Finds first BOM in scalar sequence
//     - SpanAtIndex (133-139): Creates span for a scalar index
//
//   Lines 143-154: Decode function
//     - Implements Decode-Ok and Decode-Err rules
//     - SPEC_RULE annotations present
//
//   Lines 156-185: StripBOM function
//     - Implements StripBOM-Empty, StripBOM-None, StripBOM-Start, StripBOM-Embedded
//     - Handles leading BOM removal and embedded BOM detection
//     - SPEC_RULE annotations present
//
//   Lines 187-223: NormalizeLF function
//     - Implements Norm-Empty, Norm-CRLF, Norm-CR, Norm-LF, Norm-Other
//     - Converts all line endings to LF
//     - SPEC_RULE annotations present
//
// ---------------------------------------------------------------------------
// SOURCE FILE: C:\Dev\Cursive\cursive-bootstrap\src\00_core\unicode.cpp
// ---------------------------------------------------------------------------
//   Lines 305-333: Prohibited character checking
//     - IsProhibited (305-311): Checks General_Category = Cc exclusions
//     - FirstProhibitedOutsideLiteral (313-328): Finds first prohibited outside literals
//     - NoProhibited (330-333): Validates no prohibited chars (WF-Prohibited)
//
//   Lines 114-302: Literal span scanning helpers (for LiteralSpan computation)
//     - ByteSpan struct (116-119)
//     - IsHexDigit, HexValue, IsUnicodeScalarValue (121-141)
//     - IsStringChar, IsCharContent (143-149)
//     - ScanEscape (151-204): Escape sequence validation
//     - ScanStringLiteral (206-235): String literal boundary detection
//     - ScanCharLiteral (237-266): Char literal boundary detection
//     - LiteralByteSpans (268-288): Collects all literal spans
//     - ByteInLiteralSpan (290-301): Tests if offset is inside a literal
//
// ---------------------------------------------------------------------------
// SOURCE FILE: C:\Dev\Cursive\cursive-bootstrap\include\cursive0\00_core\source_text.h
// ---------------------------------------------------------------------------
//   Lines 13-19: Type definitions
//     - UnicodeScalar = uint32_t
//     - kLF = 0x0A, kCR = 0x0D, kBOM = 0xFEFF
//
//   Lines 20-31: Utf8Len function (inline)
//     - Returns byte length for encoding a scalar
//
//   Lines 33-43: Utf8Offsets function (inline)
//     - Computes byte offset for each scalar position
//
//   Lines 45-59: LineStarts function (inline)
//     - Computes byte offsets of line start positions
//
//   Lines 61-91: Additional encoding helpers
//     - ByteLenFromScalars, AppendUtf8, EncodeUtf8
//
//   Lines 93-108: Result types
//     - DecodeResult { scalars, ok }
//     - StripBOMResult { scalars, had_bom, embedded_index }
//
//   Lines 110-118: SourceFile struct
//     - path, bytes, scalars, text, byte_len, line_starts, line_count
//
// ---------------------------------------------------------------------------
// DEPENDENCIES
// ---------------------------------------------------------------------------
//   Required types:
//   - UnicodeScalar (uint32_t)
//   - SourceFile struct
//   - DecodeResult, StripBOMResult structs
//   - Span type (from 01_core/span)
//
//   Required helpers:
//   - Utf8Len, Utf8Offsets, LineStarts, EncodeUtf8 (from source_text.h)
//   - SpanOf (from span.cpp)
//   - Diagnostic emission (MakeDiagnostic, Emit)
//
// ---------------------------------------------------------------------------
// REFACTORING NOTES
// ---------------------------------------------------------------------------
//   1. Consider separating into smaller focused files:
//      - utf8_codec.cpp: Decode, EncodeUtf8, Utf8Len, Utf8Offsets
//      - bom_handling.cpp: StripBOM, FirstBomIndex
//      - line_normalization.cpp: NormalizeLF, LineStarts
//      - prohibited_check.cpp: IsProhibited, FirstProhibitedOutsideLiteral
//
//   2. The literal span scanning is duplicated for lexer security checks.
//      Consider extracting to a shared literal_scanner module.
//
//   3. Performance optimization: The current implementation makes multiple
//      passes over the scalar sequence. Consider a single-pass approach for
//      combined normalization + line mapping + prohibited checking.
//
//   4. Error reporting could be improved with more specific error codes for:
//      - Invalid UTF-8 sequence type (overlong, truncated, invalid lead byte)
//      - Specific prohibited character category
//
//   5. The source_text.h helpers are inline; consider whether they should
//      remain inline or move to source_normalize.cpp for better compilation.
//
// ===========================================================================

// TODO: Migrate implementation from source files listed above

