#include "cursive0/syntax/parser.h"

#include <iterator>
#include <string_view>
#include <utility>
#include <vector>

#include "cursive0/core/assert_spec.h"
#include "cursive0/core/diagnostic_messages.h"
#include "cursive0/core/diagnostics.h"
#include "cursive0/core/span.h"
#include "cursive0/syntax/keyword_policy.h"

namespace cursive0::syntax {

namespace {
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

bool IsTerminatorToken(const Token& tok) {
  return tok.kind == TokenKind::Newline ||
         (tok.kind == TokenKind::Punctuator && tok.lexeme == ";");
}

bool IsAssignOp(const Token& tok) {
  if (tok.kind != TokenKind::Operator) {
    return false;
  }
  return tok.lexeme == "=" || tok.lexeme == "+=" || tok.lexeme == "-=" ||
         tok.lexeme == "*=" || tok.lexeme == "/=" || tok.lexeme == "%=";
}

bool IsCompoundAssignOp(const Token& tok) {
  if (tok.kind != TokenKind::Operator) {
    return false;
  }
  return tok.lexeme == "+=" || tok.lexeme == "-=" || tok.lexeme == "*=" ||
         tok.lexeme == "/=" || tok.lexeme == "%=";
}

bool IsLiteralToken(const Token& tok) {
  return tok.kind == TokenKind::IntLiteral ||
         tok.kind == TokenKind::FloatLiteral ||
         tok.kind == TokenKind::StringLiteral ||
         tok.kind == TokenKind::CharLiteral ||
         tok.kind == TokenKind::BoolLiteral ||
         tok.kind == TokenKind::NullLiteral;
}

bool IsExprStartToken(const Token& tok) {
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
           tok.lexeme == "widen" || tok.lexeme == "comptime" ||
           // C0X Extension: Structured Concurrency (§18)
           tok.lexeme == "parallel" || tok.lexeme == "spawn" ||
           tok.lexeme == "dispatch" ||
           // C0X Extension: Async expressions (§19)
           tok.lexeme == "yield" || tok.lexeme == "sync" ||
           tok.lexeme == "race" || tok.lexeme == "all";
  }
  return false;
}

bool IsPlaceExpr(const ExprPtr& expr) {
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
    return IsPlaceExpr(deref->value);
  }
  return false;
}

void EmitMissingTerminator(core::DiagnosticStream& diags,
                           const core::Span& span) {
  SPEC_RULE("Missing-Terminator-Err");
  auto diag = core::MakeDiagnostic("E-SRC-0510", span);
  if (!diag) {
    return;
  }
  diags = core::Emit(diags, *diag);
}

bool EndsWithBlock(const ExprPtr& expr) {
  if (!expr) {
    return false;
  }
  if (std::holds_alternative<LoopInfiniteExpr>(expr->node) ||
      std::holds_alternative<LoopConditionalExpr>(expr->node) ||
      std::holds_alternative<LoopIterExpr>(expr->node) ||
      std::holds_alternative<MatchExpr>(expr->node) ||
      std::holds_alternative<BlockExpr>(expr->node) ||
      std::holds_alternative<UnsafeBlockExpr>(expr->node)) {
    return true;
  }
  if (const auto* if_expr = std::get_if<IfExpr>(&expr->node)) {
    // If has else branch that ends with block
    if (if_expr->else_expr) {
      return EndsWithBlock(if_expr->else_expr);
    }
    // If without else still ends with a block (the then branch)
    return true;
  }
  return false;
}

bool RequiresTerminator(const Stmt& stmt) {
  if (const auto* expr_stmt = std::get_if<ExprStmt>(&stmt)) {
    // Expressions ending with blocks don't require explicit terminators
    if (EndsWithBlock(expr_stmt->value)) {
      return false;
    }
    return true;
  }
  return std::holds_alternative<LetStmt>(stmt) ||
         std::holds_alternative<VarStmt>(stmt) ||
         std::holds_alternative<ShadowLetStmt>(stmt) ||
         std::holds_alternative<ShadowVarStmt>(stmt) ||
         std::holds_alternative<AssignStmt>(stmt) ||
         std::holds_alternative<CompoundAssignStmt>(stmt);
}

