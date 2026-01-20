#include "cursive0/codegen/globals.h"

#include <algorithm>
#include <set>
#include <vector>

#include "cursive0/codegen/checks.h"
#include "cursive0/codegen/abi.h"
#include "cursive0/codegen/cleanup.h"
#include "cursive0/codegen/lower_expr.h"
#include "cursive0/codegen/lower_pat.h"
#include "cursive0/codegen/layout.h"
#include "cursive0/codegen/mangle.h"
#include "cursive0/core/assert_spec.h"
#include "cursive0/core/symbols.h"

namespace cursive0::codegen {

namespace {

std::string ModulePathString(const syntax::ModulePath& path) {
  std::string out;
  for (std::size_t i = 0; i < path.size(); ++i) {
    if (i) {
      out += "::";
    }
    out += path[i];
  }
  return out;
}

bool IsMoveExprLite(const syntax::ExprPtr& expr) {
  return expr && std::holds_alternative<syntax::MoveExpr>(expr->node);
}

bool IsPlaceExprLite(const syntax::ExprPtr& expr) {
  if (!expr) {
    return false;
  }
  return std::visit(
      [&](const auto& node) -> bool {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::IdentifierExpr>) {
          return true;
        } else if constexpr (std::is_same_v<T, syntax::FieldAccessExpr>) {
          return IsPlaceExprLite(node.base);
        } else if constexpr (std::is_same_v<T, syntax::TupleAccessExpr>) {
          return IsPlaceExprLite(node.base);
        } else if constexpr (std::is_same_v<T, syntax::IndexAccessExpr>) {
          return IsPlaceExprLite(node.base);
        } else if constexpr (std::is_same_v<T, syntax::DerefExpr>) {
          return IsPlaceExprLite(node.value);
        }
        return false;
      },
      expr->node);
}

bool StaticHasResponsibility(const syntax::StaticDecl& item) {
  const auto& init = item.binding.init;
  if (!init) {
    return true;
  }
  if (!IsPlaceExprLite(init)) {
    return true;
  }
  return IsMoveExprLite(init);
}

}  // namespace

// ============================================================================
// Helper functions
// ============================================================================

// Helpers moved to lower_expr.h

// SeqIRList([]) = ε
// SeqIRList([IR] ++ IRs) = SeqIR(IR, SeqIRList(IRs))
static IRPtr SeqIRList(const std::vector<IRPtr>& irs) {
  SPEC_DEF("SeqIRList", "");
  return SeqIR(std::vector<IRPtr>(irs.begin(), irs.end()));
}

// Rev([]) = []
// Rev([x] ++ xs) = Rev(xs) ++ [x]
template <typename T>
static std::vector<T> Rev(const std::vector<T>& vec) {
  SPEC_DEF("Rev", "");
  std::vector<T> result(vec.rbegin(), vec.rend());
  return result;
}

static std::vector<syntax::ModulePath> TopoOrderFromEdges(
    const std::vector<syntax::ModulePath>& modules,
    const std::vector<std::pair<std::size_t, std::size_t>>& edges) {
  const std::size_t n = modules.size();
  if (n == 0) {
    return {};
  }
  std::vector<std::vector<std::size_t>> adj(n);
  std::vector<std::size_t> indeg(n, 0);
  for (const auto& edge : edges) {
    if (edge.first >= n || edge.second >= n) {
      continue;
    }
    adj[edge.first].push_back(edge.second);
    indeg[edge.second] += 1;
  }
  std::set<std::size_t> ready;
  for (std::size_t i = 0; i < n; ++i) {
    if (indeg[i] == 0) {
      ready.insert(i);
    }
  }
  std::vector<syntax::ModulePath> order;
  order.reserve(n);
  while (!ready.empty()) {
    const std::size_t u = *ready.begin();
    ready.erase(ready.begin());
    order.push_back(modules[u]);
    for (const auto v : adj[u]) {
      if (indeg[v] == 0) {
        continue;
      }
      indeg[v] -= 1;
      if (indeg[v] == 0) {
        ready.insert(v);
      }
    }
  }
  if (order.size() != n) {
    return {};
  }
  return order;
}

