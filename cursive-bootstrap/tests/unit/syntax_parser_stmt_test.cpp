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
using cursive0::syntax::ASTFile;
using cursive0::syntax::AssignStmt;
using cursive0::syntax::BreakStmt;
using cursive0::syntax::CompoundAssignStmt;
using cursive0::syntax::ContinueStmt;
using cursive0::syntax::DeferStmt;
using cursive0::syntax::ExprStmt;
using cursive0::syntax::FrameStmt;
using cursive0::syntax::IdentifierExpr;
using cursive0::syntax::LetStmt;
using cursive0::syntax::ParseFile;
using cursive0::syntax::ProcedureDecl;
using cursive0::syntax::RegionStmt;
using cursive0::syntax::ResultStmt;
using cursive0::syntax::ReturnStmt;
using cursive0::syntax::ShadowLetStmt;
using cursive0::syntax::ShadowVarStmt;
using cursive0::syntax::UnsafeBlockStmt;
using cursive0::syntax::VarStmt;

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

}  // namespace

int main() {
  SPEC_COV("Parse-Statement");
  SPEC_COV("Parse-Statement-Err");
  SPEC_COV("Parse-Block");
  SPEC_COV("ParseStmtSeq-Cons");
  SPEC_COV("ParseStmtSeq-TailExpr");
  SPEC_COV("ParseStmtSeq-End");
  SPEC_COV("Parse-Binding-Stmt");
  SPEC_COV("Parse-Shadow-Stmt");
  SPEC_COV("Parse-Return-Stmt");
  SPEC_COV("Parse-Result-Stmt");
  SPEC_COV("Parse-Defer-Stmt");
  SPEC_COV("Parse-Frame-Stmt");
  SPEC_COV("Parse-Frame-Explicit");
  SPEC_COV("Parse-Unsafe-Block");
  SPEC_COV("Parse-Break-Stmt");
  SPEC_COV("Parse-Continue-Stmt");
  SPEC_COV("Parse-Expr-Stmt");
  SPEC_COV("Parse-Assign-Stmt");
  SPEC_COV("AssignOrCompound-Assign");
  SPEC_COV("AssignOrCompound-Compound");
  SPEC_COV("Parse-Region-Stmt");
  SPEC_COV("Parse-Region-Alias-None");
  SPEC_COV("Parse-Region-Alias-Some");
  SPEC_COV("Parse-Region-Opts-None");
  SPEC_COV("Parse-Region-Opts-Some");
  SPEC_COV("ConsumeTerminatorOpt-Req-Yes");
  SPEC_COV("ConsumeTerminatorOpt-Req-No");
  SPEC_COV("ConsumeTerminatorOpt-Opt-Yes");
  SPEC_COV("ConsumeTerminatorOpt-Opt-No");

  const SourceFile source = MakeSourceFile(
      "stmts.cursive",
      "procedure demo() { let a = 1; var b = 2; shadow let c = 3; shadow var d = 4; "
      "a = 5; a += 1; a; defer { } region { } region(1) as r { } frame { } "
      "r.frame { } unsafe { } result 7; break; continue; return 9; } "
      "procedure tail() { let x = 1; x }");

  auto result = ParseFile(source);
  assert(result.file.has_value());
  assert(result.diags.empty());
  const ASTFile& file = *result.file;
  assert(file.items.size() == 2);

  const auto& demo = std::get<ProcedureDecl>(file.items[0]);
  assert(demo.body != nullptr);
  assert(demo.body->tail_opt == nullptr);
  assert(demo.body->stmts.size() == 17);

  assert(std::holds_alternative<LetStmt>(demo.body->stmts[0]));
  assert(std::holds_alternative<VarStmt>(demo.body->stmts[1]));
  assert(std::holds_alternative<ShadowLetStmt>(demo.body->stmts[2]));
  assert(std::holds_alternative<ShadowVarStmt>(demo.body->stmts[3]));
  assert(std::holds_alternative<AssignStmt>(demo.body->stmts[4]));
  assert(std::holds_alternative<CompoundAssignStmt>(demo.body->stmts[5]));
  assert(std::holds_alternative<ExprStmt>(demo.body->stmts[6]));
  assert(std::holds_alternative<DeferStmt>(demo.body->stmts[7]));

  const auto& region_none = std::get<RegionStmt>(demo.body->stmts[8]);
  assert(region_none.opts_opt == nullptr);
  assert(!region_none.alias_opt.has_value());

  const auto& region_some = std::get<RegionStmt>(demo.body->stmts[9]);
  assert(region_some.opts_opt != nullptr);
  assert(region_some.alias_opt.has_value());
  assert(region_some.alias_opt.value() == "r");

  const auto& frame_implicit = std::get<FrameStmt>(demo.body->stmts[10]);
  assert(!frame_implicit.target_opt.has_value());

  const auto& frame_explicit = std::get<FrameStmt>(demo.body->stmts[11]);
  assert(frame_explicit.target_opt.has_value());
  assert(frame_explicit.target_opt.value() == "r");

  assert(std::holds_alternative<UnsafeBlockStmt>(demo.body->stmts[12]));
  assert(std::holds_alternative<ResultStmt>(demo.body->stmts[13]));
  assert(std::holds_alternative<BreakStmt>(demo.body->stmts[14]));
  assert(std::holds_alternative<ContinueStmt>(demo.body->stmts[15]));

  const auto& ret_stmt = std::get<ReturnStmt>(demo.body->stmts[16]);
  assert(ret_stmt.value_opt != nullptr);

  const auto& tail = std::get<ProcedureDecl>(file.items[1]);
  assert(tail.body != nullptr);
  assert(tail.body->stmts.size() == 1);
  assert(tail.body->tail_opt != nullptr);
  assert(std::holds_alternative<IdentifierExpr>(tail.body->tail_opt->node));

  {
    const SourceFile source = MakeSourceFile(
        "stmts_opt_term.cursive",
        "procedure opt() { return 1 }");
    auto result = ParseFile(source);
    assert(result.file.has_value());
    assert(result.diags.empty());
  }

  {
    const SourceFile source = MakeSourceFile(
        "stmts_req_term.cursive",
        "procedure bad() { let x = 1 return 2; }");
    auto result = ParseFile(source);
    assert(result.file.has_value());
    assert(!result.diags.empty());
  }

  {
    const SourceFile source = MakeSourceFile(
        "stmts_err.cursive",
        "procedure bad() { @ }");
    auto result = ParseFile(source);
    assert(result.file.has_value());
    assert(!result.diags.empty());
  }

  return 0;
}
