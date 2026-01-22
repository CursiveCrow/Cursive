#include "cursive0/codegen/llvm/llvm_emit.h"
#include "cursive0/codegen/abi/abi.h"
#include "cursive0/codegen/globals.h"
#include "cursive0/codegen/layout/layout.h"
#include "cursive0/core/assert_spec.h"
#include "cursive0/runtime/runtime_interface.h"

// LLVM Includes
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"

#include <string_view>

namespace cursive0::codegen {

// T-LLVM-015: Entrypoint and Context Construction
// Generates the C main function that:
// 1. Initializes the Cursive runtime context
// 2. Clears the panic record
// 3. Calls the user's main procedure
// 4. Runs deinitializers or panics

namespace {

analysis::ScopeContext BuildScope(const LowerCtx* ctx) {
  analysis::ScopeContext scope;
  if (ctx && ctx->sigma) {
    scope.sigma = *ctx->sigma;
    scope.current_module = ctx->module_path;
  }
  return scope;
}

llvm::Value* ByteGEP(LLVMEmitter& emitter,
                     llvm::IRBuilder<>* builder,
                     llvm::Value* base_ptr,
                     std::uint64_t offset) {
  llvm::Value* idx = llvm::ConstantInt::get(
      llvm::Type::getInt64Ty(emitter.GetContext()), offset);
  return builder->CreateGEP(llvm::Type::getInt8Ty(emitter.GetContext()), base_ptr, idx);
}

llvm::Value* LoadAtOffset(LLVMEmitter& emitter,
                          llvm::IRBuilder<>* builder,
                          llvm::Value* base_ptr,
                          std::uint64_t offset,
                          llvm::Type* value_ty) {
  if (!base_ptr || !value_ty) {
    return nullptr;
  }
  llvm::Value* ptr = offset == 0 ? base_ptr : ByteGEP(emitter, builder, base_ptr, offset);
  return builder->CreateLoad(value_ty, ptr);
}

IRValue StringImmediate(std::string_view text) {
  IRValue value;
  value.kind = IRValue::Kind::Immediate;
  value.name = "\"" + std::string(text) + "\"";
  value.bytes.assign(text.begin(), text.end());
  return value;
}

IRPtr EmitRuntimeTrace(std::string_view rule_id) {
  if (rule_id.empty()) {
    return EmptyIR();
  }
  IRCall call;
  call.callee.kind = IRValue::Kind::Symbol;
  call.callee.name = RuntimeSpecTraceEmitSym();
  call.args.push_back(StringImmediate(rule_id));
  call.args.push_back(StringImmediate(""));
  return MakeIR(std::move(call));
}

}  // namespace

void LLVMEmitter::EmitEntryPoint() {
  SPEC_RULE("EntrySym-Decl");
  SPEC_RULE("ContextInitSym-Decl");
  SPEC_RULE("EntryStub-Decl");

  const std::string entry_sym = "main";
  if (entry_sym.empty()) {
    SPEC_RULE("EntrySym-Err");
    if (LowerCtx* ctx = GetCurrentCtx()) {
      ctx->ReportCodegenFailure();
    }
    return;
  }

  if (module_->getFunction(entry_sym)) {
    return;
  }

  llvm::Type* int_ty = llvm::Type::getInt32Ty(context_);
  llvm::FunctionType* ft = llvm::FunctionType::get(int_ty, {}, false);

  llvm::Function* main_fn = llvm::Function::Create(
      ft,
      llvm::Function::ExternalLinkage,
      entry_sym,
      module_.get());
  if (!main_fn) {
    SPEC_RULE("EntryStub-Err");
    if (LowerCtx* ctx = GetCurrentCtx()) {
      ctx->ReportCodegenFailure();
    }
    return;
  }
  main_fn->setCallingConv(llvm::CallingConv::C);

  llvm::BasicBlock* entry_bb = llvm::BasicBlock::Create(context_, "entry", main_fn);
  auto* builder = static_cast<llvm::IRBuilder<>*>(builder_.get());
  builder->SetInsertPoint(entry_bb);

  ClearLocals();
  ClearTempValues();
  ClearSymbolAliases();

  LowerCtx* ctx = GetCurrentCtx();
  if (!ctx || !ctx->main_symbol.has_value()) {
    if (ctx) {
      ctx->ReportCodegenFailure();
    }
    SPEC_RULE("EntryStub-Err");
    builder->CreateRet(llvm::ConstantInt::get(int_ty, 0));
    return;
  }

  llvm::IRBuilder<> entry_builder(&main_fn->getEntryBlock(),
                                  main_fn->getEntryBlock().begin());
  llvm::Type* panic_rec_ty = GetLLVMType(PanicRecordType());
  llvm::AllocaInst* panic_record = entry_builder.CreateAlloca(
      panic_rec_ty, nullptr, "panic_record");
  llvm::AllocaInst* panic_slot = entry_builder.CreateAlloca(
      GetOpaquePtr(), nullptr, std::string(kPanicOutName));
  builder->CreateStore(panic_record, panic_slot);
  SetLocal(std::string(kPanicOutName), panic_slot);

  EmitIR(EmitRuntimeTrace("EntryStub-Decl"));

  // Call runtime context_init to construct Context
  IRValue ctx_value;
  ctx_value.kind = IRValue::Kind::Opaque;
  ctx_value.name = "__entry_ctx";

  EmitIR(EmitRuntimeTrace("ContextInitSym-Decl"));

  IRCall init_call;
  init_call.callee.kind = IRValue::Kind::Symbol;
  init_call.callee.name = ContextInitSym();
  init_call.result = ctx_value;
  EmitIR(MakeIR(std::move(init_call)));

  EmitIR(MakeIR(IRClearPanic{}));

  // Call user main with [ctx, __panic]
  IRValue panic_arg;
  panic_arg.kind = IRValue::Kind::Local;
  panic_arg.name = std::string(kPanicOutName);

  IRValue ret_value;
  ret_value.kind = IRValue::Kind::Opaque;
  ret_value.name = "__entry_ret";

  IRCall main_call;
  main_call.callee.kind = IRValue::Kind::Symbol;
  main_call.callee.name = *ctx->main_symbol;
  main_call.args = {ctx_value, panic_arg};
  main_call.result = ret_value;
  EmitIR(MakeIR(std::move(main_call)));

  // Inspect panic record
  const auto scope = BuildScope(ctx);
  std::vector<analysis::TypeRef> fields;
  fields.push_back(analysis::MakeTypePrim("bool"));
  fields.push_back(analysis::MakeTypePrim("u32"));
  const auto layout = RecordLayoutOf(scope, fields);
  if (!layout.has_value() || layout->offsets.size() < 2) {
    ctx->ReportCodegenFailure();
    SPEC_RULE("EntryStub-Err");
    builder->CreateRet(llvm::ConstantInt::get(int_ty, 0));
    return;
  }

  llvm::Type* bool_ty = GetLLVMType(analysis::MakeTypePrim("bool"));
  llvm::Type* u32_ty = GetLLVMType(analysis::MakeTypePrim("u32"));
  llvm::Value* flag = LoadAtOffset(*this, builder, panic_record,
                                   layout->offsets[0], bool_ty);
  llvm::Value* code = LoadAtOffset(*this, builder, panic_record,
                                   layout->offsets[1], u32_ty);
  if (!flag || !code) {
    ctx->ReportCodegenFailure();
    SPEC_RULE("EntryStub-Err");
    builder->CreateRet(llvm::ConstantInt::get(int_ty, 0));
    return;
  }

  llvm::Value* is_panic = builder->CreateICmpNE(
      flag, llvm::Constant::getNullValue(flag->getType()));

  llvm::BasicBlock* panic_bb = llvm::BasicBlock::Create(context_, "panic_fail", main_fn);
  llvm::BasicBlock* ok_bb = llvm::BasicBlock::Create(context_, "panic_ok", main_fn);
  builder->CreateCondBr(is_panic, panic_bb, ok_bb);

  builder->SetInsertPoint(panic_bb);
  if (llvm::Function* panic_fn = GetFunction(RuntimePanicSym())) {
    builder->CreateCall(panic_fn, {code});
    builder->CreateUnreachable();
  } else {
    ctx->ReportCodegenFailure();
    SPEC_RULE("EntryStub-Err");
    builder->CreateRet(llvm::ConstantInt::get(int_ty, 0));
  }

  builder->SetInsertPoint(ok_bb);
  IRPtr deinit_ir = EmitDeinitPlan(ctx->init_order, *ctx);
  if (deinit_ir) {
    EmitIR(deinit_ir);
  }

  llvm::Value* ret = GetTempValue(ret_value);
  if (!ret) {
    ret = llvm::ConstantInt::get(int_ty, 0);
  }
  builder->CreateRet(ret);
}

} // namespace cursive0::codegen
