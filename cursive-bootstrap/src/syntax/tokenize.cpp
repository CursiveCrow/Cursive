#include "cursive0/syntax/lexer.h"

#include <string>
#include <string_view>
#include <vector>

#include "cursive0/core/assert_spec.h"
#include "cursive0/core/diagnostic_messages.h"
#include "cursive0/core/source_text.h"
#include "cursive0/core/span.h"
#include "cursive0/core/unicode.h"

namespace cursive0::syntax {

namespace {

struct TerminatorResult {
  std::size_t index = 0;
  bool closed = false;
};

bool MatchPrefix(const std::vector<core::UnicodeScalar>& scalars,
                 std::size_t start,
                 std::string_view lexeme) {
  if (start + lexeme.size() > scalars.size()) {
    return false;
  }
  for (std::size_t i = 0; i < lexeme.size(); ++i) {
    if (scalars[start + i] != static_cast<unsigned char>(lexeme[i])) {
      return false;
    }
  }
  return true;
}

TerminatorResult FindTerminator(const std::vector<core::UnicodeScalar>& scalars,
                                std::size_t start,
                                core::UnicodeScalar quote) {
  TerminatorResult result;
  const std::size_t n = scalars.size();
  std::size_t backslashes = 0;
  for (std::size_t p = start + 1; p < n; ++p) {
    const core::UnicodeScalar c = scalars[p];
    if (c == core::kLF) {
      result.index = p;
      result.closed = false;
      return result;
    }
    if (c == '\\') {
      ++backslashes;
      continue;
    }
    if (c == quote && (backslashes % 2 == 0)) {
      result.index = p;
      result.closed = true;
      return result;
    }
    backslashes = 0;
  }
  result.index = n;
  result.closed = false;
  return result;
}

std::size_t StringTerminator(const std::vector<core::UnicodeScalar>& scalars,
                             std::size_t start) {
  return FindTerminator(scalars, start, '"').index;
}

std::size_t CharTerminator(const std::vector<core::UnicodeScalar>& scalars,
                           std::size_t start) {
  return FindTerminator(scalars, start, '\'').index;
}

bool LineFeedOrEOFBeforeClose(const std::vector<core::UnicodeScalar>& scalars,
                              std::size_t start,
                              core::UnicodeScalar quote) {
  return !FindTerminator(scalars, start, quote).closed;
}

core::Span SpanOfText(const core::SourceFile& source,
                      const std::vector<std::size_t>& offsets,
                      std::size_t i,
                      std::size_t j) {
  return core::SpanOf(source, offsets[i], offsets[j]);
}

std::string LexemeSlice(const core::SourceFile& source,
                        const std::vector<std::size_t>& offsets,
                        std::size_t i,
                        std::size_t j) {
  const std::size_t start = offsets[i];
  const std::size_t end = offsets[j];
  return source.text.substr(start, end - start);
}

void AppendDiags(core::DiagnosticStream& out,
                 const core::DiagnosticStream& add) {
  for (const auto& diag : add) {
    out = core::Emit(out, diag);
  }
}

void AppendSensitiveInSpan(const std::vector<core::UnicodeScalar>& scalars,
                           std::size_t i,
                           std::size_t j,
                           std::vector<std::size_t>& sens) {
  for (std::size_t p = i; p < j; ++p) {
    if (core::IsSensitive(scalars[p])) {
      sens.push_back(p);
    }
  }
}

}  // namespace

LexSmallStepResult LexSmallStep(const core::SourceFile& source) {
  SPEC_RULE("Lex-Start");
  LexSmallStepResult result;
  const auto& scalars = source.scalars;
  const auto offsets = core::Utf8Offsets(scalars);

  std::size_t i = 0;
  std::vector<Token> tokens;
  std::vector<DocComment> docs;
  std::vector<std::size_t> sensitive;

  while (true) {
    if (i >= scalars.size()) {
      SPEC_RULE("Lex-End");
      result.ok = true;
      break;
    }

    const core::UnicodeScalar c = scalars[i];

    if (IsWhitespace(c)) {
      SPEC_RULE("Lex-Whitespace");
      ++i;
      continue;
    }

    if (IsLineFeed(c)) {
      SPEC_RULE("Lex-Newline");
      Token tok;
      tok.kind = TokenKind::Newline;
      tok.lexeme = LexemeSlice(source, offsets, i, i + 1);
      tok.span = SpanOfText(source, offsets, i, i + 1);
      tokens.push_back(tok);
      ++i;
      continue;
    }

    if (MatchPrefix(scalars, i, "///") || MatchPrefix(scalars, i, "//!")) {
      SPEC_RULE("Lex-Doc-Comment");
      CommentScanResult doc = ScanDocComment(source, i);
      AppendDiags(result.diags, doc.diags);
      if (doc.ok && doc.doc.has_value()) {
        docs.push_back(*doc.doc);
        i = doc.next;
        continue;
      }
    }

    if (MatchPrefix(scalars, i, "//")) {
      SPEC_RULE("Lex-Line-Comment");
      CommentScanResult line = ScanLineComment(source, i);
      AppendDiags(result.diags, line.diags);
      if (line.ok) {
        i = line.next;
        continue;
      }
    }

    if (MatchPrefix(scalars, i, "/*")) {
      SPEC_RULE("Lex-Block-Comment");
      CommentScanResult block = ScanBlockComment(source, i);
      AppendDiags(result.diags, block.diags);
      if (!block.ok) {
        result.ok = false;
        result.error_code = "E-SRC-0306";
        break;
      }
      i = block.next;
      continue;
    }

    if (c == '"' && LineFeedOrEOFBeforeClose(scalars, i, '"')) {
      SPEC_RULE("Lex-String-Unterminated-Recover");
      const auto span = SpanOfText(source, offsets, i, i + 1);
      if (auto diag = core::MakeDiagnostic("E-SRC-0301", span)) {
        result.diags = core::Emit(result.diags, *diag);
      }
      const std::size_t term = StringTerminator(scalars, i);
      AppendSensitiveInSpan(scalars, i, term, sensitive);
      i = term;
      continue;
    }

    if (c == '\'' && LineFeedOrEOFBeforeClose(scalars, i, '\'')) {
      SPEC_RULE("Lex-Char-Unterminated-Recover");
      const auto span = SpanOfText(source, offsets, i, i + 1);
      if (auto diag = core::MakeDiagnostic("E-SRC-0303", span)) {
        result.diags = core::Emit(result.diags, *diag);
      }
      const std::size_t term = CharTerminator(scalars, i);
      AppendSensitiveInSpan(scalars, i, term, sensitive);
      i = term;
      continue;
    }

    if (core::IsSensitive(c)) {
      SPEC_RULE("Lex-Sensitive");
      sensitive.push_back(i);
      ++i;
      continue;
    }

    NextTokenResult next = NextToken(source, i);
    if (!next.ok) {
      SPEC_RULE("Lex-Token-Err");
      AppendDiags(result.diags, next.diags);
      result.ok = false;
      if (!next.diags.empty()) {
        result.error_code = next.diags.front().code;
      } else {
        result.error_code = "E-SRC-0309";
      }
      break;
    }

    SPEC_RULE("Lex-Token");
    AppendDiags(result.diags, next.diags);
    Token tok;
    tok.kind = next.kind;
    tok.lexeme = LexemeSlice(source, offsets, i, next.next);
    tok.span = SpanOfText(source, offsets, i, next.next);
    tokens.push_back(tok);

    if (tok.kind != TokenKind::StringLiteral &&
        tok.kind != TokenKind::CharLiteral) {
      AppendSensitiveInSpan(scalars, i, next.next, sensitive);
    }

    i = next.next;
  }

  result.output.tokens = std::move(tokens);
  result.output.docs = std::move(docs);
  result.sensitive = std::move(sensitive);
  return result;
}

TokenizeResult Tokenize(const core::SourceFile& source) {
  TokenizeResult result;
  LexSmallStepResult lexed = LexSmallStep(source);
  result.diags = lexed.diags;

  if (!lexed.ok) {
    SPEC_RULE("Tokenize-Err");
    result.output.reset();
    return result;
  }

  LexSecureResult secure = LexSecure(source, lexed.output.tokens, lexed.sensitive);
  AppendDiags(result.diags, secure.diags);
  if (!secure.ok) {
    SPEC_RULE("Tokenize-Secure-Err");
    result.output.reset();
    return result;
  }

  SPEC_RULE("Tokenize-Ok");
  result.output = std::move(lexed.output);
  return result;
}

}  // namespace cursive0::syntax
