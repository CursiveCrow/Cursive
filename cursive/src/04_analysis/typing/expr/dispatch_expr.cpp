// =================================================================
// File: 04_analysis/typing/expr/dispatch_expr.cpp
// Construct: Dispatch Expression Type Checking (data parallelism)
// Spec Section: 17.2.3
// Spec Rules: T-Dispatch, T-Dispatch-Reduce
// =================================================================
//
// DISPATCH EXPRESSION (dispatch pat in range key? opts { body }):
//   1. Verify inside parallel block
//   2. Type range expression (must be range type)
//   3. Bind pattern to iteration index
//   4. Process key clause if present
//   5. Process options (reduce, ordered, chunk)
//   6. Type body expression
//   7. Compute result type based on options
//
// DISPATCH OPTIONS:
//   - reduce: op - reduction operation (+, *, min, max, and, or)
//   - ordered - preserve iteration order for side effects
//   - chunk: expr - set chunk size for work distribution
//
// KEY CLAUSE:
//   - key path mode - acquire keys for parallel access
//   - Enables safe parallel mutation
//   - Key pattern determines parallelism
//
// =================================================================

#include "04_analysis/typing/expr/dispatch_expr.h"

#include <optional>
#include <string_view>
#include <type_traits>
#include <unordered_set>
#include <vector>

#include "00_core/assert_spec.h"
#include "04_analysis/typing/type_expr.h"
#include "04_analysis/typing/type_pattern.h"
#include "04_analysis/typing/type_stmt.h"
#include "04_analysis/typing/typecheck.h"

namespace cursive::analysis::expr {

namespace {

static inline void SpecDefsDispatch() {
  SPEC_DEF("T-Dispatch", "17.2.3");
  SPEC_DEF("T-Dispatch-Reduce", "17.2.3");
}

static TypeRef StripPermRefine(const TypeRef& type) {
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

static bool IsUsizeType(const TypeRef& type) {
  const auto stripped = StripPermRefine(type);
  if (!stripped) {
    return false;
  }
  const auto* prim = std::get_if<TypePrim>(&stripped->node);
  return prim && prim->name == "usize";
}

static std::optional<TypeRef> InferDispatchIndexType(
    const TypeRef& range_type) {
  return ::cursive::analysis::RangeElementType(range_type);
}

static std::optional<std::string_view> RootBindingOfPlace(
    const ast::ExprPtr& place) {
  if (!place) {
    return std::nullopt;
  }
  return std::visit(
      [&](const auto& node) -> std::optional<std::string_view> {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, ast::IdentifierExpr>) {
          return std::string_view(node.name);
        } else if constexpr (std::is_same_v<T, ast::PathExpr>) {
          if (node.path.empty()) {
            return std::string_view(node.name);
          }
          return std::nullopt;
        } else if constexpr (std::is_same_v<T, ast::FieldAccessExpr>) {
          return RootBindingOfPlace(node.base);
        } else if constexpr (std::is_same_v<T, ast::TupleAccessExpr>) {
          return RootBindingOfPlace(node.base);
        } else if constexpr (std::is_same_v<T, ast::IndexAccessExpr>) {
          return RootBindingOfPlace(node.base);
        } else if constexpr (std::is_same_v<T, ast::MoveExpr>) {
          return RootBindingOfPlace(node.place);
        } else {
          return std::nullopt;
        }
      },
      place->node);
}

struct DispatchCaptureInfo {
  std::unordered_set<IdKey> captures;
  std::unordered_set<IdKey> explicit_moves;
};

class DispatchCaptureCollector {
 public:
  explicit DispatchCaptureCollector(const TypeEnv& env) : env_(env) {
    local_scopes_.emplace_back();
  }

  DispatchCaptureInfo Collect(const ast::Block& body,
                              const ast::PatternPtr& iter_pattern) {
    PushScope();
    DeclarePattern(iter_pattern);
    for (const auto& stmt : body.stmts) {
      VisitStmt(stmt);
    }
    VisitExpr(body.tail_opt);
    PopScope();
    return {captures_, explicit_moves_};
  }