// ============================================================================
// §6.7 Init/Deinit Symbol Generation
// ============================================================================

std::string InitSym(const syntax::ModulePath& module_path) {
  SPEC_DEF("InitSym", "");

  // InitSym(m) = PathSig(["cursive", "runtime", "init"] ++ PathOfModule(m))
  std::vector<std::string> path = {"cursive", "runtime", "init"};
  path.insert(path.end(), module_path.begin(), module_path.end());
  return core::Mangle(core::StringOfPath(path));
}

std::string DeinitSym(const syntax::ModulePath& module_path) {
  SPEC_DEF("DeinitSym", "");

  // DeinitSym(m) = PathSig(["cursive", "runtime", "deinit"] ++ PathOfModule(m))
  std::vector<std::string> path = {"cursive", "runtime", "deinit"};
  path.insert(path.end(), module_path.begin(), module_path.end());
  return core::Mangle(core::StringOfPath(path));
}

std::string InitFn(const syntax::ModulePath& module_path) {
  SPEC_RULE("InitFn");
  return InitSym(module_path);
}

std::string DeinitFn(const syntax::ModulePath& module_path) {
  SPEC_RULE("DeinitFn");
  return DeinitSym(module_path);
}

// ============================================================================
// §6.7 Static Init Lowering
// ============================================================================

IRPtr LowerStaticInitItem(const syntax::ModulePath& module_path,
                          const syntax::StaticDecl& item,
                          LowerCtx& ctx) {
  SPEC_RULE("Lower-StaticInit-Item");

  const auto& binding = item.binding;
  if (!binding.init) {
    return EmptyIR();
  }

  std::vector<IRPtr> ir_parts;

  auto init_result = LowerExpr(*binding.init, ctx);
  ir_parts.push_back(init_result.ir);

  if (binding.pat) {
    ir_parts.push_back(LowerBindPattern(*binding.pat, init_result.value, ctx));
  }

  std::vector<std::pair<std::string, IRValue>> binds;
  const auto names = StaticBindList(binding);
  binds.reserve(names.size());
  for (const auto& name : names) {
    IRValue local;
    local.kind = IRValue::Kind::Local;
    local.name = name;
    binds.emplace_back(name, local);
  }

  ir_parts.push_back(StaticStoreIR(item, module_path, binds));
  ir_parts.push_back(InitPanicHandle(ModulePathString(module_path), ctx));

  return SeqIR(std::move(ir_parts));
}

IRPtr LowerStaticInitItems(const syntax::ModulePath& module_path,
                           const std::vector<const syntax::StaticDecl*>& items,
                           LowerCtx& ctx) {
  // (Lower-StaticInitItems-Empty)
  if (items.empty()) {
    SPEC_RULE("Lower-StaticInitItems-Empty");
    return EmptyIR();
  }
  
  // (Lower-StaticInitItems-Cons)
  SPEC_RULE("Lower-StaticInitItems-Cons");
  
  std::vector<IRPtr> ir_parts;
  ir_parts.reserve(items.size());
  
  for (const auto* item : items) {
    ir_parts.push_back(LowerStaticInitItem(module_path, *item, ctx));
  }
  
  return SeqIRList(ir_parts);
}

IRPtr LowerStaticInit(const syntax::ModulePath& module_path,
                      const syntax::ASTModule& module,
                      LowerCtx& ctx) {
  SPEC_RULE("Lower-StaticInit");
  
  // (Lower-StaticInit)
  // StaticItems(Project(Γ), m) = items
  // Lower-StaticInitItems(m, items) ⇓ IR
  // Lower-StaticInit(m) ⇓ IR
  
  auto items = StaticItems(module);
  return LowerStaticInitItems(module_path, items, ctx);
}

// ============================================================================
// §6.7 Static Deinit Lowering
// ============================================================================

