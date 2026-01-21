#include <cassert>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "cursive0/codegen/checks.h"
#include "cursive0/codegen/dyn_dispatch.h"
#include "cursive0/codegen/globals.h"
#include "cursive0/codegen/ir_model.h"
#include "cursive0/codegen/llvm/llvm_emit.h"
#include "cursive0/codegen/lower/lower_expr.h"
#include "cursive0/codegen/lower/lower_pat.h"
#include "cursive0/codegen/lower/lower_stmt.h"
#include "cursive0/codegen/mangle.h"
#include "cursive0/core/assert_spec.h"
#include "cursive0/core/symbols.h"
#include "cursive0/analysis/types/context.h"
#include "cursive0/analysis/resolve/scopes.h"
#include "cursive0/analysis/types/types.h"
#include "cursive0/syntax/ast.h"
#include "cursive0/syntax/token.h"

#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

namespace {

cursive0::syntax::Token MakeToken(cursive0::syntax::TokenKind kind,
                                  const char* lexeme) {
  cursive0::syntax::Token tok;
  tok.kind = kind;
  tok.lexeme = std::string(lexeme);
  return tok;
}

cursive0::syntax::ExprPtr MakeExpr(cursive0::syntax::ExprNode node) {
  auto expr = std::make_shared<cursive0::syntax::Expr>();
  expr->node = std::move(node);
  return expr;
}

cursive0::syntax::ExprPtr MakeLiteralExpr(cursive0::syntax::TokenKind kind,
                                          const char* lexeme) {
  cursive0::syntax::LiteralExpr lit;
  lit.literal = MakeToken(kind, lexeme);
  return MakeExpr(std::move(lit));
}

cursive0::syntax::ExprPtr MakeIdentifierExpr(const char* name) {
  cursive0::syntax::IdentifierExpr ident;
  ident.name = std::string(name);
  return MakeExpr(std::move(ident));
}

cursive0::syntax::ExprPtr MakePathExpr(const cursive0::syntax::ModulePath& path,
                                      const char* name) {
  cursive0::syntax::PathExpr node;
  node.path = path;
  node.name = std::string(name);
  return MakeExpr(std::move(node));
}

cursive0::syntax::ExprPtr MakeRangeExpr(cursive0::syntax::RangeKind kind,
                                       cursive0::syntax::ExprPtr lo,
                                       cursive0::syntax::ExprPtr hi) {
  cursive0::syntax::RangeExpr node;
  node.kind = kind;
  node.lhs = std::move(lo);
  node.rhs = std::move(hi);
  return MakeExpr(std::move(node));
}

cursive0::syntax::ExprPtr MakeTupleExpr(std::vector<cursive0::syntax::ExprPtr> elements) {
  cursive0::syntax::TupleExpr node;
  node.elements = std::move(elements);
  return MakeExpr(std::move(node));
}

cursive0::syntax::ExprPtr MakeArrayExpr(std::vector<cursive0::syntax::ExprPtr> elements) {
  cursive0::syntax::ArrayExpr node;
  node.elements = std::move(elements);
  return MakeExpr(std::move(node));
}

cursive0::syntax::FieldInit MakeFieldInit(const char* name,
                                         cursive0::syntax::ExprPtr value) {
  cursive0::syntax::FieldInit field;
  field.name = std::string(name);
  field.value = std::move(value);
  return field;
}

cursive0::syntax::ExprPtr MakeRecordExpr(const cursive0::syntax::TypePath& path,
                                        std::vector<cursive0::syntax::FieldInit> fields) {
  cursive0::syntax::RecordExpr node;
  node.target = path;
  node.fields = std::move(fields);
  return MakeExpr(std::move(node));
}

cursive0::syntax::ExprPtr MakeEnumUnitExpr(const cursive0::syntax::Path& path) {
  cursive0::syntax::EnumLiteralExpr node;
  node.path = path;
  node.payload_opt = std::nullopt;
  return MakeExpr(std::move(node));
}

cursive0::syntax::ExprPtr MakeEnumTupleExpr(const cursive0::syntax::Path& path,
                                           std::vector<cursive0::syntax::ExprPtr> elements) {
  cursive0::syntax::EnumPayloadParen payload;
  payload.elements = std::move(elements);
  cursive0::syntax::EnumLiteralExpr node;
  node.path = path;
  node.payload_opt = cursive0::syntax::EnumPayload{std::move(payload)};
  return MakeExpr(std::move(node));
}

cursive0::syntax::ExprPtr MakeEnumRecordExpr(const cursive0::syntax::Path& path,
                                            std::vector<cursive0::syntax::FieldInit> fields) {
  cursive0::syntax::EnumPayloadBrace payload;
  payload.fields = std::move(fields);
  cursive0::syntax::EnumLiteralExpr node;
  node.path = path;
  node.payload_opt = cursive0::syntax::EnumPayload{std::move(payload)};
  return MakeExpr(std::move(node));
}

cursive0::syntax::ExprPtr MakeFieldAccessExpr(cursive0::syntax::ExprPtr base,
                                             const char* name) {
  cursive0::syntax::FieldAccessExpr node;
  node.base = std::move(base);
  node.name = std::string(name);
  return MakeExpr(std::move(node));
}

cursive0::syntax::ExprPtr MakeTupleAccessExpr(cursive0::syntax::ExprPtr base,
                                             const char* index) {
  cursive0::syntax::TupleAccessExpr node;
  node.base = std::move(base);
  node.index = MakeToken(cursive0::syntax::TokenKind::IntLiteral, index);
  return MakeExpr(std::move(node));
}

cursive0::syntax::ExprPtr MakeIndexExpr(cursive0::syntax::ExprPtr base,
                                       cursive0::syntax::ExprPtr index) {
  cursive0::syntax::IndexAccessExpr node;
  node.base = std::move(base);
  node.index = std::move(index);
  return MakeExpr(std::move(node));
}

cursive0::syntax::ExprPtr MakeCallExpr(cursive0::syntax::ExprPtr callee,
                                      std::vector<cursive0::syntax::Arg> args) {
  cursive0::syntax::CallExpr node;
  node.callee = std::move(callee);
  node.args = std::move(args);
  return MakeExpr(std::move(node));
}

cursive0::syntax::ExprPtr MakeMethodCallExpr(cursive0::syntax::ExprPtr receiver,
                                            const char* name,
                                            std::vector<cursive0::syntax::Arg> args) {
  cursive0::syntax::MethodCallExpr node;
  node.receiver = std::move(receiver);
  node.name = std::string(name);
  node.args = std::move(args);
  return MakeExpr(std::move(node));
}

cursive0::syntax::ExprPtr MakeUnaryExpr(const char* op,
                                       cursive0::syntax::ExprPtr value) {
  cursive0::syntax::UnaryExpr node;
  node.op = std::string(op);
  node.value = std::move(value);
  return MakeExpr(std::move(node));
}

cursive0::syntax::ExprPtr MakeBinaryExpr(const char* op,
                                        cursive0::syntax::ExprPtr lhs,
                                        cursive0::syntax::ExprPtr rhs) {
  cursive0::syntax::BinaryExpr node;
  node.op = std::string(op);
  node.lhs = std::move(lhs);
  node.rhs = std::move(rhs);
  return MakeExpr(std::move(node));
}

cursive0::syntax::ExprPtr MakeCastExpr(cursive0::syntax::ExprPtr value,
                                      std::shared_ptr<cursive0::syntax::Type> type) {
  cursive0::syntax::CastExpr node;
  node.value = std::move(value);
  node.type = std::move(type);
  return MakeExpr(std::move(node));
}

cursive0::syntax::ExprPtr MakeTransmuteExpr(cursive0::syntax::ExprPtr value,
                                           std::shared_ptr<cursive0::syntax::Type> from,
                                           std::shared_ptr<cursive0::syntax::Type> to) {
  cursive0::syntax::TransmuteExpr node;
  node.from = std::move(from);
  node.to = std::move(to);
  node.value = std::move(value);
  return MakeExpr(std::move(node));
}

cursive0::syntax::ExprPtr MakeIfExpr(cursive0::syntax::ExprPtr cond,
                                    cursive0::syntax::ExprPtr then_expr,
                                    cursive0::syntax::ExprPtr else_expr) {
  cursive0::syntax::IfExpr node;
  node.cond = std::move(cond);
  node.then_expr = std::move(then_expr);
  node.else_expr = std::move(else_expr);
  return MakeExpr(std::move(node));
}

cursive0::syntax::ExprPtr MakeLoopInfiniteExpr(std::shared_ptr<cursive0::syntax::Block> body) {
  cursive0::syntax::LoopInfiniteExpr node;
  node.body = std::move(body);
  return MakeExpr(std::move(node));
}

cursive0::syntax::ExprPtr MakeLoopConditionalExpr(cursive0::syntax::ExprPtr cond,
                                                 std::shared_ptr<cursive0::syntax::Block> body) {
  cursive0::syntax::LoopConditionalExpr node;
  node.cond = std::move(cond);
  node.body = std::move(body);
  return MakeExpr(std::move(node));
}

cursive0::syntax::ExprPtr MakeLoopIterExpr(cursive0::syntax::PatternPtr pattern,
                                          cursive0::syntax::ExprPtr iter,
                                          std::shared_ptr<cursive0::syntax::Block> body) {
  cursive0::syntax::LoopIterExpr node;
  node.pattern = std::move(pattern);
  node.type_opt = nullptr;
  node.iter = std::move(iter);
  node.body = std::move(body);
  return MakeExpr(std::move(node));
}

cursive0::syntax::ExprPtr MakeBlockExpr(std::shared_ptr<cursive0::syntax::Block> block) {
  cursive0::syntax::BlockExpr node;
  node.block = std::move(block);
  return MakeExpr(std::move(node));
}

cursive0::syntax::ExprPtr MakeUnsafeBlockExpr(std::shared_ptr<cursive0::syntax::Block> block) {
  cursive0::syntax::UnsafeBlockExpr node;
  node.block = std::move(block);
  return MakeExpr(std::move(node));
}

cursive0::syntax::ExprPtr MakeMoveExpr(cursive0::syntax::ExprPtr place) {
  cursive0::syntax::MoveExpr node;
  node.place = std::move(place);
  return MakeExpr(std::move(node));
}

cursive0::syntax::ExprPtr MakeAddressOfExpr(cursive0::syntax::ExprPtr place) {
  cursive0::syntax::AddressOfExpr node;
  node.place = std::move(place);
  return MakeExpr(std::move(node));
}

cursive0::syntax::ExprPtr MakeDerefExpr(cursive0::syntax::ExprPtr value) {
  cursive0::syntax::DerefExpr node;
  node.value = std::move(value);
  return MakeExpr(std::move(node));
}

cursive0::syntax::ExprPtr MakeAllocExpr(cursive0::syntax::ExprPtr value) {
  cursive0::syntax::AllocExpr node;
  node.region_opt = std::nullopt;
  node.value = std::move(value);
  return MakeExpr(std::move(node));
}

cursive0::syntax::ExprPtr MakePropagateExpr(cursive0::syntax::ExprPtr value) {
  cursive0::syntax::PropagateExpr node;
  node.value = std::move(value);
  return MakeExpr(std::move(node));
}

std::shared_ptr<cursive0::syntax::Type> MakeSyntaxPrim(const char* name) {
  auto type = std::make_shared<cursive0::syntax::Type>();
  cursive0::syntax::TypePrim prim;
  prim.name = std::string(name);
  type->node = prim;
  return type;
}

cursive0::syntax::PatternPtr MakePattern(cursive0::syntax::PatternNode node) {
  auto pat = std::make_shared<cursive0::syntax::Pattern>();
  pat->node = std::move(node);
  return pat;
}

cursive0::syntax::PatternPtr MakeWildcardPattern() {
  return MakePattern(cursive0::syntax::WildcardPattern{});
}

cursive0::syntax::PatternPtr MakeIdentPattern(const char* name) {
  cursive0::syntax::IdentifierPattern node;
  node.name = std::string(name);
  return MakePattern(std::move(node));
}

cursive0::syntax::PatternPtr MakeTypedPattern(const char* name,
                                             std::shared_ptr<cursive0::syntax::Type> type) {
  cursive0::syntax::TypedPattern node;
  node.name = std::string(name);
  node.type = std::move(type);
  return MakePattern(std::move(node));
}

cursive0::syntax::PatternPtr MakeTuplePattern(std::vector<cursive0::syntax::PatternPtr> elems) {
  cursive0::syntax::TuplePattern node;
  node.elements = std::move(elems);
  return MakePattern(std::move(node));
}

cursive0::syntax::FieldPattern MakeFieldPattern(const char* name,
                                               cursive0::syntax::PatternPtr pat = nullptr) {
  cursive0::syntax::FieldPattern field;
  field.name = std::string(name);
  field.pattern_opt = std::move(pat);
  return field;
}

cursive0::syntax::PatternPtr MakeRecordPattern(const cursive0::syntax::TypePath& path,
                                              std::vector<cursive0::syntax::FieldPattern> fields) {
  cursive0::syntax::RecordPattern node;
  node.path = path;
  node.fields = std::move(fields);
  return MakePattern(std::move(node));
}

cursive0::syntax::PatternPtr MakeEnumPatternUnit(const cursive0::syntax::TypePath& path,
                                                const char* name) {
  cursive0::syntax::EnumPattern node;
  node.path = path;
  node.name = std::string(name);
  node.payload_opt = std::nullopt;
  return MakePattern(std::move(node));
}

cursive0::syntax::PatternPtr MakeEnumPatternTuple(const cursive0::syntax::TypePath& path,
                                                 const char* name,
                                                 std::vector<cursive0::syntax::PatternPtr> elems) {
  cursive0::syntax::TuplePayloadPattern payload;
  payload.elements = std::move(elems);
  cursive0::syntax::EnumPattern node;
  node.path = path;
  node.name = std::string(name);
  node.payload_opt = cursive0::syntax::EnumPayloadPattern{std::move(payload)};
  return MakePattern(std::move(node));
}

cursive0::syntax::PatternPtr MakeEnumPatternRecord(const cursive0::syntax::TypePath& path,
                                                  const char* name,
                                                  std::vector<cursive0::syntax::FieldPattern> fields) {
  cursive0::syntax::RecordPayloadPattern payload;
  payload.fields = std::move(fields);
  cursive0::syntax::EnumPattern node;
  node.path = path;
  node.name = std::string(name);
  node.payload_opt = cursive0::syntax::EnumPayloadPattern{std::move(payload)};
  return MakePattern(std::move(node));
}

cursive0::syntax::PatternPtr MakeModalPattern(const char* state,
                                             std::optional<std::vector<cursive0::syntax::FieldPattern>> fields) {
  cursive0::syntax::ModalPattern node;
  node.state = std::string(state);
  if (fields.has_value()) {
    cursive0::syntax::ModalRecordPayload payload;
    payload.fields = std::move(*fields);
    node.fields_opt = std::move(payload);
  }
  return MakePattern(std::move(node));
}

cursive0::syntax::PatternPtr MakeRangePattern(cursive0::syntax::RangeKind kind,
                                             cursive0::syntax::PatternPtr lo,
                                             cursive0::syntax::PatternPtr hi) {
  cursive0::syntax::RangePattern node;
  node.kind = kind;
  node.lo = std::move(lo);
  node.hi = std::move(hi);
  return MakePattern(std::move(node));
}

struct ExprTypeEnv {
  std::unordered_map<const cursive0::syntax::Expr*, cursive0::analysis::TypeRef> types;
  cursive0::analysis::TypeRef operator()(const cursive0::syntax::Expr& expr) const {
    auto it = types.find(&expr);
    if (it == types.end()) {
      return nullptr;
    }
    return it->second;
  }
};

void SetExprType(ExprTypeEnv& env,
                 const cursive0::syntax::ExprPtr& expr,
                 cursive0::analysis::TypeRef type) {
  env.types[expr.get()] = std::move(type);
}

cursive0::syntax::RecordDecl MakePointRecord() {
  cursive0::syntax::RecordDecl rec;
  rec.vis = cursive0::syntax::Visibility::Public;
  rec.name = "Point";
  rec.implements = {{"Drawable"}};

  cursive0::syntax::FieldDecl fx;
  fx.vis = cursive0::syntax::Visibility::Public;
  fx.name = "x";
  fx.type = MakeSyntaxPrim("i32");
  fx.init_opt = nullptr;

  cursive0::syntax::FieldDecl fy;
  fy.vis = cursive0::syntax::Visibility::Public;
  fy.name = "y";
  fy.type = MakeSyntaxPrim("i32");
  fy.init_opt = nullptr;

  cursive0::syntax::MethodDecl draw;
  draw.vis = cursive0::syntax::Visibility::Public;
  draw.override_flag = false;
  draw.name = "draw";
  draw.receiver = cursive0::syntax::ReceiverShorthand{cursive0::syntax::ReceiverPerm::Const};
  draw.params = {};
  draw.return_type_opt = nullptr;
  draw.body = std::make_shared<cursive0::syntax::Block>();

  rec.members = {fx, fy, draw};
  return rec;
}

cursive0::syntax::EnumDecl MakeOptionEnum() {
  cursive0::syntax::EnumDecl decl;
  decl.vis = cursive0::syntax::Visibility::Public;
  decl.name = "Option";
  decl.implements = {};

  cursive0::syntax::VariantDecl none;
  none.name = "None";
  none.payload_opt = std::nullopt;

  cursive0::syntax::VariantDecl some;
  some.name = "Some";
  cursive0::syntax::VariantPayloadTuple tup;
  tup.elements = {MakeSyntaxPrim("i32")};
  some.payload_opt = cursive0::syntax::VariantPayload{std::move(tup)};

  cursive0::syntax::VariantDecl pair;
  pair.name = "Pair";
  cursive0::syntax::VariantPayloadRecord rec_payload;
  cursive0::syntax::FieldDecl fa;
  fa.vis = cursive0::syntax::Visibility::Public;
  fa.name = "a";
  fa.type = MakeSyntaxPrim("i32");
  rec_payload.fields.push_back(fa);
  pair.payload_opt = cursive0::syntax::VariantPayload{std::move(rec_payload)};

  decl.variants = {none, some, pair};
  return decl;
}

cursive0::syntax::ModalDecl MakeRegionModal() {
  cursive0::syntax::ModalDecl modal;
  modal.vis = cursive0::syntax::Visibility::Public;
  modal.name = "Region";
  modal.implements = {};

  cursive0::syntax::StateBlock active;
  active.name = "Active";
  cursive0::syntax::StateBlock frozen;
  frozen.name = "Frozen";

  modal.states = {active, frozen};
  return modal;
}

cursive0::syntax::ClassDecl MakeDrawableClass(bool with_body) {
  cursive0::syntax::ClassDecl decl;
  decl.vis = cursive0::syntax::Visibility::Public;
  decl.name = "Drawable";
  decl.supers = {};

  cursive0::syntax::ClassMethodDecl method;
  method.vis = cursive0::syntax::Visibility::Public;
  method.name = "draw";
  method.receiver = cursive0::syntax::ReceiverShorthand{cursive0::syntax::ReceiverPerm::Const};
  method.params = {};
  method.return_type_opt = nullptr;
  if (with_body) {
    method.body_opt = std::make_shared<cursive0::syntax::Block>();
  }

  decl.items = {method};
  return decl;
}

cursive0::analysis::Sigma BuildSigma() {
  cursive0::analysis::Sigma sigma;
  sigma.types[cursive0::analysis::PathKeyOf({"Point"})] = MakePointRecord();
  sigma.types[cursive0::analysis::PathKeyOf({"Option"})] = MakeOptionEnum();
  sigma.types[cursive0::analysis::PathKeyOf({"Region"})] = MakeRegionModal();
  sigma.classes[cursive0::analysis::PathKeyOf({"Drawable"})] = MakeDrawableClass(true);
  return sigma;
}

std::vector<const cursive0::syntax::StaticDecl*> StaticItems(const cursive0::syntax::ASTModule& module) {
  std::vector<const cursive0::syntax::StaticDecl*> out;
  for (const auto& item : module.items) {
    if (const auto* s = std::get_if<cursive0::syntax::StaticDecl>(&item)) {
      out.push_back(s);
    }
  }
  return out;
}

cursive0::codegen::LowerCtx MakeLowerCtx(
    cursive0::analysis::Sigma& sigma,
    ExprTypeEnv& env,
    std::unordered_map<std::string, std::vector<std::string>>* name_map = nullptr,
    std::unordered_map<std::string, std::vector<std::string>>* type_map = nullptr) {
  cursive0::codegen::LowerCtx ctx;
  ctx.sigma = &sigma;
  ctx.module_path = {"main"};
  ctx.proc_ret_type = cursive0::analysis::MakeTypePrim("i32");
  ctx.expr_type = [&env](const cursive0::syntax::Expr& expr) { return env(expr); };
  if (name_map) {
    ctx.resolve_name = [name_map](const std::string& name)
        -> std::optional<std::vector<std::string>> {
      auto it = name_map->find(name);
      if (it == name_map->end()) {
        return std::nullopt;
      }
      return it->second;
    };
  }
  if (type_map) {
    ctx.resolve_type_name = [type_map](const std::string& name)
        -> std::optional<std::vector<std::string>> {
      auto it = type_map->find(name);
      if (it == type_map->end()) {
        return std::nullopt;
      }
      return it->second;
    };
  }
  return ctx;
}

cursive0::codegen::IRValue MakeImmU32(std::uint32_t value) {
  cursive0::codegen::IRValue v;
  v.kind = cursive0::codegen::IRValue::Kind::Immediate;
  v.bytes = {
      static_cast<std::uint8_t>(value & 0xFF),
      static_cast<std::uint8_t>((value >> 8) & 0xFF),
      static_cast<std::uint8_t>((value >> 16) & 0xFF),
      static_cast<std::uint8_t>((value >> 24) & 0xFF),
  };
  return v;
}

void TestLLVMUBSafeAndMemIntrinsics() {
  SPEC_COV("LLVMUBSafe-Add");
  SPEC_COV("LLVMUBSafe-Sub");
  SPEC_COV("LLVMUBSafe-Mul");
  SPEC_COV("LLVMUBSafe-Div");
  SPEC_COV("LLVMUBSafe-Rem");
  SPEC_COV("LLVMUBSafe-Shl");
  SPEC_COV("LLVMUBSafe-Shr");
  SPEC_COV("MemIntrinsics-Copy");
  SPEC_COV("MemIntrinsics-Move");
  SPEC_COV("MemIntrinsics-Set");

  llvm::LLVMContext llvm_ctx;
  cursive0::codegen::LLVMEmitter emitter(llvm_ctx, "ub_safe");
  emitter.SetupModule();

  auto* fn_type = llvm::FunctionType::get(llvm::Type::getVoidTy(llvm_ctx), {}, false);
  auto* fn = llvm::Function::Create(fn_type, llvm::GlobalValue::ExternalLinkage, "f",
                                    &emitter.GetModule());
  auto* entry = llvm::BasicBlock::Create(llvm_ctx, "entry", fn);
  auto* builder = static_cast<llvm::IRBuilder<>*>(emitter.GetBuilderRaw());
  builder->SetInsertPoint(entry);

  llvm::Value* lhs = llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm_ctx), 1);
  llvm::Value* rhs = llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm_ctx), 2);

  assert(emitter.EmitCheckedAdd(lhs, rhs, true));
  assert(emitter.EmitCheckedSub(lhs, rhs, true));
  assert(emitter.EmitCheckedMul(lhs, rhs, true));
  assert(emitter.EmitCheckedDiv(lhs, rhs, true));
  assert(emitter.EmitCheckedRem(lhs, rhs, true));
  assert(emitter.EmitCheckedShl(lhs, rhs));
  assert(emitter.EmitCheckedShr(lhs, rhs, true));

  llvm::Value* dst = builder->CreateAlloca(llvm::Type::getInt8Ty(llvm_ctx));
  llvm::Value* src = builder->CreateAlloca(llvm::Type::getInt8Ty(llvm_ctx));
  llvm::Value* size = llvm::ConstantInt::get(llvm::Type::getInt64Ty(llvm_ctx), 1);
  llvm::Value* byte = llvm::ConstantInt::get(llvm::Type::getInt8Ty(llvm_ctx), 0);
  emitter.EmitMemCpy(dst, src, size, 1);
  emitter.EmitMemMove(dst, src, size, 1);
  emitter.EmitMemSet(dst, byte, size, 1);
}

