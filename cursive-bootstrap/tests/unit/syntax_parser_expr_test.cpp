#include <cassert>
#include <cstdio>
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
using cursive0::syntax::ArrayExpr;
using cursive0::syntax::BlockExpr;
using cursive0::syntax::ExprPtr;
using cursive0::syntax::LiteralExpr;
using cursive0::syntax::MatchExpr;
using cursive0::syntax::MethodCallExpr;
using cursive0::syntax::ParseElemResult;
using cursive0::syntax::ParseExpr;
using cursive0::syntax::ParseExprOpt;
using cursive0::syntax::Parser;
using cursive0::syntax::PtrNullExpr;
using cursive0::syntax::QualifiedApplyExpr;
using cursive0::syntax::QualifiedNameExpr;
using cursive0::syntax::RecordExpr;
using cursive0::syntax::Tokenize;
using cursive0::syntax::TokenizeResult;
using cursive0::syntax::TupleExpr;
using cursive0::syntax::UnsafeBlockExpr;
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
  SourceFile source = MakeSourceFile("expr.cursive", text);
  TokenizeResult tok = Tokenize(source);
  assert(tok.output.has_value());
  auto tokens = std::make_shared<std::vector<cursive0::syntax::Token>>(
      FilterNewlines(tok.output->tokens));
  Parser parser = MakeParser(*tokens, source);
  parser.owned_tokens = tokens;
  return parser;
}

static ParseElemResult<ExprPtr> ParseExprOk(const std::string& text) {
  Parser parser = MakeParserFor(text);
  ParseElemResult<ExprPtr> result = ParseExpr(parser);
  assert(result.elem != nullptr);
  if (!result.parser.diags.empty()) {
    std::fprintf(stderr, "ParseExprOk diagnostics for: %s\n", text.c_str());
  }
  assert(result.parser.diags.empty());
  assert(PStateOk(result.parser));
  assert(AtEof(result.parser));
  return result;
}

static ParseElemResult<ExprPtr> ParseExprDiag(const std::string& text) {
  Parser parser = MakeParserFor(text);
  ParseElemResult<ExprPtr> result = ParseExpr(parser);
  assert(result.elem != nullptr);
  assert(!result.parser.diags.empty());
  assert(PStateOk(result.parser));
  return result;
}

static void ParseExprOptNone() {
  Parser parser = MakeParserFor(";");
  ParseElemResult<ExprPtr> result = ParseExprOpt(parser);
  assert(result.elem == nullptr);
  assert(result.parser.diags.empty());
}

static void ParseExprOptSome(const std::string& text) {
  Parser parser = MakeParserFor(text);
  ParseElemResult<ExprPtr> result = ParseExprOpt(parser);
  assert(result.elem != nullptr);
  assert(result.parser.diags.empty());
  assert(AtEof(result.parser));
}

}  // namespace

