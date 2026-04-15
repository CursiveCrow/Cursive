/*
 * =============================================================================
 * MIGRATION MAPPING: contract_check.cpp
 * =============================================================================
 *
 * SPEC REFERENCE:
 *   - CursiveSpecification.md, Section 14 "Contracts" (line 23181)
 *   - CursiveSpecification.md, Section 14.4 "Contract Syntax" (line 23183)
 *   - CursiveSpecification.md, Section 14.5 "Preconditions and Postconditions" (line 23279)
 *   - CursiveSpecification.md, Section 14.6 "Contract Well-Formedness" (lines 23400-23500)
 *
 * SOURCE FILE:
 *   - cursive-bootstrap/src/03_analysis/contracts/contract_check.cpp (lines 1-263)
 *
 * FUNCTIONS MIGRATED:
 *   - CheckContractWellFormed(Contract* contract) -> bool
 *       Validate contract syntax and semantics
 *   - CheckPrecondition(Precond* pre, ProcDecl* proc) -> bool
 *       Validate precondition references valid parameters
 *   - CheckPostcondition(Postcond* post, ProcDecl* proc) -> bool
 *       Validate postcondition with @result and @entry
 *   - ValidateContractExpression(Expr* expr) -> bool
 *       Ensure contract expression is well-typed boolean
 *   - CheckContractScope(Contract* contract, ProcDecl* proc) -> bool
 *       Validate contract only references in-scope names
 *
 * DEPENDENCIES:
 *   - Contract, Precondition, Postcondition AST nodes
 *   - Expression type checking
 *   - Name resolution
 *
 * REFACTORING NOTES:
 *   1. Contract syntax: |: P (precond), |: P => Q (pre+post), |: => Q (postcond only)
 *   2. @result references return value (postcondition only)
 *   3. @entry(expr) captures entry/old value of expression
 *   4. @entry requires BitcopyType or CloneType
 *   5. Contracts must be boolean expressions
 *   6. Contract predicates must be PURE (see contract_purity.cpp)
 *
 * CONTRACT FORMS:
 *   |: precondition
 *   |: precondition => postcondition
 *   |: => postcondition
 *
 * INTRINSICS:
 *   - @result: Return value (postcondition context only)
 *   - @entry(expr): Captured entry value
 *
 * DIAGNOSTIC CODES:
 *   - E-SEM-2802: Impure expression in contract predicate
 *   - E-SEM-2805: @entry() result type not BitcopyType or CloneType
 *   - E-SEM-2806: @result used outside postcondition
 *   - E-CON-0001: Invalid contract syntax
 *   - E-CON-0002: @result outside postcondition
 *   - E-CON-0003: @entry with non-copyable type
 *   - E-CON-0004: Contract not boolean
 *   - E-CON-0005: Undefined name in contract
 *
 * =============================================================================
 */

#include "04_analysis/contracts/contract_check.h"

#include <optional>
#include <string>
#include <unordered_set>

#include "00_core/assert_spec.h"
#include "04_analysis/caps/cap_requirements.h"
#include "04_analysis/composite/classes.h"
#include "04_analysis/composite/record_methods.h"
#include "04_analysis/contracts/verification.h"
#include "04_analysis/modal/modal.h"
#include "04_analysis/modal/modal_transitions.h"
#include "04_analysis/resolve/scopes_lookup.h"
#include "04_analysis/typing/type_expr.h"
#include "04_analysis/typing/type_lower.h"

namespace cursive::analysis
{

  namespace
  {

    static inline void SpecDefsContractCheck()
    {
      SPEC_DEF("WF-Contract", "C0X.5.W");
      SPEC_DEF("ContractPure", "C0X.5.W");
      SPEC_DEF("PreContext", "C0X.5.W");
      SPEC_DEF("PostContext", "C0X.5.W");
      SPEC_DEF("TypeInvariant", "C0X.5.W");
      SPEC_DEF("LoopInvariant", "C0X.5.W");
      SPEC_DEF("LSP", "C0X.5.W");
    }

    // Check if expression contains @result
    bool ContainsResult(const ast::ExprPtr &expr)
    {
      if (!expr)
        return false;

      if (std::holds_alternative<ast::ResultExpr>(expr->node))
      {
        return true;
      }

      // Recursively check sub-expressions
      return std::visit(
          [](const auto &node) -> bool
          {
            using T = std::decay_t<decltype(node)>;
            if constexpr (std::is_same_v<T, ast::BinaryExpr>)
            {
              return ContainsResult(node.lhs) || ContainsResult(node.rhs);
            }
            else if constexpr (std::is_same_v<T, ast::UnaryExpr>)
            {
              return ContainsResult(node.value);
            }
            else if constexpr (std::is_same_v<T, ast::PipelineExpr>)
            {
              return ContainsResult(node.lhs) || ContainsResult(node.rhs);
            }
            else if constexpr (std::is_same_v<T, ast::CallExpr>)
            {
              if (ContainsResult(node.callee))
                return true;
              for (const auto &arg : node.args)
              {
                if (ContainsResult(arg.value))
                  return true;
              }
              return false;
            }
            else if constexpr (std::is_same_v<T, ast::MethodCallExpr>)
            {
              if (ContainsResult(node.receiver))
                return true;
              for (const auto &arg : node.args)
              {
                if (ContainsResult(arg.value))
                  return true;
              }
              return false;
            }
            return false;
          },
          expr->node);
    }

