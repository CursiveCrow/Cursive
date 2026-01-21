#include <cassert>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "cursive0/core/assert_spec.h"
#include "cursive0/analysis/types/context.h"
#include "cursive0/analysis/resolve/scopes.h"
#include "cursive0/analysis/types/type_expr.h"
#include "cursive0/analysis/types/type_match.h"
#include "cursive0/analysis/types/type_pattern.h"
#include "cursive0/analysis/types/type_stmt.h"
#include "cursive0/analysis/types/types.h"
#include "cursive0/syntax/ast.h"

namespace {

using cursive0::analysis::CheckMatchExpr;
using cursive0::analysis::CheckResult;
using cursive0::analysis::ExprTypeResult;
using cursive0::analysis::MakeTypeModalState;
using cursive0::analysis::MakeTypePath;
using cursive0::analysis::MakeTypePerm;
using cursive0::analysis::MakeTypePrim;
using cursive0::analysis::MakeTypeTuple;
using cursive0::analysis::MakeTypeUnion;
using cursive0::analysis::Permission;
using cursive0::analysis::ScopeContext;
using cursive0::analysis::StmtTypeContext;
using cursive0::analysis::TypeBinding;
using cursive0::analysis::TypeEnv;
using cursive0::analysis::TypeMatchExpr;
using cursive0::analysis::TypeMatchPattern;
using cursive0::analysis::TypeRef;
using cursive0::analysis::PathKeyOf;
using cursive0::analysis::IdKeyOf;

using cursive0::syntax::Block;
using cursive0::syntax::BlockExpr;
using cursive0::syntax::EnumDecl;
using cursive0::syntax::EnumPattern;
using cursive0::syntax::EnumPayloadParen;
using cursive0::syntax::EnumPayloadBrace;
using cursive0::syntax::EnumPayloadPattern;
using cursive0::syntax::RecordPayloadPattern;
using cursive0::syntax::Expr;
using cursive0::syntax::ExprNode;
using cursive0::syntax::ExprPtr;
using cursive0::syntax::FieldDecl;
using cursive0::syntax::FieldPattern;
using cursive0::syntax::IdentifierExpr;
using cursive0::syntax::IdentifierPattern;
using cursive0::syntax::LiteralExpr;
using cursive0::syntax::LiteralPattern;
using cursive0::syntax::MatchArm;
using cursive0::syntax::MatchExpr;
using cursive0::syntax::ModalDecl;
using cursive0::syntax::ModalPattern;
using cursive0::syntax::ModalRecordPayload;
using cursive0::syntax::Mutability;
using cursive0::syntax::Pattern;
using cursive0::syntax::PatternNode;
using cursive0::syntax::PatternPtr;
using cursive0::syntax::RangeKind;
using cursive0::syntax::RangePattern;
using cursive0::syntax::RecordDecl;
using cursive0::syntax::RecordMember;
using cursive0::syntax::RecordPattern;
using cursive0::syntax::StateBlock;
using cursive0::syntax::StateFieldDecl;
using cursive0::syntax::StateMember;
using cursive0::syntax::Token;
using cursive0::syntax::TokenKind;
using cursive0::syntax::TuplePattern;
using cursive0::syntax::TuplePayloadPattern;
using cursive0::syntax::TypedPattern;
using cursive0::syntax::Type;
using cursive0::syntax::TypePath;
using cursive0::syntax::VariantDecl;
using cursive0::syntax::VariantPayload;
using cursive0::syntax::VariantPayloadRecord;
using cursive0::syntax::VariantPayloadTuple;
using cursive0::syntax::Visibility;
using cursive0::syntax::WildcardPattern;
using cursive0::syntax::MoveExpr;

cursive0::core::Span EmptySpan() { return {}; }

std::shared_ptr<Type> MakeTypePrimAst(const char* name) {
  auto type = std::make_shared<Type>();
  type->node = cursive0::syntax::TypePrim{std::string(name)};
  return type;
}

Token MakeToken(TokenKind kind, const char* lexeme) {
  Token tok;
  tok.kind = kind;
  tok.lexeme = lexeme;
  return tok;
}

ExprPtr MakeExpr(ExprNode node) {
  auto expr = std::make_shared<Expr>();
  expr->node = std::move(node);
  return expr;
}

ExprPtr MakeIdentExpr(const char* name) {
  return MakeExpr(IdentifierExpr{std::string(name)});
}

ExprPtr MakeIntLiteralExpr(const char* lexeme) {
  return MakeExpr(LiteralExpr{MakeToken(TokenKind::IntLiteral, lexeme)});
}

ExprPtr MakeBoolLiteralExpr(bool value) {
  return MakeExpr(LiteralExpr{MakeToken(TokenKind::BoolLiteral,
                                        value ? "true" : "false")});
}

ExprPtr MakeMoveExpr(const ExprPtr& place) {
  MoveExpr move;
  move.place = place;
  return MakeExpr(move);
}

ExprPtr MakeBlockBody(const ExprPtr& tail) {
  auto block = std::make_shared<Block>();
  block->stmts = {};
  block->tail_opt = tail;
  block->span = EmptySpan();
  BlockExpr block_expr;
  block_expr.block = block;
  return MakeExpr(block_expr);
}

PatternPtr MakePattern(PatternNode node) {
  auto pat = std::make_shared<Pattern>();
  pat->node = std::move(node);
  return pat;
}

PatternPtr MakeWildcardPattern() {
  return MakePattern(WildcardPattern{});
}

PatternPtr MakeIdentPattern(const char* name) {
  return MakePattern(IdentifierPattern{std::string(name)});
}

PatternPtr MakeTypedPattern(const char* name, const std::shared_ptr<Type>& type) {
  return MakePattern(TypedPattern{std::string(name), type});
}

PatternPtr MakeLiteralPattern(TokenKind kind, const char* lexeme) {
  return MakePattern(LiteralPattern{MakeToken(kind, lexeme)});
}

PatternPtr MakeTuplePattern(std::vector<PatternPtr> elements) {
  TuplePattern tuple;
  tuple.elements = std::move(elements);
  return MakePattern(tuple);
}

FieldPattern MakeFieldPattern(const char* name, PatternPtr pat_opt = nullptr) {
  FieldPattern field;
  field.name = std::string(name);
  field.pattern_opt = pat_opt;
  field.span = EmptySpan();
  return field;
}

PatternPtr MakeRecordPattern(const TypePath& path, std::vector<FieldPattern> fields) {
  RecordPattern record;
  record.path = path;
  record.fields = std::move(fields);
  return MakePattern(record);
}

PatternPtr MakeEnumPatternUnit(const TypePath& path, const char* name) {
  EnumPattern pat;
  pat.path = path;
  pat.name = std::string(name);
  pat.payload_opt = std::nullopt;
  return MakePattern(pat);
}

PatternPtr MakeEnumPatternTuple(const TypePath& path,
                                const char* name,
                                std::vector<PatternPtr> elements) {
  EnumPattern pat;
  pat.path = path;
  pat.name = std::string(name);
  TuplePayloadPattern payload;
  payload.elements = std::move(elements);
  pat.payload_opt = EnumPayloadPattern{payload};
  return MakePattern(pat);
}

PatternPtr MakeEnumPatternRecord(const TypePath& path,
                                 const char* name,
                                 std::vector<FieldPattern> fields) {
  EnumPattern pat;
  pat.path = path;
  pat.name = std::string(name);
  RecordPayloadPattern payload;
  payload.fields = std::move(fields);
  pat.payload_opt = EnumPayloadPattern{payload};
  return MakePattern(pat);
}

PatternPtr MakeModalPattern(const char* state, std::vector<FieldPattern> fields) {
  ModalPattern pat;
  pat.state = std::string(state);
  ModalRecordPayload payload;
  payload.fields = std::move(fields);
  pat.fields_opt = payload;
  return MakePattern(pat);
}

PatternPtr MakeRangePattern(RangeKind kind, PatternPtr lo, PatternPtr hi) {
  RangePattern range;
  range.kind = kind;
  range.lo = std::move(lo);
  range.hi = std::move(hi);
  return MakePattern(range);
}

MatchArm MakeArm(PatternPtr pattern, ExprPtr body, ExprPtr guard = nullptr) {
  MatchArm arm;
  arm.pattern = std::move(pattern);
  arm.guard_opt = guard;
  arm.body = std::move(body);
  arm.span = EmptySpan();
  return arm;
}

MatchExpr MakeMatchExpr(ExprPtr value, std::vector<MatchArm> arms) {
  MatchExpr match;
  match.value = std::move(value);
  match.arms = std::move(arms);
  return match;
}

RecordDecl MakeRecordDecl(const char* name, std::vector<FieldDecl> fields) {
  RecordDecl decl;
  decl.vis = Visibility::Public;
  decl.name = std::string(name);
  decl.implements = {};
  for (auto& field : fields) {
    decl.members.emplace_back(field);
  }
  decl.span = EmptySpan();
  decl.doc = {};
  return decl;
}

FieldDecl MakeFieldDecl(const char* name, const char* type_name) {
  FieldDecl field;
  field.vis = Visibility::Public;
  field.name = std::string(name);
  field.type = MakeTypePrimAst(type_name);
  field.init_opt = nullptr;
  field.span = EmptySpan();
  field.doc_opt = std::nullopt;
  return field;
}

VariantDecl MakeVariantUnit(const char* name) {
  VariantDecl decl;
  decl.name = std::string(name);
  decl.payload_opt = std::nullopt;
  decl.discriminant_opt = std::nullopt;
  decl.span = EmptySpan();
  decl.doc_opt = std::nullopt;
  return decl;
}

VariantDecl MakeVariantTuple(const char* name,
                             std::vector<std::shared_ptr<Type>> elements) {
  VariantPayloadTuple payload;
  payload.elements = std::move(elements);
  VariantDecl decl;
  decl.name = std::string(name);
  decl.payload_opt = VariantPayload{payload};
  decl.discriminant_opt = std::nullopt;
  decl.span = EmptySpan();
  decl.doc_opt = std::nullopt;
  return decl;
}

VariantDecl MakeVariantRecord(const char* name, std::vector<FieldDecl> fields) {
  VariantPayloadRecord payload;
  payload.fields = std::move(fields);
  VariantDecl decl;
  decl.name = std::string(name);
  decl.payload_opt = VariantPayload{payload};
  decl.discriminant_opt = std::nullopt;
  decl.span = EmptySpan();
  decl.doc_opt = std::nullopt;
  return decl;
}

EnumDecl MakeEnumDecl(const char* name, std::vector<VariantDecl> variants) {
  EnumDecl decl;
  decl.vis = Visibility::Public;
  decl.name = std::string(name);
  decl.implements = {};
  decl.variants = std::move(variants);
  decl.span = EmptySpan();
  decl.doc = {};
  return decl;
}

ModalDecl MakeModalDecl(const char* name, std::vector<StateBlock> states) {
  ModalDecl decl;
  decl.vis = Visibility::Public;
  decl.name = std::string(name);
  decl.implements = {};
  decl.states = std::move(states);
  decl.span = EmptySpan();
  decl.doc = {};
  return decl;
}

StateBlock MakeStateBlock(const char* name, std::vector<StateMember> members) {
  StateBlock block;
  block.name = std::string(name);
  block.members = std::move(members);
  block.span = EmptySpan();
  block.doc_opt = std::nullopt;
  return block;
}

StateFieldDecl MakeStateFieldDecl(const char* name, const char* type_name) {
  StateFieldDecl field;
  field.vis = Visibility::Public;
  field.name = std::string(name);
  field.type = MakeTypePrimAst(type_name);
  field.span = EmptySpan();
  field.doc_opt = std::nullopt;
  return field;
}

ScopeContext MakeContext() {
  ScopeContext ctx;
  ctx.current_module = {"test"};
  return ctx;
}

void AddRecordType(ScopeContext& ctx, const TypePath& path,
                   const RecordDecl& decl) {
  ctx.sigma.types.emplace(PathKeyOf(path), decl);
}

void AddEnumType(ScopeContext& ctx, const TypePath& path,
                 const EnumDecl& decl) {
  ctx.sigma.types.emplace(PathKeyOf(path), decl);
}

void AddModalType(ScopeContext& ctx, const TypePath& path,
                  const ModalDecl& decl) {
  ctx.sigma.types.emplace(PathKeyOf(path), decl);
}

TypeEnv EmptyEnv() {
  TypeEnv env;
  env.scopes.emplace_back();
  return env;
}

void AddBinding(TypeEnv& env, const char* name, const TypeRef& type) {
  env.scopes.back().emplace(IdKeyOf(name), TypeBinding{Mutability::Let, type});
}

StmtTypeContext DefaultTypeCtx() {
  StmtTypeContext ctx;
  ctx.return_type = MakeTypePrim("()");
  return ctx;
}

void SpecCoverage() {
  SPEC_COV("Pat-StripPerm");
  SPEC_COV("Pat-Dup-R-Err");
  SPEC_COV("Pat-Wildcard-R");
  SPEC_COV("Pat-Ident-R");
  SPEC_COV("Pat-Literal-R");
  SPEC_COV("Pat-Tuple-R-Arity-Err");
  SPEC_COV("Pat-Tuple-R");
  SPEC_COV("Pat-Record-R");
  SPEC_COV("RecordPattern-UnknownField");
  SPEC_COV("Pat-Enum-Unit-R");
  SPEC_COV("Pat-Enum-Tuple-R");
  SPEC_COV("Pat-Enum-Record-R");
  SPEC_COV("Pat-Modal-R");
  SPEC_COV("Pat-Modal-State-R");
  SPEC_COV("Pat-Range-R");
  SPEC_COV("RangePattern-NonConst");
  SPEC_COV("RangePattern-Empty");
  SPEC_COV("ArmBody-Expr");
  SPEC_COV("ArmBody-Block");
  SPEC_COV("ArmBody-Expr-Chk");
  SPEC_COV("ArmBody-Block-Chk");
  SPEC_COV("T-Match-Enum");
  SPEC_COV("Chk-Match-Enum");
  SPEC_COV("T-Match-Modal");
  SPEC_COV("Chk-Match-Modal");
  SPEC_COV("Match-Modal-NonExhaustive");
  SPEC_COV("T-Match-Union");
  SPEC_COV("Chk-Match-Union");
  SPEC_COV("Match-Union-NonExhaustive");
  SPEC_COV("T-Match-Other");
  SPEC_COV("Chk-Match-Other");
}

}  // namespace

