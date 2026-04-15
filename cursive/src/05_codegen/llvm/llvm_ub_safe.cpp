// =============================================================================
// MIGRATION MAPPING: llvm_ub_safe.cpp
// =============================================================================
//
// SPEC REFERENCE: CursiveSpecification.md
//   - Section 6.12 LLVM 21 Backend Requirements
//   - UB avoidance rules
//   - Poison value handling
//   - Safe arithmetic operations
//
// SOURCE FILE: cursive-bootstrap/src/04_codegen/llvm/llvm_ub_safe.cpp
//   - Lines 1-100: UB and poison avoidance helpers
//   - Lines 18-47: CoerceInteger helper
//   - Lines 49-67: ByteGEP, StoreAtOffset helpers
//   - Lines 69-100: LoadPanicOutPtr, StorePanicRecord
//
// DEPENDENCIES:
//   - cursive/include/05_codegen/llvm/llvm_ub_safe.h
//   - cursive/include/05_codegen/llvm/llvm_emit.h (LLVMEmitter)
//   - cursive/include/05_codegen/checks/panic.h
//   - llvm/IR/IRBuilder.h
//   - llvm/IR/Instructions.h
//   - llvm/IR/Intrinsics.h
// =============================================================================

#include "05_codegen/llvm/llvm_ub_safe.h"

#include "00_core/spec_trace.h"
#include "05_codegen/checks/checks.h"
#include "05_codegen/checks/panic.h"
#include "05_codegen/layout/layout.h"
#include "05_codegen/llvm/llvm_emit.h"

#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/Module.h"

