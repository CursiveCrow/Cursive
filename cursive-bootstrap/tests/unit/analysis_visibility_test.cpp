#include <cassert>
#include <memory>

#include "cursive0/analysis/resolve/visibility.h"

namespace {

using cursive0::analysis::AccessResult;
using cursive0::analysis::CanAccess;
using cursive0::analysis::CanAccessVis;
using cursive0::analysis::ScopeContext;
using cursive0::analysis::TopLevelVis;
using cursive0::syntax::ASTItem;
using cursive0::syntax::ASTModule;
using cursive0::syntax::Block;
using cursive0::syntax::ModulePath;
using cursive0::syntax::ProcedureDecl;
using cursive0::syntax::Visibility;

std::shared_ptr<Block> MakeBlock() {
  return std::make_shared<Block>();
}

ASTItem MakeProcedure(Visibility vis, const char* name) {
  ProcedureDecl proc;
  proc.vis = vis;
  proc.name = name;
  proc.body = MakeBlock();
  return proc;
}

ASTModule MakeModule(ModulePath path, std::vector<ASTItem> items) {
  ASTModule module;
  module.path = std::move(path);
  module.items = std::move(items);
  return module;
}

ScopeContext MakeContext(ModulePath current,
                         std::vector<ASTModule> modules) {
  ScopeContext ctx;
  ctx.current_module = std::move(current);
  ctx.sigma.mods = std::move(modules);
  ctx.scopes = {cursive0::analysis::Scope{},
                cursive0::analysis::Scope{},
                cursive0::analysis::Scope{}};
  return ctx;
}

}  // namespace

int main() {
  const ModulePath lib_path = {"lib"};
  const ModulePath main_path = {"main"};

  {
    SPEC_COV("Access-Public");
    auto ctx = MakeContext(main_path,
                           {MakeModule(lib_path, {MakeProcedure(Visibility::Public, "pub")})});
    const AccessResult result = CanAccess(ctx, lib_path, "pub");
    assert(result.ok);
    assert(!result.diag_id.has_value());
  }

  {
    SPEC_COV("Access-Internal");
    auto ctx = MakeContext(main_path,
                           {MakeModule(lib_path, {MakeProcedure(Visibility::Internal, "int")})});
    const AccessResult result = CanAccess(ctx, lib_path, "int");
    assert(result.ok);
  }

  {
    SPEC_COV("Access-Private");
    auto ctx = MakeContext(lib_path,
                           {MakeModule(lib_path, {MakeProcedure(Visibility::Private, "priv")})});
    const AccessResult result = CanAccess(ctx, lib_path, "priv");
    assert(result.ok);
  }

  {
    SPEC_COV("Access-Protected");
    auto ctx = MakeContext(lib_path,
                           {MakeModule(lib_path, {MakeProcedure(Visibility::Protected, "prot")})});
    const AccessResult result = CanAccess(ctx, lib_path, "prot");
    assert(result.ok);
  }

  {
    SPEC_COV("Access-Err");
    auto ctx = MakeContext(main_path,
                           {MakeModule(lib_path, {MakeProcedure(Visibility::Protected, "prot")})});
    const AccessResult result = CanAccess(ctx, lib_path, "prot");
    assert(!result.ok);
    assert(result.diag_id == "Access-Err");
  }

  {
    SPEC_COV("Protected-TopLevel-Ok");
    const ASTItem item = MakeProcedure(Visibility::Internal, "ok");
    const AccessResult result = TopLevelVis(item);
    assert(result.ok);
    assert(!result.diag_id.has_value());
  }

  {
    SPEC_COV("Protected-TopLevel-Err");
    const ASTItem item = MakeProcedure(Visibility::Protected, "bad");
    const AccessResult result = TopLevelVis(item);
    assert(!result.ok);
    assert(result.diag_id == "Protected-TopLevel-Err");
  }

  {
    const AccessResult result = CanAccessVis(main_path, main_path, Visibility::Private);
    assert(result.ok);
  }

  return 0;
}
