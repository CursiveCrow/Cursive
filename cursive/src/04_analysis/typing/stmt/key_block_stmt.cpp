// =============================================================================
// key_block_stmt.cpp - Key block statement typing
// =============================================================================
//
// SPEC REFERENCE: CursiveSpecification.md
//   Section 17.1: Keys and Synchronization (lines 24089+)
//   - Key block grammar (line 24094)
//   - Key acquisition semantics (line 24123)
//   - Nested keys (line 24450)
//   - Speculative blocks (line 24536)
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/types/type_stmt.cpp
//
// =============================================================================

#include "04_analysis/typing/type_stmt.h"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include "00_core/assert_spec.h"
#include "02_source/ast/ast.h"
#include "04_analysis/caps/cap_concurrency.h"
#include "04_analysis/attributes/attribute_registry.h"
#include "04_analysis/keys/key_lifetimes.h"
#include "04_analysis/typing/type_equiv.h"
#include "04_analysis/typing/type_expr.h"
#include "04_analysis/typing/type_lookup.h"
#include "00_core/diagnostic_messages.h"

namespace cursive::analysis {

namespace {

static inline void SpecDefsKeyBlockStmt() {
  SPEC_DEF("T-KeyBlockStmt", "17.1");
  SPEC_DEF("KeyAcquisition", "17.1");
  SPEC_DEF("KeyRelease", "17.1");
  SPEC_DEF("NestedKeys", "17.1");
  SPEC_DEF("SpeculativeExec", "17.1");
  SPEC_DEF("K-Dynamic-Index-Conflict", "17.3.2");
  SPEC_DEF("K-Static-Required", "17.3.2");
  SPEC_DEF("K-Read-Block-No-Write", "17.2.1");
  SPEC_DEF("K-Read-Write-Reject", "17.2.1");
  SPEC_DEF("KeyBlock-GPU-Err", "17.1");
}

static bool IsDynamicIndexExpr(const ScopeContext& ctx,
                               const ast::ExprPtr& expr) {
  const auto const_len = ConstLen(ctx, expr);
  return !(const_len.ok && const_len.value.has_value());
}

static bool IsMemoryOrderAttributeName(std::string_view name) {
  return name == attrs::kRelaxed ||
         name == attrs::kAcquire ||
         name == attrs::kRelease ||
         name == attrs::kAcqRel ||
         name == attrs::kSeqCst;
}

static void CollectYieldReleasePointsExpr(
    const ast::ExprPtr& expr,
    std::vector<core::Span>& out) {
  if (!expr) {
    return;
  }
  std::visit(
      [&](const auto& node) {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, ast::YieldExpr>) {
          if (node.release) {
            out.push_back(expr->span);
          }
          CollectYieldReleasePointsExpr(node.value, out);
        } else if constexpr (std::is_same_v<T, ast::YieldFromExpr>) {
          if (node.release) {
            out.push_back(expr->span);
          }
          CollectYieldReleasePointsExpr(node.value, out);
        } else if constexpr (std::is_same_v<T, ast::AttributedExpr>) {
          CollectYieldReleasePointsExpr(node.expr, out);
        } else if constexpr (std::is_same_v<T, ast::BinaryExpr>) {
          CollectYieldReleasePointsExpr(node.lhs, out);
          CollectYieldReleasePointsExpr(node.rhs, out);
        } else if constexpr (std::is_same_v<T, ast::UnaryExpr>) {
          CollectYieldReleasePointsExpr(node.value, out);
        } else if constexpr (std::is_same_v<T, ast::FieldAccessExpr>) {
          CollectYieldReleasePointsExpr(node.base, out);
        } else if constexpr (std::is_same_v<T, ast::TupleAccessExpr>) {
          CollectYieldReleasePointsExpr(node.base, out);
        } else if constexpr (std::is_same_v<T, ast::IndexAccessExpr>) {
          CollectYieldReleasePointsExpr(node.base, out);
          CollectYieldReleasePointsExpr(node.index, out);
        } else if constexpr (std::is_same_v<T, ast::CallExpr>) {
          CollectYieldReleasePointsExpr(node.callee, out);
          for (const auto& arg : node.args) {
            CollectYieldReleasePointsExpr(arg.value, out);
          }
        } else if constexpr (std::is_same_v<T, ast::MethodCallExpr>) {
          CollectYieldReleasePointsExpr(node.receiver, out);
          for (const auto& arg : node.args) {
            CollectYieldReleasePointsExpr(arg.value, out);
          }
        } else if constexpr (std::is_same_v<T, ast::TupleExpr>) {
          for (const auto& elem : node.elements) {
            CollectYieldReleasePointsExpr(elem, out);
          }
        } else if constexpr (std::is_same_v<T, ast::ArrayExpr>) {
          for (const auto& elem : node.elements) {
            CollectYieldReleasePointsExpr(elem, out);
          }
        } else if constexpr (std::is_same_v<T, ast::ArrayRepeatExpr>) {
          CollectYieldReleasePointsExpr(node.value, out);
          CollectYieldReleasePointsExpr(node.count, out);
        } else if constexpr (std::is_same_v<T, ast::RecordExpr>) {
          for (const auto& field : node.fields) {
            CollectYieldReleasePointsExpr(field.value, out);
          }
        } else if constexpr (std::is_same_v<T, ast::EnumLiteralExpr>) {
          if (node.payload_opt.has_value()) {
            std::visit(
                [&](const auto& payload) {
                  using P = std::decay_t<decltype(payload)>;
                  if constexpr (std::is_same_v<P, ast::EnumPayloadParen>) {
                    for (const auto& elem : payload.elements) {
                      CollectYieldReleasePointsExpr(elem, out);
                    }
                  } else if constexpr (std::is_same_v<P, ast::EnumPayloadBrace>) {
                    for (const auto& field : payload.fields) {
                      CollectYieldReleasePointsExpr(field.value, out);
                    }
                  }
                },
                *node.payload_opt);
          }
        } else if constexpr (std::is_same_v<T, ast::IfExpr>) {
          CollectYieldReleasePointsExpr(node.cond, out);
          CollectYieldReleasePointsExpr(node.then_expr, out);
          CollectYieldReleasePointsExpr(node.else_expr, out);
        } else if constexpr (std::is_same_v<T, ast::IfCaseExpr>) {
          CollectYieldReleasePointsExpr(node.scrutinee, out);
          for (const auto& case_clause : node.cases) {
            CollectYieldReleasePointsExpr(case_clause.body, out);
          }
          CollectYieldReleasePointsExpr(node.else_expr, out);
        } else if constexpr (std::is_same_v<T, ast::IfIsExpr>) {
          CollectYieldReleasePointsExpr(node.scrutinee, out);
          CollectYieldReleasePointsExpr(node.then_expr, out);
          CollectYieldReleasePointsExpr(node.else_expr, out);
        } else if constexpr (std::is_same_v<T, ast::BlockExpr>) {
          if (node.block) {
            for (const auto& stmt : node.block->stmts) {
              std::visit(
                  [&](const auto& stmt_node) {
                    using ST = std::decay_t<decltype(stmt_node)>;
                    if constexpr (std::is_same_v<ST, ast::ExprStmt>) {
                      CollectYieldReleasePointsExpr(stmt_node.value, out);
                    } else if constexpr (std::is_same_v<ST, ast::LetStmt>) {
                      CollectYieldReleasePointsExpr(stmt_node.binding.init, out);
                    } else if constexpr (std::is_same_v<ST, ast::VarStmt>) {
                      CollectYieldReleasePointsExpr(stmt_node.binding.init, out);
                    } else if constexpr (std::is_same_v<ST, ast::AssignStmt>) {
                      CollectYieldReleasePointsExpr(stmt_node.place, out);
                      CollectYieldReleasePointsExpr(stmt_node.value, out);
                    } else if constexpr (std::is_same_v<ST, ast::ReturnStmt> ||
                                         std::is_same_v<ST, ast::BreakStmt>) {
                      CollectYieldReleasePointsExpr(stmt_node.value_opt, out);
                    }
                  },
                  stmt);
            }
            CollectYieldReleasePointsExpr(node.block->tail_opt, out);
          }
        } else if constexpr (std::is_same_v<T, ast::UnsafeBlockExpr>) {
          if (node.block) {
            for (const auto& stmt : node.block->stmts) {
              if (const auto* expr_stmt = std::get_if<ast::ExprStmt>(&stmt)) {
                CollectYieldReleasePointsExpr(expr_stmt->value, out);
              }
            }
            CollectYieldReleasePointsExpr(node.block->tail_opt, out);
          }
        } else if constexpr (std::is_same_v<T, ast::LoopInfiniteExpr>) {
          if (node.body) {
            for (const auto& stmt : node.body->stmts) {
              if (const auto* expr_stmt = std::get_if<ast::ExprStmt>(&stmt)) {
                CollectYieldReleasePointsExpr(expr_stmt->value, out);
              }
            }
            CollectYieldReleasePointsExpr(node.body->tail_opt, out);
          }
        } else if constexpr (std::is_same_v<T, ast::LoopConditionalExpr>) {
          CollectYieldReleasePointsExpr(node.cond, out);
          if (node.body) {
            for (const auto& stmt : node.body->stmts) {
              if (const auto* expr_stmt = std::get_if<ast::ExprStmt>(&stmt)) {
                CollectYieldReleasePointsExpr(expr_stmt->value, out);
              }
            }
            CollectYieldReleasePointsExpr(node.body->tail_opt, out);
          }
        } else if constexpr (std::is_same_v<T, ast::LoopIterExpr>) {
          CollectYieldReleasePointsExpr(node.iter, out);
          if (node.body) {
            for (const auto& stmt : node.body->stmts) {
              if (const auto* expr_stmt = std::get_if<ast::ExprStmt>(&stmt)) {
                CollectYieldReleasePointsExpr(expr_stmt->value, out);
              }
            }
            CollectYieldReleasePointsExpr(node.body->tail_opt, out);
          }
        }
      },
      expr->node);
}

static std::vector<core::Span> CollectYieldReleasePoints(
    const ast::Block& body) {
  std::vector<core::Span> out;
  for (const auto& stmt : body.stmts) {
    std::visit(
        [&](const auto& node) {
          using T = std::decay_t<decltype(node)>;
          if constexpr (std::is_same_v<T, ast::ExprStmt>) {
            CollectYieldReleasePointsExpr(node.value, out);
          } else if constexpr (std::is_same_v<T, ast::LetStmt>) {
            CollectYieldReleasePointsExpr(node.binding.init, out);
          } else if constexpr (std::is_same_v<T, ast::VarStmt>) {
            CollectYieldReleasePointsExpr(node.binding.init, out);
          } else if constexpr (std::is_same_v<T, ast::AssignStmt>) {
            CollectYieldReleasePointsExpr(node.place, out);
            CollectYieldReleasePointsExpr(node.value, out);
          } else if constexpr (std::is_same_v<T, ast::ReturnStmt> ||
                               std::is_same_v<T, ast::BreakStmt>) {
            CollectYieldReleasePointsExpr(node.value_opt, out);
          }
        },
        stmt);
  }
  CollectYieldReleasePointsExpr(body.tail_opt, out);
  return out;
}

static bool KeyPathHasDynamicIndex(const ScopeContext& ctx,
                                   const ast::KeyPathExpr& path) {
  for (const auto& seg : path.segs) {
    if (const auto* idx = std::get_if<ast::KeySegIndex>(&seg)) {
      if (IsDynamicIndexExpr(ctx, idx->expr)) {
        return true;
      }
    }
  }
  return false;
}

static bool ExtractRootAndIndices(const ast::ExprPtr& expr,
                                  std::string& root,
                                  std::vector<ast::ExprPtr>& indices) {
  if (!expr) {
    return false;
  }
  if (const auto* ident = std::get_if<ast::IdentifierExpr>(&expr->node)) {
    root = ident->name;
    return true;
  }
  if (const auto* field = std::get_if<ast::FieldAccessExpr>(&expr->node)) {
    return ExtractRootAndIndices(field->base, root, indices);
  }
  if (const auto* index = std::get_if<ast::IndexAccessExpr>(&expr->node)) {
    if (!ExtractRootAndIndices(index->base, root, indices)) {
      return false;
    }
    indices.push_back(index->index);
    return true;
  }
  return false;
}

static void CollectIndexAccessesOnRoot(const ast::ExprPtr& expr,
                                       std::string_view root,
                                       std::vector<ast::ExprPtr>& out) {
  if (!expr) {
    return;
  }
  std::visit(
      [&](const auto& node) {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, ast::IndexAccessExpr>) {
          std::string base_root;
          std::vector<ast::ExprPtr> ignored;
          if (ExtractRootAndIndices(node.base, base_root, ignored) &&
              IdEq(base_root, std::string(root))) {
            out.push_back(node.index);
          }
          CollectIndexAccessesOnRoot(node.base, root, out);
          CollectIndexAccessesOnRoot(node.index, root, out);
        } else if constexpr (std::is_same_v<T, ast::FieldAccessExpr>) {
          CollectIndexAccessesOnRoot(node.base, root, out);
        } else if constexpr (std::is_same_v<T, ast::UnaryExpr>) {
          CollectIndexAccessesOnRoot(node.value, root, out);
        } else if constexpr (std::is_same_v<T, ast::BinaryExpr>) {
          CollectIndexAccessesOnRoot(node.lhs, root, out);
          CollectIndexAccessesOnRoot(node.rhs, root, out);
        } else if constexpr (std::is_same_v<T, ast::CallExpr>) {
          CollectIndexAccessesOnRoot(node.callee, root, out);
          for (const auto& arg : node.args) {
            CollectIndexAccessesOnRoot(arg.value, root, out);
          }
        } else if constexpr (std::is_same_v<T, ast::MethodCallExpr>) {
          CollectIndexAccessesOnRoot(node.receiver, root, out);
          for (const auto& arg : node.args) {
            CollectIndexAccessesOnRoot(arg.value, root, out);
          }
        } else if constexpr (std::is_same_v<T, ast::CastExpr>) {
          CollectIndexAccessesOnRoot(node.value, root, out);
        }
      },
      expr->node);
}

