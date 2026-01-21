#include "cursive0/eval/interpreter.h"

#include <optional>
#include <vector>

#include "cursive0/core/symbols.h"
#include "cursive0/eval/init.h"
#include "cursive0/eval/cleanup.h"
#include "cursive0/eval/apply.h"
#include "cursive0/analysis/resolve/collect_toplevel.h"
#include "cursive0/analysis/resolve/scopes_lookup.h"
#include "cursive0/analysis/resolve/resolver.h"
#include "cursive0/eval/eval.h"
#include "cursive0/syntax/ast.h"

namespace cursive0::eval {

namespace {

analysis::ModuleNames ModuleNamesFromScope(const analysis::ScopeContext& ctx) {
  if (ctx.project) {
    return analysis::ModuleNamesOf(*ctx.project);
  }
  analysis::ModuleNames names;
  names.reserve(ctx.sigma.mods.size());
  for (const auto& mod : ctx.sigma.mods) {
    names.push_back(core::StringOfPath(mod.path));
  }
  return names;
}

struct ScopeContextRestorer {
  explicit ScopeContextRestorer(analysis::ScopeContext& ctx)
      : ctx_(&ctx),
        snapshot_(ctx) {}

  ~ScopeContextRestorer() {
    if (ctx_) {
      *ctx_ = snapshot_;
    }
  }

  ScopeContextRestorer(const ScopeContextRestorer&) = delete;
  ScopeContextRestorer& operator=(const ScopeContextRestorer&) = delete;

 private:
  analysis::ScopeContext* ctx_ = nullptr;
  analysis::ScopeContext snapshot_;
};

struct MainDeclResult {
  const syntax::ProcedureDecl* decl = nullptr;
  bool multiple = false;
  syntax::ModulePath module_path;
};

MainDeclResult FindMainDecl(const analysis::ScopeContext& ctx) {
  MainDeclResult result;
  for (const auto& mod : ctx.sigma.mods) {
    for (const auto& item : mod.items) {
      const auto* proc = std::get_if<syntax::ProcedureDecl>(&item);
      if (!proc) {
        continue;
      }
      if (!analysis::IdEq(proc->name, "main")) {
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
      analysis::IdEq(path->path[0], "Context");
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
  return ret && analysis::IdEq(ret->name, "i32");
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
                        const analysis::TypePath& class_path,
                        ScopeId scope_id) {
  const Addr addr = AllocAddr(sigma);
  WriteAddr(sigma, addr, Value{UnitVal{}});
  sigma.addr_tags[addr] = RuntimeTag{RuntimeTagKind::ScopeTag, scope_id};

  RawPtrVal raw;
  raw.qual = analysis::RawPtrQual::Imm;
  raw.addr = addr;

  DynamicVal dyn;
  dyn.class_path = class_path;
  dyn.data = raw;
  dyn.concrete_type = nullptr;

  return Value{dyn};
}

Value MakeSystemValue() {
  RecordVal rec;
  rec.record_type = analysis::MakeTypePath(analysis::TypePath{"System"});
  rec.fields = {};
  return Value{rec};
}

Value ContextInitSigma(Sigma& sigma) {
  const ScopeId scope_id = EnsureContextScope(sigma);
  Value fs = MakeDynamicHandle(sigma, analysis::TypePath{"FileSystem"}, scope_id);
  if (const auto* dyn = std::get_if<DynamicVal>(&fs.node)) {
    sigma.fs_handles[dyn->data.addr] = FileSystemHandle{std::nullopt, ""};
  }
  Value heap = MakeDynamicHandle(sigma, analysis::TypePath{"HeapAllocator"}, scope_id);
  Value sys = MakeSystemValue();

  RecordVal ctx;
  ctx.record_type = analysis::MakeTypePath(analysis::TypePath{"Context"});
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
                                analysis::ScopeContext& scope) {
  InterpreterResult result;
  ScopeContextRestorer restore(scope);
  auto name_maps = analysis::CollectNameMaps(scope);
  const auto module_names = ModuleNamesFromScope(scope);

  SemanticsContext ctx;
  ctx.sema = &scope;
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
                                  analysis::ScopeContext& scope) {
  InterpreterResult result;
  ScopeContextRestorer restore(scope);
  scope.sigma.mods = {module};
  scope.current_module = module.path;
  analysis::PopulateSigma(scope);

  auto name_maps = analysis::CollectNameMaps(scope);
  const auto module_names = ModuleNamesFromScope(scope);

  SemanticsContext ctx;
  ctx.sema = &scope;
  ctx.name_maps = &name_maps.name_maps;
  ctx.module_names = &module_names;

  const auto init_result = Init(ctx, sigma);
  if (!init_result.ok) {
    result.has_control = true;
    result.control.kind = ControlKind::Panic;
    result.control.value = std::nullopt;
    sigma.panic_code = PanicCode(PanicReason::InitPanic);
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
                                   analysis::ScopeContext& scope) {
  InterpreterResult result;
  if (project.assembly.kind != "executable") {
    result.has_control = true;
    result.control.kind = ControlKind::Panic;
    result.control.value = std::nullopt;
    return result;
  }

  ScopeContextRestorer restore(scope);
  scope.project = &project;
  auto name_maps = analysis::CollectNameMaps(scope);
  const auto module_names = ModuleNamesFromScope(scope);

  SemanticsContext ctx;
  ctx.sema = &scope;
  ctx.name_maps = &name_maps.name_maps;
  ctx.module_names = &module_names;

  const auto main_result = FindMainDecl(scope);
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
    sigma.panic_code = PanicCode(PanicReason::InitPanic);
    SPEC_RULE("Interpret-Project-Init-Panic");
    return result;
  }

  if (!main_result.module_path.empty()) {
    scope.current_module = main_result.module_path;
    const auto key = analysis::PathKeyOf(main_result.module_path);
    const auto it = name_maps.name_maps.find(key);
    if (it != name_maps.name_maps.end()) {
      scope.scopes = {analysis::Scope{}, it->second, analysis::UniverseBindings()};
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

}  // namespace cursive0::eval
