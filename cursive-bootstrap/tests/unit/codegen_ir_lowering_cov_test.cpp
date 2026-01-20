#include <cassert>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "cursive0/codegen/abi.h"
#include "cursive0/codegen/cleanup.h"
#include "cursive0/codegen/globals.h"
#include "cursive0/codegen/llvm_emit.h"
#include "cursive0/codegen/lower_expr.h"
#include "cursive0/core/assert_spec.h"
#include "cursive0/core/symbols.h"
#include "cursive0/sema/context.h"
#include "cursive0/sema/scopes.h"
#include "cursive0/sema/types.h"
#include "cursive0/syntax/ast.h"

#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

namespace {

using cursive0::codegen::GlobalVTable;
using cursive0::codegen::GlobalZero;
using cursive0::codegen::IR;
using cursive0::codegen::IRAddrOf;
using cursive0::codegen::IRAlloc;
using cursive0::codegen::IRBindVar;
using cursive0::codegen::IRBlock;
using cursive0::codegen::IRBranch;
using cursive0::codegen::IRCall;
using cursive0::codegen::IRCallVTable;
using cursive0::codegen::IRCheckPoison;
using cursive0::codegen::IRClearPanic;
using cursive0::codegen::IRDecls;
using cursive0::codegen::IRFrame;
using cursive0::codegen::IRIf;
using cursive0::codegen::IRInitPanicHandle;
using cursive0::codegen::IRLoop;
using cursive0::codegen::IRLoopKind;
using cursive0::codegen::IRLowerPanic;
using cursive0::codegen::IRMatch;
using cursive0::codegen::IRMatchArm;
using cursive0::codegen::IRMoveState;
using cursive0::codegen::IROpaque;
using cursive0::codegen::IRPanicCheck;
using cursive0::codegen::IRPhi;
using cursive0::codegen::IRPtr;
using cursive0::codegen::IRReadPath;
using cursive0::codegen::IRReadPlace;
using cursive0::codegen::IRReadPtr;
using cursive0::codegen::IRReadVar;
using cursive0::codegen::IRRegion;
using cursive0::codegen::IRSeq;
using cursive0::codegen::IRStoreGlobal;
using cursive0::codegen::IRStoreVar;
using cursive0::codegen::IRStoreVarNoDrop;
using cursive0::codegen::IRValue;
using cursive0::codegen::IRWritePlace;
using cursive0::codegen::IRWritePtr;
using cursive0::codegen::LowerCtx;
using cursive0::codegen::LLVMEmitter;
using cursive0::codegen::ProcIR;
using cursive0::core::Mangle;
using cursive0::core::StringOfPath;
using cursive0::sema::MakeTypeArray;
using cursive0::sema::MakeTypeModalState;
using cursive0::sema::MakeTypePath;
using cursive0::sema::MakeTypePerm;
using cursive0::sema::MakeTypePrim;
using cursive0::sema::MakeTypePtr;
using cursive0::sema::MakeTypeRawPtr;
using cursive0::sema::MakeTypeTuple;
using cursive0::sema::Permission;
using cursive0::sema::PtrState;
using cursive0::sema::RawPtrQual;
using cursive0::sema::TypeRef;
using cursive0::syntax::Expr;
using cursive0::syntax::IdentifierExpr;
using cursive0::syntax::ModalDecl;
using cursive0::syntax::Pattern;
using cursive0::syntax::RecordDecl;
using cursive0::syntax::StateBlock;
using cursive0::syntax::Visibility;
using cursive0::syntax::WildcardPattern;

IRPtr MakeOpaque() {
  auto ir = std::make_shared<IR>();
  ir->node = IROpaque{};
  return ir;
}

IRValue MakeLocal(const char* name) {
  IRValue value;
  value.kind = IRValue::Kind::Local;
  value.name = name;
  return value;
}

IRValue MakeSymbol(const std::string& name) {
  IRValue value;
  value.kind = IRValue::Kind::Symbol;
  value.name = name;
  return value;
}

IRValue MakeImmU32(std::uint32_t value) {
  IRValue v;
  v.kind = IRValue::Kind::Immediate;
  v.bytes = {
      static_cast<std::uint8_t>(value & 0xFF),
      static_cast<std::uint8_t>((value >> 8) & 0xFF),
      static_cast<std::uint8_t>((value >> 16) & 0xFF),
      static_cast<std::uint8_t>((value >> 24) & 0xFF),
  };
  return v;
}

IRValue MakeImmBool(bool value) {
  IRValue v;
  v.kind = IRValue::Kind::Immediate;
  v.bytes = {static_cast<std::uint8_t>(value ? 1 : 0)};
  return v;
}

IRValue MakeUnitOpaque() {
  IRValue v;
  v.kind = IRValue::Kind::Opaque;
  v.name = "unit";
  return v;
}

std::shared_ptr<Pattern> MakeWildcardPat() {
  auto pat = std::make_shared<Pattern>();
  pat->node = WildcardPattern{};
  return pat;
}

Expr MakeIdentExpr(const char* name) {
  Expr expr;
  IdentifierExpr ident;
  ident.name = name;
  expr.node = ident;
  return expr;
}

sema::Sigma BuildSigma() {
  sema::Sigma sigma;

  RecordDecl rec;
  rec.vis = Visibility::Public;
  rec.name = "Rec";
  rec.members = {};
  rec.implements = {};
  rec.span = {};
  rec.doc = {};
  sigma.types[cursive0::sema::PathKeyOf({"test", "Rec"})] = rec;

  RecordDecl opts;
  opts.vis = Visibility::Public;
  opts.name = "RegionOptions";
  opts.members = {};
  opts.implements = {};
  opts.span = {};
  opts.doc = {};
  sigma.types[cursive0::sema::PathKeyOf({"RegionOptions"})] = opts;

  StateBlock active;
  active.name = "Active";
  active.members = {};
  active.span = {};
  active.doc_opt = std::nullopt;

  StateBlock frozen;
  frozen.name = "Frozen";
  frozen.members = {};
  frozen.span = {};
  frozen.doc_opt = std::nullopt;

  ModalDecl region;
  region.vis = Visibility::Public;
  region.name = "Region";
  region.implements = {};
  region.states = {active, frozen};
  region.span = {};
  region.doc = {};
  sigma.types[cursive0::sema::PathKeyOf({"Region"})] = region;

  return sigma;
}

struct LowerHarness {
  llvm::LLVMContext llvm_ctx;
  LowerCtx ctx;
  std::unique_ptr<LLVMEmitter> emitter;
  llvm::IRBuilder<>* builder = nullptr;