 private:
  void PushScope() { local_scopes_.emplace_back(); }
  void PopScope() {
    if (!local_scopes_.empty()) {
      local_scopes_.pop_back();
    }
  }

  bool IsLocal(const IdKey& name) const {
    for (auto it = local_scopes_.rbegin(); it != local_scopes_.rend(); ++it) {
      if (it->find(name) != it->end()) {
        return true;
      }
    }
    return false;
  }

  void DeclareName(std::string_view name) {
    if (local_scopes_.empty()) {
      local_scopes_.emplace_back();
    }
    local_scopes_.back().insert(IdKeyOf(name));
  }

  void DeclarePattern(const ast::PatternPtr& pattern) {
    if (!pattern) {
      return;
    }
    std::vector<IdKey> names;
    CollectPatNames(*pattern, names);
    for (const auto& name : names) {
      if (local_scopes_.empty()) {
        local_scopes_.emplace_back();
      }
      local_scopes_.back().insert(name);
    }
  }

  void CaptureIfOuter(std::string_view name) {
    const auto key = IdKeyOf(name);
    if (IsLocal(key)) {
      return;
    }
    if (BindOf(env_, name).has_value()) {
      captures_.insert(key);
    }
  }

  void MarkExplicitMoveIfOuter(const ast::ExprPtr& place) {
    const auto root = RootBindingOfPlace(place);
    if (!root.has_value()) {
      return;
    }
    const auto key = IdKeyOf(*root);
    if (IsLocal(key)) {
      return;
    }
    if (!BindOf(env_, *root).has_value()) {
      return;
    }
    explicit_moves_.insert(key);
  }

  void VisitKeyPath(const ast::KeyPathExpr& path) {
    CaptureIfOuter(path.root);
    for (const auto& seg : path.segs) {
      if (const auto* idx = std::get_if<ast::KeySegIndex>(&seg)) {
        VisitExpr(idx->expr);
      }
    }
  }