static bool BodyWritesRoot(const ast::Block& body, std::string_view root) {
  for (const auto& stmt : body.stmts) {
    if (const auto* assign = std::get_if<ast::AssignStmt>(&stmt)) {
      std::string place_root;
      std::vector<ast::ExprPtr> indices;
      if (ExtractRootAndIndices(assign->place, place_root, indices) &&
          IdEq(place_root, std::string(root))) {
        return true;
      }
      continue;
    }
    if (const auto* compound = std::get_if<ast::CompoundAssignStmt>(&stmt)) {
      std::string place_root;
      std::vector<ast::ExprPtr> indices;
      if (ExtractRootAndIndices(compound->place, place_root, indices) &&
          IdEq(place_root, std::string(root))) {
        return true;
      }
      continue;
    }
  }
  return false;
}

static bool BodyHasDynamicIndexConflict(const ScopeContext& ctx,
                                        const ast::Block& body,
                                        std::string_view root) {
  for (const auto& stmt : body.stmts) {
    const auto* assign = std::get_if<ast::AssignStmt>(&stmt);
    if (!assign) {
      continue;
    }
    std::string place_root;
    std::vector<ast::ExprPtr> lhs_indices;
    if (!ExtractRootAndIndices(assign->place, place_root, lhs_indices) ||
        !IdEq(place_root, std::string(root))) {
      continue;
    }
    bool lhs_dynamic = false;
    for (const auto& idx : lhs_indices) {
      if (IsDynamicIndexExpr(ctx, idx)) {
        lhs_dynamic = true;
        break;
      }
    }
    if (!lhs_dynamic) {
      continue;
    }
    std::vector<ast::ExprPtr> rhs_indices;
    CollectIndexAccessesOnRoot(assign->value, root, rhs_indices);
    for (const auto& rhs_idx : rhs_indices) {
      if (IsDynamicIndexExpr(ctx, rhs_idx)) {
        return true;
      }
    }
  }
  return false;
}

