#include <cassert>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "cursive0/core/assert_spec.h"
#include "cursive0/core/source_load.h"
#include "cursive0/analysis/types/context.h"
#include "cursive0/analysis/resolve/scopes.h"
#include "cursive0/analysis/types/type_expr.h"
#include "cursive0/analysis/types/type_stmt.h"
#include "cursive0/analysis/types/types.h"
#include "cursive0/syntax/ast.h"
#include "cursive0/syntax/lexer.h"

namespace {

using cursive0::analysis::BitcopyType;
using cursive0::analysis::CastValid;
using cursive0::analysis::CheckIfExpr;
using cursive0::analysis::CheckPlaceAgainst;
using cursive0::analysis::EqType;
using cursive0::analysis::ExprTypeResult;
using cursive0::analysis::IsPlaceExpr;
using cursive0::analysis::MakeTypeArray;
using cursive0::analysis::MakeTypeModalState;
using cursive0::analysis::MakeTypePath;
using cursive0::analysis::MakeTypePerm;
using cursive0::analysis::MakeTypePrim;
using cursive0::analysis::MakeTypePtr;
using cursive0::analysis::MakeTypeRawPtr;
using cursive0::analysis::MakeTypeRange;
using cursive0::analysis::MakeTypeSlice;
using cursive0::analysis::MakeTypeTuple;
using cursive0::analysis::MakeTypeUnion;
using cursive0::analysis::MakeTypeDynamic;
using cursive0::analysis::OrdType;
using cursive0::analysis::PathKeyOf;
using cursive0::analysis::Permission;
using cursive0::analysis::PtrState;
using cursive0::analysis::RawPtrQual;
using cursive0::analysis::ScopeContext;
using cursive0::analysis::StmtTypeContext;
using cursive0::analysis::StripPerm;
using cursive0::analysis::Entity;
using cursive0::analysis::EntityKind;
using cursive0::analysis::EntitySource;
using cursive0::analysis::IdKeyOf;
using cursive0::analysis::TypeBinding;
using cursive0::analysis::TypeBinaryExpr;
using cursive0::analysis::TypeCastExpr;
using cursive0::analysis::TypeEnv;
using cursive0::analysis::TypeExpr;
using cursive0::analysis::TypeIdentifierExpr;
using cursive0::analysis::TypeIfExpr;
using cursive0::analysis::TypePlace;
using cursive0::analysis::TypePrim;
using cursive0::analysis::TypeRange;
using cursive0::analysis::TypeRangeExpr;
using cursive0::analysis::TypeRef;
using cursive0::analysis::TypeUnaryExpr;
using cursive0::analysis::TypeAddressOfExpr;
using cursive0::analysis::TypeAllocExpr;
using cursive0::analysis::TypeDerefExpr;
using cursive0::analysis::TypeDerefPlace;
using cursive0::analysis::TypeEnumLiteralExpr;
using cursive0::analysis::TypeFieldAccessExpr;
using cursive0::analysis::TypeFieldAccessPlace;
using cursive0::analysis::TypeMoveExpr;
using cursive0::analysis::TypePathExpr;
using cursive0::analysis::TypePathType;
using cursive0::analysis::TypePerm;
using cursive0::analysis::TypePropagateExpr;
using cursive0::analysis::TypeRecordExpr;
using cursive0::analysis::TypeTransmuteExpr;

using cursive0::syntax::AddressOfExpr;
using cursive0::syntax::AllocExpr;
using cursive0::syntax::ASTModule;
using cursive0::syntax::ClassDecl;
using cursive0::syntax::ClassItem;
using cursive0::syntax::ClassMethodDecl;
using cursive0::syntax::MethodCallExpr;
using cursive0::syntax::MethodDecl;
using cursive0::syntax::Receiver;
using cursive0::syntax::ReceiverPerm;
using cursive0::syntax::ReceiverShorthand;
using cursive0::syntax::BinaryExpr;
using cursive0::syntax::Block;
using cursive0::syntax::Binding;
using cursive0::syntax::Pattern;
using cursive0::syntax::IdentifierPattern;
using cursive0::syntax::StaticDecl;
using cursive0::syntax::Mutability;
using cursive0::syntax::CastExpr;
using cursive0::syntax::DerefExpr;
using cursive0::syntax::EnumLiteralExpr;
using cursive0::syntax::EnumPayloadBrace;
using cursive0::syntax::EnumPayloadParen;
using cursive0::syntax::ExprPtr;
using cursive0::syntax::FieldAccessExpr;
using cursive0::syntax::FieldDecl;
using cursive0::syntax::FieldInit;
using cursive0::syntax::IdentifierExpr;
using cursive0::syntax::IfExpr;
using cursive0::syntax::IndexAccessExpr;
using cursive0::syntax::LiteralExpr;
using cursive0::syntax::ModulePath;
using cursive0::syntax::MoveExpr;
using cursive0::syntax::ModalStateRef;
using cursive0::syntax::ModalDecl;
using cursive0::syntax::PathExpr;
using cursive0::syntax::ProcedureDecl;
using cursive0::syntax::PropagateExpr;
using cursive0::syntax::RangeExpr;
using cursive0::syntax::RangeKind;
using cursive0::syntax::RecordDecl;
using cursive0::syntax::RecordExpr;
using cursive0::syntax::StateBlock;
using cursive0::syntax::StateFieldDecl;
using cursive0::syntax::StateMember;
using cursive0::syntax::Token;
using cursive0::syntax::TokenKind;
using cursive0::syntax::TransmuteExpr;
using cursive0::syntax::TupleExpr;
using cursive0::syntax::TypePath;
using cursive0::syntax::VariantDecl;
using cursive0::syntax::VariantPayloadRecord;
using cursive0::syntax::VariantPayloadTuple;
using cursive0::syntax::Visibility;

TypeRef Prim(const char* name) { return MakeTypePrim(name); }

ExprPtr MakeExpr(cursive0::syntax::ExprNode node) {
  auto expr = std::make_shared<cursive0::syntax::Expr>();
  expr->node = std::move(node);
  return expr;
}

ExprPtr MakeExprWithSpan(cursive0::syntax::ExprNode node,
                         const cursive0::core::Span& span) {
  auto expr = std::make_shared<cursive0::syntax::Expr>();
  expr->span = span;
  expr->node = std::move(node);
  return expr;
}

ExprPtr MakeIdent(const char* name) { return MakeExpr(IdentifierExpr{name}); }

TypeEnv MakeEnvWithBinding(std::string_view name, TypeRef type) {
  TypeEnv env;
  env.scopes.emplace_back();
  TypeBinding binding;
  binding.type = type;
  binding.mut = cursive0::syntax::Mutability::Let;
  env.scopes.back().emplace(std::string(name), binding);
  return env;
}

cursive0::core::Span EmptySpan() { return {}; }

std::string WriteTempSource(const std::string& text) {
  static int counter = 0;
  const auto filename =
      std::string("cursive_unsafe_span_") + std::to_string(counter++) + ".cursive";
  const auto path = std::filesystem::temp_directory_path() / filename;
  std::ofstream out(path, std::ios::binary);
  assert(out);
  out << text;
  out.close();
  return path.string();
}

cursive0::core::Span SpanForMarker(const std::string& path,
                                  const std::string& text,
                                  std::string_view marker) {
  const auto pos = text.find(marker);
  assert(pos != std::string::npos);
  const auto end = pos + marker.size();

  auto line_col = [&](std::size_t offset) {
    std::size_t line = 1;
    std::size_t col = 1;
    const auto limit = offset < text.size() ? offset : text.size();
    for (std::size_t i = 0; i < limit; ++i) {
      if (text[i] == '\n') {
        ++line;
        col = 1;
      } else {
        ++col;
      }
    }
    return std::pair<std::size_t, std::size_t>{line, col};
  };

  const auto start_loc = line_col(pos);
  const auto end_loc = line_col(end);

  cursive0::core::Span span;
  span.file = path;
  span.start_offset = pos;
  span.end_offset = end;
  span.start_line = start_loc.first;
  span.start_col = start_loc.second;
  span.end_line = end_loc.first;
  span.end_col = end_loc.second;
  return span;
}

ExprPtr MakeIntLiteral(int value) {
  Token tok;
  tok.kind = TokenKind::IntLiteral;
  tok.lexeme = std::to_string(value);
  tok.span = EmptySpan();
  return MakeExpr(LiteralExpr{tok});
}

ExprPtr MakeBoolLiteral(bool value) {
  Token tok;
  tok.kind = TokenKind::BoolLiteral;
  tok.lexeme = value ? "true" : "false";
  tok.span = EmptySpan();
  return MakeExpr(LiteralExpr{tok});
}

std::shared_ptr<cursive0::syntax::Type> MakeTypePrimAst(const char* name) {
  auto type = std::make_shared<cursive0::syntax::Type>();
  type->span = EmptySpan();
  type->node = cursive0::syntax::TypePrim{std::string(name)};
  return type;
}

std::shared_ptr<cursive0::syntax::Type> MakeTypePathAst(TypePath path) {
  auto type = std::make_shared<cursive0::syntax::Type>();
  type->span = EmptySpan();
  type->node = cursive0::syntax::TypePathType{std::move(path)};
  return type;
}

std::shared_ptr<cursive0::syntax::Type> MakeTypeTupleAst(
    std::vector<std::shared_ptr<cursive0::syntax::Type>> elements) {
  auto type = std::make_shared<cursive0::syntax::Type>();
  type->span = EmptySpan();
  cursive0::syntax::TypeTuple tuple;
  tuple.elements = std::move(elements);
  type->node = std::move(tuple);
  return type;
}

FieldDecl MakeFieldDecl(Visibility vis,
                        const char* name,
                        std::shared_ptr<cursive0::syntax::Type> type) {
  FieldDecl field;
  field.vis = vis;
  field.name = name;
  field.type = std::move(type);
  field.init_opt = nullptr;
  field.span = EmptySpan();
  field.doc_opt = std::nullopt;
  return field;
}

StateFieldDecl MakeStateFieldDecl(Visibility vis,
                                  const char* name,
                                  std::shared_ptr<cursive0::syntax::Type> type) {
  StateFieldDecl field;
  field.vis = vis;
  field.name = name;
  field.type = std::move(type);
  field.span = EmptySpan();
  field.doc_opt = std::nullopt;
  return field;
}

RecordDecl MakeRecordDecl(Visibility vis,
                          const char* name,
                          std::vector<FieldDecl> fields) {
  RecordDecl record;
  record.vis = vis;
  record.name = name;
  record.implements = {};
  record.members.clear();
  for (auto& field : fields) {
    record.members.emplace_back(std::move(field));
  }
  record.span = EmptySpan();
  record.doc = {};
  return record;
}

StateBlock MakeStateBlock(const char* name, std::vector<StateMember> members) {
  StateBlock block;
  block.name = name;
  block.members = std::move(members);
  block.span = EmptySpan();
  block.doc_opt = std::nullopt;
  return block;
}

ModalDecl MakeModalDecl(Visibility vis,
                        const char* name,
                        std::vector<StateBlock> states) {
  ModalDecl decl;
  decl.vis = vis;
  decl.name = name;
  decl.implements = {};
  decl.states = std::move(states);
  decl.span = EmptySpan();
  decl.doc = {};
  return decl;
}

std::shared_ptr<Block> MakeEmptyBlock() {
  auto block = std::make_shared<Block>();
  block->stmts = {};
  block->tail_opt = nullptr;
  block->span = EmptySpan();
  return block;
}

MethodDecl MakeMethodDecl(Visibility vis,
                          const char* name,
                          Receiver receiver,
                          std::shared_ptr<cursive0::syntax::Type> return_type) {
  MethodDecl decl;
  decl.vis = vis;
  decl.override_flag = false;
  decl.name = name;
  decl.receiver = std::move(receiver);
  decl.params = {};
  decl.return_type_opt = std::move(return_type);
  decl.body = MakeEmptyBlock();
  decl.span = EmptySpan();
  decl.doc_opt = std::nullopt;
  return decl;
}

ClassMethodDecl MakeClassMethodDecl(
    Visibility vis,
    const char* name,
    Receiver receiver,
    std::shared_ptr<cursive0::syntax::Type> return_type,
    bool has_body) {
  ClassMethodDecl decl;
  decl.vis = vis;
  decl.name = name;
  decl.receiver = std::move(receiver);
  decl.params = {};
  decl.return_type_opt = std::move(return_type);
  decl.body_opt = has_body ? MakeEmptyBlock() : nullptr;
  decl.span = EmptySpan();
  decl.doc_opt = std::nullopt;
  return decl;
}

ClassDecl MakeClassDecl(Visibility vis,
                        const char* name,
                        std::vector<ClassItem> items) {
  ClassDecl decl;
  decl.vis = vis;
  decl.name = name;
  decl.supers = {};
  decl.items = std::move(items);
  decl.span = EmptySpan();
  decl.doc = {};
  return decl;
}

VariantDecl MakeVariantUnit(const char* name) {
  VariantDecl decl;
  decl.name = name;
  decl.payload_opt = std::nullopt;
  decl.discriminant_opt = std::nullopt;
  decl.span = EmptySpan();
  decl.doc_opt = std::nullopt;
  return decl;
}

VariantDecl MakeVariantTuple(
    const char* name,
    std::vector<std::shared_ptr<cursive0::syntax::Type>> elements) {
  VariantPayloadTuple payload;
  payload.elements = std::move(elements);
  VariantDecl decl;
  decl.name = name;
  decl.payload_opt = payload;
  decl.discriminant_opt = std::nullopt;
  decl.span = EmptySpan();
  decl.doc_opt = std::nullopt;
  return decl;
}

VariantDecl MakeVariantRecord(const char* name,
                              std::vector<FieldDecl> fields) {
  VariantPayloadRecord payload;
  payload.fields = std::move(fields);
  VariantDecl decl;
  decl.name = name;
  decl.payload_opt = payload;
  decl.discriminant_opt = std::nullopt;
  decl.span = EmptySpan();
  decl.doc_opt = std::nullopt;
  return decl;
}

cursive0::syntax::EnumDecl MakeEnumDecl(Visibility vis,
                                        const char* name,
                                        std::vector<VariantDecl> variants) {
  cursive0::syntax::EnumDecl decl;
  decl.vis = vis;
  decl.name = name;
  decl.implements = {};
  decl.variants = std::move(variants);
  decl.span = EmptySpan();
  decl.doc = {};
  return decl;
}

ScopeContext MakeContext(const ModulePath& module = ModulePath{"test"}) {
  ScopeContext ctx;
  ctx.current_module = module;
  return ctx;
}

void AddRecordType(ScopeContext& ctx,
                   const TypePath& path,
                   const RecordDecl& record) {
  ctx.sigma.types.emplace(PathKeyOf(path), record);
}

void AddModalType(ScopeContext& ctx,
                  const TypePath& path,
                  const ModalDecl& decl) {
  ctx.sigma.types.emplace(PathKeyOf(path), decl);
}

void AddEnumType(ScopeContext& ctx,
                 const TypePath& path,
                 const cursive0::syntax::EnumDecl& decl) {
  ctx.sigma.types.emplace(PathKeyOf(path), decl);
}

void AddClassDecl(ScopeContext& ctx,
                  const TypePath& path,
                  const ClassDecl& decl) {
  ctx.sigma.classes.emplace(PathKeyOf(path), decl);
}

ASTModule MakeModuleWithProcedure(const ModulePath& path, const char* name) {
  ProcedureDecl decl;
  decl.vis = Visibility::Public;
  decl.name = name;
  decl.params = {};
  decl.return_type_opt = MakeTypePrimAst("i32");
  auto body = std::make_shared<Block>();
  body->stmts = {};
  body->tail_opt = nullptr;
  body->span = EmptySpan();
  decl.body = body;
  decl.span = EmptySpan();
  decl.doc = {};

  ASTModule module;
  module.path = path;
  module.items = {decl};
  module.module_doc = {};
  return module;
}

ASTModule MakeModuleWithStatic(const ModulePath& path,
                               const char* name,
                               std::shared_ptr<cursive0::syntax::Type> type,
                               ExprPtr init) {
  Binding binding;
  auto pat = std::make_shared<Pattern>();
  pat->node = IdentifierPattern{std::string(name)};
  binding.pat = std::move(pat);
  binding.type_opt = std::move(type);
  Token op;
  op.kind = TokenKind::Operator;
  op.lexeme = "=";
  op.span = EmptySpan();
  binding.op = op;
  binding.init = std::move(init);
  binding.span = EmptySpan();

  StaticDecl decl;
  decl.vis = Visibility::Public;
  decl.mut = Mutability::Let;
  decl.binding = std::move(binding);
  decl.span = EmptySpan();
  decl.doc = {};

  ASTModule module;
  module.path = path;
  module.items = {decl};
  module.module_doc = {};
  return module;
}

}  // namespace

