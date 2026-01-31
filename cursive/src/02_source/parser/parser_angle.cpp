// =============================================================================
// MIGRATION MAPPING: parser_angle.cpp
// =============================================================================
//
// DESTINATION: cursive/src/02_source/parser/parser_angle.cpp
// PURPOSE: Angle bracket handling for generic parameters/arguments
//
// =============================================================================
// SPEC REFERENCES
// =============================================================================
//
// C0updated.md Section 3.3.3 (Lines 3004-3016): SplitShiftR and angle handling
//
// **SplitSpan2 (lines 3004-3011)**:
//   SplitSpan2(sp) = (sp_L, sp_R) where:
//   - sp_L.file = sp.file, sp_R.file = sp.file
//   - sp_L.start_offset = sp.start_offset, sp_L.end_offset = sp.start_offset + 1
//   - sp_R.start_offset = sp.start_offset + 1, sp_R.end_offset = sp.start_offset + 2
//   - Similar for line/col fields
//
// **SplitShiftR (lines 3013-3015)**:
//   SplitShiftR(P) = <K', i, D, j, d, Delta>
//   where Tok(P) = <Operator(">>"), ">>", sp> && (sp_L, sp_R) = SplitSpan2(sp)
//   K' = K[0..i) ++ [<Operator(">"), ">", sp_L>, <Operator(">"), ">", sp_R>] ++ K[i+1..]
//
// This is used to handle cases like `Vec<Vec<i32>>` where `>>` must be split
// into two `>` tokens for proper generic argument parsing.
//
// C0updated.md Section 3.3.4 (Lines 3056-3057): Type argument start token
//   TypeArgsStartTok(t) <=> IsOp(t, "<")
//
// =============================================================================
// SOURCE FILES
// =============================================================================
//
// PRIMARY SOURCE: cursive-bootstrap/src/02_syntax/parser_angle.cpp
//   Lines 1-113: Complete file
//
// FUNCTIONS TO MIGRATE:
//
// 1. SplitSpan2 (lines 7-27):
//    SPEC: Section 3.3.3 lines 3004-3011
//    Splits a 2-character span into two 1-character spans
//
//    Source:
//      std::pair<core::Span, core::Span> SplitSpan2(const core::Span& sp) {
//        core::Span left = sp;
//        core::Span right = sp;
//        left.start_offset = sp.start_offset;
//        left.end_offset = sp.start_offset + 1;
//        right.start_offset = sp.start_offset + 1;
//        right.end_offset = sp.start_offset + 2;
//        left.start_line = sp.start_line;
//        left.end_line = sp.start_line;
//        right.start_line = sp.start_line;
//        right.end_line = sp.start_line;
//        left.start_col = sp.start_col;
//        left.end_col = sp.start_col + 1;
//        right.start_col = sp.start_col + 1;
//        right.end_col = sp.start_col + 2;
//        return {left, right};
//      }
//
// 2. SplitShiftR (lines 29-64):
//    SPEC: Section 3.3.3 lines 3013-3015
//    Splits ">>" token into two ">" tokens for nested generics
//
//    Source:
//      Parser SplitShiftR(const Parser& parser) {
//        if (!parser.tokens || parser.index >= parser.tokens->size()) {
//          return parser;
//        }
//        const Token& tok = (*parser.tokens)[parser.index];
//        if (tok.kind != TokenKind::Operator || tok.lexeme != ">>") {
//          return parser;
//        }
//        const auto spans = SplitSpan2(tok.span);
//        Token left = tok;
//        left.kind = TokenKind::Operator;
//        left.lexeme = ">";
//        left.span = spans.first;
//        Token right = tok;
//        right.kind = TokenKind::Operator;
//        right.lexeme = ">";
//        right.span = spans.second;
//        std::vector<Token> updated;
//        updated.reserve(parser.tokens->size() + 1);
//        for (std::size_t i = 0; i < parser.tokens->size(); ++i) {
//          if (i == parser.index) {
//            updated.push_back(left);
//            updated.push_back(right);
//            continue;
//          }
//          updated.push_back((*parser.tokens)[i]);
//        }
//        Parser out = parser;
//        out.owned_tokens = std::make_shared<std::vector<Token>>(std::move(updated));
//        out.tokens = out.owned_tokens.get();
//        return out;
//      }
//
// 3. AngleDelta (lines 66-80):
//    Helper to compute depth change from angle bracket tokens
//
//    Source:
//      int AngleDelta(const Token& tok) {
//        if (tok.kind != TokenKind::Operator) { return 0; }
//        if (tok.lexeme == "<") { return 1; }
//        if (tok.lexeme == ">") { return -1; }
//        if (tok.lexeme == ">>") { return -2; }
//        return 0;
//      }
//
// 4. AngleStepResult struct (header lines 61-64):
//    Result type for AngleStep
//
//    Source (from parser.h):
//      struct AngleStepResult {
//        Parser parser;
//        int depth = 0;
//      };
//
// 5. AngleStep (lines 82-91):
//    Advances parser and updates angle bracket depth
//
//    Source:
//      AngleStepResult AngleStep(const Parser& parser, int depth) {
//        AngleStepResult result;
//        result.parser = parser;
//        result.depth = depth;
//        if (const Token* tok = Tok(parser)) {
//          result.depth = depth + AngleDelta(*tok);
//        }
//        Advance(result.parser);
//        return result;
//      }
//
// 6. AngleScan (lines 93-107):
//    Scans forward to find matching closing angle bracket
//
//    Source:
//      Parser AngleScan(const Parser& start, const Parser& parser, int depth) {
//        Parser current = parser;
//        int d = depth;
//        for (;;) {
//          if (AtEof(current)) { return start; }
//          AngleStepResult step = AngleStep(current, d);
//          current = step.parser;
//          d = step.depth;
//          if (d == 0) { return current; }
//        }
//      }
//
// 7. SkipAngles (lines 109-111):
//    Convenience wrapper to skip angle-bracketed content
//
//    Source:
//      Parser SkipAngles(const Parser& parser) {
//        return AngleScan(parser, parser, 0);
//      }
//
// =============================================================================
// DEPENDENCIES
// =============================================================================
//
// REQUIRED HEADERS:
//   - cursive/include/02_source/parser/parser.hpp
//   - cursive/include/02_source/lexer/token.hpp
//   - cursive/include/00_core/span.hpp
//   - <utility> (for std::pair)
//   - <memory> (for std::shared_ptr)
//   - <vector>
//
// REQUIRED FUNCTIONS:
//   - Tok (parser_state.cpp)
//   - Advance (parser_state.cpp)
//   - AtEof (parser_state.cpp)
//
// TYPES REQUIRED:
//   - Parser
//   - Token, TokenKind
//   - core::Span
//   - AngleStepResult
//
// =============================================================================
// REFACTORING NOTES
// =============================================================================
//
// 1. OWNED_TOKENS PATTERN:
//    SplitShiftR modifies the token stream by creating a new vector and
//    storing it in parser.owned_tokens. This pattern is important:
//    - The new vector is heap-allocated via std::make_shared
//    - parser.tokens points to owned_tokens.get()
//    - The shared_ptr keeps the vector alive
//
// 2. NO SPEC_RULE MARKERS:
//    The source file doesn't have explicit SPEC_RULE markers. Consider adding:
//    - "SplitSpan2" for the span splitting
//    - "SplitShiftR" for the token splitting
//
// 3. ANGLE BRACKET DISAMBIGUATION:
//    This module handles a classic parsing challenge: `>>` can be either:
//    - Right shift operator: `x >> 2`
//    - Two closing angle brackets: `Vec<Vec<i32>>`
//    The parser uses context (depth tracking) to decide when to split.
//
// 4. ANGLESCAN USAGE:
//    AngleScan is used for lookahead to find the end of generic arguments.
//    If it reaches EOF without finding balanced brackets, it returns the
//    start position (indicating no valid generic argument list).
//
// 5. DEPTH TRACKING:
//    AngleDelta returns:
//    - +1 for "<" (opening)
//    - -1 for ">" (closing)
//    - -2 for ">>" (two closings)
//    The ">>" case is handled before SplitShiftR would split it.
//
// 6. SKIP_ANGLES VS PARSE_GENERIC_ARGS:
//    SkipAngles is for lookahead/validation only; it doesn't construct AST.
//    Actual generic argument parsing is in parser_types.cpp.
//
// 7. NAMESPACE:
//    Source uses cursive0::syntax. Update to target namespace.
//
// =============================================================================