IRPtr LowerStaticDeinitNames(const syntax::ModulePath& module_path,
                             const syntax::StaticDecl& item,
                             const std::vector<std::string>& names,
                             LowerCtx& ctx) {
  if (names.empty()) {
    SPEC_RULE("Lower-StaticDeinitNames-Empty");
    return EmptyIR();
  }

  std::vector<IRPtr> ir_parts;
  ir_parts.reserve(names.size());

  const bool has_resp = StaticHasResponsibility(item);
  const auto bind_types = StaticBindTypes(item.binding, module_path, ctx);
  auto type_for = [&](const std::string& name) -> sema::TypeRef {
    for (const auto& [bind_name, bind_type] : bind_types) {
      if (bind_name == name) {
        return bind_type;
      }
    }
    return nullptr;
  };

  for (const auto& name : names) {
    if (!has_resp) {
      SPEC_RULE("Lower-StaticDeinitNames-Cons-NoResp");
      continue;
    }
    SPEC_RULE("Lower-StaticDeinitNames-Cons-Resp");

    IRReadPath read;
    read.path = module_path;
    read.name = name;

    IRValue loaded_value = ctx.FreshTempValue("static");

    sema::TypeRef type = type_for(name);

    IRPtr drop_ir = EmitDrop(type, loaded_value, ctx);
    ir_parts.push_back(SeqIR({MakeIR(std::move(read)), drop_ir}));
  }

  if (ir_parts.empty()) {
    return EmptyIR();
  }
  return SeqIRList(ir_parts);
}

IRPtr LowerStaticDeinitItem(const syntax::ModulePath& module_path,
                            const syntax::StaticDecl& item,
                            LowerCtx& ctx) {
  SPEC_RULE("Lower-StaticDeinit-Item");
  
  // (Lower-StaticDeinit-Item)
  // item = StaticDecl(vis, mut, binding, span, doc)
  // binding = ⟨pat, _, _, _, _⟩
  // xs = Rev(StaticBindList(binding))
  // Lower-StaticDeinitNames(PathOfModule(m), item, xs) ⇓ IR
  // Lower-StaticDeinitItem(m, item) ⇓ IR
  
  auto names = StaticBindList(item.binding);
  auto reversed_names = Rev(names);
  
  return LowerStaticDeinitNames(module_path, item, reversed_names, ctx);
}

IRPtr LowerStaticDeinitItems(const syntax::ModulePath& module_path,
                             const std::vector<const syntax::StaticDecl*>& items,
                             LowerCtx& ctx) {
  // (Lower-StaticDeinitItems-Empty)
  if (items.empty()) {
    SPEC_RULE("Lower-StaticDeinitItems-Empty");
    return EmptyIR();
  }
  
  // (Lower-StaticDeinitItems-Cons)
  SPEC_RULE("Lower-StaticDeinitItems-Cons");
  
  std::vector<IRPtr> ir_parts;
  ir_parts.reserve(items.size());
  
  for (const auto* item : items) {
    ir_parts.push_back(LowerStaticDeinitItem(module_path, *item, ctx));
  }
  
  return SeqIRList(ir_parts);
}

IRPtr LowerStaticDeinit(const syntax::ModulePath& module_path,
                        const syntax::ASTModule& module,
                        LowerCtx& ctx) {
  SPEC_RULE("Lower-StaticDeinit");
  
  // (Lower-StaticDeinit)
  // StaticItems(Project(Γ), m) = items
  // Lower-StaticDeinitItems(m, Rev(items)) ⇓ IR
  // Lower-StaticDeinit(m) ⇓ IR
  
  auto items = StaticItems(module);
  auto reversed_items = Rev(items);
  
  return LowerStaticDeinitItems(module_path, reversed_items, ctx);
}

// ============================================================================
// §6.7 Init/Deinit Call IR
// ============================================================================