  void Init(const IRDecls& decls) {
    emitter = std::make_unique<LLVMEmitter>(llvm_ctx, "ir_lower_cov");
    emitter->EmitModule(decls, ctx);
    auto* fn_type =
        llvm::FunctionType::get(llvm::Type::getVoidTy(llvm_ctx), {}, false);
    auto* fn = llvm::Function::Create(
        fn_type, llvm::GlobalValue::ExternalLinkage, "ir_lower_fn",
        &emitter->GetModule());
    auto* entry = llvm::BasicBlock::Create(llvm_ctx, "entry", fn);
    builder = static_cast<llvm::IRBuilder<>*>(emitter->GetBuilderRaw());
    builder->SetInsertPoint(entry);
  }
};

llvm::AllocaInst* EntryAlloca(LowerHarness& h,
                              llvm::Type* ty,
                              const std::string& name) {
  llvm::Function* func = h.builder->GetInsertBlock()->getParent();
  llvm::IRBuilder<> entry_builder(&func->getEntryBlock(),
                                  func->getEntryBlock().begin());
  return entry_builder.CreateAlloca(ty, nullptr, name);
}

void DefineLocal(LowerHarness& h,
                 const std::string& name,
                 llvm::Type* llvm_ty,
                 llvm::Constant* init,
                 const TypeRef& type) {
  auto* slot = EntryAlloca(h, llvm_ty, name);
  h.builder->CreateStore(init, slot);
  h.emitter->SetLocal(name, slot);
  h.ctx.RegisterVar(name, type, true, false);
}

void DefinePtrLocal(LowerHarness& h,
                    const std::string& name,
                    const TypeRef& type) {
  llvm::Type* ptr_ty = h.emitter->GetOpaquePtr();
  auto* slot = EntryAlloca(h, ptr_ty, name);
  auto* null_ptr =
      llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(ptr_ty));
  h.builder->CreateStore(null_ptr, slot);
  h.emitter->SetLocal(name, slot);
  h.ctx.RegisterVar(name, type, true, false);
}