struct ParseStmtCoreResult {
  Parser parser;
  Stmt stmt;
  bool matched = false;
};

struct ParseStmtSeqResult {
  Parser parser;
  std::vector<Stmt> stmts;
  ExprPtr tail_opt;
};

std::shared_ptr<Block> MakeBlockNode(const core::Span& span,
                                     std::vector<Stmt> stmts,
                                     ExprPtr tail_opt) {
  auto node = std::make_shared<Block>();
  node->stmts = std::move(stmts);
  node->tail_opt = std::move(tail_opt);
  node->span = span;
  return node;
}

ParseElemResult<ExprPtr> ParseRegionOptsOpt(Parser parser) {
  if (!IsPunc(parser, "(")) {
    SPEC_RULE("Parse-Region-Opts-None");
    return {parser, nullptr};
  }
  SPEC_RULE("Parse-Region-Opts-Some");
  Parser next = parser;
  Advance(next);
  ParseElemResult<ExprPtr> expr = ParseExpr(next);
  if (!IsPunc(expr.parser, ")")) {
    EmitParseSyntaxErr(expr.parser, TokSpan(expr.parser));
  } else {
    Advance(expr.parser);
  }
  return {expr.parser, expr.elem};
}

ParseElemResult<std::optional<Identifier>> ParseRegionAliasOpt(Parser parser) {
  if (!IsKw(parser, "as")) {
    SPEC_RULE("Parse-Region-Alias-None");
    return {parser, std::nullopt};
  }
  SPEC_RULE("Parse-Region-Alias-Some");
  Parser next = parser;
  Advance(next);
  ParseElemResult<Identifier> name = ParseIdent(next);
  return {name.parser, name.elem};
}

