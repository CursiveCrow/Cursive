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
using cursive0::sema::ResPatternResult;
using cursive0::sema::ResolveContext;
using cursive0::sema::ResolveExpr;
using cursive0::sema::ResolvePattern;
using cursive0::sema::ResolveStmtSeq;
using cursive0::sema::Scope;
using cursive0::sema::ScopeContext;
using cursive0::syntax::Arg;
using cursive0::syntax::BinaryExpr;
using cursive0::syntax::Block;
using cursive0::syntax::BlockExpr;
using cursive0::syntax::CallExpr;
using cursive0::syntax::EnumLiteralExpr;
using cursive0::syntax::EnumPayload;
using cursive0::syntax::EnumPayloadBrace;
using cursive0::syntax::EnumPayloadParen;
using cursive0::syntax::EnumPayloadPattern;
using cursive0::syntax::EnumPattern;
using cursive0::syntax::Expr;
using cursive0::syntax::ExprNode;
using cursive0::syntax::ExprPtr;
using cursive0::syntax::FieldInit;
using cursive0::syntax::FieldPattern;
using cursive0::syntax::Identifier;
using cursive0::syntax::IdentifierExpr;
using cursive0::syntax::LiteralExpr;
using cursive0::syntax::LiteralPattern;
using cursive0::syntax::LetStmt;
using cursive0::syntax::ShadowLetStmt;
using cursive0::syntax::DeferStmt;
using cursive0::syntax::RegionStmt;
using cursive0::syntax::LoopIterExpr;
using cursive0::syntax::MatchArm;
using cursive0::syntax::MatchExpr;
using cursive0::syntax::ModalPattern;
using cursive0::syntax::ModalRecordPayload;
using cursive0::syntax::ModulePath;
using cursive0::syntax::Path;
using cursive0::syntax::PathExpr;
using cursive0::syntax::Pattern;
using cursive0::syntax::PatternNode;
using cursive0::syntax::PatternPtr;
using cursive0::syntax::QualifiedNameExpr;
using cursive0::syntax::RangeKind;
using cursive0::syntax::RangePattern;
using cursive0::syntax::RecordPattern;
using cursive0::syntax::RecordPayloadPattern;
using cursive0::syntax::RecordExpr;
using cursive0::syntax::TuplePattern;
using cursive0::syntax::TuplePayloadPattern;
using cursive0::syntax::Type;
using cursive0::syntax::TypeModalState;
using cursive0::syntax::TypePath;
using cursive0::syntax::TypePathType;
using cursive0::syntax::VarStmt;
using cursive0::syntax::Stmt;
using cursive0::syntax::ShadowVarStmt;
using cursive0::syntax::FrameStmt;
using cursive0::syntax::WildcardPattern;
using cursive0::syntax::TypedPattern;
using cursive0::syntax::Token;
using cursive0::syntax::TokenKind;

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

template <typename NodeT>
ExprPtr MakeExpr(NodeT node) {
  auto expr = std::make_shared<Expr>();
  expr->span = EmptySpan();
  expr->node = std::move(node);
  return expr;
}

ExprPtr MakeLiteralExpr(TokenKind kind, std::string_view lexeme) {
  return MakeExpr(LiteralExpr{MakeToken(kind, lexeme)});
}

ExprPtr MakeIdentExpr(std::string_view name) {
  IdentifierExpr ident;
  ident.name = std::string(name);
  return MakeExpr(ident);
}

ExprPtr MakePathExpr(const ModulePath& path, std::string_view name) {
  PathExpr expr;
  expr.path = path;
  expr.name = std::string(name);
  return MakeExpr(expr);
}

ExprPtr MakeQualifiedNameExpr(const ModulePath& path, std::string_view name) {
  QualifiedNameExpr expr;
  expr.path = path;
  expr.name = std::string(name);
  return MakeExpr(expr);
}

Arg MakeArg(const ExprPtr& value) {
  Arg arg;
  arg.value = value;
  arg.span = EmptySpan();
  return arg;
}

