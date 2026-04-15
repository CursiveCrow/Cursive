// =============================================================================
// File: 04_analysis/typing/expr/parallel_expr.cpp
// Parallel Block Expression Typing
// Spec Section: 17.2
// =============================================================================
//
// SPEC REFERENCE: CursiveSpecification.md
//   Section 17.2: Structured Parallelism
//   - T-Parallel: Parallel block typing
//   - T-Parallel-Empty: Empty parallel block
//   - T-Parallel-Single: Single spawn
//   - T-Parallel-Multi: Multiple spawns
//   - Structured concurrency invariant
//
// =============================================================================

#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

#include "00_core/assert_spec.h"
#include "00_core/diagnostic_messages.h"
#include "00_core/diagnostics.h"
#include "04_analysis/caps/cap_concurrency.h"
#include "04_analysis/typing/context.h"
#include "04_analysis/typing/place_types.h"
#include "04_analysis/typing/type_expr.h"
#include "04_analysis/typing/type_infer.h"
#include "04_analysis/typing/type_lookup.h"
#include "04_analysis/typing/type_stmt.h"
#include "04_analysis/typing/types.h"
#include "02_source/ast/ast.h"

namespace cursive::analysis {

namespace {

static inline void SpecDefsParallel() {
  SPEC_DEF("T-Parallel", "17.2");
  SPEC_DEF("T-Parallel-Empty", "17.2");
  SPEC_DEF("T-Parallel-Single", "17.2");
  SPEC_DEF("T-Parallel-Multi", "17.2");
  SPEC_DEF("ParallelBlockOpts", "17.2");
  SPEC_DEF("CaptureSemantics", "17.2");
  SPEC_DEF("ForkJoin", "17.2");
  SPEC_DEF("ExecutionDomain", "17.2");
  SPEC_DEF("T-GPU-Nested-Err", "18.1.1");
}

core::Diagnostic MakeInternalTypingDiagnostic(core::Severity severity,
                                              const std::string& message) {
  core::Diagnostic diag;
  diag.severity = severity;
  diag.message = message;
  return diag;
}

// Strip permission qualifiers
static TypeRef StripPermLocal(const TypeRef& type) {
  if (!type) {
    return type;
  }
  TypeRef cur = type;
  while (cur) {
    if (const auto* perm = std::get_if<TypePerm>(&cur->node)) {
      cur = perm->base;
      continue;
    }
    if (const auto* refine = std::get_if<TypeRefine>(&cur->node)) {
      cur = refine->base;
      continue;
    }
    break;
  }
  return cur;
}

// Check if type is $ExecutionDomain
static bool IsExecutionDomainType(const TypeRef& type) {
  const auto stripped = StripPermLocal(type);
  if (!stripped) {
    return false;
  }

  // Accept ExecutionDomain and built-in domain subclasses (Cpu/Gpu/Inline).
  if (const auto* dyn = std::get_if<TypeDynamic>(&stripped->node)) {
    if (IsExecutionDomainTypePath(dyn->path)) {
      return true;
    }
  }

  // Also allow path-based references.
  if (const auto* path = std::get_if<TypePathType>(&stripped->node)) {
    if (IsExecutionDomainTypePath(path->path)) {
      return true;
    }
  }

  return false;
}

static bool IsGpuDomainType(const TypeRef& type) {
  const auto stripped = StripPermLocal(type);
  if (!stripped) {
    return false;
  }
  if (const auto* dyn = std::get_if<TypeDynamic>(&stripped->node)) {
    return IsGpuDomainTypePath(dyn->path);
  }
  if (const auto* path = std::get_if<TypePathType>(&stripped->node)) {
    return IsGpuDomainTypePath(path->path);
  }
  return false;
}

// Check if type is CancelToken
static bool IsCancelTokenType(const TypeRef& type) {
  const auto stripped = StripPermLocal(type);
  if (!stripped) {
    return false;
  }

  // Spec rule BlockOptOk(Cancel(e)) requires TypePath(["CancelToken"]),
  // not a state-qualified CancelToken@S value.
  if (const auto* path = std::get_if<TypePathType>(&stripped->node)) {
    if (IsCancelTokenTypePath(path->path)) {
      return true;
    }
  }

  return false;
}

// Check if type is string (for name option)
static bool IsStringType(const TypeRef& type) {
  const auto stripped = StripPermLocal(type);
  if (!stripped) {
    return false;
  }

  if (std::holds_alternative<TypeString>(stripped->node)) {
    return true;
  }

  return false;
}

static bool DispatchHasReduce(const ast::DispatchExpr& dispatch) {
  for (const auto& opt : dispatch.opts) {
    if (opt.kind == ast::DispatchOptionKind::Reduce) {
      return true;
    }
  }
  return false;
}

struct ParallelResultCollect {
  bool ok = true;
  std::optional<std::string_view> diag_id;
  std::vector<TypeRef> types;
};

static ast::ExprPtr StripAttributedExpr(ast::ExprPtr expr) {
  ast::ExprPtr cur = expr;
  while (cur) {
    const auto* attr = std::get_if<ast::AttributedExpr>(&cur->node);
    if (!attr) {
      break;
    }
    cur = attr->expr;
  }
  return cur;
}

static bool IsCollectableParallelExpr(const ast::ExprPtr& expr) {
  const auto stripped_expr = StripAttributedExpr(expr);
  if (!stripped_expr) {
    return false;
  }
  if (std::holds_alternative<ast::SpawnExpr>(stripped_expr->node)) {
    return true;
  }
  if (const auto* dispatch = std::get_if<ast::DispatchExpr>(&stripped_expr->node)) {
    return DispatchHasReduce(*dispatch);
  }
  return false;
}

static ParallelResultCollect CollectParallelResultExpr(
    const ExprTypeFn& type_expr,
    const ast::ExprPtr& expr,
    ParallelResultCollect in) {
  if (!in.ok || !expr) {
    return in;
  }

  const auto stripped_expr = StripAttributedExpr(expr);
  if (!stripped_expr) {
    return in;
  }

  if (const auto* spawn = std::get_if<ast::SpawnExpr>(&stripped_expr->node)) {
    (void)spawn;
    const auto typed = type_expr(stripped_expr);
    if (!typed.ok) {
      in.ok = false;
      in.diag_id = typed.diag_id;
      return in;
    }
    TypeRef collected = typed.type;
    if (const auto inner = ExtractSpawnedInner(StripPermLocal(typed.type))) {
      collected = *inner;
    }
    in.types.push_back(collected);
    return in;
  }

  if (const auto* dispatch = std::get_if<ast::DispatchExpr>(&stripped_expr->node)) {
    if (!DispatchHasReduce(*dispatch)) {
      return in;
    }
    const auto typed = type_expr(stripped_expr);
    if (!typed.ok) {
      in.ok = false;
      in.diag_id = typed.diag_id;
      return in;
    }
    in.types.push_back(typed.type);
    return in;
  }

  return in;
}

static ParallelResultCollect CollectParallelResultStmt(
    const ExprTypeFn& type_expr,
    const ast::Stmt& stmt,
    ParallelResultCollect in) {
  if (!in.ok) {
    return in;
  }
  if (const auto* expr_stmt = std::get_if<ast::ExprStmt>(&stmt)) {
    return CollectParallelResultExpr(type_expr, expr_stmt->value, std::move(in));
  }
  if (const auto* let_stmt = std::get_if<ast::LetStmt>(&stmt)) {
    return CollectParallelResultExpr(type_expr, let_stmt->binding.init, std::move(in));
  }
  if (const auto* var_stmt = std::get_if<ast::VarStmt>(&stmt)) {
    return CollectParallelResultExpr(type_expr, var_stmt->binding.init, std::move(in));
  }
  if (const auto* shadow_let = std::get_if<ast::ShadowLetStmt>(&stmt)) {
    return CollectParallelResultExpr(type_expr, shadow_let->init, std::move(in));
  }
  if (const auto* shadow_var = std::get_if<ast::ShadowVarStmt>(&stmt)) {
    return CollectParallelResultExpr(type_expr, shadow_var->init, std::move(in));
  }
  if (const auto* assign = std::get_if<ast::AssignStmt>(&stmt)) {
    return CollectParallelResultExpr(type_expr, assign->value, std::move(in));
  }
  if (const auto* compound = std::get_if<ast::CompoundAssignStmt>(&stmt)) {
    return CollectParallelResultExpr(type_expr, compound->value, std::move(in));
  }
  return in;
}

class ParallelCaptureCollector {
 public:
  explicit ParallelCaptureCollector(const TypeEnv& env) : env_(env) {
    local_scopes_.emplace_back();
  }

