#pragma once

#include <optional>
#include <string_view>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "cursive0/core/diagnostics.h"
#include "cursive0/analysis/memory/calls.h"
#include "cursive0/analysis/types/context.h"
#include "cursive0/analysis/types/place_types.h"
#include "cursive0/analysis/types/type_infer.h"
#include "cursive0/analysis/types/types.h"
#include "cursive0/syntax/ast.h"

namespace cursive0::analysis {

struct TypeBinding {
  syntax::Mutability mut = syntax::Mutability::Let;
  TypeRef type;
};

using TypeScope = std::unordered_map<IdKey, TypeBinding>;

struct TypeEnv {
  std::vector<TypeScope> scopes;
};

TypeEnv PushScope(const TypeEnv& env);
TypeEnv PopScope(const TypeEnv& env);
std::optional<TypeBinding> BindOf(const TypeEnv& env, std::string_view name);
std::optional<syntax::Mutability> MutOf(const TypeEnv& env,
                                        std::string_view name);

struct FlowInfo {
  std::vector<TypeRef> results;
  std::vector<TypeRef> breaks;
  bool break_void = false;
};

struct StmtTypeResult {
  bool ok = false;
  std::optional<std::string_view> diag_id;
  TypeEnv env;
  FlowInfo flow;
};

struct StmtSeqResult {
  bool ok = false;
  std::optional<std::string_view> diag_id;
  TypeEnv env;
  FlowInfo flow;
};

struct BlockInfoResult {
  bool ok = false;
  std::optional<std::string_view> diag_id;
  TypeRef type;
  std::vector<TypeRef> breaks;
  bool break_void = false;
};

struct PatternTypeResult {
  bool ok = false;
  std::optional<std::string_view> diag_id;
  std::vector<std::pair<std::string, TypeRef>> bindings;
};

enum class LoopFlag {
  None,
  Loop,
};

struct StmtTypeContext {
  TypeRef return_type;
  LoopFlag loop_flag = LoopFlag::None;
  bool in_unsafe = false;
  core::DiagnosticStream* diags = nullptr;
  TypeEnv* env_ref = nullptr;
};

PatternTypeResult TypePattern(const ScopeContext& ctx,
                              const syntax::PatternPtr& pattern,
                              const TypeRef& expected);

StmtTypeResult TypeStmt(const ScopeContext& ctx,
                        const StmtTypeContext& type_ctx,
                        const syntax::Stmt& stmt,
                        const TypeEnv& env,
                        const ExprTypeFn& type_expr,
                        const IdentTypeFn& type_ident,
                        const PlaceTypeFn& type_place,
                        TypeEnv* env_ref = nullptr);

StmtSeqResult TypeStmtSeq(const ScopeContext& ctx,
                          const StmtTypeContext& type_ctx,
                          const std::vector<syntax::Stmt>& stmts,
                          const TypeEnv& env,
                          const ExprTypeFn& type_expr,
                          const IdentTypeFn& type_ident,
                          const PlaceTypeFn& type_place,
                          TypeEnv* env_ref = nullptr);

BlockInfoResult TypeBlockInfo(const ScopeContext& ctx,
                              const StmtTypeContext& type_ctx,
                              const syntax::Block& block,
                              const TypeEnv& env,
                              const ExprTypeFn& type_expr,
                              const IdentTypeFn& type_ident,
                              const PlaceTypeFn& type_place,
                              TypeEnv* env_ref = nullptr);

ExprTypeResult TypeBlock(const ScopeContext& ctx,
                         const StmtTypeContext& type_ctx,
                         const syntax::Block& block,
                         const TypeEnv& env,
                         const ExprTypeFn& type_expr,
                         const IdentTypeFn& type_ident,
                         const PlaceTypeFn& type_place,
                         TypeEnv* env_ref = nullptr);

CheckResult CheckBlock(const ScopeContext& ctx,
                       const StmtTypeContext& type_ctx,
                       const syntax::Block& block,
                       const TypeEnv& env,
                       const TypeRef& expected,
                       const ExprTypeFn& type_expr,
                       const IdentTypeFn& type_ident,
                       const PlaceTypeFn& type_place,
                       TypeEnv* env_ref = nullptr);

ExprTypeResult TypeBlockExpr(const ScopeContext& ctx,
                             const StmtTypeContext& type_ctx,
                             const syntax::BlockExpr& expr,
                             const TypeEnv& env,
                             const ExprTypeFn& type_expr,
                             const IdentTypeFn& type_ident,
                             const PlaceTypeFn& type_place);

CheckResult CheckBlockExpr(const ScopeContext& ctx,
                           const StmtTypeContext& type_ctx,
                           const syntax::BlockExpr& expr,
                           const TypeEnv& env,
                           const TypeRef& expected,
                           const ExprTypeFn& type_expr,
                           const IdentTypeFn& type_ident,
                           const PlaceTypeFn& type_place);

ExprTypeResult TypeUnsafeBlockExpr(const ScopeContext& ctx,
                                   const StmtTypeContext& type_ctx,
                                   const syntax::UnsafeBlockExpr& expr,
                                   const TypeEnv& env,
                                   const ExprTypeFn& type_expr,
                                   const IdentTypeFn& type_ident,
                                   const PlaceTypeFn& type_place);

CheckResult CheckUnsafeBlockExpr(const ScopeContext& ctx,
                                 const StmtTypeContext& type_ctx,
                                 const syntax::UnsafeBlockExpr& expr,
                                 const TypeEnv& env,
                                 const TypeRef& expected,
                                 const ExprTypeFn& type_expr,
                                 const IdentTypeFn& type_ident,
                                 const PlaceTypeFn& type_place);

ExprTypeResult TypeLoopInfiniteExpr(const ScopeContext& ctx,
                                    const StmtTypeContext& type_ctx,
                                    const syntax::LoopInfiniteExpr& expr,
                                    const TypeEnv& env,
                                    const ExprTypeFn& type_expr,
                                    const IdentTypeFn& type_ident,
                                    const PlaceTypeFn& type_place);

ExprTypeResult TypeLoopConditionalExpr(const ScopeContext& ctx,
                                       const StmtTypeContext& type_ctx,
                                       const syntax::LoopConditionalExpr& expr,
                                       const TypeEnv& env,
                                       const ExprTypeFn& type_expr,
                                       const IdentTypeFn& type_ident,
                                       const PlaceTypeFn& type_place);

ExprTypeResult TypeLoopIterExpr(const ScopeContext& ctx,
                                const StmtTypeContext& type_ctx,
                                const syntax::LoopIterExpr& expr,
                                const TypeEnv& env,
                                const ExprTypeFn& type_expr,
                                const IdentTypeFn& type_ident,
                                const PlaceTypeFn& type_place);

}  // namespace cursive0::analysis
