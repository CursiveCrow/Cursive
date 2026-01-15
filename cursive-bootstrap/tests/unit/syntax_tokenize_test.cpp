#include <cassert>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include "cursive0/core/assert_spec.h"
#include "cursive0/core/diagnostics.h"
#include "cursive0/core/source_text.h"
#include "cursive0/core/span.h"
#include "cursive0/syntax/lexer.h"
#include "cursive0/syntax/token.h"

namespace {

using cursive0::core::DiagnosticStream;
using cursive0::core::LineStarts;
using cursive0::core::SourceFile;
using cursive0::core::Span;
using cursive0::core::SpanOf;
using cursive0::core::UnicodeScalar;
using cursive0::syntax::DocKind;
using cursive0::syntax::LexSmallStepResult;
using cursive0::syntax::LexSmallStep;
using cursive0::syntax::Token;
using cursive0::syntax::Tokenize;
using cursive0::syntax::TokenizeResult;
using cursive0::syntax::TokenKind;

static std::vector<UnicodeScalar> AsScalars(const std::string& text) {
  std::vector<UnicodeScalar> out;
  out.reserve(text.size());
  for (unsigned char c : text) {
    out.push_back(static_cast<UnicodeScalar>(c));
  }
  return out;
}

static SourceFile MakeSourceFile(const std::string& path,
                                 const std::vector<UnicodeScalar>& scalars) {
  SourceFile s;
  s.path = path;
  s.scalars = scalars;
  s.text = cursive0::core::EncodeUtf8(scalars);
  s.bytes.assign(s.text.begin(), s.text.end());
  s.byte_len = s.text.size();
  s.line_starts = LineStarts(scalars);
  s.line_count = s.line_starts.size();
  return s;
}

static void ExpectSpanEq(const Span& a, const Span& b) {
  assert(a.file == b.file);
  assert(a.start_offset == b.start_offset);
  assert(a.end_offset == b.end_offset);
  assert(a.start_line == b.start_line);
  assert(a.start_col == b.start_col);
  assert(a.end_line == b.end_line);
  assert(a.end_col == b.end_col);
}

static bool HasDiag(const DiagnosticStream& diags, std::string_view code) {
  for (const auto& diag : diags) {
    if (diag.code == code) {
      return true;
    }
  }
  return false;
}

static std::size_t CountNewlines(const std::vector<Token>& tokens) {
  std::size_t count = 0;
  for (const auto& tok : tokens) {
    if (tok.kind == TokenKind::Newline) {
      ++count;
    }
  }
  return count;
}

}  // namespace

