#include "cursive0/02_syntax/parser.h"

#include <utility>

namespace cursive0::syntax {

std::pair<core::Span, core::Span> SplitSpan2(const core::Span& sp) {
  core::Span left = sp;
  core::Span right = sp;

  left.start_offset = sp.start_offset;
  left.end_offset = sp.start_offset + 1;
  right.start_offset = sp.start_offset + 1;
  right.end_offset = sp.start_offset + 2;

  left.start_line = sp.start_line;
  left.end_line = sp.start_line;
  right.start_line = sp.start_line;
  right.end_line = sp.start_line;

  left.start_col = sp.start_col;
  left.end_col = sp.start_col + 1;
  right.start_col = sp.start_col + 1;
  right.end_col = sp.start_col + 2;

  return {left, right};
}

Parser SplitShiftR(const Parser& parser) {
  if (!parser.tokens || parser.index >= parser.tokens->size()) {
    return parser;
  }
  const Token& tok = (*parser.tokens)[parser.index];
  if (tok.kind != TokenKind::Operator || tok.lexeme != ">>") {
    return parser;
  }

  const auto spans = SplitSpan2(tok.span);
  Token left = tok;
  left.kind = TokenKind::Operator;
  left.lexeme = ">";
  left.span = spans.first;

  Token right = tok;
  right.kind = TokenKind::Operator;
  right.lexeme = ">";
  right.span = spans.second;

  std::vector<Token> updated;
  updated.reserve(parser.tokens->size() + 1);
  for (std::size_t i = 0; i < parser.tokens->size(); ++i) {
    if (i == parser.index) {
      updated.push_back(left);
      updated.push_back(right);
      continue;
    }
    updated.push_back((*parser.tokens)[i]);
  }

  Parser out = parser;
  out.owned_tokens = std::make_shared<std::vector<Token>>(std::move(updated));
  out.tokens = out.owned_tokens.get();
  return out;
}

int AngleDelta(const Token& tok) {
  if (tok.kind != TokenKind::Operator) {
    return 0;
  }
  if (tok.lexeme == "<") {
    return 1;
  }
  if (tok.lexeme == ">") {
    return -1;
  }
  if (tok.lexeme == ">>") {
    return -2;
  }
  return 0;
}

AngleStepResult AngleStep(const Parser& parser, int depth) {
  AngleStepResult result;
  result.parser = parser;
  result.depth = depth;
  if (const Token* tok = Tok(parser)) {
    result.depth = depth + AngleDelta(*tok);
  }
  Advance(result.parser);
  return result;
}

Parser AngleScan(const Parser& start, const Parser& parser, int depth) {
  Parser current = parser;
  int d = depth;
  for (;;) {
    if (AtEof(current)) {
      return start;
    }
    AngleStepResult step = AngleStep(current, d);
    current = step.parser;
    d = step.depth;
    if (d == 0) {
      return current;
    }
  }
}

Parser SkipAngles(const Parser& parser) {
  return AngleScan(parser, parser, 0);
}

}  // namespace cursive0::syntax