    // Check if expression contains @entry
    bool ContainsEntry(const ast::ExprPtr &expr)
    {
      if (!expr)
        return false;

      if (std::holds_alternative<ast::EntryExpr>(expr->node))
      {
        return true;
      }

      return std::visit(
          [](const auto &node) -> bool
          {
            using T = std::decay_t<decltype(node)>;
            if constexpr (std::is_same_v<T, ast::BinaryExpr>)
            {
              return ContainsEntry(node.lhs) || ContainsEntry(node.rhs);
            }
            else if constexpr (std::is_same_v<T, ast::UnaryExpr>)
            {
              return ContainsEntry(node.value);
            }
            else if constexpr (std::is_same_v<T, ast::PipelineExpr>)
            {
              return ContainsEntry(node.lhs) || ContainsEntry(node.rhs);
            }
            else if constexpr (std::is_same_v<T, ast::CallExpr>)
            {
              if (ContainsEntry(node.callee))
                return true;
              for (const auto &arg : node.args)
              {
                if (ContainsEntry(arg.value))
                  return true;
              }
              return false;
            }
            return false;
          },
          expr->node);
    }

    void AddImplicationFacts(StaticProofContext &proof_ctx,
                             const ast::ExprPtr &predicate,
                             const core::Span &target_span)
    {
      if (!predicate)
      {
        return;
      }
      if (const auto *binary = std::get_if<ast::BinaryExpr>(&predicate->node);
          binary && binary->op == "&&")
      {
        AddImplicationFacts(proof_ctx, binary->lhs, target_span);
        AddImplicationFacts(proof_ctx, binary->rhs, target_span);
        return;
      }
      // Implication checks are not tied to CFG dominance in a concrete function
      // body, so use the target span for all facts in the proof context.
      AddFact(proof_ctx, predicate, target_span);
    }

    bool PredicateImplies(const ast::ExprPtr &antecedent,
                          const ast::ExprPtr &consequent)
    {
      if (!consequent)
      {
        return true;
      }

      StaticProofContext proof_ctx;
      if (antecedent)
      {
        AddImplicationFacts(proof_ctx, antecedent, consequent->span);
      }

      const auto proof =
          StaticProofAt(proof_ctx, consequent ? consequent->span : core::Span{},
                        consequent);
      return proof.provable;
    }

    const ast::ASTModule *FindModuleByPath(const ScopeContext &ctx,
                                           const ast::ModulePath &path)
    {
      for (const auto &mod : ctx.sigma.mods)
      {
        if (mod.path == path)
        {
          return &mod;
        }
      }
      return nullptr;
    }

    const ast::ProcedureDecl *FindProcedureInModule(const ast::ASTModule &module,
                                                    std::string_view name)
    {
      for (const auto &item : module.items)
      {
        if (const auto *proc = std::get_if<ast::ProcedureDecl>(&item))
        {
          if (IdEq(proc->name, name))
          {
            return proc;
          }
        }
      }
      return nullptr;
    }

    std::optional<const ast::ProcedureDecl *> LookupProcedureForCallee(
        const ScopeContext &ctx,
        const ast::ExprPtr &callee)
    {
      if (!callee)
      {
        return std::nullopt;
      }

      std::string name;
      std::optional<ast::ModulePath> origin;

      if (const auto *ident = std::get_if<ast::IdentifierExpr>(&callee->node))
      {
        const auto ent = ResolveValueName(ctx, ident->name);
        if (ent && ent->origin_opt.has_value())
        {
          origin = *ent->origin_opt;
          name = ent->target_opt.value_or(std::string(ident->name));
        }
        else
        {
          origin = ctx.current_module;
          name = ident->name;
        }
      }
      else if (const auto *qualified =
                   std::get_if<ast::QualifiedNameExpr>(&callee->node))
      {
        origin = qualified->path;
        name = qualified->name;
      }
      else if (const auto *path_expr = std::get_if<ast::PathExpr>(&callee->node))
      {
        origin = path_expr->path.empty() ? ctx.current_module : path_expr->path;
        name = path_expr->name;
      }
      else
      {
        return std::nullopt;
      }

      if (!origin.has_value())
      {
        return std::nullopt;
      }
      const auto *module = FindModuleByPath(ctx, *origin);
      if (!module)
      {
        return std::nullopt;
      }
      const auto *proc = FindProcedureInModule(*module, name);
      if (!proc)
      {
        return std::nullopt;
      }
      return proc;
    }

