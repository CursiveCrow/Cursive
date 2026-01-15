#pragma once

#include <cstddef>
#include <vector>
#include <optional>
#include <string>

#include "cursive0/core/source_text.h"
#include "cursive0/core/diagnostics.h"
#include "cursive0/syntax/token.h"

namespace cursive0::syntax {

struct ScalarRange {
  std::size_t start = 0;
  std::size_t end = 0;
};

enum class DocKind {
  LineDoc,
  ModuleDoc,
};

struct DocComment {
  DocKind kind = DocKind::LineDoc;
  std::string text;
  core::Span span;
};

struct LexerOutput {
  std::vector<Token> tokens;
  std::vector<DocComment> docs;
};

struct CommentScanResult {
  bool ok = true;
  std::size_t next = 0;
  ScalarRange range;
  std::optional<DocComment> doc;
  core::DiagnosticStream diags;
};

struct LiteralScanResult {
  bool ok = true;
  std::size_t next = 0;
  std::optional<ScalarRange> range;
  core::DiagnosticStream diags;
};

struct IdentScanResult {
  bool ok = true;
  std::size_t next = 0;
  TokenKind kind = TokenKind::Identifier;
  Lexeme lexeme;
  core::DiagnosticStream diags;
};

struct NextTokenResult {
  bool ok = false;
  std::size_t next = 0;
  TokenKind kind = TokenKind::Unknown;
  core::DiagnosticStream diags;
};

struct LexSecureResult {
  bool ok = true;
  core::DiagnosticStream diags;
};

struct LexSmallStepResult {
  bool ok = true;
  std::string error_code;
  LexerOutput output;
  std::vector<std::size_t> sensitive;
  core::DiagnosticStream diags;
};

struct TokenizeResult {
  std::optional<LexerOutput> output;
  core::DiagnosticStream diags;
};

bool IsWhitespace(core::UnicodeScalar c);
bool IsLineFeed(core::UnicodeScalar c);

CommentScanResult ScanLineComment(const core::SourceFile& source,
                                 std::size_t start);
CommentScanResult ScanDocComment(const core::SourceFile& source,
                                std::size_t start);
CommentScanResult ScanBlockComment(const core::SourceFile& source,
                                  std::size_t start);

LiteralScanResult ScanIntLiteral(const core::SourceFile& source,
                                 std::size_t start);
LiteralScanResult ScanFloatLiteral(const core::SourceFile& source,
                                   std::size_t start);
LiteralScanResult ScanStringLiteral(const core::SourceFile& source,
                                    std::size_t start);
LiteralScanResult ScanCharLiteral(const core::SourceFile& source,
                                  std::size_t start);

IdentScanResult ScanIdentToken(const core::SourceFile& source,
                                 std::size_t start);

NextTokenResult NextToken(const core::SourceFile& source,
                            std::size_t start);

LexSecureResult LexSecure(const core::SourceFile& source,
                          const std::vector<Token>& tokens,
                          const std::vector<std::size_t>& sensitive);

std::vector<core::Span> UnsafeSpans(const std::vector<Token>& tokens);

LexSmallStepResult LexSmallStep(const core::SourceFile& source);

TokenizeResult Tokenize(const core::SourceFile& source);

// Emits newline tokens for LF scalars that are not covered by suppressed ranges.
std::vector<Token> LexNewlines(const core::SourceFile& source,
                               const std::vector<ScalarRange>& suppressed);

// Removes newline tokens that represent line continuations.
std::vector<Token> FilterNewlines(const std::vector<Token>& tokens);

}  // namespace cursive0::syntax
