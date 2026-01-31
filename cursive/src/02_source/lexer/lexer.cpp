// =============================================================================
// MIGRATION MAPPING: lexer.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 3.2.8 - Operator and Punctuator Lexing (lines 2394-2404)
//   Section 3.2.9 - Maximal-Munch Rule (lines 2405-2445)
//   Section 3.2.11 - Tokenization Small-Step (Lex-Newline at line 2519-2522)
//
// SOURCE FILE: cursive-bootstrap/src/02_syntax/lexer.cpp
//   Lines 1-407 (entire file)
//
// DEPENDENCIES:
//   - cursive/include/02_source/lexer.h (NextTokenResult, etc.)
//   - cursive/include/02_source/token.h (Token, TokenKind)
//   - cursive/src/00_core/keywords.cpp (kCursive0Operators, kCursive0Punctuators)
//   - cursive/src/00_core/unicode.cpp (IsIdentStart, Utf8Offsets)
//   - cursive/src/02_source/lexer/lexer_ident.cpp (ScanIdentToken)
//   - cursive/src/02_source/lexer/lexer_literals.cpp (ScanStringLiteral, etc.)
//
// =============================================================================
// CONTENT TO MIGRATE:
// =============================================================================
//
// INTERNAL HELPERS (anonymous namespace, source lines 14-191):
//
// 1. IsInRange() - Range check (lines 16-18)
// 2. IsSuppressed() - Check if index in suppressed ranges (lines 20-27)
//    Used for filtering newlines inside comments/literals
//
// 3. IsPunc() - Punctuator token check (lines 29-31)
// 4. BeginsOperand() - Check if token can start operand (lines 33-63)
//    Used for newline filtering heuristics
//    Includes keywords: if, match, loop, unsafe, move, transmute, widen,
//                       parallel, spawn, dispatch, yield, sync, race, all
//
// 5. IsAmbigOp() - Ambiguous operator check (lines 65-68)
//    Operators that can be binary or unary: +, -, *, &, |
//
// 6. IsUnaryOnly() - Unary-only operator check (lines 70-72)
//    Operators: !, ~, ?
//
// 7. DeltaDepth() - Bracket depth delta (lines 74-85)
//    Returns +1 for (, [, {; -1 for ), ], }
//
// 8. Candidate struct - Lexer candidate (lines 87-91)
//    {kind, next, diags}
//
// 9. KindPriority() - Token kind priority for disambiguation (lines 93-112)
//    Implements spec KindPriority from lines 2419-2428:
//    - Literals: priority 3
//    - Identifier/Keyword: priority 2
//    - Operator: priority 1
//    - Punctuator: priority 0
//
// 10. IsDecDigit() - Decimal digit check (lines 114-116)
// 11. IsQuote() - Quote character check (lines 118-120)
//
// 12. HasFloatSuffix() - Check for float suffix (lines 122-147)
//     SPEC: Float literals MUST have suffix (f, f16, f32, f64)
//     This is critical for disambiguating from tuple access (t.0.0)
//
// 13. MatchLexeme() - Match prefix against scalars (lines 150-163)
//
// 14. AppendOpCandidates() - Add operator candidates (lines 165-176)
//     Implements spec OpMatch(T, i) from line 2399
//     Iterates kCursive0Operators, matches prefix
//
// 15. AppendPuncCandidates() - Add punctuator candidates (lines 178-189)
//     Implements spec PuncMatch(T, i) from line 2400
//     Iterates kCursive0Punctuators, matches prefix
//
// MAIN FUNCTIONS:
//
// 16. NextToken() - Maximal munch token selection (lines 193-290)
//     Implements spec Max-Munch rule (lines 2432-2435):
//       PickLongest(C) = argmax_{(k, j) in C} <j, KindPriority(k)>
//
//     Steps:
//       a. Build candidate set based on first character:
//          - Quote: try string/char literals
//          - Digit: try float/int literals (float only if has suffix)
//          - IdentStart: try identifier token
//          - Otherwise: try operators/punctuators
//       b. If no candidates: emit E-SRC-0309 (Max-Munch-Err)
//       c. Select longest match, break ties by KindPriority
//
//     CRITICAL: Float detection requires HasFloatSuffix check
//     SPEC line 2189-2190: Float suffix REQUIRED to avoid ambiguity
//
// 17. LexNewlines() - Extract newline tokens (lines 292-318)
//     Implements spec Lex-Newline from line 2519-2522
//     Filters newlines in suppressed ranges (inside literals/comments)
//
// 18. FilterNewlines() - ASI-style newline filtering (lines 320-405)
//     NOT IN SPEC - implementation-specific heuristics
//
//     Filtering rules:
//     a. Inside ( ) or [ ]: filter (expression context)
//     b. Inside { }: keep (statement context)
//     c. After comma: filter (continuation)
//     d. After binary operator with following operand: filter
//     e. Before ., ::, ~>: filter (method chaining)
//
// =============================================================================
// REFACTORING NOTES:
// =============================================================================
//
// - SPEC line 2112 defines OperatorSet (44 operators)
// - SPEC line 2115 defines PunctuatorSet (12 punctuators)
// - SPEC line 2117: OperatorSet INTERSECT PunctuatorSet = empty
//
// - Candidate generation follows spec Candidates(T, i) at line 2410-2415:
//   - IsQuote: StringTok + CharTok
//   - DecDigit: FloatTok + IntTok
//   - IdentStart: IdentToken
//   - Otherwise: OpTok + PuncTok
//
// - PickLongest implements spec Longest() from line 2417
// - KindPriority provides stable ordering for tie-breaking
//
// - FilterNewlines is NOT spec-defined; it implements newline-as-terminator
//   semantics from Cursive grammar. Consider documenting the algorithm.
//
// - The float suffix requirement (HasFloatSuffix) is critical:
//   Without it, "1.0" would be FloatLiteral, breaking "tuple.0.0" parsing
//
// =============================================================================

#include "cursive/include/02_source/lexer.h"

// TODO: Migrate from cursive-bootstrap/src/02_syntax/lexer.cpp
