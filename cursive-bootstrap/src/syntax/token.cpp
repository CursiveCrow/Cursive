#include "cursive0/syntax/token.h"

#include "cursive0/core/assert_spec.h"

namespace cursive0::syntax {

static inline void SpecDefsTokenTypes() {
  SPEC_DEF("TokenKind", "1.6.2");
  SPEC_DEF("RawToken", "1.6.2");
  SPEC_DEF("Token", "1.6.2");
}

bool NoUnknownOk(const std::vector<Token>& tokens) {
  SPEC_RULE("No-Unknown-Ok");
  SpecDefsTokenTypes();
  for (const auto& tok : tokens) {
    if (tok.kind == TokenKind::Unknown) {
      return false;
    }
  }
  return true;
}

Token AttachSpan(const core::SourceFile& source, const RawToken& raw) {
  SPEC_RULE("Attach-Token-Ok");
  SpecDefsTokenTypes();
  Token tok;
  tok.kind = raw.kind;
  tok.lexeme = raw.lexeme;
  tok.span = core::SpanOf(source, raw.start_offset, raw.end_offset);
  return tok;
}

std::vector<Token> AttachSpans(const core::SourceFile& source,
                               const std::vector<RawToken>& raws) {
  SPEC_RULE("Attach-Tokens-Ok");
  SpecDefsTokenTypes();
  std::vector<Token> out;
  out.reserve(raws.size());
  for (const auto& raw : raws) {
    out.push_back(AttachSpan(source, raw));
  }
  return out;
}

EofToken MakeEofToken(const core::SourceFile& source) {
  SPEC_DEF("TokenEOF", "3.2.1");
  EofToken eof;
  eof.span = core::SpanOf(source, source.byte_len, source.byte_len);
  return eof;
}

}  // namespace cursive0::syntax
