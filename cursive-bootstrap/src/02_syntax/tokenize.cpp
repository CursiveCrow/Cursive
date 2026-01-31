#include "cursive0/02_syntax/lexer.h"

#include <cctype>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include "cursive0/00_core/assert_spec.h"
#include "cursive0/00_core/diagnostic_messages.h"
#include "cursive0/00_core/source_text.h"
#include "cursive0/00_core/span.h"
#include "cursive0/00_core/unicode.h"

namespace cursive0::syntax {

namespace {

void DebugLexFail(const core::SourceFile& source,
                  const std::vector<core::UnicodeScalar>& scalars,
                  const std::vector<std::size_t>& offsets,
                  std::size_t index) {
  const char* flag = std::getenv("CURSIVE0_DEBUG_LEX");
  if (!flag || !*flag) {
    return;
  }

  const std::size_t n = scalars.size();
  const std::size_t byte_index = index < offsets.size() ? offsets[index] : 0;
  const core::UnicodeScalar cp = index < n ? scalars[index] : 0;

  std::cerr << "[cursivec0] lex: Max-Munch-Err at scalar=" << index
            << " byte=" << byte_index
            << " codepoint=U+"
            << std::hex << std::uppercase << std::setw(4) << std::setfill('0')
            << static_cast<std::uint32_t>(cp)
            << std::dec << "\n";

  const std::size_t lo = index > 16 ? index - 16 : 0;
  const std::size_t hi = std::min(n, index + 17);

  std::string context;
  context.reserve((hi - lo) + 8);
  for (std::size_t i = lo; i < hi; ++i) {
    const core::UnicodeScalar c = scalars[i];
    if (c == '\n') {
      context += "\\n";
    } else if (c >= 0x20 && c <= 0x7E) {
      context.push_back(static_cast<char>(c));
    } else {
      context.push_back('.');
    }
  }

  std::cerr << "[cursivec0] lex: context=\"" << context << "\"\n";
  std::cerr << "[cursivec0] lex: window=[";
  for (std::size_t i = lo; i < hi; ++i) {
    if (i > lo) {
      std::cerr << " ";
    }
    std::cerr << "U+"
              << std::hex << std::uppercase << std::setw(4) << std::setfill('0')
              << static_cast<std::uint32_t>(scalars[i])
              << std::dec;
    if (i == index) {
      std::cerr << "*";
    }
  }
  std::cerr << "]\n";
}

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
      DebugLexFail(source, scalars, offsets, i);
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