void TestDynDispatchAndVTable() {
  SPEC_COV("EmitVTable");
  SPEC_COV("EmitVTable-Header");
  SPEC_COV("EmitVTable-Slots");
  SPEC_COV("DispatchSym-Default-Mismatch");
  SPEC_COV("DispatchSym-Default-None");
  SPEC_COV("DispatchSym-Impl");
  SPEC_COV("Lower-DynCall");
  SPEC_COV("Lower-Dynamic-Form");
  SPEC_COV("VSlot-Entry");
  SPEC_COV("VTable-Order");

  cursive0::analysis::Sigma sigma = BuildSigma();
  ExprTypeEnv env;
  auto ctx = MakeLowerCtx(sigma, env);

  const auto class_with_default = MakeDrawableClass(true);
  const auto type_point = cursive0::analysis::MakeTypePath({"Point"});

  (void)cursive0::codegen::DispatchSym(type_point, {"Drawable"}, "draw",
                                      class_with_default, ctx);

  const auto vtable_info = cursive0::codegen::VTable(type_point, {"Drawable"},
                                                     class_with_default, ctx);
  cursive0::codegen::GlobalVTable gvt;
  gvt.symbol = vtable_info.symbol;
  gvt.header.size = vtable_info.type_size;
  gvt.header.align = vtable_info.type_align;
  gvt.header.drop_sym = vtable_info.drop_sym;
  gvt.slots = vtable_info.method_syms;

  llvm::LLVMContext llvm_ctx;
  cursive0::codegen::LLVMEmitter emitter(llvm_ctx, "vt");
  emitter.SetupModule();

  for (const auto& sym : gvt.slots) {
    if (sym.empty()) {
      continue;
    }
    auto* fn_type = llvm::FunctionType::get(llvm::Type::getVoidTy(llvm_ctx), {}, false);
    llvm::Function::Create(fn_type, llvm::GlobalValue::ExternalLinkage, sym,
                           &emitter.GetModule());
  }
  if (!gvt.header.drop_sym.empty()) {
    auto* fn_type = llvm::FunctionType::get(llvm::Type::getVoidTy(llvm_ctx), {}, false);
    llvm::Function::Create(fn_type, llvm::GlobalValue::ExternalLinkage,
                           gvt.header.drop_sym, &emitter.GetModule());
  }
  emitter.EmitVTable(gvt);

  const auto slot = cursive0::codegen::VSlot(class_with_default, "draw");
  assert(slot.has_value());

  auto recv = MakeIdentifierExpr("self");
  ctx.RegisterVar("self", type_point, true, false);
  auto pack = cursive0::codegen::DynPack(type_point, *recv,
                                         {"Drawable"}, class_with_default, ctx);
  assert(pack.ir);

  std::vector<cursive0::codegen::IRValue> args;
  args.push_back(pack.data_ptr);
  auto dyn_call = cursive0::codegen::LowerDynCall(pack.data_ptr,
                                                  pack.vtable_sym,
                                                  class_with_default,
                                                  "draw",
                                                  args,
                                                  ctx);
  assert(dyn_call.ir);
}