void DefinePanicSlot(LowerHarness& h) {
  llvm::Type* ptr_ty = h.emitter->GetOpaquePtr();
  auto* slot = EntryAlloca(h, ptr_ty, std::string(cursive0::codegen::kPanicOutName));
  auto* null_ptr =
      llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(ptr_ty));
  h.builder->CreateStore(null_ptr, slot);
  h.emitter->SetLocal(std::string(cursive0::codegen::kPanicOutName), slot);
}

void TestRecordCtorLowering() {
  SPEC_COV("Lower-CallIR-RecordCtor");
  sema::Sigma sigma;
  RecordDecl rec;
  rec.vis = Visibility::Public;
  rec.name = "Rec";
  rec.members = {};
  rec.implements = {};
  rec.span = {};
  rec.doc = {};
  sigma.types[cursive0::sema::PathKeyOf({"Rec"})] = rec;

  LowerCtx ctx;
  ctx.sigma = &sigma;

  Expr callee = MakeIdentExpr("Rec");
  cursive0::syntax::CallExpr call;
  call.callee = std::make_shared<Expr>(callee);
  call.args = {};

  Expr expr;
  expr.node = call;
  auto result = cursive0::codegen::LowerExpr(expr, ctx);
  assert(result.ir != nullptr);
}