    bool HasCapabilityParams(const ast::ProcedureDecl &proc)
    {
      for (const auto &param : proc.params)
      {
        if (!param.type)
        {
          continue;
        }
        if (!InferCapabilitiesFromAstType(*param.type).IsEmpty())
        {
          return true;
        }
      }
      return false;
    }

    bool HasCapabilityParams(const std::vector<ast::Param> &params)
    {
      for (const auto &param : params)
      {
        if (!param.type)
        {
          continue;
        }
        if (!InferCapabilitiesFromAstType(*param.type).IsEmpty())
        {
          return true;
        }
      }
      return false;
    }

    struct PurityStack
    {
      std::unordered_set<const ast::ProcedureDecl *> procedures;
      std::unordered_set<const ast::MethodDecl *> record_methods;
      std::unordered_set<const ast::ClassMethodDecl *> class_methods;
      std::unordered_set<const ast::StateMethodDecl *> state_methods;
    };

    bool IsImpureExpr(const ContractContext *ctx,
                      const ast::ExprPtr &expr,
                      PurityStack &purity_stack);

    TypeRef LookupExprTypeFromContext(const ContractContext *ctx,
                                      const ast::ExprPtr &expr)
    {
      if (!ctx || !ctx->scope_ctx || !ctx->scope_ctx->expr_types || !expr)
      {
        return nullptr;
      }
      const auto it = ctx->scope_ctx->expr_types->find(expr.get());
      if (it == ctx->scope_ctx->expr_types->end())
      {
        return nullptr;
      }
      return it->second;
    }

    TypeRef LookupContractBindingType(const ContractContext *ctx,
                                      std::string_view name)
    {
      if (!ctx)
      {
        return nullptr;
      }
      if (IdEq(name, "self"))
      {
        return ctx->receiver_type;
      }
      const auto it = ctx->params.find(std::string(name));
      if (it != ctx->params.end())
      {
        return it->second;
      }
      return nullptr;
    }

    TypeRef InferContractExprType(const ContractContext *ctx,
                                  const ast::ExprPtr &expr)
    {
      if (!expr)
      {
        return nullptr;
      }

      if (TypeRef from_map = LookupExprTypeFromContext(ctx, expr))
      {
        return from_map;
      }

      return std::visit(
          [&](const auto &node) -> TypeRef
          {
            using T = std::decay_t<decltype(node)>;
            if constexpr (std::is_same_v<T, ast::IdentifierExpr>)
            {
              return LookupContractBindingType(ctx, node.name);
            }
            else if constexpr (std::is_same_v<T, ast::ResultExpr>)
            {
              return (ctx && ctx->is_postcondition) ? ctx->return_type : nullptr;
            }
            else if constexpr (std::is_same_v<T, ast::EntryExpr>)
            {
              return InferContractExprType(ctx, node.expr);
            }
            else if constexpr (std::is_same_v<T, ast::AttributedExpr>)
            {
              return InferContractExprType(ctx, node.expr);
            }
            else if constexpr (std::is_same_v<T, ast::CastExpr>)
            {
              if (!ctx || !ctx->scope_ctx || !node.type)
              {
                return nullptr;
              }
              const auto lowered = LowerType(*ctx->scope_ctx, node.type);
              return lowered.ok ? lowered.type : nullptr;
            }
            return nullptr;
          },
          expr->node);
    }

    bool ReceiverIsConst(const ast::Receiver &receiver, const ScopeContext *scope_ctx)
    {
      return std::visit(
          [&](const auto &recv) -> bool
          {
            using R = std::decay_t<decltype(recv)>;
            if constexpr (std::is_same_v<R, ast::ReceiverShorthand>)
            {
              return recv.perm == ast::ReceiverPerm::Const;
            }
            else
            {
              if (!scope_ctx || !recv.type || recv.mode_opt.has_value())
              {
                return false;
              }
              const auto lowered = LowerType(*scope_ctx, recv.type);
              if (!lowered.ok || !lowered.type)
              {
                return false;
              }
              return PermOfType(lowered.type) == Permission::Const;
            }
          },
          receiver);
    }

    TypeRef StripPermOrSelf(const TypeRef &type)
    {
      if (!type)
      {
        return nullptr;
      }
      TypeRef stripped = StripPerm(type);
      return stripped ? stripped : type;
    }

