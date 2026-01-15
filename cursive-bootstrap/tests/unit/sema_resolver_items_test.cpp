#include <cassert>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "cursive0/core/assert_spec.h"
#include "cursive0/core/symbols.h"
#include "cursive0/sema/resolver.h"
#include "cursive0/sema/scopes.h"
#include "cursive0/syntax/ast.h"
#include "cursive0/syntax/token.h"

namespace {

using cursive0::core::StringOfPath;
using cursive0::sema::Entity;
using cursive0::sema::EntityKind;
using cursive0::sema::EntitySource;
using cursive0::sema::IdKeyOf;
using cursive0::sema::ModuleNames;
using cursive0::sema::NameMap;
using cursive0::sema::NameMapTable;
using cursive0::sema::PathKeyOf;
using cursive0::sema::ResolveContext;
using cursive0::sema::ResolveItem;
using cursive0::sema::ResolveModule;
using cursive0::sema::ResolveModules;
using cursive0::sema::Scope;
using cursive0::sema::ScopeContext;
using cursive0::syntax::ASTItem;
using cursive0::syntax::ASTModule;
using cursive0::syntax::Block;
using cursive0::syntax::ClassDecl;
using cursive0::syntax::ClassFieldDecl;
using cursive0::syntax::ClassMethodDecl;
using cursive0::syntax::ClassPath;
using cursive0::syntax::EnumDecl;
using cursive0::syntax::EnumPayload;
using cursive0::syntax::Expr;
using cursive0::syntax::ExprPtr;
using cursive0::syntax::FieldDecl;
using cursive0::syntax::Identifier;
using cursive0::syntax::LiteralExpr;
using cursive0::syntax::ModalDecl;
using cursive0::syntax::ModulePath;
using cursive0::syntax::Param;
using cursive0::syntax::Path;
using cursive0::syntax::PatternPtr;
using cursive0::syntax::ProcedureDecl;
using cursive0::syntax::RecordDecl;
using cursive0::syntax::RecordMember;
using cursive0::syntax::StateBlock;
using cursive0::syntax::StateFieldDecl;
using cursive0::syntax::StateMember;
using cursive0::syntax::StateMethodDecl;
using cursive0::syntax::StaticDecl;
using cursive0::syntax::Token;
using cursive0::syntax::TokenKind;
using cursive0::syntax::TransitionDecl;
using cursive0::syntax::Type;
using cursive0::syntax::TypeAliasDecl;
using cursive0::syntax::TypeDynamic;
using cursive0::syntax::TypeModalState;
using cursive0::syntax::TypePath;
using cursive0::syntax::TypePathType;
using cursive0::syntax::TypePerm;
using cursive0::syntax::TypePermType;
using cursive0::syntax::UsingDecl;
using cursive0::syntax::UsingPath;
using cursive0::syntax::VariantDecl;
using cursive0::syntax::VariantPayload;
using cursive0::syntax::VariantPayloadRecord;
using cursive0::syntax::VariantPayloadTuple;
using cursive0::syntax::Visibility;

struct TestEnv {
  ScopeContext ctx;
  NameMapTable name_maps;
  ModuleNames module_names;
  ResolveContext res_ctx;