int main() {
  SPEC_COV("Lex-Start");
  SPEC_COV("Lex-End");
  SPEC_COV("Lex-Whitespace");
  SPEC_COV("Lex-Newline");
  SPEC_COV("Lex-Line-Comment");
  SPEC_COV("Lex-Doc-Comment");
  SPEC_COV("Lex-Block-Comment");
  SPEC_COV("Lex-String-Unterminated-Recover");
  SPEC_COV("Lex-Char-Unterminated-Recover");
  SPEC_COV("Lex-Sensitive");
  SPEC_COV("Lex-Token");
  SPEC_COV("Lex-Token-Err");
  SPEC_COV("Tokenize-Ok");
  SPEC_COV("Tokenize-Secure-Err");
  SPEC_COV("Tokenize-Err");

  {
    const auto scalars = AsScalars("/// doc\nlet x = 1 /* block */\n// line\nx");
    const auto source = MakeSourceFile("tokenize_basic.cursive", scalars);
    LexSmallStepResult lexed = LexSmallStep(source);
    assert(lexed.ok);
    assert(lexed.output.docs.size() == 1);
    assert(lexed.output.docs[0].kind == DocKind::LineDoc);
    assert(lexed.output.docs[0].text == "doc");
    assert(CountNewlines(lexed.output.tokens) == 3);
  }

  {
    const auto scalars = AsScalars("\"oops\nx");
    const auto source = MakeSourceFile("unterminated_string.cursive", scalars);
    LexSmallStepResult lexed = LexSmallStep(source);
    assert(lexed.ok);
    assert(HasDiag(lexed.diags, "E-SRC-0301"));
    assert(!HasDiag(lexed.diags, "E-SRC-0309"));
    assert(CountNewlines(lexed.output.tokens) == 1);
    assert(!lexed.output.tokens.empty());
    assert(lexed.output.tokens.back().lexeme == "x");
  }

  {
    const auto scalars = AsScalars("'a\nb");
    const auto source = MakeSourceFile("unterminated_char.cursive", scalars);
    LexSmallStepResult lexed = LexSmallStep(source);
    assert(lexed.ok);
    assert(HasDiag(lexed.diags, "E-SRC-0303"));
    assert(!HasDiag(lexed.diags, "E-SRC-0309"));
  }

  {
    const auto scalars = AsScalars("`");
    const auto source = MakeSourceFile("lex_token_err.cursive", scalars);
    LexSmallStepResult lexed = LexSmallStep(source);
    assert(!lexed.ok);
    assert(HasDiag(lexed.diags, "E-SRC-0309"));
    TokenizeResult tokenized = Tokenize(source);
    assert(!tokenized.output.has_value());
    assert(HasDiag(tokenized.diags, "E-SRC-0309"));
  }

  {
    const auto scalars = AsScalars("/* unterminated");
    const auto source = MakeSourceFile("block_unterminated.cursive", scalars);
    TokenizeResult tokenized = Tokenize(source);
    assert(!tokenized.output.has_value());
    assert(HasDiag(tokenized.diags, "E-SRC-0306"));
  }

  {
    std::vector<UnicodeScalar> scalars;
    scalars.push_back('a');
    scalars.push_back(0x200C);
    scalars.push_back('b');
    const auto source = MakeSourceFile("sensitive.cursive", scalars);
    TokenizeResult tokenized = Tokenize(source);
    assert(!tokenized.output.has_value());
    assert(HasDiag(tokenized.diags, "E-SRC-0308"));
  }

  {
    const auto scalars = AsScalars("let x=1");
    const auto source = MakeSourceFile("tokenize_ok.cursive", scalars);
    LexSmallStepResult lexed = LexSmallStep(source);
    TokenizeResult tokenized = Tokenize(source);
    assert(lexed.ok);
    assert(tokenized.output.has_value());
    assert(tokenized.output->docs.size() == lexed.output.docs.size());
    assert(tokenized.output->tokens.size() == lexed.output.tokens.size());
    for (std::size_t i = 0; i < lexed.output.tokens.size(); ++i) {
      const Token& a = tokenized.output->tokens[i];
      const Token& b = lexed.output.tokens[i];
      assert(a.kind == b.kind);
      assert(a.lexeme == b.lexeme);
      ExpectSpanEq(a.span, b.span);
    }
    const Span expect = SpanOf(source, 0, 3);
    ExpectSpanEq(tokenized.output->tokens[0].span, expect);
  }

  {
    const auto scalars = AsScalars("1_");
    const auto source = MakeSourceFile("tokenize_diag_match.cursive", scalars);
    LexSmallStepResult lexed = LexSmallStep(source);
    TokenizeResult tokenized = Tokenize(source);
    assert(lexed.ok);
    assert(tokenized.output.has_value());
    assert(lexed.diags.size() == tokenized.diags.size());
    for (std::size_t i = 0; i < lexed.diags.size(); ++i) {
      assert(lexed.diags[i].code == tokenized.diags[i].code);
      if (lexed.diags[i].span.has_value()) {
        assert(tokenized.diags[i].span.has_value());
        ExpectSpanEq(*lexed.diags[i].span, *tokenized.diags[i].span);
      } else {
        assert(!tokenized.diags[i].span.has_value());
      }
    }
  }

  return 0;
}