int main() {
  SPEC_COV("Parse-Expr");
  SPEC_COV("Parse-ExprOpt-None");
  SPEC_COV("Parse-ExprOpt-Yes");
  SPEC_COV("Parse-Arg");
  SPEC_COV("Parse-ArgList-Cons");
  SPEC_COV("Parse-ArgList-Empty");
  SPEC_COV("Parse-ArgMoveOpt-None");
  SPEC_COV("Parse-ArgMoveOpt-Yes");
  SPEC_COV("Parse-ArgTail-Comma");
  SPEC_COV("Parse-ArgTail-End");
  SPEC_COV("Parse-ArgTail-TrailingComma");
  SPEC_COV("Parse-ExprList-Cons");
  SPEC_COV("Parse-ExprList-Empty");
  SPEC_COV("Parse-ExprListTail-Comma");
  SPEC_COV("Parse-ExprListTail-End");
  SPEC_COV("Parse-ExprListTail-TrailingComma");
  SPEC_COV("Parse-TupleExprElems-Empty");
  SPEC_COV("Parse-TupleExprElems-Many");
  SPEC_COV("Parse-TupleExprElems-Single");
  SPEC_COV("Parse-TupleExprElems-TrailingComma");
  SPEC_COV("Parse-Array-Literal");
  SPEC_COV("Parse-Record-Literal");
  SPEC_COV("Parse-Record-Literal-ModalState");
  SPEC_COV("Parse-FieldInit-Explicit");
  SPEC_COV("Parse-FieldInit-Shorthand");
  SPEC_COV("Parse-FieldInitList-Cons");
  SPEC_COV("Parse-FieldInitList-Empty");
  SPEC_COV("Parse-FieldInitTail-Comma");
  SPEC_COV("Parse-FieldInitTail-End");
  SPEC_COV("Parse-FieldInitTail-TrailingComma");
  SPEC_COV("Parse-Parenthesized-Expr");
  SPEC_COV("Parse-Tuple-Literal");
  SPEC_COV("Parse-Identifier-Expr");
  SPEC_COV("Parse-Literal-Expr");
  SPEC_COV("Parse-Primary-Err");
  SPEC_COV("Parse-Null-Ptr");
  SPEC_COV("Parse-Place-Deref");
  SPEC_COV("Parse-Place-Postfix");
  SPEC_COV("Parse-Place-Err");
  SPEC_COV("Parse-Syntax-Err");
  SPEC_COV("Parse-Postfix");
  SPEC_COV("Parse-PostfixTail-Cons");
  SPEC_COV("Parse-PostfixTail-Stop");
  SPEC_COV("Postfix-Call");
  SPEC_COV("Postfix-Field");
  SPEC_COV("Postfix-Index");
  SPEC_COV("Postfix-MethodCall");
  SPEC_COV("Postfix-Propagate");
  SPEC_COV("Postfix-TupleIndex");
  SPEC_COV("Parse-Qualified-Apply-Paren");
  SPEC_COV("Parse-Qualified-Apply-Brace");
  SPEC_COV("Parse-Qualified-Name");
  SPEC_COV("Parse-If-Expr");
  SPEC_COV("Parse-ElseOpt-Block");
  SPEC_COV("Parse-ElseOpt-If");
  SPEC_COV("Parse-ElseOpt-None");
  SPEC_COV("Parse-Match-Expr");
  SPEC_COV("Parse-MatchArm");
  SPEC_COV("Parse-MatchArms-Cons");
  SPEC_COV("Parse-MatchArmsTail-Comma");
  SPEC_COV("Parse-MatchArmsTail-End");
  SPEC_COV("Parse-ArmBody-Expr");
  SPEC_COV("Parse-ArmBody-Block");
  SPEC_COV("Parse-GuardOpt-Yes");
  SPEC_COV("Parse-GuardOpt-None");
  SPEC_COV("Parse-Loop-Expr");
  SPEC_COV("Parse-LoopTail-Cond");
  SPEC_COV("Parse-LoopTail-Iter");
  SPEC_COV("Parse-LoopTail-Infinite");
  SPEC_COV("TryParsePatternIn-Ok");
  SPEC_COV("TryParsePatternIn-Fail");
  SPEC_COV("Parse-Range-Full");
  SPEC_COV("Parse-Range-To");
  SPEC_COV("Parse-Range-ToInc");
  SPEC_COV("Parse-Range-Lhs");
  SPEC_COV("Parse-RangeTail-From");
  SPEC_COV("Parse-RangeTail-Excl");
  SPEC_COV("Parse-RangeTail-Incl");
  SPEC_COV("Parse-RangeTail-None");
  SPEC_COV("Parse-Unary-Prefix");
  SPEC_COV("Parse-Unary-Deref");
  SPEC_COV("Parse-Unary-AddressOf");
  SPEC_COV("Parse-Unary-Move");
  SPEC_COV("Parse-Unary-Widen");
  SPEC_COV("Parse-Unary-Postfix");
  SPEC_COV("Parse-Cast");
  SPEC_COV("Parse-CastTail-As");
  SPEC_COV("Parse-CastTail-None");
  SPEC_COV("Parse-Power");
  SPEC_COV("Parse-PowerTail-Cons");
  SPEC_COV("Parse-PowerTail-None");
  SPEC_COV("Parse-LeftChain");
  SPEC_COV("Parse-LeftChain-Cons");
  SPEC_COV("Parse-LeftChain-Stop");
  SPEC_COV("Parse-Alloc-Implicit");
  SPEC_COV("Parse-Block-Expr");
  SPEC_COV("Parse-Unsafe-Expr");
  SPEC_COV("Parse-Transmute-Expr");
  SPEC_COV("Parse-Transmute-Expr-ShiftSplit");

  ParseExprOptNone();
  ParseExprOptSome("1");

  ParseExprOk("x");
  ParseExprOk("1");
  ParseExprOk("!x");
  ParseExprOk("-x");
  ParseExprOk("*x");
  ParseExprOk("&x");
  ParseExprOk("&*x");
  ParseExprDiag("&1");
  ParseExprOk("move x");
  ParseExprOk("widen x");
  ParseExprOk("x as i32");
  ParseExprOk("x + y * 2");
  ParseExprOk("2 ** 3");
  ParseExprOk("^x");

  ParseExprOk("f()");
  ParseExprOk("f(1)");
  ParseExprOk("f(move x)");
  ParseExprOk("f(1, 2)");
  ParseExprDiag("f(1,)");

  ParseExprOk("obj.field");
  ParseExprOk("tuple.0");
  ParseExprOk("arr[0]");
  ParseExprOk("obj~>m()");
  ParseExprOk("f()?");

  {
    ParseElemResult<ExprPtr> expr = ParseExprOk("(1)");
    assert(std::holds_alternative<LiteralExpr>(expr.elem->node));
  }
  {
    ParseElemResult<ExprPtr> expr = ParseExprOk("()");
    assert(std::holds_alternative<TupleExpr>(expr.elem->node));
  }
  {
    ParseElemResult<ExprPtr> expr = ParseExprOk("(1, 2)");
    assert(std::holds_alternative<TupleExpr>(expr.elem->node));
  }
  ParseExprOk("(1;)");
  ParseExprDiag("(1,)");

  {
    ParseElemResult<ExprPtr> expr = ParseExprOk("[1, 2]");
    assert(std::holds_alternative<ArrayExpr>(expr.elem->node));
  }
  ParseExprDiag("[]");
  ParseExprDiag("[1,]");

  {
    ParseElemResult<ExprPtr> expr = ParseExprOk("R { x: 1, y }");
    assert(std::holds_alternative<RecordExpr>(expr.elem->node));
  }
  ParseExprOk("File@Open { x: 1 }");
  ParseExprDiag("R { }");
  ParseExprDiag("R { x: 1, }");

  ParseExprOk("Foo::bar + 1");
  {
    ParseElemResult<ExprPtr> expr = ParseExprOk("Foo::bar(1)");
    assert(std::holds_alternative<QualifiedApplyExpr>(expr.elem->node));
  }
  {
    ParseElemResult<ExprPtr> expr = ParseExprOk("Foo::Bar { x: 1 }");
    assert(std::holds_alternative<QualifiedApplyExpr>(expr.elem->node));
  }

  {
    ParseElemResult<ExprPtr> expr = ParseExprOk("Ptr::null()");
    assert(std::holds_alternative<PtrNullExpr>(expr.elem->node));
  }

  {
    ParseElemResult<ExprPtr> expr = ParseExprOk("if 1 { } else { }");
    (void)expr;
  }
  ParseExprOk("if 1 { } else if 2 { }");
  ParseExprOk("if 1 { }");

  {
    ParseElemResult<ExprPtr> expr = ParseExprOk(
        "match x { _ => 1, _ => { } }");
    assert(std::holds_alternative<MatchExpr>(expr.elem->node));
  }
  ParseExprOk("match x { _ if y => 1 }");

  ParseExprOk("loop { }");
  ParseExprOk("loop x in xs { }");
  ParseExprOk("loop x > 0 { }");

  ParseExprOk("..");
  ParseExprOk("..=10");
  ParseExprOk("..10");
  ParseExprOk("1..");
  ParseExprOk("1..2");
  ParseExprOk("1..=2");

  ParseExprOk("{ }");
  {
    ParseElemResult<ExprPtr> expr = ParseExprOk("unsafe { }");
    assert(std::holds_alternative<UnsafeBlockExpr>(expr.elem->node));
  }

  ParseExprOk("transmute::<i32, i32>(0)");
  ParseExprOk("transmute::<Ptr<Ptr<i32>>, Ptr<Ptr<i32>>>(x)");

  ParseExprDiag("@");

  return 0;
}
