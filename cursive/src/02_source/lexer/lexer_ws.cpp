// =============================================================================
// MIGRATION MAPPING: lexer_ws.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 3.2.2 - Character Classes (Whitespace, LineFeed) (lines 2052-2058)
//   Section 3.2.5 - Comment and Whitespace Scanning (lines 2119-2169)
//
// SOURCE FILE: cursive-bootstrap/src/02_syntax/lexer_ws.cpp
//   Lines 1-171 (entire file)
//
// DEPENDENCIES:
//   - cursive/include/02_source/lexer.h (CommentScanResult, DocComment, DocKind, ScalarRange)
//   - cursive/src/00_core/unicode.cpp (UnicodeScalar, kLF, Utf8Offsets, EncodeUtf8)
//   - cursive/src/00_core/span.cpp (SpanOf)
//   - cursive/src/00_core/diagnostic_messages.cpp (MakeDiagnostic, Emit)
//
// =============================================================================
// CONTENT TO MIGRATE:
// =============================================================================
//
// INTERNAL HELPERS (anonymous namespace, source lines 15-63):
//
// 1. Match2() (lines 17-22)
//    Check if two consecutive scalars match given characters
//    Used for "//" and "/*" detection
//
// 2. DocMarker() (lines 24-36)
//    Implements spec DocMarker from lines 2026-2029:
//    DocMarker(T, i) =
//      LineDoc   if T[i..i+3] = "///"
//      ModuleDoc if T[i..i+3] = "//!"
//      bottom    otherwise
//
// 3. DocBody() (lines 38-52)
//    Implements spec DocBody from line 2025:
//    DocBody(T, i, j) = StripLeadingSpace(T[i+3..j))
//    Where StripLeadingSpace removes leading U+0020 if present
//
// 4. SpanOfText() (lines 54-62)
//    Convert scalar indices to Span using Utf8Offsets
//
// MAIN FUNCTIONS:
//
// 5. IsWhitespace() (lines 66-68)
//    Implements spec Whitespace from line 2054:
//    Whitespace(c) = c in {U+0020, U+0009, U+000C}
//    (space, tab, form feed)
//
// 6. IsLineFeed() (lines 70-72)
//    Implements spec LineFeed from line 2058:
//    LineFeed(c) = c = U+000A
//
// 7. ScanLineComment() (lines 74-97)
//    Implements spec (Scan-Line-Comment) from lines 2125-2128:
//      T[i] = '/' AND T[i+1] = '/'
//      j = min{p | i <= p AND (p = |T| OR T[p] = LF)}
//      => ScanLineComment(T, i) => j
//
//    Scans from "//" to end of line (before LF) or end of file
//    Returns: {ok, next, range}
//
// 8. ScanDocComment() (lines 99-115)
//    Implements spec (Doc-Comment) from lines 2134-2137:
//      ScanLineComment(T, i) => j
//      kind = DocMarker(T, i) != bottom
//      body = DocBody(T, i, j)
//      => DocComment(T, i) => <kind, body, span>
//
//    Extends ScanLineComment to extract doc comment metadata
//    Returns: CommentScanResult with optional DocComment
//
// 9. ScanBlockComment() (lines 117-168)
//    Implements spec block comment rules from lines 2143-2168:
//
//    (Block-Start) line 2145-2148:
//      T[i..i+2] = "/*" => depth += 1, i += 2
//
//    (Block-End) line 2150-2153:
//      T[i..i+2] = "*/" AND depth > 1 => depth -= 1, i += 2
//
//    (Block-Done) line 2155-2158:
//      T[i..i+2] = "*/" AND depth = 1 => return i+2
//
//    (Block-Step) line 2160-2163:
//      T[i..i+2] != "/*" AND T[i..i+2] != "*/" => i += 1
//
//    (Block-Comment-Unterminated) line 2165-2168:
//      Reached end of file with depth > 0 => E-SRC-0306
//
//    Key: Block comments NEST in Cursive (unlike C)
//    /* outer /* inner */ still open */
//
// =============================================================================
// SPEC DEFINITIONS:
// =============================================================================
//
// DocComment record (line 2019):
//   DocComment = <kind, text, span>
//
// DocKind enum (line 2021):
//   DocKind = {LineDoc, ModuleDoc}
//
// StripLeadingSpace (lines 2022-2024):
//   Remove single leading space from doc body if present
//
// BlockState (line 2143):
//   BlockState = {BlockScan(T, i, d, i_0), BlockDone(j)}
//   d = nesting depth, i_0 = start position (for error span)
//
// =============================================================================
// REFACTORING NOTES:
// =============================================================================
//
// - Block comments nest; this differs from C/C++
// - Doc comments are "///" (line) and "//!" (module)
// - Doc body strips exactly one leading space if present
//
// - ScanLineComment returns range [start, j) where j is before LF
// - ScanBlockComment returns range [start, j) where j is after "*/"
//
// - Unterminated block comment (E-SRC-0306) span is "/*" at start
//
// - The spec explicitly excludes CR (U+000D) from whitespace
//   CR-LF sequences: CR is NOT whitespace, LF triggers newline token
//   This means "\r\n" is treated as two characters, not one
//
// - Consider: Should CRLF normalization happen in source loading?
//   Current implementation treats CR as non-whitespace
//
// =============================================================================

#include "cursive/include/02_source/lexer.h"

// TODO: Migrate from cursive-bootstrap/src/02_syntax/lexer_ws.cpp
