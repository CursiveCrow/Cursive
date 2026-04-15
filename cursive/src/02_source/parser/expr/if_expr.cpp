// =============================================================================
// if_expr.cpp - If expression parsing
// =============================================================================

#include "02_source/parser/parser.h"

#include <memory>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

#include "00_core/assert_spec.h"
#include "00_core/span.h"
#include "02_source/ast/ast.h"
#include "02_source/lexer/keyword_policy.h"

namespace cursive::ast {

using cursive::lexer::IsIdentTok;
using cursive::lexer::Token;
using cursive::lexer::TokenKind;

ExprPtr MakeExpr(const core::Span& span, ExprNode node);
PatternPtr MakePattern(const core::Span& span, PatternNode node);
bool IsKw(const Parser& parser, std::string_view kw);
bool IsOp(const Parser& parser, std::string_view op);
bool IsPunc(const Parser& parser, std::string_view punc);
ParseElemResult<Identifier> ParseIdent(Parser parser);
ParseQualifiedHeadResult ParseQualifiedHead(Parser parser);
ParseElemResult<ExprPtr> ParseExprNoBrace(Parser parser);
ParseElemResult<std::shared_ptr<Block>> ParseBlock(Parser parser);
ParseElemResult<std::shared_ptr<Pattern>> ParsePattern(Parser parser);
ParseElemResult<ExprPtr> ParsePrimary(Parser parser, bool allow_brace);

namespace {

ExprPtr WrapBlockExpr(Parser start, const ParseElemResult<std::shared_ptr<Block>>& block) {
  BlockExpr blk;
  blk.block = block.elem;
  return MakeExpr(SpanBetween(start, block.parser), blk);
}

void SkipCaseListNewlines(Parser& parser) {
  while (const Token* tok = Tok(parser)) {
    if (tok->kind != TokenKind::Newline) {
      break;
    }
    Advance(parser);
  }
}

struct ElseOptResult {
  Parser parser;
  ExprPtr else_opt;
};

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
  return {block.parser, WrapBlockExpr(next, block)};
}

struct IfCaseListResult {
  Parser parser;
  std::vector<IfCaseClause> cases;
  ExprPtr else_opt;
};

ParseElemResult<ExprPtr> ParseCaseBodyBlock(Parser parser) {
  ParseElemResult<std::shared_ptr<Block>> block = ParseBlock(parser);
  return {block.parser, WrapBlockExpr(parser, block)};
}

std::optional<ParseElemResult<PatternPtr>>
TryParseBraceDisambiguatedIfCasePattern(Parser parser) {
  const Token* tok = Tok(parser);
  if (!tok) {
    return std::nullopt;
  }

  if (IsOp(parser, "@")) {
    SPEC_RULE("Parse-IfCase-Pattern-ModalHead");
    Parser start = parser;
    Parser next = parser;
    Advance(next);
    ParseElemResult<Identifier> name = ParseIdent(next);
    ModalPattern pat;
    pat.state = name.elem;
    pat.fields_opt = std::nullopt;
    return ParseElemResult<PatternPtr>{
        name.parser,
        MakePattern(SpanBetween(start, name.parser), pat)};
  }

  if (!IsIdentTok(*tok)) {
    return std::nullopt;
  }

  Parser next = parser;
  Advance(next);
  if (!IsOp(next, "::")) {
    return std::nullopt;
  }

  SPEC_RULE("Parse-IfCase-Pattern-EnumHead");
  Parser start = parser;
  ParseQualifiedHeadResult head = ParseQualifiedHead(parser);
  EnumPattern pat;
  pat.path = head.module_path;
  pat.name = head.name;
  pat.payload_opt = std::nullopt;
  return ParseElemResult<PatternPtr>{
      head.parser,
      MakePattern(SpanBetween(start, head.parser), pat)};
}

ParseElemResult<IfCaseClause> ParseIfCaseClause(Parser parser) {
  Parser speculative = Clone(parser);
  ParseElemResult<PatternPtr> full_pattern = ParsePattern(speculative);
  if (IsPunc(full_pattern.parser, "{")) {
    ParseElemResult<ExprPtr> body = ParseCaseBodyBlock(full_pattern.parser);
    IfCaseClause clause;
    clause.pattern = full_pattern.elem;
    clause.body = body.elem;
    return {body.parser, std::move(clause)};
  }

  if (std::optional<ParseElemResult<PatternPtr>> fallback =
          TryParseBraceDisambiguatedIfCasePattern(Clone(parser));
      fallback.has_value() && IsPunc(fallback->parser, "{")) {
    ParseElemResult<ExprPtr> body = ParseCaseBodyBlock(fallback->parser);
    IfCaseClause clause;
    clause.pattern = fallback->elem;
    clause.body = body.elem;
    return {body.parser, std::move(clause)};
  }

  EmitParseSyntaxErr(full_pattern.parser, TokSpan(full_pattern.parser));
  return {full_pattern.parser, IfCaseClause{}};
}

IfCaseListResult ParseIfCaseList(Parser parser) {
  Parser cur = parser;
  std::vector<IfCaseClause> cases;
  ExprPtr else_opt;

  SkipCaseListNewlines(cur);
  while (!AtEof(cur) && !IsPunc(cur, "}")) {
    if (IsKw(cur, "else")) {
      if (cases.empty()) {
        EmitParseSyntaxErr(cur, TokSpan(cur));
      }
      Parser after_else = cur;
      Advance(after_else);
      ParseElemResult<std::shared_ptr<Block>> else_block = ParseBlock(after_else);
      else_opt = WrapBlockExpr(after_else, else_block);
      cur = else_block.parser;
      SkipCaseListNewlines(cur);
      break;
    }

    SPEC_RULE("Parse-IfCase");
    ParseElemResult<IfCaseClause> clause = ParseIfCaseClause(cur);
    if (clause.parser.index <= cur.index) {
      Parser sync = cur;
      SyncStmt(sync);
      if (sync.index <= cur.index) {
        break;
      }
      cur = sync;
      SkipCaseListNewlines(cur);
      continue;
    }
    cases.push_back(std::move(clause.elem));

    cur = clause.parser;
    SkipCaseListNewlines(cur);
  }

  return {cur, std::move(cases), else_opt};
}

}  // namespace