    bool IsPureProcedure(const ScopeContext &scope_ctx,
                         const ast::ProcedureDecl &proc,
                         PurityStack &purity_stack);

    bool IsPureRecordMethod(const ScopeContext &scope_ctx,
                            const ast::MethodDecl &method,
                            const TypeRef &receiver_type,
                            PurityStack &purity_stack);

    bool IsPureClassMethod(const ScopeContext &scope_ctx,
                           const ast::ClassMethodDecl &method,
                           const TypeRef &receiver_type,
                           PurityStack &purity_stack);

    bool IsPureStateMethod(const ScopeContext &scope_ctx,
                           const ast::StateMethodDecl &method,
                           const TypeRef &receiver_type,
                           PurityStack &purity_stack);

    bool IsImpureStmt(const ContractContext *ctx,
                      const ast::Stmt &stmt,
                      PurityStack &purity_stack)
    {
      return std::visit(
          [&](const auto &node) -> bool
          {
            using T = std::decay_t<decltype(node)>;
            if constexpr (std::is_same_v<T, ast::LetStmt> ||
                          std::is_same_v<T, ast::VarStmt>)
            {
              return IsImpureExpr(ctx, node.binding.init, purity_stack);
            }
            else if constexpr (std::is_same_v<T, ast::ShadowLetStmt> ||
                               std::is_same_v<T, ast::ShadowVarStmt>)
            {
              return IsImpureExpr(ctx, node.init, purity_stack);
            }
            else if constexpr (std::is_same_v<T, ast::ExprStmt>)
            {
              return IsImpureExpr(ctx, node.value, purity_stack);
            }
            else if constexpr (std::is_same_v<T, ast::ReturnStmt>)
            {
              return IsImpureExpr(ctx, node.value_opt, purity_stack);
            }
            else if constexpr (std::is_same_v<T, ast::AssignStmt> ||
                               std::is_same_v<T, ast::CompoundAssignStmt> ||
                               std::is_same_v<T, ast::DeferStmt> ||
                               std::is_same_v<T, ast::RegionStmt> ||
                               std::is_same_v<T, ast::FrameStmt> ||
                               std::is_same_v<T, ast::KeyBlockStmt> ||
                               std::is_same_v<T, ast::UnsafeBlockStmt> ||
                               std::is_same_v<T, ast::BreakStmt> ||
                               std::is_same_v<T, ast::ContinueStmt>)
            {
              return true;
            }
            return false;
          },
          stmt);
    }

    bool IsImpureBlock(const ContractContext *ctx,
                       const ast::Block &block,
                       PurityStack &purity_stack)
    {
      for (const auto &stmt : block.stmts)
      {
        if (IsImpureStmt(ctx, stmt, purity_stack))
        {
          return true;
        }
      }
      return IsImpureExpr(ctx, block.tail_opt, purity_stack);
    }

    bool IsPureProcedure(const ScopeContext &scope_ctx,
                         const ast::ProcedureDecl &proc,
                         PurityStack &purity_stack)
    {
      if (HasCapabilityParams(proc) || !proc.body)
      {
        return false;
      }
      if (purity_stack.procedures.find(&proc) != purity_stack.procedures.end())
      {
        // Recursion is allowed for pure procedures. Treat active-cycle calls as
        // provisionally pure and let the enclosing traversal detect concrete
        // impure constructs in the strongly connected body.
        return true;
      }
      purity_stack.procedures.insert(&proc);
      ContractContext proc_ctx;
      proc_ctx.scope_ctx = &scope_ctx;
      for (const auto &param : proc.params)
      {
        if (!param.type)
        {
          continue;
        }
        const auto lowered = LowerType(scope_ctx, param.type);
        if (!lowered.ok || !lowered.type)
        {
          continue;
        }
        proc_ctx.params[param.name] = lowered.type;
      }
      const bool pure = !IsImpureBlock(&proc_ctx, *proc.body, purity_stack);
      purity_stack.procedures.erase(&proc);
      return pure;
    }

    bool IsPureRecordMethod(const ScopeContext &scope_ctx,
                            const ast::MethodDecl &method,
                            const TypeRef &receiver_type,
                            PurityStack &purity_stack)
    {
      if (!method.body || HasCapabilityParams(method.params) ||
          !ReceiverIsConst(method.receiver, &scope_ctx))
      {
        return false;
      }
      if (purity_stack.record_methods.find(&method) !=
          purity_stack.record_methods.end())
      {
        return true;
      }
      purity_stack.record_methods.insert(&method);
      ContractContext method_ctx;
      method_ctx.scope_ctx = &scope_ctx;
      method_ctx.receiver_type = receiver_type;
      for (const auto &param : method.params)
      {
        if (!param.type)
        {
          continue;
        }
        const auto lowered = LowerType(scope_ctx, param.type);
        if (!lowered.ok || !lowered.type)
        {
          continue;
        }
        method_ctx.params[param.name] = lowered.type;
      }
      const bool pure = !IsImpureBlock(&method_ctx, *method.body, purity_stack);
      purity_stack.record_methods.erase(&method);
      return pure;
    }