  void Init(const ModulePath& current = ModulePath{"main"}) {
    ctx = ScopeContext{};
    ctx.current_module = current;
    ctx.scopes = {Scope{}, Scope{}, Scope{}};
    name_maps.clear();
    module_names.clear();
    name_maps.emplace(PathKeyOf(current), NameMap{});
    module_names.push_back(StringOfPath(current));
    res_ctx.ctx = &ctx;
    res_ctx.name_maps = &name_maps;
    res_ctx.module_names = &module_names;
    res_ctx.can_access = nullptr;
  }
};

cursive0::core::Span EmptySpan() {
  return {};
}

Token MakeToken(TokenKind kind, std::string_view lexeme) {
  Token token;
  token.kind = kind;
  token.lexeme = std::string(lexeme);
  return token;
}

ExprPtr MakeLiteralExpr(TokenKind kind, std::string_view lexeme) {
  auto expr = std::make_shared<Expr>();
  expr->span = EmptySpan();
  expr->node = LiteralExpr{MakeToken(kind, lexeme)};
  return expr;
}

std::shared_ptr<Type> MakeTypePathType(const TypePath& path) {
  auto type = std::make_shared<Type>();
  type->span = EmptySpan();
  type->node = TypePathType{path};
  return type;
}

std::shared_ptr<Type> MakeTypePathType(std::string_view name) {
  TypePath path;
  path.emplace_back(std::string(name));
  return MakeTypePathType(path);
}

std::shared_ptr<Type> MakeTypeModalState(std::string_view name,
                                         std::string_view state) {
  auto type = std::make_shared<Type>();
  TypeModalState node;
  node.path = TypePath{std::string(name)};
  node.state = std::string(state);
  type->span = EmptySpan();
  type->node = std::move(node);
  return type;
}

std::shared_ptr<Type> MakeTypeDynamic(const ClassPath& path) {
  auto type = std::make_shared<Type>();
  TypeDynamic node;
  node.path = path;
  type->span = EmptySpan();
  type->node = std::move(node);
  return type;
}

std::shared_ptr<Type> MakeTypePerm(TypePerm perm, const std::shared_ptr<Type>& base) {
  auto type = std::make_shared<Type>();
  TypePermType node;
  node.perm = perm;
  node.base = base;
  type->span = EmptySpan();
  type->node = std::move(node);
  return type;
}

void AddValue(ScopeContext& ctx, std::string_view name) {
  ctx.scopes.front().emplace(
      IdKeyOf(name),
      Entity{EntityKind::Value, std::nullopt, std::nullopt, EntitySource::Decl});
}

void AddType(ScopeContext& ctx,
             std::string_view name,
             std::optional<ModulePath> origin = std::nullopt,
             std::optional<Identifier> target = std::nullopt) {
  ctx.scopes.front().emplace(
      IdKeyOf(name),
      Entity{EntityKind::Type, std::move(origin), std::move(target),
             EntitySource::Decl});
}

void AddModuleType(ScopeContext& ctx,
                   std::string_view name,
                   std::optional<ModulePath> origin = std::nullopt,
                   std::optional<Identifier> target = std::nullopt) {
  if (ctx.scopes.size() >= 2) {
    ctx.scopes[ctx.scopes.size() - 2].emplace(
        IdKeyOf(name),
        Entity{EntityKind::Type, std::move(origin), std::move(target),
               EntitySource::Decl});
  } else {
    AddType(ctx, name, std::move(origin), std::move(target));
  }
}

void AddClass(ScopeContext& ctx,
              std::string_view name,
              std::optional<ModulePath> origin = std::nullopt,
              std::optional<Identifier> target = std::nullopt) {
  ctx.scopes.front().emplace(
      IdKeyOf(name),
      Entity{EntityKind::Class, std::move(origin), std::move(target),
             EntitySource::Decl});
}

void AddName(NameMapTable& maps,
             const ModulePath& module,
             std::string_view name,
             EntityKind kind,
             std::optional<ModulePath> origin = std::nullopt,
             std::optional<Identifier> target = std::nullopt) {
  auto& map = maps[PathKeyOf(module)];
  map.emplace(
      IdKeyOf(name),
      Entity{kind, std::move(origin), std::move(target), EntitySource::Decl});
}

RecordDecl MakeRecordDecl(std::string_view name) {
  RecordDecl decl;
  decl.vis = Visibility::Public;
  decl.name = std::string(name);
  decl.implements = {};
  decl.members = {};
  decl.span = EmptySpan();
  decl.doc = {};
  return decl;
}

EnumDecl MakeEnumDecl(std::string_view name) {
  EnumDecl decl;
  decl.vis = Visibility::Public;
  decl.name = std::string(name);
  decl.implements = {};
  decl.variants = {};
  decl.span = EmptySpan();
  decl.doc = {};
  return decl;
}

void AddRecordType(ScopeContext& ctx, const TypePath& path) {
  ctx.sigma.types.emplace(PathKeyOf(path), MakeRecordDecl(path.back()));
}

void AddEnumType(ScopeContext& ctx, const TypePath& path) {
  ctx.sigma.types.emplace(PathKeyOf(path), MakeEnumDecl(path.back()));
}

FieldDecl MakeField(std::string_view name, const std::shared_ptr<Type>& type) {
  FieldDecl field;
  field.vis = Visibility::Public;
  field.name = std::string(name);
  field.type = type;
  field.init_opt = MakeLiteralExpr(TokenKind::IntLiteral, "0");
  field.span = EmptySpan();
  field.doc_opt = std::nullopt;
  return field;
}

ProcedureDecl MakeProcedure(std::string_view name,
                            std::vector<Param> params,
                            const std::shared_ptr<Type>& return_type_opt) {
  ProcedureDecl decl;
  decl.vis = Visibility::Public;
  decl.name = std::string(name);
  decl.params = std::move(params);
  decl.return_type_opt = return_type_opt;
  decl.body = nullptr;
  decl.span = EmptySpan();
  decl.doc = {};
  return decl;
}

ClassMethodDecl MakeClassMethod(std::string_view name,
                                const cursive0::syntax::Receiver& receiver,
                                const std::shared_ptr<Type>& return_type_opt,
                                const std::shared_ptr<Block>& body_opt) {
  ClassMethodDecl decl;
  decl.vis = Visibility::Public;
  decl.name = std::string(name);
  decl.receiver = receiver;
  decl.params = {};
  decl.return_type_opt = return_type_opt;
  decl.body_opt = body_opt;
  decl.span = EmptySpan();
  decl.doc_opt = std::nullopt;
  return decl;
}

ClassFieldDecl MakeClassField(std::string_view name,
                              const std::shared_ptr<Type>& type) {
  ClassFieldDecl decl;
  decl.vis = Visibility::Public;
  decl.name = std::string(name);
  decl.type = type;
  decl.span = EmptySpan();
  decl.doc_opt = std::nullopt;
  return decl;
}

StateFieldDecl MakeStateField(std::string_view name,
                              const std::shared_ptr<Type>& type) {
  StateFieldDecl decl;
  decl.vis = Visibility::Public;
  decl.name = std::string(name);
  decl.type = type;
  decl.span = EmptySpan();
  decl.doc_opt = std::nullopt;
  return decl;
}

StateMethodDecl MakeStateMethod(std::string_view name) {
  StateMethodDecl decl;
  decl.vis = Visibility::Public;
  decl.name = std::string(name);
  decl.params = {};
  decl.return_type_opt = nullptr;
  decl.body = nullptr;
  decl.span = EmptySpan();
  decl.doc_opt = std::nullopt;
  return decl;
}

TransitionDecl MakeTransition(std::string_view name, std::string_view target) {
  TransitionDecl decl;
  decl.vis = Visibility::Public;
  decl.name = std::string(name);
  decl.params = {};
  decl.target_state = std::string(target);
  decl.body = nullptr;
  decl.span = EmptySpan();
  decl.doc_opt = std::nullopt;
  return decl;
}

}  // namespace

