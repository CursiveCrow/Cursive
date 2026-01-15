#include "cursive0/syntax/parser.h"


#include <optional>
#include <span>
#include <string_view>
#include <utility>

#include "cursive0/core/diagnostic_messages.h"
#include "cursive0/core/diagnostics.h"
#include "cursive0/core/span.h"
#include "cursive0/syntax/keyword_policy.h"

namespace cursive0::syntax {

namespace {

#include "precedence_table.inc"
void EmitUnsupportedConstruct(Parser& parser) {
  auto diag = core::MakeDiagnostic("E-UNS-0101");
  if (!diag) {
    return;
  }
  parser.diags = core::Emit(parser.diags, *diag);
}

bool IsKw(const Parser& parser, std::string_view kw) {
  const Token* tok = Tok(parser);
  return tok && IsKwTok(*tok, kw);
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

void NormalizeBindingPattern(std::shared_ptr<Pattern>& pat,
                             std::shared_ptr<Type>& type_opt) {
  if (!pat || type_opt) {
    return;
  }
  const auto* typed = std::get_if<TypedPattern>(&pat->node);
  if (!typed) {
    return;
  }
  auto normalized = std::make_shared<Pattern>();
  normalized->span = pat->span;
  normalized->node = IdentifierPattern{typed->name};
  type_opt = typed->type;
  pat = std::move(normalized);
}

bool IsExprStart(const Token& tok) {
  if (IsIdentTok(tok) || IsLiteralToken(tok)) {
    return true;
  }
  if (tok.kind == TokenKind::Punctuator) {
    return tok.lexeme == "(" || tok.lexeme == "[" || tok.lexeme == "{";
  }
  if (tok.kind == TokenKind::Operator) {
    return tok.lexeme == "!" || tok.lexeme == "-" || tok.lexeme == "&" ||
           tok.lexeme == "*" || tok.lexeme == "^";
  }
  if (tok.kind == TokenKind::Keyword) {
    return tok.lexeme == "if" || tok.lexeme == "match" ||
           tok.lexeme == "loop" || tok.lexeme == "unsafe" ||
           tok.lexeme == "move" || tok.lexeme == "transmute" ||
           tok.lexeme == "widen";
  }
  return false;
}

bool IsPostfixStart(const Token& tok) {
  if (tok.kind == TokenKind::Punctuator) {
    return tok.lexeme == "." || tok.lexeme == "[" || tok.lexeme == "(";
  }
  if (tok.kind == TokenKind::Operator) {
    return tok.lexeme == "~>" || tok.lexeme == "?";
  }
  return false;
}

bool IsOpInSet(const Token& tok, std::span<const std::string_view> ops) {
  if (tok.kind != TokenKind::Operator) {
    return false;
  }
  for (const std::string_view op : ops) {
    if (tok.lexeme == op) {
      return true;
    }
  }
  return false;
}

Parser SkipBracedBlock(Parser parser) {
  if (!IsPunc(parser, "{")) {
    return parser;
  }
  Parser cur = parser;
  int depth = 0;
  while (!AtEof(cur)) {
    const Token* tok = Tok(cur);
    if (tok && tok->kind == TokenKind::Punctuator) {
      if (tok->lexeme == "{") {
        depth += 1;
      } else if (tok->lexeme == "}") {
        depth -= 1;
        Advance(cur);
        if (depth == 0) {
          return cur;
        }
        continue;
      }
    }
    Advance(cur);
  }
  return cur;
}

core::Span SpanCover(const core::Span& start, const core::Span& end) {
  core::Span span = start;
  span.end_offset = end.end_offset;
  span.end_line = end.end_line;
  span.end_col = end.end_col;
  return span;
}

ExprPtr MakeExpr(const core::Span& span, ExprNode node) {
  auto expr = std::make_shared<Expr>();
  expr->span = span;
  expr->node = std::move(node);
  return expr;
}

bool IsPlace(const ExprPtr& expr) {
  if (!expr) {
    return false;
  }
  if (std::holds_alternative<IdentifierExpr>(expr->node)) {
    return true;
  }
  if (std::holds_alternative<FieldAccessExpr>(expr->node)) {
    return true;
  }
  if (std::holds_alternative<TupleAccessExpr>(expr->node)) {
    return true;
  }
  if (std::holds_alternative<IndexAccessExpr>(expr->node)) {
    return true;
  }
  if (const auto* deref = std::get_if<DerefExpr>(&expr->node)) {
    return IsPlace(deref->value);
  }
  return false;
}

ParseElemResult<ExprPtr> ParseExprNoBrace(Parser parser);

ParseElemResult<ExprPtr> ParseRange(Parser parser, bool allow_brace);
ParseElemResult<ExprPtr> ParseRangeTail(Parser parser, const ExprPtr& lhs,
                                        const Parser& start,
                                        bool allow_brace);

using ParseExprFn = ParseElemResult<ExprPtr> (*)(Parser, bool);

ParseElemResult<ExprPtr> ParseLeftChain(Parser parser,
                                       std::span<const std::string_view> ops,
                                       ParseExprFn parse_higher,
                                       bool allow_brace);
ParseElemResult<ExprPtr> ParseLeftChainTail(Parser parser, ExprPtr lhs,
                                           std::span<const std::string_view> ops,
                                           ParseExprFn parse_higher,
                                           bool allow_brace);

ParseElemResult<ExprPtr> ParseLogicalOr(Parser parser, bool allow_brace);
ParseElemResult<ExprPtr> ParseLogicalAnd(Parser parser, bool allow_brace);
ParseElemResult<ExprPtr> ParseComparison(Parser parser, bool allow_brace);
ParseElemResult<ExprPtr> ParseBitOr(Parser parser, bool allow_brace);
ParseElemResult<ExprPtr> ParseBitXor(Parser parser, bool allow_brace);
ParseElemResult<ExprPtr> ParseBitAnd(Parser parser, bool allow_brace);
ParseElemResult<ExprPtr> ParseShift(Parser parser, bool allow_brace);
ParseElemResult<ExprPtr> ParseAdd(Parser parser, bool allow_brace);
ParseElemResult<ExprPtr> ParseMul(Parser parser, bool allow_brace);
ParseElemResult<ExprPtr> ParsePower(Parser parser, bool allow_brace);
ParseElemResult<ExprPtr> ParsePowerTail(Parser parser, ExprPtr lhs,
                                       bool allow_brace);
ParseElemResult<ExprPtr> ParseCast(Parser parser, bool allow_brace);
ParseElemResult<ExprPtr> ParseCastTail(Parser parser, ExprPtr lhs);
ParseElemResult<ExprPtr> ParseUnary(Parser parser, bool allow_brace);
ParseElemResult<ExprPtr> ParsePostfix(Parser parser, bool allow_brace);
ParseElemResult<ExprPtr> ParsePostfixTail(Parser parser, ExprPtr expr,
                                         bool allow_brace);
ParseElemResult<ExprPtr> PostfixStep(Parser parser, ExprPtr expr);
ParseElemResult<ExprPtr> ParsePrimary(Parser parser, bool allow_brace);

ParseElemResult<std::vector<Arg>> ParseArgList(Parser parser);
ParseElemResult<Arg> ParseArg(Parser parser);
ParseElemResult<bool> ParseArgMoveOpt(Parser parser);
ParseElemResult<std::vector<Arg>> ParseArgTail(Parser parser,
                                               std::vector<Arg> xs);

ParseElemResult<std::vector<ExprPtr>> ParseExprList(Parser parser);
ParseElemResult<std::vector<ExprPtr>> ParseExprListTail(Parser parser,
                                                        std::vector<ExprPtr> xs);

ParseElemResult<std::vector<ExprPtr>> ParseTupleExprElems(Parser parser);

struct TupleScanResult {
  bool is_tuple = false;
};

int ParenDelta(const Token& tok) {
  if (tok.kind != TokenKind::Punctuator) {
    return 0;
  }
  if (tok.lexeme == "(") {
    return 1;
  }
  if (tok.lexeme == ")") {
    return -1;
  }
  return 0;
}

TupleScanResult TupleScan(Parser parser, int depth) {
  TupleScanResult result;
  Parser cur = parser;
  int d = depth;
  for (;;) {
    if (AtEof(cur)) {
      result.is_tuple = false;
      return result;
    }
    const Token* tok = Tok(cur);
    if (!tok) {
      result.is_tuple = false;
      return result;
    }
    if (tok->kind == TokenKind::Punctuator && tok->lexeme == ")" && d == 1) {
      result.is_tuple = false;
      return result;
    }
    if (tok->kind == TokenKind::Punctuator &&
        (tok->lexeme == "," || tok->lexeme == ";") && d == 1) {
      result.is_tuple = true;
      return result;
    }
    d += ParenDelta(*tok);
    Advance(cur);
  }
}

bool TupleParen(Parser parser) {
  if (!IsPunc(parser, "(")) {
    return false;
  }
  Parser next = parser;
  Advance(next);
  if (IsPunc(next, ")")) {
    return true;
  }
  TupleScanResult scan = TupleScan(next, 1);
  return scan.is_tuple;
}

ParseElemResult<std::vector<FieldInit>> ParseFieldInitList(Parser parser);
ParseElemResult<FieldInit> ParseFieldInit(Parser parser);
ParseElemResult<std::vector<FieldInit>> ParseFieldInitTail(Parser parser,
                                                           std::vector<FieldInit> xs);

struct MatchArmsResult {
  Parser parser;
  std::vector<MatchArm> arms;
};

MatchArmsResult ParseMatchArms(Parser parser);
MatchArmsResult ParseMatchArmsTail(Parser parser, std::vector<MatchArm> xs);

struct GuardOptResult {
  Parser parser;
  ExprPtr guard_opt;
};

GuardOptResult ParseGuardOpt(Parser parser);

struct ArmBodyResult {
  Parser parser;
  ExprPtr body;
};

ArmBodyResult ParseArmBody(Parser parser);

struct LoopTailResult {
  Parser parser;
  ExprPtr loop_expr;
};

LoopTailResult ParseLoopTail(Parser parser);

struct TryPatternInResult {
  Parser parser;
  std::shared_ptr<Pattern> pattern;
  bool ok = false;
};

TryPatternInResult TryParsePatternIn(Parser parser);

struct ElseOptResult {
  Parser parser;
  ExprPtr else_opt;
};

ElseOptResult ParseElseOpt(Parser parser);

ParseElemResult<ExprPtr> ParsePlace(Parser parser, bool allow_brace);

ParseElemResult<ExprPtr> ParseExprNoBrace(Parser parser) {
  return ParseRange(parser, false);
}

ParseElemResult<ExprPtr> ParseRange(Parser parser, bool allow_brace) {
  Parser start = parser;
  if (IsOp(parser, "..=")) {
    SPEC_RULE("Parse-Range-ToInc");
    Parser next = parser;
    Advance(next);
    ParseElemResult<ExprPtr> rhs = ParseLogicalOr(next, allow_brace);
    RangeExpr range;
    range.kind = RangeKind::ToInclusive;
    range.lhs = nullptr;
    range.rhs = rhs.elem;
    return {rhs.parser, MakeExpr(SpanBetween(start, rhs.parser), range)};
  }
  if (IsOp(parser, "..")) {
    Parser next = parser;
    Advance(next);
    const Token* after = Tok(next);
    if (!after || !IsExprStart(*after)) {
      SPEC_RULE("Parse-Range-Full");
      RangeExpr range;
      range.kind = RangeKind::Full;
      range.lhs = nullptr;
      range.rhs = nullptr;
      return {next, MakeExpr(SpanBetween(start, next), range)};
    }
    SPEC_RULE("Parse-Range-To");
    ParseElemResult<ExprPtr> rhs = ParseLogicalOr(next, allow_brace);
    RangeExpr range;
    range.kind = RangeKind::To;
    range.lhs = nullptr;
    range.rhs = rhs.elem;
    return {rhs.parser, MakeExpr(SpanBetween(start, rhs.parser), range)};
  }

  SPEC_RULE("Parse-Range-Lhs");
  ParseElemResult<ExprPtr> lhs = ParseLogicalOr(parser, allow_brace);
  return ParseRangeTail(lhs.parser, lhs.elem, start, allow_brace);
}

ParseElemResult<ExprPtr> ParseRangeTail(Parser parser, const ExprPtr& lhs,
                                        const Parser& start,
                                        bool allow_brace) {
  if (!(IsOp(parser, "..") || IsOp(parser, "..="))) {
    SPEC_RULE("Parse-RangeTail-None");
    return {parser, lhs};
  }

  if (IsOp(parser, "..")) {
    Parser after = parser;
    Advance(after);
    const Token* next = Tok(after);
    if (!next || !IsExprStart(*next)) {
      SPEC_RULE("Parse-RangeTail-From");
      RangeExpr range;
      range.kind = RangeKind::From;
      range.lhs = lhs;
      range.rhs = nullptr;
      return {after, MakeExpr(SpanBetween(start, after), range)};
    }
    SPEC_RULE("Parse-RangeTail-Excl");
    ParseElemResult<ExprPtr> rhs = ParseLogicalOr(after, allow_brace);
    RangeExpr range;
    range.kind = RangeKind::Exclusive;
    range.lhs = lhs;
    range.rhs = rhs.elem;
    return {rhs.parser, MakeExpr(SpanBetween(start, rhs.parser), range)};
  }

  SPEC_RULE("Parse-RangeTail-Incl");
  Parser after = parser;
  Advance(after);
  ParseElemResult<ExprPtr> rhs = ParseLogicalOr(after, allow_brace);
  RangeExpr range;
  range.kind = RangeKind::Inclusive;
  range.lhs = lhs;
  range.rhs = rhs.elem;
  return {rhs.parser, MakeExpr(SpanBetween(start, rhs.parser), range)};
}

ParseElemResult<ExprPtr> ParseLeftChain(Parser parser,
                                       std::span<const std::string_view> ops,
                                       ParseExprFn parse_higher,
                                       bool allow_brace) {
  SPEC_RULE("Parse-LeftChain");
  ParseElemResult<ExprPtr> lhs = parse_higher(parser, allow_brace);
  return ParseLeftChainTail(lhs.parser, lhs.elem, ops, parse_higher, allow_brace);
}

ParseElemResult<ExprPtr> ParseLeftChainTail(Parser parser, ExprPtr lhs,
                                           std::span<const std::string_view> ops,
                                           ParseExprFn parse_higher,
                                           bool allow_brace) {
  const Token* tok = Tok(parser);
  if (!tok || !IsOpInSet(*tok, ops)) {
    SPEC_RULE("Parse-LeftChain-Stop");
    return {parser, lhs};
  }
  SPEC_RULE("Parse-LeftChain-Cons");
  const Identifier op = tok->lexeme;
  Parser next = parser;
  Advance(next);
  ParseElemResult<ExprPtr> rhs = parse_higher(next, allow_brace);
  BinaryExpr bin;
  bin.op = op;
  bin.lhs = lhs;
  bin.rhs = rhs.elem;
  ExprPtr expr = MakeExpr(SpanCover(lhs->span, rhs.elem->span), bin);
  return ParseLeftChainTail(rhs.parser, expr, ops, parse_higher, allow_brace);
}

ParseElemResult<ExprPtr> ParseLogicalOr(Parser parser, bool allow_brace) {
  return ParseLeftChain(parser, kLogicalOrOps, ParseLogicalAnd, allow_brace);
}

ParseElemResult<ExprPtr> ParseLogicalAnd(Parser parser, bool allow_brace) {
  return ParseLeftChain(parser, kLogicalAndOps, ParseComparison, allow_brace);
}

ParseElemResult<ExprPtr> ParseComparison(Parser parser, bool allow_brace) {
  return ParseLeftChain(parser, kComparisonOps, ParseBitOr, allow_brace);
}

ParseElemResult<ExprPtr> ParseBitOr(Parser parser, bool allow_brace) {
  return ParseLeftChain(parser, kBitOrOps, ParseBitXor, allow_brace);
}

ParseElemResult<ExprPtr> ParseBitXor(Parser parser, bool allow_brace) {
  return ParseLeftChain(parser, kBitXorOps, ParseBitAnd, allow_brace);
}

ParseElemResult<ExprPtr> ParseBitAnd(Parser parser, bool allow_brace) {
  return ParseLeftChain(parser, kBitAndOps, ParseShift, allow_brace);
}

ParseElemResult<ExprPtr> ParseShift(Parser parser, bool allow_brace) {
  return ParseLeftChain(parser, kShiftOps, ParseAdd, allow_brace);
}

ParseElemResult<ExprPtr> ParseAdd(Parser parser, bool allow_brace) {
  return ParseLeftChain(parser, kAddOps, ParseMul, allow_brace);
}

ParseElemResult<ExprPtr> ParseMul(Parser parser, bool allow_brace) {
  return ParseLeftChain(parser, kMulOps, ParsePower, allow_brace);
}

ParseElemResult<ExprPtr> ParsePower(Parser parser, bool allow_brace) {
  SPEC_RULE("Parse-Power");
  ParseElemResult<ExprPtr> lhs = ParseCast(parser, allow_brace);
  return ParsePowerTail(lhs.parser, lhs.elem, allow_brace);
}

ParseElemResult<ExprPtr> ParsePowerTail(Parser parser, ExprPtr lhs,
                                       bool allow_brace) {
  if (!IsOp(parser, "**")) {
    SPEC_RULE("Parse-PowerTail-None");
    return {parser, lhs};
  }
  SPEC_RULE("Parse-PowerTail-Cons");
  Parser next = parser;
  Advance(next);
  ParseElemResult<ExprPtr> rhs = ParsePower(next, allow_brace);
  BinaryExpr bin;
  bin.op = "**";
  bin.lhs = lhs;
  bin.rhs = rhs.elem;
  return {rhs.parser, MakeExpr(SpanCover(lhs->span, rhs.elem->span), bin)};
}

ParseElemResult<ExprPtr> ParseCast(Parser parser, bool allow_brace) {
  SPEC_RULE("Parse-Cast");
  ParseElemResult<ExprPtr> lhs = ParseUnary(parser, allow_brace);
  return ParseCastTail(lhs.parser, lhs.elem);
}

ParseElemResult<ExprPtr> ParseCastTail(Parser parser, ExprPtr lhs) {
  if (!IsKw(parser, "as")) {
    SPEC_RULE("Parse-CastTail-None");
    return {parser, lhs};
  }
  SPEC_RULE("Parse-CastTail-As");
  Parser next = parser;
  Advance(next);
  ParseElemResult<std::shared_ptr<Type>> ty = ParseType(next);
  CastExpr cast;
  cast.value = lhs;
  cast.type = ty.elem;
  return {ty.parser, MakeExpr(SpanCover(lhs->span, ty.elem->span), cast)};
}

ParseElemResult<ExprPtr> ParseUnary(Parser parser, bool allow_brace) {
  if (IsOp(parser, "!") || IsOp(parser, "-")) {
    SPEC_RULE("Parse-Unary-Prefix");
    const Token* tok = Tok(parser);
    Identifier op = tok ? tok->lexeme : "";
    Parser next = parser;
    Advance(next);
    ParseElemResult<ExprPtr> rhs = ParseUnary(next, allow_brace);
    UnaryExpr unary;
    unary.op = op;
    unary.value = rhs.elem;
    return {rhs.parser, MakeExpr(SpanCover(TokSpan(parser), rhs.elem->span),
                                 unary)};
  }
  if (IsOp(parser, "*")) {
    SPEC_RULE("Parse-Unary-Deref");
    Parser next = parser;
    Advance(next);
    ParseElemResult<ExprPtr> rhs = ParseUnary(next, allow_brace);
    DerefExpr deref;
    deref.value = rhs.elem;
    return {rhs.parser,
            MakeExpr(SpanCover(TokSpan(parser), rhs.elem->span), deref)};
  }
  if (IsOp(parser, "&")) {
    SPEC_RULE("Parse-Unary-AddressOf");
    Parser next = parser;
    Advance(next);
    ParseElemResult<ExprPtr> place = ParsePlace(next, allow_brace);
    AddressOfExpr addr;
    addr.place = place.elem;
    return {place.parser,
            MakeExpr(SpanCover(TokSpan(parser), place.elem->span), addr)};
  }
  if (IsKw(parser, "move")) {
    SPEC_RULE("Parse-Unary-Move");
    Parser next = parser;
    Advance(next);
    ParseElemResult<ExprPtr> place = ParsePlace(next, allow_brace);
    MoveExpr move;
    move.place = place.elem;
    return {place.parser,
            MakeExpr(SpanCover(TokSpan(parser), place.elem->span), move)};
  }
  if (IsKw(parser, "widen")) {
    SPEC_RULE("Parse-Unary-Widen");
    Parser next = parser;
    Advance(next);
    ParseElemResult<ExprPtr> rhs = ParseUnary(next, allow_brace);
    UnaryExpr unary;
    unary.op = "widen";
    unary.value = rhs.elem;
    return {rhs.parser,
            MakeExpr(SpanCover(TokSpan(parser), rhs.elem->span), unary)};
  }
  SPEC_RULE("Parse-Unary-Postfix");
  return ParsePostfix(parser, allow_brace);
}

ParseElemResult<ExprPtr> ParsePostfix(Parser parser, bool allow_brace) {
  SPEC_RULE("Parse-Postfix");
  ParseElemResult<ExprPtr> primary = ParsePrimary(parser, allow_brace);
  return ParsePostfixTail(primary.parser, primary.elem, allow_brace);
}

ParseElemResult<ExprPtr> ParsePostfixTail(Parser parser, ExprPtr expr,
                                         bool allow_brace) {
  const Token* tok = Tok(parser);
  if (!tok || !IsPostfixStart(*tok)) {
    SPEC_RULE("Parse-PostfixTail-Stop");
    return {parser, expr};
  }
  SPEC_RULE("Parse-PostfixTail-Cons");
  ParseElemResult<ExprPtr> step = PostfixStep(parser, expr);
  return ParsePostfixTail(step.parser, step.elem, allow_brace);
}

ParseElemResult<ExprPtr> PostfixStep(Parser parser, ExprPtr expr) {
  if (IsPunc(parser, ".")) {
    Parser next = parser;
    Advance(next);
    const Token* tok = Tok(next);
    if (tok && IsIdentTok(*tok)) {
      SPEC_RULE("Postfix-Field");
      Identifier name = tok->lexeme;
      Parser after = next;
      Advance(after);
      FieldAccessExpr field;
      field.base = expr;
      field.name = name;
      return {after, MakeExpr(SpanCover(expr->span, tok->span), field)};
    }
    if (tok && tok->kind == TokenKind::IntLiteral) {
      SPEC_RULE("Postfix-TupleIndex");
      Token index = *tok;
      Parser after = next;
      Advance(after);
      TupleAccessExpr access;
      access.base = expr;
      access.index = index;
      return {after, MakeExpr(SpanCover(expr->span, index.span), access)};
    }
    EmitParseSyntaxErr(next, TokSpan(next));
    Parser sync = next;
    SyncStmt(sync);
    return {sync, MakeExpr(SpanBetween(parser, sync), ErrorExpr{})};
  }
  if (IsPunc(parser, "[")) {
    SPEC_RULE("Postfix-Index");
    Parser next = parser;
    Advance(next);
    ParseElemResult<ExprPtr> index = ParseExpr(next);
    if (!IsPunc(index.parser, "]")) {
      EmitParseSyntaxErr(index.parser, TokSpan(index.parser));
      Parser sync = index.parser;
      SyncStmt(sync);
      return {sync, MakeExpr(SpanBetween(parser, sync), ErrorExpr{})};
    }
    core::Span end_span = TokSpan(index.parser);
    Parser after = index.parser;
    Advance(after);
    IndexAccessExpr access;
    access.base = expr;
    access.index = index.elem;
    return {after, MakeExpr(SpanCover(expr->span, end_span), access)};
  }
  if (IsPunc(parser, "(")) {
    SPEC_RULE("Postfix-Call");
    Parser next = parser;
    Advance(next);
    ParseElemResult<std::vector<Arg>> args = ParseArgList(next);
    if (!IsPunc(args.parser, ")")) {
      EmitParseSyntaxErr(args.parser, TokSpan(args.parser));
      Parser sync = args.parser;
      SyncStmt(sync);
      return {sync, MakeExpr(SpanBetween(parser, sync), ErrorExpr{})};
    }
    core::Span end_span = TokSpan(args.parser);
    Parser after = args.parser;
    Advance(after);
    CallExpr call;
    call.callee = expr;
    call.args = std::move(args.elem);
    return {after, MakeExpr(SpanCover(expr->span, end_span), call)};
  }
  if (IsOp(parser, "~>")) {
    SPEC_RULE("Postfix-MethodCall");
    Parser next = parser;
    Advance(next);
    ParseElemResult<Identifier> name = ParseIdent(next);
    if (!IsPunc(name.parser, "(")) {
      EmitParseSyntaxErr(name.parser, TokSpan(name.parser));
      Parser sync = name.parser;
      SyncStmt(sync);
      return {sync, MakeExpr(SpanBetween(parser, sync), ErrorExpr{})};
    }
    Parser after_l = name.parser;
    Advance(after_l);
    ParseElemResult<std::vector<Arg>> args = ParseArgList(after_l);
    if (!IsPunc(args.parser, ")")) {
      EmitParseSyntaxErr(args.parser, TokSpan(args.parser));
      Parser sync = args.parser;
      SyncStmt(sync);
      return {sync, MakeExpr(SpanBetween(parser, sync), ErrorExpr{})};
    }
    core::Span end_span = TokSpan(args.parser);
    Parser after = args.parser;
    Advance(after);
    MethodCallExpr call;
    call.receiver = expr;
    call.name = name.elem;
    call.args = std::move(args.elem);
    return {after, MakeExpr(SpanCover(expr->span, end_span), call)};
  }
  if (IsOp(parser, "?")) {
    SPEC_RULE("Postfix-Propagate");
    core::Span end_span = TokSpan(parser);
    Parser next = parser;
    Advance(next);
    PropagateExpr prop;
    prop.value = expr;
    return {next, MakeExpr(SpanCover(expr->span, end_span), prop)};
  }

  EmitParseSyntaxErr(parser, TokSpan(parser));
  Parser sync = parser;
  SyncStmt(sync);
  return {sync, MakeExpr(SpanBetween(parser, sync), ErrorExpr{})};
}

ParseElemResult<ExprPtr> ParsePrimary(Parser parser, bool allow_brace) {
  const Token* tok = Tok(parser);
  if (tok && (tok->kind == TokenKind::Identifier ||
              tok->kind == TokenKind::Keyword) &&
      tok->lexeme == "comptime") {
    Parser next = parser;
    Advance(next);
    if (IsPunc(next, "{")) {
      SPEC_RULE("Parse-Comptime-Unsupported");
      EmitUnsupportedConstruct(parser);
      Parser sync = parser;
      SyncStmt(sync);
      return {sync, MakeExpr(SpanBetween(parser, sync), ErrorExpr{})};
    }
  }

  if (tok && IsLiteralToken(*tok)) {
    SPEC_RULE("Parse-Literal-Expr");
    LiteralExpr lit;
    lit.literal = *tok;
    Parser next = parser;
    Advance(next);
    return {next, MakeExpr(tok->span, lit)};
  }

  if (tok && IsIdentTok(*tok) && tok->lexeme == "Ptr") {
    Parser next = parser;
    Advance(next);
    if (IsOp(next, "::")) {
      Parser after_colon = next;
      Advance(after_colon);
      const Token* lit = Tok(after_colon);
      Parser after_lit = after_colon;
      if (lit && lit->kind == TokenKind::NullLiteral) {
        Advance(after_lit);
        if (IsPunc(after_lit, "(")) {
          Parser after_l = after_lit;
          Advance(after_l);
          if (IsPunc(after_l, ")")) {
            SPEC_RULE("Parse-Null-Ptr");
            Parser after = after_l;
            Advance(after);
            PtrNullExpr ptr;
            return {after, MakeExpr(SpanBetween(parser, after), ptr)};
          }
        }
      }
    }
  }

  if (tok && IsOpTok(*tok, "^")) {
    SPEC_RULE("Parse-Alloc-Implicit");
    Parser next = parser;
    Advance(next);
    ParseElemResult<ExprPtr> expr = ParseExpr(next);
    AllocExpr alloc;
    alloc.region_opt = std::nullopt;
    alloc.value = expr.elem;
    return {expr.parser, MakeExpr(SpanBetween(parser, expr.parser), alloc)};
  }

  if (tok && IsIdentTok(*tok)) {
    Parser next = parser;
    Advance(next);
    const Token* look = Tok(next);
    const bool is_qual = look && IsOpTok(*look, "::");
    const bool is_modal = look && IsOpTok(*look, "@");
    const bool is_record = look &&
        look->kind == TokenKind::Punctuator && look->lexeme == "{";
    if (!look || (!is_qual && (!allow_brace || !is_modal) &&
                  (!allow_brace || !is_record))) {
      SPEC_RULE("Parse-Identifier-Expr");
      IdentifierExpr ident;
      ident.name = tok->lexeme;
      return {next, MakeExpr(tok->span, ident)};
    }
  }

  if (tok && IsIdentTok(*tok)) {
    Parser next = parser;
    Advance(next);
    const Token* look = Tok(next);
    if (look && IsOpTok(*look, "::")) {
      ParseQualifiedHeadResult head = ParseQualifiedHead(parser);
      const Token* after_head = Tok(head.parser);
      if (after_head && IsOpTok(*after_head, "<")) {
        SPEC_RULE("Parse-Qualified-Generic-Unsupported");
        Parser skip = SkipAngles(head.parser);
        EmitUnsupportedConstruct(skip);
        SyncStmt(skip);
        return {skip, MakeExpr(SpanBetween(parser, skip), ErrorExpr{})};
      }
      if (after_head && IsPuncTok(*after_head, "(")) {
        SPEC_RULE("Parse-Qualified-Apply-Paren");
        Parser after_l = head.parser;
        Advance(after_l);
        ParseElemResult<std::vector<Arg>> args = ParseArgList(after_l);
        if (!IsPunc(args.parser, ")")) {
          EmitParseSyntaxErr(args.parser, TokSpan(args.parser));
          Parser sync = args.parser;
          SyncStmt(sync);
          return {sync, MakeExpr(SpanBetween(parser, sync), ErrorExpr{})};
        }
        Parser after = args.parser;
        Advance(after);
        QualifiedApplyExpr app;
        app.path = head.module_path;
        app.name = head.name;
        app.args = ParenArgs{std::move(args.elem)};
        return {after, MakeExpr(SpanBetween(parser, after), app)};
      }
      if (allow_brace && after_head && IsPuncTok(*after_head, "{")) {
        SPEC_RULE("Parse-Qualified-Apply-Brace");
        Parser after_l = head.parser;
        Advance(after_l);
        ParseElemResult<std::vector<FieldInit>> fields = ParseFieldInitList(after_l);
        if (!IsPunc(fields.parser, "}")) {
          EmitParseSyntaxErr(fields.parser, TokSpan(fields.parser));
          Parser sync = fields.parser;
          SyncStmt(sync);
          return {sync, MakeExpr(SpanBetween(parser, sync), ErrorExpr{})};
        }
        Parser after = fields.parser;
        Advance(after);
        QualifiedApplyExpr app;
        app.path = head.module_path;
        app.name = head.name;
        app.args = BraceArgs{std::move(fields.elem)};
        return {after, MakeExpr(SpanBetween(parser, after), app)};
      }
      if (after_head && !IsPuncTok(*after_head, "(") &&
          !IsPuncTok(*after_head, "{") &&
          !(after_head->kind == TokenKind::Operator && after_head->lexeme == "@")) {
        SPEC_RULE("Parse-Qualified-Name");
        QualifiedNameExpr qname;
        qname.path = head.module_path;
        qname.name = head.name;
        return {head.parser, MakeExpr(SpanBetween(parser, head.parser), qname)};
      }
    }
  }

  if (tok && IsPuncTok(*tok, "(")) {
    if (!TupleParen(parser)) {
      SPEC_RULE("Parse-Parenthesized-Expr");
      Parser next = parser;
      Advance(next);
      ParseElemResult<ExprPtr> expr = ParseExpr(next);
      if (!IsPunc(expr.parser, ")")) {
        EmitParseSyntaxErr(expr.parser, TokSpan(expr.parser));
        Parser sync = expr.parser;
        SyncStmt(sync);
        return {sync, MakeExpr(SpanBetween(parser, sync), ErrorExpr{})};
      }
      Parser after = expr.parser;
      Advance(after);
      return {after, expr.elem};
    }
    SPEC_RULE("Parse-Tuple-Literal");
    Parser next = parser;
    Advance(next);
    ParseElemResult<std::vector<ExprPtr>> elems = ParseTupleExprElems(next);
    if (!IsPunc(elems.parser, ")")) {
      EmitParseSyntaxErr(elems.parser, TokSpan(elems.parser));
      Parser sync = elems.parser;
      SyncStmt(sync);
      return {sync, MakeExpr(SpanBetween(parser, sync), ErrorExpr{})};
    }
    Parser after = elems.parser;
    Advance(after);
    TupleExpr tup;
    tup.elements = std::move(elems.elem);
    return {after, MakeExpr(SpanBetween(parser, after), tup)};
  }

  if (tok && IsPuncTok(*tok, "[")) {
    SPEC_RULE("Parse-Array-Literal");
    Parser next = parser;
    Advance(next);
    ParseElemResult<std::vector<ExprPtr>> elems = ParseExprList(next);
    if (!IsPunc(elems.parser, "]")) {
      EmitParseSyntaxErr(elems.parser, TokSpan(elems.parser));
      Parser sync = elems.parser;
      SyncStmt(sync);
      return {sync, MakeExpr(SpanBetween(parser, sync), ErrorExpr{})};
    }
    Parser after = elems.parser;
    Advance(after);
    ArrayExpr arr;
    arr.elements = std::move(elems.elem);
    return {after, MakeExpr(SpanBetween(parser, after), arr)};
  }

  if (allow_brace && tok && IsIdentTok(*tok)) {
    Parser start = parser;
    Parser next = parser;
    Advance(next);
    const Token* look = Tok(next);
    if (look && (IsOpTok(*look, "@") || IsPuncTok(*look, "{"))) {
      ParseElemResult<TypePath> path = ParseTypePath(start);
      if (IsOp(path.parser, "@")) {
        SPEC_RULE("Parse-Record-Literal-ModalState");
        Parser after_at = path.parser;
        Advance(after_at);
        ParseElemResult<Identifier> state = ParseIdent(after_at);
        if (!IsPunc(state.parser, "{")) {
          EmitParseSyntaxErr(state.parser, TokSpan(state.parser));
          Parser sync = state.parser;
          SyncStmt(sync);
          return {sync, MakeExpr(SpanBetween(parser, sync), ErrorExpr{})};
        }
        Parser after_l = state.parser;
        Advance(after_l);
        ParseElemResult<std::vector<FieldInit>> fields = ParseFieldInitList(after_l);
        if (!IsPunc(fields.parser, "}")) {
          EmitParseSyntaxErr(fields.parser, TokSpan(fields.parser));
          Parser sync = fields.parser;
          SyncStmt(sync);
          return {sync, MakeExpr(SpanBetween(parser, sync), ErrorExpr{})};
        }
        Parser after = fields.parser;
        Advance(after);
        RecordExpr rec;
        ModalStateRef modal;
        modal.path = path.elem;
        modal.state = state.elem;
        rec.target = modal;
        rec.fields = std::move(fields.elem);
        return {after, MakeExpr(SpanBetween(parser, after), rec)};
      }
      if (IsPunc(path.parser, "{") && path.elem.size() == 1) {
        SPEC_RULE("Parse-Record-Literal");
        Parser after_l = path.parser;
        Advance(after_l);
        ParseElemResult<std::vector<FieldInit>> fields = ParseFieldInitList(after_l);
        if (!IsPunc(fields.parser, "}")) {
          EmitParseSyntaxErr(fields.parser, TokSpan(fields.parser));
          Parser sync = fields.parser;
          SyncStmt(sync);
          return {sync, MakeExpr(SpanBetween(parser, sync), ErrorExpr{})};
        }
        Parser after = fields.parser;
        Advance(after);
        RecordExpr rec;
        rec.target = path.elem;
        rec.fields = std::move(fields.elem);
        return {after, MakeExpr(SpanBetween(parser, after), rec)};
      }
    }
  }

  if (tok && IsKwTok(*tok, "if")) {
    SPEC_RULE("Parse-If-Expr");
    Parser next = parser;
    Advance(next);
    ParseElemResult<ExprPtr> cond = ParseExprNoBrace(next);
    ParseElemResult<std::shared_ptr<Block>> then_block = ParseBlock(cond.parser);
    BlockExpr then_expr;
    then_expr.block = then_block.elem;
    ExprPtr then_node = MakeExpr(SpanBetween(cond.parser, then_block.parser),
                                 then_expr);
    ElseOptResult else_opt = ParseElseOpt(then_block.parser);
    IfExpr ifexpr;
    ifexpr.cond = cond.elem;
    ifexpr.then_expr = then_node;
    ifexpr.else_expr = else_opt.else_opt;
    return {else_opt.parser, MakeExpr(SpanBetween(parser, else_opt.parser), ifexpr)};
  }

  if (tok && IsKwTok(*tok, "match")) {
    SPEC_RULE("Parse-Match-Expr");
    Parser next = parser;
    Advance(next);
    ParseElemResult<ExprPtr> value = ParseExprNoBrace(next);
    if (!IsPunc(value.parser, "{")) {
      EmitParseSyntaxErr(value.parser, TokSpan(value.parser));
      Parser sync = value.parser;
      SyncStmt(sync);
      return {sync, MakeExpr(SpanBetween(parser, sync), ErrorExpr{})};
    }
    Parser after_l = value.parser;
    Advance(after_l);
    MatchArmsResult arms = ParseMatchArms(after_l);
    if (!IsPunc(arms.parser, "}")) {
      EmitParseSyntaxErr(arms.parser, TokSpan(arms.parser));
      Parser sync = arms.parser;
      SyncStmt(sync);
      return {sync, MakeExpr(SpanBetween(parser, sync), ErrorExpr{})};
    }
    Parser after = arms.parser;
    Advance(after);
    MatchExpr match;
    match.value = value.elem;
    match.arms = std::move(arms.arms);
    return {after, MakeExpr(SpanBetween(parser, after), match)};
  }

  if (tok && IsKwTok(*tok, "loop")) {
    SPEC_RULE("Parse-Loop-Expr");
    Parser next = parser;
    Advance(next);
    LoopTailResult tail = ParseLoopTail(next);
    return {tail.parser, tail.loop_expr};
  }

  if (tok && IsPuncTok(*tok, "{")) {
    SPEC_RULE("Parse-Block-Expr");
    ParseElemResult<std::shared_ptr<Block>> block = ParseBlock(parser);
    BlockExpr blk;
    blk.block = block.elem;
    return {block.parser, MakeExpr(SpanBetween(parser, block.parser), blk)};
  }

  if (tok && IsKwTok(*tok, "unsafe")) {
    SPEC_RULE("Parse-Unsafe-Expr");
    Parser next = parser;
    Advance(next);
    ParseElemResult<std::shared_ptr<Block>> block = ParseBlock(next);
    UnsafeBlockExpr unsafe;
    unsafe.block = block.elem;
    return {block.parser, MakeExpr(SpanBetween(parser, block.parser), unsafe)};
  }

  if (tok && IsKwTok(*tok, "transmute")) {
    Parser next = parser;
    Advance(next);
    if (!IsOp(next, "::")) {
      EmitParseSyntaxErr(next, TokSpan(next));
      Parser sync = next;
      SyncStmt(sync);
      return {sync, MakeExpr(SpanBetween(parser, sync), ErrorExpr{})};
    }
    Parser after_colon = next;
    Advance(after_colon);
    if (!IsOp(after_colon, "<")) {
      EmitParseSyntaxErr(after_colon, TokSpan(after_colon));
      Parser sync = after_colon;
      SyncStmt(sync);
      return {sync, MakeExpr(SpanBetween(parser, sync), ErrorExpr{})};
    }
    Parser after_lt = after_colon;
    Advance(after_lt);
    ParseElemResult<std::shared_ptr<Type>> t1 = ParseType(after_lt);
    if (!IsPunc(t1.parser, ",")) {
      EmitParseSyntaxErr(t1.parser, TokSpan(t1.parser));
      Parser sync = t1.parser;
      SyncStmt(sync);
      return {sync, MakeExpr(SpanBetween(parser, sync), ErrorExpr{})};
    }
    Parser after_comma = t1.parser;
    Advance(after_comma);
    ParseElemResult<std::shared_ptr<Type>> t2 = ParseType(after_comma);
    if (IsOp(t2.parser, ">>")) {
      SPEC_RULE("Parse-Transmute-Expr-ShiftSplit");
      Parser split = SplitShiftR(t2.parser);
      Parser at_second = split;
      Advance(at_second);
      if (!IsOp(at_second, ">")) {
        EmitParseSyntaxErr(at_second, TokSpan(at_second));
        Parser sync = at_second;
        SyncStmt(sync);
        return {sync, MakeExpr(SpanBetween(parser, sync), ErrorExpr{})};
      }
      Parser after_gt = at_second;
      Advance(after_gt);
      if (!IsPunc(after_gt, "(")) {
        EmitParseSyntaxErr(after_gt, TokSpan(after_gt));
        Parser sync = after_gt;
        SyncStmt(sync);
        return {sync, MakeExpr(SpanBetween(parser, sync), ErrorExpr{})};
      }
      Parser after_l = after_gt;
      Advance(after_l);
      ParseElemResult<ExprPtr> expr = ParseExpr(after_l);
      if (!IsPunc(expr.parser, ")")) {
        EmitParseSyntaxErr(expr.parser, TokSpan(expr.parser));
        Parser sync = expr.parser;
        SyncStmt(sync);
        return {sync, MakeExpr(SpanBetween(parser, sync), ErrorExpr{})};
      }
      Parser after = expr.parser;
      Advance(after);
      TransmuteExpr trans;
      trans.from = t1.elem;
      trans.to = t2.elem;
      trans.value = expr.elem;
      return {after, MakeExpr(SpanBetween(parser, after), trans)};
    }
    if (IsOp(t2.parser, ">")) {
      SPEC_RULE("Parse-Transmute-Expr");
      Parser after_gt = t2.parser;
      Advance(after_gt);
      if (!IsPunc(after_gt, "(")) {
        EmitParseSyntaxErr(after_gt, TokSpan(after_gt));
        Parser sync = after_gt;
        SyncStmt(sync);
        return {sync, MakeExpr(SpanBetween(parser, sync), ErrorExpr{})};
      }
      Parser after_l = after_gt;
      Advance(after_l);
      ParseElemResult<ExprPtr> expr = ParseExpr(after_l);
      if (!IsPunc(expr.parser, ")")) {
        EmitParseSyntaxErr(expr.parser, TokSpan(expr.parser));
        Parser sync = expr.parser;
        SyncStmt(sync);
        return {sync, MakeExpr(SpanBetween(parser, sync), ErrorExpr{})};
      }
      Parser after = expr.parser;
      Advance(after);
      TransmuteExpr trans;
      trans.from = t1.elem;
      trans.to = t2.elem;
      trans.value = expr.elem;
      return {after, MakeExpr(SpanBetween(parser, after), trans)};
    }
    EmitParseSyntaxErr(t2.parser, TokSpan(t2.parser));
    Parser sync = t2.parser;
    SyncStmt(sync);
    return {sync, MakeExpr(SpanBetween(parser, sync), ErrorExpr{})};
  }

  SPEC_RULE("Parse-Primary-Err");
  Parser next = AdvanceOrEOF(parser);
  EmitParseSyntaxErr(next, TokSpan(parser));
  SyncStmt(next);
  return {next, MakeExpr(SpanBetween(parser, next), ErrorExpr{})};
}

ParseElemResult<std::vector<Arg>> ParseArgList(Parser parser) {
  if (IsPunc(parser, ")")) {
    SPEC_RULE("Parse-ArgList-Empty");
    return {parser, {}};
  }
  SPEC_RULE("Parse-ArgList-Cons");
  ParseElemResult<Arg> first = ParseArg(parser);
  std::vector<Arg> args;
  args.push_back(first.elem);
  return ParseArgTail(first.parser, std::move(args));
}

ParseElemResult<Arg> ParseArg(Parser parser) {
  SPEC_RULE("Parse-Arg");
  Parser start = parser;
  ParseElemResult<bool> moved = ParseArgMoveOpt(parser);
  ParseElemResult<ExprPtr> expr = ParseExpr(moved.parser);
  Arg arg;
  arg.moved = moved.elem;
  arg.value = expr.elem;
  arg.span = SpanBetween(start, expr.parser);
  return {expr.parser, arg};
}

ParseElemResult<bool> ParseArgMoveOpt(Parser parser) {
  if (!IsKw(parser, "move")) {
    SPEC_RULE("Parse-ArgMoveOpt-None");
    return {parser, false};
  }
  SPEC_RULE("Parse-ArgMoveOpt-Yes");
  Parser next = parser;
  Advance(next);
  return {next, true};
}

ParseElemResult<std::vector<Arg>> ParseArgTail(Parser parser,
                                               std::vector<Arg> xs) {
  if (IsPunc(parser, ")")) {
    SPEC_RULE("Parse-ArgTail-End");
    return {parser, xs};
  }
  if (IsPunc(parser, ",")) {
    Parser after = parser;
    Advance(after);
    if (IsPunc(after, ")")) {
      SPEC_RULE("Parse-ArgTail-TrailingComma");
      EmitUnsupportedConstruct(after);
      return {after, xs};
    }
    SPEC_RULE("Parse-ArgTail-Comma");
    ParseElemResult<Arg> arg = ParseArg(after);
    xs.push_back(arg.elem);
    return ParseArgTail(arg.parser, std::move(xs));
  }
  EmitParseSyntaxErr(parser, TokSpan(parser));
  return {parser, xs};
}

ParseElemResult<std::vector<ExprPtr>> ParseExprList(Parser parser) {
  SPEC_RULE("List-Start");
  if (IsPunc(parser, ")") || IsPunc(parser, "]")) {
    SPEC_RULE("Parse-ExprList-Empty");
    SPEC_RULE("List-Done");
    EmitUnsupportedConstruct(parser);
    return {parser, {}};
  }
  SPEC_RULE("Parse-ExprList-Cons");
  SPEC_RULE("List-Cons");
  ParseElemResult<ExprPtr> first = ParseExpr(parser);
  std::vector<ExprPtr> elems;
  elems.push_back(first.elem);
  return ParseExprListTail(first.parser, std::move(elems));
}

ParseElemResult<std::vector<ExprPtr>> ParseExprListTail(Parser parser,
                                                        std::vector<ExprPtr> xs) {
  if (IsPunc(parser, ")") || IsPunc(parser, "]") || IsPunc(parser, "}")) {
    SPEC_RULE("Parse-ExprListTail-End");
    SPEC_RULE("List-Done");
    return {parser, xs};
  }
  if (IsPunc(parser, ",")) {
    Parser after = parser;
    Advance(after);
    if (IsPunc(after, ")") || IsPunc(after, "]") || IsPunc(after, "}")) {
      SPEC_RULE("Parse-ExprListTail-TrailingComma");
      EmitUnsupportedConstruct(after);
      return {after, xs};
    }
    SPEC_RULE("Parse-ExprListTail-Comma");
    ParseElemResult<ExprPtr> elem = ParseExpr(after);
    SPEC_RULE("List-Cons");
    xs.push_back(elem.elem);
    return ParseExprListTail(elem.parser, std::move(xs));
  }
  EmitParseSyntaxErr(parser, TokSpan(parser));
  return {parser, xs};
}

ParseElemResult<std::vector<ExprPtr>> ParseTupleExprElems(Parser parser) {
  if (IsPunc(parser, ")")) {
    SPEC_RULE("Parse-TupleExprElems-Empty");
    return {parser, {}};
  }
  ParseElemResult<ExprPtr> first = ParseExpr(parser);
  if (IsPunc(first.parser, ";")) {
    SPEC_RULE("Parse-TupleExprElems-Single");
    Parser after = first.parser;
    Advance(after);
    std::vector<ExprPtr> elems;
    elems.push_back(first.elem);
    return {after, elems};
  }
  if (IsPunc(first.parser, ",")) {
    Parser after = first.parser;
    Advance(after);
    if (IsPunc(after, ")")) {
      SPEC_RULE("Parse-TupleExprElems-TrailingComma");
      EmitUnsupportedConstruct(after);
      std::vector<ExprPtr> elems;
      elems.push_back(first.elem);
      return {after, elems};
    }
    SPEC_RULE("Parse-TupleExprElems-Many");
    ParseElemResult<ExprPtr> second = ParseExpr(after);
    std::vector<ExprPtr> rest;
    rest.push_back(second.elem);
    ParseElemResult<std::vector<ExprPtr>> tail =
        ParseExprListTail(second.parser, std::move(rest));
    std::vector<ExprPtr> elems;
    elems.reserve(1 + tail.elem.size());
    elems.push_back(first.elem);
    elems.insert(elems.end(), tail.elem.begin(), tail.elem.end());
    return {tail.parser, elems};
  }
  EmitParseSyntaxErr(first.parser, TokSpan(first.parser));
  return {first.parser, {}};
}

ParseElemResult<std::vector<FieldInit>> ParseFieldInitList(Parser parser) {
  if (IsPunc(parser, "}")) {
    SPEC_RULE("Parse-FieldInitList-Empty");
    EmitUnsupportedConstruct(parser);
    return {parser, {}};
  }
  SPEC_RULE("Parse-FieldInitList-Cons");
  ParseElemResult<FieldInit> first = ParseFieldInit(parser);
  std::vector<FieldInit> fields;
  fields.push_back(first.elem);
  return ParseFieldInitTail(first.parser, std::move(fields));
}

ParseElemResult<FieldInit> ParseFieldInit(Parser parser) {
  Parser start = parser;
  ParseElemResult<Identifier> name = ParseIdent(parser);
  if (IsPunc(name.parser, ":")) {
    SPEC_RULE("Parse-FieldInit-Explicit");
    Parser after_colon = name.parser;
    Advance(after_colon);
    ParseElemResult<ExprPtr> expr = ParseExpr(after_colon);
    FieldInit init;
    init.name = name.elem;
    init.value = expr.elem;
    init.span = SpanBetween(start, expr.parser);
    return {expr.parser, init};
  }
  SPEC_RULE("Parse-FieldInit-Shorthand");
  IdentifierExpr ident;
  ident.name = name.elem;
  ExprPtr value = MakeExpr(SpanBetween(start, name.parser), ident);
  FieldInit init;
  init.name = name.elem;
  init.value = value;
  init.span = SpanBetween(start, name.parser);
  return {name.parser, init};
}

ParseElemResult<std::vector<FieldInit>> ParseFieldInitTail(Parser parser,
                                                           std::vector<FieldInit> xs) {
  if (IsPunc(parser, "}")) {
    SPEC_RULE("Parse-FieldInitTail-End");
    return {parser, xs};
  }
  if (IsPunc(parser, ",")) {
    Parser after = parser;
    Advance(after);
    if (IsPunc(after, "}")) {
      SPEC_RULE("Parse-FieldInitTail-TrailingComma");
      EmitUnsupportedConstruct(after);
      return {after, xs};
    }
    SPEC_RULE("Parse-FieldInitTail-Comma");
    ParseElemResult<FieldInit> field = ParseFieldInit(after);
    xs.push_back(field.elem);
    return ParseFieldInitTail(field.parser, std::move(xs));
  }
  EmitParseSyntaxErr(parser, TokSpan(parser));
  return {parser, xs};
}

MatchArmsResult ParseMatchArms(Parser parser) {
  SPEC_RULE("Parse-MatchArms-Cons");
  SPEC_RULE("Parse-MatchArm");
  ParseElemResult<std::shared_ptr<Pattern>> pat = ParsePattern(parser);
  GuardOptResult guard = ParseGuardOpt(pat.parser);
  if (!IsOp(guard.parser, "=>")) {
    EmitParseSyntaxErr(guard.parser, TokSpan(guard.parser));
    Parser sync = guard.parser;
    SyncStmt(sync);
    return {sync, {}};
  }
  Parser after_arrow = guard.parser;
  Advance(after_arrow);
  ArmBodyResult body = ParseArmBody(after_arrow);
  MatchArm arm;
  arm.pattern = pat.elem;
  arm.guard_opt = guard.guard_opt;
  arm.body = body.body;
  arm.span = SpanBetween(parser, body.parser);
  std::vector<MatchArm> arms;
  arms.push_back(arm);
  return ParseMatchArmsTail(body.parser, std::move(arms));
}

MatchArmsResult ParseMatchArmsTail(Parser parser, std::vector<MatchArm> xs) {
  if (IsPunc(parser, "}")) {
    SPEC_RULE("Parse-MatchArmsTail-End");
    return {parser, xs};
  }
  if (IsPunc(parser, ",")) {
    const TokenKindMatch end_set[] = {MatchPunct("}")};
    if (EmitTrailingCommaErr(parser, end_set)) {
      Parser after = parser;
      Advance(after);
      return {after, xs};
    }
    SPEC_RULE("Parse-MatchArmsTail-Comma");
    SPEC_RULE("Parse-MatchArm");
    Parser after = parser;
    Advance(after);
    ParseElemResult<std::shared_ptr<Pattern>> pat = ParsePattern(after);
    GuardOptResult guard = ParseGuardOpt(pat.parser);
    if (!IsOp(guard.parser, "=>")) {
      EmitParseSyntaxErr(guard.parser, TokSpan(guard.parser));
      Parser sync = guard.parser;
      SyncStmt(sync);
      return {sync, xs};
    }
    Parser after_arrow = guard.parser;
    Advance(after_arrow);
    ArmBodyResult body = ParseArmBody(after_arrow);
    MatchArm arm;
    arm.pattern = pat.elem;
    arm.guard_opt = guard.guard_opt;
    arm.body = body.body;
    arm.span = SpanBetween(after, body.parser);
    xs.push_back(arm);
    return ParseMatchArmsTail(body.parser, std::move(xs));
  }
  EmitParseSyntaxErr(parser, TokSpan(parser));
  return {parser, xs};
}

GuardOptResult ParseGuardOpt(Parser parser) {
  if (!IsKw(parser, "if")) {
    SPEC_RULE("Parse-GuardOpt-None");
    return {parser, nullptr};
  }
  SPEC_RULE("Parse-GuardOpt-Yes");
  Parser next = parser;
  Advance(next);
  ParseElemResult<ExprPtr> guard = ParseExpr(next);
  return {guard.parser, guard.elem};
}

ArmBodyResult ParseArmBody(Parser parser) {
  if (IsPunc(parser, "{")) {
    SPEC_RULE("Parse-ArmBody-Block");
    ParseElemResult<std::shared_ptr<Block>> block = ParseBlock(parser);
    BlockExpr blk;
    blk.block = block.elem;
    return {block.parser, MakeExpr(SpanBetween(parser, block.parser), blk)};
  }
  SPEC_RULE("Parse-ArmBody-Expr");
  ParseElemResult<ExprPtr> expr = ParseExpr(parser);
  return {expr.parser, expr.elem};
}

TryPatternInResult TryParsePatternIn(Parser parser) {
  Parser clone = Clone(parser);
  ParseElemResult<std::shared_ptr<Pattern>> pat = ParsePattern(clone);
  ParseElemResult<std::shared_ptr<Type>> ty = ParseTypeAnnotOpt(pat.parser);
  const Token* tok = Tok(ty.parser);
  if (tok && IsIdentTok(*tok) && tok->lexeme == "in") {
    SPEC_RULE("TryParsePatternIn-Ok");
    Parser merged = MergeDiag(parser, ty.parser, pat.parser);
    TryPatternInResult out;
    out.parser = merged;
    out.pattern = pat.elem;
    out.ok = true;
    return out;
  }
  SPEC_RULE("TryParsePatternIn-Fail");
  TryPatternInResult out;
  out.parser = parser;
  out.pattern = nullptr;
  out.ok = false;
  return out;
}

LoopTailResult ParseLoopTail(Parser parser) {
  if (IsPunc(parser, "{")) {
    SPEC_RULE("Parse-LoopTail-Infinite");
    ParseElemResult<std::shared_ptr<Block>> body = ParseBlock(parser);
    LoopInfiniteExpr loop;
    loop.body = body.elem;
    return {body.parser, MakeExpr(SpanBetween(parser, body.parser), loop)};
  }
  TryPatternInResult try_in = TryParsePatternIn(parser);
  if (try_in.ok) {
    SPEC_RULE("Parse-LoopTail-Iter");
    ParseElemResult<std::shared_ptr<Type>> ty = ParseTypeAnnotOpt(try_in.parser);
    const Token* tok = Tok(ty.parser);
    if (!tok || !IsIdentTok(*tok) || tok->lexeme != "in") {
      EmitParseSyntaxErr(ty.parser, TokSpan(ty.parser));
      Parser sync = ty.parser;
      SyncStmt(sync);
      return {sync, MakeExpr(SpanBetween(parser, sync), ErrorExpr{})};
    }
    Parser after_in = ty.parser;
    Advance(after_in);
    ParseElemResult<ExprPtr> iter = ParseExprNoBrace(after_in);
    ParseElemResult<std::shared_ptr<Block>> body = ParseBlock(iter.parser);
    auto pattern = try_in.pattern;
    auto type_opt = ty.elem;
    NormalizeBindingPattern(pattern, type_opt);
    LoopIterExpr loop;
    loop.pattern = pattern;
    loop.type_opt = type_opt;
    loop.iter = iter.elem;
    loop.body = body.elem;
    return {body.parser, MakeExpr(SpanBetween(parser, body.parser), loop)};
  }
  SPEC_RULE("Parse-LoopTail-Cond");
  ParseElemResult<ExprPtr> cond = ParseExprNoBrace(parser);
  ParseElemResult<std::shared_ptr<Block>> body = ParseBlock(cond.parser);
  LoopConditionalExpr loop;
  loop.cond = cond.elem;
  loop.body = body.elem;
  return {body.parser, MakeExpr(SpanBetween(parser, body.parser), loop)};
}

ElseOptResult ParseElseOpt(Parser parser) {
  if (!IsKw(parser, "else")) {
    SPEC_RULE("Parse-ElseOpt-None");
    return {parser, nullptr};
  }
  Parser next = parser;
  Advance(next);
  if (IsKw(next, "if")) {
    SPEC_RULE("Parse-ElseOpt-If");
    ParseElemResult<ExprPtr> expr = ParsePrimary(next, true);
    return {expr.parser, expr.elem};
  }
  SPEC_RULE("Parse-ElseOpt-Block");
  ParseElemResult<std::shared_ptr<Block>> block = ParseBlock(next);
  BlockExpr blk;
  blk.block = block.elem;
  return {block.parser, MakeExpr(SpanBetween(parser, block.parser), blk)};
}

ParseElemResult<ExprPtr> ParsePlace(Parser parser, bool allow_brace) {
  if (IsOp(parser, "*")) {
    SPEC_RULE("Parse-Place-Deref");
    Parser next = parser;
    Advance(next);
    ParseElemResult<ExprPtr> inner = ParsePlace(next, allow_brace);
    DerefExpr deref;
    deref.value = inner.elem;
    return {inner.parser,
            MakeExpr(SpanCover(TokSpan(parser), inner.elem->span), deref)};
  }
  ParseElemResult<ExprPtr> expr = ParsePostfix(parser, allow_brace);
  if (IsPlace(expr.elem)) {
    SPEC_RULE("Parse-Place-Postfix");
    return expr;
  }
  SPEC_RULE("Parse-Place-Err");
  EmitParseSyntaxErr(expr.parser, TokSpan(parser));
  Parser sync = expr.parser;
  SyncStmt(sync);
  return {sync, MakeExpr(SpanBetween(parser, sync), ErrorExpr{})};
}

}  // namespace

ParseElemResult<ExprPtr> ParseExpr(Parser parser) {
  SPEC_RULE("Parse-Expr");
  return ParseRange(parser, true);
}

ParseElemResult<ExprPtr> ParseExprOpt(Parser parser) {
  const Token* tok = Tok(parser);
  if (!tok || tok->kind == TokenKind::Newline ||
      (tok->kind == TokenKind::Punctuator &&
       (tok->lexeme == ";" || tok->lexeme == "}")) ||
      AtEof(parser)) {
    SPEC_RULE("Parse-ExprOpt-None");
    return {parser, nullptr};
  }
  SPEC_RULE("Parse-ExprOpt-Yes");
  return ParseExpr(parser);
}

}  // namespace cursive0::syntax