    bool IsPureClassMethod(const ScopeContext &scope_ctx,
                           const ast::ClassMethodDecl &method,
                           const TypeRef &receiver_type,
                           PurityStack &purity_stack)
    {
      if (!method.body_opt || HasCapabilityParams(method.params) ||
          !ReceiverIsConst(method.receiver, &scope_ctx))
      {
        return false;
      }
      if (purity_stack.class_methods.find(&method) !=
          purity_stack.class_methods.end())
      {
        return true;
      }
      purity_stack.class_methods.insert(&method);
      ContractContext method_ctx;
      method_ctx.scope_ctx = &scope_ctx;
      method_ctx.receiver_type = receiver_type;
      for (const auto &param : method.params)
      {
        if (!param.type)
        {
          continue;
        }
        const auto lowered = LowerType(scope_ctx, param.type);
        if (!lowered.ok || !lowered.type)
        {
          continue;
        }
        method_ctx.params[param.name] = lowered.type;
      }
      const bool pure =
          !IsImpureBlock(&method_ctx, *method.body_opt, purity_stack);
      purity_stack.class_methods.erase(&method);
      return pure;
    }

    bool IsPureStateMethod(const ScopeContext &scope_ctx,
                           const ast::StateMethodDecl &method,
                           const TypeRef &receiver_type,
                           PurityStack &purity_stack)
    {
      if (!method.body || HasCapabilityParams(method.params) ||
          !ReceiverIsConst(method.receiver, &scope_ctx))
      {
        return false;
      }
      if (purity_stack.state_methods.find(&method) !=
          purity_stack.state_methods.end())
      {
        return true;
      }
      purity_stack.state_methods.insert(&method);
      ContractContext method_ctx;
      method_ctx.scope_ctx = &scope_ctx;
      method_ctx.receiver_type = receiver_type;
      for (const auto &param : method.params)
      {
        if (!param.type)
        {
          continue;
        }
        const auto lowered = LowerType(scope_ctx, param.type);
        if (!lowered.ok || !lowered.type)
        {
          continue;
        }
        method_ctx.params[param.name] = lowered.type;
      }
      const bool pure = !IsImpureBlock(&method_ctx, *method.body, purity_stack);
      purity_stack.state_methods.erase(&method);
      return pure;
    }

