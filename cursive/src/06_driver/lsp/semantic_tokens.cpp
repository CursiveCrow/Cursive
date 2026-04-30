#include "06_driver/lsp/semantic_tokens.h"

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

#include "00_core/source_load.h"
#include "02_source/lexer.h"
#include "06_driver/lsp/protocol.h"
#include "06_driver/tooling/line_index.h"
#include "06_driver/tooling/uri.h"

namespace cursive::driver::lsp {

namespace {

struct SemanticToken {
  std::size_t line = 0;
  std::size_t start = 0;
  std::size_t length = 0;
  int token_type = 0;
  int modifiers = 0;
};

std::string TypeForIdentifier(const std::filesystem::path& path,
                              const lexer::Token& token,
                              const tooling::AnalysisSnapshot& snapshot) {
  const auto* symbol =
      snapshot.language_service.ResolvedSymbolAt(path, token.span.start_offset);
  if (symbol == nullptr) {
    return "variable";
  }
  switch (symbol->kind) {
    case analysis::LanguageSymbolKind::Record:
    case analysis::LanguageSymbolKind::Modal:
    case analysis::LanguageSymbolKind::TypeAlias:
      return "type";
    case analysis::LanguageSymbolKind::Class:
      return "class";
    case analysis::LanguageSymbolKind::Enum:
      return "enum";
    case analysis::LanguageSymbolKind::EnumMember:
      return "enumMember";
    case analysis::LanguageSymbolKind::Function:
      return "function";
    case analysis::LanguageSymbolKind::Method:
      return "method";
    case analysis::LanguageSymbolKind::Field:
    case analysis::LanguageSymbolKind::State:
      return "property";
    case analysis::LanguageSymbolKind::Parameter:
      return "parameter";
    default:
      return "variable";
  }
}

std::string TypeForToken(const std::filesystem::path& path,
                         const lexer::Token& token,
                         const tooling::AnalysisSnapshot& snapshot) {
  switch (token.kind) {
    case lexer::TokenKind::Keyword:
      return "keyword";
    case lexer::TokenKind::Identifier:
      return TypeForIdentifier(path, token, snapshot);
    case lexer::TokenKind::IntLiteral:
    case lexer::TokenKind::FloatLiteral:
      return "number";
    case lexer::TokenKind::StringLiteral:
    case lexer::TokenKind::CharLiteral:
      return "string";
    case lexer::TokenKind::Operator:
    case lexer::TokenKind::Punctuator:
      return "operator";
    default:
      return {};
  }
}

void AddToken(std::vector<SemanticToken>& out,
              const tooling::LineIndex& index,
              const core::Span& span,
              int type_index) {
  const tooling::LineRange range = index.RangeFor(span);
  if (range.start.line != range.end.line ||
      range.end.character <= range.start.character) {
    return;
  }
  out.push_back(SemanticToken{
      range.start.line,
      range.start.character,
      range.end.character - range.start.character,
      type_index,
      0,
  });
}

}  // namespace

llvm::json::Object SemanticTokensFull(
    const std::filesystem::path& path,
    const std::string& text_utf8,
    const tooling::AnalysisSnapshot& snapshot) {
  llvm::json::Array data;
  std::vector<std::uint8_t> bytes(text_utf8.begin(), text_utf8.end());
  core::SourceLoadResult loaded = core::LoadSource(path.generic_string(), bytes);
  if (!loaded.source.has_value()) {
    llvm::json::Object empty;
    empty["data"] = std::move(data);
    return empty;
  }

  tooling::LineIndex index(loaded.source->text);
  lexer::TokenizeDiagnosticResult tokenized =
      lexer::TokenizeWithDiagnostics(*loaded.source);

  std::vector<SemanticToken> tokens;
  if (tokenized.output.has_value()) {
    for (const auto& token : tokenized.output->tokens) {
      const std::string type = TypeForToken(path, token, snapshot);
      if (type.empty()) {
        continue;
      }
      AddToken(tokens, index, token.span, SemanticTokenTypeIndex(type));
    }
    for (const auto& doc : tokenized.output->docs) {
      AddToken(tokens, index, doc.span, SemanticTokenTypeIndex("comment"));
    }
  }

  std::sort(tokens.begin(), tokens.end(), [](const SemanticToken& lhs,
                                             const SemanticToken& rhs) {
    if (lhs.line != rhs.line) {
      return lhs.line < rhs.line;
    }
    return lhs.start < rhs.start;
  });

  std::size_t prev_line = 0;
  std::size_t prev_start = 0;
  bool first = true;
  for (const auto& token : tokens) {
    const std::size_t delta_line = first ? token.line : token.line - prev_line;
    const std::size_t delta_start =
        (first || delta_line != 0) ? token.start : token.start - prev_start;
    data.emplace_back(static_cast<std::int64_t>(delta_line));
    data.emplace_back(static_cast<std::int64_t>(delta_start));
    data.emplace_back(static_cast<std::int64_t>(token.length));
    data.emplace_back(static_cast<std::int64_t>(token.token_type));
    data.emplace_back(static_cast<std::int64_t>(token.modifiers));
    prev_line = token.line;
    prev_start = token.start;
    first = false;
  }

  llvm::json::Object result;
  result["data"] = std::move(data);
  return result;
}

}  // namespace cursive::driver::lsp