  std::unordered_set<IdKey> Collect(const ast::Block& block) {
    VisitBlock(block);
    return captures_;
  }

 private:
  void PushScope() { local_scopes_.emplace_back(); }

  void PopScope() {
    if (!local_scopes_.empty()) {
      local_scopes_.pop_back();
    }
  }

  bool IsLocal(std::string_view name) const {
    const IdKey key{name};
    for (auto it = local_scopes_.rbegin(); it != local_scopes_.rend(); ++it) {
      if (it->find(key) != it->end()) {
        return true;
      }
    }
    return false;
  }

  void DeclareName(std::string_view name) {
    if (local_scopes_.empty()) {
      local_scopes_.emplace_back();
    }
    local_scopes_.back().insert(IdKey{name});
  }

  void DeclarePattern(const ast::PatternPtr& pattern) {
    if (!pattern) {
      return;
    }
    std::vector<IdKey> names;
    CollectPatNames(*pattern, names);
    for (const auto& name : names) {
      DeclareName(name);
    }
  }

  void CaptureIfOuter(std::string_view name) {
    if (IsLocal(name)) {
      return;
    }
    if (BindOf(env_, name).has_value()) {
      captures_.insert(IdKey{name});
    }
  }

  void VisitKeyPath(const ast::KeyPathExpr& path) {
    CaptureIfOuter(path.root);
    for (const auto& seg : path.segs) {
      if (const auto* idx = std::get_if<ast::KeySegIndex>(&seg)) {
        VisitExpr(idx->expr);
      }
    }
  }

