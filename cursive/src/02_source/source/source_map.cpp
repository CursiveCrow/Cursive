// ===========================================================================
// MIGRATION MAPPING: source_map.cpp
// ===========================================================================
//
// PURPOSE:
//   Source location and span computation: mapping byte offsets to line/column,
//   creating spans, clamping spans to valid ranges.
//
// ---------------------------------------------------------------------------
// SPEC REFERENCE: C:\Dev\Cursive\C0updated.md
// ---------------------------------------------------------------------------
//   Section 1.6.1 - Span and SourceLocation types (referenced from 3.1)
//   Section 3.1 - Source Loading and Normalization
//
//   Key definitions:
//   - Lines 1812-1825: Logical Line Map and Locate function
//     * Utf8Offsets([]) = [0] (1814)
//     * Utf8Offsets(c::cs) = [0] ++ [o + Utf8Len(c) | o in Utf8Offsets(cs)] (1815)
//     * LineStarts(T) = [0] ++ [Utf8Offsets(T)[i] + 1 | 0 <= i < |T| && T[i] = LF] (1817)
//     * LineCount(T) = |LineStarts(T)| (1818)
//     * Locate(S, o) formula (1820-1825):
//       - L = S.line_starts
//       - o' = min(o, S.byte_len)
//       - k = max{ j | L[j] <= o' }
//       - Result: (file = S.path, offset = o', line = k + 1, col = o' - L[k] + 1)
//
//   - Lines 1958-1993: Diagnostic Spans for Source Loading
//     * S_tmp construction (1960)
//     * O = Utf8Offsets(T), O[|T|] = ByteLen(T) (1962-1963)
//     * SpanAtIndex(T, i) = SpanOf(S_tmp, O[i], O[i+1]) (1965)
//     * SpanAtLineStart(T, k) computation (1967-1971)
//
//   - Lines 2039-2046: Lexer span helpers (in Section 3.2.1)
//     * ScalarIndex(T) = { i | 0 <= i <= |T| } (2040)
//     * ByteOf(T, i) = O[i] (2042)
//     * SpanOfText(S, i, j) = SpanOf(S, ByteOf(T, i), ByteOf(T, j)) (2044)
//     * Lexeme(T, i, j) = T[i..j) (2046)
//
//   - Lines 2473-2479: Span construction helpers (in Section 3.2.10)
//     * SpanFrom(t_a, t_b) = (t_a.span.file, t_a.span.start_offset, ...) (2475)
//
// ---------------------------------------------------------------------------
// SOURCE FILE: C:\Dev\Cursive\cursive-bootstrap\src\00_core\span.cpp
// ---------------------------------------------------------------------------
//   Lines 9-12: SpecDefsSpanTypes
//     - SPEC_DEF for SourceLocation and Span from 1.6.1
//
//   Lines 14-17: SpanRange function
//     - Returns (start_offset, end_offset) pair from Span
//
//   Lines 19-38: Locate function
//     - Implements the Locate(S, o) judgment from spec
//     - Uses binary search (upper_bound) on line_starts
//     - Computes line number (1-indexed) and column
//     - SPEC_RULE("WF-Location")
//
//   Lines 40-48: ClampSpan function
//     - Clamps start/end to valid byte range [0, source.byte_len]
//     - Ensures start <= end
//
//   Lines 50-66: SpanOf function
//     - Main span construction from source and byte range
//     - Calls ClampSpan, then Locate for start/end positions
//     - Fills all Span fields: file, offsets, lines, columns
//     - SPEC_RULE("Span-Of") and SPEC_RULE("WF-Span")
//
// ---------------------------------------------------------------------------
// SOURCE FILE: C:\Dev\Cursive\cursive-bootstrap\src\00_core\source_load.cpp
// ---------------------------------------------------------------------------
//   Lines 133-139: SpanAtIndex helper
//     - Creates span for a single scalar index
//     - Uses Utf8Offsets to map scalar index to byte offsets
//
// ---------------------------------------------------------------------------
// SOURCE FILE: C:\Dev\Cursive\cursive-bootstrap\src\02_syntax\parser.cpp
// ---------------------------------------------------------------------------
//   Lines 30-36: SpanFrom helper (for token-to-token spans)
//     - Combines start token's start position with end token's end position
//     - Used for AST node spans covering multiple tokens
//
//   Lines 76-85: SpanBetween helper (for parser state spans)
//     - Creates span between two parser positions
//     - Handles EOF and missing tokens
//
// ---------------------------------------------------------------------------
// DEPENDENCIES
// ---------------------------------------------------------------------------
//   Required types:
//   - SourceFile struct (path, byte_len, line_starts, line_count)
//   - SourceLocation struct (file, offset, line, column)
//   - Span struct (file, start_offset, end_offset, start_line, start_col, end_line, end_col)
//   - Token struct (for token-based span helpers)
//
//   Required helpers:
//   - Utf8Offsets (from source_text.h or source_normalize)
//   - std::upper_bound (for binary search in Locate)
//
// ---------------------------------------------------------------------------
// REFACTORING NOTES
// ---------------------------------------------------------------------------
//   1. The Locate function is performance-critical during error reporting.
//      Consider caching line lookups or using interpolation search for
//      very large files.
//
//   2. SpanFrom and SpanBetween are parser-specific helpers that could
//      potentially live in parser utilities rather than core span module.
//
//   3. Consider adding span merging/union utilities for combining adjacent
//      spans (e.g., for multi-token AST nodes).
//
//   4. The current implementation creates new Span objects on each call.
//      For memory-sensitive scenarios, consider span interning or
//      a span table with indices.
//
//   5. Error case handling: When offset > byte_len, behavior should be
//      well-defined. Current ClampSpan handles this, but document the
//      invariant explicitly.
//
//   6. The line_starts vector must be computed before Locate can be called.
//      Consider lazy computation or validation that line_starts is populated.
//
// ===========================================================================

// TODO: Migrate implementation from source files listed above

