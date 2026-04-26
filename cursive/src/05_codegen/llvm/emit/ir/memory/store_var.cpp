// =============================================================================
// File: 05_codegen/llvm/emit/ir/memory/store_var.cpp
// Canonical owner for LLVM IR variable store instructions lowering.
// =============================================================================
#include "../../ir_instruction_visitor.h"

namespace cursive::codegen::emit_detail {

void IRInstructionVisitor::operator()(const IRStoreVar &store) const
{
  llvm::Value *slot = emitter.GetLocal(store.name);
  llvm::Value *source_storage = emitter.GetAddressableStorage(store.value);
  if (!slot)
  {
    slot = emitter.GetLocalHomeStorage(store.name);
    if (slot)
    {
      emitter.SetLocal(store.name, slot);
    }
  }
  if (!slot)
  {
    llvm::Value *value = EvaluateOrDefault(store.value);
    llvm::Type *slot_ty = nullptr;
    if (auto *alloca = llvm::dyn_cast<llvm::AllocaInst>(source_storage))
    {
      slot_ty = alloca->getAllocatedType();
    }
    if (!slot_ty && value)
    {
      slot_ty = value->getType();
    }
    if (!slot_ty || slot_ty->isVoidTy())
    {
      slot_ty = llvm::Type::getInt64Ty(emitter.GetContext());
    }

    llvm::Function *func = builder.GetInsertBlock()->getParent();
    llvm::IRBuilder<> entry_builder(&func->getEntryBlock(),
                                   func->getEntryBlock().begin());
    llvm::Value *new_slot =
        entry_builder.CreateAlloca(slot_ty, nullptr, store.name);
    emitter.RegisterLocalBindStorage(store.name, new_slot);

    if (!value)
    {
      value = llvm::Constant::getNullValue(slot_ty);
    }
    else if (auto *alloca = llvm::dyn_cast<llvm::AllocaInst>(new_slot))
    {
      value = CoerceTo(&builder, value, alloca->getAllocatedType());
      if (!value)
      {
        value = llvm::Constant::getNullValue(alloca->getAllocatedType());
      }
    }

    builder.CreateStore(value, new_slot);
    emitter.ReleaseTempStorage(store.value);
    return;
  }
  if (source_storage)
  {
    if (source_storage == slot)
    {
      emitter.ForgetTempStorage(store.value);
      return;
    }
  }
  llvm::Value *value = EvaluateOrDefault(store.value);
  if (auto *alloca = llvm::dyn_cast<llvm::AllocaInst>(slot))
  {
    value = CoerceTo(&builder, value, alloca->getAllocatedType());
    if (!value)
    {
      value = llvm::Constant::getNullValue(alloca->getAllocatedType());
    }
  }
  builder.CreateStore(value, slot);
  emitter.ReleaseTempStorage(store.value);
}

void IRInstructionVisitor::operator()(const IRStoreVarNoDrop &store) const
{
  (*this)(IRStoreVar{store.name, store.value});
}

} // namespace cursive::codegen::emit_detail