IRPtr InitCallIR(const syntax::ModulePath& module_path, LowerCtx& ctx) {
  SPEC_RULE("InitCallIR");
  
  // (InitCallIR)
  // InitFn(m) ⇓ sym
  // InitCallIR(m) ⇓ SeqIR(CallIR(sym, [PanicOutName]), PanicCheck)
  
  std::string sym = InitFn(module_path);
  
  IRCall call;
  call.callee.kind = IRValue::Kind::Symbol;
  call.callee.name = sym;
  
  // Add PanicOutName as argument
  IRValue panic_out;
  panic_out.kind = IRValue::Kind::Local;
  panic_out.name = std::string(kPanicOutName);
  call.args.push_back(panic_out);
  
  std::vector<IRPtr> parts;
  parts.push_back(MakeIR(std::move(call)));
  parts.push_back(PanicCheck(ctx));
  
  return SeqIR(std::move(parts));
}

IRPtr DeinitCallIR(const syntax::ModulePath& module_path, LowerCtx& ctx) {
  SPEC_RULE("DeinitCallIR");
  
  // (DeinitCallIR)
  // DeinitFn(m) ⇓ sym
  // DeinitCallIR(m) ⇓ SeqIR(CallIR(sym, [PanicOutName]), PanicCheck)
  
  std::string sym = DeinitFn(module_path);
  
  IRCall call;
  call.callee.kind = IRValue::Kind::Symbol;
  call.callee.name = sym;
  
  // Add PanicOutName as argument
  IRValue panic_out;
  panic_out.kind = IRValue::Kind::Local;
  panic_out.name = std::string(kPanicOutName);
  call.args.push_back(panic_out);
  
  std::vector<IRPtr> parts;
  parts.push_back(MakeIR(std::move(call)));
  parts.push_back(PanicCheck(ctx));
  
  return SeqIR(std::move(parts));
}

// ============================================================================
// §6.7 Init/Deinit Plan Generation
// ============================================================================

IRPtr EmitInitPlan(const std::vector<syntax::ModulePath>& init_order,
                   LowerCtx& ctx) {
  SPEC_RULE("EmitInitPlan");
  
  // (EmitInitPlan)
  // InitOrder = [m_1,...,m_k]
  // ∀i, InitCallIR(m_i) ⇓ IR_i
  // IR_init = SeqIRList([IR_1,...,IR_k])
  // EmitInitPlan(P) ⇓ IR_init
  
  std::vector<syntax::ModulePath> order = init_order;
  if (order.empty()) {
    if (!ctx.init_modules.empty()) {
      const auto fallback =
          TopoOrderFromEdges(ctx.init_modules, ctx.init_eager_edges);
      if (!fallback.empty()) {
        order = fallback;
      } else {
        order = ctx.init_modules;
      }
    } else if (ctx.sigma) {
      order.reserve(ctx.sigma->mods.size());
      for (const auto& mod : ctx.sigma->mods) {
        order.push_back(mod.path);
      }
    }
  } else if (!ctx.init_modules.empty() &&
             order.size() != ctx.init_modules.size()) {
    const auto fallback =
        TopoOrderFromEdges(ctx.init_modules, ctx.init_eager_edges);
    if (!fallback.empty()) {
      order = fallback;
    }
  }

  std::vector<IRPtr> ir_parts;
  ir_parts.reserve(order.size());
  
  for (const auto& module_path : order) {
    ir_parts.push_back(InitCallIR(module_path, ctx));
  }
  
  return SeqIRList(ir_parts);
}

IRPtr EmitDeinitPlan(const std::vector<syntax::ModulePath>& init_order,
                     LowerCtx& ctx) {
  SPEC_RULE("EmitDeinitPlan");
  
  // (EmitDeinitPlan)
  // InitOrder = [m_1,...,m_k]
  // ∀i, DeinitCallIR(m_i) ⇓ IR_i
  // IR_deinit = SeqIRList(Rev([IR_1,...,IR_k]))
  // EmitDeinitPlan(P) ⇓ IR_deinit
  
  auto reversed_order = Rev(init_order);
  
  std::vector<IRPtr> ir_parts;
  ir_parts.reserve(reversed_order.size());
  
  for (const auto& module_path : reversed_order) {
    ir_parts.push_back(DeinitCallIR(module_path, ctx));
  }
  
  return SeqIRList(ir_parts);
}