  void VisitBlock(const ast::Block& block) {
    PushScope();
    for (const auto& stmt : block.stmts) {
      VisitStmt(stmt);
    }
    VisitExpr(block.tail_opt);
    PopScope();
  }

  void VisitStmt(const ast::Stmt& stmt) {
    std::visit(
        [&](const auto& node) {
          using T = std::decay_t<decltype(node)>;
          if constexpr (std::is_same_v<T, ast::LetStmt> ||
                        std::is_same_v<T, ast::VarStmt>) {
            VisitExpr(node.binding.init);
            DeclarePattern(node.binding.pat);
          } else if constexpr (std::is_same_v<T, ast::ShadowLetStmt> ||
                               std::is_same_v<T, ast::ShadowVarStmt>) {
            VisitExpr(node.init);
            DeclareName(node.name);
          } else if constexpr (std::is_same_v<T, ast::AssignStmt> ||
                               std::is_same_v<T, ast::CompoundAssignStmt>) {
            VisitExpr(node.place);
            VisitExpr(node.value);
          } else if constexpr (std::is_same_v<T, ast::ExprStmt>) {
            VisitExpr(node.value);
          } else if constexpr (std::is_same_v<T, ast::DeferStmt> ||
                               std::is_same_v<T, ast::UnsafeBlockStmt>) {
            if (node.body) {
              VisitBlock(*node.body);
            }
          } else if constexpr (std::is_same_v<T, ast::RegionStmt>) {
            VisitExpr(node.opts_opt);
            if (node.body) {
              PushScope();
              if (node.alias_opt.has_value()) {
                DeclareName(*node.alias_opt);
              }
              for (const auto& inner : node.body->stmts) {
                VisitStmt(inner);
              }
              VisitExpr(node.body->tail_opt);
              PopScope();
            }
          } else if constexpr (std::is_same_v<T, ast::FrameStmt>) {
            if (node.target_opt.has_value()) {
              CaptureIfOuter(*node.target_opt);
            }
            if (node.body) {
              VisitBlock(*node.body);
            }
          } else if constexpr (std::is_same_v<T, ast::ReturnStmt> ||
                               std::is_same_v<T, ast::BreakStmt>) {
            VisitExpr(node.value_opt);
          } else if constexpr (std::is_same_v<T, ast::StaticAssertStmt>) {
            VisitExpr(node.condition);
          } else if constexpr (std::is_same_v<T, ast::KeyBlockStmt>) {
            for (const auto& path : node.paths) {
              VisitKeyPath(path);
            }
            if (node.body) {
              VisitBlock(*node.body);
            }
          }
        },
        stmt);
  }