static bool IsGpuDomainType(const TypeRef& type) {
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
  if (!cur) {
    return false;
  }
  if (const auto* dyn = std::get_if<TypeDynamic>(&cur->node)) {
    return IsGpuDomainTypePath(dyn->path);
  }
  if (const auto* path = std::get_if<TypePathType>(&cur->node)) {
    return IsGpuDomainTypePath(path->path);
  }
  return false;
}

static TypeRef StripKeyPathType(const TypeRef& type) {
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

static std::size_t CountKeyMarkers(const ast::KeyPathExpr& path) {
  std::size_t count = path.root_marked ? 1u : 0u;
  for (const auto& seg : path.segs) {
    std::visit(
        [&](const auto& node) {
          if (node.marked) {
            ++count;
          }
        },
        seg);
  }
  return count;
}

static TypeRef AdvanceKeyPathType(const ScopeContext& ctx,
                                  const TypeRef& current,
                                  const ast::KeySeg& seg) {
  if (!current) {
    return current;
  }

  const TypeRef base = StripKeyPathType(current);
  if (!base) {
    return current;
  }

  if (const auto* field = std::get_if<ast::KeySegField>(&seg)) {
    if (const auto* path_type = std::get_if<TypePathType>(&base->node)) {
      const auto* record = LookupRecordDecl(ctx, path_type->path);
      if (record) {
        if (const auto field_type = FieldType(*record, field->name, ctx);
            field_type.has_value()) {
          return *field_type;
        }
      }
    }
    return current;
  }

  const auto* index = std::get_if<ast::KeySegIndex>(&seg);
  if (!index) {
    return current;
  }

  if (const auto* arr = std::get_if<TypeArray>(&base->node)) {
    return arr->element;
  }
  if (const auto* slice = std::get_if<TypeSlice>(&base->node)) {
    return slice->element;
  }
  return current;
}

static std::optional<std::string_view> ValidateKeyPathConformance(
    const ScopeContext& ctx,
    const ast::KeyPathExpr& path,
    const TypeEnv& env) {
  const auto binding = BindOf(env, path.root);
  if (!binding.has_value()) {
    return "E-CON-0031";
  }

  if (PermOfType(binding->type) != Permission::Shared) {
    return "E-CON-0032";
  }

  if (CountKeyMarkers(path) > 1) {
    return "E-CON-0003";
  }

  TypeRef current_type = binding->type;
  for (const auto& seg : path.segs) {
    if (const auto* field = std::get_if<ast::KeySegField>(&seg)) {
      const TypeRef base = StripKeyPathType(current_type);
      bool is_record_base = false;
      if (base) {
        if (const auto* path_type = std::get_if<TypePathType>(&base->node)) {
          is_record_base = LookupRecordDecl(ctx, path_type->path) != nullptr;
        }
      }

      if (field->marked && !is_record_base) {
        return "E-CON-0033";
      }
    }
    current_type = AdvanceKeyPathType(ctx, current_type, seg);
  }

  return std::nullopt;
}

}  // namespace

