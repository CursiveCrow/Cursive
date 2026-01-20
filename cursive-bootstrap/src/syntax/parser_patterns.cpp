#include "cursive0/syntax/parser.h"

#include <optional>
#include <string_view>
#include <utility>
#include <vector>

#include "cursive0/core/diagnostic_messages.h"
#include "cursive0/core/diagnostics.h"
#include "cursive0/syntax/keyword_policy.h"

namespace cursive0::syntax {

namespace {
void EmitUnsupportedConstruct(Parser& parser) {
  auto diag = core::MakeDiagnostic("E-UNS-0101", TokSpan(parser));
  if (!diag) {
    return;
  }
  parser.diags = core::Emit(parser.diags, *diag);
}

bool IsOp(const Parser& parser, std::string_view op) {
  const Token* tok = Tok(parser);
  return tok && IsOpTok(*tok, op);
}

bool IsPunc(const Parser& parser, std::string_view punc) {
  const Token* tok = Tok(parser);
  return tok && IsPuncTok(*tok, punc);
}

bool IsLiteralToken(const Token& tok) {
  return tok.kind == TokenKind::IntLiteral ||
         tok.kind == TokenKind::FloatLiteral ||
         tok.kind == TokenKind::StringLiteral ||
         tok.kind == TokenKind::CharLiteral ||
         tok.kind == TokenKind::BoolLiteral ||
         tok.kind == TokenKind::NullLiteral;
}

bool IsPatternStart(const Token& tok) {
  if (IsLiteralToken(tok) || IsIdentTok(tok)) {
    return true;
  }
  if (tok.kind == TokenKind::Punctuator) {
    return tok.lexeme == "(";
  }
  return tok.kind == TokenKind::Operator && tok.lexeme == "@";
}

PatternPtr MakePattern(const core::Span& span, PatternNode node) {
  auto pat = std::make_shared<Pattern>();
  pat->span = span;
  pat->node = std::move(node);
  return pat;
}

struct EnumPayloadOptResult {
  Parser parser;
  std::optional<EnumPayloadPattern> payload_opt;
};

struct ModalPayloadOptResult {
  Parser parser;
  std::optional<ModalRecordPayload> fields_opt;
};

ParseElemResult<PatternPtr> ParsePatternRange(Parser parser);
ParseElemResult<PatternPtr> ParsePatternAtom(Parser parser);

ParseElemResult<std::vector<PatternPtr>> ParsePatternListTail(
    Parser parser,
    std::vector<PatternPtr> xs);

[[maybe_unused]] ParseElemResult<std::vector<PatternPtr>> ParsePatternList(Parser parser) {
  if (IsPunc(parser, ")")) {
    SPEC_RULE("Parse-PatternList-Empty");
    return {parser, {}};
  }
  SPEC_RULE("Parse-PatternList-Cons");
  ParseElemResult<PatternPtr> first = ParsePattern(parser);
  std::vector<PatternPtr> elems;
  elems.push_back(first.elem);
  return ParsePatternListTail(first.parser, std::move(elems));
}

ParseElemResult<std::vector<PatternPtr>> ParsePatternListTail(
    Parser parser,
    std::vector<PatternPtr> xs) {
  if (IsPunc(parser, ")")) {
    SPEC_RULE("Parse-PatternListTail-End");
    return {parser, xs};
  }
  if (IsPunc(parser, ",")) {
    Parser after = parser;
    Advance(after);
    if (IsPunc(after, ")")) {
      SPEC_RULE("Parse-PatternListTail-TrailingComma");
      EmitUnsupportedConstruct(after);
      return {after, xs};
    }
    SPEC_RULE("Parse-PatternListTail-Comma");
    ParseElemResult<PatternPtr> elem = ParsePattern(after);
    xs.push_back(elem.elem);
    return ParsePatternListTail(elem.parser, std::move(xs));
  }
  EmitParseSyntaxErr(parser, TokSpan(parser));
  return {parser, xs};
}

ParseElemResult<std::vector<PatternPtr>> ParseTuplePatternElems(Parser parser) {
  if (IsPunc(parser, ")")) {
    SPEC_RULE("Parse-TuplePatternElems-Empty");
    return {parser, {}};
  }
  ParseElemResult<PatternPtr> first = ParsePattern(parser);
  if (IsPunc(first.parser, ";")) {
    SPEC_RULE("Parse-TuplePatternElems-Single");
    Parser after = first.parser;
    Advance(after);
    return {after, {first.elem}};
  }
  if (IsPunc(first.parser, ",")) {
    Parser after = first.parser;
    Advance(after);
    if (IsPunc(after, ")")) {
      SPEC_RULE("Parse-TuplePatternElems-TrailingComma");
      EmitUnsupportedConstruct(after);
      return {after, {first.elem}};
    }
    SPEC_RULE("Parse-TuplePatternElems-Many");
    ParseElemResult<PatternPtr> second = ParsePattern(after);
    ParseElemResult<std::vector<PatternPtr>> tail =
        ParsePatternListTail(second.parser, {second.elem});
    std::vector<PatternPtr> elems;
    elems.reserve(1 + tail.elem.size());
    elems.push_back(first.elem);
    elems.insert(elems.end(), tail.elem.begin(), tail.elem.end());
    return {tail.parser, elems};
  }
  EmitParseSyntaxErr(first.parser, TokSpan(first.parser));
  return {first.parser, {first.elem}};
}

ParseElemResult<std::vector<FieldPattern>> ParseFieldPatternTail(
    Parser parser,
    std::vector<FieldPattern> xs);

struct FieldPatternTailOptResult {
  Parser parser;
  PatternPtr pattern_opt;
};

FieldPatternTailOptResult ParseFieldPatternTailOpt(Parser parser) {
  if (!IsPunc(parser, ":")) {
    SPEC_RULE("Parse-FieldPatternTailOpt-None");
    return {parser, nullptr};
  }
  SPEC_RULE("Parse-FieldPatternTailOpt-Yes");
  Parser after = parser;
  Advance(after);
  ParseElemResult<PatternPtr> pat = ParsePattern(after);
  return {pat.parser, pat.elem};
}

ParseElemResult<FieldPattern> ParseFieldPattern(Parser parser) {
  SPEC_RULE("Parse-FieldPattern");
  Parser start = parser;
  ParseElemResult<Identifier> name = ParseIdent(parser);
  FieldPatternTailOptResult tail = ParseFieldPatternTailOpt(name.parser);
  FieldPattern field;
  field.name = name.elem;
  field.pattern_opt = tail.pattern_opt;
  field.span = SpanBetween(start, tail.parser);
  return {tail.parser, field};
}

ParseElemResult<std::vector<FieldPattern>> ParseFieldPatternList(Parser parser) {
  if (IsPunc(parser, "}")) {
    SPEC_RULE("Parse-FieldPatternList-Empty");
    return {parser, {}};
  }
  SPEC_RULE("Parse-FieldPatternList-Cons");
  ParseElemResult<FieldPattern> first = ParseFieldPattern(parser);
  std::vector<FieldPattern> fields;
  fields.push_back(first.elem);
  return ParseFieldPatternTail(first.parser, std::move(fields));
}

ParseElemResult<std::vector<FieldPattern>> ParseFieldPatternTail(
    Parser parser,
    std::vector<FieldPattern> xs) {
  if (IsPunc(parser, "}")) {
    SPEC_RULE("Parse-FieldPatternTail-End");
    return {parser, xs};
  }
  if (IsPunc(parser, ",")) {
    Parser after = parser;
    Advance(after);
    if (IsPunc(after, "}")) {
      SPEC_RULE("Parse-FieldPatternTail-TrailingComma");
      EmitUnsupportedConstruct(after);
      return {after, xs};
    }
    SPEC_RULE("Parse-FieldPatternTail-Comma");
    ParseElemResult<FieldPattern> field = ParseFieldPattern(after);
    xs.push_back(field.elem);
    return ParseFieldPatternTail(field.parser, std::move(xs));
  }
  EmitParseSyntaxErr(parser, TokSpan(parser));
  return {parser, xs};
}

EnumPayloadOptResult ParseEnumPatternPayloadOpt(Parser parser) {
  if (!IsPunc(parser, "(") && !IsPunc(parser, "{")) {
    SPEC_RULE("Parse-EnumPatternPayloadOpt-None");
    return {parser, std::nullopt};
  }
  if (IsPunc(parser, "(")) {
    SPEC_RULE("Parse-EnumPatternPayloadOpt-Tuple");
    Parser next = parser;
    Advance(next);
    ParseElemResult<std::vector<PatternPtr>> elems = ParseTuplePatternElems(next);
    if (!IsPunc(elems.parser, ")")) {
      EmitParseSyntaxErr(elems.parser, TokSpan(elems.parser));
      return {elems.parser, std::nullopt};
    }
    Parser after = elems.parser;
    Advance(after);
    TuplePayloadPattern payload;
    payload.elements = std::move(elems.elem);
    return {after, EnumPayloadPattern{std::move(payload)}};
  }
  SPEC_RULE("Parse-EnumPatternPayloadOpt-Record");
  Parser next = parser;
  Advance(next);
  ParseElemResult<std::vector<FieldPattern>> fields = ParseFieldPatternList(next);
  if (!IsPunc(fields.parser, "}")) {
    EmitParseSyntaxErr(fields.parser, TokSpan(fields.parser));
    return {fields.parser, std::nullopt};
  }
  Parser after = fields.parser;
  Advance(after);
  RecordPayloadPattern payload;
  payload.fields = std::move(fields.elem);
  return {after, EnumPayloadPattern{std::move(payload)}};
}

ModalPayloadOptResult ParseModalPatternPayloadOpt(Parser parser) {
  if (!IsPunc(parser, "{")) {
    SPEC_RULE("Parse-ModalPatternPayloadOpt-None");
    return {parser, std::nullopt};
  }
  SPEC_RULE("Parse-ModalPatternPayloadOpt-Record");
  Parser next = parser;
  Advance(next);
  ParseElemResult<std::vector<FieldPattern>> fields = ParseFieldPatternList(next);
  if (!IsPunc(fields.parser, "}")) {
    EmitParseSyntaxErr(fields.parser, TokSpan(fields.parser));
    return {fields.parser, std::nullopt};
  }
  Parser after = fields.parser;
  Advance(after);
  ModalRecordPayload payload;
  payload.fields = std::move(fields.elem);
  return {after, payload};
}

ParseElemResult<PatternPtr> ParsePatternAtom(Parser parser) {
  const Token* tok = Tok(parser);
  if (!tok) {
    EmitParseSyntaxErr(parser, TokSpan(parser));
    return {parser, MakePattern(TokSpan(parser), WildcardPattern{})};
  }

  if (IsLiteralToken(*tok)) {
    SPEC_RULE("Parse-Pattern-Literal");
    Parser next = parser;
    Advance(next);
    LiteralPattern lit;
    lit.literal = *tok;
    return {next, MakePattern(tok->span, lit)};
  }

  if (IsIdentTok(*tok) && tok->lexeme == "_") {
    SPEC_RULE("Parse-Pattern-Wildcard");
    Parser next = parser;
    Advance(next);
    return {next, MakePattern(tok->span, WildcardPattern{})};
  }

  if (IsIdentTok(*tok)) {
    Parser next = parser;
    Advance(next);
    if (IsPunc(next, ":")) {
      SPEC_RULE("Parse-Pattern-Typed");
      Parser after = next;
      Advance(after);
      ParseElemResult<std::shared_ptr<Type>> ty = ParseType(after);
      TypedPattern pat;
      pat.name = tok->lexeme;
      pat.type = ty.elem;
      return {ty.parser, MakePattern(SpanBetween(parser, ty.parser), pat)};
    }
  }

  if (IsIdentTok(*tok)) {
    Parser next = parser;
    Advance(next);
    if (IsOp(next, "::")) {
      SPEC_RULE("Parse-Pattern-Enum");
      ParseQualifiedHeadResult head = ParseQualifiedHead(parser);
      EnumPayloadOptResult payload = ParseEnumPatternPayloadOpt(head.parser);
      EnumPattern pat;
      pat.path = head.module_path;
      pat.name = head.name;
      pat.payload_opt = payload.payload_opt;
      return {payload.parser, MakePattern(SpanBetween(parser, payload.parser), pat)};
    }
  }

  if (IsPunc(parser, "(")) {
    SPEC_RULE("Parse-Pattern-Tuple");
    Parser next = parser;
    Advance(next);
    ParseElemResult<std::vector<PatternPtr>> elems = ParseTuplePatternElems(next);
    if (!IsPunc(elems.parser, ")")) {
      EmitParseSyntaxErr(elems.parser, TokSpan(elems.parser));
      return {elems.parser,
              MakePattern(SpanBetween(parser, elems.parser), WildcardPattern{})};
    }
    Parser after = elems.parser;
    Advance(after);
    TuplePattern pat;
    pat.elements = std::move(elems.elem);
    return {after, MakePattern(SpanBetween(parser, after), pat)};
  }

  if (IsIdentTok(*tok)) {
    Parser start = parser;
    ParseElemResult<TypePath> path = ParseTypePath(parser);
    if (IsPunc(path.parser, "{")) {
      SPEC_RULE("Parse-Pattern-Record");
      Parser after = path.parser;
      Advance(after);
      ParseElemResult<std::vector<FieldPattern>> fields = ParseFieldPatternList(after);
      if (!IsPunc(fields.parser, "}")) {
        EmitParseSyntaxErr(fields.parser, TokSpan(fields.parser));
        return {fields.parser,
                MakePattern(SpanBetween(start, fields.parser), WildcardPattern{})};
      }
      Parser done = fields.parser;
      Advance(done);
      RecordPattern pat;
      pat.path = path.elem;
      pat.fields = std::move(fields.elem);
      return {done, MakePattern(SpanBetween(start, done), pat)};
    }
  }

  if (IsOp(parser, "@")) {
    SPEC_RULE("Parse-Pattern-Modal");
    Parser next = parser;
    Advance(next);
    ParseElemResult<Identifier> name = ParseIdent(next);
    ModalPayloadOptResult payload = ParseModalPatternPayloadOpt(name.parser);
    ModalPattern pat;
    pat.state = name.elem;
    pat.fields_opt = payload.fields_opt;
    return {payload.parser, MakePattern(SpanBetween(parser, payload.parser), pat)};
  }

  if (IsIdentTok(*tok)) {
    SPEC_RULE("Parse-Pattern-Identifier");
    Parser next = parser;
    Advance(next);
    IdentifierPattern pat;
    pat.name = tok->lexeme;
    return {next, MakePattern(tok->span, pat)};
  }

  EmitParseSyntaxErr(parser, TokSpan(parser));
  return {parser, MakePattern(TokSpan(parser), WildcardPattern{})};
}

ParseElemResult<PatternPtr> ParsePatternRange(Parser parser) {
  ParseElemResult<PatternPtr> lhs = ParsePatternAtom(parser);
  if (IsOp(lhs.parser, "..") || IsOp(lhs.parser, "..=")) {
    SPEC_RULE("Parse-Pattern-Range");
    Parser after = lhs.parser;
    const bool inclusive = IsOp(after, "..=");
    Advance(after);
    ParseElemResult<PatternPtr> rhs = ParsePatternAtom(after);
    RangePattern pat;
    pat.kind = inclusive ? RangeKind::Inclusive : RangeKind::Exclusive;
    pat.lo = lhs.elem;
    pat.hi = rhs.elem;
    return {rhs.parser, MakePattern(SpanBetween(parser, rhs.parser), pat)};
  }
  SPEC_RULE("Parse-Pattern-Range-None");
  return lhs;
}

}  // namespace

ParseElemResult<PatternPtr> ParsePattern(Parser parser) {
  const Token* tok = Tok(parser);
  if (!tok || !IsPatternStart(*tok)) {
    SPEC_RULE("Parse-Pattern-Err");
    EmitParseSyntaxErr(parser, TokSpan(parser));
    return {parser, MakePattern(TokSpan(parser), WildcardPattern{})};
  }
  SPEC_RULE("Parse-Pattern");
  return ParsePatternRange(parser);
}

}  // namespace cursive0::syntax
