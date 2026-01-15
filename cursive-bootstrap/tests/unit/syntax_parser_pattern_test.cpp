#include <cassert>
#include <string>
#include <vector>
#include <variant>

#include "cursive0/core/assert_spec.h"
#include "cursive0/core/source_text.h"
#include "cursive0/syntax/ast.h"
#include "cursive0/syntax/lexer.h"
#include "cursive0/syntax/parser.h"

namespace {

using cursive0::core::LineStarts;
using cursive0::core::SourceFile;
using cursive0::core::UnicodeScalar;
using cursive0::syntax::EnumPattern;
using cursive0::syntax::IdentifierPattern;
using cursive0::syntax::LiteralPattern;
using cursive0::syntax::ModalPattern;
using cursive0::syntax::ParseElemResult;
using cursive0::syntax::ParsePattern;
using cursive0::syntax::Parser;
using cursive0::syntax::PatternPtr;
using cursive0::syntax::RecordPattern;
using cursive0::syntax::Tokenize;
using cursive0::syntax::TokenizeResult;
using cursive0::syntax::TuplePattern;
using cursive0::syntax::TypedPattern;
using cursive0::syntax::WildcardPattern;
using cursive0::syntax::FilterNewlines;
using cursive0::syntax::MakeParser;
using cursive0::syntax::AtEof;
using cursive0::syntax::PStateOk;

static std::vector<UnicodeScalar> AsScalars(const std::string& text) {
  std::vector<UnicodeScalar> out;
  out.reserve(text.size());
  for (unsigned char c : text) {
    out.push_back(static_cast<UnicodeScalar>(c));
  }
  return out;
}

static SourceFile MakeSourceFile(const std::string& path,
                                 const std::string& text) {
  SourceFile s;
  s.path = path;
  s.scalars = AsScalars(text);
  s.text = cursive0::core::EncodeUtf8(s.scalars);
  s.bytes.assign(s.text.begin(), s.text.end());
  s.byte_len = s.text.size();
  s.line_starts = LineStarts(s.scalars);
  s.line_count = s.line_starts.size();
  return s;
}

static Parser MakeParserFor(const std::string& text) {
  SourceFile source = MakeSourceFile("pattern.cursive", text);
  TokenizeResult tok = Tokenize(source);
  assert(tok.output.has_value());
  auto tokens = std::make_shared<std::vector<cursive0::syntax::Token>>(
      FilterNewlines(tok.output->tokens));
  Parser parser = MakeParser(*tokens, source);
  parser.owned_tokens = tokens;
  return parser;
}

static ParseElemResult<PatternPtr> ParsePatternOk(const std::string& text) {
  Parser parser = MakeParserFor(text);
  ParseElemResult<PatternPtr> result = ParsePattern(parser);
  assert(result.elem != nullptr);
  assert(result.parser.diags.empty());
  assert(PStateOk(result.parser));
  assert(AtEof(result.parser));
  return result;
}

static ParseElemResult<PatternPtr> ParsePatternDiag(const std::string& text) {
  Parser parser = MakeParserFor(text);
  ParseElemResult<PatternPtr> result = ParsePattern(parser);
  assert(result.elem != nullptr);
  assert(!result.parser.diags.empty());
  assert(PStateOk(result.parser));
  return result;
}

}  // namespace

int main() {
  SPEC_COV("Parse-Pattern");
  SPEC_COV("Parse-Pattern-Err");
  SPEC_COV("Parse-Pattern-Identifier");
  SPEC_COV("Parse-Pattern-Literal");
  SPEC_COV("Parse-Pattern-Wildcard");
  SPEC_COV("Parse-Pattern-Typed");
  SPEC_COV("Parse-Pattern-Enum");
  SPEC_COV("Parse-Pattern-Record");
  SPEC_COV("Parse-Pattern-Tuple");
  SPEC_COV("Parse-Pattern-Modal");
  SPEC_COV("Parse-Pattern-Range");
  SPEC_COV("Parse-Pattern-Range-None");
  SPEC_COV("Parse-EnumPatternPayloadOpt-None");
  SPEC_COV("Parse-EnumPatternPayloadOpt-Tuple");
  SPEC_COV("Parse-EnumPatternPayloadOpt-Record");
  SPEC_COV("Parse-ModalPatternPayloadOpt-None");
  SPEC_COV("Parse-ModalPatternPayloadOpt-Record");
  SPEC_COV("Parse-FieldPattern");
  SPEC_COV("Parse-FieldPatternList-Cons");
  SPEC_COV("Parse-FieldPatternList-Empty");
  SPEC_COV("Parse-FieldPatternTail-Comma");
  SPEC_COV("Parse-FieldPatternTail-End");
  SPEC_COV("Parse-FieldPatternTail-TrailingComma");
  SPEC_COV("Parse-FieldPatternTailOpt-None");
  SPEC_COV("Parse-FieldPatternTailOpt-Yes");
  SPEC_COV("Parse-TuplePatternElems-Empty");
  SPEC_COV("Parse-TuplePatternElems-Many");
  SPEC_COV("Parse-TuplePatternElems-Single");
  SPEC_COV("Parse-TuplePatternElems-TrailingComma");
  SPEC_COV("Parse-PatternList-Cons");
  SPEC_COV("Parse-PatternList-Empty");
  SPEC_COV("Parse-PatternListTail-Comma");
  SPEC_COV("Parse-PatternListTail-End");
  SPEC_COV("Parse-PatternListTail-TrailingComma");

  {
    ParseElemResult<PatternPtr> pat = ParsePatternOk("x");
    assert(std::holds_alternative<IdentifierPattern>(pat.elem->node));
  }
  {
    ParseElemResult<PatternPtr> pat = ParsePatternOk("42");
    assert(std::holds_alternative<LiteralPattern>(pat.elem->node));
  }
  {
    ParseElemResult<PatternPtr> pat = ParsePatternOk("_");
    assert(std::holds_alternative<WildcardPattern>(pat.elem->node));
  }
  {
    ParseElemResult<PatternPtr> pat = ParsePatternOk("x: i32");
    assert(std::holds_alternative<TypedPattern>(pat.elem->node));
  }
  {
    ParseElemResult<PatternPtr> pat = ParsePatternOk("Enum::Variant");
    assert(std::holds_alternative<EnumPattern>(pat.elem->node));
  }
  ParsePatternOk("Enum::Variant(1, _)");
  ParsePatternOk("Enum::Variant{ x: _, y }");

  {
    ParseElemResult<PatternPtr> pat = ParsePatternOk("Rec{ x: _, y }");
    assert(std::holds_alternative<RecordPattern>(pat.elem->node));
  }

  {
    ParseElemResult<PatternPtr> pat = ParsePatternOk("(a, b)");
    assert(std::holds_alternative<TuplePattern>(pat.elem->node));
  }
  ParsePatternOk("()");
  ParsePatternOk("(a;)");
  ParsePatternDiag("(a,)");

  {
    ParseElemResult<PatternPtr> pat = ParsePatternOk("@Open");
    assert(std::holds_alternative<ModalPattern>(pat.elem->node));
  }
  ParsePatternOk("@Open{ x: _ }");

  ParsePatternOk("1 .. 2");
  ParsePatternOk("1 ..= 2");
  ParsePatternOk("a");

  ParsePatternOk("Enum::Empty{}");
  ParsePatternDiag("Rec{ x: _, }");

  ParsePatternDiag("+");

  return 0;
}
