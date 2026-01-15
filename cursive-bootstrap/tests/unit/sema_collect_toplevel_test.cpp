#include <cassert>
#include <string>
#include <utility>
#include <vector>

#include "cursive0/core/assert_spec.h"
#include "cursive0/core/symbols.h"
#include "cursive0/sema/collect_toplevel.h"

namespace {

using cursive0::core::StringOfPath;
using cursive0::sema::CollectNames;
using cursive0::sema::CollectNameMaps;
using cursive0::sema::Entity;
using cursive0::sema::EntityKind;
using cursive0::sema::EntitySource;
using cursive0::sema::IdKeyOf;
using cursive0::sema::ItemBindings;
using cursive0::sema::ItemOfPath;
using cursive0::sema::ModuleNames;
using cursive0::sema::NameMap;
using cursive0::sema::NameMapBuildResult;
using cursive0::sema::NameMapTable;
using cursive0::sema::NamesStart;
using cursive0::sema::NamesState;
using cursive0::sema::NamesStep;
using cursive0::sema::PatNames;
using cursive0::sema::PathKeyOf;
using cursive0::sema::ResolveUsingPath;
using cursive0::sema::Scope;
using cursive0::sema::ScopeContext;
using cursive0::sema::UsingNames;
using cursive0::syntax::ASTItem;
using cursive0::syntax::ASTModule;
using cursive0::syntax::Binding;
using cursive0::syntax::ClassDecl;
using cursive0::syntax::EnumDecl;
using cursive0::syntax::EnumPattern;
using cursive0::syntax::EnumPayloadPattern;
using cursive0::syntax::ErrorItem;
using cursive0::syntax::FieldPattern;
using cursive0::syntax::Identifier;
using cursive0::syntax::IdentifierPattern;
using cursive0::syntax::LiteralPattern;
using cursive0::syntax::ModalDecl;
using cursive0::syntax::ModalPattern;
using cursive0::syntax::ModalRecordPayload;
using cursive0::syntax::ModulePath;
using cursive0::syntax::Pattern;
using cursive0::syntax::PatternPtr;
using cursive0::syntax::ProcedureDecl;
using cursive0::syntax::RangeKind;
using cursive0::syntax::RangePattern;
using cursive0::syntax::RecordDecl;
using cursive0::syntax::RecordPattern;
using cursive0::syntax::StaticDecl;
using cursive0::syntax::Token;
using cursive0::syntax::TuplePattern;
using cursive0::syntax::TuplePayloadPattern;
using cursive0::syntax::TypedPattern;
using cursive0::syntax::TypeAliasDecl;
using cursive0::syntax::UsingDecl;
using cursive0::syntax::UsingList;
using cursive0::syntax::UsingPath;
using cursive0::syntax::UsingSpec;
using cursive0::syntax::Visibility;
using cursive0::syntax::WildcardPattern;

cursive0::core::Span EmptySpan() {
  return {};
}

PatternPtr MakeIdentPattern(const std::string& name) {
  auto pat = std::make_shared<Pattern>();
  pat->span = EmptySpan();
  pat->node = IdentifierPattern{name};
  return pat;
}

PatternPtr MakeTypedPattern(const std::string& name) {
  auto pat = std::make_shared<Pattern>();
  pat->span = EmptySpan();
  pat->node = TypedPattern{name, nullptr};
  return pat;
}

PatternPtr MakeWildcardPattern() {
  auto pat = std::make_shared<Pattern>();
  pat->span = EmptySpan();
  pat->node = WildcardPattern{};
  return pat;
}

PatternPtr MakeLiteralPattern() {
  auto pat = std::make_shared<Pattern>();
  pat->span = EmptySpan();
  pat->node = LiteralPattern{Token{}};
  return pat;
}

PatternPtr MakeTuplePattern(const std::vector<PatternPtr>& elements) {
  auto pat = std::make_shared<Pattern>();
  pat->span = EmptySpan();
  pat->node = TuplePattern{elements};
  return pat;
}

PatternPtr MakeRecordPattern(const std::vector<FieldPattern>& fields) {
  auto pat = std::make_shared<Pattern>();
  pat->span = EmptySpan();
  pat->node = RecordPattern{ModulePath{"rec"}, fields};
  return pat;
}

PatternPtr MakeEnumPatternNone() {
  auto pat = std::make_shared<Pattern>();
  pat->span = EmptySpan();
  pat->node = EnumPattern{ModulePath{"pkg"}, "Variant", std::nullopt};
  return pat;
}

PatternPtr MakeEnumPatternTuple(const std::vector<PatternPtr>& elems) {
  auto pat = std::make_shared<Pattern>();
  pat->span = EmptySpan();
  TuplePayloadPattern payload{elems};
  pat->node = EnumPattern{ModulePath{"pkg"}, "Variant",
                          EnumPayloadPattern{payload}};
  return pat;
}

PatternPtr MakeEnumPatternRecord(const std::vector<FieldPattern>& fields) {
  auto pat = std::make_shared<Pattern>();
  pat->span = EmptySpan();
  cursive0::syntax::RecordPayloadPattern payload{fields};
  pat->node = EnumPattern{ModulePath{"pkg"}, "Variant",
                          EnumPayloadPattern{payload}};
  return pat;
}

PatternPtr MakeRangePattern(PatternPtr lo, PatternPtr hi) {
  auto pat = std::make_shared<Pattern>();
  pat->span = EmptySpan();
  pat->node = RangePattern{RangeKind::Inclusive, std::move(lo), std::move(hi)};
  return pat;
}

PatternPtr MakeModalPattern(const std::vector<FieldPattern>& fields) {
  auto pat = std::make_shared<Pattern>();
  pat->span = EmptySpan();
  ModalRecordPayload payload{fields};
  pat->node = ModalPattern{"State", payload};
  return pat;
}

ProcedureDecl MakeProcedure(const std::string& name,
                            Visibility vis = Visibility::Public) {
  ProcedureDecl decl;
  decl.vis = vis;
  decl.name = name;
  decl.params = {};
  decl.return_type_opt = nullptr;
  decl.body = std::make_shared<cursive0::syntax::Block>();
  decl.span = EmptySpan();
  decl.doc = {};
  return decl;
}

RecordDecl MakeRecord(const std::string& name,
                      Visibility vis = Visibility::Public) {
  RecordDecl decl;
  decl.vis = vis;
  decl.name = name;
  decl.implements = {};
  decl.members = {};
  decl.span = EmptySpan();
  decl.doc = {};
  return decl;
}

EnumDecl MakeEnum(const std::string& name, Visibility vis = Visibility::Public) {
  EnumDecl decl;
  decl.vis = vis;
  decl.name = name;
  decl.implements = {};
  decl.variants = {};
  decl.span = EmptySpan();
  decl.doc = {};
  return decl;
}

ModalDecl MakeModal(const std::string& name,
                    Visibility vis = Visibility::Public) {
  ModalDecl decl;
  decl.vis = vis;
  decl.name = name;
  decl.implements = {};
  decl.states = {};
  decl.span = EmptySpan();
  decl.doc = {};
  return decl;
}

ClassDecl MakeClass(const std::string& name, Visibility vis = Visibility::Public) {
  ClassDecl decl;
  decl.vis = vis;
  decl.name = name;
  decl.supers = {};
  decl.items = {};
  decl.span = EmptySpan();
  decl.doc = {};
  return decl;
}

TypeAliasDecl MakeTypeAlias(const std::string& name,
                            Visibility vis = Visibility::Public) {
  TypeAliasDecl decl;
  decl.vis = vis;
  decl.name = name;
  decl.type = nullptr;
  decl.span = EmptySpan();
  decl.doc = {};
  return decl;
}

StaticDecl MakeStatic(const PatternPtr& pat,
                      Visibility vis = Visibility::Public) {
  StaticDecl decl;
  decl.vis = vis;
  decl.mut = cursive0::syntax::Mutability::Let;
  Binding binding;
  binding.pat = pat;
  binding.type_opt = nullptr;
  binding.op = Token{};
  binding.init = nullptr;
  binding.span = EmptySpan();
  decl.binding = binding;
  decl.span = EmptySpan();
  decl.doc = {};
  return decl;
}

UsingDecl MakeUsingPath(const ModulePath& path,
                        std::optional<Identifier> alias = std::nullopt,
                        Visibility vis = Visibility::Public) {
  UsingDecl decl;
  decl.vis = vis;
  decl.clause = UsingPath{path, alias};
  decl.span = EmptySpan();
  decl.doc = {};
  return decl;
}

UsingDecl MakeUsingList(const ModulePath& path,
                        const std::vector<UsingSpec>& specs,
                        Visibility vis = Visibility::Public) {
  UsingDecl decl;
  decl.vis = vis;
  decl.clause = UsingList{path, specs};
  decl.span = EmptySpan();
  decl.doc = {};
  return decl;
}

ASTModule MakeModule(const ModulePath& path, const std::vector<ASTItem>& items) {
  ASTModule mod;
  mod.path = path;
  mod.items = items;
  mod.module_doc = {};
  return mod;
}

ModuleNames MakeModuleNames(const std::vector<ASTModule>& modules) {
  ModuleNames names;
  for (const auto& mod : modules) {
    names.push_back(StringOfPath(mod.path));
  }
  return names;
}

NameMap MakeNameMap(const ModulePath& module_path,
                    const std::vector<std::pair<std::string, EntityKind>>& entries) {
  NameMap map;
  for (const auto& entry : entries) {
    map.emplace(IdKeyOf(entry.first),
                Entity{entry.second, module_path, std::nullopt,
                       EntitySource::Decl});
  }
  return map;
}

ScopeContext MakeContext(const std::vector<ASTModule>& modules,
                         const ModulePath& current) {
  ScopeContext ctx;
  ctx.sigma.mods = modules;
  ctx.current_module = current;
  ctx.scopes = {Scope{}, Scope{}, Scope{}};
  return ctx;
}

}  // namespace