ParseStmtCoreResult ParseStmtCore(Parser parser) {
  Parser start = parser;
  const Token* tok = Tok(parser);
  if (!tok) {
    return {parser, ErrorStmt{TokSpan(parser)}, false};
  }

  if (tok->kind == TokenKind::Keyword &&
      (tok->lexeme == "let" || tok->lexeme == "var")) {
    SPEC_RULE("Parse-Binding-Stmt");
    const bool is_let = tok->lexeme == "let";
    ParseElemResult<Binding> binding = ParseBindingAfterLetVar(parser);
    if (is_let) {
      SPEC_RULE("LetOrVarStmt-Let");
      LetStmt stmt;
      stmt.binding = std::move(binding.elem);
      stmt.span = stmt.binding.span;
      return {binding.parser, stmt, true};
    }
    SPEC_RULE("LetOrVarStmt-Var");
    VarStmt stmt;
    stmt.binding = std::move(binding.elem);
    stmt.span = stmt.binding.span;
    return {binding.parser, stmt, true};
  }

  if (IsKw(parser, "shadow")) {
    Parser after_shadow = parser;
    Advance(after_shadow);
    if (IsKw(after_shadow, "let") || IsKw(after_shadow, "var")) {
      SPEC_RULE("Parse-Shadow-Stmt");
      ParseElemResult<Stmt> stmt = ParseShadowBinding(after_shadow);
      return {stmt.parser, std::move(stmt.elem), true};
    }
  }

  if (IsKw(parser, "return")) {
    SPEC_RULE("Parse-Return-Stmt");
    Parser next = parser;
    Advance(next);
    ParseElemResult<ExprPtr> expr = ParseExprOpt(next);
    ReturnStmt stmt;
    stmt.value_opt = expr.elem;
    stmt.span = SpanBetween(start, expr.parser);
    return {expr.parser, stmt, true};
  }

  if (IsKw(parser, "break")) {
    SPEC_RULE("Parse-Break-Stmt");
    Parser next = parser;
    Advance(next);
    ParseElemResult<ExprPtr> expr = ParseExprOpt(next);
    BreakStmt stmt;
    stmt.value_opt = expr.elem;
    stmt.span = SpanBetween(start, expr.parser);
    return {expr.parser, stmt, true};
  }

  if (IsKw(parser, "continue")) {
    SPEC_RULE("Parse-Continue-Stmt");
    Parser next = parser;
    Advance(next);
    ContinueStmt stmt;
    stmt.span = SpanBetween(start, next);
    return {next, stmt, true};
  }

  if (IsKw(parser, "unsafe")) {
    SPEC_RULE("Parse-Unsafe-Block");
    Parser next = parser;
    Advance(next);
    ParseElemResult<std::shared_ptr<Block>> block = ParseBlock(next);
    UnsafeBlockStmt stmt;
    stmt.body = block.elem;
    stmt.span = SpanBetween(start, block.parser);
    return {block.parser, stmt, true};
  }

  if (IsKw(parser, "defer")) {
    SPEC_RULE("Parse-Defer-Stmt");
    Parser next = parser;
    Advance(next);
    ParseElemResult<std::shared_ptr<Block>> block = ParseBlock(next);
    DeferStmt stmt;
    stmt.body = block.elem;
    stmt.span = SpanBetween(start, block.parser);
    return {block.parser, stmt, true};
  }

  if (IsKw(parser, "region")) {
    SPEC_RULE("Parse-Region-Stmt");
    Parser next = parser;
    Advance(next);
    ParseElemResult<ExprPtr> opts = ParseRegionOptsOpt(next);
    ParseElemResult<std::optional<Identifier>> alias =
        ParseRegionAliasOpt(opts.parser);
    ParseElemResult<std::shared_ptr<Block>> block = ParseBlock(alias.parser);
    RegionStmt stmt;
    stmt.opts_opt = opts.elem;
    stmt.alias_opt = alias.elem;
    stmt.body = block.elem;
    stmt.span = SpanBetween(start, block.parser);
    return {block.parser, stmt, true};
  }

  if (IsKw(parser, "frame")) {
    SPEC_RULE("Parse-Frame-Stmt");
    Parser next = parser;
    Advance(next);
    ParseElemResult<std::shared_ptr<Block>> block = ParseBlock(next);
    FrameStmt stmt;
    stmt.target_opt = std::nullopt;
    stmt.body = block.elem;
    stmt.span = SpanBetween(start, block.parser);
    return {block.parser, stmt, true};
  }

  // C0X Extension: Key block statement parsing
  // key_block_stmt ::= "#" key_path_list key_block_mod* key_mode? block_expr
  if (IsOp(parser, "#")) {
    SPEC_RULE("Parse-Key-Block-Stmt");
    Parser next = parser;
    Advance(next);  // consume #
    
    // Parse key path list
    std::vector<KeyPathExpr> paths;
    Parser path_start = next;
    // Parse root identifier
    if (const Token* root_tok = Tok(next); root_tok && IsIdentTok(*root_tok)) {
      KeyPathExpr path_expr;
      path_expr.root = root_tok->lexeme;
      path_expr.span = root_tok->span;
      Advance(next);
      
      // Parse key segments
      while (true) {
        if (IsPunc(next, ".")) {
          // Field segment
          Parser after_dot = next;
          Advance(after_dot);
          bool marked = false;
          if (IsOp(after_dot, "#")) {
            marked = true;
            Advance(after_dot);
          }
          if (const Token* field_tok = Tok(after_dot); field_tok && IsIdentTok(*field_tok)) {
            KeySegField seg;
            seg.marked = marked;
            seg.name = field_tok->lexeme;
            path_expr.segs.push_back(seg);
            path_expr.span = SpanBetween(path_start, after_dot);
            Advance(after_dot);
            next = after_dot;
          } else {
            break;
          }
        } else if (IsPunc(next, "[")) {
          // Index segment
          Parser after_bracket = next;
          Advance(after_bracket);
          bool marked = false;
          if (IsOp(after_bracket, "#")) {
            marked = true;
            Advance(after_bracket);
          }
          ParseElemResult<ExprPtr> index_expr = ParseExpr(after_bracket);
          if (IsPunc(index_expr.parser, "]")) {
            Advance(index_expr.parser);
            KeySegIndex seg;
            seg.marked = marked;
            seg.expr = index_expr.elem;
            path_expr.segs.push_back(seg);
            path_expr.span = SpanBetween(path_start, index_expr.parser);
            next = index_expr.parser;
          } else {
            break;
          }
        } else {
          break;
        }
      }
      paths.push_back(std::move(path_expr));
      
      // Parse additional paths separated by comma
      while (IsPunc(next, ",")) {
        Advance(next);
        if (const Token* root = Tok(next); root && IsIdentTok(*root)) {
          KeyPathExpr additional_path;
          additional_path.root = root->lexeme;
          additional_path.span = root->span;
          Advance(next);
          // Parse segments for additional path (similar to above)
          while (IsPunc(next, ".") || IsPunc(next, "[")) {
            if (IsPunc(next, ".")) {
              Parser after_dot = next;
              Advance(after_dot);
              bool marked = IsOp(after_dot, "#");
              if (marked) Advance(after_dot);
              if (const Token* field_tok = Tok(after_dot); field_tok && IsIdentTok(*field_tok)) {
                KeySegField seg;
                seg.marked = marked;
                seg.name = field_tok->lexeme;
                additional_path.segs.push_back(seg);
                Advance(after_dot);
                next = after_dot;
              } else break;
            } else if (IsPunc(next, "[")) {
              Parser after_bracket = next;
              Advance(after_bracket);
              bool marked = IsOp(after_bracket, "#");
              if (marked) Advance(after_bracket);
              ParseElemResult<ExprPtr> idx = ParseExpr(after_bracket);
              if (IsPunc(idx.parser, "]")) {
                Advance(idx.parser);
                KeySegIndex seg;
                seg.marked = marked;
                seg.expr = idx.elem;
                additional_path.segs.push_back(seg);
                next = idx.parser;
              } else break;
            }
          }
          paths.push_back(std::move(additional_path));
        } else {
          break;
        }
      }
    }
    
    // Parse key block modifiers: dynamic, speculative, release
    // Note: These are fixed identifiers per §3.3.4, not keywords
    std::vector<KeyBlockMod> mods;
    const Token* mod_tok = Tok(next);
    while (mod_tok && mod_tok->kind == TokenKind::Identifier &&
           (mod_tok->lexeme == "dynamic" || mod_tok->lexeme == "speculative" ||
            mod_tok->lexeme == "release")) {
      if (mod_tok->lexeme == "dynamic") {
        mods.push_back(KeyBlockMod::Dynamic);
      } else if (mod_tok->lexeme == "speculative") {
        mods.push_back(KeyBlockMod::Speculative);
      } else if (mod_tok->lexeme == "release") {
        mods.push_back(KeyBlockMod::Release);
      }
      Advance(next);
      mod_tok = Tok(next);
    }

    // Parse key mode: read or write
    // Note: These are fixed identifiers per §3.3.4, not keywords
    std::optional<KeyMode> mode;
    const Token* mode_tok = Tok(next);
    if (mode_tok && mode_tok->kind == TokenKind::Identifier) {
      if (mode_tok->lexeme == "read") {
        mode = KeyMode::Read;
        Advance(next);
      } else if (mode_tok->lexeme == "write") {
        mode = KeyMode::Write;
        Advance(next);
      }
    }
    
    // Parse block body
    ParseElemResult<std::shared_ptr<Block>> block = ParseBlock(next);
    
    KeyBlockStmt stmt;
    stmt.paths = std::move(paths);
    stmt.mods = std::move(mods);
    stmt.mode = mode;
    stmt.body = block.elem;
    stmt.span = SpanBetween(start, block.parser);
    return {block.parser, stmt, true};
  }

  if (tok->kind == TokenKind::Identifier) {
    Parser after_name = parser;
    Advance(after_name);
    if (IsPunc(after_name, ".") &&
        IsKw(AdvanceOrEOF(after_name), "frame")) {
      SPEC_RULE("Parse-Frame-Explicit");
      Identifier name = tok->lexeme;
      Parser after_dot = after_name;
      Advance(after_dot);
      Parser after_frame = after_dot;
      Advance(after_frame);
      ParseElemResult<std::shared_ptr<Block>> block = ParseBlock(after_frame);
      FrameStmt stmt;
      stmt.target_opt = name;
      stmt.body = block.elem;
      stmt.span = SpanBetween(start, block.parser);
      return {block.parser, stmt, true};
    }
  }

  if (!IsExprStartToken(*tok)) {
    return {parser, ErrorStmt{TokSpan(parser)}, false};
  }

  ParseElemResult<ExprPtr> expr = ParseExpr(parser);
  if (expr.elem && std::holds_alternative<ErrorExpr>(expr.elem->node)) {
    return {expr.parser, ErrorStmt{SpanBetween(start, expr.parser)}, true};
  }
  const Token* op = Tok(expr.parser);
  if (op && IsAssignOp(*op)) {
    SPEC_RULE("Parse-Assign-Stmt");
    if (!IsPlaceExpr(expr.elem)) {
      EmitParseSyntaxErr(expr.parser, TokSpan(parser));
      Parser sync = expr.parser;
      SyncStmt(sync);
      return {sync, ErrorStmt{SpanBetween(start, sync)}, true};
    }
    Parser next = expr.parser;
    Advance(next);
    ParseElemResult<ExprPtr> rhs = ParseExpr(next);
    if (IsCompoundAssignOp(*op)) {
      SPEC_RULE("AssignOrCompound-Compound");
      CompoundAssignStmt stmt;
      stmt.place = expr.elem;
      stmt.op = op->lexeme;
      stmt.value = rhs.elem;
      stmt.span = SpanBetween(start, rhs.parser);
      return {rhs.parser, stmt, true};
    }
    SPEC_RULE("AssignOrCompound-Assign");
    AssignStmt stmt;
    stmt.place = expr.elem;
    stmt.value = rhs.elem;
    stmt.span = SpanBetween(start, rhs.parser);
    return {rhs.parser, stmt, true};
  }

  SPEC_RULE("Parse-Expr-Stmt");
  ExprStmt stmt;
  stmt.value = expr.elem;
  stmt.span = SpanBetween(start, expr.parser);
  return {expr.parser, stmt, true};
}

