#include "cursive0/sema/visibility.h"

#include <optional>
#include <type_traits>

#include "cursive0/core/assert_spec.h"
#include "cursive0/core/diagnostic_messages.h"
#include "cursive0/core/diagnostics.h"
#include "cursive0/sema/scopes.h"

namespace cursive0::sema {

namespace {

static inline void SpecDefsVisibility() {
  SPEC_DEF("DeclOf", "5.1.4");
  SPEC_DEF("ModuleOf", "5.1.4");
  SPEC_DEF("ModuleOfPath", "5.1.4");
  SPEC_DEF("Vis", "5.1.4");
  SPEC_DEF("TopLevelDecl", "5.1.4");
}

std::optional<syntax::Visibility> VisOpt(const syntax::ASTItem& item) {
  SpecDefsVisibility();
  return std::visit(
      [](const auto& it) -> std::optional<syntax::Visibility> {
        using T = std::decay_t<decltype(it)>;
        if constexpr (std::is_same_v<T, syntax::ErrorItem>) {
          return std::nullopt;
        } else {
          return it.vis;
        }
      },
      item);
}

bool NameMatches(const IdKey& key, std::string_view candidate) {
  return key == IdKeyOf(candidate);
}

bool PatternBindsName(const syntax::Pattern& pattern, const IdKey& key);

bool PatternBindsName(const syntax::PatternPtr& pattern, const IdKey& key) {
  if (!pattern) {
    return false;
  }
  return PatternBindsName(*pattern, key);
}

bool FieldPatternBindsName(const syntax::FieldPattern& field,
                           const IdKey& key) {
  if (field.pattern_opt) {
    return PatternBindsName(field.pattern_opt, key);
  }
  return NameMatches(key, field.name);
}

bool RecordFieldsBindName(const std::vector<syntax::FieldPattern>& fields,
                          const IdKey& key) {
  for (const auto& field : fields) {
    if (FieldPatternBindsName(field, key)) {
      return true;
    }
  }
  return false;
}

bool PatternBindsName(const syntax::Pattern& pattern, const IdKey& key) {
  return std::visit(
      [&](const auto& node) -> bool {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::IdentifierPattern>) {
          return NameMatches(key, node.name);
        } else if constexpr (std::is_same_v<T, syntax::TypedPattern>) {
          return NameMatches(key, node.name);
        } else if constexpr (std::is_same_v<T, syntax::WildcardPattern> ||
                             std::is_same_v<T, syntax::LiteralPattern>) {
          return false;
        } else if constexpr (std::is_same_v<T, syntax::TuplePattern>) {
          for (const auto& elem : node.elements) {
            if (PatternBindsName(elem, key)) {
              return true;
            }
          }
          return false;
        } else if constexpr (std::is_same_v<T, syntax::RecordPattern>) {
          return RecordFieldsBindName(node.fields, key);
        } else if constexpr (std::is_same_v<T, syntax::EnumPattern>) {
          if (!node.payload_opt) {
            return false;
          }
          return std::visit(
              [&](const auto& payload) -> bool {
                using P = std::decay_t<decltype(payload)>;
                if constexpr (std::is_same_v<P, syntax::TuplePayloadPattern>) {
                  for (const auto& elem : payload.elements) {
                    if (PatternBindsName(elem, key)) {
                      return true;
                    }
                  }
                  return false;
                } else {
                  return RecordFieldsBindName(payload.fields, key);
                }
              },
              *node.payload_opt);
        } else if constexpr (std::is_same_v<T, syntax::ModalPattern>) {
          if (!node.fields_opt) {
            return false;
          }
          return RecordFieldsBindName(node.fields_opt->fields, key);
        } else if constexpr (std::is_same_v<T, syntax::RangePattern>) {
          return PatternBindsName(node.lo, key) || PatternBindsName(node.hi, key);
        } else {
          return false;
        }
      },
      pattern.node);
}

bool UsingClauseBindsName(const syntax::UsingClause& clause,
                          const IdKey& key) {
  return std::visit(
      [&](const auto& node) -> bool {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::UsingPath>) {
          if (node.alias_opt) {
            return NameMatches(key, *node.alias_opt);
          }
          if (node.path.empty()) {
            return false;
          }
          return NameMatches(key, node.path.back());
        } else {
          for (const auto& spec : node.specs) {
            if (spec.alias_opt) {
              if (NameMatches(key, *spec.alias_opt)) {
                return true;
              }
              continue;
            }
            if (NameMatches(key, spec.name)) {
              return true;
            }
          }
          return false;
        }
      },
      clause);
}