int main() {
  {
    SPEC_COV("Pat-Typed");
    const auto names = PatNames(*MakeTypedPattern("x"));
    assert(names.size() == 1);
    assert(names[0] == "x");
  }

  {
    SPEC_COV("Pat-Wild");
    const auto names = PatNames(*MakeWildcardPattern());
    assert(names.empty());
  }

  {
    SPEC_COV("Pat-Lit");
    const auto names = PatNames(*MakeLiteralPattern());
    assert(names.empty());
  }

  {
    SPEC_COV("Pat-Record-Field-Explicit");
    SPEC_COV("Pat-Record-Field-Implicit");
    FieldPattern explicit_field{"a", MakeIdentPattern("x"), EmptySpan()};
    FieldPattern implicit_field{"b", nullptr, EmptySpan()};
    const auto names = PatNames(*MakeRecordPattern({explicit_field, implicit_field}));
    assert(names.size() == 2);
    assert(names[0] == "x");
    assert(names[1] == "b");
  }

  {
    SPEC_COV("Pat-Enum-None");
    const auto names = PatNames(*MakeEnumPatternNone());
    assert(names.empty());
  }

  {
    SPEC_COV("Pat-Enum-Tuple");
    const auto names = PatNames(*MakeEnumPatternTuple({MakeIdentPattern("x")}));
    assert(names.size() == 1);
  }

  {
    SPEC_COV("Pat-Enum-Record");
    FieldPattern implicit_field{"c", nullptr, EmptySpan()};
    const auto names = PatNames(*MakeEnumPatternRecord({implicit_field}));
    assert(names.size() == 1);
    assert(names[0] == "c");
  }

  {
    SPEC_COV("Pat-Range");
    const auto names =
        PatNames(*MakeRangePattern(MakeIdentPattern("lo"),
                                   MakeIdentPattern("hi")));
    assert(names.size() == 2);
  }

  {
    const auto names =
        PatNames(*MakeModalPattern({FieldPattern{"m", nullptr, EmptySpan()}}));
    assert(names.size() == 1);
  }

  {
    SPEC_COV("ItemOfPath");
    SPEC_COV("ItemOfPath-None");
    ASTModule pkg = MakeModule({"pkg"}, {MakeProcedure("Thing")});
    ScopeContext ctx = MakeContext({pkg}, pkg.path);
    NameMapTable name_maps;
    name_maps.emplace(PathKeyOf(pkg.path),
                      MakeNameMap(pkg.path, {{"Thing", EntityKind::Value}}));
    ModuleNames module_names = MakeModuleNames({pkg});

    const auto ok = ItemOfPath(ctx, name_maps, module_names,
                               ModulePath{"pkg", "Thing"});
    assert(ok.ok);
    const auto none = ItemOfPath(ctx, name_maps, module_names,
                                 ModulePath{"pkg", "Missing"});
    assert(!none.ok);
  }

  {
    SPEC_COV("Resolve-Using-Item");
    SPEC_COV("Resolve-Using-Module");
    SPEC_COV("Resolve-Using-Ambig");
    SPEC_COV("Resolve-Using-None");
    ASTModule pkg = MakeModule({"pkg"}, {MakeProcedure("Thing")});
    ASTModule pkg_item = MakeModule({"pkg", "Thing"}, {});
    ScopeContext ctx = MakeContext({pkg, pkg_item}, pkg.path);
    NameMapTable name_maps;
    name_maps.emplace(PathKeyOf(pkg.path),
                      MakeNameMap(pkg.path, {{"Thing", EntityKind::Value}}));
    name_maps.emplace(PathKeyOf(pkg_item.path), NameMap{});
    ModuleNames module_names = MakeModuleNames({pkg, pkg_item});

    const auto item = ResolveUsingPath(ctx, name_maps, module_names,
                                       ModulePath{"pkg", "Thing"});
    assert(!item.ok && item.diag_id == "Resolve-Using-Ambig");

    const auto mod = ResolveUsingPath(ctx, name_maps, module_names,
                                      ModulePath{"pkg"});
    assert(mod.ok && mod.is_module);

    ModuleNames item_only_names = {StringOfPath(pkg.path)};
    const auto item_only = ResolveUsingPath(ctx, name_maps, item_only_names,
                                            ModulePath{"pkg", "Thing"});
    assert(item_only.ok && item_only.item.has_value());

    const auto none = ResolveUsingPath(ctx, name_maps, module_names,
                                       ModulePath{"nope"});
    assert(!none.ok && none.diag_id == "Resolve-Using-None");
  }

  {
    SPEC_COV("Using-Path-Item");
    SPEC_COV("Using-Path-Module");
    SPEC_COV("Using-Path-Item-Public-Err");
    ASTModule pkg = MakeModule({"pkg"},
                               {MakeProcedure("Thing", Visibility::Public),
                                MakeProcedure("Secret", Visibility::Private)});
    ASTModule main_mod = MakeModule({"main"}, {});
    ScopeContext ctx = MakeContext({pkg, main_mod}, main_mod.path);
    NameMapTable name_maps;
    name_maps.emplace(PathKeyOf(pkg.path),
                      MakeNameMap(pkg.path, {{"Thing", EntityKind::Value},
                                              {"Secret", EntityKind::Value}}));
    name_maps.emplace(PathKeyOf(main_mod.path), NameMap{});
    ModuleNames module_names = MakeModuleNames({pkg, main_mod});

    const UsingDecl using_item = MakeUsingPath({"pkg", "Thing"}, std::nullopt,
                                               Visibility::Internal);
    const auto item_names = UsingNames(ctx, name_maps, module_names, using_item);
    assert(item_names.ok);
    assert(item_names.bindings.size() == 1);

    const UsingDecl using_module =
        MakeUsingPath({"pkg"}, std::nullopt, Visibility::Internal);
    const auto module_names_res =
        UsingNames(ctx, name_maps, module_names, using_module);
    assert(module_names_res.ok);
    assert(module_names_res.bindings.size() == 1);

    const UsingDecl using_public =
        MakeUsingPath({"pkg", "Secret"}, std::nullopt, Visibility::Public);
    const auto public_res =
        UsingNames(ctx, name_maps, module_names, using_public);
    assert(!public_res.ok);
    assert(public_res.diag_id == "Using-Path-Item-Public-Err");
  }

  {
    SPEC_COV("Using-List");
    SPEC_COV("Using-List-Dup");
    SPEC_COV("Using-List-Public-Err");
    ASTModule pkg = MakeModule({"pkg"},
                               {MakeProcedure("Thing", Visibility::Public),
                                MakeProcedure("Secret", Visibility::Private)});
    ASTModule main_mod = MakeModule({"main"}, {});
    ScopeContext ctx = MakeContext({pkg, main_mod}, main_mod.path);
    NameMapTable name_maps;
    name_maps.emplace(PathKeyOf(pkg.path),
                      MakeNameMap(pkg.path, {{"Thing", EntityKind::Value},
                                              {"Secret", EntityKind::Value}}));
    name_maps.emplace(PathKeyOf(main_mod.path), NameMap{});
    ModuleNames module_names = MakeModuleNames({pkg, main_mod});

    UsingDecl list_ok =
        MakeUsingList({"pkg"}, {UsingSpec{"Thing", std::nullopt}},
                      Visibility::Internal);
    const auto list_ok_res = UsingNames(ctx, name_maps, module_names, list_ok);
    assert(list_ok_res.ok);

    UsingDecl list_dup =
        MakeUsingList({"pkg"},
                      {UsingSpec{"Thing", std::nullopt},
                       UsingSpec{"Thing", std::nullopt}},
                      Visibility::Internal);
    const auto list_dup_res =
        UsingNames(ctx, name_maps, module_names, list_dup);
    assert(!list_dup_res.ok);
    assert(list_dup_res.diag_id == "Using-List-Dup");

    UsingDecl list_public =
        MakeUsingList({"pkg"}, {UsingSpec{"Secret", std::nullopt}},
                      Visibility::Public);
    const auto list_public_res =
        UsingNames(ctx, name_maps, module_names, list_public);
    assert(!list_public_res.ok);
    assert(list_public_res.diag_id == "Using-List-Public-Err");
  }

  {
    SPEC_COV("Bind-Procedure");
    SPEC_COV("Bind-Record");
    SPEC_COV("Bind-Enum");
    SPEC_COV("Bind-Class");
    SPEC_COV("Bind-TypeAlias");
    SPEC_COV("Bind-Static");
    SPEC_COV("Bind-ErrorItem");
    SPEC_COV("Bind-Using");
    SPEC_COV("Bind-Using-Err");
    ASTModule pkg = MakeModule({"pkg"}, {MakeProcedure("Thing")});
    ASTModule main_mod = MakeModule({"main"}, {});
    ScopeContext ctx = MakeContext({pkg, main_mod}, main_mod.path);
    NameMapTable name_maps;
    name_maps.emplace(PathKeyOf(pkg.path),
                      MakeNameMap(pkg.path, {{"Thing", EntityKind::Value}}));
    name_maps.emplace(PathKeyOf(main_mod.path), NameMap{});
    ModuleNames module_names = MakeModuleNames({pkg, main_mod});

    auto bind_proc =
        ItemBindings(ctx, name_maps, module_names,
                     ASTItem{MakeProcedure("Proc")}, main_mod.path);
    assert(bind_proc.ok);
    assert(bind_proc.bindings.size() == 1);
    assert(bind_proc.bindings[0].ent.kind == EntityKind::Value);

    auto bind_record =
        ItemBindings(ctx, name_maps, module_names,
                     ASTItem{MakeRecord("Rec")}, main_mod.path);
    assert(bind_record.ok);
    assert(bind_record.bindings[0].ent.kind == EntityKind::Type);

    auto bind_enum =
        ItemBindings(ctx, name_maps, module_names,
                     ASTItem{MakeEnum("En")}, main_mod.path);
    assert(bind_enum.ok);

    auto bind_class =
        ItemBindings(ctx, name_maps, module_names,
                     ASTItem{MakeClass("Cls")}, main_mod.path);
    assert(bind_class.ok);
    assert(bind_class.bindings[0].ent.kind == EntityKind::Class);

    auto bind_alias =
        ItemBindings(ctx, name_maps, module_names,
                     ASTItem{MakeTypeAlias("Alias")}, main_mod.path);
    assert(bind_alias.ok);

    auto bind_static =
        ItemBindings(ctx, name_maps, module_names,
                     ASTItem{MakeStatic(MakeIdentPattern("s"))},
                     main_mod.path);
    assert(bind_static.ok);

    auto bind_error = ItemBindings(ctx, name_maps, module_names,
                                   ASTItem{ErrorItem{EmptySpan()}},
                                   main_mod.path);
    assert(bind_error.ok);

    auto bind_using = ItemBindings(ctx, name_maps, module_names,
                                   ASTItem{MakeUsingPath({"pkg", "Thing"})},
                                   main_mod.path);
    assert(bind_using.ok);

    auto bind_using_err =
        ItemBindings(ctx, name_maps, module_names,
                     ASTItem{MakeUsingPath({"missing"})}, main_mod.path);
    assert(!bind_using_err.ok);
  }

  {
    SPEC_COV("Collect-Ok");
    SPEC_COV("Collect-Scan");
    SPEC_COV("Names-Start");
    SPEC_COV("Names-Step");
    SPEC_COV("Names-Done");
    ASTModule mod = MakeModule({"main"},
                               {MakeProcedure("A"), MakeRecord("B")});
    ScopeContext ctx = MakeContext({mod}, mod.path);
    NameMapTable name_maps;
    name_maps.emplace(PathKeyOf(mod.path),
                      MakeNameMap(mod.path, {{"A", EntityKind::Value},
                                              {"B", EntityKind::Type}}));
    ModuleNames module_names = MakeModuleNames({mod});

    const auto collected = CollectNames(ctx, name_maps, module_names, mod);
    assert(collected.ok);
    assert(collected.names.size() == 2);

    NamesState state = NamesStart(mod);
    state = NamesStep(ctx, name_maps, module_names, state);
    state = NamesStep(ctx, name_maps, module_names, state);
    state = NamesStep(ctx, name_maps, module_names, state);
    assert(state.kind == NamesState::Kind::Done);
  }

  {
    ASTModule mod1 = MakeModule({"main"},
                               {MakeProcedure("A"), MakeRecord("B")});
    ASTModule mod2 = MakeModule({"main"},
                               {MakeRecord("B"), MakeProcedure("A")});
    ScopeContext ctx = MakeContext({mod1}, mod1.path);
    NameMapTable name_maps;
    ModuleNames module_names = MakeModuleNames({mod1});

    const auto res1 = CollectNames(ctx, name_maps, module_names, mod1);
    const auto res2 = CollectNames(ctx, name_maps, module_names, mod2);
    assert(res1.ok && res2.ok);
    assert(res1.names.size() == res2.names.size());
  }

  {
    SPEC_COV("Collect-Dup");
    SPEC_COV("Names-Step-Dup");
    ASTModule mod = MakeModule({"main"},
                               {MakeProcedure("A"), MakeProcedure("A")});
    ScopeContext ctx = MakeContext({mod}, mod.path);
    NameMapTable name_maps;
    name_maps.emplace(PathKeyOf(mod.path),
                      MakeNameMap(mod.path, {{"A", EntityKind::Value}}));
    ModuleNames module_names = MakeModuleNames({mod});

    const auto collected = CollectNames(ctx, name_maps, module_names, mod);
    assert(!collected.ok);
    assert(collected.diag_id == "Collect-Dup");

    NamesState state = NamesStart(mod);
    state = NamesStep(ctx, name_maps, module_names, state);
    state = NamesStep(ctx, name_maps, module_names, state);
    assert(state.kind == NamesState::Kind::Error);
  }

  {
    SPEC_COV("Collect-Err");
    SPEC_COV("Names-Step-Err");
    ASTModule mod = MakeModule({"main"}, {MakeUsingPath({"missing"})});
    ScopeContext ctx = MakeContext({mod}, mod.path);
    NameMapTable name_maps;
    name_maps.emplace(PathKeyOf(mod.path), NameMap{});
    ModuleNames module_names = MakeModuleNames({mod});

    const auto collected = CollectNames(ctx, name_maps, module_names, mod);
    assert(!collected.ok);

    NamesState state = NamesStart(mod);
    state = NamesStep(ctx, name_maps, module_names, state);
    assert(state.kind == NamesState::Kind::Error);
  }

  {
    SPEC_COV("DeclNames-Empty");
    SPEC_COV("DeclNames-Using");
    SPEC_COV("DeclNames-Item");
    ASTModule mod = MakeModule({"main"},
                               {MakeUsingPath({"pkg"}), MakeProcedure("A")});
    const auto names = cursive0::sema::DeclNames(mod);
    assert(names.size() == 1);

    ASTModule empty_mod = MakeModule({"empty"}, {});
    const auto empty_names = cursive0::sema::DeclNames(empty_mod);
    assert(empty_names.empty());
  }

  {
    ScopeContext ctx = MakeContext({}, {});
    NameMapBuildResult maps = CollectNameMaps(ctx);
    assert(maps.name_maps.empty());
  }

  return 0;
}