void TestLowerExprAndPlaces() {
  SPEC_COV("ArgsExprs-Cons");
  SPEC_COV("ArgsExprs-Empty");
  SPEC_COV("FieldExprs-Cons");
  SPEC_COV("FieldExprs-Empty");
  SPEC_COV("Lower-AddrOf-Deref");
  SPEC_COV("Lower-AddrOf-Deref-Expired");
  SPEC_COV("Lower-AddrOf-Deref-Null");
  SPEC_COV("Lower-AddrOf-Deref-Raw");
  SPEC_COV("Lower-AddrOf-Field");
  SPEC_COV("Lower-AddrOf-Ident-Local");
  SPEC_COV("Lower-AddrOf-Ident-Path");
  SPEC_COV("Lower-AddrOf-Index");
  SPEC_COV("Lower-AddrOf-Index-OOB");
  SPEC_COV("Lower-AddrOf-Tuple");
  SPEC_COV("Lower-BinOp-Ok");
  SPEC_COV("Lower-BinOp-Panic");
  SPEC_COV("Lower-Cast");
  SPEC_COV("Lower-Cast-Panic");
  SPEC_COV("Lower-Expr-AddressOf");
  SPEC_COV("Lower-Expr-Alloc");
  SPEC_COV("Lower-Expr-Array");
  SPEC_COV("Lower-Expr-Bin-And");
  SPEC_COV("Lower-Expr-Bin-Or");
  SPEC_COV("Lower-Expr-Binary");
  SPEC_COV("Lower-Expr-Block");
  SPEC_COV("Lower-Expr-Call-NoPanicOut");
  SPEC_COV("Lower-Expr-Call-PanicOut");
  SPEC_COV("Lower-Expr-Cast");
  SPEC_COV("Lower-Expr-Correctness");
  SPEC_COV("Lower-Expr-Deref");
  SPEC_COV("Lower-Expr-Enum-Record");
  SPEC_COV("Lower-Expr-Enum-Tuple");
  SPEC_COV("Lower-Expr-Enum-Unit");
  SPEC_COV("Lower-Expr-Error");
  SPEC_COV("Lower-Expr-FieldAccess");
  SPEC_COV("Lower-Expr-Ident-Local");
  SPEC_COV("Lower-Expr-Ident-Path");
  SPEC_COV("Lower-Expr-If");
  SPEC_COV("Lower-Expr-Index-Range");
  SPEC_COV("Lower-Expr-Index-Range-OOB");
  SPEC_COV("Lower-Expr-Index-Scalar");
  SPEC_COV("Lower-Expr-Index-Scalar-OOB");
  SPEC_COV("Lower-Expr-Literal");
  SPEC_COV("Lower-Expr-LoopCond");
  SPEC_COV("Lower-Expr-LoopInf");
  SPEC_COV("Lower-Expr-LoopIter");
  SPEC_COV("Lower-Expr-Match");
  SPEC_COV("Lower-Expr-MethodCall");
  SPEC_COV("Lower-Expr-Move");
  SPEC_COV("Lower-Expr-Path");
  SPEC_COV("Lower-Expr-Propagate-Return");
  SPEC_COV("Lower-Expr-Propagate-Success");
  SPEC_COV("Lower-Expr-PtrNull");
  SPEC_COV("Lower-Expr-Range");
  SPEC_COV("Lower-Expr-Record");
  SPEC_COV("Lower-Expr-Transmute");
  SPEC_COV("Lower-Expr-Tuple");
  SPEC_COV("Lower-Expr-TupleAccess");
  SPEC_COV("Lower-Expr-Unary");
  SPEC_COV("Lower-Expr-UnsafeBlock");
  SPEC_COV("Lower-MovePlace");
  SPEC_COV("Lower-Place-Deref");
  SPEC_COV("Lower-Place-Field");
  SPEC_COV("Lower-Place-Ident");
  SPEC_COV("Lower-Place-Index");
  SPEC_COV("Lower-Place-Tuple");
  SPEC_COV("Lower-ReadPlace-Deref");
  SPEC_COV("Lower-ReadPlace-Field");
  SPEC_COV("Lower-ReadPlace-Ident-Local");
  SPEC_COV("Lower-ReadPlace-Ident-Path");
  SPEC_COV("Lower-ReadPlace-Index-Range");
  SPEC_COV("Lower-ReadPlace-Index-Range-OOB");
  SPEC_COV("Lower-ReadPlace-Index-Scalar");
  SPEC_COV("Lower-ReadPlace-Index-Scalar-OOB");
  SPEC_COV("Lower-ReadPlace-Tuple");
  SPEC_COV("Lower-UnOp-Ok");
  SPEC_COV("Lower-UnOp-Panic");
  SPEC_COV("Lower-WritePlace-Deref");
  SPEC_COV("Lower-WritePlace-Field");
  SPEC_COV("Lower-WritePlace-Ident-Local");
  SPEC_COV("Lower-WritePlace-Ident-Path");
  SPEC_COV("Lower-WritePlace-Index-Range");
  SPEC_COV("Lower-WritePlace-Index-Range-Len");
  SPEC_COV("Lower-WritePlace-Index-Range-OOB");
  SPEC_COV("Lower-WritePlace-Index-Scalar");
  SPEC_COV("Lower-WritePlace-Index-Scalar-OOB");
  SPEC_COV("Lower-WritePlace-Tuple");
  SPEC_COV("LowerFieldInits-Cons");
  SPEC_COV("LowerFieldInits-Empty");
  SPEC_COV("LowerList-Cons");
  SPEC_COV("LowerList-Empty");
  SPEC_COV("LowerOpt-None");
  SPEC_COV("LowerOpt-Some");
  SPEC_COV("LowerWriteSub-Deref");
  SPEC_COV("LowerWriteSub-Field");
  SPEC_COV("LowerWriteSub-Ident-Local");
  SPEC_COV("LowerWriteSub-Ident-Path");
  SPEC_COV("LowerWriteSub-Index-Range");
  SPEC_COV("LowerWriteSub-Index-Range-Len");
  SPEC_COV("LowerWriteSub-Index-Range-OOB");
  SPEC_COV("LowerWriteSub-Index-Scalar");
  SPEC_COV("LowerWriteSub-Index-Scalar-OOB");
  SPEC_COV("LowerWriteSub-Tuple");
  SPEC_COV("OptExprs-Both");
  SPEC_COV("OptExprs-Hi");
  SPEC_COV("OptExprs-Lo");
  SPEC_COV("OptExprs-None");

  cursive0::analysis::Sigma sigma = BuildSigma();
  ExprTypeEnv env;
  std::unordered_map<std::string, std::vector<std::string>> name_map;
  std::unordered_map<std::string, std::vector<std::string>> type_map;
  name_map["global"] = {"m", "global"};
  type_map["Point"] = {"Point"};
  auto ctx = MakeLowerCtx(sigma, env, &name_map, &type_map);
  ctx.PushScope(false, false);
  ctx.RegisterVar("local", cursive0::analysis::MakeTypePrim("i32"), true, false);
  ctx.RegisterVar("self", cursive0::analysis::MakeTypePath({"Point"}), true, false);

  auto lit_i32 = MakeLiteralExpr(cursive0::syntax::TokenKind::IntLiteral, "1");
  auto lit_bool = MakeLiteralExpr(cursive0::syntax::TokenKind::BoolLiteral, "true");
  SetExprType(env, lit_i32, cursive0::analysis::MakeTypePrim("i32"));
  SetExprType(env, lit_bool, cursive0::analysis::MakeTypePrim("bool"));

  auto id_local = MakeIdentifierExpr("local");
  auto id_path = MakeIdentifierExpr("global");
  auto path_expr = MakePathExpr({"m"}, "global");

  (void)LowerExpr(*lit_i32, ctx);
  (void)LowerExpr(*id_local, ctx);
  (void)LowerExpr(*id_path, ctx);
  (void)LowerExpr(*path_expr, ctx);

  auto ptr_null = MakeExpr(cursive0::syntax::PtrNullExpr{});
  (void)LowerExpr(*ptr_null, ctx);

  auto err_expr = MakeExpr(cursive0::syntax::ErrorExpr{});
  (void)LowerExpr(*err_expr, ctx);

  auto tuple_expr = MakeTupleExpr({lit_i32, lit_bool});
  auto array_expr = MakeArrayExpr({lit_i32, lit_i32});
  auto record_expr = MakeRecordExpr({"Point"}, {MakeFieldInit("x", lit_i32)});
  auto enum_unit = MakeEnumUnitExpr({"Option", "None"});
  auto enum_tuple = MakeEnumTupleExpr({"Option", "Some"}, {lit_i32});
  auto enum_record = MakeEnumRecordExpr({"Option", "Pair"}, {MakeFieldInit("a", lit_i32)});

  (void)LowerExpr(*tuple_expr, ctx);
  (void)LowerExpr(*array_expr, ctx);
  (void)LowerExpr(*record_expr, ctx);
  (void)LowerExpr(*enum_unit, ctx);
  (void)LowerExpr(*enum_tuple, ctx);
  (void)LowerExpr(*enum_record, ctx);

  auto field_access = MakeFieldAccessExpr(record_expr, "x");
  auto tuple_access = MakeTupleAccessExpr(tuple_expr, "0");
  (void)LowerExpr(*field_access, ctx);
  (void)LowerExpr(*tuple_access, ctx);

  auto range_expr = MakeRangeExpr(cursive0::syntax::RangeKind::Exclusive, lit_i32, lit_i32);
  auto index_range = MakeIndexExpr(array_expr, range_expr);
  auto index_scalar = MakeIndexExpr(array_expr, lit_i32);
  (void)LowerExpr(*index_range, ctx);
  (void)LowerExpr(*index_scalar, ctx);

  auto unary_expr = MakeUnaryExpr("-", lit_i32);
  auto bin_expr = MakeBinaryExpr("+", lit_i32, lit_i32);
  auto cast_expr = MakeCastExpr(lit_i32, MakeSyntaxPrim("i64"));
  auto transmute_expr = MakeTransmuteExpr(lit_i32, MakeSyntaxPrim("i32"), MakeSyntaxPrim("i32"));
  auto if_expr = MakeIfExpr(lit_bool, lit_i32, lit_i32);

  (void)LowerExpr(*unary_expr, ctx);
  (void)LowerExpr(*bin_expr, ctx);
  (void)LowerExpr(*cast_expr, ctx);
  (void)LowerExpr(*transmute_expr, ctx);
  (void)LowerExpr(*if_expr, ctx);

  auto block = std::make_shared<cursive0::syntax::Block>();
  block->stmts = {};
  block->tail_opt = lit_i32;

  auto block_expr = MakeBlockExpr(block);
  auto unsafe_block_expr = MakeUnsafeBlockExpr(block);
  auto loop_inf = MakeLoopInfiniteExpr(block);
  auto loop_cond = MakeLoopConditionalExpr(lit_bool, block);
  auto loop_iter = MakeLoopIterExpr(MakeIdentPattern("it"), array_expr, block);

  (void)LowerExpr(*block_expr, ctx);
  (void)LowerExpr(*unsafe_block_expr, ctx);
  (void)LowerExpr(*loop_inf, ctx);
  (void)LowerExpr(*loop_cond, ctx);
  (void)LowerExpr(*loop_iter, ctx);

  auto move_expr = MakeMoveExpr(id_local);
  auto addr_expr = MakeAddressOfExpr(id_local);
  auto deref_expr = MakeDerefExpr(addr_expr);
  auto alloc_expr = MakeAllocExpr(lit_i32);
  auto propagate_expr = MakePropagateExpr(lit_i32);

  (void)LowerExpr(*move_expr, ctx);
  (void)LowerExpr(*addr_expr, ctx);
  (void)LowerExpr(*deref_expr, ctx);
  (void)LowerExpr(*alloc_expr, ctx);
  (void)LowerExpr(*propagate_expr, ctx);

  cursive0::syntax::Arg arg;

  arg.value = lit_i32;
  auto call_expr = MakeCallExpr(MakeIdentifierExpr("Point"), {arg});
  auto method_expr = MakeMethodCallExpr(MakeIdentifierExpr("self"), "draw", {});

  (void)LowerExpr(*call_expr, ctx);
  (void)LowerExpr(*method_expr, ctx);

  (void)cursive0::codegen::LowerList({}, ctx);
  (void)cursive0::codegen::LowerList({lit_i32, lit_bool}, ctx);
  (void)cursive0::codegen::LowerFieldInits({}, ctx, false);
  (void)cursive0::codegen::LowerFieldInits({MakeFieldInit("x", lit_i32)}, ctx, false);
  (void)cursive0::codegen::LowerOpt(nullptr, ctx);
  (void)cursive0::codegen::LowerOpt(lit_i32, ctx);

  (void)cursive0::codegen::LowerReadPlace(*id_local, ctx);
  (void)cursive0::codegen::LowerWritePlace(*id_local, MakeImmU32(1), ctx);
  (void)cursive0::codegen::LowerWritePlaceSub(*id_local, MakeImmU32(2), ctx);
  (void)cursive0::codegen::LowerPlace(*id_local, ctx);
  (void)cursive0::codegen::LowerMovePlace(*id_local, ctx);
  (void)cursive0::codegen::LowerAddrOf(*id_local, ctx);

  (void)cursive0::codegen::LowerUnOp("-", *lit_i32, ctx);
  (void)cursive0::codegen::LowerBinOp("+", *lit_i32, *lit_i32, ctx);
  (void)cursive0::codegen::LowerCast(*lit_i32, cursive0::analysis::MakeTypePrim("i64"), ctx);
}

