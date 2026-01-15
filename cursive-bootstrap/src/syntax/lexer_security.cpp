#include "cursive0/syntax/lexer.h"

#include <cstddef>
#include <optional>
#include <vector>

#include "cursive0/core/assert_spec.h"
#include "cursive0/core/diagnostic_messages.h"
#include "cursive0/core/source_text.h"
#include "cursive0/core/span.h"

namespace cursive0::syntax {

namespace {

core::Span SpanOfText(const core::SourceFile& source,
                      const std::vector<std::size_t>& offsets,
                      std::size_t i,
                      std::size_t j) {
  const std::size_t start = offsets[i];
  const std::size_t end = offsets[j];
  return core::SpanOf(source, start, end);
}

bool IsLBrace(const Token& tok) {
  return tok.kind == TokenKind::Punctuator && tok.lexeme == "{";
}

bool IsRBrace(const Token& tok) {
  return tok.kind == TokenKind::Punctuator && tok.lexeme == "}";
}

std::optional<std::size_t> NextNonNewline(const std::vector<Token>& tokens,
                                          std::size_t index) {
  for (std::size_t i = index; i < tokens.size(); ++i) {
    if (tokens[i].kind != TokenKind::Newline) {
      return i;
    }
  }
  return std::nullopt;
}

std::optional<std::size_t> MatchBrace(const std::vector<Token>& tokens,
                                      std::size_t open_index) {
  int depth = 0;
  for (std::size_t i = open_index; i < tokens.size(); ++i) {
    if (IsLBrace(tokens[i])) {
      ++depth;
    } else if (IsRBrace(tokens[i])) {
      --depth;
      if (depth == 0 && i > open_index) {
        return i;
      }
    }
  }
  return std::nullopt;
}

core::Span SpanFrom(const Token& start, const Token& end) {
  core::Span span = start.span;
  span.end_offset = end.span.end_offset;
  span.end_line = end.span.end_line;
  span.end_col = end.span.end_col;
  return span;
}

std::vector<core::Span> ComputeUnsafeSpans(const std::vector<Token>& tokens) {
  std::vector<core::Span> spans;
  for (std::size_t i = 0; i < tokens.size(); ++i) {
    const Token& tok = tokens[i];
    if (tok.kind != TokenKind::Keyword || tok.lexeme != "unsafe") {
      continue;
    }
    const auto next = NextNonNewline(tokens, i + 1);
    if (!next.has_value() || !IsLBrace(tokens[*next])) {
      continue;
    }
    const auto match = MatchBrace(tokens, *next);
    if (!match.has_value()) {
      continue;
    }
    spans.push_back(SpanFrom(tokens[*next], tokens[*match]));
  }
  return spans;
}

bool UnsafeAtByte(std::size_t offset,
                  const std::vector<core::Span>& spans) {
  for (const auto& sp : spans) {
    const auto range = core::SpanRange(sp);
    if (offset >= range.first && offset < range.second) {
      return true;
    }
  }
  return false;
}

}  // namespace

std::vector<core::Span> UnsafeSpans(const std::vector<Token>& tokens) {
  return ComputeUnsafeSpans(tokens);
}

LexSecureResult LexSecure(const core::SourceFile& source,
                          const std::vector<Token>& tokens,
                          const std::vector<std::size_t>& sensitive) {
  SPEC_RULE("LexSecure-Err");
  SPEC_RULE("LexSecure-Warn");

  LexSecureResult result;
  if (sensitive.empty()) {
    return result;
  }

  const auto offsets = core::Utf8Offsets(source.scalars);
  const std::vector<core::Span> unsafe_spans = UnsafeSpans(tokens);
  for (std::size_t p : sensitive) {
    if (!UnsafeAtByte(offsets[p], unsafe_spans)) {
      const core::Span span = SpanOfText(source, offsets, p, p + 1);
      const auto diag = core::MakeDiagnostic("E-SRC-0308", span);
      if (diag.has_value()) {
        result.diags = core::Emit(result.diags, *diag);
      }
      result.ok = false;
      return result;
    }
  }

  for (std::size_t p : sensitive) {
    const core::Span span = SpanOfText(source, offsets, p, p + 1);
    const auto diag = core::MakeDiagnostic("W-SRC-0308", span);
    if (diag.has_value()) {
      result.diags = core::Emit(result.diags, *diag);
    }
  }

  return result;
}


}  // namespace cursive0::syntax
