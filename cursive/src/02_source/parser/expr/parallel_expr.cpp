// =============================================================================
// parallel_expr.cpp - Parallel Expression Parsing
// =============================================================================
//
// SPEC REFERENCE: CursiveSpecification.md, Section 3.3.8.6, Lines 5264-5267
// HELPER RULES: Section 3.3.8.7, Lines 5594-5639
//
// FORMAL RULE - Parse-Parallel-Expr (Lines 5264-5267):
// -----------------------------------------------------------------------------
// IsKw(Tok(P), `parallel`)
// Gamma |- ParseExpr_NoBrace(Advance(P)) => (P_1, domain)
// Gamma |- ParseParallelOptsOpt(P_1) => (P_2, opts)
// Gamma |- ParseBlock(P_2) => (P_3, body)
// ────────────────────────────────────────────────────────────────────────────
// Gamma |- ParsePrimary(P) => (P_3, ParallelExpr(domain, opts, body))
//
// SEMANTICS:
// - `parallel domain_expr [options]? { body }`
// - domain_expr: Expression yielding an ExecutionDomain ($cpu(), $gpu(), etc.)
// - Options: `cancel: token_expr`, `name: "string"`
// - Body: Block containing spawn/dispatch statements
// - All spawned work must complete before parallel block exits (fork-join)
//
// =============================================================================

#include "02_source/parser/parser.h"

#include <memory>
#include <vector>

#include "00_core/assert_spec.h"
#include "00_core/span.h"
#include "02_source/ast/ast.h"
#include "02_source/lexer/keyword_policy.h"

namespace cursive::ast {

// Use lexer types
using cursive::lexer::Token;
using cursive::lexer::TokenKind;

// Forward declarations from other parser modules
ExprPtr MakeExpr(const core::Span& span, ExprNode node);
core::Span SpanCover(const core::Span& start, const core::Span& end);
bool IsPunc(const Parser& parser, std::string_view punc);
void SkipNewlines(Parser& parser);
ParseElemResult<ExprPtr> ParseExpr(Parser parser);
ParseElemResult<ExprPtr> ParseExprNoBrace(Parser parser);
ParseElemResult<std::shared_ptr<Block>> ParseBlock(Parser parser);

// =============================================================================
// ParseParallelExpr - Parse parallel expression
// =============================================================================
//
// SPEC: Lines 5264-5267
// parallel domain_expr [options]? { body }
// Options: cancel, name

ParseElemResult<ExprPtr> ParseParallelExpr(Parser parser) {
  SPEC_RULE("Parse-Parallel-Expr");
  Parser next = parser;
  Advance(next);

  // Parse domain expression.
  ParseElemResult<ExprPtr> domain = ParseExprNoBrace(next);

  // Parse optional [options]
  std::vector<ParallelOption> opts;
  Parser after_opts = domain.parser;
  if (IsPunc(after_opts, "[")) {
    Advance(after_opts);
    SkipNewlines(after_opts);
    if (IsPunc(after_opts, "]")) {
      SPEC_RULE("Parse-ParallelOptList-Empty");
      EmitParseSyntaxErr(after_opts, TokSpan(after_opts));
      Advance(after_opts);
    } else {
      // Parse option list
      while (Tok(after_opts) && !IsPunc(after_opts, "]")) {
        SkipNewlines(after_opts);
        if (IsPunc(after_opts, "]")) {
          break;
        }

        const Token* opt_tok = Tok(after_opts);
        if (!opt_tok || (opt_tok->kind != TokenKind::Identifier &&
                         opt_tok->kind != TokenKind::Keyword)) {
          EmitParseSyntaxErr(after_opts, TokSpan(after_opts));
          break;
        }
        core::Span opt_start = TokSpan(after_opts);
        ParallelOption opt;
        if (opt_tok->lexeme == "cancel") {
          opt.kind = ParallelOptionKind::Cancel;
        } else {
          EmitParseSyntaxErr(after_opts, TokSpan(after_opts));
          break;
        }
        Advance(after_opts);
        if (!IsPunc(after_opts, ":")) {
          EmitParseSyntaxErr(after_opts, TokSpan(after_opts));
          break;
        }
        Advance(after_opts);

        ParseElemResult<ExprPtr> opt_val = ParseExpr(after_opts);
        opt.value = opt_val.elem;
        opt.span = SpanCover(opt_start, TokSpan(opt_val.parser));
        opts.push_back(opt);
        after_opts = opt_val.parser;
        SkipNewlines(after_opts);
        if (IsPunc(after_opts, ",")) {
          const TokenKindMatch end_set[] = {MatchPunct("]")};
          Parser after_comma = after_opts;
          Advance(after_comma);
          SkipNewlines(after_comma);
          if (IsPunc(after_comma, "]")) {
            SPEC_RULE("Parse-ParallelOptListTail-TrailingComma");
            EmitTrailingCommaErr(after_opts, end_set);
            after_comma.diags = after_opts.diags;
            after_opts = after_comma;
            break;
          }
          after_opts = after_comma;
          continue;
        }
        break;
      }
      SkipNewlines(after_opts);
      if (IsPunc(after_opts, "]")) {
        Advance(after_opts);
      } else {
        EmitParseSyntaxErr(after_opts, TokSpan(after_opts));
        while (Tok(after_opts) && !IsPunc(after_opts, "]") &&
               !IsPunc(after_opts, "{")) {
          Advance(after_opts);
        }
        if (IsPunc(after_opts, "]")) {
          Advance(after_opts);
        }
      }
    }
  }

  // Parse block body
  ParseElemResult<std::shared_ptr<Block>> body = ParseBlock(after_opts);
  ParallelExpr par;
  par.domain = domain.elem;
  par.opts = std::move(opts);
  par.body = body.elem;
  return {body.parser, MakeExpr(SpanBetween(parser, body.parser), par)};
}

}  // namespace cursive::ast