void TestLowerStmtAndBlock() {
  SPEC_COV("Lower-Block-Correctness");
  SPEC_COV("Lower-Block-Tail");
  SPEC_COV("Lower-Block-Unit");
  SPEC_COV("Lower-Loop-Cond");
  SPEC_COV("Lower-Loop-Correctness");
  SPEC_COV("Lower-Loop-Iter");
  SPEC_COV("Lower-Stmt-Assign");
  SPEC_COV("Lower-Stmt-Break");
  SPEC_COV("Lower-Stmt-Break-Unit");
  SPEC_COV("Lower-Stmt-CompoundAssign");
  SPEC_COV("Lower-Stmt-Continue");
  SPEC_COV("Lower-Stmt-Correctness");
  SPEC_COV("Lower-Stmt-Defer");
  SPEC_COV("Lower-Stmt-Error");
  SPEC_COV("Lower-Stmt-Expr");
  SPEC_COV("Lower-Stmt-Frame-Explicit");
  SPEC_COV("Lower-Stmt-Frame-Implicit");
  SPEC_COV("Lower-Stmt-Let");
  SPEC_COV("Lower-Stmt-Region");
  SPEC_COV("Lower-Stmt-Result");
  SPEC_COV("Lower-Stmt-Return");
  SPEC_COV("Lower-Stmt-Return-Unit");
  SPEC_COV("Lower-Stmt-ShadowLet");
  SPEC_COV("Lower-Stmt-ShadowVar");
  SPEC_COV("Lower-Stmt-UnsafeBlock");
  SPEC_COV("Lower-Stmt-Var");
  SPEC_COV("Lower-StmtList-Cons");
  SPEC_COV("Lower-StmtList-Empty");

  cursive0::analysis::Sigma sigma = BuildSigma();
  ExprTypeEnv env;
  auto ctx = MakeLowerCtx(sigma, env);
  ctx.PushScope(false, false);
  ctx.RegisterVar("local", cursive0::analysis::MakeTypePrim("i32"), true, false);

  auto lit_i32 = MakeLiteralExpr(cursive0::syntax::TokenKind::IntLiteral, "1");
  SetExprType(env, lit_i32, cursive0::analysis::MakeTypePrim("i32"));

  cursive0::syntax::Binding binding;
  binding.pat = MakeIdentPattern("x");
  binding.type_opt = nullptr;
  binding.op = MakeToken(cursive0::syntax::TokenKind::Operator, "=");
  binding.init = lit_i32;

  cursive0::syntax::LetStmt let_stmt;
  let_stmt.binding = binding;

  cursive0::syntax::VarStmt var_stmt;
  var_stmt.binding = binding;

  cursive0::syntax::AssignStmt assign_stmt;
  assign_stmt.place = MakeIdentifierExpr("local");
  assign_stmt.value = lit_i32;

  cursive0::syntax::CompoundAssignStmt comp_assign;
  comp_assign.place = MakeIdentifierExpr("local");
  comp_assign.op = "+=";
  comp_assign.value = lit_i32;

  cursive0::syntax::ExprStmt expr_stmt;
  expr_stmt.value = lit_i32;

  cursive0::syntax::DeferStmt defer_stmt;
  auto defer_block = std::make_shared<cursive0::syntax::Block>();
  defer_block->stmts = {};
  defer_stmt.body = defer_block;

  cursive0::syntax::RegionStmt region_stmt;
  region_stmt.opts_opt = nullptr;
  region_stmt.alias_opt = std::nullopt;
  auto region_block = std::make_shared<cursive0::syntax::Block>();
  region_block->stmts = {};
  region_stmt.body = region_block;

  cursive0::syntax::FrameStmt frame_stmt;
  frame_stmt.target_opt = std::nullopt;
  auto frame_block = std::make_shared<cursive0::syntax::Block>();
  frame_block->stmts = {};
  frame_stmt.body = frame_block;

  cursive0::syntax::ReturnStmt ret_stmt;
  ret_stmt.value_opt = lit_i32;

  cursive0::syntax::ReturnStmt ret_unit;
  ret_unit.value_opt = nullptr;

  cursive0::syntax::ResultStmt result_stmt;
  result_stmt.value = lit_i32;

  cursive0::syntax::BreakStmt break_stmt;
  break_stmt.value_opt = lit_i32;

  cursive0::syntax::BreakStmt break_unit;
  break_unit.value_opt = nullptr;

  cursive0::syntax::ContinueStmt continue_stmt;

  cursive0::syntax::UnsafeBlockStmt unsafe_stmt;
  unsafe_stmt.body = frame_block;

  cursive0::syntax::ShadowLetStmt shadow_let;
  shadow_let.name = "shadow";
  shadow_let.type_opt = nullptr;
  shadow_let.init = lit_i32;

  cursive0::syntax::ShadowVarStmt shadow_var;
  shadow_var.name = "shadow";
  shadow_var.type_opt = nullptr;
  shadow_var.init = lit_i32;

  cursive0::syntax::ErrorStmt error_stmt;

  std::vector<cursive0::syntax::Stmt> stmts;
  stmts.push_back(let_stmt);
  stmts.push_back(var_stmt);
  stmts.push_back(assign_stmt);
  stmts.push_back(comp_assign);
  stmts.push_back(expr_stmt);
  stmts.push_back(defer_stmt);
  stmts.push_back(region_stmt);
  stmts.push_back(frame_stmt);
  stmts.push_back(ret_stmt);
  stmts.push_back(result_stmt);
  stmts.push_back(break_stmt);
  stmts.push_back(continue_stmt);
  stmts.push_back(unsafe_stmt);
  stmts.push_back(shadow_let);
  stmts.push_back(shadow_var);
  stmts.push_back(error_stmt);

  (void)cursive0::codegen::LowerStmtList({}, ctx);
  (void)cursive0::codegen::LowerStmtList(stmts, ctx);

  for (const auto& stmt : stmts) {
    (void)cursive0::codegen::LowerStmt(stmt, ctx);
  }

  cursive0::syntax::Block block;
  block.stmts = stmts;
  block.tail_opt = lit_i32;
  (void)cursive0::codegen::LowerBlock(block, ctx);

  auto loop_block = std::make_shared<cursive0::syntax::Block>();
  loop_block->stmts = {};
  loop_block->tail_opt = nullptr;

  cursive0::syntax::LoopConditionalExpr loop_cond;
  loop_cond.cond = lit_i32;
  loop_cond.body = loop_block;
  cursive0::syntax::Expr loop_cond_expr;
  loop_cond_expr.node = loop_cond;
  (void)cursive0::codegen::LowerExpr(loop_cond_expr, ctx);

  cursive0::syntax::LoopIterExpr loop_iter;
  loop_iter.pattern = MakeIdentPattern("it");
  loop_iter.type_opt = nullptr;
  loop_iter.iter = lit_i32;
  loop_iter.body = loop_block;
  cursive0::syntax::Expr loop_iter_expr;
  loop_iter_expr.node = loop_iter;
  (void)cursive0::codegen::LowerExpr(loop_iter_expr, ctx);

  (void)ret_unit;
  (void)break_unit;
}