ParseElemResult<ExprPtr> ParseIfExpr(Parser parser) {
  SPEC_RULE("Parse-If-Expr");
  Parser start = parser;
  Parser next = parser;
  Advance(next);  // consume "if"

  ParseElemResult<ExprPtr> first = ParseExprNoBrace(next);

  if (IsKw(first.parser, "is")) {
    Parser after_is = first.parser;
    Advance(after_is);  // consume "is"

    if (IsPunc(after_is, "{")) {
      SPEC_RULE("Parse-If-Is-CaseList");
      Parser after_lbrace = after_is;
      Advance(after_lbrace);
      IfCaseListResult case_list = ParseIfCaseList(after_lbrace);
      if (!IsPunc(case_list.parser, "}")) {
        EmitParseSyntaxErr(case_list.parser, TokSpan(case_list.parser));
        Parser sync = case_list.parser;
        SyncStmt(sync);
        return {sync, MakeExpr(SpanBetween(start, sync), ErrorExpr{})};
      }
      Parser after_rbrace = case_list.parser;
      Advance(after_rbrace);

      if (case_list.cases.empty()) {
        EmitParseSyntaxErr(after_is, TokSpan(after_is));
        Parser sync = after_rbrace;
        SyncStmt(sync);
        return {sync, MakeExpr(SpanBetween(start, sync), ErrorExpr{})};
      }

      IfCaseExpr if_case;
      if_case.scrutinee = first.elem;
      if_case.cases = std::move(case_list.cases);
      if_case.else_expr = case_list.else_opt;

      return {after_rbrace, MakeExpr(SpanBetween(start, after_rbrace), if_case)};
    }

    SPEC_RULE("Parse-If-Is-Single");
    ParseElemResult<IfCaseClause> clause = ParseIfCaseClause(after_is);
    ElseOptResult else_opt = ParseElseOpt(clause.parser);

    IfIsExpr if_is;
    if_is.scrutinee = first.elem;
    if_is.pattern = clause.elem.pattern;
    if_is.then_expr = clause.elem.body;
    if_is.else_expr = else_opt.else_opt;
    return {else_opt.parser, MakeExpr(SpanBetween(start, else_opt.parser), if_is)};
  }

  ParseElemResult<std::shared_ptr<Block>> then_block = ParseBlock(first.parser);
  ExprPtr then_node = WrapBlockExpr(first.parser, then_block);
  ElseOptResult else_opt = ParseElseOpt(then_block.parser);

  IfExpr ifexpr;
  ifexpr.cond = first.elem;
  ifexpr.then_expr = then_node;
  ifexpr.else_expr = else_opt.else_opt;

  return {else_opt.parser, MakeExpr(SpanBetween(start, else_opt.parser), ifexpr)};
}

}  // namespace cursive::ast