int main() {
  {
    SPEC_COV("ResolveType-Path");
    SPEC_COV("ResolveType-Dynamic");
    SPEC_COV("ResolveType-ModalState");
    SPEC_COV("ResolveType-Hom");
    SPEC_COV("ResolveTypePath-Ident");
    SPEC_COV("ResolveTypePath-Ident-Local");
    SPEC_COV("ResolveTypePath-Qual");
    SPEC_COV("ResolveClassPath-Ident");
    SPEC_COV("ResolveClassPath-Qual");

    TestEnv env;
    env.Init();
    AddModuleType(env.ctx, "Local");
    AddType(env.ctx, "Alias", ModulePath{"pkg"}, Identifier{"Real"});
    AddClass(env.ctx, "Cls", ModulePath{"pkg"}, Identifier{"Cls"});
    AddEnumType(env.ctx, TypePath{"Local"});
    AddRecordType(env.ctx, TypePath{"Alias"});

    const ModulePath pkg = {"pkg"};
    env.module_names.push_back(StringOfPath(pkg));
    AddName(env.name_maps, pkg, "QualType", EntityKind::Type, pkg);
    AddName(env.name_maps, pkg, "QualClass", EntityKind::Class, pkg);

    const auto local_res = cursive0::sema::ResolveType(
        env.res_ctx, MakeTypePathType("Local"));
    assert(local_res.ok);

    const auto alias_res = cursive0::sema::ResolveType(
        env.res_ctx, MakeTypePathType("Alias"));
    assert(alias_res.ok);

    const auto qual_res = cursive0::sema::ResolveType(
        env.res_ctx, MakeTypePathType(TypePath{"pkg", "QualType"}));
    assert(qual_res.ok);

    const auto dyn_res = cursive0::sema::ResolveType(
        env.res_ctx, MakeTypeDynamic(ClassPath{"Cls"}));
    assert(dyn_res.ok);

    const auto dyn_qual_res = cursive0::sema::ResolveType(
        env.res_ctx, MakeTypeDynamic(ClassPath{"pkg", "QualClass"}));
    assert(dyn_qual_res.ok);

    const auto modal_res = cursive0::sema::ResolveType(
        env.res_ctx, MakeTypeModalState("Local", "State"));
    assert(modal_res.ok);

    const auto hom_res = cursive0::sema::ResolveType(
        env.res_ctx, MakeTypePerm(TypePerm::Const, MakeTypePathType("Local")));
    assert(hom_res.ok);
  }

  {
    SPEC_COV("ResolveItem-Static");
    SPEC_COV("ResolveItem-Using");
    SPEC_COV("ResolveItem-TypeAlias");

    TestEnv env;
    env.Init();
    AddType(env.ctx, "Alias");
    const ModulePath pkg = {"pkg"};
    env.module_names.push_back(StringOfPath(pkg));
    AddName(env.name_maps, pkg, "QualType", EntityKind::Type, pkg);

    StaticDecl stat;
    stat.vis = Visibility::Public;
    stat.mut = cursive0::syntax::Mutability::Let;
    stat.binding.init = MakeLiteralExpr(TokenKind::IntLiteral, "1");
    stat.binding.type_opt = nullptr;
    stat.binding.pat = nullptr;
    stat.binding.span = EmptySpan();
    stat.span = EmptySpan();
    stat.doc = {};
    const auto stat_res = ResolveItem(env.res_ctx, ASTItem{stat});
    assert(stat_res.ok);

    UsingDecl use;
    use.vis = Visibility::Public;
    UsingPath path;
    path.path = ModulePath{"pkg"};
    path.alias_opt = std::nullopt;
    use.clause = path;
    use.span = EmptySpan();
    use.doc = {};
    const auto use_res = ResolveItem(env.res_ctx, ASTItem{use});
    assert(use_res.ok);

    TypeAliasDecl alias;
    alias.vis = Visibility::Public;
    alias.name = "AliasType";
    alias.type = MakeTypePathType(TypePath{"pkg", "QualType"});
    alias.span = EmptySpan();
    alias.doc = {};
    const auto alias_res = ResolveItem(env.res_ctx, ASTItem{alias});
    assert(alias_res.ok);
  }

  {
    SPEC_COV("ResolveItem-Procedure");
    SPEC_COV("ResolveParams-Empty");
    SPEC_COV("ResolveParams-Cons");
    SPEC_COV("ResolveParam");
    SPEC_COV("ResolveTypeOpt-None");
    SPEC_COV("ResolveTypeOpt-Some");

    TestEnv env;
    env.Init();
    AddModuleType(env.ctx, "Local");

    const auto proc_empty = ResolveItem(
        env.res_ctx,
        ASTItem{MakeProcedure("p0", {}, nullptr)});
    assert(proc_empty.ok);

    Param param;
    param.mode = std::nullopt;
    param.name = "x";
    param.type = MakeTypePathType("Local");
    param.span = EmptySpan();

    const auto proc_one = ResolveItem(
        env.res_ctx,
        ASTItem{MakeProcedure("p1", {param}, MakeTypePathType("Local"))});
    assert(proc_one.ok);
  }

  {
    SPEC_COV("ResolveItem-Record");
    SPEC_COV("ResolveClassPathList-Cons");
    SPEC_COV("ResolveClassPathList-Empty");
    SPEC_COV("ResolveClassPath-Ident");
    SPEC_COV("ResolveClassPath-Qual");
    SPEC_COV("ResolveRecordMember-Field");
    SPEC_COV("ResolveRecordMember-Method");
    SPEC_COV("ResolveRecordMemberList-Cons");
    SPEC_COV("ResolveRecordMemberList-Empty");
    SPEC_COV("BindSelf-Record");
    SPEC_COV("ResolveReceiver-Explicit");

    TestEnv env;
    env.Init();
    AddType(env.ctx, "Local");
    AddClass(env.ctx, "Cls", ModulePath{"pkg"}, Identifier{"Cls"});
    const ModulePath pkg = {"pkg"};
    env.module_names.push_back(StringOfPath(pkg));
    AddName(env.name_maps, pkg, "QualClass", EntityKind::Class, pkg);

    RecordDecl record;
    record.vis = Visibility::Public;
    record.name = "Rec";
    record.implements = {ClassPath{"Cls"}, ClassPath{"pkg", "QualClass"}};
    record.members = {};
    record.span = EmptySpan();
    record.doc = {};

    record.members.push_back(MakeField("field", MakeTypePathType("Local")));

    cursive0::syntax::MethodDecl method;
    method.vis = Visibility::Public;
    method.override_flag = false;
    method.name = "m";
    method.receiver =
        cursive0::syntax::ReceiverExplicit{std::nullopt, MakeTypePathType("Self")};
    method.params = {};
    method.return_type_opt = nullptr;
    method.body = nullptr;
    method.span = EmptySpan();
    method.doc_opt = std::nullopt;
    record.members.push_back(method);

    const auto record_res = ResolveItem(env.res_ctx, ASTItem{record});
    assert(record_res.ok);

    RecordDecl empty;
    empty.vis = Visibility::Public;
    empty.name = "EmptyRec";
    empty.implements = {};
    empty.members = {};
    empty.span = EmptySpan();
    empty.doc = {};
    const auto empty_res = ResolveItem(env.res_ctx, ASTItem{empty});
    assert(empty_res.ok);
  }

  {
    SPEC_COV("ResolveItem-Class");
    SPEC_COV("ResolveClassItemList-Cons");
    SPEC_COV("ResolveClassItemList-Empty");
    SPEC_COV("ResolveClassItem-Field");
    SPEC_COV("ResolveClassItem-Method-Abstract");
    SPEC_COV("ResolveClassItem-Method-Concrete");
    SPEC_COV("BindSelf-Class");
    SPEC_COV("ResolveReceiver-Shorthand");
    SPEC_COV("ResolveReceiver-Explicit");

    TestEnv env;
    env.Init();
    AddType(env.ctx, "Local");

    ClassDecl cls;
    cls.vis = Visibility::Public;
    cls.name = "Widget";
    cls.supers = {};
    cls.items = {};
    cls.span = EmptySpan();
    cls.doc = {};

    cls.items.push_back(MakeClassField("value", MakeTypePathType("Local")));

    cls.items.push_back(MakeClassMethod(
        "read", cursive0::syntax::Receiver{cursive0::syntax::ReceiverShorthand{
                       cursive0::syntax::ReceiverPerm::Const}},
        nullptr, nullptr));

    cls.items.push_back(MakeClassMethod(
        "write",
        cursive0::syntax::ReceiverExplicit{std::nullopt, MakeTypePathType("Self")},
        nullptr,
        std::make_shared<Block>(Block{})));

    const auto class_res = ResolveItem(env.res_ctx, ASTItem{cls});
    assert(class_res.ok);

    ClassDecl empty;
    empty.vis = Visibility::Public;
    empty.name = "Empty";
    empty.supers = {};
    empty.items = {};
    empty.span = EmptySpan();
    empty.doc = {};
    const auto empty_res = ResolveItem(env.res_ctx, ASTItem{empty});
    assert(empty_res.ok);
  }

  {
    SPEC_COV("ResolveItem-Enum");
    SPEC_COV("ResolveVariant");
    SPEC_COV("ResolveVariantList-Cons");
    SPEC_COV("ResolveVariantList-Empty");
    SPEC_COV("ResolveVariantPayloadOpt-None");
    SPEC_COV("ResolveVariantPayloadOpt-Tuple");
    SPEC_COV("ResolveVariantPayloadOpt-Record");
    SPEC_COV("ResolveTypeList-Empty");
    SPEC_COV("ResolveTypeList-Cons");
    SPEC_COV("ResolveFieldDeclList-Empty");
    SPEC_COV("ResolveFieldDeclList-Cons");
    SPEC_COV("ResolveFieldDecl");

    TestEnv env;
    env.Init();
    AddType(env.ctx, "Local");

    EnumDecl empty_enum = MakeEnumDecl("Empty");
    const auto empty_res = ResolveItem(env.res_ctx, ASTItem{empty_enum});
    assert(empty_res.ok);

    EnumDecl e = MakeEnumDecl("E");
    VariantDecl none;
    none.name = "None";
    none.payload_opt = std::nullopt;
    none.discriminant_opt = std::nullopt;
    none.span = EmptySpan();
    none.doc_opt = std::nullopt;
    e.variants.push_back(none);

    VariantDecl tuple_empty;
    tuple_empty.name = "TupleEmpty";
    tuple_empty.payload_opt = VariantPayload{VariantPayloadTuple{}};
    tuple_empty.discriminant_opt = std::nullopt;
    tuple_empty.span = EmptySpan();
    tuple_empty.doc_opt = std::nullopt;
    e.variants.push_back(tuple_empty);

    VariantDecl tuple_one;
    tuple_one.name = "TupleOne";
    VariantPayloadTuple tup_payload;
    tup_payload.elements = {MakeTypePathType("Local")};
    tuple_one.payload_opt = VariantPayload{tup_payload};
    tuple_one.discriminant_opt = std::nullopt;
    tuple_one.span = EmptySpan();
    tuple_one.doc_opt = std::nullopt;
    e.variants.push_back(tuple_one);

    VariantDecl record_empty;
    record_empty.name = "RecordEmpty";
    record_empty.payload_opt = VariantPayload{VariantPayloadRecord{}};
    record_empty.discriminant_opt = std::nullopt;
    record_empty.span = EmptySpan();
    record_empty.doc_opt = std::nullopt;
    e.variants.push_back(record_empty);

    VariantDecl record_one;
    record_one.name = "RecordOne";
    VariantPayloadRecord rec_payload;
    rec_payload.fields = {MakeField("f", MakeTypePathType("Local"))};
    record_one.payload_opt = VariantPayload{rec_payload};
    record_one.discriminant_opt = std::nullopt;
    record_one.span = EmptySpan();
    record_one.doc_opt = std::nullopt;
    e.variants.push_back(record_one);

    const auto enum_res = ResolveItem(env.res_ctx, ASTItem{e});
    assert(enum_res.ok);
  }

  {
    SPEC_COV("ResolveItem-Modal");
    SPEC_COV("ResolveStateBlockList-Empty");
    SPEC_COV("ResolveStateBlockList-Cons");
    SPEC_COV("ResolveStateBlock");
    SPEC_COV("ResolveStateMemberList-Empty");
    SPEC_COV("ResolveStateMemberList-Cons");
    SPEC_COV("ResolveStateMember-Field");
    SPEC_COV("ResolveStateMember-Method");
    SPEC_COV("ResolveStateMember-Transition");

    TestEnv env;
    env.Init();
    AddType(env.ctx, "Local");

    ModalDecl empty_modal;
    empty_modal.vis = Visibility::Public;
    empty_modal.name = "EmptyModal";
    empty_modal.implements = {};
    empty_modal.states = {};
    empty_modal.span = EmptySpan();
    empty_modal.doc = {};
    const auto empty_res = ResolveItem(env.res_ctx, ASTItem{empty_modal});
    assert(empty_res.ok);

    ModalDecl modal;
    modal.vis = Visibility::Public;
    modal.name = "Modal";
    modal.implements = {};
    modal.states = {};
    modal.span = EmptySpan();
    modal.doc = {};

    StateBlock empty_state;
    empty_state.name = "Empty";
    empty_state.members = {};
    empty_state.span = EmptySpan();
    empty_state.doc_opt = std::nullopt;
    modal.states.push_back(empty_state);

    StateBlock state;
    state.name = "Active";
    state.members = {};
    state.span = EmptySpan();
    state.doc_opt = std::nullopt;

    state.members.push_back(MakeStateField("value", MakeTypePathType("Local")));
    state.members.push_back(MakeStateMethod("tick"));
    state.members.push_back(MakeTransition("to_empty", "Empty"));
    modal.states.push_back(state);

    const auto modal_res = ResolveItem(env.res_ctx, ASTItem{modal});
    assert(modal_res.ok);
  }

  {
    SPEC_COV("Res-Start");
    SPEC_COV("Res-Names");
    SPEC_COV("Res-Items");
    SPEC_COV("ResolveItems-Empty");
    SPEC_COV("ResolveItems-Cons");
    SPEC_COV("ResolveModule-Ok");
    SPEC_COV("Validate-ModulePath-Ok");

    TestEnv env;
    env.Init();

    ASTModule empty_mod;
    empty_mod.path = Path{"main"};
    empty_mod.items = {};
    empty_mod.module_doc = {};
    empty_mod.unsafe_spans = {};
    const auto empty_res = ResolveModule(env.res_ctx, empty_mod);
    assert(empty_res.ok);

    ASTModule one_mod;
    one_mod.path = Path{"main"};
    one_mod.items = {ASTItem{MakeProcedure("p", {}, nullptr)}};
    one_mod.module_doc = {};
    one_mod.unsafe_spans = {};
    const auto one_res = ResolveModule(env.res_ctx, one_mod);
    assert(one_res.ok);
  }

  {
    SPEC_COV("Validate-ModulePath-Reserved-Err");

    TestEnv env;
    env.Init();
    ASTModule bad;
    bad.path = Path{"cursive"};
    bad.items = {};
    bad.module_doc = {};
    bad.unsafe_spans = {};
    const auto bad_res = ResolveModule(env.res_ctx, bad);
    assert(!bad_res.ok);
  }

  {
    SPEC_COV("ResolveModules-Err-Resolve");
    TestEnv env;
    env.Init();
    ASTModule bad;
    bad.path = Path{"cursive"};
    bad.items = {};
    bad.module_doc = {};
    bad.unsafe_spans = {};
    env.ctx.sigma.mods = {bad};
    env.module_names.push_back(StringOfPath(bad.path));
    const auto result = ResolveModules(env.res_ctx);
    assert(!result.ok);
  }

  {
    SPEC_COV("ResolveModules-Ok");
    TestEnv env;
    env.Init();
    ASTModule good;
    good.path = Path{"main"};
    good.items = {};
    good.module_doc = {};
    good.unsafe_spans = {};
    env.ctx.sigma.mods = {good};
    env.module_names.push_back(StringOfPath(good.path));
    const auto result = ResolveModules(env.res_ctx);
    assert(result.ok);
  }

  return 0;
}