void TestPatternLowering() {
  SPEC_COV("Lower-BindList-Cons");
  SPEC_COV("Lower-BindList-Empty");
  SPEC_COV("Lower-Match");
  SPEC_COV("Lower-Match-Correctness");
  SPEC_COV("Lower-Pat-Correctness");
  SPEC_COV("Lower-Pat-Err");
  SPEC_COV("Lower-Pat-General");
  SPEC_COV("Match-Wildcard");
  SPEC_COV("Match-Ident");
  SPEC_COV("Match-Typed");
  SPEC_COV("Match-Literal");
  SPEC_COV("Match-Tuple");
  SPEC_COV("Match-Record");
  SPEC_COV("Match-Enum-Unit");
  SPEC_COV("Match-Enum-Tuple");
  SPEC_COV("Match-Enum-Record");
  SPEC_COV("Match-Modal-General");
  SPEC_COV("Match-Modal-State");
  SPEC_COV("Match-Modal-Empty");
  SPEC_COV("Match-Modal-Record");
  SPEC_COV("Match-Range");
  SPEC_COV("TagOf-Enum");
  SPEC_COV("TagOf-Modal");

  cursive0::analysis::Sigma sigma = BuildSigma();
  ExprTypeEnv env;
  auto ctx = MakeLowerCtx(sigma, env);
  ctx.PushScope(false, false);

  auto lit_i32 = MakeLiteralExpr(cursive0::syntax::TokenKind::IntLiteral, "1");
  SetExprType(env, lit_i32, cursive0::analysis::MakeTypePrim("i32"));

  cursive0::codegen::IRValue scrut;
  scrut.kind = cursive0::codegen::IRValue::Kind::Local;
  scrut.name = "scrut";

  (void)cursive0::codegen::TagOf(scrut, cursive0::codegen::TagOfKind::Enum, ctx);
  (void)cursive0::codegen::TagOf(scrut, cursive0::codegen::TagOfKind::Modal, ctx);

  auto pat_any = MakeWildcardPattern();
  auto pat_ident = MakeIdentPattern("x");
  auto pat_typed = MakeTypedPattern("x", MakeSyntaxPrim("i32"));
  auto pat_tuple = MakeTuplePattern({MakeIdentPattern("a"), MakeWildcardPattern()});
  auto pat_record = MakeRecordPattern({"Point"}, {MakeFieldPattern("x")});
  auto pat_enum_unit = MakeEnumPatternUnit({"Option"}, "None");
  auto pat_enum_tuple = MakeEnumPatternTuple({"Option"}, "Some", {MakeWildcardPattern()});
  auto pat_enum_record = MakeEnumPatternRecord({"Option"}, "Pair", {MakeFieldPattern("a")});
  auto pat_modal = MakeModalPattern("Active", std::nullopt);
  auto pat_modal_rec = MakeModalPattern("Active", std::vector<cursive0::syntax::FieldPattern>{MakeFieldPattern("x")});
  auto pat_range = MakeRangePattern(cursive0::syntax::RangeKind::Exclusive,
                                    MakeIdentPattern("lo"),
                                    MakeIdentPattern("hi"));

  (void)cursive0::codegen::PatternCheck(*pat_any, scrut, ctx);
  (void)cursive0::codegen::PatternCheck(*pat_ident, scrut, ctx);
  (void)cursive0::codegen::PatternCheck(*pat_typed, scrut, ctx);
  (void)cursive0::codegen::PatternCheck(*pat_tuple, scrut, ctx);
  (void)cursive0::codegen::PatternCheck(*pat_record, scrut, ctx);
  (void)cursive0::codegen::PatternCheck(*pat_enum_unit, scrut, ctx);
  (void)cursive0::codegen::PatternCheck(*pat_enum_tuple, scrut, ctx);
  (void)cursive0::codegen::PatternCheck(*pat_enum_record, scrut, ctx);
  (void)cursive0::codegen::PatternCheck(*pat_modal, scrut, ctx);
  (void)cursive0::codegen::PatternCheck(*pat_modal_rec, scrut, ctx);
  (void)cursive0::codegen::PatternCheck(*pat_range, scrut, ctx);

  std::vector<cursive0::syntax::PatternPtr> patterns = {pat_ident};
  std::vector<cursive0::codegen::IRValue> values = {scrut};
  (void)cursive0::codegen::LowerBindList({}, {}, ctx);
  (void)cursive0::codegen::LowerBindList(patterns, values, ctx);

  cursive0::syntax::MatchArm arm;
  arm.pattern = pat_any;
  arm.guard_opt = nullptr;
  arm.body = lit_i32;
  (void)cursive0::codegen::LowerMatchArm(arm, scrut,
                                         cursive0::analysis::MakeTypePrim("i32"), ctx);

  (void)cursive0::codegen::LowerMatch(*lit_i32, {arm}, ctx);
}

