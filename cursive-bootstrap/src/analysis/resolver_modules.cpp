#include "cursive0/sema/resolver.h"

#include <utility>
#include <memory>
#include <string_view>
#include <string>
#include <type_traits>

#include "cursive0/core/assert_spec.h"
#include "cursive0/core/diagnostic_messages.h"
#include "cursive0/sema/collect_toplevel.h"
#include "cursive0/sema/region_type.h"
#include "cursive0/sema/cap_filesystem.h"
#include "cursive0/sema/cap_heap.h"
#include "cursive0/sema/cap_system.h"
#include "cursive0/sema/scopes.h"
#include "cursive0/sema/scopes_intro.h"
#include "cursive0/sema/visibility.h"

namespace cursive0::sema {

namespace {

static inline void SpecDefsResolverModules() {
  SPEC_DEF("ResolveModules", "5.1.7");
  SPEC_DEF("ResolveModule", "5.1.7");
  SPEC_DEF("ResolveItems", "5.1.7");
  SPEC_DEF("ResolveItem", "5.1.7");
  SPEC_DEF("ValidateModulePath", "5.1.7");
  SPEC_DEF("ValidateModuleNames", "5.1.7");
  SPEC_DEF("CollectNames", "5.1.5");
  SPEC_DEF("TopLevelVis", "5.1.4");
  SPEC_DEF("BindSelfRecord", "5.1.7");
  SPEC_DEF("BindSelfClass", "5.1.7");
}

syntax::ExprPtr MakeLiteralExpr(syntax::TokenKind kind, std::string_view lexeme) {
  auto expr = std::make_shared<syntax::Expr>();
  syntax::Token token;
  token.kind = kind;
  token.lexeme = std::string(lexeme);
  expr->node = syntax::LiteralExpr{token};
  return expr;
}

std::shared_ptr<syntax::Type> MakeTypePrimNode(std::string_view name) {
  auto type = std::make_shared<syntax::Type>();
  type->node = syntax::TypePrim{std::string(name)};
  return type;
}

std::shared_ptr<syntax::Type> MakeTypeStringNode() {
  auto type = std::make_shared<syntax::Type>();
  type->node = syntax::TypeString{std::nullopt};
  return type;
}

std::shared_ptr<syntax::Type> MakeTypePathNode(std::string_view name) {
  auto type = std::make_shared<syntax::Type>();
  syntax::TypePath path;
  path.emplace_back(std::string(name));
  type->node = syntax::TypePathType{std::move(path)};
  return type;
}

syntax::ClassMethodDecl MakeClassMethodDecl(std::string_view name,
                                            const syntax::Receiver& receiver,
                                            const std::shared_ptr<syntax::Type>& return_type_opt) {
  syntax::ClassMethodDecl method{};
  method.vis = syntax::Visibility::Public;
  method.name = std::string(name);
  method.receiver = receiver;
  method.params = {};
  method.return_type_opt = return_type_opt;
  method.body_opt = nullptr;
  method.span = core::Span{};
  method.doc_opt = std::nullopt;
  return method;
}

syntax::ClassDecl BuildDropClassDecl() {
  syntax::ClassDecl decl{};
  decl.vis = syntax::Visibility::Public;
  decl.name = "Drop";
  decl.supers = {};
  decl.items = {MakeClassMethodDecl(
      "drop", syntax::Receiver{syntax::ReceiverShorthand{syntax::ReceiverPerm::Unique}},
      nullptr)};
  decl.span = core::Span{};
  decl.doc = {};
  return decl;
}

syntax::ClassDecl BuildBitcopyClassDecl() {
  syntax::ClassDecl decl{};
  decl.vis = syntax::Visibility::Public;
  decl.name = "Bitcopy";
  decl.supers = {};
  decl.items = {};
  decl.span = core::Span{};
  decl.doc = {};
  return decl;
}

syntax::ClassDecl BuildCloneClassDecl() {
  syntax::ClassDecl decl{};
  decl.vis = syntax::Visibility::Public;
  decl.name = "Clone";
  decl.supers = {};
  decl.items = {MakeClassMethodDecl(
      "clone", syntax::Receiver{syntax::ReceiverShorthand{syntax::ReceiverPerm::Const}},
      MakeTypePathNode("Self"))};
  decl.span = core::Span{};
  decl.doc = {};
  return decl;
}

syntax::RecordDecl BuildRegionOptionsDecl() {
  syntax::RecordDecl record;
  record.vis = syntax::Visibility::Public;
  record.name = "RegionOptions";
  record.implements = {};
  record.members = {};

  syntax::FieldDecl stack_size;
  stack_size.vis = syntax::Visibility::Public;
  stack_size.name = "stack_size";
  stack_size.type = MakeTypePrimNode("usize");
  stack_size.init_opt = MakeLiteralExpr(syntax::TokenKind::IntLiteral, "0");
  record.members.emplace_back(stack_size);

  syntax::FieldDecl name;
  name.vis = syntax::Visibility::Public;
  name.name = "name";
  name.type = MakeTypeStringNode();
  name.init_opt = MakeLiteralExpr(syntax::TokenKind::StringLiteral, "\"\"");
  record.members.emplace_back(name);

  return record;
}

PathKey RegionOptionsKey() {
  syntax::Path path;
  path.emplace_back("RegionOptions");
  return PathKeyOf(path);
}


std::optional<std::string_view> CodeForResolveDiag(
    std::string_view diag_id) {
  if (diag_id == "ResolveExpr-Ident-Err" ||
      diag_id == "ResolveQual-Name-Err" ||
      diag_id == "ResolveQual-Apply-Err" ||
      diag_id == "ResolveQual-Apply-Brace-Err" ||
      diag_id == "Expr-Unresolved-Err") {
    return "E-MOD-1301";
  }
  if (diag_id == "ResolveModulePath-Err") {
    return "E-MOD-1304";
  }
  if (diag_id == "Validate-ModulePath-Reserved-Err") {
    return "E-CNF-0402";
  }
  if (diag_id == "Validate-Module-Keyword-Err") {
    return "E-CNF-0401";
  }
  if (diag_id == "Validate-Module-Prim-Shadow-Err") {
    return "E-CNF-0403";
  }
  if (diag_id == "Validate-Module-Special-Shadow-Err") {
    return "E-CNF-0404";
  }
  if (diag_id == "Validate-Module-Async-Shadow-Err") {
    return "E-CNF-0405";
  }
  if (diag_id == "Protected-TopLevel-Err") {
    return "E-MOD-2440";
  }
  if (diag_id == "Intro-Reserved-Gen-Err" ||
      diag_id == "Shadow-Reserved-Gen-Err") {
    return "E-CNF-0406";
  }
  if (diag_id == "Intro-Reserved-Cursive-Err" ||
      diag_id == "Shadow-Reserved-Cursive-Err") {
    return "E-CNF-0402";
  }
  if (diag_id == "Intro-Shadow-Required") {
    return "E-MOD-1303";
  }
  if (diag_id == "Shadow-Unnecessary") {
    return "E-MOD-1306";
  }
  if (diag_id == "Access-Err") {
    return "E-MOD-1207";
  }
  return std::nullopt;
}

std::optional<core::Span> SpanOfItem(const syntax::ASTItem& item) {
  return std::visit(
      [](const auto& it) -> std::optional<core::Span> { return it.span; },
      item);
}

core::DiagnosticStream EmitResolveDiag(core::DiagnosticStream diags,
                                       std::string_view diag_id,
                                       const std::optional<core::Span>& span) {
  const auto code = CodeForResolveDiag(diag_id);
  if (!code.has_value()) {
    return diags;
  }
  if (auto diag = core::MakeDiagnostic(*code, span)) {
    diags = core::Emit(diags, *diag);
  }
  return diags;
}

syntax::Path FullPath(const syntax::ModulePath& module_path,
                      std::string_view name) {
  syntax::Path out = module_path;
  out.emplace_back(name);
  return out;
}

}  // namespace

void PopulateSigma(ScopeContext& ctx) {
  ctx.sigma.types.clear();
  ctx.sigma.classes.clear();
  {
    syntax::Path path;
    path.emplace_back("Drop");
    ctx.sigma.classes[PathKeyOf(path)] = BuildDropClassDecl();
  }
  {
    syntax::Path path;
    path.emplace_back("Bitcopy");
    ctx.sigma.classes[PathKeyOf(path)] = BuildBitcopyClassDecl();
  }
  {
    syntax::Path path;
    path.emplace_back("Clone");
    ctx.sigma.classes[PathKeyOf(path)] = BuildCloneClassDecl();
  }
  ctx.sigma.types[RegionOptionsKey()] = BuildRegionOptionsDecl();
  {
    syntax::Path path;
    path.emplace_back("Region");
    ctx.sigma.types[PathKeyOf(path)] = BuildRegionModalDecl();
  }
  {
    syntax::Path path;
    path.emplace_back("File");
    ctx.sigma.types[PathKeyOf(path)] = BuildFileModalDecl();
  }
  {
    syntax::Path path;
    path.emplace_back("DirIter");
    ctx.sigma.types[PathKeyOf(path)] = BuildDirIterModalDecl();
  }
  {
    syntax::Path path;
    path.emplace_back("DirEntry");
    ctx.sigma.types[PathKeyOf(path)] = BuildDirEntryRecordDecl();
  }
  {
    syntax::Path path;
    path.emplace_back("FileKind");
    ctx.sigma.types[PathKeyOf(path)] = BuildFileKindEnumDecl();
  }
  {
    syntax::Path path;
    path.emplace_back("IoError");
    ctx.sigma.types[PathKeyOf(path)] = BuildIoErrorEnumDecl();
  }
  {
    syntax::Path path;
    path.emplace_back("AllocationError");
    ctx.sigma.types[PathKeyOf(path)] = BuildAllocationErrorEnumDecl();
  }
  {
    syntax::Path path;
    path.emplace_back("Context");
    ctx.sigma.types[PathKeyOf(path)] = BuildContextRecordDecl();
  }
  {
    syntax::Path path;
    path.emplace_back("System");
    ctx.sigma.types[PathKeyOf(path)] = BuildSystemRecordDecl();
  }
  for (const auto& module : ctx.sigma.mods) {
    for (const auto& item : module.items) {
      std::visit(
          [&](const auto& node) {
            using T = std::decay_t<decltype(node)>;
            if constexpr (std::is_same_v<T, syntax::RecordDecl> ||
                          std::is_same_v<T, syntax::EnumDecl> ||
                          std::is_same_v<T, syntax::ModalDecl> ||
                          std::is_same_v<T, syntax::TypeAliasDecl>) {
              ctx.sigma.types[PathKeyOf(FullPath(module.path, node.name))] = node;
            } else if constexpr (std::is_same_v<T, syntax::ClassDecl>) {
              ctx.sigma.classes[PathKeyOf(FullPath(module.path, node.name))] = node;
            } else {
              return;
            }
          },
          item);
    }
  }
}

ResolveModulesResult ResolveModules(ResolveContext& ctx) {
  SpecDefsResolverModules();
  ResolveModulesResult result;
  if (!ctx.ctx) {
    return result;
  }
  result.ok = true;
  bool had_resolve_error = false;
  const auto& modules = ctx.ctx->sigma.mods;
  result.modules.reserve(modules.size());
  for (const auto& module : modules) {
    ctx.ctx->current_module = module.path;
    const auto resolved = ResolveModule(ctx, module);
    if (!resolved.ok) {
      result.ok = false;
      if (!had_resolve_error) {
        SPEC_RULE("ResolveModules-Err-Resolve");
        had_resolve_error = true;
      }
      if (resolved.diag_id.has_value()) {
        result.diags = EmitResolveDiag(result.diags, *resolved.diag_id,
                                       resolved.span);
      }
    } else {
      result.modules.push_back(resolved.module);
    }
  }
  if (result.ok) {
    SPEC_RULE("ResolveModules-Ok");
  }
  return result;
}

ResolveModuleResult ResolveModule(ResolveContext& ctx,
                                  const syntax::ASTModule& module) {
  SpecDefsResolverModules();
  SPEC_RULE("Res-Start");
  if (!ctx.ctx || !ctx.name_maps || !ctx.module_names) {
    return {};
  }

  const auto collected =
      CollectNames(*ctx.ctx, *ctx.name_maps, *ctx.module_names, module);
  if (!collected.ok) {
    SPEC_RULE("ResolveModule-Err");
    return {false, collected.diag_id, collected.span, {}};
  }

  SPEC_RULE("Res-Names");

  if (ReservedModulePath(module.path)) {
    SPEC_RULE("Validate-ModulePath-Reserved-Err");
    return {false, "Validate-ModulePath-Reserved-Err", std::nullopt, {}};
  }
  SPEC_RULE("Validate-ModulePath-Ok");

  const auto names_ok = ValidateModuleNames(collected.names);
  if (!names_ok.ok) {
    SPEC_RULE("ResolveModule-Err");
    return {false, names_ok.diag_id, std::nullopt, {}};
  }

  ScopeContext module_ctx;
  module_ctx.project = ctx.ctx->project;
  module_ctx.sigma = ctx.ctx->sigma;
  module_ctx.current_module = module.path;
  module_ctx.scopes = {Scope{}, collected.names, UniverseBindings()};

  ResolveContext child = ctx;
  child.ctx = &module_ctx;

  const auto items = ResolveItems(child, module.items);
  if (!items.ok) {
    SPEC_RULE("ResolveModule-Err");
    return {false, items.diag_id, items.span, {}};
  }

  SPEC_RULE("Res-Items");
  SPEC_RULE("ResolveModule-Ok");
  syntax::ASTModule out = module;
  out.items = items.value;
  return {true, std::nullopt, std::nullopt, std::move(out)};
}

ResItemsResult ResolveItems(ResolveContext& ctx,
                            const std::vector<syntax::ASTItem>& items) {
  SpecDefsResolverModules();
  SPEC_RULE("Res-Start");
  ResItemsResult result;
  if (items.empty()) {
    SPEC_RULE("ResolveItems-Empty");
    result.ok = true;
    return result;
  }
  result.value.reserve(items.size());
  for (const auto& item : items) {
    const auto vis = TopLevelVis(item);
    if (!vis.ok) {
      SPEC_RULE("ResolveItems-Err");
      return {false, vis.diag_id, SpanOfItem(item), {}};
    }
    const auto resolved = ResolveItem(ctx, item);
    if (!resolved.ok) {
      SPEC_RULE("ResolveItems-Err");
      return {false, resolved.diag_id, resolved.span, {}};
    }
    result.value.push_back(resolved.value);
    SPEC_RULE("ResolveItems-Cons");
  }
  result.ok = true;
  return result;
}

}  // namespace cursive0::sema