namespace cursive::codegen {

namespace {

const analysis::ScopeContext& BuildScope(const LowerCtx* ctx) {
  static const analysis::ScopeContext kEmptyScope{};

  struct ScopeCache {
    const LowerCtx* ctx = nullptr;
    const analysis::Sigma* sigma = nullptr;
    std::vector<std::string> module_path;
    analysis::ScopeContext scope;
  };

  thread_local ScopeCache cache;
  if (!ctx || !ctx->sigma) {
    return kEmptyScope;
  }

  if (cache.ctx != ctx || cache.sigma != ctx->sigma ||
      cache.module_path != ctx->module_path) {
    cache.ctx = ctx;
    cache.sigma = ctx->sigma;
    cache.module_path = ctx->module_path;
    cache.scope = analysis::ScopeContext{};
    cache.scope.sigma = *ctx->sigma;
    cache.scope.sigma_source = ctx->sigma;
    cache.scope.current_module = ctx->module_path;
  }

  return cache.scope;
}

llvm::Value* LoadPanicOutPtr(LLVMEmitter& emitter, llvm::IRBuilder<>* builder) {
  llvm::Value* slot = emitter.GetLocal(std::string(kPanicOutName));
  if (!slot) {
    return emitter.GetHostedSessionPanicPtr();
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
  const auto& scope = BuildScope(ctx);

  // Panic record layout: { panicked: bool, code: u32 }
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

  EmitStoreAtOffset(emitter, builder, ptr, layout->offsets[0], panic_val);
  EmitStoreAtOffset(emitter, builder, ptr, layout->offsets[1], code_val);
}

void EmitReturnDefault(LLVMEmitter& emitter, llvm::IRBuilder<>* builder) {
  llvm::Function* func = builder->GetInsertBlock()->getParent();
  llvm::Type* ret_ty = func->getReturnType();
  if (ret_ty->isVoidTy()) {
    builder->CreateRetVoid();
    return;
  }
  builder->CreateRet(llvm::Constant::getNullValue(ret_ty));
}

void EmitPanicImpl(LLVMEmitter& emitter, PanicReason reason) {
  auto* builder = static_cast<llvm::IRBuilder<>*>(emitter.GetBuilderRaw());
  StorePanicRecord(emitter, builder, PanicCode(reason));
  EmitReturnDefault(emitter, builder);
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
  EmitPanicImpl(emitter, reason);

  builder->SetInsertPoint(ok_bb);
  return val;
}

}  // namespace

// =============================================================================
// §6.12.4 UB and Poison Avoidance
// =============================================================================

// -----------------------------------------------------------------------------
// Integer Type Coercion
// -----------------------------------------------------------------------------

llvm::Value* CoerceInteger(llvm::IRBuilderBase* builder_base,
                           llvm::Value* value,
                           llvm::Type* target,
                           bool is_unsigned) {
  if (!value || !target) {
    return value;
  }

  auto* builder = static_cast<llvm::IRBuilder<>*>(builder_base);

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

// -----------------------------------------------------------------------------
// Checked Arithmetic (T-LLVM-004)
// -----------------------------------------------------------------------------

llvm::Value* EmitCheckedAdd(LLVMEmitter& emitter,
                            llvm::Value* lhs,
                            llvm::Value* rhs,
                            bool is_signed) {
  SPEC_RULE("LLVMUBSafe-Add");

  auto* builder = static_cast<llvm::IRBuilder<>*>(emitter.GetBuilderRaw());
  llvm::Module* module = &emitter.GetModule();

  llvm::Intrinsic::ID id = is_signed ? llvm::Intrinsic::sadd_with_overflow
                                     : llvm::Intrinsic::uadd_with_overflow;
  llvm::Function* intrinsic = llvm::Intrinsic::getDeclaration(module, id, {lhs->getType()});
  llvm::Value* result = builder->CreateCall(intrinsic, {lhs, rhs});
  return EmitCheckedWithOverflow(emitter, result, PanicReason::Overflow);
}

llvm::Value* EmitCheckedSub(LLVMEmitter& emitter,
                            llvm::Value* lhs,
                            llvm::Value* rhs,
                            bool is_signed) {
  SPEC_RULE("LLVMUBSafe-Sub");

  auto* builder = static_cast<llvm::IRBuilder<>*>(emitter.GetBuilderRaw());
  llvm::Module* module = &emitter.GetModule();

  llvm::Intrinsic::ID id = is_signed ? llvm::Intrinsic::ssub_with_overflow
                                     : llvm::Intrinsic::usub_with_overflow;
  llvm::Function* intrinsic = llvm::Intrinsic::getDeclaration(module, id, {lhs->getType()});
  llvm::Value* result = builder->CreateCall(intrinsic, {lhs, rhs});
  return EmitCheckedWithOverflow(emitter, result, PanicReason::Overflow);
}

llvm::Value* EmitCheckedMul(LLVMEmitter& emitter,
                            llvm::Value* lhs,
                            llvm::Value* rhs,
                            bool is_signed) {
  SPEC_RULE("LLVMUBSafe-Mul");

  auto* builder = static_cast<llvm::IRBuilder<>*>(emitter.GetBuilderRaw());
  llvm::Module* module = &emitter.GetModule();

  llvm::Intrinsic::ID id = is_signed ? llvm::Intrinsic::smul_with_overflow
                                     : llvm::Intrinsic::umul_with_overflow;
  llvm::Function* intrinsic = llvm::Intrinsic::getDeclaration(module, id, {lhs->getType()});
  llvm::Value* result = builder->CreateCall(intrinsic, {lhs, rhs});
  return EmitCheckedWithOverflow(emitter, result, PanicReason::Overflow);
}

llvm::Value* EmitCheckedDiv(LLVMEmitter& emitter,
                            llvm::Value* lhs,
                            llvm::Value* rhs,
                            bool is_signed) {
  SPEC_RULE("LLVMUBSafe-Div");

  auto* builder = static_cast<llvm::IRBuilder<>*>(emitter.GetBuilderRaw());
  if (!lhs || !rhs) {
    return nullptr;
  }

  // Floating point division has IEEE semantics (no UB)
  if (lhs->getType()->isFloatingPointTy() || rhs->getType()->isFloatingPointTy()) {
    return builder->CreateFDiv(lhs, rhs);
  }

  // Integer division - coerce types and freeze result
  if (!lhs->getType()->isIntegerTy() || !rhs->getType()->isIntegerTy()) {
    return nullptr;
  }

  rhs = CoerceInteger(builder, rhs, lhs->getType(), !is_signed);
  llvm::Value* div = is_signed ? builder->CreateSDiv(lhs, rhs)
                               : builder->CreateUDiv(lhs, rhs);
  return builder->CreateFreeze(div);
}

llvm::Value* EmitCheckedRem(LLVMEmitter& emitter,
                            llvm::Value* lhs,
                            llvm::Value* rhs,
                            bool is_signed) {
  SPEC_RULE("LLVMUBSafe-Rem");

  auto* builder = static_cast<llvm::IRBuilder<>*>(emitter.GetBuilderRaw());
  if (!lhs || !rhs) {
    return nullptr;
  }

  // Floating point remainder has IEEE semantics
  if (lhs->getType()->isFloatingPointTy() || rhs->getType()->isFloatingPointTy()) {
    return builder->CreateFRem(lhs, rhs);
  }

  // Integer remainder - coerce types and freeze result
  if (!lhs->getType()->isIntegerTy() || !rhs->getType()->isIntegerTy()) {
    return nullptr;
  }

  rhs = CoerceInteger(builder, rhs, lhs->getType(), !is_signed);
  llvm::Value* rem = is_signed ? builder->CreateSRem(lhs, rhs)
                               : builder->CreateURem(lhs, rhs);
  return builder->CreateFreeze(rem);
}

llvm::Value* EmitCheckedShl(LLVMEmitter& emitter,
                            llvm::Value* lhs,
                            llvm::Value* rhs) {
  SPEC_RULE("LLVMUBSafe-Shl");

  auto* builder = static_cast<llvm::IRBuilder<>*>(emitter.GetBuilderRaw());
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

llvm::Value* EmitCheckedShr(LLVMEmitter& emitter,
                            llvm::Value* lhs,
                            llvm::Value* rhs,
                            bool is_signed) {
  SPEC_RULE("LLVMUBSafe-Shr");

  auto* builder = static_cast<llvm::IRBuilder<>*>(emitter.GetBuilderRaw());
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

// =============================================================================
// §6.12.5 Memory Intrinsics (T-LLVM-005)
// =============================================================================

void EmitMemCpy(LLVMEmitter& emitter,
                llvm::Value* dst,
                llvm::Value* src,
                llvm::Value* size,
                std::uint64_t align) {
  SPEC_RULE("MemIntrinsics-Copy");

  // Per spec: overlap is unknown, use memmove
  auto* builder = static_cast<llvm::IRBuilder<>*>(emitter.GetBuilderRaw());
  builder->CreateMemMove(dst, llvm::MaybeAlign(align), src, llvm::MaybeAlign(align), size);
}

void EmitMemSet(LLVMEmitter& emitter,
                llvm::Value* dst,
                llvm::Value* val,
                llvm::Value* size,
                std::uint64_t align) {
  SPEC_RULE("MemIntrinsics-Set");

  auto* builder = static_cast<llvm::IRBuilder<>*>(emitter.GetBuilderRaw());
  builder->CreateMemSet(dst, val, size, llvm::MaybeAlign(align));
}

void EmitMemMove(LLVMEmitter& emitter,
                 llvm::Value* dst,
                 llvm::Value* src,
                 llvm::Value* size,
                 std::uint64_t align) {
  SPEC_RULE("MemIntrinsics-Move");

  auto* builder = static_cast<llvm::IRBuilder<>*>(emitter.GetBuilderRaw());
  builder->CreateMemMove(dst, llvm::MaybeAlign(align), src, llvm::MaybeAlign(align), size);
}

// -----------------------------------------------------------------------------
// Pointer Arithmetic Helpers
// -----------------------------------------------------------------------------

llvm::Value* EmitByteGEP(LLVMEmitter& emitter,
                         llvm::IRBuilderBase* builder_base,
                         llvm::Value* base_ptr,
                         std::uint64_t offset) {
  if (!builder_base || !base_ptr) {
    return nullptr;
  }

  auto* builder = static_cast<llvm::IRBuilder<>*>(builder_base);
  llvm::LLVMContext& ctx = emitter.GetContext();
  llvm::Value* idx = llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx), offset);
  return builder->CreateGEP(llvm::Type::getInt8Ty(ctx), base_ptr, idx);
}

void EmitStoreAtOffset(LLVMEmitter& emitter,
                       llvm::IRBuilderBase* builder_base,
                       llvm::Value* base_ptr,
                       std::uint64_t offset,
                       llvm::Value* value) {
  if (!builder_base || !base_ptr || !value) {
    return;
  }

  auto* builder = static_cast<llvm::IRBuilder<>*>(builder_base);
  llvm::Value* ptr = offset == 0 ? base_ptr : EmitByteGEP(emitter, builder, base_ptr, offset);
  builder->CreateStore(value, ptr);
}

llvm::Value* EmitLoadAtOffset(LLVMEmitter& emitter,
                              llvm::IRBuilderBase* builder_base,
                              llvm::Value* base_ptr,
                              std::uint64_t offset,
                              llvm::Type* load_type) {
  if (!builder_base || !base_ptr || !load_type) {
    return nullptr;
  }

  auto* builder = static_cast<llvm::IRBuilder<>*>(builder_base);
  llvm::Value* ptr = offset == 0 ? base_ptr : EmitByteGEP(emitter, builder, base_ptr, offset);
  return builder->CreateLoad(load_type, ptr);
}

// -----------------------------------------------------------------------------
// Poison and Freeze
// -----------------------------------------------------------------------------

llvm::Value* EmitFreeze(llvm::IRBuilderBase* builder_base, llvm::Value* value) {
  if (!builder_base || !value) {
    return value;
  }

  auto* builder = static_cast<llvm::IRBuilder<>*>(builder_base);
  return builder->CreateFreeze(value);
}

bool MightBePoison(llvm::Value* value) {
  // Conservative approximation: operations that can produce poison
  if (!value) {
    return false;
  }

  // Arithmetic operations can produce poison with nsw/nuw flags
  if (auto* binop = llvm::dyn_cast<llvm::BinaryOperator>(value)) {
    return binop->hasNoSignedWrap() || binop->hasNoUnsignedWrap();
  }

  // Division and remainder can produce poison (div by zero)
  if (auto* inst = llvm::dyn_cast<llvm::Instruction>(value)) {
    unsigned op = inst->getOpcode();
    if (op == llvm::Instruction::SDiv || op == llvm::Instruction::UDiv ||
        op == llvm::Instruction::SRem || op == llvm::Instruction::URem) {
      return true;
    }
  }

  // GEP with inbounds can produce poison
  if (auto* gep = llvm::dyn_cast<llvm::GetElementPtrInst>(value)) {
    return gep->isInBounds();
  }

  return false;
}

// -----------------------------------------------------------------------------
// LLVMEmitter UB-Safe Wrappers
// -----------------------------------------------------------------------------

llvm::Value* LLVMEmitter::EmitCheckedAdd(llvm::Value* lhs,
                                         llvm::Value* rhs,
                                         bool is_signed) {
  return cursive::codegen::EmitCheckedAdd(*this, lhs, rhs, is_signed);
}

llvm::Value* LLVMEmitter::EmitCheckedSub(llvm::Value* lhs,
                                         llvm::Value* rhs,
                                         bool is_signed) {
  return cursive::codegen::EmitCheckedSub(*this, lhs, rhs, is_signed);
}

llvm::Value* LLVMEmitter::EmitCheckedMul(llvm::Value* lhs,
                                         llvm::Value* rhs,
                                         bool is_signed) {
  return cursive::codegen::EmitCheckedMul(*this, lhs, rhs, is_signed);
}

llvm::Value* LLVMEmitter::EmitCheckedDiv(llvm::Value* lhs,
                                         llvm::Value* rhs,
                                         bool is_signed) {
  return cursive::codegen::EmitCheckedDiv(*this, lhs, rhs, is_signed);
}

llvm::Value* LLVMEmitter::EmitCheckedRem(llvm::Value* lhs,
                                         llvm::Value* rhs,
                                         bool is_signed) {
  return cursive::codegen::EmitCheckedRem(*this, lhs, rhs, is_signed);
}

llvm::Value* LLVMEmitter::EmitCheckedShl(llvm::Value* lhs,
                                         llvm::Value* rhs) {
  return cursive::codegen::EmitCheckedShl(*this, lhs, rhs);
}

llvm::Value* LLVMEmitter::EmitCheckedShr(llvm::Value* lhs,
                                         llvm::Value* rhs,
                                         bool is_signed) {
  return cursive::codegen::EmitCheckedShr(*this, lhs, rhs, is_signed);
}

void LLVMEmitter::EmitMemCpy(llvm::Value* dst,
                             llvm::Value* src,
                             llvm::Value* size,
                             uint64_t align) {
  cursive::codegen::EmitMemCpy(*this, dst, src, size, align);
}

void LLVMEmitter::EmitMemSet(llvm::Value* dst,
                             llvm::Value* val,
                             llvm::Value* size,
                             uint64_t align) {
  cursive::codegen::EmitMemSet(*this, dst, val, size, align);
}

void LLVMEmitter::EmitMemMove(llvm::Value* dst,
                              llvm::Value* src,
                              llvm::Value* size,
                              uint64_t align) {
  cursive::codegen::EmitMemMove(*this, dst, src, size, align);
}

}  // namespace cursive::codegen



