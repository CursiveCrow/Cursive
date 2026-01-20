#include "cursive0/semantics/init.h"

#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "cursive0/sema/collect_toplevel.h"
#include "cursive0/sema/init_planner.h"
#include "cursive0/semantics/exec.h"
#include "cursive0/semantics/eval.h"

namespace cursive0::semantics {

namespace {

Outcome PanicOutcome() {
  Control ctrl;
  ctrl.kind = ControlKind::Panic;
  ctrl.value = std::nullopt;
  return MakeCtrl(ctrl);
}

bool IsMoveExpr(const syntax::Expr& expr) {
  return std::holds_alternative<syntax::MoveExpr>(expr.node);
}

bool IsPlaceExpr(const syntax::Expr& expr) {
  return std::visit(
      [&](const auto& node) -> bool {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::IdentifierExpr>) {
          return true;
        } else if constexpr (std::is_same_v<T, syntax::FieldAccessExpr>) {
          return node.base ? IsPlaceExpr(*node.base) : false;
        } else if constexpr (std::is_same_v<T, syntax::TupleAccessExpr>) {
          return node.base ? IsPlaceExpr(*node.base) : false;
        } else if constexpr (std::is_same_v<T, syntax::IndexAccessExpr>) {
          return node.base ? IsPlaceExpr(*node.base) : false;
        } else if constexpr (std::is_same_v<T, syntax::DerefExpr>) {
          return node.value ? IsPlaceExpr(*node.value) : false;
        }
        return false;
      },
      expr.node);
}

Responsibility RespOfInit(const syntax::Expr& init) {
  if (!IsPlaceExpr(init)) {
    return Responsibility::Resp;
  }
  if (IsMoveExpr(init)) {
    return Responsibility::Resp;
  }
  return Responsibility::Alias;
}

Movability MovOf(const syntax::Token& op) {
  if (op.lexeme == ":=") {
    return Movability::Immov;
  }
  return Movability::Mov;
}

BindInfo BindInfoForBinding(const syntax::Binding& binding) {
  BindInfo info;
  info.mov = MovOf(binding.op);
  if (binding.init) {
    info.resp = RespOfInit(*binding.init);
  }
  return info;
}

bool StaticHasResp(const syntax::StaticDecl& decl) {
  if (!decl.binding.init) {
    return false;
  }
  const auto info = BindInfoForBinding(decl.binding);
  return info.resp == Responsibility::Resp;
}

void EnsurePoisonFlag(Sigma& sigma, const sema::PathKey& path) {
  if (sigma.poison_flags.find(path) != sigma.poison_flags.end()) {
    return;
  }
  const Addr addr = AllocAddr(sigma);
  sigma.poison_flags[path] = addr;
  BoolVal value;
  value.value = false;
  WriteAddr(sigma, addr, Value{value});
}

void SetPoisonFlag(Sigma& sigma, const sema::PathKey& path) {
  EnsurePoisonFlag(sigma, path);
  const auto it = sigma.poison_flags.find(path);
  if (it == sigma.poison_flags.end()) {
    return;
  }
  BoolVal value;
  value.value = true;
  WriteAddr(sigma, it->second, Value{value});
}

void EnsureStaticAddr(Sigma& sigma,
                      const sema::PathKey& path,
                      std::string_view name) {
  const auto key = std::make_pair(path, std::string(name));
  if (sigma.static_addrs.find(key) != sigma.static_addrs.end()) {
    return;
  }
  const Addr addr = AllocAddr(sigma);
  sigma.static_addrs.emplace(key, addr);
  WriteAddr(sigma, addr, Value{UnitVal{}});
}

std::optional<const syntax::ASTModule*> FindModule(
    const sema::ScopeContext& ctx,
    const syntax::ModulePath& path) {
  for (const auto& mod : ctx.sigma.mods) {
    if (sema::PathKeyOf(mod.path) == sema::PathKeyOf(path)) {
      return &mod;
    }
  }
  return std::nullopt;
}

std::set<sema::PathKey> PoisonSetForInit(
    const sema::InitPlan& plan,
    const syntax::ModulePath& module_path) {
  std::set<sema::PathKey> out;
  if (plan.graph.modules.empty()) {
    out.insert(sema::PathKeyOf(module_path));
    return out;
  }
  std::size_t target = plan.graph.modules.size();
  for (std::size_t i = 0; i < plan.graph.modules.size(); ++i) {
    if (sema::PathKeyOf(plan.graph.modules[i]) == sema::PathKeyOf(module_path)) {
      target = i;
      break;
    }
  }
  if (target == plan.graph.modules.size()) {
    out.insert(sema::PathKeyOf(module_path));
    return out;
  }
  const std::size_t n = plan.graph.modules.size();
  std::vector<std::vector<std::size_t>> incoming(n);
  for (const auto& edge : plan.graph.eager_edges) {
    if (edge.first < n && edge.second < n) {
      incoming[edge.second].push_back(edge.first);
    }
  }
  std::vector<char> visited(n, false);
  std::vector<std::size_t> stack;
  visited[target] = true;
  stack.push_back(target);
  while (!stack.empty()) {
    const auto cur = stack.back();
    stack.pop_back();
    for (const auto pred : incoming[cur]) {
      if (!visited[pred]) {
        visited[pred] = true;
        stack.push_back(pred);
      }
    }
  }
  for (std::size_t i = 0; i < n; ++i) {
    if (visited[i]) {
      out.insert(sema::PathKeyOf(plan.graph.modules[i]));
    }
  }
  if (out.empty()) {
    out.insert(sema::PathKeyOf(module_path));
  }
  return out;
}

}  // namespace