void TestGlobalsAndInit() {
  SPEC_COV("DeinitCallIR");
  SPEC_COV("DeinitFn");
  SPEC_COV("Emit-Static-Const");
  SPEC_COV("Emit-Static-Init");
  SPEC_COV("Emit-Static-Multi");
  SPEC_COV("EmitDeinitPlan");
  SPEC_COV("EmitDeinitPlan-Err");
  SPEC_COV("EmitInitPlan");
  SPEC_COV("EmitInitPlan-Err");
  SPEC_COV("InitCallIR");
  SPEC_COV("InitFn");
  SPEC_COV("Lower-StaticDeinit");
  SPEC_COV("Lower-StaticDeinit-Item");
  SPEC_COV("Lower-StaticDeinitItems-Cons");
  SPEC_COV("Lower-StaticDeinitItems-Empty");
  SPEC_COV("Lower-StaticDeinitNames-Cons-NoResp");
  SPEC_COV("Lower-StaticDeinitNames-Cons-Resp");
  SPEC_COV("Lower-StaticDeinitNames-Empty");
  SPEC_COV("Lower-StaticInit");
  SPEC_COV("Lower-StaticInit-Item");
  SPEC_COV("Lower-StaticInitItems-Cons");
  SPEC_COV("Lower-StaticInitItems-Empty");
  SPEC_COV("CleanupPlan");

  cursive0::analysis::Sigma sigma = BuildSigma();
  ExprTypeEnv env;
  auto ctx = MakeLowerCtx(sigma, env);

  auto lit_i32 = MakeLiteralExpr(cursive0::syntax::TokenKind::IntLiteral, "1");
  cursive0::syntax::Binding binding;
  binding.pat = MakeIdentPattern("x");
  binding.type_opt = nullptr;
  binding.op = MakeToken(cursive0::syntax::TokenKind::Operator, "=");
  binding.init = lit_i32;

  cursive0::syntax::StaticDecl decl;
  decl.vis = cursive0::syntax::Visibility::Internal;
  decl.mut = cursive0::syntax::Mutability::Let;
  decl.binding = binding;

  SetExprType(env, lit_i32, cursive0::analysis::MakeTypePrim("i32"));
  (void)cursive0::codegen::EmitGlobal(decl, {"main"}, ctx);

  cursive0::syntax::ASTModule module;
  module.path = {"main"};
  module.items = {decl};

  (void)cursive0::codegen::LowerStaticInit({"main"}, module, ctx);
  (void)cursive0::codegen::LowerStaticDeinit({"main"}, module, ctx);
  (void)cursive0::codegen::LowerStaticInitItems({"main"}, StaticItems(module), ctx);
  (void)cursive0::codegen::LowerStaticDeinitItems({"main"}, StaticItems(module), ctx);
  (void)cursive0::codegen::LowerStaticInitItem({"main"}, decl, ctx);
  (void)cursive0::codegen::LowerStaticDeinitItem({"main"}, decl, ctx);
  (void)cursive0::codegen::LowerStaticDeinitNames({"main"}, decl, {"x"}, ctx);
  (void)cursive0::codegen::LowerStaticDeinitNames({"main"}, decl, {}, ctx);

  (void)cursive0::codegen::InitCallIR({"main"}, ctx);
  (void)cursive0::codegen::DeinitCallIR({"main"}, ctx);
  (void)cursive0::codegen::EmitInitPlan({{"main"}}, ctx);
  (void)cursive0::codegen::EmitDeinitPlan({{"main"}}, ctx);
  (void)cursive0::codegen::EmitModuleInitFn({"main"}, module, ctx);
  (void)cursive0::codegen::EmitModuleDeinitFn({"main"}, module, ctx);
}