ParseStmtSeqResult ParseStmtSeq(Parser parser) {
  // Skip leading newlines
  while (Tok(parser) && Tok(parser)->kind == TokenKind::Newline) {
    Advance(parser);
  }
  
  if (IsPunc(parser, "}")) {
    SPEC_RULE("ParseStmtSeq-End");
    return {parser, {}, nullptr};
  }

  Parser probe = Clone(parser);
  ParseElemResult<ExprPtr> tail = ParseExpr(probe);
  Parser tail_end = Clone(tail.parser);
  while (Tok(tail_end) && Tok(tail_end)->kind == TokenKind::Newline) {
    Advance(tail_end);
  }
  if (tail.parser.diags.empty() && IsPunc(tail_end, "}")) {
    SPEC_RULE("ParseStmtSeq-TailExpr");
    Parser merged = MergeDiag(parser, tail.parser, tail_end);
    return {merged, {}, tail.elem};
  }

  SPEC_RULE("ParseStmtSeq-Cons");
  ParseElemResult<Stmt> head = ParseStmt(parser);
  ParseStmtSeqResult tail_seq = ParseStmtSeq(head.parser);
  std::vector<Stmt> stmts;
  stmts.reserve(1 + tail_seq.stmts.size());
  stmts.push_back(std::move(head.elem));
  stmts.insert(stmts.end(),
               std::make_move_iterator(tail_seq.stmts.begin()),
               std::make_move_iterator(tail_seq.stmts.end()));
  tail_seq.stmts = std::move(stmts);
  return tail_seq;
}

}  // namespace

