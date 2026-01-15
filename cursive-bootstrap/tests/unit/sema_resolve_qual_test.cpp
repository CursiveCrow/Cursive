#include <cassert>
#include <string>
#include <vector>

#include "cursive0/core/assert_spec.h"
#include "cursive0/core/symbols.h"
#include "cursive0/sema/resolve_qual.h"
#include "cursive0/sema/scopes.h"

namespace {

using cursive0::core::StringOfPath;
using cursive0::sema::Entity;
using cursive0::sema::EntityKind;
using cursive0::sema::EntitySource;
using cursive0::sema::IdKeyOf;
using cursive0::sema::ModuleNames;
using cursive0::sema::NameMap;
using cursive0::sema::NameMapTable;
using cursive0::sema::PathEq;
using cursive0::sema::PathKeyOf;
using cursive0::sema::ResolveArgs;
using cursive0::sema::ResolveArgsResult;
using cursive0::sema::ResolveEnumRecord;
using cursive0::sema::ResolveEnumTuple;
using cursive0::sema::ResolveEnumUnit;
using cursive0::sema::ResolveExprResult;
using cursive0::sema::ResolveFieldInits;
using cursive0::sema::ResolveQualContext;
using cursive0::sema::ResolveQualifiedForm;
using cursive0::sema::ResolveQualifiedFormResult;
using cursive0::sema::ResolveRecordPath;
using cursive0::sema::ResolveTypePathResult;
using cursive0::sema::ScopeContext;
using cursive0::syntax::Arg;
using cursive0::syntax::EnumDecl;
using cursive0::syntax::EnumLiteralExpr;
using cursive0::syntax::EnumPayloadBrace;
using cursive0::syntax::EnumPayloadParen;
using cursive0::syntax::Expr;
using cursive0::syntax::ExprPtr;
using cursive0::syntax::FieldInit;
using cursive0::syntax::IdentifierExpr;
using cursive0::syntax::ModulePath;
using cursive0::syntax::PathExpr;
using cursive0::syntax::QualifiedApplyExpr;
using cursive0::syntax::QualifiedNameExpr;
using cursive0::syntax::RecordDecl;
using cursive0::syntax::RecordExpr;
using cursive0::syntax::TypePath;
using cursive0::syntax::VariantDecl;
using cursive0::syntax::VariantPayloadRecord;
using cursive0::syntax::VariantPayloadTuple;
using cursive0::syntax::Visibility;

const std::vector<TypePath>* g_allowed_type_paths = nullptr;

cursive0::core::Span EmptySpan() {
  return {};
}

ExprPtr MakeExpr(const cursive0::syntax::ExprNode& node) {
  auto expr = std::make_shared<Expr>();
  expr->span = EmptySpan();
  expr->node = node;
  return expr;
}

ExprPtr MakeIdentExpr(const std::string& name) {
  IdentifierExpr ident;
  ident.name = name;
  return MakeExpr(ident);
}

ExprPtr MakeQualifiedNameExpr(const ModulePath& path, const std::string& name) {
  QualifiedNameExpr qname;
  qname.path = path;
  qname.name = name;
  return MakeExpr(qname);
}

ExprPtr MakeQualifiedApplyParenExpr(const ModulePath& path,
                                    const std::string& name,
                                    const std::vector<Arg>& args) {
  QualifiedApplyExpr app;
  app.path = path;
  app.name = name;
  app.args = cursive0::syntax::ParenArgs{args};
  return MakeExpr(app);
}

ExprPtr MakeQualifiedApplyBraceExpr(const ModulePath& path,
                                    const std::string& name,
                                    const std::vector<FieldInit>& fields) {
  QualifiedApplyExpr app;
  app.path = path;
  app.name = name;
  app.args = cursive0::syntax::BraceArgs{fields};
  return MakeExpr(app);
}

ResolveExprResult ResolveExprOk(const ScopeContext&,
                                const NameMapTable&,
                                const ModuleNames&,
                                const ExprPtr& expr) {
  return {true, std::nullopt, expr};
}

ResolveTypePathResult ResolveTypePathFromList(const ScopeContext&,
                                              const NameMapTable&,
                                              const ModuleNames&,
                                              const TypePath& path) {
  if (!g_allowed_type_paths) {
    return {false, std::nullopt, {}};
  }
  for (const auto& allowed : *g_allowed_type_paths) {
    if (PathEq(allowed, path)) {
      return {true, std::nullopt, allowed};
    }
  }
  return {false, std::nullopt, {}};
}

ResolveQualContext MakeQualContext(ScopeContext& ctx,
                                   NameMapTable& names,
                                   ModuleNames& module_names) {
  ResolveQualContext out;
  out.ctx = &ctx;
  out.name_maps = &names;
  out.module_names = &module_names;
  out.resolve_expr = ResolveExprOk;
  out.resolve_type_path = ResolveTypePathFromList;
  out.can_access = nullptr;
  return out;
}

ModuleNames MakeModuleNames(const std::vector<ModulePath>& paths) {
  ModuleNames names;
  names.reserve(paths.size());
  for (const auto& path : paths) {
    names.push_back(StringOfPath(path));
  }
  return names;
}

NameMapTable MakeNameMapForValue(const ModulePath& module_path,
                                 const std::string& name) {
  NameMapTable table;
  NameMap map;
  map.emplace(IdKeyOf(name),
              Entity{EntityKind::Value, module_path, std::nullopt,
                     EntitySource::Decl});
  table.emplace(PathKeyOf(module_path), std::move(map));
  return table;
}

RecordDecl MakeRecord(const std::string& name) {
  RecordDecl decl;
  decl.vis = Visibility::Public;
  decl.name = name;
  decl.implements = {};
  decl.members = {};
  decl.span = EmptySpan();
  decl.doc = {};
  return decl;
}

VariantDecl MakeVariantUnit(const std::string& name) {
  VariantDecl decl;
  decl.name = name;
  decl.payload_opt = std::nullopt;
  decl.discriminant_opt = std::nullopt;
  decl.span = EmptySpan();
  decl.doc_opt = std::nullopt;
  return decl;
}

VariantDecl MakeVariantTuple(const std::string& name) {
  VariantDecl decl;
  decl.name = name;
  decl.payload_opt = VariantPayloadTuple{};
  decl.discriminant_opt = std::nullopt;
  decl.span = EmptySpan();
  decl.doc_opt = std::nullopt;
  return decl;
}

VariantDecl MakeVariantRecord(const std::string& name) {
  VariantDecl decl;
  decl.name = name;
  decl.payload_opt = VariantPayloadRecord{};
  decl.discriminant_opt = std::nullopt;
  decl.span = EmptySpan();
  decl.doc_opt = std::nullopt;
  return decl;
}

EnumDecl MakeEnum(const std::string& name,
                  const std::vector<VariantDecl>& variants) {
  EnumDecl decl;
  decl.vis = Visibility::Public;
  decl.name = name;
  decl.implements = {};
  decl.variants = variants;
  decl.span = EmptySpan();
  decl.doc = {};
  return decl;
}

void AddRecordType(ScopeContext& ctx, const TypePath& path) {
  ctx.sigma.types.emplace(PathKeyOf(path), MakeRecord(path.back()));
}

void AddEnumType(ScopeContext& ctx,
                 const TypePath& path,
                 const std::vector<VariantDecl>& variants) {
  ctx.sigma.types.emplace(PathKeyOf(path), MakeEnum(path.back(), variants));
}

void TestResolveArgs() {
  ScopeContext ctx;
  NameMapTable names;
  ModuleNames modules;
  ResolveQualContext qctx = MakeQualContext(ctx, names, modules);

  SPEC_COV("ResolveArgs-Empty");
  const auto empty = ResolveArgs(qctx, {});
  assert(empty.ok);
  assert(empty.args.empty());

  SPEC_COV("ResolveArgs-Cons");
  Arg first;
  first.value = MakeIdentExpr("x");
  first.span = EmptySpan();
  Arg second;
  second.value = MakeIdentExpr("y");
  second.span = EmptySpan();
  const auto cons = ResolveArgs(qctx, {first, second});
  assert(cons.ok);
  assert(cons.args.size() == 2);
}

void TestResolveFieldInits() {
  ScopeContext ctx;
  NameMapTable names;
  ModuleNames modules;
  ResolveQualContext qctx = MakeQualContext(ctx, names, modules);

  SPEC_COV("ResolveFieldInits-Empty");
  const auto empty = ResolveFieldInits(qctx, {});
  assert(empty.ok);
  assert(empty.fields.empty());

  SPEC_COV("ResolveFieldInits-Cons");
  FieldInit first;
  first.name = "a";
  first.value = MakeIdentExpr("x");
  first.span = EmptySpan();
  FieldInit second;
  second.name = "b";
  second.value = MakeIdentExpr("y");
  second.span = EmptySpan();
  const auto cons = ResolveFieldInits(qctx, {first, second});
  assert(cons.ok);
  assert(cons.fields.size() == 2);
}

void TestResolveRecordAndEnumPaths() {
  ScopeContext ctx;
  NameMapTable names;
  ModuleNames modules;
  ResolveQualContext qctx = MakeQualContext(ctx, names, modules);

  TypePath record_path = {"pkg", "Rec"};
  TypePath enum_path = {"pkg", "E"};
  AddRecordType(ctx, record_path);
  AddEnumType(ctx, enum_path,
              {MakeVariantUnit("UnitVar"), MakeVariantTuple("TupleVar"),
               MakeVariantRecord("RecordVar")});

  std::vector<TypePath> ok_paths = {record_path, enum_path};
  g_allowed_type_paths = &ok_paths;

  SPEC_COV("Resolve-RecordPath");
  const auto record = ResolveRecordPath(qctx, {"pkg"}, "Rec");
  assert(record.ok);
  assert(PathEq(record.path, record_path));

  SPEC_COV("Resolve-EnumUnit");
  const auto unit = ResolveEnumUnit(qctx, {"pkg", "E"}, "UnitVar");
  assert(unit.ok);

  SPEC_COV("Resolve-EnumTuple");
  const auto tuple = ResolveEnumTuple(qctx, {"pkg", "E"}, "TupleVar");
  assert(tuple.ok);

  SPEC_COV("Resolve-EnumRecord");
  const auto record_enum = ResolveEnumRecord(qctx, {"pkg", "E"}, "RecordVar");
  assert(record_enum.ok);

  g_allowed_type_paths = nullptr;
}

void TestResolveQualifiedNameValue() {
  ScopeContext ctx;
  ctx.current_module = {"main"};
  NameMapTable names = MakeNameMapForValue({"pkg"}, "value");
  ModuleNames modules = MakeModuleNames({{"pkg"}});
  ResolveQualContext qctx = MakeQualContext(ctx, names, modules);

  SPEC_COV("ResolveQual-Name-Value");
  auto expr = MakeQualifiedNameExpr({"pkg"}, "value");
  const auto resolved = ResolveQualifiedForm(qctx, *expr);
  assert(resolved.ok);
  const auto* path = std::get_if<PathExpr>(&resolved.expr->node);
  assert(path);
  assert(PathEq(path->path, ModulePath{"pkg"}));
  assert(path->name == "value");
}

void TestResolveQualifiedNameRecord() {
  ScopeContext ctx;
  ctx.current_module = {"main"};
  NameMapTable names;
  ModuleNames modules = MakeModuleNames({{"pkg"}});
  ResolveQualContext qctx = MakeQualContext(ctx, names, modules);

  TypePath record_path = {"pkg", "Rec"};
  AddRecordType(ctx, record_path);
  std::vector<TypePath> ok_paths = {record_path};
  g_allowed_type_paths = &ok_paths;

  SPEC_COV("ResolveQual-Name-Record");
  auto expr = MakeQualifiedNameExpr({"pkg"}, "Rec");
  const auto resolved = ResolveQualifiedForm(qctx, *expr);
  assert(resolved.ok);
  const auto* path = std::get_if<PathExpr>(&resolved.expr->node);
  assert(path);
  assert(PathEq(path->path, ModulePath{"pkg"}));
  assert(path->name == "Rec");

  g_allowed_type_paths = nullptr;
}

void TestResolveQualifiedNameEnum() {
  ScopeContext ctx;
  ctx.current_module = {"main"};
  NameMapTable names;
  ModuleNames modules = MakeModuleNames({{"pkg"}});
  ResolveQualContext qctx = MakeQualContext(ctx, names, modules);

  TypePath enum_path = {"pkg", "E"};
  AddEnumType(ctx, enum_path, {MakeVariantUnit("UnitVar")});
  std::vector<TypePath> ok_paths = {enum_path};
  g_allowed_type_paths = &ok_paths;

  SPEC_COV("ResolveQual-Name-Enum");
  auto expr = MakeQualifiedNameExpr({"pkg", "E"}, "UnitVar");
  const auto resolved = ResolveQualifiedForm(qctx, *expr);
  assert(resolved.ok);
  const auto* literal = std::get_if<EnumLiteralExpr>(&resolved.expr->node);
  assert(literal);
  assert(PathEq(literal->path, TypePath{"pkg", "E", "UnitVar"}));
  assert(!literal->payload_opt.has_value());

  g_allowed_type_paths = nullptr;
}

void TestResolveQualifiedNameErr() {
  ScopeContext ctx;
  ctx.current_module = {"main"};
  NameMapTable names;
  ModuleNames modules = MakeModuleNames({{"pkg"}});
  ResolveQualContext qctx = MakeQualContext(ctx, names, modules);

  SPEC_COV("ResolveQual-Name-Err");
  auto expr = MakeQualifiedNameExpr({"pkg"}, "Missing");
  const auto resolved = ResolveQualifiedForm(qctx, *expr);
  assert(!resolved.ok);
  assert(resolved.diag_id == "ResolveExpr-Ident-Err");
}

void TestResolveQualifiedApplyValue() {
  ScopeContext ctx;
  ctx.current_module = {"main"};
  NameMapTable names = MakeNameMapForValue({"pkg"}, "value");
  ModuleNames modules = MakeModuleNames({{"pkg"}});
  ResolveQualContext qctx = MakeQualContext(ctx, names, modules);

  SPEC_COV("ResolveQual-Apply-Value");
  Arg arg;
  arg.value = MakeIdentExpr("x");
  arg.span = EmptySpan();
  auto expr = MakeQualifiedApplyParenExpr({"pkg"}, "value", {arg});
  const auto resolved = ResolveQualifiedForm(qctx, *expr);
  assert(resolved.ok);
  const auto* call = std::get_if<cursive0::syntax::CallExpr>(&resolved.expr->node);
  assert(call);
  const auto* callee = std::get_if<PathExpr>(&call->callee->node);
  assert(callee);
  assert(PathEq(callee->path, ModulePath{"pkg"}));
  assert(callee->name == "value");
}

void TestResolveQualifiedApplyRecord() {
  ScopeContext ctx;
  ctx.current_module = {"main"};
  NameMapTable names;
  ModuleNames modules = MakeModuleNames({{"pkg"}});
  ResolveQualContext qctx = MakeQualContext(ctx, names, modules);

  TypePath record_path = {"pkg", "Rec"};
  AddRecordType(ctx, record_path);
  std::vector<TypePath> ok_paths = {record_path};
  g_allowed_type_paths = &ok_paths;

  SPEC_COV("ResolveQual-Apply-Record");
  Arg arg;
  arg.value = MakeIdentExpr("x");
  arg.span = EmptySpan();
  auto expr = MakeQualifiedApplyParenExpr({"pkg"}, "Rec", {arg});
  const auto resolved = ResolveQualifiedForm(qctx, *expr);
  assert(resolved.ok);
  const auto* call = std::get_if<cursive0::syntax::CallExpr>(&resolved.expr->node);
  assert(call);
  const auto* callee = std::get_if<PathExpr>(&call->callee->node);
  assert(callee);
  assert(PathEq(callee->path, ModulePath{"pkg"}));
  assert(callee->name == "Rec");

  g_allowed_type_paths = nullptr;
}

void TestResolveQualifiedApplyEnumTuple() {
  ScopeContext ctx;
  ctx.current_module = {"main"};
  NameMapTable names;
  ModuleNames modules = MakeModuleNames({{"pkg"}});
  ResolveQualContext qctx = MakeQualContext(ctx, names, modules);

  TypePath enum_path = {"pkg", "E"};
  AddEnumType(ctx, enum_path, {MakeVariantTuple("TupleVar")});
  std::vector<TypePath> ok_paths = {enum_path};
  g_allowed_type_paths = &ok_paths;

  SPEC_COV("ResolveQual-Apply-Enum-Tuple");
  Arg arg;
  arg.value = MakeIdentExpr("x");
  arg.span = EmptySpan();
  auto expr = MakeQualifiedApplyParenExpr({"pkg", "E"}, "TupleVar", {arg});
  const auto resolved = ResolveQualifiedForm(qctx, *expr);
  assert(resolved.ok);
  const auto* literal = std::get_if<EnumLiteralExpr>(&resolved.expr->node);
  assert(literal);
  assert(literal->payload_opt.has_value());
  const auto* payload =
      std::get_if<EnumPayloadParen>(&*literal->payload_opt);
  assert(payload);
  assert(payload->elements.size() == 1);

  g_allowed_type_paths = nullptr;
}

void TestResolveQualifiedApplyErr() {
  ScopeContext ctx;
  ctx.current_module = {"main"};
  NameMapTable names;
  ModuleNames modules = MakeModuleNames({{"pkg"}});
  ResolveQualContext qctx = MakeQualContext(ctx, names, modules);

  SPEC_COV("ResolveQual-Apply-Err");
  Arg arg;
  arg.value = MakeIdentExpr("x");
  arg.span = EmptySpan();
  auto expr = MakeQualifiedApplyParenExpr({"pkg"}, "Missing", {arg});
  const auto resolved = ResolveQualifiedForm(qctx, *expr);
  assert(!resolved.ok);
  assert(resolved.diag_id == "ResolveExpr-Ident-Err");
}

void TestResolveQualifiedApplyRecordLit() {
  ScopeContext ctx;
  ctx.current_module = {"main"};
  NameMapTable names;
  ModuleNames modules = MakeModuleNames({{"pkg"}});
  ResolveQualContext qctx = MakeQualContext(ctx, names, modules);

  TypePath record_path = {"pkg", "Rec"};
  AddRecordType(ctx, record_path);
  std::vector<TypePath> ok_paths = {record_path};
  g_allowed_type_paths = &ok_paths;

  SPEC_COV("ResolveQual-Apply-RecordLit");
  FieldInit field;
  field.name = "a";
  field.value = MakeIdentExpr("x");
  field.span = EmptySpan();
  auto expr = MakeQualifiedApplyBraceExpr({"pkg"}, "Rec", {field});
  const auto resolved = ResolveQualifiedForm(qctx, *expr);
  assert(resolved.ok);
  const auto* rec = std::get_if<RecordExpr>(&resolved.expr->node);
  assert(rec);
  const auto* target = std::get_if<TypePath>(&rec->target);
  assert(target);
  assert(PathEq(*target, record_path));
  assert(rec->fields.size() == 1);

  g_allowed_type_paths = nullptr;
}

void TestResolveQualifiedApplyEnumRecord() {
  ScopeContext ctx;
  ctx.current_module = {"main"};
  NameMapTable names;
  ModuleNames modules = MakeModuleNames({{"pkg"}});
  ResolveQualContext qctx = MakeQualContext(ctx, names, modules);

  TypePath enum_path = {"pkg", "E"};
  AddEnumType(ctx, enum_path, {MakeVariantRecord("RecordVar")});
  std::vector<TypePath> ok_paths = {enum_path};
  g_allowed_type_paths = &ok_paths;

  SPEC_COV("ResolveQual-Apply-Enum-Record");
  FieldInit field;
  field.name = "a";
  field.value = MakeIdentExpr("x");
  field.span = EmptySpan();
  auto expr = MakeQualifiedApplyBraceExpr({"pkg", "E"}, "RecordVar", {field});
  const auto resolved = ResolveQualifiedForm(qctx, *expr);
  assert(resolved.ok);
  const auto* literal = std::get_if<EnumLiteralExpr>(&resolved.expr->node);
  assert(literal);
  assert(literal->payload_opt.has_value());
  const auto* payload =
      std::get_if<EnumPayloadBrace>(&*literal->payload_opt);
  assert(payload);
  assert(payload->fields.size() == 1);

  g_allowed_type_paths = nullptr;
}

void TestResolveQualifiedApplyBraceErr() {
  ScopeContext ctx;
  ctx.current_module = {"main"};
  NameMapTable names;
  ModuleNames modules = MakeModuleNames({{"pkg"}});
  ResolveQualContext qctx = MakeQualContext(ctx, names, modules);

  SPEC_COV("ResolveQual-Apply-Brace-Err");
  FieldInit field;
  field.name = "a";
  field.value = MakeIdentExpr("x");
  field.span = EmptySpan();
  auto expr = MakeQualifiedApplyBraceExpr({"pkg"}, "Missing", {field});
  const auto resolved = ResolveQualifiedForm(qctx, *expr);
  assert(!resolved.ok);
  assert(resolved.diag_id == "ResolveExpr-Ident-Err");
}

}  // namespace

int main() {
  TestResolveArgs();
  TestResolveFieldInits();
  TestResolveRecordAndEnumPaths();
  TestResolveQualifiedNameValue();
  TestResolveQualifiedNameRecord();
  TestResolveQualifiedNameEnum();
  TestResolveQualifiedNameErr();
  TestResolveQualifiedApplyValue();
  TestResolveQualifiedApplyRecord();
  TestResolveQualifiedApplyEnumTuple();
  TestResolveQualifiedApplyErr();
  TestResolveQualifiedApplyRecordLit();
  TestResolveQualifiedApplyEnumRecord();
  TestResolveQualifiedApplyBraceErr();
  return 0;
}