int main() {
  SpecCoverage();

  ScopeContext ctx = MakeContext();

  // Record type for patterns
  RecordDecl record = MakeRecordDecl(
      "Point", {MakeFieldDecl("x", "i32"), MakeFieldDecl("y", "i32")});
  AddRecordType(ctx, TypePath{"Point"}, record);

  // Enum type for patterns/match
  EnumDecl enum_decl = MakeEnumDecl(
      "Color", {MakeVariantUnit("Red"),
                MakeVariantTuple("Green", {MakeTypePrimAst("i32")}),
                MakeVariantRecord("Blue", {MakeFieldDecl("b", "i32")})});
  AddEnumType(ctx, TypePath{"Color"}, enum_decl);

  // Modal type for patterns/match
  StateBlock on_state = MakeStateBlock(
      "On", {StateMember{MakeStateFieldDecl("flag", "bool")}});
  StateBlock off_state = MakeStateBlock(
      "Off", {StateMember{MakeStateFieldDecl("flag", "bool")}});
  ModalDecl modal_decl = MakeModalDecl("Device", {on_state, off_state});
  AddModalType(ctx, TypePath{"Device"}, modal_decl);

  // Pattern typing: strip perm
  {
    const auto pat = MakeIdentPattern("x");
    const auto expected = MakeTypePerm(Permission::Const, MakeTypePrim("i32"));
    const auto res = TypeMatchPattern(ctx, pat, expected);
    assert(res.ok);
  }

  // Pattern typing: duplicate names
  {
    const auto pat = MakeTuplePattern({MakeIdentPattern("x"),
                                       MakeIdentPattern("x")});
    const auto expected = MakeTypeTuple({MakeTypePrim("i32"),
                                         MakeTypePrim("i32")});
    const auto res = TypeMatchPattern(ctx, pat, expected);
    assert(!res.ok);
    assert(res.diag_id.has_value());
    assert(*res.diag_id == "Pat-Dup-Err");
  }

  // Pattern typing: wildcard, ident, literal
  {
    const auto expected = MakeTypePrim("i32");
    const auto wild = TypeMatchPattern(ctx, MakeWildcardPattern(), expected);
    assert(wild.ok);
    const auto ident = TypeMatchPattern(ctx, MakeIdentPattern("x"), expected);
    assert(ident.ok);
    const auto lit = TypeMatchPattern(
        ctx, MakeLiteralPattern(TokenKind::IntLiteral, "5"), expected);
    assert(lit.ok);
  }

  // Tuple pattern arity error and success
  {
    const auto expected = MakeTypeTuple({MakeTypePrim("i32"), MakeTypePrim("i32")});
    const auto bad = TypeMatchPattern(ctx, MakeTuplePattern({MakeIdentPattern("x")}),
                                      expected);
    assert(!bad.ok);
    assert(bad.diag_id.has_value());
    assert(*bad.diag_id == "Pat-Tuple-Arity-Err");

    const auto good = TypeMatchPattern(
        ctx, MakeTuplePattern({MakeIdentPattern("x"), MakeIdentPattern("y")}),
        expected);
    assert(good.ok);
  }

  // Record pattern ok and unknown field
  {
    const auto expected = MakeTypePath({"Point"});
    const auto ok_pat = MakeRecordPattern(
        {"Point"}, {MakeFieldPattern("x"), MakeFieldPattern("y")});
    const auto ok = TypeMatchPattern(ctx, ok_pat, expected);
    assert(ok.ok);

    const auto bad_pat = MakeRecordPattern(
        {"Point"}, {MakeFieldPattern("z")});
    const auto bad = TypeMatchPattern(ctx, bad_pat, expected);
    assert(!bad.ok);
    assert(bad.diag_id.has_value());
    assert(*bad.diag_id == "RecordPattern-UnknownField");
  }

  // Enum patterns
  {
    const auto expected = MakeTypePath({"Color"});
    const auto unit = TypeMatchPattern(ctx, MakeEnumPatternUnit({"Color"}, "Red"),
                                       expected);
    assert(unit.ok);

    const auto tuple = TypeMatchPattern(
        ctx,
        MakeEnumPatternTuple({"Color"}, "Green", {MakeIdentPattern("x")}),
        expected);
    assert(tuple.ok);

    const auto record_pat = TypeMatchPattern(
        ctx,
        MakeEnumPatternRecord({"Color"}, "Blue", {MakeFieldPattern("b")}),
        expected);
    assert(record_pat.ok);
  }

  // Modal patterns: general and state-specific
  {
    const auto expected = MakeTypePath({"Device"});
    const auto general = TypeMatchPattern(
        ctx, MakeModalPattern("On", {MakeFieldPattern("flag")}), expected);
    assert(general.ok);

    const auto state_expected = MakeTypeModalState({"Device"}, "On");
    const auto state_pat = TypeMatchPattern(
        ctx, MakeModalPattern("On", {MakeFieldPattern("flag")}), state_expected);
    assert(state_pat.ok);
  }

  // Range patterns
  {
    const auto expected = MakeTypePrim("i32");
    const auto range_ok = TypeMatchPattern(
        ctx,
        MakeRangePattern(RangeKind::Inclusive,
                         MakeLiteralPattern(TokenKind::IntLiteral, "1"),
                         MakeLiteralPattern(TokenKind::IntLiteral, "3")),
        expected);
    assert(range_ok.ok);

    const auto non_const = TypeMatchPattern(
        ctx,
        MakeRangePattern(RangeKind::Inclusive,
                         MakeIdentPattern("lo"),
                         MakeLiteralPattern(TokenKind::IntLiteral, "3")),
        expected);
    assert(!non_const.ok);
    assert(non_const.diag_id.has_value());
    assert(*non_const.diag_id == "RangePattern-NonConst");

    const auto empty = TypeMatchPattern(
        ctx,
        MakeRangePattern(RangeKind::Exclusive,
                         MakeLiteralPattern(TokenKind::IntLiteral, "3"),
                         MakeLiteralPattern(TokenKind::IntLiteral, "3")),
        expected);
    assert(!empty.ok);
    assert(empty.diag_id.has_value());
    assert(*empty.diag_id == "RangePattern-Empty");
  }

  // Match typing: enum
  {
    TypeEnv env = EmptyEnv();
    AddBinding(env, "c", MakeTypePath({"Color"}));
    auto scrutinee = MakeMoveExpr(MakeIdentExpr("c"));
    auto body = MakeIntLiteralExpr("1");
    auto match = MakeMatchExpr(
        scrutinee,
        {MakeArm(MakeEnumPatternUnit({"Color"}, "Red"), body),
         MakeArm(MakeEnumPatternTuple({"Color"}, "Green", {MakeIdentPattern("x")}), body),
         MakeArm(MakeEnumPatternRecord({"Color"}, "Blue", {MakeFieldPattern("b")}), body)});

    const auto typed = TypeMatchExpr(ctx, DefaultTypeCtx(), match, env);
    assert(typed.ok);
    assert(typed.type);
    const auto* prim = std::get_if<cursive0::analysis::TypePrim>(&typed.type->node);
    assert(prim && prim->name == "i32");

    const auto check = CheckMatchExpr(ctx, DefaultTypeCtx(), match, env,
                                      MakeTypePrim("i32"));
    assert(check.ok);
  }

  // Match typing: modal (exhaustive)
  {
    TypeEnv env = EmptyEnv();
    AddBinding(env, "m", MakeTypePath({"Device"}));
    auto scrutinee = MakeMoveExpr(MakeIdentExpr("m"));
    auto body = MakeBoolLiteralExpr(true);
    auto match = MakeMatchExpr(
        scrutinee,
        {MakeArm(MakeModalPattern("On", {MakeFieldPattern("flag")}), body),
         MakeArm(MakeModalPattern("Off", {MakeFieldPattern("flag")}), body)});

    const auto typed = TypeMatchExpr(ctx, DefaultTypeCtx(), match, env);
    assert(typed.ok);

    const auto check = CheckMatchExpr(ctx, DefaultTypeCtx(), match, env,
                                      MakeTypePrim("bool"));
    assert(check.ok);
  }

  // Match typing: modal non-exhaustive
  {
    TypeEnv env = EmptyEnv();
    AddBinding(env, "m", MakeTypePath({"Device"}));
    auto scrutinee = MakeMoveExpr(MakeIdentExpr("m"));
    auto body = MakeBoolLiteralExpr(true);
    auto match = MakeMatchExpr(
        scrutinee,
        {MakeArm(MakeModalPattern("On", {MakeFieldPattern("flag")}), body)});

    const auto typed = TypeMatchExpr(ctx, DefaultTypeCtx(), match, env);
    assert(!typed.ok);
    assert(typed.diag_id.has_value());
    assert(*typed.diag_id == "Match-Modal-NonExhaustive");
  }

  // Match typing: union
  {
    TypeEnv env = EmptyEnv();
    auto union_type = MakeTypeUnion({MakeTypePrim("i32"), MakeTypePrim("bool")});
    AddBinding(env, "u", union_type);
    auto scrutinee = MakeMoveExpr(MakeIdentExpr("u"));
    auto body = MakeIntLiteralExpr("1");
    auto match = MakeMatchExpr(
        scrutinee,
        {MakeArm(MakeTypedPattern("x", MakeTypePrimAst("i32")), body),
         MakeArm(MakeTypedPattern("y", MakeTypePrimAst("bool")), body)});

    const auto typed = TypeMatchExpr(ctx, DefaultTypeCtx(), match, env);
    assert(typed.ok);

    const auto check = CheckMatchExpr(ctx, DefaultTypeCtx(), match, env,
                                      MakeTypePrim("i32"));
    assert(check.ok);
  }

  // Match typing: union non-exhaustive
  {
    TypeEnv env = EmptyEnv();
    auto union_type = MakeTypeUnion({MakeTypePrim("i32"), MakeTypePrim("bool")});
    AddBinding(env, "u", union_type);
    auto scrutinee = MakeMoveExpr(MakeIdentExpr("u"));
    auto body = MakeIntLiteralExpr("1");
    auto match = MakeMatchExpr(
        scrutinee,
        {MakeArm(MakeTypedPattern("x", MakeTypePrimAst("i32")), body)});

    const auto typed = TypeMatchExpr(ctx, DefaultTypeCtx(), match, env);
    assert(!typed.ok);
    assert(typed.diag_id.has_value());
    assert(*typed.diag_id == "Match-Union-NonExhaustive");
  }

  // Match typing: other with block body (ArmBody-Block)
  {
    auto scrutinee = MakeIntLiteralExpr("1");
    auto body = MakeBlockBody(MakeIntLiteralExpr("2"));
    auto match = MakeMatchExpr(scrutinee, {MakeArm(MakeWildcardPattern(), body)});

    const auto typed = TypeMatchExpr(ctx, DefaultTypeCtx(), match, EmptyEnv());
    assert(typed.ok);
  }

  // Match checking: other with block body (ArmBody-Block-Chk)
  {
    auto scrutinee = MakeIntLiteralExpr("1");
    auto body = MakeBlockBody(MakeIntLiteralExpr("2"));
    auto match = MakeMatchExpr(scrutinee, {MakeArm(MakeWildcardPattern(), body)});

    const auto check = CheckMatchExpr(ctx, DefaultTypeCtx(), match, EmptyEnv(),
                                      MakeTypePrim("i32"));
    assert(check.ok);
  }

  return 0;
}