void TestIRLoweringSuccess() {
  SPEC_COV("LowerIR-Module");
  SPEC_COV("EmitLLVM-Ok");
  SPEC_COV("LowerIRDecl-Proc-Gen");
  SPEC_COV("LowerIRDecl-Proc-User");
  SPEC_COV("RuntimeDecls");
  SPEC_COV("BindSlot-Local");
  SPEC_COV("BindSlot-Static");
  SPEC_COV("UpdateValid-BindVar");
  SPEC_COV("Lower-BindVarIR");
  SPEC_COV("Lower-StoreVarIR");
  SPEC_COV("Lower-StoreVarNoDropIR");
  SPEC_COV("Lower-ReadVarIR");
  SPEC_COV("Lower-ReadPlaceIR");
  SPEC_COV("Lower-WritePlaceIR");
  SPEC_COV("Lower-ReadPtrIR");
  SPEC_COV("Lower-ReadPtrIR-Raw");
  SPEC_COV("Lower-WritePtrIR");
  SPEC_COV("Lower-WritePtrIR-Null");
  SPEC_COV("Lower-WritePtrIR-Expired");
  SPEC_COV("Lower-WritePtrIR-Raw");
  SPEC_COV("Lower-AddrOfIR");
  SPEC_COV("Lower-AllocIR");
  SPEC_COV("Lower-BlockIR");
  SPEC_COV("Lower-IfIR");
  SPEC_COV("Lower-LoopIR");
  SPEC_COV("Lower-MatchIR");
  SPEC_COV("Lower-MoveStateIR");
  SPEC_COV("Lower-FrameIR");
  SPEC_COV("Lower-BranchIR");
  SPEC_COV("Lower-PhiIR");
  SPEC_COV("Lower-RegionIR");
  SPEC_COV("Lower-CallIR-Func");
  SPEC_COV("Lower-CallVTable");
  SPEC_COV("Lower-ReadPathIR-Runtime");
  SPEC_COV("Lower-ReadPathIR-Record");
  SPEC_COV("Lower-ReadPathIR-Static-User");
  SPEC_COV("Lower-ReadPathIR-Static-Gen");
  SPEC_COV("Lower-ReadPathIR-Proc-User");
  SPEC_COV("Lower-ReadPathIR-Proc-Gen");
  SPEC_COV("Lower-StoreGlobal");
  SPEC_COV("LowerIRInstr-Seq");
  SPEC_COV("LowerIRInstr-Empty");
  SPEC_COV("LowerIRInstr-CheckPoison");
  SPEC_COV("LowerIRInstr-ClearPanic");
  SPEC_COV("LowerIRInstr-PanicCheck");
  SPEC_COV("LowerIRInstr-LowerPanic");
  SPEC_COV("SetPoison-OnInitFail");
  SPEC_COV("CheckPoison-Err");
  SPEC_COV("SetPoison-Err");
  SPEC_COV("PoisonFlag-Err");

  sema::Sigma sigma = BuildSigma();
  LowerHarness h;
  h.ctx.sigma = &sigma;
  h.ctx.module_path = {"test"};

  const std::string callee_sym = Mangle(StringOfPath({"test", "callee"}));
  ProcIR callee_sig;
  callee_sig.symbol = callee_sym;
  callee_sig.params = {{cursive0::sema::ParamMode::Move, "x", MakeTypePrim("i32")}};
  callee_sig.ret = MakeTypePrim("i32");
  callee_sig.body = MakeOpaque();
  h.ctx.RegisterProcSig(callee_sig);

  const std::string proc_gen_sym = Mangle(StringOfPath({"test", "proc_gen"}));
  ProcIR proc_gen_sig;
  proc_gen_sig.symbol = proc_gen_sym;
  proc_gen_sig.params = {};
  proc_gen_sig.ret = MakeTypePrim("()");
  proc_gen_sig.body = MakeOpaque();
  h.ctx.RegisterProcSig(proc_gen_sig);

  const std::string proc_user_sym = Mangle(StringOfPath({"test", "proc_user"}));
  h.ctx.RegisterProcModule(proc_user_sym, {"user"});

  const std::string static_sym = StaticSymPath({"main"}, "g");
  h.ctx.RegisterStaticType(static_sym, MakeTypePrim("i32"));
  h.ctx.resolve_name = [](const std::string& name)
      -> std::optional<std::vector<std::string>> {
    if (name == "g") {
      return std::vector<std::string>{"main", "g"};
    }
    return std::nullopt;
  };

  const std::string drop_sym =
      cursive0::codegen::DropGlueSym(MakeTypePrim("i32"), h.ctx);
  ProcIR drop_proc;
  drop_proc.symbol = drop_sym;
  drop_proc.params = {};
  drop_proc.ret = MakeTypePrim("()");
  drop_proc.body = MakeOpaque();

  ProcIR user_proc;
  user_proc.symbol = Mangle(StringOfPath({"test", "user_proc"}));
  user_proc.params = {};
  user_proc.ret = MakeTypePrim("()");
  user_proc.body = MakeOpaque();

  GlobalZero gz;
  gz.symbol = "g";
  gz.size = 4;

  IRDecls decls = {gz, user_proc, drop_proc};
  h.Init(decls);
  assert(h.emitter != nullptr);

  DefinePanicSlot(h);

  auto ptr_valid = MakeTypePtr(MakeTypePrim("i32"), PtrState::Valid);
  auto ptr_null = MakeTypePtr(MakeTypePrim("i32"), PtrState::Null);
  auto ptr_expired = MakeTypePtr(MakeTypePrim("i32"), PtrState::Expired);
  auto ptr_raw = MakeTypeRawPtr(RawPtrQual::Mut, MakeTypePrim("i32"));

  DefinePtrLocal(h, "p_valid", ptr_valid);
  DefinePtrLocal(h, "p_null", ptr_null);
  DefinePtrLocal(h, "p_expired", ptr_expired);
  DefinePtrLocal(h, "p_raw", ptr_raw);

  auto region_type = MakeTypePerm(Permission::Unique,
                                  MakeTypeModalState({"Region"}, "Active"));
  llvm::Type* region_ty = h.emitter->GetLLVMType(region_type);
  DefineLocal(h, "region", region_ty,
              llvm::Constant::getNullValue(region_ty), region_type);

  auto opts_type = MakeTypePath({"RegionOptions"});
  llvm::Type* opts_ty = h.emitter->GetLLVMType(opts_type);
  DefineLocal(h, "opts", opts_ty,
              llvm::Constant::getNullValue(opts_ty), opts_type);

  llvm::Type* ptr_ty = h.emitter->GetOpaquePtr();
  llvm::StructType* dyn_ty = llvm::StructType::get(h.emitter->GetContext(),
                                                   {ptr_ty, ptr_ty});
  auto* dyn_slot = EntryAlloca(h, dyn_ty, "dyn");
  auto* null_ptr = llvm::ConstantPointerNull::get(
      llvm::cast<llvm::PointerType>(ptr_ty));
  auto* dyn_init = llvm::ConstantStruct::get(dyn_ty, {null_ptr, null_ptr});
  h.builder->CreateStore(dyn_init, dyn_slot);
  h.emitter->SetLocal("dyn", dyn_slot);

  IRBindVar bind;
  bind.name = "x";
  bind.type = MakeTypePrim("i32");
  bind.value = MakeImmU32(1);
  h.emitter->EmitIR(cursive0::codegen::MakeIR(std::move(bind)));

  IRStoreVar store;
  store.name = "x";
  store.value = MakeImmU32(2);
  h.emitter->EmitIR(cursive0::codegen::MakeIR(std::move(store)));

  IRStoreVar store_static;
  store_static.name = "g";
  store_static.value = MakeImmU32(13);
  h.emitter->EmitIR(cursive0::codegen::MakeIR(std::move(store_static)));

  IRStoreVarNoDrop store_nodrop;
  store_nodrop.name = "x";
  store_nodrop.value = MakeImmU32(3);
  h.emitter->EmitIR(cursive0::codegen::MakeIR(std::move(store_nodrop)));

  IRReadVar read;
  read.name = "x";
  h.emitter->EmitIR(cursive0::codegen::MakeIR(std::move(read)));

  IRReadPlace read_place;
  read_place.place.repr = "x";
  h.emitter->EmitIR(cursive0::codegen::MakeIR(std::move(read_place)));

  IRWritePlace write_place;
  write_place.place.repr = "x";
  write_place.value = MakeImmU32(4);
  h.emitter->EmitIR(cursive0::codegen::MakeIR(std::move(write_place)));

  IRReadPtr read_ptr;
  read_ptr.ptr = MakeLocal("p_valid");
  read_ptr.result = h.ctx.FreshTempValue("read_ptr");
  h.ctx.RegisterValueType(read_ptr.result, MakeTypePrim("i32"));
  h.emitter->EmitIR(cursive0::codegen::MakeIR(std::move(read_ptr)));

  IRReadPtr read_raw;
  read_raw.ptr = MakeLocal("p_raw");
  read_raw.result = h.ctx.FreshTempValue("read_raw");
  h.ctx.RegisterValueType(read_raw.result, MakeTypePrim("i32"));
  h.emitter->EmitIR(cursive0::codegen::MakeIR(std::move(read_raw)));

  IRWritePtr write_valid;
  write_valid.ptr = MakeLocal("p_valid");
  write_valid.value = MakeImmU32(5);
  h.emitter->EmitIR(cursive0::codegen::MakeIR(std::move(write_valid)));

  IRWritePtr write_null;
  write_null.ptr = MakeLocal("p_null");
  write_null.value = MakeImmU32(6);
  h.emitter->EmitIR(cursive0::codegen::MakeIR(std::move(write_null)));

  IRWritePtr write_expired;
  write_expired.ptr = MakeLocal("p_expired");
  write_expired.value = MakeImmU32(7);
  h.emitter->EmitIR(cursive0::codegen::MakeIR(std::move(write_expired)));

  IRWritePtr write_raw;
  write_raw.ptr = MakeLocal("p_raw");
  write_raw.value = MakeImmU32(8);
  h.emitter->EmitIR(cursive0::codegen::MakeIR(std::move(write_raw)));

  IRStoreGlobal store_global;
  store_global.symbol = "g";
  store_global.value = MakeImmU32(9);
  h.emitter->EmitIR(cursive0::codegen::MakeIR(std::move(store_global)));

  IRAddrOf addr;
  addr.place.repr = "x";
  addr.result = MakeImmU32(0);
  h.emitter->EmitIR(cursive0::codegen::MakeIR(std::move(addr)));

  IRIf if_ir;
  if_ir.cond = MakeImmBool(true);
  if_ir.then_ir = MakeOpaque();
  if_ir.then_value = MakeImmU32(10);
  if_ir.else_ir = MakeOpaque();
  if_ir.else_value = MakeImmU32(11);
  if_ir.result = h.ctx.FreshTempValue("if_res");
  h.ctx.RegisterValueType(if_ir.result, MakeTypePrim("i32"));
  h.emitter->EmitIR(cursive0::codegen::MakeIR(std::move(if_ir)));

  IRBlock block;
  block.setup = MakeOpaque();
  block.body = MakeOpaque();
  block.value = h.ctx.FreshTempValue("block_val");
  h.emitter->EmitIR(cursive0::codegen::MakeIR(std::move(block)));

  IRLoop loop;
  loop.kind = IRLoopKind::Infinite;
  loop.body_ir = MakeOpaque();
  loop.body_value = h.ctx.FreshTempValue("loop_val");
  loop.result = h.ctx.FreshTempValue("loop_res");
  h.emitter->EmitIR(cursive0::codegen::MakeIR(std::move(loop)));

  IRMoveState move_state;
  move_state.place.repr = "x";
  h.emitter->EmitIR(cursive0::codegen::MakeIR(std::move(move_state)));

  IRMatch match;
  match.scrutinee = MakeImmU32(42);
  match.scrutinee_type = MakeTypePrim("i32");
  IRMatchArm arm;
  arm.pattern = MakeWildcardPat();
  arm.body = MakeOpaque();
  arm.value = MakeImmU32(99);
  match.arms = {arm};
  match.result = h.ctx.FreshTempValue("match_res");
  h.ctx.RegisterValueType(match.result, MakeTypePrim("i32"));
  h.emitter->EmitIR(cursive0::codegen::MakeIR(std::move(match)));

  IRRegion region;
  region.owner = MakeLocal("opts");
  region.alias = std::nullopt;
  region.body = MakeOpaque();
  region.value = h.ctx.FreshTempValue("region_val");
  h.emitter->EmitIR(cursive0::codegen::MakeIR(std::move(region)));

  IRAlloc alloc;
  alloc.region = MakeLocal("region");
  alloc.value = MakeUnitOpaque();
  alloc.result = h.ctx.FreshTempValue("alloc_res");
  h.ctx.RegisterValueType(alloc.result, MakeTypePrim("()"));
  h.emitter->EmitIR(cursive0::codegen::MakeIR(std::move(alloc)));

  IRCall call;
  call.callee = MakeSymbol(callee_sym);
  call.args = {MakeImmU32(12)};
  call.result = h.ctx.FreshTempValue("call_res");
  h.ctx.RegisterValueType(call.result, MakeTypePrim("i32"));
  h.emitter->EmitIR(cursive0::codegen::MakeIR(std::move(call)));

  IRCallVTable vcall;
  vcall.base = MakeLocal("dyn");
  vcall.slot = 0;
  vcall.args = {};
  vcall.result = h.ctx.FreshTempValue("vcall_res");
  h.ctx.RegisterValueType(vcall.result, MakeTypePrim("i32"));
  h.emitter->EmitIR(cursive0::codegen::MakeIR(std::move(vcall)));

  IRFrame frame;
  frame.body = MakeOpaque();
  frame.value = h.ctx.FreshTempValue("frame_val");
  h.emitter->EmitIR(cursive0::codegen::MakeIR(std::move(frame)));

  IRBranch branch;
  branch.cond = std::nullopt;
  branch.true_label = "t";
  branch.false_label = "f";
  h.emitter->EmitIR(cursive0::codegen::MakeIR(std::move(branch)));

  IRPhi phi;
  phi.type = MakeTypePrim("i32");
  phi.incoming = {};
  phi.value = h.ctx.FreshTempValue("phi_val");
  h.emitter->EmitIR(cursive0::codegen::MakeIR(std::move(phi)));

  IRCheckPoison check_poison;
  check_poison.module = "test::mod";
  h.emitter->EmitIR(cursive0::codegen::MakeIR(std::move(check_poison)));

  IRClearPanic clear;
  h.emitter->EmitIR(cursive0::codegen::MakeIR(std::move(clear)));

  IRPanicCheck panic_check;
  panic_check.cleanup_ir = MakeOpaque();
  h.emitter->EmitIR(cursive0::codegen::MakeIR(std::move(panic_check)));

  IRLowerPanic lower_panic;
  lower_panic.reason = "Bounds";
  lower_panic.cleanup_ir = MakeOpaque();
  h.emitter->EmitIR(cursive0::codegen::MakeIR(std::move(lower_panic)));

  IRInitPanicHandle init_handle;
  init_handle.module = "test::init";
  init_handle.poison_modules = {};
  h.emitter->EmitIR(cursive0::codegen::MakeIR(std::move(init_handle)));

  IRSeq seq;
  seq.items = {MakeOpaque()};
  h.emitter->EmitIR(cursive0::codegen::MakeIR(std::move(seq)));

  h.emitter->EmitIR(MakeOpaque());

  IRReadPath read_runtime;
  read_runtime.path = {"string"};
  read_runtime.name = "from";
  h.emitter->EmitIR(cursive0::codegen::MakeIR(std::move(read_runtime)));

  IRReadPath read_record;
  read_record.path = {"test"};
  read_record.name = "Rec";
  h.emitter->EmitIR(cursive0::codegen::MakeIR(std::move(read_record)));

  const std::string static_user_sym =
      Mangle(StringOfPath({"test", "StaticUser"}));
  h.ctx.RegisterStaticType(static_user_sym, MakeTypePrim("i32"));
  h.ctx.RegisterStaticModule(static_user_sym, {"test"});
  IRReadPath read_static_user;
  read_static_user.path = {"test"};
  read_static_user.name = "StaticUser";
  h.emitter->EmitIR(cursive0::codegen::MakeIR(std::move(read_static_user)));

  const std::string static_gen_sym =
      Mangle(StringOfPath({"test", "StaticGen"}));
  h.ctx.RegisterStaticType(static_gen_sym, MakeTypePrim("i32"));
  IRReadPath read_static_gen;
  read_static_gen.path = {"test"};
  read_static_gen.name = "StaticGen";
  h.emitter->EmitIR(cursive0::codegen::MakeIR(std::move(read_static_gen)));

  IRReadPath read_proc_user;
  read_proc_user.path = {"test"};
  read_proc_user.name = "proc_user";
  h.emitter->EmitIR(cursive0::codegen::MakeIR(std::move(read_proc_user)));

  IRReadPath read_proc_gen;
  read_proc_gen.path = {"test"};
  read_proc_gen.name = "proc_gen";
  h.emitter->EmitIR(cursive0::codegen::MakeIR(std::move(read_proc_gen)));
}

