// =============================================================================
// MIGRATION MAPPING: tokenize.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 3.2.11 - Tokenization (Small-Step) (lines 2499-2567)
//   Section 3.2.12 - Tokenize (Big-Step) (lines 2568-2586)
//
// SOURCE FILE: cursive-bootstrap/src/02_syntax/tokenize.cpp
//   Lines 1-337 (entire file)
//
// DEPENDENCIES:
//   - cursive/include/02_source/lexer.h (LexSmallStepResult, TokenizeResult, etc.)
//   - cursive/src/02_source/lexer/lexer.cpp (NextToken)
//   - cursive/src/02_source/lexer/lexer_ws.cpp (ScanLineComment, ScanDocComment, ScanBlockComment)
//   - cursive/src/02_source/lexer/lexer_security.cpp (LexSecure)
//   - cursive/src/00_core/unicode.cpp (Utf8Offsets, IsSensitive)
//   - cursive/src/00_core/diagnostic_messages.cpp (MakeDiagnostic, Emit)
//
// =============================================================================
// CONTENT TO MIGRATE:
// =============================================================================
//
// INTERNAL HELPERS (anonymous namespace, source lines 19-171):
//
// 1. DebugLexFail() - Debug output for lex failures (lines 21-72)
//    Triggered by CURSIVE0_DEBUG_LEX environment variable
//    Outputs scalar index, byte index, codepoint, context window
//
// 2. TerminatorResult struct (lines 74-77)
//    Used for string/char literal terminator finding
//
// 3. MatchPrefix() - Multi-char prefix matching (lines 79-91)
//    Used for comment detection ("//", "/*", "///", "//!")
//
// 4. FindTerminator() - Find literal terminator (lines 93-120)
//    Implements: StringTerminator/CharTerminator from spec 3.2.6
//    Handles backslash escape counting for quote detection
//    SPEC: BackslashCount(T, p) = max{k | ... T[r] = "\\"}
//    SPEC: UnescapedQuote(T, p) = T[p] = '"' AND BackslashCount mod 2 = 0
//
// 5. StringTerminator() - wrapper (lines 122-125)
// 6. CharTerminator() - wrapper (lines 127-130)
//
// 7. LineFeedOrEOFBeforeClose() - (lines 132-136)
//    Implements spec predicate from line 2308
//    Used to detect unterminated literals
//
// 8. SpanOfText() - Span from scalar indices (lines 138-143)
//    Implements spec SpanOfText(S, i, j) from line 2044
//
// 9. LexemeSlice() - Extract lexeme substring (lines 145-152)
//    Implements spec Lexeme(T, i, j) from line 2046
//
// 10. AppendDiags() - Diagnostic stream merge (lines 154-159)
//
// 11. AppendSensitiveInSpan() - Collect sensitive positions (lines 161-170)
//     Implements spec SensitiveInSpan(T, i, j) from line 2460
//
// MAIN FUNCTIONS:
//
// 12. LexSmallStep() - Small-step tokenization (lines 174-310)
//     Implements spec rules from 3.2.11:
//     - Lex-Start (line 2505-2507)
//     - Lex-End (line 2509-2512)
//     - Lex-Whitespace (line 2514-2517)
//     - Lex-Newline (line 2519-2522)
//     - Lex-Doc-Comment (line 2529-2532)
//     - Lex-Line-Comment (line 2524-2527)
//     - Lex-Block-Comment (line 2534-2537)
//     - Lex-String-Unterminated-Recover (line 2539-2542)
//     - Lex-Char-Unterminated-Recover (line 2544-2547)
//     - Lex-Sensitive (line 2549-2552)
//     - Lex-Token (line 2558-2561)
//     - Lex-Token-Err (line 2563-2566)
//
//     Loop structure:
//       while i < |T|:
//         if Whitespace(T[i]): skip
//         if LineFeed(T[i]): emit Newline token
//         if "///"|"//!": ScanDocComment
//         if "//": ScanLineComment
//         if "/*": ScanBlockComment
//         if unterminated string: recover
//         if unterminated char: recover
//         if Sensitive(T[i]): record position
//         else: NextToken
//
// 13. Tokenize() - Big-step tokenization (lines 312-334)
//     Implements spec rules from 3.2.12:
//     - Tokenize-Err (line 2580-2583)
//     - Tokenize-Secure-Err (line 2575-2578)
//     - Tokenize-Ok (line 2570-2573)
//
//     Steps:
//       1. Run LexSmallStep
//       2. If failed, return error (Tokenize-Err)
//       3. Run LexSecure on result
//       4. If failed, return error (Tokenize-Secure-Err)
//       5. Return success (Tokenize-Ok)
//
// =============================================================================
// REFACTORING NOTES:
// =============================================================================
//
// - State machine could be refactored to use discriminated union
// - Consider separating debug code into separate translation unit
// - The spec defines LexState explicitly (line 2501):
//   LexState = {LexStart(S), LexScan(S, i, K, D, Sens), LexDone(K, D, Sens), LexError(code)}
// - Recovery behavior for unterminated literals continues tokenization
// - Sensitive character handling must occur BEFORE NextToken check
// - SensitiveTok helper (spec line 2554-2556) excludes string/char literals
//
// =============================================================================

#include "cursive/include/02_source/lexer.h"

// TODO: Migrate from cursive-bootstrap/src/02_syntax/tokenize.cpp
