#include "cursive0/semantics/interpreter.h"

#include <optional>
#include <vector>

#include "cursive0/core/symbols.h"
#include "cursive0/semantics/init.h"
#include "cursive0/semantics/cleanup.h"
#include "cursive0/semantics/apply.h"
#include "cursive0/sema/collect_toplevel.h"
#include "cursive0/sema/scopes_lookup.h"
#include "cursive0/sema/resolver.h"
#include "cursive0/semantics/eval.h"
#include "cursive0/syntax/ast.h"

namespace cursive0::semantics {

namespace {

sema::ModuleNames ModuleNamesFromScope(const sema::ScopeContext& ctx) {
  if (ctx.project) {
    return sema::ModuleNamesOf(*ctx.project);
  }
  sema::ModuleNames names;
  names.reserve(ctx.sigma.mods.size());
  for (const auto& mod : ctx.sigma.mods) {
    names.push_back(core::StringOfPath(mod.path));
  }
  return names;
}

struct MainDeclResult {
  const syntax::ProcedureDecl* decl = nullptr;
  bool multiple = false;
  syntax::ModulePath module_path;
};

MainDeclResult FindMainDecl(const sema::ScopeContext& ctx) {
  MainDeclResult result;
  for (const auto& mod : ctx.sigma.mods) {
    for (const auto& item : mod.items) {
      const auto* proc = std::get_if<syntax::ProcedureDecl>(&item);
      if (!proc) {
        continue;
      }
      if (!sema::IdEq(proc->name, "main")) {
        continue;
      }
      if (result.decl) {
        result.multiple = true;
        return result;
      }
      result.decl = proc;
      result.module_path = mod.path;
    }
  }
  return result;
}

bool BuiltInContextType(const std::shared_ptr<syntax::Type>& type) {
  if (!type) {
    return false;
  }
  const auto* path = std::get_if<syntax::TypePathType>(&type->node);
  return path && path->path.size() == 1 &&
      sema::IdEq(path->path[0], "Context");
}

bool MainSigOk(const syntax::ProcedureDecl& decl) {
  if (decl.vis != syntax::Visibility::Public) {
    return false;
  }
  if (decl.params.size() != 1) {
    return false;
  }
  const auto& param = decl.params[0];
  if (param.mode.has_value() && *param.mode != syntax::ParamMode::Move) {
    return false;
  }
  if (!BuiltInContextType(param.type)) {
    return false;
  }
  const auto* ret = decl.return_type_opt
                        ? std::get_if<syntax::TypePrim>(&decl.return_type_opt->node)
                        : nullptr;
  return ret && sema::IdEq(ret->name, "i32");
}

ScopeId EnsureContextScope(Sigma& sigma) {
  if (!sigma.scope_stack.empty()) {
    return sigma.scope_stack.front().id;
  }
  ScopeEntry scope;
  if (!PushScope(sigma, &scope)) {
    return 0;
  }
  return scope.id;
}

Value MakeDynamicHandle(Sigma& sigma,
                        const sema::TypePath& class_path,
                        ScopeId scope_id) {
  const Addr addr = AllocAddr(sigma);
  WriteAddr(sigma, addr, Value{UnitVal{}});
  sigma.addr_tags[addr] = RuntimeTag{RuntimeTagKind::ScopeTag, scope_id};

  RawPtrVal raw;
  raw.qual = sema::RawPtrQual::Imm;
  raw.addr = addr;

  DynamicVal dyn;
  dyn.class_path = class_path;
  dyn.data = raw;
  dyn.concrete_type = nullptr;

  return Value{dyn};
}

Value MakeSystemValue() {
  RecordVal rec;
  rec.record_type = sema::MakeTypePath(sema::TypePath{"System"});
  rec.fields = {};
  return Value{rec};
}

Value ContextInitSigma(Sigma& sigma) {
  const ScopeId scope_id = EnsureContextScope(sigma);
  Value fs = MakeDynamicHandle(sigma, sema::TypePath{"FileSystem"}, scope_id);
  if (const auto* dyn = std::get_if<DynamicVal>(&fs.node)) {
    sigma.fs_handles[dyn->data.addr] = FileSystemHandle{std::nullopt, ""};
  }
  Value heap = MakeDynamicHandle(sigma, sema::TypePath{"HeapAllocator"}, scope_id);
  Value sys = MakeSystemValue();

  RecordVal ctx;
  ctx.record_type = sema::MakeTypePath(sema::TypePath{"Context"});
  ctx.fields = {
      {"fs", fs},
      {"heap", heap},
      {"sys", sys},
  };
  SPEC_RULE("ContextInitSigma");
  return Value{ctx};
}

}  // namespace

InterpreterResult InterpretExpr(const syntax::Expr& expr,
                                Sigma& sigma,
                                const sema::ScopeContext& scope) {
  InterpreterResult result;

  sema::ScopeContext local = scope;
  const auto current_module = local.current_module;
  auto name_maps = sema::CollectNameMaps(local);
  local.current_module = current_module;
  const auto module_names = ModuleNamesFromScope(local);

  SemanticsContext ctx;
  ctx.sema = &local;
  ctx.name_maps = &name_maps.name_maps;
  ctx.module_names = &module_names;

  const Outcome out = EvalSigma(ctx, expr, sigma);
  if (IsCtrl(out)) {
    result.has_control = true;
    result.control = std::get<Control>(out.node);
    return result;
  }
  result.has_value = true;
  result.value = std::get<Value>(out.node);
  return result;
}


InterpreterResult InterpretModule(const syntax::ASTModule& module,
                                  Sigma& sigma,
                                  const sema::ScopeContext& scope) {
  InterpreterResult result;
  sema::ScopeContext local = scope;
  local.sigma.mods = {module};
  local.current_module = module.path;
  sema::PopulateSigma(local);

  auto name_maps = sema::CollectNameMaps(local);
  const auto module_names = ModuleNamesFromScope(local);

  SemanticsContext ctx;
  ctx.sema = &local;
  ctx.name_maps = &name_maps.name_maps;
  ctx.module_names = &module_names;

  const auto init_result = Init(ctx, sigma);
  if (!init_result.ok) {
    result.has_control = true;
    result.control.kind = ControlKind::Panic;
    result.control.value = std::nullopt;
    return result;
  }

  const auto status = Deinit(ctx, sigma);
  if (status != CleanupStatus::Ok) {
    result.has_control = true;
    result.control.kind = ControlKind::Panic;
    result.control.value = std::nullopt;
    return result;
  }

  result.has_value = true;
  result.value = Value{UnitVal{}};
  return result;
}

InterpreterResult InterpretProject(const project::Project& project,
                                   Sigma& sigma,
                                   const sema::ScopeContext& scope) {
  InterpreterResult result;
  if (project.assembly.kind != "executable") {
    result.has_control = true;
    result.control.kind = ControlKind::Panic;
    result.control.value = std::nullopt;
    return result;
  }

  sema::ScopeContext local = scope;
  local.project = &project;
  const auto current_module = local.current_module;
  auto name_maps = sema::CollectNameMaps(local);
  local.current_module = current_module;
  const auto module_names = ModuleNamesFromScope(local);

  SemanticsContext ctx;
  ctx.sema = &local;
  ctx.name_maps = &name_maps.name_maps;
  ctx.module_names = &module_names;

  const auto main_result = FindMainDecl(local);
  if (!main_result.decl || main_result.multiple || !MainSigOk(*main_result.decl)) {
    result.has_control = true;
    result.control.kind = ControlKind::Panic;
    result.control.value = std::nullopt;
    return result;
  }

  const Value ctx_value = ContextInitSigma(sigma);
  const auto init_result = Init(ctx, sigma);
  if (!init_result.ok) {
    result.has_control = true;
    result.control.kind = ControlKind::Panic;
    result.control.value = std::nullopt;
    SPEC_RULE("Interpret-Project-Init-Panic");
    return result;
  }

  if (!main_result.module_path.empty()) {
    local.current_module = main_result.module_path;
    const auto key = sema::PathKeyOf(main_result.module_path);
    const auto it = name_maps.name_maps.find(key);
    if (it != name_maps.name_maps.end()) {
      local.scopes = {sema::Scope{}, it->second, sema::UniverseBindings()};
    }
  }

  std::vector<BindingValue> args;
  args.reserve(1);
  args.push_back(ctx_value);
  Outcome out = ApplyProcSigma(ctx, *main_result.decl, args, sigma);
  if (IsCtrl(out)) {
    const auto ctrl = std::get<Control>(out.node);
    if (ctrl.kind == ControlKind::Panic || ctrl.kind == ControlKind::Abort) {
      result.has_control = true;
      result.control = ctrl;
      SPEC_RULE("Interpret-Project-Main-Ctrl");
      return result;
    }
    result.has_control = true;
    result.control.kind = ControlKind::Panic;
    result.control.value = std::nullopt;
    return result;
  }

  const auto status = Deinit(ctx, sigma);
  if (status != CleanupStatus::Ok) {
    result.has_control = true;
    result.control.kind = ControlKind::Panic;
    result.control.value = std::nullopt;
    SPEC_RULE("Interpret-Project-Deinit-Panic");
    return result;
  }

  result.has_value = true;
  result.value = std::get<Value>(out.node);
  SPEC_RULE("Interpret-Project-Ok");
  return result;
}

}  // namespace cursive0::semantics