FieldInit MakeFieldInit(std::string_view name, const ExprPtr& value) {
  FieldInit field;
  field.name = std::string(name);
  field.value = value;
  field.span = EmptySpan();
  return field;
}

ExprPtr MakeCallExpr(const ExprPtr& callee, std::vector<Arg> args) {
  CallExpr call;
  call.callee = callee;
  call.args = std::move(args);
  return MakeExpr(call);
}

template <typename NodeT>
PatternPtr MakePattern(NodeT node) {
  auto pat = std::make_shared<Pattern>();
  pat->span = EmptySpan();
  pat->node = std::move(node);
  return pat;
}

PatternPtr MakeWildcardPattern() {
  return MakePattern(WildcardPattern{});
}

PatternPtr MakeIdentPattern(std::string_view name) {
  return MakePattern(cursive0::syntax::IdentifierPattern{std::string(name)});
}

PatternPtr MakeLiteralPattern(TokenKind kind, std::string_view lexeme) {
  return MakePattern(LiteralPattern{MakeToken(kind, lexeme)});
}

PatternPtr MakeTypedPattern(std::string_view name, const std::shared_ptr<Type>& type) {
  TypedPattern pat;
  pat.name = std::string(name);
  pat.type = type;
  return MakePattern(std::move(pat));
}

PatternPtr MakeTuplePattern(std::vector<PatternPtr> elements) {
  TuplePattern pat;
  pat.elements = std::move(elements);
  return MakePattern(std::move(pat));
}

FieldPattern MakeFieldPattern(std::string_view name, const PatternPtr& pat_opt) {
  FieldPattern field;
  field.name = std::string(name);
  field.pattern_opt = pat_opt;
  field.span = EmptySpan();
  return field;
}

PatternPtr MakeRecordPattern(const TypePath& path, std::vector<FieldPattern> fields) {
  RecordPattern pat;
  pat.path = path;
  pat.fields = std::move(fields);
  return MakePattern(std::move(pat));
}

PatternPtr MakeEnumPattern(const TypePath& path,
                           std::string_view name,
                           std::optional<EnumPayloadPattern> payload_opt) {
  EnumPattern pat;
  pat.path = path;
  pat.name = std::string(name);
  pat.payload_opt = std::move(payload_opt);
  return MakePattern(std::move(pat));
}

PatternPtr MakeModalPattern(std::string_view state,
                            std::optional<ModalRecordPayload> fields_opt) {
  ModalPattern pat;
  pat.state = std::string(state);
  pat.fields_opt = std::move(fields_opt);
  return MakePattern(std::move(pat));
}

PatternPtr MakeRangePattern(RangeKind kind, const PatternPtr& lo, const PatternPtr& hi) {
  RangePattern pat;
  pat.kind = kind;
  pat.lo = lo;
  pat.hi = hi;
  return MakePattern(std::move(pat));
}

std::shared_ptr<Type> MakeTypePathType(std::string_view name) {
  auto type = std::make_shared<Type>();
  TypePath path;
  path.emplace_back(std::string(name));
  type->span = EmptySpan();
  type->node = TypePathType{std::move(path)};
  return type;
}

std::shared_ptr<Type> MakeTypeModalState(std::string_view modal,
                                         std::string_view state) {
  auto type = std::make_shared<Type>();
  TypeModalState node;
  node.path = TypePath{std::string(modal)};
  node.state = std::string(state);
  type->span = EmptySpan();
  type->node = std::move(node);
  return type;
}

void AddValue(ScopeContext& ctx, std::string_view name) {
  ctx.scopes.front().emplace(
      IdKeyOf(name),
      Entity{EntityKind::Value, std::nullopt, std::nullopt, EntitySource::Decl});
}

void AddOuterValue(ScopeContext& ctx, std::string_view name) {
  if (ctx.scopes.size() >= 2) {
    ctx.scopes[1].emplace(
        IdKeyOf(name),
        Entity{EntityKind::Value, std::nullopt, std::nullopt,
               EntitySource::Decl});
  } else {
    AddValue(ctx, name);
  }
}