    bool IsImpureExpr(const ContractContext *ctx,
                      const ast::ExprPtr &expr,
                      PurityStack &purity_stack)
    {
      if (!expr)
      {
        return false;
      }

      return std::visit(
          [&](const auto &node) -> bool
          {
            using T = std::decay_t<decltype(node)>;

            if constexpr (std::is_same_v<T, ast::LiteralExpr> ||
                          std::is_same_v<T, ast::PtrNullExpr> ||
                          std::is_same_v<T, ast::IdentifierExpr> ||
                          std::is_same_v<T, ast::QualifiedNameExpr> ||
                          std::is_same_v<T, ast::PathExpr> ||
                          std::is_same_v<T, ast::ResultExpr>)
            {
              return false;
            }
            else if constexpr (std::is_same_v<T, ast::BinaryExpr>)
            {
              return IsImpureExpr(ctx, node.lhs, purity_stack) ||
                     IsImpureExpr(ctx, node.rhs, purity_stack);
            }
            else if constexpr (std::is_same_v<T, ast::RangeExpr>)
            {
              return IsImpureExpr(ctx, node.lhs, purity_stack) ||
                     IsImpureExpr(ctx, node.rhs, purity_stack);
            }
            else if constexpr (std::is_same_v<T, ast::UnaryExpr>)
            {
              return IsImpureExpr(ctx, node.value, purity_stack);
            }
            else if constexpr (std::is_same_v<T, ast::CastExpr>)
            {
              return IsImpureExpr(ctx, node.value, purity_stack);
            }
            else if constexpr (std::is_same_v<T, ast::PipelineExpr>)
            {
              return IsImpureExpr(ctx, node.lhs, purity_stack) ||
                     IsImpureExpr(ctx, node.rhs, purity_stack);
            }
            else if constexpr (std::is_same_v<T, ast::FieldAccessExpr>)
            {
              return IsImpureExpr(ctx, node.base, purity_stack);
            }
            else if constexpr (std::is_same_v<T, ast::TupleAccessExpr>)
            {
              return IsImpureExpr(ctx, node.base, purity_stack);
            }
            else if constexpr (std::is_same_v<T, ast::IndexAccessExpr>)
            {
              return IsImpureExpr(ctx, node.base, purity_stack) ||
                     IsImpureExpr(ctx, node.index, purity_stack);
            }
            else if constexpr (std::is_same_v<T, ast::AddressOfExpr>)
            {
              return IsImpureExpr(ctx, node.place, purity_stack);
            }
            else if constexpr (std::is_same_v<T, ast::DerefExpr>)
            {
              return IsImpureExpr(ctx, node.value, purity_stack);
            }
            else if constexpr (std::is_same_v<T, ast::IfExpr>)
            {
              return IsImpureExpr(ctx, node.cond, purity_stack) ||
                     IsImpureExpr(ctx, node.then_expr, purity_stack) ||
                     IsImpureExpr(ctx, node.else_expr, purity_stack);
            }
            else if constexpr (std::is_same_v<T, ast::IfCaseExpr>)
            {
              if (IsImpureExpr(ctx, node.scrutinee, purity_stack))
              {
                return true;
              }
              for (const auto &arm : node.cases)
              {
                if (IsImpureExpr(ctx, arm.body, purity_stack))
                {
                  return true;
                }
              }
              return IsImpureExpr(ctx, node.else_expr, purity_stack);
            }
            else if constexpr (std::is_same_v<T, ast::IfIsExpr>)
            {
              return IsImpureExpr(ctx, node.scrutinee, purity_stack) ||
                     IsImpureExpr(ctx, node.then_expr, purity_stack) ||
                     IsImpureExpr(ctx, node.else_expr, purity_stack);
            }
            else if constexpr (std::is_same_v<T, ast::TupleExpr>)
            {
              for (const auto &elem : node.elements)
              {
                if (IsImpureExpr(ctx, elem, purity_stack))
                {
                  return true;
                }
              }
              return false;
            }
            else if constexpr (std::is_same_v<T, ast::ArrayExpr>)
            {
              bool has_impure_subexpr = false;
              ast::ForEachArrayExprSubexpr(node, [&](const ast::ExprPtr &elem)
              {
                if (has_impure_subexpr)
                {
                  return;
                }
                if (IsImpureExpr(ctx, elem, purity_stack))
                {
                  has_impure_subexpr = true;
                }
              });
              return has_impure_subexpr;
            }
            else if constexpr (std::is_same_v<T, ast::ArrayRepeatExpr>)
            {
              return IsImpureExpr(ctx, node.value, purity_stack) ||
                     IsImpureExpr(ctx, node.count, purity_stack);
            }
            else if constexpr (std::is_same_v<T, ast::RecordExpr>)
            {
              for (const auto &field : node.fields)
              {
                if (IsImpureExpr(ctx, field.value, purity_stack))
                {
                  return true;
                }
              }
              return false;
            }
            else if constexpr (std::is_same_v<T, ast::EnumLiteralExpr>)
            {
              if (!node.payload_opt.has_value())
              {
                return false;
              }
              return std::visit(
                  [&](const auto &payload) -> bool
                  {
                    using P = std::decay_t<decltype(payload)>;
                    if constexpr (std::is_same_v<P, ast::EnumPayloadParen>)
                    {
                      for (const auto &elem : payload.elements)
                      {
                        if (IsImpureExpr(ctx, elem, purity_stack))
                        {
                          return true;
                        }
                      }
                      return false;
                    }
                    else
                    {
                      for (const auto &field : payload.fields)
                      {
                        if (IsImpureExpr(ctx, field.value, purity_stack))
                        {
                          return true;
                        }
                      }
                      return false;
                    }
                  },
                  *node.payload_opt);
            }
            else if constexpr (std::is_same_v<T, ast::QualifiedApplyExpr>)
            {
              if (std::holds_alternative<ast::ParenArgs>(node.args))
              {
                const auto &paren = std::get<ast::ParenArgs>(node.args);
                for (const auto &arg : paren.args)
                {
                  if (IsImpureExpr(ctx, arg.value, purity_stack))
                  {
                    return true;
                  }
                }
                return true;
              }
              const auto &brace = std::get<ast::BraceArgs>(node.args);
              for (const auto &field : brace.fields)
              {
                if (IsImpureExpr(ctx, field.value, purity_stack))
                {
                  return true;
                }
              }
              return true;
            }
            else if constexpr (std::is_same_v<T, ast::EntryExpr>)
            {
              return IsImpureExpr(ctx, node.expr, purity_stack);
            }
            else if constexpr (std::is_same_v<T, ast::AttributedExpr>)
            {
              return IsImpureExpr(ctx, node.expr, purity_stack);
            }
            else if constexpr (std::is_same_v<T, ast::SizeofExpr> ||
                               std::is_same_v<T, ast::AlignofExpr>)
            {
              return false;
            }
            else if constexpr (std::is_same_v<T, ast::CallExpr>)
            {
              if (IsImpureExpr(ctx, node.callee, purity_stack))
              {
                return true;
              }
              for (const auto &arg : node.args)
              {
                if (IsImpureExpr(ctx, arg.value, purity_stack))
                {
                  return true;
                }
              }

              if (!ctx || !ctx->scope_ctx)
              {
                return true;
              }
              const auto callee_proc = LookupProcedureForCallee(*ctx->scope_ctx, node.callee);
              if (!callee_proc.has_value() || !*callee_proc)
              {
                return true;
              }
              return !IsPureProcedure(*ctx->scope_ctx, **callee_proc, purity_stack);
            }
            else if constexpr (std::is_same_v<T, ast::MethodCallExpr>)
            {
              if (IsImpureExpr(ctx, node.receiver, purity_stack))
              {
                return true;
              }
              for (const auto &arg : node.args)
              {
                if (IsImpureExpr(ctx, arg.value, purity_stack))
                {
                  return true;
                }
              }

              if (!ctx || !ctx->scope_ctx)
              {
                return true;
              }
              TypeRef receiver_type = InferContractExprType(ctx, node.receiver);
              receiver_type = StripPermOrSelf(receiver_type);
              if (!receiver_type)
              {
                return true;
              }

              if (const auto *modal = std::get_if<TypeModalState>(&receiver_type->node))
              {
                const ast::ModalDecl *decl = LookupModalDecl(*ctx->scope_ctx, modal->path);
                if (!decl)
                {
                  return true;
                }
                if (LookupTransitionDecl(*decl, modal->state, node.name))
                {
                  return true;
                }
                const ast::StateMethodDecl *state_method =
                    LookupStateMethodDecl(*decl, modal->state, node.name);
                if (!state_method)
                {
                  return true;
                }
                return !IsPureStateMethod(*ctx->scope_ctx, *state_method,
                                          receiver_type, purity_stack);
              }

              const auto lookup = LookupMethodStatic(*ctx->scope_ctx, receiver_type, node.name);
              if (!lookup.ok)
              {
                return true;
              }
              if (lookup.record_method)
              {
                return !IsPureRecordMethod(*ctx->scope_ctx, *lookup.record_method,
                                           receiver_type, purity_stack);
              }
              if (lookup.class_method)
              {
                return !IsPureClassMethod(*ctx->scope_ctx, *lookup.class_method,
                                          receiver_type, purity_stack);
              }
              return true;
            }
            else if constexpr (std::is_same_v<T, ast::BlockExpr>)
            {
              return node.block ? IsImpureBlock(ctx, *node.block, purity_stack) : false;
            }
            else if constexpr (std::is_same_v<T, ast::AllocExpr> ||
                               std::is_same_v<T, ast::MoveExpr> ||
                               std::is_same_v<T, ast::YieldExpr> ||
                               std::is_same_v<T, ast::YieldFromExpr> ||
                               std::is_same_v<T, ast::WaitExpr> ||
                               std::is_same_v<T, ast::SyncExpr> ||
                               std::is_same_v<T, ast::SpawnExpr> ||
                               std::is_same_v<T, ast::ParallelExpr> ||
                               std::is_same_v<T, ast::DispatchExpr> ||
                               std::is_same_v<T, ast::TransmuteExpr> ||
                               std::is_same_v<T, ast::UnsafeBlockExpr> ||
                               std::is_same_v<T, ast::LoopInfiniteExpr> ||
                               std::is_same_v<T, ast::LoopConditionalExpr> ||
                               std::is_same_v<T, ast::LoopIterExpr> ||
                               std::is_same_v<T, ast::ClosureExpr> ||
                               std::is_same_v<T, ast::FenceExpr> ||
                               std::is_same_v<T, ast::PropagateExpr>)
            {
              return true;
            }
            return false;
          },
          expr->node);
    }

