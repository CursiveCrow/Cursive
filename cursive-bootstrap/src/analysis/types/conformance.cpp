#include "cursive0/analysis/types/conformance.h"

#include <array>
#include <cstddef>
#include <string_view>
#include <vector>

#include "cursive0/core/assert_spec.h"
#include "cursive0/core/diagnostic_messages.h"
#include "cursive0/core/diagnostics.h"
#include "cursive0/core/source_text.h"
#include "cursive0/core/unicode.h"
#include "cursive0/syntax/ast.h"
#include "cursive0/syntax/lexer.h"
#include "cursive0/analysis/types/types.h"

namespace cursive0::analysis {

namespace {

struct PermSyntaxMatch {
  std::size_t index = 0;
};

struct AttrSyntaxMatch {
  std::size_t index = 0;
};

struct UnwindAttrMatch {
  std::size_t index = 0;
};

struct UnsupportedMatch {
  std::size_t index = 0;
};

struct WhereClauseMatch {
  std::size_t index = 0;
};

static inline void SpecDefsSubsetPerms() {
  SPEC_DEF("PermSyntax", "5.2.2");
  SPEC_DEF("PermSet_C0", "1.1.1");
  SPEC_DEF("S_Perms", "1.1.1");
}

static inline void SpecDefsUnsupportedConstructs() {
  SPEC_DEF("UnsupportedConstruct", "1.4");
  SPEC_DEF("UnsupportedForm", "1.4");
  SPEC_DEF("S0Unsupported", "1.1.1");
  SPEC_DEF("UnsupportedGrammarFamily", "3.3.2.7");
  SPEC_DEF("UnsupportedClassItem", "3.3.6.10");
  SPEC_DEF("UnsupportedWhereClause", "3.3.6.11");
  SPEC_DEF("ComptimeForm", "4");
}

static constexpr std::array<std::string_view, 30> kUnsupportedTokens = {
    "derive", "extern", "attribute", "import", "opaque_type",
    "refinement_type", "closure", "pipeline", "async", "parallel",
    "dispatch", "spawn", "metaprogramming", "Network", "Reactor",
    "GPUFactory", "CPUFactory", "AsyncRuntime", "comptime",
    "key_system", "extern_block", "foreign_decl", "class_generics",
    "class_where_clause", "associated_type", "modal_class", "class_contract",
    "type_item", "abstract_state", "override_in_class"};

static bool IsUnsupportedToken(std::string_view token) {
  for (const auto keyword : kUnsupportedTokens) {
    if (token == keyword) {
      return true;
    }
  }
  return false;
}

static bool IsIdentOrKeywordTok(const syntax::Token& tok,
                                std::string_view lexeme) {
  return (tok.kind == syntax::TokenKind::Identifier ||
          tok.kind == syntax::TokenKind::Keyword) &&
         tok.lexeme == lexeme;
}

static bool IsPuncTok(const syntax::Token& tok, std::string_view lexeme) {
  return tok.kind == syntax::TokenKind::Punctuator && tok.lexeme == lexeme;
}

static bool IsOpTok(const syntax::Token& tok, std::string_view lexeme) {
  return tok.kind == syntax::TokenKind::Operator && tok.lexeme == lexeme;
}

static bool IsAdjacent(const syntax::Token& left, const syntax::Token& right) {
  return left.span.file == right.span.file &&
         left.span.end_offset == right.span.start_offset;
}

static std::optional<std::vector<syntax::Token>> TokenizeForSubset(
    const core::SourceFile& source) {
  const auto tok = syntax::Tokenize(source);
  if (!tok.output.has_value()) {
    return std::nullopt;
  }
  return syntax::FilterNewlines(tok.output->tokens);
}

static std::vector<PermSyntaxMatch> FindPermSyntaxMatches(
    const std::vector<syntax::Token>& tokens) {
  std::vector<PermSyntaxMatch> matches;
  for (std::size_t i = 0; i < tokens.size(); ++i) {
    const auto& tok = tokens[i];
    if (IsIdentOrKeywordTok(tok, "shared")) {
      matches.push_back(PermSyntaxMatch{i});
      continue;
    }
    if (IsOpTok(tok, "~%")) {
      matches.push_back(PermSyntaxMatch{i});
      continue;
    }
    if (IsOpTok(tok, "~") && i + 1 < tokens.size() &&
        IsOpTok(tokens[i + 1], "%") && IsAdjacent(tok, tokens[i + 1])) {
      matches.push_back(PermSyntaxMatch{i});
    }
  }
  return matches;
}

static std::vector<AttrSyntaxMatch> FindAttrSyntaxMatches(
    const std::vector<syntax::Token>& tokens) {
  // C0X Extension: Attributes are now supported, so don't flag them as unsupported
  (void)tokens;
  return {};
}

static std::vector<UnwindAttrMatch> FindUnwindAttrMatches(
    const std::vector<syntax::Token>& tokens) {
  std::vector<UnwindAttrMatch> matches;
  if (tokens.size() < 3) {
    return matches;
  }
  for (std::size_t i = 0; i + 2 < tokens.size(); ++i) {
    if (IsPuncTok(tokens[i], "[") && IsPuncTok(tokens[i + 1], "[") &&
        IsIdentOrKeywordTok(tokens[i + 2], "unwind")) {
      matches.push_back(UnwindAttrMatch{i});
    }
  }
  return matches;
}

static std::vector<UnsupportedMatch> FindUnsupportedConstructMatches(
    const std::vector<syntax::Token>& tokens) {
  std::vector<UnsupportedMatch> matches;
  for (std::size_t i = 0; i < tokens.size(); ++i) {
    const auto& tok = tokens[i];
    if ((tok.kind == syntax::TokenKind::Identifier ||
         tok.kind == syntax::TokenKind::Keyword) &&
        IsUnsupportedToken(tok.lexeme)) {
      matches.push_back(UnsupportedMatch{i});
      continue;
    }
    // C0X Extension: modal class is now supported, so don't flag it
    // if (IsIdentOrKeywordTok(tok, "modal") && i + 1 < tokens.size() &&
    //     IsIdentOrKeywordTok(tokens[i + 1], "class")) {
    //   matches.push_back(UnsupportedMatch{i});
    // }
  }
  return matches;
}

static std::vector<WhereClauseMatch> FindWhereClauseMatches(
    const std::vector<syntax::Token>& tokens) {
  // C0X Extension: where clauses are now supported, so don't flag them as unsupported
  (void)tokens;
  return {};
}

static bool SpanContains(const core::Span& outer, const core::Span& inner) {
  return outer.file == inner.file &&
         outer.start_offset <= inner.start_offset &&
         outer.end_offset >= inner.end_offset;
}

static std::vector<core::Span> ItemSpans(const syntax::ASTFile& ast) {
  std::vector<core::Span> spans;
  spans.reserve(ast.items.size());
  for (const auto& item : ast.items) {
    std::visit([&](const auto& node) { spans.push_back(node.span); }, item);
  }
  return spans;
}

static bool SpanContainsAny(const std::vector<core::Span>& spans,
                            const core::Span& target) {
  for (const auto& span : spans) {
    if (SpanContains(span, target)) {
      return true;
    }
  }
  return false;
}

}  // namespace

static inline void SpecDefsConformance() {
  SPEC_DEF("C0Conforming", "1.1");
  SPEC_DEF("WF", "1.1");
  SPEC_DEF("Phase1Order", "1.1");
  SPEC_DEF("Phase3Order", "1.1");
  SPEC_DEF("Phase4Order", "1.1");
  SPEC_DEF("Subset", "1.1");
}

bool WF(const PhaseOrderResult& phases) {
  SpecDefsConformance();
  return phases.phase1_ok && phases.phase3_ok && phases.phase4_ok;
}

bool C0Conforming(const ConformanceInput& input) {
  SpecDefsConformance();
  return WF(input.phase_orders) && input.subset_ok;
}

bool RejectIllFormed(const ConformanceInput& input) {
  SPEC_RULE("Reject-IllFormed");
  return !C0Conforming(input);
}

SubsetResult CheckC0SubsetPermTokens(const std::vector<syntax::Token>& tokens) {
  SpecDefsSubsetPerms();

  // C0X Extension: shared permission is now supported
  // The permission lattice is: unique <: shared <: const
  // We no longer reject shared/~% syntax
  (void)tokens;
  SubsetResult result;
  return result;
}

SubsetResult CheckC0SubsetPermSyntax(const core::SourceFile& source) {
  const auto tokens = TokenizeForSubset(source);
  if (!tokens.has_value()) {
    return SubsetResult{};
  }
  return CheckC0SubsetPermTokens(*tokens);
}

SubsetResult CheckC0UnwindAttrUnsupportedTokens(
    const std::vector<syntax::Token>& tokens) {
  SubsetResult result;
  const auto matches = FindUnwindAttrMatches(tokens);
  if (matches.empty()) {
    return result;
  }

  result.subset_ok = false;
  for (const auto& match : matches) {
    (void)match;
    SPEC_RULE("WF-Unwind-Unsupported");
    auto diag = cursive0::core::MakeDiagnostic("E-UNS-0111");
    if (diag) {
      result.diags = cursive0::core::Emit(result.diags, *diag);
    }
  }

  return result;
}

SubsetResult CheckC0UnwindAttrUnsupported(const core::SourceFile& source) {
  const auto tokens = TokenizeForSubset(source);
  if (!tokens.has_value()) {
    return SubsetResult{};
  }
  return CheckC0UnwindAttrUnsupportedTokens(*tokens);
}

SubsetResult CheckC0AttrSyntaxUnsupportedTokens(
    const std::vector<syntax::Token>& tokens) {
  SubsetResult result;
  const auto matches = FindAttrSyntaxMatches(tokens);
  if (matches.empty()) {
    return result;
  }

  result.subset_ok = false;
  for (const auto& match : matches) {
    (void)match;
    SPEC_RULE("WF-Attr-Unsupported");
    auto diag = cursive0::core::MakeDiagnostic("E-UNS-0113");
    if (diag) {
      result.diags = cursive0::core::Emit(result.diags, *diag);
    }
  }

  return result;
}

SubsetResult CheckC0AttrSyntaxUnsupported(const core::SourceFile& source) {
  const auto tokens = TokenizeForSubset(source);
  if (!tokens.has_value()) {
    return SubsetResult{};
  }
  return CheckC0AttrSyntaxUnsupportedTokens(*tokens);
}

SubsetResult CheckC0UnsupportedConstructsTokens(
    const std::vector<syntax::Token>& tokens) {
  SpecDefsUnsupportedConstructs();

  SubsetResult result;
  const auto matches = FindUnsupportedConstructMatches(tokens);
  if (matches.empty()) {
    return result;
  }

  result.subset_ok = false;
  for (const auto& match : matches) {
    SPEC_RULE("Unsupported-Construct");
    const auto& tok = tokens[match.index];
    if (auto diag = cursive0::core::MakeDiagnostic("E-UNS-0101", tok.span)) {
      result.diags = cursive0::core::Emit(result.diags, *diag);
    }
  }

  return result;
}

SubsetResult CheckC0UnsupportedConstructs(const core::SourceFile& source) {
  const auto tokens = TokenizeForSubset(source);
  if (!tokens.has_value()) {
    return SubsetResult{};
  }
  return CheckC0UnsupportedConstructsTokens(*tokens);
}

SubsetResult CheckC0UnsupportedFormsAst(const syntax::ASTFile& ast,
                                        const core::SourceFile& source) {
  SpecDefsUnsupportedConstructs();
  SubsetResult result;
  const auto tokens = TokenizeForSubset(source);
  if (!tokens.has_value()) {
    return result;
  }
  const auto spans = ItemSpans(ast);
  if (spans.empty()) {
    return result;
  }

  auto emit_unsupported = [&](const syntax::Token& tok) {
    SPEC_RULE("Unsupported-Construct");
    result.subset_ok = false;
    if (auto diag = cursive0::core::MakeDiagnostic("E-UNS-0101", tok.span)) {
      result.diags = cursive0::core::Emit(result.diags, *diag);
    }
  };

  for (const auto& match : FindWhereClauseMatches(*tokens)) {
    const auto& tok = (*tokens)[match.index];
    if (SpanContainsAny(spans, tok.span)) {
      emit_unsupported(tok);
    }
  }

  return result;
}

}  // namespace cursive0::analysis