int main() {
  ScopeContext ctx;
  StmtTypeContext type_ctx;

  const std::string unsafe_text = "unsafe { p }";
  const auto unsafe_path = WriteTempSource(unsafe_text);
  const auto unsafe_span = SpanForMarker(unsafe_path, unsafe_text, "p");

  ctx.current_module = {"test"};

  const std::vector<std::uint8_t> unsafe_bytes(unsafe_text.begin(),
                                               unsafe_text.end());
  const auto unsafe_loaded =
      cursive0::core::LoadSource(unsafe_path, unsafe_bytes);
  assert(unsafe_loaded.source.has_value());
  const auto unsafe_tok = cursive0::syntax::Tokenize(*unsafe_loaded.source);
  assert(unsafe_tok.output.has_value());
  const auto unsafe_filtered =
      cursive0::syntax::FilterNewlines(unsafe_tok.output->tokens);
  const auto unsafe_spans =
      cursive0::syntax::UnsafeSpans(unsafe_filtered);

  cursive0::syntax::ASTModule unsafe_module;
  unsafe_module.path = {"test"};
  unsafe_module.items = {};
  unsafe_module.module_doc = {};
  cursive0::syntax::UnsafeSpanSet unsafe_set;
  unsafe_set.path = unsafe_path;
  unsafe_set.spans = unsafe_spans;
  unsafe_module.unsafe_spans = {std::move(unsafe_set)};
  ctx.sigma.mods.push_back(std::move(unsafe_module));

  // ========== Helper predicate tests ==========

  {
    SPEC_COV("StripPerm");
    auto perm_type = MakeTypePerm(Permission::Const, Prim("i32"));
    auto stripped = StripPerm(perm_type);
    const auto* prim = std::get_if<TypePrim>(&stripped->node);
    assert(prim && prim->name == "i32");
  }

  {
    SPEC_COV("BitcopyType");
    assert(BitcopyType(ctx, Prim("i32")));
    assert(BitcopyType(ctx, Prim("bool")));
    assert(BitcopyType(ctx, Prim("char")));
    assert(BitcopyType(ctx, MakeTypePtr(Prim("i32"), PtrState::Valid)));
    // Records are not bitcopy by default
    assert(!BitcopyType(ctx, MakeTypePath({"test", "MyRecord"})));

    auto local_ctx = MakeContext();
    auto record = MakeRecordDecl(Visibility::Public, "MyRecord", {});
    record.implements = {{"Bitcopy"}};
    AddRecordType(local_ctx, {"test", "MyRecord"}, record);
    assert(BitcopyType(local_ctx, MakeTypePath({"test", "MyRecord"})));
    assert(!BitcopyType(local_ctx,
                        MakeTypePerm(Permission::Unique, Prim("i32"))));
  }

  {
    SPEC_COV("EqType");
    assert(EqType(Prim("i32")));
    assert(EqType(Prim("bool")));
    assert(EqType(Prim("char")));
    assert(EqType(MakeTypePtr(Prim("i32"), PtrState::Valid)));
    // Tuples are not eq type
    assert(!EqType(MakeTypeTuple({Prim("i32"), Prim("bool")})));
  }

  {
    SPEC_COV("OrdType");
    assert(OrdType(Prim("i32")));
    assert(OrdType(Prim("u64")));
    assert(OrdType(Prim("f32")));
    assert(OrdType(Prim("char")));
    // bool is not orderable
    assert(!OrdType(Prim("bool")));
  }

  {
    SPEC_COV("CastValid");
    // Numeric to numeric
    assert(CastValid(Prim("i32"), Prim("u64")));
    assert(CastValid(Prim("f32"), Prim("f64")));
    // Bool to int
    assert(CastValid(Prim("bool"), Prim("u8")));
    // Int to bool
    assert(CastValid(Prim("i32"), Prim("bool")));
    // Char to u32
    assert(CastValid(Prim("char"), Prim("u32")));
    // Invalid casts
    assert(!CastValid(Prim("bool"), Prim("f32")));
  }

  {
    SPEC_COV("IsPlace");
    auto ident_expr = MakeIdent("x");
    assert(IsPlaceExpr(ident_expr));

    auto binary_expr = MakeExpr(BinaryExpr{"+", MakeIdent("x"), MakeIdent("y")});
    assert(!IsPlaceExpr(binary_expr));
  }

  // ========== Lift + place checks ==========

  {
    SPEC_COV("Lift-Expr");
    auto env = MakeEnvWithBinding("x", Prim("i32"));
    auto res = TypeExpr(ctx, type_ctx, MakeIdent("x"), env);
    assert(res.ok);
  }

  {
    SPEC_COV("Place-Check");
    auto env = MakeEnvWithBinding("x", Prim("i32"));
    auto res = CheckPlaceAgainst(ctx, type_ctx, MakeIdent("x"), Prim("i32"), env);
    assert(res.ok);
  }

  // ========== Identifier typing tests ==========

  {
    SPEC_COV("T-Ident");
    auto env = MakeEnvWithBinding("x", Prim("i32"));
    auto res = TypeIdentifierExpr(ctx, IdentifierExpr{"x"}, env);
    assert(res.ok);
    const auto* prim = std::get_if<TypePrim>(&res.type->node);
    assert(prim && prim->name == "i32");
  }

  {
    SPEC_COV("ValueUse-NonBitcopyPlace");
    auto env = MakeEnvWithBinding("r", MakeTypePath({"mod", "Rec"}));
    auto res = TypeIdentifierExpr(ctx, IdentifierExpr{"r"}, env);
    assert(!res.ok);
    assert(res.diag_id ==
           std::optional<std::string_view>("ValueUse-NonBitcopyPlace"));
  }

  {
    SPEC_COV("P-Ident");
    auto env = MakeEnvWithBinding("x", Prim("i32"));
    auto expr = MakeIdent("x");
    auto res = TypePlace(ctx, type_ctx, expr, env);
    assert(res.ok);
    const auto* prim = std::get_if<TypePrim>(&res.type->node);
    assert(prim && prim->name == "i32");
  }

  // ========== Path typing tests ==========

  {
    SPEC_COV("T-Path-Value");
    ScopeContext path_ctx = MakeContext({"mod"});
    path_ctx.sigma.mods.push_back(MakeModuleWithProcedure({"mod"}, "foo"));
    PathExpr expr{{"mod"}, "foo"};
    auto res = TypePathExpr(path_ctx, expr);
    assert(res.ok);
    assert(std::holds_alternative<cursive0::analysis::TypeFunc>(res.type->node));
  }

  // ========== Field access tests ==========

  {
    ScopeContext rec_ctx = MakeContext({"mod"});
    auto record = MakeRecordDecl(
        Visibility::Public,
        "Rec",
        {MakeFieldDecl(Visibility::Public, "x", MakeTypePrimAst("i32"))});
    AddRecordType(rec_ctx, {"mod", "Rec"}, record);

    {
      SPEC_COV("T-Field-Record");
      auto env = MakeEnvWithBinding("r", MakeTypePath({"mod", "Rec"}));
      FieldAccessExpr expr{MakeExpr(MoveExpr{MakeIdent("r")}), "x"};
      auto res = TypeFieldAccessExpr(rec_ctx, type_ctx, expr, env);
      assert(res.ok);
      const auto* prim = std::get_if<TypePrim>(&res.type->node);
      assert(prim && prim->name == "i32");
    }

    {
      SPEC_COV("T-Field-Record-Perm");
      auto env = MakeEnvWithBinding(
          "rp", MakeTypePerm(Permission::Const, MakeTypePath({"mod", "Rec"})));
      FieldAccessExpr expr{MakeExpr(MoveExpr{MakeIdent("rp")}), "x"};
      auto res = TypeFieldAccessExpr(rec_ctx, type_ctx, expr, env);
      assert(res.ok);
      const auto* perm = std::get_if<TypePerm>(&res.type->node);
      assert(perm && perm->perm == Permission::Const);
    }

    {
      SPEC_COV("P-Field-Record");
      auto env = MakeEnvWithBinding("r", MakeTypePath({"mod", "Rec"}));
      FieldAccessExpr expr{MakeIdent("r"), "x"};
      auto res = TypeFieldAccessPlace(rec_ctx, type_ctx, expr, env);
      assert(res.ok);
      const auto* prim = std::get_if<TypePrim>(&res.type->node);
      assert(prim && prim->name == "i32");
    }

    {
      SPEC_COV("P-Field-Record-Perm");
      auto env = MakeEnvWithBinding(
          "rp", MakeTypePerm(Permission::Const, MakeTypePath({"mod", "Rec"})));
      FieldAccessExpr expr{MakeIdent("rp"), "x"};
      auto res = TypeFieldAccessPlace(rec_ctx, type_ctx, expr, env);
      assert(res.ok);
      const auto* perm = std::get_if<TypePerm>(&res.type->node);
      assert(perm && perm->perm == Permission::Const);
    }

    {
      SPEC_COV("FieldAccess-Unknown");
      auto env = MakeEnvWithBinding("r", MakeTypePath({"mod", "Rec"}));
      FieldAccessExpr expr{MakeExpr(MoveExpr{MakeIdent("r")}), "y"};
      auto res = TypeFieldAccessExpr(rec_ctx, type_ctx, expr, env);
      assert(!res.ok);
      assert(res.diag_id ==
             std::optional<std::string_view>("FieldAccess-Unknown"));
    }
  }

  {
    SPEC_COV("FieldAccess-NotVisible");
    ScopeContext rec_ctx = MakeContext({"other"});
    auto record = MakeRecordDecl(
        Visibility::Public,
        "RecPrivate",
        {MakeFieldDecl(Visibility::Private, "x", MakeTypePrimAst("i32"))});
    AddRecordType(rec_ctx, {"mod", "RecPrivate"}, record);
    auto env = MakeEnvWithBinding("r", MakeTypePath({"mod", "RecPrivate"}));
    FieldAccessExpr expr{MakeExpr(MoveExpr{MakeIdent("r")}), "x"};
    auto res = TypeFieldAccessExpr(rec_ctx, type_ctx, expr, env);
    assert(!res.ok);
    assert(res.diag_id ==
           std::optional<std::string_view>("FieldAccess-NotVisible"));
  }

  {
    SPEC_COV("FieldAccess-Enum");
    ScopeContext enum_ctx = MakeContext({"mod"});
    auto enum_decl = MakeEnumDecl(Visibility::Public, "E", {MakeVariantUnit("V")});
    AddEnumType(enum_ctx, {"mod", "E"}, enum_decl);
    auto env = MakeEnvWithBinding("e", MakeTypePath({"mod", "E"}));
    FieldAccessExpr expr{MakeExpr(MoveExpr{MakeIdent("e")}), "x"};
    auto res = TypeFieldAccessExpr(enum_ctx, type_ctx, expr, env);
    assert(!res.ok);
    assert(res.diag_id ==
           std::optional<std::string_view>("FieldAccess-Unknown"));
  }

  // ========== Method call tests ==========

  {
    SPEC_COV("T-MethodCall");
    ScopeContext rec_ctx = MakeContext({"mod"});
    auto record = MakeRecordDecl(Visibility::Public, "Rec", {});
    record.members.emplace_back(MakeMethodDecl(
        Visibility::Public, "foo", ReceiverShorthand{ReceiverPerm::Const},
        MakeTypePrimAst("i32")));
    AddRecordType(rec_ctx, {"mod", "Rec"}, record);

    auto env = MakeEnvWithBinding("r", MakeTypePath({"mod", "Rec"}));
    MethodCallExpr expr{MakeIdent("r"), "foo", {}};
    auto res = TypeExpr(rec_ctx, type_ctx, MakeExpr(expr), env);
    assert(res.ok);
    const auto* prim = std::get_if<TypePrim>(&res.type->node);
    assert(prim && prim->name == "i32");
  }

  {
    SPEC_COV("LookupMethod-NotFound");
    ScopeContext rec_ctx = MakeContext({"mod"});
    auto record = MakeRecordDecl(Visibility::Public, "Rec", {});
    AddRecordType(rec_ctx, {"mod", "Rec"}, record);

    auto env = MakeEnvWithBinding("r", MakeTypePath({"mod", "Rec"}));
    MethodCallExpr expr{MakeIdent("r"), "foo", {}};
    auto res = TypeExpr(rec_ctx, type_ctx, MakeExpr(expr), env);
    assert(!res.ok);
    assert(res.diag_id ==
           std::optional<std::string_view>("LookupMethod-NotFound"));
  }

  {
    SPEC_COV("LookupMethod-Ambig");
    ScopeContext rec_ctx = MakeContext({"mod"});
    auto record = MakeRecordDecl(Visibility::Public, "Rec", {});
    record.implements = {TypePath{"mod", "ClA"}, TypePath{"mod", "ClB"}};
    AddRecordType(rec_ctx, {"mod", "Rec"}, record);

    auto cls_a = MakeClassDecl(
        Visibility::Public,
        "ClA",
        {MakeClassMethodDecl(Visibility::Public,
                             "foo",
                             ReceiverShorthand{ReceiverPerm::Const},
                             MakeTypePrimAst("i32"),
                             true)});
    auto cls_b = MakeClassDecl(
        Visibility::Public,
        "ClB",
        {MakeClassMethodDecl(Visibility::Public,
                             "foo",
                             ReceiverShorthand{ReceiverPerm::Const},
                             MakeTypePrimAst("i32"),
                             true)});
    AddClassDecl(rec_ctx, {"mod", "ClA"}, cls_a);
    AddClassDecl(rec_ctx, {"mod", "ClB"}, cls_b);

    auto env = MakeEnvWithBinding("r", MakeTypePath({"mod", "Rec"}));
    MethodCallExpr expr{MakeIdent("r"), "foo", {}};
    auto res = TypeExpr(rec_ctx, type_ctx, MakeExpr(expr), env);
    assert(!res.ok);
    assert(res.diag_id ==
           std::optional<std::string_view>("LookupMethod-Ambig"));
  }

  {
    SPEC_COV("MethodCall-RecvPerm-Err");
    ScopeContext rec_ctx = MakeContext({"mod"});
    auto record = MakeRecordDecl(Visibility::Public, "Rec", {});
    record.members.emplace_back(MakeMethodDecl(
        Visibility::Public, "foo", ReceiverShorthand{ReceiverPerm::Unique},
        MakeTypePrimAst("i32")));
    AddRecordType(rec_ctx, {"mod", "Rec"}, record);

    auto env = MakeEnvWithBinding(
        "r", MakeTypePerm(Permission::Const, MakeTypePath({"mod", "Rec"})));
    MethodCallExpr expr{MakeIdent("r"), "foo", {}};
    auto res = TypeExpr(rec_ctx, type_ctx, MakeExpr(expr), env);
    assert(!res.ok);
    assert(res.diag_id ==
           std::optional<std::string_view>("MethodCall-RecvPerm-Err"));
  }

  {
    SPEC_COV("T-Dynamic-MethodCall");
    ScopeContext dyn_ctx = MakeContext({"mod"});
    auto cls = MakeClassDecl(
        Visibility::Public,
        "DynCl",
        {MakeClassMethodDecl(Visibility::Public,
                             "foo",
                             ReceiverShorthand{ReceiverPerm::Const},
                             MakeTypePrimAst("i32"),
                             true)});
    AddClassDecl(dyn_ctx, {"mod", "DynCl"}, cls);

    auto env = MakeEnvWithBinding("d", MakeTypeDynamic({"mod", "DynCl"}));
    MethodCallExpr expr{MakeIdent("d"), "foo", {}};
    auto res = TypeExpr(dyn_ctx, type_ctx, MakeExpr(expr), env);
    assert(res.ok);
    const auto* prim = std::get_if<TypePrim>(&res.type->node);
    assert(prim && prim->name == "i32");
  }

  // ========== Unary operator tests ==========

  {
    SPEC_COV("T-Not-Bool");
    auto env = MakeEnvWithBinding("b", Prim("bool"));
    cursive0::syntax::UnaryExpr expr{"!", MakeIdent("b")};
    auto res = TypeUnaryExpr(ctx, type_ctx, expr, env);
    assert(res.ok);
    const auto* prim = std::get_if<TypePrim>(&res.type->node);
    assert(prim && prim->name == "bool");
  }

  {
    SPEC_COV("T-Not-Int");
    auto env = MakeEnvWithBinding("n", Prim("u32"));
    cursive0::syntax::UnaryExpr expr{"!", MakeIdent("n")};
    auto res = TypeUnaryExpr(ctx, type_ctx, expr, env);
    assert(res.ok);
    const auto* prim = std::get_if<TypePrim>(&res.type->node);
    assert(prim && prim->name == "u32");
  }

  {
    SPEC_COV("T-Neg");
    auto env = MakeEnvWithBinding("n", Prim("i32"));
    cursive0::syntax::UnaryExpr expr{"-", MakeIdent("n")};
    auto res = TypeUnaryExpr(ctx, type_ctx, expr, env);
    assert(res.ok);
    const auto* prim = std::get_if<TypePrim>(&res.type->node);
    assert(prim && prim->name == "i32");
  }

  // ========== Binary operator tests ==========

  {
    SPEC_COV("T-Arith");
    auto env = MakeEnvWithBinding("x", Prim("i32"));
    env.scopes.back().emplace(
        "y", TypeBinding{cursive0::syntax::Mutability::Let, Prim("i32")});
    BinaryExpr expr{"+", MakeIdent("x"), MakeIdent("y")};
    auto res = TypeBinaryExpr(ctx, type_ctx, expr, env);
    assert(res.ok);
    const auto* prim = std::get_if<TypePrim>(&res.type->node);
    assert(prim && prim->name == "i32");
  }

  {
    SPEC_COV("T-Bitwise");
    auto env = MakeEnvWithBinding("x", Prim("u32"));
    env.scopes.back().emplace(
        "y", TypeBinding{cursive0::syntax::Mutability::Let, Prim("u32")});
    BinaryExpr expr{"&", MakeIdent("x"), MakeIdent("y")};
    auto res = TypeBinaryExpr(ctx, type_ctx, expr, env);
    assert(res.ok);
    const auto* prim = std::get_if<TypePrim>(&res.type->node);
    assert(prim && prim->name == "u32");
  }

  {
    SPEC_COV("T-Shift");
    auto env = MakeEnvWithBinding("x", Prim("i64"));
    env.scopes.back().emplace(
        "y", TypeBinding{cursive0::syntax::Mutability::Let, Prim("u32")});
    BinaryExpr expr{"<<", MakeIdent("x"), MakeIdent("y")};
    auto res = TypeBinaryExpr(ctx, type_ctx, expr, env);
    assert(res.ok);
    const auto* prim = std::get_if<TypePrim>(&res.type->node);
    assert(prim && prim->name == "i64");
  }

  {
    SPEC_COV("T-Compare-Eq");
    auto env = MakeEnvWithBinding("x", Prim("i32"));
    env.scopes.back().emplace(
        "y", TypeBinding{cursive0::syntax::Mutability::Let, Prim("i32")});
    BinaryExpr expr{"==", MakeIdent("x"), MakeIdent("y")};
    auto res = TypeBinaryExpr(ctx, type_ctx, expr, env);
    assert(res.ok);
    const auto* prim = std::get_if<TypePrim>(&res.type->node);
    assert(prim && prim->name == "bool");
  }

  {
    auto env = MakeEnvWithBinding("x", Prim("i32"));
    env.scopes.back().emplace(
        "y", TypeBinding{cursive0::syntax::Mutability::Let, Prim("u32")});
    BinaryExpr expr{"==", MakeIdent("x"), MakeIdent("y")};
    auto res = TypeBinaryExpr(ctx, type_ctx, expr, env);
    assert(!res.ok);
  }

  {
    SPEC_COV("T-Compare-Ord");
    auto env = MakeEnvWithBinding("x", Prim("i32"));
    env.scopes.back().emplace(
        "y", TypeBinding{cursive0::syntax::Mutability::Let, Prim("i32")});
    BinaryExpr expr{"<", MakeIdent("x"), MakeIdent("y")};
    auto res = TypeBinaryExpr(ctx, type_ctx, expr, env);
    assert(res.ok);
    const auto* prim = std::get_if<TypePrim>(&res.type->node);
    assert(prim && prim->name == "bool");
  }

  {
    auto env = MakeEnvWithBinding("x", Prim("i32"));
    env.scopes.back().emplace(
        "y", TypeBinding{cursive0::syntax::Mutability::Let, Prim("u32")});
    BinaryExpr expr{"<", MakeIdent("x"), MakeIdent("y")};
    auto res = TypeBinaryExpr(ctx, type_ctx, expr, env);
    assert(!res.ok);
  }

  {
    SPEC_COV("T-Logical");
    auto env = MakeEnvWithBinding("a", Prim("bool"));
    env.scopes.back().emplace(
        "b", TypeBinding{cursive0::syntax::Mutability::Let, Prim("bool")});
    BinaryExpr expr{"&&", MakeIdent("a"), MakeIdent("b")};
    auto res = TypeBinaryExpr(ctx, type_ctx, expr, env);
    assert(res.ok);
    const auto* prim = std::get_if<TypePrim>(&res.type->node);
    assert(prim && prim->name == "bool");
  }

  // ========== Cast tests ==========

  {
    SPEC_COV("T-Cast");
    auto env = MakeEnvWithBinding("x", Prim("i32"));
    auto target_type = MakeTypePrimAst("u64");
    CastExpr expr{MakeIdent("x"), target_type};
    auto res = TypeCastExpr(ctx, type_ctx, expr, env);
    assert(res.ok);
    const auto* prim = std::get_if<TypePrim>(&res.type->node);
    assert(prim && prim->name == "u64");
  }

  // ========== If expression tests ==========

  {
    SPEC_COV("T-If");
    auto env = MakeEnvWithBinding("cond", Prim("bool"));
    env.scopes.back().emplace(
        "x", TypeBinding{cursive0::syntax::Mutability::Let, Prim("i32")});
    env.scopes.back().emplace(
        "y", TypeBinding{cursive0::syntax::Mutability::Let, Prim("i32")});
    IfExpr expr{MakeIdent("cond"), MakeIdent("x"), MakeIdent("y")};
    auto res = TypeIfExpr(ctx, type_ctx, expr, env);
    assert(res.ok);
    const auto* prim = std::get_if<TypePrim>(&res.type->node);
    assert(prim && prim->name == "i32");
  }

  {
    SPEC_COV("T-If-No-Else");
    auto env = MakeEnvWithBinding("cond", Prim("bool"));
    auto then_expr = MakeExpr(TupleExpr{});
    IfExpr expr{MakeIdent("cond"), then_expr, nullptr};
    auto res = TypeIfExpr(ctx, type_ctx, expr, env);
    assert(res.ok);
    const auto* prim = std::get_if<TypePrim>(&res.type->node);
    assert(prim && prim->name == "()");
  }

  {
    SPEC_COV("Chk-If");
    auto env = MakeEnvWithBinding("cond", Prim("bool"));
    env.scopes.back().emplace(
        "x", TypeBinding{cursive0::syntax::Mutability::Let, Prim("i32")});
    env.scopes.back().emplace(
        "y", TypeBinding{cursive0::syntax::Mutability::Let, Prim("i32")});
    IfExpr expr{MakeIdent("cond"), MakeIdent("x"), MakeIdent("y")};
    auto res = CheckIfExpr(ctx, type_ctx, expr, Prim("i32"), env);
    assert(res.ok);
  }

  {
    SPEC_COV("Chk-If-No-Else");
    auto env = MakeEnvWithBinding("cond", Prim("bool"));
    auto then_expr = MakeExpr(TupleExpr{});
    IfExpr expr{MakeIdent("cond"), then_expr, nullptr};
    auto res = CheckIfExpr(ctx, type_ctx, expr, Prim("()"), env);
    assert(res.ok);
  }

  // ========== Range expression tests ==========

  {
    SPEC_COV("Range-Full");
    TypeEnv env;
    env.scopes.emplace_back();
    RangeExpr expr{RangeKind::Full, nullptr, nullptr};
    auto res = TypeRangeExpr(ctx, type_ctx, expr, env);
    assert(res.ok);
    assert(std::holds_alternative<TypeRange>(res.type->node));
  }

  {
    SPEC_COV("Range-To");
    auto env = MakeEnvWithBinding("end", Prim("usize"));
    RangeExpr expr{RangeKind::To, nullptr, MakeIdent("end")};
    auto res = TypeRangeExpr(ctx, type_ctx, expr, env);
    assert(res.ok);
  }

  {
    SPEC_COV("Range-ToInclusive");
    auto env = MakeEnvWithBinding("end", Prim("usize"));
    RangeExpr expr{RangeKind::ToInclusive, nullptr, MakeIdent("end")};
    auto res = TypeRangeExpr(ctx, type_ctx, expr, env);
    assert(res.ok);
  }

  {
    SPEC_COV("Range-From");
    auto env = MakeEnvWithBinding("start", Prim("usize"));
    RangeExpr expr{RangeKind::From, MakeIdent("start"), nullptr};
    auto res = TypeRangeExpr(ctx, type_ctx, expr, env);
    assert(res.ok);
  }

  {
    SPEC_COV("Range-Exclusive");
    auto env = MakeEnvWithBinding("start", Prim("usize"));
    env.scopes.back().emplace(
        "end", TypeBinding{cursive0::syntax::Mutability::Let, Prim("usize")});
    RangeExpr expr{RangeKind::Exclusive, MakeIdent("start"), MakeIdent("end")};
    auto res = TypeRangeExpr(ctx, type_ctx, expr, env);
    assert(res.ok);
  }

  {
    SPEC_COV("Range-Inclusive");
    auto env = MakeEnvWithBinding("start", Prim("usize"));
    env.scopes.back().emplace(
        "end", TypeBinding{cursive0::syntax::Mutability::Let, Prim("usize")});
    RangeExpr expr{RangeKind::Inclusive, MakeIdent("start"), MakeIdent("end")};
    auto res = TypeRangeExpr(ctx, type_ctx, expr, env);
    assert(res.ok);
  }

  {
    SPEC_COV("Range-NonIndex-Err");
    auto env = MakeEnvWithBinding("start", Prim("i32"));  // Not usize
    RangeExpr expr{RangeKind::From, MakeIdent("start"), nullptr};
    auto res = TypeRangeExpr(ctx, type_ctx, expr, env);
    assert(!res.ok);
    assert(res.diag_id == std::optional<std::string_view>("Range-NonIndex-Err"));
  }

  // ========== Address-of tests ==========

  {
    SPEC_COV("AddrOf-NonPlace");
    auto env = MakeEnvWithBinding("x", Prim("i32"));
    auto non_place = MakeExpr(BinaryExpr{"+", MakeIdent("x"), MakeIdent("x")});
    AddressOfExpr expr{non_place};
    auto res = TypeAddressOfExpr(ctx, type_ctx, expr, env);
    assert(!res.ok);
    assert(res.diag_id == std::optional<std::string_view>("AddrOf-NonPlace"));
  }

  {
    SPEC_COV("AddrOf-Index-Array-NonUsize");
    auto env = MakeEnvWithBinding("arr", MakeTypeArray(Prim("u8"), 4));
    env.scopes.back().emplace(
        "idx", TypeBinding{cursive0::syntax::Mutability::Let, Prim("i32")});
    auto index = MakeExpr(IndexAccessExpr{MakeIdent("arr"), MakeIdent("idx")});
    AddressOfExpr expr{index};
    auto res = TypeAddressOfExpr(ctx, type_ctx, expr, env);
    assert(!res.ok);
    assert(res.diag_id ==
           std::optional<std::string_view>("Index-Array-NonUsize"));
  }

  {
    SPEC_COV("AddrOf-Index-Slice-NonUsize");
    auto env = MakeEnvWithBinding("slice", MakeTypeSlice(Prim("u8")));
    env.scopes.back().emplace(
        "idx", TypeBinding{cursive0::syntax::Mutability::Let, Prim("i32")});
    auto index =
        MakeExpr(IndexAccessExpr{MakeIdent("slice"), MakeIdent("idx")});
    AddressOfExpr expr{index};
    auto res = TypeAddressOfExpr(ctx, type_ctx, expr, env);
    assert(!res.ok);
    assert(res.diag_id ==
           std::optional<std::string_view>("Index-Slice-NonUsize"));
  }

  {
    ScopeContext idx_ctx = MakeContext({"mod"});
    idx_ctx.scopes.emplace_back();
    idx_ctx.scopes.back().emplace(
        IdKeyOf("idx"),
        Entity{EntityKind::Value, ModulePath{"mod"}, std::nullopt,
               EntitySource::Decl});
    idx_ctx.sigma.mods.push_back(
        MakeModuleWithStatic(ModulePath{"mod"}, "idx",
                             MakeTypePrimAst("usize"), MakeIntLiteral(1)));

    auto env = MakeEnvWithBinding("arr", MakeTypeArray(Prim("u8"), 4));
    env.scopes.back().emplace(
        "idx",
        TypeBinding{cursive0::syntax::Mutability::Let,
                    MakeTypePerm(Permission::Const, Prim("usize"))});
    auto index = MakeExpr(IndexAccessExpr{MakeIdent("arr"), MakeIdent("idx")});
    AddressOfExpr expr{index};
    auto res = TypeAddressOfExpr(idx_ctx, type_ctx, expr, env);
    assert(res.ok);
  }

  {
    SPEC_COV("T-AddrOf");
    auto env = MakeEnvWithBinding("x", Prim("i32"));
    AddressOfExpr expr{MakeIdent("x")};
    auto res = TypeAddressOfExpr(ctx, type_ctx, expr, env);
    assert(res.ok);
    const auto* ptr = std::get_if<cursive0::analysis::TypePtr>(&res.type->node);
    assert(ptr);
    const auto* prim = std::get_if<TypePrim>(&ptr->element->node);
    assert(prim && prim->name == "i32");
  }

  // ========== Dereference tests ==========

  {
    SPEC_COV("T-Deref-Ptr");
    auto env = MakeEnvWithBinding("p", MakeTypePtr(Prim("i32"), PtrState::Valid));
    DerefExpr expr{MakeIdent("p")};
    auto res = TypeDerefExpr(ctx, type_ctx, expr, env);
    assert(res.ok);
    const auto* prim = std::get_if<TypePrim>(&res.type->node);
    assert(prim && prim->name == "i32");
  }

  {
    SPEC_COV("Deref-Null");
    auto env = MakeEnvWithBinding("p", MakeTypePtr(Prim("i32"), PtrState::Null));
    DerefExpr expr{MakeIdent("p")};
    auto res = TypeDerefExpr(ctx, type_ctx, expr, env);
    assert(!res.ok);
    assert(res.diag_id == std::optional<std::string_view>("Deref-Null"));
  }

  {
    SPEC_COV("Deref-Expired");
    auto env =
        MakeEnvWithBinding("p", MakeTypePtr(Prim("i32"), PtrState::Expired));
    DerefExpr expr{MakeIdent("p")};
    auto res = TypeDerefExpr(ctx, type_ctx, expr, env);
    assert(!res.ok);
    assert(res.diag_id == std::optional<std::string_view>("Deref-Expired"));
  }

  {
    SPEC_COV("Deref-Raw-Unsafe");
    auto env =
        MakeEnvWithBinding("p", MakeTypeRawPtr(RawPtrQual::Imm, Prim("i32")));
    DerefExpr expr{MakeIdent("p")};
    auto res = TypeDerefExpr(ctx, type_ctx, expr, env);
    assert(!res.ok);
    assert(res.diag_id == std::optional<std::string_view>("Deref-Raw-Unsafe"));
  }

  {
    SPEC_COV("T-Deref-Raw");
    auto env =
        MakeEnvWithBinding("p", MakeTypeRawPtr(RawPtrQual::Imm, Prim("i32")));
    auto expr = MakeExprWithSpan(DerefExpr{MakeIdent("p")}, unsafe_span);
    auto res = TypeExpr(ctx, type_ctx, expr, env);
    assert(res.ok);
    const auto* prim = std::get_if<TypePrim>(&res.type->node);
    assert(prim && prim->name == "i32");
  }

  {
    SPEC_COV("P-Deref-Ptr");
    auto env = MakeEnvWithBinding("p", MakeTypePtr(Prim("i32"), PtrState::Valid));
    DerefExpr expr{MakeIdent("p")};
    auto res = TypeDerefPlace(ctx, type_ctx, expr, env);
    assert(res.ok);
    const auto* prim = std::get_if<TypePrim>(&res.type->node);
    assert(prim && prim->name == "i32");
  }

  {
    SPEC_COV("P-Deref-Raw-Imm");
    auto env =
        MakeEnvWithBinding("p", MakeTypeRawPtr(RawPtrQual::Imm, Prim("i32")));
    auto expr = MakeExprWithSpan(DerefExpr{MakeIdent("p")}, unsafe_span);
    auto res = TypePlace(ctx, type_ctx, expr, env);
    assert(res.ok);
    const auto* perm = std::get_if<TypePerm>(&res.type->node);
    assert(perm && perm->perm == Permission::Const);
  }

  {
    SPEC_COV("P-Deref-Raw-Mut");
    auto env =
        MakeEnvWithBinding("p", MakeTypeRawPtr(RawPtrQual::Mut, Prim("i32")));
    auto expr = MakeExprWithSpan(DerefExpr{MakeIdent("p")}, unsafe_span);
    auto res = TypePlace(ctx, type_ctx, expr, env);
    assert(res.ok);
    const auto* perm = std::get_if<TypePerm>(&res.type->node);
    assert(perm && perm->perm == Permission::Unique);
  }

  // ========== Move tests ==========

  {
    SPEC_COV("T-Move");
    auto env = MakeEnvWithBinding("r", MakeTypePath({"mod", "Rec"}));
    MoveExpr expr{MakeIdent("r")};
    auto res = TypeMoveExpr(ctx, type_ctx, expr, env);
    assert(res.ok);
    assert(std::holds_alternative<TypePathType>(res.type->node));
  }

  // ========== Alloc tests ==========

  {
    SPEC_COV("T-Alloc-Explicit");
    auto env = MakeEnvWithBinding(
        "r", MakeTypeModalState({"Region"}, "Active"));
    env.scopes.back().emplace(
        "x", TypeBinding{cursive0::syntax::Mutability::Let, Prim("i32")});
    AllocExpr expr{std::optional<std::string>("r"), MakeIdent("x")};
    auto res = TypeAllocExpr(ctx, type_ctx, expr, env);
    assert(res.ok);
    const auto* prim = std::get_if<TypePrim>(&res.type->node);
    assert(prim && prim->name == "i32");
  }

  {
    SPEC_COV("T-Alloc-Implicit");
    auto env = MakeEnvWithBinding(
        "r", MakeTypeModalState({"Region"}, "Active"));
    env.scopes.back().emplace(
        "x", TypeBinding{cursive0::syntax::Mutability::Let, Prim("i32")});
    AllocExpr expr{std::nullopt, MakeIdent("x")};
    auto res = TypeAllocExpr(ctx, type_ctx, expr, env);
    assert(res.ok);
    const auto* prim = std::get_if<TypePrim>(&res.type->node);
    assert(prim && prim->name == "i32");
  }

  {
    auto env = MakeEnvWithBinding("x", Prim("i32"));
    AllocExpr expr{std::optional<std::string>("r"), MakeIdent("x")};
    auto res = TypeAllocExpr(ctx, type_ctx, expr, env);
    assert(!res.ok);
    assert(res.diag_id ==
           std::optional<std::string_view>("ResolveExpr-Ident-Err"));
  }

  {
    SPEC_COV("Alloc-Region-NotFound-Err");
    auto env = MakeEnvWithBinding("x", Prim("i32"));
    env.scopes.back().emplace(
        "r", TypeBinding{cursive0::syntax::Mutability::Let, Prim("i32")});
    AllocExpr expr{std::optional<std::string>("r"), MakeIdent("x")};
    auto res = TypeAllocExpr(ctx, type_ctx, expr, env);
    assert(!res.ok);
    assert(res.diag_id ==
           std::optional<std::string_view>("Alloc-Region-NotFound-Err"));
  }

  {
    SPEC_COV("Alloc-Implicit-NoRegion-Err");
    auto env = MakeEnvWithBinding("x", Prim("i32"));
    AllocExpr expr{std::nullopt, MakeIdent("x")};
    auto res = TypeAllocExpr(ctx, type_ctx, expr, env);
    assert(!res.ok);
    assert(res.diag_id ==
           std::optional<std::string_view>("Alloc-Implicit-NoRegion-Err"));
  }

  // ========== Transmute tests ==========

  {
    SPEC_COV("Transmute-Unsafe-Err");
    auto env = MakeEnvWithBinding("x", Prim("i32"));
    TransmuteExpr expr{MakeTypePrimAst("i32"), MakeTypePrimAst("u32"),
                       MakeIdent("x")};
    auto res = TypeTransmuteExpr(ctx, type_ctx, expr, env);
    assert(!res.ok);
    assert(res.diag_id ==
           std::optional<std::string_view>("Transmute-Unsafe-Err"));
  }

  {
    SPEC_COV("T-Transmute-SizeEq");
    auto env = MakeEnvWithBinding("x", Prim("i32"));
    TransmuteExpr expr{MakeTypePrimAst("i32"), MakeTypePrimAst("i64"),
                       MakeIdent("x")};
    auto node = MakeExprWithSpan(expr, unsafe_span);
    auto res = TypeExpr(ctx, type_ctx, node, env);
    assert(!res.ok);
    assert(res.diag_id ==
           std::optional<std::string_view>("T-Transmute-SizeEq"));
  }

  {
    SPEC_COV("T-Transmute-AlignEq");
    auto env = MakeEnvWithBinding(
        "t", MakeTypeTuple({Prim("u8"), Prim("u8")}));
    auto from = MakeTypeTupleAst({MakeTypePrimAst("u8"), MakeTypePrimAst("u8")});
    auto to = MakeTypePrimAst("u16");
    TransmuteExpr expr{from, to, MakeExpr(MoveExpr{MakeIdent("t")})};
    auto node = MakeExprWithSpan(expr, unsafe_span);
    auto res = TypeExpr(ctx, type_ctx, node, env);
    assert(!res.ok);
    assert(res.diag_id ==
           std::optional<std::string_view>("T-Transmute-AlignEq"));
  }

  {
    SPEC_COV("T-Transmute");
    auto env = MakeEnvWithBinding("x", Prim("i32"));
    TransmuteExpr expr{MakeTypePrimAst("i32"), MakeTypePrimAst("u32"),
                       MakeIdent("x")};
    auto node = MakeExprWithSpan(expr, unsafe_span);
    auto res = TypeExpr(ctx, type_ctx, node, env);
    assert(res.ok);
    const auto* prim = std::get_if<TypePrim>(&res.type->node);
    assert(prim && prim->name == "u32");
  }

  // ========== Propagate tests ==========

  {
    SPEC_COV("T-Propagate");
    auto env =
        MakeEnvWithBinding("u", MakeTypeUnion({Prim("i32"), Prim("bool")}));
    StmtTypeContext ret_ctx = type_ctx;
    ret_ctx.return_type = Prim("i32");
    PropagateExpr expr{MakeExpr(MoveExpr{MakeIdent("u")})};
    auto res = TypePropagateExpr(ctx, ret_ctx, expr, env);
    assert(res.ok);
    const auto* prim = std::get_if<TypePrim>(&res.type->node);
    assert(prim && prim->name == "bool");
  }

  // ========== Record literal tests ==========

  {
    ScopeContext rec_ctx = MakeContext({"mod"});
    auto record = MakeRecordDecl(
        Visibility::Public,
        "Point",
        {MakeFieldDecl(Visibility::Public, "x", MakeTypePrimAst("i32")),
         MakeFieldDecl(Visibility::Public, "y", MakeTypePrimAst("bool"))});
    AddRecordType(rec_ctx, {"mod", "Point"}, record);

    {
      SPEC_COV("T-Record-Literal");
      RecordExpr expr{TypePath{"mod", "Point"},
                      {FieldInit{"x", MakeIntLiteral(1), EmptySpan()},
                       FieldInit{"y", MakeBoolLiteral(true), EmptySpan()}}};
      TypeEnv env;
      env.scopes.emplace_back();
      auto res = TypeRecordExpr(rec_ctx, type_ctx, expr, env);
      assert(res.ok);
      const auto* path = std::get_if<TypePathType>(&res.type->node);
      assert(path && path->path == TypePath({"mod", "Point"}));
    }

    {
      SPEC_COV("Record-FieldInit-Dup");
      RecordExpr expr{TypePath{"mod", "Point"},
                      {FieldInit{"x", MakeIntLiteral(1), EmptySpan()},
                       FieldInit{"x", MakeIntLiteral(2), EmptySpan()}}};
      TypeEnv env;
      env.scopes.emplace_back();
      auto res = TypeRecordExpr(rec_ctx, type_ctx, expr, env);
      assert(!res.ok);
      assert(res.diag_id ==
             std::optional<std::string_view>("Record-FieldInit-Dup"));
    }

    {
      SPEC_COV("Record-FieldInit-Missing");
      RecordExpr expr{TypePath{"mod", "Point"},
                      {FieldInit{"x", MakeIntLiteral(1), EmptySpan()}}};
      TypeEnv env;
      env.scopes.emplace_back();
      auto res = TypeRecordExpr(rec_ctx, type_ctx, expr, env);
      assert(!res.ok);
      assert(res.diag_id ==
             std::optional<std::string_view>("Record-FieldInit-Missing"));
    }

    {
      SPEC_COV("Record-Field-Unknown");
      RecordExpr expr{TypePath{"mod", "Point"},
                      {FieldInit{"z", MakeIntLiteral(1), EmptySpan()},
                       FieldInit{"y", MakeBoolLiteral(true), EmptySpan()}}};
      TypeEnv env;
      env.scopes.emplace_back();
      auto res = TypeRecordExpr(rec_ctx, type_ctx, expr, env);
      assert(!res.ok);
      assert(res.diag_id ==
             std::optional<std::string_view>("Record-Field-Unknown"));
    }

    {
      SPEC_COV("Record-FieldInit-Missing");
      auto modal = MakeModalDecl(
          Visibility::Public,
          "Stateful",
          {MakeStateBlock(
              "Active",
              {MakeStateFieldDecl(Visibility::Public, "x", MakeTypePrimAst("i32")),
               MakeStateFieldDecl(Visibility::Public, "y", MakeTypePrimAst("bool"))})});
      AddModalType(rec_ctx, {"mod", "Stateful"}, modal);

      RecordExpr expr;
      ModalStateRef modal_ref;
      modal_ref.path = TypePath{"mod", "Stateful"};
      modal_ref.state = "Active";
      expr.target = modal_ref;
      expr.fields = {FieldInit{"x", MakeIntLiteral(1), EmptySpan()}};
      TypeEnv env;
      env.scopes.emplace_back();
      auto res = TypeRecordExpr(rec_ctx, type_ctx, expr, env);
      assert(!res.ok);
      assert(res.diag_id ==
             std::optional<std::string_view>("Record-FieldInit-Missing"));
    }
  }

  {
    SPEC_COV("Record-Field-NotVisible");
    ScopeContext rec_ctx = MakeContext({"other"});
    auto record = MakeRecordDecl(
        Visibility::Public,
        "Secret",
        {MakeFieldDecl(Visibility::Private, "x", MakeTypePrimAst("i32"))});
    AddRecordType(rec_ctx, {"mod", "Secret"}, record);
    RecordExpr expr{TypePath{"mod", "Secret"},
                    {FieldInit{"x", MakeIntLiteral(1), EmptySpan()}}};
    TypeEnv env;
    env.scopes.emplace_back();
    auto res = TypeRecordExpr(rec_ctx, type_ctx, expr, env);
    assert(!res.ok);
    assert(res.diag_id ==
           std::optional<std::string_view>("Record-Field-NotVisible"));
  }

  {
    SPEC_COV("Record-Field-NonBitcopy-Move");
    ScopeContext rec_ctx = MakeContext({"mod"});
    auto record = MakeRecordDecl(
        Visibility::Public,
        "Wrap",
        {MakeFieldDecl(Visibility::Public,
                       "inner",
                       MakeTypePathAst(TypePath{"mod", "Inner"}))});
    AddRecordType(rec_ctx, {"mod", "Wrap"}, record);
    auto env = MakeEnvWithBinding("inner", MakeTypePath({"mod", "Inner"}));
    RecordExpr expr{TypePath{"mod", "Wrap"},
                    {FieldInit{"inner", MakeIdent("inner"), EmptySpan()}}};
    auto res = TypeRecordExpr(rec_ctx, type_ctx, expr, env);
    assert(!res.ok);
    assert(res.diag_id ==
           std::optional<std::string_view>("Record-Field-NonBitcopy-Move"));
  }

  // ========== Enum literal tests ==========

  {
    ScopeContext enum_ctx = MakeContext({"mod"});
    auto enum_decl = MakeEnumDecl(
        Visibility::Public,
        "E",
        {MakeVariantUnit("Unit"),
         MakeVariantTuple("Pair",
                          {MakeTypePrimAst("i32"), MakeTypePrimAst("bool")}),
         MakeVariantRecord(
             "Rec",
             {MakeFieldDecl(Visibility::Public, "x", MakeTypePrimAst("i32")),
              MakeFieldDecl(Visibility::Public, "y", MakeTypePrimAst("bool"))})});
    AddEnumType(enum_ctx, {"mod", "E"}, enum_decl);
    TypeEnv env;
    env.scopes.emplace_back();

    {
      SPEC_COV("T-Enum-Lit-Unit");
      EnumLiteralExpr expr{TypePath{"mod", "E", "Unit"}, std::nullopt};
      auto res = TypeEnumLiteralExpr(enum_ctx, type_ctx, expr, env);
      assert(res.ok);
      const auto* path = std::get_if<TypePathType>(&res.type->node);
      assert(path && path->path == TypePath({"mod", "E"}));
    }

    {
      SPEC_COV("T-Enum-Lit-Tuple");
      EnumPayloadParen payload;
      payload.elements = {MakeIntLiteral(1), MakeBoolLiteral(true)};
      EnumLiteralExpr expr{TypePath{"mod", "E", "Pair"}, payload};
      auto res = TypeEnumLiteralExpr(enum_ctx, type_ctx, expr, env);
      assert(res.ok);
      const auto* path = std::get_if<TypePathType>(&res.type->node);
      assert(path && path->path == TypePath({"mod", "E"}));
    }

    {
      SPEC_COV("T-Enum-Lit-Record");
      EnumPayloadBrace payload;
      payload.fields = {FieldInit{"x", MakeIntLiteral(3), EmptySpan()},
                        FieldInit{"y", MakeBoolLiteral(false), EmptySpan()}};
      EnumLiteralExpr expr{TypePath{"mod", "E", "Rec"}, payload};
      auto res = TypeEnumLiteralExpr(enum_ctx, type_ctx, expr, env);
      assert(res.ok);
      const auto* path = std::get_if<TypePathType>(&res.type->node);
      assert(path && path->path == TypePath({"mod", "E"}));
    }
  }

  // ========== Error handling tests ==========

  {
    SPEC_COV("Expr-Unresolved-Err");
    TypeEnv env;
    env.scopes.emplace_back();
    auto expr =
        MakeExpr(cursive0::syntax::QualifiedNameExpr{{"test"}, "foo"});
    auto res = TypeExpr(ctx, type_ctx, expr, env);
    assert(!res.ok);
    assert(res.diag_id ==
           std::optional<std::string_view>("ResolveExpr-Ident-Err"));
  }

  {
    SPEC_COV("T-ErrorExpr");
    TypeEnv env;
    env.scopes.emplace_back();
    auto expr = MakeExpr(cursive0::syntax::ErrorExpr{});
    auto res = TypeExpr(ctx, type_ctx, expr, env);
    assert(res.ok);
    const auto* prim = std::get_if<TypePrim>(&res.type->node);
    assert(prim && prim->name == "!");
  }

  return 0;
}