    // Check if expression is observably mutating/impure in contract context.
    bool IsMutating(const ContractContext *ctx, const ast::ExprPtr &expr)
    {
      PurityStack purity_stack;
      return IsImpureExpr(ctx, expr, purity_stack);
    }

  } // namespace

  ContractCheckResult CheckContractWellFormed(
      const ContractContext &ctx,
      const ast::ContractClause &contract)
  {
    SpecDefsContractCheck();
    SPEC_RULE("WF-Contract");

    ContractCheckResult result;

    // Check precondition
    if (contract.precondition)
    {
      auto pre_result = CheckPrecondition(ctx, contract.precondition);
      if (!pre_result.ok)
      {
        return pre_result;
      }
    }

    // Check postcondition
    if (contract.postcondition)
    {
      ContractContext post_ctx = ctx;
      post_ctx.is_postcondition = true;
      auto post_result = CheckPostcondition(post_ctx, contract.postcondition);
      if (!post_result.ok)
      {
        return post_result;
      }
    }

    return result;
  }

  ContractCheckResult CheckPrecondition(
      const ContractContext &ctx,
      const ast::ExprPtr &expr)
  {
    SpecDefsContractCheck();
    SPEC_RULE("Check-Pre");

    ContractCheckResult result;

    // Precondition must not contain @result or @entry
    if (ContainsResult(expr))
    {
      result.ok = false;
      result.diag_id = "E-SEM-2806"; // @result in precondition
      result.span = expr->span;
      return result;
    }

    if (ContainsEntry(expr))
    {
      result.ok = false;
      result.diag_id = "E-SEM-2852"; // @entry outside postcondition scope
      result.span = expr->span;
      return result;
    }

    // Check purity
    auto purity = CheckPurity(ctx, expr);
    if (!purity.ok)
    {
      return purity;
    }

    return result;
  }