void TestIRLoweringErrors() {
  SPEC_COV("BindSlot-Err");
  SPEC_COV("Lower-ReadVarIR-Err");
  SPEC_COV("Lower-WritePtrIR-Raw-Err");
  SPEC_COV("LowerIRInstr-Err");

  {
    LowerHarness h;
    h.ctx.module_path = {"test"};
    h.Init({});
    IRReadVar read;
    read.name = "missing";
    h.emitter->EmitIR(cursive0::codegen::MakeIR(std::move(read)));
    h.emitter->EmitIR(MakeOpaque());
    assert(h.ctx.codegen_failed);
  }

  {
    LowerHarness h;
    h.ctx.module_path = {"test"};
    h.Init({});
    auto ptr_raw_imm = MakeTypeRawPtr(RawPtrQual::Imm, MakeTypePrim("i32"));
    DefinePtrLocal(h, "p_raw_imm", ptr_raw_imm);
    IRWritePtr write_raw_err;
    write_raw_err.ptr = MakeLocal("p_raw_imm");
    write_raw_err.value = MakeImmU32(1);
    h.emitter->EmitIR(cursive0::codegen::MakeIR(std::move(write_raw_err)));
    assert(h.ctx.codegen_failed);
  }

  {
    LowerHarness h;
    h.ctx.module_path = {"test"};
    h.Init({});
    IRBindVar bind;
    bind.name = "x";
    bind.type = MakeTypePrim("i32");
    bind.value = MakeImmU32(1);
    h.emitter->EmitIR(cursive0::codegen::MakeIR(std::move(bind)));
    h.ctx.MarkMoved("x");
    IRReadVar read_moved;
    read_moved.name = "x";
    h.emitter->EmitIR(cursive0::codegen::MakeIR(std::move(read_moved)));
    assert(h.ctx.codegen_failed);
  }
}

void TestVTableError() {
  SPEC_COV("EmitVTable-Err");
  SPEC_COV("LowerIRDecl-VTable");

  LowerCtx ctx;
  llvm::LLVMContext llvm_ctx;
  LLVMEmitter emitter(llvm_ctx, "vtable_err");
  GlobalVTable vt;
  vt.symbol = "vt_err";
  vt.header.size = 0;
  vt.header.align = 1;
  vt.header.drop_sym = "missing_drop";
  vt.slots = {"missing_slot"};
  IRDecls decls = {vt};
  emitter.EmitModule(decls, ctx);
  assert(ctx.codegen_failed);
}

}  // namespace

int main() {
  TestRecordCtorLowering();
  TestIRLoweringSuccess();
  TestIRLoweringErrors();
  TestVTableError();
  return 0;
}