  void VisitExpr(const ast::ExprPtr& expr) {
    if (!expr) {
      return;
    }

    std::visit(
        [&](const auto& node) {
          using T = std::decay_t<decltype(node)>;
          if constexpr (std::is_same_v<T, ast::IdentifierExpr>) {
            CaptureIfOuter(node.name);
          } else if constexpr (std::is_same_v<T, ast::RangeExpr>) {
            VisitExpr(node.lhs);
            VisitExpr(node.rhs);
          } else if constexpr (std::is_same_v<T, ast::BinaryExpr>) {
            VisitExpr(node.lhs);
            VisitExpr(node.rhs);
          } else if constexpr (std::is_same_v<T, ast::CastExpr>) {
            VisitExpr(node.value);
          } else if constexpr (std::is_same_v<T, ast::UnaryExpr>) {
            VisitExpr(node.value);
          } else if constexpr (std::is_same_v<T, ast::DerefExpr>) {
            VisitExpr(node.value);
          } else if constexpr (std::is_same_v<T, ast::AddressOfExpr>) {
            VisitExpr(node.place);
          } else if constexpr (std::is_same_v<T, ast::MoveExpr>) {
            VisitExpr(node.place);
          } else if constexpr (std::is_same_v<T, ast::AllocExpr>) {
            VisitExpr(node.value);
          } else if constexpr (std::is_same_v<T, ast::TupleExpr> ||
                               std::is_same_v<T, ast::ArrayExpr>) {
            for (const auto& elem : node.elements) {
              VisitExpr(elem);
            }
          } else if constexpr (std::is_same_v<T, ast::ArrayRepeatExpr>) {
            VisitExpr(node.value);
            VisitExpr(node.count);
          } else if constexpr (std::is_same_v<T, ast::RecordExpr>) {
            for (const auto& field : node.fields) {
              VisitExpr(field.value);
            }
          } else if constexpr (std::is_same_v<T, ast::IfExpr>) {
            VisitExpr(node.cond);
            VisitExpr(node.then_expr);
            VisitExpr(node.else_expr);
          } else if constexpr (std::is_same_v<T, ast::IfCaseExpr>) {
            VisitExpr(node.scrutinee);
            for (const auto& arm : node.cases) {
              PushScope();
              DeclarePattern(arm.pattern);
              VisitExpr(arm.body);
              PopScope();
            }
            VisitExpr(node.else_expr);
          } else if constexpr (std::is_same_v<T, ast::IfIsExpr>) {
            VisitExpr(node.scrutinee);
            PushScope();
            DeclarePattern(node.pattern);
            VisitExpr(node.then_expr);
            PopScope();
            VisitExpr(node.else_expr);
          } else if constexpr (std::is_same_v<T, ast::LoopInfiniteExpr>) {
            if (node.invariant_opt.has_value()) {
              VisitExpr(node.invariant_opt->predicate);
            }
            if (node.body) {
              VisitBlock(*node.body);
            }
          } else if constexpr (std::is_same_v<T, ast::LoopConditionalExpr>) {
            VisitExpr(node.cond);
            if (node.invariant_opt.has_value()) {
              VisitExpr(node.invariant_opt->predicate);
            }
            if (node.body) {
              VisitBlock(*node.body);
            }
          } else if constexpr (std::is_same_v<T, ast::LoopIterExpr>) {
            VisitExpr(node.iter);
            PushScope();
            DeclarePattern(node.pattern);
            if (node.invariant_opt.has_value()) {
              VisitExpr(node.invariant_opt->predicate);
            }
            if (node.body) {
              for (const auto& stmt : node.body->stmts) {
                VisitStmt(stmt);
              }
              VisitExpr(node.body->tail_opt);
            }
            PopScope();
          } else if constexpr (std::is_same_v<T, ast::BlockExpr> ||
                               std::is_same_v<T, ast::UnsafeBlockExpr>) {
            if (node.block) {
              VisitBlock(*node.block);
            }
          } else if constexpr (std::is_same_v<T, ast::AttributedExpr>) {
            VisitExpr(node.expr);
          } else if constexpr (std::is_same_v<T, ast::TransmuteExpr>) {
            VisitExpr(node.value);
          } else if constexpr (std::is_same_v<T, ast::ClosureExpr>) {
            PushScope();
            for (const auto& param : node.params) {
              DeclareName(param.name);
            }
            VisitExpr(node.body);
            PopScope();
          } else if constexpr (std::is_same_v<T, ast::PipelineExpr>) {
            VisitExpr(node.lhs);
            VisitExpr(node.rhs);
          } else if constexpr (std::is_same_v<T, ast::FieldAccessExpr>) {
            VisitExpr(node.base);
          } else if constexpr (std::is_same_v<T, ast::TupleAccessExpr>) {
            VisitExpr(node.base);
          } else if constexpr (std::is_same_v<T, ast::IndexAccessExpr>) {
            VisitExpr(node.base);
            VisitExpr(node.index);
          } else if constexpr (std::is_same_v<T, ast::CallExpr>) {
            VisitExpr(node.callee);
            for (const auto& arg : node.args) {
              VisitExpr(arg.value);
            }
          } else if constexpr (std::is_same_v<T, ast::MethodCallExpr>) {
            VisitExpr(node.receiver);
            for (const auto& arg : node.args) {
              VisitExpr(arg.value);
            }
          } else if constexpr (std::is_same_v<T, ast::PropagateExpr>) {
            VisitExpr(node.value);
          } else if constexpr (std::is_same_v<T, ast::EntryExpr>) {
            VisitExpr(node.expr);
          } else if constexpr (std::is_same_v<T, ast::YieldExpr> ||
                               std::is_same_v<T, ast::YieldFromExpr> ||
                               std::is_same_v<T, ast::SyncExpr>) {
            VisitExpr(node.value);
          } else if constexpr (std::is_same_v<T, ast::RaceExpr>) {
            for (const auto& arm : node.arms) {
              VisitExpr(arm.expr);
              if (arm.pattern) {
                PushScope();
                DeclarePattern(arm.pattern);
                VisitExpr(arm.handler.value);
                PopScope();
              } else {
                VisitExpr(arm.handler.value);
              }
            }
          } else if constexpr (std::is_same_v<T, ast::AllExpr>) {
            for (const auto& sub : node.exprs) {
              VisitExpr(sub);
            }
          } else if constexpr (std::is_same_v<T, ast::ParallelExpr>) {
            VisitExpr(node.domain);
            for (const auto& opt : node.opts) {
              VisitExpr(opt.value);
            }
            if (node.body) {
              VisitBlock(*node.body);
            }
          } else if constexpr (std::is_same_v<T, ast::SpawnExpr>) {
            for (const auto& opt : node.opts) {
              VisitExpr(opt.value);
            }
            if (node.body) {
              VisitBlock(*node.body);
            }
          } else if constexpr (std::is_same_v<T, ast::WaitExpr>) {
            VisitExpr(node.handle);
          } else if constexpr (std::is_same_v<T, ast::DispatchExpr>) {
            VisitExpr(node.range);
            if (node.key_clause.has_value()) {
              VisitKeyPath(node.key_clause->key_path);
            }
            for (const auto& opt : node.opts) {
              VisitExpr(opt.chunk_expr);
            }
            PushScope();
            DeclarePattern(node.pattern);
            if (node.body) {
              for (const auto& stmt : node.body->stmts) {
                VisitStmt(stmt);
              }
              VisitExpr(node.body->tail_opt);
            }
            PopScope();
          }
        },
        expr->node);
  }

