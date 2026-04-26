// =============================================================================
// File: 05_codegen/llvm/emit/ir/control/if.cpp
// Canonical owner for LLVM IR if instruction lowering.
// =============================================================================
#include "../../ir_instruction_visitor.h"

namespace cursive::codegen::emit_detail {

void IRInstructionVisitor::operator()(const IRIf &node) const
{
  llvm::Function *func = builder.GetInsertBlock()->getParent();
  llvm::Value *raw_cond = emitter.EvaluateIRValue(node.cond);
  const bool cond_defaulted = (raw_cond == nullptr);
  if (!raw_cond)
  {
    raw_cond = DefaultFor(node.cond);
  }
  if (core::IsDebugEnabled("return") && func)
  {
    const std::string func_name = func->getName().str();
    if (func_name.find("ContractPredicateIntegrationShift") != std::string::npos)
    {
      std::string cond_ty = "<null>";
      if (raw_cond && raw_cond->getType())
      {
        std::string cond_ty_buf;
        llvm::raw_string_ostream os(cond_ty_buf);
        raw_cond->getType()->print(os);
        os.flush();
        cond_ty = cond_ty_buf;
      }
      std::cerr << "[llvm-if-debug] func=" << func_name
                << " cond_kind=" << static_cast<int>(node.cond.kind)
                << " cond_name=" << node.cond.name
                << " cond_defaulted=" << (cond_defaulted ? "1" : "0")
                << " cond_llvm_ty=" << cond_ty
                << "\n";
    }
  }
  llvm::Value *cond = raw_cond;
  cond = AsBool(&builder, cond);

  llvm::BasicBlock *then_bb =
      llvm::BasicBlock::Create(emitter.GetContext(), "if.then", func);
  llvm::BasicBlock *else_bb =
      llvm::BasicBlock::Create(emitter.GetContext(), "if.else", func);
  llvm::BasicBlock *merge_bb =
      llvm::BasicBlock::Create(emitter.GetContext(), "if.merge", func);

  builder.CreateCondBr(cond, then_bb, else_bb);
  const LLVMEmitter::FlowStateSnapshot branch_state =
      emitter.SaveFlowState();

  struct IncomingValue
  {
    llvm::BasicBlock *pred = nullptr;
    llvm::Value *value = nullptr;
    llvm::Value *storage = nullptr;
  };

  std::vector<IncomingValue> incoming;

  builder.SetInsertPoint(then_bb);
  emitter.RestoreFlowState(branch_state);
  emitter.EmitIR(node.then_ir);
  llvm::BasicBlock *then_end = builder.GetInsertBlock();
  if (!then_end->getTerminator())
  {
    llvm::Value *then_storage = emitter.GetAddressableStorage(node.then_value);
    llvm::Value *then_val = EvaluateOrDefault(node.then_value);
    builder.CreateBr(merge_bb);
    incoming.push_back({then_end, then_val, then_storage});
  }

  builder.SetInsertPoint(else_bb);
  emitter.RestoreFlowState(branch_state);
  emitter.EmitIR(node.else_ir);
  llvm::BasicBlock *else_end = builder.GetInsertBlock();
  if (!else_end->getTerminator())
  {
    llvm::Value *else_storage = emitter.GetAddressableStorage(node.else_value);
    llvm::Value *else_val = EvaluateOrDefault(node.else_value);
    builder.CreateBr(merge_bb);
    incoming.push_back({else_end, else_val, else_storage});
  }

  builder.SetInsertPoint(merge_bb);
  emitter.RestoreFlowState(branch_state);
  if (incoming.empty())
  {
    return;
  }

  llvm::Type *result_ty = ExpectedLLVMType(node.result);
  if (!result_ty)
  {
    result_ty = incoming.front().value
                    ? incoming.front().value->getType()
                    : llvm::Type::getInt64Ty(emitter.GetContext());
  }
  if (!result_ty || result_ty->isVoidTy())
  {
    return;
  }

  const bool aggregate_result = IsAddressBackedAggregateType(result_ty);

  if (aggregate_result)
  {
    auto coerce_storage_in_predecessor =
        [&](llvm::BasicBlock *pred, llvm::Value *storage) -> llvm::Value *
    {
      if (!storage || !storage->getType()->isPointerTy())
      {
        return nullptr;
      }
      llvm::Type *expected_ptr_ty = llvm::PointerType::get(result_ty, 0);
      if (storage->getType() == expected_ptr_ty)
      {
        return storage;
      }
      if (pred && pred->getTerminator())
      {
        llvm::IRBuilder<> pred_builder(pred->getTerminator());
        return pred_builder.CreateBitCast(storage, expected_ptr_ty);
      }
      return builder.CreateBitCast(storage, expected_ptr_ty);
    };

    bool all_have_storage = !incoming.empty();
    for (const auto &entry : incoming)
    {
      if (!entry.storage)
      {
        all_have_storage = false;
        break;
      }
    }

    llvm::Value *merged_storage = nullptr;
    if (all_have_storage)
    {
      if (incoming.size() == 1)
      {
        merged_storage = coerce_storage_in_predecessor(
            incoming.front().pred, incoming.front().storage);
      }
      else
      {
        llvm::Type *expected_ptr_ty = llvm::PointerType::get(result_ty, 0);
        llvm::PHINode *phi =
            builder.CreatePHI(expected_ptr_ty, incoming.size(), "if.result.addr");
        for (const auto &entry : incoming)
        {
          llvm::Value *coerced =
              coerce_storage_in_predecessor(entry.pred, entry.storage);
          phi->addIncoming(
              coerced ? coerced
                      : llvm::ConstantPointerNull::get(
                            llvm::cast<llvm::PointerType>(expected_ptr_ty)),
              entry.pred);
        }
        merged_storage = phi;
      }
    }
    else
    {
      merged_storage =
          emitter.AcquireReusableAggregateStorage(func, result_ty, "if.result");
      llvm::Type *expected_ptr_ty = llvm::PointerType::get(result_ty, 0);
      if (merged_storage && merged_storage->getType() != expected_ptr_ty)
      {
        merged_storage = builder.CreateBitCast(merged_storage, expected_ptr_ty);
      }
      for (const auto &entry : incoming)
      {
        llvm::Value *candidate =
            entry.value ? entry.value : llvm::Constant::getNullValue(result_ty);
        llvm::IRBuilder<> pred_builder(entry.pred->getTerminator());
        llvm::Value *coerced = CoerceTo(&pred_builder, candidate, result_ty);
        if (!coerced)
        {
          coerced = llvm::Constant::getNullValue(result_ty);
        }
        pred_builder.CreateStore(coerced, merged_storage);
      }
    }

    if (merged_storage)
    {
      emitter.ForgetTempStorage(node.result);
      emitter.SetTempStorage(node.result, merged_storage);
      return;
    }
  }

  llvm::Value *merged = nullptr;
  auto coerce_in_predecessor = [&](llvm::BasicBlock *pred, llvm::Value *value) -> llvm::Value *
  {
    llvm::Value *candidate = value ? value : llvm::Constant::getNullValue(result_ty);
    if (!candidate)
    {
      return llvm::Constant::getNullValue(result_ty);
    }
    if (pred && pred->getTerminator())
    {
      llvm::IRBuilder<> pred_builder(pred->getTerminator());
      llvm::Value *coerced = CoerceTo(&pred_builder, candidate, result_ty);
      return coerced ? coerced : llvm::Constant::getNullValue(result_ty);
    }
    llvm::Value *coerced = CoerceTo(&builder, candidate, result_ty);
    return coerced ? coerced : llvm::Constant::getNullValue(result_ty);
  };
  if (incoming.size() == 1)
  {
    merged = coerce_in_predecessor(incoming.front().pred, incoming.front().value);
  }
  else
  {
    llvm::PHINode *phi = builder.CreatePHI(result_ty, incoming.size(), "if.result");
    for (const auto &entry : incoming)
    {
      llvm::Value *coerced = coerce_in_predecessor(entry.pred, entry.value);
      phi->addIncoming(coerced, entry.pred);
    }
    merged = phi;
  }
  emitter.SetTempValue(node.result, merged);
}

} // namespace cursive::codegen::emit_detail
