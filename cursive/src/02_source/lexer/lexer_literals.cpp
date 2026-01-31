// =============================================================================
// MIGRATION MAPPING: lexer_literals.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 3.2.6 - Literal Lexing (lines 2170-2364)
//
// SOURCE FILE: cursive-bootstrap/src/02_syntax/lexer_literals.cpp
//   Lines 1-641 (entire file)
//
// DEPENDENCIES:
//   - cursive/include/02_source/lexer.h (LiteralScanResult, ScalarRange)
//   - cursive/src/00_core/unicode.cpp (UnicodeScalar, kLF, Utf8Offsets)
//   - cursive/src/00_core/diagnostic_messages.cpp (MakeDiagnostic, Emit)
//   - cursive/src/00_core/span.cpp (SpanOf)
//
// =============================================================================
// CONTENT TO MIGRATE:
// =============================================================================
//
// INTERNAL HELPERS (anonymous namespace, source lines 18-383):
//
// 1. DigitScanResult struct (lines 22-26)
//    {ok, malformed, next} - result of digit sequence scan
//
// 2. TerminatorResult struct (lines 28-31)
//    {index, closed} - result of literal terminator search
//
// 3. Digit predicates (lines 33-47):
//    - IsDecDigit(c): '0'-'9'  [spec line 2077]
//    - IsHexDigit(c): dec OR 'a'-'f' OR 'A'-'F'  [spec line 2078]
//    - IsOctDigit(c): '0'-'7'  [spec line 2079]
//    - IsBinDigit(c): '0' or '1'  [spec line 2080]
//
// 4. IsUnicodeScalarValue() (lines 49-54)
//    Valid range: 0 to 0x10FFFF, excluding surrogates 0xD800-0xDFFF
//    [spec line 2329: CharValueRange]
//
// 5. HexValue() (lines 56-64)
//    Convert hex char to numeric value
//
// 6. StartsWith(), EndsWith() (lines 66-73)
//    String prefix/suffix utilities
//
// 7. NumericUnderscoreOk() (lines 75-123)
//    Implements spec NumericUnderscoreOk from lines 2220-2225:
//    - NOT StartsWithUnderscore
//    - NOT EndsWithUnderscore
//    - NOT AfterBasePrefixUnderscore (0x_, 0o_, 0b_)
//    - NOT AdjacentExponentUnderscore (_e, e_, _E, E_)
//    - NOT BeforeSuffixUnderscore (_i8, _f32, etc.)
//
// 8. DecimalLeadingZero() (lines 125-134)
//    Implements spec DecimalLeadingZero from lines 2282-2283:
//    Multi-digit decimal starting with '0' (after removing underscores)
//    Triggers warning W-SRC-0301
//
// 9. ScanDigits() (lines 136-169)
//    Generic digit sequence scanner with underscore support
//    Returns {ok, malformed, next}
//    Implements spec DecRun/HexRun/OctRun/BinRun from lines 2229-2232
//
// 10. ScanEscapeMatch() (lines 171-225)
//     Implements spec EscapeMatch from line 2309:
//     Valid escapes: \\, \", \', \n, \r, \t, \0, \xHH, \u{H+}
//     [spec SimpleEscape at line 2205]
//     [spec EscapeOk predicates at lines 2206-2208]
//
// 11. FirstBadEscape() (lines 227-244)
//     Implements spec FirstBadEscape from line 2311:
//     Find first invalid escape sequence in literal
//
// 12. CharScalarCount() (lines 246-267)
//     Implements spec CharScalarCount from line 2341:
//     Count scalar values in char literal (should be exactly 1)
//
// 13. FindTerminator() (lines 269-296)
//     Find closing quote handling backslash escapes
//     Implements spec BackslashCount/UnescapedQuote from lines 2305-2306
//
// 14. MatchSuffix() (lines 298-310)
//     Match suffix string at position
//
// 15. MatchIntSuffix() (lines 312-326)
//     Implements spec IntSuffixSet from line 2217:
//     {i8, i16, i32, i64, i128, u8, u16, u32, u64, u128, isize, usize}
//
// 16. MatchFloatSuffix() (lines 328-350)
//     Implements spec FloatSuffixSet from line 2218:
//     {f, f16, f32, f64}
//     Note: 'f' alone is valid (width inferred from context, defaults f32)
//
// 17. SpanOfText(), LexemeSlice() (lines 352-371)
//     Utility functions for span/lexeme extraction
//
// 18. EmitDiag() (lines 373-381)
//     Diagnostic emission helper
//
// MAIN FUNCTIONS:
//
// 19. ScanIntLiteral() (lines 385-464)
//     Implements spec (Lex-Int) from lines 2263-2266:
//       DecDigit(T[i]) AND j = NumericScanEnd(T, i) AND NumericKind = IntLiteral
//
//     Handles:
//     - Decimal: digit+
//     - Hex: 0x digit+
//     - Octal: 0o digit+
//     - Binary: 0b digit+
//     - Optional suffix from IntSuffixSet
//
//     Errors:
//     - E-SRC-0304 for malformed literals
//     - W-SRC-0301 for decimal leading zeros
//
// 20. ScanFloatLiteral() (lines 466-555)
//     Implements spec (Lex-Float) from lines 2268-2271:
//       DecDigit(T[i]) AND j = NumericScanEnd(T, i) AND NumericKind = FloatLiteral
//
//     Format: integer_part "." fractional_part? exponent? float_suffix
//     CRITICAL: Float suffix is REQUIRED per spec line 2189-2190
//
//     Handles:
//     - Decimal core: digit+ "." digit*
//     - Exponent: [eE][+-]?digit+
//     - Required suffix: f, f16, f32, f64
//
//     Special case: "1.." (range) NOT float - check T[p+1] != '.'
//
// 21. ScanStringLiteral() (lines 557-593)
//     Implements spec (Lex-String) from lines 2300-2303
//
//     Related rules:
//     - (Lex-String-Unterminated) line 2313-2316
//     - (Lex-String-BadEscape) line 2318-2321
//
//     Errors:
//     - E-SRC-0301 for unterminated
//     - E-SRC-0302 for bad escape
//
// 22. ScanCharLiteral() (lines 595-638)
//     Implements spec (Lex-Char) from lines 2323-2326
//
//     Related rules:
//     - (Lex-Char-Unterminated) line 2343-2346
//     - (Lex-Char-BadEscape) line 2348-2351
//     - (Lex-Char-Invalid) line 2353-2356
//
//     Errors:
//     - E-SRC-0303 for unterminated or invalid (not exactly 1 scalar)
//     - E-SRC-0302 for bad escape
//
// =============================================================================
// SPEC GRAMMAR (lines 2176-2202):
// =============================================================================
//
// integer_literal  ::= (decimal_integer | hex_integer | octal_integer | binary_integer) int_suffix?
// decimal_integer  ::= dec_digit ("_"* dec_digit)*
// hex_integer      ::= "0x" hex_digit ("_"* hex_digit)*
// octal_integer    ::= "0o" oct_digit ("_"* oct_digit)*
// binary_integer   ::= "0b" bin_digit ("_"* bin_digit)*
// int_suffix       ::= "i8" | "i16" | "i32" | "i64" | "i128" | "u8" | "u16" | "u32" | "u64" | "u128" | "isize" | "usize"
//
// float_literal ::= decimal_integer "." decimal_integer? exponent? float_suffix
// exponent      ::= ("e" | "E") ("+" | "-")? decimal_integer
// float_suffix  ::= "f" | "f16" | "f32" | "f64"
//
// string_literal   ::= '"' (string_char | escape_sequence)* '"'
// char_literal     ::= "'" (char_content | escape_sequence) "'"
// escape_sequence  ::= "\n" | "\r" | "\t" | "\\" | "\"" | "\'" | "\0" | "\x" hex hex | "\u{" hex+ "}"
//
// =============================================================================
// REFACTORING NOTES:
// =============================================================================
//
// - Consider using std::from_chars for hex parsing where available
// - ScanDigits uses callback predicate; could be template parameter
// - Escape sequence validation is thorough; consider table-driven approach
// - CharScalarCount handles escape sequences correctly
//
// - CRITICAL: Float suffix is mandatory per spec 3.2.6 line 2189-2190
//   "Float literals MUST have a suffix"
//   This avoids ambiguity with tuple access (t.0.0)
//
// - Leading zero warning (W-SRC-0301) only for decimal, not based integers
//
// - Unicode escape \u{...} validates:
//   1. 1-6 hex digits
//   2. Value is valid Unicode scalar (not surrogate)
//
// =============================================================================

#include "cursive/include/02_source/lexer.h"

// TODO: Migrate from cursive-bootstrap/src/02_syntax/lexer_literals.cpp