  const TypeEnv& env_;
  std::vector<std::unordered_set<IdKey>> local_scopes_;
  std::unordered_set<IdKey> captures_;
};

enum class GpuSafeCaptureCheck {
  Safe,
  Unsafe,
  GenericUnbounded,
};

static GpuSafeCaptureCheck MergeGpuSafeCaptureCheck(
    GpuSafeCaptureCheck lhs,
    GpuSafeCaptureCheck rhs) {
  if (lhs == GpuSafeCaptureCheck::GenericUnbounded ||
      rhs == GpuSafeCaptureCheck::GenericUnbounded) {
    return GpuSafeCaptureCheck::GenericUnbounded;
  }
  if (lhs == GpuSafeCaptureCheck::Unsafe ||
      rhs == GpuSafeCaptureCheck::Unsafe) {
    return GpuSafeCaptureCheck::Unsafe;
  }
  return GpuSafeCaptureCheck::Safe;
}

static bool HasGpuSafePredicateForParam(const ast::WhereClause& where_clause,
                                        const std::string& param_name) {
  for (const auto& pred : where_clause.predicates) {
    if (!IdEq(pred.predicate, "GpuSafe") || !pred.type) {
      continue;
    }
    const auto* path = std::get_if<ast::TypePathType>(&pred.type->node);
    if (!path || path->path.size() != 1) {
      continue;
    }
    if (IdEq(path->path[0], param_name)) {
      return true;
    }
  }
  return false;
}

static bool MissingGpuSafeWhereBounds(
    const std::optional<ast::GenericParams>& generic_params_opt,
    const std::optional<ast::WhereClause>& where_clause_opt) {
  if (!generic_params_opt.has_value() || generic_params_opt->params.empty()) {
    return false;
  }
  if (!where_clause_opt.has_value()) {
    return true;
  }
  for (const auto& param : generic_params_opt->params) {
    if (!HasGpuSafePredicateForParam(*where_clause_opt, param.name)) {
      return true;
    }
  }
  return false;
}

static GpuSafeCaptureCheck EvaluateGpuSafeCaptureType(
    const ScopeContext& ctx, const TypeRef& type) {
  const auto stripped = StripPermLocal(type);
  if (!stripped) {
    return GpuSafeCaptureCheck::Safe;
  }

  return std::visit(
      [&](const auto& node) -> GpuSafeCaptureCheck {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, TypePrim> ||
                      std::is_same_v<T, TypeRange> ||
                      std::is_same_v<T, TypeRangeInclusive> ||
                      std::is_same_v<T, TypeRangeFrom> ||
                      std::is_same_v<T, TypeRangeTo> ||
                      std::is_same_v<T, TypeRangeToInclusive> ||
                      std::is_same_v<T, TypeRangeFull> ||
                      std::is_same_v<T, TypeModalState> ||
                      std::is_same_v<T, TypeOpaque>) {
          return GpuSafeCaptureCheck::Safe;
        } else if constexpr (std::is_same_v<T, TypePathType>) {
          if (const auto* record = LookupRecordDecl(ctx, node.path)) {
            if (MissingGpuSafeWhereBounds(record->generic_params,
                                          record->where_clause)) {
              return GpuSafeCaptureCheck::GenericUnbounded;
            }
            return GpuSafeCaptureCheck::Safe;
          }
          if (const auto* enm = LookupEnumDecl(ctx, node.path)) {
            if (MissingGpuSafeWhereBounds(enm->generic_params,
                                          enm->where_clause)) {
              return GpuSafeCaptureCheck::GenericUnbounded;
            }
            return GpuSafeCaptureCheck::Safe;
          }
          return GpuSafeCaptureCheck::Safe;
        } else if constexpr (std::is_same_v<T, TypePtr> ||
                             std::is_same_v<T, TypeRawPtr> ||
                             std::is_same_v<T, TypeDynamic> ||
                             std::is_same_v<T, TypeFunc> ||
                             std::is_same_v<T, TypeClosure> ||
                             std::is_same_v<T, TypeSlice>) {
          return GpuSafeCaptureCheck::Unsafe;
        } else if constexpr (std::is_same_v<T, TypeString> ||
                             std::is_same_v<T, TypeBytes>) {
          return GpuSafeCaptureCheck::Unsafe;
        } else if constexpr (std::is_same_v<T, TypeTuple>) {
          auto status = GpuSafeCaptureCheck::Safe;
          for (const auto& elem : node.elements) {
            status = MergeGpuSafeCaptureCheck(
                status, EvaluateGpuSafeCaptureType(ctx, elem));
          }
          return status;
        } else if constexpr (std::is_same_v<T, TypeArray>) {
          return EvaluateGpuSafeCaptureType(ctx, node.element);
        } else if constexpr (std::is_same_v<T, TypeUnion>) {
          auto status = GpuSafeCaptureCheck::Safe;
          for (const auto& member : node.members) {
            status = MergeGpuSafeCaptureCheck(
                status, EvaluateGpuSafeCaptureType(ctx, member));
          }
          return status;
        } else if constexpr (std::is_same_v<T, TypeRefine>) {
          return EvaluateGpuSafeCaptureType(ctx, node.base);
        } else if constexpr (std::is_same_v<T, TypePerm>) {
          return EvaluateGpuSafeCaptureType(ctx, node.base);
        } else {
          return GpuSafeCaptureCheck::Unsafe;
        }
      },
      stripped->node);
}

static void EmitSupplementalTypeDiag(const StmtTypeContext& type_ctx,
                                     std::string_view code) {
  if (!type_ctx.diags) {
    return;
  }
  if (auto diag = core::MakeDiagnosticById(code)) {
    core::Emit(*type_ctx.diags, *diag);
    return;
  }
  core::Emit(*type_ctx.diags, MakeInternalTypingDiagnostic(
                                  core::Severity::Error,
                                  "Internal error: unresolved diagnostic id '" +
                                      std::string(code) + "'"));
}

}  // namespace

