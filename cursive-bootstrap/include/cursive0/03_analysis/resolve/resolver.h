#pragma once

#include <optional>
#include <string_view>
#include <vector>

#include "cursive0/00_core/diagnostics.h"
#include "cursive0/03_analysis/types/context.h"
#include "cursive0/03_analysis/resolve/scopes_lookup.h"
#include "cursive0/02_syntax/ast.h"

namespace cursive0::analysis {

struct ResolveContext {
  ScopeContext* ctx = nullptr;
  const NameMapTable* name_maps = nullptr;
  const ModuleNames* module_names = nullptr;
  CanAccessFn can_access = nullptr;
};

template <typename T>
struct ResolveResult {
  bool ok = false;
  std::optional<std::string_view> diag_id;
  std::optional<core::Span> span;
  T value{};
};

using ResExprResult = ResolveResult<syntax::ExprPtr>;
using ResTypeResult = ResolveResult<std::shared_ptr<syntax::Type>>;
using ResTypePathResult = ResolveResult<syntax::TypePath>;
using ResClassPathResult = ResolveResult<syntax::ClassPath>;
using ResPatternResult = ResolveResult<syntax::PatternPtr>;
using ResParamsResult = ResolveResult<std::vector<syntax::Param>>;
using ResItemsResult = ResolveResult<std::vector<syntax::ASTItem>>;

struct ResolveStmtResult {
  bool ok = false;
  std::optional<std::string_view> diag_id;
  std::optional<core::Span> span;
  syntax::Stmt stmt{};
};

struct ResolveStmtSeqResult {
  bool ok = false;
  std::optional<std::string_view> diag_id;
  std::optional<core::Span> span;
  std::vector<syntax::Stmt> stmts;
};

struct ResolveBlockResult {
  bool ok = false;
  std::optional<std::string_view> diag_id;
  std::optional<core::Span> span;
  syntax::Block block;
};

struct ResolveModuleResult {
  bool ok = false;
  std::optional<std::string_view> diag_id;
  std::optional<core::Span> span;
  syntax::ASTModule module;
};

struct ResolveModulesResult {
  bool ok = false;
  core::DiagnosticStream diags;
  std::vector<syntax::ASTModule> modules;
};

void PopulateSigma(ScopeContext& ctx);

ResolveModulesResult ResolveModules(ResolveContext& ctx);
ResolveModuleResult ResolveModule(ResolveContext& ctx,
                                  const syntax::ASTModule& module);

ResItemsResult ResolveItems(ResolveContext& ctx,
                                const std::vector<syntax::ASTItem>& items);
ResolveResult<syntax::ASTItem> ResolveItem(ResolveContext& ctx,
                                           const syntax::ASTItem& item);

ResExprResult ResolveExpr(ResolveContext& ctx,
                              const syntax::ExprPtr& expr);
ResPatternResult ResolvePattern(ResolveContext& ctx,
                                    const syntax::PatternPtr& pattern);
ResTypeResult ResolveType(ResolveContext& ctx,
                              const std::shared_ptr<syntax::Type>& type);
ResTypePathResult ResolveTypePath(ResolveContext& ctx,
                                      const syntax::TypePath& path);
ResClassPathResult ResolveClassPath(ResolveContext& ctx,
                                        const syntax::ClassPath& path);

ResolveStmtResult ResolveStmt(ResolveContext& ctx,
                              const syntax::Stmt& stmt);
ResolveStmtSeqResult ResolveStmtSeq(ResolveContext& ctx,
                                    const std::vector<syntax::Stmt>& stmts);
ResolveBlockResult ResolveBlock(ResolveContext& ctx,
                                const syntax::Block& block);

}  // namespace cursive0::analysis
