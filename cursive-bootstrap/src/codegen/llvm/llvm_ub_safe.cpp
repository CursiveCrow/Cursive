#include "cursive0/codegen/llvm/llvm_emit.h"
#include "cursive0/codegen/abi/abi.h"
#include "cursive0/codegen/checks.h"
#include "cursive0/codegen/layout/layout.h"
#include "cursive0/core/assert_spec.h"
#include "cursive0/runtime/runtime_interface.h"
#include "cursive0/analysis/types/types.h"

// LLVM Includes
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/Module.h"

namespace cursive0::codegen {

// T-LLVM-004: UB and Poison Avoidance

namespace {

analysis::ScopeContext BuildScope(const LowerCtx* ctx) {
  analysis::ScopeContext scope;
  if (ctx && ctx->sigma) {
    scope.sigma = *ctx->sigma;
    scope.current_module = ctx->module_path;
  }
  return scope;
}

llvm::Value* CoerceInteger(llvm::IRBuilder<>* builder,
                           llvm::Value* value,
                           llvm::Type* target,
                           bool is_unsigned) {
  if (!value || !target) {
    return value;
  }
  llvm::Type* src = value->getType();
  if (src == target) {
    return value;
  }
  if (!src->isIntegerTy() || !target->isIntegerTy()) {
    return value;
  }
  return is_unsigned ? builder->CreateZExtOrTrunc(value, target)
                     : builder->CreateSExtOrTrunc(value, target);
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

void EmitPanic(LLVMEmitter& emitter, PanicReason reason) {
  auto* builder = static_cast<llvm::IRBuilder<>*>(emitter.GetBuilderRaw());
  StorePanicRecord(emitter, builder, PanicCode(reason));
  EmitReturn(emitter, builder);
}

llvm::Value* EmitCheckedWithOverflow(LLVMEmitter& emitter,
                                     llvm::Value* result_struct,
                                     PanicReason reason) {
  auto* builder = static_cast<llvm::IRBuilder<>*>(emitter.GetBuilderRaw());
  llvm::LLVMContext& ctx = emitter.GetContext();

  llvm::Value* val = builder->CreateExtractValue(result_struct, 0);
  llvm::Value* overflow = builder->CreateExtractValue(result_struct, 1);
  val = builder->CreateFreeze(val);

  llvm::Function* func = builder->GetInsertBlock()->getParent();
  llvm::BasicBlock* ok_bb = llvm::BasicBlock::Create(ctx, "op_ok", func);
  llvm::BasicBlock* fail_bb = llvm::BasicBlock::Create(ctx, "op_fail", func);
  builder->CreateCondBr(overflow, fail_bb, ok_bb);

  builder->SetInsertPoint(fail_bb);
  EmitPanic(emitter, reason);

  builder->SetInsertPoint(ok_bb);
  return val;
}

}  // namespace

llvm::Value* LLVMEmitter::EmitCheckedAdd(llvm::Value* lhs, llvm::Value* rhs, bool is_signed) {
  SPEC_RULE("LLVMUBSafe-Add");

  auto* builder = static_cast<llvm::IRBuilder<>*>(builder_.get());
  llvm::Intrinsic::ID id = is_signed ? llvm::Intrinsic::sadd_with_overflow
                                      : llvm::Intrinsic::uadd_with_overflow;
  llvm::Function* intrinsic = llvm::Intrinsic::getDeclaration(module_.get(), id, {lhs->getType()});
  llvm::Value* result = builder->CreateCall(intrinsic, {lhs, rhs});
  return EmitCheckedWithOverflow(*this, result, PanicReason::Overflow);
}

llvm::Value* LLVMEmitter::EmitCheckedSub(llvm::Value* lhs, llvm::Value* rhs, bool is_signed) {
  SPEC_RULE("LLVMUBSafe-Sub");

  auto* builder = static_cast<llvm::IRBuilder<>*>(builder_.get());
  llvm::Intrinsic::ID id = is_signed ? llvm::Intrinsic::ssub_with_overflow
                                      : llvm::Intrinsic::usub_with_overflow;
  llvm::Function* intrinsic = llvm::Intrinsic::getDeclaration(module_.get(), id, {lhs->getType()});
  llvm::Value* result = builder->CreateCall(intrinsic, {lhs, rhs});
  return EmitCheckedWithOverflow(*this, result, PanicReason::Overflow);
}

llvm::Value* LLVMEmitter::EmitCheckedMul(llvm::Value* lhs, llvm::Value* rhs, bool is_signed) {
  SPEC_RULE("LLVMUBSafe-Mul");

  auto* builder = static_cast<llvm::IRBuilder<>*>(builder_.get());
  llvm::Intrinsic::ID id = is_signed ? llvm::Intrinsic::smul_with_overflow
                                      : llvm::Intrinsic::umul_with_overflow;
  llvm::Function* intrinsic = llvm::Intrinsic::getDeclaration(module_.get(), id, {lhs->getType()});
  llvm::Value* result = builder->CreateCall(intrinsic, {lhs, rhs});
  return EmitCheckedWithOverflow(*this, result, PanicReason::Overflow);
}

llvm::Value* LLVMEmitter::EmitCheckedDiv(llvm::Value* lhs, llvm::Value* rhs, bool is_signed) {
  SPEC_RULE("LLVMUBSafe-Div");

  auto* builder = static_cast<llvm::IRBuilder<>*>(builder_.get());
  if (!lhs || !rhs) {
    return nullptr;
  }
  if (lhs->getType()->isFloatingPointTy() || rhs->getType()->isFloatingPointTy()) {
    return builder->CreateFDiv(lhs, rhs);
  }
  if (!lhs->getType()->isIntegerTy() || !rhs->getType()->isIntegerTy()) {
    return nullptr;
  }
  rhs = CoerceInteger(builder, rhs, lhs->getType(), !is_signed);
  llvm::Value* div = is_signed ? builder->CreateSDiv(lhs, rhs)
                               : builder->CreateUDiv(lhs, rhs);
  return builder->CreateFreeze(div);
}

llvm::Value* LLVMEmitter::EmitCheckedRem(llvm::Value* lhs, llvm::Value* rhs, bool is_signed) {
  SPEC_RULE("LLVMUBSafe-Rem");

  auto* builder = static_cast<llvm::IRBuilder<>*>(builder_.get());
  if (!lhs || !rhs) {
    return nullptr;
  }
  if (lhs->getType()->isFloatingPointTy() || rhs->getType()->isFloatingPointTy()) {
    return builder->CreateFRem(lhs, rhs);
  }
  if (!lhs->getType()->isIntegerTy() || !rhs->getType()->isIntegerTy()) {
    return nullptr;
  }
  rhs = CoerceInteger(builder, rhs, lhs->getType(), !is_signed);
  llvm::Value* rem = is_signed ? builder->CreateSRem(lhs, rhs)
                               : builder->CreateURem(lhs, rhs);
  return builder->CreateFreeze(rem);
}

llvm::Value* LLVMEmitter::EmitCheckedShl(llvm::Value* lhs, llvm::Value* rhs) {
  SPEC_RULE("LLVMUBSafe-Shl");

  auto* builder = static_cast<llvm::IRBuilder<>*>(builder_.get());
  if (!lhs || !rhs) {
    return nullptr;
  }
  if (!lhs->getType()->isIntegerTy() || !rhs->getType()->isIntegerTy()) {
    return nullptr;
  }
  rhs = CoerceInteger(builder, rhs, lhs->getType(), true);
  llvm::Value* result = builder->CreateShl(lhs, rhs);
  return builder->CreateFreeze(result);
}

llvm::Value* LLVMEmitter::EmitCheckedShr(llvm::Value* lhs, llvm::Value* rhs, bool is_signed) {
  SPEC_RULE("LLVMUBSafe-Shr");

  auto* builder = static_cast<llvm::IRBuilder<>*>(builder_.get());
  if (!lhs || !rhs) {
    return nullptr;
  }
  if (!lhs->getType()->isIntegerTy() || !rhs->getType()->isIntegerTy()) {
    return nullptr;
  }
  rhs = CoerceInteger(builder, rhs, lhs->getType(), true);
  llvm::Value* result = is_signed ? builder->CreateAShr(lhs, rhs)
                                  : builder->CreateLShr(lhs, rhs);
  return builder->CreateFreeze(result);
}

} // namespace cursive0::codegen