ExprTypeResult TypeParallelExpr(const ScopeContext& ctx,
                                const StmtTypeContext& type_ctx,
                                const ast::ParallelExpr& expr,
                                const TypeEnv& env,
                                const ExprTypeFn& type_expr,
                                const IdentTypeFn& type_ident,
                                const PlaceTypeFn& type_place) {
  SpecDefsParallel();
  ExprTypeResult result;

  if (!expr.domain || !expr.body) {
    return result;
  }

  // 1. Type the domain expression (must be $ExecutionDomain)
  SPEC_RULE("ExecutionDomain");
  const auto domain_type = type_expr(expr.domain);
  if (!domain_type.ok) {
    result.diag_id = domain_type.diag_id;
    return result;
  }

  if (!IsExecutionDomainType(domain_type.type)) {
    // Domain expression must type as $ExecutionDomain.
    result.diag_id = "E-CON-0102";
    return result;
  }

  if (type_ctx.in_parallel &&
      IsGpuDomainType(type_ctx.parallel_domain) &&
      IsGpuDomainType(domain_type.type)) {
    SPEC_RULE("T-GPU-Nested-Err");
    result.diag_id = "T-GPU-Nested-Err";
    return result;
  }

  // 2. Check block options
  SPEC_RULE("ParallelBlockOpts");
  for (const auto& opt : expr.opts) {
    if (!opt.value) {
      continue;
    }

    const auto opt_type = type_expr(opt.value);
    if (!opt_type.ok) {
      result.diag_id = opt_type.diag_id;
      return result;
    }

    switch (opt.kind) {
      case ast::ParallelOptionKind::Name:
        if (!IsStringType(opt_type.type)) {
          result.diag_id = "E-CON-0103";
          return result;
        }
        break;

      case ast::ParallelOptionKind::Cancel:
        if (!IsCancelTokenType(opt_type.type)) {
          result.diag_id = "E-CON-0103";
          return result;
        }
        break;
    }
  }

  // 3. Create parallel context for body typing
  StmtTypeContext parallel_ctx = type_ctx;
  parallel_ctx.in_parallel = true;
  parallel_ctx.parallel_domain = domain_type.type;
  std::unordered_set<IdKey> parallel_ancestor_bindings;
  if (type_ctx.parallel_ancestor_bindings) {
    parallel_ancestor_bindings.insert(type_ctx.parallel_ancestor_bindings->begin(),
                                      type_ctx.parallel_ancestor_bindings->end());
  }
  if (type_ctx.parallel_bindings) {
    parallel_ancestor_bindings.insert(type_ctx.parallel_bindings->begin(),
                                      type_ctx.parallel_bindings->end());
  }
  std::unordered_set<IdKey> parallel_bindings;
  parallel_ctx.parallel_bindings = &parallel_bindings;
  parallel_ctx.parallel_ancestor_bindings = &parallel_ancestor_bindings;
  TypeEnv parallel_env = env;
  parallel_ctx.env_ref = &parallel_env;

  // Rebind recursive typing callbacks to the parallel context so nested
  // spawn/dispatch expressions are checked with in_parallel=true and the
  // evolving statement environment.
  const ExprTypeFn parallel_type_expr = [&](const ast::ExprPtr& inner) {
    return TypeExpr(ctx, parallel_ctx, inner, parallel_env);
  };
  const PlaceTypeFn parallel_type_place = [&](const ast::ExprPtr& inner) {
    return TypePlace(ctx, parallel_ctx, inner, parallel_env);
  };
  const IdentTypeFn parallel_type_ident = [&](std::string_view name) -> ExprTypeResult {
    return TypeIdentifierExpr(ctx, ast::IdentifierExpr{std::string(name)}, parallel_env);
  };

  // 4. Type the body block
  const auto body_info = TypeBlockInfo(ctx, parallel_ctx, *expr.body, parallel_env,
                                       parallel_type_expr, parallel_type_ident,
                                       parallel_type_place, &parallel_env);
  if (!body_info.ok) {
    result.diag_id = body_info.diag_id;
    return result;
  }

  // GPU domains have additional capture restrictions.
  if (IsGpuDomainType(domain_type.type)) {
    const auto captures = ParallelCaptureCollector(env).Collect(*expr.body);
    for (const auto& captured_name : captures) {
      const auto binding = BindOf(env, captured_name);
      if (!binding.has_value()) {
        continue;
      }
      if (PermOfType(binding->type) == Permission::Shared) {
        result.diag_id = "E-CON-0151";
        return result;
      }
      const auto gpu_safe = EvaluateGpuSafeCaptureType(ctx, binding->type);
      if (gpu_safe == GpuSafeCaptureCheck::GenericUnbounded) {
        result.diag_id = "GpuSafe-Generic-Unbounded-Err";
        return result;
      }
      if (gpu_safe == GpuSafeCaptureCheck::Unsafe) {
        EmitSupplementalTypeDiag(type_ctx, "E-TYP-2640");
        result.diag_id = "E-CON-0153";
        return result;
      }
    }
  }

  // A non-collectable tail expression makes the parallel expression result
  // equal to the block body type, matching T-Parallel and BlockInfo-Tail.
  const bool explicit_result =
      expr.body->tail_opt && !IsCollectableParallelExpr(expr.body->tail_opt);
  if (explicit_result) {
    SPEC_RULE("T-Parallel");
    result.ok = true;
    result.type = body_info.type;
    return result;
  }

  // 5. Determine completion result type from collected spawn/dispatch work.
  SPEC_RULE("ForkJoin");
  ParallelResultCollect collected;
  for (const auto& stmt : expr.body->stmts) {
    collected = CollectParallelResultStmt(parallel_type_expr, stmt, std::move(collected));
    if (!collected.ok) {
      result.diag_id = collected.diag_id;
      return result;
    }
  }
  if (expr.body->tail_opt) {
    collected = CollectParallelResultExpr(parallel_type_expr, expr.body->tail_opt,
                                          std::move(collected));
    if (!collected.ok) {
      result.diag_id = collected.diag_id;
      return result;
    }
  }

  if (collected.types.empty()) {
    // No collectable work result.
    SPEC_RULE("T-Parallel-Empty");
    result.ok = true;
    result.type = MakeTypePrim("()");
    return result;
  }

  if (collected.types.size() == 1) {
    // Single collectable result.
    SPEC_RULE("T-Parallel-Single");
    result.ok = true;
    result.type = collected.types.front();
    return result;
  }

  // Multiple collectable results become a tuple in enqueue order.
  SPEC_RULE("T-Parallel-Multi");
  result.ok = true;
  result.type = MakeTypeTuple(std::move(collected.types));

  SPEC_RULE("T-Parallel");
  return result;
}

}  // namespace cursive::analysis