InitResult Init(const SemanticsContext& ctx, Sigma& sigma) {
  InitResult result;
  if (!ctx.sema || !ctx.name_maps) {
    return result;
  }
  const auto plan_result = sema::BuildInitPlan(*ctx.sema, *ctx.name_maps);
  if (!plan_result.ok || !plan_result.plan.topo_ok) {
    return result;
  }
  const auto& plan = plan_result.plan;
  SPEC_RULE("Init-Start");

  for (const auto& module_path : plan.graph.modules) {
    EnsurePoisonFlag(sigma, sema::PathKeyOf(module_path));
  }

  for (const auto& mod : ctx.sema->sigma.mods) {
    const auto module_key = sema::PathKeyOf(mod.path);
    for (const auto& item : mod.items) {
      const auto* decl = std::get_if<syntax::StaticDecl>(&item);
      if (!decl) {
        continue;
      }
      const auto names = sema::PatNames(*decl->binding.pat);
      for (const auto& name : names) {
        EnsureStaticAddr(sigma, module_key, name);
      }
    }
  }

  for (std::size_t mod_index = 0; mod_index < plan.init_order.size(); ++mod_index) {
    const auto& module_path = plan.init_order[mod_index];
    SPEC_RULE("Init-Next-Module");
    const auto mod_opt = FindModule(*ctx.sema, module_path);
    if (!mod_opt.has_value()) {
      continue;
    }
    const auto& mod = **mod_opt;
    const auto module_key = sema::PathKeyOf(mod.path);
    for (const auto& item : mod.items) {
      const auto* decl = std::get_if<syntax::StaticDecl>(&item);
      if (!decl || !decl->binding.init) {
        continue;
      }
      Outcome init_out = EvalSigma(ctx, *decl->binding.init, sigma);
      if (IsCtrl(init_out)) {
        SPEC_RULE("Init-Panic");
        const auto poisoned = PoisonSetForInit(plan, module_path);
        for (const auto& key : poisoned) {
          SetPoisonFlag(sigma, key);
        }
        result.poisoned = poisoned;
        SPEC_RULE("Init-Fail");
        return result;
      }
      const auto env = BindPatternVal(ctx, *decl->binding.pat,
                                      std::get<Value>(init_out.node));
      if (!env.has_value()) {
        SPEC_RULE("Init-Panic");
        const auto poisoned = PoisonSetForInit(plan, module_path);
        for (const auto& key : poisoned) {
          SetPoisonFlag(sigma, key);
        }
        result.poisoned = poisoned;
        SPEC_RULE("Init-Fail");
        return result;
      }
      const auto binds = BindOrder(*decl->binding.pat, *env);
      for (const auto& [name, value] : binds) {
        const auto it = sigma.static_addrs.find({module_key, name});
        if (it == sigma.static_addrs.end()) {
          SPEC_RULE("Init-Panic");
          const auto poisoned = PoisonSetForInit(plan, module_path);
          for (const auto& key : poisoned) {
            SetPoisonFlag(sigma, key);
          }
          result.poisoned = poisoned;
          SPEC_RULE("Init-Fail");
          return result;
        }
        WriteAddr(sigma, it->second, value);
      }
      SPEC_RULE("Init-Step");
    }
  }

  SPEC_RULE("Init-Done");
  result.ok = true;
  SPEC_RULE("Init-Ok");
  return result;
}

CleanupStatus Deinit(const SemanticsContext& ctx, Sigma& sigma) {
  if (!ctx.sema || !ctx.name_maps) {
    SPEC_RULE("Deinit-Ok");
    return CleanupStatus::Ok;
  }
  const auto plan_result = sema::BuildInitPlan(*ctx.sema, *ctx.name_maps);
  if (!plan_result.ok || !plan_result.plan.topo_ok) {
    SPEC_RULE("Deinit-Panic");
    return CleanupStatus::Panic;
  }
  const auto& plan = plan_result.plan;
  std::vector<CleanupItem> items;
  for (std::size_t idx = plan.init_order.size(); idx-- > 0;) {
    const auto& module_path = plan.init_order[idx];
    const auto mod_opt = FindModule(*ctx.sema, module_path);
    if (!mod_opt.has_value()) {
      continue;
    }
    const auto& mod = **mod_opt;
    const auto module_key = sema::PathKeyOf(mod.path);
    for (auto item_it = mod.items.rbegin(); item_it != mod.items.rend(); ++item_it) {
      const auto* decl = std::get_if<syntax::StaticDecl>(&*item_it);
      if (!decl) {
        continue;
      }
      if (!StaticHasResp(*decl)) {
        continue;
      }
      auto names = sema::PatNames(*decl->binding.pat);
      for (auto name_it = names.rbegin(); name_it != names.rend(); ++name_it) {
        items.push_back(DropStatic{module_key, std::string(*name_it)});
      }
    }
  }
  const auto status = Cleanup(ctx, items, sigma);
  if (status == CleanupStatus::Ok) {
    SPEC_RULE("Deinit-Ok");
  } else {
    SPEC_RULE("Deinit-Panic");
  }
  return status == CleanupStatus::Ok ? CleanupStatus::Ok
                                     : CleanupStatus::Panic;
}

}  // namespace cursive0::semantics