Parser MakeParser(const std::vector<Token>& tokens,
                  const std::vector<DocComment>& docs,
                  const core::SourceFile& source) {
  Parser parser;
  parser.tokens = &tokens;
  parser.index = 0;
  parser.docs = &docs;
  parser.doc_index = 0;
  parser.depth = 0;
  parser.eof = MakeEofToken(source);
  return parser;
}

Parser MakeParser(const std::vector<Token>& tokens,
                  const core::SourceFile& source) {
  static const std::vector<DocComment> kEmptyDocs;
  return MakeParser(tokens, kEmptyDocs, source);
}

bool AtEof(const Parser& parser) {
  if (!parser.tokens) {
    return true;
  }
  return parser.index >= parser.tokens->size();
}

const Token* Tok(const Parser& parser) {
  if (!parser.tokens) {
    return nullptr;
  }
  if (parser.index >= parser.tokens->size()) {
    return nullptr;
  }
  return &(*parser.tokens)[parser.index];
}

const core::Span& TokSpan(const Parser& parser) {
  if (const auto* tok = Tok(parser)) {
    return tok->span;
  }
  return parser.eof.span;
}

void Advance(Parser& parser) {
  if (AtEof(parser)) {
    return;
  }
  parser.index += 1;
}
void ConsumeTerminatorOpt(Parser& parser, TerminatorPolicy policy) {
  const Token* tok = Tok(parser);
  const bool is_term = tok && IsTerminatorToken(*tok);

  if (policy == TerminatorPolicy::Required) {
    if (is_term) {
      SPEC_RULE("ConsumeTerminatorOpt-Req-Yes");
      Advance(parser);
      return;
    }
    SPEC_RULE("ConsumeTerminatorOpt-Req-No");
    EmitMissingTerminator(parser.diags, TokSpan(parser));
    SyncStmt(parser);
    return;
  }

  if (is_term) {
    SPEC_RULE("ConsumeTerminatorOpt-Opt-Yes");
    Advance(parser);
  } else {
    SPEC_RULE("ConsumeTerminatorOpt-Opt-No");
  }
}