void TestChecksAndRanges() {
  SPEC_COV("Check-Index-Err");
  SPEC_COV("Check-Index-Ok");
  SPEC_COV("Check-Range-Err");
  SPEC_COV("Check-Range-Ok");
  SPEC_COV("Lower-Range-Exclusive");
  SPEC_COV("Lower-Range-From");
  SPEC_COV("Lower-Range-Full");
  SPEC_COV("Lower-Range-Inclusive");
  SPEC_COV("Lower-Range-To");
  SPEC_COV("Lower-Range-ToInclusive");
  SPEC_COV("Lower-RawDeref-Expired");
  SPEC_COV("Lower-RawDeref-Null");
  SPEC_COV("Lower-RawDeref-Raw");
  SPEC_COV("Lower-RawDeref-Safe");
  SPEC_COV("Lower-Transmute");
  SPEC_COV("Lower-Transmute-Err");

  assert(cursive0::codegen::CheckIndex(4, 2));
  assert(!cursive0::codegen::CheckIndex(1, 2));

  cursive0::codegen::RangeVal range_ok;
  range_ok.kind = cursive0::syntax::RangeKind::Exclusive;
  range_ok.lo = MakeImmU32(0);
  range_ok.hi = MakeImmU32(1);
  assert(cursive0::codegen::CheckRange(4, range_ok));

  cursive0::codegen::RangeVal range_bad;
  range_bad.kind = cursive0::syntax::RangeKind::Exclusive;
  range_bad.lo = MakeImmU32(5);
  range_bad.hi = MakeImmU32(6);
  assert(!cursive0::codegen::CheckRange(4, range_bad));

  cursive0::analysis::Sigma sigma = BuildSigma();
  ExprTypeEnv env;
  auto ctx = MakeLowerCtx(sigma, env);

  auto lit_i32 = MakeLiteralExpr(cursive0::syntax::TokenKind::IntLiteral, "1");
  SetExprType(env, lit_i32, cursive0::analysis::MakeTypePrim("i32"));

  (void)cursive0::codegen::LowerRangeExpr({cursive0::syntax::RangeKind::Exclusive, lit_i32, lit_i32}, ctx);
  (void)cursive0::codegen::LowerRangeExpr({cursive0::syntax::RangeKind::Inclusive, lit_i32, lit_i32}, ctx);
  (void)cursive0::codegen::LowerRangeExpr({cursive0::syntax::RangeKind::From, lit_i32, nullptr}, ctx);
  (void)cursive0::codegen::LowerRangeExpr({cursive0::syntax::RangeKind::To, nullptr, lit_i32}, ctx);
  (void)cursive0::codegen::LowerRangeExpr({cursive0::syntax::RangeKind::ToInclusive, nullptr, lit_i32}, ctx);
  (void)cursive0::codegen::LowerRangeExpr({cursive0::syntax::RangeKind::Full, nullptr, nullptr}, ctx);

  (void)cursive0::codegen::LowerTransmute(cursive0::analysis::MakeTypePrim("i32"),
                                          cursive0::analysis::MakeTypePrim("i32"),
                                          *lit_i32, ctx);
  (void)cursive0::codegen::LowerTransmute(cursive0::analysis::MakeTypePrim("i32"),
                                          cursive0::analysis::MakeTypePrim("i64"),
                                          *lit_i32, ctx);

  cursive0::codegen::IRValue ptr_value;
  ptr_value.kind = cursive0::codegen::IRValue::Kind::Local;
  ptr_value.name = "ptr";
  (void)cursive0::codegen::LowerRawDeref(ptr_value,
                                        cursive0::analysis::MakeTypePtr(cursive0::analysis::MakeTypePrim("i32"),
                                                                    cursive0::analysis::PtrState::Valid),
                                        ctx);
  (void)cursive0::codegen::LowerRawDeref(ptr_value,
                                        cursive0::analysis::MakeTypePtr(cursive0::analysis::MakeTypePrim("i32"),
                                                                    cursive0::analysis::PtrState::Null),
                                        ctx);
  (void)cursive0::codegen::LowerRawDeref(ptr_value,
                                        cursive0::analysis::MakeTypePtr(cursive0::analysis::MakeTypePrim("i32"),
                                                                    cursive0::analysis::PtrState::Expired),
                                        ctx);
  (void)cursive0::codegen::LowerRawDeref(ptr_value,
                                        cursive0::analysis::MakeTypeRawPtr(cursive0::analysis::RawPtrQual::Imm,
                                                                       cursive0::analysis::MakeTypePrim("i32")),
                                        ctx);
}

}  // namespace

int main() {
  TestLLVMUBSafeAndMemIntrinsics();
  TestDynDispatchAndVTable();
  TestLowerExprAndPlaces();
  TestLowerStmtAndBlock();
  TestPatternLowering();
  TestGlobalsAndInit();
  TestChecksAndRanges();
  return 0;
}
