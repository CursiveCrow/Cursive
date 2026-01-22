#include "cursive0/syntax/lexer.h"

#include <string_view>

#include "cursive0/core/assert_spec.h"
#include "cursive0/core/diagnostic_messages.h"
#include "cursive0/core/keywords.h"
#include "cursive0/core/source_text.h"
#include "cursive0/core/span.h"
#include "cursive0/core/unicode.h"

namespace cursive0::syntax {

namespace {

bool IsInRange(std::size_t index, const ScalarRange& range) {
  return index >= range.start && index < range.end;
}

bool IsSuppressed(std::size_t index, const std::vector<ScalarRange>& ranges) {
  for (const auto& range : ranges) {
    if (IsInRange(index, range)) {
      return true;
    }
  }
  return false;
}

bool IsPunc(const Token& tok, std::string_view lexeme) {
  return tok.kind == TokenKind::Punctuator && tok.lexeme == lexeme;
}

bool BeginsOperand(const Token& tok) {
  switch (tok.kind) {
    case TokenKind::Identifier:
    case TokenKind::IntLiteral:
    case TokenKind::FloatLiteral:
    case TokenKind::StringLiteral:
    case TokenKind::CharLiteral:
    case TokenKind::BoolLiteral:
    case TokenKind::NullLiteral:
      return true;
    case TokenKind::Punctuator:
      return tok.lexeme == "(" || tok.lexeme == "[" || tok.lexeme == "{";
    case TokenKind::Operator:
      return tok.lexeme == "!" || tok.lexeme == "-" || tok.lexeme == "&" ||
             tok.lexeme == "*" || tok.lexeme == "^";
    case TokenKind::Keyword:
      return tok.lexeme == "if" || tok.lexeme == "match" ||
             tok.lexeme == "loop" || tok.lexeme == "unsafe" ||
             tok.lexeme == "move" || tok.lexeme == "transmute" ||
             tok.lexeme == "widen";
    default:
      return false;
  }
}

bool IsAmbigOp(std::string_view lexeme) {
  return lexeme == "+" || lexeme == "-" || lexeme == "*" || lexeme == "&" ||
         lexeme == "|";
}

bool IsUnaryOnly(std::string_view lexeme) {
  return lexeme == "!" || lexeme == "~" || lexeme == "?";
}

int DeltaDepth(const Token& tok) {
  if (tok.kind != TokenKind::Punctuator) {
    return 0;
  }
  if (tok.lexeme == "(" || tok.lexeme == "[" || tok.lexeme == "{") {
    return 1;
  }
  if (tok.lexeme == ")" || tok.lexeme == "]" || tok.lexeme == "}") {
    return -1;
  }
  return 0;
}

struct Candidate {
  TokenKind kind = TokenKind::Unknown;
  std::size_t next = 0;
  core::DiagnosticStream diags;
};

int KindPriority(TokenKind kind) {
  switch (kind) {
    case TokenKind::IntLiteral:
    case TokenKind::FloatLiteral:
    case TokenKind::StringLiteral:
    case TokenKind::CharLiteral:
    case TokenKind::BoolLiteral:
    case TokenKind::NullLiteral:
      return 3;
    case TokenKind::Identifier:
    case TokenKind::Keyword:
      return 2;
    case TokenKind::Operator:
      return 1;
    case TokenKind::Punctuator:
      return 0;
    default:
      return -1;
  }
}

bool IsDecDigit(core::UnicodeScalar c) {
  return c >= '0' && c <= '9';
}

bool IsQuote(core::UnicodeScalar c) {
  return c == '"' || c == '\'';
}

bool HasFloatMarker(const std::vector<core::UnicodeScalar>& scalars,
                    std::size_t start,
                    std::size_t end) {
  for (std::size_t i = start; i < end; ++i) {
    const core::UnicodeScalar c = scalars[i];
    if (c == '.' || c == 'e' || c == 'E') {
      return true;
    }
  }
  if (end >= start + 3) {
    const core::UnicodeScalar a = scalars[end - 3];
    const core::UnicodeScalar b = scalars[end - 2];
    const core::UnicodeScalar c = scalars[end - 1];
    if (a == 'f') {
      if ((b == '1' && c == '6') || (b == '3' && c == '2') ||
          (b == '6' && c == '4')) {
        return true;
      }
    }
  }
  return false;
}


bool MatchLexeme(const std::vector<core::UnicodeScalar>& scalars,
                 std::size_t start,
                 std::string_view lexeme) {
  const std::size_t n = scalars.size();
  if (start + lexeme.size() > n) {
    return false;
  }
  for (std::size_t i = 0; i < lexeme.size(); ++i) {
    if (scalars[start + i] != static_cast<unsigned char>(lexeme[i])) {
      return false;
    }
  }
  return true;
}

void AppendOpCandidates(const std::vector<core::UnicodeScalar>& scalars,
                        std::size_t start,
                        std::vector<Candidate>& out) {
  for (std::string_view op : core::kCursive0Operators) {
    if (MatchLexeme(scalars, start, op)) {
      Candidate cand;
      cand.kind = TokenKind::Operator;
      cand.next = start + op.size();
      out.push_back(cand);
    }
  }
}

void AppendPuncCandidates(const std::vector<core::UnicodeScalar>& scalars,
                          std::size_t start,
                          std::vector<Candidate>& out) {
  for (std::string_view punc : core::kCursive0Punctuators) {
    if (MatchLexeme(scalars, start, punc)) {
      Candidate cand;
      cand.kind = TokenKind::Punctuator;
      cand.next = start + punc.size();
      out.push_back(cand);
    }
  }
}

}  // namespace

NextTokenResult NextToken(const core::SourceFile& source,
                          std::size_t start) {
  SPEC_RULE("Max-Munch");
  SPEC_RULE("Max-Munch-Err");
  NextTokenResult result;
  result.ok = false;
  result.next = start;
  result.kind = TokenKind::Unknown;

  const auto& scalars = source.scalars;
  if (start >= scalars.size()) {
    return result;
  }

  std::vector<Candidate> candidates;
  const core::UnicodeScalar first = scalars[start];

  if (IsQuote(first)) {
    LiteralScanResult str = ScanStringLiteral(source, start);
    if (str.ok) {
      Candidate cand;
      cand.kind = TokenKind::StringLiteral;
      cand.next = str.next;
      cand.diags = str.diags;
      candidates.push_back(cand);
    }
    LiteralScanResult chr = ScanCharLiteral(source, start);
    if (chr.ok) {
      Candidate cand;
      cand.kind = TokenKind::CharLiteral;
      cand.next = chr.next;
      cand.diags = chr.diags;
      candidates.push_back(cand);
    }
  } else if (IsDecDigit(first)) {
    LiteralScanResult flt = ScanFloatLiteral(source, start);
    if (flt.ok || (flt.next > start &&
                   HasFloatMarker(scalars, start, flt.next))) {
      Candidate cand;
      cand.kind = TokenKind::FloatLiteral;
      cand.next = flt.next;
      cand.diags = flt.diags;
      candidates.push_back(cand);
    }
    LiteralScanResult integer = ScanIntLiteral(source, start);
    if (integer.ok || integer.next > start) {
      Candidate cand;
      cand.kind = TokenKind::IntLiteral;
      cand.next = integer.next;
      cand.diags = integer.diags;
      candidates.push_back(cand);
    }
  } else if (core::IsIdentStart(first)) {
    IdentScanResult ident = ScanIdentToken(source, start);
    if (ident.ok) {
      Candidate cand;
      cand.kind = ident.kind;
      cand.next = ident.next;
      cand.diags = ident.diags;
      candidates.push_back(cand);
    }
  } else {
    AppendOpCandidates(scalars, start, candidates);
    AppendPuncCandidates(scalars, start, candidates);
  }

  if (candidates.empty()) {
    const auto offsets = core::Utf8Offsets(scalars);
    if (start + 1 < offsets.size()) {
      const std::size_t start_offset = offsets[start];
      const std::size_t end_offset = offsets[start + 1];
      const auto span = core::SpanOf(source, start_offset, end_offset);
      if (auto diag = core::MakeDiagnostic("E-SRC-0309", span)) {
        result.diags = core::Emit(result.diags, *diag);
      }
    }
    return result;
  }

  const Candidate* best = &candidates[0];
  for (const Candidate& cand : candidates) {
    if (cand.next > best->next) {
      best = &cand;
      continue;
    }
    if (cand.next == best->next &&
        KindPriority(cand.kind) > KindPriority(best->kind)) {
      best = &cand;
    }
  }

  result.ok = true;
  result.kind = best->kind;
  result.next = best->next;
  result.diags = best->diags;
  return result;
}

std::vector<Token> LexNewlines(const core::SourceFile& source,
                               const std::vector<ScalarRange>& suppressed) {
  SPEC_RULE("Lex-Newline");
  const auto& scalars = source.scalars;
  std::vector<Token> out;
  if (scalars.empty()) {
    return out;
  }

  const auto offsets = core::Utf8Offsets(scalars);
  for (std::size_t i = 0; i < scalars.size(); ++i) {
    if (scalars[i] != core::kLF) {
      continue;
    }
    if (IsSuppressed(i, suppressed)) {
      continue;
    }
    const std::size_t start = offsets[i];
    const std::size_t end = offsets[i + 1];
    Token tok;
    tok.kind = TokenKind::Newline;
    tok.lexeme = "\n";
    tok.span = core::SpanOf(source, start, end);
    out.push_back(tok);
  }
  return out;
}

std::vector<Token> FilterNewlines(const std::vector<Token>& tokens) {
  const std::size_t n = tokens.size();
  
  // Track depths separately: parentheses/brackets are expression contexts (filter newlines),
  // braces are block contexts (keep newlines as statement terminators)
  std::vector<int> paren_depth(n + 1, 0);   // ( )
  std::vector<int> bracket_depth(n + 1, 0); // [ ]
  for (std::size_t i = 0; i < n; ++i) {
    paren_depth[i + 1] = paren_depth[i];
    bracket_depth[i + 1] = bracket_depth[i];
    if (tokens[i].kind == TokenKind::Punctuator) {
      if (tokens[i].lexeme == "(") paren_depth[i + 1]++;
      else if (tokens[i].lexeme == ")") paren_depth[i + 1]--;
      else if (tokens[i].lexeme == "[") bracket_depth[i + 1]++;
      else if (tokens[i].lexeme == "]") bracket_depth[i + 1]--;
    }
  }

  std::vector<std::size_t> prev_index(n, static_cast<std::size_t>(-1));
  for (std::size_t i = 1; i < n; ++i) {
    if (tokens[i - 1].kind != TokenKind::Newline) {
      prev_index[i] = i - 1;
    }
  }

  std::vector<std::size_t> next_index(n, static_cast<std::size_t>(-1));
  std::size_t next_non_newline = static_cast<std::size_t>(-1);
  for (std::size_t i = n; i-- > 0;) {
    next_index[i] = next_non_newline;
    if (tokens[i].kind != TokenKind::Newline) {
      next_non_newline = i;
    }
  }

  std::vector<Token> out;
  out.reserve(tokens.size());

  for (std::size_t i = 0; i < n; ++i) {
    const Token& tok = tokens[i];
    if (tok.kind != TokenKind::Newline) {
      out.push_back(tok);
      continue;
    }

    bool cont = false;
    
    // Inside parentheses or brackets: filter newlines (expression context)
    // Inside braces: keep newlines (statement context)
    if (paren_depth[i] > 0 || bracket_depth[i] > 0) {
      cont = true;
    }

    // After comma: continuation (allows multi-line argument lists, etc.)
    if (!cont && prev_index[i] != static_cast<std::size_t>(-1)) {
      const Token& prev = tokens[prev_index[i]];
      if (IsPunc(prev, ",")) {
        cont = true;
      } else if (prev.kind == TokenKind::Operator) {
        if (IsAmbigOp(prev.lexeme)) {
          if (next_index[i] != static_cast<std::size_t>(-1)) {
            const Token& next = tokens[next_index[i]];
            if (BeginsOperand(next)) {
              cont = true;
            }
          }
        } else if (!IsUnaryOnly(prev.lexeme)) {
          cont = true;
        }
      }
    }

    // Before `.`, `::`, `~>`: continuation (method chaining, qualified names)
    if (!cont && next_index[i] != static_cast<std::size_t>(-1)) {
      const Token& next = tokens[next_index[i]];
      if (next.lexeme == "." || next.lexeme == "::" || next.lexeme == "~>") {
        cont = true;
      }
    }

    if (!cont) {
      out.push_back(tok);
    }
  }

  return out;
}

}  // namespace cursive0::syntax
