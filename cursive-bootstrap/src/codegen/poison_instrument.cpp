#include "cursive0/codegen/llvm/llvm_emit.h"
#include "cursive0/codegen/checks.h"
#include "cursive0/codegen/abi/abi.h"
#include "cursive0/codegen/layout/layout.h"
#include "cursive0/core/assert_spec.h"
#include "cursive0/core/symbols.h"
#include "cursive0/runtime/runtime_interface.h"
#include "cursive0/analysis/types/types.h"

// LLVM Includes
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"

namespace cursive0::codegen {

// T-LLVM-016: Poisoning Instrumentation
// Implements poison flag tracking for detecting use of uninitialized/moved memory.

namespace {

analysis::ScopeContext BuildScope(const LowerCtx* ctx) {
  analysis::ScopeContext scope;
  if (ctx && ctx->sigma) {
    scope.sigma = *ctx->sigma;
    scope.current_module = ctx->module_path;
  }
  return scope;
}

std::vector<std::string> SplitModulePath(const std::string& module_name) {
  std::vector<std::string> out;
  std::size_t start = 0;
  while (start <= module_name.size()) {
    std::size_t pos = module_name.find("::", start);
    if (pos == std::string::npos) {
      out.push_back(module_name.substr(start));
      break;
    }
    out.push_back(module_name.substr(start, pos - start));
    start = pos + 2;
  }
  if (out.size() == 1 && out[0].empty()) {
    out.clear();
  }
  return out;
}

// Helper to get or create the poison flag global for a module
static llvm::GlobalVariable* GetOrCreatePoisonFlag(LLVMEmitter& emitter,
                                                   const std::string& module_name) {
  std::vector<std::string> module_path = SplitModulePath(module_name);
  std::vector<std::string> full = {"cursive", "runtime", "poison"};
  full.insert(full.end(), module_path.begin(), module_path.end());
  const std::string sym = core::Mangle(core::StringOfPath(full));

  if (auto* existing = emitter.GetModule().getGlobalVariable(sym, true)) {
    return existing;
  }

  bool define_flag = true;
  if (LowerCtx* ctx = emitter.GetCurrentCtx()) {
    define_flag = (core::StringOfPath(ctx->module_path) == module_name);
  }
  auto* bool_ty = emitter.GetLLVMType(analysis::MakeTypePrim("bool"));
  auto* init = define_flag ? llvm::Constant::getNullValue(bool_ty) : nullptr;
  auto* flag = new llvm::GlobalVariable(
      emitter.GetModule(),
      bool_ty,
      false,
      llvm::GlobalValue::ExternalLinkage,
      init,
      sym);
  return flag;
}

llvm::Value* ByteGEP(LLVMEmitter& emitter,
                     llvm::IRBuilder<>* builder,
                     llvm::Value* base_ptr,
                     std::uint64_t offset) {
  llvm::Value* idx = llvm::ConstantInt::get(llvm::Type::getInt64Ty(emitter.GetContext()), offset);
  return builder->CreateGEP(llvm::Type::getInt8Ty(emitter.GetContext()), base_ptr, idx);
}

void StoreAtOffset(LLVMEmitter& emitter,
                   llvm::IRBuilder<>* builder,
                   llvm::Value* base_ptr,
                   std::uint64_t offset,
                   llvm::Value* value) {
  if (!base_ptr || !value) {
    return;
  }
  llvm::Value* ptr = offset == 0 ? base_ptr : ByteGEP(emitter, builder, base_ptr, offset);
  builder->CreateStore(value, ptr);
}

llvm::Value* LoadPanicOutPtr(LLVMEmitter& emitter,
                             llvm::IRBuilder<>* builder) {
  llvm::Value* slot = emitter.GetLocal(std::string(kPanicOutName));
  if (!slot) {
    return nullptr;
  }
  if (auto* alloca = llvm::dyn_cast<llvm::AllocaInst>(slot)) {
    return builder->CreateLoad(alloca->getAllocatedType(), alloca);
  }
  return builder->CreateLoad(emitter.GetOpaquePtr(), slot);
}

void StorePanicRecord(LLVMEmitter& emitter,
                      llvm::IRBuilder<>* builder,
                      std::uint16_t code) {
  LowerCtx* ctx = emitter.GetCurrentCtx();
  if (!ctx) {
    return;
  }
  llvm::Value* ptr = LoadPanicOutPtr(emitter, builder);
  if (!ptr) {
    return;
  }
  const auto scope = BuildScope(ctx);
  std::vector<analysis::TypeRef> fields;
  fields.push_back(analysis::MakeTypePrim("bool"));
  fields.push_back(analysis::MakeTypePrim("u32"));
  const auto layout = RecordLayoutOf(scope, fields);
  if (!layout.has_value() || layout->offsets.size() < 2) {
    return;
  }
  llvm::LLVMContext& ctx_ll = emitter.GetContext();
  llvm::Value* panic_val = llvm::ConstantInt::get(llvm::Type::getInt8Ty(ctx_ll), 1);
  llvm::Value* code_val = llvm::ConstantInt::get(llvm::Type::getInt32Ty(ctx_ll), code);
  StoreAtOffset(emitter, builder, ptr, layout->offsets[0], panic_val);
  StoreAtOffset(emitter, builder, ptr, layout->offsets[1], code_val);
}

void EmitReturn(LLVMEmitter& emitter, llvm::IRBuilder<>* builder) {
  llvm::Function* func = builder->GetInsertBlock()->getParent();
  llvm::Type* ret_ty = func->getReturnType();
  if (ret_ty->isVoidTy()) {
    builder->CreateRetVoid();
    return;
  }
  builder->CreateRet(llvm::Constant::getNullValue(ret_ty));
}

}  // namespace

void LLVMEmitter::EmitPoisonCheck(const std::string& module_name) {
  SPEC_RULE("PoisonCheck");
  SPEC_RULE("PoisonFlag-Get");

  llvm::GlobalVariable* flag = GetOrCreatePoisonFlag(*this, module_name);

  auto* builder = static_cast<llvm::IRBuilder<>*>(builder_.get());
  llvm::BasicBlock* current = builder->GetInsertBlock();
  if (!current) return;

  llvm::Function* func = current->getParent();
  if (!func) return;

  llvm::Type* bool_ty = GetLLVMType(analysis::MakeTypePrim("bool"));
  llvm::Value* flag_val = builder->CreateLoad(bool_ty, flag);
  llvm::Value* poisoned = builder->CreateICmpNE(
      flag_val, llvm::Constant::getNullValue(flag_val->getType()));

  llvm::BasicBlock* poison_bb = llvm::BasicBlock::Create(context_, "poisoned", func);
  llvm::BasicBlock* ok_bb = llvm::BasicBlock::Create(context_, "not_poisoned", func);

  builder->CreateCondBr(poisoned, poison_bb, ok_bb);

  builder->SetInsertPoint(poison_bb);
  StorePanicRecord(*this, builder, PanicCode(PanicReason::InitPanic));
  EmitReturn(*this, builder);

  builder->SetInsertPoint(ok_bb);
}

// Set poison flag for a binding
void EmitSetPoison(LLVMEmitter& emitter, const std::string& module_name, bool value) {
  SPEC_RULE("PoisonFlag-Set");

  llvm::GlobalVariable* flag = GetOrCreatePoisonFlag(emitter, module_name);

  auto* builder = static_cast<llvm::IRBuilder<>*>(emitter.GetBuilderRaw());
  llvm::Type* bool_ty = emitter.GetLLVMType(analysis::MakeTypePrim("bool"));
  llvm::Value* val = llvm::ConstantInt::get(bool_ty, value ? 1 : 0);
  builder->CreateStore(val, flag);
}

// Clear poison flag after reassignment
void EmitClearPoison(LLVMEmitter& emitter, const std::string& module_name) {
  SPEC_RULE("PoisonFlag-Clear");
  EmitSetPoison(emitter, module_name, false);
}

// Set poison flag on move
void EmitPoisonOnMove(LLVMEmitter& emitter, const std::string& module_name) {
  SPEC_RULE("PoisonFlag-OnMove");
  EmitSetPoison(emitter, module_name, true);
}

} // namespace cursive0::codegen