void ConsumeTerminatorReq(Parser& parser) {
  const Token* tok = Tok(parser);
  if (tok && IsTerminatorToken(*tok)) {
    SPEC_RULE("ConsumeTerminatorReq-Yes");
    Advance(parser);
    return;
  }
  SPEC_RULE("ConsumeTerminatorReq-No");
  EmitMissingTerminator(parser.diags, TokSpan(parser));
  SyncStmt(parser);
}

ParseElemResult<Stmt> ParseStmt(Parser parser) {
  ParseStmtCoreResult core = ParseStmtCore(parser);
  if (!core.matched) {
    SPEC_RULE("Parse-Statement-Err");
    EmitParseSyntaxErr(parser, TokSpan(parser));
    Parser next = AdvanceOrEOF(parser);
    Parser sync = next;
    SyncStmt(sync);
    return {sync, ErrorStmt{SpanBetween(parser, sync)}};
  }

  SPEC_RULE("Parse-Statement");
  Parser next = core.parser;
  TerminatorPolicy policy = RequiresTerminator(core.stmt)
                                ? TerminatorPolicy::Required
                                : TerminatorPolicy::Optional;
  ConsumeTerminatorOpt(next, policy);
  return {next, std::move(core.stmt)};
}

ParseElemResult<std::shared_ptr<Block>> ParseBlock(Parser parser) {
  // Skip newlines before opening brace (C0X: allows where clause on separate line)
  while (Tok(parser) && Tok(parser)->kind == TokenKind::Newline) {
    Advance(parser);
  }
  Parser start = parser;
  if (!IsPunc(parser, "{")) {
    EmitParseSyntaxErr(parser, TokSpan(parser));
    Parser next = AdvanceOrEOF(parser);
    core::Span span = SpanBetween(start, next);
    return {next, MakeBlockNode(span, {}, nullptr)};
  }

  SPEC_RULE("Parse-Block");
  Parser next = parser;
  Advance(next);
  ParseStmtSeqResult seq = ParseStmtSeq(next);
  if (!IsPunc(seq.parser, "}")) {
    EmitParseSyntaxErr(seq.parser, TokSpan(seq.parser));
    core::Span span = SpanBetween(start, seq.parser);
    return {seq.parser, MakeBlockNode(span, std::move(seq.stmts), seq.tail_opt)};
  }
  Parser after = seq.parser;
  Advance(after);
  core::Span span = SpanBetween(start, after);
  return {after, MakeBlockNode(span, std::move(seq.stmts), seq.tail_opt)};
}

}  // namespace cursive0::syntax