// ============================================================================
// §6.7 Module Init/Deinit Function IR
// ============================================================================

ProcIR EmitModuleInitFn(const syntax::ModulePath& module_path,
                        const syntax::ASTModule& module,
                        LowerCtx& ctx) {
  SPEC_DEF("EmitModuleInitFn", "");
  
  ProcIR proc;
  proc.symbol = InitFn(module_path);
  
  // Init function takes a panic out parameter
  IRParam panic_param;
  panic_param.mode = sema::ParamMode::Move;
  panic_param.name = std::string(kPanicOutName);
  panic_param.type = PanicOutType();
  proc.params.push_back(std::move(panic_param));
  
  // Return type is unit
  proc.ret = sema::MakeTypePrim("()");
  
  // Body is the static init lowering
  proc.body = LowerStaticInit(module_path, module, ctx);
  
  return proc;
}

ProcIR EmitModuleDeinitFn(const syntax::ModulePath& module_path,
                          const syntax::ASTModule& module,
                          LowerCtx& ctx) {
  SPEC_DEF("EmitModuleDeinitFn", "");
  
  ProcIR proc;
  proc.symbol = DeinitFn(module_path);
  
  // Deinit function takes a panic out parameter
  IRParam panic_param;
  panic_param.mode = sema::ParamMode::Move;
  panic_param.name = std::string(kPanicOutName);
  panic_param.type = PanicOutType();
  proc.params.push_back(std::move(panic_param));
  
  // Return type is unit
  proc.ret = sema::MakeTypePrim("()");
  
  // Body is the static deinit lowering
  proc.body = LowerStaticDeinit(module_path, module, ctx);
  
  return proc;
}

// ============================================================================
// Spec Rule Anchors
// ============================================================================

void AnchorInitRules() {
  // §6.7 Init/Deinit
  SPEC_RULE("InitFn");
  SPEC_RULE("DeinitFn");
  SPEC_RULE("Lower-StaticInit-Item");
  SPEC_RULE("Lower-StaticInitItems-Empty");
  SPEC_RULE("Lower-StaticInitItems-Cons");
  SPEC_RULE("Lower-StaticInit");
  SPEC_RULE("Lower-StaticDeinitNames-Empty");
  SPEC_RULE("Lower-StaticDeinitNames-Cons-Resp");
  SPEC_RULE("Lower-StaticDeinitNames-Cons-NoResp");
  SPEC_RULE("Lower-StaticDeinit-Item");
  SPEC_RULE("Lower-StaticDeinitItems-Empty");
  SPEC_RULE("Lower-StaticDeinitItems-Cons");
  SPEC_RULE("Lower-StaticDeinit");
  SPEC_RULE("InitCallIR");
  SPEC_RULE("DeinitCallIR");
  SPEC_RULE("EmitInitPlan");
  SPEC_RULE("EmitInitPlan-Err");
  SPEC_RULE("EmitDeinitPlan");
  SPEC_RULE("EmitDeinitPlan-Err");

  // A7.1 Initialization Order and Poisoning
  SPEC_RULE("Init-Start");
  SPEC_RULE("Init-Step");
  SPEC_RULE("Init-Next-Module");
  SPEC_RULE("Init-Panic");
  SPEC_RULE("Init-Done");
  SPEC_RULE("Init-Ok");
  SPEC_RULE("Init-Fail");
  SPEC_RULE("Deinit-Ok");
  SPEC_RULE("Deinit-Panic");
  
  // Definitions
  SPEC_DEF("InitSym", "");
  SPEC_DEF("DeinitSym", "");
  SPEC_DEF("SeqIRList", "");
  SPEC_DEF("Rev", "");
  SPEC_DEF("EmitModuleInitFn", "");
  SPEC_DEF("EmitModuleDeinitFn", "");
}

}  // namespace cursive0::codegen