void AddRegionAlias(ScopeContext& ctx, std::string_view name) {
  ctx.scopes.front().emplace(
      IdKeyOf(name),
      Entity{EntityKind::Value, std::nullopt, std::nullopt,
             EntitySource::RegionAlias});
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

cursive0::syntax::RecordDecl MakeRecordDecl(std::string_view name) {
  cursive0::syntax::RecordDecl decl;
  decl.vis = cursive0::syntax::Visibility::Public;
  decl.name = std::string(name);
  decl.implements = {};
  decl.members = {};
  decl.span = EmptySpan();
  decl.doc = {};
  return decl;
}

cursive0::syntax::EnumDecl MakeEnumDecl(std::string_view name) {
  cursive0::syntax::EnumDecl decl;
  decl.vis = cursive0::syntax::Visibility::Public;
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

}  // namespace

int main() {
  {
    SPEC_COV("ResolvePat-Wild");
    SPEC_COV("ResolvePat-Ident");
    SPEC_COV("ResolvePat-Literal");
    SPEC_COV("ResolvePat-Typed");
    SPEC_COV("ResolvePat-Tuple");
    SPEC_COV("ResolvePatternList-Empty");
    SPEC_COV("ResolvePatternList-Cons");
    SPEC_COV("ResolvePat-Record");
    SPEC_COV("ResolveFieldPattern-Explicit");
    SPEC_COV("ResolveFieldPattern-Implicit");
    SPEC_COV("ResolveFieldPatternList-Cons");
    SPEC_COV("ResolveFieldPatternList-Empty");
    SPEC_COV("ResolvePat-Enum");
    SPEC_COV("ResolveEnumPayloadPattern-None");
    SPEC_COV("ResolveEnumPayloadPattern-Tuple");
    SPEC_COV("ResolveEnumPayloadPattern-Record");
    SPEC_COV("ResolvePat-Modal");
    SPEC_COV("ResolveFieldPatternListOpt-None");
    SPEC_COV("ResolveFieldPatternListOpt-Some");
    SPEC_COV("ResolvePat-Range");

    TestEnv env;
    env.Init();
    AddType(env.ctx, "T");
    AddType(env.ctx, "Rec");
    AddType(env.ctx, "Choice");
    AddRecordType(env.ctx, TypePath{"Rec"});
    AddEnumType(env.ctx, TypePath{"Choice"});

    const ResPatternResult wild = ResolvePattern(env.res_ctx, MakeWildcardPattern());
    assert(wild.ok);

    const ResPatternResult ident =
        ResolvePattern(env.res_ctx, MakeIdentPattern("x"));
    assert(ident.ok);

    const ResPatternResult lit =
        ResolvePattern(env.res_ctx, MakeLiteralPattern(TokenKind::IntLiteral, "1"));
    assert(lit.ok);

    const ResPatternResult typed =
        ResolvePattern(env.res_ctx, MakeTypedPattern("t", MakeTypePathType("T")));
    assert(typed.ok);

    const ResPatternResult tuple_empty =
        ResolvePattern(env.res_ctx, MakeTuplePattern({}));
    assert(tuple_empty.ok);

    const ResPatternResult tuple_cons =
        ResolvePattern(env.res_ctx, MakeTuplePattern({MakeIdentPattern("a")}));
    assert(tuple_cons.ok);

    std::vector<FieldPattern> fields;
    fields.push_back(MakeFieldPattern(
        "x", MakeLiteralPattern(TokenKind::IntLiteral, "0")));
    fields.push_back(MakeFieldPattern("y", nullptr));
    const ResPatternResult rec =
        ResolvePattern(env.res_ctx, MakeRecordPattern(TypePath{"Rec"}, fields));
    assert(rec.ok);

    const ResPatternResult rec_empty =
        ResolvePattern(env.res_ctx, MakeRecordPattern(TypePath{"Rec"}, {}));
    assert(rec_empty.ok);

    const ResPatternResult enum_none =
        ResolvePattern(env.res_ctx, MakeEnumPattern(TypePath{"Choice"}, "Unit",
                                                   std::nullopt));
    assert(enum_none.ok);

    TuplePayloadPattern tuple_payload;
    tuple_payload.elements = {MakeIdentPattern("e")};
    const ResPatternResult enum_tuple =
        ResolvePattern(env.res_ctx, MakeEnumPattern(TypePath{"Choice"}, "Tuple",
                                                   EnumPayloadPattern{tuple_payload}));
    assert(enum_tuple.ok);

    RecordPayloadPattern record_payload;
    record_payload.fields = {MakeFieldPattern("f", MakeIdentPattern("f"))};
    const ResPatternResult enum_record =
        ResolvePattern(env.res_ctx, MakeEnumPattern(TypePath{"Choice"}, "Rec",
                                                   EnumPayloadPattern{record_payload}));
    assert(enum_record.ok);

    const ResPatternResult modal_none =
        ResolvePattern(env.res_ctx, MakeModalPattern("State", std::nullopt));
    assert(modal_none.ok);

    ModalRecordPayload modal_payload;
    modal_payload.fields = {MakeFieldPattern("m", MakeIdentPattern("m"))};
    const ResPatternResult modal_some =
        ResolvePattern(env.res_ctx, MakeModalPattern("State",
                                                    std::optional<ModalRecordPayload>(modal_payload)));
    assert(modal_some.ok);

    const ResPatternResult range =
        ResolvePattern(env.res_ctx,
                       MakeRangePattern(RangeKind::Inclusive,
                                        MakeLiteralPattern(TokenKind::IntLiteral, "0"),
                                        MakeIdentPattern("hi")));
    assert(range.ok);
  }

  {
    SPEC_COV("ResolvePat-Enum-Record-Fallback");
    TestEnv env;
    env.Init();
    AddType(env.ctx, "Outer");
    const ModulePath outer = {"Outer"};
    env.module_names.push_back(StringOfPath(outer));
    AddName(env.name_maps, outer, "Inner", EntityKind::Type, outer);

    RecordPayloadPattern payload;
    payload.fields = {MakeFieldPattern("f", nullptr)};
    const ResPatternResult fallback =
        ResolvePattern(env.res_ctx,
                       MakeEnumPattern(TypePath{"Outer"}, "Inner",
                                       EnumPayloadPattern{payload}));
    assert(fallback.ok);
  }

  {
    SPEC_COV("ResolveExpr-Match");
    SPEC_COV("ResolveArm");
    SPEC_COV("ResolveArms-Cons");
    SPEC_COV("ResolveExprOpt-None");
    SPEC_COV("ResolveExprOpt-Some");
    SPEC_COV("BindNames-Cons");
    SPEC_COV("BindNames-Empty");
    SPEC_COV("BindPattern");

    TestEnv env;
    env.Init();
    MatchArm arm1;
    arm1.pattern = MakeIdentPattern("x");
    arm1.guard_opt = nullptr;
    arm1.body = MakeLiteralExpr(TokenKind::IntLiteral, "1");
    arm1.span = EmptySpan();

    MatchArm arm2;
    arm2.pattern = MakeWildcardPattern();
    arm2.guard_opt = MakeLiteralExpr(TokenKind::IntLiteral, "0");
    arm2.body = MakeLiteralExpr(TokenKind::IntLiteral, "2");
    arm2.span = EmptySpan();

    MatchExpr match;
    match.value = MakeLiteralExpr(TokenKind::IntLiteral, "0");
    match.arms = {arm1, arm2};

    const auto resolved = ResolveExpr(env.res_ctx, MakeExpr(match));
    assert(resolved.ok);
  }

  {
    SPEC_COV("ResolveArms-Empty");
    SPEC_COV("ResolveExpr-Match");

    TestEnv env;
    env.Init();
    MatchExpr match;
    match.value = MakeLiteralExpr(TokenKind::IntLiteral, "0");
    match.arms = {};
    const auto resolved = ResolveExpr(env.res_ctx, MakeExpr(match));
    assert(resolved.ok);
  }

  {
    SPEC_COV("ResolveExpr-Ident");
    SPEC_COV("ResolveExpr-Qualified");
    SPEC_COV("ResolveExpr-Hom");

    TestEnv env;
    env.Init();
    AddValue(env.ctx, "x");
    const ModulePath pkg = {"pkg"};
    env.module_names.push_back(StringOfPath(pkg));
    AddName(env.name_maps, pkg, "qval", EntityKind::Value, pkg);

    const auto ident_res = ResolveExpr(env.res_ctx, MakeIdentExpr("x"));
    assert(ident_res.ok);

    const auto qual_res =
        ResolveExpr(env.res_ctx, MakeQualifiedNameExpr(pkg, "qval"));
    assert(qual_res.ok);

    BinaryExpr bin;
    bin.op = "+";
    bin.lhs = MakeLiteralExpr(TokenKind::IntLiteral, "1");
    bin.rhs = MakeLiteralExpr(TokenKind::IntLiteral, "2");
    const auto hom_res = ResolveExpr(env.res_ctx, MakeExpr(bin));
    assert(hom_res.ok);
  }

  {
    SPEC_COV("ResolveExpr-Call");
    SPEC_COV("ResolveExprList-Empty");
    SPEC_COV("ResolveExprList-Cons");
    SPEC_COV("ResolveCallee-Ident-Value");
    SPEC_COV("ResolveCallee-Ident-Record");
    SPEC_COV("ResolveCallee-Path-Value");
    SPEC_COV("ResolveCallee-Path-Record");
    SPEC_COV("ResolveCallee-Other");

    TestEnv env;
    env.Init();
    AddValue(env.ctx, "f");
    AddType(env.ctx, "Rec");
    AddRecordType(env.ctx, TypePath{"Rec"});
    AddRecordType(env.ctx, TypePath{"pkg", "Rec"});
    const ModulePath pkg = {"pkg"};
    env.module_names.push_back(StringOfPath(pkg));
    AddName(env.name_maps, pkg, "g", EntityKind::Value, pkg);
    AddName(env.name_maps, pkg, "Rec", EntityKind::Type, pkg);

    const auto call_value =
        ResolveExpr(env.res_ctx, MakeCallExpr(MakeIdentExpr("f"), {}));
    assert(call_value.ok);

    const auto call_record =
        ResolveExpr(env.res_ctx, MakeCallExpr(MakeIdentExpr("Rec"), {}));
    assert(call_record.ok);

    const auto call_path_value =
        ResolveExpr(env.res_ctx, MakeCallExpr(MakePathExpr(pkg, "g"), {}));
    assert(call_path_value.ok);

    const auto call_path_record =
        ResolveExpr(env.res_ctx, MakeCallExpr(MakePathExpr(pkg, "Rec"), {}));
    assert(call_path_record.ok);

    const auto call_other =
        ResolveExpr(env.res_ctx,
                    MakeCallExpr(MakeLiteralExpr(TokenKind::IntLiteral, "0"),
                                 {MakeArg(MakeLiteralExpr(TokenKind::IntLiteral, "1"))}));
    assert(call_other.ok);
  }

  {
    SPEC_COV("ResolveExpr-RecordExpr");
    SPEC_COV("ResolveTypeRef-Path");
    SPEC_COV("ResolveTypeRef-ModalState");

    TestEnv env;
    env.Init();
    AddType(env.ctx, "Rec");
    AddType(env.ctx, "Modal");

    RecordExpr rec_expr;
    rec_expr.target = TypePath{"Rec"};
    rec_expr.fields = {MakeFieldInit("x", MakeLiteralExpr(TokenKind::IntLiteral, "1"))};
    const auto rec_res = ResolveExpr(env.res_ctx, MakeExpr(rec_expr));
    assert(rec_res.ok);

    RecordExpr modal_expr;
    cursive0::syntax::ModalStateRef modal_ref;
    modal_ref.path = TypePath{"Modal"};
    modal_ref.state = "S";
    modal_expr.target = modal_ref;
    modal_expr.fields = {MakeFieldInit("y", MakeLiteralExpr(TokenKind::IntLiteral, "2"))};
    const auto modal_res = ResolveExpr(env.res_ctx, MakeExpr(modal_expr));
    assert(modal_res.ok);
  }

  {
    SPEC_COV("ResolveExpr-EnumLiteral");
    SPEC_COV("ResolveEnumPayload-None");
    SPEC_COV("ResolveEnumPayload-Tuple");
    SPEC_COV("ResolveEnumPayload-Record");

    TestEnv env;
    env.Init();

    EnumLiteralExpr none_expr;
    none_expr.path = Path{"E", "None"};
    none_expr.payload_opt = std::nullopt;
    const auto none_res = ResolveExpr(env.res_ctx, MakeExpr(none_expr));
    assert(none_res.ok);

    EnumPayloadParen tuple_payload;
    tuple_payload.elements = {MakeLiteralExpr(TokenKind::IntLiteral, "1")};
    EnumLiteralExpr tuple_expr;
    tuple_expr.path = Path{"E", "Tuple"};
    tuple_expr.payload_opt = EnumPayload{tuple_payload};
    const auto tuple_res = ResolveExpr(env.res_ctx, MakeExpr(tuple_expr));
    assert(tuple_res.ok);

    EnumPayloadBrace record_payload;
    record_payload.fields = {MakeFieldInit("x", MakeLiteralExpr(TokenKind::IntLiteral, "2"))};
    EnumLiteralExpr record_expr;
    record_expr.path = Path{"E", "Record"};
    record_expr.payload_opt = EnumPayload{record_payload};
    const auto record_res = ResolveExpr(env.res_ctx, MakeExpr(record_expr));
    assert(record_res.ok);
  }

  {
    SPEC_COV("ResolveExpr-Alloc-Explicit");
    SPEC_COV("ResolveExpr-Alloc-Implicit");

    TestEnv env;
    env.Init();
    AddRegionAlias(env.ctx, "r");
    cursive0::syntax::AllocExpr explicit_alloc;
    explicit_alloc.region_opt = "r";
    explicit_alloc.value = MakeLiteralExpr(TokenKind::IntLiteral, "0");
    const auto explicit_res = ResolveExpr(env.res_ctx, MakeExpr(explicit_alloc));
    assert(explicit_res.ok);

    cursive0::syntax::AllocExpr implicit_alloc;
    implicit_alloc.region_opt = std::nullopt;
    implicit_alloc.value = MakeLiteralExpr(TokenKind::IntLiteral, "1");
    const auto implicit_res = ResolveExpr(env.res_ctx, MakeExpr(implicit_alloc));
    assert(implicit_res.ok);
  }

  {
    SPEC_COV("ResolveExpr-LoopIter");

    TestEnv env;
    env.Init();
    AddType(env.ctx, "IterType");

    LoopIterExpr loop;
    loop.pattern = MakeIdentPattern("item");
    loop.type_opt = MakeTypePathType("IterType");
    loop.iter = MakeLiteralExpr(TokenKind::IntLiteral, "0");
    loop.body = nullptr;
    const auto resolved = ResolveExpr(env.res_ctx, MakeExpr(loop));
    assert(resolved.ok);
  }

  {
    SPEC_COV("ResolveExpr-Block");
    TestEnv env;
    env.Init();
    Block block;
    block.stmts = {};
    block.tail_opt = nullptr;
    block.span = EmptySpan();
    BlockExpr expr;
    expr.block = std::make_shared<Block>(block);
    const auto resolved = ResolveExpr(env.res_ctx, MakeExpr(expr));
    assert(resolved.ok);
  }

  {
    SPEC_COV("ResolveStmtSeq-Empty");
    SPEC_COV("ResolveStmtSeq-Cons");
    SPEC_COV("ResolveStmt-Var");
    SPEC_COV("ResolveStmt-ShadowVar");
    SPEC_COV("ResolveStmt-Frame-Explicit");

    TestEnv env;
    env.Init();
    AddValue(env.ctx, "frame");
    AddOuterValue(env.ctx, "shadow");

    VarStmt var_stmt;
    var_stmt.binding.init = MakeLiteralExpr(TokenKind::IntLiteral, "1");
    var_stmt.binding.type_opt = nullptr;
    var_stmt.binding.pat = nullptr;
    var_stmt.binding.span = EmptySpan();
    var_stmt.span = EmptySpan();

    ShadowVarStmt shadow_stmt;
    shadow_stmt.name = "shadow";
    shadow_stmt.type_opt = nullptr;
    shadow_stmt.init = MakeLiteralExpr(TokenKind::IntLiteral, "2");
    shadow_stmt.span = EmptySpan();

    FrameStmt frame_stmt;
    frame_stmt.target_opt = Identifier{"frame"};
    frame_stmt.body = std::make_shared<Block>(Block{});
    frame_stmt.span = EmptySpan();

    const auto empty_seq = ResolveStmtSeq(env.res_ctx, {});
    assert(empty_seq.ok);

    const std::vector<cursive0::syntax::Stmt> stmts = {var_stmt, shadow_stmt, frame_stmt};
    const auto seq = ResolveStmtSeq(env.res_ctx, stmts);
    assert(seq.ok);
  }

  {
    SPEC_COV("ResolveExpr-Ident-Err");
    TestEnv env;
    env.Init();
    const auto missing = ResolveExpr(env.res_ctx, MakeIdentExpr("missing"));
    assert(!missing.ok);
    assert(missing.diag_id ==
           std::optional<std::string_view>("ResolveExpr-Ident-Err"));
  }

  {
    SPEC_COV("ResolveExpr-Alloc-Explicit-ByAlias");
    TestEnv env;
    env.Init();
    AddRegionAlias(env.ctx, "r");
    BinaryExpr bin;
    bin.op = "^";
    bin.lhs = MakeIdentExpr("r");
    bin.rhs = MakeLiteralExpr(TokenKind::IntLiteral, "1");
    const auto alloc = ResolveExpr(env.res_ctx, MakeExpr(bin));
    assert(alloc.ok);
  }

  {
    SPEC_COV("ResolveStmt-Let");
    SPEC_COV("ResolveStmt-ShadowLet");
    SPEC_COV("ResolveStmt-Defer");
    SPEC_COV("ResolveStmt-Region");
    SPEC_COV("ResolveStmt-Frame-Implicit");

    TestEnv env;
    env.Init();
    AddOuterValue(env.ctx, "shadowed");

    LetStmt let_stmt;
    let_stmt.binding.init = MakeLiteralExpr(TokenKind::IntLiteral, "1");
    let_stmt.binding.type_opt = nullptr;
    let_stmt.binding.pat = nullptr;
    let_stmt.binding.span = EmptySpan();
    let_stmt.span = EmptySpan();

    ShadowLetStmt shadow_stmt;
    shadow_stmt.name = "shadowed";
    shadow_stmt.type_opt = nullptr;
    shadow_stmt.init = MakeLiteralExpr(TokenKind::IntLiteral, "2");
    shadow_stmt.span = EmptySpan();

    Block empty_block;
    empty_block.stmts = {};
    empty_block.tail_opt = nullptr;
    empty_block.span = EmptySpan();
    auto block_ptr = std::make_shared<Block>(empty_block);

    DeferStmt defer_stmt;
    defer_stmt.body = block_ptr;
    defer_stmt.span = EmptySpan();

    RegionStmt region_stmt;
    region_stmt.opts_opt = nullptr;
    region_stmt.alias_opt = std::nullopt;
    region_stmt.body = block_ptr;
    region_stmt.span = EmptySpan();

    FrameStmt frame_stmt;
    frame_stmt.target_opt = std::nullopt;
    frame_stmt.body = block_ptr;
    frame_stmt.span = EmptySpan();

    const std::vector<cursive0::syntax::Stmt> stmts = {
        let_stmt, shadow_stmt, defer_stmt, region_stmt, frame_stmt};
    const auto seq = ResolveStmtSeq(env.res_ctx, stmts);
    assert(seq.ok);
  }

  return 0;
}