bool ItemBindsName(const syntax::ASTItem& item, const IdKey& key) {
  return std::visit(
      [&](const auto& it) -> bool {
        using T = std::decay_t<decltype(it)>;
        if constexpr (std::is_same_v<T, syntax::UsingDecl>) {
          return UsingClauseBindsName(it.clause, key);
        } else if constexpr (std::is_same_v<T, syntax::StaticDecl>) {
          if (!it.binding.pat) {
            return false;
          }
          return PatternBindsName(*it.binding.pat, key);
        } else if constexpr (std::is_same_v<T, syntax::ProcedureDecl> ||
                             std::is_same_v<T, syntax::RecordDecl> ||
                             std::is_same_v<T, syntax::EnumDecl> ||
                             std::is_same_v<T, syntax::ModalDecl> ||
                             std::is_same_v<T, syntax::ClassDecl> ||
                             std::is_same_v<T, syntax::TypeAliasDecl>) {
          return NameMatches(key, it.name);
        } else {
          return false;
        }
      },
      item);
}

const syntax::ASTModule* FindModuleByPath(const Sigma& sigma,
                                          const syntax::ModulePath& path) {
  for (const auto& mod : sigma.mods) {
    if (PathEq(mod.path, path)) {
      return &mod;
    }
  }
  return nullptr;
}

const syntax::ASTItem* FindDeclByName(const ScopeContext& ctx,
                                      const syntax::ModulePath& module_path,
                                      std::string_view name) {
  const auto* module = FindModuleByPath(ctx.sigma, module_path);
  if (!module) {
    return nullptr;
  }
  const auto key = IdKeyOf(name);
  for (const auto& item : module->items) {
    if (ItemBindsName(item, key)) {
      return &item;
    }
  }
  return nullptr;
}

std::optional<core::Span> SpanOfItem(const syntax::ASTItem& item) {
  return std::visit(
      [](const auto& it) -> std::optional<core::Span> { return it.span; },
      item);
}

void EmitDiag(core::DiagnosticStream& diags,
              std::string_view code,
              const std::optional<core::Span>& span) {
  if (auto diag = core::MakeDiagnostic(code, span)) {
    diags = core::Emit(diags, *diag);
  }
}

void CheckExpr(const ScopeContext& ctx,
               const syntax::Expr& expr,
               core::DiagnosticStream& diags);

void CheckExpr(const ScopeContext& ctx,
               const syntax::ExprPtr& expr,
               core::DiagnosticStream& diags) {
  if (!expr) {
    return;
  }
  CheckExpr(ctx, *expr, diags);
}

void CheckArgs(const ScopeContext& ctx,
               const std::vector<syntax::Arg>& args,
               core::DiagnosticStream& diags) {
  for (const auto& arg : args) {
    CheckExpr(ctx, arg.value, diags);
  }
}

void CheckFieldInits(const ScopeContext& ctx,
                     const std::vector<syntax::FieldInit>& fields,
                     core::DiagnosticStream& diags) {
  for (const auto& field : fields) {
    CheckExpr(ctx, field.value, diags);
  }
}

void CheckApplyArgs(const ScopeContext& ctx,
                    const syntax::ApplyArgs& args,
                    core::DiagnosticStream& diags) {
  std::visit(
      [&](const auto& node) {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::ParenArgs>) {
          CheckArgs(ctx, node.args, diags);
        } else {
          CheckFieldInits(ctx, node.fields, diags);
        }
      },
      args);
}

void CheckBlock(const ScopeContext& ctx,
                const syntax::Block& block,
                core::DiagnosticStream& diags);