  ContractCheckResult CheckPostcondition(
      const ContractContext &ctx,
      const ast::ExprPtr &expr)
  {
    SpecDefsContractCheck();
    SPEC_RULE("Check-Post");

    ContractCheckResult result;

    // Postcondition may contain @result and @entry
    // Check purity
    auto purity = CheckPurity(ctx, expr);
    if (!purity.ok)
    {
      return purity;
    }

    return result;
  }

  ContractCheckResult CheckPurity(const ast::ExprPtr &expr)
  {
    ContractContext empty_ctx;
    return CheckPurity(empty_ctx, expr);
  }

  ContractCheckResult CheckPurity(const ContractContext &ctx,
                                  const ast::ExprPtr &expr)
  {
    SpecDefsContractCheck();
    SPEC_RULE("ContractPure");

    ContractCheckResult result;

    if (IsMutating(&ctx, expr))
    {
      result.ok = false;
      result.diag_id = "E-SEM-2802"; // Impure expression in contract
      if (expr)
      {
        result.span = expr->span;
      }
    }

    return result;
  }

  TypeInvariantResult CheckTypeInvariant(
      const ContractContext &ctx,
      const ast::TypeInvariant &invariant)
  {
    SpecDefsContractCheck();
    SPEC_RULE("TypeInvariant");

    TypeInvariantResult result;

    // @result is invalid in type invariants (non-return context).
    if (ContainsResult(invariant.predicate))
    {
      result.ok = false;
      result.diag_id = "E-SEM-2854";
      return result;
    }

    // Check predicate purity
    auto purity = CheckPurity(ctx, invariant.predicate);
    if (!purity.ok)
    {
      result.ok = false;
      result.diag_id = "E-SEM-3004";
    }

    (void)ctx;
    return result;
  }

  ContractCheckResult CheckLoopInvariant(
      const ContractContext &ctx,
      const ast::LoopInvariant &invariant)
  {
    SpecDefsContractCheck();
    SPEC_RULE("LoopInvariant");

    ContractCheckResult result;

    // Loop invariants may not contain @result
    if (ContainsResult(invariant.predicate))
    {
      result.ok = false;
      result.diag_id = "E-SEM-2854"; // @result in loop invariant
      result.span = invariant.span;
      return result;
    }

    // Check predicate purity
    auto purity = CheckPurity(ctx, invariant.predicate);
    if (!purity.ok)
    {
      result.ok = false;
      result.diag_id = "E-SEM-3004";
      result.span = purity.span.has_value() ? purity.span : std::optional<core::Span>(invariant.span);
    }

    (void)ctx;
    return result;
  }

  BehavioralSubtypingResult CheckBehavioralSubtyping(
      const ast::ContractClause &class_contract,
      const ast::ContractClause &impl_contract)
  {
    SpecDefsContractCheck();
    SPEC_RULE("LSP");

    BehavioralSubtypingResult result;

    // Per §14.8.1 verification strategy:
    // 1) Verify class precondition implies implementation precondition.
    // 2) Verify implementation postcondition implies class postcondition.
    result.precondition_weaker =
        PredicateImplies(class_contract.precondition, impl_contract.precondition);
    if (!result.precondition_weaker)
    {
      result.ok = false;
      result.diag_id = "E-SEM-2803";
      return result;
    }

    result.postcondition_stronger =
        PredicateImplies(impl_contract.postcondition, class_contract.postcondition);
    if (!result.postcondition_stronger)
    {
      result.ok = false;
      result.diag_id = "E-SEM-2804";
      return result;
    }

    return result;
  }

} // namespace cursive::analysis