StmtTypeResult TypeKeyBlockStmt(const ScopeContext& ctx,
                                const StmtTypeContext& type_ctx,
                                const ast::KeyBlockStmt& node,
                                const TypeEnv& env,
                                const ExprTypeFn& type_expr,
                                const IdentTypeFn& type_ident,
                                const PlaceTypeFn& type_place) {
  SpecDefsKeyBlockStmt();
  (void)type_expr;
  (void)type_ident;
  (void)type_place;

  if (!node.body) {
    return {false, std::nullopt, {}, {}};
  }

  bool has_memory_order_attr = false;
  std::size_t memory_order_attr_count = 0;
  if (!node.attrs.empty()) {
    ast::AttributeList statement_attrs;
    ast::AttributeList key_block_attrs;
    for (const auto& attr : node.attrs) {
      if (IsMemoryOrderAttributeName(attr.name)) {
        key_block_attrs.push_back(attr);
        has_memory_order_attr = true;
        ++memory_order_attr_count;
      } else {
        statement_attrs.push_back(attr);
      }
    }

    if (!statement_attrs.empty()) {
      const auto statement_attr_validation =
          ValidateAttributes(statement_attrs, AttributeTarget::Statement);
      if (!statement_attr_validation.ok) {
        return {false, statement_attr_validation.diag_id, {}, {},
                statement_attr_validation.message};
      }
      if (const auto log_diag = ValidateLogAttributesForObservedType(
              ctx, statement_attrs, MakeTypePrim("()"), env)) {
        return {false, log_diag, {}, {}};
      }
    }

    if (!key_block_attrs.empty()) {
      const auto key_block_attr_validation =
          ValidateAttributes(key_block_attrs, AttributeTarget::KeyBlock);
      if (!key_block_attr_validation.ok) {
        return {false, key_block_attr_validation.diag_id, {}, {},
                key_block_attr_validation.message};
      }
      if (memory_order_attr_count > 1) {
        return {false, "E-MOD-2450", {}, {}};
      }
    }
  }

  if (type_ctx.in_parallel && IsGpuDomainType(type_ctx.parallel_domain)) {
    SPEC_RULE("KeyBlock-GPU-Err");
    return {false, "KeyBlock-GPU-Err", {}, {}};
  }

  for (const auto& path : node.paths) {
    if (const auto diag_id = ValidateKeyPathConformance(ctx, path, env);
        diag_id.has_value()) {
      return {false, *diag_id, {}, {}};
    }
  }

  const bool has_dynamic_mod = std::find(node.mods.begin(), node.mods.end(),
                                         ast::KeyBlockMod::Dynamic) !=
                               node.mods.end();
  const bool has_speculative_mod =
      std::find(node.mods.begin(), node.mods.end(),
                ast::KeyBlockMod::Speculative) != node.mods.end();
  const bool has_release_mod = std::find(node.mods.begin(), node.mods.end(),
                                         ast::KeyBlockMod::Release) !=
                               node.mods.end();

  if (has_memory_order_attr && (type_ctx.in_speculative || has_speculative_mod)) {
    return {false, "E-CON-0096", {}, {}};
  }

  if (has_speculative_mod && has_release_mod) {
    return {false, "E-CON-0094", {}, {}};
  }
  if (has_speculative_mod &&
      (!node.mode.has_value() || *node.mode != ast::KeyMode::Write)) {
    return {false, "E-CON-0095", {}, {}};
  }

  std::unordered_set<std::string> keyed_roots;
  bool has_dynamic_key_path = false;
  for (const auto& path : node.paths) {
    keyed_roots.insert(path.root);
    if (KeyPathHasDynamicIndex(ctx, path)) {
      has_dynamic_key_path = true;
    }
  }

  if (has_dynamic_key_path) {
    for (const auto& root : keyed_roots) {
      if (BodyHasDynamicIndexConflict(ctx, *node.body, root)) {
        SPEC_RULE("K-Dynamic-Index-Conflict");
        return {false, "E-CON-0010", {}, {}};
      }
    }
    if (!has_dynamic_mod && !type_ctx.contract_dynamic) {
      SPEC_RULE("K-Static-Required");
      return {false, "E-CON-0020", {}, {}};
    }
  }

  for (const auto& root : keyed_roots) {
    if (!BodyWritesRoot(*node.body, root)) {
      continue;
    }
    if (node.mode.has_value() && *node.mode == ast::KeyMode::Read) {
      SPEC_RULE("K-Read-Block-No-Write");
      return {false, "E-CON-0070", {}, {}};
    }
    if (!node.mode.has_value()) {
      SPEC_RULE("K-Read-Write-Reject");
      return {false, "E-CON-0060", {}, {}};
    }
  }

  // Create a context with keys_held = true for the body.
  // The inner expression typing closures must be rebuilt with this context;
  // otherwise nested expression checks (notably yield) observe the outer
  // context and miss key-held constraints.
  TypeEnv key_env = env;
  StmtTypeContext key_ctx = type_ctx;
  key_ctx.keys_held = true;
  key_ctx.key_mode = node.mode;
  key_ctx.in_speculative = has_speculative_mod;
  key_ctx.env_ref = &key_env;

  auto current_env = [&]() -> const TypeEnv& { return key_env; };
  ExprTypeFn key_type_expr = [&](const ast::ExprPtr& inner) -> ExprTypeResult {
    return TypeExpr(ctx, key_ctx, inner, current_env());
  };
  IdentTypeFn key_type_ident = [&](std::string_view name) -> ExprTypeResult {
    return TypeIdentifierExpr(ctx, ast::IdentifierExpr{std::string(name)},
                              current_env());
  };
  PlaceTypeFn key_type_place = [&](const ast::ExprPtr& inner) -> PlaceTypeResult {
    return TypePlace(ctx, key_ctx, inner, current_env());
  };

  // Type the key block body
  const auto typed = TypeBlock(ctx, key_ctx, *node.body, key_env, key_type_expr,
                               key_type_ident, key_type_place, &key_env);
  if (!typed.ok) {
    return {false, typed.diag_id, {}, {}, typed.diag_detail};
  }

  if (type_ctx.diags) {
    const auto yield_release_points = CollectYieldReleasePoints(*node.body);
    if (!yield_release_points.empty()) {
      const auto stale_warnings = CheckStaleness(*node.body, yield_release_points);
      for (const auto& warning : stale_warnings) {
        if (warning.suppressed) {
          continue;
        }
        if (auto diag = core::MakeDiagnosticById("W-CON-0011", warning.yield_span)) {
          core::Emit(*type_ctx.diags, *diag);
        }
      }
    }
  }

  SPEC_RULE("T-KeyBlockStmt");
  return {true, std::nullopt, env, {}};
}

}  // namespace cursive::analysis
