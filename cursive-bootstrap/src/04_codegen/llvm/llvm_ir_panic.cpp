// =================================================================
// File: 04_codegen/llvm/llvm_ir_panic.cpp
// Construct: LLVM IR Panic Emission Utilities
// Spec Section: 6.12
// Spec Rules: PanicRecord, PoisonFlag
// =================================================================
#include "cursive0/04_codegen/llvm/llvm_ir_panic.h"

#include "cursive0/00_core/assert_spec.h"
#include "cursive0/00_core/symbols.h"
#include "cursive0/03_analysis/types/types.h"
#include "cursive0/04_codegen/abi/abi.h"
#include "cursive0/04_codegen/checks.h"
#include "cursive0/04_codegen/layout/layout.h"
#include "cursive0/04_codegen/llvm/llvm_emit.h"
#include "cursive0/04_codegen/llvm/llvm_ir_utils.h"

#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Instructions.h"

namespace cursive0::codegen {

std::uint16_t PanicCodeFromString(const std::string& reason) {
  if (reason == "ErrorExpr") return PanicCode(PanicReason::ErrorExpr);
  if (reason == "ErrorStmt") return PanicCode(PanicReason::ErrorStmt);
  if (reason == "DivZero") return PanicCode(PanicReason::DivZero);
  if (reason == "Overflow") return PanicCode(PanicReason::Overflow);
  if (reason == "Shift") return PanicCode(PanicReason::Shift);
  if (reason == "Bounds") return PanicCode(PanicReason::Bounds);
  if (reason == "Cast") return PanicCode(PanicReason::Cast);
  if (reason == "NullDeref") return PanicCode(PanicReason::NullDeref);
  if (reason == "ExpiredDeref") return PanicCode(PanicReason::ExpiredDeref);
  if (reason == "InitPanic") return PanicCode(PanicReason::InitPanic);
  return PanicCode(PanicReason::Other);
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

bool IsInitFunction(LLVMEmitter& emitter, llvm::Function* func) {
  if (!func) {
    return false;
  }
  const std::string prefix =
      core::Mangle(core::StringOfPath({"cursive", "runtime", "init"}));
  return func->getName().starts_with(prefix);
}

std::vector<std::string> SplitModulePathString(const std::string& module) {
  std::vector<std::string> path;
  std::string acc;
  for (std::size_t i = 0; i < module.size();) {
    if (i + 1 < module.size() && module[i] == ':' && module[i + 1] == ':') {
      path.push_back(acc);
      acc.clear();
      i += 2;
      continue;
    }
    acc.push_back(module[i++]);
  }
  if (!acc.empty()) {
    path.push_back(acc);
  }
  return path;
}

llvm::GlobalVariable* GetOrCreatePoisonFlag(LLVMEmitter& emitter,
                                            const std::vector<std::string>& module_path) {
  SPEC_RULE("PoisonFlag-Decl");
  std::vector<std::string> full = {"cursive", "runtime", "poison"};
  full.insert(full.end(), module_path.begin(), module_path.end());
  const std::string sym = core::Mangle(core::StringOfPath(full));
  if (auto* existing = emitter.GetModule().getGlobalVariable(sym, true)) {
    return existing;
  }
  bool define_flag = true;
  LowerCtx* ctx = emitter.GetCurrentCtx();
  if (ctx) {
    define_flag = (ctx->module_path == module_path);
  }
  auto* bool_ty = emitter.GetLLVMType(analysis::MakeTypePrim("bool"));
  if (!bool_ty) {
    SPEC_RULE("PoisonFlag-Err");
    if (ctx) {
      ctx->ReportCodegenFailure();
    }
    return nullptr;
  }
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

std::vector<std::string> PoisonSetForInit(const LowerCtx& ctx) {
  const std::string module_name = core::StringOfPath(ctx.module_path);
  if (ctx.init_modules.empty()) {
    return {module_name};
  }

  std::size_t target = ctx.init_modules.size();
  for (std::size_t i = 0; i < ctx.init_modules.size(); ++i) {
    if (core::StringOfPath(ctx.init_modules[i]) == module_name) {
      target = i;
      break;
    }
  }
  if (target == ctx.init_modules.size()) {
    return {module_name};
  }

  const std::size_t n = ctx.init_modules.size();
  std::vector<std::vector<std::size_t>> outgoing(n);
  for (const auto& edge : ctx.init_eager_edges) {
    if (edge.first < n && edge.second < n) {
      outgoing[edge.first].push_back(edge.second);
    }
  }

  std::vector<char> visited(n, false);
  std::vector<std::size_t> stack;
  visited[target] = true;
  stack.push_back(target);
  while (!stack.empty()) {
    const std::size_t cur = stack.back();
    stack.pop_back();
    for (const auto succ : outgoing[cur]) {
      if (!visited[succ]) {
        visited[succ] = true;
        stack.push_back(succ);
      }
    }
  }

  std::vector<std::string> out;
  out.reserve(n);
  for (std::size_t i = 0; i < n; ++i) {
    if (visited[i]) {
      out.push_back(core::StringOfPath(ctx.init_modules[i]));
    }
  }
  if (out.empty()) {
    out.push_back(module_name);
  }
  return out;
}

void StoreInitPanicRecord(LLVMEmitter& emitter,
                          llvm::IRBuilder<>* builder) {
  LowerCtx* ctx = emitter.GetCurrentCtx();
  if (!ctx) {
    return;
  }
  llvm::Function* func = builder->GetInsertBlock()->getParent();
  if (!IsInitFunction(emitter, func)) {
    return;
  }

  const auto poison = PoisonSetForInit(*ctx);
  if (!poison.empty()) {
    llvm::Type* bool_ty = emitter.GetLLVMType(analysis::MakeTypePrim("bool"));
    llvm::Value* val = llvm::ConstantInt::get(bool_ty, 1);
    for (const auto& module_name : poison) {
      const auto path = SplitModulePathString(module_name);
      llvm::GlobalVariable* flag = GetOrCreatePoisonFlag(emitter, path);
      if (!flag) {
        SPEC_RULE("SetPoison-Err");
        ctx->ReportCodegenFailure();
        return;
      }
      builder->CreateStore(val, flag);
    }
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
  llvm::Value* code_val = llvm::ConstantInt::get(llvm::Type::getInt32Ty(ctx_ll),
                                                 PanicCode(PanicReason::InitPanic));
  StoreAtOffset(emitter, builder, ptr, layout->offsets[0], panic_val);
  StoreAtOffset(emitter, builder, ptr, layout->offsets[1], code_val);
}

void StorePanicRecord(LLVMEmitter& emitter,
                      llvm::IRBuilder<>* builder,
                      std::uint16_t code) {
  LowerCtx* ctx = emitter.GetCurrentCtx();
  if (!ctx) {
    return;
  }
  llvm::Function* func = builder->GetInsertBlock()->getParent();
  if (IsInitFunction(emitter, func)) {
    StoreInitPanicRecord(emitter, builder);
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

void ClearPanicRecord(LLVMEmitter& emitter,
                      llvm::IRBuilder<>* builder) {
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
  llvm::Value* panic_val = llvm::ConstantInt::get(llvm::Type::getInt8Ty(ctx_ll), 0);
  llvm::Value* code_val = llvm::ConstantInt::get(llvm::Type::getInt32Ty(ctx_ll), 0);
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

void EmitPanicIfFalse(LLVMEmitter& emitter,
                      llvm::IRBuilder<>* builder,
                      llvm::Value* ok,
                      std::uint16_t code) {
  if (!ok) {
    return;
  }
  llvm::Function* func = builder->GetInsertBlock()->getParent();
  llvm::BasicBlock* ok_bb = llvm::BasicBlock::Create(emitter.GetContext(), "check_ok", func);
  llvm::BasicBlock* fail_bb = llvm::BasicBlock::Create(emitter.GetContext(), "check_fail", func);
  builder->CreateCondBr(ok, ok_bb, fail_bb);

  builder->SetInsertPoint(fail_bb);
  StorePanicRecord(emitter, builder, code);
  builder->CreateBr(ok_bb);

  builder->SetInsertPoint(ok_bb);
}

void EmitPanicReturnIfFalse(LLVMEmitter& emitter,
                            llvm::IRBuilder<>* builder,
                            llvm::Value* ok,
                            std::uint16_t code) {
  if (!ok) {
    return;
  }
  llvm::Function* func = builder->GetInsertBlock()->getParent();
  llvm::BasicBlock* ok_bb = llvm::BasicBlock::Create(emitter.GetContext(), "check_ok", func);
  llvm::BasicBlock* fail_bb = llvm::BasicBlock::Create(emitter.GetContext(), "check_fail", func);
  builder->CreateCondBr(ok, ok_bb, fail_bb);

  builder->SetInsertPoint(fail_bb);
  StorePanicRecord(emitter, builder, code);
  EmitReturn(emitter, builder);

  builder->SetInsertPoint(ok_bb);
}

}  // namespace cursive0::codegen