void CheckStmt(const ScopeContext& ctx,
               const syntax::Stmt& stmt,
               core::DiagnosticStream& diags) {
  std::visit(
      [&](const auto& node) {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::LetStmt> ||
                      std::is_same_v<T, syntax::VarStmt>) {
          CheckExpr(ctx, node.binding.init, diags);
        } else if constexpr (std::is_same_v<T, syntax::ShadowLetStmt> ||
                             std::is_same_v<T, syntax::ShadowVarStmt>) {
          CheckExpr(ctx, node.init, diags);
        } else if constexpr (std::is_same_v<T, syntax::AssignStmt> ||
                             std::is_same_v<T, syntax::CompoundAssignStmt>) {
          CheckExpr(ctx, node.place, diags);
          CheckExpr(ctx, node.value, diags);
        } else if constexpr (std::is_same_v<T, syntax::ExprStmt>) {
          CheckExpr(ctx, node.value, diags);
        } else if constexpr (std::is_same_v<T, syntax::DeferStmt>) {
          if (node.body) {
            CheckBlock(ctx, *node.body, diags);
          }
        } else if constexpr (std::is_same_v<T, syntax::RegionStmt>) {
          CheckExpr(ctx, node.opts_opt, diags);
          if (node.body) {
            CheckBlock(ctx, *node.body, diags);
          }
        } else if constexpr (std::is_same_v<T, syntax::FrameStmt>) {
          if (node.body) {
            CheckBlock(ctx, *node.body, diags);
          }
        } else if constexpr (std::is_same_v<T, syntax::ReturnStmt>) {
          CheckExpr(ctx, node.value_opt, diags);
        } else if constexpr (std::is_same_v<T, syntax::ResultStmt>) {
          CheckExpr(ctx, node.value, diags);
        } else if constexpr (std::is_same_v<T, syntax::BreakStmt>) {
          CheckExpr(ctx, node.value_opt, diags);
        } else if constexpr (std::is_same_v<T, syntax::UnsafeBlockStmt>) {
          if (node.body) {
            CheckBlock(ctx, *node.body, diags);
          }
        } else {
          return;
        }
      },
      stmt);
}

void CheckBlock(const ScopeContext& ctx,
                const syntax::Block& block,
                core::DiagnosticStream& diags) {
  for (const auto& stmt : block.stmts) {
    CheckStmt(ctx, stmt, diags);
  }
  CheckExpr(ctx, block.tail_opt, diags);
}

