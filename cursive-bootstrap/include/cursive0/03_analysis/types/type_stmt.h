#pragma once

#include <optional>
#include <string_view>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "cursive0/00_core/diagnostics.h"
#include "cursive0/03_analysis/memory/calls.h"
#include "cursive0/03_analysis/types/context.h"
#include "cursive0/03_analysis/types/place_types.h"
#include "cursive0/03_analysis/types/type_infer.h"
#include "cursive0/03_analysis/types/types.h"
#include "cursive0/02_syntax/ast.h"

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

// Returns Async signature for Async/alias types, or std::nullopt if not async.
std::optional<AsyncSig> AsyncSigOf(const ScopeContext& ctx, const TypeRef& type);

enum class LoopFlag {
  None,
  Loop,
};

enum class ContractPhase {
  None,
  Precondition,
  Postcondition,
};

struct StmtTypeContext {
  TypeRef return_type;
  LoopFlag loop_flag = LoopFlag::None;
  bool in_unsafe = false;
  core::DiagnosticStream* diags = nullptr;
  TypeEnv* env_ref = nullptr;
  struct OpaqueReturnState* opaque_return = nullptr;
  // C0X Extension: Structured Concurrency (§10.1)
  bool in_parallel = false;          // true when inside a parallel block
  TypeRef parallel_domain;           // domain type when in_parallel is true
  bool keys_held = false;            // true when keys are held (for wait restriction)
  std::unordered_set<IdKey>* unique_captures = nullptr;  // track unique captures
  // C0X Extension: Contract predicates / invariants
  ContractPhase contract_phase = ContractPhase::None;
  bool require_pure = false;
  const syntax::ContractClause* contract = nullptr;
  bool contract_dynamic = false;
};

struct OpaqueReturnState {
  const syntax::Type* origin = nullptr;
  TypePath class_path;
  TypeRef underlying;
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

// C0X Extension: Structured Concurrency (§10, §18)

// §18.1.1 Parallel block expression typing (T-Parallel)
ExprTypeResult TypeParallelExpr(const ScopeContext& ctx,
                                const StmtTypeContext& type_ctx,
                                const syntax::ParallelExpr& expr,
                                const TypeEnv& env,
                                const ExprTypeFn& type_expr,
                                const IdentTypeFn& type_ident,
                                const PlaceTypeFn& type_place);

// §18.4.1 Spawn expression typing (T-Spawn)
ExprTypeResult TypeSpawnExpr(const ScopeContext& ctx,
                             const StmtTypeContext& type_ctx,
                             const syntax::SpawnExpr& expr,
                             const TypeEnv& env,
                             const ExprTypeFn& type_expr,
                             const IdentTypeFn& type_ident,
                             const PlaceTypeFn& type_place);

// §10.3 Wait expression typing (T-Wait)
// Note: wait implicitly consumes the handle, so accepts type_place for non-Bitcopy handles
ExprTypeResult TypeWaitExpr(const ScopeContext& ctx,
                            const StmtTypeContext& type_ctx,
                            const syntax::WaitExpr& expr,
                            const TypeEnv& env,
                            const ExprTypeFn& type_expr,
                            const PlaceTypeFn& type_place);

// §18.5.1 Dispatch expression typing (T-Dispatch, T-Dispatch-Reduce)
ExprTypeResult TypeDispatchExpr(const ScopeContext& ctx,
                                const StmtTypeContext& type_ctx,
                                const syntax::DispatchExpr& expr,
                                const TypeEnv& env,
                                const ExprTypeFn& type_expr,
                                const IdentTypeFn& type_ident,
                                const PlaceTypeFn& type_place);

// C0X Extension: Async expressions (§19)
ExprTypeResult TypeYieldExpr(const ScopeContext& ctx,
                             const StmtTypeContext& type_ctx,
                             const syntax::YieldExpr& expr,
                             const TypeEnv& env,
                             const ExprTypeFn& type_expr);

ExprTypeResult TypeYieldFromExpr(const ScopeContext& ctx,
                                 const StmtTypeContext& type_ctx,
                                 const syntax::YieldFromExpr& expr,
                                 const TypeEnv& env,
                                 const ExprTypeFn& type_expr);

ExprTypeResult TypeSyncExpr(const ScopeContext& ctx,
                            const StmtTypeContext& type_ctx,
                            const syntax::SyncExpr& expr,
                            const TypeEnv& env,
                            const ExprTypeFn& type_expr);

ExprTypeResult TypeRaceExpr(const ScopeContext& ctx,
                            const StmtTypeContext& type_ctx,
                            const syntax::RaceExpr& expr,
                            const TypeEnv& env,
                            const ExprTypeFn& type_expr,
                            const IdentTypeFn& type_ident,
                            const PlaceTypeFn& type_place);

ExprTypeResult TypeAllExpr(const ScopeContext& ctx,
                           const StmtTypeContext& type_ctx,
                           const syntax::AllExpr& expr,
                           const TypeEnv& env,
                           const ExprTypeFn& type_expr);

}  // namespace cursive0::analysis