  void VisitStmt(const ast::Stmt& stmt) {
    std::visit(
        [&](const auto& node) {
          using T = std::decay_t<decltype(node)>;
          if constexpr (std::is_same_v<T, ast::LetStmt>) {
            VisitExpr(node.binding.init);
            DeclarePattern(node.binding.pat);
          } else if constexpr (std::is_same_v<T, ast::VarStmt>) {
            if (node.binding.init) {
              VisitExpr(node.binding.init);
            }
            DeclarePattern(node.binding.pat);
          } else if constexpr (std::is_same_v<T, ast::ShadowLetStmt>) {
            VisitExpr(node.init);
            DeclareName(node.name);
          } else if constexpr (std::is_same_v<T, ast::ShadowVarStmt>) {
            if (node.init) {
              VisitExpr(node.init);
            }
            DeclareName(node.name);
          } else if constexpr (std::is_same_v<T, ast::AssignStmt> ||
                               std::is_same_v<T, ast::CompoundAssignStmt>) {
            VisitExpr(node.place);
            VisitExpr(node.value);
          } else if constexpr (std::is_same_v<T, ast::ExprStmt>) {
            VisitExpr(node.value);
          } else if constexpr (std::is_same_v<T, ast::ReturnStmt> ||
                               std::is_same_v<T, ast::BreakStmt>) {
            VisitExpr(node.value_opt);
          } else if constexpr (std::is_same_v<T, ast::DeferStmt> ||
                               std::is_same_v<T, ast::UnsafeBlockStmt> ||
                               std::is_same_v<T, ast::RegionStmt> ||
                               std::is_same_v<T, ast::FrameStmt>) {
            if (node.body) {
              VisitBlock(*node.body);
            }
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

  void VisitBlock(const ast::Block& block) {
    PushScope();
    for (const auto& stmt : block.stmts) {
      VisitStmt(stmt);
    }
    VisitExpr(block.tail_opt);
    PopScope();
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
          } else if constexpr (std::is_same_v<T, ast::PathExpr>) {
            if (node.path.empty()) {
              CaptureIfOuter(node.name);
            }
          } else if constexpr (std::is_same_v<T, ast::QualifiedApplyExpr>) {
            if (std::holds_alternative<ast::ParenArgs>(node.args)) {
              const auto& args = std::get<ast::ParenArgs>(node.args).args;
              for (const auto& arg : args) {
                VisitExpr(arg.value);
              }
            } else {
              const auto& fields = std::get<ast::BraceArgs>(node.args).fields;
              for (const auto& field : fields) {
                VisitExpr(field.value);
              }
            }
          } else if constexpr (std::is_same_v<T, ast::BinaryExpr>) {
            VisitExpr(node.lhs);
            VisitExpr(node.rhs);
          } else if constexpr (std::is_same_v<T, ast::UnaryExpr> ||
                               std::is_same_v<T, ast::CastExpr> ||
                               std::is_same_v<T, ast::DerefExpr> ||
                               std::is_same_v<T, ast::PropagateExpr> ||
                               std::is_same_v<T, ast::AllocExpr> ||
                               std::is_same_v<T, ast::TransmuteExpr> ||
                               std::is_same_v<T, ast::YieldExpr> ||
                               std::is_same_v<T, ast::YieldFromExpr> ||
                               std::is_same_v<T, ast::SyncExpr>) {
            VisitExpr(node.value);
          } else if constexpr (std::is_same_v<T, ast::EntryExpr>) {
            VisitExpr(node.expr);
          } else if constexpr (std::is_same_v<T, ast::AddressOfExpr>) {
            VisitExpr(node.place);
          } else if constexpr (std::is_same_v<T, ast::MoveExpr>) {
            MarkExplicitMoveIfOuter(node.place);
            VisitExpr(node.place);
          } else if constexpr (std::is_same_v<T, ast::IfExpr>) {
            VisitExpr(node.cond);
            VisitExpr(node.then_expr);
            VisitExpr(node.else_expr);
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
          } else if constexpr (std::is_same_v<T, ast::EnumLiteralExpr>) {
            if (!node.payload_opt.has_value()) {
              return;
            }
            std::visit(
                [&](const auto& payload) {
                  using P = std::decay_t<decltype(payload)>;
                  if constexpr (std::is_same_v<P, ast::EnumPayloadParen>) {
                    for (const auto& elem : payload.elements) {
                      VisitExpr(elem);
                    }
                  } else {
                    for (const auto& field : payload.fields) {
                      VisitExpr(field.value);
                    }
                  }
                },
                *node.payload_opt);
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
          } else if constexpr (std::is_same_v<T, ast::FieldAccessExpr> ||
                               std::is_same_v<T, ast::TupleAccessExpr>) {
            VisitExpr(node.base);
          } else if constexpr (std::is_same_v<T, ast::IndexAccessExpr>) {
            VisitExpr(node.base);
            VisitExpr(node.index);
          } else if constexpr (std::is_same_v<T, ast::IfCaseExpr>) {
            VisitExpr(node.scrutinee);
            for (const auto& case_clause : node.cases) {
              PushScope();
              DeclarePattern(case_clause.pattern);
              VisitExpr(case_clause.body);
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
          }
        },
        expr->node);
  }

  const TypeEnv& env_;
  std::vector<std::unordered_set<IdKey>> local_scopes_;
  std::unordered_set<IdKey> captures_;
  std::unordered_set<IdKey> explicit_moves_;
};

static bool BodyMutatesSharedBinding(const ast::Block& body,
                                     const TypeEnv& env) {
  auto mutates_place = [&](const ast::ExprPtr& place) {
    const auto root = RootBindingOfPlace(place);
    if (!root.has_value()) {
      return false;
    }
    const auto binding = BindOf(env, *root);
    if (!binding.has_value()) {
      return false;
    }
    return PermOfType(binding->type) == Permission::Shared;
  };

  for (const auto& stmt : body.stmts) {
    if (const auto* assign = std::get_if<ast::AssignStmt>(&stmt)) {
      if (mutates_place(assign->place)) {
        return true;
      }
    } else if (const auto* compound =
                   std::get_if<ast::CompoundAssignStmt>(&stmt)) {
      if (mutates_place(compound->place)) {
        return true;
      }
    }
  }
  return false;
}

static bool ExprUsesAnyName(const ast::ExprPtr& expr,
                            const std::unordered_set<IdKey>& names) {
  if (!expr || names.empty()) {
    return false;
  }
  return std::visit(
      [&](const auto& node) -> bool {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, ast::IdentifierExpr>) {
          return names.find(IdKeyOf(node.name)) != names.end();
        } else if constexpr (std::is_same_v<T, ast::PathExpr>) {
          return node.path.empty() &&
                 names.find(IdKeyOf(node.name)) != names.end();
        } else if constexpr (std::is_same_v<T, ast::BinaryExpr>) {
          return ExprUsesAnyName(node.lhs, names) ||
                 ExprUsesAnyName(node.rhs, names);
        } else if constexpr (std::is_same_v<T, ast::UnaryExpr>) {
          return ExprUsesAnyName(node.value, names);
        } else if constexpr (std::is_same_v<T, ast::CallExpr>) {
          if (ExprUsesAnyName(node.callee, names)) {
            return true;
          }
          for (const auto& arg : node.args) {
            if (ExprUsesAnyName(arg.value, names)) {
              return true;
            }
          }
          return false;
        } else if constexpr (std::is_same_v<T, ast::MethodCallExpr>) {
          if (ExprUsesAnyName(node.receiver, names)) {
            return true;
          }
          for (const auto& arg : node.args) {
            if (ExprUsesAnyName(arg.value, names)) {
              return true;
            }
          }
          return false;
        } else if constexpr (std::is_same_v<T, ast::FieldAccessExpr>) {
          return ExprUsesAnyName(node.base, names);
        } else if constexpr (std::is_same_v<T, ast::TupleAccessExpr>) {
          return ExprUsesAnyName(node.base, names);
        } else if constexpr (std::is_same_v<T, ast::IndexAccessExpr>) {
          return ExprUsesAnyName(node.base, names) ||
                 ExprUsesAnyName(node.index, names);
        } else if constexpr (std::is_same_v<T, ast::CastExpr>) {
          return ExprUsesAnyName(node.value, names);
        } else if constexpr (std::is_same_v<T, ast::IfExpr>) {
          return ExprUsesAnyName(node.cond, names) ||
                 ExprUsesAnyName(node.then_expr, names) ||
                 ExprUsesAnyName(node.else_expr, names);
        } else if constexpr (std::is_same_v<T, ast::IfCaseExpr>) {
          if (ExprUsesAnyName(node.scrutinee, names)) {
            return true;
          }
          for (const auto& case_clause : node.cases) {
            if (ExprUsesAnyName(case_clause.body, names)) {
              return true;
            }
          }
          return ExprUsesAnyName(node.else_expr, names);
        } else if constexpr (std::is_same_v<T, ast::IfIsExpr>) {
          return ExprUsesAnyName(node.scrutinee, names) ||
                 ExprUsesAnyName(node.then_expr, names) ||
                 ExprUsesAnyName(node.else_expr, names);
        } else if constexpr (std::is_same_v<T, ast::RangeExpr>) {
          return ExprUsesAnyName(node.lhs, names) ||
                 ExprUsesAnyName(node.rhs, names);
        } else if constexpr (std::is_same_v<T, ast::TupleExpr>) {
          for (const auto& elem : node.elements) {
            if (ExprUsesAnyName(elem, names)) {
              return true;
            }
          }
          return false;
        } else if constexpr (std::is_same_v<T, ast::ArrayExpr>) {
          for (const auto& elem : node.elements) {
            if (ExprUsesAnyName(elem, names)) {
              return true;
            }
          }
          return false;
        } else if constexpr (std::is_same_v<T, ast::ArrayRepeatExpr>) {
          return ExprUsesAnyName(node.value, names) ||
                 ExprUsesAnyName(node.count, names);
        } else if constexpr (std::is_same_v<T, ast::RecordExpr>) {
          for (const auto& field : node.fields) {
            if (ExprUsesAnyName(field.value, names)) {
              return true;
            }
          }
          return false;
        } else if constexpr (std::is_same_v<T, ast::EntryExpr>) {
          return ExprUsesAnyName(node.expr, names);
        } else {
          return false;
        }
      },
      expr->node);
}

static bool KeyClauseDependsOnLoopVariable(
    const std::optional<ast::DispatchKeyClause>& key_clause,
    const ast::PatternPtr& pattern) {
  if (!key_clause || !pattern) {
    return false;
  }

  std::vector<IdKey> pattern_names;
  CollectPatNames(*pattern, pattern_names);
  std::unordered_set<IdKey> pattern_name_set;
  for (const auto& name : pattern_names) {
    pattern_name_set.insert(name);
  }
  if (pattern_name_set.empty()) {
    return false;
  }

  for (const auto& seg : key_clause->key_path.segs) {
    if (const auto* index = std::get_if<ast::KeySegIndex>(&seg)) {
      if (ExprUsesAnyName(index->expr, pattern_name_set)) {
        return true;
      }
    }
  }
  return false;
}

static bool IsAssociativeReduce(const ast::DispatchOption& opt) {
  if (opt.kind != ast::DispatchOptionKind::Reduce) {
    return true;
  }
  if (opt.reduce_op == ast::ReduceOp::Custom ||
      !opt.custom_reduce_name.empty()) {
    // User-defined reducers are not statically known to be associative.
    return false;
  }
  switch (opt.reduce_op) {
    case ast::ReduceOp::Add:
    case ast::ReduceOp::Mul:
    case ast::ReduceOp::Min:
    case ast::ReduceOp::Max:
    case ast::ReduceOp::And:
    case ast::ReduceOp::Or:
      return true;
    case ast::ReduceOp::Custom:
      return false;
  }
  return false;
}

}  // namespace

ExprTypeResult TypeDispatchExprImpl(const ScopeContext& ctx,
                                    const StmtTypeContext& type_ctx,
                                    const ast::DispatchExpr& expr,
                                    const TypeEnv& env,
                                    const TypeExprFn& type_expr,
                                    const TypeIdentFn& type_ident,
                                    const PlaceTypeFn& type_place) {
  SpecDefsDispatch();
  SPEC_RULE("T-Dispatch");
  ExprTypeResult result;

  // Check that we're inside a parallel block (section 18.5.1)
  if (!type_ctx.in_parallel) {
    result.diag_id = "E-CON-0140";  // dispatch without enclosing parallel block
    return result;
  }

  // Type check range expression
  ExprTypeResult range_result = type_expr(expr.range);
  if (!range_result.ok) {
    result.diag_id = range_result.diag_id;
    return result;
  }

  // Check range is a range family type.
  const auto stripped_range = StripPerm(range_result.type);
  if (!::cursive::analysis::IsRangeType(stripped_range)) {
    result.diag_id = "Assign-Type-Err";
    return result;
  }
  if (stripped_range &&
      std::holds_alternative<TypeRangeFull>(stripped_range->node)) {
    // Unbounded dispatch ranges are not finite iteration domains.
    result.diag_id = "Assign-Type-Err";
    return result;
  }

  // Infer the iteration variable type from the range element type.
  TypeRef index_type = InferDispatchIndexType(stripped_range).value_or(
      MakeTypePrim("usize"));

  // Validate key clause root binding (section 18.5.1)
  if (expr.key_clause.has_value()) {
    const auto binding = BindOf(env, expr.key_clause->key_path.root);
    if (!binding.has_value()) {
      result.diag_id = "ResolveExpr-Ident-Err";
      return result;
    }
  }

  // Add pattern binding to environment
  TypeEnv body_env = PushScope(env);
  if (expr.pattern) {
    PatternTypeResult pat_result = TypePattern(ctx, expr.pattern, index_type);
    if (!pat_result.ok) {
      result.diag_id = pat_result.diag_id;
      return result;
    }
    // Add bindings to environment
    for (const auto& [name, type] : pat_result.bindings) {
      body_env.scopes.back()[name] = TypeBinding{ast::Mutability::Let, type};
    }
  }

  // Type check body
  if (!expr.body) {
    result.ok = true;
    result.type = MakeTypePrim("()");
    return result;
  }

  ExprTypeResult body_result = TypeBlock(ctx, type_ctx, *expr.body, body_env,
                                         type_expr, type_ident, type_place);
  if (!body_result.ok) {
    result.diag_id = body_result.diag_id;
    return result;
  }

  // Capture analysis (section 18.3):
  // - unique captures require explicit move (E-CON-0120)
  // - explicit move from an outer parallel scope is forbidden (E-CON-0122)
  const auto capture_info =
      DispatchCaptureCollector(env).Collect(*expr.body, expr.pattern);
  for (const auto& captured_name : capture_info.captures) {
    const auto binding = BindOf(env, captured_name);
    if (!binding.has_value()) {
      continue;
    }

    const bool is_explicit_move =
        capture_info.explicit_moves.find(captured_name) !=
        capture_info.explicit_moves.end();

    if (is_explicit_move && type_ctx.parallel_ancestor_bindings &&
        type_ctx.parallel_ancestor_bindings->find(captured_name) !=
            type_ctx.parallel_ancestor_bindings->end()) {
      result.diag_id = "E-CON-0122";
      return result;
    }

    if (PermOfType(binding->type) == Permission::Unique && !is_explicit_move) {
      result.diag_id = "E-CON-0120";
      return result;
    }
  }

  bool has_ordered = false;
  bool has_reduce = false;
  bool non_associative_reduce = false;
  for (const auto& opt : expr.opts) {
    if (opt.kind == ast::DispatchOptionKind::Ordered) {
      has_ordered = true;
      continue;
    }
    if (opt.kind == ast::DispatchOptionKind::Chunk) {
      const auto chunk_typed = type_expr(opt.chunk_expr);
      if (!chunk_typed.ok) {
        result.diag_id = chunk_typed.diag_id;
        return result;
      }
      if (!IsUsizeType(chunk_typed.type)) {
        result.diag_id = "Assign-Type-Err";
        return result;
      }
      continue;
    }
    if (opt.kind == ast::DispatchOptionKind::Reduce) {
      has_reduce = true;
      if (!IsAssociativeReduce(opt)) {
        non_associative_reduce = true;
      }
    }
  }

  if (non_associative_reduce && !has_ordered) {
    result.diag_id = "E-CON-0143";
    return result;
  }

  const bool mutates_shared =
      expr.body ? BodyMutatesSharedBinding(*expr.body, env) : false;
  if (mutates_shared) {
    if (!expr.key_clause.has_value()) {
      result.diag_id = "E-CON-0141";
      return result;
    }
    if (!KeyClauseDependsOnLoopVariable(expr.key_clause, expr.pattern)) {
      result.diag_id = "E-CON-0142";
      return result;
    }
  }

  if (has_reduce) {
    SPEC_RULE("T-Dispatch-Reduce");
    result.ok = true;
    result.type = body_result.type;
    return result;
  }

  // Basic dispatch returns unit
  result.ok = true;
  result.type = MakeTypePrim("()");
  return result;
}

}  // namespace cursive::analysis::expr