void CheckExpr(const ScopeContext& ctx,
               const syntax::Expr& expr,
               core::DiagnosticStream& diags) {
  std::visit(
      [&](const auto& node) {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::QualifiedNameExpr>) {
          const auto access = CanAccess(ctx, node.path, node.name);
          if (!access.ok && access.diag_id == "Access-Err") {
            EmitDiag(diags, "E-MOD-1207", expr.span);
          }
        } else if constexpr (std::is_same_v<T, syntax::PathExpr>) {
          return;
        } else if constexpr (std::is_same_v<T, syntax::QualifiedApplyExpr>) {
          const auto access = CanAccess(ctx, node.path, node.name);
          if (!access.ok && access.diag_id == "Access-Err") {
            EmitDiag(diags, "E-MOD-1207", expr.span);
          }
          CheckApplyArgs(ctx, node.args, diags);
        } else if constexpr (std::is_same_v<T, syntax::EnumLiteralExpr>) {
          if (!node.payload_opt) {
            return;
          }
          std::visit(
              [&](const auto& payload) {
                using P = std::decay_t<decltype(payload)>;
                if constexpr (std::is_same_v<P, syntax::EnumPayloadParen>) {
                  for (const auto& elem : payload.elements) {
                    CheckExpr(ctx, elem, diags);
                  }
                } else if constexpr (std::is_same_v<P,
                                                    syntax::EnumPayloadBrace>) {
                  CheckFieldInits(ctx, payload.fields, diags);
                }
              },
              *node.payload_opt);
        } else if constexpr (std::is_same_v<T, syntax::RangeExpr>) {
          CheckExpr(ctx, node.lhs, diags);
          CheckExpr(ctx, node.rhs, diags);
        } else if constexpr (std::is_same_v<T, syntax::BinaryExpr>) {
          CheckExpr(ctx, node.lhs, diags);
          CheckExpr(ctx, node.rhs, diags);
        } else if constexpr (std::is_same_v<T, syntax::CastExpr>) {
          CheckExpr(ctx, node.value, diags);
        } else if constexpr (std::is_same_v<T, syntax::UnaryExpr>) {
          CheckExpr(ctx, node.value, diags);
        } else if constexpr (std::is_same_v<T, syntax::DerefExpr>) {
          CheckExpr(ctx, node.value, diags);
        } else if constexpr (std::is_same_v<T, syntax::AddressOfExpr>) {
          CheckExpr(ctx, node.place, diags);
        } else if constexpr (std::is_same_v<T, syntax::MoveExpr>) {
          CheckExpr(ctx, node.place, diags);
        } else if constexpr (std::is_same_v<T, syntax::AllocExpr>) {
          CheckExpr(ctx, node.value, diags);
        } else if constexpr (std::is_same_v<T, syntax::TupleExpr>) {
          for (const auto& elem : node.elements) {
            CheckExpr(ctx, elem, diags);
          }
        } else if constexpr (std::is_same_v<T, syntax::ArrayExpr>) {
          for (const auto& elem : node.elements) {
            CheckExpr(ctx, elem, diags);
          }
        } else if constexpr (std::is_same_v<T, syntax::RecordExpr>) {
          CheckFieldInits(ctx, node.fields, diags);
        } else if constexpr (std::is_same_v<T, syntax::IfExpr>) {
          CheckExpr(ctx, node.cond, diags);
          CheckExpr(ctx, node.then_expr, diags);
          CheckExpr(ctx, node.else_expr, diags);
        } else if constexpr (std::is_same_v<T, syntax::MatchExpr>) {
          CheckExpr(ctx, node.value, diags);
          for (const auto& arm : node.arms) {
            CheckExpr(ctx, arm.guard_opt, diags);
            CheckExpr(ctx, arm.body, diags);
          }
        } else if constexpr (std::is_same_v<T, syntax::LoopInfiniteExpr>) {
          if (node.body) {
            CheckBlock(ctx, *node.body, diags);
          }
        } else if constexpr (std::is_same_v<T, syntax::LoopConditionalExpr>) {
          CheckExpr(ctx, node.cond, diags);
          if (node.body) {
            CheckBlock(ctx, *node.body, diags);
          }
        } else if constexpr (std::is_same_v<T, syntax::LoopIterExpr>) {
          CheckExpr(ctx, node.iter, diags);
          if (node.body) {
            CheckBlock(ctx, *node.body, diags);
          }
        } else if constexpr (std::is_same_v<T, syntax::BlockExpr>) {
          if (node.block) {
            CheckBlock(ctx, *node.block, diags);
          }
        } else if constexpr (std::is_same_v<T, syntax::UnsafeBlockExpr>) {
          if (node.block) {
            CheckBlock(ctx, *node.block, diags);
          }
        } else if constexpr (std::is_same_v<T, syntax::TransmuteExpr>) {
          CheckExpr(ctx, node.value, diags);
        } else if constexpr (std::is_same_v<T, syntax::FieldAccessExpr>) {
          CheckExpr(ctx, node.base, diags);
        } else if constexpr (std::is_same_v<T, syntax::TupleAccessExpr>) {
          CheckExpr(ctx, node.base, diags);
        } else if constexpr (std::is_same_v<T, syntax::IndexAccessExpr>) {
          CheckExpr(ctx, node.base, diags);
          CheckExpr(ctx, node.index, diags);
        } else if constexpr (std::is_same_v<T, syntax::CallExpr>) {
          CheckExpr(ctx, node.callee, diags);
          CheckArgs(ctx, node.args, diags);
        } else if constexpr (std::is_same_v<T, syntax::MethodCallExpr>) {
          CheckExpr(ctx, node.receiver, diags);
          CheckArgs(ctx, node.args, diags);
        } else if constexpr (std::is_same_v<T, syntax::PropagateExpr>) {
          CheckExpr(ctx, node.value, diags);
        } else {
          return;
        }
      },
      expr.node);
}

void CheckItem(const ScopeContext& ctx,
               const syntax::ASTItem& item,
               core::DiagnosticStream& diags) {
  std::visit(
      [&](const auto& node) {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::StaticDecl>) {
          CheckExpr(ctx, node.binding.init, diags);
        } else if constexpr (std::is_same_v<T, syntax::ProcedureDecl>) {
          if (node.body) {
            CheckBlock(ctx, *node.body, diags);
          }
        } else if constexpr (std::is_same_v<T, syntax::RecordDecl>) {
          for (const auto& member : node.members) {
            if (const auto* field =
                    std::get_if<syntax::FieldDecl>(&member)) {
              CheckExpr(ctx, field->init_opt, diags);
            } else if (const auto* method =
                           std::get_if<syntax::MethodDecl>(&member)) {
              if (method->body) {
                CheckBlock(ctx, *method->body, diags);
              }
            }
          }
        } else if constexpr (std::is_same_v<T, syntax::ModalDecl>) {
          for (const auto& state : node.states) {
            for (const auto& member : state.members) {
              if (const auto* method =
                      std::get_if<syntax::StateMethodDecl>(&member)) {
                if (method->body) {
                  CheckBlock(ctx, *method->body, diags);
                }
              } else if (const auto* transition =
                             std::get_if<syntax::TransitionDecl>(&member)) {
                if (transition->body) {
                  CheckBlock(ctx, *transition->body, diags);
                }
              }
            }
          }
        } else if constexpr (std::is_same_v<T, syntax::ClassDecl>) {
          for (const auto& item : node.items) {
            if (const auto* method =
                    std::get_if<syntax::ClassMethodDecl>(&item)) {
              if (method->body_opt) {
                CheckBlock(ctx, *method->body_opt, diags);
              }
            }
          }
        } else {
          return;
        }
      },
      item);
}

}  // namespace

AccessResult CanAccessVis(const syntax::ModulePath& accessor_module,
                          const syntax::ModulePath& decl_module,
                          syntax::Visibility vis) {
  SpecDefsVisibility();
  switch (vis) {
    case syntax::Visibility::Public:
      SPEC_RULE("Access-Public");
      return {true, std::nullopt};
    case syntax::Visibility::Internal:
      SPEC_RULE("Access-Internal");
      return {true, std::nullopt};
    case syntax::Visibility::Private:
      if (PathEq(accessor_module, decl_module)) {
        SPEC_RULE("Access-Private");
        return {true, std::nullopt};
      }
      SPEC_RULE("Access-Err");
      return {false, "Access-Err"};
    case syntax::Visibility::Protected:
      if (PathEq(accessor_module, decl_module)) {
        SPEC_RULE("Access-Protected");
        return {true, std::nullopt};
      }
      SPEC_RULE("Access-Err");
      return {false, "Access-Err"};
  }
  return {true, std::nullopt};
}

AccessResult CanAccess(const ScopeContext& ctx,
                       const syntax::ModulePath& module_path,
                       std::string_view name) {
  SpecDefsVisibility();
  const auto* item = FindDeclByName(ctx, module_path, name);
  if (!item) {
    return {true, std::nullopt};
  }
  const auto vis = VisOpt(*item);
  if (!vis) {
    return {true, std::nullopt};
  }
  return CanAccessVis(CurrentModule(ctx), module_path, *vis);
}

AccessResult TopLevelVis(const syntax::ASTItem& item) {
  SpecDefsVisibility();
  const auto vis = VisOpt(item);
  if (!vis.has_value()) {
    return {true, std::nullopt};
  }
  if (*vis != syntax::Visibility::Protected) {
    SPEC_RULE("Protected-TopLevel-Ok");
    return {true, std::nullopt};
  }
  SPEC_RULE("Protected-TopLevel-Err");
  return {false, "Protected-TopLevel-Err"};
}

core::DiagnosticStream CheckModuleVisibility(const ScopeContext& ctx,
                                             const syntax::ASTModule& module) {
  SpecDefsVisibility();
  core::DiagnosticStream diags;
  for (const auto& item : module.items) {
    const auto vis = TopLevelVis(item);
    if (!vis.ok && vis.diag_id == "Protected-TopLevel-Err") {
      EmitDiag(diags, "E-MOD-2440", SpanOfItem(item));
    }
    CheckItem(ctx, item, diags);
  }
  return diags;
}

}  // namespace cursive0::sema
