// =============================================================================
// MIGRATION MAPPING: llvm_emit.cpp
// =============================================================================
//
// SPEC REFERENCE: CursiveSpecification.md
//   - Section 6.12 LLVM 21 Backend Requirements (lines 17287-17650)
//   - LowerIRDecl rules (lines 17600-17645)
//   - IR to LLVM emission
//   - Procedure body emission
//   - Global variable emission
//
// SOURCE FILE: cursive-bootstrap/src/04_codegen/ir_lowering.cpp
//   - Main IR to LLVM lowering visitor
//   - Lines 3900-4200: Expression lowering (widen, transmute, etc.)
//   - Lines 4183+: IRTransmute LLVM emission
//
// SOURCE FILE: cursive-bootstrap/src/04_codegen/llvm/llvm_module.cpp
//   - Lines 46-100: Memory operation helpers
//   - Lines 91-100: LoadPanicOutPtr
//
// DEPENDENCIES:
//   - cursive/include/05_codegen/llvm/llvm_emit.h
//   - cursive/include/05_codegen/ir/ir_model.h (all IR node types)
//   - cursive/include/05_codegen/llvm/llvm_types.h (GetLLVMType)
//   - llvm/IR/IRBuilder.h
//   - llvm/IR/Instructions.h
// =============================================================================

#include "05_codegen/llvm/llvm_emit.h"

#include "00_core/process_config.h"
#include "00_core/host/services.h"
#include "00_core/spec_trace.h"
#include "00_core/symbols.h"
#include "04_analysis/caps/cap_concurrency.h"
#include "04_analysis/modal/builtin_modal_intrinsics.h"
#include "04_analysis/modal/modal.h"
#include "04_analysis/modal/modal_widen.h"
#include "04_analysis/resolve/scopes.h"
#include "04_analysis/generics/monomorphize.h"
#include "04_analysis/typing/type_predicates.h"
#include "04_analysis/typing/type_equiv.h"
#include "04_analysis/typing/type_lookup.h"
#include "04_analysis/typing/type_lower.h"
#include "05_codegen/abi/abi.h"
#include "05_codegen/cleanup/cleanup.h"
#include "05_codegen/checks/panic.h"
#include "05_codegen/dyn_dispatch/dyn_dispatch.h"
#include "05_codegen/globals/entrypoint.h"
#include "05_codegen/globals/binding_storage.h"
#include "05_codegen/globals/globals.h"
#include "05_codegen/globals/init.h"
#include "05_codegen/globals/literal_emit.h"
#include "05_codegen/intrinsics/intrinsics_interface.h"
#include "05_codegen/intrinsics/builtins.h"
#include "05_codegen/intrinsics/async_frame.h"
#include "04_analysis/layout/layout.h"
#include "05_codegen/llvm/llvm_attr.h"
#include "05_codegen/llvm/llvm_call.h"
#include "05_codegen/llvm/emit/internal_helpers.h"
#include "05_codegen/llvm/llvm_module.h"
#include "05_codegen/llvm/llvm_ir_panic.h"
#include "05_codegen/llvm/llvm_types.h"
#include "05_codegen/llvm/llvm_ub_safe.h"

#include "llvm/IR/Attributes.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Comdat.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/Alignment.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/TargetParser/Triple.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"
#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/APInt.h"

#include <chrono>
#include <cctype>
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <mutex>
#include <set>
#include <string>
#include <vector>

namespace cursive::codegen
{

  namespace
  {

    using AsyncCombinatorKind = analysis::BuiltinAsyncCombinatorKind;
    using namespace emit_detail;

    std::optional<AsyncCombinatorKind> AsyncCombinatorKindFromSymbol(
        std::string_view symbol)
    {
      return analysis::LookupBuiltinAsyncCombinatorByRuntimeSymbol(symbol);
    }

    llvm::Value *EmitSliceLenFromAddr(LLVMEmitter &emitter,
                                      llvm::IRBuilder<> &builder,
                                      const analysis::TypeRef &type,
                                      llvm::Value *addr)
    {
      if (!type || !addr || !addr->getType()->isPointerTy())
      {
        return nullptr;
      }

      analysis::TypeRef normalized = analysis::StripPerm(type);
      if (!normalized)
      {
        normalized = type;
      }

      if (!std::holds_alternative<analysis::TypeSlice>(normalized->node) &&
          !std::holds_alternative<analysis::TypeString>(normalized->node) &&
          !std::holds_alternative<analysis::TypeBytes>(normalized->node))
      {
        return nullptr;
      }

      llvm::Type *slice_ll = emitter.GetLLVMType(normalized);
      auto *slice_struct_ty = llvm::dyn_cast_or_null<llvm::StructType>(slice_ll);
      if (!slice_struct_ty || slice_struct_ty->getNumElements() < 2 ||
          !slice_struct_ty->getElementType(1)->isIntegerTy())
      {
        return nullptr;
      }

      llvm::Value *typed_ptr =
          builder.CreateBitCast(addr, llvm::PointerType::get(slice_ll, 0));
      llvm::Value *loaded_slice = builder.CreateLoad(slice_ll, typed_ptr);
      if (!loaded_slice || !loaded_slice->getType()->isStructTy())
      {
        return nullptr;
      }
      return builder.CreateExtractValue(loaded_slice, {1u});
    }

    llvm::Value *EmitIndexLenFromAddr(LLVMEmitter &emitter,
                                      llvm::IRBuilder<> &builder,
                                      const analysis::TypeRef &type,
                                      llvm::Value *addr)
    {
      if (!type || !addr)
      {
        return nullptr;
      }

      analysis::TypeRef normalized = analysis::StripPerm(type);
      if (!normalized)
      {
        normalized = type;
      }

      if (const auto *arr = std::get_if<analysis::TypeArray>(&normalized->node))
      {
        return llvm::ConstantInt::get(llvm::Type::getInt64Ty(emitter.GetContext()),
                                      static_cast<std::uint64_t>(arr->length));
      }

      return EmitSliceLenFromAddr(emitter, builder, normalized, addr);
    }

    llvm::Value *EmitSequenceDataPtrFromAddr(LLVMEmitter &emitter,
                                             llvm::IRBuilder<> &builder,
                                             const analysis::TypeRef &type,
                                             llvm::Value *addr)
    {
      if (!type || !addr || !addr->getType()->isPointerTy())
      {
        return nullptr;
      }

      analysis::TypeRef normalized = analysis::StripPerm(type);
      if (!normalized)
      {
        normalized = type;
      }

      if (!std::holds_alternative<analysis::TypeSlice>(normalized->node))
      {
        return nullptr;
      }

      llvm::Type *sequence_ll = emitter.GetLLVMType(normalized);
      auto *sequence_struct_ty = llvm::dyn_cast_or_null<llvm::StructType>(sequence_ll);
      if (!sequence_struct_ty || sequence_struct_ty->getNumElements() < 2 ||
          !sequence_struct_ty->getElementType(0)->isPointerTy())
      {
        return nullptr;
      }

      llvm::Value *typed_ptr =
          builder.CreateBitCast(addr, llvm::PointerType::get(sequence_ll, 0));
      llvm::Value *loaded_sequence = builder.CreateLoad(sequence_ll, typed_ptr);
      if (!loaded_sequence || !loaded_sequence->getType()->isStructTy())
      {
        return nullptr;
      }
      return builder.CreateExtractValue(loaded_sequence, {0u});
    }

    struct IndexedSequenceIterState
    {
      analysis::TypeRef element_type = nullptr;
      llvm::Value *length = nullptr;
      llvm::Value *array_ptr = nullptr;
      llvm::ArrayType *array_type = nullptr;
      llvm::Value *data_ptr = nullptr;
    };

    struct IndexedSequenceLoweredIterState
    {
      IndexedSequenceIterState iter;
      llvm::AllocaInst *idx_slot = nullptr;
      llvm::AllocaInst *elem_slot = nullptr;
    };

    bool PrepareIndexedSequenceIter(LLVMEmitter &emitter,
                                    llvm::IRBuilder<> &entry_builder,
                                    llvm::IRBuilder<> &builder,
                                    const analysis::TypeRef &type,
                                    llvm::Value *value,
                                    IndexedSequenceIterState &out)
    {
      if (!type || !value)
      {
        return false;
      }

      analysis::TypeRef normalized = analysis::StripPerm(type);
      if (!normalized)
      {
        normalized = type;
      }

      llvm::Type *i64_ty = llvm::Type::getInt64Ty(emitter.GetContext());

      if (const auto *arr = std::get_if<analysis::TypeArray>(&normalized->node))
      {
        out.element_type = arr->element;
        out.length = llvm::ConstantInt::get(
            i64_ty, static_cast<std::uint64_t>(arr->length));

        if (auto *array_ty = llvm::dyn_cast<llvm::ArrayType>(value->getType()))
        {
          llvm::AllocaInst *array_slot =
              entry_builder.CreateAlloca(array_ty, nullptr, "iter.value.slot");
          builder.CreateStore(value, array_slot);
          out.array_ptr = array_slot;
          out.array_type = array_ty;
          return true;
        }

        llvm::Type *array_ll = emitter.GetLLVMType(normalized);
        auto *array_ty = llvm::dyn_cast_or_null<llvm::ArrayType>(array_ll);
        if (array_ty && value->getType()->isPointerTy())
        {
          out.array_ptr =
              builder.CreateBitCast(value, llvm::PointerType::get(array_ty, 0));
          out.array_type = array_ty;
          return true;
        }

        return false;
      }

      if (const auto *slice = std::get_if<analysis::TypeSlice>(&normalized->node))
      {
        out.element_type = slice->element;

        llvm::Value *len = nullptr;
        llvm::Value *data_ptr = nullptr;
        if (value->getType()->isStructTy())
        {
          len = builder.CreateExtractValue(value, {1u});
          data_ptr = builder.CreateExtractValue(value, {0u});
        }
        else if (value->getType()->isPointerTy())
        {
          len = EmitSliceLenFromAddr(emitter, builder, normalized, value);
          data_ptr = EmitSequenceDataPtrFromAddr(emitter, builder, normalized, value);
        }

        if (!len || !len->getType()->isIntegerTy() || !data_ptr ||
            !data_ptr->getType()->isPointerTy())
        {
          return false;
        }

        if (len->getType()->getIntegerBitWidth() != 64)
        {
          len = builder.CreateIntCast(len, i64_ty, false);
        }

        out.length = len;
        out.data_ptr = data_ptr;
        return true;
      }

      return false;
    }

    llvm::Value *EmitIndexedSequenceElem(LLVMEmitter &emitter,
                                         llvm::IRBuilder<> &builder,
                                         const IndexedSequenceIterState &iter,
                                         llvm::Value *index);

    bool EmitSeqIterInit(LLVMEmitter &emitter,
                         llvm::IRBuilder<> &entry_builder,
                         llvm::IRBuilder<> &builder,
                         const analysis::TypeRef &type,
                         llvm::Value *value,
                         IndexedSequenceLoweredIterState &out)
    {
      IndexedSequenceIterState prepared;
      if (!PrepareIndexedSequenceIter(
              emitter, entry_builder, builder, type, value, prepared))
      {
        return false;
      }

      llvm::Type *elem_ll = emitter.GetLLVMType(prepared.element_type);
      if (!elem_ll || elem_ll->isVoidTy())
      {
        return false;
      }

      llvm::Type *i64_ty = llvm::Type::getInt64Ty(emitter.GetContext());
      out.iter = std::move(prepared);
      out.idx_slot =
          entry_builder.CreateAlloca(i64_ty, nullptr, "iter.seq.idx");
      out.elem_slot =
          entry_builder.CreateAlloca(elem_ll, nullptr, "iter.seq.elem");
      builder.CreateStore(llvm::ConstantInt::get(i64_ty, 0), out.idx_slot);
      builder.CreateStore(llvm::Constant::getNullValue(elem_ll), out.elem_slot);
      return true;
    }

    llvm::Value *EmitSeqIterNext(LLVMEmitter &emitter,
                                 llvm::IRBuilder<> &builder,
                                 const IndexedSequenceLoweredIterState &iter)
    {
      if (!iter.idx_slot || !iter.elem_slot || !iter.iter.length ||
          !iter.iter.length->getType()->isIntegerTy())
      {
        return llvm::ConstantInt::getFalse(emitter.GetContext());
      }

      llvm::Function *func = builder.GetInsertBlock()
                                 ? builder.GetInsertBlock()->getParent()
                                 : nullptr;
      if (!func)
      {
        return llvm::ConstantInt::getFalse(emitter.GetContext());
      }

      llvm::Type *i64_ty = llvm::Type::getInt64Ty(emitter.GetContext());
      llvm::Value *idx_cur = builder.CreateLoad(i64_ty, iter.idx_slot);
      llvm::Value *iter_len = iter.iter.length;
      if (iter_len->getType()->getIntegerBitWidth() != 64)
      {
        iter_len = builder.CreateIntCast(iter_len, i64_ty, false);
      }
      llvm::Value *in_range = builder.CreateICmpULT(idx_cur, iter_len);

      llvm::BasicBlock *next_ok =
          llvm::BasicBlock::Create(emitter.GetContext(), "iter.seq.next.ok", func);
      llvm::BasicBlock *next_done =
          llvm::BasicBlock::Create(emitter.GetContext(), "iter.seq.next.done", func);
      llvm::BasicBlock *next_cont =
          llvm::BasicBlock::Create(emitter.GetContext(), "iter.seq.next.cont", func);

      builder.CreateCondBr(in_range, next_ok, next_done);

      builder.SetInsertPoint(next_ok);
      llvm::Value *elem = EmitIndexedSequenceElem(emitter, builder, iter.iter, idx_cur);
      if (!elem)
      {
        elem = llvm::Constant::getNullValue(iter.elem_slot->getAllocatedType());
      }
      builder.CreateStore(elem, iter.elem_slot);
      llvm::Value *idx_next =
          builder.CreateAdd(idx_cur, llvm::ConstantInt::get(i64_ty, 1));
      builder.CreateStore(idx_next, iter.idx_slot);
      builder.CreateBr(next_cont);

      builder.SetInsertPoint(next_done);
      builder.CreateBr(next_cont);

      builder.SetInsertPoint(next_cont);
      llvm::PHINode *has_value =
          builder.CreatePHI(llvm::Type::getInt1Ty(emitter.GetContext()), 2);
      has_value->addIncoming(
          llvm::ConstantInt::getTrue(emitter.GetContext()), next_ok);
      has_value->addIncoming(
          llvm::ConstantInt::getFalse(emitter.GetContext()), next_done);
      return has_value;
    }

    llvm::Value *LoadSeqIterElem(llvm::IRBuilder<> &builder,
                                 const IndexedSequenceLoweredIterState &iter)
    {
      if (!iter.elem_slot)
      {
        return nullptr;
      }
      return builder.CreateLoad(iter.elem_slot->getAllocatedType(), iter.elem_slot);
    }

    llvm::Value *EmitIndexedSequenceElem(LLVMEmitter &emitter,
                                         llvm::IRBuilder<> &builder,
                                         const IndexedSequenceIterState &iter,
                                         llvm::Value *index)
    {
      if (!iter.element_type || !index || !index->getType()->isIntegerTy())
      {
        return nullptr;
      }

      llvm::Type *elem_ll = emitter.GetLLVMType(iter.element_type);
      if (!elem_ll)
      {
        return nullptr;
      }

      llvm::Value *i64_index = index;
      llvm::Type *i64_ty = llvm::Type::getInt64Ty(emitter.GetContext());
      if (i64_index->getType()->getIntegerBitWidth() != 64)
      {
        i64_index = builder.CreateIntCast(i64_index, i64_ty, false);
      }

      if (iter.array_ptr && iter.array_type)
      {
        llvm::Value *zero = llvm::ConstantInt::get(i64_ty, 0);
        llvm::Value *elem_ptr = builder.CreateGEP(
            iter.array_type, iter.array_ptr, {zero, i64_index});
        return builder.CreateLoad(elem_ll, elem_ptr);
      }

      if (iter.data_ptr && iter.data_ptr->getType()->isPointerTy())
      {
        llvm::Value *typed_data_ptr = builder.CreateBitCast(
            iter.data_ptr, llvm::PointerType::get(elem_ll, 0));
        llvm::Value *elem_ptr =
            builder.CreateGEP(elem_ll, typed_data_ptr, i64_index);
        return builder.CreateLoad(elem_ll, elem_ptr);
      }

      return nullptr;
    }

    struct IRNodePerfBucket
    {
      std::size_t count = 0;
      long long total_self_ms = 0;
      long long max_self_ms = 0;
    };

    struct IRNodePerfFrame
    {
      std::size_t kind_index = 0;
      std::chrono::steady_clock::time_point start;
      long long child_ms = 0;
    };

    constexpr std::size_t kIRNodePerfKindCount = 62;

    struct IRProcPerfContext
    {
      std::array<IRNodePerfBucket, kIRNodePerfKindCount> buckets{};
      std::vector<IRNodePerfFrame> stack;
    };

    thread_local IRProcPerfContext *g_ir_proc_perf_ctx = nullptr;

    const char *IRNodePerfKindName(std::size_t index)
    {
      static constexpr std::array<const char *, kIRNodePerfKindCount> names = {
          "IROpaque",
          "IRSeq",
          "IRCall",
          "IRCallVTable",
          "IRStoreGlobal",
          "IRReadVar",
          "IRReadPath",
          "IRBindVar",
          "IRStoreVar",
          "IRStoreVarNoDrop",
          "IRReadPlace",
          "IRWritePlace",
          "IRAddrOf",
          "IRReadPtr",
          "IRWritePtr",
          "IRUnaryOp",
          "IRFence",
          "IRBinaryOp",
          "IRCast",
          "IRTransmute",
          "IRCheckIndex",
          "IRCheckRange",
          "IRCheckSliceLen",
          "IRCheckOp",
          "IRCheckCast",
          "IRAlloc",
          "IRContextBundleBuild",
          "IRReturn",
          "IRResult",
          "IRBreak",
          "IRContinue",
          "IRDefer",
          "IRMoveState",
          "IRIf",
          "IRBlock",
          "IRLoop",
          "IRIfCase",
          "IRRegion",
          "IRFrame",
          "IRBranch",
          "IRPhi",
          "IRClearPanic",
          "IRPanicCheck",
          "IRInitPanicHandle",
          "IRHandleDeinitPanic",
          "IRRestoreDeinitPanic",
          "IRCheckPoison",
          "IRLowerPanic",
          "IRParallel",
          "IRSpawn",
          "IRWait",
          "IRCancelCheck",
          "IRCancelSuppress",
          "IRDispatch",
          "IRYield",
          "IRYieldFrom",
          "IRSync",
          "IRRaceReturn",
          "IRRaceYield",
          "IRAll",
          "IRAsyncComplete",
          "IRAsyncFail",
      };
      if (index < names.size())
      {
        return names[index];
      }
      return "IRUnknown";
    }

    long long IRProcPerfTotalSelfMs(const IRProcPerfContext &ctx)
    {
      long long total = 0;
      for (const auto &bucket : ctx.buckets)
      {
        total += bucket.total_self_ms;
      }
      return total;
    }

    void AppendTopIRNodePerf(std::string &line, const IRProcPerfContext &ctx)
    {
      struct TopEntry
      {
        std::size_t idx = 0;
        long long total_self_ms = -1;
        std::size_t count = 0;
        long long max_self_ms = 0;
      };

      std::array<TopEntry, 3> top{};
      for (std::size_t i = 0; i < top.size(); ++i)
      {
        top[i].total_self_ms = -1;
      }

      for (std::size_t i = 0; i < ctx.buckets.size(); ++i)
      {
        const auto &bucket = ctx.buckets[i];
        if (bucket.count == 0 || bucket.total_self_ms <= 0)
        {
          continue;
        }

        TopEntry candidate;
        candidate.idx = i;
        candidate.total_self_ms = bucket.total_self_ms;
        candidate.count = bucket.count;
        candidate.max_self_ms = bucket.max_self_ms;

        for (std::size_t pos = 0; pos < top.size(); ++pos)
        {
          if (candidate.total_self_ms <= top[pos].total_self_ms)
          {
            continue;
          }
          for (std::size_t shift = top.size() - 1; shift > pos; --shift)
          {
            top[shift] = top[shift - 1];
          }
          top[pos] = candidate;
          break;
        }
      }

      for (std::size_t i = 0; i < top.size(); ++i)
      {
        if (top[i].total_self_ms < 0)
        {
          continue;
        }
        line += " ir_top" + std::to_string(i + 1) + "=" +
                IRNodePerfKindName(top[i].idx) + ":" +
                std::to_string(top[i].total_self_ms) + "ms/" +
                std::to_string(top[i].count) + "x(max=" +
                std::to_string(top[i].max_self_ms) + "ms)";
      }
    }

    bool IsForeignAbiAggregateLLVMType(llvm::Type *ty)
    {
      return ty && (ty->isStructTy() || ty->isArrayTy());
    }

    bool IsWin64CAbiAggregateDirectSize(std::uint64_t size)
    {
      return size == 1 || size == 2 || size == 4 || size == 8;
    }

    llvm::Type *Win64CAbiDirectAggregateCarrier(llvm::LLVMContext &ctx,
                                                std::uint64_t size)
    {
      switch (size)
      {
      case 1:
        return llvm::Type::getInt8Ty(ctx);
      case 2:
        return llvm::Type::getInt16Ty(ctx);
      case 4:
        return llvm::Type::getInt32Ty(ctx);
      case 8:
        return llvm::Type::getInt64Ty(ctx);
      default:
        return nullptr;
      }
    }

    bool IsClosurePairLLVMType(llvm::Type *ty)
    {
      auto *struct_ty = llvm::dyn_cast_or_null<llvm::StructType>(ty);
      if (!struct_ty || struct_ty->getNumElements() != 2)
      {
        return false;
      }
      return struct_ty->getElementType(0)->isPointerTy() &&
             struct_ty->getElementType(1)->isPointerTy();
    }

    llvm::Function *FunctionFromLLVMValue(llvm::Value *value)
    {
      llvm::Value *current = value;
      while (current)
      {
        if (auto *fn = llvm::dyn_cast<llvm::Function>(current))
        {
          return fn;
        }
        if (auto *ce = llvm::dyn_cast<llvm::ConstantExpr>(current))
        {
          if (ce->isCast() && ce->getNumOperands() >= 1)
          {
            current = ce->getOperand(0);
            continue;
          }
        }
        if (auto *cast = llvm::dyn_cast<llvm::CastInst>(current))
        {
          current = cast->getOperand(0);
          continue;
        }
        break;
      }
      return nullptr;
    }

    std::uint64_t AlignUpBytes(std::uint64_t value, std::uint64_t align)
    {
      if (align == 0)
      {
        return value;
      }
      const std::uint64_t rem = value % align;
      return rem == 0 ? value : (value + (align - rem));
    }

    std::vector<ast::ModulePath> ComputeEntryInitOrder(const LowerCtx &ctx)
    {
      if (!ctx.init_order.empty())
      {
        return ctx.init_order;
      }

      if (!ctx.init_modules.empty())
      {
        const std::size_t n = ctx.init_modules.size();
        if (ctx.init_eager_edges.empty())
        {
          return ctx.init_modules;
        }

        std::vector<std::vector<std::size_t>> outgoing(n);
        std::vector<std::size_t> indeg(n, 0);
        for (const auto &edge : ctx.init_eager_edges)
        {
          if (edge.first >= n || edge.second >= n)
          {
            continue;
          }
          outgoing[edge.first].push_back(edge.second);
          indeg[edge.second] += 1;
        }

        std::set<std::size_t> ready;
        for (std::size_t i = 0; i < n; ++i)
        {
          if (indeg[i] == 0)
          {
            ready.insert(i);
          }
        }

        std::vector<ast::ModulePath> topo;
        topo.reserve(n);
        while (!ready.empty())
        {
          const std::size_t cur = *ready.begin();
          ready.erase(ready.begin());
          topo.push_back(ctx.init_modules[cur]);
          for (const auto succ : outgoing[cur])
          {
            if (indeg[succ] == 0)
            {
              continue;
            }
            indeg[succ] -= 1;
            if (indeg[succ] == 0)
            {
              ready.insert(succ);
            }
          }
        }

        if (topo.size() == n)
        {
          return topo;
        }
        return ctx.init_modules;
      }

      if (ctx.sigma)
      {
        return ComputeInitOrderFromSigma(*ctx.sigma);
      }

      return {};
    }

    llvm::Value *CreateTaggedPayloadI8Ptr(LLVMEmitter &emitter,
                                          llvm::IRBuilder<> *builder,
                                          llvm::StructType *tagged_ty,
                                          llvm::Value *tagged_slot,
                                          std::uint64_t payload_align)
    {
      if (!builder || !tagged_ty || !tagged_slot)
      {
        return nullptr;
      }

      const llvm::DataLayout &dl = emitter.GetModule().getDataLayout();
      llvm::Type *disc_ty = tagged_ty->getElementType(0);
      const std::uint64_t disc_size =
          static_cast<std::uint64_t>(dl.getTypeAllocSize(disc_ty));
      const std::uint64_t payload_off =
          AlignUpBytes(disc_size, std::max<std::uint64_t>(1, payload_align));

      llvm::Type *i8_ty = llvm::Type::getInt8Ty(emitter.GetContext());
      llvm::Type *i64_ty = llvm::Type::getInt64Ty(emitter.GetContext());
      llvm::Value *base_i8 = builder->CreateBitCast(
          tagged_slot, llvm::PointerType::get(i8_ty, 0));
      return builder->CreateGEP(
          i8_ty,
          base_i8,
          llvm::ConstantInt::get(i64_ty, payload_off));
    }

    bool IsUnitTypeRef(const analysis::TypeRef &type)
    {
      if (!type)
      {
        return false;
      }
      if (const auto *prim = std::get_if<analysis::TypePrim>(&type->node))
      {
        return prim->name == "()";
      }
      if (const auto *tuple = std::get_if<analysis::TypeTuple>(&type->node))
      {
        return tuple->elements.empty();
      }
      return false;
    }

    bool IsNeverTypeRef(const analysis::TypeRef &type)
    {
      if (!type)
      {
        return false;
      }
      if (const auto *prim = std::get_if<analysis::TypePrim>(&type->node))
      {
        return prim->name == "!";
      }
      return false;
    }

    bool IsRuntimeHandleModalPath(const analysis::TypePath &path)
    {
      return analysis::IsBuiltinRuntimeHandleModalTypePath(path);
    }

    std::uint64_t AsyncStateIndexOrDefault(const analysis::ScopeContext &scope,
                                           std::string_view state_name,
                                           std::uint64_t fallback)
    {
      const ast::ModalDecl *async_decl = analysis::LookupModalDecl(scope, {"Async"});
      if (!async_decl)
      {
        return fallback;
      }
      for (std::size_t i = 0; i < async_decl->states.size(); ++i)
      {
        if (analysis::IdEq(async_decl->states[i].name, std::string(state_name)))
        {
          return static_cast<std::uint64_t>(i);
        }
      }
      return fallback;
    }

    struct AsyncStateDiscs
    {
      std::uint64_t suspended = 0;
      std::uint64_t completed = 1;
      std::optional<std::uint64_t> failed;
    };

    AsyncStateDiscs LoweredAsyncStateDiscs(
        const analysis::ScopeContext &scope,
        const std::optional<::cursive::analysis::layout::LoweredAsyncType> &lowered)
    {
      AsyncStateDiscs discs;
      if (lowered.has_value())
      {
        for (std::size_t i = 0; i < lowered->states.size(); ++i)
        {
          if (lowered->states[i] == "Suspended")
          {
            discs.suspended = static_cast<std::uint64_t>(i);
          }
          else if (lowered->states[i] == "Completed")
          {
            discs.completed = static_cast<std::uint64_t>(i);
          }
          else if (lowered->states[i] == "Failed")
          {
            discs.failed = static_cast<std::uint64_t>(i);
          }
        }
        return discs;
      }

      discs.suspended = AsyncStateIndexOrDefault(scope, "Suspended", 0);
      discs.completed = AsyncStateIndexOrDefault(scope, "Completed", 1);
      discs.failed = AsyncStateIndexOrDefault(scope, "Failed", 2);
      return discs;
    }

    AsyncStateDiscs LoweredAsyncStateDiscs(
        const analysis::ScopeContext &scope,
        const analysis::TypeRef &async_type)
    {
      return LoweredAsyncStateDiscs(scope, ::cursive::analysis::layout::LowerAsyncType(async_type));
    }

    AsyncStateDiscs LoweredAsyncStateDiscs(
        const analysis::ScopeContext &scope,
        const analysis::AsyncSig &sig)
    {
      return LoweredAsyncStateDiscs(scope, ::cursive::analysis::layout::LowerAsyncType(sig));
    }

    constexpr std::uint64_t kAsyncPayloadFramePtrOffset = 8;

    llvm::Value *AsyncFrameAddr(LLVMEmitter &emitter,
                                llvm::IRBuilder<> *builder,
                                llvm::Value *frame_ptr,
                                std::uint64_t offset)
    {
      if (!builder || !frame_ptr)
      {
        return nullptr;
      }
      llvm::Type *i8_ty = llvm::Type::getInt8Ty(emitter.GetContext());
      llvm::Type *i64_ty = llvm::Type::getInt64Ty(emitter.GetContext());
      llvm::Value *base = frame_ptr;
      if (base->getType() != llvm::PointerType::get(i8_ty, 0))
      {
        base = builder->CreateBitCast(base, llvm::PointerType::get(i8_ty, 0));
      }
      return builder->CreateGEP(
          i8_ty,
          base,
          llvm::ConstantInt::get(i64_ty, offset));
    }

    llvm::Value *AsyncFrameTypedPtr(LLVMEmitter &emitter,
                                    llvm::IRBuilder<> *builder,
                                    llvm::Value *frame_ptr,
                                    std::uint64_t offset,
                                    llvm::Type *pointee)
    {
      if (!builder || !frame_ptr || !pointee)
      {
        return nullptr;
      }
      llvm::Value *addr = AsyncFrameAddr(emitter, builder, frame_ptr, offset);
      if (!addr)
      {
        return nullptr;
      }
      return builder->CreateBitCast(addr, llvm::PointerType::get(pointee, 0));
    }

    llvm::Value *NullOpaquePtr(LLVMEmitter &emitter)
    {
      return llvm::ConstantPointerNull::get(
          llvm::cast<llvm::PointerType>(emitter.GetOpaquePtr()));
    }

    llvm::Value *CoerceTo(llvm::IRBuilder<> *builder,
                          llvm::Value *value,
                          llvm::Type *target_ty);

    llvm::Value *CoerceToTyped(LLVMEmitter &emitter,
                               llvm::IRBuilder<> *builder,
                               llvm::Value *value,
                               llvm::Type *target_ty,
                               const analysis::TypeRef &source_type,
                               const analysis::TypeRef &target_type);

    llvm::Value *CoerceOrNullOpaquePtr(LLVMEmitter &emitter,
                                       llvm::IRBuilder<> *builder,
                                       llvm::Value *value)
    {
      if (!builder)
      {
        return NullOpaquePtr(emitter);
      }
      if (!value)
      {
        return NullOpaquePtr(emitter);
      }
      llvm::Value *coerced = CoerceTo(builder, value, emitter.GetOpaquePtr());
      if (coerced)
      {
        return coerced;
      }
      if (value->getType()->isPointerTy())
      {
        coerced = builder->CreateBitCast(value, emitter.GetOpaquePtr());
      }
      return coerced ? coerced : NullOpaquePtr(emitter);
    }

    llvm::Value *EmitRuntimeCallBySymbol(LLVMEmitter &emitter,
                                         llvm::IRBuilder<> *builder,
                                         const std::string &symbol,
                                         const std::vector<llvm::Value *> &args)
    {
      if (!builder)
      {
        return nullptr;
      }
      std::optional<RuntimeFuncInfo> info = GetRuntimeFuncInfo(symbol);
      if (!info.has_value())
      {
        return nullptr;
      }

      llvm::Function *fn = emitter.GetModule().getFunction(symbol);
      constexpr bool kUseCAbiAggregateSRet = true;
      if (!fn)
      {
        ABICallResult abi = emitter.ComputeCallABI(
            info->params,
            info->ret,
            kUseCAbiAggregateSRet);
        if (abi.func_type)
        {
          fn = llvm::Function::Create(
              abi.func_type,
              llvm::GlobalValue::ExternalLinkage,
              symbol,
              &emitter.GetModule());
          fn->setCallingConv(llvm::CallingConv::C);
        }
      }
      if (!fn)
      {
        return nullptr;
      }

      return EmitABICall(
          emitter,
          builder,
          fn,
          info->params,
          info->ret,
          args,
          kUseCAbiAggregateSRet);
    }

    llvm::Value *EmitAsyncResumeRuntimeCall(LLVMEmitter &emitter,
                                            llvm::IRBuilder<> *builder,
                                            llvm::Value *suspended,
                                            llvm::Value *input,
                                            llvm::Value *panic_out)
    {
      if (!builder)
      {
        return nullptr;
      }
      return EmitRuntimeCallBySymbol(
          emitter,
          builder,
          BuiltinSymAsyncResume(),
          {
              CoerceOrNullOpaquePtr(emitter, builder, suspended),
              CoerceOrNullOpaquePtr(emitter, builder, input),
              CoerceOrNullOpaquePtr(emitter, builder, panic_out),
          });
    }

    void StoreAsyncFrameKeySnapshot(LLVMEmitter &emitter,
                                    llvm::IRBuilder<> *builder,
                                    llvm::Value *frame_ptr,
                                    llvm::Value *released_handle)
    {
      if (!builder || !frame_ptr)
      {
        return;
      }
      llvm::Value *slot_ptr = AsyncFrameTypedPtr(
          emitter,
          builder,
          frame_ptr,
          kAsyncFrameKeySnapshotOffset,
          emitter.GetOpaquePtr());
      if (!slot_ptr)
      {
        return;
      }
      llvm::Value *stored = CoerceOrNullOpaquePtr(emitter, builder, released_handle);
      builder->CreateStore(stored, slot_ptr);
    }

    void StoreAsyncFrameHostedEnv(LLVMEmitter &emitter,
                                  llvm::IRBuilder<> *builder,
                                  llvm::Value *frame_ptr,
                                  llvm::Value *hosted_env)
    {
      if (!builder || !frame_ptr)
      {
        return;
      }
      llvm::Value *slot_ptr = AsyncFrameTypedPtr(
          emitter,
          builder,
          frame_ptr,
          kAsyncFrameHostedEnvOffset,
          emitter.GetOpaquePtr());
      if (!slot_ptr)
      {
        return;
      }
      llvm::Value *stored = CoerceOrNullOpaquePtr(emitter, builder, hosted_env);
      builder->CreateStore(stored, slot_ptr);
    }

    llvm::Value *LoadAsyncFrameKeySnapshot(LLVMEmitter &emitter,
                                           llvm::IRBuilder<> *builder,
                                           llvm::Value *frame_ptr)
    {
      if (!builder || !frame_ptr)
      {
        return NullOpaquePtr(emitter);
      }
      llvm::Value *slot_ptr = AsyncFrameTypedPtr(
          emitter,
          builder,
          frame_ptr,
          kAsyncFrameKeySnapshotOffset,
          emitter.GetOpaquePtr());
      if (!slot_ptr)
      {
        return NullOpaquePtr(emitter);
      }
      return builder->CreateLoad(emitter.GetOpaquePtr(), slot_ptr);
    }

    llvm::Value *EmitKeyReleaseAll(LLVMEmitter &emitter,
                                   llvm::IRBuilder<> *builder)
    {
      llvm::Value *released = EmitRuntimeCallBySymbol(
          emitter,
          builder,
          ConcurrencySymKeyReleaseAll(),
          {});
      return CoerceOrNullOpaquePtr(emitter, builder, released);
    }

    void EmitKeyReacquire(LLVMEmitter &emitter,
                          llvm::IRBuilder<> *builder,
                          llvm::Value *released_handle)
    {
      if (!builder)
      {
        return;
      }
      std::vector<llvm::Value *> args;
      args.push_back(CoerceOrNullOpaquePtr(emitter, builder, released_handle));
      (void)EmitRuntimeCallBySymbol(
          emitter,
          builder,
          ConcurrencySymKeyReacquire(),
          args);
    }

    llvm::Value *LoadLocalValue(LLVMEmitter &emitter,
                                llvm::IRBuilder<> *builder,
                                const std::string &name)
    {
      if (!builder)
      {
        return nullptr;
      }
      IRValue local;
      local.kind = IRValue::Kind::Local;
      local.name = name;
      return emitter.EvaluateIRValue(local);
    }

    bool StoreProcedureOutValue(LLVMEmitter &emitter,
                                llvm::IRBuilder<> *builder,
                                llvm::Function *func,
                                const std::string &symbol,
                                const LowerCtx::ProcSigInfo *sig,
                                llvm::Value *value,
                                const analysis::TypeRef &source_type)
    {
      if (!builder || !func || !sig)
      {
        return false;
      }

      llvm::Type *out_ty = emitter.GetLLVMType(sig->ret);
      if (!out_ty || out_ty->isVoidTy())
      {
        return false;
      }

      llvm::Value *stored =
          CoerceToTyped(emitter, builder, value, out_ty, source_type, sig->ret);
      if (!stored)
      {
        stored = CoerceTo(builder, value, out_ty);
      }
      if (!stored)
      {
        stored = llvm::Constant::getNullValue(out_ty);
      }

      auto store_to_ptr = [&](llvm::Value *out_ptr) -> bool
      {
        if (!out_ptr)
        {
          return false;
        }
        llvm::Type *target_ptr_ty = llvm::PointerType::get(out_ty, 0);
        if (out_ptr->getType()->isIntegerTy())
        {
          out_ptr = builder->CreateIntToPtr(out_ptr, target_ptr_ty);
        }
        else
        {
          llvm::Value *coerced = CoerceTo(builder, out_ptr, target_ptr_ty);
          if (coerced)
          {
            out_ptr = coerced;
          }
          else if (out_ptr->getType()->isPointerTy())
          {
            out_ptr = builder->CreateBitCast(out_ptr, target_ptr_ty);
          }
        }
        if (!out_ptr || !out_ptr->getType()->isPointerTy())
        {
          return false;
        }
        builder->CreateStore(stored, out_ptr);
        return true;
      };

      if (HasNamedParam(sig->params, kAsyncOutParamName))
      {
        llvm::Value *explicit_out =
            LoadLocalValue(emitter, builder, std::string(kAsyncOutParamName));
        if (!explicit_out)
        {
          if (const LowerCtx *ctx = emitter.GetCurrentCtx())
          {
            const_cast<LowerCtx *>(ctx)->ReportCodegenFailure();
          }
          return false;
        }
        if (!store_to_ptr(explicit_out))
        {
          if (const LowerCtx *ctx = emitter.GetCurrentCtx())
          {
            const_cast<LowerCtx *>(ctx)->ReportCodegenFailure();
          }
          return false;
        }
        return true;
      }

      ABICallResult abi = emitter.ComputeProcABI(symbol, sig->params, sig->ret);
      if (!abi.valid || !abi.has_sret || func->arg_size() == 0)
      {
        return false;
      }

      return store_to_ptr(func->getArg(0));
    }

    llvm::Value *ResolveProcedureOutPtr(LLVMEmitter &emitter,
                                        llvm::IRBuilder<> *builder,
                                        llvm::Function *func,
                                        const std::string &symbol,
                                        const LowerCtx::ProcSigInfo *sig)
    {
      if (!builder || !func || !sig)
      {
        return nullptr;
      }

      llvm::Type *out_ty = emitter.GetLLVMType(sig->ret);
      if (!out_ty || out_ty->isVoidTy())
      {
        return nullptr;
      }

      auto normalize_out_ptr = [&](llvm::Value *out_ptr) -> llvm::Value *
      {
        if (!out_ptr)
        {
          return nullptr;
        }

        llvm::Type *target_ptr_ty = llvm::PointerType::get(out_ty, 0);
        if (out_ptr->getType()->isIntegerTy())
        {
          out_ptr = builder->CreateIntToPtr(out_ptr, target_ptr_ty);
        }
        else
        {
          llvm::Value *coerced = CoerceTo(builder, out_ptr, target_ptr_ty);
          if (coerced)
          {
            out_ptr = coerced;
          }
          else if (out_ptr->getType()->isPointerTy())
          {
            out_ptr = builder->CreateBitCast(out_ptr, target_ptr_ty);
          }
        }

        return (out_ptr && out_ptr->getType()->isPointerTy()) ? out_ptr : nullptr;
      };

      if (HasNamedParam(sig->params, kAsyncOutParamName))
      {
        llvm::Value *explicit_out =
            LoadLocalValue(emitter, builder, std::string(kAsyncOutParamName));
        return normalize_out_ptr(explicit_out);
      }

      ABICallResult abi = emitter.ComputeProcABI(symbol, sig->params, sig->ret);
      if (!abi.valid || !abi.has_sret || func->arg_size() == 0)
      {
        return nullptr;
      }

      return normalize_out_ptr(func->getArg(0));
    }

    std::optional<std::size_t> ParseTupleFieldIndex(std::string_view text)
    {
      if (text.empty())
      {
        return std::nullopt;
      }
      std::size_t index = 0;
      for (char ch : text)
      {
        if (ch < '0' || ch > '9')
        {
          return std::nullopt;
        }
        index = index * 10 + static_cast<std::size_t>(ch - '0');
      }
      return index;
    }

    struct FieldAccessMeta
    {
      std::size_t index = 0;
      analysis::TypeRef field_type;
      std::vector<analysis::TypeRef> aggregate_fields;
      ::cursive::analysis::layout::RecordLayoutOptions layout_options{};
    };

    analysis::TypeRef ResolveAliasTypeInScope(const analysis::ScopeContext &scope,
                                              const analysis::TypeRef &type,
                                              std::size_t depth = 0)
    {
      analysis::TypeRef stripped = analysis::StripPerm(type);
      if (!stripped)
      {
        stripped = type;
      }
      if (!stripped || depth > 16)
      {
        return stripped;
      }

      const auto *path = std::get_if<analysis::TypePathType>(&stripped->node);
      if (!path)
      {
        return stripped;
      }

      ast::Path syntax_path;
      syntax_path.reserve(path->path.size());
      for (const auto &seg : path->path)
      {
        syntax_path.push_back(seg);
      }
      const auto it = scope.sigma.types.find(analysis::PathKeyOf(syntax_path));
      if (it == scope.sigma.types.end())
      {
        return stripped;
      }

      const auto *alias = std::get_if<ast::TypeAliasDecl>(&it->second);
      if (!alias)
      {
        return stripped;
      }

      const auto lowered = ::cursive::analysis::layout::LowerTypeForLayout(scope, alias->type);
      if (!lowered.has_value())
      {
        return stripped;
      }

      analysis::TypeRef inst = *lowered;
      if (alias->generic_params &&
          !alias->generic_params->params.empty() &&
          !path->generic_args.empty())
      {
        analysis::TypeSubst subst =
            analysis::BuildSubstitution(alias->generic_params->params,
                                        path->generic_args);
        inst = analysis::InstantiateType(inst, subst);
      }
      return ResolveAliasTypeInScope(scope, inst, depth + 1);
    }

    std::optional<FieldAccessMeta> ResolveFieldAccessMeta(
        const analysis::ScopeContext &scope,
        const analysis::TypeRef &base_type,
        std::string_view field_name)
    {
      analysis::TypeRef stripped = ResolveAliasTypeInScope(scope, base_type);
      if (!stripped)
      {
        stripped = base_type;
      }
      if (!stripped)
      {
        return std::nullopt;
      }

      if (const auto *tuple = std::get_if<analysis::TypeTuple>(&stripped->node))
      {
        std::optional<std::size_t> index = ParseTupleFieldIndex(field_name);
        if (!index.has_value())
        {
          if (tuple->elements.size() == 1)
          {
            index = 0;
          }
          else
          {
            return std::nullopt;
          }
        }
        if (*index >= tuple->elements.size())
        {
          return std::nullopt;
        }
        FieldAccessMeta meta;
        meta.index = *index;
        meta.field_type = tuple->elements[*index];
        meta.aggregate_fields = tuple->elements;
        return meta;
      }

      if (const auto *modal_state = std::get_if<analysis::TypeModalState>(&stripped->node))
      {
        const ast::ModalDecl *modal_decl = analysis::LookupModalDecl(scope, modal_state->path);
        if (!modal_decl)
        {
          return std::nullopt;
        }
        analysis::TypeSubst modal_subst;
        if (modal_decl->generic_params && !modal_decl->generic_params->params.empty())
        {
          if (modal_state->generic_args.size() > modal_decl->generic_params->params.size())
          {
            return std::nullopt;
          }
          modal_subst = analysis::BuildSubstitution(
              modal_decl->generic_params->params,
              modal_state->generic_args);
        }

        const ast::StateBlock *state_block = nullptr;
        for (const auto &state : modal_decl->states)
        {
          if (analysis::IdEq(state.name, modal_state->state))
          {
            state_block = &state;
            break;
          }
        }
        if (!state_block)
        {
          return std::nullopt;
        }

        FieldAccessMeta meta;
        bool found = false;
        std::size_t field_index = 0;
        for (const auto &member : state_block->members)
        {
          const auto *field = std::get_if<ast::StateFieldDecl>(&member);
          if (!field)
          {
            continue;
          }
          auto lowered = ::cursive::analysis::layout::LowerTypeForLayout(scope, field->type);
          analysis::TypeRef lowered_type = analysis::MakeTypePrim("u8");
          if (lowered.has_value())
          {
            lowered_type = *lowered;
            if (!modal_subst.empty())
            {
              lowered_type = analysis::InstantiateType(lowered_type, modal_subst);
            }
          }
          meta.aggregate_fields.push_back(lowered_type);
          if (analysis::IdEq(field->name, field_name))
          {
            meta.index = field_index;
            meta.field_type = lowered_type;
            found = true;
          }
          ++field_index;
        }
        if (!found)
        {
          return std::nullopt;
        }
        return meta;
      }

      const auto *path = std::get_if<analysis::TypePathType>(&stripped->node);
      if (!path)
      {
        return std::nullopt;
      }
      const ast::RecordDecl *record = analysis::LookupRecordDecl(scope, path->path);
      if (!record && !path->path.empty())
      {
        auto suffix_matches = [&](const analysis::PathKey &key) -> bool
        {
          if (key.size() < path->path.size())
          {
            return false;
          }
          const std::size_t offset = key.size() - path->path.size();
          for (std::size_t i = 0; i < path->path.size(); ++i)
          {
            if (!analysis::IdEq(key[offset + i], path->path[i]))
            {
              return false;
            }
          }
          return true;
        };

        auto in_current_module = [&](const analysis::PathKey &key) -> bool
        {
          if (scope.current_module.empty() || key.size() < scope.current_module.size())
          {
            return false;
          }
          for (std::size_t i = 0; i < scope.current_module.size(); ++i)
          {
            if (!analysis::IdEq(key[i], scope.current_module[i]))
            {
              return false;
            }
          }
          return true;
        };

        const ast::RecordDecl *unique_suffix_match = nullptr;
        bool suffix_ambiguous = false;
        const ast::RecordDecl *module_suffix_match = nullptr;
        bool module_ambiguous = false;

        for (const auto &[type_key, type_decl] : scope.sigma.types)
        {
          const auto *rec = std::get_if<ast::RecordDecl>(&type_decl);
          if (!rec || !suffix_matches(type_key))
          {
            continue;
          }
          if (!unique_suffix_match)
          {
            unique_suffix_match = rec;
          }
          else if (unique_suffix_match != rec)
          {
            suffix_ambiguous = true;
          }
          if (in_current_module(type_key))
          {
            if (!module_suffix_match)
            {
              module_suffix_match = rec;
            }
            else if (module_suffix_match != rec)
            {
              module_ambiguous = true;
            }
          }
        }

        if (module_suffix_match && !module_ambiguous)
        {
          record = module_suffix_match;
        }
        else if (unique_suffix_match && !suffix_ambiguous)
        {
          record = unique_suffix_match;
        }
      }
      if (!record)
      {
        return std::nullopt;
      }
      analysis::TypeSubst record_subst;
      ::cursive::analysis::layout::RecordLayoutOptions record_layout_options{};
      if (record->generic_params && !record->generic_params->params.empty())
      {
        if (path->generic_args.size() > record->generic_params->params.size())
        {
          return std::nullopt;
        }
        record_subst = analysis::BuildSubstitution(
            record->generic_params->params,
            path->generic_args);
      }
      record_layout_options = ::cursive::analysis::layout::ResolveRecordLayoutOptions(record->attrs);

      FieldAccessMeta meta;
      bool found = false;
      std::size_t field_index = 0;
      for (const auto &member : record->members)
      {
        const auto *field = std::get_if<ast::FieldDecl>(&member);
        if (!field)
        {
          continue;
        }
        auto lowered = ::cursive::analysis::layout::LowerTypeForLayout(scope, field->type);
        analysis::TypeRef lowered_type = analysis::MakeTypePrim("u8");
        if (lowered.has_value())
        {
          lowered_type = *lowered;
          if (!record_subst.empty())
          {
            lowered_type = analysis::InstantiateType(lowered_type, record_subst);
          }
        }
        meta.aggregate_fields.push_back(lowered_type);
        if (analysis::IdEq(field->name, field_name))
        {
          meta.index = field_index;
          meta.field_type = lowered_type;
          found = true;
        }
        ++field_index;
      }
      if (!found)
      {
        return std::nullopt;
      }
      meta.layout_options = record_layout_options;
      return meta;
    }

    llvm::Value *CoerceTo(llvm::IRBuilder<> *builder,
                          llvm::Value *value,
                          llvm::Type *target_ty)
    {
      if (!builder || !value || !target_ty)
      {
        return value;
      }
      if (value->getType() == target_ty)
      {
        return value;
      }
      return CoerceValue(builder, value, target_ty);
    }

    analysis::TypeRef StripPermType(const analysis::TypeRef &type)
    {
      if (!type)
      {
        return nullptr;
      }
      if (analysis::TypeRef stripped = analysis::StripPerm(type))
      {
        return stripped;
      }
      return type;
    }

    analysis::TypeRef ResolveAliasType(const LowerCtx *ctx,
                                       const analysis::TypeRef &type,
                                       std::size_t depth = 0)
    {
      analysis::TypeRef stripped = StripPermType(type);
      if (!stripped || !ctx || !ctx->sigma || depth > 16)
      {
        return stripped;
      }
      const analysis::ScopeContext &scope = BuildScope(ctx);
      return ResolveAliasTypeInScope(scope, stripped, depth);
    }

    bool IsUnitType(const analysis::TypeRef &type)
    {
      analysis::TypeRef stripped = StripPermType(type);
      if (!stripped)
      {
        return false;
      }
      const auto *prim = std::get_if<analysis::TypePrim>(&stripped->node);
      return prim && prim->name == "()";
    }

    bool IsBoolType(const analysis::TypeRef &type)
    {
      analysis::TypeRef stripped = StripPermType(type);
      if (!stripped)
      {
        return false;
      }
      const auto *prim = std::get_if<analysis::TypePrim>(&stripped->node);
      return prim && prim->name == "bool";
    }

    bool IsBoolBinOp(std::string_view op)
    {
      return op == "==" || op == "===" || op == "!=" || op == "<" || op == "<=" || op == ">" ||
             op == ">=" || op == "&&" || op == "||";
    }

    bool IsNeverType(const analysis::TypeRef &type)
    {
      analysis::TypeRef stripped = StripPermType(type);
      if (!stripped)
      {
        return false;
      }
      const auto *prim = std::get_if<analysis::TypePrim>(&stripped->node);
      return prim && prim->name == "!";
    }

    std::optional<std::size_t> FindUnionMemberIndex(
        const std::vector<analysis::TypeRef> &members,
        const analysis::TypeRef &member_type)
    {
      analysis::TypeRef stripped_member = StripPermType(member_type);
      if (!stripped_member)
      {
        return std::nullopt;
      }
      for (std::size_t i = 0; i < members.size(); ++i)
      {
        const auto equiv = analysis::TypeEquiv(members[i], stripped_member);
        if (equiv.ok && equiv.equiv)
        {
          return i;
        }
      }
      return std::nullopt;
    }

    std::optional<std::size_t> InferUnionMemberIndexFromValue(
        LLVMEmitter &emitter,
        llvm::Value *source_value,
        const std::vector<analysis::TypeRef> &members)
    {
      if (!source_value || members.empty())
      {
        return std::nullopt;
      }

      llvm::Type *source_ty = source_value->getType();

      std::optional<std::size_t> exact_bool_match;
      if (source_ty->isIntegerTy() && source_ty->getIntegerBitWidth() > 1)
      {
        // For non-boolean integer immediates, prefer numeric members over bool.
        for (std::size_t i = 0; i < members.size(); ++i)
        {
          llvm::Type *member_ty = emitter.GetLLVMType(members[i]);
          if (!member_ty || member_ty != source_ty)
          {
            continue;
          }
          if (IsBoolType(members[i]))
          {
            exact_bool_match = i;
            continue;
          }
          return i;
        }
      }
      else
      {
        for (std::size_t i = 0; i < members.size(); ++i)
        {
          llvm::Type *member_ty = emitter.GetLLVMType(members[i]);
          if (member_ty && member_ty == source_ty)
          {
            return i;
          }
        }
      }

      // Bool literals are currently materialized as i1 immediates.
      if (source_ty->isIntegerTy(1))
      {
        for (std::size_t i = 0; i < members.size(); ++i)
        {
          if (IsBoolType(members[i]))
          {
            return i;
          }
        }
      }

      // Integer fallback: choose the narrowest non-bool integer member that can hold source bits.
      if (source_ty->isIntegerTy())
      {
        const unsigned source_bits = source_ty->getIntegerBitWidth();
        std::optional<std::size_t> best_index;
        unsigned best_bits = 0;
        for (std::size_t i = 0; i < members.size(); ++i)
        {
          if (IsBoolType(members[i]))
          {
            continue;
          }
          llvm::Type *member_ty = emitter.GetLLVMType(members[i]);
          if (!member_ty || !member_ty->isIntegerTy())
          {
            continue;
          }
          const unsigned member_bits = member_ty->getIntegerBitWidth();
          if (member_bits < source_bits)
          {
            continue;
          }
          if (!best_index.has_value() || member_bits < best_bits)
          {
            best_index = i;
            best_bits = member_bits;
          }
        }
        if (best_index.has_value())
        {
          return best_index;
        }

        // For integer literals lowered as wider immediates (e.g., i64 fallback),
        // allow narrowing when the constant value provably fits the member width.
        if (const auto *cst = llvm::dyn_cast<llvm::ConstantInt>(source_value))
        {
          const llvm::APInt source_ap = cst->getValue();
          std::optional<std::size_t> narrow_index;
          unsigned narrow_bits = 0;
          for (std::size_t i = 0; i < members.size(); ++i)
          {
            if (IsBoolType(members[i]))
            {
              continue;
            }
            llvm::Type *member_ty = emitter.GetLLVMType(members[i]);
            if (!member_ty || !member_ty->isIntegerTy())
            {
              continue;
            }
            const unsigned member_bits = member_ty->getIntegerBitWidth();
            llvm::APInt probe = source_ap;
            if (member_bits < source_ap.getBitWidth())
            {
              probe = source_ap.trunc(member_bits);
              const bool fits_signed = probe.sext(source_ap.getBitWidth()) == source_ap;
              const bool fits_unsigned = probe.zext(source_ap.getBitWidth()) == source_ap;
              if (!fits_signed && !fits_unsigned)
              {
                continue;
              }
            }
            if (!narrow_index.has_value() || member_bits < narrow_bits)
            {
              narrow_index = i;
              narrow_bits = member_bits;
            }
          }
          if (narrow_index.has_value())
          {
            return narrow_index;
          }
        }
      }

      if (exact_bool_match.has_value())
      {
        return exact_bool_match;
      }

      return std::nullopt;
    }

    bool UnionDebugEnabled()
    {
      return core::IsDebugEnabled("union");
    }

    llvm::Value *PackUnionFromMember(LLVMEmitter &emitter,
                                     llvm::IRBuilder<> *builder,
                                     llvm::Value *source_value,
                                     llvm::Type *target_ty,
                                     const analysis::TypeRef &source_type,
                                     const analysis::TypeRef &target_type)
    {
      if (!builder || !source_value || !target_ty)
      {
        return nullptr;
      }

      analysis::TypeRef stripped_target = StripPermType(target_type);
      const auto *target_union =
          stripped_target ? std::get_if<analysis::TypeUnion>(&stripped_target->node) : nullptr;
      if (!target_union)
      {
        return nullptr;
      }

      const LowerCtx *ctx = emitter.GetCurrentCtx();
      if (!ctx || !ctx->sigma)
      {
        return nullptr;
      }
      const analysis::ScopeContext &scope = BuildScope(ctx);
      const auto current_fn_name = [&]() -> std::string
      {
        if (!builder || !builder->GetInsertBlock())
        {
          return "<no-func>";
        }
        if (llvm::Function *fn = builder->GetInsertBlock()->getParent())
        {
          return fn->getName().str();
        }
        return "<no-func>";
      };
      const auto union_layout = ::cursive::analysis::layout::UnionLayoutOf(scope, *target_union);
      if (!union_layout.has_value())
      {
        if (UnionDebugEnabled())
        {
          std::cerr << "[union-debug] fn=" << current_fn_name() << " pack: no layout\n";
        }
        return nullptr;
      }
      if (UnionDebugEnabled())
      {
        std::cerr << "[union-debug] fn=" << current_fn_name() << " members:";
        for (const auto &member : union_layout->member_list)
        {
          analysis::TypeRef stripped = StripPermType(member);
          if (const auto *prim = stripped ? std::get_if<analysis::TypePrim>(&stripped->node) : nullptr)
          {
            std::cerr << " " << prim->name;
          }
          else if (stripped && std::holds_alternative<analysis::TypeUnion>(stripped->node))
          {
            std::cerr << " <union>";
          }
          else if (stripped && std::holds_alternative<analysis::TypeTuple>(stripped->node))
          {
            std::cerr << " <tuple>";
          }
          else
          {
            std::cerr << " <other>";
          }
        }
        std::cerr << "\n";
      }

      std::optional<std::size_t> member_index =
          FindUnionMemberIndex(union_layout->member_list, source_type);
      if (!member_index.has_value())
      {
        member_index = InferUnionMemberIndexFromValue(emitter, source_value, union_layout->member_list);
      }
      if (!member_index.has_value() || *member_index >= union_layout->member_list.size())
      {
        if (UnionDebugEnabled())
        {
          std::cerr << "[union-debug] fn=" << current_fn_name() << " pack: no member index src_ty="
                    << (source_value ? source_value->getType()->isIntegerTy() ? "int" : "other" : "null")
                    << " members=" << union_layout->member_list.size() << "\n";
        }
        return nullptr;
      }
      if (UnionDebugEnabled())
      {
        std::cerr << "[union-debug] fn=" << current_fn_name()
                  << " pack: selected member index=" << *member_index
                  << " niche=" << (union_layout->niche ? 1 : 0) << "\n";
      }
      const analysis::TypeRef member_type = union_layout->member_list[*member_index];

      if (union_layout->niche)
      {
        std::optional<std::size_t> payload_index;
        for (std::size_t i = 0; i < union_layout->member_list.size(); ++i)
        {
          if (!IsUnitType(union_layout->member_list[i]))
          {
            payload_index = i;
            break;
          }
        }
        if (!payload_index.has_value())
        {
          return nullptr;
        }
        if (*member_index != *payload_index)
        {
          return llvm::Constant::getNullValue(target_ty);
        }
        llvm::Value *packed = source_value;
        if (llvm::Type *payload_ty = emitter.GetLLVMType(member_type))
        {
          packed = CoerceTo(builder, packed, payload_ty);
          if (!packed)
          {
            packed = llvm::Constant::getNullValue(payload_ty);
          }
        }
        return packed;
      }

      auto *union_struct_ty = llvm::dyn_cast<llvm::StructType>(target_ty);
      if (!union_struct_ty || union_struct_ty->getNumElements() < 2)
      {
        return nullptr;
      }

      llvm::Value *union_value = llvm::Constant::getNullValue(union_struct_ty);
      llvm::Type *disc_ty = union_struct_ty->getElementType(0);
      llvm::Value *disc = CoerceTo(
          builder,
          llvm::ConstantInt::get(llvm::Type::getInt64Ty(emitter.GetContext()), *member_index),
          disc_ty);
      if (!disc)
      {
        disc = llvm::Constant::getNullValue(disc_ty);
      }
      union_value = builder->CreateInsertValue(union_value, disc, {0u});

      if (IsUnitType(member_type))
      {
        return union_value;
      }

      llvm::Type *member_ty = emitter.GetLLVMType(member_type);
      if (!member_ty)
      {
        return union_value;
      }

      llvm::Value *coerced_member = CoerceTo(builder, source_value, member_ty);
      if (!coerced_member)
      {
        coerced_member = llvm::Constant::getNullValue(member_ty);
      }

      llvm::Function *current_fn =
          builder->GetInsertBlock() ? builder->GetInsertBlock()->getParent() : nullptr;
      if (!current_fn)
      {
        return union_value;
      }

      llvm::IRBuilder<> entry_builder(
          &current_fn->getEntryBlock(),
          current_fn->getEntryBlock().begin());
      llvm::AllocaInst *union_slot = entry_builder.CreateAlloca(union_struct_ty);
      builder->CreateStore(union_value, union_slot);

      llvm::Value *payload_i8 = CreateTaggedPayloadI8Ptr(
          emitter,
          builder,
          union_struct_ty,
          union_slot,
          union_layout->payload_align);
      if (!payload_i8)
      {
        return union_value;
      }
      llvm::Value *field_ptr = builder->CreateBitCast(
          payload_i8,
          llvm::PointerType::get(member_ty, 0));
      llvm::StoreInst *store = builder->CreateStore(coerced_member, field_ptr);
      store->setAlignment(llvm::Align(1));
      return builder->CreateLoad(union_struct_ty, union_slot);
    }

    llvm::Value *CoerceToTyped(LLVMEmitter &emitter,
                               llvm::IRBuilder<> *builder,
                               llvm::Value *value,
                               llvm::Type *target_ty,
                               const analysis::TypeRef &source_type,
                               const analysis::TypeRef &target_type)
    {
      if (!builder || !value || !target_ty)
      {
        return value;
      }

      const LowerCtx *ctx = emitter.GetCurrentCtx();
      const auto current_fn_name = [&]() -> std::string
      {
        if (!builder || !builder->GetInsertBlock())
        {
          return "<no-func>";
        }
        if (llvm::Function *fn = builder->GetInsertBlock()->getParent())
        {
          return fn->getName().str();
        }
        return "<no-func>";
      };
      analysis::TypeRef stripped_source = ResolveAliasType(ctx, source_type);
      analysis::TypeRef stripped_target = ResolveAliasType(ctx, target_type);

      const auto *source_array =
          stripped_source ? std::get_if<analysis::TypeArray>(&stripped_source->node) : nullptr;
      const auto *target_array =
          stripped_target ? std::get_if<analysis::TypeArray>(&stripped_target->node) : nullptr;
      if (source_array && target_array &&
          source_array->length == target_array->length)
      {
        auto *target_array_ty = llvm::dyn_cast<llvm::ArrayType>(target_ty);
        llvm::Value *array_value = value;
        if (array_value && array_value->getType()->isPointerTy())
        {
          llvm::Type *source_array_ty = emitter.GetLLVMType(stripped_source);
          if (source_array_ty)
          {
            llvm::Value *typed_ptr = array_value;
            llvm::Type *source_ptr_ty = llvm::PointerType::get(source_array_ty, 0);
            if (typed_ptr->getType() != source_ptr_ty)
            {
              typed_ptr = builder->CreateBitCast(typed_ptr, source_ptr_ty);
            }
            array_value = builder->CreateLoad(source_array_ty, typed_ptr);
          }
        }
        if (target_array_ty && array_value && array_value->getType()->isArrayTy() &&
            llvm::cast<llvm::ArrayType>(array_value->getType())->getNumElements() ==
                target_array_ty->getNumElements())
        {
          llvm::Value *out = llvm::UndefValue::get(target_array_ty);
          llvm::Type *target_elem_ty = target_array_ty->getElementType();
          for (std::uint64_t i = 0; i < target_array->length; ++i)
          {
            llvm::Value *elem =
                builder->CreateExtractValue(array_value, {static_cast<unsigned>(i)});
            llvm::Value *coerced = CoerceToTyped(
                emitter,
                builder,
                elem,
                target_elem_ty,
                source_array->element,
                target_array->element);
            if (!coerced)
            {
              coerced = CoerceTo(builder, elem, target_elem_ty);
            }
            if (!coerced)
            {
              coerced = llvm::Constant::getNullValue(target_elem_ty);
            }
            out = builder->CreateInsertValue(out, coerced, {static_cast<unsigned>(i)});
          }
          return out;
        }
      }

      const auto *target_union =
          stripped_target ? std::get_if<analysis::TypeUnion>(&stripped_target->node) : nullptr;
      if (!target_union)
      {
        return CoerceTo(builder, value, target_ty);
      }

      if (UnionDebugEnabled())
      {
        const std::string source_text =
            stripped_source ? analysis::TypeToString(stripped_source) : std::string("<null>");
        const std::string target_text =
            stripped_target ? analysis::TypeToString(stripped_target) : std::string("<null>");
        std::cerr << "[union-debug] fn=" << current_fn_name()
                  << " typed-coerce: source_type="
                  << (stripped_source ? "known" : "unknown")
                  << " source=" << source_text
                  << " target=" << target_text
                  << " source_llvm="
                  << (value && value->getType()->isIntegerTy() ? "int" : value && value->getType()->isStructTy() ? "struct"
                                                                     : value && value->getType()->isPointerTy()  ? "ptr"
                                                                                                                 : "other")
                  << " target_llvm="
                  << (target_ty && target_ty->isIntegerTy() ? "int" : target_ty && target_ty->isStructTy() ? "struct"
                                                                  : target_ty && target_ty->isPointerTy()  ? "ptr"
                                                                                                           : "other")
                  << "\n";
      }
      if (stripped_source && std::holds_alternative<analysis::TypeUnion>(stripped_source->node))
      {
        const auto equiv = analysis::TypeEquiv(stripped_source, stripped_target);
        if (equiv.ok && equiv.equiv)
        {
          // Preserve direct union->union coercion only when the runtime value is
          // already carried in union representation. If the source value is still
          // a concrete member payload (e.g. literal bound to union-typed place),
          // we must materialize tag+payload via PackUnionFromMember below.
          if (value->getType() == target_ty)
          {
            return CoerceTo(builder, value, target_ty);
          }
        }
      }

      if (llvm::Value *packed = PackUnionFromMember(
              emitter, builder, value, target_ty, stripped_source, stripped_target))
      {
        return packed;
      }

      return CoerceTo(builder, value, target_ty);
    }

    llvm::Value *AsBool(llvm::IRBuilder<> *builder, llvm::Value *value)
    {
      if (!builder || !value)
      {
        return llvm::ConstantInt::getFalse(builder->getContext());
      }
      llvm::Type *ty = value->getType();
      if (ty->isIntegerTy(1))
      {
        return value;
      }
      if (ty->isIntegerTy())
      {
        llvm::Value *zero = llvm::ConstantInt::get(ty, 0);
        return builder->CreateICmpNE(value, zero);
      }
      if (ty->isPointerTy())
      {
        llvm::Value *null_ptr = llvm::ConstantPointerNull::get(
            llvm::cast<llvm::PointerType>(ty));
        return builder->CreateICmpNE(value, null_ptr);
      }
      return llvm::ConstantInt::getFalse(builder->getContext());
    }

    llvm::Value *EmitTypedEq(llvm::IRBuilder<> *builder,
                             llvm::Value *lhs,
                             llvm::Value *rhs);

    bool DebugTargetEnumPath(const std::vector<std::string> &path)
    {
      return !path.empty() && analysis::IdEq(path.back(), "TypeEnumCase");
    }

    std::string LLVMValueRepr(llvm::Value *value)
    {
      if (!value)
      {
        return "<null>";
      }
      std::string out;
      llvm::raw_string_ostream os(out);
      value->print(os);
      return os.str();
    }

    llvm::Value *EmitAggregateEq(llvm::IRBuilder<> *builder,
                                 llvm::Value *lhs,
                                 llvm::Value *rhs)
    {
      if (!builder || !lhs || !rhs)
      {
        return llvm::ConstantInt::getFalse(builder->getContext());
      }
      if (lhs->getType() != rhs->getType())
      {
        return nullptr;
      }

      llvm::Type *ty = lhs->getType();
      if (auto *struct_ty = llvm::dyn_cast<llvm::StructType>(ty))
      {
        llvm::Value *acc = llvm::ConstantInt::getTrue(builder->getContext());
        for (unsigned i = 0; i < struct_ty->getNumElements(); ++i)
        {
          llvm::Value *lhs_elem = builder->CreateExtractValue(lhs, {i});
          llvm::Value *rhs_elem = builder->CreateExtractValue(rhs, {i});
          llvm::Value *elem_eq = EmitTypedEq(builder, lhs_elem, rhs_elem);
          acc = builder->CreateAnd(acc, AsBool(builder, elem_eq));
        }
        return acc;
      }

      if (auto *array_ty = llvm::dyn_cast<llvm::ArrayType>(ty))
      {
        llvm::Value *acc = llvm::ConstantInt::getTrue(builder->getContext());
        for (uint64_t i = 0; i < array_ty->getNumElements(); ++i)
        {
          llvm::Value *lhs_elem = builder->CreateExtractValue(lhs, {static_cast<unsigned>(i)});
          llvm::Value *rhs_elem = builder->CreateExtractValue(rhs, {static_cast<unsigned>(i)});
          llvm::Value *elem_eq = EmitTypedEq(builder, lhs_elem, rhs_elem);
          acc = builder->CreateAnd(acc, AsBool(builder, elem_eq));
        }
        return acc;
      }

      return nullptr;
    }

    llvm::Value *EmitTypedEq(llvm::IRBuilder<> *builder,
                             llvm::Value *lhs,
                             llvm::Value *rhs)
    {
      if (!builder || !lhs || !rhs)
      {
        return llvm::ConstantInt::getFalse(builder->getContext());
      }

      if (lhs->getType() != rhs->getType())
      {
        rhs = CoerceTo(builder, rhs, lhs->getType());
        if (!rhs || lhs->getType() != rhs->getType())
        {
          return llvm::ConstantInt::getFalse(builder->getContext());
        }
      }

      llvm::Type *ty = lhs->getType();
      if (ty->isFloatingPointTy())
      {
        return builder->CreateFCmpOEQ(lhs, rhs);
      }
      if (ty->isIntegerTy() || ty->isPointerTy())
      {
        return builder->CreateICmpEQ(lhs, rhs);
      }
      if (llvm::Value *aggregate_eq = EmitAggregateEq(builder, lhs, rhs))
      {
        return aggregate_eq;
      }
      return llvm::ConstantInt::getFalse(builder->getContext());
    }

    std::string BasePlaceIdentifier(const std::string &repr)
    {
      std::size_t i = 0;
      while (i < repr.size() && std::isspace(static_cast<unsigned char>(repr[i])))
      {
        ++i;
      }
      if (i >= repr.size())
      {
        return {};
      }
      const unsigned char first = static_cast<unsigned char>(repr[i]);
      if (!(std::isalpha(first) || repr[i] == '_'))
      {
        return {};
      }
      std::size_t j = i + 1;
      while (j < repr.size())
      {
        const unsigned char ch = static_cast<unsigned char>(repr[j]);
        if (!(std::isalnum(ch) || repr[j] == '_'))
        {
          break;
        }
        ++j;
      }
      return repr.substr(i, j - i);
    }

  } // namespace

  // T-LLVM-001: Set module header (triple, datalayout)


  // T-LLVM-002: Opaque Pointer Model
  llvm::Type *LLVMEmitter::GetOpaquePtr()
  {
    SPEC_DEF("OpaquePointerModel", "§6.12.2");
    return llvm::PointerType::get(context_, 0);
  }

  // T-LLVM-007: Type Mapping
  llvm::Type *LLVMEmitter::GetLLVMType(analysis::TypeRef type)
  {
    if (!type)
    {
      SPEC_RULE("LLVMTy-Err");
      return llvm::Type::getVoidTy(context_);
    }

    if (type_cache_.count(type))
    {
      return type_cache_[type];
    }

    llvm::Type *ll_ty = nullptr;

    if (current_ctx_ && current_ctx_->sigma)
    {
      const analysis::ScopeContext &scope = BuildScope(current_ctx_);
      if (const auto async_sig = analysis::AsyncSigOf(scope, type))
      {
        SPEC_RULE("LLVMTy-Async");
        std::vector<analysis::TypeRef> async_args;
        async_args.reserve(4);
        async_args.push_back(async_sig->out);
        async_args.push_back(async_sig->in);
        async_args.push_back(async_sig->result);
        async_args.push_back(async_sig->err);
        ll_ty = BuildAsyncLLVMType(*this, async_args);
        type_cache_[type] = ll_ty;
        return ll_ty;
      }
    }

    if (const auto *prim = std::get_if<analysis::TypePrim>(&type->node))
    {
      SPEC_RULE("LLVMTy-Prim");
      ll_ty = GetPrimType(context_, prim->name);
    }
    else if (const auto *perm = std::get_if<analysis::TypePerm>(&type->node))
    {
      SPEC_RULE("LLVMTy-Perm");
      ll_ty = GetLLVMType(perm->base);
    }
    else if (const auto *refine = std::get_if<analysis::TypeRefine>(&type->node))
    {
      SPEC_RULE("LLVMTy-Refine");
      // Refinement types are representationally identical to their base type.
      ll_ty = GetLLVMType(refine->base);
    }
    else if (const auto *opaque = std::get_if<analysis::TypeOpaque>(&type->node))
    {
      SPEC_RULE("LLVMTy-Opaque");
      if (opaque->origin && current_ctx_ && current_ctx_->sigma)
      {
        const analysis::ScopeContext &scope = BuildScope(current_ctx_);
        const auto it = scope.sigma.opaque_underlying.find(opaque->origin);
        if (it != scope.sigma.opaque_underlying.end() && it->second &&
            it->second.get() != type.get())
        {
          ll_ty = GetLLVMType(it->second);
        }
      }
      if (!ll_ty)
      {
        SPEC_RULE("LLVMTy-Err");
        if (current_ctx_)
        {
          current_ctx_->ReportCodegenFailure();
        }
      }
    }
    else if (std::holds_alternative<analysis::TypePtr>(type->node))
    {
      SPEC_RULE("LLVMTy-Ptr");
      ll_ty = GetOpaquePtr();
    }
    else if (std::holds_alternative<analysis::TypeRawPtr>(type->node))
    {
      SPEC_RULE("LLVMTy-RawPtr");
      ll_ty = GetOpaquePtr();
    }
    else if (std::holds_alternative<analysis::TypeFunc>(type->node))
    {
      SPEC_RULE("LLVMTy-Func");
      ll_ty = GetOpaquePtr();
    }
    else if (const auto *closure = std::get_if<analysis::TypeClosure>(&type->node))
    {
      (void)closure;
      // Runtime closure values are lowered as a pair (env_ptr, code_ptr).
      // Both components are represented as opaque pointers at LLVM level.
      SPEC_RULE("LLVMTy-Tuple");
      llvm::Type *ptr_ty = GetOpaquePtr();
      ll_ty = llvm::StructType::get(context_, {ptr_ty, ptr_ty});
    }
    else if (const auto *tuple = std::get_if<analysis::TypeTuple>(&type->node))
    {
      SPEC_RULE("LLVMTy-Tuple");
      if (!current_ctx_ || !current_ctx_->sigma)
      {
        ll_ty = llvm::StructType::get(context_, {});
      }
      else
      {
        const analysis::ScopeContext &scope = BuildScope(current_ctx_);
        const auto layout = ::cursive::analysis::layout::RecordLayoutOf(scope, tuple->elements);
        std::vector<llvm::Type *> elems;
        if (layout.has_value())
        {
          elems = ComputeStructElements(*this, tuple->elements, layout->offsets, layout->layout.size);
        }
        ll_ty = llvm::StructType::get(context_, elems);
      }
    }
    else if (const auto *uni = std::get_if<analysis::TypeUnion>(&type->node))
    {
      SPEC_RULE("LLVMTy-Union");
      if (!current_ctx_ || !current_ctx_->sigma)
      {
        ll_ty = llvm::Type::getInt8Ty(context_);
      }
      else
      {
        const analysis::ScopeContext &scope = BuildScope(current_ctx_);
        const auto layout = ::cursive::analysis::layout::UnionLayoutOf(scope, *uni);
        if (UnionDebugEnabled())
        {
          std::cerr << "[union-debug-llvmtype] union=" << analysis::TypeToString(type)
                    << " layout=" << (layout.has_value() ? "ok" : "missing");
          if (layout.has_value())
          {
            std::cerr << " niche=" << (layout->niche ? 1 : 0)
                      << " payload_size=" << layout->payload_size;
            if (layout->disc_type.has_value())
            {
              std::cerr << " disc=" << *layout->disc_type;
            }
          }
          std::cerr << "\n";
        }
        if (layout.has_value())
        {
          auto is_unit_type = [](const analysis::TypeRef &member) -> bool
          {
            if (!member)
            {
              return false;
            }
            analysis::TypeRef stripped = analysis::StripPerm(member);
            if (!stripped)
            {
              stripped = member;
            }
            const auto *prim = std::get_if<analysis::TypePrim>(&stripped->node);
            return prim && prim->name == "()";
          };

          if (layout->niche)
          {
            analysis::TypeRef payload_member = nullptr;
            for (const auto &member : layout->member_list)
            {
              if (!is_unit_type(member))
              {
                payload_member = member;
                break;
              }
            }
            if (payload_member)
            {
              ll_ty = GetLLVMType(payload_member);
            }
            else
            {
              ll_ty = llvm::Type::getInt8Ty(context_);
            }
          }
          else
          {
            analysis::TypeRef disc_type = analysis::MakeTypePrim("u8");
            if (layout->disc_type.has_value())
            {
              disc_type = analysis::MakeTypePrim(*layout->disc_type);
            }
            ll_ty = CreateTaggedStructType(*this,
                                           disc_type,
                                           layout->payload_size,
                                           layout->payload_align,
                                           layout->layout.size);
          }
        }
      }
    }
    else if (const auto *path = std::get_if<analysis::TypePathType>(&type->node))
    {
      if (IsRuntimeHandleModalPath(path->path))
      {
        ll_ty = GetOpaquePtr();
      }
      else if (current_ctx_ && current_ctx_->sigma)
      {
        const analysis::ScopeContext &scope = BuildScope(current_ctx_);
        if (analysis::IsAsyncType(type))
        {
          if (const auto async_sig = analysis::GetAsyncSig(type))
          {
            std::vector<analysis::TypeRef> async_args;
            async_args.reserve(4);
            async_args.push_back(async_sig->out);
            async_args.push_back(async_sig->in);
            async_args.push_back(async_sig->result);
            async_args.push_back(async_sig->err);
            ll_ty = BuildAsyncLLVMType(*this, async_args);
          }
        }
        if (const ast::RecordDecl *record = analysis::LookupRecordDecl(scope, path->path))
        {
          SPEC_RULE("LLVMTy-Tuple");
          analysis::TypeSubst record_subst;
          if (record->generic_params && !record->generic_params->params.empty())
          {
            if (path->generic_args.size() > record->generic_params->params.size())
            {
              return nullptr;
            }
            record_subst = analysis::BuildSubstitution(
                record->generic_params->params,
                path->generic_args);
          }
          std::vector<analysis::TypeRef> fields;
          for (const auto &member : record->members)
          {
            const auto *field = std::get_if<ast::FieldDecl>(&member);
            if (!field)
            {
              continue;
            }
            auto lowered = ::cursive::analysis::layout::LowerTypeForLayout(scope, field->type);
            if (lowered.has_value())
            {
              analysis::TypeRef field_type = *lowered;
              if (!record_subst.empty())
              {
                field_type = analysis::InstantiateType(field_type, record_subst);
              }
              fields.push_back(field_type);
            }
            else
            {
              fields.push_back(analysis::MakeTypePrim("u8"));
            }
          }
          const auto record_layout_options = ::cursive::analysis::layout::ResolveRecordLayoutOptions(record->attrs);
          if (const auto layout = ::cursive::analysis::layout::RecordLayoutOf(scope, fields, record_layout_options))
          {
            std::vector<llvm::Type *> elems =
                ComputeStructElements(*this,
                                      fields,
                                      layout->offsets,
                                      layout->layout.size,
                                      layout->layout.align);
            ll_ty = llvm::StructType::get(context_, elems, record_layout_options.packed);
          }
          else
          {
            ll_ty = llvm::StructType::get(context_, {});
          }
        }
        if (!ll_ty)
        {
          if (const ast::EnumDecl *enum_decl = analysis::LookupEnumDecl(scope, path->path))
          {
            SPEC_RULE("LLVMTy-Enum");
            if (const auto layout = ::cursive::analysis::layout::EnumLayoutOf(
                    scope,
                    *enum_decl,
                    ::cursive::analysis::layout::ResolveEnumLayoutOptions(enum_decl->attrs)))
            {
              analysis::TypeRef disc_type = analysis::MakeTypePrim(layout->disc_type);
              if (layout->payload_size == 0) {
                ll_ty = GetLLVMType(disc_type);
              } else {
                ll_ty = CreateTaggedStructType(*this,
                                               disc_type,
                                               layout->payload_size,
                                               layout->payload_align,
                                               layout->layout.size);
              }
            }
          }
        }
        if (!ll_ty)
        {
          ast::TypePath syntax_path;
          syntax_path.reserve(path->path.size());
          for (const auto &seg : path->path)
          {
            syntax_path.push_back(seg);
          }
          const auto it = scope.sigma.types.find(analysis::PathKeyOf(syntax_path));
          if (it != scope.sigma.types.end())
          {
            if (const auto *alias = std::get_if<ast::TypeAliasDecl>(&it->second))
            {
              if (const auto lowered = ::cursive::analysis::layout::LowerTypeForLayout(scope, alias->type))
              {
                analysis::TypeRef inst = *lowered;
                if (alias->generic_params &&
                    !alias->generic_params->params.empty())
                {
                  if (path->generic_args.size() > alias->generic_params->params.size())
                  {
                    return nullptr;
                  }
                  analysis::TypeSubst subst =
                      analysis::BuildSubstitution(alias->generic_params->params,
                                                  path->generic_args);
                  inst = analysis::InstantiateType(inst, subst);
                }
                ll_ty = GetLLVMType(inst);
              }
            }
          }
        }
        if (!ll_ty)
        {
          if (const auto builtin_layout =
                  analysis::LookupBuiltinModalLayout(path->path))
          {
            ll_ty = CreateTaggedStructType(
                *this,
                analysis::MakeTypePrim(builtin_layout->disc_prim),
                builtin_layout->payload_size,
                builtin_layout->payload_align,
                builtin_layout->size);
          }
        }
        if (!ll_ty)
        {
          ast::TypePath syntax_path;
          syntax_path.reserve(path->path.size());
          for (const auto &seg : path->path)
          {
            syntax_path.push_back(seg);
          }
          const auto it = scope.sigma.types.find(analysis::PathKeyOf(syntax_path));
          if (it != scope.sigma.types.end())
          {
            if (const auto *modal = std::get_if<ast::ModalDecl>(&it->second))
            {
              SPEC_RULE("LLVMTy-Tuple");
              if (const auto layout = ::cursive::analysis::layout::ModalLayoutOf(scope, *modal, path->generic_args))
              {
                if (layout->disc_type.has_value())
                {
                  analysis::TypeRef disc_type = analysis::MakeTypePrim(*layout->disc_type);
                  ll_ty = CreateTaggedStructType(*this,
                                                 disc_type,
                                                 layout->payload_size,
                                                 layout->payload_align,
                                                 layout->layout.size);
                }
                else
                {
                  ll_ty = llvm::ArrayType::get(
                      llvm::Type::getInt8Ty(context_),
                      static_cast<std::uint64_t>(layout->layout.size));
                }
              }
            }
          }
        }
      }
    }
    else if (const auto *modal_state = std::get_if<analysis::TypeModalState>(&type->node))
    {
      if (IsRuntimeHandleModalPath(modal_state->path))
      {
        ll_ty = GetOpaquePtr();
      }
      else
      {
        if (analysis::IsAsyncType(type))
        {
          if (const auto async_sig = analysis::GetAsyncSig(type))
          {
            std::vector<analysis::TypeRef> async_args;
            async_args.reserve(4);
            async_args.push_back(async_sig->out);
            async_args.push_back(async_sig->in);
            async_args.push_back(async_sig->result);
            async_args.push_back(async_sig->err);
            ll_ty = BuildAsyncLLVMType(*this, async_args);
          }
        }
        if (!ll_ty)
        {
          if (const auto builtin_layout =
                  analysis::LookupBuiltinModalLayout(modal_state->path))
          {
            ll_ty = CreateTaggedStructType(
                *this,
                analysis::MakeTypePrim(builtin_layout->disc_prim),
                builtin_layout->payload_size,
                builtin_layout->payload_align,
                builtin_layout->size);
          }
        }
        if (!ll_ty && current_ctx_ && current_ctx_->sigma)
        {
          const analysis::ScopeContext &scope = BuildScope(current_ctx_);
          const ast::ModalDecl *modal_decl =
              analysis::LookupModalDecl(scope, modal_state->path);
          analysis::TypeSubst modal_subst;
          if (modal_decl)
          {
            if (modal_decl->generic_params &&
                !modal_decl->generic_params->params.empty())
            {
              if (modal_state->generic_args.size() >
                  modal_decl->generic_params->params.size())
              {
                return nullptr;
              }
              modal_subst = analysis::BuildSubstitution(
                  modal_decl->generic_params->params,
                  modal_state->generic_args);
            }
            const ast::StateBlock *state_block = nullptr;
            for (const auto &state : modal_decl->states)
            {
              if (analysis::IdEq(state.name, modal_state->state))
              {
                state_block = &state;
                break;
              }
            }

            if (state_block)
            {
              SPEC_RULE("LLVMTy-Tuple");
              std::vector<analysis::TypeRef> fields;
              for (const auto &member : state_block->members)
              {
                const auto *field = std::get_if<ast::StateFieldDecl>(&member);
                if (!field)
                {
                  continue;
                }
                auto lowered = ::cursive::analysis::layout::LowerTypeForLayout(scope, field->type);
                if (lowered.has_value())
                {
                  analysis::TypeRef field_type = *lowered;
                  if (!modal_subst.empty())
                  {
                    field_type = analysis::InstantiateType(field_type, modal_subst);
                  }
                  fields.push_back(field_type);
                }
                else
                {
                  fields.push_back(analysis::MakeTypePrim("u8"));
                }
              }
              if (const auto layout = ::cursive::analysis::layout::RecordLayoutOf(scope, fields))
              {
                std::vector<llvm::Type *> elems = ComputeStructElements(
                    *this, fields, layout->offsets, layout->layout.size);
                ll_ty = llvm::StructType::get(context_, elems);
              }
              else
              {
                ll_ty = llvm::StructType::get(context_, {});
              }
            }
          }

          if (!ll_ty && modal_decl)
          {
            // Fallback to general modal layout if state layout synthesis fails.
            if (const auto layout = ::cursive::analysis::layout::ModalLayoutOf(scope, *modal_decl, modal_state->generic_args))
            {
              if (layout->disc_type.has_value())
              {
                analysis::TypeRef disc_type = analysis::MakeTypePrim(*layout->disc_type);
                ll_ty = CreateTaggedStructType(*this,
                                               disc_type,
                                               layout->payload_size,
                                               layout->payload_align,
                                               layout->layout.size);
              }
              else
              {
                ll_ty = llvm::ArrayType::get(
                    llvm::Type::getInt8Ty(context_),
                    static_cast<std::uint64_t>(layout->layout.size));
              }
            }
          }
        }
      }
    }
    else if (const auto *arr = std::get_if<analysis::TypeArray>(&type->node))
    {
      SPEC_RULE("LLVMTy-Array");
      llvm::Type *elem_ty = GetLLVMType(arr->element);
      ll_ty = llvm::ArrayType::get(elem_ty, arr->length);
    }
    else if (std::holds_alternative<analysis::TypeSlice>(type->node))
    {
      SPEC_RULE("LLVMTy-Slice");
      ll_ty = GetSliceType(context_);
    }
    else if (const auto *str = std::get_if<analysis::TypeString>(&type->node))
    {
      if (str->state.has_value() && *str->state == analysis::StringState::View)
      {
        SPEC_RULE("LLVMTy-StringView");
        ll_ty = GetStringViewType(context_);
      }
      else if (str->state.has_value() && *str->state == analysis::StringState::Managed)
      {
        SPEC_RULE("LLVMTy-StringManaged");
        ll_ty = GetStringManagedType(context_);
      }
      else
      {
        SPEC_RULE("LLVMTy-Modal-StringBytes");
        const std::uint64_t payload_size = 3 * ::cursive::analysis::layout::kPtrSize;
        const std::uint64_t payload_align = ::cursive::analysis::layout::kPtrAlign;
        const std::uint64_t payload_off =
            ((1 + payload_align - 1) / payload_align) * payload_align;
        const std::uint64_t total_size_raw = payload_off + payload_size;
        const std::uint64_t total_size =
            ((total_size_raw + payload_align - 1) / payload_align) * payload_align;
        ll_ty = CreateTaggedStructType(
            *this,
            analysis::MakeTypePrim("u8"),
            payload_size,
            payload_align,
            total_size);
      }
    }
    else if (const auto *bytes = std::get_if<analysis::TypeBytes>(&type->node))
    {
      if (bytes->state.has_value() && *bytes->state == analysis::BytesState::View)
      {
        SPEC_RULE("LLVMTy-BytesView");
        ll_ty = GetBytesViewType(context_);
      }
      else if (bytes->state.has_value() && *bytes->state == analysis::BytesState::Managed)
      {
        SPEC_RULE("LLVMTy-BytesManaged");
        ll_ty = GetBytesManagedType(context_);
      }
      else
      {
        SPEC_RULE("LLVMTy-Modal-StringBytes");
        const std::uint64_t payload_size = 3 * ::cursive::analysis::layout::kPtrSize;
        const std::uint64_t payload_align = ::cursive::analysis::layout::kPtrAlign;
        const std::uint64_t payload_off =
            ((1 + payload_align - 1) / payload_align) * payload_align;
        const std::uint64_t total_size_raw = payload_off + payload_size;
        const std::uint64_t total_size =
            ((total_size_raw + payload_align - 1) / payload_align) * payload_align;
        ll_ty = CreateTaggedStructType(
            *this,
            analysis::MakeTypePrim("u8"),
            payload_size,
            payload_align,
            total_size);
      }
    }
    else if (std::holds_alternative<analysis::TypeDynamic>(type->node))
    {
      SPEC_RULE("LLVMTy-Dynamic");
      ll_ty = GetDynamicType(context_);
    }
    else if (analysis::IsRangeType(type))
    {
      analysis::TypeRef stripped = type;
      while (stripped)
      {
        if (const auto *perm = std::get_if<analysis::TypePerm>(&stripped->node))
        {
          stripped = perm->base;
          continue;
        }
        if (const auto *refine = std::get_if<analysis::TypeRefine>(&stripped->node))
        {
          stripped = refine->base;
          continue;
        }
        break;
      }

      std::vector<analysis::TypeRef> fields;
      if (const auto *range = stripped ? std::get_if<analysis::TypeRange>(&stripped->node)
                                       : nullptr)
      {
        SPEC_RULE("LLVMTy-Range");
        fields.push_back(range->base);
        fields.push_back(range->base);
      }
      else if (const auto *range = stripped ? std::get_if<analysis::TypeRangeInclusive>(&stripped->node)
                                            : nullptr)
      {
        SPEC_RULE("LLVMTy-RangeInclusive");
        fields.push_back(range->base);
        fields.push_back(range->base);
      }
      else if (const auto *range = stripped ? std::get_if<analysis::TypeRangeFrom>(&stripped->node)
                                            : nullptr)
      {
        SPEC_RULE("LLVMTy-RangeFrom");
        fields.push_back(range->base);
      }
      else if (const auto *range = stripped ? std::get_if<analysis::TypeRangeTo>(&stripped->node)
                                            : nullptr)
      {
        SPEC_RULE("LLVMTy-RangeTo");
        fields.push_back(range->base);
      }
      else if (const auto *range =
                   stripped ? std::get_if<analysis::TypeRangeToInclusive>(&stripped->node)
                            : nullptr)
      {
        SPEC_RULE("LLVMTy-RangeToInclusive");
        fields.push_back(range->base);
      }
      else if (stripped &&
               std::holds_alternative<analysis::TypeRangeFull>(stripped->node))
      {
        SPEC_RULE("LLVMTy-RangeFull");
      }

      if (fields.empty())
      {
        ll_ty = llvm::StructType::get(context_, {});
      }
      else
      {
        const analysis::ScopeContext scope = BuildScope(current_ctx_);
        if (const auto layout = ::cursive::analysis::layout::RecordLayoutOf(scope, fields))
        {
          std::vector<llvm::Type *> elems = ComputeStructElements(
              *this, fields, layout->offsets, layout->layout.size);
          ll_ty = llvm::StructType::get(context_, elems);
        }
        else
        {
          std::vector<llvm::Type *> elems;
          elems.reserve(fields.size());
          for (const auto &field : fields)
          {
            elems.push_back(GetLLVMType(field));
          }
          ll_ty = llvm::StructType::get(context_, elems);
        }
      }
    }
    else
    {
      SPEC_RULE("LLVMTy-Err");
      ll_ty = llvm::Type::getInt8Ty(context_);
    }

    if (!ll_ty)
    {
      ll_ty = llvm::Type::getInt8Ty(context_);
    }

    type_cache_[type] = ll_ty;
    return ll_ty;
  }









  // T-LLVM-011: Call ABI
  ABICallResult LLVMEmitter::ComputeCallABI(const std::vector<IRParam> &params,
                                            analysis::TypeRef ret_type,
                                            bool use_c_abi_aggregate_sret,
                                            bool foreign_boundary_mode_independent)
  {
    SPEC_RULE("LLVMCall-ByValue");
    SPEC_RULE("LLVMCall-SRet");

    ABICallResult result;
    const analysis::ScopeContext &scope = BuildScope(current_ctx_);
    auto invalidate = [&]() -> ABICallResult {
      if (current_ctx_)
      {
        current_ctx_->ReportCodegenFailure();
      }
      return result;
    };

    // Build parameter list for ABI computation
    std::vector<std::pair<std::optional<analysis::ParamMode>, analysis::TypeRef>> abi_params;
    abi_params.reserve(params.size());
    for (const auto &param : params)
    {
      abi_params.push_back({param.mode, param.type});
    }

    // Compute ABI call info
    const auto param_policy =
        foreign_boundary_mode_independent
            ? ABIParamPolicy::ForeignBoundary
            : ABIParamPolicy::ModeAware;
    const auto call_info = ABICall(scope, abi_params, ret_type, param_policy);
    if (!call_info.has_value())
    {
      SPEC_RULE("LLVMCall-Err");
      llvm::Function *debug_func = nullptr;
      auto *debug_builder = static_cast<llvm::IRBuilder<> *>(GetBuilderRaw());
      if (debug_builder && debug_builder->GetInsertBlock())
      {
        debug_func = debug_builder->GetInsertBlock()->getParent();
      }
      std::cerr << "[cursive] ABICall failed in "
                << (debug_func ? debug_func->getName().str() : std::string("<no-func>"))
                << " ret_null=" << (ret_type ? 0 : 1)
                << " param_count=" << params.size()
                << " ret_type="
                << (ret_type ? analysis::TypeToString(ret_type) : std::string("<null>"))
                << "\n";
      for (std::size_t i = 0; i < params.size(); ++i)
      {
        const int mode_tag = params[i].mode.has_value() ? static_cast<int>(*params[i].mode) : -1;
        std::cerr << "[cursive]   param[" << i << "] mode=" << mode_tag
                  << " type_null=" << (params[i].type ? 0 : 1)
                  << " type="
                  << (params[i].type ? analysis::TypeToString(params[i].type) : std::string("<null>"))
                  << "\n";
      }
      if (current_ctx_)
      {
        current_ctx_->ReportCodegenFailure();
      }
      return result;
    }

    result.param_kinds = call_info->param_kinds;
    result.param_carriers.assign(params.size(), ABIArgCarrierKind::Direct);

    const bool win64_foreign_abi =
        use_c_abi_aggregate_sret &&
        target_profile_ == project::TargetProfile::X86_64Win64;

    bool c_abi_sret = false;
    if (win64_foreign_abi && ret_type)
    {
      const auto ret_size = ::cursive::analysis::layout::SizeOf(scope, ret_type);
      llvm::Type *ret_ll = GetLLVMType(ret_type);
      if (ret_size.has_value() && *ret_size > 0 &&
          IsForeignAbiAggregateLLVMType(ret_ll) &&
          !IsWin64CAbiAggregateDirectSize(*ret_size))
      {
        c_abi_sret = true;
      }
    }

    result.has_sret = call_info->has_sret || c_abi_sret;
    llvm::Type *sret_storage_ty = ret_type ? GetLLVMType(ret_type) : nullptr;

    // Compute return type
    if (result.has_sret)
    {
      SPEC_RULE("LLVMRetLower-SRet");
      if (!sret_storage_ty || sret_storage_ty->isVoidTy())
      {
        SPEC_RULE("LLVMRetLower-Err");
        return invalidate();
      }
      result.ret_type = llvm::Type::getVoidTy(context_);
    }
    else
    {
      const auto size = ::cursive::analysis::layout::SizeOf(scope, ret_type);
      if (!size.has_value())
      {
        SPEC_RULE("LLVMRetLower-Err");
        return invalidate();
      }
      else if (*size == 0)
      {
        SPEC_RULE("LLVMRetLower-ByValue-ZST");
        result.ret_type = llvm::Type::getVoidTy(context_);
      }
      else
      {
        SPEC_RULE("LLVMRetLower-ByValue");
        result.ret_type = GetLLVMType(ret_type);
        if (win64_foreign_abi && ret_type)
        {
          const auto ret_size = ::cursive::analysis::layout::SizeOf(scope, ret_type);
          if (ret_size.has_value() && *ret_size > 0 &&
              IsForeignAbiAggregateLLVMType(result.ret_type))
          {
            if (llvm::Type *carrier =
                    Win64CAbiDirectAggregateCarrier(context_, *ret_size))
            {
              result.ret_type = carrier;
            }
            else
            {
              result.ret_type = llvm::Type::getVoidTy(context_);
            }
          }
        }
        if (!result.ret_type)
        {
          SPEC_RULE("LLVMRetLower-Err");
          return invalidate();
        }
      }
    }

    result.param_indices.assign(params.size(), std::nullopt);

    // Build parameter list
    if (result.has_sret)
    {
      result.param_types.push_back(GetOpaquePtr());
      result.llvm_param_attrs.push_back(
          ComputeSRetParamAttrs(ret_type, sret_storage_ty, current_ctx_));
    }

    unsigned llvm_index = result.has_sret ? 1u : 0u;
    for (std::size_t i = 0; i < params.size(); ++i)
    {
      if (i >= result.param_kinds.size())
      {
        break;
      }
      const auto kind = result.param_kinds[i];
      if (kind == PassKind::ByRef)
      {
        SPEC_RULE("LLVMArgLower-ByRef");
        result.param_types.push_back(GetOpaquePtr());
        result.llvm_param_attrs.push_back(
            ComputeLoweredParamAttrs(params[i].name,
                                     params[i].type,
                                     kind,
                                     current_ctx_));
        result.param_indices[i] = llvm_index++;
        result.param_carriers[i] = ABIArgCarrierKind::Indirect;
        continue;
      }

      const auto size = ::cursive::analysis::layout::SizeOf(scope, params[i].type);
      if (!size.has_value())
      {
        SPEC_RULE("LLVMArgLower-Err");
        return invalidate();
      }
      if (*size == 0)
      {
        result.param_indices[i] = std::nullopt;
        continue;
      }

      llvm::Type *llvm_ty = GetLLVMType(params[i].type);
      if (win64_foreign_abi && llvm_ty && IsForeignAbiAggregateLLVMType(llvm_ty))
      {
        if (llvm::Type *carrier =
                Win64CAbiDirectAggregateCarrier(context_, *size))
        {
          llvm_ty = carrier;
        }
        else
        {
          llvm_ty = GetOpaquePtr();
          result.param_carriers[i] = ABIArgCarrierKind::Indirect;
        }
      }
      if (!llvm_ty)
      {
        SPEC_RULE("LLVMArgLower-Err");
        return invalidate();
      }
      result.param_types.push_back(llvm_ty);
      result.llvm_param_attrs.push_back(
          ComputeLoweredParamAttrs(params[i].name,
                                   params[i].type,
                                   kind,
                                   current_ctx_));
      result.param_indices[i] = llvm_index++;
    }

    result.func_type = llvm::FunctionType::get(result.ret_type, result.param_types, false);
    result.valid = true;
    return result;
  }

  // T-LLVM-008: Emit the full module


  void LLVMEmitter::EmitProc(const ProcIR &proc)
  {
    if (emit_detail::IsGeneratedProcSymbol(proc.symbol))
    {
      SPEC_RULE("LowerIRDecl-Proc-Gen");
    }
    else
    {
      SPEC_RULE("LowerIRDecl-Proc-User");
    }

    llvm::Function *func = functions_[proc.symbol];
    if (!func)
    {
      return;
    }

    using Clock = std::chrono::steady_clock;
    const bool perf_enabled = EmitPerfLoggingEnabled();
    const bool log_all_procs = perf_enabled && EmitPerfLogAllProcs();
    const long long slow_proc_threshold_ms =
        perf_enabled ? EmitPerfSlowProcThresholdMs() : 0;
    const std::string perf_module_label =
        (perf_enabled && current_ctx_) ? ModulePerfLabel(*current_ctx_)
                                       : std::string();
    const auto proc_start = perf_enabled ? Clock::now() : Clock::time_point{};
    auto phase_start = proc_start;

    long long state_reset_ms = 0;
    long long prologue_ms = 0;
    long long abi_ms = 0;
    long long bind_params_ms = 0;
    long long panic_slot_ms = 0;
    long long async_setup_ms = 0;
    long long body_emit_ms = 0;
    long long async_clear_ms = 0;
    long long terminator_fix_ms = 0;
    long long final_cleanup_ms = 0;
    long long ir_self_total_ms = 0;

    std::size_t bound_params = 0;
    std::size_t bound_params_by_ref = 0;
    std::size_t bound_params_by_value = 0;
    std::size_t async_slots_typed = 0;
    std::size_t async_slots_restored = 0;
    std::size_t inserted_terminators = 0;
    bool panic_slot_materialized = false;
    bool async_resume_mode = false;
    IRProcPerfContext ir_proc_perf;
    IRProcPerfContext *prior_ir_ctx = nullptr;

    ClearLocals();
    ClearTempValues();
    ClearSymbolAliases();
    active_regions_.clear();
    hosted_env_value_ = nullptr;
    if (perf_enabled)
    {
      const auto now = Clock::now();
      state_reset_ms = ElapsedMs(phase_start, now);
      phase_start = now;
    }

    llvm::BasicBlock *entry = llvm::BasicBlock::Create(context_, "entry", func);
    auto *builder = static_cast<llvm::IRBuilder<> *>(builder_.get());
    builder->SetInsertPoint(entry);
    if (perf_enabled)
    {
      const auto now = Clock::now();
      prologue_ms = ElapsedMs(phase_start, now);
      phase_start = now;
    }

    const bool needs_hosted_env = RequiresHostedEnvParam(proc.symbol);
    const bool has_explicit_hosted_env = HasLeadingHostedEnvParam(proc.params);
    const bool needs_implicit_hosted_env =
        needs_hosted_env && !has_explicit_hosted_env;
    std::vector<IRParam> abi_params = proc.params;
    std::size_t abi_param_base = 0u;
    if (needs_implicit_hosted_env)
    {
      abi_params.insert(abi_params.begin(), HostedEnvParam());
      abi_param_base = 1u;
    }
    ABICallResult abi = ComputeProcABI(proc.symbol, proc.params, proc.ret);
    if (!abi.valid || !abi.func_type)
    {
      if (builder && builder->GetInsertBlock() &&
          !builder->GetInsertBlock()->getTerminator())
      {
        builder->CreateUnreachable();
      }
      return;
    }
    if (perf_enabled)
    {
      const auto now = Clock::now();
      abi_ms = ElapsedMs(phase_start, now);
      phase_start = now;
    }

    if (needs_implicit_hosted_env && !abi.param_indices.empty() &&
        abi.param_indices[0].has_value())
    {
      const unsigned idx = *abi.param_indices[0];
      if (idx < func->arg_size())
      {
        llvm::Argument *arg = func->getArg(idx);
        arg->setName(kHostedEnvParamName);
        llvm::Value *env_value = arg;
        if (llvm::Type *env_ty = GetLLVMType(HostedEnvParamType()))
        {
          if (env_value->getType() != env_ty)
          {
            if (llvm::Value *coerced = CoerceTo(builder, env_value, env_ty))
            {
              env_value = coerced;
            }
          }
        }
        hosted_env_value_ = env_value;
        RegisterLocalBindStorage(std::string(kHostedEnvParamName), env_value);
        local_types_[std::string(kHostedEnvParamName)] = HostedEnvParamType();
      }
    }

    const auto &param_scope = BuildScope(current_ctx_);

    auto bind_zero_sized_param = [&](const IRParam &param) -> bool
    {
      if (!param.type)
      {
        return false;
      }

      const auto size = ::cursive::analysis::layout::SizeOf(param_scope, param.type);
      if (!size.has_value() || *size != 0)
      {
        return false;
      }

      llvm::Type *llvm_ty = GetLLVMType(param.type);
      if (!llvm_ty || llvm_ty->isVoidTy())
      {
        return false;
      }

      SPEC_RULE("BindSlot-Param-ByValue");
      llvm::IRBuilder<> entry_builder(&func->getEntryBlock(), func->getEntryBlock().begin());
      llvm::AllocaInst *alloca = entry_builder.CreateAlloca(llvm_ty, nullptr, param.name);
      builder->CreateStore(llvm::Constant::getNullValue(llvm_ty), alloca);
      RegisterLocalBindStorage(param.name, alloca);
      local_types_[param.name] = param.type;
      const std::string stable_name =
          param.stable_name.empty() ? param.name : param.stable_name;
      if (stable_name != param.name) {
        RegisterLocalBindStorage(stable_name, alloca);
        if (param.type) {
          local_types_[stable_name] = param.type;
        }
      }
      ++bound_params;
      ++bound_params_by_value;
      return true;
    };

    SPEC_RULE("ParamInitIR");
    // Map parameters into locals
    for (std::size_t i = 0; i < proc.params.size(); ++i)
    {
      const std::size_t abi_index = i + abi_param_base;
      if (abi_index >= abi.param_indices.size())
      {
        bind_zero_sized_param(proc.params[i]);
        continue;
      }
      if (!abi.param_indices[abi_index].has_value())
      {
        bind_zero_sized_param(proc.params[i]);
        continue;
      }
      unsigned idx = *abi.param_indices[abi_index];
      if (idx >= func->arg_size())
      {
        bind_zero_sized_param(proc.params[i]);
        continue;
      }
      llvm::Argument *arg = func->getArg(idx);
      arg->setName(proc.params[i].name);

      auto register_explicit_hosted_env = [&](llvm::Value *env_value) {
        if (proc.params[i].name != std::string(kHostedEnvParamName) || !env_value)
        {
          return;
        }
        if (llvm::Type *env_ty = GetLLVMType(HostedEnvParamType()))
        {
          if (env_value->getType() != env_ty)
          {
            if (llvm::Value *coerced = CoerceTo(builder, env_value, env_ty))
            {
              env_value = coerced;
            }
          }
        }
        hosted_env_value_ = env_value;
      };

      const ABIArgCarrierKind carrier =
          abi_index < abi.param_carriers.size() ? abi.param_carriers[abi_index]
                                                : ABIArgCarrierKind::Direct;

      if (abi_index < abi.param_kinds.size() &&
          abi.param_kinds[abi_index] == PassKind::ByRef)
      {
        SPEC_RULE("BindSlot-Param-ByRef");
        llvm::Type *llvm_ty = GetLLVMType(proc.params[i].type);
        llvm::Value *typed_ptr = arg;
        if (typed_ptr && typed_ptr->getType()->isPointerTy() && llvm_ty)
        {
          typed_ptr =
              builder->CreateBitCast(typed_ptr, llvm::PointerType::get(llvm_ty, 0));
        }
        RegisterLocalBindStorage(proc.params[i].name, typed_ptr);
        if (proc.params[i].type)
        {
          local_types_[proc.params[i].name] = proc.params[i].type;
        }
        const std::string stable_name =
            proc.params[i].stable_name.empty()
                ? proc.params[i].name
                : proc.params[i].stable_name;
        if (stable_name != proc.params[i].name) {
          RegisterLocalBindStorage(stable_name, typed_ptr);
          if (proc.params[i].type) {
            local_types_[stable_name] = proc.params[i].type;
          }
        }
        register_explicit_hosted_env(typed_ptr);
        ++bound_params;
        ++bound_params_by_ref;
        continue;
      }

      SPEC_RULE("BindSlot-Param-ByValue");
      llvm::Type *llvm_ty = GetLLVMType(proc.params[i].type);
      llvm::IRBuilder<> entry_builder(&func->getEntryBlock(), func->getEntryBlock().begin());
      llvm::AllocaInst *alloca = entry_builder.CreateAlloca(llvm_ty, nullptr, proc.params[i].name);
      llvm::Value *stored_value = arg;
      if (carrier == ABIArgCarrierKind::Indirect &&
          arg && arg->getType()->isPointerTy() && llvm_ty)
      {
        llvm::Value *typed_ptr = arg;
        llvm::Type *expected_ptr_ty = llvm::PointerType::get(llvm_ty, 0);
        if (typed_ptr->getType() != expected_ptr_ty)
        {
          typed_ptr = builder->CreateBitCast(typed_ptr, expected_ptr_ty);
        }
        stored_value = builder->CreateLoad(llvm_ty, typed_ptr);
      }
      if (stored_value && llvm_ty && stored_value->getType() != llvm_ty)
      {
        if (llvm::Value *coerced = CoerceTo(builder, stored_value, llvm_ty))
        {
          stored_value = coerced;
        }
      }
      builder->CreateStore(stored_value, alloca);
      RegisterLocalBindStorage(proc.params[i].name, alloca);
      if (proc.params[i].type)
      {
        local_types_[proc.params[i].name] = proc.params[i].type;
      }
      const std::string stable_name =
          proc.params[i].stable_name.empty()
              ? proc.params[i].name
              : proc.params[i].stable_name;
      if (stable_name != proc.params[i].name) {
        RegisterLocalBindStorage(stable_name, alloca);
        if (proc.params[i].type) {
          local_types_[stable_name] = proc.params[i].type;
        }
      }
      register_explicit_hosted_env(arg);
      ++bound_params;
      ++bound_params_by_value;
    }
    if (perf_enabled)
    {
      const auto now = Clock::now();
      bind_params_ms = ElapsedMs(phase_start, now);
      phase_start = now;
    }

    // Some procedures (for example exported ABI-entry procedures) intentionally
    // omit the hidden panic out-parameter. Cleanup/panic IR still refers to the
    // canonical local name, so materialize a local panic slot when absent.
    if (!GetLocal(std::string(kPanicOutName)))
    {
      llvm::Type *panic_record_ty = GetLLVMType(PanicRecordType());
      llvm::Type *panic_ptr_ty = GetLLVMType(PanicOutType());
      if (panic_record_ty && panic_ptr_ty && panic_ptr_ty->isPointerTy())
      {
        llvm::IRBuilder<> entry_builder(&func->getEntryBlock(), func->getEntryBlock().begin());
        llvm::AllocaInst *panic_out_alloca =
            entry_builder.CreateAlloca(panic_ptr_ty, nullptr, std::string(kPanicOutName));
        llvm::Value *panic_record_ptr = nullptr;
        if (current_ctx_ && current_ctx_->shared_library_project)
        {
          panic_record_ptr = GetSharedLibraryImagePanicPtr();
          if (IsHostedLibraryBuild())
          {
            panic_record_ptr = GetHostedSessionPanicPtr(panic_record_ptr);
          }
          if (panic_record_ptr)
          {
            llvm::Value *typed_panic_ptr =
                CoerceTo(builder,
                         panic_record_ptr,
                         llvm::PointerType::get(panic_record_ty, 0));
            if (!typed_panic_ptr && panic_record_ptr->getType()->isPointerTy())
            {
              typed_panic_ptr =
                  builder->CreateBitCast(panic_record_ptr,
                                         llvm::PointerType::get(panic_record_ty, 0));
            }
            if (typed_panic_ptr)
            {
              builder->CreateStore(llvm::Constant::getNullValue(panic_record_ty),
                                   typed_panic_ptr);
              panic_record_ptr = typed_panic_ptr;
            }
          }
        }
        if (!panic_record_ptr)
        {
          llvm::AllocaInst *panic_record_alloca =
              entry_builder.CreateAlloca(panic_record_ty, nullptr, "__c0_panic_record");
          builder->CreateStore(llvm::Constant::getNullValue(panic_record_ty),
                               panic_record_alloca);
          panic_record_ptr =
              IsHostedLibraryBuild()
                  ? GetHostedSessionPanicPtr(panic_record_alloca)
                  : static_cast<llvm::Value *>(panic_record_alloca);
        }
        if (panic_record_ptr->getType() != panic_ptr_ty)
        {
          panic_record_ptr = builder->CreateBitCast(panic_record_ptr, panic_ptr_ty);
        }
        builder->CreateStore(panic_record_ptr, panic_out_alloca);
        RegisterLocalBindStorage(std::string(kPanicOutName), panic_out_alloca);
        local_types_[std::string(kPanicOutName)] = PanicOutType();
        panic_slot_materialized = true;
      }
    }
    if (perf_enabled)
    {
      const auto now = Clock::now();
      panic_slot_ms = ElapsedMs(phase_start, now);
      phase_start = now;
    }

    AsyncEmitState async_state_storage;
    bool async_state_active = false;
    const LowerCtx::AsyncProcInfo *async_info =
        current_ctx_ ? current_ctx_->LookupAsyncProc(proc.symbol) : nullptr;
    if (async_info)
    {
      async_state_storage.info = async_info;
      async_state_active = true;
      async_resume_mode = async_info->is_resume;

      llvm::IRBuilder<> entry_builder(&func->getEntryBlock(), func->getEntryBlock().begin());
      for (const auto &slot_name : async_info->slot_order)
      {
        const auto slot_it = async_info->slots.find(slot_name);
        if (slot_it == async_info->slots.end())
        {
          continue;
        }
        const auto &slot = slot_it->second;
        llvm::Type *slot_ty = GetLLVMType(slot.type);
        if (!slot_ty || slot_ty->isVoidTy())
        {
          continue;
        }
        ++async_slots_typed;

        llvm::Value *local_slot = GetLocal(slot_name);
        if (!local_slot)
        {
          local_slot = entry_builder.CreateAlloca(slot_ty, nullptr, slot_name);
          RegisterLocalBindStorage(slot_name, local_slot);
        }
        SetLocalType(slot_name, slot.type);
      }

      if (async_info->is_resume)
      {
        async_state_storage.frame_ptr = LoadLocalValue(*this, builder, "__c0_async_frame");
        async_state_storage.input_ptr = LoadLocalValue(*this, builder, "__c0_async_input");

        if (async_state_storage.frame_ptr)
        {
          for (const auto &slot_name : async_info->slot_order)
          {
            const auto slot_it = async_info->slots.find(slot_name);
            if (slot_it == async_info->slots.end())
            {
              continue;
            }
            const auto &slot = slot_it->second;
            llvm::Value *local_slot = GetLocal(slot_name);
            if (!local_slot || !local_slot->getType()->isPointerTy())
            {
              continue;
            }
            llvm::Type *slot_ty = GetLLVMType(slot.type);
            if (!slot_ty || slot_ty->isVoidTy())
            {
              continue;
            }
            llvm::Value *frame_slot_ptr = AsyncFrameTypedPtr(
                *this,
                builder,
                async_state_storage.frame_ptr,
                slot.offset,
                slot_ty);
            if (!frame_slot_ptr)
            {
              continue;
            }
            llvm::LoadInst *loaded = builder->CreateLoad(slot_ty, frame_slot_ptr);
            loaded->setAlignment(llvm::Align(std::max<std::uint64_t>(1, slot.align)));
            ++async_slots_restored;
            llvm::Value *target_ptr = local_slot;
            llvm::Type *expected_ptr_ty = llvm::PointerType::get(slot_ty, 0);
            if (target_ptr->getType() != expected_ptr_ty)
            {
              target_ptr = builder->CreateBitCast(target_ptr, expected_ptr_ty);
            }
            builder->CreateStore(loaded, target_ptr);
          }

          llvm::Type *i64_ty = llvm::Type::getInt64Ty(context_);
          llvm::Value *resume_state_ptr = AsyncFrameTypedPtr(
              *this,
              builder,
              async_state_storage.frame_ptr,
              kAsyncFrameResumeStateOffset,
              i64_ty);
          llvm::Value *resume_state = nullptr;
          if (resume_state_ptr)
          {
            resume_state = builder->CreateLoad(i64_ty, resume_state_ptr);
          }
          else
          {
            resume_state = llvm::ConstantInt::get(i64_ty, 0);
          }
          llvm::BasicBlock *start_bb =
              llvm::BasicBlock::Create(context_, "async.resume.start", func);
          async_state_storage.resume_switch =
              builder->CreateSwitch(resume_state, start_bb);
          builder->SetInsertPoint(start_bb);
        }
      }

      SetAsyncState(&async_state_storage);
    }
    if (perf_enabled)
    {
      const auto now = Clock::now();
      async_setup_ms = ElapsedMs(phase_start, now);
      phase_start = now;
    }

    // Emit procedure body
    if (perf_enabled)
    {
      ir_proc_perf.stack.clear();
      ir_proc_perf.stack.reserve(256);
      prior_ir_ctx = g_ir_proc_perf_ctx;
      g_ir_proc_perf_ctx = &ir_proc_perf;
    }
    EmitIR(proc.body);
    if (perf_enabled)
    {
      g_ir_proc_perf_ctx = prior_ir_ctx;
      ir_self_total_ms = IRProcPerfTotalSelfMs(ir_proc_perf);
    }
    if (perf_enabled)
    {
      const auto now = Clock::now();
      body_emit_ms = ElapsedMs(phase_start, now);
      phase_start = now;
    }

    if (async_state_active)
    {
      SetAsyncState(nullptr);
    }
    if (perf_enabled)
    {
      const auto now = Clock::now();
      async_clear_ms = ElapsedMs(phase_start, now);
      phase_start = now;
    }

    // Ensure all blocks are terminated
    llvm::Type *ret_ty = func->getReturnType();
    for (auto &block : *func)
    {
      if (block.getTerminator())
      {
        continue;
      }
      builder->SetInsertPoint(&block);
      if (ret_ty->isVoidTy())
      {
        builder->CreateRetVoid();
      }
      else
      {
        builder->CreateRet(llvm::Constant::getNullValue(ret_ty));
      }
      ++inserted_terminators;
    }
    if (perf_enabled)
    {
      const auto now = Clock::now();
      terminator_fix_ms = ElapsedMs(phase_start, now);
      phase_start = now;
    }

    ClearLocals();
    ClearTempValues();
    ClearSymbolAliases();
    if (perf_enabled)
    {
      const auto now = Clock::now();
      final_cleanup_ms = ElapsedMs(phase_start, now);
      const long long total_ms = ElapsedMs(proc_start, now);
      const bool log_this_proc = log_all_procs || total_ms >= slow_proc_threshold_ms;
      if (log_this_proc)
      {
        std::string perf_line =
            "module=" + perf_module_label + " stage=emit-proc symbol=" +
            proc.symbol + " total_ms=" + std::to_string(total_ms) +
            " state_reset_ms=" + std::to_string(state_reset_ms) +
            " prologue_ms=" + std::to_string(prologue_ms) + " abi_ms=" +
            std::to_string(abi_ms) + " bind_params_ms=" +
            std::to_string(bind_params_ms) + " panic_slot_ms=" +
            std::to_string(panic_slot_ms) + " async_setup_ms=" +
            std::to_string(async_setup_ms) + " body_emit_ms=" +
            std::to_string(body_emit_ms) + " async_clear_ms=" +
            std::to_string(async_clear_ms) + " terminator_fix_ms=" +
            std::to_string(terminator_fix_ms) + " final_cleanup_ms=" +
            std::to_string(final_cleanup_ms) + " param_total=" +
            std::to_string(proc.params.size()) + " bound_params=" +
            std::to_string(bound_params) + " bound_by_ref=" +
            std::to_string(bound_params_by_ref) + " bound_by_value=" +
            std::to_string(bound_params_by_value) + " panic_slot_materialized=" +
            (panic_slot_materialized ? "1" : "0") + " async_active=" +
            (async_state_active ? "1" : "0") + " async_resume=" +
            (async_resume_mode ? "1" : "0") + " async_slots_typed=" +
            std::to_string(async_slots_typed) + " async_slots_restored=" +
            std::to_string(async_slots_restored) + " inserted_terminators=" +
            std::to_string(inserted_terminators) + " ir_self_ms=" +
            std::to_string(ir_self_total_ms);
        if (perf_enabled)
        {
          AppendTopIRNodePerf(perf_line, ir_proc_perf);
        }
        EmitPerfLogLine(perf_line);
      }
    }
  }


  bool LLVMEmitter::IsHostedLibraryBuild() const
  {
    return current_ctx_ && current_ctx_->hosted_library;
  }

  bool LLVMEmitter::RequiresHostedEnvParam(const std::string &symbol) const
  {
    if (!IsHostedLibraryBuild() || symbol.empty() || !current_ctx_)
    {
      return false;
    }
    return current_ctx_->hosted_explicit_env_procs.find(symbol) !=
           current_ctx_->hosted_explicit_env_procs.end();
  }

  bool LLVMEmitter::HasHostedStateSlot(const std::string &symbol) const
  {
    if (!IsHostedLibraryBuild() || symbol.empty())
    {
      return false;
    }
    return hosted_layout_.slots.find(symbol) != hosted_layout_.slots.end();
  }

  llvm::Value *LLVMEmitter::GetHostedCurrentEnvPtr()
  {
    if (!IsHostedLibraryBuild())
    {
      return nullptr;
    }
    if (hosted_env_value_)
    {
      return hosted_env_value_;
    }
    auto *builder = static_cast<llvm::IRBuilder<> *>(builder_.get());
    if (!builder || !builder->GetInsertBlock())
    {
      return nullptr;
    }
    llvm::Type *opaque_ptr_ty = GetOpaquePtr();
    if (!opaque_ptr_ty)
    {
      return nullptr;
    }
    llvm::Function *fn = module_->getFunction(kHostRuntimeCurrentEnvSym);
    if (!fn)
    {
      llvm::FunctionType *fn_ty =
          llvm::FunctionType::get(opaque_ptr_ty, {}, false);
      fn = llvm::Function::Create(
          fn_ty,
          llvm::GlobalValue::ExternalLinkage,
          kHostRuntimeCurrentEnvSym,
          module_.get());
      fn->setCallingConv(llvm::CallingConv::C);
    }
    llvm::CallInst *call = builder->CreateCall(fn->getFunctionType(), fn, {});
    call->setCallingConv(fn->getCallingConv());
    return call;
  }

  llvm::Value *LLVMEmitter::GetSharedLibraryImagePanicPtr(
      llvm::Value *fallback_ptr)
  {
    if (!current_ctx_ || !current_ctx_->shared_library_project)
    {
      return fallback_ptr;
    }
    llvm::Type *panic_ty = GetLLVMType(PanicRecordType());
    if (!panic_ty)
    {
      return fallback_ptr;
    }

    llvm::GlobalVariable *panic_gv =
        module_->getNamedGlobal(kImagePanicRecordSym);
    if (!panic_gv)
    {
      panic_gv = new llvm::GlobalVariable(
          *module_,
          panic_ty,
          false,
          llvm::GlobalValue::CommonLinkage,
          llvm::Constant::getNullValue(panic_ty),
          kImagePanicRecordSym);
      panic_gv->setAlignment(llvm::Align(4));
    }
    return panic_gv ? static_cast<llvm::Value *>(panic_gv) : fallback_ptr;
  }

  llvm::Value *LLVMEmitter::GetHostedStatePtr(const std::string &symbol,
                                              llvm::Type *value_ty,
                                              llvm::Value *fallback_ptr)
  {
    if (!IsHostedLibraryBuild() || !value_ty)
    {
      return fallback_ptr;
    }
    const auto it = hosted_layout_.slots.find(symbol);
    if (it == hosted_layout_.slots.end())
    {
      return fallback_ptr;
    }
    auto *builder = static_cast<llvm::IRBuilder<> *>(builder_.get());
    if (!builder || !builder->GetInsertBlock())
    {
      return fallback_ptr;
    }

    llvm::Type *ptr_ty = llvm::PointerType::get(value_ty, 0);
    auto coerce_fallback = [&]() -> llvm::Value * {
      if (!fallback_ptr)
      {
        return nullptr;
      }
      llvm::Value *typed_fallback = CoerceTo(builder, fallback_ptr, ptr_ty);
      if (!typed_fallback && fallback_ptr->getType()->isPointerTy())
      {
        typed_fallback = builder->CreateBitCast(fallback_ptr, ptr_ty);
      }
      return typed_fallback;
    };

    llvm::Value *env = GetHostedCurrentEnvPtr();
    if (!env)
    {
      return coerce_fallback();
    }

    llvm::Type *i8_ty = llvm::Type::getInt8Ty(context_);
    llvm::Value *env_i8 = CoerceTo(builder, env, llvm::PointerType::get(i8_ty, 0));
    if (!env_i8)
    {
      env_i8 = builder->CreateBitCast(env, llvm::PointerType::get(i8_ty, 0));
    }

    auto build_slot_ptr = [&](llvm::Value *session_env_i8) -> llvm::Value * {
      llvm::Value *slot_ptr = session_env_i8;
      if (it->second.offset != 0u)
      {
        slot_ptr = builder->CreateGEP(
            i8_ty,
            session_env_i8,
            llvm::ConstantInt::get(llvm::Type::getInt64Ty(context_),
                                   it->second.offset));
      }
      return builder->CreateBitCast(slot_ptr, ptr_ty);
    };

    llvm::Value *typed_fallback = coerce_fallback();
    if (!typed_fallback)
    {
      return build_slot_ptr(env_i8);
    }

    llvm::Function *fn = builder->GetInsertBlock()->getParent();
    llvm::BasicBlock *session_bb =
        llvm::BasicBlock::Create(context_, "hosted.state.session", fn);
    llvm::BasicBlock *fallback_bb =
        llvm::BasicBlock::Create(context_, "hosted.state.fallback", fn);
    llvm::BasicBlock *merge_bb =
        llvm::BasicBlock::Create(context_, "hosted.state.merge", fn);
    llvm::Value *has_env = builder->CreateICmpNE(
        env_i8,
        llvm::ConstantPointerNull::get(
            llvm::cast<llvm::PointerType>(env_i8->getType())));
    builder->CreateCondBr(has_env, session_bb, fallback_bb);

    builder->SetInsertPoint(session_bb);
    llvm::Value *session_ptr = build_slot_ptr(env_i8);
    builder->CreateBr(merge_bb);
    session_bb = builder->GetInsertBlock();

    builder->SetInsertPoint(fallback_bb);
    builder->CreateBr(merge_bb);
    fallback_bb = builder->GetInsertBlock();

    builder->SetInsertPoint(merge_bb);
    llvm::PHINode *phi = builder->CreatePHI(ptr_ty, 2);
    phi->addIncoming(session_ptr, session_bb);
    phi->addIncoming(typed_fallback, fallback_bb);
    return phi;
  }

  llvm::Value *LLVMEmitter::GetHostedSessionPanicPtr(llvm::Value *fallback_ptr)
  {
    if (!IsHostedLibraryBuild())
    {
      return fallback_ptr;
    }
    auto *builder = static_cast<llvm::IRBuilder<> *>(builder_.get());
    if (!builder || !builder->GetInsertBlock())
    {
      return fallback_ptr;
    }
    llvm::Type *panic_ty = GetLLVMType(PanicRecordType());
    if (!panic_ty)
    {
      return fallback_ptr;
    }
    llvm::Type *panic_ptr_ty = llvm::PointerType::get(panic_ty, 0);
    auto coerce_fallback = [&]() -> llvm::Value * {
      if (!fallback_ptr)
      {
        return nullptr;
      }
      llvm::Value *typed_fallback = CoerceTo(builder, fallback_ptr, panic_ptr_ty);
      if (!typed_fallback && fallback_ptr->getType()->isPointerTy())
      {
        typed_fallback = builder->CreateBitCast(fallback_ptr, panic_ptr_ty);
      }
      return typed_fallback;
    };

    llvm::Value *env = GetHostedCurrentEnvPtr();
    if (!env)
    {
      return coerce_fallback();
    }

    llvm::Type *i8_ty = llvm::Type::getInt8Ty(context_);
    llvm::Value *env_i8 = CoerceTo(builder, env, llvm::PointerType::get(i8_ty, 0));
    if (!env_i8)
    {
      env_i8 = builder->CreateBitCast(env, llvm::PointerType::get(i8_ty, 0));
    }

    auto build_session_panic_ptr = [&](llvm::Value *session_env_i8) -> llvm::Value * {
      llvm::Value *panic_i8 = session_env_i8;
      if (hosted_layout_.panic_offset != 0u)
      {
        panic_i8 = builder->CreateGEP(
            i8_ty,
            session_env_i8,
            llvm::ConstantInt::get(llvm::Type::getInt64Ty(context_),
                                   hosted_layout_.panic_offset));
      }
      return builder->CreateBitCast(panic_i8, panic_ptr_ty);
    };

    llvm::Value *typed_fallback = coerce_fallback();
    if (!typed_fallback)
    {
      return build_session_panic_ptr(env_i8);
    }

    llvm::Function *fn = builder->GetInsertBlock()->getParent();
    llvm::BasicBlock *session_bb =
        llvm::BasicBlock::Create(context_, "hosted.panic.session", fn);
    llvm::BasicBlock *fallback_bb =
        llvm::BasicBlock::Create(context_, "hosted.panic.fallback", fn);
    llvm::BasicBlock *merge_bb =
        llvm::BasicBlock::Create(context_, "hosted.panic.merge", fn);
    llvm::Value *has_env = builder->CreateICmpNE(
        env_i8,
        llvm::ConstantPointerNull::get(
            llvm::cast<llvm::PointerType>(env_i8->getType())));
    builder->CreateCondBr(has_env, session_bb, fallback_bb);

    builder->SetInsertPoint(session_bb);
    llvm::Value *session_ptr = build_session_panic_ptr(env_i8);
    builder->CreateBr(merge_bb);
    session_bb = builder->GetInsertBlock();

    builder->SetInsertPoint(fallback_bb);
    builder->CreateBr(merge_bb);
    fallback_bb = builder->GetInsertBlock();

    builder->SetInsertPoint(merge_bb);
    llvm::PHINode *phi = builder->CreatePHI(panic_ptr_ty, 2);
    phi->addIncoming(session_ptr, session_bb);
    phi->addIncoming(typed_fallback, fallback_bb);
    return phi;
  }

  void LLVMEmitter::EmitHostedLifecycleExports()
  {
    if (!current_ctx_ || !current_ctx_->hosted_library ||
        current_ctx_->hosted_exports.empty() ||
        !IsProjectEntryModule(current_ctx_))
    {
      return;
    }

    auto *i1_ty = llvm::Type::getInt1Ty(context_);
    auto *i8_ty = llvm::Type::getInt8Ty(context_);
    auto *i32_ty = llvm::Type::getInt32Ty(context_);
    auto *i64_ty = llvm::Type::getInt64Ty(context_);
    llvm::Type *usize_ty = llvm::Type::getInt64Ty(context_);
    llvm::Type *opaque_ptr_ty = GetOpaquePtr();
    auto *opaque_ptr_ptr_ty = llvm::cast<llvm::PointerType>(opaque_ptr_ty);
    llvm::Type *panic_record_ty = GetLLVMType(PanicRecordType());
    llvm::Type *context_ty = GetLLVMType(analysis::MakeTypePath({"Context"}));
    llvm::GlobalVariable *owner_token_gv =
        EnsureHostedOwnerTokenGlobal(module_.get(), context_, true);
    if (!owner_token_gv)
    {
      current_ctx_->ReportCodegenFailure();
      return;
    }
    llvm::Value *owner_token =
        llvm::ConstantExpr::getBitCast(owner_token_gv, opaque_ptr_ty);

    auto ensure_runtime_fn = [&](const char *name,
                                 llvm::Type *ret_ty,
                                 std::vector<llvm::Type *> params) -> llvm::Function * {
      llvm::Function *fn = module_->getFunction(name);
      if (!fn)
      {
        llvm::FunctionType *fn_ty = llvm::FunctionType::get(ret_ty, params, false);
        fn = llvm::Function::Create(
            fn_ty,
            llvm::GlobalValue::ExternalLinkage,
            name,
            module_.get());
        fn->setCallingConv(llvm::CallingConv::C);
      }
      return fn;
    };

    auto call_proc_with_panic = [&](llvm::IRBuilder<> &builder,
                                    const std::string &sym,
                                    llvm::Value *panic_ptr,
                                    llvm::Value *env_ptr) {
      const LowerCtx::ProcSigInfo *sig =
          current_ctx_ ? current_ctx_->LookupProcSig(sym) : nullptr;
      if (!sig)
      {
        return;
      }
      std::vector<IRParam> call_params = sig->params;
      if (RequiresHostedEnvParam(sym) && !HasLeadingHostedEnvParam(call_params))
      {
        call_params.insert(call_params.begin(), HostedEnvParam());
      }
      ABICallResult abi = ComputeCallABI(call_params, sig->ret);
      if (!abi.valid || !abi.func_type)
      {
        if (current_ctx_)
        {
          current_ctx_->ReportCodegenFailure();
        }
        return;
      }
      llvm::Function *fn = module_->getFunction(sym);
      if (!fn)
      {
        fn = llvm::Function::Create(
            abi.func_type,
            llvm::GlobalValue::ExternalLinkage,
            sym,
            module_.get());
      }
      std::vector<llvm::Value *> args;
      args.reserve(fn->arg_size());
      for (unsigned i = 0; i < fn->arg_size(); ++i)
      {
        llvm::Type *param_ty = fn->getFunctionType()->getParamType(i);
        llvm::Value *arg = nullptr;
        if (!call_params.empty() && call_params[0].name == kHostedEnvParamName &&
            i < abi.param_indices.size() && abi.param_indices[0].has_value() &&
            *abi.param_indices[0] == i)
        {
          arg = CoerceTo(&builder, env_ptr, param_ty);
          if (!arg && env_ptr && env_ptr->getType() == param_ty)
          {
            arg = env_ptr;
          }
        }
        if (!abi.param_indices.empty() && abi.param_indices.back().has_value() &&
            *abi.param_indices.back() == i)
        {
          arg = CoerceTo(&builder, panic_ptr, param_ty);
          if (!arg && panic_ptr && panic_ptr->getType() == param_ty)
          {
            arg = panic_ptr;
          }
        }
        if (!arg)
        {
          if (current_ctx_)
          {
            current_ctx_->ReportCodegenFailure();
          }
          return;
        }
        args.push_back(arg);
      }
      builder.CreateCall(fn->getFunctionType(), fn, args);
    };

    auto panic_offsets = [&]() {
      std::uint64_t flag_offset = 0;
      std::uint64_t code_offset = 4;
      const analysis::ScopeContext &scope = BuildScope(current_ctx_);
      const auto layout = ::cursive::analysis::layout::RecordLayoutOf(scope,
                                         {analysis::MakeTypePrim("bool"),
                                          analysis::MakeTypePrim("u32")});
      if (layout.has_value() && layout->offsets.size() >= 2)
      {
        flag_offset = layout->offsets[0];
        code_offset = layout->offsets[1];
      }
      return std::pair<std::uint64_t, std::uint64_t>{flag_offset, code_offset};
    }();

    auto panic_field_ptr = [&](llvm::IRBuilder<> &builder,
                               llvm::Value *panic_ptr,
                               std::uint64_t offset,
                               llvm::Type *field_ty) -> llvm::Value * {
      llvm::Value *panic_i8 =
          CoerceTo(&builder, panic_ptr, llvm::PointerType::get(i8_ty, 0));
      if (!panic_i8)
      {
        panic_i8 =
            builder.CreateBitCast(panic_ptr, llvm::PointerType::get(i8_ty, 0));
      }
      llvm::Value *field_i8 = panic_i8;
      if (offset != 0u)
      {
        field_i8 = builder.CreateGEP(
            i8_ty, panic_i8, llvm::ConstantInt::get(i64_ty, offset));
      }
      return builder.CreateBitCast(field_i8, llvm::PointerType::get(field_ty, 0));
    };

    auto clear_panic_record = [&](llvm::IRBuilder<> &builder, llvm::Value *panic_ptr) {
      if (!panic_ptr)
      {
        return;
      }
      builder.CreateStore(llvm::ConstantInt::get(llvm::Type::getInt8Ty(context_), 0),
                          panic_field_ptr(builder,
                                          panic_ptr,
                                          panic_offsets.first,
                                          llvm::Type::getInt8Ty(context_)));
      builder.CreateStore(llvm::ConstantInt::get(i32_ty, 0),
                          panic_field_ptr(builder,
                                          panic_ptr,
                                          panic_offsets.second,
                                          i32_ty));
    };

    auto load_panic_flag = [&](llvm::IRBuilder<> &builder,
                               llvm::Value *panic_ptr) -> llvm::Value * {
      return builder.CreateICmpNE(
          builder.CreateLoad(
              llvm::Type::getInt8Ty(context_),
              panic_field_ptr(builder,
                              panic_ptr,
                              panic_offsets.first,
                              llvm::Type::getInt8Ty(context_))),
          llvm::ConstantInt::get(llvm::Type::getInt8Ty(context_), 0));
    };

    auto load_panic_code = [&](llvm::IRBuilder<> &builder,
                               llvm::Value *panic_ptr) -> llvm::Value * {
      return builder.CreateLoad(
          i32_ty,
          panic_field_ptr(builder, panic_ptr, panic_offsets.second, i32_ty));
    };

    auto build_env_slot_ptr = [&](llvm::IRBuilder<> &builder,
                                  llvm::Value *env_ptr,
                                  std::uint64_t offset,
                                  llvm::Type *target_ty) -> llvm::Value * {
      llvm::Value *env_i8 =
          CoerceTo(&builder, env_ptr, llvm::PointerType::get(i8_ty, 0));
      if (!env_i8)
      {
        env_i8 =
            builder.CreateBitCast(env_ptr, llvm::PointerType::get(i8_ty, 0));
      }
      llvm::Value *slot_i8 = env_i8;
      if (offset != 0u)
      {
        slot_i8 = builder.CreateGEP(
            i8_ty, env_i8, llvm::ConstantInt::get(i64_ty, offset));
      }
      return builder.CreateBitCast(slot_i8, llvm::PointerType::get(target_ty, 0));
    };

    llvm::Function *abi_fn = ensure_runtime_fn(kHostAbiVersionSym, i32_ty, {});
    if (abi_fn->empty())
    {
      llvm::BasicBlock *entry = llvm::BasicBlock::Create(context_, "entry", abi_fn);
      auto *builder = static_cast<llvm::IRBuilder<> *>(builder_.get());
      builder->SetInsertPoint(entry);
      builder->CreateRet(llvm::ConstantInt::get(i32_ty, kHostAbiVersion));
    }

    llvm::Function *create_fn = ensure_runtime_fn(kHostSessionCreateSym, usize_ty, {});
    if (create_fn->empty())
    {
      llvm::BasicBlock *entry = llvm::BasicBlock::Create(context_, "entry", create_fn);
      auto *builder = static_cast<llvm::IRBuilder<> *>(builder_.get());
      builder->SetInsertPoint(entry);

      llvm::Function *alloc_fn =
          ensure_runtime_fn(kHostRuntimeAllocSym, opaque_ptr_ty, {usize_ty});
      llvm::Function *free_fn =
          ensure_runtime_fn(kHostRuntimeFreeSym,
                            llvm::Type::getVoidTy(context_),
                            {opaque_ptr_ty});
      llvm::Function *register_fn =
          ensure_runtime_fn(kHostRuntimeRegisterSym,
                            usize_ty,
                            {opaque_ptr_ty, opaque_ptr_ty});
      llvm::Function *try_enter_fn = ensure_runtime_fn(
          kHostRuntimeTryEnterSym,
          i32_ty,
          {usize_ty, opaque_ptr_ty, opaque_ptr_ptr_ty});
      llvm::Function *leave_fn = ensure_runtime_fn(
          kHostRuntimeLeaveSym, i32_ty, {usize_ty, opaque_ptr_ty});
      llvm::Function *retire_fn = ensure_runtime_fn(
          kHostRuntimeTryRetireSym,
          i32_ty,
          {usize_ty, opaque_ptr_ty, opaque_ptr_ptr_ty});
      llvm::Function *abort_live_fn = ensure_runtime_fn(
          kHostRuntimeAbortLiveSym,
          i32_ty,
          {usize_ty, opaque_ptr_ty, opaque_ptr_ptr_ty});

      llvm::Value *env_ptr =
          builder->CreateCall(alloc_fn, {llvm::ConstantInt::get(usize_ty, hosted_layout_.size)});
      llvm::BasicBlock *alloc_ok =
          llvm::BasicBlock::Create(context_, "host.create.alloc.ok", create_fn);
      llvm::BasicBlock *alloc_fail =
          llvm::BasicBlock::Create(context_, "host.create.alloc.fail", create_fn);
      builder->CreateCondBr(
          builder->CreateICmpNE(
              env_ptr, llvm::ConstantPointerNull::get(opaque_ptr_ptr_ty)),
          alloc_ok,
          alloc_fail);

      builder->SetInsertPoint(alloc_fail);
      builder->CreateRet(llvm::ConstantInt::get(usize_ty, 0));

      builder->SetInsertPoint(alloc_ok);
      builder->CreateMemSet(
          env_ptr,
          llvm::ConstantInt::get(i8_ty, 0),
          llvm::ConstantInt::get(i64_ty, hosted_layout_.size),
          llvm::Align(1));

      if (context_ty)
      {
        llvm::Value *ctx_ptr = build_env_slot_ptr(
            *builder, env_ptr, hosted_layout_.context_offset, context_ty);
        const std::string context_init_sym = ContextInitSym();
        if (std::optional<RuntimeFuncInfo> init_info =
                GetRuntimeFuncInfo(context_init_sym))
        {
          ABICallResult init_abi =
              ComputeCallABI(init_info->params, init_info->ret, true);
          if (!init_abi.valid || !init_abi.func_type)
          {
            current_ctx_->ReportCodegenFailure();
            return;
          }
          llvm::Function *context_init_fn = module_->getFunction(context_init_sym);
          if (!context_init_fn)
          {
            context_init_fn = llvm::Function::Create(
                init_abi.func_type,
                llvm::GlobalValue::ExternalLinkage,
                context_init_sym,
                module_.get());
            context_init_fn->setCallingConv(llvm::CallingConv::C);
          }
          std::vector<llvm::Value *> init_args;
          for (unsigned i = 0; i < context_init_fn->arg_size(); ++i)
          {
            llvm::Type *param_ty =
                context_init_fn->getFunctionType()->getParamType(i);
            llvm::Value *arg = nullptr;
            if (init_abi.has_sret && i == 0u)
            {
              arg = CoerceTo(builder, ctx_ptr, param_ty);
            }
            if (!arg)
            {
              current_ctx_->ReportCodegenFailure();
              return;
            }
            init_args.push_back(arg);
          }
          llvm::CallInst *init_call =
              builder->CreateCall(context_init_fn->getFunctionType(),
                                  context_init_fn,
                                  init_args);
          init_call->setCallingConv(context_init_fn->getCallingConv());
          if (!init_abi.has_sret && !init_call->getType()->isVoidTy())
          {
            builder->CreateStore(init_call, ctx_ptr);
          }
        }
      }

      for (const auto &[symbol, slot] : hosted_layout_.slots)
      {
        if (slot.zero_init || slot.bytes.empty() || slot.size == 0u)
        {
          continue;
        }

        // Hosted sessions must start from the static initializer template,
        // not from the current live DLL-global contents.
        std::vector<std::uint8_t> template_bytes(slot.size, 0u);
        const std::size_t copy_size =
            std::min<std::size_t>(template_bytes.size(), slot.bytes.size());
        std::copy(slot.bytes.begin(),
                  slot.bytes.begin() + copy_size,
                  template_bytes.begin());

        const std::string template_name = symbol + "__host_template";
        llvm::GlobalVariable *template_gv = module_->getNamedGlobal(template_name);
        if (!template_gv)
        {
          llvm::ArrayType *template_ty =
              llvm::ArrayType::get(i8_ty, template_bytes.size());
          llvm::Constant *template_init =
              llvm::ConstantDataArray::get(context_, template_bytes);
          template_gv = new llvm::GlobalVariable(
              *module_,
              template_ty,
              true,
              llvm::GlobalValue::InternalLinkage,
              template_init,
              template_name);
          template_gv->setAlignment(llvm::Align(1));
        }

        llvm::Value *slot_ptr =
            build_env_slot_ptr(*builder, env_ptr, slot.offset, llvm::Type::getInt8Ty(context_));
        llvm::Value *src_ptr = builder->CreateBitCast(
            template_gv, llvm::PointerType::get(i8_ty, 0));
        llvm::Value *dst_ptr = builder->CreateBitCast(
            slot_ptr, llvm::PointerType::get(i8_ty, 0));
        builder->CreateMemCpy(dst_ptr,
                              llvm::Align(1),
                              src_ptr,
                              llvm::Align(1),
                              llvm::ConstantInt::get(i64_ty, slot.size));
      }

      llvm::AllocaInst *entered_env =
          builder->CreateAlloca(opaque_ptr_ty, nullptr, "host_env");
      builder->CreateStore(
          llvm::ConstantPointerNull::get(opaque_ptr_ptr_ty), entered_env);
      llvm::Value *handle = builder->CreateCall(register_fn, {owner_token, env_ptr});
      llvm::BasicBlock *registered_ok =
          llvm::BasicBlock::Create(context_, "host.create.register.ok", create_fn);
      llvm::BasicBlock *registered_fail =
          llvm::BasicBlock::Create(context_, "host.create.register.fail", create_fn);
      builder->CreateCondBr(
          builder->CreateICmpNE(handle, llvm::ConstantInt::get(usize_ty, 0)),
          registered_ok,
          registered_fail);

      builder->SetInsertPoint(registered_fail);
      builder->CreateCall(free_fn, {env_ptr});
      builder->CreateRet(llvm::ConstantInt::get(usize_ty, 0));

      builder->SetInsertPoint(registered_ok);
      llvm::Value *entered_ok = builder->CreateCall(
          try_enter_fn,
          {handle, owner_token, CoerceTo(builder, entered_env, opaque_ptr_ptr_ty)});
      llvm::BasicBlock *enter_ok =
          llvm::BasicBlock::Create(context_, "host.create.enter.ok", create_fn);
      llvm::BasicBlock *enter_fail =
          llvm::BasicBlock::Create(context_, "host.create.enter.fail", create_fn);
      builder->CreateCondBr(
          builder->CreateICmpNE(entered_ok, llvm::ConstantInt::get(i32_ty, 0)),
          enter_ok,
          enter_fail);

      builder->SetInsertPoint(enter_fail);
      {
      builder->CreateStore(
          llvm::ConstantPointerNull::get(opaque_ptr_ptr_ty), entered_env);
      llvm::Value *retired = builder->CreateCall(
          retire_fn,
          {handle, owner_token, CoerceTo(builder, entered_env, opaque_ptr_ptr_ty)});
      llvm::BasicBlock *retire_ok_bb =
          llvm::BasicBlock::Create(context_, "host.create.retire.ok", create_fn);
      llvm::BasicBlock *retire_fail_bb =
          llvm::BasicBlock::Create(context_, "host.create.retire.fail", create_fn);
      builder->CreateCondBr(
          builder->CreateICmpNE(retired, llvm::ConstantInt::get(i32_ty, 0)),
          retire_ok_bb,
          retire_fail_bb);

      builder->SetInsertPoint(retire_ok_bb);
      {
        llvm::Value *cleanup_env = builder->CreateLoad(opaque_ptr_ty, entered_env);
        llvm::Value *use_cleanup_env = builder->CreateSelect(
            builder->CreateICmpNE(
                cleanup_env,
                llvm::ConstantPointerNull::get(opaque_ptr_ptr_ty)),
            cleanup_env,
            env_ptr);
        builder->CreateCall(free_fn, {use_cleanup_env});
        builder->CreateRet(llvm::ConstantInt::get(usize_ty, 0));
      }

      builder->SetInsertPoint(retire_fail_bb);
      builder->CreateStore(
          llvm::ConstantPointerNull::get(opaque_ptr_ptr_ty), entered_env);
      llvm::Value *aborted = builder->CreateCall(
          abort_live_fn,
          {handle, owner_token, CoerceTo(builder, entered_env, opaque_ptr_ptr_ty)});
      llvm::BasicBlock *abort_ok_bb =
          llvm::BasicBlock::Create(context_, "host.create.abort.ok", create_fn);
      llvm::BasicBlock *abort_fail_bb =
          llvm::BasicBlock::Create(context_, "host.create.abort.fail", create_fn);
      builder->CreateCondBr(
          builder->CreateICmpNE(aborted, llvm::ConstantInt::get(i32_ty, 0)),
          abort_ok_bb,
          abort_fail_bb);

      builder->SetInsertPoint(abort_ok_bb);
      {
        llvm::Value *cleanup_env = builder->CreateLoad(opaque_ptr_ty, entered_env);
        llvm::Value *use_cleanup_env = builder->CreateSelect(
            builder->CreateICmpNE(
                cleanup_env,
                llvm::ConstantPointerNull::get(opaque_ptr_ptr_ty)),
            cleanup_env,
            env_ptr);
        builder->CreateCall(free_fn, {use_cleanup_env});
        builder->CreateRet(llvm::ConstantInt::get(usize_ty, 0));
      }

      builder->SetInsertPoint(abort_fail_bb);
      builder->CreateRet(llvm::ConstantInt::get(usize_ty, 0));
      }

      builder->SetInsertPoint(enter_ok);
      llvm::Value *panic_ptr =
          build_env_slot_ptr(*builder, env_ptr, hosted_layout_.panic_offset, panic_record_ty);
      clear_panic_record(*builder, panic_ptr);
      for (std::size_t module_index = 0;
           module_index < current_ctx_->init_order.size();
           ++module_index)
      {
        const auto &module_path = current_ctx_->init_order[module_index];
        call_proc_with_panic(*builder, InitFn(module_path), panic_ptr, env_ptr);
        llvm::BasicBlock *cont =
            llvm::BasicBlock::Create(context_, "host.create.init.cont", create_fn);
        llvm::BasicBlock *fail =
            llvm::BasicBlock::Create(context_, "host.create.init.fail", create_fn);
        builder->CreateCondBr(load_panic_flag(*builder, panic_ptr), fail, cont);
        builder->SetInsertPoint(fail);
        {
        clear_panic_record(*builder, panic_ptr);
        for (std::size_t deinit_index = module_index; deinit_index > 0; --deinit_index)
        {
          call_proc_with_panic(*builder,
                               DeinitFn(current_ctx_->init_order[deinit_index - 1]),
                               panic_ptr,
                               env_ptr);
          clear_panic_record(*builder, panic_ptr);
        }
        llvm::Value *leave_ok =
            builder->CreateCall(leave_fn, {handle, owner_token});
        llvm::BasicBlock *leave_ok_bb =
            llvm::BasicBlock::Create(context_, "host.create.leave.ok", create_fn);
        llvm::BasicBlock *leave_fail_bb =
            llvm::BasicBlock::Create(context_, "host.create.leave.fail", create_fn);
        builder->CreateCondBr(
            builder->CreateICmpNE(leave_ok, llvm::ConstantInt::get(i32_ty, 0)),
            leave_ok_bb,
            leave_fail_bb);
        builder->SetInsertPoint(leave_fail_bb);
        builder->CreateStore(
            llvm::ConstantPointerNull::get(opaque_ptr_ptr_ty), entered_env);
        llvm::Value *aborted_retry = builder->CreateCall(
            abort_live_fn,
            {handle,
             owner_token,
             CoerceTo(builder, entered_env, opaque_ptr_ptr_ty)});
        llvm::BasicBlock *abort_ok_bb =
            llvm::BasicBlock::Create(context_, "host.create.abort.ok", create_fn);
        llvm::BasicBlock *abort_fail_bb =
            llvm::BasicBlock::Create(context_, "host.create.abort.fail", create_fn);
        builder->CreateCondBr(
            builder->CreateICmpNE(aborted_retry, llvm::ConstantInt::get(i32_ty, 0)),
            abort_ok_bb,
            abort_fail_bb);
        builder->SetInsertPoint(abort_ok_bb);
        {
          llvm::Value *cleanup_env = builder->CreateLoad(opaque_ptr_ty, entered_env);
          llvm::Value *use_cleanup_env = builder->CreateSelect(
              builder->CreateICmpNE(
                  cleanup_env,
                  llvm::ConstantPointerNull::get(opaque_ptr_ptr_ty)),
              cleanup_env,
              env_ptr);
          builder->CreateCall(free_fn, {use_cleanup_env});
          builder->CreateRet(llvm::ConstantInt::get(usize_ty, 0));
        }
        builder->SetInsertPoint(abort_fail_bb);
        builder->CreateRet(llvm::ConstantInt::get(usize_ty, 0));
        builder->SetInsertPoint(leave_ok_bb);
        builder->CreateStore(
            llvm::ConstantPointerNull::get(opaque_ptr_ptr_ty), entered_env);
        llvm::Value *retired = builder->CreateCall(
            retire_fn,
            {handle,
             owner_token,
             CoerceTo(builder, entered_env, opaque_ptr_ptr_ty)});
        llvm::BasicBlock *retire_ok_bb =
            llvm::BasicBlock::Create(context_, "host.create.retire.ok", create_fn);
        llvm::BasicBlock *retire_fail_bb =
            llvm::BasicBlock::Create(context_, "host.create.retire.fail", create_fn);
        builder->CreateCondBr(
            builder->CreateICmpNE(retired, llvm::ConstantInt::get(i32_ty, 0)),
            retire_ok_bb,
            retire_fail_bb);
        builder->SetInsertPoint(retire_ok_bb);
        {
          llvm::Value *cleanup_env = builder->CreateLoad(opaque_ptr_ty, entered_env);
          llvm::Value *use_cleanup_env = builder->CreateSelect(
              builder->CreateICmpNE(
                  cleanup_env,
                  llvm::ConstantPointerNull::get(opaque_ptr_ptr_ty)),
              cleanup_env,
              env_ptr);
          builder->CreateCall(free_fn, {use_cleanup_env});
          builder->CreateRet(llvm::ConstantInt::get(usize_ty, 0));
        }
        builder->SetInsertPoint(retire_fail_bb);
        builder->CreateStore(
            llvm::ConstantPointerNull::get(opaque_ptr_ptr_ty), entered_env);
        llvm::Value *aborted = builder->CreateCall(
            abort_live_fn,
            {handle,
             owner_token,
             CoerceTo(builder, entered_env, opaque_ptr_ptr_ty)});
        llvm::BasicBlock *abort_ok_bb2 =
            llvm::BasicBlock::Create(context_, "host.create.abort.ok", create_fn);
        llvm::BasicBlock *abort_fail_bb2 =
            llvm::BasicBlock::Create(context_, "host.create.abort.fail", create_fn);
        builder->CreateCondBr(
            builder->CreateICmpNE(aborted, llvm::ConstantInt::get(i32_ty, 0)),
            abort_ok_bb2,
            abort_fail_bb2);
        builder->SetInsertPoint(abort_ok_bb2);
        {
          llvm::Value *cleanup_env = builder->CreateLoad(opaque_ptr_ty, entered_env);
          llvm::Value *use_cleanup_env = builder->CreateSelect(
              builder->CreateICmpNE(
                  cleanup_env,
                  llvm::ConstantPointerNull::get(opaque_ptr_ptr_ty)),
              cleanup_env,
              env_ptr);
          builder->CreateCall(free_fn, {use_cleanup_env});
          builder->CreateRet(llvm::ConstantInt::get(usize_ty, 0));
        }
        builder->SetInsertPoint(abort_fail_bb2);
        builder->CreateRet(llvm::ConstantInt::get(usize_ty, 0));
        }
        builder->SetInsertPoint(cont);
      }

      llvm::Value *leave_ok = builder->CreateCall(leave_fn, {handle, owner_token});
      llvm::BasicBlock *leave_ok_bb =
          llvm::BasicBlock::Create(context_, "host.create.leave.ok", create_fn);
      llvm::BasicBlock *leave_fail_bb =
          llvm::BasicBlock::Create(context_, "host.create.leave.fail", create_fn);
      builder->CreateCondBr(
          builder->CreateICmpNE(leave_ok, llvm::ConstantInt::get(i32_ty, 0)),
          leave_ok_bb,
          leave_fail_bb);
      builder->SetInsertPoint(leave_fail_bb);
      {
      builder->CreateStore(
          llvm::ConstantPointerNull::get(opaque_ptr_ptr_ty), entered_env);
      llvm::Value *aborted = builder->CreateCall(
          abort_live_fn,
          {handle,
           owner_token,
           CoerceTo(builder, entered_env, opaque_ptr_ptr_ty)});
      llvm::BasicBlock *abort_ok_bb =
          llvm::BasicBlock::Create(context_, "host.create.abort.ok", create_fn);
      llvm::BasicBlock *abort_fail_bb =
          llvm::BasicBlock::Create(context_, "host.create.abort.fail", create_fn);
      builder->CreateCondBr(
          builder->CreateICmpNE(aborted, llvm::ConstantInt::get(i32_ty, 0)),
          abort_ok_bb,
          abort_fail_bb);
      builder->SetInsertPoint(abort_ok_bb);
      {
        llvm::Value *cleanup_env = builder->CreateLoad(opaque_ptr_ty, entered_env);
        llvm::Value *use_cleanup_env = builder->CreateSelect(
            builder->CreateICmpNE(
                cleanup_env,
                llvm::ConstantPointerNull::get(opaque_ptr_ptr_ty)),
            cleanup_env,
            env_ptr);
        builder->CreateCall(free_fn, {use_cleanup_env});
        builder->CreateRet(llvm::ConstantInt::get(usize_ty, 0));
      }
      builder->SetInsertPoint(abort_fail_bb);
      builder->CreateRet(llvm::ConstantInt::get(usize_ty, 0));
      }
      builder->SetInsertPoint(leave_ok_bb);
      builder->CreateRet(handle);
    }

    llvm::Function *destroy_fn =
        ensure_runtime_fn(kHostSessionDestroySym, i32_ty, {usize_ty});
    if (destroy_fn->empty())
    {
      llvm::BasicBlock *entry = llvm::BasicBlock::Create(context_, "entry", destroy_fn);
      auto *builder = static_cast<llvm::IRBuilder<> *>(builder_.get());
      builder->SetInsertPoint(entry);

      llvm::Function *free_fn =
          ensure_runtime_fn(kHostRuntimeFreeSym,
                            llvm::Type::getVoidTy(context_),
                            {opaque_ptr_ty});
      llvm::Function *retire_fn = ensure_runtime_fn(
          kHostRuntimeTryRetireSym,
          i32_ty,
          {usize_ty, opaque_ptr_ty, opaque_ptr_ptr_ty});
      llvm::Function *enter_retired_fn = ensure_runtime_fn(
          kHostRuntimeEnterRetiredSym,
          i32_ty,
          {usize_ty, opaque_ptr_ty, opaque_ptr_ty});
      llvm::Function *leave_retired_fn = ensure_runtime_fn(
          kHostRuntimeLeaveRetiredSym, i32_ty, {usize_ty, opaque_ptr_ty});
      llvm::Function *abort_retired_fn = ensure_runtime_fn(
          kHostRuntimeAbortRetiredSym,
          i32_ty,
          {usize_ty, opaque_ptr_ty, opaque_ptr_ty});

      llvm::Argument *handle_arg = destroy_fn->getArg(0);
      llvm::AllocaInst *retired_env =
          builder->CreateAlloca(opaque_ptr_ty, nullptr, "host_env");
      builder->CreateStore(
          llvm::ConstantPointerNull::get(opaque_ptr_ptr_ty), retired_env);
      llvm::Value *retired_ok = builder->CreateCall(
          retire_fn,
          {handle_arg, owner_token, CoerceTo(builder, retired_env, opaque_ptr_ptr_ty)});
      llvm::BasicBlock *retire_ok =
          llvm::BasicBlock::Create(context_, "host.destroy.retire.ok", destroy_fn);
      llvm::BasicBlock *retire_fail =
          llvm::BasicBlock::Create(context_, "host.destroy.retire.fail", destroy_fn);
      builder->CreateCondBr(
          builder->CreateICmpNE(retired_ok, llvm::ConstantInt::get(i32_ty, 0)),
          retire_ok,
          retire_fail);

      builder->SetInsertPoint(retire_fail);
      builder->CreateRet(llvm::ConstantInt::get(i32_ty, 0));

      builder->SetInsertPoint(retire_ok);
      llvm::Value *env_ptr = builder->CreateLoad(opaque_ptr_ty, retired_env);
      llvm::Value *entered_env = builder->CreateCall(
          enter_retired_fn, {handle_arg, owner_token, env_ptr});
      llvm::BasicBlock *enter_ok =
          llvm::BasicBlock::Create(context_, "host.destroy.enter.ok", destroy_fn);
      llvm::BasicBlock *enter_fail =
          llvm::BasicBlock::Create(context_, "host.destroy.enter.fail", destroy_fn);
      builder->CreateCondBr(
          builder->CreateICmpNE(entered_env, llvm::ConstantInt::get(i32_ty, 0)),
          enter_ok,
          enter_fail);

      builder->SetInsertPoint(enter_fail);
      {
        llvm::Value *aborted = builder->CreateCall(
            abort_retired_fn, {handle_arg, owner_token, env_ptr});
        llvm::BasicBlock *abort_ok_bb =
            llvm::BasicBlock::Create(context_, "host.destroy.enter.abort.ok", destroy_fn);
        llvm::BasicBlock *abort_fail_bb =
            llvm::BasicBlock::Create(context_, "host.destroy.enter.abort.fail", destroy_fn);
        builder->CreateCondBr(
            builder->CreateICmpNE(aborted, llvm::ConstantInt::get(i32_ty, 0)),
            abort_ok_bb,
            abort_fail_bb);
        builder->SetInsertPoint(abort_ok_bb);
        builder->CreateCall(free_fn, {env_ptr});
        builder->CreateRet(llvm::ConstantInt::get(i32_ty, 0));
        builder->SetInsertPoint(abort_fail_bb);
        builder->CreateCall(free_fn, {env_ptr});
        builder->CreateRet(llvm::ConstantInt::get(i32_ty, 0));
      }

      builder->SetInsertPoint(enter_ok);
      llvm::Value *panic_ptr =
          build_env_slot_ptr(*builder, env_ptr, hosted_layout_.panic_offset, panic_record_ty);
      clear_panic_record(*builder, panic_ptr);
      for (auto it = current_ctx_->init_order.rbegin();
           it != current_ctx_->init_order.rend();
           ++it)
      {
        call_proc_with_panic(*builder, DeinitFn(*it), panic_ptr, env_ptr);
        HandleDeinitPanic(*this, builder, panic_ptr);
      }
      RestoreDeinitPanicIfAny(*this, builder, panic_ptr);
      llvm::Value *had_panic = LoadPanicFlag(*this, builder, panic_ptr);
      llvm::Value *leave_ok =
          builder->CreateCall(leave_retired_fn, {handle_arg, owner_token});
      llvm::BasicBlock *leave_ok_bb =
          llvm::BasicBlock::Create(context_, "host.destroy.leave.ok", destroy_fn);
      llvm::BasicBlock *leave_fail_bb =
          llvm::BasicBlock::Create(context_, "host.destroy.leave.fail", destroy_fn);
      builder->CreateCondBr(
          builder->CreateICmpNE(leave_ok, llvm::ConstantInt::get(i32_ty, 0)),
          leave_ok_bb,
          leave_fail_bb);
      builder->SetInsertPoint(leave_fail_bb);
      {
      llvm::Value *aborted = builder->CreateCall(
          abort_retired_fn, {handle_arg, owner_token, env_ptr});
      llvm::BasicBlock *abort_ok_bb =
          llvm::BasicBlock::Create(context_, "host.destroy.abort.ok", destroy_fn);
      llvm::BasicBlock *abort_fail_bb =
          llvm::BasicBlock::Create(context_, "host.destroy.abort.fail", destroy_fn);
      builder->CreateCondBr(
          builder->CreateICmpNE(aborted, llvm::ConstantInt::get(i32_ty, 0)),
          abort_ok_bb,
          abort_fail_bb);
      builder->SetInsertPoint(abort_ok_bb);
      builder->CreateCall(free_fn, {env_ptr});
      builder->CreateRet(llvm::ConstantInt::get(i32_ty, 0));
      builder->SetInsertPoint(abort_fail_bb);
      builder->CreateRet(llvm::ConstantInt::get(i32_ty, 0));
      }
      builder->SetInsertPoint(leave_ok_bb);
      builder->CreateCall(free_fn, {env_ptr});
      llvm::Value *destroy_ok = builder->CreateSelect(
          had_panic,
          llvm::ConstantInt::get(i32_ty, 0),
          llvm::ConstantInt::get(i32_ty, 1));
      builder->CreateRet(destroy_ok);
    }
  }

  void LLVMEmitter::EmitHostedExportThunks()
  {
    if (!current_ctx_ || !current_ctx_->hosted_library ||
        current_ctx_->hosted_exports.empty())
    {
      return;
    }

    auto *builder = static_cast<llvm::IRBuilder<> *>(builder_.get());
    auto *i1_ty = llvm::Type::getInt1Ty(context_);
    auto *i8_ty = llvm::Type::getInt8Ty(context_);
    auto *i32_ty = llvm::Type::getInt32Ty(context_);
    auto *i64_ty = llvm::Type::getInt64Ty(context_);
    llvm::Type *usize_ty = llvm::Type::getInt64Ty(context_);
    llvm::Type *opaque_ptr_ty = GetOpaquePtr();
    auto *opaque_ptr_ptr_ty = llvm::cast<llvm::PointerType>(opaque_ptr_ty);
    llvm::Type *panic_record_ty = GetLLVMType(PanicRecordType());
    llvm::Type *context_ty = GetLLVMType(analysis::MakeTypePath({"Context"}));
    llvm::GlobalVariable *owner_token_gv = EnsureHostedOwnerTokenGlobal(
        module_.get(), context_, IsProjectEntryModule(current_ctx_));
    if (!owner_token_gv)
    {
      current_ctx_->ReportCodegenFailure();
      return;
    }
    llvm::Value *owner_token =
        llvm::ConstantExpr::getBitCast(owner_token_gv, opaque_ptr_ty);
    const analysis::ScopeContext &scope = BuildScope(current_ctx_);
    auto panic_offsets = [&]() {
      std::uint64_t flag_offset = 0;
      std::uint64_t code_offset = 4;
      const auto layout = ::cursive::analysis::layout::RecordLayoutOf(
          scope,
          {analysis::MakeTypePrim("bool"), analysis::MakeTypePrim("u32")});
      if (layout.has_value() && layout->offsets.size() >= 2)
      {
        flag_offset = layout->offsets[0];
        code_offset = layout->offsets[1];
      }
      return std::pair<std::uint64_t, std::uint64_t>{flag_offset, code_offset};
    }();

    auto ensure_runtime_fn = [&](const char *name,
                                 llvm::Type *ret_ty,
                                 std::vector<llvm::Type *> params) -> llvm::Function * {
      llvm::Function *fn = module_->getFunction(name);
      if (!fn)
      {
        llvm::FunctionType *fn_ty = llvm::FunctionType::get(ret_ty, params, false);
        fn = llvm::Function::Create(
            fn_ty,
            llvm::GlobalValue::ExternalLinkage,
            name,
            module_.get());
        fn->setCallingConv(llvm::CallingConv::C);
      }
      return fn;
    };

    auto runtime_panic_fn = [&]() -> llvm::Function * {
      llvm::Function *fn = module_->getFunction(RuntimePanicSym());
      if (!fn)
      {
        llvm::FunctionType *fn_ty =
            llvm::FunctionType::get(llvm::Type::getVoidTy(context_), {i32_ty}, false);
        fn = llvm::Function::Create(
            fn_ty,
            llvm::GlobalValue::ExternalLinkage,
            RuntimePanicSym(),
            module_.get());
        fn->setCallingConv(llvm::CallingConv::C);
      }
      return fn;
    }();

    auto build_env_slot_ptr = [&](llvm::IRBuilder<> &irb,
                                  llvm::Value *env_ptr,
                                  std::uint64_t offset,
                                  llvm::Type *target_ty) -> llvm::Value * {
      llvm::Value *env_i8 =
          CoerceTo(&irb, env_ptr, llvm::PointerType::get(i8_ty, 0));
      if (!env_i8)
      {
        env_i8 = irb.CreateBitCast(env_ptr, llvm::PointerType::get(i8_ty, 0));
      }
      llvm::Value *slot_i8 = env_i8;
      if (offset != 0u)
      {
        slot_i8 = irb.CreateGEP(
            i8_ty, env_i8, llvm::ConstantInt::get(i64_ty, offset));
      }
      return irb.CreateBitCast(slot_i8, llvm::PointerType::get(target_ty, 0));
    };

    auto panic_field_ptr = [&](llvm::IRBuilder<> &irb,
                               llvm::Value *panic_ptr,
                               std::uint64_t offset,
                               llvm::Type *field_ty) -> llvm::Value * {
      llvm::Value *panic_i8 =
          CoerceTo(&irb, panic_ptr, llvm::PointerType::get(i8_ty, 0));
      if (!panic_i8)
      {
        panic_i8 = irb.CreateBitCast(panic_ptr, llvm::PointerType::get(i8_ty, 0));
      }
      llvm::Value *field_i8 = panic_i8;
      if (offset != 0u)
      {
        field_i8 = irb.CreateGEP(
            i8_ty, panic_i8, llvm::ConstantInt::get(i64_ty, offset));
      }
      return irb.CreateBitCast(field_i8, llvm::PointerType::get(field_ty, 0));
    };

    auto clear_panic_record = [&](llvm::IRBuilder<> &irb, llvm::Value *panic_ptr) {
      irb.CreateStore(llvm::ConstantInt::get(llvm::Type::getInt8Ty(context_), 0),
                      panic_field_ptr(irb,
                                      panic_ptr,
                                      panic_offsets.first,
                                      llvm::Type::getInt8Ty(context_)));
      irb.CreateStore(llvm::ConstantInt::get(i32_ty, 0),
                      panic_field_ptr(irb, panic_ptr, panic_offsets.second, i32_ty));
    };

    auto load_panic_flag = [&](llvm::IRBuilder<> &irb,
                               llvm::Value *panic_ptr) -> llvm::Value * {
      return irb.CreateICmpNE(
          irb.CreateLoad(
              llvm::Type::getInt8Ty(context_),
              panic_field_ptr(irb,
                              panic_ptr,
                              panic_offsets.first,
                              llvm::Type::getInt8Ty(context_))),
          llvm::ConstantInt::get(llvm::Type::getInt8Ty(context_), 0));
    };

    auto load_panic_code = [&](llvm::IRBuilder<> &irb,
                               llvm::Value *panic_ptr) -> llvm::Value * {
      return irb.CreateLoad(
          i32_ty,
          panic_field_ptr(irb, panic_ptr, panic_offsets.second, i32_ty));
    };

    auto normalize_type =
        [&](auto &&self, analysis::TypeRef type, std::size_t depth) -> analysis::TypeRef {
      if (!type || depth > 16u)
      {
        return type;
      }
      analysis::TypeRef cur = analysis::StripPerm(type);
      if (!cur)
      {
        cur = type;
      }
      while (cur)
      {
        if (const auto *refine = std::get_if<analysis::TypeRefine>(&cur->node))
        {
          cur = analysis::StripPerm(refine->base);
          if (!cur)
          {
            cur = refine->base;
          }
          continue;
        }
        break;
      }
      if (const auto *path = cur ? std::get_if<analysis::TypePathType>(&cur->node) : nullptr)
      {
        if (path->generic_args.empty())
        {
          ast::Path syntax_path;
          for (const auto &comp : path->path)
          {
            syntax_path.push_back(comp);
          }
          const auto it = scope.sigma.types.find(analysis::PathKeyOf(syntax_path));
          if (it != scope.sigma.types.end())
          {
            if (const auto *alias = std::get_if<ast::TypeAliasDecl>(&it->second))
            {
              const auto lowered = analysis::LowerType(scope, alias->type);
              if (lowered.ok && lowered.type)
              {
                return self(self, lowered.type, depth + 1u);
              }
            }
          }
        }
      }
      return cur;
    };

    auto context_field_value =
        [&](llvm::IRBuilder<> &irb,
            llvm::Value *ctx_value,
            std::string_view field_name) -> llvm::Value * {
      struct ContextFieldInfo {
        const char *name;
        analysis::TypeRef type;
      };
      const std::array<ContextFieldInfo, 5> fields = {{
          {"fs", analysis::MakeTypeDynamic({"FileSystem"})},
          {"net", analysis::MakeTypeDynamic({"Network"})},
          {"heap", analysis::MakeTypeDynamic({"HeapAllocator"})},
          {"sys", analysis::MakeTypePath({"System"})},
          {"reactor", analysis::MakeTypeDynamic({"Reactor"})},
      }};
      std::size_t extract_index = 0u;
      for (const auto &field : fields)
      {
        const auto size = ::cursive::analysis::layout::SizeOf(scope, field.type).value_or(0u);
        if (std::string_view(field.name) == field_name)
        {
          if (size == 0u)
          {
            llvm::Type *field_ty = GetLLVMType(field.type);
            return field_ty && !field_ty->isVoidTy()
                       ? llvm::Constant::getNullValue(field_ty)
                       : nullptr;
          }
          return irb.CreateExtractValue(ctx_value, {static_cast<unsigned>(extract_index)});
        }
        if (size != 0u)
        {
          ++extract_index;
        }
      }
      return nullptr;
    };

    auto build_context_bundle =
        [&](auto &&self,
            llvm::IRBuilder<> &irb,
            analysis::TypeRef target_type,
            std::string_view field_name,
            llvm::Value *root_ctx_ptr,
            llvm::Value *root_ctx_value) -> llvm::Value * {
      analysis::TypeRef cur = normalize_type(normalize_type, target_type, 0u);
      if (!cur)
      {
        return nullptr;
      }

      if (const auto *dyn = std::get_if<analysis::TypeDynamic>(&cur->node))
      {
        if (field_name == "cpu" || field_name == "gpu" || field_name == "inline")
        {
          const analysis::TypeRef expected_context_type =
              analysis::MakeTypePath({"Context"});
          const analysis::TypeRef expected_domain_type =
              analysis::MakeTypeDynamic({"ExecutionDomain"});
          std::string runtime_sym =
              field_name == "cpu" ? BuiltinSymContextCpu()
              : field_name == "gpu" ? BuiltinSymContextGpu()
                                    : BuiltinSymContextInline();
          if (auto runtime_info = GetRuntimeFuncInfo(runtime_sym))
          {
            const auto ctx_eq =
                analysis::TypeEquiv(runtime_info->params.size() == 1u
                                        ? runtime_info->params[0].type
                                        : nullptr,
                                    expected_context_type);
            const auto ret_eq =
                analysis::TypeEquiv(runtime_info->ret, expected_domain_type);
            const auto target_eq = analysis::TypeEquiv(cur, expected_domain_type);
            if (runtime_info->params.size() != 1u || !ctx_eq.ok || !ctx_eq.equiv ||
                !ret_eq.ok || !ret_eq.equiv || !target_eq.ok || !target_eq.equiv)
            {
              current_ctx_->ReportCodegenFailure();
              return nullptr;
            }

            ABICallResult abi =
                ComputeCallABI(runtime_info->params, runtime_info->ret, true);
            if (!abi.valid || !abi.func_type || abi.param_kinds.size() != 1u)
            {
              current_ctx_->ReportCodegenFailure();
              return nullptr;
            }
            llvm::Function *fn = module_->getFunction(runtime_sym);
            if (!fn)
            {
              fn = llvm::Function::Create(
                  abi.func_type,
                  llvm::GlobalValue::ExternalLinkage,
                  runtime_sym,
                  module_.get());
              fn->setCallingConv(llvm::CallingConv::C);
            }

            llvm::Value *context_arg = nullptr;
            if (abi.param_kinds[0] == PassKind::ByRef)
            {
              context_arg = root_ctx_ptr;
            }
            else
            {
              context_arg = root_ctx_value;
            }
            if (!context_arg)
            {
              current_ctx_->ReportCodegenFailure();
              return nullptr;
            }

            return EmitABICall(
                *this,
                &irb,
                fn,
                runtime_info->params,
                runtime_info->ret,
                {context_arg},
                true);
          }
          current_ctx_->ReportCodegenFailure();
          return nullptr;
        }
        return context_field_value(irb, root_ctx_value, field_name);
      }

      if (const auto *path = std::get_if<analysis::TypePathType>(&cur->node))
      {
        if (path->generic_args.empty() && path->path.size() == 1u &&
            path->path.front() == "System")
        {
          llvm::Type *target_ll = GetLLVMType(cur);
          return target_ll && !target_ll->isVoidTy()
                     ? llvm::Constant::getNullValue(target_ll)
                     : nullptr;
        }

        if (const ast::RecordDecl *record =
                analysis::LookupRecordDecl(scope, path->path))
        {
          llvm::Type *target_ll = GetLLVMType(cur);
          if (!target_ll || target_ll->isVoidTy())
          {
            return nullptr;
          }
          llvm::Value *aggregate = llvm::Constant::getNullValue(target_ll);
          unsigned insert_index = 0u;
          for (const auto &member : record->members)
          {
            const auto *field = std::get_if<ast::FieldDecl>(&member);
            if (!field)
            {
              continue;
            }
            auto lowered = analysis::LowerType(scope, field->type);
            if (!lowered.ok || !lowered.type)
            {
              continue;
            }
            llvm::Value *field_value = self(
                self, irb, lowered.type, field->name, root_ctx_ptr, root_ctx_value);
            const auto field_size = ::cursive::analysis::layout::SizeOf(scope, lowered.type).value_or(0u);
            if (field_size == 0u)
            {
              continue;
            }
            if (!field_value)
            {
              llvm::Type *field_ty = GetLLVMType(lowered.type);
              if (!field_ty || field_ty->isVoidTy())
              {
                continue;
              }
              field_value = llvm::Constant::getNullValue(field_ty);
            }
            aggregate = irb.CreateInsertValue(
                aggregate, field_value, {insert_index++});
          }
          return aggregate;
        }
      }

      return context_field_value(irb, root_ctx_value, field_name);
    };

    llvm::Function *try_enter_fn = ensure_runtime_fn(
        kHostRuntimeTryEnterSym,
        i32_ty,
        {usize_ty, opaque_ptr_ty, opaque_ptr_ptr_ty});
    llvm::Function *leave_fn = ensure_runtime_fn(
        kHostRuntimeLeaveSym, i32_ty, {usize_ty, opaque_ptr_ty});

    for (const auto &info : current_ctx_->hosted_exports)
    {
      const auto *proc_module = current_ctx_->LookupProcModule(info.internal_symbol);
      if (!proc_module || *proc_module != current_ctx_->module_path)
      {
        continue;
      }

      std::vector<IRParam> thunk_params;
      thunk_params.push_back(IRParam{analysis::ParamMode::Move,
                                     "__cursive_session",
                                     "__cursive_session",
                                     analysis::MakeTypePrim("usize")});
      thunk_params.insert(thunk_params.end(),
                          info.visible_params.begin(),
                          info.visible_params.end());
      ABICallResult thunk_abi = ComputeCallABI(
          thunk_params,
          info.ret,
          /*use_c_abi_aggregate_sret=*/true,
          /*foreign_boundary_mode_independent=*/true);
      if (!thunk_abi.valid || !thunk_abi.func_type)
      {
        current_ctx_->ReportCodegenFailure();
        continue;
      }
      llvm::Function *thunk = module_->getFunction(info.thunk_symbol);
      if (!thunk)
      {
        thunk = llvm::Function::Create(
            thunk_abi.func_type,
            llvm::GlobalValue::ExternalLinkage,
            info.thunk_symbol,
            module_.get());
      }
      thunk->setCallingConv(CallingConvForAbi(info.abi));
      if (!thunk->empty())
      {
        continue;
      }

      llvm::BasicBlock *entry = llvm::BasicBlock::Create(context_, "entry", thunk);
      builder->SetInsertPoint(entry);
      locals_.clear();
      local_types_.clear();
      values_.clear();

      struct PreparedArg {
        llvm::Value *value = nullptr;
        llvm::Value *storage = nullptr;
        analysis::TypeRef type;
      };
      std::vector<PreparedArg> prepared(thunk_params.size());
      llvm::IRBuilder<> entry_builder(&thunk->getEntryBlock(),
                                      thunk->getEntryBlock().begin());
      auto create_entry_alloca =
          [&](llvm::Type *alloc_ty, llvm::StringRef name) -> llvm::AllocaInst * {
        llvm::IRBuilder<> alloca_builder(&thunk->getEntryBlock(),
                                         thunk->getEntryBlock().begin());
        return alloca_builder.CreateAlloca(alloc_ty, nullptr, name);
      };
      llvm::Type *ret_ll = GetLLVMType(info.ret);
      llvm::Value *thunk_sret_storage = nullptr;
      if (thunk_abi.has_sret && ret_ll && !ret_ll->isVoidTy() &&
          thunk->arg_size() > 0u)
      {
        thunk_sret_storage = thunk->getArg(0);
      }
      auto typed_thunk_sret_storage =
          [&](llvm::IRBuilder<> &irb) -> llvm::Value * {
        if (!thunk_sret_storage || !ret_ll || ret_ll->isVoidTy())
        {
          return nullptr;
        }
        llvm::Value *typed_ptr = thunk_sret_storage;
        llvm::Type *target_ptr_ty = llvm::PointerType::get(ret_ll, 0);
        if (typed_ptr->getType() != target_ptr_ty)
        {
          typed_ptr = irb.CreateBitCast(typed_ptr, target_ptr_ty);
        }
        return typed_ptr;
      };
      auto zero_thunk_sret =
          [&](llvm::IRBuilder<> &irb) {
        llvm::Value *typed_ptr = typed_thunk_sret_storage(irb);
        if (!typed_ptr)
        {
          return;
        }
        const llvm::DataLayout &dl = module_->getDataLayout();
        llvm::Value *dst_ptr = irb.CreateBitCast(
            typed_ptr, llvm::PointerType::get(i8_ty, 0));
        irb.CreateMemSet(dst_ptr,
                         llvm::ConstantInt::get(i8_ty, 0),
                         llvm::ConstantInt::get(i64_ty, dl.getTypeStoreSize(ret_ll)),
                         llvm::Align(1));
      };
      auto store_into_thunk_sret =
          [&](llvm::IRBuilder<> &irb,
              llvm::Value *aggregate_value,
              llvm::Value *aggregate_storage) -> bool {
        llvm::Value *typed_ptr = typed_thunk_sret_storage(irb);
        if (!typed_ptr)
        {
          return false;
        }
        if (aggregate_storage)
        {
          const llvm::DataLayout &dl = module_->getDataLayout();
          llvm::Value *src_ptr = aggregate_storage;
          llvm::Type *target_ptr_ty = llvm::PointerType::get(ret_ll, 0);
          if (src_ptr->getType() != target_ptr_ty)
          {
            src_ptr = irb.CreateBitCast(src_ptr, target_ptr_ty);
          }
          llvm::Value *dst_i8 = irb.CreateBitCast(
              typed_ptr, llvm::PointerType::get(i8_ty, 0));
          llvm::Value *src_i8 = irb.CreateBitCast(
              src_ptr, llvm::PointerType::get(i8_ty, 0));
          irb.CreateMemCpy(dst_i8,
                           llvm::Align(1),
                           src_i8,
                           llvm::Align(1),
                           llvm::ConstantInt::get(
                               i64_ty, dl.getTypeStoreSize(ret_ll)));
          return true;
        }
        if (!aggregate_value)
        {
          return false;
        }
        if (aggregate_value->getType() != ret_ll)
        {
          if (llvm::Value *coerced = CoerceTo(&irb, aggregate_value, ret_ll))
          {
            aggregate_value = coerced;
          }
          else
          {
            return false;
          }
        }
        irb.CreateStore(aggregate_value, typed_ptr);
        return true;
      };
      auto emit_hosted_boundary_failure =
          [&](llvm::IRBuilder<> &irb, llvm::Value *panic_code_value) {
        if (info.unwind_mode == LowerCtx::ExportUnwindMode::Catch)
        {
          zero_thunk_sret(irb);
          if (thunk->getReturnType()->isVoidTy())
          {
            irb.CreateRetVoid();
          }
          else
          {
            irb.CreateRet(llvm::Constant::getNullValue(thunk->getReturnType()));
          }
          return;
        }
        if (!panic_code_value)
        {
          panic_code_value = llvm::ConstantInt::get(
              i32_ty, PanicCode(PanicReason::Other));
        }
        if (panic_code_value->getType() != i32_ty)
        {
          panic_code_value = irb.CreateIntCast(panic_code_value, i32_ty, false);
        }
        irb.CreateCall(runtime_panic_fn, {panic_code_value});
        irb.CreateUnreachable();
      };
      for (std::size_t i = 0; i < thunk_params.size(); ++i)
      {
        if (i >= thunk_abi.param_indices.size() || !thunk_abi.param_indices[i].has_value())
        {
          continue;
        }
        llvm::Argument *arg = thunk->getArg(*thunk_abi.param_indices[i]);
        arg->setName(thunk_params[i].name);
        llvm::Type *param_ll = GetLLVMType(thunk_params[i].type);
        const ABIArgCarrierKind carrier =
            i < thunk_abi.param_carriers.size()
                ? thunk_abi.param_carriers[i]
                : ABIArgCarrierKind::Direct;
        if (thunk_abi.param_kinds[i] == PassKind::ByRef)
        {
          prepared[i].storage = arg;
          prepared[i].type = thunk_params[i].type;
          if (param_ll)
          {
            llvm::Value *typed_ptr = arg;
            if (typed_ptr->getType() != llvm::PointerType::get(param_ll, 0))
            {
              typed_ptr =
                  builder->CreateBitCast(typed_ptr, llvm::PointerType::get(param_ll, 0));
            }
            prepared[i].storage = typed_ptr;
            prepared[i].value = builder->CreateLoad(param_ll, typed_ptr);
          }
        }
        else if (carrier == ABIArgCarrierKind::Indirect)
        {
          prepared[i].type = thunk_params[i].type;
          if (param_ll)
          {
            llvm::Value *typed_ptr = arg;
            llvm::Type *target_ptr_ty = llvm::PointerType::get(param_ll, 0);
            if (typed_ptr->getType() != target_ptr_ty)
            {
              typed_ptr = builder->CreateBitCast(typed_ptr, target_ptr_ty);
            }
            prepared[i].storage = typed_ptr;
            prepared[i].value = builder->CreateLoad(param_ll, typed_ptr);
          }
          else
          {
            prepared[i].value = arg;
          }
        }
        else
        {
          prepared[i].type = thunk_params[i].type;
          llvm::Value *prepared_value = arg;
          if (param_ll && prepared_value && prepared_value->getType() != param_ll)
          {
            if (llvm::Value *coerced = CoerceTo(builder, prepared_value, param_ll))
            {
              prepared_value = coerced;
            }
          }
          prepared[i].value = prepared_value;
          if (param_ll && !param_ll->isVoidTy())
          {
            llvm::AllocaInst *slot =
                create_entry_alloca(param_ll, thunk_params[i].name);
            llvm::Value *stored_value = prepared_value;
            if (!stored_value)
            {
              stored_value = llvm::Constant::getNullValue(param_ll);
            }
            else if (stored_value->getType() != param_ll)
            {
              if (llvm::Value *coerced = CoerceTo(builder, stored_value, param_ll))
              {
                stored_value = coerced;
              }
              else
              {
                stored_value = llvm::Constant::getNullValue(param_ll);
              }
            }
            builder->CreateStore(stored_value, slot);
            prepared[i].storage = slot;
          }
        }
      }

      llvm::Value *handle = prepared[0].value;
      if (!handle)
      {
        handle = llvm::ConstantInt::get(usize_ty, 0);
      }
      if (handle->getType() != usize_ty)
      {
        handle = builder->CreateIntCast(handle, usize_ty, false);
      }

      llvm::AllocaInst *entered_env =
          create_entry_alloca(opaque_ptr_ty, "host_env");
      builder->CreateStore(
          llvm::ConstantPointerNull::get(opaque_ptr_ptr_ty), entered_env);

      llvm::Value *entered_ok = builder->CreateCall(
          try_enter_fn,
          {handle, owner_token, CoerceTo(builder, entered_env, opaque_ptr_ptr_ty)});
      llvm::BasicBlock *entered_bb =
          llvm::BasicBlock::Create(context_, "host.enter.ok", thunk);
      llvm::BasicBlock *rejected_bb =
          llvm::BasicBlock::Create(context_, "host.enter.reject", thunk);
      builder->CreateCondBr(
          builder->CreateICmpNE(entered_ok, llvm::ConstantInt::get(i32_ty, 0)),
          entered_bb,
          rejected_bb);

      builder->SetInsertPoint(rejected_bb);
      emit_hosted_boundary_failure(
          *builder,
          llvm::ConstantInt::get(i32_ty, PanicCode(PanicReason::ForeignPre)));

      builder->SetInsertPoint(entered_bb);
      llvm::Value *env_ptr = builder->CreateLoad(opaque_ptr_ty, entered_env);
      llvm::Value *ctx_ptr =
          build_env_slot_ptr(*builder, env_ptr, hosted_layout_.context_offset, context_ty);
      llvm::Value *ctx_value = builder->CreateLoad(context_ty, ctx_ptr);
      llvm::Value *panic_ptr =
          build_env_slot_ptr(*builder, env_ptr, hosted_layout_.panic_offset, panic_record_ty);
      clear_panic_record(*builder, panic_ptr);

      const LowerCtx::ProcSigInfo *internal_sig =
          current_ctx_->LookupProcSig(info.internal_symbol);
      if (!internal_sig)
      {
        current_ctx_->ReportCodegenFailure();
        llvm::Value *leave_ok =
            builder->CreateCall(leave_fn, {handle, owner_token});
        llvm::BasicBlock *leave_ok_bb =
            llvm::BasicBlock::Create(context_, "host.call.leave.ok", thunk);
        llvm::BasicBlock *leave_fail_bb =
            llvm::BasicBlock::Create(context_, "host.call.leave.fail", thunk);
        builder->CreateCondBr(
            builder->CreateICmpNE(leave_ok, llvm::ConstantInt::get(i32_ty, 0)),
            leave_ok_bb,
            leave_fail_bb);
        builder->SetInsertPoint(leave_fail_bb);
        emit_hosted_boundary_failure(
            *builder,
            llvm::ConstantInt::get(i32_ty, PanicCode(PanicReason::Other)));
        builder->SetInsertPoint(leave_ok_bb);
        emit_hosted_boundary_failure(
            *builder,
            llvm::ConstantInt::get(i32_ty, PanicCode(PanicReason::Other)));
        continue;
      }

      std::vector<PreparedArg> source_args;
      const bool internal_needs_hosted_env =
          RequiresHostedEnvParam(info.internal_symbol);
      std::vector<IRParam> internal_params = internal_sig->params;
      if (internal_needs_hosted_env &&
          !HasLeadingHostedEnvParam(internal_params))
      {
        internal_params.insert(internal_params.begin(), HostedEnvParam());
      }
      source_args.reserve(internal_params.size());
      if (internal_needs_hosted_env)
      {
        PreparedArg env_arg;
        env_arg.type = HostedEnvParamType();
        env_arg.value = CoerceTo(builder, env_ptr, GetLLVMType(env_arg.type));
        if (!env_arg.value)
        {
          env_arg.value = env_ptr;
        }
        source_args.push_back(env_arg);
      }
      PreparedArg ctx_arg;
      ctx_arg.type = info.context_type;
      ctx_arg.value = build_context_bundle(
          build_context_bundle, *builder, info.context_type, "", ctx_ptr, ctx_value);
      if (llvm::Type *ctx_ll = GetLLVMType(info.context_type))
      {
        llvm::AllocaInst *slot = create_entry_alloca(ctx_ll, "host_ctx");
        if (ctx_arg.value)
        {
          builder->CreateStore(ctx_arg.value, slot);
        }
        else
        {
          builder->CreateStore(llvm::Constant::getNullValue(ctx_ll), slot);
        }
        ctx_arg.storage = slot;
      }
      source_args.push_back(ctx_arg);
      for (std::size_t i = 1; i < prepared.size(); ++i)
      {
        source_args.push_back(prepared[i]);
      }
      PreparedArg panic_arg;
      panic_arg.type = PanicOutType();
      panic_arg.value = panic_ptr;
      panic_arg.storage = nullptr;
      source_args.push_back(panic_arg);

      ABICallResult internal_abi =
          ComputeCallABI(internal_params, internal_sig->ret);
      llvm::Function *callee = module_->getFunction(info.internal_symbol);
      if (!callee)
      {
        callee = functions_[info.internal_symbol];
      }
      std::vector<llvm::Value *> call_args;
      llvm::AllocaInst *ret_slot = nullptr;
      if (internal_abi.has_sret && ret_ll && !ret_ll->isVoidTy())
      {
        ret_slot = create_entry_alloca(ret_ll, "host_ret");
        call_args.push_back(CoerceTo(builder, ret_slot, opaque_ptr_ty));
      }
      for (std::size_t i = 0; i < internal_params.size(); ++i)
      {
        const auto kind =
            i < internal_abi.param_kinds.size() ? internal_abi.param_kinds[i]
                                                : PassKind::ByValue;
        llvm::Value *arg = nullptr;
        if (i < source_args.size())
        {
          if (kind == PassKind::ByRef)
          {
            arg = source_args[i].storage;
            if (!arg && source_args[i].value && source_args[i].type)
            {
              if (llvm::Type *arg_ll = GetLLVMType(source_args[i].type))
              {
                llvm::AllocaInst *slot =
                    create_entry_alloca(arg_ll, "host_arg");
                builder->CreateStore(source_args[i].value, slot);
                arg = slot;
              }
            }
          }
          else
          {
            arg = source_args[i].value;
          }
        }
        if (!arg)
        {
          llvm::Type *param_ty =
              callee ? callee->getFunctionType()->getParamType(
                           static_cast<unsigned>(call_args.size()))
                     : nullptr;
          arg = param_ty ? llvm::Constant::getNullValue(param_ty)
                         : llvm::ConstantInt::get(i64_ty, 0);
        }
        else if (callee)
        {
          llvm::Type *param_ty =
              callee->getFunctionType()->getParamType(
                  static_cast<unsigned>(call_args.size()));
          if (arg->getType() != param_ty)
          {
            if (llvm::Value *coerced = CoerceTo(builder, arg, param_ty))
            {
              arg = coerced;
            }
          }
        }
        call_args.push_back(arg);
      }

      llvm::Value *result_value = nullptr;
      if (callee)
      {
        llvm::CallInst *call =
            builder->CreateCall(callee->getFunctionType(), callee, call_args);
        if (internal_abi.has_sret)
        {
          if (ret_slot)
          {
            result_value = builder->CreateLoad(ret_ll, ret_slot);
          }
        }
        else if (!call->getType()->isVoidTy())
        {
          result_value = call;
        }
      }

      llvm::Value *panic_flag = load_panic_flag(*builder, panic_ptr);
      llvm::Value *panic_code = load_panic_code(*builder, panic_ptr);
      llvm::Value *leave_ok = builder->CreateCall(leave_fn, {handle, owner_token});
      llvm::BasicBlock *leave_ok_bb =
          llvm::BasicBlock::Create(context_, "host.call.leave.ok", thunk);
      llvm::BasicBlock *leave_fail_bb =
          llvm::BasicBlock::Create(context_, "host.call.leave.fail", thunk);
      builder->CreateCondBr(
          builder->CreateICmpNE(leave_ok, llvm::ConstantInt::get(i32_ty, 0)),
          leave_ok_bb,
          leave_fail_bb);
      builder->SetInsertPoint(leave_fail_bb);
      emit_hosted_boundary_failure(
          *builder,
          llvm::ConstantInt::get(i32_ty, PanicCode(PanicReason::Other)));
      builder->SetInsertPoint(leave_ok_bb);

      llvm::BasicBlock *panic_bb =
          llvm::BasicBlock::Create(context_, "host.call.panic", thunk);
      llvm::BasicBlock *ok_bb =
          llvm::BasicBlock::Create(context_, "host.call.ok", thunk);
      builder->CreateCondBr(
          panic_flag,
          panic_bb,
          ok_bb);

      builder->SetInsertPoint(panic_bb);
      emit_hosted_boundary_failure(*builder, panic_code);

      builder->SetInsertPoint(ok_bb);
      if (thunk->getReturnType()->isVoidTy())
      {
        if (thunk_abi.has_sret)
        {
          const bool stored = store_into_thunk_sret(
              *builder, result_value, ret_slot);
          if (!stored)
          {
            zero_thunk_sret(*builder);
          }
        }
        builder->CreateRetVoid();
      }
      else
      {
        if (!result_value)
        {
          result_value = llvm::Constant::getNullValue(thunk->getReturnType());
        }
        else if (result_value->getType() != thunk->getReturnType())
        {
          if (llvm::Value *coerced =
                  CoerceTo(builder, result_value, thunk->getReturnType()))
          {
            result_value = coerced;
          }
          else
          {
            result_value = llvm::Constant::getNullValue(thunk->getReturnType());
          }
        }
        builder->CreateRet(result_value);
      }
    }
  }

  void LLVMEmitter::EmitEntryPoint()
  {
    if (!current_ctx_)
    {
      return;
    }

    const auto entry_decl = EntryStubDecl(*current_ctx_);
    if (!entry_decl.has_value())
    {
      return;
    }

    SPEC_RULE("LowerIRDecl-EntryPoint");

    const bool returns_exit_code =
        target_profile_ == project::TargetProfile::X86_64SysV;
    // Create the generated entry stub symbol. Windows uses it as the process
    // entrypoint directly; x86_64 SysV links a runtime-provided _start shim
    // that calls this stub and exits with its return code.
    llvm::FunctionType *main_ty =
        llvm::FunctionType::get(
            returns_exit_code ? llvm::Type::getInt32Ty(context_)
                              : llvm::Type::getVoidTy(context_),
            {},
            false);

    llvm::Function *main_fn = llvm::Function::Create(
        main_ty,
        llvm::GlobalValue::ExternalLinkage,
        entry_decl->symbol,
        module_.get());

    llvm::BasicBlock *entry = llvm::BasicBlock::Create(context_, "entry", main_fn);
    auto *builder = static_cast<llvm::IRBuilder<> *>(builder_.get());
    builder->SetInsertPoint(entry);

    // Configure runtime log/trace sink when --log, --log-file, or --trace is used.
    if (current_ctx_->log_enabled)
    {
      const std::string set_sink_sym = RuntimeConformanceSetSinkSym();
      llvm::Function *set_sink_fn = module_->getFunction(set_sink_sym);
      llvm::Type *void_ty = llvm::Type::getVoidTy(context_);
      llvm::Type *i8_ty = llvm::Type::getInt8Ty(context_);
      llvm::Type *i64_ty = llvm::Type::getInt64Ty(context_);
      if (!set_sink_fn)
      {
        llvm::FunctionType *set_sink_ty = llvm::FunctionType::get(
            void_ty, {i8_ty, GetOpaquePtr(), i64_ty}, false);
        set_sink_fn = llvm::Function::Create(set_sink_ty,
                                             llvm::GlobalValue::ExternalLinkage,
                                             set_sink_sym,
                                             module_.get());
      }

      if (set_sink_fn)
      {
        const bool to_file =
            current_ctx_->log_to_file &&
            !current_ctx_->log_file_path.empty();
        const bool to_console = current_ctx_->log_to_console || !to_file;
        std::uint8_t sink_mode = 0u;
        if (to_file && to_console)
        {
          sink_mode = 2u;
        }
        else if (to_file)
        {
          sink_mode = 1u;
        }
        llvm::Value *sink_kind =
            llvm::ConstantInt::get(i8_ty, sink_mode);
        llvm::Value *path_ptr =
            llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(GetOpaquePtr()));
        llvm::Value *path_len = llvm::ConstantInt::get(i64_ty, 0u);
        if (to_file)
        {
          llvm::Value *raw_path =
              builder->CreateGlobalStringPtr(current_ctx_->log_file_path);
          path_ptr = CoerceTo(builder, raw_path, GetOpaquePtr());
          if (!path_ptr)
          {
            path_ptr = llvm::ConstantPointerNull::get(
                llvm::cast<llvm::PointerType>(GetOpaquePtr()));
          }
          path_len = llvm::ConstantInt::get(
              i64_ty, static_cast<uint64_t>(current_ctx_->log_file_path.size()));
        }

        llvm::FunctionType *set_sink_ty = set_sink_fn->getFunctionType();
        std::vector<llvm::Value *> set_sink_args;
        set_sink_args.reserve(set_sink_ty->getNumParams());
        for (unsigned i = 0; i < set_sink_ty->getNumParams(); ++i)
        {
          llvm::Type *param_ty = set_sink_ty->getParamType(i);
          llvm::Value *arg = nullptr;
          if (i == 0)
          {
            arg = CoerceTo(builder, sink_kind, param_ty);
          }
          else if (i == 1)
          {
            arg = CoerceTo(builder, path_ptr, param_ty);
          }
          else if (i == 2)
          {
            arg = CoerceTo(builder, path_len, param_ty);
          }
          if (!arg)
          {
            current_ctx_->ReportCodegenFailure();
            return;
          }
          set_sink_args.push_back(arg);
        }

        llvm::CallInst *set_sink_call =
            builder->CreateCall(set_sink_ty, set_sink_fn, set_sink_args);
        set_sink_call->setCallingConv(set_sink_fn->getCallingConv());
      }

      if (!current_ctx_->trace_root.empty())
      {
        const std::string set_root_sym = RuntimeConformanceSetRootSym();
        llvm::Function *set_root_fn = module_->getFunction(set_root_sym);
        if (!set_root_fn)
        {
          llvm::FunctionType *set_root_ty = llvm::FunctionType::get(
              void_ty, {GetOpaquePtr(), i64_ty}, false);
          set_root_fn = llvm::Function::Create(set_root_ty,
                                               llvm::GlobalValue::ExternalLinkage,
                                               set_root_sym,
                                               module_.get());
        }
        if (set_root_fn)
        {
          llvm::Value *raw_root =
              builder->CreateGlobalStringPtr(current_ctx_->trace_root);
          llvm::Value *root_ptr = CoerceTo(builder, raw_root, GetOpaquePtr());
          if (!root_ptr)
          {
            root_ptr = llvm::ConstantPointerNull::get(
                llvm::cast<llvm::PointerType>(GetOpaquePtr()));
          }
          llvm::Value *root_len = llvm::ConstantInt::get(
              i64_ty, static_cast<uint64_t>(current_ctx_->trace_root.size()));
          llvm::FunctionType *set_root_ty = set_root_fn->getFunctionType();
          llvm::Value *root_arg0 = CoerceTo(builder, root_ptr, set_root_ty->getParamType(0));
          llvm::Value *root_arg1 = CoerceTo(builder, root_len, set_root_ty->getParamType(1));
          if (!root_arg0)
          {
            root_arg0 = llvm::Constant::getNullValue(set_root_ty->getParamType(0));
          }
          if (!root_arg1)
          {
            root_arg1 = llvm::Constant::getNullValue(set_root_ty->getParamType(1));
          }
          llvm::CallInst *set_root_call =
              builder->CreateCall(set_root_ty, set_root_fn, {root_arg0, root_arg1});
          set_root_call->setCallingConv(set_root_fn->getCallingConv());
        }
      }

      // Configure runtime category filtering as an explicit bitmask:
      // bit0=log, bit1=diagnostic, bit2=runtime.
      const std::string log_filter_sym = RuntimeConformanceSetLogFilterSym();
      llvm::Function *log_filter_fn = module_->getFunction(log_filter_sym);
      if (!log_filter_fn)
      {
        llvm::FunctionType *log_filter_ty = llvm::FunctionType::get(
            void_ty, {i8_ty}, false);
        log_filter_fn = llvm::Function::Create(log_filter_ty,
                                               llvm::GlobalValue::ExternalLinkage,
                                               log_filter_sym,
                                               module_.get());
      }
      if (log_filter_fn)
      {
        std::uint8_t filter_mode = current_ctx_->trace ? 0x7u : 0x1u;
        if (current_ctx_->trace_filter_mask.has_value())
        {
          filter_mode = *current_ctx_->trace_filter_mask;
        }
        llvm::Value *enabled = llvm::ConstantInt::get(i8_ty, filter_mode);
        llvm::FunctionType *log_filter_ty = log_filter_fn->getFunctionType();
        llvm::Value *arg = CoerceTo(builder, enabled, log_filter_ty->getParamType(0));
        if (!arg)
        {
          arg = llvm::Constant::getNullValue(log_filter_ty->getParamType(0));
        }
        llvm::CallInst *log_filter_call =
            builder->CreateCall(log_filter_ty, log_filter_fn, {arg});
        log_filter_call->setCallingConv(log_filter_fn->getCallingConv());
      }

      if (current_ctx_->trace_min_level.has_value())
      {
        const std::string min_level_sym = RuntimeConformanceSetMinLevelSym();
        llvm::Function *min_level_fn = module_->getFunction(min_level_sym);
        if (!min_level_fn)
        {
          llvm::FunctionType *min_level_ty = llvm::FunctionType::get(
              void_ty, {i8_ty}, false);
          min_level_fn = llvm::Function::Create(min_level_ty,
                                                llvm::GlobalValue::ExternalLinkage,
                                                min_level_sym,
                                                module_.get());
        }
        if (min_level_fn)
        {
          llvm::Value *level = llvm::ConstantInt::get(i8_ty, *current_ctx_->trace_min_level);
          llvm::FunctionType *min_level_ty = min_level_fn->getFunctionType();
          llvm::Value *arg = CoerceTo(builder, level, min_level_ty->getParamType(0));
          if (!arg)
          {
            arg = llvm::Constant::getNullValue(min_level_ty->getParamType(0));
          }
          llvm::CallInst *min_level_call =
              builder->CreateCall(min_level_ty, min_level_fn, {arg});
          min_level_call->setCallingConv(min_level_fn->getCallingConv());
        }
      }
    }

    // Call the Cursive main function
    llvm::Function *cursive_main = nullptr;
    if (const auto it = functions_.find(*current_ctx_->main_symbol);
        it != functions_.end())
    {
      cursive_main = it->second;
    }
    if (!cursive_main)
    {
      if (returns_exit_code)
      {
        builder->CreateRet(
            llvm::ConstantInt::get(llvm::Type::getInt32Ty(context_), 1));
      }
      else
      {
        llvm::Function *runtime_exit_fn = module_->getFunction(BuiltinSymSystemExit());
        if (!runtime_exit_fn)
        {
          llvm::FunctionType *exit_ty =
              llvm::FunctionType::get(llvm::Type::getVoidTy(context_),
                                      {llvm::Type::getInt32Ty(context_)},
                                      false);
          runtime_exit_fn = llvm::Function::Create(
              exit_ty,
              llvm::GlobalValue::ExternalLinkage,
              BuiltinSymSystemExit(),
              module_.get());
          runtime_exit_fn->setCallingConv(llvm::CallingConv::C);
        }
        builder->CreateCall(
            runtime_exit_fn->getFunctionType(),
            runtime_exit_fn,
            {llvm::ConstantInt::get(llvm::Type::getInt32Ty(context_), 1)});
        builder->CreateUnreachable();
      }
      return;
    }

    llvm::Type *i8_ty = llvm::Type::getInt8Ty(context_);
    llvm::Type *i32_ty = llvm::Type::getInt32Ty(context_);
    llvm::Type *i64_ty = llvm::Type::getInt64Ty(context_);
    llvm::Type *i8_ptr_ty = llvm::PointerType::get(i8_ty, 0);

    llvm::Type *panic_storage_ty = GetLLVMType(PanicRecordType());
    if (!panic_storage_ty || panic_storage_ty->isVoidTy())
    {
      panic_storage_ty = llvm::ArrayType::get(i8_ty, 8);
    }
    llvm::AllocaInst *panic_storage =
        builder->CreateAlloca(panic_storage_ty, nullptr, "entry_panic");
    builder->CreateStore(llvm::Constant::getNullValue(panic_storage_ty), panic_storage);

    llvm::Value *panic_ptr = CoerceTo(builder, panic_storage, GetOpaquePtr());
    if (!panic_ptr)
    {
      panic_ptr = builder->CreatePointerCast(panic_storage, GetOpaquePtr());
    }

    std::uint64_t panic_flag_offset = 0;
    std::uint64_t panic_code_offset = 4;
    {
      const analysis::ScopeContext &scope = BuildScope(current_ctx_);
      const auto layout = ::cursive::analysis::layout::RecordLayoutOf(scope, {
                                                    analysis::MakeTypePrim("bool"),
                                                    analysis::MakeTypePrim("u32"),
                                                });
      if (layout.has_value() && layout->offsets.size() >= 2)
      {
        panic_flag_offset = layout->offsets[0];
        panic_code_offset = layout->offsets[1];
      }
    }

    auto load_panic_flag = [&]() -> llvm::Value *
    {
      return LoadPanicFlag(*this, builder, panic_ptr);
    };

    auto load_panic_code = [&]() -> llvm::Value *
    {
      llvm::Value *code = LoadPanicCodeValue(*this, builder, panic_ptr);
      if (!code)
      {
        return llvm::ConstantInt::get(i32_ty, 1);
      }
      return code;
    };

    const std::string runtime_panic_sym = RuntimePanicSym();
    llvm::Function *runtime_panic_fn = module_->getFunction(runtime_panic_sym);
    if (!runtime_panic_fn)
    {
      llvm::FunctionType *panic_ty =
          llvm::FunctionType::get(llvm::Type::getVoidTy(context_), {i32_ty}, false);
      runtime_panic_fn = llvm::Function::Create(
          panic_ty,
          llvm::GlobalValue::ExternalLinkage,
          runtime_panic_sym,
          module_.get());
      runtime_panic_fn->setCallingConv(llvm::CallingConv::C);
    }

    std::vector<llvm::Value *> call_args;
    llvm::FunctionType *callee_ty = cursive_main->getFunctionType();
    call_args.reserve(callee_ty->getNumParams());
    for (unsigned i = 0; i < callee_ty->getNumParams(); ++i)
    {
      llvm::Type *param_ty = callee_ty->getParamType(i);
      call_args.push_back(llvm::Constant::getNullValue(param_ty));
    }

    analysis::TypeRef ctx_type = analysis::MakeTypePath({"Context"});
    llvm::Type *ctx_storage_ty = GetLLVMType(ctx_type);
    if (!ctx_storage_ty || ctx_storage_ty->isVoidTy())
    {
      ctx_storage_ty = llvm::ArrayType::get(i8_ty, 64);
    }
    llvm::AllocaInst *ctx_storage =
        builder->CreateAlloca(ctx_storage_ty, nullptr, "entry_ctx");
    builder->CreateStore(llvm::Constant::getNullValue(ctx_storage_ty), ctx_storage);

    const std::string context_init_sym = ContextInitSym();
    bool context_initialized = false;
    if (std::optional<RuntimeFuncInfo> init_info = GetRuntimeFuncInfo(context_init_sym))
    {
      const bool use_c_abi_aggregate_sret = true;
      ABICallResult init_abi =
          ComputeCallABI(init_info->params, init_info->ret, use_c_abi_aggregate_sret);
      if (!init_abi.valid || !init_abi.func_type)
      {
        current_ctx_->ReportCodegenFailure();
        return;
      }
      llvm::FunctionType *init_ty = init_abi.func_type;
      llvm::Function *context_init_fn = module_->getFunction(context_init_sym);
      if (!context_init_fn)
      {
        context_init_fn = llvm::Function::Create(
            init_ty,
            llvm::GlobalValue::ExternalLinkage,
            context_init_sym,
            module_.get());
      }

      if (context_init_fn)
      {
        std::vector<llvm::Value *> init_args;
        init_args.reserve(init_ty->getNumParams());
        for (unsigned i = 0; i < init_ty->getNumParams(); ++i)
        {
          llvm::Type *param_ty = init_ty->getParamType(i);
          llvm::Value *arg = nullptr;
          if (init_abi.has_sret && i == 0)
          {
            arg = CoerceTo(builder, ctx_storage, param_ty);
          }
          if (!arg)
          {
            current_ctx_->ReportCodegenFailure();
            return;
          }
          init_args.push_back(arg);
        }
        llvm::CallInst *init_call =
            builder->CreateCall(init_ty, context_init_fn, init_args);
        init_call->setCallingConv(context_init_fn->getCallingConv());
        context_initialized = true;

        if (!init_abi.has_sret && !init_call->getType()->isVoidTy())
        {
          llvm::Value *out_ptr = CoerceTo(
              builder,
              ctx_storage,
              llvm::PointerType::get(init_call->getType(), 0));
          if (out_ptr && out_ptr->getType()->isPointerTy())
          {
            builder->CreateStore(init_call, out_ptr);
          }
        }
      }
    }

    if (!context_initialized)
    {
      llvm::Function *context_init_fn = module_->getFunction(context_init_sym);
      if (!context_init_fn)
      {
        llvm::FunctionType *init_ty =
            llvm::FunctionType::get(llvm::Type::getVoidTy(context_),
                                    {GetOpaquePtr()},
                                    false);
        context_init_fn = llvm::Function::Create(
            init_ty,
            llvm::GlobalValue::ExternalLinkage,
            context_init_sym,
            module_.get());
      }
      if (context_init_fn)
      {
        llvm::FunctionType *init_ty = context_init_fn->getFunctionType();
        std::vector<llvm::Value *> init_args;
        init_args.reserve(init_ty->getNumParams());
        for (unsigned i = 0; i < init_ty->getNumParams(); ++i)
        {
          llvm::Type *param_ty = init_ty->getParamType(i);
          llvm::Value *arg = nullptr;
          if (i == 0)
          {
            arg = CoerceTo(builder, ctx_storage, param_ty);
          }
          if (!arg)
          {
            arg = llvm::Constant::getNullValue(param_ty);
          }
          init_args.push_back(arg);
        }
        llvm::CallInst *init_call =
            builder->CreateCall(init_ty, context_init_fn, init_args);
        init_call->setCallingConv(context_init_fn->getCallingConv());
      }
    }

    const LowerCtx::ProcSigInfo *main_sig =
        current_ctx_ ? current_ctx_->LookupProcSig(*current_ctx_->main_symbol) : nullptr;
    llvm::Value *root_ctx_value = nullptr;
    if (ctx_storage_ty && !ctx_storage_ty->isVoidTy())
    {
      root_ctx_value = builder->CreateLoad(ctx_storage_ty, ctx_storage);
    }

    const analysis::ScopeContext &entry_scope = BuildScope(current_ctx_);
    auto normalize_context_type =
        [&](auto &&self, analysis::TypeRef type, std::size_t depth) -> analysis::TypeRef {
      if (!type || depth > 16u)
      {
        return type;
      }
      analysis::TypeRef cur = analysis::StripPerm(type);
      if (!cur)
      {
        cur = type;
      }
      while (cur)
      {
        if (const auto *refine = std::get_if<analysis::TypeRefine>(&cur->node))
        {
          cur = analysis::StripPerm(refine->base);
          if (!cur)
          {
            cur = refine->base;
          }
          continue;
        }
        break;
      }
      if (const auto *path = cur ? std::get_if<analysis::TypePathType>(&cur->node) : nullptr)
      {
        if (path->generic_args.empty())
        {
          ast::Path syntax_path;
          for (const auto &comp : path->path)
          {
            syntax_path.push_back(comp);
          }
          const auto it = entry_scope.sigma.types.find(analysis::PathKeyOf(syntax_path));
          if (it != entry_scope.sigma.types.end())
          {
            if (const auto *alias = std::get_if<ast::TypeAliasDecl>(&it->second))
            {
              const auto lowered = analysis::LowerType(entry_scope, alias->type);
              if (lowered.ok && lowered.type)
              {
                return self(self, lowered.type, depth + 1u);
              }
            }
          }
        }
      }
      return cur;
    };

    auto entry_context_field_value =
        [&](llvm::IRBuilder<> &irb,
            llvm::Value *ctx_value,
            std::string_view field_name) -> llvm::Value * {
      struct ContextFieldInfo {
        const char *name;
        analysis::TypeRef type;
      };
      const std::array<ContextFieldInfo, 5> fields = {{
          {"fs", analysis::MakeTypeDynamic({"FileSystem"})},
          {"net", analysis::MakeTypeDynamic({"Network"})},
          {"heap", analysis::MakeTypeDynamic({"HeapAllocator"})},
          {"sys", analysis::MakeTypePath({"System"})},
          {"reactor", analysis::MakeTypeDynamic({"Reactor"})},
      }};
      std::size_t extract_index = 0u;
      for (const auto &field : fields)
      {
        const auto size = ::cursive::analysis::layout::SizeOf(entry_scope, field.type).value_or(0u);
        if (std::string_view(field.name) == field_name)
        {
          if (size == 0u)
          {
            llvm::Type *field_ty = GetLLVMType(field.type);
            return field_ty && !field_ty->isVoidTy()
                       ? llvm::Constant::getNullValue(field_ty)
                       : nullptr;
          }
          return ctx_value
                     ? irb.CreateExtractValue(
                           ctx_value, {static_cast<unsigned>(extract_index)})
                     : nullptr;
        }
        if (size != 0u)
        {
          ++extract_index;
        }
      }
      return nullptr;
    };

    auto build_entry_context_bundle =
        [&](auto &&self,
            llvm::IRBuilder<> &irb,
            analysis::TypeRef target_type,
            std::string_view field_name,
            llvm::Value *root_ctx_ptr,
            llvm::Value *root_ctx_loaded) -> llvm::Value * {
      analysis::TypeRef cur = normalize_context_type(normalize_context_type, target_type, 0u);
      if (!cur)
      {
        return nullptr;
      }

      if (const auto *dyn = std::get_if<analysis::TypeDynamic>(&cur->node))
      {
        if (field_name == "cpu" || field_name == "gpu" || field_name == "inline")
        {
          const analysis::TypeRef expected_context_type =
              analysis::MakeTypePath({"Context"});
          const analysis::TypeRef expected_domain_type =
              analysis::MakeTypeDynamic({"ExecutionDomain"});
          std::string runtime_sym =
              field_name == "cpu" ? BuiltinSymContextCpu()
              : field_name == "gpu" ? BuiltinSymContextGpu()
                                    : BuiltinSymContextInline();
          if (auto runtime_info = GetRuntimeFuncInfo(runtime_sym))
          {
            const auto ctx_eq =
                analysis::TypeEquiv(runtime_info->params.size() == 1u
                                        ? runtime_info->params[0].type
                                        : nullptr,
                                    expected_context_type);
            const auto ret_eq =
                analysis::TypeEquiv(runtime_info->ret, expected_domain_type);
            const auto target_eq = analysis::TypeEquiv(cur, expected_domain_type);
            if (runtime_info->params.size() != 1u || !ctx_eq.ok || !ctx_eq.equiv ||
                !ret_eq.ok || !ret_eq.equiv || !target_eq.ok || !target_eq.equiv)
            {
              current_ctx_->ReportCodegenFailure();
              return nullptr;
            }

            ABICallResult abi =
                ComputeCallABI(runtime_info->params, runtime_info->ret, true);
            if (!abi.valid || !abi.func_type || abi.param_kinds.size() != 1u)
            {
              current_ctx_->ReportCodegenFailure();
              return nullptr;
            }
            llvm::Function *fn = module_->getFunction(runtime_sym);
            if (!fn)
            {
              fn = llvm::Function::Create(
                  abi.func_type,
                  llvm::GlobalValue::ExternalLinkage,
                  runtime_sym,
                  module_.get());
              fn->setCallingConv(llvm::CallingConv::C);
            }

            llvm::Value *context_arg =
                abi.param_kinds[0] == PassKind::ByRef ? root_ctx_ptr : root_ctx_loaded;
            if (!context_arg)
            {
              current_ctx_->ReportCodegenFailure();
              return nullptr;
            }

            return EmitABICall(
                *this,
                &irb,
                fn,
                runtime_info->params,
                runtime_info->ret,
                {context_arg},
                true);
          }
          current_ctx_->ReportCodegenFailure();
          return nullptr;
        }
        return entry_context_field_value(irb, root_ctx_loaded, field_name);
      }

      if (const auto *path = std::get_if<analysis::TypePathType>(&cur->node))
      {
        if (path->generic_args.empty() && path->path.size() == 1u &&
            path->path.front() == "System")
        {
          llvm::Type *target_ll = GetLLVMType(cur);
          return target_ll && !target_ll->isVoidTy()
                     ? llvm::Constant::getNullValue(target_ll)
                     : nullptr;
        }

        if (const ast::RecordDecl *record =
                analysis::LookupRecordDecl(entry_scope, path->path))
        {
          llvm::Type *target_ll = GetLLVMType(cur);
          if (!target_ll || target_ll->isVoidTy())
          {
            return nullptr;
          }
          llvm::Value *aggregate = llvm::Constant::getNullValue(target_ll);
          unsigned insert_index = 0u;
          for (const auto &member : record->members)
          {
            const auto *field = std::get_if<ast::FieldDecl>(&member);
            if (!field)
            {
              continue;
            }
            auto lowered = analysis::LowerType(entry_scope, field->type);
            if (!lowered.ok || !lowered.type)
            {
              continue;
            }
            llvm::Value *field_value = self(
                self, irb, lowered.type, field->name, root_ctx_ptr, root_ctx_loaded);
            const auto field_size = ::cursive::analysis::layout::SizeOf(entry_scope, lowered.type).value_or(0u);
            if (field_size == 0u)
            {
              continue;
            }
            if (!field_value)
            {
              llvm::Type *field_ty = GetLLVMType(lowered.type);
              if (!field_ty || field_ty->isVoidTy())
              {
                continue;
              }
              field_value = llvm::Constant::getNullValue(field_ty);
            }
            aggregate = irb.CreateInsertValue(aggregate, field_value, {insert_index++});
          }
          return aggregate;
        }
      }

      return entry_context_field_value(irb, root_ctx_loaded, field_name);
    };

    if (callee_ty->getNumParams() >= 1)
    {
      llvm::Type *param_ty = callee_ty->getParamType(0);
      llvm::Value *context_arg_value = nullptr;
      if (main_sig && !main_sig->params.empty() && main_sig->params[0].type)
      {
        context_arg_value = build_entry_context_bundle(
            build_entry_context_bundle,
            *builder,
            main_sig->params[0].type,
            "",
            ctx_storage,
            root_ctx_value);
      }
      if (!context_arg_value)
      {
        context_arg_value = root_ctx_value;
      }

      if (param_ty->isPointerTy())
      {
        llvm::Value *arg = nullptr;
        if (main_sig && !main_sig->params.empty() && main_sig->params[0].type)
        {
          analysis::TypeRef normalized_main_ctx =
              normalize_context_type(
                  normalize_context_type, main_sig->params[0].type, 0u);
          llvm::Type *main_ctx_ll = normalized_main_ctx
                                        ? GetLLVMType(normalized_main_ctx)
                                        : nullptr;
          if (main_ctx_ll && !main_ctx_ll->isVoidTy() &&
              normalized_main_ctx &&
              !analysis::TypeEquiv(
                   normalized_main_ctx, analysis::MakeTypePath({"Context"}))
                   .equiv)
          {
            llvm::AllocaInst *bundle_storage =
                builder->CreateAlloca(main_ctx_ll, nullptr, "entry_ctx_bundle");
            if (context_arg_value && context_arg_value->getType() == main_ctx_ll)
            {
              builder->CreateStore(context_arg_value, bundle_storage);
              arg = CoerceTo(builder, bundle_storage, param_ty);
            }
          }
        }
        if (!arg)
        {
          arg = CoerceTo(builder, ctx_storage, param_ty);
        }
        if (!arg)
        {
          current_ctx_->ReportCodegenFailure();
          return;
        }
        call_args[0] = arg;
      }
      else if (context_arg_value)
      {
        llvm::Value *arg = CoerceTo(builder, context_arg_value, param_ty);
        if (!arg && context_arg_value->getType() == param_ty)
        {
          arg = context_arg_value;
        }
        if (!arg)
        {
          current_ctx_->ReportCodegenFailure();
          return;
        }
        call_args[0] = arg;
      }
    }
    if (callee_ty->getNumParams() >= 2)
    {
      llvm::Type *param_ty = callee_ty->getParamType(1);
      if (param_ty->isPointerTy())
      {
        llvm::Value *arg = CoerceTo(builder, panic_ptr, param_ty);
        if (!arg)
        {
          current_ctx_->ReportCodegenFailure();
          return;
        }
        call_args[1] = arg;
      }
    }

    for (llvm::Value *arg : call_args)
    {
      if (!arg)
      {
        current_ctx_->ReportCodegenFailure();
        return;
      }
    }

    llvm::CallInst *call = builder->CreateCall(callee_ty, cursive_main, call_args);
    call->setCallingConv(cursive_main->getCallingConv());

    llvm::Value *exit_code = nullptr;
    if (call->getType()->isVoidTy())
    {
      exit_code = llvm::ConstantInt::get(i32_ty, 0);
    }
    else
    {
      exit_code = CoerceTo(builder, call, i32_ty);
      if (!exit_code)
      {
        exit_code = llvm::ConstantInt::get(i32_ty, 0);
      }
    }

    llvm::BasicBlock *panic_bb =
        llvm::BasicBlock::Create(context_, "entry.panic", main_fn);
    llvm::BasicBlock *deinit_bb =
        llvm::BasicBlock::Create(context_, "entry.deinit", main_fn);
    if (llvm::Value *has_panic = load_panic_flag())
    {
      builder->CreateCondBr(has_panic, panic_bb, deinit_bb);
    }
    else
    {
      builder->CreateBr(deinit_bb);
    }

    builder->SetInsertPoint(panic_bb);
    llvm::Value *panic_code = load_panic_code();
    panic_code = CoerceTo(builder, panic_code, i32_ty);
    if (!panic_code)
    {
      panic_code = llvm::ConstantInt::get(i32_ty, 1);
    }
    if (runtime_panic_fn)
    {
      llvm::FunctionType *panic_ty = runtime_panic_fn->getFunctionType();
      llvm::Value *panic_arg = CoerceTo(builder, panic_code, panic_ty->getParamType(0));
      if (!panic_arg)
      {
        panic_arg = llvm::Constant::getNullValue(panic_ty->getParamType(0));
      }
      llvm::CallInst *panic_call =
          builder->CreateCall(panic_ty, runtime_panic_fn, {panic_arg});
      panic_call->setCallingConv(runtime_panic_fn->getCallingConv());
      builder->CreateUnreachable();
    }
    else
    {
      llvm::Function *runtime_exit_fn = module_->getFunction(BuiltinSymSystemExit());
      if (!runtime_exit_fn)
      {
        llvm::FunctionType *exit_ty =
            llvm::FunctionType::get(llvm::Type::getVoidTy(context_),
                                    {i32_ty},
                                    false);
        runtime_exit_fn = llvm::Function::Create(
            exit_ty,
            llvm::GlobalValue::ExternalLinkage,
            BuiltinSymSystemExit(),
            module_.get());
        runtime_exit_fn->setCallingConv(llvm::CallingConv::C);
      }
      builder->CreateCall(runtime_exit_fn->getFunctionType(),
                          runtime_exit_fn,
                          {panic_code});
      builder->CreateUnreachable();
    }

    builder->SetInsertPoint(deinit_bb);
    std::vector<ast::ModulePath> init_order = ComputeEntryInitOrder(*current_ctx_);
    for (auto it = init_order.rbegin(); it != init_order.rend(); ++it)
    {
      const std::string deinit_sym = DeinitFn(*it);
      llvm::Function *deinit_fn = nullptr;
      if (const auto fn_it = functions_.find(deinit_sym); fn_it != functions_.end())
      {
        deinit_fn = fn_it->second;
      }
      else
      {
        deinit_fn = module_->getFunction(deinit_sym);
      }
      if (!deinit_fn)
      {
        llvm::FunctionType *deinit_ty = llvm::FunctionType::get(
            llvm::Type::getVoidTy(context_),
            {GetOpaquePtr()},
            false);
        deinit_fn = llvm::Function::Create(
            deinit_ty,
            llvm::GlobalValue::ExternalLinkage,
            deinit_sym,
            module_.get());
      }

      if (deinit_fn)
      {
        llvm::FunctionType *deinit_ty = deinit_fn->getFunctionType();
        std::vector<llvm::Value *> deinit_args;
        deinit_args.reserve(deinit_ty->getNumParams());
        for (unsigned i = 0; i < deinit_ty->getNumParams(); ++i)
        {
          llvm::Type *param_ty = deinit_ty->getParamType(i);
          llvm::Value *arg = nullptr;
          if (i == 0)
          {
            arg = CoerceTo(builder, panic_ptr, param_ty);
          }
          if (!arg)
          {
            arg = llvm::Constant::getNullValue(param_ty);
          }
          deinit_args.push_back(arg);
        }
        llvm::CallInst *deinit_call =
            builder->CreateCall(deinit_ty, deinit_fn, deinit_args);
        deinit_call->setCallingConv(deinit_fn->getCallingConv());
        HandleDeinitPanic(*this, builder, panic_ptr);
      }
      else if (current_ctx_)
      {
        current_ctx_->ReportCodegenFailure();
      }
    }

    RestoreDeinitPanicIfAny(*this, builder, panic_ptr);

    if (llvm::Value *has_panic = load_panic_flag())
    {
      llvm::BasicBlock *ret_bb =
          llvm::BasicBlock::Create(context_, "entry.ret", main_fn);
      builder->CreateCondBr(has_panic, panic_bb, ret_bb);
      builder->SetInsertPoint(ret_bb);
    }

    if (returns_exit_code)
    {
      builder->CreateRet(exit_code);
    }
    else
    {
      llvm::Function *runtime_exit_fn = module_->getFunction(BuiltinSymSystemExit());
      if (!runtime_exit_fn)
      {
        llvm::FunctionType *exit_ty =
            llvm::FunctionType::get(llvm::Type::getVoidTy(context_),
                                    {i32_ty},
                                    false);
        runtime_exit_fn = llvm::Function::Create(
            exit_ty,
            llvm::GlobalValue::ExternalLinkage,
            BuiltinSymSystemExit(),
            module_.get());
        runtime_exit_fn->setCallingConv(llvm::CallingConv::C);
      }
      builder->CreateCall(runtime_exit_fn->getFunctionType(),
                          runtime_exit_fn,
                          {exit_code});
      builder->CreateUnreachable();
    }
  }

  void LLVMEmitter::EmitLibraryEntryPoint()
  {
    if (!current_ctx_ || !current_ctx_->shared_library_project)
    {
      return;
    }
    if (!IsProjectEntryModule(current_ctx_))
    {
      return;
    }

    auto *i1_ty = llvm::Type::getInt1Ty(context_);
    auto *i8_ty = llvm::Type::getInt8Ty(context_);
    auto *i32_ty = llvm::Type::getInt32Ty(context_);
    auto *i64_ty = llvm::Type::getInt64Ty(context_);
    llvm::Type *opaque_ptr_ty = GetOpaquePtr();
    llvm::Type *panic_record_ty = GetLLVMType(PanicRecordType());
    if (!opaque_ptr_ty || !panic_record_ty)
    {
      if (current_ctx_)
      {
        current_ctx_->ReportCodegenFailure();
      }
      return;
    }

    llvm::Function *entry_fn = module_->getFunction(kLibraryEntrySym);
    if (!entry_fn)
    {
      llvm::FunctionType *entry_ty = llvm::FunctionType::get(
          i32_ty, {opaque_ptr_ty, i32_ty, opaque_ptr_ty}, false);
      entry_fn = llvm::Function::Create(entry_ty,
                                        llvm::GlobalValue::ExternalLinkage,
                                        kLibraryEntrySym,
                                        module_.get());
      entry_fn->setCallingConv(llvm::CallingConv::C);
    }
    if (!entry_fn->empty())
    {
      return;
    }

    llvm::GlobalVariable *attached_gv = module_->getNamedGlobal("__cursive_library_attached");
    if (!attached_gv)
    {
      attached_gv = new llvm::GlobalVariable(*module_,
                                             i1_ty,
                                             false,
                                             llvm::GlobalValue::InternalLinkage,
                                             llvm::ConstantInt::getFalse(context_),
                                             "__cursive_library_attached");
    }

    llvm::BasicBlock *entry_bb = llvm::BasicBlock::Create(context_, "entry", entry_fn);
    llvm::BasicBlock *attach_bb =
        llvm::BasicBlock::Create(context_, "dll.attach", entry_fn);
    llvm::BasicBlock *detach_bb =
        llvm::BasicBlock::Create(context_, "dll.detach", entry_fn);
    llvm::BasicBlock *other_bb =
        llvm::BasicBlock::Create(context_, "dll.other", entry_fn);
    auto *builder = static_cast<llvm::IRBuilder<> *>(builder_.get());
    builder->SetInsertPoint(entry_bb);

    llvm::Argument *reason_arg = entry_fn->getArg(1);
    reason_arg->setName("fdwReason");

    llvm::SwitchInst *dispatch =
        builder->CreateSwitch(reason_arg, other_bb, 2);
    dispatch->addCase(llvm::ConstantInt::get(i32_ty, 1), attach_bb);
    dispatch->addCase(llvm::ConstantInt::get(i32_ty, 0), detach_bb);

    auto panic_offsets = [&]() {
      std::uint64_t flag_offset = 0;
      std::uint64_t code_offset = 4;
      const analysis::ScopeContext &scope = BuildScope(current_ctx_);
      const auto layout = ::cursive::analysis::layout::RecordLayoutOf(
          scope,
          {analysis::MakeTypePrim("bool"), analysis::MakeTypePrim("u32")});
      if (layout.has_value() && layout->offsets.size() >= 2)
      {
        flag_offset = layout->offsets[0];
        code_offset = layout->offsets[1];
      }
      return std::pair<std::uint64_t, std::uint64_t>{flag_offset, code_offset};
    }();

    auto panic_field_ptr = [&](llvm::IRBuilder<> &irb,
                               llvm::Value *panic_ptr,
                               std::uint64_t offset,
                               llvm::Type *field_ty) -> llvm::Value * {
      llvm::Value *panic_i8 =
          CoerceTo(&irb, panic_ptr, llvm::PointerType::get(i8_ty, 0));
      if (!panic_i8)
      {
        panic_i8 =
            irb.CreateBitCast(panic_ptr, llvm::PointerType::get(i8_ty, 0));
      }
      llvm::Value *field_i8 = panic_i8;
      if (offset != 0u)
      {
        field_i8 = irb.CreateGEP(
            i8_ty, panic_i8, llvm::ConstantInt::get(i64_ty, offset));
      }
      return irb.CreateBitCast(field_i8, llvm::PointerType::get(field_ty, 0));
    };

    auto clear_panic_record = [&](llvm::IRBuilder<> &irb, llvm::Value *panic_ptr) {
      irb.CreateStore(llvm::ConstantInt::get(i8_ty, 0),
                      panic_field_ptr(irb, panic_ptr, panic_offsets.first, i8_ty));
      irb.CreateStore(llvm::ConstantInt::get(i32_ty, 0),
                      panic_field_ptr(irb, panic_ptr, panic_offsets.second, i32_ty));
    };

    auto load_panic_flag = [&](llvm::IRBuilder<> &irb,
                               llvm::Value *panic_ptr) -> llvm::Value * {
      return irb.CreateICmpNE(
          irb.CreateLoad(
              i8_ty,
              panic_field_ptr(irb, panic_ptr, panic_offsets.first, i8_ty)),
          llvm::ConstantInt::get(i8_ty, 0));
    };

    auto ensure_proc_fn = [&](const std::string &sym) -> llvm::Function * {
      const LowerCtx::ProcSigInfo *sig =
          current_ctx_ ? current_ctx_->LookupProcSig(sym) : nullptr;
      if (!sig)
      {
        if (current_ctx_)
        {
          current_ctx_->ReportCodegenFailure();
        }
        return nullptr;
      }
      ABICallResult abi = ComputeCallABI(sig->params, sig->ret);
      if (!abi.valid || !abi.func_type)
      {
        if (current_ctx_)
        {
          current_ctx_->ReportCodegenFailure();
        }
        return nullptr;
      }
      llvm::Function *fn = module_->getFunction(sym);
      if (!fn)
      {
        fn = llvm::Function::Create(abi.func_type,
                                    llvm::GlobalValue::ExternalLinkage,
                                    sym,
                                    module_.get());
      }
      fn->setCallingConv(CallingConvForAbi(sig->abi));
      return fn;
    };

    auto call_proc_with_panic = [&](llvm::IRBuilder<> &irb,
                                    const std::string &sym,
                                    llvm::Value *panic_ptr) {
      const LowerCtx::ProcSigInfo *sig =
          current_ctx_ ? current_ctx_->LookupProcSig(sym) : nullptr;
      llvm::Function *fn = ensure_proc_fn(sym);
      if (!sig || !fn)
      {
        return;
      }
      ABICallResult abi = ComputeCallABI(sig->params, sig->ret);
      std::vector<llvm::Value *> args;
      args.reserve(fn->arg_size());
      for (unsigned i = 0; i < fn->arg_size(); ++i)
      {
        llvm::Value *arg = nullptr;
        if (!abi.param_indices.empty() && abi.param_indices.back().has_value() &&
            *abi.param_indices.back() == i)
        {
          arg = CoerceTo(&irb, panic_ptr, fn->getFunctionType()->getParamType(i));
          if (!arg && panic_ptr &&
              panic_ptr->getType() == fn->getFunctionType()->getParamType(i))
          {
            arg = panic_ptr;
          }
        }
        if (!arg)
        {
          if (current_ctx_)
          {
            current_ctx_->ReportCodegenFailure();
          }
          return;
        }
        args.push_back(arg);
      }
      llvm::CallInst *call =
          irb.CreateCall(fn->getFunctionType(), fn, args);
      call->setCallingConv(fn->getCallingConv());
    };

    builder->SetInsertPoint(attach_bb);
    llvm::LoadInst *attached_before = builder->CreateLoad(i1_ty, attached_gv);
    llvm::BasicBlock *attach_work_bb =
        llvm::BasicBlock::Create(context_, "dll.attach.work", entry_fn);
    llvm::BasicBlock *attach_done_bb =
        llvm::BasicBlock::Create(context_, "dll.attach.done", entry_fn);
    builder->CreateCondBr(attached_before, attach_done_bb, attach_work_bb);

    builder->SetInsertPoint(attach_done_bb);
    builder->CreateRet(llvm::ConstantInt::get(i32_ty, 1));

    builder->SetInsertPoint(attach_work_bb);
    llvm::Value *panic_record_ptr = GetSharedLibraryImagePanicPtr();
    if (!panic_record_ptr)
    {
      if (current_ctx_)
      {
        current_ctx_->ReportCodegenFailure();
      }
      builder->CreateRet(llvm::ConstantInt::get(i32_ty, 0));
      return;
    }
    clear_panic_record(*builder, panic_record_ptr);

    for (std::size_t module_index = 0;
         module_index < current_ctx_->init_order.size();
         ++module_index)
    {
      const auto &module_path = current_ctx_->init_order[module_index];
      call_proc_with_panic(*builder, InitFn(module_path), panic_record_ptr);
      llvm::BasicBlock *cont_bb =
          llvm::BasicBlock::Create(context_, "dll.attach.cont", entry_fn);
      llvm::BasicBlock *fail_bb =
          llvm::BasicBlock::Create(context_, "dll.attach.fail", entry_fn);
      builder->CreateCondBr(load_panic_flag(*builder, panic_record_ptr),
                            fail_bb,
                            cont_bb);
      builder->SetInsertPoint(fail_bb);
      clear_panic_record(*builder, panic_record_ptr);
      for (std::size_t deinit_index = module_index; deinit_index > 0; --deinit_index)
      {
        const auto &deinit_path = current_ctx_->init_order[deinit_index - 1];
        call_proc_with_panic(*builder, DeinitFn(deinit_path), panic_record_ptr);
        clear_panic_record(*builder, panic_record_ptr);
      }
      builder->CreateStore(llvm::ConstantInt::getFalse(context_), attached_gv);
      builder->CreateRet(llvm::ConstantInt::get(i32_ty, 0));
      builder->SetInsertPoint(cont_bb);
    }

    builder->CreateStore(llvm::ConstantInt::getTrue(context_), attached_gv);
    builder->CreateRet(llvm::ConstantInt::get(i32_ty, 1));

    builder->SetInsertPoint(detach_bb);
    llvm::LoadInst *attached_now = builder->CreateLoad(i1_ty, attached_gv);
    llvm::BasicBlock *detach_work_bb =
        llvm::BasicBlock::Create(context_, "dll.detach.work", entry_fn);
    llvm::BasicBlock *detach_done_bb =
        llvm::BasicBlock::Create(context_, "dll.detach.done", entry_fn);
    builder->CreateCondBr(attached_now, detach_work_bb, detach_done_bb);

    builder->SetInsertPoint(detach_work_bb);
    clear_panic_record(*builder, panic_record_ptr);
    for (auto it = current_ctx_->init_order.rbegin();
         it != current_ctx_->init_order.rend();
         ++it)
    {
      const auto &module_path = *it;
      call_proc_with_panic(*builder, DeinitFn(module_path), panic_record_ptr);
      HandleDeinitPanic(*this, builder, panic_record_ptr);
    }
    RestoreDeinitPanicIfAny(*this, builder, panic_record_ptr);
    llvm::Value *detach_had_panic = load_panic_flag(*builder, panic_record_ptr);
    builder->CreateStore(llvm::ConstantInt::getFalse(context_), attached_gv);
    llvm::Value *detach_ok = builder->CreateSelect(
        detach_had_panic,
        llvm::ConstantInt::get(i32_ty, 0),
        llvm::ConstantInt::get(i32_ty, 1));
    builder->CreateRet(detach_ok);

    builder->SetInsertPoint(detach_done_bb);
    builder->CreateRet(llvm::ConstantInt::get(i32_ty, 1));

    builder->SetInsertPoint(other_bb);
    builder->CreateRet(llvm::ConstantInt::get(i32_ty, 1));
  }

  void LLVMEmitter::EmitPosixLibraryLifecycleHooks()
  {
    if (!current_ctx_ || !current_ctx_->shared_library_project ||
        project::ObjectFormatOf(target_profile_) != project::ObjectFormat::Elf ||
        !IsProjectEntryModule(current_ctx_))
    {
      return;
    }

    llvm::Function *entry_fn = module_->getFunction(kLibraryEntrySym);
    llvm::Type *opaque_ptr_ty = GetOpaquePtr();
    if (!entry_fn || !opaque_ptr_ty)
    {
      if (current_ctx_)
      {
        current_ctx_->ReportCodegenFailure();
      }
      return;
    }

    auto *i32_ty = llvm::Type::getInt32Ty(context_);
    auto *opaque_ptr_ptr_ty = llvm::cast<llvm::PointerType>(opaque_ptr_ty);
    auto ensure_runtime_panic = [&]() -> llvm::Function * {
      llvm::Function *fn = module_->getFunction(RuntimePanicSym());
      if (!fn)
      {
        llvm::FunctionType *fn_ty =
            llvm::FunctionType::get(llvm::Type::getVoidTy(context_),
                                    {i32_ty},
                                    false);
        fn = llvm::Function::Create(fn_ty,
                                    llvm::GlobalValue::ExternalLinkage,
                                    RuntimePanicSym(),
                                    module_.get());
        fn->setCallingConv(llvm::CallingConv::C);
      }
      return fn;
    };

    auto ensure_hook = [&](const char *name) -> llvm::Function * {
      llvm::Function *fn = module_->getFunction(name);
      if (!fn)
      {
        llvm::FunctionType *fn_ty =
            llvm::FunctionType::get(llvm::Type::getVoidTy(context_), {}, false);
        fn = llvm::Function::Create(fn_ty,
                                    llvm::GlobalValue::InternalLinkage,
                                    name,
                                    module_.get());
        fn->setCallingConv(llvm::CallingConv::C);
      }
      return fn;
    };

    llvm::Function *ctor_fn = ensure_hook(kLibraryCtorSym);
    if (ctor_fn && ctor_fn->empty())
    {
      llvm::BasicBlock *entry_bb =
          llvm::BasicBlock::Create(context_, "entry", ctor_fn);
      llvm::BasicBlock *ok_bb =
          llvm::BasicBlock::Create(context_, "ctor.ok", ctor_fn);
      llvm::BasicBlock *fail_bb =
          llvm::BasicBlock::Create(context_, "ctor.fail", ctor_fn);
      auto *builder = static_cast<llvm::IRBuilder<> *>(builder_.get());
      builder->SetInsertPoint(entry_bb);
      llvm::CallInst *attach_call =
          builder->CreateCall(entry_fn->getFunctionType(),
                              entry_fn,
                              {llvm::ConstantPointerNull::get(opaque_ptr_ptr_ty),
                               llvm::ConstantInt::get(i32_ty, 1),
                               llvm::ConstantPointerNull::get(opaque_ptr_ptr_ty)});
      attach_call->setCallingConv(entry_fn->getCallingConv());
      builder->CreateCondBr(
          builder->CreateICmpNE(attach_call, llvm::ConstantInt::get(i32_ty, 0)),
          ok_bb,
          fail_bb);

      builder->SetInsertPoint(fail_bb);
      llvm::Function *runtime_panic_fn = ensure_runtime_panic();
      builder->CreateCall(runtime_panic_fn,
                          {llvm::ConstantInt::get(
                              i32_ty,
                              PanicCode(PanicReason::ForeignPre))});
      builder->CreateUnreachable();

      builder->SetInsertPoint(ok_bb);
      builder->CreateRetVoid();
    }

    llvm::Function *dtor_fn = ensure_hook(kLibraryDtorSym);
    if (dtor_fn && dtor_fn->empty())
    {
      llvm::BasicBlock *entry_bb =
          llvm::BasicBlock::Create(context_, "entry", dtor_fn);
      llvm::BasicBlock *ok_bb =
          llvm::BasicBlock::Create(context_, "dtor.ok", dtor_fn);
      llvm::BasicBlock *fail_bb =
          llvm::BasicBlock::Create(context_, "dtor.fail", dtor_fn);
      auto *builder = static_cast<llvm::IRBuilder<> *>(builder_.get());
      builder->SetInsertPoint(entry_bb);
      llvm::CallInst *detach_call =
          builder->CreateCall(entry_fn->getFunctionType(),
                              entry_fn,
                              {llvm::ConstantPointerNull::get(opaque_ptr_ptr_ty),
                               llvm::ConstantInt::get(i32_ty, 0),
                               llvm::ConstantPointerNull::get(opaque_ptr_ptr_ty)});
      detach_call->setCallingConv(entry_fn->getCallingConv());
      builder->CreateCondBr(
          builder->CreateICmpNE(detach_call, llvm::ConstantInt::get(i32_ty, 0)),
          ok_bb,
          fail_bb);

      builder->SetInsertPoint(fail_bb);
      llvm::Function *runtime_panic_fn = ensure_runtime_panic();
      builder->CreateCall(runtime_panic_fn,
                          {llvm::ConstantInt::get(
                              i32_ty,
                              PanicCode(PanicReason::ForeignPost))});
      builder->CreateUnreachable();

      builder->SetInsertPoint(ok_bb);
      builder->CreateRetVoid();
    }

    if (ctor_fn)
    {
      llvm::appendToGlobalCtors(*module_, ctor_fn, 65535);
    }
    if (dtor_fn)
    {
      llvm::appendToGlobalDtors(*module_, dtor_fn, 65535);
    }
  }

  void LLVMEmitter::ApplySharedLibraryDefinitionVisibility()
  {
    if (!current_ctx_ || !current_ctx_->shared_library_project ||
        current_ctx_->shared_library_export_symbols.empty() ||
        project::ObjectFormatOf(target_profile_) != project::ObjectFormat::Elf)
    {
      return;
    }

    std::unordered_set<std::string> exported(
        current_ctx_->shared_library_export_symbols.begin(),
        current_ctx_->shared_library_export_symbols.end());

    auto apply_visibility = [&](llvm::GlobalValue &value) {
      if (value.isDeclaration() || value.hasLocalLinkage())
      {
        return;
      }
      if (exported.find(value.getName().str()) != exported.end())
      {
        value.setVisibility(llvm::GlobalValue::DefaultVisibility);
        return;
      }
      value.setVisibility(llvm::GlobalValue::HiddenVisibility);
    };

    for (auto &fn : *module_)
    {
      apply_visibility(fn);
    }
    for (auto &global : module_->globals())
    {
      apply_visibility(global);
    }
  }

  void LLVMEmitter::EmitPoisonCheck(const std::string &module_name)
  {
    SPEC_RULE("LowerIRDecl-PoisonCheck");
    auto *builder = static_cast<llvm::IRBuilder<> *>(builder_.get());
    if (!builder || !builder->GetInsertBlock() ||
        builder->GetInsertBlock()->getTerminator())
    {
      return;
    }

    const auto module_path = SplitModulePathString(module_name);
    llvm::Value *flag_ptr = GetPoisonFlagPtr(*this, module_path);
    if (!flag_ptr)
    {
      if (current_ctx_)
      {
        current_ctx_->ReportCodegenFailure();
      }
      return;
    }

    llvm::Type *bool_ty = GetLLVMType(analysis::MakeTypePrim("bool"));
    if (!bool_ty)
    {
      if (current_ctx_)
      {
        current_ctx_->ReportCodegenFailure();
      }
      return;
    }

    llvm::Value *poisoned = builder->CreateLoad(bool_ty, flag_ptr);
    llvm::Function *func = builder->GetInsertBlock()->getParent();
    llvm::BasicBlock *panic_bb =
        llvm::BasicBlock::Create(context_, "poison.take", func);
    llvm::BasicBlock *cont_bb =
        llvm::BasicBlock::Create(context_, "poison.cont", func);
    builder->CreateCondBr(AsBool(builder, poisoned), panic_bb, cont_bb);

    builder->SetInsertPoint(panic_bb);
    StorePanicRecord(*this, builder, PanicCode(PanicReason::InitPanic));
    if (current_ctx_)
    {
      CleanupPlan cleanup_plan = ComputeCleanupPlanToFunctionRoot(*current_ctx_);
      IRPtr cleanup_ir = EmitCleanupOnPanic(cleanup_plan, *current_ctx_);
      if (cleanup_ir)
      {
        EmitIR(cleanup_ir);
      }
    }
    if (!builder->GetInsertBlock()->getTerminator())
    {
      EmitReturn(*this, builder);
    }

    builder->SetInsertPoint(cont_bb);
  }


  // T-LLVM-009: Instruction emission
  void LLVMEmitter::EmitIR(const IRPtr &ir)
  {
    if (!ir)
    {
      return;
    }
    auto *builder = static_cast<llvm::IRBuilder<> *>(builder_.get());
    if (!builder || !builder->GetInsertBlock() ||
        builder->GetInsertBlock()->getTerminator())
    {
      return;
    }

    using Clock = std::chrono::steady_clock;
    const bool ir_perf_enabled = g_ir_proc_perf_ctx != nullptr;
    if (ir_perf_enabled)
    {
      IRNodePerfFrame frame;
      frame.kind_index = ir->node.index();
      frame.start = Clock::now();
      frame.child_ms = 0;
      g_ir_proc_perf_ctx->stack.push_back(frame);
    }

    struct Visitor
    {
      LLVMEmitter &emitter;
      llvm::IRBuilder<> &builder;

      llvm::Type *ExpectedLLVMType(const IRValue &value) const
      {
        analysis::TypeRef type = LookupValueType(value);
        if (!type)
        {
          return nullptr;
        }
        return emitter.GetLLVMType(type);
      }

      analysis::TypeRef LookupValueType(const IRValue &value) const
      {
        if (value.kind == IRValue::Kind::Local)
        {
          if (analysis::TypeRef local_type = emitter.LookupLocalType(value.name))
          {
            return local_type;
          }
        }

        const LowerCtx *ctx = emitter.GetCurrentCtx();
        if (!ctx)
        {
          return nullptr;
        }
        if (analysis::TypeRef type = ctx->LookupValueType(value))
        {
          return type;
        }
        if (value.kind == IRValue::Kind::Local)
        {
          if (const BindingState *state = ctx->GetBindingState(value.name))
          {
            return state->type;
          }
        }
        return nullptr;
      }

      llvm::Value *DefaultFor(const IRValue &value) const
      {
        if (llvm::Type *ty = ExpectedLLVMType(value))
        {
          return llvm::Constant::getNullValue(ty);
        }
        return llvm::ConstantInt::get(llvm::Type::getInt64Ty(emitter.GetContext()), 0);
      }

      llvm::Value *EvaluateOrDefault(const IRValue &value) const
      {
        llvm::Value *out = emitter.EvaluateIRValue(value);
        if (!out)
        {
          out = DefaultFor(value);
        }
        return out;
      }

      bool IsAddressBackedAggregateType(llvm::Type *ty) const
      {
        if (!ty)
        {
          return false;
        }
        if (ty->isArrayTy())
        {
          return true;
        }
        if (auto *struct_ty = llvm::dyn_cast<llvm::StructType>(ty))
        {
          return struct_ty->getNumElements() != 0;
        }
        return false;
      }

      llvm::Value *ForwardedAggregateStorage(const IRValue &value) const
      {
        if (value.kind != IRValue::Kind::Opaque)
        {
          return nullptr;
        }
        llvm::Type *result_ty = ExpectedLLVMType(value);
        if (!IsAddressBackedAggregateType(result_ty))
        {
          return nullptr;
        }
        llvm::Value *storage = emitter.GetAddressableStorage(value);
        if (!storage || !storage->getType()->isPointerTy())
        {
          return nullptr;
        }
        llvm::Type *expected_ptr_ty = llvm::PointerType::get(result_ty, 0);
        if (storage->getType() != expected_ptr_ty)
        {
          storage = builder.CreateBitCast(storage, expected_ptr_ty);
        }
        return storage;
      }

      void SetForwardedOrMaterializedResult(const IRValue &value) const
      {
        if (llvm::Value *storage = ForwardedAggregateStorage(value))
        {
          emitter.ForgetTempStorage(value);
          emitter.SetTempStorage(value, storage);
          return;
        }
        emitter.SetTempValue(value, EvaluateOrDefault(value));
      }

      std::optional<std::uint64_t> StaticRangeLength(const IRRange &range,
                                                     std::uint64_t base_len) const
      {
        auto bound_or = [&](const std::optional<IRValue> &bound,
                            std::uint64_t default_value) -> std::optional<std::uint64_t>
        {
          if (!bound.has_value())
          {
            return default_value;
          }
          return ImmediateU64(*bound);
        };

        switch (range.kind)
        {
        case IRRangeKind::Full:
          return base_len;
        case IRRangeKind::From:
        {
          auto lo = bound_or(range.lo, 0);
          if (!lo.has_value() || *lo > base_len)
          {
            return std::nullopt;
          }
          return base_len - *lo;
        }
        case IRRangeKind::To:
        {
          auto hi = bound_or(range.hi, base_len);
          if (!hi.has_value() || *hi > base_len)
          {
            return std::nullopt;
          }
          return *hi;
        }
        case IRRangeKind::ToInclusive:
        {
          auto hi = bound_or(range.hi, 0);
          if (!hi.has_value() || *hi >= base_len)
          {
            return std::nullopt;
          }
          return *hi + 1;
        }
        case IRRangeKind::Exclusive:
        {
          auto lo = bound_or(range.lo, 0);
          auto hi = bound_or(range.hi, base_len);
          if (!lo.has_value() || !hi.has_value() || *lo > *hi || *hi > base_len)
          {
            return std::nullopt;
          }
          return *hi - *lo;
        }
        case IRRangeKind::Inclusive:
        {
          auto lo = bound_or(range.lo, 0);
          auto hi = bound_or(range.hi, 0);
          if (!lo.has_value() || !hi.has_value() || *lo > *hi || *hi >= base_len)
          {
            return std::nullopt;
          }
          return (*hi - *lo) + 1;
        }
        }
        return std::nullopt;
      }

      std::optional<std::uint64_t> StaticLengthOf(const IRValue &value) const
      {
        const LowerCtx *ctx = emitter.GetCurrentCtx();
        if (!ctx)
        {
          return std::nullopt;
        }
        analysis::TypeRef type = analysis::StripPerm(ctx->LookupValueType(value));
        for (int depth = 0; type && depth < 4; ++depth)
        {
          if (auto *arr = std::get_if<analysis::TypeArray>(&type->node))
          {
            return arr->length;
          }
          if (auto *ptr = std::get_if<analysis::TypePtr>(&type->node))
          {
            type = analysis::StripPerm(ptr->element);
            continue;
          }
          if (auto *raw = std::get_if<analysis::TypeRawPtr>(&type->node))
          {
            type = analysis::StripPerm(raw->element);
            continue;
          }
          break;
        }

        if (const DerivedValueInfo *derived = ctx->LookupDerivedValue(value))
        {
          auto loop_range_trip_count =
              [&](const IRRange &range) -> std::optional<std::uint64_t>
          {
            auto bound_or = [&](const std::optional<IRValue> &bound,
                                std::uint64_t default_value)
                -> std::optional<std::uint64_t>
            {
              if (!bound.has_value())
              {
                return default_value;
              }
              return ImmediateU64(*bound);
            };
            switch (range.kind)
            {
            case IRRangeKind::Exclusive:
            {
              auto lo = bound_or(range.lo, 0);
              auto hi = bound_or(range.hi, 0);
              if (!lo.has_value() || !hi.has_value() || *hi < *lo)
              {
                return std::nullopt;
              }
              return *hi - *lo;
            }
            case IRRangeKind::Inclusive:
            {
              auto lo = bound_or(range.lo, 0);
              auto hi = bound_or(range.hi, 0);
              if (!lo.has_value() || !hi.has_value() || *hi < *lo)
              {
                return std::nullopt;
              }
              return (*hi - *lo) + 1;
            }
            case IRRangeKind::To:
            {
              auto hi = bound_or(range.hi, 0);
              if (!hi.has_value())
              {
                return std::nullopt;
              }
              return *hi;
            }
            case IRRangeKind::ToInclusive:
            {
              auto hi = bound_or(range.hi, 0);
              if (!hi.has_value())
              {
                return std::nullopt;
              }
              return *hi + 1;
            }
            case IRRangeKind::From:
            case IRRangeKind::Full:
              return std::nullopt;
            }
            return std::nullopt;
          };
          switch (derived->kind)
          {
          case DerivedValueInfo::Kind::ArrayLit:
            return static_cast<std::uint64_t>(derived->elements.size());
          case DerivedValueInfo::Kind::ArrayRepeat:
          {
            llvm::Value *count_value = emitter.EvaluateIRValue(derived->repeat_count);
            auto *count_int = llvm::dyn_cast_or_null<llvm::ConstantInt>(count_value);
            if (!count_int)
            {
              return std::nullopt;
            }
            return count_int->getZExtValue();
          }
          case DerivedValueInfo::Kind::ArraySegments:
          {
            std::uint64_t total = 0;
            for (const auto &segment : derived->array_segments)
            {
              if (segment.kind == DerivedArraySegment::Kind::Element)
              {
                total += 1;
                continue;
              }
              if (!segment.count.has_value())
              {
                return std::nullopt;
              }
              llvm::Value *count_value = emitter.EvaluateIRValue(*segment.count);
              auto *count_int = llvm::dyn_cast_or_null<llvm::ConstantInt>(count_value);
              if (!count_int)
              {
                return std::nullopt;
              }
              total += count_int->getZExtValue();
            }
            return total;
          }
          case DerivedValueInfo::Kind::Slice:
          {
            auto base_len = StaticLengthOf(derived->base);
            if (!base_len.has_value())
            {
              return std::nullopt;
            }
            if (derived->range_value.has_value())
            {
              if (const DerivedValueInfo *range_derived =
                      ctx->LookupDerivedValue(*derived->range_value))
              {
                if (range_derived->kind == DerivedValueInfo::Kind::RangeLit)
                {
                  return StaticRangeLength(range_derived->range, *base_len);
                }
              }
              return std::nullopt;
            }
            return StaticRangeLength(derived->range, *base_len);
          }
          case DerivedValueInfo::Kind::RangeLit:
            return loop_range_trip_count(derived->range);
          default:
            break;
          }
        }
        return std::nullopt;
      }

      analysis::TypeRef NormalizeValueType(const IRValue &value) const
      {
        const LowerCtx *ctx = emitter.GetCurrentCtx();
        if (!ctx)
        {
          return nullptr;
        }

        analysis::TypeRef type = LookupValueType(value);
        if (!type)
        {
          return nullptr;
        }

        const analysis::ScopeContext &scope = BuildScope(ctx);
        for (int depth = 0; type && depth < 4; ++depth)
        {
          if (analysis::TypeRef stripped = analysis::StripPerm(type))
          {
            type = stripped;
          }
          if (analysis::TypeRef resolved = ResolveAliasTypeInScope(scope, type))
          {
            type = resolved;
            if (analysis::TypeRef stripped = analysis::StripPerm(type))
            {
              type = stripped;
            }
          }
          if (const auto *ptr = std::get_if<analysis::TypePtr>(&type->node))
          {
            type = ptr->element;
            continue;
          }
          if (const auto *raw = std::get_if<analysis::TypeRawPtr>(&type->node))
          {
            type = raw->element;
            continue;
          }
          break;
        }

        if (analysis::TypeRef stripped = analysis::StripPerm(type))
        {
          return stripped;
        }
        return type;
      }

      bool IsDynamicSequenceType(const analysis::TypeRef &type) const
      {
        if (!type)
        {
          return false;
        }
        if (std::holds_alternative<analysis::TypeSlice>(type->node))
        {
          return true;
        }
        if (const auto *str = std::get_if<analysis::TypeString>(&type->node))
        {
          return str->state.has_value() &&
                 *str->state == analysis::StringState::View;
        }
        if (const auto *bytes = std::get_if<analysis::TypeBytes>(&type->node))
        {
          return bytes->state.has_value() &&
                 *bytes->state == analysis::BytesState::View;
        }
        return false;
      }

      llvm::Value *DynamicLengthOf(const IRValue &value) const
      {
        llvm::Value *runtime_value = EvaluateOrDefault(value);
        if (!runtime_value)
        {
          return nullptr;
        }

        if (auto *struct_ty = llvm::dyn_cast<llvm::StructType>(runtime_value->getType()))
        {
          if (struct_ty->getNumElements() >= 2 &&
              struct_ty->getElementType(1)->isIntegerTy())
          {
            return builder.CreateExtractValue(runtime_value, {1u});
          }
        }

        analysis::TypeRef normalized_type = NormalizeValueType(value);
        if (!IsDynamicSequenceType(normalized_type))
        {
          return nullptr;
        }

        if (!runtime_value->getType()->isPointerTy())
        {
          return nullptr;
        }
        return EmitSliceLenFromAddr(emitter, builder, normalized_type, runtime_value);
      }

      std::optional<std::uint64_t> ImmediateU64(const IRValue &value) const
      {
        if (value.kind == IRValue::Kind::Immediate)
        {
          std::uint64_t out = 0;
          for (std::size_t i = 0; i < value.bytes.size() && i < 8; ++i)
          {
            out |= static_cast<std::uint64_t>(value.bytes[i]) << (8 * i);
          }
          return out;
        }
        return std::nullopt;
      }

      bool IsSignedIntegerType(const analysis::TypeRef &type) const
      {
        analysis::TypeRef stripped = analysis::StripPerm(type);
        if (!stripped)
        {
          return false;
        }
        auto *prim = std::get_if<analysis::TypePrim>(&stripped->node);
        if (!prim)
        {
          return false;
        }
        const std::string &name = prim->name;
        if (name == "isize")
        {
          return true;
        }
        return !name.empty() && name[0] == 'i' && name != "i1";
      }

      bool IsCharType(const analysis::TypeRef &type) const
      {
        analysis::TypeRef stripped = analysis::StripPerm(type);
        if (!stripped)
        {
          return false;
        }
        const auto *prim = std::get_if<analysis::TypePrim>(&stripped->node);
        return prim && prim->name == "char";
      }

      llvm::Value *EmitBuiltinEqCall(llvm::IRBuilder<> &builder,
                                     const analysis::TypeRef &type,
                                     llvm::Value *lhs,
                                     llvm::Value *rhs) const
      {
        if (!lhs || !rhs || !analysis::EqType(type))
        {
          return nullptr;
        }
        return EmitTypedEq(&builder, lhs, rhs);
      }

      struct BuiltinSuccessorResult
      {
        llvm::Value *has_next = nullptr;
        llvm::Value *next = nullptr;
      };

      std::optional<BuiltinSuccessorResult> EmitBuiltinSuccessor(
          llvm::IRBuilder<> &builder,
          const analysis::TypeRef &type,
          llvm::Value *value) const
      {
        if (!value || !value->getType()->isIntegerTy() ||
            !analysis::BuiltinStepType(type))
        {
          return std::nullopt;
        }

        if (IsCharType(type))
        {
          llvm::Constant *max_scalar =
              llvm::ConstantInt::get(value->getType(), 0x10FFFFu);
          llvm::Constant *one = llvm::ConstantInt::get(value->getType(), 1u);
          llvm::Constant *surrogate_start =
              llvm::ConstantInt::get(value->getType(), 0xD800u);
          llvm::Constant *surrogate_end =
              llvm::ConstantInt::get(value->getType(), 0xE000u);
          llvm::Value *has_next = builder.CreateICmpNE(value, max_scalar);
          llvm::Value *plus_one = builder.CreateAdd(value, one);
          llvm::Value *is_surrogate_start =
              builder.CreateICmpEQ(plus_one, surrogate_start);
          llvm::Value *next =
              builder.CreateSelect(is_surrogate_start, surrogate_end, plus_one);
          return BuiltinSuccessorResult{
              .has_next = has_next,
              .next = next,
          };
        }

        const unsigned width = value->getType()->getIntegerBitWidth();
        llvm::Constant *max_value = nullptr;
        if (IsSignedIntegerType(type))
        {
          max_value = llvm::ConstantInt::get(
              value->getType(), llvm::APInt::getSignedMaxValue(width));
        }
        else
        {
          max_value = llvm::ConstantInt::get(
              value->getType(), llvm::APInt::getAllOnes(width));
        }
        llvm::Value *has_next = builder.CreateICmpNE(value, max_value);
        llvm::Value *next =
            builder.CreateAdd(value, llvm::ConstantInt::get(value->getType(), 1u));
        return BuiltinSuccessorResult{
            .has_next = has_next,
            .next = next,
        };
      }

      std::optional<BuiltinSuccessorResult> EmitBuiltinPredecessor(
          llvm::IRBuilder<> &builder,
          const analysis::TypeRef &type,
          llvm::Value *value) const
      {
        if (!value || !value->getType()->isIntegerTy() ||
            !analysis::BuiltinStepType(type))
        {
          return std::nullopt;
        }

        if (IsCharType(type))
        {
          llvm::Constant *min_scalar =
              llvm::ConstantInt::get(value->getType(), 0u);
          llvm::Constant *one = llvm::ConstantInt::get(value->getType(), 1u);
          llvm::Constant *surrogate_last =
              llvm::ConstantInt::get(value->getType(), 0xDFFFu);
          llvm::Constant *scalar_before_surrogates =
              llvm::ConstantInt::get(value->getType(), 0xD7FFu);
          llvm::Value *has_prev = builder.CreateICmpNE(value, min_scalar);
          llvm::Value *minus_one = builder.CreateSub(value, one);
          llvm::Value *is_surrogate_last =
              builder.CreateICmpEQ(minus_one, surrogate_last);
          llvm::Value *prev = builder.CreateSelect(
              is_surrogate_last, scalar_before_surrogates, minus_one);
          return BuiltinSuccessorResult{
              .has_next = has_prev,
              .next = prev,
          };
        }

        const unsigned width = value->getType()->getIntegerBitWidth();
        llvm::Constant *min_value = nullptr;
        if (IsSignedIntegerType(type))
        {
          min_value = llvm::ConstantInt::get(
              value->getType(), llvm::APInt::getSignedMinValue(width));
        }
        else
        {
          min_value = llvm::ConstantInt::get(value->getType(), 0u);
        }
        llvm::Value *has_prev = builder.CreateICmpNE(value, min_value);
        llvm::Value *prev =
            builder.CreateSub(value, llvm::ConstantInt::get(value->getType(), 1u));
        return BuiltinSuccessorResult{
            .has_next = has_prev,
            .next = prev,
        };
      }

      struct MaterializedRangeValue
      {
        IRRangeKind kind = IRRangeKind::Full;
        llvm::Value *lo = nullptr;
        llvm::Value *hi = nullptr;
      };

      std::optional<MaterializedRangeValue> ResolveRangeValue(
          const IRValue &value,
          llvm::Type *bound_ty = nullptr,
          std::optional<IRRangeKind> fallback_kind = std::nullopt) const
      {
        analysis::TypeRef range_type = NormalizeValueType(value);

        MaterializedRangeValue out;
        std::optional<unsigned> lo_index;
        std::optional<unsigned> hi_index;
        auto configure_for_kind = [&](IRRangeKind kind) -> bool
        {
          out.kind = kind;
          lo_index.reset();
          hi_index.reset();
          switch (kind)
          {
          case IRRangeKind::Full:
            return true;
          case IRRangeKind::From:
            lo_index = 0u;
            return true;
          case IRRangeKind::To:
          case IRRangeKind::ToInclusive:
            hi_index = 0u;
            return true;
          case IRRangeKind::Exclusive:
          case IRRangeKind::Inclusive:
            lo_index = 0u;
            hi_index = 1u;
            return true;
          }
          return false;
        };

        if (range_type && analysis::IsRangeType(range_type))
        {
          if (std::holds_alternative<analysis::TypeRange>(range_type->node))
          {
            if (!configure_for_kind(IRRangeKind::Exclusive))
            {
              return std::nullopt;
            }
          }
          else if (std::holds_alternative<analysis::TypeRangeInclusive>(
                       range_type->node))
          {
            if (!configure_for_kind(IRRangeKind::Inclusive))
            {
              return std::nullopt;
            }
          }
          else if (std::holds_alternative<analysis::TypeRangeFrom>(
                       range_type->node))
          {
            if (!configure_for_kind(IRRangeKind::From))
            {
              return std::nullopt;
            }
          }
          else if (std::holds_alternative<analysis::TypeRangeTo>(
                       range_type->node))
          {
            if (!configure_for_kind(IRRangeKind::To))
            {
              return std::nullopt;
            }
          }
          else if (std::holds_alternative<analysis::TypeRangeToInclusive>(
                       range_type->node))
          {
            if (!configure_for_kind(IRRangeKind::ToInclusive))
            {
              return std::nullopt;
            }
          }
          else if (std::holds_alternative<analysis::TypeRangeFull>(
                       range_type->node))
          {
            if (!configure_for_kind(IRRangeKind::Full))
            {
              return std::nullopt;
            }
          }
          else
          {
            return std::nullopt;
          }
        }
        else if (fallback_kind.has_value())
        {
          if (!configure_for_kind(*fallback_kind))
          {
            return std::nullopt;
          }
        }
        else
        {
          return std::nullopt;
        }

        if (!lo_index.has_value() && !hi_index.has_value())
        {
          return out;
        }

        llvm::Value *raw = EvaluateOrDefault(value);
        if (!raw)
        {
          return std::nullopt;
        }
        llvm::Type *range_ll = range_type ? emitter.GetLLVMType(range_type) : nullptr;

        if (raw->getType()->isPointerTy())
        {
          if (!range_ll)
          {
            return std::nullopt;
          }
          llvm::Value *typed_ptr = raw;
          llvm::Type *expected_ptr_ty = llvm::PointerType::get(range_ll, 0);
          if (typed_ptr->getType() != expected_ptr_ty)
          {
            typed_ptr = builder.CreateBitCast(typed_ptr, expected_ptr_ty);
          }
          raw = builder.CreateLoad(range_ll, typed_ptr);
        }
        else if (range_ll && raw->getType() != range_ll)
        {
          raw = CoerceTo(&builder, raw, range_ll);
        }
        if (!raw)
        {
          return std::nullopt;
        }

        auto extract_bound = [&](unsigned index) -> llvm::Value *
        {
          auto *struct_ty = llvm::dyn_cast<llvm::StructType>(raw->getType());
          if (!struct_ty || index >= struct_ty->getNumElements())
          {
            return nullptr;
          }
          llvm::Value *bound = builder.CreateExtractValue(raw, {index});
          if (!bound || !bound->getType()->isIntegerTy())
          {
            return nullptr;
          }
          llvm::Type *target_ty =
              bound_ty ? bound_ty : llvm::Type::getInt64Ty(emitter.GetContext());
          if (!target_ty->isIntegerTy())
          {
            return nullptr;
          }
          if (bound->getType() != target_ty)
          {
            bound = builder.CreateIntCast(bound, target_ty, false);
          }
          return bound;
        };

        if (lo_index.has_value())
        {
          out.lo = extract_bound(*lo_index);
          if (!out.lo)
          {
            return std::nullopt;
          }
        }
        if (hi_index.has_value())
        {
          out.hi = extract_bound(*hi_index);
          if (!out.hi)
          {
            return std::nullopt;
          }
        }
        return out;
      }

      void operator()(const IROpaque &) const {}

      void operator()(const IRSeq &seq) const
      {
        struct ForwardTargetInfo
        {
          const std::string *name = nullptr;
          const IRValue *value = nullptr;
          analysis::TypeRef bind_type = nullptr;
          llvm::Value *direct_storage = nullptr;
        };

        auto find_call_producing = [&](auto &&self,
                                       const IRPtr &ir,
                                       const IRValue &target_value) -> const IRCall *
        {
          if (!ir)
          {
            return nullptr;
          }
          if (const auto *call = std::get_if<IRCall>(&ir->node))
          {
            if (call->result.kind == target_value.kind &&
                call->result.name == target_value.name)
            {
              return call;
            }
            return nullptr;
          }
          const auto *nested = std::get_if<IRSeq>(&ir->node);
          if (nested)
          {
            for (const auto &child : nested->items)
            {
              if (const IRCall *call = self(self, child, target_value))
              {
                return call;
              }
            }
            return nullptr;
          }
          if (const auto *if_ir = std::get_if<IRIf>(&ir->node))
          {
            if (const IRCall *call = self(self, if_ir->then_ir, target_value))
            {
              return call;
            }
            return self(self, if_ir->else_ir, target_value);
          }
          if (const auto *block = std::get_if<IRBlock>(&ir->node))
          {
            if (const IRCall *call = self(self, block->setup, target_value))
            {
              return call;
            }
            return self(self, block->body, target_value);
          }
          if (const auto *loop = std::get_if<IRLoop>(&ir->node))
          {
            if (const IRCall *call = self(self, loop->iter_ir, target_value))
            {
              return call;
            }
            if (const IRCall *call = self(self, loop->cond_ir, target_value))
            {
              return call;
            }
            return self(self, loop->body_ir, target_value);
          }
          if (const auto *if_case = std::get_if<IRIfCase>(&ir->node))
          {
            for (const auto &arm : if_case->arms)
            {
              if (const IRCall *call = self(self, arm.body, target_value))
              {
                return call;
              }
            }
            return nullptr;
          }
          if (const auto *region = std::get_if<IRRegion>(&ir->node))
          {
            return self(self, region->body, target_value);
          }
          if (const auto *frame = std::get_if<IRFrame>(&ir->node))
          {
            return self(self, frame->body, target_value);
          }
          return nullptr;
        };

        auto target_info = [&](const IRPtr &ir) -> ForwardTargetInfo
        {
          if (!ir)
          {
            return {};
          }
          if (const auto *store = std::get_if<IRStoreVar>(&ir->node))
          {
            return ForwardTargetInfo{&store->name, &store->value, nullptr};
          }
          if (const auto *store_nodrop = std::get_if<IRStoreVarNoDrop>(&ir->node))
          {
            return ForwardTargetInfo{&store_nodrop->name, &store_nodrop->value, nullptr};
          }
          if (const auto *bind = std::get_if<IRBindVar>(&ir->node))
          {
            return ForwardTargetInfo{&bind->name, &bind->value, bind->type};
          }
          if (const auto *ret = std::get_if<IRReturn>(&ir->node))
          {
            llvm::Function *func =
                builder.GetInsertBlock() ? builder.GetInsertBlock()->getParent() : nullptr;
            const LowerCtx *active_ctx = emitter.GetCurrentCtx();
            if (!func || !active_ctx)
            {
              return {};
            }
            const std::string symbol = std::string(func->getName());
            const LowerCtx::ProcSigInfo *sig = active_ctx->LookupProcSig(symbol);
            if (!sig || !sig->ret)
            {
              return {};
            }
            llvm::Type *ret_ty = emitter.GetLLVMType(sig->ret);
            if (!ret_ty || ret_ty->isVoidTy())
            {
              return {};
            }
            const bool aggregate_ret =
                ret_ty->isArrayTy() ||
                (llvm::isa<llvm::StructType>(ret_ty) &&
                 llvm::cast<llvm::StructType>(ret_ty)->getNumElements() != 0);
            if (!aggregate_ret)
            {
              return {};
            }
            ABICallResult abi = emitter.ComputeProcABI(symbol, sig->params, sig->ret);
            if (!abi.valid || !abi.has_sret || func->arg_size() == 0)
            {
              return {};
            }
            return ForwardTargetInfo{nullptr, &ret->value, nullptr, func->getArg(0)};
          }
          return {};
        };

        auto ensure_home_slot = [&](const ForwardTargetInfo &target) -> llvm::Value *
        {
          if (target.direct_storage)
          {
            return target.direct_storage;
          }
          if (!target.name)
          {
            return nullptr;
          }

          llvm::Value *home_slot = emitter.GetLocalBindStorage(*target.name);
          if (home_slot || !target.bind_type)
          {
            return home_slot;
          }

          llvm::Type *slot_ty = emitter.GetLLVMType(target.bind_type);
          if (!slot_ty || slot_ty->isVoidTy())
          {
            return nullptr;
          }

          llvm::Function *func = builder.GetInsertBlock()->getParent();
          llvm::IRBuilder<> entry_builder(&func->getEntryBlock(),
                                          func->getEntryBlock().begin());
          home_slot = entry_builder.CreateAlloca(slot_ty, nullptr, *target.name);
          emitter.SetLocalHomeStorage(*target.name, home_slot);
          return home_slot;
        };

        for (std::size_t index = 0; index < seq.items.size(); ++index)
        {
          if (builder.GetInsertBlock()->getTerminator())
          {
            break;
          }
          const auto &item = seq.items[index];
          if (index + 1 < seq.items.size())
          {
            for (std::size_t lookahead = index + 1;
                 lookahead < seq.items.size();
                 ++lookahead)
            {
              if (std::holds_alternative<IROpaque>(seq.items[lookahead]->node) ||
                  std::holds_alternative<IRPanicCheck>(seq.items[lookahead]->node))
              {
                continue;
              }
              const ForwardTargetInfo target = target_info(seq.items[lookahead]);
              if (!target.value || (!target.name && !target.direct_storage))
              {
                continue;
              }
              if (const auto *call =
                      find_call_producing(find_call_producing, item, *target.value))
              {
                llvm::Value *home_slot = ensure_home_slot(target);
                if (home_slot && home_slot->getType()->isPointerTy())
                {
                  emitter.SetPreferredResultStorage(call->result, home_slot);
                  break;
                }
              }
            }
          }
          emitter.EmitIR(item);
        }
      }

      void operator()(const IRBindVar &bind) const
      {
        emitter.EmitBindVar(bind);
      }

      void operator()(const IRReadVar &) const
      {
        // Read semantics are represented by IRValue::Local in operand positions.
      }

      void operator()(const IRReadPath &read) const
      {
        std::vector<std::string> full = read.path;
        full.push_back(read.name);
        const std::string qualified = core::StringOfPath(full);
        const std::string mangled = core::Mangle(qualified);
        const LowerCtx *ctx = emitter.GetCurrentCtx();
        const bool known_proc = ctx && (ctx->LookupProcSig(mangled) != nullptr);
        const bool known_static = ctx && static_cast<bool>(ctx->LookupStaticType(mangled));
        const bool known_drop_glue = ctx && static_cast<bool>(ctx->LookupDropGlueType(mangled));
        const auto *record_ctor_path = ctx ? ctx->LookupRecordCtor(mangled) : nullptr;
        const bool known_record_ctor = record_ctor_path != nullptr;
        const bool known_runtime = IsRuntimeFunction(mangled);
        auto emit_poison_if_user_module = [&](const std::vector<std::string>* module_path) {
          if (!module_path || module_path->empty()) {
            return;
          }
          if (ctx && !ctx->module_path.empty() && !module_path->empty()) {
            const std::string& current_root = ctx->module_path.front();
            const std::string& target_root = module_path->front();
            const bool cross_library_boundary =
                current_root != target_root &&
                ctx->library_assembly_names.contains(target_root);
            if (cross_library_boundary) {
              return;
            }
          }
          emitter.EmitPoisonCheck(core::StringOfPath(*module_path));
        };

        if (known_record_ctor)
        {
          std::vector<std::string> owner_module = *record_ctor_path;
          if (!owner_module.empty())
          {
            owner_module.pop_back();
          }
          emit_poison_if_user_module(&owner_module);
          emitter.SetSymbolAlias(read.name, mangled);
          emitter.SetSymbolAlias(qualified, mangled);
          return;
        }

        if (emitter.GetFunction(mangled) || emitter.GetGlobal(mangled) ||
            known_proc || known_static || known_drop_glue || known_runtime)
        {
          if (known_proc)
          {
            emit_poison_if_user_module(ctx->LookupProcModule(mangled));
          }
          else if (known_static)
          {
            emit_poison_if_user_module(ctx->LookupStaticModule(mangled));
          }
          emitter.SetSymbolAlias(read.name, mangled);
          emitter.SetSymbolAlias(qualified, mangled);
          return;
        }
        if (emitter.GetFunction(read.name) || emitter.GetGlobal(read.name))
        {
          emitter.SetSymbolAlias(read.name, read.name);
          emitter.SetSymbolAlias(qualified, read.name);
        }
      }

      void operator()(const IRStoreVar &store) const
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

      void operator()(const IRStoreVarNoDrop &store) const
      {
        (*this)(IRStoreVar{store.name, store.value});
      }

      void operator()(const IRCall &call) const
      {
        std::vector<llvm::Value *> args;
        args.reserve(call.args.size());
        for (const auto &arg : call.args)
        {
          args.push_back(EvaluateOrDefault(arg));
        }

        if (call.callee.kind == IRValue::Kind::Symbol &&
            IsDropGlueSymbol(call.callee.name) &&
            !call.args.empty())
        {
          if (llvm::Value *storage = emitter.GetAddressableStorage(call.args.front()))
          {
            args.front() = storage;
          }
        }

        if (call.callee.kind == IRValue::Kind::Symbol)
        {
          const LowerCtx *comb_ctx = emitter.GetCurrentCtx();
          analysis::TypeRef comb_source_async_type =
              (comb_ctx && !call.args.empty())
                  ? comb_ctx->LookupValueType(call.args[0])
                  : nullptr;
          const analysis::ScopeContext &comb_scope = BuildScope(comb_ctx);
          const auto comb_source_sig =
              analysis::AsyncSigOf(comb_scope, comb_source_async_type);

          std::optional<AsyncCombinatorKind> comb_kind =
              AsyncCombinatorKindFromSymbol(call.callee.name);
          if (!comb_kind.has_value() && comb_source_sig.has_value())
          {
            comb_kind = analysis::LookupBuiltinAsyncCombinator(call.callee.name);
          }

          if (comb_kind.has_value())
          {
            if (args.empty() || call.args.empty())
            {
              emitter.SetTempValue(call.result, DefaultFor(call.result));
              return;
            }

            auto infer_callable_ret_type = [&](const IRValue &target) -> analysis::TypeRef
            {
              if (comb_ctx)
              {
                analysis::TypeRef callee_type =
                    analysis::StripPerm(comb_ctx->LookupValueType(target));
                if (!callee_type)
                {
                  callee_type = comb_ctx->LookupValueType(target);
                }
                if (const auto *fn =
                        callee_type ? std::get_if<analysis::TypeFunc>(&callee_type->node)
                                    : nullptr)
                {
                  return fn->ret;
                }
                if (const auto *closure =
                        callee_type ? std::get_if<analysis::TypeClosure>(&callee_type->node)
                                    : nullptr)
                {
                  return closure->ret;
                }
              }
              if (target.kind == IRValue::Kind::Symbol && comb_ctx)
              {
                if (const auto *sig = comb_ctx->LookupProcSig(target.name))
                {
                  return sig->ret;
                }
                if (const auto alias = emitter.LookupSymbolAlias(target.name))
                {
                  if (const auto *sig = comb_ctx->LookupProcSig(*alias))
                  {
                    return sig->ret;
                  }
                }
              }
              return nullptr;
            };

            analysis::TypeRef source_async_type =
                comb_source_async_type;
            analysis::TypeRef result_async_type =
                comb_ctx ? comb_ctx->LookupValueType(call.result) : nullptr;

            const auto source_sig = comb_source_sig;
            if (!result_async_type && source_sig)
            {
              if (*comb_kind == AsyncCombinatorKind::Map && call.args.size() >= 2)
              {
                if (analysis::TypeRef out_type = infer_callable_ret_type(call.args[1]))
                {
                  result_async_type = analysis::MakeTypePath(
                      {"Async"},
                      {out_type, source_sig->in, source_sig->result, source_sig->err});
                }
              }
              else if (*comb_kind == AsyncCombinatorKind::Fold &&
                       call.args.size() >= 2 && comb_ctx)
              {
                analysis::TypeRef acc_type = comb_ctx->LookupValueType(call.args[1]);
                if (acc_type)
                {
                  result_async_type = analysis::MakeTypePath(
                      {"Async"},
                      {analysis::MakeTypePrim("()"),
                       analysis::MakeTypePrim("()"),
                       acc_type,
                       source_sig->err});
                }
              }
              else if (*comb_kind == AsyncCombinatorKind::Chain &&
                       call.args.size() >= 2)
              {
                if (analysis::TypeRef chain_ret = infer_callable_ret_type(call.args[1]))
                {
                  result_async_type = chain_ret;
                }
              }
              else if (*comb_kind == AsyncCombinatorKind::Filter ||
                       *comb_kind == AsyncCombinatorKind::Take)
              {
                result_async_type = source_async_type;
              }
            }

            if (!result_async_type)
            {
              result_async_type = source_async_type;
            }

            const auto result_sig = analysis::AsyncSigOf(comb_scope, result_async_type);
            llvm::Value *source_async = args[0];
            auto *async_struct =
                llvm::dyn_cast_or_null<llvm::StructType>(
                    source_async ? source_async->getType() : nullptr);
            if (!comb_source_sig || !result_sig || !source_async || !async_struct ||
                async_struct->getNumElements() < 1 ||
                !async_struct->getElementType(0)->isIntegerTy())
            {
              emitter.SetTempValue(call.result, source_async ? source_async : DefaultFor(call.result));
              return;
            }

            llvm::Function *func =
                builder.GetInsertBlock() ? builder.GetInsertBlock()->getParent() : nullptr;
            if (!func)
            {
              emitter.SetTempValue(call.result, DefaultFor(call.result));
              return;
            }

            llvm::IRBuilder<> entry_builder(
                &func->getEntryBlock(),
                func->getEntryBlock().begin());
            llvm::AllocaInst *async_slot = entry_builder.CreateAlloca(async_struct);
            builder.CreateStore(source_async, async_slot);

            llvm::Type *expected_result_ty = ExpectedLLVMType(call.result);
            if (!expected_result_ty && result_async_type)
            {
              expected_result_ty = emitter.GetLLVMType(result_async_type);
            }
            if (!expected_result_ty)
            {
              expected_result_ty = source_async->getType();
            }
            llvm::AllocaInst *result_slot = nullptr;
            if (expected_result_ty && !expected_result_ty->isVoidTy())
            {
              result_slot = entry_builder.CreateAlloca(expected_result_ty);
              builder.CreateStore(llvm::Constant::getNullValue(expected_result_ty), result_slot);
            }

            auto materialize_as_type = [&](llvm::Value *value, llvm::Type *dst_ty) -> llvm::Value *
            {
              if (!value || !dst_ty)
              {
                return nullptr;
              }
              if (value->getType() == dst_ty)
              {
                return value;
              }
              if (llvm::Value *coerced = CoerceTo(&builder, value, dst_ty))
              {
                return coerced;
              }

              llvm::AllocaInst *dst_slot = entry_builder.CreateAlloca(dst_ty);
              builder.CreateStore(llvm::Constant::getNullValue(dst_ty), dst_slot);
              llvm::AllocaInst *src_slot = entry_builder.CreateAlloca(value->getType());
              builder.CreateStore(value, src_slot);

              llvm::Type *i8_ty = llvm::Type::getInt8Ty(emitter.GetContext());
              llvm::Type *i64_ty = llvm::Type::getInt64Ty(emitter.GetContext());
              llvm::Value *dst_i8 = builder.CreateBitCast(dst_slot, llvm::PointerType::get(i8_ty, 0));
              llvm::Value *src_i8 = builder.CreateBitCast(src_slot, llvm::PointerType::get(i8_ty, 0));
              const llvm::DataLayout &dl = emitter.GetModule().getDataLayout();
              const std::uint64_t src_size =
                  static_cast<std::uint64_t>(dl.getTypeAllocSize(value->getType()));
              const std::uint64_t dst_size =
                  static_cast<std::uint64_t>(dl.getTypeAllocSize(dst_ty));
              const std::uint64_t copy_size = std::min(src_size, dst_size);
              if (copy_size > 0)
              {
                builder.CreateMemCpy(
                    dst_i8,
                    llvm::Align(1),
                    src_i8,
                    llvm::Align(1),
                    llvm::ConstantInt::get(i64_ty, copy_size));
              }
              return builder.CreateLoad(dst_ty, dst_slot);
            };

            auto store_result = [&](llvm::Value *value)
            {
              if (!result_slot || !expected_result_ty || expected_result_ty->isVoidTy())
              {
                return;
              }
              llvm::Value *out = value;
              if (!out)
              {
                out = llvm::Constant::getNullValue(expected_result_ty);
              }
              else if (out->getType() != expected_result_ty)
              {
                if (result_async_type)
                {
                  if (llvm::Value *typed = CoerceToTyped(
                          emitter,
                          &builder,
                          out,
                          expected_result_ty,
                          source_async_type,
                          result_async_type))
                  {
                    out = typed;
                  }
                  else if (llvm::Value *plain = CoerceTo(&builder, out, expected_result_ty))
                  {
                    out = plain;
                  }
                  else
                  {
                    out = materialize_as_type(out, expected_result_ty);
                  }
                }
                else if (llvm::Value *plain = CoerceTo(&builder, out, expected_result_ty))
                {
                  out = plain;
                }
                else
                {
                  out = materialize_as_type(out, expected_result_ty);
                }
              }
              if (!out)
              {
                out = llvm::Constant::getNullValue(expected_result_ty);
              }
              builder.CreateStore(out, result_slot);
            };

            auto finish_from = [&](llvm::Value *value)
            {
              if (result_slot && expected_result_ty && !expected_result_ty->isVoidTy())
              {
                if (value)
                {
                  store_result(value);
                }
                llvm::Value *out = builder.CreateLoad(expected_result_ty, result_slot);
                emitter.SetTempValue(call.result, out ? out : DefaultFor(call.result));
                return;
              }
              emitter.SetTempValue(call.result, value ? value : DefaultFor(call.result));
            };

            std::size_t temp_index = 0;
            auto make_temp_local = [&](std::string_view stem,
                                       llvm::Value *value,
                                       const analysis::TypeRef & /*type*/) -> IRValue
            {
              // These are ephemeral SSA values, not addressable locals.
              // Store them as opaque temps so EvaluateIRValue reads the
              // materialized value instead of falling back through local slots.
              IRValue temp;
              temp.kind = IRValue::Kind::Opaque;
              temp.name = call.result.name + "." + std::string(stem) + "." +
                          std::to_string(temp_index++);
              emitter.SetTempValue(temp, value);
              return temp;
            };

            auto invoke_callable =
                [&](const IRValue &callee,
                    const std::vector<std::pair<llvm::Value *, analysis::TypeRef>> &call_args,
                    std::string_view stem) -> llvm::Value *
            {
              IRCall inner;
              inner.callee = callee;
              inner.args.reserve(call_args.size() + 1);
              for (std::size_t i = 0; i < call_args.size(); ++i)
              {
                std::string arg_stem = std::string(stem) + "_arg" + std::to_string(i);
                inner.args.push_back(make_temp_local(arg_stem, call_args[i].first, call_args[i].second));
              }

              llvm::Value *panic_arg_value =
                  LoadLocalValue(emitter, &builder, std::string(kPanicOutName));
              const analysis::TypeRef panic_arg_type = PanicOutType();
              llvm::Type *panic_arg_ll = emitter.GetLLVMType(panic_arg_type);
              if (panic_arg_ll && panic_arg_value &&
                  panic_arg_value->getType() != panic_arg_ll)
              {
                if (llvm::Value *coerced = CoerceTo(&builder, panic_arg_value, panic_arg_ll))
                {
                  panic_arg_value = coerced;
                }
                else if (panic_arg_value->getType()->isPointerTy() &&
                         panic_arg_ll->isPointerTy())
                {
                  panic_arg_value = builder.CreateBitCast(panic_arg_value, panic_arg_ll);
                }
                else
                {
                  panic_arg_value = nullptr;
                }
              }
              if (!panic_arg_value && panic_arg_ll && !panic_arg_ll->isVoidTy())
              {
                panic_arg_value = llvm::Constant::getNullValue(panic_arg_ll);
              }
              if (panic_arg_value)
              {
                inner.args.push_back(
                    make_temp_local(std::string(stem) + "_panic", panic_arg_value, panic_arg_type));
              }

              inner.result.kind = IRValue::Kind::Opaque;
              inner.result.name = call.result.name + "." + std::string(stem) + ".ret." +
                                  std::to_string(temp_index++);
              const analysis::TypeRef callable_ret_type = infer_callable_ret_type(callee);
              (*this)(inner);
              llvm::Value *out = emitter.EvaluateIRValue(inner.result);
              if (!out && callable_ret_type)
              {
                if (llvm::Value *storage = emitter.GetTempStorage(inner.result))
                {
                  if (llvm::Type *ret_ty = emitter.GetLLVMType(callable_ret_type))
                  {
                    llvm::Value *typed_ptr = storage;
                    llvm::Type *expected_ptr_ty = llvm::PointerType::get(ret_ty, 0);
                    if (typed_ptr->getType() != expected_ptr_ty)
                    {
                      typed_ptr = builder.CreateBitCast(typed_ptr, expected_ptr_ty);
                    }
                    out = builder.CreateLoad(ret_ty, typed_ptr);
                  }
                }
              }
              if (!out)
              {
                llvm::Type *fallback_ty =
                    callable_ret_type ? emitter.GetLLVMType(callable_ret_type) : nullptr;
                if (fallback_ty && !fallback_ty->isVoidTy())
                {
                  out = llvm::Constant::getNullValue(fallback_ty);
                }
                else
                {
                  out = llvm::ConstantInt::get(
                      llvm::Type::getInt64Ty(emitter.GetContext()), 0);
                }
              }
              return out;
            };

            auto load_payload_from_slot =
                [&](llvm::AllocaInst *slot,
                    const analysis::TypeRef &payload_type) -> llvm::Value *
            {
              if (!slot || !payload_type || IsUnitTypeRef(payload_type) || IsNeverTypeRef(payload_type))
              {
                return nullptr;
              }
              llvm::Type *payload_ll = emitter.GetLLVMType(payload_type);
              if (!payload_ll || payload_ll->isVoidTy())
              {
                return nullptr;
              }
              llvm::Value *payload_i8 = CreateTaggedPayloadI8Ptr(
                  emitter,
                  &builder,
                  async_struct,
                  slot,
                  ::cursive::analysis::layout::kPtrAlign);
              if (!payload_i8)
              {
                return nullptr;
              }
              llvm::Value *payload_ptr =
                  builder.CreateBitCast(payload_i8, llvm::PointerType::get(payload_ll, 0));
              llvm::LoadInst *loaded = builder.CreateLoad(payload_ll, payload_ptr);
              loaded->setAlignment(llvm::Align(1));
              return loaded;
            };

            auto store_payload_to_slot =
                [&](llvm::AllocaInst *slot,
                    llvm::Value *payload_value,
                    const analysis::TypeRef &payload_type)
            {
              if (!slot || !payload_value || !payload_type ||
                  IsUnitTypeRef(payload_type) || IsNeverTypeRef(payload_type))
              {
                return;
              }
              llvm::Type *payload_ll = emitter.GetLLVMType(payload_type);
              if (!payload_ll || payload_ll->isVoidTy())
              {
                return;
              }
              llvm::Value *coerced = payload_value;
              if (coerced->getType() != payload_ll)
              {
                if (llvm::Value *typed = CoerceToTyped(
                        emitter,
                        &builder,
                        coerced,
                        payload_ll,
                        payload_type,
                        payload_type))
                {
                  coerced = typed;
                }
                else if (llvm::Value *plain = CoerceTo(&builder, coerced, payload_ll))
                {
                  coerced = plain;
                }
                else
                {
                  coerced = materialize_as_type(coerced, payload_ll);
                }
              }
              if (!coerced)
              {
                return;
              }
              llvm::Value *payload_i8 = CreateTaggedPayloadI8Ptr(
                  emitter,
                  &builder,
                  async_struct,
                  slot,
                  ::cursive::analysis::layout::kPtrAlign);
              if (!payload_i8)
              {
                return;
              }
              llvm::Value *payload_ptr =
                  builder.CreateBitCast(payload_i8, llvm::PointerType::get(payload_ll, 0));
              llvm::StoreInst *stored = builder.CreateStore(coerced, payload_ptr);
              stored->setAlignment(llvm::Align(1));
            };

            auto make_async_complete = [&](llvm::Value *payload_value,
                                           const analysis::TypeRef &payload_type) -> llvm::Value *
            {
              analysis::TypeRef complete_payload_type =
                  payload_type ? payload_type : analysis::MakeTypePrim("()");
              llvm::Value *payload = payload_value;
              if (!payload)
              {
                payload = llvm::ConstantInt::get(
                    llvm::Type::getInt64Ty(emitter.GetContext()), 0);
              }
              IRValue payload_ir = make_temp_local("async_complete_payload", payload, complete_payload_type);
              IRAsyncComplete complete;
              complete.value = payload_ir;
              complete.result.kind = IRValue::Kind::Opaque;
              complete.result.name = call.result.name + ".async_complete." +
                                     std::to_string(temp_index++);
              complete.async_type = result_async_type;
              complete.result_type = complete_payload_type;
              (*this)(complete);
              return emitter.EvaluateIRValue(complete.result);
            };

            auto make_async_fail = [&](llvm::Value *payload_value,
                                       const analysis::TypeRef &payload_type) -> llvm::Value *
            {
              analysis::TypeRef fail_payload_type =
                  payload_type ? payload_type : analysis::MakeTypePrim("!");
              llvm::Value *payload = payload_value;
              if (!payload)
              {
                payload = llvm::ConstantInt::get(
                    llvm::Type::getInt64Ty(emitter.GetContext()), 0);
              }
              IRValue payload_ir = make_temp_local("async_fail_payload", payload, fail_payload_type);
              IRAsyncFail fail;
              fail.value = payload_ir;
              fail.result.kind = IRValue::Kind::Opaque;
              fail.result.name = call.result.name + ".async_fail." +
                                 std::to_string(temp_index++);
              fail.async_type = result_async_type;
              fail.error_type = fail_payload_type;
              (*this)(fail);
              return emitter.EvaluateIRValue(fail.result);
            };

            const AsyncStateDiscs source_discs =
                LoweredAsyncStateDiscs(comb_scope, *source_sig);
            const std::uint64_t suspended_disc = source_discs.suspended;
            const std::uint64_t completed_disc = source_discs.completed;
            const std::optional<std::uint64_t> failed_disc = source_discs.failed;

            llvm::Type *i8_ty = llvm::Type::getInt8Ty(emitter.GetContext());
            llvm::Type *opaque_ptr_ty = emitter.GetOpaquePtr();
            auto *opaque_ptr_ptr_ty = llvm::cast<llvm::PointerType>(opaque_ptr_ty);
            llvm::Value *panic_ptr = LoadLocalValue(emitter, &builder, std::string(kPanicOutName));
            if (panic_ptr)
            {
              if (llvm::Value *coerced = CoerceTo(&builder, panic_ptr, opaque_ptr_ty))
              {
                panic_ptr = coerced;
              }
              else if (panic_ptr->getType()->isPointerTy())
              {
                panic_ptr = builder.CreateBitCast(panic_ptr, opaque_ptr_ty);
              }
              else
              {
                panic_ptr = nullptr;
              }
            }
            if (!panic_ptr)
            {
              panic_ptr = llvm::ConstantPointerNull::get(opaque_ptr_ptr_ty);
            }

            auto emit_resume_step = [&]()
            {
              llvm::Value *suspended_ptr = builder.CreateBitCast(async_slot, opaque_ptr_ty);
              llvm::Value *unit_input = llvm::ConstantPointerNull::get(opaque_ptr_ptr_ty);
              llvm::Value *resume_call = EmitAsyncResumeRuntimeCall(
                  emitter,
                  &builder,
                  suspended_ptr,
                  unit_input,
                  panic_ptr);
              llvm::Value *resumed_async = materialize_as_type(resume_call, async_struct);
              if (!resumed_async)
              {
                resumed_async = llvm::Constant::getNullValue(async_struct);
              }
              builder.CreateStore(resumed_async, async_slot);
            };

            if (*comb_kind == AsyncCombinatorKind::Map)
            {
              if (call.args.size() < 2 || args.size() < 2)
              {
                finish_from(source_async);
                return;
              }
              llvm::BasicBlock *suspended_bb =
                  llvm::BasicBlock::Create(emitter.GetContext(), "ac.map.suspended", func);
              llvm::BasicBlock *merge_bb =
                  llvm::BasicBlock::Create(emitter.GetContext(), "ac.map.merge", func);
              llvm::Value *current_async = builder.CreateLoad(async_struct, async_slot);
              llvm::Value *disc = builder.CreateExtractValue(current_async, {0u});
              llvm::Value *is_suspended = EmitTypedEq(
                  &builder,
                  disc,
                  llvm::ConstantInt::get(disc->getType(), suspended_disc));
              builder.CreateCondBr(AsBool(&builder, is_suspended), suspended_bb, merge_bb);

              builder.SetInsertPoint(suspended_bb);
              llvm::Value *output = load_payload_from_slot(async_slot, source_sig->out);
              llvm::Value *mapped = invoke_callable(
                  call.args[1],
                  {{output, source_sig->out}},
                  "map_fn");
              store_payload_to_slot(async_slot, mapped, result_sig->out);
              builder.CreateBr(merge_bb);

              builder.SetInsertPoint(merge_bb);
              finish_from(builder.CreateLoad(async_struct, async_slot));
              return;
            }

            if (*comb_kind == AsyncCombinatorKind::Filter)
            {
              if (call.args.size() < 2 || args.size() < 2)
              {
                finish_from(source_async);
                return;
              }
              llvm::BasicBlock *loop_bb =
                  llvm::BasicBlock::Create(emitter.GetContext(), "ac.filter.loop", func);
              llvm::BasicBlock *suspended_bb =
                  llvm::BasicBlock::Create(emitter.GetContext(), "ac.filter.suspended", func);
              llvm::BasicBlock *resume_bb =
                  llvm::BasicBlock::Create(emitter.GetContext(), "ac.filter.resume", func);
              llvm::BasicBlock *exit_bb =
                  llvm::BasicBlock::Create(emitter.GetContext(), "ac.filter.exit", func);
              builder.CreateBr(loop_bb);

              builder.SetInsertPoint(loop_bb);
              llvm::Value *current_async = builder.CreateLoad(async_struct, async_slot);
              llvm::Value *disc = builder.CreateExtractValue(current_async, {0u});
              auto *disc_ty = llvm::cast<llvm::IntegerType>(disc->getType());
              llvm::SwitchInst *sw = builder.CreateSwitch(
                  disc, exit_bb, failed_disc.has_value() ? 3 : 2);
              sw->addCase(llvm::ConstantInt::get(disc_ty, suspended_disc), suspended_bb);
              sw->addCase(llvm::ConstantInt::get(disc_ty, completed_disc), exit_bb);
              if (failed_disc.has_value())
              {
                sw->addCase(llvm::ConstantInt::get(disc_ty, *failed_disc), exit_bb);
              }

              builder.SetInsertPoint(suspended_bb);
              llvm::Value *output = load_payload_from_slot(async_slot, source_sig->out);
              llvm::Value *pred_val = invoke_callable(
                  call.args[1],
                  {{output, source_sig->out}},
                  "filter_pred");
              builder.CreateCondBr(AsBool(&builder, pred_val), exit_bb, resume_bb);

              builder.SetInsertPoint(resume_bb);
              emit_resume_step();
              builder.CreateBr(loop_bb);

              builder.SetInsertPoint(exit_bb);
              finish_from(builder.CreateLoad(async_struct, async_slot));
              return;
            }

            if (*comb_kind == AsyncCombinatorKind::Take)
            {
              if (call.args.size() < 2 || args.size() < 2 || !result_sig->result)
              {
                finish_from(source_async);
                return;
              }
              llvm::Value *count = args[1];
              llvm::Type *i64_ty = llvm::Type::getInt64Ty(emitter.GetContext());
              if (count->getType() != i64_ty)
              {
                if (llvm::Value *coerced = CoerceTo(&builder, count, i64_ty))
                {
                  count = coerced;
                }
                else
                {
                  count = llvm::ConstantInt::get(i64_ty, 1);
                }
              }

              llvm::BasicBlock *zero_bb =
                  llvm::BasicBlock::Create(emitter.GetContext(), "ac.take.zero", func);
              llvm::BasicBlock *nonzero_bb =
                  llvm::BasicBlock::Create(emitter.GetContext(), "ac.take.nonzero", func);
              llvm::BasicBlock *merge_bb =
                  llvm::BasicBlock::Create(emitter.GetContext(), "ac.take.merge", func);
              llvm::Value *is_zero = builder.CreateICmpEQ(
                  count, llvm::ConstantInt::get(i64_ty, 0));
              builder.CreateCondBr(is_zero, zero_bb, nonzero_bb);

              builder.SetInsertPoint(zero_bb);
              llvm::Value *completed = make_async_complete(nullptr, result_sig->result);
              store_result(completed);
              builder.CreateBr(merge_bb);

              builder.SetInsertPoint(nonzero_bb);
              store_result(builder.CreateLoad(async_struct, async_slot));
              builder.CreateBr(merge_bb);

              builder.SetInsertPoint(merge_bb);
              finish_from(nullptr);
              return;
            }

            if (*comb_kind == AsyncCombinatorKind::Fold)
            {
              if (call.args.size() < 3 || args.size() < 3)
              {
                finish_from(source_async);
                return;
              }
              analysis::TypeRef acc_type = result_sig->result;
              llvm::Type *acc_ll =
                  acc_type ? emitter.GetLLVMType(acc_type) : args[1]->getType();
              if (!acc_ll || acc_ll->isVoidTy())
              {
                acc_ll = args[1]->getType();
              }
              llvm::AllocaInst *acc_slot = entry_builder.CreateAlloca(acc_ll);
              llvm::Value *init_acc = args[1];
              if (init_acc->getType() != acc_ll)
              {
                if (llvm::Value *coerced = CoerceTo(&builder, init_acc, acc_ll))
                {
                  init_acc = coerced;
                }
                else
                {
                  init_acc = materialize_as_type(init_acc, acc_ll);
                }
              }
              if (!init_acc)
              {
                init_acc = llvm::Constant::getNullValue(acc_ll);
              }
              builder.CreateStore(init_acc, acc_slot);

              llvm::BasicBlock *loop_bb =
                  llvm::BasicBlock::Create(emitter.GetContext(), "ac.fold.loop", func);
              llvm::BasicBlock *suspended_bb =
                  llvm::BasicBlock::Create(emitter.GetContext(), "ac.fold.suspended", func);
              llvm::BasicBlock *resume_bb =
                  llvm::BasicBlock::Create(emitter.GetContext(), "ac.fold.resume", func);
              llvm::BasicBlock *completed_bb =
                  llvm::BasicBlock::Create(emitter.GetContext(), "ac.fold.completed", func);
              llvm::BasicBlock *failed_bb = failed_disc.has_value()
                                                ? llvm::BasicBlock::Create(
                                                      emitter.GetContext(),
                                                      "ac.fold.failed",
                                                      func)
                                                : nullptr;
              llvm::BasicBlock *merge_bb =
                  llvm::BasicBlock::Create(emitter.GetContext(), "ac.fold.merge", func);
              builder.CreateBr(loop_bb);

              builder.SetInsertPoint(loop_bb);
              llvm::Value *current_async = builder.CreateLoad(async_struct, async_slot);
              llvm::Value *disc = builder.CreateExtractValue(current_async, {0u});
              auto *disc_ty = llvm::cast<llvm::IntegerType>(disc->getType());
              llvm::SwitchInst *sw = builder.CreateSwitch(
                  disc, completed_bb, failed_disc.has_value() ? 3 : 2);
              sw->addCase(llvm::ConstantInt::get(disc_ty, suspended_disc), suspended_bb);
              sw->addCase(llvm::ConstantInt::get(disc_ty, completed_disc), completed_bb);
              if (failed_disc.has_value())
              {
                sw->addCase(llvm::ConstantInt::get(disc_ty, *failed_disc), failed_bb);
              }

              builder.SetInsertPoint(suspended_bb);
              llvm::Value *out = load_payload_from_slot(async_slot, source_sig->out);
              llvm::Value *acc = builder.CreateLoad(acc_ll, acc_slot);
              llvm::Value *next_acc = invoke_callable(
                  call.args[2],
                  {{acc, acc_type}, {out, source_sig->out}},
                  "fold_fn");
              if (next_acc->getType() != acc_ll)
              {
                if (llvm::Value *coerced = CoerceTo(&builder, next_acc, acc_ll))
                {
                  next_acc = coerced;
                }
                else
                {
                  next_acc = materialize_as_type(next_acc, acc_ll);
                }
              }
              if (!next_acc)
              {
                next_acc = llvm::Constant::getNullValue(acc_ll);
              }
              builder.CreateStore(next_acc, acc_slot);
              builder.CreateBr(resume_bb);

              builder.SetInsertPoint(resume_bb);
              emit_resume_step();
              builder.CreateBr(loop_bb);

              builder.SetInsertPoint(completed_bb);
              llvm::Value *final_acc = builder.CreateLoad(acc_ll, acc_slot);
              llvm::Value *complete = make_async_complete(final_acc, acc_type);
              store_result(complete);
              builder.CreateBr(merge_bb);

              if (failed_bb)
              {
                builder.SetInsertPoint(failed_bb);
                llvm::Value *err = load_payload_from_slot(async_slot, source_sig->err);
                llvm::Value *fail = make_async_fail(err, source_sig->err);
                store_result(fail);
                builder.CreateBr(merge_bb);
              }

              builder.SetInsertPoint(merge_bb);
              finish_from(nullptr);
              return;
            }

            if (*comb_kind == AsyncCombinatorKind::Chain)
            {
              if (call.args.size() < 2 || args.size() < 2)
              {
                finish_from(source_async);
                return;
              }
              llvm::BasicBlock *loop_bb =
                  llvm::BasicBlock::Create(emitter.GetContext(), "ac.chain.loop", func);
              llvm::BasicBlock *suspended_bb =
                  llvm::BasicBlock::Create(emitter.GetContext(), "ac.chain.suspended", func);
              llvm::BasicBlock *resume_bb =
                  llvm::BasicBlock::Create(emitter.GetContext(), "ac.chain.resume", func);
              llvm::BasicBlock *completed_bb =
                  llvm::BasicBlock::Create(emitter.GetContext(), "ac.chain.completed", func);
              llvm::BasicBlock *failed_bb = failed_disc.has_value()
                                                ? llvm::BasicBlock::Create(
                                                      emitter.GetContext(),
                                                      "ac.chain.failed",
                                                      func)
                                                : nullptr;
              llvm::BasicBlock *merge_bb =
                  llvm::BasicBlock::Create(emitter.GetContext(), "ac.chain.merge", func);
              builder.CreateBr(loop_bb);

              builder.SetInsertPoint(loop_bb);
              llvm::Value *current_async = builder.CreateLoad(async_struct, async_slot);
              llvm::Value *disc = builder.CreateExtractValue(current_async, {0u});
              auto *disc_ty = llvm::cast<llvm::IntegerType>(disc->getType());
              llvm::SwitchInst *sw = builder.CreateSwitch(
                  disc, completed_bb, failed_disc.has_value() ? 3 : 2);
              sw->addCase(llvm::ConstantInt::get(disc_ty, suspended_disc), suspended_bb);
              sw->addCase(llvm::ConstantInt::get(disc_ty, completed_disc), completed_bb);
              if (failed_disc.has_value())
              {
                sw->addCase(llvm::ConstantInt::get(disc_ty, *failed_disc), failed_bb);
              }

              builder.SetInsertPoint(suspended_bb);
              builder.CreateBr(resume_bb);

              builder.SetInsertPoint(resume_bb);
              emit_resume_step();
              builder.CreateBr(loop_bb);

              builder.SetInsertPoint(completed_bb);
              llvm::Value *completed_value = load_payload_from_slot(async_slot, source_sig->result);
              llvm::Value *chained_async = invoke_callable(
                  call.args[1],
                  {{completed_value, source_sig->result}},
                  "chain_fn");
              store_result(chained_async);
              builder.CreateBr(merge_bb);

              if (failed_bb)
              {
                builder.SetInsertPoint(failed_bb);
                llvm::Value *err = load_payload_from_slot(async_slot, source_sig->err);
                llvm::Value *fail = make_async_fail(err, source_sig->err);
                store_result(fail);
                builder.CreateBr(merge_bb);
              }

              builder.SetInsertPoint(merge_bb);
              finish_from(nullptr);
              return;
            }

            finish_from(source_async);
            return;
          }
        }

        const LowerCtx *ctx = emitter.GetCurrentCtx();
        const LowerCtx::ProcSigInfo *sig = nullptr;
        LowerCtx::ProcSigInfo inferred_sig;
        LowerCtx::ProcSigInfo closure_adjusted_sig;
        LowerCtx::ProcSigInfo hosted_adjusted_sig;
        std::string callee_symbol = call.callee.name;
        const bool is_async_resume_runtime_symbol =
            (call.callee.kind == IRValue::Kind::Symbol) &&
            (call.callee.name == BuiltinSymAsyncResume());
        if (is_async_resume_runtime_symbol)
        {
          llvm::Type *opaque_ptr_ty = emitter.GetOpaquePtr();
          auto *opaque_ptr_ptr_ty = llvm::cast<llvm::PointerType>(opaque_ptr_ty);

          llvm::Value *panic_arg_value =
              LoadLocalValue(emitter, &builder, std::string(kPanicOutName));
          if (panic_arg_value)
          {
            if (llvm::Value *coerced = CoerceTo(&builder, panic_arg_value, opaque_ptr_ty))
            {
              panic_arg_value = coerced;
            }
            else if (panic_arg_value->getType()->isPointerTy())
            {
              panic_arg_value = builder.CreateBitCast(panic_arg_value, opaque_ptr_ty);
            }
            else
            {
              panic_arg_value = nullptr;
            }
          }
          if (!panic_arg_value)
          {
            panic_arg_value = llvm::ConstantPointerNull::get(opaque_ptr_ptr_ty);
          }

          if (args.size() < 3)
          {
            args.resize(3, llvm::ConstantPointerNull::get(opaque_ptr_ptr_ty));
          }
          if (!args[2] || llvm::isa<llvm::ConstantPointerNull>(args[2]))
          {
            args[2] = panic_arg_value;
          }
        }
        const bool is_foundational_eq_symbol =
            (call.callee.kind == IRValue::Kind::Symbol) &&
            (call.callee.name == BuiltinSymEqEq());
        const bool is_foundational_step_successor_symbol =
            (call.callee.kind == IRValue::Kind::Symbol) &&
            (call.callee.name == BuiltinSymStepSuccessor());
        const bool is_foundational_step_predecessor_symbol =
            (call.callee.kind == IRValue::Kind::Symbol) &&
            (call.callee.name == BuiltinSymStepPredecessor());
        if (is_foundational_eq_symbol ||
            is_foundational_step_successor_symbol ||
            is_foundational_step_predecessor_symbol)
        {
          auto report_builtin_failure = [&]()
          {
            if (ctx)
            {
              const_cast<LowerCtx *>(ctx)->ReportCodegenFailure();
            }
            emitter.SetTempValue(call.result, DefaultFor(call.result));
          };

          if (args.empty())
          {
            report_builtin_failure();
            return;
          }

          const analysis::TypeRef recv_type = NormalizeValueType(call.args[0]);
          if (!recv_type)
          {
            report_builtin_failure();
            return;
          }

          if (is_foundational_eq_symbol)
          {
            if (args.size() < 2)
            {
              report_builtin_failure();
              return;
            }
            llvm::Value *eq = EmitBuiltinEqCall(builder, recv_type, args[0], args[1]);
            if (!eq)
            {
              report_builtin_failure();
              return;
            }
            emitter.SetTempValue(call.result, eq);
            return;
          }

          const analysis::TypeRef result_type = NormalizeValueType(call.result);
          llvm::Type *target_ll = ExpectedLLVMType(call.result);
          if (!target_ll && result_type)
          {
            target_ll = emitter.GetLLVMType(result_type);
          }
          if (!result_type || !target_ll)
          {
            report_builtin_failure();
            return;
          }

          const auto step_result =
              is_foundational_step_successor_symbol
                  ? EmitBuiltinSuccessor(builder, recv_type, args[0])
                  : EmitBuiltinPredecessor(builder, recv_type, args[0]);
          if (!step_result.has_value())
          {
            report_builtin_failure();
            return;
          }

          llvm::Value *packed_some = PackUnionFromMember(
              emitter, &builder, step_result->next, target_ll, recv_type, result_type);
          const analysis::TypeRef unit_type = analysis::MakeTypePrim("()");
          llvm::Type *unit_ll = emitter.GetLLVMType(unit_type);
          llvm::Value *unit_value = unit_ll ? llvm::UndefValue::get(unit_ll) : nullptr;
          llvm::Value *packed_none = PackUnionFromMember(
              emitter, &builder, unit_value, target_ll, unit_type, result_type);
          llvm::Function *fn =
              builder.GetInsertBlock() ? builder.GetInsertBlock()->getParent() : nullptr;
          if (!packed_some || !packed_none || !fn)
          {
            report_builtin_failure();
            return;
          }

          llvm::BasicBlock *step_some_bb =
              llvm::BasicBlock::Create(emitter.GetContext(), "step.some", fn);
          llvm::BasicBlock *step_none_bb =
              llvm::BasicBlock::Create(emitter.GetContext(), "step.none", fn);
          llvm::BasicBlock *step_merge_bb =
              llvm::BasicBlock::Create(emitter.GetContext(), "step.merge", fn);
          builder.CreateCondBr(step_result->has_next, step_some_bb, step_none_bb);

          builder.SetInsertPoint(step_some_bb);
          builder.CreateBr(step_merge_bb);
          llvm::BasicBlock *step_some_end = builder.GetInsertBlock();

          builder.SetInsertPoint(step_none_bb);
          builder.CreateBr(step_merge_bb);
          llvm::BasicBlock *step_none_end = builder.GetInsertBlock();

          builder.SetInsertPoint(step_merge_bb);
          llvm::PHINode *phi = builder.CreatePHI(target_ll, 2);
          phi->addIncoming(packed_some, step_some_end);
          phi->addIncoming(packed_none, step_none_end);
          emitter.SetTempValue(call.result, phi);
          return;
        }
        bool use_c_abi_aggregate_sret = false;
        if (call.callee.kind == IRValue::Kind::Symbol)
        {
          if (auto alias = emitter.LookupSymbolAlias(call.callee.name))
          {
            callee_symbol = *alias;
          }
          if (auto runtime = GetRuntimeFuncInfo(callee_symbol))
          {
            inferred_sig.params = runtime->params;
            inferred_sig.ret = runtime->ret;
            sig = &inferred_sig;
            use_c_abi_aggregate_sret = true;
          }
          else if (auto runtime = GetRuntimeFuncInfo(call.callee.name))
          {
            inferred_sig.params = runtime->params;
            inferred_sig.ret = runtime->ret;
            sig = &inferred_sig;
            callee_symbol = call.callee.name;
            use_c_abi_aggregate_sret = true;
          }
          if (IsRuntimeFunction(callee_symbol) ||
              IsRuntimeFunction(call.callee.name))
          {
            // Runtime symbols cross a foreign C ABI boundary. Aggregate return
            // lowering must always honor platform C ABI (including hidden sret
            // where required, e.g. Win64 C0DynObject returns).
            use_c_abi_aggregate_sret = true;
          }
          if (!sig && ctx)
          {
            sig = ctx->LookupProcSig(callee_symbol);
            if (!sig)
            {
              sig = ctx->LookupProcSig(call.callee.name);
            }
          }
          if (!sig)
          {
            if (auto runtime = GetRuntimeFuncInfo(callee_symbol))
            {
              inferred_sig.params = runtime->params;
              inferred_sig.ret = runtime->ret;
              sig = &inferred_sig;
              // Runtime symbols are foreign C-ABI boundaries. Aggregate return
              // lowering must follow platform C ABI (including sret where needed).
              use_c_abi_aggregate_sret = true;
            }
            else if (auto runtime = GetRuntimeFuncInfo(call.callee.name))
            {
              inferred_sig.params = runtime->params;
              inferred_sig.ret = runtime->ret;
              sig = &inferred_sig;
              callee_symbol = call.callee.name;
              // Runtime symbols are foreign C-ABI boundaries. Aggregate return
              // lowering must follow platform C ABI (including sret where needed).
              use_c_abi_aggregate_sret = true;
            }
          }
        }
        if (!sig && ctx)
        {
          auto is_complete_sig = [](const LowerCtx::ProcSigInfo &info) -> bool
          {
            if (!info.ret)
            {
              return false;
            }
            for (const auto &param : info.params)
            {
              if (!param.type)
              {
                return false;
              }
            }
            return true;
          };
          // Prefer concrete closure-code signatures over inferred function types.
          // This preserves ABI decisions (notably sret) when analysis-level closure
          // types are still partially inferred.
          if (call.callee.kind == IRValue::Kind::Opaque)
          {
            if (const DerivedValueInfo *derived = ctx->LookupDerivedValue(call.callee))
            {
              if (derived->kind == DerivedValueInfo::Kind::Tuple &&
                  derived->tuple_index == 1)
              {
                if (const DerivedValueInfo *base_derived =
                        ctx->LookupDerivedValue(derived->base))
                {
                  if (base_derived->kind == DerivedValueInfo::Kind::TupleLit &&
                      base_derived->elements.size() > 1)
                  {
                    const IRValue &code_elem = base_derived->elements[1];
                    if (code_elem.kind == IRValue::Kind::Symbol)
                    {
                      if (const LowerCtx::ProcSigInfo *concrete =
                              ctx->LookupProcSig(code_elem.name))
                      {
                        sig = concrete;
                        callee_symbol = code_elem.name;
                      }
                    }
                  }
                }
              }
            }
          }

          analysis::TypeRef callee_type = analysis::StripPerm(ctx->LookupValueType(call.callee));
          if (!callee_type)
          {
            callee_type = ctx->LookupValueType(call.callee);
          }
          if (!callee_type && call.callee.kind == IRValue::Kind::Local)
          {
            callee_type = analysis::StripPerm(emitter.LookupLocalType(call.callee.name));
            if (!callee_type)
            {
              callee_type = emitter.LookupLocalType(call.callee.name);
            }
          }
          if (!sig && !callee_type && call.callee.kind == IRValue::Kind::Opaque)
          {
            if (const DerivedValueInfo *derived = ctx->LookupDerivedValue(call.callee))
            {
              if (derived->kind == DerivedValueInfo::Kind::Tuple &&
                  derived->tuple_index == 1)
              {
                analysis::TypeRef base_type =
                    analysis::StripPerm(ctx->LookupValueType(derived->base));
                if (!base_type)
                {
                  base_type = ctx->LookupValueType(derived->base);
                }
                if (const auto *closure = base_type
                                              ? std::get_if<analysis::TypeClosure>(&base_type->node)
                                              : nullptr)
                {
                  std::vector<analysis::TypeFuncParam> params;
                  analysis::TypeFuncParam env_param;
                  env_param.mode = analysis::ParamMode::Move;
                  env_param.type = analysis::MakeTypePtr(
                      analysis::MakeTypePrim("u8"),
                      analysis::PtrState::Valid);
                  params.push_back(std::move(env_param));
                  for (const auto &[is_move, param_type] : closure->params)
                  {
                    analysis::TypeFuncParam p;
                    if (is_move)
                    {
                      p.mode = analysis::ParamMode::Move;
                    }
                    p.type = param_type;
                    params.push_back(std::move(p));
                  }
                  analysis::TypeFuncParam panic_param;
                  panic_param.mode = analysis::ParamMode::Move;
                  panic_param.type = PanicOutType();
                  params.push_back(std::move(panic_param));
                  callee_type = analysis::MakeTypeFunc(std::move(params), closure->ret);
                }
                else if (const auto *tuple = base_type
                                                 ? std::get_if<analysis::TypeTuple>(&base_type->node)
                                                 : nullptr)
                {
                  if (tuple->elements.size() == 2)
                  {
                    callee_type = analysis::StripPerm(tuple->elements[1]);
                    if (!callee_type)
                    {
                      callee_type = tuple->elements[1];
                    }
                  }
                }
              }
            }
          }
          if (!sig)
          {
            if (const auto *closure = callee_type
                                          ? std::get_if<analysis::TypeClosure>(&callee_type->node)
                                          : nullptr)
            {
              inferred_sig.params.clear();
              analysis::TypeFuncParam env_func_param;
              env_func_param.mode = analysis::ParamMode::Move;
              env_func_param.type = analysis::MakeTypePtr(
                  analysis::MakeTypePrim("u8"),
                  analysis::PtrState::Valid);
              IRParam env_ir_param;
              env_ir_param.mode = env_func_param.mode;
              env_ir_param.name = "env";
              env_ir_param.type = env_func_param.type;
              inferred_sig.params.push_back(std::move(env_ir_param));
              for (std::size_t i = 0; i < closure->params.size(); ++i)
              {
                IRParam p;
                if (closure->params[i].first)
                {
                  p.mode = analysis::ParamMode::Move;
                }
                p.name = "arg" + std::to_string(i);
                p.type = closure->params[i].second;
                inferred_sig.params.push_back(std::move(p));
              }
              // Closure calls lower as env + user args + hidden panic-out.
              if (call.args.size() == closure->params.size() + 2)
              {
                inferred_sig.params.push_back(PanicOutParam());
              }
              inferred_sig.ret = closure->ret;
              if (is_complete_sig(inferred_sig))
              {
                sig = &inferred_sig;
              }
            }
            else if (const auto *fn = callee_type
                                          ? std::get_if<analysis::TypeFunc>(&callee_type->node)
                                          : nullptr)
            {
              inferred_sig.params.clear();
              inferred_sig.params.reserve(fn->params.size());
              for (std::size_t i = 0; i < fn->params.size(); ++i)
              {
                IRParam p;
                p.mode = fn->params[i].mode;
                p.name = "arg" + std::to_string(i);
                p.type = fn->params[i].type;
                inferred_sig.params.push_back(std::move(p));
              }
              // Hidden panic-out is part of LoweredSigOf even when SigOf comes
              // from ExprType(callee)=TypeFunc(...). If lowering appended one extra
              // call argument, recover that ABI parameter here to keep call
              // signatures consistent for indirect procedure values.
              if (call.args.size() == inferred_sig.params.size() + 1)
              {
                inferred_sig.params.push_back(PanicOutParam());
              }
              inferred_sig.ret = fn->ret;
              if (is_complete_sig(inferred_sig))
              {
                sig = &inferred_sig;
              }
            }
          }

          // Recover concrete signatures for local procedure values bound from a
          // symbol (e.g. `let f: (i32,i32)->i32 = EqAdd; f(...)`). Local
          // binding states are popped after lowering; recover from the local
          // alloca store to keep indirect-call emission aligned with spec
          // LoweredSigOf/NeedsPanicOut requirements.
          if (!sig && call.callee.kind == IRValue::Kind::Local)
          {
            if (llvm::Value *local_slot = emitter.GetLocal(call.callee.name))
            {
              llvm::Function *stored_fn = nullptr;
              if (auto *alloca = llvm::dyn_cast<llvm::AllocaInst>(local_slot))
              {
                for (llvm::User *user : alloca->users())
                {
                  auto *store = llvm::dyn_cast<llvm::StoreInst>(user);
                  if (!store || store->getPointerOperand() != alloca)
                  {
                    continue;
                  }
                  if (llvm::Function *fn = FunctionFromLLVMValue(store->getValueOperand()))
                  {
                    stored_fn = fn;
                    break;
                  }
                }
              }
              else
              {
                stored_fn = FunctionFromLLVMValue(local_slot);
              }

              if (stored_fn)
              {
                const std::string stored_name = std::string(stored_fn->getName());
                if (const LowerCtx::ProcSigInfo *concrete = ctx->LookupProcSig(stored_name))
                {
                  sig = concrete;
                  callee_symbol = stored_name;
                }
              }
            }
          }
        }

        if (sig && emitter.RequiresHostedEnvParam(callee_symbol))
        {
          hosted_adjusted_sig = *sig;
          if (!HasLeadingHostedEnvParam(hosted_adjusted_sig.params))
          {
            hosted_adjusted_sig.params.insert(
                hosted_adjusted_sig.params.begin(),
                HostedEnvParam());
          }
          sig = &hosted_adjusted_sig;
          llvm::Value *env_arg = emitter.GetHostedCurrentEnvPtr();
          if (!env_arg)
          {
            env_arg = llvm::ConstantPointerNull::get(
                llvm::cast<llvm::PointerType>(emitter.GetOpaquePtr()));
          }
          args.insert(args.begin(), env_arg);
        }

          const bool raw_export_boundary =
              ctx && ctx->LookupExportUnwindMode(callee_symbol).has_value();
          if (sig && (sig->ffi_import || raw_export_boundary))
          {
            // Foreign-boundary-visible signatures must honor platform C ABI
            // aggregate-return lowering, including hidden sret where required.
            use_c_abi_aggregate_sret = true;
          }

          llvm::Value *callee = emitter.EvaluateIRValue(call.callee);
        if (!callee && call.callee.kind == IRValue::Kind::Symbol)
        {
          if (llvm::Function *existing = emitter.GetModule().getFunction(callee_symbol))
          {
            callee = existing;
          }
          else
          {
            llvm::FunctionType *decl_ty = nullptr;
            if (sig)
            {
              ABICallResult abi = emitter.ComputeCallABI(
                  sig->params, sig->ret, use_c_abi_aggregate_sret);
              decl_ty = abi.func_type;
            }
            if (!decl_ty && IsRuntimeFunction(callee_symbol))
            {
              llvm::Type *ret_ty = ExpectedLLVMType(call.result);
              if (!ret_ty)
              {
                ret_ty = llvm::Type::getVoidTy(emitter.GetContext());
              }
              std::vector<llvm::Type *> arg_tys;
              arg_tys.reserve(args.size());
              for (llvm::Value *arg : args)
              {
                arg_tys.push_back(arg ? arg->getType() : emitter.GetOpaquePtr());
              }
              decl_ty = llvm::FunctionType::get(ret_ty, arg_tys, false);
            }
            if (decl_ty)
            {
              llvm::Function *declared = llvm::Function::Create(
                  decl_ty,
                  llvm::GlobalValue::ExternalLinkage,
                  callee_symbol,
                  &emitter.GetModule());
              declared->setCallingConv(
                  sig ? CallingConvForAbi(sig->abi) : llvm::CallingConv::C);
              callee = declared;
            }
          }
        }

        const bool unresolved_drop_glue =
            call.callee.kind == IRValue::Kind::Symbol &&
            (IsDropGlueSymbol(callee_symbol) || IsDropGlueSymbol(call.callee.name));

        if (!callee)
        {
          if (!unresolved_drop_glue &&
              core::IsDebugEnabled("call"))
          {
            llvm::Function *caller_fn =
                builder.GetInsertBlock() ? builder.GetInsertBlock()->getParent() : nullptr;
            const std::string caller_name =
                caller_fn ? caller_fn->getName().str() : std::string("<no-func>");
            std::fprintf(stderr,
                         "[cursive] unresolved call: caller=%s kind=%d callee=%s resolved=%s args=%zu\n",
                         caller_name.c_str(),
                         static_cast<int>(call.callee.kind),
                         call.callee.name.c_str(),
                         callee_symbol.c_str(),
                         args.size());
          }
          if (!unresolved_drop_glue && ctx)
          {
            const_cast<LowerCtx *>(ctx)->ReportCodegenFailure();
          }
          emitter.SetTempValue(call.result, DefaultFor(call.result));
          return;
        }

        bool callee_is_closure_pair = false;
        if (IsClosurePairLLVMType(callee->getType()))
        {
          llvm::Value *env_ptr = builder.CreateExtractValue(callee, {0u});
          llvm::Value *code_ptr = builder.CreateExtractValue(callee, {1u});
          if (env_ptr && code_ptr)
          {
            args.insert(args.begin(), env_ptr);
            callee = code_ptr;
            callee_is_closure_pair = true;
          }
        }

        if (callee_is_closure_pair && sig &&
            sig->params.size() + 1 == args.size())
        {
          closure_adjusted_sig = *sig;
          IRParam env_param;
          env_param.mode = analysis::ParamMode::Move;
          env_param.name = "__env";
          env_param.type = analysis::MakeTypeRawPtr(
              analysis::RawPtrQual::Imm,
              analysis::MakeTypePrim("u8"));
          closure_adjusted_sig.params.insert(
              closure_adjusted_sig.params.begin(),
              std::move(env_param));
          sig = &closure_adjusted_sig;
        }

        if (!sig && ctx)
        {
          llvm::Function *fn = FunctionFromLLVMValue(callee);
          if (fn)
          {
            const std::string fn_name = std::string(fn->getName());
            sig = ctx->LookupProcSig(fn_name);
            if (!sig && !callee_symbol.empty())
            {
              sig = ctx->LookupProcSig(callee_symbol);
            }
          }
        }

        llvm::Value *call_result = nullptr;
        llvm::Value *call_result_storage = nullptr;
        llvm::Function *debug_current_fn =
            builder.GetInsertBlock() ? builder.GetInsertBlock()->getParent() : nullptr;
        const std::string debug_current_name =
            debug_current_fn ? debug_current_fn->getName().str() : std::string();
        const bool debug_primitives_fn =
            debug_current_name.find("PrimitivesLiterals") != std::string::npos ||
            debug_current_name.find("VerifyPrimitivesLiteralsItem") != std::string::npos;
        const bool debug_string_bytes_call =
            callee_symbol.find("string") != std::string::npos ||
            callee_symbol.find("bytes") != std::string::npos;
        const bool debug_call_trace =
            core::IsDebugEnabled("obj") &&
            (debug_primitives_fn || debug_string_bytes_call);
        if (debug_call_trace)
        {
          std::fprintf(stderr, "[llvm-call-debug] caller=%s callee=%s sig=%d arg_count=%zu\n",
                       debug_current_name.c_str(), callee_symbol.c_str(), sig ? 1 : 0, args.size());
          for (std::size_t i = 0; i < args.size(); ++i)
          {
            llvm::Type *aty = args[i] ? args[i]->getType() : nullptr;
            std::string aty_text;
            if (aty)
            {
              llvm::raw_string_ostream os(aty_text);
              aty->print(os);
              os.flush();
            }
            else
            {
              aty_text = "<null>";
            }
            std::fprintf(stderr, "[llvm-call-debug]   arg[%zu] llvm=%s\n", i, aty_text.c_str());
          }
        }
        if (call.callee.kind == IRValue::Kind::Symbol && ctx)
        {
          if (const auto *proc_module = ctx->LookupProcModule(callee_symbol))
          {
            SPEC_RULE("LowerIRInstr-CheckPoison");
            emitter.EmitPoisonCheck(core::StringOfPath(*proc_module));
          }
        }
        if (sig)
        {
          if (debug_call_trace)
          {
            std::fprintf(stderr, "[llvm-call-debug]   sig_param_count=%zu\n", sig->params.size());
            for (std::size_t i = 0; i < sig->params.size(); ++i)
            {
              const int mode_tag = sig->params[i].mode.has_value() ? static_cast<int>(*sig->params[i].mode) : -1;
              std::fprintf(stderr, "[llvm-call-debug]   sig_param[%zu] mode=%d type=%s\n",
                           i, mode_tag,
                           sig->params[i].type ? analysis::TypeToString(sig->params[i].type).c_str() : "<null>");
            }
          }
          llvm::Value *preferred_result_storage =
              emitter.TakePreferredResultStorage(call.result);
          const bool ffi_import_boundary = sig->ffi_import;
          const bool ffi_import_catch = ffi_import_boundary &&
              sig->ffi_import_unwind_mode ==
                  LowerCtx::FfiImportUnwindMode::Catch;
          const bool foreign_boundary_mode_independent =
              ffi_import_boundary || raw_export_boundary;
          call_result = EmitABICall(
              emitter,
              &builder,
              callee,
              sig->params,
              sig->ret,
              args,
              use_c_abi_aggregate_sret,
              ffi_import_boundary,
              ffi_import_catch,
              std::nullopt,
              &call.args,
              &call_result_storage,
              preferred_result_storage,
              foreign_boundary_mode_independent);
        }
        else
        {
          llvm::FunctionType *fn_ty = nullptr;
          if (auto *fn = llvm::dyn_cast<llvm::Function>(callee))
          {
            fn_ty = fn->getFunctionType();
          }
          if (fn_ty)
          {
            std::vector<llvm::Value *> coerced_args;
            coerced_args.reserve(fn_ty->getNumParams());
            for (unsigned i = 0; i < fn_ty->getNumParams(); ++i)
            {
              llvm::Type *param_ty = fn_ty->getParamType(i);
              llvm::Value *arg = i < args.size()
                                     ? args[i]
                                     : llvm::Constant::getNullValue(param_ty);
              arg = CoerceTo(&builder, arg, param_ty);
              if (!arg)
              {
                arg = llvm::Constant::getNullValue(param_ty);
              }
              coerced_args.push_back(arg);
            }
            llvm::CallInst *call_inst = builder.CreateCall(fn_ty, callee, coerced_args);
            if (auto *callee_fn = llvm::dyn_cast<llvm::Function>(callee))
            {
              call_inst->setCallingConv(callee_fn->getCallingConv());
            }
            if (!call_inst->getType()->isVoidTy())
            {
              call_result = call_inst;
            }
          }
        }

        const LowerCtx *active_ctx = emitter.GetCurrentCtx();
        analysis::TypeRef call_result_type =
            active_ctx ? active_ctx->LookupValueType(call.result) : nullptr;

        if ((call_result || call_result_storage) &&
            (callee_symbol == BuiltinSymAsyncResume() ||
             is_async_resume_runtime_symbol))
        {
          auto repack_async_resume_union =
              [&](llvm::Value *raw_result,
                  const analysis::TypeRef &expected_type) -> llvm::Value *
          {
            if (!raw_result || !expected_type || !active_ctx)
            {
              return raw_result;
            }

            const analysis::TypeRef stripped_target =
                ResolveAliasType(active_ctx, expected_type);
            const auto *target_union =
                stripped_target
                    ? std::get_if<analysis::TypeUnion>(&stripped_target->node)
                    : nullptr;
            if (!target_union || target_union->members.empty())
            {
              return raw_result;
            }

            const analysis::ScopeContext &scope = BuildScope(active_ctx);
            const auto union_layout = ::cursive::analysis::layout::UnionLayoutOf(scope, *target_union);
            if (!union_layout.has_value() || union_layout->niche)
            {
              return raw_result;
            }

            auto same_modal_path = [](const analysis::TypePath &lhs,
                                      const analysis::TypePath &rhs) -> bool
            {
              if (lhs.size() != rhs.size())
              {
                return false;
              }
              for (std::size_t i = 0; i < lhs.size(); ++i)
              {
                if (!analysis::IdEq(lhs[i], rhs[i]))
                {
                  return false;
                }
              }
              return true;
            };
            auto same_generic_args = [](const std::vector<analysis::TypeRef> &lhs,
                                        const std::vector<analysis::TypeRef> &rhs) -> bool
            {
              if (lhs.size() != rhs.size())
              {
                return false;
              }
              for (std::size_t i = 0; i < lhs.size(); ++i)
              {
                const auto eq = analysis::TypeEquiv(lhs[i], rhs[i]);
                if (!eq.ok || !eq.equiv)
                {
                  return false;
                }
              }
              return true;
            };

            struct AsyncMember
            {
              analysis::TypeRef type;
            };
            std::optional<analysis::TypePath> async_path;
            std::vector<analysis::TypeRef> async_args;
            std::optional<AsyncMember> suspended_member;
            std::optional<AsyncMember> completed_member;
            std::optional<AsyncMember> failed_member;

            for (const auto &member : union_layout->member_list)
            {
              analysis::TypeRef stripped_member = analysis::StripPerm(member);
              const auto *modal_state =
                  stripped_member
                      ? std::get_if<analysis::TypeModalState>(&stripped_member->node)
                      : nullptr;
              if (!modal_state ||
                  !analysis::IsAsyncModalPath(modal_state->path))
              {
                return raw_result;
              }

              if (!async_path.has_value())
              {
                async_path = modal_state->path;
                async_args = modal_state->generic_args;
              }
              else if (!same_modal_path(*async_path, modal_state->path) ||
                       !same_generic_args(async_args, modal_state->generic_args))
              {
                return raw_result;
              }

              if (analysis::IdEq(modal_state->state, "Suspended"))
              {
                suspended_member = AsyncMember{stripped_member};
              }
              else if (analysis::IdEq(modal_state->state, "Completed"))
              {
                completed_member = AsyncMember{stripped_member};
              }
              else if (analysis::IdEq(modal_state->state, "Failed"))
              {
                failed_member = AsyncMember{stripped_member};
              }
            }

            if (!suspended_member.has_value() &&
                !completed_member.has_value() &&
                !failed_member.has_value())
            {
              return raw_result;
            }

            llvm::Type *target_ll = ExpectedLLVMType(call.result);
            if (!target_ll)
            {
              target_ll = emitter.GetLLVMType(stripped_target);
            }
            if (!target_ll)
            {
              return raw_result;
            }

            auto pack_member = [&](const std::optional<AsyncMember> &member_opt) -> llvm::Value *
            {
              if (!member_opt.has_value() || !member_opt->type)
              {
                return nullptr;
              }
              return PackUnionFromMember(
                  emitter,
                  &builder,
                  raw_result,
                  target_ll,
                  member_opt->type,
                  stripped_target);
            };

            llvm::Value *packed_suspended = pack_member(suspended_member);
            llvm::Value *packed_completed = pack_member(completed_member);
            llvm::Value *packed_failed = pack_member(failed_member);

            llvm::Value *raw_disc = nullptr;
            if (raw_result->getType()->isIntegerTy())
            {
              raw_disc = raw_result;
            }
            else if (auto *raw_struct =
                         llvm::dyn_cast<llvm::StructType>(raw_result->getType());
                     raw_struct && raw_struct->getNumElements() >= 1 &&
                     raw_struct->getElementType(0)->isIntegerTy())
            {
              raw_disc = builder.CreateExtractValue(raw_result, {0u});
            }
            if (!raw_disc || !raw_disc->getType()->isIntegerTy())
            {
              return raw_result;
            }

            analysis::TypeRef async_runtime_type = nullptr;
            if (async_path.has_value())
            {
              async_runtime_type =
                  analysis::MakeTypePath(*async_path, async_args);
            }
            const AsyncStateDiscs async_discs =
                LoweredAsyncStateDiscs(scope, async_runtime_type);
            const std::uint64_t suspended_disc = async_discs.suspended;
            const std::uint64_t completed_disc = async_discs.completed;
            const std::optional<std::uint64_t> failed_disc = async_discs.failed;

            llvm::Value *mapped = nullptr;
            if (packed_failed)
            {
              mapped = packed_failed;
            }
            else if (packed_completed)
            {
              mapped = packed_completed;
            }
            else if (packed_suspended)
            {
              mapped = packed_suspended;
            }
            else
            {
              mapped = CoerceTo(&builder, raw_result, target_ll);
              if (!mapped)
              {
                mapped = llvm::Constant::getNullValue(target_ll);
              }
            }

            auto select_state = [&](llvm::Value *packed_state, std::uint64_t disc_value)
            {
              if (!packed_state)
              {
                return;
              }
              llvm::Value *is_state = EmitTypedEq(
                  &builder,
                  raw_disc,
                  llvm::ConstantInt::get(raw_disc->getType(), disc_value));
              mapped = builder.CreateSelect(AsBool(&builder, is_state), packed_state, mapped);
            };

            select_state(packed_suspended, suspended_disc);
            select_state(packed_completed, completed_disc);
            if (failed_disc.has_value())
            {
              select_state(packed_failed, *failed_disc);
            }
            return mapped;
          };

          llvm::Value *raw_async_result = call_result;
          if (!raw_async_result && call_result_storage && sig && sig->ret)
          {
            if (llvm::Type *raw_ty = emitter.GetLLVMType(sig->ret))
            {
              raw_async_result = builder.CreateLoad(raw_ty, call_result_storage);
            }
          }
          llvm::Value *repacked =
              repack_async_resume_union(raw_async_result, call_result_type);
          if (repacked && repacked != raw_async_result && call_result_storage)
          {
            llvm::Type *target_ty = ExpectedLLVMType(call.result);
            if (!target_ty && call_result_type)
            {
              target_ty = emitter.GetLLVMType(call_result_type);
            }
            if (target_ty)
            {
              llvm::Value *store_value = CoerceTo(&builder, repacked, target_ty);
              if (!store_value)
              {
                store_value = repacked;
              }
              builder.CreateStore(store_value, call_result_storage);
            }
          }
          call_result = repacked;
        }

        bool never_call = false;
        if (sig && IsNeverType(sig->ret))
        {
          never_call = true;
        }
        else if (call_result_type)
        {
          never_call = IsNeverType(call_result_type);
        }

        if (debug_call_trace)
        {
          const std::string sig_ret_text =
              (sig && sig->ret) ? analysis::TypeToString(sig->ret) : std::string("<none>");
          const std::string result_ty_text =
              call_result_type ? analysis::TypeToString(call_result_type) : std::string("<none>");
          std::fprintf(stderr, "[llvm-call-debug]   sig_ret=%s result_type=%s never=%d\n",
                       sig_ret_text.c_str(), result_ty_text.c_str(), never_call ? 1 : 0);
        }

        if (call_result_storage)
        {
          emitter.SetTempStorage(call.result, call_result_storage);
        }
        if (!call_result && !call_result_storage)
        {
          call_result = DefaultFor(call.result);
        }
        if (call_result)
        {
          emitter.SetTempValue(call.result, call_result);
        }

        if (never_call && !builder.GetInsertBlock()->getTerminator())
        {
          builder.CreateUnreachable();
        }
      }

      void operator()(const IRUnaryOp &unary) const
      {
        llvm::Value *operand = EvaluateOrDefault(unary.operand);
        llvm::Value *result = nullptr;
        if (unary.op == "!")
        {
          const LowerCtx *active_ctx = emitter.GetCurrentCtx();
          analysis::TypeRef operand_type =
              active_ctx ? ResolveAliasType(active_ctx, active_ctx->LookupValueType(unary.operand))
                         : nullptr;
          analysis::TypeRef result_type =
              active_ctx ? ResolveAliasType(active_ctx, active_ctx->LookupValueType(unary.result))
                         : nullptr;
          const bool logical_not =
              IsBoolType(operand_type) || IsBoolType(result_type) ||
              operand->getType()->isIntegerTy(1);

          if (logical_not)
          {
            // bool `!` must invert truthiness (0/1), not payload bits.
            // i8 bools require canonicalization through AsBool first.
            result = builder.CreateNot(AsBool(&builder, operand));
          }
          else if (operand->getType()->isIntegerTy())
          {
            // Integer `!` is bitwise not per spec.
            result = builder.CreateNot(operand);
          }
          else
          {
            // Defensive fallback for non-integer truthy values.
            result = builder.CreateNot(AsBool(&builder, operand));
          }
        }
        else if (unary.op == "-" && operand->getType()->isIntegerTy())
        {
          result = builder.CreateNeg(operand);
        }
        else if (unary.op == "-" && operand->getType()->isFloatingPointTy())
        {
          result = builder.CreateFNeg(operand);
        }
        else if (unary.op == "~" && operand->getType()->isIntegerTy())
        {
          result = builder.CreateNot(operand);
        }
        else if (unary.op == "widen")
        {
          const LowerCtx *active_ctx = emitter.GetCurrentCtx();
          analysis::TypeRef operand_type =
              active_ctx ? StripPermType(active_ctx->LookupValueType(unary.operand)) : nullptr;
          analysis::TypeRef result_type =
              active_ctx ? StripPermType(active_ctx->LookupValueType(unary.result)) : nullptr;
          if (!operand_type)
          {
            operand_type = StripPermType(unary.operand_type);
          }
          if (!result_type)
          {
            result_type = StripPermType(unary.result_type);
          }

          const auto *modal_state =
              operand_type ? std::get_if<analysis::TypeModalState>(&operand_type->node) : nullptr;
          const auto *modal_ref =
              result_type ? std::get_if<analysis::TypePathType>(&result_type->node) : nullptr;

          auto bitcopy_to_type = [&](llvm::Value *src, llvm::Type *dst_ty) -> llvm::Value *
          {
            if (!src || !dst_ty)
            {
              return nullptr;
            }
            llvm::Function *current_fn =
                builder.GetInsertBlock() ? builder.GetInsertBlock()->getParent() : nullptr;
            if (!current_fn)
            {
              return nullptr;
            }
            llvm::IRBuilder<> entry_builder(
                &current_fn->getEntryBlock(),
                current_fn->getEntryBlock().begin());
            llvm::AllocaInst *dst_slot = entry_builder.CreateAlloca(dst_ty);
            llvm::AllocaInst *src_slot = entry_builder.CreateAlloca(src->getType());
            builder.CreateStore(src, src_slot);

            llvm::Type *i8_ty = llvm::Type::getInt8Ty(emitter.GetContext());
            llvm::Type *i64_ty = llvm::Type::getInt64Ty(emitter.GetContext());
            llvm::Value *dst_i8 = builder.CreateBitCast(dst_slot, llvm::PointerType::get(i8_ty, 0));
            llvm::Value *src_i8 = builder.CreateBitCast(src_slot, llvm::PointerType::get(i8_ty, 0));

            const llvm::DataLayout &dl = emitter.GetModule().getDataLayout();
            const std::uint64_t src_size =
                static_cast<std::uint64_t>(dl.getTypeAllocSize(src->getType()));
            const std::uint64_t dst_size =
                static_cast<std::uint64_t>(dl.getTypeAllocSize(dst_ty));
            const std::uint64_t copy_size = std::min(src_size, dst_size);
            if (copy_size > 0)
            {
              builder.CreateMemCpy(
                  dst_i8,
                  llvm::Align(1),
                  src_i8,
                  llvm::Align(1),
                  llvm::ConstantInt::get(i64_ty, copy_size));
            }
            return builder.CreateLoad(dst_ty, dst_slot);
          };

          if (active_ctx && modal_state)
          {
            const analysis::ScopeContext &scope = BuildScope(active_ctx);
            const ast::ModalDecl *modal_decl =
                analysis::LookupModalDecl(scope, modal_state->path);
            if (!modal_decl && modal_ref)
            {
              modal_decl = analysis::LookupModalDecl(scope, modal_ref->path);
            }
            if (modal_decl)
            {
              llvm::Type *target_ty = ExpectedLLVMType(unary.result);
              if (!target_ty && result_type)
              {
                target_ty = emitter.GetLLVMType(result_type);
              }
              if (!target_ty)
              {
                target_ty = operand ? operand->getType() : nullptr;
              }

              if (const auto modal_layout = ::cursive::analysis::layout::ModalLayoutOf(scope, *modal_decl, modal_state->generic_args))
              {
                if (modal_layout->disc_type.has_value())
                {
                  auto *target_struct = llvm::dyn_cast_or_null<llvm::StructType>(target_ty);
                  if (target_struct && target_struct->getNumElements() >= 1)
                  {
                    std::optional<std::uint64_t> state_index;
                    for (std::size_t i = 0; i < modal_decl->states.size(); ++i)
                    {
                      if (analysis::IdEq(modal_decl->states[i].name, modal_state->state))
                      {
                        state_index = static_cast<std::uint64_t>(i);
                        break;
                      }
                    }

                    llvm::Function *current_fn =
                        builder.GetInsertBlock() ? builder.GetInsertBlock()->getParent() : nullptr;
                    if (state_index.has_value() && current_fn)
                    {
                      llvm::IRBuilder<> entry_builder(
                          &current_fn->getEntryBlock(),
                          current_fn->getEntryBlock().begin());
                      llvm::AllocaInst *dst_slot = entry_builder.CreateAlloca(target_struct);
                      builder.CreateStore(llvm::Constant::getNullValue(target_struct), dst_slot);

                      llvm::Value *disc_ptr = builder.CreateStructGEP(target_struct, dst_slot, 0);
                      llvm::Type *disc_ty = target_struct->getElementType(0);
                      llvm::Value *disc_value = disc_ty->isIntegerTy()
                                                    ? llvm::ConstantInt::get(disc_ty, *state_index)
                                                    : nullptr;
                      if (!disc_value)
                      {
                        disc_value = llvm::Constant::getNullValue(disc_ty);
                      }
                      builder.CreateStore(disc_value, disc_ptr);

                      llvm::Value *payload_i8 = CreateTaggedPayloadI8Ptr(
                          emitter,
                          &builder,
                          target_struct,
                          dst_slot,
                          modal_layout->payload_align);
                      if (payload_i8 && operand)
                      {
                        llvm::AllocaInst *src_slot = entry_builder.CreateAlloca(operand->getType());
                        builder.CreateStore(operand, src_slot);

                        llvm::Type *i8_ty = llvm::Type::getInt8Ty(emitter.GetContext());
                        llvm::Type *i64_ty = llvm::Type::getInt64Ty(emitter.GetContext());
                        llvm::Value *src_i8 = builder.CreateBitCast(
                            src_slot,
                            llvm::PointerType::get(i8_ty, 0));

                        const llvm::DataLayout &dl = emitter.GetModule().getDataLayout();
                        const std::uint64_t src_size =
                            static_cast<std::uint64_t>(dl.getTypeAllocSize(operand->getType()));
                        const std::uint64_t copy_size =
                            std::min(src_size, modal_layout->payload_size);
                        if (copy_size > 0)
                        {
                          builder.CreateMemCpy(
                              payload_i8,
                              llvm::Align(1),
                              src_i8,
                              llvm::Align(1),
                              llvm::ConstantInt::get(i64_ty, copy_size));
                        }
                      }
                      result = builder.CreateLoad(target_struct, dst_slot);
                    }
                  }
                }
                else if (target_ty)
                {
                  result = bitcopy_to_type(operand, target_ty);
                }
              }
            }
          }
        }
        if (!result)
        {
          result = DefaultFor(unary.result);
        }
        if (llvm::Type *expected = ExpectedLLVMType(unary.result))
        {
          result = CoerceTo(&builder, result, expected);
        }
        emitter.SetTempValue(unary.result, result);
      }

      void operator()(const IRFence &fence) const
      {
        llvm::AtomicOrdering ordering = llvm::AtomicOrdering::SequentiallyConsistent;
        switch (fence.order)
        {
        case IRFenceOrder::Acquire:
          ordering = llvm::AtomicOrdering::Acquire;
          break;
        case IRFenceOrder::Release:
          ordering = llvm::AtomicOrdering::Release;
          break;
        case IRFenceOrder::SeqCst:
          ordering = llvm::AtomicOrdering::SequentiallyConsistent;
          break;
        }
        builder.CreateFence(ordering);
        emitter.SetTempValue(fence.result, DefaultFor(fence.result));
      }

      void operator()(const IRBinaryOp &bin) const
      {
        llvm::Function *debug_fn =
            builder.GetInsertBlock() ? builder.GetInsertBlock()->getParent() : nullptr;
        const std::string debug_fn_name =
            debug_fn ? debug_fn->getName().str() : std::string();
        const bool debug_binop = core::IsDebugEnabled("binop") &&
                                 debug_fn_name.find("PropagationMaybeDouble") != std::string::npos;
        const bool debug_contract_binop = core::IsDebugEnabled("return") &&
                                          debug_fn_name.find("ContractPredicateIntegrationShift") != std::string::npos;
        if (debug_binop)
        {
          const LowerCtx *active_ctx = emitter.GetCurrentCtx();
          analysis::TypeRef lhs_ty = active_ctx ? active_ctx->LookupValueType(bin.lhs) : nullptr;
          analysis::TypeRef rhs_ty = active_ctx ? active_ctx->LookupValueType(bin.rhs) : nullptr;
          analysis::TypeRef res_ty = active_ctx ? active_ctx->LookupValueType(bin.result) : nullptr;
          std::cerr << "[binop-debug] fn=" << debug_fn_name
                    << " op=" << bin.op
                    << " result=" << bin.result.name
                    << " lhs_kind=" << static_cast<int>(bin.lhs.kind)
                    << " lhs_name=" << bin.lhs.name
                    << " lhs_type=" << (lhs_ty ? analysis::TypeToString(lhs_ty) : std::string("<null>"))
                    << " rhs_kind=" << static_cast<int>(bin.rhs.kind)
                    << " rhs_name=" << bin.rhs.name
                    << " rhs_type=" << (rhs_ty ? analysis::TypeToString(rhs_ty) : std::string("<null>"))
                    << " result_type=" << (res_ty ? analysis::TypeToString(res_ty) : std::string("<null>"))
                    << "\n";
        }

        // Do not fold immediate equality from raw IR lexeme/byte payload equality.
        // Distinct immediate construction paths can encode equivalent typed values
        // with different byte widths (for example i32 vs i64 literal payload width),
        // and raw-byte folding is unsound. Evaluate both sides and use typed equality
        // so coercion rules decide semantic equivalence.

        llvm::Value *lhs = EvaluateOrDefault(bin.lhs);
        llvm::Value *rhs = EvaluateOrDefault(bin.rhs);
        llvm::Value *result = nullptr;

        if (lhs->getType() != rhs->getType())
        {
          if (lhs->getType()->isIntegerTy() && rhs->getType()->isIntegerTy())
          {
            unsigned width =
                std::max(lhs->getType()->getIntegerBitWidth(),
                         rhs->getType()->getIntegerBitWidth());
            llvm::Type *common = llvm::Type::getIntNTy(emitter.GetContext(), width);
            lhs = CoerceTo(&builder, lhs, common);
            rhs = CoerceTo(&builder, rhs, common);
          }
          else if (lhs->getType()->isPointerTy() && rhs->getType()->isPointerTy())
          {
            llvm::Type *common = emitter.GetOpaquePtr();
            lhs = CoerceTo(&builder, lhs, common);
            rhs = CoerceTo(&builder, rhs, common);
          }
          else
          {
            rhs = CoerceTo(&builder, rhs, lhs->getType());
          }
        }

        const LowerCtx *type_ctx = emitter.GetCurrentCtx();
        analysis::TypeRef lhs_type = nullptr;
        analysis::TypeRef rhs_type = nullptr;
        if (type_ctx)
        {
          lhs_type = type_ctx->LookupValueType(bin.lhs);
          rhs_type = type_ctx->LookupValueType(bin.rhs);
        }
        if (!lhs_type && bin.lhs.kind == IRValue::Kind::Local)
        {
          lhs_type = emitter.LookupLocalType(bin.lhs.name);
        }
        if (!rhs_type && bin.rhs.kind == IRValue::Kind::Local)
        {
          rhs_type = emitter.LookupLocalType(bin.rhs.name);
        }
        auto is_integer_type = [](const analysis::TypeRef &type) -> bool
        {
          analysis::TypeRef stripped = analysis::StripPerm(type);
          if (!stripped)
          {
            return false;
          }
          auto *prim = std::get_if<analysis::TypePrim>(&stripped->node);
          if (!prim)
          {
            return false;
          }
          const std::string &name = prim->name;
          if (name == "isize" || name == "usize")
          {
            return true;
          }
          return !name.empty() && (name[0] == 'i' || name[0] == 'u');
        };
        const bool int_ops_signed = [&]() -> bool
        {
          if (lhs_type && is_integer_type(lhs_type))
          {
            return IsSignedIntegerType(lhs_type);
          }
          if (rhs_type && is_integer_type(rhs_type))
          {
            return IsSignedIntegerType(rhs_type);
          }
          return true;
        }();

        const std::string &op = bin.op;
        if (op == "+")
        {
          result = lhs->getType()->isFloatingPointTy()
                       ? builder.CreateFAdd(lhs, rhs)
                       : emitter.EmitCheckedAdd(lhs, rhs, int_ops_signed);
        }
        else if (op == "-")
        {
          result = lhs->getType()->isFloatingPointTy()
                       ? builder.CreateFSub(lhs, rhs)
                       : emitter.EmitCheckedSub(lhs, rhs, int_ops_signed);
        }
        else if (op == "*")
        {
          result = lhs->getType()->isFloatingPointTy()
                       ? builder.CreateFMul(lhs, rhs)
                       : emitter.EmitCheckedMul(lhs, rhs, int_ops_signed);
        }
        else if (op == "**")
        {
          if (lhs->getType()->isFloatingPointTy() &&
              lhs->getType() == rhs->getType())
          {
            llvm::Function *pow_fn = llvm::Intrinsic::getDeclaration(
                &emitter.GetModule(), llvm::Intrinsic::pow, lhs->getType());
            if (pow_fn)
            {
              result = builder.CreateCall(pow_fn, {lhs, rhs});
            }
          }
          else if (lhs->getType()->isIntegerTy() &&
                   rhs->getType()->isIntegerTy())
          {
            llvm::Function *current_fn =
                builder.GetInsertBlock() ? builder.GetInsertBlock()->getParent() : nullptr;
            if (current_fn)
            {
              const bool signed_int = [&]() -> bool
              {
                if (analysis::TypeRef lhs_type = LookupValueType(bin.lhs))
                {
                  return IsSignedIntegerType(lhs_type);
                }
                return true;
              }();

              llvm::Type *int_ty = lhs->getType();
              llvm::IRBuilder<> entry_builder(
                  &current_fn->getEntryBlock(),
                  current_fn->getEntryBlock().begin());
              llvm::AllocaInst *result_slot = entry_builder.CreateAlloca(int_ty);
              llvm::AllocaInst *exp_slot = entry_builder.CreateAlloca(rhs->getType());

              builder.CreateStore(llvm::ConstantInt::get(int_ty, 1), result_slot);
              builder.CreateStore(rhs, exp_slot);

              if (signed_int)
              {
                llvm::Value *exp_start = builder.CreateLoad(rhs->getType(), exp_slot);
                llvm::Value *non_negative = builder.CreateICmpSGE(
                    exp_start, llvm::ConstantInt::get(rhs->getType(), 0));
                EmitPanicReturnIfFalse(
                    emitter,
                    &builder,
                    non_negative,
                    PanicCode(PanicReason::Overflow));
              }

              llvm::BasicBlock *loop_cond =
                  llvm::BasicBlock::Create(emitter.GetContext(), "pow.cond", current_fn);
              llvm::BasicBlock *loop_body =
                  llvm::BasicBlock::Create(emitter.GetContext(), "pow.body", current_fn);
              llvm::BasicBlock *loop_done =
                  llvm::BasicBlock::Create(emitter.GetContext(), "pow.done", current_fn);

              builder.CreateBr(loop_cond);

              builder.SetInsertPoint(loop_cond);
              llvm::Value *exp_curr = builder.CreateLoad(rhs->getType(), exp_slot);
              llvm::Value *exp_is_zero = builder.CreateICmpEQ(
                  exp_curr, llvm::ConstantInt::get(rhs->getType(), 0));
              builder.CreateCondBr(exp_is_zero, loop_done, loop_body);

              builder.SetInsertPoint(loop_body);
              llvm::Value *res_curr = builder.CreateLoad(int_ty, result_slot);
              llvm::Intrinsic::ID mul_intrinsic = signed_int
                                                      ? llvm::Intrinsic::smul_with_overflow
                                                      : llvm::Intrinsic::umul_with_overflow;
              llvm::Function *mul_fn = llvm::Intrinsic::getDeclaration(
                  &emitter.GetModule(), mul_intrinsic, int_ty);
              llvm::Value *mul_pair = builder.CreateCall(mul_fn, {res_curr, lhs});
              llvm::Value *mul_value = builder.CreateExtractValue(mul_pair, {0u});
              llvm::Value *mul_overflow = builder.CreateExtractValue(mul_pair, {1u});
              llvm::Value *no_overflow = builder.CreateNot(mul_overflow);
              EmitPanicReturnIfFalse(
                  emitter,
                  &builder,
                  no_overflow,
                  PanicCode(PanicReason::Overflow));
              builder.CreateStore(mul_value, result_slot);

              llvm::Value *exp_next = builder.CreateSub(
                  exp_curr, llvm::ConstantInt::get(rhs->getType(), 1));
              builder.CreateStore(exp_next, exp_slot);
              builder.CreateBr(loop_cond);

              builder.SetInsertPoint(loop_done);
              result = builder.CreateLoad(int_ty, result_slot);
            }
          }
        }
        else if (op == "/")
        {
          result = lhs->getType()->isIntegerTy()
                       ? emitter.EmitCheckedDiv(lhs, rhs, int_ops_signed)
                       : builder.CreateFDiv(lhs, rhs);
        }
        else if (op == "%")
        {
          result = lhs->getType()->isIntegerTy()
                       ? emitter.EmitCheckedRem(lhs, rhs, int_ops_signed)
                       : builder.CreateFRem(lhs, rhs);
        }
        else if (op == "==" || op == "===")
        {
          result = EmitTypedEq(&builder, lhs, rhs);
        }
        else if (op == "!=")
        {
          llvm::Value *eq = EmitTypedEq(&builder, lhs, rhs);
          result = builder.CreateNot(AsBool(&builder, eq));
        }
        else if (op == "<")
        {
          result = lhs->getType()->isFloatingPointTy() ? builder.CreateFCmpOLT(lhs, rhs)
                                                       : (int_ops_signed
                                                              ? builder.CreateICmpSLT(lhs, rhs)
                                                              : builder.CreateICmpULT(lhs, rhs));
        }
        else if (op == "<=")
        {
          result = lhs->getType()->isFloatingPointTy() ? builder.CreateFCmpOLE(lhs, rhs)
                                                       : (int_ops_signed
                                                              ? builder.CreateICmpSLE(lhs, rhs)
                                                              : builder.CreateICmpULE(lhs, rhs));
        }
        else if (op == ">")
        {
          result = lhs->getType()->isFloatingPointTy() ? builder.CreateFCmpOGT(lhs, rhs)
                                                       : (int_ops_signed
                                                              ? builder.CreateICmpSGT(lhs, rhs)
                                                              : builder.CreateICmpUGT(lhs, rhs));
        }
        else if (op == ">=")
        {
          result = lhs->getType()->isFloatingPointTy() ? builder.CreateFCmpOGE(lhs, rhs)
                                                       : (int_ops_signed
                                                              ? builder.CreateICmpSGE(lhs, rhs)
                                                              : builder.CreateICmpUGE(lhs, rhs));
        }
        else if (op == "&")
        {
          result = builder.CreateAnd(lhs, rhs);
        }
        else if (op == "|")
        {
          result = builder.CreateOr(lhs, rhs);
        }
        else if (op == "^")
        {
          result = builder.CreateXor(lhs, rhs);
        }
        else if (op == "<<")
        {
          result = emitter.EmitCheckedShl(lhs, rhs);
        }
        else if (op == ">>")
        {
          result = emitter.EmitCheckedShr(lhs, rhs, int_ops_signed);
        }
        else if (op == "&&")
        {
          result = builder.CreateAnd(AsBool(&builder, lhs), AsBool(&builder, rhs));
        }
        else if (op == "||")
        {
          result = builder.CreateOr(AsBool(&builder, lhs), AsBool(&builder, rhs));
        }

        if (!result)
        {
          result = DefaultFor(bin.result);
        }
        const LowerCtx *active_ctx = emitter.GetCurrentCtx();
        analysis::TypeRef target_type =
            active_ctx ? active_ctx->LookupValueType(bin.result) : nullptr;
        analysis::TypeRef source_type = nullptr;
        if (IsBoolBinOp(bin.op))
        {
          source_type = analysis::MakeTypePrim("bool");
        }
        else
        {
          if (active_ctx)
          {
            source_type = active_ctx->LookupValueType(bin.lhs);
            if (!source_type)
            {
              source_type = active_ctx->LookupValueType(bin.rhs);
            }
          }
          if (!source_type && bin.lhs.kind == IRValue::Kind::Local)
          {
            source_type = emitter.LookupLocalType(bin.lhs.name);
          }
          if (!source_type && bin.rhs.kind == IRValue::Kind::Local)
          {
            source_type = emitter.LookupLocalType(bin.rhs.name);
          }
        }
        if (llvm::Type *expected = ExpectedLLVMType(bin.result))
        {
          if (IsBoolBinOp(bin.op))
          {
            // Boolean operators are semantically boolean. Keep their value in a
            // scalar boolean representation even if stale type metadata points to
            // an unrelated non-boolean type.
            result = AsBool(&builder, result);
            // Never coerce bool operators to pointer-typed temporaries. The
            // generic coercion path materializes null for int->ptr and can force
            // conditionals to constant-false despite a true comparison result.
            if (expected->isIntegerTy())
            {
              if (llvm::Value *coerced = CoerceTo(&builder, result, expected))
              {
                result = coerced;
              }
            }
          }
          else if (target_type)
          {
            llvm::Value *coerced = CoerceToTyped(
                emitter,
                &builder,
                result,
                expected,
                source_type,
                target_type);
            if (coerced)
            {
              result = coerced;
            }
            else
            {
              result = llvm::Constant::getNullValue(expected);
            }
          }
          else
          {
            result = CoerceTo(&builder, result, expected);
          }
        }
        emitter.SetTempValue(bin.result, result);
        if (debug_contract_binop)
        {
          analysis::TypeRef result_type = nullptr;
          if (const LowerCtx *active_ctx = emitter.GetCurrentCtx())
          {
            result_type = active_ctx->LookupValueType(bin.result);
          }
          std::string llvm_ty = "<null>";
          if (result && result->getType())
          {
            std::string llvm_ty_buf;
            llvm::raw_string_ostream os(llvm_ty_buf);
            result->getType()->print(os);
            os.flush();
            llvm_ty = llvm_ty_buf;
          }
          bool is_const = false;
          bool const_bool = false;
          if (auto *cint = llvm::dyn_cast<llvm::ConstantInt>(result))
          {
            is_const = true;
            const_bool = cint->getValue().isOne();
          }
          std::cerr << "[llvm-binop-debug] fn=" << debug_fn_name
                    << " op=" << bin.op
                    << " result=" << bin.result.name
                    << " result_type="
                    << (result_type ? analysis::TypeToString(result_type)
                                    : std::string("<null>"))
                    << " llvm_ty=" << llvm_ty
                    << " is_const=" << (is_const ? "1" : "0");
          if (is_const)
          {
            std::cerr << " const_bool=" << (const_bool ? "1" : "0");
          }
          std::cerr << "\n";
        }
      }

      void operator()(const IRIf &node) const
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

      void operator()(const IRReturn &ret) const
      {
        llvm::Function *func = builder.GetInsertBlock()->getParent();
        llvm::Type *ret_ty = func->getReturnType();
        const LowerCtx *ctx = emitter.GetCurrentCtx();
        const std::string sym = std::string(func->getName());
        const LowerCtx::ProcSigInfo *sig = ctx ? ctx->LookupProcSig(sym) : nullptr;
        analysis::TypeRef source_type = ctx ? ctx->LookupValueType(ret.value) : nullptr;
        const bool debug_return = core::IsDebugEnabled("return") &&
                                  sym.find("PropagationMaybeDouble") != std::string::npos;
        if (debug_return)
        {
          std::string ret_ty_text;
          if (ret_ty)
          {
            llvm::raw_string_ostream os(ret_ty_text);
            ret_ty->print(os);
            os.flush();
          }
          else
          {
            ret_ty_text = "<null>";
          }
          std::cerr << "[return-debug] fn=" << sym
                    << " ret_kind=" << static_cast<int>(ret.value.kind)
                    << " ret_name=" << ret.value.name
                    << " source_type="
                    << (source_type ? analysis::TypeToString(source_type) : std::string("<null>"))
                    << " sig_ret="
                    << (sig && sig->ret ? analysis::TypeToString(sig->ret) : std::string("<null>"))
                    << " llvm_ret=" << ret_ty_text << "\n";
        }

        if (ret_ty->isVoidTy())
        {
          if (sig && sig->ret)
          {
            llvm::Value *out_ptr =
                ResolveProcedureOutPtr(emitter, &builder, func, sym, sig);
            llvm::Value *source_storage = emitter.GetAddressableStorage(ret.value);
            if (out_ptr && source_storage)
            {
              llvm::Value *normalized_source = source_storage;
              llvm::Type *out_ty = emitter.GetLLVMType(sig->ret);
              llvm::Type *target_ptr_ty =
                  out_ty ? llvm::PointerType::get(out_ty, 0) : nullptr;
              if (target_ptr_ty)
              {
                if (normalized_source->getType()->isIntegerTy())
                {
                  normalized_source =
                      builder.CreateIntToPtr(normalized_source, target_ptr_ty);
                }
                else
                {
                  llvm::Value *coerced =
                      CoerceTo(&builder, normalized_source, target_ptr_ty);
                  if (coerced)
                  {
                    normalized_source = coerced;
                  }
                  else if (normalized_source->getType()->isPointerTy())
                  {
                    normalized_source =
                        builder.CreateBitCast(normalized_source, target_ptr_ty);
                  }
                }
              }

              if (normalized_source && normalized_source->getType()->isPointerTy() &&
                  out_ptr->stripPointerCasts() ==
                      normalized_source->stripPointerCasts())
              {
                builder.CreateRetVoid();
                return;
              }
            }
          }

          llvm::Value *value = EvaluateOrDefault(ret.value);
          if (debug_return && value)
          {
            std::string value_ty_text;
            llvm::raw_string_ostream os(value_ty_text);
            value->getType()->print(os);
            os.flush();
            std::cerr << "[return-debug] fn=" << sym
                      << " pre-coerce llvm_value_ty=" << value_ty_text << "\n";
          }
          (void)StoreProcedureOutValue(
              emitter,
              &builder,
              func,
              sym,
              sig,
              value,
              source_type);
          builder.CreateRetVoid();
          return;
        }
        llvm::Value *value = EvaluateOrDefault(ret.value);
        if (debug_return && value)
        {
          std::string value_ty_text;
          llvm::raw_string_ostream os(value_ty_text);
          value->getType()->print(os);
          os.flush();
          std::cerr << "[return-debug] fn=" << sym
                    << " pre-coerce llvm_value_ty=" << value_ty_text << "\n";
        }
        value = CoerceToTyped(
            emitter,
            &builder,
            value,
            ret_ty,
            source_type,
            sig ? sig->ret : nullptr);
        if (!value)
        {
          value = llvm::Constant::getNullValue(ret_ty);
        }
        if (debug_return && value)
        {
          std::string value_ty_text;
          llvm::raw_string_ostream os(value_ty_text);
          value->getType()->print(os);
          os.flush();
          std::cerr << "[return-debug] fn=" << sym
                    << " post-coerce llvm_value_ty=" << value_ty_text << "\n";
        }
        builder.CreateRet(value);
      }

      void operator()(const IRResult &result) const
      {
        SetForwardedOrMaterializedResult(result.value);
      }

      void operator()(const IRClearPanic &) const
      {
        ClearPanicRecord(emitter, &builder);
      }

      void operator()(const IRPanicCheck &check) const
      {
        llvm::Value *panic_ptr = LoadPanicOutPtr(emitter, &builder);
        if (!panic_ptr)
        {
          return;
        }
        llvm::Type *i8_ty = llvm::Type::getInt8Ty(emitter.GetContext());
        llvm::Type *i8_ptr_ty = llvm::PointerType::get(emitter.GetContext(), 0);
        llvm::Value *flag_ptr = CoerceTo(&builder, panic_ptr, i8_ptr_ty);
        if (!flag_ptr)
        {
          return;
        }
        llvm::Value *flag = builder.CreateLoad(i8_ty, flag_ptr);
        llvm::Value *has_panic = builder.CreateICmpNE(flag, llvm::ConstantInt::get(i8_ty, 0));

        llvm::Function *func = builder.GetInsertBlock()->getParent();
        llvm::BasicBlock *panic_bb =
            llvm::BasicBlock::Create(emitter.GetContext(), "panic.take", func);
        llvm::BasicBlock *cont_bb =
            llvm::BasicBlock::Create(emitter.GetContext(), "panic.cont", func);
        builder.CreateCondBr(has_panic, panic_bb, cont_bb);

        builder.SetInsertPoint(panic_bb);
        if (check.cleanup_ir)
        {
          emitter.EmitIR(check.cleanup_ir);
        }
        const bool entry_stub_panic =
            func && func->getName() == EntrySym() &&
            emitter.GetCurrentCtx() && emitter.GetCurrentCtx()->executable_project;
        if (entry_stub_panic && !builder.GetInsertBlock()->getTerminator())
        {
          llvm::Value *panic_code = LoadPanicCode(emitter, &builder);
          if (!panic_code)
          {
            panic_code = llvm::ConstantInt::get(
                llvm::Type::getInt32Ty(emitter.GetContext()), 1);
          }
          llvm::Function *runtime_panic_fn =
              emitter.GetModule().getFunction(RuntimePanicSym());
          if (!runtime_panic_fn)
          {
            llvm::FunctionType *panic_ty = llvm::FunctionType::get(
                llvm::Type::getVoidTy(emitter.GetContext()),
                {llvm::Type::getInt32Ty(emitter.GetContext())},
                false);
            runtime_panic_fn = llvm::Function::Create(
                panic_ty,
                llvm::GlobalValue::ExternalLinkage,
                RuntimePanicSym(),
                &emitter.GetModule());
            runtime_panic_fn->setCallingConv(llvm::CallingConv::C);
          }
          llvm::Type *i32_ty = llvm::Type::getInt32Ty(emitter.GetContext());
          panic_code = CoerceTo(&builder, panic_code, i32_ty);
          if (!panic_code)
          {
            panic_code = llvm::ConstantInt::get(i32_ty, 1);
          }
          builder.CreateCall(runtime_panic_fn->getFunctionType(),
                             runtime_panic_fn,
                             {panic_code});
          builder.CreateUnreachable();
          builder.SetInsertPoint(cont_bb);
          return;
        }
        if (!builder.GetInsertBlock()->getTerminator())
        {
          EmitReturn(emitter, &builder);
        }

        builder.SetInsertPoint(cont_bb);
      }

      void operator()(const IRLowerPanic &panic) const
      {
        const std::uint16_t code = PanicCodeFromString(panic.reason);
        StorePanicRecord(emitter, &builder, code);
        if (panic.cleanup_ir)
        {
          emitter.EmitIR(panic.cleanup_ir);
        }
        if (!builder.GetInsertBlock()->getTerminator())
        {
          EmitReturn(emitter, &builder);
        }
      }

      void operator()(const IRCheckOp &check) const
      {
        llvm::Value *ok = llvm::ConstantInt::getTrue(emitter.GetContext());
        const std::uint16_t code = PanicCodeFromString(check.reason);

        if ((check.op == "/" || check.op == "%") && check.rhs.has_value())
        {
          llvm::Value *rhs = EvaluateOrDefault(*check.rhs);
          if (rhs && rhs->getType()->isIntegerTy())
          {
            llvm::Value *zero = llvm::ConstantInt::get(rhs->getType(), 0);
            ok = builder.CreateICmpNE(rhs, zero);
          }
        }
        else if ((check.op == "<<" || check.op == ">>") && check.rhs.has_value())
        {
          llvm::Value *lhs = EvaluateOrDefault(check.lhs);
          llvm::Value *rhs = EvaluateOrDefault(*check.rhs);
          if (rhs && rhs->getType()->isIntegerTy())
          {
            llvm::Value *rhs64 = rhs;
            if (rhs64->getType()->getIntegerBitWidth() != 64)
            {
              rhs64 = builder.CreateIntCast(rhs64, llvm::Type::getInt64Ty(emitter.GetContext()), false);
            }
            std::uint64_t width = 64;
            if (lhs && lhs->getType()->isIntegerTy())
            {
              width = lhs->getType()->getIntegerBitWidth();
            }
            ok = builder.CreateICmpULT(
                rhs64,
                llvm::ConstantInt::get(llvm::Type::getInt64Ty(emitter.GetContext()), width));
          }
        }
        else if (check.op == "-" && !check.rhs.has_value())
        {
          llvm::Value *lhs = EvaluateOrDefault(check.lhs);
          if (lhs && lhs->getType()->isIntegerTy())
          {
            bool signed_ty = true;
            if (const LowerCtx *ctx = emitter.GetCurrentCtx())
            {
              analysis::TypeRef lhs_type = ctx->LookupValueType(check.lhs);
              if (lhs_type)
              {
                signed_ty = IsSignedIntegerType(lhs_type);
              }
            }
            if (signed_ty)
            {
              const unsigned bits = lhs->getType()->getIntegerBitWidth();
              llvm::Value *minv = llvm::ConstantInt::get(
                  lhs->getType(),
                  llvm::APInt::getSignedMinValue(bits));
              ok = builder.CreateICmpNE(lhs, minv);
            }
          }
        }
        else if (check.op == "nonnull")
        {
          llvm::Value *lhs = EvaluateOrDefault(check.lhs);
          if (lhs)
          {
            if (lhs->getType()->isPointerTy())
            {
              llvm::Value *null_ptr = llvm::ConstantPointerNull::get(
                  llvm::cast<llvm::PointerType>(lhs->getType()));
              ok = builder.CreateICmpNE(lhs, null_ptr);
            }
            else if (lhs->getType()->isIntegerTy())
            {
              llvm::Value *zero = llvm::ConstantInt::get(lhs->getType(), 0);
              ok = builder.CreateICmpNE(lhs, zero);
            }
            else
            {
              ok = AsBool(&builder, lhs);
            }
          }
        }
        else if (check.op == "addr_active")
        {
          llvm::Value *lhs = EvaluateOrDefault(check.lhs);
          if (lhs)
          {
            ok = AsBool(&builder, lhs);
          }
        }

        EmitPanicReturnIfFalse(emitter, &builder, ok, code);
      }

      void operator()(const IRCheckIndex &check) const
      {
        auto len_opt = StaticLengthOf(check.base);
        llvm::Value *dynamic_len = nullptr;
        if (!len_opt.has_value())
        {
          llvm::Value *base = EvaluateOrDefault(check.base);
          if (base)
          {
            if (auto *arr_ty = llvm::dyn_cast<llvm::ArrayType>(base->getType()))
            {
              len_opt = arr_ty->getNumElements();
            }
            else
            {
              dynamic_len = DynamicLengthOf(check.base);
            }
          }
        }
        if (!len_opt.has_value() && dynamic_len == nullptr)
        {
          return;
        }
        llvm::Value *idx = EvaluateOrDefault(check.index);
        if (!idx || !idx->getType()->isIntegerTy())
        {
          return;
        }
        llvm::Value *idx64 = idx;
        if (idx64->getType()->getIntegerBitWidth() != 64)
        {
          idx64 = builder.CreateIntCast(idx64, llvm::Type::getInt64Ty(emitter.GetContext()), false);
        }
        llvm::Value *len64 = nullptr;
        if (len_opt.has_value())
        {
          len64 = llvm::ConstantInt::get(llvm::Type::getInt64Ty(emitter.GetContext()), *len_opt);
        }
        else
        {
          len64 = dynamic_len;
          if (!len64 || !len64->getType()->isIntegerTy())
          {
            return;
          }
          if (len64->getType()->getIntegerBitWidth() != 64)
          {
            len64 = builder.CreateIntCast(
                len64, llvm::Type::getInt64Ty(emitter.GetContext()), false);
          }
        }
        llvm::Value *ok = builder.CreateICmpULT(
            idx64,
            len64);
        EmitPanicReturnIfFalse(emitter, &builder, ok, PanicCode(PanicReason::Bounds));
      }

      void operator()(const IRCheckRange &check) const
      {
        auto len_opt = StaticLengthOf(check.base);
        llvm::Value *dynamic_len = nullptr;
        if (!len_opt.has_value())
        {
          llvm::Value *base = EvaluateOrDefault(check.base);
          if (base)
          {
            if (auto *arr_ty = llvm::dyn_cast<llvm::ArrayType>(base->getType()))
            {
              len_opt = arr_ty->getNumElements();
            }
            else
            {
              dynamic_len = DynamicLengthOf(check.base);
            }
          }
        }
        if (!len_opt.has_value() && dynamic_len == nullptr)
        {
          return;
        }
        llvm::Type *i64 = llvm::Type::getInt64Ty(emitter.GetContext());
        llvm::Value *len = nullptr;
        if (len_opt.has_value())
        {
          len = llvm::ConstantInt::get(i64, *len_opt);
        }
        else
        {
          len = dynamic_len;
          if (!len || !len->getType()->isIntegerTy())
          {
            return;
          }
          if (len->getType()->getIntegerBitWidth() != 64)
          {
            len = builder.CreateIntCast(len, i64, false);
          }
        }

        auto as_u64 = [&](const std::optional<IRValue> &value, std::uint64_t default_value) -> llvm::Value *
        {
          if (!value.has_value())
          {
            return llvm::ConstantInt::get(i64, default_value);
          }
          llvm::Value *raw = EvaluateOrDefault(*value);
          if (!raw || !raw->getType()->isIntegerTy())
          {
            return nullptr;
          }
          if (raw->getType()->getIntegerBitWidth() != 64)
          {
            raw = builder.CreateIntCast(raw, i64, false);
          }
          return raw;
        };

        auto runtime_range =
            check.range_value.has_value()
                ? ResolveRangeValue(*check.range_value, i64, check.range.kind)
                                          : std::nullopt;

        llvm::Value *ok = llvm::ConstantInt::getTrue(emitter.GetContext());
        if (runtime_range.has_value())
        {
          switch (runtime_range->kind)
          {
          case IRRangeKind::Full:
            break;
          case IRRangeKind::From:
          {
            llvm::Value *lo = runtime_range->lo
                                  ? runtime_range->lo
                                  : llvm::ConstantInt::get(i64, 0);
            ok = builder.CreateICmpULE(lo, len);
            break;
          }
          case IRRangeKind::To:
          {
            llvm::Value *hi = runtime_range->hi ? runtime_range->hi : len;
            ok = builder.CreateICmpULE(hi, len);
            break;
          }
          case IRRangeKind::ToInclusive:
          {
            llvm::Value *hi = runtime_range->hi
                                  ? runtime_range->hi
                                  : llvm::ConstantInt::get(i64, 0);
            ok = builder.CreateICmpULT(hi, len);
            break;
          }
          case IRRangeKind::Exclusive:
          {
            llvm::Value *lo = runtime_range->lo
                                  ? runtime_range->lo
                                  : llvm::ConstantInt::get(i64, 0);
            llvm::Value *hi = runtime_range->hi ? runtime_range->hi : len;
            llvm::Value *lo_le_hi = builder.CreateICmpULE(lo, hi);
            llvm::Value *hi_le_len = builder.CreateICmpULE(hi, len);
            ok = builder.CreateAnd(lo_le_hi, hi_le_len);
            break;
          }
          case IRRangeKind::Inclusive:
          {
            llvm::Value *lo = runtime_range->lo
                                  ? runtime_range->lo
                                  : llvm::ConstantInt::get(i64, 0);
            llvm::Value *hi = runtime_range->hi
                                  ? runtime_range->hi
                                  : llvm::ConstantInt::get(i64, 0);
            llvm::Value *lo_le_hi = builder.CreateICmpULE(lo, hi);
            llvm::Value *hi_lt_len = builder.CreateICmpULT(hi, len);
            ok = builder.CreateAnd(lo_le_hi, hi_lt_len);
            break;
          }
          }
        }
        else
        {
          switch (check.range.kind)
          {
          case IRRangeKind::Full:
            break;
          case IRRangeKind::From:
          {
            llvm::Value *lo = as_u64(check.range.lo, 0);
            if (!lo)
            {
              return;
            }
            ok = builder.CreateICmpULE(lo, len);
            break;
          }
          case IRRangeKind::To:
          {
            llvm::Value *hi = as_u64(check.range.hi, *len_opt);
            if (!hi)
            {
              return;
            }
            ok = builder.CreateICmpULE(hi, len);
            break;
          }
          case IRRangeKind::ToInclusive:
          {
            llvm::Value *hi = as_u64(check.range.hi, 0);
            if (!hi)
            {
              return;
            }
            ok = builder.CreateICmpULT(hi, len);
            break;
          }
          case IRRangeKind::Exclusive:
          {
            llvm::Value *lo = as_u64(check.range.lo, 0);
            llvm::Value *hi = as_u64(check.range.hi, *len_opt);
            if (!lo || !hi)
            {
              return;
            }
            llvm::Value *lo_le_hi = builder.CreateICmpULE(lo, hi);
            llvm::Value *hi_le_len = builder.CreateICmpULE(hi, len);
            ok = builder.CreateAnd(lo_le_hi, hi_le_len);
            break;
          }
          case IRRangeKind::Inclusive:
          {
            llvm::Value *lo = as_u64(check.range.lo, 0);
            llvm::Value *hi = as_u64(check.range.hi, 0);
            if (!lo || !hi)
            {
              return;
            }
            llvm::Value *lo_le_hi = builder.CreateICmpULE(lo, hi);
            llvm::Value *hi_lt_len = builder.CreateICmpULT(hi, len);
            ok = builder.CreateAnd(lo_le_hi, hi_lt_len);
            break;
          }
          }
        }
        EmitPanicReturnIfFalse(emitter, &builder, ok, PanicCode(PanicReason::Bounds));
      }

      void operator()(const IRCheckSliceLen &check) const
      {
        llvm::Type *i64 = llvm::Type::getInt64Ty(emitter.GetContext());
        auto length_value = [&](const IRValue &value) -> llvm::Value *
        {
          if (auto static_len = StaticLengthOf(value))
          {
            return llvm::ConstantInt::get(i64, *static_len);
          }
          llvm::Value *dynamic_len = DynamicLengthOf(value);
          if (!dynamic_len || !dynamic_len->getType()->isIntegerTy())
          {
            return nullptr;
          }
          if (dynamic_len->getType()->getIntegerBitWidth() != 64)
          {
            dynamic_len = builder.CreateIntCast(dynamic_len, i64, false);
          }
          return dynamic_len;
        };

        llvm::Value *base_len = length_value(check.base);
        llvm::Value *value_len = length_value(check.value);
        if (!base_len || !value_len)
        {
          return;
        }

        auto to_i64 = [&](const std::optional<IRValue> &value,
                          llvm::Value *default_value) -> llvm::Value *
        {
          if (!value.has_value())
          {
            return default_value;
          }
          llvm::Value *raw = EvaluateOrDefault(*value);
          if (!raw || !raw->getType()->isIntegerTy())
          {
            return nullptr;
          }
          if (raw->getType()->getIntegerBitWidth() != 64)
          {
            raw = builder.CreateIntCast(raw, i64, false);
          }
          return raw;
        };

        auto runtime_range =
            check.range_value.has_value()
                ? ResolveRangeValue(*check.range_value, i64, check.range.kind)
                                          : std::nullopt;

        llvm::Value *expected_len = nullptr;
        if (runtime_range.has_value())
        {
          switch (runtime_range->kind)
          {
          case IRRangeKind::Full:
            expected_len = base_len;
            break;
          case IRRangeKind::From:
          {
            llvm::Value *lo = runtime_range->lo
                                  ? runtime_range->lo
                                  : llvm::ConstantInt::get(i64, 0);
            expected_len = builder.CreateSub(base_len, lo);
            break;
          }
          case IRRangeKind::To:
          {
            llvm::Value *hi = runtime_range->hi ? runtime_range->hi : base_len;
            expected_len = hi;
            break;
          }
          case IRRangeKind::ToInclusive:
          {
            llvm::Value *hi = runtime_range->hi
                                  ? runtime_range->hi
                                  : llvm::ConstantInt::get(i64, 0);
            expected_len = builder.CreateAdd(hi, llvm::ConstantInt::get(i64, 1));
            break;
          }
          case IRRangeKind::Exclusive:
          {
            llvm::Value *lo = runtime_range->lo
                                  ? runtime_range->lo
                                  : llvm::ConstantInt::get(i64, 0);
            llvm::Value *hi = runtime_range->hi ? runtime_range->hi : base_len;
            expected_len = builder.CreateSub(hi, lo);
            break;
          }
          case IRRangeKind::Inclusive:
          {
            llvm::Value *lo = runtime_range->lo
                                  ? runtime_range->lo
                                  : llvm::ConstantInt::get(i64, 0);
            llvm::Value *hi = runtime_range->hi
                                  ? runtime_range->hi
                                  : llvm::ConstantInt::get(i64, 0);
            llvm::Value *span = builder.CreateSub(hi, lo);
            expected_len = builder.CreateAdd(span, llvm::ConstantInt::get(i64, 1));
            break;
          }
          }
        }
        else
        {
          switch (check.range.kind)
          {
          case IRRangeKind::Full:
            expected_len = base_len;
            break;
          case IRRangeKind::From:
          {
            llvm::Value *lo = to_i64(check.range.lo, llvm::ConstantInt::get(i64, 0));
            if (!lo)
            {
              return;
            }
            expected_len = builder.CreateSub(base_len, lo);
            break;
          }
          case IRRangeKind::To:
          {
            llvm::Value *hi = to_i64(check.range.hi, base_len);
            if (!hi)
            {
              return;
            }
            expected_len = hi;
            break;
          }
          case IRRangeKind::ToInclusive:
          {
            llvm::Value *hi = to_i64(check.range.hi, llvm::ConstantInt::get(i64, 0));
            if (!hi)
            {
              return;
            }
            expected_len = builder.CreateAdd(hi, llvm::ConstantInt::get(i64, 1));
            break;
          }
          case IRRangeKind::Exclusive:
          {
            llvm::Value *lo = to_i64(check.range.lo, llvm::ConstantInt::get(i64, 0));
            llvm::Value *hi = to_i64(check.range.hi, base_len);
            if (!lo || !hi)
            {
              return;
            }
            expected_len = builder.CreateSub(hi, lo);
            break;
          }
          case IRRangeKind::Inclusive:
          {
            llvm::Value *lo = to_i64(check.range.lo, llvm::ConstantInt::get(i64, 0));
            llvm::Value *hi = to_i64(check.range.hi, llvm::ConstantInt::get(i64, 0));
            if (!lo || !hi)
            {
              return;
            }
            llvm::Value *span = builder.CreateSub(hi, lo);
            expected_len = builder.CreateAdd(span, llvm::ConstantInt::get(i64, 1));
            break;
          }
          }
        }
        if (!expected_len)
        {
          return;
        }

        llvm::Value *ok = builder.CreateICmpEQ(value_len, expected_len);
        EmitPanicReturnIfFalse(emitter, &builder, ok, PanicCode(PanicReason::Bounds));
      }

      void operator()(const IRCheckCast &check) const
      {
        llvm::Type *target_ty = emitter.GetLLVMType(check.target);
        llvm::Value *value = EvaluateOrDefault(check.value);
        if (!target_ty || !value || !target_ty->isIntegerTy() || !value->getType()->isIntegerTy())
        {
          return;
        }
        const unsigned src_bits = value->getType()->getIntegerBitWidth();
        const unsigned dst_bits = target_ty->getIntegerBitWidth();
        if (dst_bits >= src_bits)
        {
          return;
        }

        bool signed_src = false;
        if (analysis::TypeRef src_type = LookupValueType(check.value))
        {
          signed_src = IsSignedIntegerType(src_type);
        }
        llvm::Value *narrowed = builder.CreateIntCast(value, target_ty, signed_src);
        llvm::Value *widened = builder.CreateIntCast(narrowed, value->getType(), signed_src);
        llvm::Value *ok = builder.CreateICmpEQ(value, widened);
        EmitPanicReturnIfFalse(emitter, &builder, ok, PanicCode(PanicReason::Cast));
      }
      void operator()(const IRReadPlace &) const {}
      void operator()(const IRWritePlace &) const {}
      void operator()(const IRAddrOf &addrof) const
      {
        auto cache_if_safe = [&](llvm::Value *value)
        {
          if (!value)
          {
            return;
          }
          if (llvm::isa<llvm::Constant>(value) ||
              llvm::isa<llvm::GlobalValue>(value) ||
              llvm::isa<llvm::Argument>(value) ||
              llvm::isa<llvm::AllocaInst>(value))
          {
            emitter.SetTempValue(addrof.result, value);
          }
        };

        // Prefer derived address materialization (AddrField/AddrTuple/AddrIndex/etc.).
        // Falling back to place-root lookup for these cases collapses addresses to the
        // base binding and loses field/element offsets.
        if (llvm::Value *derived_ptr = emitter.EvaluateIRValue(addrof.result);
            derived_ptr && derived_ptr->getType()->isPointerTy())
        {
          if (llvm::Type *expected = ExpectedLLVMType(addrof.result))
          {
            if (expected->isPointerTy())
            {
              derived_ptr = CoerceTo(&builder, derived_ptr, expected);
            }
          }
          cache_if_safe(derived_ptr);
          return;
        }

        std::string base_name = BasePlaceIdentifier(addrof.place.repr);
        if (base_name.empty())
        {
          base_name = addrof.place.repr;
        }

        const LowerCtx *active_ctx = emitter.GetCurrentCtx();
        const BindingState *local_binding =
            active_ctx ? active_ctx->GetBindingState(base_name) : nullptr;

        llvm::Value *ptr = emitter.GetLocalBindStorage(base_name);
        bool hosted_state_resolution_failed = false;
        auto resolve_hosted_state_ptr = [&](const std::string &symbol_name,
                                            llvm::Type *static_ll) -> llvm::Value * {
          llvm::Value *fallback = nullptr;
          if (llvm::Value *global_value = emitter.GetGlobal(symbol_name))
          {
            fallback = global_value;
          }
          if (!fallback)
          {
            fallback = emitter.GetModule().getNamedGlobal(symbol_name);
          }
          llvm::Value *state_ptr =
              emitter.GetHostedStatePtr(symbol_name, static_ll, fallback);
          if (!state_ptr && emitter.HasHostedStateSlot(symbol_name) && !fallback)
          {
            hosted_state_resolution_failed = true;
          }
          return state_ptr;
        };
        if (!ptr)
        {
          if (auto alias = emitter.LookupSymbolAlias(base_name))
          {
            if (active_ctx && emitter.IsHostedLibraryBuild())
            {
              analysis::TypeRef static_type = active_ctx->LookupStaticType(*alias);
              if (static_type)
              {
                if (llvm::Type *static_ll = emitter.GetLLVMType(static_type))
                {
                  ptr = resolve_hosted_state_ptr(*alias, static_ll);
                }
              }
            }
            if (!ptr && !hosted_state_resolution_failed)
            {
              ptr = emitter.GetGlobal(*alias);
            }
            if (!ptr && !hosted_state_resolution_failed)
            {
              ptr = emitter.GetFunction(*alias);
            }
          }
        }
        if (!ptr && !hosted_state_resolution_failed)
        {
          if (active_ctx)
          {
            auto try_hosted_state = [&](const std::string &symbol_name) -> llvm::Value * {
              if (!emitter.IsHostedLibraryBuild())
              {
                return nullptr;
              }
              analysis::TypeRef static_type = active_ctx->LookupStaticType(symbol_name);
              if (!static_type)
              {
                return nullptr;
              }
              llvm::Type *static_ll = emitter.GetLLVMType(static_type);
              if (!static_ll || static_ll->isVoidTy())
              {
                return nullptr;
              }
              return resolve_hosted_state_ptr(symbol_name, static_ll);
            };

            if (auto alias = emitter.LookupSymbolAlias(base_name))
            {
              ptr = try_hosted_state(*alias);
            }
            if (!ptr)
            {
              ptr = try_hosted_state(base_name);
            }
          }
        }
        if (hosted_state_resolution_failed)
        {
          if (active_ctx)
          {
            const_cast<LowerCtx *>(active_ctx)->ReportCodegenFailure();
          }
          emitter.SetTempValue(addrof.result, DefaultFor(addrof.result));
          return;
        }
        if (!ptr && local_binding)
        {
          const_cast<LowerCtx *>(active_ctx)->ReportCodegenFailure();
          emitter.SetTempValue(addrof.result, DefaultFor(addrof.result));
          return;
        }
        if (!ptr)
        {
          ptr = emitter.GetGlobal(base_name);
        }
        if (!ptr)
        {
          ptr = emitter.GetFunction(base_name);
        }

        if (!ptr || !ptr->getType()->isPointerTy())
        {
          if (local_binding && active_ctx)
          {
            const_cast<LowerCtx *>(active_ctx)->ReportCodegenFailure();
          }
          emitter.SetTempValue(addrof.result, DefaultFor(addrof.result));
          return;
        }

        if (llvm::Type *expected = ExpectedLLVMType(addrof.result))
        {
          if (expected->isPointerTy())
          {
            ptr = CoerceTo(&builder, ptr, expected);
          }
        }
        cache_if_safe(ptr ? ptr : DefaultFor(addrof.result));
      }
      void operator()(const IRReadPtr &read) const
      {
        llvm::Type *result_ty = ExpectedLLVMType(read.result);
        if (!result_ty)
        {
          emitter.SetTempValue(read.result, DefaultFor(read.result));
          return;
        }
        llvm::Value *ptr = EvaluateOrDefault(read.ptr);
        if (!ptr)
        {
          emitter.SetTempValue(read.result, DefaultFor(read.result));
          return;
        }

        llvm::PointerType *typed_ptr_ty = llvm::PointerType::get(result_ty, 0);
        if (ptr->getType()->isIntegerTy())
        {
          ptr = builder.CreateIntToPtr(ptr, typed_ptr_ty);
        }
        else if (!ptr->getType()->isPointerTy())
        {
          emitter.SetTempValue(read.result, DefaultFor(read.result));
          return;
        }
        llvm::Value *typed_ptr = ptr;
        if (typed_ptr->getType() != typed_ptr_ty)
        {
          auto *src_ptr_ty = llvm::dyn_cast<llvm::PointerType>(typed_ptr->getType());
          if (!src_ptr_ty)
          {
            emitter.SetTempValue(read.result, DefaultFor(read.result));
            return;
          }
          if (src_ptr_ty->getAddressSpace() == typed_ptr_ty->getAddressSpace())
          {
            typed_ptr = builder.CreateBitCast(typed_ptr, typed_ptr_ty);
          }
          else
          {
            typed_ptr = CoerceTo(&builder, typed_ptr, typed_ptr_ty);
            if (!typed_ptr)
            {
              emitter.SetTempValue(read.result, DefaultFor(read.result));
              return;
            }
          }
        }

        llvm::Value *loaded = builder.CreateLoad(result_ty, typed_ptr);
        emitter.SetTempValue(read.result, loaded ? loaded : DefaultFor(read.result));
      }

      void operator()(const IRWritePtr &write) const
      {
        auto hosted_state_pointer = [&](const IRValue &value) -> bool
        {
          const LowerCtx *active_ctx = emitter.GetCurrentCtx();
          if (!active_ctx || !emitter.IsHostedLibraryBuild())
          {
            return false;
          }

          IRValue cursor = value;
          for (int depth = 0; depth < 16; ++depth)
          {
            if (cursor.kind == IRValue::Kind::Symbol)
            {
              std::string symbol = cursor.name;
              if (auto alias = emitter.LookupSymbolAlias(symbol))
              {
                symbol = *alias;
              }
              return emitter.HasHostedStateSlot(symbol);
            }
            if (cursor.kind != IRValue::Kind::Opaque)
            {
              return false;
            }

            const DerivedValueInfo *derived = active_ctx->LookupDerivedValue(cursor);
            if (!derived)
            {
              return false;
            }

            switch (derived->kind)
            {
            case DerivedValueInfo::Kind::AddrStatic:
            {
              std::string symbol = derived->name;
              if (!derived->static_path.empty() && !derived->name.empty())
              {
                if (auto* lower_ctx = emitter.GetCurrentCtx();
                    lower_ctx && lower_ctx->sigma) {
                  if (auto addr =
                          StaticAddr(*lower_ctx->sigma,
                                     derived->static_path,
                                     derived->name)) {
                    symbol = addr->name;
                  } else {
                    symbol = StaticSymPath(derived->static_path,
                                           derived->name);
                  }
                } else {
                  symbol = StaticSymPath(derived->static_path,
                                         derived->name);
                }
              }
              if (emitter.HasHostedStateSlot(symbol))
              {
                return true;
              }
              if (!derived->name.empty())
              {
                if (auto alias = emitter.LookupSymbolAlias(derived->name))
                {
                  if (emitter.HasHostedStateSlot(*alias))
                  {
                    return true;
                  }
                }
                if (emitter.HasHostedStateSlot(derived->name))
                {
                  return true;
                }
              }
              return false;
            }
            case DerivedValueInfo::Kind::AddrField:
            case DerivedValueInfo::Kind::AddrTuple:
            case DerivedValueInfo::Kind::AddrIndex:
            case DerivedValueInfo::Kind::AddrDeref:
            case DerivedValueInfo::Kind::LoadFromAddr:
              cursor = derived->base;
              continue;
            default:
              return false;
            }
          }
          return false;
        };

        llvm::Value *ptr = emitter.EvaluateIRValue(write.ptr);
        if (!ptr)
        {
          if (hosted_state_pointer(write.ptr))
          {
            if (const LowerCtx *active_ctx = emitter.GetCurrentCtx())
            {
              const_cast<LowerCtx *>(active_ctx)->ReportCodegenFailure();
            }
            return;
          }
          ptr = DefaultFor(write.ptr);
        }
        llvm::Value *value = EvaluateOrDefault(write.value);
        if (!ptr || !value)
        {
          return;
        }

        llvm::Type *value_ty = value->getType();
        llvm::PointerType *typed_ptr_ty = llvm::PointerType::get(value_ty, 0);
        if (ptr->getType()->isIntegerTy())
        {
          ptr = builder.CreateIntToPtr(ptr, typed_ptr_ty);
        }
        else if (!ptr->getType()->isPointerTy())
        {
          return;
        }

        llvm::Value *typed_ptr = ptr;
        if (typed_ptr->getType() != typed_ptr_ty)
        {
          auto *src_ptr_ty = llvm::dyn_cast<llvm::PointerType>(typed_ptr->getType());
          if (!src_ptr_ty)
          {
            return;
          }
          if (src_ptr_ty->getAddressSpace() == typed_ptr_ty->getAddressSpace())
          {
            typed_ptr = builder.CreateBitCast(typed_ptr, typed_ptr_ty);
          }
          else
          {
            typed_ptr = CoerceTo(&builder, typed_ptr, typed_ptr_ty);
            if (!typed_ptr)
            {
              return;
            }
          }
        }

        builder.CreateStore(value, typed_ptr);
      }
      void operator()(const IRCast &cast) const
      {
        llvm::Value *src = EvaluateOrDefault(cast.value);
        llvm::Type *target_ty = emitter.GetLLVMType(cast.target);
        if (!src || !target_ty)
        {
          emitter.SetTempValue(cast.result, DefaultFor(cast.result));
          return;
        }

        auto is_signed_type = [this](const analysis::TypeRef &type) -> bool
        {
          if (!type)
          {
            return false;
          }
          analysis::TypeRef stripped = analysis::StripPerm(type);
          if (!stripped)
          {
            stripped = type;
          }
          return this->IsSignedIntegerType(stripped);
        };

        const bool signed_src = is_signed_type(LookupValueType(cast.value));
        const bool signed_dst = is_signed_type(cast.target);

        llvm::Value *out = nullptr;
        llvm::Type *src_ty = src->getType();

        if (src_ty == target_ty)
        {
          out = src;
        }
        else if (target_ty->isIntegerTy(1))
        {
          out = AsBool(&builder, src);
        }
        else if (src_ty->isIntegerTy(1) && target_ty->isIntegerTy())
        {
          out = builder.CreateZExt(src, target_ty);
        }
        else if (src_ty->isIntegerTy() && target_ty->isIntegerTy())
        {
          out = builder.CreateIntCast(src, target_ty, signed_src);
        }
        else if (src_ty->isIntegerTy() && target_ty->isFloatingPointTy())
        {
          out = signed_src
                    ? builder.CreateSIToFP(src, target_ty)
                    : builder.CreateUIToFP(src, target_ty);
        }
        else if (src_ty->isFloatingPointTy() && target_ty->isIntegerTy())
        {
          out = signed_dst
                    ? builder.CreateFPToSI(src, target_ty)
                    : builder.CreateFPToUI(src, target_ty);
        }
        else if (src_ty->isFloatingPointTy() && target_ty->isFloatingPointTy())
        {
          out = builder.CreateFPCast(src, target_ty);
        }
        else if (src_ty->isPointerTy() && target_ty->isPointerTy())
        {
          out = builder.CreateBitCast(src, target_ty);
        }
        else if (src_ty->isPointerTy() && target_ty->isIntegerTy())
        {
          out = builder.CreatePtrToInt(src, target_ty);
        }
        else if (src_ty->isIntegerTy() && target_ty->isPointerTy())
        {
          out = builder.CreateIntToPtr(src, target_ty);
        }

        if (!out)
        {
          out = CoerceTo(&builder, src, target_ty);
        }
        if (!out)
        {
          out = llvm::Constant::getNullValue(target_ty);
        }
        emitter.SetTempValue(cast.result, out);
      }
      void operator()(const IRTransmute &transmute) const
      {
        llvm::Value *src = EvaluateOrDefault(transmute.value);
        llvm::Type *target_ty = emitter.GetLLVMType(transmute.to);
        if (!target_ty)
        {
          target_ty = ExpectedLLVMType(transmute.result);
        }
        if (!src || !target_ty || target_ty->isVoidTy())
        {
          emitter.SetTempValue(transmute.result, DefaultFor(transmute.result));
          return;
        }

        llvm::Type *src_ty = src->getType();
        llvm::Value *out = nullptr;
        if (src_ty == target_ty)
        {
          out = src;
        }
        else if (src_ty->isPointerTy() && target_ty->isPointerTy())
        {
          out = builder.CreateBitCast(src, target_ty);
        }
        else if (src_ty->isPointerTy() && target_ty->isIntegerTy())
        {
          out = builder.CreatePtrToInt(src, target_ty);
        }
        else if (src_ty->isIntegerTy() && target_ty->isPointerTy())
        {
          out = builder.CreateIntToPtr(src, target_ty);
        }
        else
        {
          const llvm::DataLayout &layout = emitter.GetModule().getDataLayout();
          const std::uint64_t src_bits = layout.getTypeSizeInBits(src_ty);
          const std::uint64_t dst_bits = layout.getTypeSizeInBits(target_ty);
          if (src_bits == dst_bits &&
              src_ty->isFirstClassType() &&
              target_ty->isFirstClassType())
          {
            if ((src_ty->isIntegerTy() && target_ty->isFloatingPointTy()) ||
                (src_ty->isFloatingPointTy() && target_ty->isIntegerTy()) ||
                (src_ty->isIntegerTy() && target_ty->isIntegerTy()) ||
                (src_ty->isFloatingPointTy() && target_ty->isFloatingPointTy()) ||
                (src_ty->isVectorTy() && target_ty->isVectorTy()))
            {
              out = builder.CreateBitCast(src, target_ty);
            }
            else
            {
              // Fallback to memory reinterpretation for equal-sized first-class
              // values when direct bitcast is not legal between categories.
              llvm::AllocaInst *slot = builder.CreateAlloca(src_ty, nullptr, "transmute.tmp");
              builder.CreateStore(src, slot);
              llvm::Value *cast_ptr = builder.CreateBitCast(
                  slot,
                  llvm::PointerType::get(target_ty, 0));
              out = builder.CreateLoad(target_ty, cast_ptr);
            }
          }
        }

        if (!out)
        {
          out = llvm::Constant::getNullValue(target_ty);
        }
        emitter.SetTempValue(transmute.result, out);
      }
      void operator()(const IRAlloc &alloc) const
      {
        const LowerCtx *active_ctx = emitter.GetCurrentCtx();
        analysis::TypeRef value_type = alloc.type;
        if (!value_type && active_ctx)
        {
          value_type = active_ctx->LookupValueType(alloc.value);
        }

        llvm::Value *value = EvaluateOrDefault(alloc.value);
        llvm::Type *value_ty = value_type ? emitter.GetLLVMType(value_type) : nullptr;
        if ((!value_ty || value_ty->isVoidTy()) && value)
        {
          value_ty = value->getType();
        }
        if (!value_ty || value_ty->isVoidTy())
        {
          emitter.SetTempValue(alloc.result, DefaultFor(alloc.result));
          return;
        }

        std::optional<IRValue> target_region = alloc.region;
        if (!target_region.has_value())
        {
          if (const IRValue *current = emitter.CurrentActiveRegion())
          {
            target_region = *current;
          }
        }
        if (!target_region.has_value())
        {
          emitter.SetTempValue(alloc.result, DefaultFor(alloc.result));
          return;
        }

        llvm::Value *region_value = EvaluateOrDefault(*target_region);
        if (!region_value)
        {
          emitter.SetTempValue(alloc.result, DefaultFor(alloc.result));
          return;
        }

        const analysis::ScopeContext &scope = BuildScope(active_ctx);
        std::uint64_t alloc_size = 0;
        std::uint64_t alloc_align = 1;
        if (value_type)
        {
          if (const auto size = ::cursive::analysis::layout::SizeOf(scope, value_type))
          {
            alloc_size = *size;
          }
          if (const auto align = ::cursive::analysis::layout::AlignOf(scope, value_type))
          {
            alloc_align = *align;
          }
        }
        if (alloc_align == 0)
        {
          alloc_align = 1;
        }

        const llvm::DataLayout &dl = emitter.GetModule().getDataLayout();
        if (alloc_size == 0 && !value_ty->isVoidTy())
        {
          alloc_size = static_cast<std::uint64_t>(dl.getTypeAllocSize(value_ty));
        }
        if (alloc_align == 1 && !value_ty->isVoidTy())
        {
          alloc_align = std::max<std::uint64_t>(
              alloc_align,
              static_cast<std::uint64_t>(dl.getABITypeAlign(value_ty).value()));
        }

        llvm::Type *usize_ty = llvm::Type::getInt64Ty(emitter.GetContext());
        llvm::Value *raw_ptr = nullptr;
        const std::string alloc_sym = BuiltinModalSymRegionAlloc();
        if (std::optional<RuntimeFuncInfo> alloc_info = GetRuntimeFuncInfo(alloc_sym))
        {
          llvm::Function *alloc_fn = emitter.GetModule().getFunction(alloc_sym);
          const bool use_c_abi_aggregate_sret = true;
          if (!alloc_fn)
          {
            ABICallResult alloc_abi = emitter.ComputeCallABI(
                alloc_info->params,
                alloc_info->ret,
                use_c_abi_aggregate_sret);
            if (alloc_abi.func_type)
            {
              alloc_fn = llvm::Function::Create(
                  alloc_abi.func_type,
                  llvm::GlobalValue::ExternalLinkage,
                  alloc_sym,
                  &emitter.GetModule());
              alloc_fn->setCallingConv(llvm::CallingConv::C);
            }
          }
          if (alloc_fn)
          {
            std::vector<llvm::Value *> alloc_args;
            alloc_args.reserve(3);
            alloc_args.push_back(region_value);
            alloc_args.push_back(llvm::ConstantInt::get(usize_ty, alloc_size));
            alloc_args.push_back(llvm::ConstantInt::get(usize_ty, alloc_align));
            raw_ptr = EmitABICall(
                emitter,
                &builder,
                alloc_fn,
                alloc_info->params,
                alloc_info->ret,
                alloc_args,
                use_c_abi_aggregate_sret);
          }
        }
        if (!raw_ptr)
        {
          emitter.SetTempValue(alloc.result, DefaultFor(alloc.result));
          return;
        }

        analysis::TypeRef source_type = active_ctx ? active_ctx->LookupValueType(alloc.value) : nullptr;
        if (!source_type)
        {
          source_type = value_type;
        }
        if (value->getType() != value_ty)
        {
          if (value_type)
          {
            llvm::Value *coerced =
                CoerceToTyped(emitter, &builder, value, value_ty, source_type, value_type);
            value = coerced ? coerced : llvm::Constant::getNullValue(value_ty);
          }
          else
          {
            llvm::Value *coerced = CoerceTo(&builder, value, value_ty);
            value = coerced ? coerced : llvm::Constant::getNullValue(value_ty);
          }
        }

        llvm::Value *typed_ptr = builder.CreateBitCast(
            raw_ptr,
            llvm::PointerType::get(value_ty, 0));
        builder.CreateStore(value, typed_ptr);

        emitter.SetTempValue(alloc.result, raw_ptr);
      }
      void operator()(const IRContextBundleBuild &build) const
      {
        const LowerCtx *active_ctx = emitter.GetCurrentCtx();
        if (!active_ctx || !build.target_type)
        {
          emitter.SetTempValue(build.result, DefaultFor(build.result));
          return;
        }

        llvm::Value *root_ctx_value = EvaluateOrDefault(build.root_ctx);
        if (!root_ctx_value)
        {
          emitter.SetTempValue(build.result, DefaultFor(build.result));
          return;
        }

        llvm::Value *root_ctx_ptr = emitter.GetAddressableStorage(build.root_ctx);
        if (!root_ctx_ptr)
        {
          llvm::Function *func =
              builder.GetInsertBlock() ? builder.GetInsertBlock()->getParent() : nullptr;
          if (func)
          {
            llvm::IRBuilder<> entry_builder(&func->getEntryBlock(),
                                            func->getEntryBlock().begin());
            llvm::AllocaInst *ctx_slot =
                entry_builder.CreateAlloca(root_ctx_value->getType(), nullptr, "ctx.bundle.root");
            builder.CreateStore(root_ctx_value, ctx_slot);
            root_ctx_ptr = ctx_slot;
          }
        }

        const analysis::ScopeContext &scope = BuildScope(active_ctx);
        auto normalize_type =
            [&](auto &&self, analysis::TypeRef type, std::size_t depth) -> analysis::TypeRef {
          if (!type || depth > 16u)
          {
            return type;
          }
          analysis::TypeRef cur = analysis::StripPerm(type);
          if (!cur)
          {
            cur = type;
          }
          while (cur)
          {
            if (const auto *refine = std::get_if<analysis::TypeRefine>(&cur->node))
            {
              cur = analysis::StripPerm(refine->base);
              if (!cur)
              {
                cur = refine->base;
              }
              continue;
            }
            break;
          }
          if (const auto *path = cur ? std::get_if<analysis::TypePathType>(&cur->node) : nullptr)
          {
            if (path->generic_args.empty())
            {
              ast::Path syntax_path;
              for (const auto &comp : path->path)
              {
                syntax_path.push_back(comp);
              }
              const auto it = scope.sigma.types.find(analysis::PathKeyOf(syntax_path));
              if (it != scope.sigma.types.end())
              {
                if (const auto *alias = std::get_if<ast::TypeAliasDecl>(&it->second))
                {
                  const auto lowered = analysis::LowerType(scope, alias->type);
                  if (lowered.ok && lowered.type)
                  {
                    return self(self, lowered.type, depth + 1u);
                  }
                }
              }
            }
          }
          return cur;
        };

        auto context_field_value =
            [&](llvm::IRBuilder<> &irb,
                llvm::Value *ctx_value,
                std::string_view field_name) -> llvm::Value * {
          struct ContextFieldInfo {
            const char *name;
            analysis::TypeRef type;
          };
          const std::array<ContextFieldInfo, 5> fields = {{
              {"fs", analysis::MakeTypeDynamic({"FileSystem"})},
              {"net", analysis::MakeTypeDynamic({"Network"})},
              {"heap", analysis::MakeTypeDynamic({"HeapAllocator"})},
              {"sys", analysis::MakeTypePath({"System"})},
              {"reactor", analysis::MakeTypeDynamic({"Reactor"})},
          }};
          std::size_t extract_index = 0u;
          for (const auto &field : fields)
          {
            const auto size = ::cursive::analysis::layout::SizeOf(scope, field.type).value_or(0u);
            if (std::string_view(field.name) == field_name)
            {
              if (size == 0u)
              {
                llvm::Type *field_ty = emitter.GetLLVMType(field.type);
                return field_ty && !field_ty->isVoidTy()
                           ? llvm::Constant::getNullValue(field_ty)
                           : nullptr;
              }
              return irb.CreateExtractValue(ctx_value, {static_cast<unsigned>(extract_index)});
            }
            if (size != 0u)
            {
              ++extract_index;
            }
          }
          return nullptr;
        };

        auto build_context_bundle =
            [&](auto &&self,
                llvm::IRBuilder<> &irb,
                analysis::TypeRef target_type,
                std::string_view field_name,
                llvm::Value *ctx_ptr,
                llvm::Value *ctx_loaded) -> llvm::Value * {
          analysis::TypeRef cur = normalize_type(normalize_type, target_type, 0u);
          if (!cur)
          {
            return nullptr;
          }

          if (const auto *dyn = std::get_if<analysis::TypeDynamic>(&cur->node))
          {
            if (field_name == "cpu" || field_name == "gpu" || field_name == "inline")
            {
              const analysis::TypeRef expected_context_type =
                  analysis::MakeTypePath({"Context"});
              const analysis::TypeRef expected_domain_type =
                  analysis::MakeTypeDynamic({"ExecutionDomain"});
              std::string runtime_sym =
                  field_name == "cpu" ? BuiltinSymContextCpu()
                  : field_name == "gpu" ? BuiltinSymContextGpu()
                                        : BuiltinSymContextInline();
              if (auto runtime_info = GetRuntimeFuncInfo(runtime_sym))
              {
                const auto ctx_eq =
                    analysis::TypeEquiv(runtime_info->params.size() == 1u
                                            ? runtime_info->params[0].type
                                            : nullptr,
                                        expected_context_type);
                const auto ret_eq =
                    analysis::TypeEquiv(runtime_info->ret, expected_domain_type);
                const auto target_eq = analysis::TypeEquiv(cur, expected_domain_type);
                if (runtime_info->params.size() != 1u || !ctx_eq.ok || !ctx_eq.equiv ||
                    !ret_eq.ok || !ret_eq.equiv || !target_eq.ok || !target_eq.equiv)
                {
                  const_cast<LowerCtx *>(active_ctx)->ReportCodegenFailure();
                  return nullptr;
                }

                ABICallResult abi =
                    emitter.ComputeCallABI(runtime_info->params, runtime_info->ret, true);
                if (!abi.valid || !abi.func_type || abi.param_kinds.size() != 1u)
                {
                  const_cast<LowerCtx *>(active_ctx)->ReportCodegenFailure();
                  return nullptr;
                }
                llvm::Function *fn = emitter.GetModule().getFunction(runtime_sym);
                if (!fn)
                {
                  fn = llvm::Function::Create(
                      abi.func_type,
                      llvm::GlobalValue::ExternalLinkage,
                      runtime_sym,
                      &emitter.GetModule());
                  fn->setCallingConv(llvm::CallingConv::C);
                }

                llvm::Value *context_arg =
                    abi.param_kinds[0] == PassKind::ByRef ? ctx_ptr : ctx_loaded;
                if (!context_arg)
                {
                  const_cast<LowerCtx *>(active_ctx)->ReportCodegenFailure();
                  return nullptr;
                }

                return EmitABICall(
                    emitter,
                    &irb,
                    fn,
                    runtime_info->params,
                    runtime_info->ret,
                    {context_arg},
                    true);
              }
              const_cast<LowerCtx *>(active_ctx)->ReportCodegenFailure();
              return nullptr;
            }
            return context_field_value(irb, ctx_loaded, field_name);
          }

          if (const auto *path = std::get_if<analysis::TypePathType>(&cur->node))
          {
            if (path->generic_args.empty() && path->path.size() == 1u &&
                path->path.front() == "System")
            {
              llvm::Type *target_ll = emitter.GetLLVMType(cur);
              return target_ll && !target_ll->isVoidTy()
                         ? llvm::Constant::getNullValue(target_ll)
                         : nullptr;
            }

            if (const ast::RecordDecl *record = analysis::LookupRecordDecl(scope, path->path))
            {
              llvm::Type *target_ll = emitter.GetLLVMType(cur);
              if (!target_ll || target_ll->isVoidTy())
              {
                return nullptr;
              }
              llvm::Value *aggregate = llvm::Constant::getNullValue(target_ll);
              unsigned insert_index = 0u;
              for (const auto &member : record->members)
              {
                const auto *field = std::get_if<ast::FieldDecl>(&member);
                if (!field)
                {
                  continue;
                }
                auto lowered = analysis::LowerType(scope, field->type);
                if (!lowered.ok || !lowered.type)
                {
                  continue;
                }
                llvm::Value *field_value = self(
                    self, irb, lowered.type, field->name, ctx_ptr, ctx_loaded);
                const auto field_size = ::cursive::analysis::layout::SizeOf(scope, lowered.type).value_or(0u);
                if (field_size == 0u)
                {
                  continue;
                }
                if (!field_value)
                {
                  llvm::Type *field_ty = emitter.GetLLVMType(lowered.type);
                  if (!field_ty || field_ty->isVoidTy())
                  {
                    continue;
                  }
                  field_value = llvm::Constant::getNullValue(field_ty);
                }
                aggregate =
                    irb.CreateInsertValue(aggregate, field_value, {insert_index++});
              }
              return aggregate;
            }
          }

          return context_field_value(irb, ctx_loaded, field_name);
        };

        llvm::Value *bundle = build_context_bundle(
            build_context_bundle, builder, build.target_type, "", root_ctx_ptr, root_ctx_value);
        emitter.SetTempValue(build.result, bundle ? bundle : DefaultFor(build.result));
      }
      void operator()(const IRBreak &brk) const
      {
        llvm::Value *break_value_slot = emitter.CurrentLoopBreakValueSlot();
        analysis::TypeRef break_result_type = emitter.CurrentLoopBreakResultType();
        if (break_value_slot)
        {
          llvm::Type *target_ty = nullptr;
          if (auto *alloca = llvm::dyn_cast<llvm::AllocaInst>(break_value_slot))
          {
            target_ty = alloca->getAllocatedType();
          }
          if (!target_ty && break_result_type)
          {
            target_ty = emitter.GetLLVMType(break_result_type);
          }
          if (target_ty && !target_ty->isVoidTy())
          {
            analysis::TypeRef source_type = analysis::MakeTypePrim("()");
            llvm::Value *value = nullptr;
            if (brk.value.has_value())
            {
              value = EvaluateOrDefault(*brk.value);
              if (const LowerCtx *ctx = emitter.GetCurrentCtx())
              {
                source_type = ctx->LookupValueType(*brk.value);
              }
              if (!source_type)
              {
                source_type = analysis::MakeTypePrim("()");
              }
            }
            else
            {
              if (llvm::Type *unit_ty = emitter.GetLLVMType(analysis::MakeTypePrim("()")))
              {
                value = llvm::Constant::getNullValue(unit_ty);
              }
            }

            if (!value)
            {
              value = llvm::Constant::getNullValue(target_ty);
            }

            if (break_result_type)
            {
              llvm::Value *coerced = CoerceToTyped(
                  emitter,
                  &builder,
                  value,
                  target_ty,
                  source_type,
                  break_result_type);
              value = coerced ? coerced : llvm::Constant::getNullValue(target_ty);
            }
            else if (value->getType() != target_ty)
            {
              llvm::Value *coerced = CoerceTo(&builder, value, target_ty);
              value = coerced ? coerced : llvm::Constant::getNullValue(target_ty);
            }

            builder.CreateStore(value, break_value_slot);
          }
        }

        if (llvm::BasicBlock *target = emitter.CurrentLoopBreakTarget())
        {
          builder.CreateBr(target);
        }
      }
      void operator()(const IRContinue &) const
      {
        if (llvm::BasicBlock *target = emitter.CurrentLoopContinueTarget())
        {
          builder.CreateBr(target);
        }
      }
      void operator()(const IRDefer &) const {}
      void operator()(const IRMoveState &) const {}
      void operator()(const IRBlock &block) const
      {
        emitter.EmitIR(block.setup);
        emitter.EmitIR(block.body);
        SetForwardedOrMaterializedResult(block.value);
      }
      void operator()(const IRLoop &loop) const
      {
        llvm::Function *func = builder.GetInsertBlock()->getParent();
        if (!func)
        {
          emitter.SetTempValue(loop.result, DefaultFor(loop.result));
          return;
        }

        const LowerCtx *active_ctx = emitter.GetCurrentCtx();
        analysis::TypeRef loop_result_type =
            active_ctx ? active_ctx->LookupValueType(loop.result) : nullptr;
        llvm::Type *loop_result_ty = ExpectedLLVMType(loop.result);
        if (!loop_result_ty && loop_result_type)
        {
          loop_result_ty = emitter.GetLLVMType(loop_result_type);
        }

        llvm::AllocaInst *loop_result_slot = nullptr;
        if (loop_result_ty && !loop_result_ty->isVoidTy())
        {
          llvm::IRBuilder<> entry_builder(
              &func->getEntryBlock(),
              func->getEntryBlock().begin());
          loop_result_slot =
              entry_builder.CreateAlloca(loop_result_ty, nullptr, "loop.result.slot");

          llvm::Value *init_value = llvm::Constant::getNullValue(loop_result_ty);
          if (loop.kind == IRLoopKind::Conditional || loop.kind == IRLoopKind::Iter)
          {
            const analysis::TypeRef unit_type = analysis::MakeTypePrim("()");
            llvm::Type *unit_ty = emitter.GetLLVMType(unit_type);
            llvm::Value *unit_value = unit_ty
                                          ? llvm::Constant::getNullValue(unit_ty)
                                          : llvm::Constant::getNullValue(loop_result_ty);

            if (loop_result_type)
            {
              llvm::Value *coerced = CoerceToTyped(
                  emitter,
                  &builder,
                  unit_value,
                  loop_result_ty,
                  unit_type,
                  loop_result_type);
              if (coerced)
              {
                init_value = coerced;
              }
            }
            else if (unit_value->getType() != loop_result_ty)
            {
              llvm::Value *coerced = CoerceTo(&builder, unit_value, loop_result_ty);
              if (coerced)
              {
                init_value = coerced;
              }
            }
            else
            {
              init_value = unit_value;
            }
          }
          builder.CreateStore(init_value, loop_result_slot);
        }

        auto *loop_end = llvm::BasicBlock::Create(emitter.GetContext(), "loop.end", func);

        if (loop.kind == IRLoopKind::Infinite)
        {
          auto *loop_head = llvm::BasicBlock::Create(emitter.GetContext(), "loop.head", func);
          auto *loop_body = llvm::BasicBlock::Create(emitter.GetContext(), "loop.body", func);
          builder.CreateBr(loop_head);

          builder.SetInsertPoint(loop_head);
          builder.CreateBr(loop_body);

          builder.SetInsertPoint(loop_body);
          emitter.PushLoopTargets(loop_end, loop_head, loop_result_slot, loop_result_type);
          emitter.EmitIR(loop.body_ir);
          emitter.PopLoopTargets();
          if (!builder.GetInsertBlock()->getTerminator())
          {
            builder.CreateBr(loop_head);
          }
        }
        else if (loop.kind == IRLoopKind::Conditional)
        {
          auto *loop_cond = llvm::BasicBlock::Create(emitter.GetContext(), "loop.cond", func);
          auto *loop_body = llvm::BasicBlock::Create(emitter.GetContext(), "loop.body", func);
          builder.CreateBr(loop_cond);

          builder.SetInsertPoint(loop_cond);
          emitter.EmitIR(loop.cond_ir);
          llvm::Value *cond = llvm::ConstantInt::getFalse(emitter.GetContext());
          if (loop.cond_value.has_value())
          {
            cond = AsBool(&builder, EvaluateOrDefault(*loop.cond_value));
          }
          builder.CreateCondBr(cond, loop_body, loop_end);

          builder.SetInsertPoint(loop_body);
          emitter.PushLoopTargets(loop_end, loop_cond, loop_result_slot, loop_result_type);
          emitter.EmitIR(loop.body_ir);
          emitter.PopLoopTargets();
          if (!builder.GetInsertBlock()->getTerminator())
          {
            builder.CreateBr(loop_cond);
          }
        }
        else
        {
          emitter.EmitIR(loop.iter_ir);
          if (builder.GetInsertBlock()->getTerminator())
          {
            // Iterator evaluation already terminated this path (panic/return).
          }
          else
          {
            const bool debug_loop_iter = core::IsDebugEnabled("loop");
            bool handled_async_iter = false;

            if (loop.iter_value.has_value() && active_ctx)
            {
              analysis::TypeRef iter_type =
                  analysis::StripPerm(active_ctx->LookupValueType(*loop.iter_value));
              if (const auto async_sig = analysis::GetAsyncSig(iter_type))
              {
                handled_async_iter = true;
                llvm::Value *iter_async = EvaluateOrDefault(*loop.iter_value);
                auto *iter_async_ty = llvm::dyn_cast<llvm::StructType>(
                    iter_async ? iter_async->getType() : nullptr);
                if (!iter_async || !iter_async_ty || iter_async_ty->getNumElements() < 1 ||
                    !iter_async_ty->getElementType(0)->isIntegerTy())
                {
                  builder.CreateBr(loop_end);
                }
                else
                {
                  llvm::IRBuilder<> entry_builder(
                      &func->getEntryBlock(),
                      func->getEntryBlock().begin());
                  llvm::AllocaInst *iter_async_slot = entry_builder.CreateAlloca(
                      iter_async_ty,
                      nullptr,
                      "iter.async.slot");
                  builder.CreateStore(iter_async, iter_async_slot);

                  llvm::Type *i8_ty = llvm::Type::getInt8Ty(emitter.GetContext());
                  llvm::Type *i64_ty = llvm::Type::getInt64Ty(emitter.GetContext());
                  llvm::Type *opaque_ptr_ty = emitter.GetOpaquePtr();
                  auto *opaque_ptr_ptr_ty = llvm::cast<llvm::PointerType>(opaque_ptr_ty);

                  auto materialize_as_type =
                      [&](llvm::Value *value, llvm::Type *dst_ty) -> llvm::Value *
                  {
                    if (!value || !dst_ty)
                    {
                      return nullptr;
                    }
                    if (value->getType() == dst_ty)
                    {
                      return value;
                    }
                    if (llvm::Value *coerced = CoerceTo(&builder, value, dst_ty))
                    {
                      return coerced;
                    }

                    llvm::AllocaInst *dst_slot = entry_builder.CreateAlloca(dst_ty);
                    builder.CreateStore(llvm::Constant::getNullValue(dst_ty), dst_slot);
                    llvm::AllocaInst *src_slot = entry_builder.CreateAlloca(value->getType());
                    builder.CreateStore(value, src_slot);

                    llvm::Value *dst_i8 = builder.CreateBitCast(
                        dst_slot, llvm::PointerType::get(i8_ty, 0));
                    llvm::Value *src_i8 = builder.CreateBitCast(
                        src_slot, llvm::PointerType::get(i8_ty, 0));
                    const llvm::DataLayout &dl = emitter.GetModule().getDataLayout();
                    const std::uint64_t src_size =
                        static_cast<std::uint64_t>(dl.getTypeAllocSize(value->getType()));
                    const std::uint64_t dst_size =
                        static_cast<std::uint64_t>(dl.getTypeAllocSize(dst_ty));
                    const std::uint64_t copy_size = std::min(src_size, dst_size);
                    if (copy_size > 0)
                    {
                      builder.CreateMemCpy(
                          dst_i8,
                          llvm::Align(1),
                          src_i8,
                          llvm::Align(1),
                          llvm::ConstantInt::get(i64_ty, copy_size));
                    }
                    return builder.CreateLoad(dst_ty, dst_slot);
                  };

                  auto extract_async_payload = [&](llvm::Value *async_value,
                                                   const analysis::TypeRef &payload_type)
                      -> llvm::Value *
                  {
                    if (!async_value || !payload_type || IsUnitTypeRef(payload_type) ||
                        IsNeverTypeRef(payload_type))
                    {
                      return nullptr;
                    }
                    llvm::Type *payload_ty = emitter.GetLLVMType(payload_type);
                    if (!payload_ty || payload_ty->isVoidTy())
                    {
                      return nullptr;
                    }
                    llvm::AllocaInst *payload_async_slot = entry_builder.CreateAlloca(iter_async_ty);
                    builder.CreateStore(async_value, payload_async_slot);
                    llvm::Value *payload_i8 = CreateTaggedPayloadI8Ptr(
                        emitter,
                        &builder,
                        iter_async_ty,
                        payload_async_slot,
                        ::cursive::analysis::layout::kPtrAlign);
                    if (!payload_i8)
                    {
                      return nullptr;
                    }
                    llvm::Value *payload_ptr = builder.CreateBitCast(
                        payload_i8,
                        llvm::PointerType::get(payload_ty, 0));
                    llvm::LoadInst *loaded = builder.CreateLoad(payload_ty, payload_ptr);
                    loaded->setAlignment(llvm::Align(1));
                    return loaded;
                  };

                  auto bind_async_iter_identifier =
                      [&](std::string_view name, llvm::Value *elem_value)
                  {
                    if (name.empty() || !elem_value)
                    {
                      return;
                    }
                    llvm::Value *local = emitter.GetLocal(std::string(name));
                    llvm::AllocaInst *slot = llvm::dyn_cast_or_null<llvm::AllocaInst>(local);
                    if (!slot)
                    {
                      llvm::IRBuilder<> entry_builder_local(
                          &func->getEntryBlock(),
                          func->getEntryBlock().begin());
                      std::string slot_name = std::string(name) + ".iter";
                      slot = entry_builder_local.CreateAlloca(
                          elem_value->getType(),
                          nullptr,
                          slot_name);
                      emitter.RegisterLocalBindStorage(std::string(name), slot);
                      if (async_sig->out)
                      {
                        emitter.SetLocalType(std::string(name), async_sig->out);
                      }
                    }

                    llvm::Type *dst_ty = slot->getAllocatedType();
                    llvm::Value *stored = elem_value;
                    const analysis::TypeRef dst_type =
                        emitter.LookupLocalType(std::string(name));
                    if (dst_type && async_sig->out)
                    {
                      llvm::Value *coerced = CoerceToTyped(
                          emitter,
                          &builder,
                          stored,
                          dst_ty,
                          async_sig->out,
                          dst_type);
                      stored = coerced ? coerced : llvm::Constant::getNullValue(dst_ty);
                    }
                    else if (stored->getType() != dst_ty)
                    {
                      llvm::Value *coerced = CoerceTo(&builder, stored, dst_ty);
                      stored = coerced ? coerced : llvm::Constant::getNullValue(dst_ty);
                    }
                    builder.CreateStore(stored, slot);
                  };

                  auto bind_async_iter_pattern = [&](llvm::Value *elem_value)
                  {
                    if (!loop.pattern || !elem_value)
                    {
                      return;
                    }
                    std::visit(
                        [&](const auto &pat)
                        {
                          using P = std::decay_t<decltype(pat)>;
                          if constexpr (std::is_same_v<P, IRIdentifierPattern>)
                          {
                            bind_async_iter_identifier(pat.name, elem_value);
                          }
                          else if constexpr (std::is_same_v<P, IRTypedPattern>)
                          {
                            bind_async_iter_identifier(pat.name, elem_value);
                          }
                          else if constexpr (std::is_same_v<P, IRWildcardPattern>)
                          {
                            return;
                          }
                        },
                        loop.pattern->node);
                  };

                  const analysis::ScopeContext &scope = BuildScope(active_ctx);
                  const AsyncStateDiscs iter_discs =
                      LoweredAsyncStateDiscs(scope, *async_sig);
                  const std::uint64_t suspended_disc = iter_discs.suspended;
                  const std::uint64_t completed_disc = iter_discs.completed;
                  const std::optional<std::uint64_t> failed_disc =
                      iter_discs.failed;
                  const std::string fn_sym = std::string(func->getName());
                  const LowerCtx::ProcSigInfo *proc_sig =
                      active_ctx ? active_ctx->LookupProcSig(fn_sym) : nullptr;

                  auto emit_async_return = [&](llvm::Value *value,
                                               const analysis::TypeRef &source_type)
                  {
                    llvm::Type *ret_ty = func->getReturnType();
                    if (ret_ty->isVoidTy())
                    {
                      (void)StoreProcedureOutValue(
                          emitter,
                          &builder,
                          func,
                          fn_sym,
                          proc_sig,
                          value,
                          source_type);
                      builder.CreateRetVoid();
                      return;
                    }

                    llvm::Value *out = CoerceToTyped(
                        emitter,
                        &builder,
                        value,
                        ret_ty,
                        source_type,
                        proc_sig ? proc_sig->ret : nullptr);
                    if (!out)
                    {
                      out = CoerceTo(&builder, value, ret_ty);
                    }
                    if (!out)
                    {
                      out = llvm::Constant::getNullValue(ret_ty);
                    }
                    builder.CreateRet(out);
                  };

                  llvm::Value *panic_ptr =
                      LoadLocalValue(emitter, &builder, std::string(kPanicOutName));
                  bool has_panic_ptr = panic_ptr != nullptr;
                  if (panic_ptr)
                  {
                    if (llvm::Value *coerced = CoerceTo(&builder, panic_ptr, opaque_ptr_ty))
                    {
                      panic_ptr = coerced;
                    }
                    else if (panic_ptr->getType()->isPointerTy())
                    {
                      panic_ptr = builder.CreateBitCast(panic_ptr, opaque_ptr_ty);
                    }
                    else
                    {
                      panic_ptr = nullptr;
                      has_panic_ptr = false;
                    }
                  }
                  if (!panic_ptr)
                  {
                    panic_ptr = llvm::ConstantPointerNull::get(opaque_ptr_ptr_ty);
                  }

                  auto *loop_cond =
                      llvm::BasicBlock::Create(emitter.GetContext(), "loop.cond", func);
                  auto *loop_body =
                      llvm::BasicBlock::Create(emitter.GetContext(), "loop.body", func);
                  auto *loop_resume =
                      llvm::BasicBlock::Create(emitter.GetContext(), "loop.resume", func);
                  auto *loop_failed = failed_disc.has_value()
                                          ? llvm::BasicBlock::Create(
                                                emitter.GetContext(),
                                                "loop.failed",
                                                func)
                                          : nullptr;
                  builder.CreateBr(loop_cond);

                  builder.SetInsertPoint(loop_cond);
                  llvm::Value *current_async = builder.CreateLoad(iter_async_ty, iter_async_slot);
                  llvm::Value *disc = builder.CreateExtractValue(current_async, {0u});
                  auto *disc_ty = llvm::cast<llvm::IntegerType>(disc->getType());
                  llvm::SwitchInst *state_sw = builder.CreateSwitch(
                      disc, loop_end, failed_disc.has_value() ? 3 : 2);
                  state_sw->addCase(
                      llvm::ConstantInt::get(disc_ty, suspended_disc), loop_body);
                  state_sw->addCase(
                      llvm::ConstantInt::get(disc_ty, completed_disc), loop_end);
                  if (failed_disc.has_value())
                  {
                    state_sw->addCase(
                        llvm::ConstantInt::get(disc_ty, *failed_disc), loop_failed);
                  }

                  builder.SetInsertPoint(loop_body);
                  bind_async_iter_pattern(extract_async_payload(current_async, async_sig->out));
                  emitter.PushLoopTargets(loop_end, loop_resume, loop_result_slot, loop_result_type);
                  emitter.EmitIR(loop.body_ir);
                  emitter.PopLoopTargets();
                  if (!builder.GetInsertBlock()->getTerminator())
                  {
                    builder.CreateBr(loop_resume);
                  }

                  builder.SetInsertPoint(loop_resume);
                  llvm::Value *suspended_ptr =
                      builder.CreateBitCast(iter_async_slot, opaque_ptr_ty);
                  llvm::Value *unit_input =
                      llvm::ConstantPointerNull::get(opaque_ptr_ptr_ty);
                  llvm::Value *resume_call = EmitAsyncResumeRuntimeCall(
                      emitter,
                      &builder,
                      suspended_ptr,
                      unit_input,
                      panic_ptr);
                  llvm::Value *resumed_async =
                      materialize_as_type(resume_call, iter_async_ty);
                  if (!resumed_async)
                  {
                    resumed_async = llvm::Constant::getNullValue(iter_async_ty);
                  }
                  builder.CreateStore(resumed_async, iter_async_slot);
                  if (has_panic_ptr)
                  {
                    llvm::Value *panic_i8 = panic_ptr;
                    if (panic_i8->getType() != llvm::PointerType::get(i8_ty, 0))
                    {
                      panic_i8 = builder.CreateBitCast(
                          panic_i8, llvm::PointerType::get(i8_ty, 0));
                    }
                    llvm::LoadInst *panic_flag = builder.CreateLoad(i8_ty, panic_i8);
                    llvm::Value *has_panic = builder.CreateICmpNE(
                        panic_flag,
                        llvm::ConstantInt::get(i8_ty, 0));
                    builder.CreateCondBr(has_panic, loop_end, loop_cond);
                  }
                  else
                  {
                    builder.CreateBr(loop_cond);
                  }

                  if (loop_failed)
                  {
                    builder.SetInsertPoint(loop_failed);
                    bool propagated_failure = false;
                    const auto outer_sig =
                        proc_sig ? analysis::GetAsyncSig(proc_sig->ret) : std::nullopt;
                    const std::optional<std::uint64_t> outer_failed_disc =
                        outer_sig ? LoweredAsyncStateDiscs(scope, *outer_sig).failed
                                  : std::nullopt;
                    if (outer_sig && outer_failed_disc.has_value() && async_sig->err)
                    {
                      llvm::Value *failed_async =
                          builder.CreateLoad(iter_async_ty, iter_async_slot);
                      llvm::Value *failed_payload =
                          extract_async_payload(failed_async, async_sig->err);
                      if (!failed_payload &&
                          !IsUnitTypeRef(async_sig->err) &&
                          !IsNeverTypeRef(async_sig->err))
                      {
                        if (llvm::Type *err_ty = emitter.GetLLVMType(async_sig->err))
                        {
                          if (!err_ty->isVoidTy())
                          {
                            failed_payload = llvm::Constant::getNullValue(err_ty);
                          }
                        }
                      }

                      llvm::Value *propagated_value = failed_payload;
                      analysis::TypeRef propagated_type = async_sig->err;
                      llvm::Type *outer_async_ty = emitter.GetLLVMType(proc_sig->ret);
                      auto *outer_async_struct =
                          llvm::dyn_cast_or_null<llvm::StructType>(outer_async_ty);
                      if (outer_async_struct &&
                          outer_async_struct->getNumElements() >= 1 &&
                          outer_async_struct->getElementType(0)->isIntegerTy())
                      {
                        llvm::AllocaInst *outer_async_slot =
                            entry_builder.CreateAlloca(outer_async_struct);
                        builder.CreateStore(
                            llvm::Constant::getNullValue(outer_async_struct),
                            outer_async_slot);

                        llvm::Type *outer_disc_ty = outer_async_struct->getElementType(0);
                        llvm::Value *outer_disc_ptr =
                            builder.CreateStructGEP(outer_async_struct, outer_async_slot, 0);
                        builder.CreateStore(
                            llvm::ConstantInt::get(outer_disc_ty, *outer_failed_disc),
                            outer_disc_ptr);

                        llvm::Value *outer_payload_i8 = CreateTaggedPayloadI8Ptr(
                            emitter,
                            &builder,
                            outer_async_struct,
                            outer_async_slot,
                            ::cursive::analysis::layout::kPtrAlign);
                        if (outer_payload_i8 &&
                            outer_sig->err &&
                            !IsUnitTypeRef(outer_sig->err) &&
                            !IsNeverTypeRef(outer_sig->err))
                        {
                          llvm::Type *outer_err_ty = emitter.GetLLVMType(outer_sig->err);
                          if (outer_err_ty && !outer_err_ty->isVoidTy())
                          {
                            llvm::Value *outer_err_value = failed_payload;
                            if (!outer_err_value)
                            {
                              outer_err_value = llvm::Constant::getNullValue(outer_err_ty);
                            }
                            else if (outer_err_value->getType() != outer_err_ty)
                            {
                              if (llvm::Value *coerced = CoerceToTyped(
                                      emitter,
                                      &builder,
                                      outer_err_value,
                                      outer_err_ty,
                                      async_sig->err,
                                      outer_sig->err))
                              {
                                outer_err_value = coerced;
                              }
                              else if (llvm::Value *coerced_plain =
                                           CoerceTo(&builder, outer_err_value, outer_err_ty))
                              {
                                outer_err_value = coerced_plain;
                              }
                              else
                              {
                                outer_err_value = llvm::Constant::getNullValue(outer_err_ty);
                              }
                            }

                            llvm::AllocaInst *src_slot =
                                entry_builder.CreateAlloca(outer_err_ty);
                            builder.CreateStore(outer_err_value, src_slot);
                            llvm::Value *src_i8 = builder.CreateBitCast(
                                src_slot, llvm::PointerType::get(i8_ty, 0));
                            const llvm::DataLayout &dl = emitter.GetModule().getDataLayout();
                            const std::uint64_t copy_size = static_cast<std::uint64_t>(
                                dl.getTypeAllocSize(outer_err_ty));
                            if (copy_size > 0)
                            {
                              builder.CreateMemCpy(
                                  outer_payload_i8,
                                  llvm::Align(1),
                                  src_i8,
                                  llvm::Align(1),
                                  llvm::ConstantInt::get(i64_ty, copy_size));
                            }
                          }
                        }

                        propagated_value =
                            builder.CreateLoad(outer_async_struct, outer_async_slot);
                        propagated_type = proc_sig->ret;
                      }

                      if (propagated_value)
                      {
                        emit_async_return(propagated_value, propagated_type);
                        propagated_failure =
                            builder.GetInsertBlock()->getTerminator() != nullptr;
                      }
                    }

                    if (!propagated_failure || !builder.GetInsertBlock()->getTerminator())
                    {
                      builder.CreateBr(loop_end);
                    }
                  }
                }
              }
            }

            if (!handled_async_iter)
            {
              std::optional<std::uint64_t> trip_count;
              analysis::TypeRef iter_elem_type = nullptr;
              bool iter_is_range_type = false;
              bool range_iter = false;
              IRRangeKind range_iter_kind = IRRangeKind::Exclusive;
              llvm::Type *range_bound_ty = nullptr;
              llvm::Value *range_lo = nullptr;
              llvm::Value *range_hi = nullptr;
              if (loop.iter_value.has_value())
              {
                trip_count = StaticLengthOf(*loop.iter_value);
                if (active_ctx)
                {
                  analysis::TypeRef iter_type =
                      active_ctx->LookupValueType(*loop.iter_value);
                  if (!iter_type &&
                      loop.iter_value->kind == IRValue::Kind::Local)
                  {
                    iter_type = emitter.LookupLocalType(loop.iter_value->name);
                  }
                  if (analysis::TypeRef stripped = analysis::StripPerm(iter_type))
                  {
                    iter_type = stripped;
                  }
                  if (iter_type)
                  {
                    if (const auto *arr = std::get_if<analysis::TypeArray>(&iter_type->node))
                    {
                      if (!trip_count.has_value())
                      {
                        trip_count = arr->length;
                      }
                      iter_elem_type = arr->element;
                    }
                    else if (const auto *slice =
                                 std::get_if<analysis::TypeSlice>(&iter_type->node))
                    {
                      iter_elem_type = slice->element;
                    }
                    else if (const auto *range =
                                 std::get_if<analysis::TypeRange>(&iter_type->node))
                    {
                      iter_is_range_type = true;
                      iter_elem_type = range->base;
                      range_iter = true;
                      range_iter_kind = IRRangeKind::Exclusive;
                    }
                    else if (const auto *range =
                                 std::get_if<analysis::TypeRangeInclusive>(&iter_type->node))
                    {
                      iter_is_range_type = true;
                      iter_elem_type = range->base;
                      range_iter = true;
                      range_iter_kind = IRRangeKind::Inclusive;
                    }
                    else if (const auto *range =
                                 std::get_if<analysis::TypeRangeFrom>(&iter_type->node))
                    {
                      iter_is_range_type = true;
                      iter_elem_type = range->base;
                      range_iter = true;
                      range_iter_kind = IRRangeKind::From;
                    }
                  }
                }
                if (range_iter)
                {
                  if (!iter_elem_type || !analysis::BuiltinStepType(iter_elem_type))
                  {
                    if (active_ctx)
                    {
                      const_cast<LowerCtx *>(active_ctx)->ReportCodegenFailure();
                    }
                    range_iter = false;
                  }
                }
                if (range_iter)
                {
                  range_bound_ty =
                      iter_elem_type ? emitter.GetLLVMType(iter_elem_type) : nullptr;
                  if (!range_bound_ty || !range_bound_ty->isIntegerTy())
                  {
                    if (active_ctx)
                    {
                      const_cast<LowerCtx *>(active_ctx)->ReportCodegenFailure();
                    }
                    range_iter = false;
                  }
                }
                if (range_iter)
                {
                  if (auto range_value = ResolveRangeValue(
                          *loop.iter_value,
                          range_bound_ty,
                          std::optional<IRRangeKind>(range_iter_kind));
                      range_value.has_value())
                  {
                    range_iter_kind = range_value->kind;
                    range_lo = range_value->lo;
                    range_hi = range_value->hi;
                  }

                  if (!range_lo)
                  {
                    if (active_ctx)
                    {
                      const_cast<LowerCtx *>(active_ctx)->ReportCodegenFailure();
                    }
                    range_iter = false;
                  }
                  const bool finite_range = range_iter_kind != IRRangeKind::From;
                  if (finite_range && !range_hi)
                  {
                    if (active_ctx)
                    {
                      const_cast<LowerCtx *>(active_ctx)->ReportCodegenFailure();
                    }
                    range_iter = false;
                  }
                }
              }

              if (debug_loop_iter)
              {
                const std::string fn_name = func ? func->getName().str() : std::string("<no-func>");
                std::cerr << "[loop-iter-debug] fn=" << fn_name
                          << " trip_count="
                          << (trip_count.has_value() ? std::to_string(*trip_count) : std::string("<none>"))
                          << " iter_elem_type="
                          << (iter_elem_type ? analysis::TypeToString(iter_elem_type) : std::string("<null>"))
                          << " range_iter=" << (range_iter ? "yes" : "no")
                          << "\n";
              }

              llvm::Type *i64_ty = llvm::Type::getInt64Ty(emitter.GetContext());
              llvm::IRBuilder<> entry_builder(
                  &func->getEntryBlock(),
                  func->getEntryBlock().begin());
              llvm::AllocaInst *range_cur_slot = nullptr;
              llvm::AllocaInst *range_done_slot = nullptr;
              std::optional<IndexedSequenceLoweredIterState> indexed_iter;
              if (range_iter)
              {
                range_cur_slot = entry_builder.CreateAlloca(
                    range_bound_ty, nullptr, "iter.range.cur");
                range_done_slot = entry_builder.CreateAlloca(
                    llvm::Type::getInt1Ty(emitter.GetContext()),
                    nullptr,
                    "iter.range.done");
                builder.CreateStore(range_lo, range_cur_slot);
                builder.CreateStore(
                    llvm::ConstantInt::getFalse(emitter.GetContext()),
                    range_done_slot);
              }
              else
              {
                if (loop.iter_value.has_value() && active_ctx)
                {
                  llvm::Value *iter_value = EvaluateOrDefault(*loop.iter_value);
                  analysis::TypeRef iter_type =
                      active_ctx->LookupValueType(*loop.iter_value);
                  if (!iter_type &&
                      loop.iter_value->kind == IRValue::Kind::Local)
                  {
                    iter_type = emitter.LookupLocalType(loop.iter_value->name);
                  }
                  if (analysis::TypeRef stripped = analysis::StripPerm(iter_type))
                  {
                    iter_type = stripped;
                  }
                  if (iter_value &&
                      EmitSeqIterInit(
                          emitter,
                          entry_builder,
                          builder,
                          iter_type,
                          iter_value,
                          indexed_iter.emplace()))
                  {
                  }
                  else
                  {
                    indexed_iter.reset();
                  }
                }
              }
              if (debug_loop_iter)
              {
                const std::string fn_name = func ? func->getName().str() : std::string("<no-func>");
                std::cerr << "[loop-iter-debug] fn=" << fn_name
                          << " indexed_iter=" << (indexed_iter.has_value() ? "yes" : "no")
                          << " range_iter=" << (range_iter ? "yes" : "no")
                          << "\n";
              }

              if ((iter_is_range_type && !range_iter) ||
                  (!range_iter && !indexed_iter.has_value()) ||
                  (range_iter && trip_count.has_value() && *trip_count == 0))
              {
                builder.CreateBr(loop_end);
              }
              else
              {

                auto bind_iter_identifier =
                    [&](std::string_view name, llvm::Value *elem_value)
                {
                  if (name.empty() || !elem_value)
                  {
                    return;
                  }
                  llvm::Value *local = emitter.GetLocal(std::string(name));
                  llvm::AllocaInst *slot = llvm::dyn_cast_or_null<llvm::AllocaInst>(local);
                  if (!slot)
                  {
                    llvm::IRBuilder<> entry_builder(
                        &func->getEntryBlock(),
                        func->getEntryBlock().begin());
                    std::string slot_name = std::string(name) + ".iter";
                    slot = entry_builder.CreateAlloca(
                        elem_value->getType(),
                        nullptr,
                        slot_name);
                    emitter.RegisterLocalBindStorage(std::string(name), slot);
                    if (iter_elem_type)
                    {
                      emitter.SetLocalType(std::string(name), iter_elem_type);
                    }
                    if (debug_loop_iter)
                    {
                      std::cerr << "[loop-iter-debug] bind-alloc name=" << name
                                << " llvm_ty=" << (elem_value->getType()->isIntegerTy() ? "int" : elem_value->getType()->isFloatingPointTy() ? "float"
                                                                                              : elem_value->getType()->isPointerTy()         ? "ptr"
                                                                                              : elem_value->getType()->isStructTy()          ? "struct"
                                                                                                                                             : "other")
                                << "\n";
                    }
                  }

                  llvm::Type *dst_ty = slot->getAllocatedType();
                  llvm::Value *stored = elem_value;
                  const analysis::TypeRef dst_type = emitter.LookupLocalType(std::string(name));
                  if (dst_type && iter_elem_type)
                  {
                    llvm::Value *coerced = CoerceToTyped(
                        emitter,
                        &builder,
                        stored,
                        dst_ty,
                        iter_elem_type,
                        dst_type);
                    stored = coerced ? coerced : llvm::Constant::getNullValue(dst_ty);
                  }
                  else if (stored->getType() != dst_ty)
                  {
                    llvm::Value *coerced = CoerceTo(&builder, stored, dst_ty);
                    stored = coerced ? coerced : llvm::Constant::getNullValue(dst_ty);
                  }
                  builder.CreateStore(stored, slot);
                  if (debug_loop_iter)
                  {
                    std::cerr << "[loop-iter-debug] bind-ok name=" << name << "\n";
                  }
                };

                auto bind_iter_pattern_value = [&](llvm::Value *elem_value)
                {
                  if (!loop.pattern || !elem_value)
                  {
                    return;
                  }
                  std::visit(
                      [&](const auto &pat)
                      {
                        using P = std::decay_t<decltype(pat)>;
                        if constexpr (std::is_same_v<P, IRIdentifierPattern>)
                        {
                          bind_iter_identifier(pat.name, elem_value);
                        }
                        else if constexpr (std::is_same_v<P, IRTypedPattern>)
                        {
                          bind_iter_identifier(pat.name, elem_value);
                        }
                        else if constexpr (std::is_same_v<P, IRWildcardPattern>)
                        {
                          return;
                        }
                      },
                      loop.pattern->node);
                };

                auto *loop_cond = llvm::BasicBlock::Create(emitter.GetContext(), "loop.cond", func);
                auto *loop_body = llvm::BasicBlock::Create(emitter.GetContext(), "loop.body", func);
                auto *loop_inc = llvm::BasicBlock::Create(emitter.GetContext(), "loop.inc", func);
                builder.CreateBr(loop_cond);

                builder.SetInsertPoint(loop_cond);
                llvm::Value *in_range = nullptr;
                if (range_iter)
                {
                  llvm::Value *done = builder.CreateLoad(
                      llvm::Type::getInt1Ty(emitter.GetContext()),
                      range_done_slot);
                  llvm::Value *cur = builder.CreateLoad(range_bound_ty, range_cur_slot);
                  switch (range_iter_kind)
                  {
                  case IRRangeKind::Exclusive:
                  {
                    llvm::Value *at_hi =
                        EmitBuiltinEqCall(builder, iter_elem_type, cur, range_hi);
                    if (!at_hi)
                    {
                      if (active_ctx)
                      {
                        const_cast<LowerCtx *>(active_ctx)->ReportCodegenFailure();
                      }
                      in_range = llvm::ConstantInt::getFalse(emitter.GetContext());
                    }
                    else
                    {
                      in_range = builder.CreateAnd(
                          builder.CreateNot(done),
                          builder.CreateNot(at_hi));
                    }
                    break;
                  }
                  case IRRangeKind::Inclusive:
                    in_range = builder.CreateNot(done);
                    break;
                  case IRRangeKind::From:
                    in_range = builder.CreateNot(done);
                    break;
                  case IRRangeKind::To:
                  case IRRangeKind::ToInclusive:
                  case IRRangeKind::Full:
                    in_range = llvm::ConstantInt::getFalse(emitter.GetContext());
                    break;
                  }
                }
                else
                {
                  if (indexed_iter.has_value())
                  {
                    in_range = EmitSeqIterNext(emitter, builder, *indexed_iter);
                  }
                  else
                  {
                    in_range = llvm::ConstantInt::getFalse(emitter.GetContext());
                  }
                }
                builder.CreateCondBr(in_range, loop_body, loop_end);

                builder.SetInsertPoint(loop_body);
                if (range_iter)
                {
                  bind_iter_pattern_value(builder.CreateLoad(range_bound_ty, range_cur_slot));
                }
                else if (indexed_iter.has_value())
                {
                  if (llvm::Value *elem = LoadSeqIterElem(builder, *indexed_iter))
                  {
                    bind_iter_pattern_value(elem);
                  }
                }
                emitter.PushLoopTargets(loop_end, loop_inc, loop_result_slot, loop_result_type);
                emitter.EmitIR(loop.body_ir);
                emitter.PopLoopTargets();
                if (!builder.GetInsertBlock()->getTerminator())
                {
                  builder.CreateBr(loop_inc);
                }

                builder.SetInsertPoint(loop_inc);
                if (range_iter)
                {
                  llvm::Value *cur = builder.CreateLoad(range_bound_ty, range_cur_slot);
                  const auto successor =
                      EmitBuiltinSuccessor(builder, iter_elem_type, cur);
                  if (!successor.has_value())
                  {
                    if (active_ctx)
                    {
                      const_cast<LowerCtx *>(active_ctx)->ReportCodegenFailure();
                    }
                    builder.CreateStore(
                        llvm::ConstantInt::getTrue(emitter.GetContext()),
                        range_done_slot);
                  }
                  else
                  {
                    llvm::Value *next_or_cur = builder.CreateSelect(
                        successor->has_next,
                        successor->next,
                        cur);
                    switch (range_iter_kind)
                    {
                    case IRRangeKind::Exclusive:
                      builder.CreateStore(
                          builder.CreateNot(successor->has_next),
                          range_done_slot);
                      builder.CreateStore(next_or_cur, range_cur_slot);
                      break;
                    case IRRangeKind::Inclusive:
                    {
                      llvm::Value *at_hi =
                          EmitBuiltinEqCall(builder, iter_elem_type, cur, range_hi);
                      if (!at_hi)
                      {
                        if (active_ctx)
                        {
                          const_cast<LowerCtx *>(active_ctx)->ReportCodegenFailure();
                        }
                        builder.CreateStore(
                            llvm::ConstantInt::getTrue(emitter.GetContext()),
                            range_done_slot);
                      }
                      else
                      {
                        llvm::Value *done_next = builder.CreateOr(
                            at_hi,
                            builder.CreateNot(successor->has_next));
                        builder.CreateStore(done_next, range_done_slot);
                        builder.CreateStore(next_or_cur, range_cur_slot);
                      }
                      break;
                    }
                    case IRRangeKind::From:
                      builder.CreateStore(
                          builder.CreateNot(successor->has_next),
                          range_done_slot);
                      builder.CreateStore(next_or_cur, range_cur_slot);
                      break;
                    case IRRangeKind::To:
                    case IRRangeKind::ToInclusive:
                    case IRRangeKind::Full:
                      builder.CreateStore(
                          llvm::ConstantInt::getTrue(emitter.GetContext()),
                          range_done_slot);
                      break;
                    }
                  }
                }
                else
                {
                }
                builder.CreateBr(loop_cond);
              }
            }
          }
        }

        builder.SetInsertPoint(loop_end);
        if (loop_result_slot && loop_result_ty && !loop_result_ty->isVoidTy())
        {
          if (loop.result.kind == IRValue::Kind::Opaque &&
              IsAddressBackedAggregateType(loop_result_ty))
          {
            llvm::Value *typed_ptr = loop_result_slot;
            llvm::Type *expected_ptr_ty = llvm::PointerType::get(loop_result_ty, 0);
            if (typed_ptr->getType() != expected_ptr_ty)
            {
              typed_ptr = builder.CreateBitCast(typed_ptr, expected_ptr_ty);
            }
            emitter.ForgetTempStorage(loop.result);
            emitter.SetTempStorage(loop.result, typed_ptr);
          }
          else
          {
            emitter.SetTempValue(loop.result, builder.CreateLoad(loop_result_ty, loop_result_slot));
          }
        }
        else
        {
          emitter.SetTempValue(loop.result, DefaultFor(loop.result));
        }
      }
      void operator()(const IRIfCase &if_case) const
      {
        llvm::Function *func = builder.GetInsertBlock()->getParent();
        if (!func)
        {
          emitter.SetTempValue(if_case.result, DefaultFor(if_case.result));
          return;
        }

        llvm::Value *scrutinee = EvaluateOrDefault(if_case.scrutinee);
        if (!scrutinee || if_case.arms.empty())
        {
          emitter.SetTempValue(if_case.result, DefaultFor(if_case.result));
          return;
        }

        auto parse_int_literal = [](const std::string &lexeme) -> std::optional<long long>
        {
          if (lexeme.empty())
          {
            return std::nullopt;
          }
          std::size_t i = 0;
          if (lexeme[i] == '+' || lexeme[i] == '-')
          {
            ++i;
          }
          const std::size_t start = i;
          while (i < lexeme.size() && std::isdigit(static_cast<unsigned char>(lexeme[i])))
          {
            ++i;
          }
          if (i == start)
          {
            return std::nullopt;
          }
          try
          {
            return std::stoll(lexeme.substr(0, i));
          }
          catch (...)
          {
            return std::nullopt;
          }
        };

        const LowerCtx *ctx = emitter.GetCurrentCtx();
        const analysis::ScopeContext &scope = BuildScope(ctx);

        auto normalize_match_type = [&](analysis::TypeRef ty) -> analysis::TypeRef
        {
          if (!ty)
          {
            return nullptr;
          }
          analysis::TypeRef stripped = analysis::StripPerm(ty);
          if (!stripped)
          {
            stripped = ty;
          }
          analysis::TypeRef resolved = ResolveAliasTypeInScope(scope, stripped);
          if (!resolved)
          {
            return stripped;
          }
          analysis::TypeRef resolved_stripped = analysis::StripPerm(resolved);
          if (!resolved_stripped)
          {
            resolved_stripped = resolved;
          }
          return resolved_stripped;
        };

        auto lookup_enum_decl = [&](analysis::TypeRef type,
                                    analysis::TypePath *out_path) -> const ast::EnumDecl *
        {
          type = normalize_match_type(type);
          const auto *path = type ? std::get_if<analysis::TypePathType>(&type->node) : nullptr;
          if (!path)
          {
            return nullptr;
          }
          if (out_path)
          {
            *out_path = path->path;
          }
          if (const ast::EnumDecl* decl = analysis::LookupEnumDecl(scope, path->path))
          {
            return decl;
          }
          if (!scope.current_module.empty() && path->path.size() == 1u)
          {
            analysis::TypePath qualified = scope.current_module;
            qualified.insert(qualified.end(), path->path.begin(), path->path.end());
            if (out_path)
            {
              *out_path = qualified;
            }
            return analysis::LookupEnumDecl(scope, qualified);
          }
          return nullptr;
        };

        auto find_variant = [](const ast::EnumDecl &decl,
                               std::string_view variant_name) -> const ast::VariantDecl *
        {
          for (const auto &variant : decl.variants)
          {
            if (analysis::IdEq(variant.name, std::string(variant_name)))
            {
              return &variant;
            }
          }
          return nullptr;
        };

        auto variant_disc = [&](const ast::EnumDecl &decl,
                                std::string_view variant_name) -> std::optional<std::uint64_t>
        {
          const auto discs = analysis::EnumDiscriminants(decl);
          if (!discs.ok || discs.discs.size() != decl.variants.size())
          {
            return std::nullopt;
          }
          for (std::size_t i = 0; i < decl.variants.size(); ++i)
          {
            if (analysis::IdEq(decl.variants[i].name, std::string(variant_name)))
            {
              return discs.discs[i];
            }
          }
          return std::nullopt;
        };

        struct EnumPayloadMemberInfo
        {
          analysis::TypeRef type;
          std::uint64_t offset = 0;
          std::uint64_t payload_size = 0;
          std::uint64_t payload_align = 1;
          bool ok = false;
        };

        auto payload_member_by_index = [&](const ast::EnumDecl &enum_decl,
                                           const ast::VariantDecl &variant,
                                           std::size_t index) -> EnumPayloadMemberInfo
        {
          EnumPayloadMemberInfo out;
          const auto enum_layout =
              ::cursive::analysis::layout::EnumLayoutOf(scope, enum_decl, ::cursive::analysis::layout::ResolveEnumLayoutOptions(enum_decl.attrs));
          if (!enum_layout.has_value())
          {
            return out;
          }
          if (!variant.payload_opt.has_value())
          {
            return out;
          }
          const auto *payload =
              std::get_if<ast::VariantPayloadTuple>(&*variant.payload_opt);
          if (!payload || index >= payload->elements.size())
          {
            return out;
          }
          std::vector<analysis::TypeRef> field_types;
          field_types.reserve(payload->elements.size());
          for (const auto &elem_ty : payload->elements)
          {
            const auto lowered = ::cursive::analysis::layout::LowerTypeForLayout(scope, elem_ty);
            if (!lowered.has_value())
            {
              return out;
            }
            field_types.push_back(*lowered);
          }
          const auto layout = ::cursive::analysis::layout::RecordLayoutOf(scope, field_types);
          if (!layout.has_value() || index >= layout->offsets.size())
          {
            return out;
          }
          out.type = field_types[index];
          out.offset = layout->offsets[index];
          out.payload_size = enum_layout->payload_size;
          out.payload_align = enum_layout->payload_align;
          out.ok = true;
          return out;
        };

        auto payload_member_by_field = [&](const ast::EnumDecl &enum_decl,
                                           const ast::VariantDecl &variant,
                                           std::string_view field_name) -> EnumPayloadMemberInfo
        {
          EnumPayloadMemberInfo out;
          const auto enum_layout =
              ::cursive::analysis::layout::EnumLayoutOf(scope, enum_decl, ::cursive::analysis::layout::ResolveEnumLayoutOptions(enum_decl.attrs));
          if (!enum_layout.has_value())
          {
            return out;
          }
          if (!variant.payload_opt.has_value())
          {
            return out;
          }
          const auto *payload =
              std::get_if<ast::VariantPayloadRecord>(&*variant.payload_opt);
          if (!payload)
          {
            return out;
          }
          std::vector<analysis::TypeRef> field_types;
          std::vector<std::string> field_names;
          field_types.reserve(payload->fields.size());
          field_names.reserve(payload->fields.size());
          for (const auto &field : payload->fields)
          {
            const auto lowered = ::cursive::analysis::layout::LowerTypeForLayout(scope, field.type);
            if (!lowered.has_value())
            {
              return out;
            }
            field_types.push_back(*lowered);
            field_names.push_back(field.name);
          }
          const auto layout = ::cursive::analysis::layout::RecordLayoutOf(scope, field_types);
          if (!layout.has_value())
          {
            return out;
          }
          for (std::size_t i = 0; i < field_names.size() && i < layout->offsets.size(); ++i)
          {
            if (analysis::IdEq(field_names[i], std::string(field_name)))
            {
              out.type = field_types[i];
              out.offset = layout->offsets[i];
              out.payload_size = enum_layout->payload_size;
              out.payload_align = enum_layout->payload_align;
              out.ok = true;
              break;
            }
          }
          return out;
        };

        auto enum_disc_value = [&](llvm::Value *enum_value) -> llvm::Value *
        {
          if (!enum_value)
          {
            return nullptr;
          }
          if (enum_value->getType()->isIntegerTy())
          {
            return enum_value;
          }
          auto *enum_ty = llvm::dyn_cast<llvm::StructType>(enum_value->getType());
          if (!enum_ty || enum_ty->getNumElements() == 0)
          {
            return nullptr;
          }
          return builder.CreateExtractValue(enum_value, {0u});
        };

        auto load_enum_payload_member = [&](llvm::Value *enum_value,
                                            const EnumPayloadMemberInfo &member) -> llvm::Value *
        {
          if (!enum_value || !member.ok || !member.type)
          {
            return nullptr;
          }
          llvm::Type *member_ty = emitter.GetLLVMType(member.type);
          auto *enum_ty = llvm::dyn_cast<llvm::StructType>(enum_value->getType());
          if (!member_ty || !enum_ty || enum_ty->getNumElements() < 2)
          {
            return nullptr;
          }
          llvm::Function *current_fn =
              builder.GetInsertBlock() ? builder.GetInsertBlock()->getParent() : nullptr;
          if (!current_fn)
          {
            return nullptr;
          }
          llvm::IRBuilder<> entry_builder(
              &current_fn->getEntryBlock(),
              current_fn->getEntryBlock().begin());
          llvm::AllocaInst *enum_slot = entry_builder.CreateAlloca(enum_ty);
          builder.CreateStore(enum_value, enum_slot);

          llvm::Value *payload_i8 = CreateTaggedPayloadI8Ptr(
              emitter,
              &builder,
              enum_ty,
              enum_slot,
              member.payload_align);
          if (!payload_i8)
          {
            return nullptr;
          }

          llvm::Type *i8_ty = llvm::Type::getInt8Ty(emitter.GetContext());
          llvm::Type *i64_ty = llvm::Type::getInt64Ty(emitter.GetContext());
          llvm::Value *field_i8 = builder.CreateGEP(
              i8_ty,
              payload_i8,
              llvm::ConstantInt::get(i64_ty, member.offset));
          llvm::Value *field_ptr = builder.CreateBitCast(
              field_i8,
              llvm::PointerType::get(member_ty, 0));
          llvm::LoadInst *load = builder.CreateLoad(member_ty, field_ptr);
          load->setAlignment(llvm::Align(1));
          return load;
        };

        auto lookup_modal_decl = [&](analysis::TypeRef type,
                                     analysis::TypePath *out_path) -> const ast::ModalDecl *
        {
          type = normalize_match_type(type);
          if (!type)
          {
            return nullptr;
          }
          if (const auto *state = std::get_if<analysis::TypeModalState>(&type->node))
          {
            if (out_path)
            {
              *out_path = state->path;
            }
            return analysis::LookupModalDecl(scope, state->path);
          }
          const auto *path = analysis::AppliedTypePath(*type);
          if (!path)
          {
            return nullptr;
          }
          if (out_path)
          {
            *out_path = *path;
          }
          return analysis::LookupModalDecl(scope, *path);
        };

        auto find_modal_state = [](const ast::ModalDecl &decl,
                                   std::string_view state_name) -> const ast::StateBlock *
        {
          for (const auto &state : decl.states)
          {
            if (analysis::IdEq(state.name, std::string(state_name)))
            {
              return &state;
            }
          }
          return nullptr;
        };

        auto modal_state_disc =
            [&](const ast::ModalDecl &decl,
                std::string_view state_name) -> std::optional<std::uint64_t>
        {
          for (std::size_t i = 0; i < decl.states.size(); ++i)
          {
            if (analysis::IdEq(decl.states[i].name, std::string(state_name)))
            {
              return static_cast<std::uint64_t>(i);
            }
          }
          return std::nullopt;
        };

        struct ModalPayloadMemberInfo
        {
          analysis::TypeRef type;
          std::uint64_t offset = 0;
          std::uint64_t payload_size = 0;
          std::uint64_t payload_align = 1;
          bool tagged = true;
          bool ok = false;
        };

        auto modal_payload_member_by_field = [&](const ast::ModalDecl &modal_decl,
                                                 const std::vector<analysis::TypeRef> &modal_args,
                                                 std::string_view state_name,
                                                 std::string_view field_name)
            -> ModalPayloadMemberInfo
        {
          ModalPayloadMemberInfo out;
          analysis::TypeSubst modal_subst;
          if (modal_decl.generic_params && !modal_decl.generic_params->params.empty())
          {
            if (modal_args.size() > modal_decl.generic_params->params.size())
            {
              return out;
            }
            modal_subst = analysis::BuildSubstitution(
                modal_decl.generic_params->params,
                modal_args);
          }

          const auto modal_layout = ::cursive::analysis::layout::ModalLayoutOf(scope, modal_decl, modal_args);
          if (!modal_layout.has_value())
          {
            return out;
          }
          out.payload_size = modal_layout->payload_size;
          out.payload_align = modal_layout->payload_align;
          out.tagged = modal_layout->disc_type.has_value();

          const ast::StateBlock *state = find_modal_state(modal_decl, state_name);
          if (!state)
          {
            return out;
          }

          std::vector<analysis::TypeRef> field_types;
          std::vector<std::string> field_names;
          for (const auto &member : state->members)
          {
            const auto *field = std::get_if<ast::StateFieldDecl>(&member);
            if (!field)
            {
              continue;
            }
            const auto lowered = ::cursive::analysis::layout::LowerTypeForLayout(scope, field->type);
            if (!lowered.has_value())
            {
              return out;
            }
            analysis::TypeRef field_type = *lowered;
            if (!modal_subst.empty())
            {
              field_type = analysis::InstantiateType(field_type, modal_subst);
            }
            field_types.push_back(field_type);
            field_names.push_back(field->name);
          }

          const auto layout = ::cursive::analysis::layout::RecordLayoutOf(scope, field_types);
          if (!layout.has_value())
          {
            return out;
          }
          for (std::size_t i = 0; i < field_names.size() && i < layout->offsets.size(); ++i)
          {
            if (analysis::IdEq(field_names[i], std::string(field_name)))
            {
              out.type = field_types[i];
              out.offset = layout->offsets[i];
              out.ok = true;
              break;
            }
          }
          return out;
        };

        auto modal_disc_value = [&](llvm::Value *modal_value) -> llvm::Value *
        {
          if (!modal_value)
          {
            return nullptr;
          }
          if (modal_value->getType()->isIntegerTy())
          {
            // Some tagged modal values with zero-sized payload lower to a raw
            // discriminant scalar. Pattern state checks must compare that
            // discriminant directly.
            return modal_value;
          }
          auto *modal_ty = llvm::dyn_cast<llvm::StructType>(modal_value->getType());
          if (!modal_ty || modal_ty->getNumElements() == 0)
          {
            return nullptr;
          }
          return builder.CreateExtractValue(modal_value, {0u});
        };

        auto load_modal_payload_member = [&](llvm::Value *modal_value,
                                             const ModalPayloadMemberInfo &member) -> llvm::Value *
        {
          if (!modal_value || !member.ok || !member.type)
          {
            return nullptr;
          }
          llvm::Type *member_ty = emitter.GetLLVMType(member.type);
          if (!member_ty)
          {
            return nullptr;
          }
          llvm::Function *current_fn =
              builder.GetInsertBlock() ? builder.GetInsertBlock()->getParent() : nullptr;
          if (!current_fn)
          {
            return nullptr;
          }

          llvm::IRBuilder<> entry_builder(
              &current_fn->getEntryBlock(),
              current_fn->getEntryBlock().begin());
          llvm::AllocaInst *modal_slot = entry_builder.CreateAlloca(modal_value->getType());
          builder.CreateStore(modal_value, modal_slot);

          llvm::Type *i8_ty = llvm::Type::getInt8Ty(emitter.GetContext());
          llvm::Type *i64_ty = llvm::Type::getInt64Ty(emitter.GetContext());
          llvm::Value *payload_i8 = nullptr;
          if (member.tagged)
          {
            auto *modal_ty = llvm::dyn_cast<llvm::StructType>(modal_value->getType());
            if (!modal_ty || modal_ty->getNumElements() < 2)
            {
              const auto member_size = ::cursive::analysis::layout::SizeOf(scope, member.type);
              if (member_size.has_value() && *member_size == 0)
              {
                return llvm::Constant::getNullValue(member_ty);
              }
              return nullptr;
            }
            payload_i8 = CreateTaggedPayloadI8Ptr(
                emitter,
                &builder,
                modal_ty,
                modal_slot,
                member.payload_align);
          }
          else
          {
            payload_i8 = builder.CreateBitCast(
                modal_slot,
                llvm::PointerType::get(i8_ty, 0));
          }
          if (!payload_i8)
          {
            return nullptr;
          }

          llvm::Value *field_i8 = builder.CreateGEP(
              i8_ty,
              payload_i8,
              llvm::ConstantInt::get(i64_ty, member.offset));
          llvm::Value *field_ptr = builder.CreateBitCast(
              field_i8,
              llvm::PointerType::get(member_ty, 0));
          llvm::LoadInst *load = builder.CreateLoad(member_ty, field_ptr);
          load->setAlignment(llvm::Align(1));
          return load;
        };

        std::function<llvm::Value *(const IRPatternPtr &,
                                    llvm::Value *,
                                    analysis::TypeRef)>
            emit_pattern_cond_for_value;

        emit_pattern_cond_for_value =
            [&](const IRPatternPtr &pattern,
                llvm::Value *subject,
                analysis::TypeRef subject_type) -> llvm::Value *
        {
          if (!pattern)
          {
            return llvm::ConstantInt::getTrue(emitter.GetContext());
          }
          return std::visit(
              [&](const auto &pat) -> llvm::Value *
              {
                using P = std::decay_t<decltype(pat)>;
                if constexpr (std::is_same_v<P, IRWildcardPattern> ||
                              std::is_same_v<P, IRIdentifierPattern>)
                {
                  return llvm::ConstantInt::getTrue(emitter.GetContext());
                }
                else if constexpr (std::is_same_v<P, IRTypedPattern>)
                {
                  if (!subject || !subject_type)
                  {
                    return llvm::ConstantInt::getFalse(emitter.GetContext());
                  }

                  analysis::TypeRef scrut = normalize_match_type(subject_type);
                  analysis::TypeRef typed_target = nullptr;
                  if (pat.type)
                  {
                    typed_target = analysis::StripPerm(pat.type);
                    if (!typed_target)
                    {
                      typed_target = pat.type;
                    }
                  }

                  const auto *target_modal_state =
                      typed_target
                          ? std::get_if<analysis::TypeModalState>(&typed_target->node)
                          : nullptr;
                  if (target_modal_state)
                  {
                    auto same_modal_path = [](const analysis::TypePath &lhs,
                                              const analysis::TypePath &rhs) -> bool
                    {
                      if (lhs.size() != rhs.size())
                      {
                        return false;
                      }
                      for (std::size_t i = 0; i < lhs.size(); ++i)
                      {
                        if (!analysis::IdEq(lhs[i], rhs[i]))
                        {
                          return false;
                        }
                      }
                      return true;
                    };

                    const auto *scrut_modal_state =
                        scrut ? std::get_if<analysis::TypeModalState>(&scrut->node) : nullptr;
                    if (scrut_modal_state &&
                        same_modal_path(scrut_modal_state->path, target_modal_state->path))
                    {
                      return analysis::IdEq(scrut_modal_state->state, target_modal_state->state)
                                 ? llvm::ConstantInt::getTrue(emitter.GetContext())
                                 : llvm::ConstantInt::getFalse(emitter.GetContext());
                    }

                    const auto *scrut_modal_ref =
                        scrut ? std::get_if<analysis::TypePathType>(&scrut->node) : nullptr;
                    if (scrut_modal_ref &&
                        same_modal_path(scrut_modal_ref->path, target_modal_state->path))
                    {
                      const ast::ModalDecl *modal_decl =
                          analysis::LookupModalDecl(scope, target_modal_state->path);
                      if (!modal_decl)
                      {
                        return llvm::ConstantInt::getFalse(emitter.GetContext());
                      }
                      const auto expected_disc =
                          modal_state_disc(*modal_decl, target_modal_state->state);
                      llvm::Value *disc = modal_disc_value(subject);
                      if (!expected_disc.has_value() || !disc ||
                          !disc->getType()->isIntegerTy())
                      {
                        return llvm::ConstantInt::getFalse(emitter.GetContext());
                      }
                      return EmitTypedEq(
                          &builder,
                          disc,
                          llvm::ConstantInt::get(disc->getType(), *expected_disc));
                    }
                  }

                  const auto *uni = scrut ? std::get_if<analysis::TypeUnion>(&scrut->node) : nullptr;
                  if (!uni || uni->members.empty())
                  {
                    // Typed patterns over non-unions are irrefutable in if-case contexts
                    // once typechecking has succeeded.
                    return llvm::ConstantInt::getTrue(emitter.GetContext());
                  }

                  auto parse_case_index = [](std::string_view name) -> std::optional<std::size_t>
                  {
                    constexpr std::string_view prefix = "__case";
                    if (name.size() <= prefix.size() || name.substr(0, prefix.size()) != prefix)
                    {
                      return std::nullopt;
                    }
                    std::size_t idx = 0;
                    for (std::size_t i = prefix.size(); i < name.size(); ++i)
                    {
                      const char ch = name[i];
                      if (ch < '0' || ch > '9')
                      {
                        return std::nullopt;
                      }
                      idx = idx * 10 + static_cast<std::size_t>(ch - '0');
                    }
                    return idx;
                  };
                  auto is_unit_type = [](const analysis::TypeRef &type) -> bool
                  {
                    if (!type)
                    {
                      return false;
                    }
                    analysis::TypeRef stripped = analysis::StripPerm(type);
                    if (!stripped)
                    {
                      return false;
                    }
                    if (const auto *prim = std::get_if<analysis::TypePrim>(&stripped->node))
                    {
                      return prim->name == "()";
                    }
                    return false;
                  };
                  auto find_member_index = [](const std::vector<analysis::TypeRef> &members,
                                              const analysis::TypeRef &target)
                      -> std::optional<std::size_t>
                  {
                    auto strip_perm_refine = [](analysis::TypeRef type) -> analysis::TypeRef
                    {
                      while (type)
                      {
                        if (const auto *perm = std::get_if<analysis::TypePerm>(&type->node))
                        {
                          type = perm->base;
                          continue;
                        }
                        if (const auto *refine = std::get_if<analysis::TypeRefine>(&type->node))
                        {
                          type = refine->base;
                          continue;
                        }
                        break;
                      }
                      return type;
                    };
                    if (!target)
                    {
                      return std::nullopt;
                    }
                    const analysis::TypeRef target_base = strip_perm_refine(target);
                    for (std::size_t i = 0; i < members.size(); ++i)
                    {
                      const auto equiv =
                          analysis::TypeEquiv(strip_perm_refine(members[i]), target_base);
                      if (equiv.ok && equiv.equiv)
                      {
                        return i;
                      }
                    }
                    return std::nullopt;
                  };

                  std::vector<analysis::TypeRef> members = uni->members;
                  std::optional<::cursive::analysis::layout::UnionLayout> union_layout = ::cursive::analysis::layout::UnionLayoutOf(scope, *uni);
                  if (union_layout.has_value())
                  {
                    members = union_layout->member_list;
                  }

                  std::optional<std::size_t> member_index;
                  if (pat.type)
                  {
                    member_index = find_member_index(members, pat.type);
                  }
                  if (!member_index.has_value())
                  {
                    member_index = parse_case_index(pat.name);
                  }
                  if (!member_index.has_value() || *member_index >= members.size())
                  {
                    return llvm::ConstantInt::getFalse(emitter.GetContext());
                  }

                  if (union_layout.has_value() && union_layout->niche)
                  {
                    // Cursive0 niche layout is currently used for pointer-valid payload unions.
                    // All non-payload members are unit and represented by the niche value.
                    std::optional<std::size_t> payload_index;
                    for (std::size_t i = 0; i < members.size(); ++i)
                    {
                      if (!is_unit_type(members[i]))
                      {
                        payload_index = i;
                        break;
                      }
                    }
                    if (!payload_index.has_value())
                    {
                      return llvm::ConstantInt::getFalse(emitter.GetContext());
                    }

                    llvm::Constant *niche_zero = llvm::Constant::getNullValue(subject->getType());
                    if (*member_index == *payload_index)
                    {
                      return builder.CreateICmpNE(subject, niche_zero);
                    }
                    return builder.CreateICmpEQ(subject, niche_zero);
                  }

                  auto *subject_struct = llvm::dyn_cast<llvm::StructType>(subject->getType());
                  if (!subject_struct || subject_struct->getNumElements() < 1)
                  {
                    return llvm::ConstantInt::getFalse(emitter.GetContext());
                  }
                  llvm::Value *disc = builder.CreateExtractValue(subject, {0u});
                  if (!disc || !disc->getType()->isIntegerTy())
                  {
                    return llvm::ConstantInt::getFalse(emitter.GetContext());
                  }
                  return EmitTypedEq(
                      &builder,
                      disc,
                      llvm::ConstantInt::get(disc->getType(), *member_index));
                }
                else if constexpr (std::is_same_v<P, IRLiteralPattern>)
                {
                  if (!subject)
                  {
                    return llvm::ConstantInt::getFalse(emitter.GetContext());
                  }
                  llvm::Value *lit = nullptr;
                  if (pat.literal.kind == IRLiteralKind::Bool)
                  {
                    lit = llvm::ConstantInt::get(
                        llvm::Type::getInt1Ty(emitter.GetContext()),
                        pat.literal.lexeme == "true" ? 1 : 0);
                  }
                  else if (pat.literal.kind == IRLiteralKind::Int)
                  {
                    if (auto parsed = parse_int_literal(pat.literal.lexeme))
                    {
                      if (subject->getType()->isIntegerTy())
                      {
                        lit = llvm::ConstantInt::get(
                            subject->getType(),
                            static_cast<uint64_t>(*parsed),
                            true);
                      }
                    }
                  }
                  if (!lit)
                  {
                    return llvm::ConstantInt::getFalse(emitter.GetContext());
                  }
                  return EmitTypedEq(&builder, subject, lit);
                }
                else if constexpr (std::is_same_v<P, IRRangePattern>)
                {
                  if (!subject || !subject->getType()->isIntegerTy())
                  {
                    return llvm::ConstantInt::getFalse(emitter.GetContext());
                  }

                  auto parse_const_pattern_int =
                      [&](const IRPatternPtr &bound) -> std::optional<long long>
                  {
                    if (!bound)
                    {
                      return std::nullopt;
                    }
                    const auto *lit = std::get_if<IRLiteralPattern>(&bound->node);
                    if (!lit || lit->literal.kind != IRLiteralKind::Int)
                    {
                      return std::nullopt;
                    }
                    return parse_int_literal(lit->literal.lexeme);
                  };

                  const std::optional<long long> lo = parse_const_pattern_int(pat.lo);
                  const std::optional<long long> hi = parse_const_pattern_int(pat.hi);
                  if (!lo.has_value() || !hi.has_value())
                  {
                    return llvm::ConstantInt::getFalse(emitter.GetContext());
                  }

                  const bool is_signed = subject_type ? IsSignedIntegerType(subject_type) : true;
                  llvm::Value *lo_value = llvm::ConstantInt::get(
                      subject->getType(),
                      static_cast<std::uint64_t>(*lo),
                      true);
                  llvm::Value *hi_value = llvm::ConstantInt::get(
                      subject->getType(),
                      static_cast<std::uint64_t>(*hi),
                      true);

                  llvm::Value *lower_ok = is_signed
                                              ? builder.CreateICmpSGE(subject, lo_value)
                                              : builder.CreateICmpUGE(subject, lo_value);

                  llvm::Value *upper_ok = nullptr;
                  if (pat.kind == IRRangeKind::Inclusive)
                  {
                    upper_ok = is_signed
                                   ? builder.CreateICmpSLE(subject, hi_value)
                                   : builder.CreateICmpULE(subject, hi_value);
                  }
                  else if (pat.kind == IRRangeKind::Exclusive)
                  {
                    upper_ok = is_signed
                                   ? builder.CreateICmpSLT(subject, hi_value)
                                   : builder.CreateICmpULT(subject, hi_value);
                  }
                  else
                  {
                    return llvm::ConstantInt::getFalse(emitter.GetContext());
                  }

                  return builder.CreateAnd(lower_ok, upper_ok);
                }
                else if constexpr (std::is_same_v<P, IRTuplePattern>)
                {
                  if (!subject || !subject_type)
                  {
                    return llvm::ConstantInt::getFalse(emitter.GetContext());
                  }
                  const analysis::TypeRef stripped_subject =
                      normalize_match_type(subject_type);
                  const auto *tuple_type =
                      stripped_subject
                          ? std::get_if<analysis::TypeTuple>(&stripped_subject->node)
                          : nullptr;
                  if (!tuple_type)
                  {
                    return llvm::ConstantInt::getFalse(emitter.GetContext());
                  }
                  auto *tuple_struct = llvm::dyn_cast<llvm::StructType>(subject->getType());
                  if (!tuple_struct ||
                      tuple_struct->getNumElements() < pat.elements.size() ||
                      tuple_type->elements.size() < pat.elements.size())
                  {
                    return llvm::ConstantInt::getFalse(emitter.GetContext());
                  }
                  llvm::Value *tuple_ok = llvm::ConstantInt::getTrue(emitter.GetContext());
                  for (std::size_t i = 0; i < pat.elements.size(); ++i)
                  {
                    llvm::Value *elem_value =
                        builder.CreateExtractValue(subject, {static_cast<unsigned>(i)});
                    llvm::Value *elem_ok = emit_pattern_cond_for_value(
                        pat.elements[i], elem_value, tuple_type->elements[i]);
                    tuple_ok = builder.CreateAnd(
                        AsBool(&builder, tuple_ok),
                        AsBool(&builder, elem_ok));
                  }
                  return tuple_ok;
                }
                else if constexpr (std::is_same_v<P, IRRecordPattern>)
                {
                  if (!subject || !subject_type)
                  {
                    return llvm::ConstantInt::getFalse(emitter.GetContext());
                  }
                  const analysis::TypeRef stripped_subject =
                      normalize_match_type(subject_type);
                  const auto *path_type =
                      stripped_subject
                          ? std::get_if<analysis::TypePathType>(&stripped_subject->node)
                          : nullptr;
                  if (!path_type)
                  {
                    return llvm::ConstantInt::getFalse(emitter.GetContext());
                  }
                  auto same_path = [](const analysis::TypePath &lhs,
                                      const analysis::TypePath &rhs) -> bool
                  {
                    if (lhs.size() != rhs.size())
                    {
                      return false;
                    }
                    for (std::size_t i = 0; i < lhs.size(); ++i)
                    {
                      if (!analysis::IdEq(lhs[i], rhs[i]))
                      {
                        return false;
                      }
                    }
                    return true;
                  };
                  if (!same_path(path_type->path, pat.path))
                  {
                    return llvm::ConstantInt::getFalse(emitter.GetContext());
                  }
                  auto *record_struct = llvm::dyn_cast<llvm::StructType>(subject->getType());
                  if (!record_struct)
                  {
                    return llvm::ConstantInt::getFalse(emitter.GetContext());
                  }
                  llvm::Value *record_ok = llvm::ConstantInt::getTrue(emitter.GetContext());
                  for (const auto &field : pat.fields)
                  {
                    if (!field.pattern)
                    {
                      continue;
                    }
                    const auto field_meta =
                        ResolveFieldAccessMeta(scope, stripped_subject, field.name);
                    if (!field_meta.has_value() ||
                        field_meta->index >= record_struct->getNumElements())
                    {
                      return llvm::ConstantInt::getFalse(emitter.GetContext());
                    }
                    llvm::Value *field_value = builder.CreateExtractValue(
                        subject, {static_cast<unsigned>(field_meta->index)});
                    llvm::Value *field_ok = emit_pattern_cond_for_value(
                        field.pattern, field_value, field_meta->field_type);
                    record_ok = builder.CreateAnd(
                        AsBool(&builder, record_ok),
                        AsBool(&builder, field_ok));
                  }
                  return record_ok;
                }
                else if constexpr (std::is_same_v<P, IREnumPattern>)
                {
                  const ast::EnumDecl *enum_decl = nullptr;
                  analysis::TypePath enum_path;
                  if (!pat.path.empty())
                  {
                    enum_decl = analysis::LookupEnumDecl(scope, pat.path);
                    if (!enum_decl && !scope.current_module.empty() && pat.path.size() == 1u)
                    {
                      analysis::TypePath qualified = scope.current_module;
                      qualified.insert(qualified.end(), pat.path.begin(), pat.path.end());
                      enum_decl = analysis::LookupEnumDecl(scope, qualified);
                      if (enum_decl)
                      {
                        enum_path = qualified;
                      }
                    }
                    else if (enum_decl)
                    {
                      enum_path = pat.path;
                    }
                  }
                  if (!enum_decl)
                  {
                    enum_decl = lookup_enum_decl(subject_type, &enum_path);
                  }
                  if (!enum_decl || !subject)
                  {
                    return llvm::ConstantInt::getFalse(emitter.GetContext());
                  }
                  const auto expected_disc = variant_disc(*enum_decl, pat.name);
                  llvm::Value *actual_disc = enum_disc_value(subject);
                  if (!expected_disc.has_value() || !actual_disc)
                  {
                    return llvm::ConstantInt::getFalse(emitter.GetContext());
                  }
                  if (core::IsDebugEnabled("obj") &&
                      DebugTargetEnumPath(enum_path.empty() ? pat.path : enum_path))
                  {
                    std::cerr << "[ifcase-enum] path="
                              << core::StringOfPath(enum_path.empty() ? pat.path : enum_path)
                              << " variant=" << pat.name
                              << " expected="
                              << static_cast<unsigned long long>(*expected_disc)
                              << " subject_type="
                              << (subject_type ? analysis::TypeToString(subject_type) : "<null>")
                              << " actual_disc=" << LLVMValueRepr(actual_disc)
                              << " subject=" << LLVMValueRepr(subject)
                              << "\n";
                  }
                  llvm::Value *disc_eq = EmitTypedEq(
                      &builder,
                      actual_disc,
                      llvm::ConstantInt::get(actual_disc->getType(), *expected_disc));
                  if (!pat.payload.has_value())
                  {
                    return disc_eq;
                  }
                  const ast::VariantDecl *variant = find_variant(*enum_decl, pat.name);
                  if (!variant)
                  {
                    return llvm::ConstantInt::getFalse(emitter.GetContext());
                  }
                  llvm::Value *payload_ok = llvm::ConstantInt::getTrue(emitter.GetContext());
                  std::visit(
                      [&](const auto &payload_pattern)
                      {
                        using PayloadP = std::decay_t<decltype(payload_pattern)>;
                        if constexpr (std::is_same_v<PayloadP, IRTuplePayloadPattern>)
                        {
                          for (std::size_t i = 0; i < payload_pattern.elements.size(); ++i)
                          {
                            const auto member = payload_member_by_index(*enum_decl, *variant, i);
                            llvm::Value *member_val = load_enum_payload_member(subject, member);
                            llvm::Value *member_ok = emit_pattern_cond_for_value(
                                payload_pattern.elements[i],
                                member_val,
                                member.type);
                            payload_ok = builder.CreateAnd(
                                AsBool(&builder, payload_ok),
                                AsBool(&builder, member_ok));
                          }
                        }
                        else
                        {
                          for (const auto &field : payload_pattern.fields)
                          {
                            if (!field.pattern)
                            {
                              continue;
                            }
                            const auto member =
                                payload_member_by_field(*enum_decl, *variant, field.name);
                            llvm::Value *member_val = load_enum_payload_member(subject, member);
                            llvm::Value *member_ok = emit_pattern_cond_for_value(
                                field.pattern,
                                member_val,
                                member.type);
                            payload_ok = builder.CreateAnd(
                                AsBool(&builder, payload_ok),
                                AsBool(&builder, member_ok));
                          }
                        }
                      },
                      *pat.payload);
                  return builder.CreateAnd(
                      AsBool(&builder, disc_eq),
                      AsBool(&builder, payload_ok));
                }
                else if constexpr (std::is_same_v<P, IRModalPattern>)
                {
                  if (!subject)
                  {
                    return llvm::ConstantInt::getFalse(emitter.GetContext());
                  }
                  analysis::TypePath modal_path;
                  const ast::ModalDecl *modal_decl = lookup_modal_decl(subject_type, &modal_path);
                  if (!modal_decl)
                  {
                    return llvm::ConstantInt::getFalse(emitter.GetContext());
                  }

                  analysis::TypeRef stripped_subject = normalize_match_type(subject_type);
                  const auto *subject_modal_state =
                      stripped_subject
                          ? std::get_if<analysis::TypeModalState>(&stripped_subject->node)
                          : nullptr;
                  const auto *subject_modal_path =
                      stripped_subject ? analysis::AppliedTypePath(*stripped_subject)
                                       : nullptr;
                  const auto *subject_modal_path_args =
                      stripped_subject ? analysis::AppliedTypeArgs(*stripped_subject)
                                       : nullptr;
                  std::vector<analysis::TypeRef> subject_modal_args;
                  if (subject_modal_state)
                  {
                    subject_modal_args = subject_modal_state->generic_args;
                  }
                  else if (subject_modal_path && subject_modal_path_args)
                  {
                    subject_modal_args = *subject_modal_path_args;
                  }
                  const auto modal_layout = ::cursive::analysis::layout::ModalLayoutOf(scope, *modal_decl, subject_modal_args);
                  const bool subject_is_modal_state = (subject_modal_state != nullptr);
                  const bool subject_is_async_modal_state =
                      subject_modal_state && analysis::IsAsyncType(stripped_subject);
                  llvm::Value *runtime_disc = modal_disc_value(subject);

                  llvm::Value *state_ok = llvm::ConstantInt::getTrue(emitter.GetContext());
                  if (subject_modal_state)
                  {
                    if (!analysis::IdEq(subject_modal_state->state, pat.state))
                    {
                      return llvm::ConstantInt::getFalse(emitter.GetContext());
                    }
                  }
                  else if ((modal_layout.has_value() && modal_layout->disc_type.has_value()) ||
                           runtime_disc != nullptr)
                  {
                    const auto expected_disc = modal_state_disc(*modal_decl, pat.state);
                    if (!expected_disc.has_value() || !runtime_disc)
                    {
                      return llvm::ConstantInt::getFalse(emitter.GetContext());
                    }
                    state_ok = EmitTypedEq(
                        &builder,
                        runtime_disc,
                        llvm::ConstantInt::get(runtime_disc->getType(), *expected_disc));
                  }
                  else
                  {
                    const auto payload_state = analysis::PayloadState(scope, *modal_decl);
                    if (!payload_state.has_value() ||
                        !analysis::IdEq(std::string(*payload_state), pat.state))
                    {
                      return llvm::ConstantInt::getFalse(emitter.GetContext());
                    }
                  }

                  llvm::Value *payload_ok = llvm::ConstantInt::getTrue(emitter.GetContext());
                  if (pat.fields.has_value())
                  {
                    for (const auto &field : pat.fields->fields)
                    {
                      if (!field.pattern)
                      {
                        continue;
                      }
                      auto member =
                          modal_payload_member_by_field(*modal_decl,
                                                        subject_modal_args,
                                                        pat.state,
                                                        field.name);
                      if (subject_is_modal_state && !subject_is_async_modal_state)
                      {
                        member.tagged = false;
                      }
                      llvm::Value *member_val = load_modal_payload_member(subject, member);
                      llvm::Value *member_ok = emit_pattern_cond_for_value(
                          field.pattern,
                          member_val,
                          member.type);
                      payload_ok = builder.CreateAnd(
                          AsBool(&builder, payload_ok),
                          AsBool(&builder, member_ok));
                    }
                  }

                  return builder.CreateAnd(
                      AsBool(&builder, state_ok),
                      AsBool(&builder, payload_ok));
                }
                else
                {
                  // Conservative fallback for patterns without direct predicate IR.
                  return llvm::ConstantInt::getTrue(emitter.GetContext());
                }
              },
              pattern->node);
        };

        auto emit_pattern_cond = [&](const IRPatternPtr &pattern) -> llvm::Value *
        {
          analysis::TypeRef scrut_type = if_case.scrutinee_type;
          if (!scrut_type && ctx)
          {
            scrut_type = ctx->LookupValueType(if_case.scrutinee);
          }
          return emit_pattern_cond_for_value(pattern, scrutinee, scrut_type);
        };

        llvm::BasicBlock *merge_bb =
            llvm::BasicBlock::Create(emitter.GetContext(), "ifcase.merge", func);
        struct IncomingValue
        {
          llvm::BasicBlock *pred = nullptr;
          llvm::Value *value = nullptr;
          llvm::Value *storage = nullptr;
        };
        std::vector<IncomingValue> incoming;
        llvm::BasicBlock *fallback_pred = nullptr;
        const LLVMEmitter::FlowStateSnapshot branch_state =
            emitter.SaveFlowState();

        for (std::size_t i = 0; i < if_case.arms.size(); ++i)
        {
          const IRIfCaseClause &arm = if_case.arms[i];
          const bool is_last = (i + 1 == if_case.arms.size());

          llvm::BasicBlock *arm_bb =
              llvm::BasicBlock::Create(emitter.GetContext(), "ifcase.case", func);
          llvm::BasicBlock *next_bb = is_last
                                          ? nullptr
                                          : llvm::BasicBlock::Create(emitter.GetContext(), "ifcase.next", func);

          emitter.RestoreFlowState(branch_state);
          llvm::Value *cond = AsBool(&builder, emit_pattern_cond(arm.pattern));
          if (is_last)
          {
            fallback_pred = builder.GetInsertBlock();
            builder.CreateCondBr(cond, arm_bb, merge_bb);
          }
          else
          {
            builder.CreateCondBr(cond, arm_bb, next_bb);
          }

          builder.SetInsertPoint(arm_bb);
          emitter.RestoreFlowState(branch_state);
        emitter.EmitIR(arm.body);
        llvm::BasicBlock *arm_end = builder.GetInsertBlock();
        if (!arm_end->getTerminator())
        {
          llvm::Value *arm_storage = emitter.GetAddressableStorage(arm.value);
          llvm::Value *arm_value = EvaluateOrDefault(arm.value);
          builder.CreateBr(merge_bb);
          incoming.push_back({arm_end, arm_value, arm_storage});
        }

          if (!is_last)
          {
            builder.SetInsertPoint(next_bb);
          }
        }

        builder.SetInsertPoint(merge_bb);
        emitter.RestoreFlowState(branch_state);
        llvm::Type *result_ty = ExpectedLLVMType(if_case.result);
        if (!result_ty)
        {
          if (!incoming.empty() && incoming.front().value)
          {
            result_ty = incoming.front().value->getType();
          }
          else
          {
            result_ty = llvm::Type::getInt64Ty(emitter.GetContext());
          }
        }
        if (!result_ty || result_ty->isVoidTy())
        {
          return;
        }

        if (fallback_pred)
        {
          incoming.push_back({fallback_pred, llvm::Constant::getNullValue(result_ty), nullptr});
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
                  builder.CreatePHI(expected_ptr_ty, incoming.size(), "ifcase.result.addr");
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
            llvm::Value *slot =
                emitter.AcquireReusableAggregateStorage(func, result_ty, "ifcase.result");
            llvm::Type *expected_ptr_ty = llvm::PointerType::get(result_ty, 0);
            if (slot && slot->getType() != expected_ptr_ty)
            {
              slot = builder.CreateBitCast(slot, expected_ptr_ty);
            }
            for (const auto &entry : incoming)
            {
              llvm::IRBuilder<> pred_builder(entry.pred->getTerminator());
              llvm::Value *candidate =
                  entry.value ? entry.value : llvm::Constant::getNullValue(result_ty);
              llvm::Value *coerced = CoerceTo(&pred_builder, candidate, result_ty);
              if (!coerced)
              {
                coerced = llvm::Constant::getNullValue(result_ty);
              }
              pred_builder.CreateStore(coerced, slot);
            }
            merged_storage = slot;
          }

          if (merged_storage)
          {
            emitter.ForgetTempStorage(if_case.result);
            emitter.SetTempStorage(if_case.result, merged_storage);
            return;
          }
        }

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

        if (incoming.empty())
        {
          emitter.SetTempValue(if_case.result, llvm::Constant::getNullValue(result_ty));
          return;
        }
        if (incoming.size() == 1)
        {
          emitter.SetTempValue(
              if_case.result,
              coerce_in_predecessor(incoming.front().pred, incoming.front().value));
          return;
        }

        llvm::PHINode *phi = builder.CreatePHI(result_ty, incoming.size(), "ifcase.result");
        for (const auto &entry : incoming)
        {
          phi->addIncoming(coerce_in_predecessor(entry.pred, entry.value), entry.pred);
        }
        emitter.SetTempValue(if_case.result, phi);
      }
      void operator()(const IRRegion &region) const
      {
        llvm::Value *opts_value = EvaluateOrDefault(region.owner);
        if (!opts_value)
        {
          emitter.EmitIR(region.body);
          SetForwardedOrMaterializedResult(region.value);
          return;
        }

        analysis::TypePath region_path;
        region_path.push_back("Region");
        analysis::TypeRef region_active_type =
            analysis::MakeTypeModalState(std::move(region_path), "Active");
        llvm::Type *region_llvm_ty = emitter.GetLLVMType(region_active_type);
        if (!region_llvm_ty || region_llvm_ty->isVoidTy())
        {
          emitter.EmitIR(region.body);
          SetForwardedOrMaterializedResult(region.value);
          return;
        }

        llvm::Function *current_fn =
            builder.GetInsertBlock() ? builder.GetInsertBlock()->getParent() : nullptr;
        if (!current_fn)
        {
          emitter.EmitIR(region.body);
          SetForwardedOrMaterializedResult(region.value);
          return;
        }

        llvm::IRBuilder<> entry_builder(
            &current_fn->getEntryBlock(),
            current_fn->getEntryBlock().begin());
        llvm::Value *region_value = nullptr;
        const auto new_scoped_sym = analysis::LookupBuiltinModalRuntimeSymbol(
            {"Region"}, std::nullopt, "new_scoped");
        if (new_scoped_sym.has_value())
        {
          if (std::optional<RuntimeFuncInfo> new_scoped_info =
                  GetRuntimeFuncInfo(*new_scoped_sym))
          {
            llvm::Function *new_scoped_fn =
                emitter.GetModule().getFunction(*new_scoped_sym);
            const bool use_c_abi_aggregate_sret = true;
            if (!new_scoped_fn)
            {
              ABICallResult new_scoped_abi = emitter.ComputeCallABI(
                  new_scoped_info->params,
                  new_scoped_info->ret,
                  use_c_abi_aggregate_sret);
              if (new_scoped_abi.func_type)
              {
                new_scoped_fn = llvm::Function::Create(
                    new_scoped_abi.func_type,
                    llvm::GlobalValue::ExternalLinkage,
                    *new_scoped_sym,
                    &emitter.GetModule());
                new_scoped_fn->setCallingConv(llvm::CallingConv::C);
              }
            }
            if (new_scoped_fn)
            {
              std::vector<llvm::Value *> new_scoped_args;
              new_scoped_args.reserve(1);
              new_scoped_args.push_back(opts_value);
              region_value = EmitABICall(
                  emitter,
                  &builder,
                  new_scoped_fn,
                  new_scoped_info->params,
                  new_scoped_info->ret,
                  new_scoped_args,
                  use_c_abi_aggregate_sret);
            }
          }
        }
        if (!region_value)
        {
          emitter.EmitIR(region.body);
          SetForwardedOrMaterializedResult(region.value);
          return;
        }

        std::optional<std::string> alias = region.alias;
        if (!alias.has_value() || alias->empty())
        {
          alias = "__cursive_region_active";
        }

        llvm::Value *previous_local = emitter.GetLocal(*alias);
        analysis::TypeRef previous_type = emitter.LookupLocalType(*alias);
        const bool had_previous_local = previous_local != nullptr;

        llvm::AllocaInst *region_slot = entry_builder.CreateAlloca(
            region_llvm_ty,
            nullptr,
            *alias);
        builder.CreateStore(region_value, region_slot);
        emitter.SetLocal(*alias, region_slot);
        emitter.SetLocalType(*alias, region_active_type);

        IRValue active_region;
        active_region.kind = IRValue::Kind::Local;
        active_region.name = *alias;

        emitter.PushActiveRegion(active_region);
        emitter.EmitIR(region.body);
        emitter.PopActiveRegion();

        if (had_previous_local)
        {
          emitter.SetLocal(*alias, previous_local);
          if (previous_type)
          {
            emitter.SetLocalType(*alias, previous_type);
          }
          else
          {
            emitter.RemoveLocal(*alias);
            emitter.SetLocal(*alias, previous_local);
          }
        }
        else
        {
          emitter.RemoveLocal(*alias);
        }

        SetForwardedOrMaterializedResult(region.value);
      }
      void operator()(const IRFrame &frame) const
      {
        emitter.EmitIR(frame.body);
        SetForwardedOrMaterializedResult(frame.value);
      }
      void operator()(const IRBranch &) const {}
      void operator()(const IRPhi &) const {}
      void operator()(const IRInitPanicHandle &handle) const
      {
        llvm::Value *panic_ptr = LoadPanicOutPtr(emitter, &builder);
        if (!panic_ptr)
        {
          return;
        }
        llvm::Type *i8_ty = llvm::Type::getInt8Ty(emitter.GetContext());
        llvm::Type *i8_ptr_ty = llvm::PointerType::get(emitter.GetContext(), 0);
        llvm::Value *flag_ptr = CoerceTo(&builder, panic_ptr, i8_ptr_ty);
        if (!flag_ptr)
        {
          return;
        }
        llvm::Value *flag = builder.CreateLoad(i8_ty, flag_ptr);
        llvm::Value *has_panic =
            builder.CreateICmpNE(flag, llvm::ConstantInt::get(i8_ty, 0));

        llvm::Function *func = builder.GetInsertBlock()->getParent();
        llvm::BasicBlock *panic_bb =
            llvm::BasicBlock::Create(emitter.GetContext(), "init.panic.take", func);
        llvm::BasicBlock *cont_bb =
            llvm::BasicBlock::Create(emitter.GetContext(), "init.panic.cont", func);
        builder.CreateCondBr(has_panic, panic_bb, cont_bb);

        builder.SetInsertPoint(panic_bb);
        StoreInitPanicRecord(emitter, &builder);
        if (handle.cleanup_ir)
        {
          emitter.EmitIR(handle.cleanup_ir);
        }
        if (!builder.GetInsertBlock()->getTerminator())
        {
          EmitReturn(emitter, &builder);
        }

        builder.SetInsertPoint(cont_bb);
      }
      void operator()(const IRHandleDeinitPanic &) const
      {
        HandleDeinitPanic(emitter, &builder);
      }
      void operator()(const IRRestoreDeinitPanic &) const
      {
        RestoreDeinitPanicIfAny(emitter, &builder);
      }
      void operator()(const IRCheckPoison &check) const
      {
        emitter.EmitPoisonCheck(check.module);
      }
      void operator()(const IRParallel &parallel) const
      {
        llvm::Type *ptr_ty = emitter.GetOpaquePtr();
        llvm::Type *usize_ty = llvm::Type::getInt64Ty(emitter.GetContext());
        llvm::Type *i8_ptr_ty =
            llvm::PointerType::get(llvm::Type::getInt8Ty(emitter.GetContext()), 0);
        llvm::Value *domain = EvaluateOrDefault(parallel.domain);
        if (!domain)
        {
          domain = llvm::Constant::getNullValue(GetDynamicType(emitter.GetContext()));
        }
        llvm::Value *cancel_token = parallel.cancel_token.has_value()
                                        ? EvaluateOrDefault(*parallel.cancel_token)
                                        : llvm::Constant::getAllOnesValue(usize_ty);
        cancel_token = CoerceTo(&builder, cancel_token, usize_ty);
        if (!cancel_token)
        {
          cancel_token = llvm::Constant::getAllOnesValue(usize_ty);
        }
        llvm::Value *name_ptr = llvm::ConstantPointerNull::get(
            llvm::cast<llvm::PointerType>(i8_ptr_ty));
        if (!parallel.name.empty())
        {
          name_ptr = builder.CreateGlobalStringPtr(parallel.name);
        }

        llvm::Value *ctx_ptr = nullptr;
        const std::string begin_sym = ConcurrencySymParallelBegin();
        if (std::optional<RuntimeFuncInfo> begin_info = GetRuntimeFuncInfo(begin_sym))
        {
          llvm::Function *begin_fn = emitter.GetModule().getFunction(begin_sym);
          const bool use_c_abi_aggregate_sret = true;
          if (!begin_fn)
          {
            ABICallResult begin_abi = emitter.ComputeCallABI(
                begin_info->params,
                begin_info->ret,
                use_c_abi_aggregate_sret);
            if (begin_abi.func_type)
            {
              begin_fn = llvm::Function::Create(
                  begin_abi.func_type,
                  llvm::GlobalValue::ExternalLinkage,
                  begin_sym,
                  &emitter.GetModule());
              begin_fn->setCallingConv(llvm::CallingConv::C);
            }
          }
          if (begin_fn)
          {
            std::vector<llvm::Value *> begin_args;
            begin_args.reserve(3);
            begin_args.push_back(domain);
            begin_args.push_back(cancel_token);
            begin_args.push_back(name_ptr);
            ctx_ptr = EmitABICall(
                emitter,
                &builder,
                begin_fn,
                begin_info->params,
                begin_info->ret,
                begin_args,
                use_c_abi_aggregate_sret);
          }
        }
        if (!ctx_ptr)
        {
          ctx_ptr = llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(ptr_ty));
        }

        emitter.SetTempValue(parallel.result, ctx_ptr);
        emitter.PushParallelContext(parallel.result);
        emitter.EmitIR(parallel.body);
        emitter.PopParallelContext();
      }
      void operator()(const IRSpawn &spawn) const
      {
        emitter.EmitIR(spawn.captured_env);

        llvm::Type *ptr_ty = emitter.GetOpaquePtr();
        llvm::Type *usize_ty = llvm::Type::getInt64Ty(emitter.GetContext());
        llvm::Type *i32_ty = llvm::Type::getInt32Ty(emitter.GetContext());
        llvm::FunctionType *body_fn_ty = llvm::FunctionType::get(
            llvm::Type::getVoidTy(emitter.GetContext()),
            {ptr_ty, ptr_ty, ptr_ty, ptr_ty},
            false);
        llvm::Type *body_fn_ptr_ty = llvm::PointerType::get(body_fn_ty, 0);

        llvm::Value *env_ptr = CoerceTo(&builder, EvaluateOrDefault(spawn.env_ptr), ptr_ty);
        if (!env_ptr)
        {
          env_ptr = llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(ptr_ty));
        }
        llvm::Value *env_size = CoerceTo(&builder, EvaluateOrDefault(spawn.env_size), usize_ty);
        if (!env_size)
        {
          env_size = llvm::ConstantInt::get(usize_ty, 0);
        }
        llvm::Value *body_fn = EvaluateOrDefault(spawn.body_fn);
        body_fn = CoerceTo(&builder, body_fn, body_fn_ptr_ty);
        if (!body_fn)
        {
          body_fn = llvm::ConstantPointerNull::get(
              llvm::cast<llvm::PointerType>(body_fn_ptr_ty));
        }
        llvm::Value *result_size =
            CoerceTo(&builder, EvaluateOrDefault(spawn.result_size), usize_ty);
        if (!result_size)
        {
          result_size = llvm::ConstantInt::get(usize_ty, 0);
        }
        llvm::Value *hosted_env = emitter.GetHostedCurrentEnvPtr();
        hosted_env = CoerceTo(&builder, hosted_env, ptr_ty);
        if (!hosted_env)
        {
          hosted_env = llvm::ConstantPointerNull::get(
              llvm::cast<llvm::PointerType>(ptr_ty));
        }
        llvm::Value *affinity_mask = llvm::ConstantInt::get(usize_ty, 0);
        if (spawn.affinity_mask.has_value())
        {
          affinity_mask =
              CoerceTo(&builder, EvaluateOrDefault(*spawn.affinity_mask), usize_ty);
          if (!affinity_mask)
          {
            affinity_mask = llvm::ConstantInt::get(usize_ty, 0);
          }
        }
        llvm::Value *priority_hint = llvm::ConstantInt::get(i32_ty, 1);
        if (spawn.priority.has_value())
        {
          priority_hint =
              CoerceTo(&builder, EvaluateOrDefault(*spawn.priority), i32_ty);
          if (!priority_hint)
          {
            priority_hint = llvm::ConstantInt::get(i32_ty, 1);
          }
        }

        llvm::Value *handle = nullptr;
        const std::string spawn_sym = ConcurrencySymSpawnCreate();
        if (std::optional<RuntimeFuncInfo> spawn_info = GetRuntimeFuncInfo(spawn_sym))
        {
          llvm::Function *spawn_fn = emitter.GetModule().getFunction(spawn_sym);
          const bool use_c_abi_aggregate_sret = true;
          if (!spawn_fn)
          {
            ABICallResult spawn_abi = emitter.ComputeCallABI(
                spawn_info->params,
                spawn_info->ret,
                use_c_abi_aggregate_sret);
            if (spawn_abi.func_type)
            {
              spawn_fn = llvm::Function::Create(
                  spawn_abi.func_type,
                  llvm::GlobalValue::ExternalLinkage,
                  spawn_sym,
                  &emitter.GetModule());
              spawn_fn->setCallingConv(llvm::CallingConv::C);
            }
          }
          if (spawn_fn)
          {
            std::vector<llvm::Value *> spawn_args;
            spawn_args.reserve(7);
            spawn_args.push_back(env_ptr);
            spawn_args.push_back(env_size);
            spawn_args.push_back(body_fn);
            spawn_args.push_back(hosted_env);
            spawn_args.push_back(result_size);
            spawn_args.push_back(affinity_mask);
            spawn_args.push_back(priority_hint);
            handle = EmitABICall(
                emitter,
                &builder,
                spawn_fn,
                spawn_info->params,
                spawn_info->ret,
                spawn_args,
                use_c_abi_aggregate_sret);
          }
        }
        if (!handle)
        {
          handle = llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(ptr_ty));
        }

        if (llvm::Type *expected = ExpectedLLVMType(spawn.result))
        {
          if (llvm::Value *coerced = CoerceTo(&builder, handle, expected))
          {
            handle = coerced;
          }
        }
        emitter.SetTempValue(spawn.result, handle);
      }
      void operator()(const IRWait &wait) const
      {
        llvm::Type *ptr_ty = emitter.GetOpaquePtr();
        llvm::Value *handle = CoerceTo(&builder, EvaluateOrDefault(wait.handle), ptr_ty);
        if (!handle)
        {
          handle = llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(ptr_ty));
        }
        llvm::Value *result_ptr = nullptr;
        const std::string wait_sym = ConcurrencySymSpawnWait();
        if (std::optional<RuntimeFuncInfo> wait_info = GetRuntimeFuncInfo(wait_sym))
        {
          llvm::Function *wait_fn = emitter.GetModule().getFunction(wait_sym);
          const bool use_c_abi_aggregate_sret = true;
          if (!wait_fn)
          {
            ABICallResult wait_abi = emitter.ComputeCallABI(
                wait_info->params,
                wait_info->ret,
                use_c_abi_aggregate_sret);
            if (wait_abi.func_type)
            {
              wait_fn = llvm::Function::Create(
                  wait_abi.func_type,
                  llvm::GlobalValue::ExternalLinkage,
                  wait_sym,
                  &emitter.GetModule());
              wait_fn->setCallingConv(llvm::CallingConv::C);
            }
          }
          if (wait_fn)
          {
            std::vector<llvm::Value *> wait_args;
            wait_args.reserve(1);
            wait_args.push_back(handle);
            result_ptr = EmitABICall(
                emitter,
                &builder,
                wait_fn,
                wait_info->params,
                wait_info->ret,
                wait_args,
                use_c_abi_aggregate_sret);
          }
        }
        if (!result_ptr)
        {
          result_ptr = llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(ptr_ty));
        }

        llvm::Value *out = nullptr;
        llvm::Type *expected = ExpectedLLVMType(wait.result);
        if (core::IsDebugEnabled("wait"))
        {
          const LowerCtx *ctx = emitter.GetCurrentCtx();
          const std::string wait_type_text =
              (ctx && ctx->LookupValueType(wait.result))
                  ? analysis::TypeToString(ctx->LookupValueType(wait.result))
                  : std::string("<null>");
          const char *expected_kind =
              !expected ? "null" : expected->isIntegerTy() ? "int"
                               : expected->isPointerTy()   ? "ptr"
                               : expected->isStructTy()    ? "struct"
                               : expected->isArrayTy()     ? "array"
                               : expected->isVoidTy()      ? "void"
                                                           : "other";
          std::fprintf(stderr,
                       "[cursive] irwait: result=%s expected=%s type=%s llvm=%s\n",
                       wait.result.name.c_str(),
                       expected ? "set" : "null",
                       wait_type_text.c_str(),
                       expected_kind);
        }
        if (expected)
        {
          if (expected->isPointerTy())
          {
            out = CoerceTo(&builder, result_ptr, expected);
          }
          else if (auto *struct_ty = llvm::dyn_cast<llvm::StructType>(expected);
                   struct_ty && struct_ty->getNumElements() == 0)
          {
            out = llvm::Constant::getNullValue(expected);
          }
          else if (expected->isArrayTy())
          {
            llvm::Value *typed_ptr = builder.CreateBitCast(
                result_ptr, llvm::PointerType::get(expected, 0));
            out = builder.CreateLoad(expected, typed_ptr);
          }
          else if (!expected->isVoidTy())
          {
            llvm::Value *typed_ptr = builder.CreateBitCast(
                result_ptr, llvm::PointerType::get(expected, 0));
            out = builder.CreateLoad(expected, typed_ptr);
          }
        }
        if (!out)
        {
          out = DefaultFor(wait.result);
        }
        emitter.SetTempValue(wait.result, out);
      }
      void operator()(const IRCancelCheck &check) const
      {
        llvm::Type *ptr_ty = emitter.GetOpaquePtr();
        llvm::Type *i1_ty = llvm::Type::getInt1Ty(emitter.GetContext());
        llvm::Value *token = CoerceTo(&builder, EvaluateOrDefault(check.token), ptr_ty);
        if (!token)
        {
          token = llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(ptr_ty));
        }

        llvm::Value *out = nullptr;
        const std::string check_sym = BuiltinSymCancelTokenActiveIsCancelled();
        if (std::optional<RuntimeFuncInfo> check_info = GetRuntimeFuncInfo(check_sym))
        {
          llvm::Function *check_fn = emitter.GetModule().getFunction(check_sym);
          const bool use_c_abi_aggregate_sret = true;
          if (!check_fn)
          {
            ABICallResult check_abi = emitter.ComputeCallABI(
                check_info->params,
                check_info->ret,
                use_c_abi_aggregate_sret);
            if (check_abi.func_type)
            {
              check_fn = llvm::Function::Create(
                  check_abi.func_type,
                  llvm::GlobalValue::ExternalLinkage,
                  check_sym,
                  &emitter.GetModule());
              check_fn->setCallingConv(llvm::CallingConv::C);
            }
          }
          if (check_fn)
          {
            std::vector<llvm::Value *> check_args;
            check_args.push_back(token);
            llvm::Value *raw = EmitABICall(
                emitter,
                &builder,
                check_fn,
                check_info->params,
                check_info->ret,
                check_args,
                use_c_abi_aggregate_sret);
            out = CoerceTo(&builder, raw, i1_ty);
          }
        }
        if (!out)
        {
          out = llvm::ConstantInt::getFalse(i1_ty);
        }
        emitter.SetTempValue(check.result, out);
      }
      void operator()(const IRCancelSuppress &) const
      {
        // Runtime scheduling already suppresses dequeued-but-unstarted
        // cancelled work before wrapper body execution. This IR node exists
        // to preserve the explicit lowering surface required by the spec.
      }
      void operator()(const IRDispatch &dispatch) const
      {
        emitter.EmitIR(dispatch.captured_env);
        emitter.EmitIR(dispatch.body);

        const LowerCtx *active_ctx = emitter.GetCurrentCtx();
        llvm::Type *ptr_ty = emitter.GetOpaquePtr();
        llvm::Type *i32_ty = llvm::Type::getInt32Ty(emitter.GetContext());
        llvm::Type *usize_ty = llvm::Type::getInt64Ty(emitter.GetContext());
        llvm::Type *range_ty = GetRangeType(emitter.GetContext());
        llvm::Type *string_view_ty = GetStringViewType(emitter.GetContext());

        auto as_usize = [&](llvm::Value *value, llvm::Value *fallback) -> llvm::Value *
        {
          if (!value)
          {
            return fallback;
          }
          if (!value->getType()->isIntegerTy())
          {
            value = CoerceTo(&builder, value, usize_ty);
          }
          else if (value->getType()->getIntegerBitWidth() != 64)
          {
            value = builder.CreateIntCast(value, usize_ty, false);
          }
          return value ? value : fallback;
        };

        auto evaluate_range_bound = [&](const std::optional<IRValue> &bound_opt,
                                        llvm::Type *target_ty) -> llvm::Value *
        {
          if (!target_ty)
          {
            return nullptr;
          }
          llvm::Value *fallback = llvm::ConstantInt::get(target_ty, 0);
          if (!bound_opt.has_value())
          {
            return fallback;
          }

          llvm::Value *value = emitter.EvaluateIRValue(*bound_opt);
          if (!value)
          {
            return fallback;
          }

          if (value->getType()->isPointerTy() && target_ty->isIntegerTy())
          {
            llvm::Type *load_ty = target_ty;
            if (active_ctx)
            {
              if (analysis::TypeRef bound_type = active_ctx->LookupValueType(*bound_opt))
              {
                if (llvm::Type *bound_ll = emitter.GetLLVMType(bound_type))
                {
                  if (bound_ll->isIntegerTy())
                  {
                    load_ty = bound_ll;
                  }
                }
              }
            }
            llvm::Value *typed_ptr = value;
            llvm::Type *ptr_to_load_ty = llvm::PointerType::get(load_ty, 0);
            if (typed_ptr->getType() != ptr_to_load_ty)
            {
              typed_ptr = builder.CreateBitCast(typed_ptr, ptr_to_load_ty);
            }
            value = builder.CreateLoad(load_ty, typed_ptr);
          }

          value = as_usize(value, fallback);
          if (value && value->getType() != target_ty)
          {
            value = CoerceTo(&builder, value, target_ty);
          }
          if (!value)
          {
            return fallback;
          }
          return value;
        };

        auto materialize_range = [&]() -> llvm::Value *
        {
          if (active_ctx)
          {
            if (const DerivedValueInfo *derived = active_ctx->LookupDerivedValue(dispatch.range))
            {
              if (derived->kind == DerivedValueInfo::Kind::RangeLit)
              {
                auto *range_struct_ty = llvm::dyn_cast<llvm::StructType>(range_ty);
                if (range_struct_ty && range_struct_ty->getNumElements() >= 3)
                {
                  llvm::Value *out = llvm::Constant::getNullValue(range_struct_ty);
                  llvm::Type *kind_ty = range_struct_ty->getElementType(0);
                  llvm::Type *lo_ty = range_struct_ty->getElementType(1);
                  llvm::Type *hi_ty = range_struct_ty->getElementType(2);

                  llvm::Value *kind = llvm::ConstantInt::get(
                      kind_ty,
                      static_cast<std::uint64_t>(derived->range.kind));

                  llvm::Value *lo = llvm::ConstantInt::get(lo_ty, 0);
                  if (derived->range.lo.has_value())
                  {
                    lo = evaluate_range_bound(derived->range.lo, lo_ty);
                  }

                  llvm::Value *hi = llvm::ConstantInt::get(hi_ty, 0);
                  if (derived->range.hi.has_value())
                  {
                    hi = evaluate_range_bound(derived->range.hi, hi_ty);
                  }

                  out = builder.CreateInsertValue(out, kind, {0u});
                  out = builder.CreateInsertValue(out, lo, {1u});
                  out = builder.CreateInsertValue(out, hi, {2u});
                  return out;
                }
              }
            }
          }

          llvm::Value *range = EvaluateOrDefault(dispatch.range);
          if (range && range->getType() != range_ty)
          {
            range = CoerceTo(&builder, range, range_ty);
          }
          if (!range)
          {
            range = llvm::Constant::getNullValue(range_ty);
          }
          return range;
        };

        llvm::Value *range = materialize_range();
        llvm::Value *elem_size = as_usize(
            EvaluateOrDefault(dispatch.elem_size),
            llvm::ConstantInt::get(usize_ty, 0));
        llvm::Value *result_size = as_usize(
            EvaluateOrDefault(dispatch.result_size),
            llvm::ConstantInt::get(usize_ty, 0));

        llvm::Value *body_fn = CoerceTo(&builder, EvaluateOrDefault(dispatch.body_fn), ptr_ty);
        if (!body_fn)
        {
          body_fn = llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(ptr_ty));
        }

        llvm::Value *env_ptr = CoerceTo(&builder, EvaluateOrDefault(dispatch.env_ptr), ptr_ty);
        if (!env_ptr)
        {
          env_ptr = llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(ptr_ty));
        }
        llvm::Value *hosted_env = emitter.GetHostedCurrentEnvPtr();
        hosted_env = CoerceTo(&builder, hosted_env, ptr_ty);
        if (!hosted_env)
        {
          hosted_env = llvm::ConstantPointerNull::get(
              llvm::cast<llvm::PointerType>(ptr_ty));
        }

        llvm::Value *result_ptr = CoerceTo(&builder, EvaluateOrDefault(dispatch.result_ptr), ptr_ty);
        if (!result_ptr)
        {
          result_ptr = llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(ptr_ty));
        }

        llvm::Value *reduce_fn = llvm::ConstantPointerNull::get(
            llvm::cast<llvm::PointerType>(ptr_ty));
        if (dispatch.reduce_fn.has_value())
        {
          reduce_fn = CoerceTo(&builder, EvaluateOrDefault(*dispatch.reduce_fn), ptr_ty);
          if (!reduce_fn)
          {
            reduce_fn = llvm::ConstantPointerNull::get(
                llvm::cast<llvm::PointerType>(ptr_ty));
          }
        }

        llvm::Value *reduce_op = llvm::Constant::getNullValue(string_view_ty);
        if (dispatch.reduce_op.has_value())
        {
          auto *view_ty = llvm::dyn_cast<llvm::StructType>(string_view_ty);
          if (view_ty && view_ty->getNumElements() >= 2)
          {
            llvm::Type *ptr_field_ty = view_ty->getElementType(0);
            llvm::Type *len_field_ty = view_ty->getElementType(1);
            llvm::Value *op_ptr = builder.CreateGlobalStringPtr(*dispatch.reduce_op);
            op_ptr = CoerceTo(&builder, op_ptr, ptr_field_ty);
            if (!op_ptr)
            {
              op_ptr = llvm::ConstantPointerNull::get(
                  llvm::cast<llvm::PointerType>(ptr_field_ty));
            }
            llvm::Value *op_len = llvm::ConstantInt::get(
                len_field_ty,
                static_cast<std::uint64_t>(dispatch.reduce_op->size()));
            llvm::Value *out = llvm::Constant::getNullValue(view_ty);
            out = builder.CreateInsertValue(out, op_ptr, {0u});
            out = builder.CreateInsertValue(out, op_len, {1u});
            reduce_op = out;
          }
        }

        llvm::Value *ordered = llvm::ConstantInt::get(i32_ty, dispatch.ordered ? 1 : 0);
        llvm::Value *chunk_size = llvm::ConstantInt::get(usize_ty, 0);
        if (dispatch.chunk_size.has_value())
        {
          chunk_size = as_usize(
              EvaluateOrDefault(*dispatch.chunk_size),
              llvm::ConstantInt::get(usize_ty, 0));
        }

        const std::string dispatch_sym = ConcurrencySymDispatchRun();
        if (std::optional<RuntimeFuncInfo> dispatch_info =
                GetRuntimeFuncInfo(dispatch_sym))
        {
          llvm::Function *dispatch_fn = emitter.GetModule().getFunction(dispatch_sym);
          const bool use_c_abi_aggregate_sret = true;
          if (!dispatch_fn)
          {
            ABICallResult dispatch_abi = emitter.ComputeCallABI(
                dispatch_info->params,
                dispatch_info->ret,
                use_c_abi_aggregate_sret);
            if (dispatch_abi.func_type)
            {
              dispatch_fn = llvm::Function::Create(
                  dispatch_abi.func_type,
                  llvm::GlobalValue::ExternalLinkage,
                  dispatch_sym,
                  &emitter.GetModule());
              dispatch_fn->setCallingConv(llvm::CallingConv::C);
            }
          }
          if (dispatch_fn)
          {
            std::vector<llvm::Value *> dispatch_args;
            dispatch_args.reserve(11);
            dispatch_args.push_back(range);
            dispatch_args.push_back(elem_size);
            dispatch_args.push_back(result_size);
            dispatch_args.push_back(body_fn);
            dispatch_args.push_back(hosted_env);
            dispatch_args.push_back(env_ptr);
            dispatch_args.push_back(reduce_op);
            dispatch_args.push_back(result_ptr);
            dispatch_args.push_back(reduce_fn);
            dispatch_args.push_back(ordered);
            dispatch_args.push_back(chunk_size);
            (void)EmitABICall(
                emitter,
                &builder,
                dispatch_fn,
                dispatch_info->params,
                dispatch_info->ret,
                dispatch_args,
                use_c_abi_aggregate_sret);
          }
        }

        const bool has_reduce = dispatch.reduce_op.has_value() || dispatch.reduce_fn.has_value();
        llvm::Value *out = nullptr;
        llvm::Type *expected = ExpectedLLVMType(dispatch.result);
        if (expected)
        {
          if (expected->isStructTy() &&
              llvm::cast<llvm::StructType>(expected)->getNumElements() == 0)
          {
            out = llvm::Constant::getNullValue(expected);
          }
          else if (has_reduce && !expected->isVoidTy())
          {
            llvm::Value *typed_result_ptr = builder.CreateBitCast(
                result_ptr, llvm::PointerType::get(expected, 0));
            out = builder.CreateLoad(expected, typed_result_ptr);
          }
        }
        if (!out)
        {
          out = DefaultFor(dispatch.result);
        }
        emitter.SetTempValue(dispatch.result, out);
      }
      void operator()(const IRYield &y) const
      {
        AsyncEmitState *async_state = emitter.GetAsyncState();
        if (!async_state || !async_state->info)
        {
          emitter.SetTempValue(y.result, EvaluateOrDefault(y.result));
          return;
        }

        const LowerCtx::AsyncProcInfo &info = *async_state->info;
        llvm::Function *func =
            builder.GetInsertBlock() ? builder.GetInsertBlock()->getParent() : nullptr;
        if (!func)
        {
          emitter.SetTempValue(y.result, DefaultFor(y.result));
          return;
        }

        const LowerCtx *active_ctx = emitter.GetCurrentCtx();
        const analysis::ScopeContext &scope = BuildScope(active_ctx);
        llvm::BasicBlock *cont_bb =
            llvm::BasicBlock::Create(emitter.GetContext(), "yield.cont", func);

        auto load_resume_input = [&](llvm::IRBuilder<> &b) -> llvm::Value *
        {
          analysis::TypeRef input_type = info.in_type;
          analysis::TypeRef target_type =
              active_ctx ? active_ctx->LookupValueType(y.result) : nullptr;
          llvm::Type *expected = ExpectedLLVMType(y.result);

          if (!input_type || IsUnitTypeRef(input_type) || IsNeverTypeRef(input_type))
          {
            if (expected && !expected->isVoidTy())
            {
              return llvm::Constant::getNullValue(expected);
            }
            return DefaultFor(y.result);
          }

          llvm::Type *input_ll = emitter.GetLLVMType(input_type);
          if (!input_ll || input_ll->isVoidTy())
          {
            if (expected && !expected->isVoidTy())
            {
              return llvm::Constant::getNullValue(expected);
            }
            return DefaultFor(y.result);
          }

          llvm::Value *input_ptr = async_state->input_ptr;
          if (!input_ptr)
          {
            llvm::Value *fallback = llvm::Constant::getNullValue(input_ll);
            if (expected)
            {
              if (target_type)
              {
                if (llvm::Value *coerced = CoerceToTyped(
                        emitter,
                        &b,
                        fallback,
                        expected,
                        input_type,
                        target_type))
                {
                  return coerced;
                }
              }
              if (llvm::Value *coerced = CoerceTo(&b, fallback, expected))
              {
                return coerced;
              }
              return llvm::Constant::getNullValue(expected);
            }
            return fallback;
          }

          llvm::Type *i8_ty = llvm::Type::getInt8Ty(emitter.GetContext());
          llvm::Value *input_i8 = input_ptr;
          if (input_i8->getType() != llvm::PointerType::get(i8_ty, 0))
          {
            input_i8 = b.CreateBitCast(input_i8, llvm::PointerType::get(i8_ty, 0));
          }
          llvm::Value *typed_input_ptr =
              b.CreateBitCast(input_i8, llvm::PointerType::get(input_ll, 0));
          llvm::LoadInst *loaded = b.CreateLoad(input_ll, typed_input_ptr);
          loaded->setAlignment(llvm::Align(1));
          llvm::Value *input_value = loaded;

          if (expected)
          {
            if (target_type)
            {
              if (llvm::Value *coerced = CoerceToTyped(
                      emitter,
                      &b,
                      input_value,
                      expected,
                      input_type,
                      target_type))
              {
                input_value = coerced;
              }
              else if (llvm::Value *coerced_plain =
                           CoerceTo(&b, input_value, expected))
              {
                input_value = coerced_plain;
              }
              else
              {
                input_value = llvm::Constant::getNullValue(expected);
              }
            }
            else if (llvm::Value *coerced = CoerceTo(&b, input_value, expected))
            {
              input_value = coerced;
            }
            else
            {
              input_value = llvm::Constant::getNullValue(expected);
            }
          }

          return input_value;
        };

        if (info.is_resume && async_state->resume_switch)
        {
          if (!async_state->resume_blocks.contains(y.state_index))
          {
            llvm::BasicBlock *resume_bb = llvm::BasicBlock::Create(
                emitter.GetContext(),
                "yield.resume." + std::to_string(y.state_index),
                func);
            async_state->resume_blocks[y.state_index] = resume_bb;
            if (auto *disc_ty = llvm::dyn_cast<llvm::IntegerType>(
                    async_state->resume_switch->getCondition()->getType()))
            {
              async_state->resume_switch->addCase(
                  llvm::ConstantInt::get(disc_ty, y.state_index),
                  resume_bb);
            }

            llvm::IRBuilder<> resume_builder(resume_bb);
            resume_builder.CreateBr(cont_bb);
          }
        }

        auto ensure_async_frame = [&]() -> llvm::Value *
        {
          if (async_state->frame_ptr)
          {
            return async_state->frame_ptr;
          }
          if (!info.is_wrapper)
          {
            return nullptr;
          }

          const std::string alloc_sym = BuiltinSymAsyncAllocFrame();
          llvm::Function *alloc_fn = emitter.GetModule().getFunction(alloc_sym);
          if (!alloc_fn)
          {
            llvm::FunctionType *alloc_ty = llvm::FunctionType::get(
                emitter.GetOpaquePtr(),
                {llvm::Type::getInt64Ty(emitter.GetContext()),
                 llvm::Type::getInt64Ty(emitter.GetContext())},
                false);
            alloc_fn = llvm::Function::Create(
                alloc_ty,
                llvm::GlobalValue::ExternalLinkage,
                alloc_sym,
                &emitter.GetModule());
          }
          if (!alloc_fn)
          {
            return nullptr;
          }

          llvm::Value *frame_raw = builder.CreateCall(
              alloc_fn,
              {llvm::ConstantInt::get(llvm::Type::getInt64Ty(emitter.GetContext()),
                                      info.frame_size),
               llvm::ConstantInt::get(llvm::Type::getInt64Ty(emitter.GetContext()),
                                      std::max<std::uint64_t>(1, info.frame_align))});
          async_state->frame_ptr = CoerceTo(&builder, frame_raw, emitter.GetOpaquePtr());
          if (!async_state->frame_ptr)
          {
            async_state->frame_ptr = frame_raw;
          }
          if (!async_state->frame_ptr)
          {
            return nullptr;
          }

          llvm::Type *i64_ty = llvm::Type::getInt64Ty(emitter.GetContext());
          llvm::Value *resume_state_ptr = AsyncFrameTypedPtr(
              emitter,
              &builder,
              async_state->frame_ptr,
              kAsyncFrameResumeStateOffset,
              i64_ty);
          if (resume_state_ptr)
          {
            builder.CreateStore(llvm::ConstantInt::get(i64_ty, 0), resume_state_ptr);
          }

          llvm::Value *resume_fn_ptr = llvm::ConstantPointerNull::get(
              llvm::cast<llvm::PointerType>(emitter.GetOpaquePtr()));
          if (llvm::Function *resume_fn = emitter.GetFunction(info.resume_symbol))
          {
            if (llvm::Value *coerced = CoerceTo(&builder, resume_fn, emitter.GetOpaquePtr()))
            {
              resume_fn_ptr = coerced;
            }
          }
          llvm::Value *resume_fn_slot = AsyncFrameTypedPtr(
              emitter,
              &builder,
              async_state->frame_ptr,
              kAsyncFrameResumeFnOffset,
              emitter.GetOpaquePtr());
          if (resume_fn_slot)
          {
            builder.CreateStore(resume_fn_ptr, resume_fn_slot);
          }

          StoreAsyncFrameHostedEnv(
              emitter,
              &builder,
              async_state->frame_ptr,
              emitter.GetHostedCurrentEnvPtr());
          StoreAsyncFrameKeySnapshot(
              emitter,
              &builder,
              async_state->frame_ptr,
              NullOpaquePtr(emitter));

          return async_state->frame_ptr;
        };

        auto snapshot_async_slots = [&]()
        {
          if (!async_state->frame_ptr)
          {
            return;
          }
          for (const auto &slot_name : info.slot_order)
          {
            const auto slot_it = info.slots.find(slot_name);
            if (slot_it == info.slots.end())
            {
              continue;
            }
            const auto &slot = slot_it->second;
            llvm::Type *slot_ty = emitter.GetLLVMType(slot.type);
            if (!slot_ty || slot_ty->isVoidTy())
            {
              continue;
            }

            llvm::Value *local_value = LoadLocalValue(emitter, &builder, slot_name);
            if (!local_value)
            {
              continue;
            }

            analysis::TypeRef source_type = emitter.LookupLocalType(slot_name);
            llvm::Value *stored_value = local_value;
            if (stored_value->getType() != slot_ty)
            {
              if (llvm::Value *coerced = CoerceToTyped(
                      emitter,
                      &builder,
                      stored_value,
                      slot_ty,
                      source_type,
                      slot.type))
              {
                stored_value = coerced;
              }
              else if (llvm::Value *coerced_plain =
                           CoerceTo(&builder, stored_value, slot_ty))
              {
                stored_value = coerced_plain;
              }
              else
              {
                stored_value = llvm::Constant::getNullValue(slot_ty);
              }
            }

            llvm::Value *frame_slot_ptr = AsyncFrameTypedPtr(
                emitter,
                &builder,
                async_state->frame_ptr,
                slot.offset,
                slot_ty);
            if (!frame_slot_ptr)
            {
              continue;
            }
            llvm::StoreInst *st = builder.CreateStore(stored_value, frame_slot_ptr);
            st->setAlignment(llvm::Align(std::max<std::uint64_t>(1, slot.align)));
          }
        };

        llvm::Value *yielded_value = EvaluateOrDefault(y.value);
        llvm::Value *frame_ptr = ensure_async_frame();
        if (y.release && frame_ptr)
        {
          llvm::Value *released = EmitKeyReleaseAll(emitter, &builder);
          StoreAsyncFrameKeySnapshot(emitter, &builder, frame_ptr, released);
        }
        if (frame_ptr)
        {
          snapshot_async_slots();
          llvm::Type *i64_ty = llvm::Type::getInt64Ty(emitter.GetContext());
          llvm::Value *state_ptr = AsyncFrameTypedPtr(
              emitter,
              &builder,
              frame_ptr,
              kAsyncFrameResumeStateOffset,
              i64_ty);
          if (state_ptr)
          {
            builder.CreateStore(
                llvm::ConstantInt::get(i64_ty, y.state_index),
                state_ptr);
          }
        }

        analysis::TypeRef async_type = info.async_type;
        llvm::Type *async_layout_ty = async_type ? emitter.GetLLVMType(async_type) : nullptr;
        auto *async_struct = llvm::dyn_cast_or_null<llvm::StructType>(async_layout_ty);
        llvm::Value *suspended_value = nullptr;
        if (async_struct && async_struct->getNumElements() >= 1 &&
            async_struct->getElementType(0)->isIntegerTy())
        {
          llvm::IRBuilder<> entry_builder(
              &func->getEntryBlock(),
              func->getEntryBlock().begin());
          llvm::AllocaInst *async_slot = entry_builder.CreateAlloca(async_struct);
          builder.CreateStore(llvm::Constant::getNullValue(async_struct), async_slot);

          llvm::Type *disc_ty = async_struct->getElementType(0);
          const AsyncStateDiscs async_discs =
              LoweredAsyncStateDiscs(scope, async_type);
          const std::uint64_t suspended_disc = async_discs.suspended;
          llvm::Value *disc_ptr = builder.CreateStructGEP(async_struct, async_slot, 0);
          builder.CreateStore(
              llvm::ConstantInt::get(disc_ty, suspended_disc),
              disc_ptr);

          llvm::Value *payload_i8 = CreateTaggedPayloadI8Ptr(
              emitter,
              &builder,
              async_struct,
              async_slot,
              ::cursive::analysis::layout::kPtrAlign);

          if (payload_i8 &&
              info.out_type &&
              !IsUnitTypeRef(info.out_type) &&
              !IsNeverTypeRef(info.out_type))
          {
            llvm::Type *out_ll = emitter.GetLLVMType(info.out_type);
            if (out_ll && !out_ll->isVoidTy())
            {
              llvm::Value *out_value = yielded_value;
              if (out_value->getType() != out_ll)
              {
                if (llvm::Value *coerced = CoerceToTyped(
                        emitter,
                        &builder,
                        out_value,
                        out_ll,
                        active_ctx ? active_ctx->LookupValueType(y.value) : nullptr,
                        info.out_type))
                {
                  out_value = coerced;
                }
                else if (llvm::Value *coerced_plain =
                             CoerceTo(&builder, out_value, out_ll))
                {
                  out_value = coerced_plain;
                }
                else
                {
                  out_value = llvm::Constant::getNullValue(out_ll);
                }
              }

              llvm::AllocaInst *src_slot = entry_builder.CreateAlloca(out_ll);
              builder.CreateStore(out_value, src_slot);
              llvm::Type *i8_ty = llvm::Type::getInt8Ty(emitter.GetContext());
              llvm::Type *i64_ty = llvm::Type::getInt64Ty(emitter.GetContext());
              llvm::Value *src_i8 = builder.CreateBitCast(
                  src_slot,
                  llvm::PointerType::get(i8_ty, 0));
              const llvm::DataLayout &dl = emitter.GetModule().getDataLayout();
              const std::uint64_t copy_size =
                  static_cast<std::uint64_t>(dl.getTypeAllocSize(out_ll));
              if (copy_size > 0)
              {
                builder.CreateMemCpy(
                    payload_i8,
                    llvm::Align(1),
                    src_i8,
                    llvm::Align(1),
                    llvm::ConstantInt::get(i64_ty, copy_size));
              }
            }
          }

          if (payload_i8 && frame_ptr)
          {
            llvm::Type *i8_ty = llvm::Type::getInt8Ty(emitter.GetContext());
            llvm::Type *i64_ty = llvm::Type::getInt64Ty(emitter.GetContext());
            llvm::Value *frame_slot_i8 = builder.CreateGEP(
                i8_ty,
                payload_i8,
                llvm::ConstantInt::get(i64_ty, kAsyncPayloadFramePtrOffset));
            llvm::Value *frame_slot = builder.CreateBitCast(
                frame_slot_i8,
                llvm::PointerType::get(emitter.GetOpaquePtr(), 0));
            llvm::Value *frame_store = CoerceTo(&builder, frame_ptr, emitter.GetOpaquePtr());
            if (!frame_store)
            {
              frame_store = builder.CreateBitCast(frame_ptr, emitter.GetOpaquePtr());
            }
            if (!frame_store)
            {
              frame_store = llvm::ConstantPointerNull::get(
                  llvm::cast<llvm::PointerType>(emitter.GetOpaquePtr()));
            }
            builder.CreateStore(frame_store, frame_slot);
          }

          suspended_value = builder.CreateLoad(async_struct, async_slot);
        }

        if (!suspended_value)
        {
          suspended_value = llvm::Constant::getNullValue(
              async_layout_ty ? async_layout_ty
                              : llvm::Type::getInt64Ty(emitter.GetContext()));
        }

        llvm::Type *ret_ty = func->getReturnType();
        const std::string sym = std::string(func->getName());
        const LowerCtx::ProcSigInfo *sig =
            active_ctx ? active_ctx->LookupProcSig(sym) : nullptr;

        if (ret_ty->isVoidTy())
        {
          (void)StoreProcedureOutValue(
              emitter,
              &builder,
              func,
              sym,
              sig,
              suspended_value,
              info.async_type);
          builder.CreateRetVoid();
        }
        else
        {
          llvm::Value *out = CoerceToTyped(
              emitter,
              &builder,
              suspended_value,
              ret_ty,
              info.async_type,
              sig ? sig->ret : nullptr);
          if (!out)
          {
            out = CoerceTo(&builder, suspended_value, ret_ty);
          }
          if (!out)
          {
            out = llvm::Constant::getNullValue(ret_ty);
          }
          builder.CreateRet(out);
        }

        builder.SetInsertPoint(cont_bb);
        if (info.is_resume && async_state->resume_switch)
        {
          if (y.release && async_state->frame_ptr)
          {
            llvm::Value *released =
                LoadAsyncFrameKeySnapshot(emitter, &builder, async_state->frame_ptr);
            EmitKeyReacquire(emitter, &builder, released);
            StoreAsyncFrameKeySnapshot(
                emitter,
                &builder,
                async_state->frame_ptr,
                NullOpaquePtr(emitter));
          }
          llvm::Value *resume_input = load_resume_input(builder);
          if (!resume_input)
          {
            resume_input = DefaultFor(y.result);
          }
          emitter.SetTempValue(y.result, resume_input);
        }
        if (!emitter.GetTempValue(y.result))
        {
          emitter.SetTempValue(y.result, DefaultFor(y.result));
        }
      }
      void operator()(const IRYieldFrom &y) const
      {
        const LowerCtx *active_ctx = emitter.GetCurrentCtx();
        analysis::TypeRef async_type = y.source_type;
        if (!async_type && active_ctx)
        {
          async_type = active_ctx->LookupValueType(y.source);
        }

        llvm::Type *expected = ExpectedLLVMType(y.result);
        analysis::TypeRef target_type =
            active_ctx ? active_ctx->LookupValueType(y.result) : nullptr;
        if (!expected && target_type)
        {
          expected = emitter.GetLLVMType(target_type);
        }

        auto fallback_result = [&](llvm::Value *value,
                                   const analysis::TypeRef &source_type) -> llvm::Value *
        {
          llvm::Value *out = value;
          if (!out && expected && !expected->isVoidTy())
          {
            out = llvm::Constant::getNullValue(expected);
          }
          if (expected)
          {
            if (target_type)
            {
              if (llvm::Value *coerced = CoerceToTyped(
                      emitter,
                      &builder,
                      out,
                      expected,
                      source_type,
                      target_type))
              {
                out = coerced;
              }
              else if (llvm::Value *plain = CoerceTo(&builder, out, expected))
              {
                out = plain;
              }
              else
              {
                out = llvm::Constant::getNullValue(expected);
              }
            }
            else if (llvm::Value *plain = CoerceTo(&builder, out, expected))
            {
              out = plain;
            }
            else
            {
              out = llvm::Constant::getNullValue(expected);
            }
          }
          if (!out)
          {
            out = DefaultFor(y.result);
          }
          return out;
        };

        const auto source_sig = analysis::GetAsyncSig(async_type);
        if (!source_sig)
        {
          emitter.SetTempValue(
              y.result,
              fallback_result(EvaluateOrDefault(y.source), async_type));
          return;
        }

        // The yield-from expression result is the delegated async result type.
        // If value-type metadata for the synthetic temp is missing, recover it
        // from the source async signature instead of defaulting to zero/null.
        if (!target_type)
        {
          target_type = source_sig->result;
        }
        if (!expected && target_type)
        {
          expected = emitter.GetLLVMType(target_type);
        }

        AsyncEmitState *async_state = emitter.GetAsyncState();
        if (!async_state || !async_state->info)
        {
          emitter.SetTempValue(
              y.result,
              fallback_result(EvaluateOrDefault(y.source), async_type));
          return;
        }

        llvm::Function *func =
            builder.GetInsertBlock() ? builder.GetInsertBlock()->getParent() : nullptr;
        if (!func)
        {
          emitter.SetTempValue(y.result, DefaultFor(y.result));
          return;
        }

        llvm::Type *async_layout_ty = emitter.GetLLVMType(async_type);
        auto *async_struct = llvm::dyn_cast_or_null<llvm::StructType>(async_layout_ty);
        if (!async_struct || async_struct->getNumElements() < 1 ||
            !async_struct->getElementType(0)->isIntegerTy())
        {
          emitter.SetTempValue(
              y.result,
              fallback_result(EvaluateOrDefault(y.source), async_type));
          return;
        }

        const LowerCtx::AsyncProcInfo &info = *async_state->info;
        const analysis::ScopeContext &scope = BuildScope(active_ctx);
        const AsyncStateDiscs source_discs =
            LoweredAsyncStateDiscs(scope, *source_sig);
        const std::uint64_t suspended_disc = source_discs.suspended;
        const std::uint64_t completed_disc = source_discs.completed;
        const std::optional<std::uint64_t> failed_disc = source_discs.failed;

        const std::string sym = std::string(func->getName());
        const LowerCtx::ProcSigInfo *proc_sig =
            active_ctx ? active_ctx->LookupProcSig(sym) : nullptr;

        llvm::IRBuilder<> entry_builder(
            &func->getEntryBlock(),
            func->getEntryBlock().begin());

        auto materialize_as_type = [&](llvm::Value *value, llvm::Type *dst_ty) -> llvm::Value *
        {
          if (!value || !dst_ty)
          {
            return nullptr;
          }
          if (value->getType() == dst_ty)
          {
            return value;
          }
          if (llvm::Value *coerced = CoerceTo(&builder, value, dst_ty))
          {
            return coerced;
          }

          llvm::AllocaInst *dst_slot = entry_builder.CreateAlloca(dst_ty);
          builder.CreateStore(llvm::Constant::getNullValue(dst_ty), dst_slot);
          llvm::AllocaInst *src_slot = entry_builder.CreateAlloca(value->getType());
          builder.CreateStore(value, src_slot);

          llvm::Type *i8_ty = llvm::Type::getInt8Ty(emitter.GetContext());
          llvm::Type *i64_ty = llvm::Type::getInt64Ty(emitter.GetContext());
          llvm::Value *dst_i8 = builder.CreateBitCast(dst_slot, llvm::PointerType::get(i8_ty, 0));
          llvm::Value *src_i8 = builder.CreateBitCast(src_slot, llvm::PointerType::get(i8_ty, 0));
          const llvm::DataLayout &dl = emitter.GetModule().getDataLayout();
          const std::uint64_t src_size =
              static_cast<std::uint64_t>(dl.getTypeAllocSize(value->getType()));
          const std::uint64_t dst_size =
              static_cast<std::uint64_t>(dl.getTypeAllocSize(dst_ty));
          const std::uint64_t copy_size = std::min(src_size, dst_size);
          if (copy_size > 0)
          {
            builder.CreateMemCpy(
                dst_i8,
                llvm::Align(1),
                src_i8,
                llvm::Align(1),
                llvm::ConstantInt::get(i64_ty, copy_size));
          }
          return builder.CreateLoad(dst_ty, dst_slot);
        };

        auto coerce_to_result = [&](llvm::Value *value,
                                    const analysis::TypeRef &source_type) -> llvm::Value *
        {
          if (!expected || expected->isVoidTy())
          {
            return nullptr;
          }
          llvm::Value *out = value;
          if (!out)
          {
            return llvm::Constant::getNullValue(expected);
          }
          if (target_type)
          {
            if (llvm::Value *coerced = CoerceToTyped(
                    emitter,
                    &builder,
                    out,
                    expected,
                    source_type,
                    target_type))
            {
              return coerced;
            }
          }
          if (llvm::Value *coerced = CoerceTo(&builder, out, expected))
          {
            return coerced;
          }
          return materialize_as_type(out, expected);
        };

        std::string source_slot_name;
        if (y.source.kind == IRValue::Kind::Local)
        {
          source_slot_name = y.source.name;
          if (!source_slot_name.empty() &&
              !info.slots.contains(source_slot_name))
          {
            std::string best_match;
            for (const auto &[slot_name, slot_info] : info.slots)
            {
              (void)slot_info;
              if (slot_name.empty())
              {
                continue;
              }
              const bool prefix_match =
                  source_slot_name.rfind(slot_name, 0) == 0 ||
                  slot_name.rfind(source_slot_name, 0) == 0;
              if (!prefix_match)
              {
                continue;
              }
              if (slot_name.size() > best_match.size())
              {
                best_match = slot_name;
              }
            }
            if (!best_match.empty())
            {
              source_slot_name = best_match;
            }
          }
        }

        llvm::Value *source_slot = nullptr;
        if (!source_slot_name.empty())
        {
          source_slot = emitter.GetLocal(source_slot_name);
        }
        if (!source_slot || !source_slot->getType()->isPointerTy())
        {
          source_slot = entry_builder.CreateAlloca(async_struct);
        }
        llvm::Type *async_ptr_ty = llvm::PointerType::get(async_struct, 0);
        if (source_slot->getType() != async_ptr_ty)
        {
          source_slot = builder.CreateBitCast(source_slot, async_ptr_ty);
        }

        auto extract_async_payload = [&](const analysis::TypeRef &payload_type) -> llvm::Value *
        {
          if (!payload_type ||
              IsUnitTypeRef(payload_type) ||
              IsNeverTypeRef(payload_type))
          {
            return nullptr;
          }
          llvm::Type *payload_ll = emitter.GetLLVMType(payload_type);
          if (!payload_ll || payload_ll->isVoidTy())
          {
            return nullptr;
          }
          llvm::Value *payload_i8 = CreateTaggedPayloadI8Ptr(
              emitter,
              &builder,
              async_struct,
              source_slot,
              ::cursive::analysis::layout::kPtrAlign);
          if (!payload_i8)
          {
            return nullptr;
          }
          llvm::Value *payload_ptr = builder.CreateBitCast(
              payload_i8,
              llvm::PointerType::get(payload_ll, 0));
          llvm::LoadInst *loaded = builder.CreateLoad(payload_ll, payload_ptr);
          loaded->setAlignment(llvm::Align(1));
          return loaded;
        };

        auto emit_async_return = [&](llvm::Value *value,
                                     const analysis::TypeRef &source_type)
        {
          llvm::Type *ret_ty = func->getReturnType();
          if (ret_ty->isVoidTy())
          {
            (void)StoreProcedureOutValue(
                emitter,
                &builder,
                func,
                sym,
                proc_sig,
                value,
                source_type);
            builder.CreateRetVoid();
            return;
          }

          llvm::Value *out = CoerceToTyped(
              emitter,
              &builder,
              value,
              ret_ty,
              source_type,
              proc_sig ? proc_sig->ret : nullptr);
          if (!out)
          {
            out = CoerceTo(&builder, value, ret_ty);
          }
          if (!out)
          {
            out = llvm::Constant::getNullValue(ret_ty);
          }
          builder.CreateRet(out);
        };

        auto ensure_async_frame = [&]() -> llvm::Value *
        {
          if (async_state->frame_ptr)
          {
            return async_state->frame_ptr;
          }
          if (!info.is_wrapper)
          {
            return nullptr;
          }

          const std::string alloc_sym = BuiltinSymAsyncAllocFrame();
          llvm::Function *alloc_fn = emitter.GetModule().getFunction(alloc_sym);
          if (!alloc_fn)
          {
            llvm::FunctionType *alloc_ty = llvm::FunctionType::get(
                emitter.GetOpaquePtr(),
                {llvm::Type::getInt64Ty(emitter.GetContext()),
                 llvm::Type::getInt64Ty(emitter.GetContext())},
                false);
            alloc_fn = llvm::Function::Create(
                alloc_ty,
                llvm::GlobalValue::ExternalLinkage,
                alloc_sym,
                &emitter.GetModule());
          }
          if (!alloc_fn)
          {
            return nullptr;
          }

          llvm::Value *frame_raw = builder.CreateCall(
              alloc_fn,
              {llvm::ConstantInt::get(llvm::Type::getInt64Ty(emitter.GetContext()),
                                      info.frame_size),
               llvm::ConstantInt::get(llvm::Type::getInt64Ty(emitter.GetContext()),
                                      std::max<std::uint64_t>(1, info.frame_align))});
          async_state->frame_ptr = CoerceTo(&builder, frame_raw, emitter.GetOpaquePtr());
          if (!async_state->frame_ptr)
          {
            async_state->frame_ptr = frame_raw;
          }
          if (!async_state->frame_ptr)
          {
            return nullptr;
          }

          llvm::Type *i64_ty = llvm::Type::getInt64Ty(emitter.GetContext());
          llvm::Value *resume_state_ptr = AsyncFrameTypedPtr(
              emitter,
              &builder,
              async_state->frame_ptr,
              kAsyncFrameResumeStateOffset,
              i64_ty);
          if (resume_state_ptr)
          {
            builder.CreateStore(llvm::ConstantInt::get(i64_ty, 0), resume_state_ptr);
          }

          llvm::Value *resume_fn_ptr = llvm::ConstantPointerNull::get(
              llvm::cast<llvm::PointerType>(emitter.GetOpaquePtr()));
          if (llvm::Function *resume_fn = emitter.GetFunction(info.resume_symbol))
          {
            if (llvm::Value *coerced = CoerceTo(&builder, resume_fn, emitter.GetOpaquePtr()))
            {
              resume_fn_ptr = coerced;
            }
          }
          llvm::Value *resume_fn_slot = AsyncFrameTypedPtr(
              emitter,
              &builder,
              async_state->frame_ptr,
              kAsyncFrameResumeFnOffset,
              emitter.GetOpaquePtr());
          if (resume_fn_slot)
          {
            builder.CreateStore(resume_fn_ptr, resume_fn_slot);
          }

          StoreAsyncFrameHostedEnv(
              emitter,
              &builder,
              async_state->frame_ptr,
              emitter.GetHostedCurrentEnvPtr());
          StoreAsyncFrameKeySnapshot(
              emitter,
              &builder,
              async_state->frame_ptr,
              NullOpaquePtr(emitter));

          return async_state->frame_ptr;
        };

        auto snapshot_async_slots = [&]()
        {
          if (!async_state->frame_ptr)
          {
            return;
          }
          for (const auto &slot_name : info.slot_order)
          {
            const auto slot_it = info.slots.find(slot_name);
            if (slot_it == info.slots.end())
            {
              continue;
            }
            const auto &slot = slot_it->second;
            llvm::Type *slot_ty = emitter.GetLLVMType(slot.type);
            if (!slot_ty || slot_ty->isVoidTy())
            {
              continue;
            }

            llvm::Value *local_value = LoadLocalValue(emitter, &builder, slot_name);
            if (!local_value)
            {
              continue;
            }

            analysis::TypeRef source_type = emitter.LookupLocalType(slot_name);
            llvm::Value *stored_value = local_value;
            if (stored_value->getType() != slot_ty)
            {
              if (llvm::Value *coerced = CoerceToTyped(
                      emitter,
                      &builder,
                      stored_value,
                      slot_ty,
                      source_type,
                      slot.type))
              {
                stored_value = coerced;
              }
              else if (llvm::Value *coerced_plain =
                           CoerceTo(&builder, stored_value, slot_ty))
              {
                stored_value = coerced_plain;
              }
              else
              {
                stored_value = llvm::Constant::getNullValue(slot_ty);
              }
            }

            llvm::Value *frame_slot_ptr = AsyncFrameTypedPtr(
                emitter,
                &builder,
                async_state->frame_ptr,
                slot.offset,
                slot_ty);
            if (!frame_slot_ptr)
            {
              continue;
            }
            llvm::StoreInst *st = builder.CreateStore(stored_value, frame_slot_ptr);
            st->setAlignment(llvm::Align(std::max<std::uint64_t>(1, slot.align)));
          }
        };

        auto emit_outer_suspended = [&](llvm::Value *yielded_value,
                                        const analysis::TypeRef &yielded_type)
        {
          llvm::Value *frame_ptr = ensure_async_frame();
          if (y.release && frame_ptr)
          {
            llvm::Value *released = EmitKeyReleaseAll(emitter, &builder);
            StoreAsyncFrameKeySnapshot(emitter, &builder, frame_ptr, released);
          }
          if (frame_ptr)
          {
            snapshot_async_slots();
            llvm::Type *i64_ty = llvm::Type::getInt64Ty(emitter.GetContext());
            llvm::Value *state_ptr = AsyncFrameTypedPtr(
                emitter,
                &builder,
                frame_ptr,
                kAsyncFrameResumeStateOffset,
                i64_ty);
            if (state_ptr)
            {
              builder.CreateStore(
                  llvm::ConstantInt::get(i64_ty, y.state_index),
                  state_ptr);
            }
          }

          analysis::TypeRef outer_async_type = info.async_type;
          llvm::Type *outer_layout_ty =
              outer_async_type ? emitter.GetLLVMType(outer_async_type) : nullptr;
          auto *outer_struct = llvm::dyn_cast_or_null<llvm::StructType>(outer_layout_ty);
          llvm::Value *suspended_value = nullptr;
          if (outer_struct && outer_struct->getNumElements() >= 1 &&
              outer_struct->getElementType(0)->isIntegerTy())
          {
            llvm::AllocaInst *outer_slot = entry_builder.CreateAlloca(outer_struct);
            builder.CreateStore(llvm::Constant::getNullValue(outer_struct), outer_slot);

            llvm::Type *disc_ty = outer_struct->getElementType(0);
            llvm::Value *disc_ptr = builder.CreateStructGEP(outer_struct, outer_slot, 0);
            builder.CreateStore(
                llvm::ConstantInt::get(disc_ty, suspended_disc),
                disc_ptr);

            llvm::Value *payload_i8 = CreateTaggedPayloadI8Ptr(
                emitter,
                &builder,
                outer_struct,
                outer_slot,
                ::cursive::analysis::layout::kPtrAlign);

            if (payload_i8 &&
                info.out_type &&
                !IsUnitTypeRef(info.out_type) &&
                !IsNeverTypeRef(info.out_type))
            {
              llvm::Type *out_ll = emitter.GetLLVMType(info.out_type);
              if (out_ll && !out_ll->isVoidTy())
              {
                llvm::Value *out_value = yielded_value;
                if (!out_value)
                {
                  out_value = llvm::Constant::getNullValue(out_ll);
                }
                else if (out_value->getType() != out_ll)
                {
                  if (llvm::Value *coerced = CoerceToTyped(
                          emitter,
                          &builder,
                          out_value,
                          out_ll,
                          yielded_type,
                          info.out_type))
                  {
                    out_value = coerced;
                  }
                  else if (llvm::Value *plain = CoerceTo(&builder, out_value, out_ll))
                  {
                    out_value = plain;
                  }
                  else
                  {
                    out_value = llvm::Constant::getNullValue(out_ll);
                  }
                }

                llvm::AllocaInst *src_slot = entry_builder.CreateAlloca(out_ll);
                builder.CreateStore(out_value, src_slot);
                llvm::Type *i8_ty = llvm::Type::getInt8Ty(emitter.GetContext());
                llvm::Type *i64_ty = llvm::Type::getInt64Ty(emitter.GetContext());
                llvm::Value *src_i8 = builder.CreateBitCast(
                    src_slot,
                    llvm::PointerType::get(i8_ty, 0));
                const llvm::DataLayout &dl = emitter.GetModule().getDataLayout();
                const std::uint64_t copy_size =
                    static_cast<std::uint64_t>(dl.getTypeAllocSize(out_ll));
                if (copy_size > 0)
                {
                  builder.CreateMemCpy(
                      payload_i8,
                      llvm::Align(1),
                      src_i8,
                      llvm::Align(1),
                      llvm::ConstantInt::get(i64_ty, copy_size));
                }
              }
            }

            if (payload_i8 && frame_ptr)
            {
              llvm::Type *i8_ty = llvm::Type::getInt8Ty(emitter.GetContext());
              llvm::Type *i64_ty = llvm::Type::getInt64Ty(emitter.GetContext());
              llvm::Value *frame_slot_i8 = builder.CreateGEP(
                  i8_ty,
                  payload_i8,
                  llvm::ConstantInt::get(i64_ty, kAsyncPayloadFramePtrOffset));
              llvm::Value *frame_slot = builder.CreateBitCast(
                  frame_slot_i8,
                  llvm::PointerType::get(emitter.GetOpaquePtr(), 0));
              llvm::Value *frame_store = CoerceTo(&builder, frame_ptr, emitter.GetOpaquePtr());
              if (!frame_store)
              {
                frame_store = builder.CreateBitCast(frame_ptr, emitter.GetOpaquePtr());
              }
              if (!frame_store)
              {
                frame_store = llvm::ConstantPointerNull::get(
                    llvm::cast<llvm::PointerType>(emitter.GetOpaquePtr()));
              }
              builder.CreateStore(frame_store, frame_slot);
            }

            suspended_value = builder.CreateLoad(outer_struct, outer_slot);
          }

          if (!suspended_value)
          {
            suspended_value = llvm::Constant::getNullValue(
                outer_layout_ty ? outer_layout_ty
                                : llvm::Type::getInt64Ty(emitter.GetContext()));
          }
          emit_async_return(suspended_value, info.async_type);
        };

        llvm::Type *i8_ty = llvm::Type::getInt8Ty(emitter.GetContext());
        llvm::Type *opaque_ptr_ty = emitter.GetOpaquePtr();
        auto *opaque_ptr_ptr_ty = llvm::cast<llvm::PointerType>(opaque_ptr_ty);

        llvm::BasicBlock *loop_bb =
            llvm::BasicBlock::Create(emitter.GetContext(), "yield_from.loop", func);
        llvm::BasicBlock *suspended_bb =
            llvm::BasicBlock::Create(emitter.GetContext(), "yield_from.suspended", func);
        llvm::BasicBlock *completed_bb =
            llvm::BasicBlock::Create(emitter.GetContext(), "yield_from.completed", func);
        llvm::BasicBlock *failed_bb = failed_disc.has_value()
                                          ? llvm::BasicBlock::Create(
                                                emitter.GetContext(),
                                                "yield_from.failed",
                                                func)
                                          : nullptr;
        llvm::BasicBlock *fallback_bb =
            llvm::BasicBlock::Create(emitter.GetContext(), "yield_from.fallback", func);
        llvm::BasicBlock *panic_bb =
            llvm::BasicBlock::Create(emitter.GetContext(), "yield_from.panic", func);
        llvm::BasicBlock *cont_bb =
            llvm::BasicBlock::Create(emitter.GetContext(), "yield_from.cont", func);
        llvm::AllocaInst *result_slot = nullptr;
        if (expected && !expected->isVoidTy())
        {
          result_slot = entry_builder.CreateAlloca(expected);
          builder.CreateStore(llvm::Constant::getNullValue(expected), result_slot);
        }

        llvm::BasicBlock *resume_entry_bb = nullptr;
        bool emit_resume_body = false;
        if (info.is_resume && async_state->resume_switch)
        {
          auto it = async_state->resume_blocks.find(y.state_index);
          if (it == async_state->resume_blocks.end())
          {
            resume_entry_bb = llvm::BasicBlock::Create(
                emitter.GetContext(),
                "yield_from.resume." + std::to_string(y.state_index),
                func);
            async_state->resume_blocks[y.state_index] = resume_entry_bb;
            if (auto *disc_ty = llvm::dyn_cast<llvm::IntegerType>(
                    async_state->resume_switch->getCondition()->getType()))
            {
              async_state->resume_switch->addCase(
                  llvm::ConstantInt::get(disc_ty, y.state_index),
                  resume_entry_bb);
            }
            emit_resume_body = true;
          }
          else
          {
            resume_entry_bb = it->second;
          }
        }

        llvm::Value *delegated_async = EvaluateOrDefault(y.source);
        llvm::Value *initial_async = materialize_as_type(delegated_async, async_struct);
        if (!initial_async)
        {
          initial_async = llvm::Constant::getNullValue(async_struct);
        }
        builder.CreateStore(initial_async, source_slot);
        builder.CreateBr(loop_bb);

        if (emit_resume_body && resume_entry_bb)
        {
          builder.SetInsertPoint(resume_entry_bb);
          if (y.release && async_state->frame_ptr)
          {
            llvm::Value *released =
                LoadAsyncFrameKeySnapshot(emitter, &builder, async_state->frame_ptr);
            EmitKeyReacquire(emitter, &builder, released);
            StoreAsyncFrameKeySnapshot(
                emitter,
                &builder,
                async_state->frame_ptr,
                NullOpaquePtr(emitter));
          }
          llvm::Value *suspended_ptr = builder.CreateBitCast(source_slot, opaque_ptr_ty);
          llvm::Value *resume_input_ptr = llvm::ConstantPointerNull::get(opaque_ptr_ptr_ty);
          if (source_sig->in &&
              !IsUnitTypeRef(source_sig->in) &&
              !IsNeverTypeRef(source_sig->in))
          {
            resume_input_ptr = async_state->input_ptr;
            if (resume_input_ptr)
            {
              if (llvm::Value *coerced = CoerceTo(&builder, resume_input_ptr, opaque_ptr_ty))
              {
                resume_input_ptr = coerced;
              }
              else if (resume_input_ptr->getType()->isPointerTy())
              {
                resume_input_ptr = builder.CreateBitCast(resume_input_ptr, opaque_ptr_ty);
              }
              else
              {
                resume_input_ptr = llvm::ConstantPointerNull::get(opaque_ptr_ptr_ty);
              }
            }
            else
            {
              resume_input_ptr = llvm::ConstantPointerNull::get(opaque_ptr_ptr_ty);
            }
          }

          llvm::Value *resume_panic_ptr =
              LoadLocalValue(emitter, &builder, std::string(kPanicOutName));
          bool resume_has_panic_ptr = resume_panic_ptr != nullptr;
          if (resume_panic_ptr)
          {
            if (llvm::Value *coerced = CoerceTo(&builder, resume_panic_ptr, opaque_ptr_ty))
            {
              resume_panic_ptr = coerced;
            }
            else if (resume_panic_ptr->getType()->isPointerTy())
            {
              resume_panic_ptr = builder.CreateBitCast(resume_panic_ptr, opaque_ptr_ty);
            }
            else
            {
              resume_panic_ptr = nullptr;
              resume_has_panic_ptr = false;
            }
          }
          if (!resume_panic_ptr)
          {
            resume_panic_ptr = llvm::ConstantPointerNull::get(opaque_ptr_ptr_ty);
          }

          llvm::Value *resume_call = EmitAsyncResumeRuntimeCall(
              emitter,
              &builder,
              suspended_ptr,
              resume_input_ptr,
              resume_panic_ptr);
          llvm::Value *resumed_async = materialize_as_type(resume_call, async_struct);
          if (!resumed_async)
          {
            resumed_async = llvm::Constant::getNullValue(async_struct);
          }
          builder.CreateStore(resumed_async, source_slot);
          if (resume_has_panic_ptr)
          {
            llvm::Value *panic_i8 = resume_panic_ptr;
            if (panic_i8->getType() != llvm::PointerType::get(i8_ty, 0))
            {
              panic_i8 = builder.CreateBitCast(panic_i8, llvm::PointerType::get(i8_ty, 0));
            }
            llvm::LoadInst *panic_flag = builder.CreateLoad(i8_ty, panic_i8);
            llvm::Value *has_panic = builder.CreateICmpNE(
                panic_flag,
                llvm::ConstantInt::get(i8_ty, 0));
            builder.CreateCondBr(has_panic, panic_bb, loop_bb);
          }
          else
          {
            builder.CreateBr(loop_bb);
          }
        }

        builder.SetInsertPoint(loop_bb);
        llvm::Value *current_async = builder.CreateLoad(async_struct, source_slot);
        llvm::Value *disc = builder.CreateExtractValue(current_async, {0u});
        auto *disc_ty = llvm::cast<llvm::IntegerType>(disc->getType());
        llvm::SwitchInst *state_sw = builder.CreateSwitch(
            disc, fallback_bb, failed_disc.has_value() ? 3 : 2);
        state_sw->addCase(llvm::ConstantInt::get(disc_ty, suspended_disc), suspended_bb);
        state_sw->addCase(llvm::ConstantInt::get(disc_ty, completed_disc), completed_bb);
        if (failed_disc.has_value())
        {
          state_sw->addCase(llvm::ConstantInt::get(disc_ty, *failed_disc), failed_bb);
        }

        builder.SetInsertPoint(suspended_bb);
        llvm::Value *yielded_value = extract_async_payload(source_sig->out);
        emit_outer_suspended(yielded_value, source_sig->out);
        if (!builder.GetInsertBlock()->getTerminator())
        {
          builder.CreateBr(cont_bb);
        }

        builder.SetInsertPoint(completed_bb);
        llvm::Value *completed_payload = extract_async_payload(source_sig->result);
        llvm::Value *completed_value = coerce_to_result(completed_payload, source_sig->result);
        if (!completed_value)
        {
          completed_value = DefaultFor(y.result);
        }
        if (result_slot)
        {
          if (completed_value->getType() != expected)
          {
            if (llvm::Value *coerced = CoerceTo(&builder, completed_value, expected))
            {
              completed_value = coerced;
            }
            else
            {
              completed_value = llvm::Constant::getNullValue(expected);
            }
          }
          builder.CreateStore(completed_value, result_slot);
        }
        builder.CreateBr(cont_bb);

        if (failed_bb)
        {
          builder.SetInsertPoint(failed_bb);
          llvm::Value *failed_payload = extract_async_payload(source_sig->err);
          if (!failed_payload &&
              source_sig->err &&
              !IsUnitTypeRef(source_sig->err) &&
              !IsNeverTypeRef(source_sig->err))
          {
            if (llvm::Type *err_ty = emitter.GetLLVMType(source_sig->err))
            {
              if (!err_ty->isVoidTy())
              {
                failed_payload = llvm::Constant::getNullValue(err_ty);
              }
            }
          }
          emit_async_return(failed_payload, source_sig->err);
          if (!builder.GetInsertBlock()->getTerminator())
          {
            builder.CreateBr(cont_bb);
          }
        }

        builder.SetInsertPoint(fallback_bb);
        llvm::Value *fallback_async = builder.CreateLoad(async_struct, source_slot);
        llvm::Value *fallback_value = coerce_to_result(fallback_async, async_type);
        if (!fallback_value)
        {
          fallback_value = DefaultFor(y.result);
        }
        if (result_slot)
        {
          if (fallback_value->getType() != expected)
          {
            if (llvm::Value *coerced = CoerceTo(&builder, fallback_value, expected))
            {
              fallback_value = coerced;
            }
            else
            {
              fallback_value = llvm::Constant::getNullValue(expected);
            }
          }
          builder.CreateStore(fallback_value, result_slot);
        }
        builder.CreateBr(cont_bb);

        builder.SetInsertPoint(panic_bb);
        if (result_slot)
        {
          builder.CreateStore(llvm::Constant::getNullValue(expected), result_slot);
        }
        builder.CreateBr(cont_bb);

        builder.SetInsertPoint(cont_bb);
        llvm::Value *result_value = nullptr;
        if (result_slot && expected && !expected->isVoidTy())
        {
          result_value = builder.CreateLoad(expected, result_slot);
        }
        if (!result_value)
        {
          result_value = DefaultFor(y.result);
        }
        emitter.SetTempValue(y.result, result_value);
      }
      void operator()(const IRSpecSnapshot &spec) const
      {
        emitter.SetTempValue(spec.result, DefaultFor(spec.result));
      }
      void operator()(const IRSpecValidate &spec) const
      {
        emitter.SetTempValue(spec.result, DefaultFor(spec.result));
      }
      void operator()(const IRSpecCommit &spec) const
      {
        llvm::Value *value = EvaluateOrDefault(spec.value);
        if (!value)
        {
          value = DefaultFor(spec.result);
        }
        emitter.SetTempValue(spec.result, value);
      }
      void operator()(const IRSpecRetry &spec) const
      {
        emitter.SetTempValue(spec.result, DefaultFor(spec.result));
      }
      void operator()(const IRSpecFallback &spec) const
      {
        emitter.EmitIR(spec.body);
        if (!emitter.GetTempValue(spec.result))
        {
          emitter.SetTempValue(spec.result, DefaultFor(spec.result));
        }
      }
      void operator()(const IRSpecLoop &spec) const
      {
        emitter.EmitIR(spec.fallback_ir);
        if (!emitter.GetTempValue(spec.result))
        {
          emitter.SetTempValue(spec.result, DefaultFor(spec.result));
        }
      }
      void operator()(const IRSync &s) const
      {
        llvm::Value *initial_async = EvaluateOrDefault(s.async_value);
        if (!initial_async)
        {
          emitter.SetTempValue(s.result, DefaultFor(s.result));
          return;
        }

        llvm::Type *expected = ExpectedLLVMType(s.result);
        const LowerCtx *active_ctx = emitter.GetCurrentCtx();
        analysis::TypeRef target_type =
            active_ctx ? active_ctx->LookupValueType(s.result) : nullptr;
        if (!expected && target_type)
        {
          expected = emitter.GetLLVMType(target_type);
        }
        if (!expected && s.result_type)
        {
          expected = emitter.GetLLVMType(s.result_type);
        }

        auto *async_struct = llvm::dyn_cast<llvm::StructType>(initial_async->getType());
        if (!async_struct || async_struct->getNumElements() < 1 ||
            !async_struct->getElementType(0)->isIntegerTy())
        {
          llvm::Value *fallback = initial_async;
          if (expected)
          {
            if (target_type)
            {
              if (llvm::Value *coerced = CoerceToTyped(
                      emitter,
                      &builder,
                      fallback,
                      expected,
                      s.async_type,
                      target_type))
              {
                fallback = coerced;
              }
              else if (llvm::Value *plain = CoerceTo(&builder, fallback, expected))
              {
                fallback = plain;
              }
              else
              {
                fallback = llvm::Constant::getNullValue(expected);
              }
            }
            else if (llvm::Value *plain = CoerceTo(&builder, fallback, expected))
            {
              fallback = plain;
            }
            else
            {
              fallback = llvm::Constant::getNullValue(expected);
            }
          }
          if (!fallback)
          {
            fallback = DefaultFor(s.result);
          }
          emitter.SetTempValue(s.result, fallback);
          return;
        }

        llvm::Function *func =
            builder.GetInsertBlock() ? builder.GetInsertBlock()->getParent() : nullptr;
        if (!func)
        {
          emitter.SetTempValue(s.result, DefaultFor(s.result));
          return;
        }

        llvm::IRBuilder<> entry_builder(
            &func->getEntryBlock(),
            func->getEntryBlock().begin());
        llvm::AllocaInst *async_slot = entry_builder.CreateAlloca(async_struct);
        builder.CreateStore(initial_async, async_slot);

        llvm::AllocaInst *result_slot = nullptr;
        if (expected && !expected->isVoidTy())
        {
          result_slot = entry_builder.CreateAlloca(expected);
          builder.CreateStore(llvm::Constant::getNullValue(expected), result_slot);
        }

        auto materialize_as_type = [&](llvm::Value *value, llvm::Type *dst_ty) -> llvm::Value *
        {
          if (!value || !dst_ty)
          {
            return nullptr;
          }
          if (value->getType() == dst_ty)
          {
            return value;
          }
          if (llvm::Value *coerced = CoerceTo(&builder, value, dst_ty))
          {
            return coerced;
          }

          llvm::AllocaInst *dst_slot = entry_builder.CreateAlloca(dst_ty);
          builder.CreateStore(llvm::Constant::getNullValue(dst_ty), dst_slot);
          llvm::AllocaInst *src_slot = entry_builder.CreateAlloca(value->getType());
          builder.CreateStore(value, src_slot);

          llvm::Type *i8_ty = llvm::Type::getInt8Ty(emitter.GetContext());
          llvm::Type *i64_ty = llvm::Type::getInt64Ty(emitter.GetContext());
          llvm::Value *dst_i8 = builder.CreateBitCast(dst_slot, llvm::PointerType::get(i8_ty, 0));
          llvm::Value *src_i8 = builder.CreateBitCast(src_slot, llvm::PointerType::get(i8_ty, 0));
          const llvm::DataLayout &dl = emitter.GetModule().getDataLayout();
          const std::uint64_t src_size =
              static_cast<std::uint64_t>(dl.getTypeAllocSize(value->getType()));
          const std::uint64_t dst_size =
              static_cast<std::uint64_t>(dl.getTypeAllocSize(dst_ty));
          const std::uint64_t copy_size = std::min(src_size, dst_size);
          if (copy_size > 0)
          {
            builder.CreateMemCpy(
                dst_i8,
                llvm::Align(1),
                src_i8,
                llvm::Align(1),
                llvm::ConstantInt::get(i64_ty, copy_size));
          }
          return builder.CreateLoad(dst_ty, dst_slot);
        };

        auto coerce_to_result = [&](llvm::Value *value,
                                    const analysis::TypeRef &source_type) -> llvm::Value *
        {
          if (!expected || expected->isVoidTy())
          {
            return nullptr;
          }
          llvm::Value *out = value;
          if (!out)
          {
            return llvm::Constant::getNullValue(expected);
          }
          if (target_type)
          {
            if (llvm::Value *coerced = CoerceToTyped(
                    emitter,
                    &builder,
                    out,
                    expected,
                    source_type,
                    target_type))
            {
              return coerced;
            }
          }
          if (llvm::Value *coerced = CoerceTo(&builder, out, expected))
          {
            return coerced;
          }
          return materialize_as_type(out, expected);
        };

        auto extract_async_payload = [&](llvm::Value *async_value,
                                         const analysis::TypeRef &payload_type) -> llvm::Value *
        {
          if (!async_value || !payload_type ||
              IsUnitTypeRef(payload_type) ||
              IsNeverTypeRef(payload_type))
          {
            return nullptr;
          }
          llvm::Type *payload_ll = emitter.GetLLVMType(payload_type);
          if (!payload_ll || payload_ll->isVoidTy())
          {
            return nullptr;
          }
          llvm::AllocaInst *payload_async_slot = entry_builder.CreateAlloca(async_struct);
          builder.CreateStore(async_value, payload_async_slot);
          llvm::Value *payload_i8 = CreateTaggedPayloadI8Ptr(
              emitter,
              &builder,
              async_struct,
              payload_async_slot,
              ::cursive::analysis::layout::kPtrAlign);
          if (!payload_i8)
          {
            return nullptr;
          }
          llvm::Value *payload_ptr = builder.CreateBitCast(
              payload_i8,
              llvm::PointerType::get(payload_ll, 0));
          llvm::LoadInst *loaded = builder.CreateLoad(payload_ll, payload_ptr);
          loaded->setAlignment(llvm::Align(1));
          return loaded;
        };

        const analysis::ScopeContext &scope = BuildScope(active_ctx);
        const AsyncStateDiscs async_discs =
            LoweredAsyncStateDiscs(scope, s.async_type);
        const std::uint64_t suspended_disc = async_discs.suspended;
        const std::uint64_t completed_disc = async_discs.completed;
        const std::optional<std::uint64_t> failed_disc = async_discs.failed;

        analysis::TypeRef completed_type = s.result_type;
        analysis::TypeRef error_type = s.error_type;
        if (const auto sig = analysis::GetAsyncSig(s.async_type))
        {
          if (!completed_type)
          {
            completed_type = sig->result;
          }
          if (!error_type)
          {
            error_type = sig->err;
          }
        }

        llvm::Type *i8_ty = llvm::Type::getInt8Ty(emitter.GetContext());
        llvm::Type *i64_ty = llvm::Type::getInt64Ty(emitter.GetContext());
        llvm::Type *opaque_ptr_ty = emitter.GetOpaquePtr();
        auto *opaque_ptr_ptr_ty = llvm::cast<llvm::PointerType>(opaque_ptr_ty);

        llvm::Value *panic_ptr = LoadLocalValue(emitter, &builder, std::string(kPanicOutName));
        bool has_panic_ptr = panic_ptr != nullptr;
        if (panic_ptr)
        {
          if (llvm::Value *coerced = CoerceTo(&builder, panic_ptr, opaque_ptr_ty))
          {
            panic_ptr = coerced;
          }
          else if (panic_ptr->getType()->isPointerTy())
          {
            panic_ptr = builder.CreateBitCast(panic_ptr, opaque_ptr_ty);
          }
          else
          {
            panic_ptr = nullptr;
            has_panic_ptr = false;
          }
        }
        if (!panic_ptr)
        {
          panic_ptr = llvm::ConstantPointerNull::get(opaque_ptr_ptr_ty);
        }

        llvm::BasicBlock *loop_bb =
            llvm::BasicBlock::Create(emitter.GetContext(), "sync.loop", func);
        llvm::BasicBlock *suspended_bb =
            llvm::BasicBlock::Create(emitter.GetContext(), "sync.suspended", func);
        llvm::BasicBlock *completed_bb =
            llvm::BasicBlock::Create(emitter.GetContext(), "sync.completed", func);
        llvm::BasicBlock *failed_bb = failed_disc.has_value()
                                          ? llvm::BasicBlock::Create(
                                                emitter.GetContext(),
                                                "sync.failed",
                                                func)
                                          : nullptr;
        llvm::BasicBlock *fallback_bb =
            llvm::BasicBlock::Create(emitter.GetContext(), "sync.fallback", func);
        llvm::BasicBlock *panic_bb =
            llvm::BasicBlock::Create(emitter.GetContext(), "sync.panic", func);
        llvm::BasicBlock *merge_bb =
            llvm::BasicBlock::Create(emitter.GetContext(), "sync.merge", func);

        builder.CreateBr(loop_bb);

        builder.SetInsertPoint(loop_bb);
        llvm::Value *current_async = builder.CreateLoad(async_struct, async_slot);
        llvm::Value *disc = builder.CreateExtractValue(current_async, {0u});
        auto *disc_ty = llvm::cast<llvm::IntegerType>(disc->getType());
        llvm::SwitchInst *state_sw = builder.CreateSwitch(
            disc, fallback_bb, failed_disc.has_value() ? 3 : 2);
        state_sw->addCase(llvm::ConstantInt::get(disc_ty, suspended_disc), suspended_bb);
        state_sw->addCase(llvm::ConstantInt::get(disc_ty, completed_disc), completed_bb);
        if (failed_disc.has_value())
        {
          state_sw->addCase(llvm::ConstantInt::get(disc_ty, *failed_disc), failed_bb);
        }

        builder.SetInsertPoint(suspended_bb);
        llvm::Value *suspended_ptr = builder.CreateBitCast(async_slot, opaque_ptr_ty);
        llvm::Value *unit_input = llvm::ConstantPointerNull::get(opaque_ptr_ptr_ty);
        llvm::Value *resume_call = EmitAsyncResumeRuntimeCall(
            emitter,
            &builder,
            suspended_ptr,
            unit_input,
            panic_ptr);
        llvm::Value *resumed_async = materialize_as_type(resume_call, async_struct);
        if (!resumed_async)
        {
          resumed_async = llvm::Constant::getNullValue(async_struct);
        }
        builder.CreateStore(resumed_async, async_slot);
        if (has_panic_ptr)
        {
          llvm::Value *panic_i8 = panic_ptr;
          if (panic_i8->getType() != llvm::PointerType::get(i8_ty, 0))
          {
            panic_i8 = builder.CreateBitCast(panic_i8, llvm::PointerType::get(i8_ty, 0));
          }
          llvm::LoadInst *panic_flag = builder.CreateLoad(i8_ty, panic_i8);
          llvm::Value *has_panic = builder.CreateICmpNE(
              panic_flag,
              llvm::ConstantInt::get(i8_ty, 0));
          builder.CreateCondBr(has_panic, panic_bb, loop_bb);
        }
        else
        {
          builder.CreateBr(loop_bb);
        }

        builder.SetInsertPoint(completed_bb);
        llvm::Value *completed_async = builder.CreateLoad(async_struct, async_slot);
        llvm::Value *completed_payload =
            extract_async_payload(completed_async, completed_type);
        llvm::Value *completed_result =
            coerce_to_result(completed_payload, completed_type);
        if (result_slot && !completed_result)
        {
          completed_result = llvm::Constant::getNullValue(expected);
        }
        if (result_slot && completed_result)
        {
          builder.CreateStore(completed_result, result_slot);
        }
        builder.CreateBr(merge_bb);

        if (failed_bb)
        {
          builder.SetInsertPoint(failed_bb);
          llvm::Value *failed_async = builder.CreateLoad(async_struct, async_slot);
          llvm::Value *failed_payload = extract_async_payload(failed_async, error_type);
          llvm::Value *failed_result = coerce_to_result(failed_payload, error_type);
          if (result_slot && !failed_result)
          {
            failed_result = llvm::Constant::getNullValue(expected);
          }
          if (result_slot && failed_result)
          {
            builder.CreateStore(failed_result, result_slot);
          }
          builder.CreateBr(merge_bb);
        }

        builder.SetInsertPoint(fallback_bb);
        llvm::Value *fallback_async = builder.CreateLoad(async_struct, async_slot);
        llvm::Value *fallback_result = coerce_to_result(fallback_async, s.async_type);
        if (result_slot && !fallback_result)
        {
          fallback_result = llvm::Constant::getNullValue(expected);
        }
        if (result_slot && fallback_result)
        {
          builder.CreateStore(fallback_result, result_slot);
        }
        builder.CreateBr(merge_bb);

        builder.SetInsertPoint(panic_bb);
        if (result_slot)
        {
          builder.CreateStore(llvm::Constant::getNullValue(expected), result_slot);
        }
        builder.CreateBr(merge_bb);

        builder.SetInsertPoint(merge_bb);
        llvm::Value *out = nullptr;
        if (result_slot && expected && !expected->isVoidTy())
        {
          out = builder.CreateLoad(expected, result_slot);
        }
        if (!out)
        {
          out = DefaultFor(s.result);
        }
        emitter.SetTempValue(s.result, out);
      }
      void operator()(const IRRaceReturn &r) const
      {
        if (r.arms.empty())
        {
          emitter.SetTempValue(r.result, DefaultFor(r.result));
          return;
        }

        const LowerCtx *active_ctx = emitter.GetCurrentCtx();
        const analysis::ScopeContext &scope = BuildScope(active_ctx);

        llvm::Type *expected = ExpectedLLVMType(r.result);
        analysis::TypeRef target_type =
            active_ctx ? active_ctx->LookupValueType(r.result) : nullptr;
        if (!expected && target_type)
        {
          expected = emitter.GetLLVMType(target_type);
        }
        if (!expected || expected->isVoidTy())
        {
          emitter.SetTempValue(r.result, DefaultFor(r.result));
          return;
        }

        llvm::Function *func =
            builder.GetInsertBlock() ? builder.GetInsertBlock()->getParent() : nullptr;
        if (!func)
        {
          emitter.SetTempValue(r.result, DefaultFor(r.result));
          return;
        }

        llvm::IRBuilder<> entry_builder(
            &func->getEntryBlock(),
            func->getEntryBlock().begin());

        struct RaceArmEval
        {
          const IRRaceArm *arm = nullptr;
          llvm::Value *async_value = nullptr;
          llvm::StructType *async_struct = nullptr;
          llvm::AllocaInst *async_slot = nullptr;
          analysis::TypeRef async_type = nullptr;
          analysis::TypeRef result_type = nullptr;
          analysis::TypeRef error_type = nullptr;
          AsyncStateDiscs discs{};
        };

        std::vector<RaceArmEval> evaluated;
        evaluated.reserve(r.arms.size());
        for (const IRRaceArm &arm : r.arms)
        {
          emitter.EmitIR(arm.async_ir);
          RaceArmEval entry;
          entry.arm = &arm;
          entry.async_value = EvaluateOrDefault(arm.async_value);
          entry.async_struct = llvm::dyn_cast_or_null<llvm::StructType>(
              entry.async_value ? entry.async_value->getType() : nullptr);
          if (active_ctx)
          {
            entry.async_type = active_ctx->LookupValueType(arm.async_value);
            if (const auto sig = analysis::GetAsyncSig(entry.async_type))
            {
              entry.result_type = sig->result;
              entry.error_type = sig->err;
            }
          }
          if (entry.async_type)
          {
            entry.discs = LoweredAsyncStateDiscs(scope, entry.async_type);
          }
          evaluated.push_back(std::move(entry));
        }

        llvm::AllocaInst *result_slot = entry_builder.CreateAlloca(expected);
        builder.CreateStore(llvm::Constant::getNullValue(expected), result_slot);

        auto materialize_as_type = [&](llvm::Value *value, llvm::Type *dst_ty) -> llvm::Value *
        {
          if (!value || !dst_ty)
          {
            return nullptr;
          }
          if (value->getType() == dst_ty)
          {
            return value;
          }
          if (llvm::Value *coerced = CoerceTo(&builder, value, dst_ty))
          {
            return coerced;
          }

          llvm::AllocaInst *dst_slot = entry_builder.CreateAlloca(dst_ty);
          builder.CreateStore(llvm::Constant::getNullValue(dst_ty), dst_slot);
          llvm::AllocaInst *src_slot = entry_builder.CreateAlloca(value->getType());
          builder.CreateStore(value, src_slot);

          llvm::Type *i8_ty = llvm::Type::getInt8Ty(emitter.GetContext());
          llvm::Type *i64_ty = llvm::Type::getInt64Ty(emitter.GetContext());
          llvm::Value *dst_i8 = builder.CreateBitCast(dst_slot, llvm::PointerType::get(i8_ty, 0));
          llvm::Value *src_i8 = builder.CreateBitCast(src_slot, llvm::PointerType::get(i8_ty, 0));
          const llvm::DataLayout &dl = emitter.GetModule().getDataLayout();
          const std::uint64_t src_size =
              static_cast<std::uint64_t>(dl.getTypeAllocSize(value->getType()));
          const std::uint64_t dst_size =
              static_cast<std::uint64_t>(dl.getTypeAllocSize(dst_ty));
          const std::uint64_t copy_size = std::min(src_size, dst_size);
          if (copy_size > 0)
          {
            builder.CreateMemCpy(
                dst_i8,
                llvm::Align(1),
                src_i8,
                llvm::Align(1),
                llvm::ConstantInt::get(i64_ty, copy_size));
          }
          return builder.CreateLoad(dst_ty, dst_slot);
        };

        auto coerce_to_result = [&](llvm::Value *value,
                                    const analysis::TypeRef &source_type) -> llvm::Value *
        {
          llvm::Value *out = value;
          if (!out)
          {
            return llvm::Constant::getNullValue(expected);
          }
          if (target_type)
          {
            if (llvm::Value *coerced = CoerceToTyped(
                    emitter,
                    &builder,
                    out,
                    expected,
                    source_type,
                    target_type))
            {
              return coerced;
            }
          }
          if (llvm::Value *coerced = CoerceTo(&builder, out, expected))
          {
            return coerced;
          }
          if (llvm::Value *copied = materialize_as_type(out, expected))
          {
            return copied;
          }
          return llvm::Constant::getNullValue(expected);
        };

        auto extract_async_payload = [&](llvm::AllocaInst *async_slot,
                                         llvm::StructType *async_struct,
                                         const analysis::TypeRef &payload_type) -> llvm::Value *
        {
          if (!async_slot || !async_struct || !payload_type ||
              IsUnitTypeRef(payload_type) ||
              IsNeverTypeRef(payload_type))
          {
            return nullptr;
          }
          if (async_struct->getNumElements() < 1 ||
              !async_struct->getElementType(0)->isIntegerTy())
          {
            return nullptr;
          }

          llvm::Type *payload_ll = emitter.GetLLVMType(payload_type);
          if (!payload_ll || payload_ll->isVoidTy())
          {
            return nullptr;
          }

          llvm::Value *payload_i8 = CreateTaggedPayloadI8Ptr(
              emitter,
              &builder,
              async_struct,
              async_slot,
              ::cursive::analysis::layout::kPtrAlign);
          if (!payload_i8)
          {
            return nullptr;
          }

          llvm::Value *payload_ptr = builder.CreateBitCast(
              payload_i8,
              llvm::PointerType::get(payload_ll, 0));
          llvm::LoadInst *loaded = builder.CreateLoad(payload_ll, payload_ptr);
          loaded->setAlignment(llvm::Align(1));
          return loaded;
        };

        for (RaceArmEval &eval : evaluated)
        {
          if (!eval.async_struct ||
              eval.async_struct->getNumElements() < 1 ||
              !eval.async_struct->getElementType(0)->isIntegerTy())
          {
            llvm::Value *out = coerce_to_result(eval.async_value, eval.async_type);
            emitter.SetTempValue(r.result, out ? out : llvm::Constant::getNullValue(expected));
            return;
          }
          eval.async_slot = entry_builder.CreateAlloca(eval.async_struct);
          builder.CreateStore(eval.async_value, eval.async_slot);
        }

        auto emit_completed_arm = [&](const RaceArmEval &arm_eval)
        {
          llvm::Value *match_payload =
              extract_async_payload(arm_eval.async_slot,
                                    arm_eval.async_struct,
                                    arm_eval.result_type);
          if (!match_payload)
          {
            match_payload = DefaultFor(arm_eval.arm->match_value);
          }
          emitter.SetTempValue(arm_eval.arm->match_value, match_payload);
          emitter.EmitIR(arm_eval.arm->handler_ir);

          analysis::TypeRef source_type = r.result_type;
          if (!source_type && active_ctx)
          {
            source_type = active_ctx->LookupValueType(arm_eval.arm->handler_result);
          }
          llvm::Value *handler_out =
              coerce_to_result(EvaluateOrDefault(arm_eval.arm->handler_result), source_type);
          builder.CreateStore(handler_out, result_slot);
        };

        auto emit_failed_arm = [&](const RaceArmEval &arm_eval)
        {
          llvm::Value *error_payload =
              extract_async_payload(arm_eval.async_slot,
                                    arm_eval.async_struct,
                                    arm_eval.error_type);
          llvm::Value *out = coerce_to_result(error_payload, arm_eval.error_type);
          builder.CreateStore(out, result_slot);
        };

        llvm::Type *i8_ty = llvm::Type::getInt8Ty(emitter.GetContext());
        llvm::Type *opaque_ptr_ty = emitter.GetOpaquePtr();
        auto *opaque_ptr_ptr_ty = llvm::cast<llvm::PointerType>(opaque_ptr_ty);

        llvm::Value *panic_ptr = LoadLocalValue(emitter, &builder, std::string(kPanicOutName));
        bool has_panic_ptr = panic_ptr != nullptr;
        if (panic_ptr)
        {
          if (llvm::Value *coerced = CoerceTo(&builder, panic_ptr, opaque_ptr_ty))
          {
            panic_ptr = coerced;
          }
          else if (panic_ptr->getType()->isPointerTy())
          {
            panic_ptr = builder.CreateBitCast(panic_ptr, opaque_ptr_ty);
          }
          else
          {
            panic_ptr = nullptr;
            has_panic_ptr = false;
          }
        }
        if (!panic_ptr)
        {
          panic_ptr = llvm::ConstantPointerNull::get(opaque_ptr_ptr_ty);
        }

        llvm::BasicBlock *loop_bb =
            llvm::BasicBlock::Create(emitter.GetContext(), "race.return.loop", func);
        llvm::BasicBlock *fallback_bb =
            llvm::BasicBlock::Create(emitter.GetContext(), "race.return.fallback", func);
        llvm::BasicBlock *panic_bb =
            llvm::BasicBlock::Create(emitter.GetContext(), "race.return.panic", func);
        llvm::BasicBlock *merge_bb =
            llvm::BasicBlock::Create(emitter.GetContext(), "race.return.merge", func);

        std::vector<llvm::BasicBlock *> completed_check(evaluated.size() + 1, nullptr);
        std::vector<llvm::BasicBlock *> completed_hit(evaluated.size(), nullptr);
        std::vector<llvm::BasicBlock *> failed_check(evaluated.size() + 1, nullptr);
        std::vector<llvm::BasicBlock *> failed_hit(evaluated.size(), nullptr);
        std::vector<llvm::BasicBlock *> resume_check(evaluated.size() + 1, nullptr);
        std::vector<llvm::BasicBlock *> resume_hit(evaluated.size(), nullptr);

        for (std::size_t i = 0; i <= evaluated.size(); ++i)
        {
          completed_check[i] = llvm::BasicBlock::Create(
              emitter.GetContext(),
              "race.return.chk.completed." + std::to_string(i),
              func);
          failed_check[i] = llvm::BasicBlock::Create(
              emitter.GetContext(),
              "race.return.chk.failed." + std::to_string(i),
              func);
          resume_check[i] = llvm::BasicBlock::Create(
              emitter.GetContext(),
              "race.return.chk.resume." + std::to_string(i),
              func);
        }
        for (std::size_t i = 0; i < evaluated.size(); ++i)
        {
          completed_hit[i] = llvm::BasicBlock::Create(
              emitter.GetContext(),
              "race.return.completed." + std::to_string(i),
              func);
          failed_hit[i] = llvm::BasicBlock::Create(
              emitter.GetContext(),
              "race.return.failed." + std::to_string(i),
              func);
          resume_hit[i] = llvm::BasicBlock::Create(
              emitter.GetContext(),
              "race.return.resume." + std::to_string(i),
              func);
        }

        builder.CreateBr(loop_bb);

        builder.SetInsertPoint(loop_bb);
        builder.CreateBr(completed_check[0]);

        for (std::size_t i = 0; i < evaluated.size(); ++i)
        {
          builder.SetInsertPoint(completed_check[i]);
          const RaceArmEval &arm_eval = evaluated[i];
          llvm::Value *current_async =
              builder.CreateLoad(arm_eval.async_struct, arm_eval.async_slot);
          llvm::Value *disc = builder.CreateExtractValue(current_async, {0u});
          llvm::Value *is_completed = EmitTypedEq(
              &builder,
              disc,
              llvm::ConstantInt::get(disc->getType(), arm_eval.discs.completed));
          builder.CreateCondBr(
              AsBool(&builder, is_completed),
              completed_hit[i],
              completed_check[i + 1]);

          builder.SetInsertPoint(completed_hit[i]);
          emit_completed_arm(arm_eval);
          builder.CreateBr(merge_bb);
        }

        builder.SetInsertPoint(completed_check[evaluated.size()]);
        builder.CreateBr(failed_check[0]);

        for (std::size_t i = 0; i < evaluated.size(); ++i)
        {
          builder.SetInsertPoint(failed_check[i]);
          const RaceArmEval &arm_eval = evaluated[i];
          llvm::Value *current_async =
              builder.CreateLoad(arm_eval.async_struct, arm_eval.async_slot);
          llvm::Value *disc = builder.CreateExtractValue(current_async, {0u});
          if (arm_eval.discs.failed.has_value())
          {
            llvm::Value *is_failed = EmitTypedEq(
                &builder,
                disc,
                llvm::ConstantInt::get(disc->getType(), *arm_eval.discs.failed));
            builder.CreateCondBr(
                AsBool(&builder, is_failed),
                failed_hit[i],
                failed_check[i + 1]);
          }
          else
          {
            builder.CreateBr(failed_check[i + 1]);
          }

          builder.SetInsertPoint(failed_hit[i]);
          emit_failed_arm(arm_eval);
          builder.CreateBr(merge_bb);
        }

        builder.SetInsertPoint(failed_check[evaluated.size()]);
        builder.CreateBr(resume_check[0]);

        for (std::size_t i = 0; i < evaluated.size(); ++i)
        {
          builder.SetInsertPoint(resume_check[i]);
          const RaceArmEval &arm_eval = evaluated[i];
          llvm::Value *current_async =
              builder.CreateLoad(arm_eval.async_struct, arm_eval.async_slot);
          llvm::Value *disc = builder.CreateExtractValue(current_async, {0u});
          llvm::Value *is_suspended = EmitTypedEq(
              &builder,
              disc,
              llvm::ConstantInt::get(disc->getType(), arm_eval.discs.suspended));
          builder.CreateCondBr(
              AsBool(&builder, is_suspended),
              resume_hit[i],
              resume_check[i + 1]);

          builder.SetInsertPoint(resume_hit[i]);
          llvm::Value *suspended_ptr = builder.CreateBitCast(arm_eval.async_slot, opaque_ptr_ty);
          llvm::Value *unit_input = llvm::ConstantPointerNull::get(opaque_ptr_ptr_ty);
          llvm::Value *resume_call = EmitAsyncResumeRuntimeCall(
              emitter,
              &builder,
              suspended_ptr,
              unit_input,
              panic_ptr);
          llvm::Value *resumed_async = materialize_as_type(resume_call, arm_eval.async_struct);
          if (!resumed_async)
          {
            resumed_async = llvm::Constant::getNullValue(arm_eval.async_struct);
          }
          builder.CreateStore(resumed_async, arm_eval.async_slot);
          if (has_panic_ptr)
          {
            llvm::Value *panic_i8 = panic_ptr;
            if (panic_i8->getType() != llvm::PointerType::get(i8_ty, 0))
            {
              panic_i8 = builder.CreateBitCast(panic_i8, llvm::PointerType::get(i8_ty, 0));
            }
            llvm::LoadInst *panic_flag = builder.CreateLoad(i8_ty, panic_i8);
            llvm::Value *has_panic = builder.CreateICmpNE(
                panic_flag,
                llvm::ConstantInt::get(i8_ty, 0));
            builder.CreateCondBr(has_panic, panic_bb, loop_bb);
          }
          else
          {
            builder.CreateBr(loop_bb);
          }
        }

        builder.SetInsertPoint(resume_check[evaluated.size()]);
        builder.CreateBr(fallback_bb);

        builder.SetInsertPoint(fallback_bb);
        builder.CreateStore(llvm::Constant::getNullValue(expected), result_slot);
        builder.CreateBr(merge_bb);

        builder.SetInsertPoint(panic_bb);
        builder.CreateStore(llvm::Constant::getNullValue(expected), result_slot);
        builder.CreateBr(merge_bb);

        builder.SetInsertPoint(merge_bb);
        llvm::Value *out = builder.CreateLoad(expected, result_slot);
        if (!out)
        {
          out = llvm::Constant::getNullValue(expected);
        }
        emitter.SetTempValue(r.result, out);
      }
      void operator()(const IRRaceYield &r) const
      {
        if (r.arms.empty())
        {
          emitter.SetTempValue(r.result, DefaultFor(r.result));
          return;
        }

        const LowerCtx *active_ctx = emitter.GetCurrentCtx();
        const analysis::ScopeContext &scope = BuildScope(active_ctx);

        const auto stream_sig = analysis::GetAsyncSig(r.stream_type);
        llvm::Type *expected = ExpectedLLVMType(r.result);
        if (!expected && r.stream_type)
        {
          expected = emitter.GetLLVMType(r.stream_type);
        }
        auto *stream_struct = llvm::dyn_cast_or_null<llvm::StructType>(expected);
        if (!stream_sig || !expected || !stream_struct)
        {
          emitter.SetTempValue(r.result, DefaultFor(r.result));
          return;
        }

        llvm::Function *func =
            builder.GetInsertBlock() ? builder.GetInsertBlock()->getParent() : nullptr;
        if (!func)
        {
          emitter.SetTempValue(r.result, DefaultFor(r.result));
          return;
        }

        llvm::IRBuilder<> entry_builder(
            &func->getEntryBlock(),
            func->getEntryBlock().begin());
        const AsyncStateDiscs stream_discs =
            LoweredAsyncStateDiscs(scope, r.stream_type);
        const std::uint64_t suspended_disc = stream_discs.suspended;

        auto extract_async_payload = [&](llvm::Value *async_value,
                                         llvm::StructType *async_struct,
                                         const analysis::TypeRef &payload_type) -> llvm::Value *
        {
          if (!async_value || !async_struct || !payload_type ||
              IsUnitTypeRef(payload_type) || IsNeverTypeRef(payload_type))
          {
            return nullptr;
          }
          llvm::Type *payload_ll = emitter.GetLLVMType(payload_type);
          if (!payload_ll || payload_ll->isVoidTy())
          {
            return nullptr;
          }
          llvm::AllocaInst *async_slot = entry_builder.CreateAlloca(async_struct);
          builder.CreateStore(async_value, async_slot);
          llvm::Value *payload_i8 = CreateTaggedPayloadI8Ptr(
              emitter,
              &builder,
              async_struct,
              async_slot,
              ::cursive::analysis::layout::kPtrAlign);
          if (!payload_i8)
          {
            return nullptr;
          }
          llvm::Value *payload_ptr = builder.CreateBitCast(
              payload_i8,
              llvm::PointerType::get(payload_ll, 0));
          llvm::LoadInst *loaded = builder.CreateLoad(payload_ll, payload_ptr);
          loaded->setAlignment(llvm::Align(1));
          return loaded;
        };

        auto make_async_complete = [&](llvm::Value *payload_value,
                                       const analysis::TypeRef &payload_type) -> llvm::Value *
        {
          static std::uint64_t temp_index = 0;
          IRValue payload_ir;
          payload_ir.kind = IRValue::Kind::Opaque;
          payload_ir.name = r.result.name + ".race_yield.complete.payload." +
                            std::to_string(temp_index++);
          emitter.SetTempValue(payload_ir, payload_value ? payload_value : DefaultFor(payload_ir));

          IRAsyncComplete complete;
          complete.value = payload_ir;
          complete.result.kind = IRValue::Kind::Opaque;
          complete.result.name = r.result.name + ".race_yield.complete." +
                                 std::to_string(temp_index++);
          complete.async_type = r.stream_type;
          complete.result_type = payload_type;
          (*this)(complete);
          return emitter.EvaluateIRValue(complete.result);
        };

        auto make_async_fail = [&](llvm::Value *payload_value,
                                   const analysis::TypeRef &payload_type) -> llvm::Value *
        {
          static std::uint64_t temp_index = 0;
          IRValue payload_ir;
          payload_ir.kind = IRValue::Kind::Opaque;
          payload_ir.name = r.result.name + ".race_yield.fail.payload." +
                            std::to_string(temp_index++);
          emitter.SetTempValue(payload_ir, payload_value ? payload_value : DefaultFor(payload_ir));

          IRAsyncFail fail;
          fail.value = payload_ir;
          fail.result.kind = IRValue::Kind::Opaque;
          fail.result.name = r.result.name + ".race_yield.fail." +
                             std::to_string(temp_index++);
          fail.async_type = r.stream_type;
          fail.error_type = payload_type;
          (*this)(fail);
          return emitter.EvaluateIRValue(fail.result);
        };

        auto make_async_suspended = [&](llvm::Value *out_value,
                                        const analysis::TypeRef &out_type) -> llvm::Value *
        {
          if (stream_struct->getNumElements() < 1 ||
              !stream_struct->getElementType(0)->isIntegerTy())
          {
            return nullptr;
          }
          llvm::AllocaInst *stream_slot = entry_builder.CreateAlloca(stream_struct);
          builder.CreateStore(llvm::Constant::getNullValue(stream_struct), stream_slot);
          llvm::Type *disc_ty = stream_struct->getElementType(0);
          llvm::Value *disc_ptr = builder.CreateStructGEP(stream_struct, stream_slot, 0);
          builder.CreateStore(
              llvm::ConstantInt::get(disc_ty, suspended_disc),
              disc_ptr);

          if (out_type && !IsUnitTypeRef(out_type) && !IsNeverTypeRef(out_type))
          {
            llvm::Type *out_ll = emitter.GetLLVMType(out_type);
            if (out_ll && !out_ll->isVoidTy())
            {
              llvm::Value *payload = out_value;
              if (!payload)
              {
                payload = llvm::Constant::getNullValue(out_ll);
              }
              else if (payload->getType() != out_ll)
              {
                if (llvm::Value *coerced = CoerceTo(&builder, payload, out_ll))
                {
                  payload = coerced;
                }
                else
                {
                  payload = llvm::Constant::getNullValue(out_ll);
                }
              }

              llvm::Value *payload_i8 = CreateTaggedPayloadI8Ptr(
                  emitter,
                  &builder,
                  stream_struct,
                  stream_slot,
                  ::cursive::analysis::layout::kPtrAlign);
              if (payload_i8)
              {
                llvm::AllocaInst *src_slot = entry_builder.CreateAlloca(out_ll);
                builder.CreateStore(payload, src_slot);

                llvm::Type *i8_ty = llvm::Type::getInt8Ty(emitter.GetContext());
                llvm::Type *i64_ty = llvm::Type::getInt64Ty(emitter.GetContext());
                llvm::Value *src_i8 = builder.CreateBitCast(
                    src_slot,
                    llvm::PointerType::get(i8_ty, 0));
                const llvm::DataLayout &dl = emitter.GetModule().getDataLayout();
                const std::uint64_t copy_size =
                    static_cast<std::uint64_t>(dl.getTypeAllocSize(out_ll));
                if (copy_size > 0)
                {
                  builder.CreateMemCpy(
                      payload_i8,
                      llvm::Align(1),
                      src_i8,
                      llvm::Align(1),
                      llvm::ConstantInt::get(i64_ty, copy_size));
                }
              }
            }
          }

          return builder.CreateLoad(stream_struct, stream_slot);
        };

        struct YieldArmEval
        {
          const IRRaceArm *arm = nullptr;
          llvm::Value *async_value = nullptr;
          llvm::StructType *async_struct = nullptr;
          analysis::TypeRef out_type = nullptr;
          analysis::TypeRef err_type = nullptr;
          AsyncStateDiscs discs{};
        };

        std::vector<YieldArmEval> evaluated;
        evaluated.reserve(r.arms.size());
        for (const IRRaceArm &arm : r.arms)
        {
          emitter.EmitIR(arm.async_ir);
          YieldArmEval eval;
          eval.arm = &arm;
          eval.async_value = EvaluateOrDefault(arm.async_value);
          eval.async_struct = llvm::dyn_cast_or_null<llvm::StructType>(
              eval.async_value ? eval.async_value->getType() : nullptr);
          if (active_ctx)
          {
            if (const auto sig = analysis::GetAsyncSig(active_ctx->LookupValueType(arm.async_value)))
            {
              eval.out_type = sig->out;
              eval.err_type = sig->err;
              eval.discs = LoweredAsyncStateDiscs(scope, *sig);
            }
          }
          evaluated.push_back(std::move(eval));
        }

        std::optional<std::size_t> first_failed;
        std::optional<std::size_t> first_suspended;
        bool all_completed = true;
        for (std::size_t i = 0; i < evaluated.size(); ++i)
        {
          const YieldArmEval &eval = evaluated[i];
          if (!eval.async_struct || eval.async_struct->getNumElements() < 1 ||
              !eval.async_struct->getElementType(0)->isIntegerTy())
          {
            all_completed = false;
            continue;
          }
          llvm::Value *disc = builder.CreateExtractValue(eval.async_value, {0u});
          auto *state = llvm::dyn_cast<llvm::ConstantInt>(disc);
          if (!state)
          {
            all_completed = false;
            continue;
          }
          const std::uint64_t v = state->getZExtValue();
          if (eval.discs.failed.has_value() &&
              v == *eval.discs.failed &&
              !first_failed.has_value())
          {
            first_failed = i;
          }
          if (v == eval.discs.suspended && !first_suspended.has_value())
          {
            first_suspended = i;
          }
          if (v != eval.discs.completed)
          {
            all_completed = false;
          }
        }

        if (first_failed.has_value())
        {
          const YieldArmEval &eval = evaluated[*first_failed];
          llvm::Value *err_payload =
              extract_async_payload(eval.async_value, eval.async_struct, eval.err_type);
          llvm::Value *failed_stream = make_async_fail(err_payload, eval.err_type);
          if (!failed_stream)
          {
            failed_stream = llvm::Constant::getNullValue(expected);
          }
          emitter.SetTempValue(r.result, failed_stream);
          return;
        }

        if (first_suspended.has_value())
        {
          const YieldArmEval &eval = evaluated[*first_suspended];
          llvm::Value *out_payload =
              extract_async_payload(eval.async_value, eval.async_struct, eval.out_type);
          if (!out_payload)
          {
            out_payload = DefaultFor(eval.arm->match_value);
          }
          emitter.SetTempValue(eval.arm->match_value, out_payload);
          emitter.EmitIR(eval.arm->handler_ir);
          llvm::Value *handler_value = EvaluateOrDefault(eval.arm->handler_result);
          llvm::Value *suspended_stream = make_async_suspended(handler_value, stream_sig->out);
          if (!suspended_stream)
          {
            suspended_stream = llvm::Constant::getNullValue(expected);
          }
          emitter.SetTempValue(r.result, suspended_stream);
          return;
        }

        if (all_completed)
        {
          llvm::Value *completed_stream = make_async_complete(nullptr, stream_sig->result);
          if (!completed_stream)
          {
            completed_stream = llvm::Constant::getNullValue(expected);
          }
          emitter.SetTempValue(r.result, completed_stream);
          return;
        }

        emitter.SetTempValue(r.result, llvm::Constant::getNullValue(expected));
      }
      void operator()(const IRAll &all) const
      {
        if (all.async_values.empty())
        {
          emitter.SetTempValue(all.result, DefaultFor(all.result));
          return;
        }

        const LowerCtx *active_ctx = emitter.GetCurrentCtx();
        const analysis::ScopeContext &scope = BuildScope(active_ctx);

        std::vector<analysis::TypeRef> tuple_elem_types;
        if (all.tuple_type)
        {
          if (const auto *tuple = std::get_if<analysis::TypeTuple>(&all.tuple_type->node))
          {
            tuple_elem_types = tuple->elements;
          }
        }

        struct AllEval
        {
          llvm::Value *async_value = nullptr;
          llvm::StructType *async_struct = nullptr;
          llvm::AllocaInst *async_slot = nullptr;
          analysis::TypeRef async_type = nullptr;
          analysis::TypeRef result_type = nullptr;
          analysis::TypeRef error_type = nullptr;
          AsyncStateDiscs discs{};
        };

        std::vector<AllEval> evaluated;
        evaluated.reserve(all.async_values.size());
        for (std::size_t i = 0; i < all.async_values.size(); ++i)
        {
          if (i < all.async_irs.size())
          {
            emitter.EmitIR(all.async_irs[i]);
          }

          AllEval entry;
          entry.async_value = EvaluateOrDefault(all.async_values[i]);
          entry.async_struct = llvm::dyn_cast_or_null<llvm::StructType>(
              entry.async_value ? entry.async_value->getType() : nullptr);
          if (active_ctx)
          {
            entry.async_type = active_ctx->LookupValueType(all.async_values[i]);
            if (const auto sig = analysis::GetAsyncSig(entry.async_type))
            {
              entry.result_type = sig->result;
              entry.error_type = sig->err;
            }
          }
          if (entry.async_type)
          {
            entry.discs = LoweredAsyncStateDiscs(scope, entry.async_type);
          }
          if (!entry.result_type && i < tuple_elem_types.size())
          {
            entry.result_type = tuple_elem_types[i];
          }
          if (!entry.error_type && i < all.error_types.size())
          {
            entry.error_type = all.error_types[i];
          }

          evaluated.push_back(std::move(entry));
        }

        llvm::Type *expected = ExpectedLLVMType(all.result);
        analysis::TypeRef target_type =
            active_ctx ? active_ctx->LookupValueType(all.result) : nullptr;
        if (!expected && target_type)
        {
          expected = emitter.GetLLVMType(target_type);
        }
        if (!expected || expected->isVoidTy())
        {
          emitter.SetTempValue(all.result, DefaultFor(all.result));
          return;
        }

        llvm::Function *func =
            builder.GetInsertBlock() ? builder.GetInsertBlock()->getParent() : nullptr;
        if (!func)
        {
          emitter.SetTempValue(all.result, DefaultFor(all.result));
          return;
        }

        llvm::IRBuilder<> entry_builder(
            &func->getEntryBlock(),
            func->getEntryBlock().begin());
        llvm::AllocaInst *result_slot = entry_builder.CreateAlloca(expected);
        builder.CreateStore(llvm::Constant::getNullValue(expected), result_slot);

        auto materialize_as_type = [&](llvm::Value *value, llvm::Type *dst_ty) -> llvm::Value *
        {
          if (!value || !dst_ty)
          {
            return nullptr;
          }
          if (value->getType() == dst_ty)
          {
            return value;
          }
          if (llvm::Value *coerced = CoerceTo(&builder, value, dst_ty))
          {
            return coerced;
          }

          llvm::AllocaInst *dst_slot = entry_builder.CreateAlloca(dst_ty);
          builder.CreateStore(llvm::Constant::getNullValue(dst_ty), dst_slot);
          llvm::AllocaInst *src_slot = entry_builder.CreateAlloca(value->getType());
          builder.CreateStore(value, src_slot);

          llvm::Type *i8_ty = llvm::Type::getInt8Ty(emitter.GetContext());
          llvm::Type *i64_ty = llvm::Type::getInt64Ty(emitter.GetContext());
          llvm::Value *dst_i8 = builder.CreateBitCast(dst_slot, llvm::PointerType::get(i8_ty, 0));
          llvm::Value *src_i8 = builder.CreateBitCast(src_slot, llvm::PointerType::get(i8_ty, 0));
          const llvm::DataLayout &dl = emitter.GetModule().getDataLayout();
          const std::uint64_t src_size =
              static_cast<std::uint64_t>(dl.getTypeAllocSize(value->getType()));
          const std::uint64_t dst_size =
              static_cast<std::uint64_t>(dl.getTypeAllocSize(dst_ty));
          const std::uint64_t copy_size = std::min(src_size, dst_size);
          if (copy_size > 0)
          {
            builder.CreateMemCpy(
                dst_i8,
                llvm::Align(1),
                src_i8,
                llvm::Align(1),
                llvm::ConstantInt::get(i64_ty, copy_size));
          }
          return builder.CreateLoad(dst_ty, dst_slot);
        };

        auto coerce_to_result = [&](llvm::Value *value,
                                    const analysis::TypeRef &source_type) -> llvm::Value *
        {
          llvm::Value *out = value;
          if (!out)
          {
            return llvm::Constant::getNullValue(expected);
          }
          if (target_type)
          {
            if (llvm::Value *coerced = CoerceToTyped(
                    emitter,
                    &builder,
                    out,
                    expected,
                    source_type,
                    target_type))
            {
              return coerced;
            }
          }
          if (llvm::Value *coerced = CoerceTo(&builder, out, expected))
          {
            return coerced;
          }
          if (llvm::Value *copied = materialize_as_type(out, expected))
          {
            return copied;
          }
          return llvm::Constant::getNullValue(expected);
        };

        auto extract_async_payload = [&](llvm::AllocaInst *async_slot,
                                         llvm::StructType *async_struct,
                                         const analysis::TypeRef &payload_type) -> llvm::Value *
        {
          if (!async_slot || !payload_type ||
              IsUnitTypeRef(payload_type) ||
              IsNeverTypeRef(payload_type))
          {
            return nullptr;
          }
          if (!async_struct || async_struct->getNumElements() < 1 ||
              !async_struct->getElementType(0)->isIntegerTy())
          {
            return nullptr;
          }

          llvm::Type *payload_ll = emitter.GetLLVMType(payload_type);
          if (!payload_ll || payload_ll->isVoidTy())
          {
            return nullptr;
          }

          llvm::Value *payload_i8 = CreateTaggedPayloadI8Ptr(
              emitter,
              &builder,
              async_struct,
              async_slot,
              ::cursive::analysis::layout::kPtrAlign);
          if (!payload_i8)
          {
            return nullptr;
          }

          llvm::Value *payload_ptr = builder.CreateBitCast(
              payload_i8,
              llvm::PointerType::get(payload_ll, 0));
          llvm::LoadInst *loaded = builder.CreateLoad(payload_ll, payload_ptr);
          loaded->setAlignment(llvm::Align(1));
          return loaded;
        };

        for (AllEval &eval : evaluated)
        {
          if (!eval.async_struct ||
              eval.async_struct->getNumElements() < 1 ||
              !eval.async_struct->getElementType(0)->isIntegerTy())
          {
            llvm::Value *out = coerce_to_result(eval.async_value, eval.async_type);
            builder.CreateStore(out, result_slot);
            emitter.SetTempValue(all.result, builder.CreateLoad(expected, result_slot));
            return;
          }
          eval.async_slot = entry_builder.CreateAlloca(eval.async_struct);
          builder.CreateStore(eval.async_value, eval.async_slot);
        }

        llvm::Type *i8_ty = llvm::Type::getInt8Ty(emitter.GetContext());
        llvm::Type *opaque_ptr_ty = emitter.GetOpaquePtr();
        auto *opaque_ptr_ptr_ty = llvm::cast<llvm::PointerType>(opaque_ptr_ty);

        llvm::Value *panic_ptr = LoadLocalValue(emitter, &builder, std::string(kPanicOutName));
        bool has_panic_ptr = panic_ptr != nullptr;
        if (panic_ptr)
        {
          if (llvm::Value *coerced = CoerceTo(&builder, panic_ptr, opaque_ptr_ty))
          {
            panic_ptr = coerced;
          }
          else if (panic_ptr->getType()->isPointerTy())
          {
            panic_ptr = builder.CreateBitCast(panic_ptr, opaque_ptr_ty);
          }
          else
          {
            panic_ptr = nullptr;
            has_panic_ptr = false;
          }
        }
        if (!panic_ptr)
        {
          panic_ptr = llvm::ConstantPointerNull::get(opaque_ptr_ptr_ty);
        }

        llvm::BasicBlock *loop_bb =
            llvm::BasicBlock::Create(emitter.GetContext(), "all.loop", func);
        llvm::BasicBlock *panic_bb =
            llvm::BasicBlock::Create(emitter.GetContext(), "all.panic", func);
        llvm::BasicBlock *fallback_bb =
            llvm::BasicBlock::Create(emitter.GetContext(), "all.fallback", func);
        llvm::BasicBlock *success_bb =
            llvm::BasicBlock::Create(emitter.GetContext(), "all.success", func);
        llvm::BasicBlock *merge_bb =
            llvm::BasicBlock::Create(emitter.GetContext(), "all.merge", func);
        std::vector<llvm::BasicBlock *> failed_check(evaluated.size() + 1, nullptr);
        std::vector<llvm::BasicBlock *> failed_hit(evaluated.size(), nullptr);
        std::vector<llvm::BasicBlock *> complete_check(evaluated.size() + 1, nullptr);
        std::vector<llvm::BasicBlock *> resume_check(evaluated.size() + 1, nullptr);
        std::vector<llvm::BasicBlock *> resume_hit(evaluated.size(), nullptr);

        for (std::size_t i = 0; i <= evaluated.size(); ++i)
        {
          failed_check[i] = llvm::BasicBlock::Create(
              emitter.GetContext(),
              "all.chk.failed." + std::to_string(i),
              func);
          complete_check[i] = llvm::BasicBlock::Create(
              emitter.GetContext(),
              "all.chk.complete." + std::to_string(i),
              func);
          resume_check[i] = llvm::BasicBlock::Create(
              emitter.GetContext(),
              "all.chk.resume." + std::to_string(i),
              func);
        }
        for (std::size_t i = 0; i < evaluated.size(); ++i)
        {
          failed_hit[i] = llvm::BasicBlock::Create(
              emitter.GetContext(),
              "all.failed." + std::to_string(i),
              func);
          resume_hit[i] = llvm::BasicBlock::Create(
              emitter.GetContext(),
              "all.resume." + std::to_string(i),
              func);
        }

        builder.CreateBr(loop_bb);

        builder.SetInsertPoint(loop_bb);
        builder.CreateBr(failed_check[0]);

        for (std::size_t i = 0; i < evaluated.size(); ++i)
        {
          builder.SetInsertPoint(failed_check[i]);
          const AllEval &eval = evaluated[i];
          llvm::Value *current_async = builder.CreateLoad(eval.async_struct, eval.async_slot);
          llvm::Value *disc = builder.CreateExtractValue(current_async, {0u});
          if (eval.discs.failed.has_value())
          {
            llvm::Value *is_failed = EmitTypedEq(
                &builder,
                disc,
                llvm::ConstantInt::get(disc->getType(), *eval.discs.failed));
            builder.CreateCondBr(
                AsBool(&builder, is_failed),
                failed_hit[i],
                failed_check[i + 1]);
          }
          else
          {
            builder.CreateBr(failed_check[i + 1]);
          }

          builder.SetInsertPoint(failed_hit[i]);
          llvm::Value *error_payload =
              extract_async_payload(eval.async_slot, eval.async_struct, eval.error_type);
          llvm::Value *out = coerce_to_result(error_payload, eval.error_type);
          builder.CreateStore(out, result_slot);
          builder.CreateBr(merge_bb);
        }

        builder.SetInsertPoint(failed_check[evaluated.size()]);
        builder.CreateBr(complete_check[0]);

        for (std::size_t i = 0; i < evaluated.size(); ++i)
        {
          builder.SetInsertPoint(complete_check[i]);
          const AllEval &eval = evaluated[i];
          llvm::Value *current_async = builder.CreateLoad(eval.async_struct, eval.async_slot);
          llvm::Value *disc = builder.CreateExtractValue(current_async, {0u});
          llvm::Value *is_completed = EmitTypedEq(
              &builder,
              disc,
              llvm::ConstantInt::get(disc->getType(), eval.discs.completed));
          builder.CreateCondBr(
              AsBool(&builder, is_completed),
              complete_check[i + 1],
              resume_check[0]);
        }

        builder.SetInsertPoint(complete_check[evaluated.size()]);
        builder.CreateBr(success_bb);

        for (std::size_t i = 0; i < evaluated.size(); ++i)
        {
          builder.SetInsertPoint(resume_check[i]);
          const AllEval &eval = evaluated[i];
          llvm::Value *current_async = builder.CreateLoad(eval.async_struct, eval.async_slot);
          llvm::Value *disc = builder.CreateExtractValue(current_async, {0u});
          llvm::Value *is_suspended = EmitTypedEq(
              &builder,
              disc,
              llvm::ConstantInt::get(disc->getType(), eval.discs.suspended));
          builder.CreateCondBr(
              AsBool(&builder, is_suspended),
              resume_hit[i],
              resume_check[i + 1]);

          builder.SetInsertPoint(resume_hit[i]);
          llvm::Value *suspended_ptr = builder.CreateBitCast(eval.async_slot, opaque_ptr_ty);
          llvm::Value *unit_input = llvm::ConstantPointerNull::get(opaque_ptr_ptr_ty);
          llvm::Value *resume_call = EmitAsyncResumeRuntimeCall(
              emitter,
              &builder,
              suspended_ptr,
              unit_input,
              panic_ptr);
          llvm::Value *resumed_async = materialize_as_type(resume_call, eval.async_struct);
          if (!resumed_async)
          {
            resumed_async = llvm::Constant::getNullValue(eval.async_struct);
          }
          builder.CreateStore(resumed_async, eval.async_slot);
          if (has_panic_ptr)
          {
            llvm::Value *panic_i8 = panic_ptr;
            if (panic_i8->getType() != llvm::PointerType::get(i8_ty, 0))
            {
              panic_i8 = builder.CreateBitCast(panic_i8, llvm::PointerType::get(i8_ty, 0));
            }
            llvm::LoadInst *panic_flag = builder.CreateLoad(i8_ty, panic_i8);
            llvm::Value *has_panic = builder.CreateICmpNE(
                panic_flag,
                llvm::ConstantInt::get(i8_ty, 0));
            builder.CreateCondBr(has_panic, panic_bb, loop_bb);
          }
          else
          {
            builder.CreateBr(loop_bb);
          }
        }

        builder.SetInsertPoint(resume_check[evaluated.size()]);
        builder.CreateBr(fallback_bb);

        builder.SetInsertPoint(success_bb);
        llvm::Value *tuple_value = nullptr;
        llvm::Type *tuple_llvm_ty = all.tuple_type ? emitter.GetLLVMType(all.tuple_type) : nullptr;
        if (tuple_llvm_ty && (tuple_llvm_ty->isStructTy() || tuple_llvm_ty->isArrayTy()))
        {
          llvm::Value *agg = llvm::Constant::getNullValue(tuple_llvm_ty);
          for (std::size_t i = 0; i < evaluated.size(); ++i)
          {
            llvm::Type *elem_ty = nullptr;
            if (auto *arr_ty = llvm::dyn_cast<llvm::ArrayType>(tuple_llvm_ty))
            {
              elem_ty = arr_ty->getElementType();
            }
            else if (auto *struct_ty = llvm::dyn_cast<llvm::StructType>(tuple_llvm_ty))
            {
              if (i < struct_ty->getNumElements())
              {
                elem_ty = struct_ty->getElementType(static_cast<unsigned>(i));
              }
            }
            if (!elem_ty || elem_ty->isVoidTy())
            {
              continue;
            }

            const AllEval &eval = evaluated[i];
            llvm::Value *completed_payload =
                extract_async_payload(eval.async_slot, eval.async_struct, eval.result_type);
            completed_payload = materialize_as_type(completed_payload, elem_ty);
            if (!completed_payload)
            {
              completed_payload = llvm::Constant::getNullValue(elem_ty);
            }
            agg = builder.CreateInsertValue(agg, completed_payload, {static_cast<unsigned>(i)});
          }
          tuple_value = agg;
        }

        llvm::Value *success_out = coerce_to_result(tuple_value, all.tuple_type);
        builder.CreateStore(success_out, result_slot);
        builder.CreateBr(merge_bb);

        builder.SetInsertPoint(panic_bb);
        builder.CreateStore(llvm::Constant::getNullValue(expected), result_slot);
        builder.CreateBr(merge_bb);

        builder.SetInsertPoint(fallback_bb);
        builder.CreateStore(llvm::Constant::getNullValue(expected), result_slot);
        builder.CreateBr(merge_bb);

        builder.SetInsertPoint(merge_bb);
        llvm::Value *out = builder.CreateLoad(expected, result_slot);
        if (!out)
        {
          out = llvm::Constant::getNullValue(expected);
        }
        emitter.SetTempValue(all.result, out);
      }
      void operator()(const IRAsyncComplete &async_complete) const
      {
        llvm::Value *wrapped_value = EvaluateOrDefault(async_complete.value);
        const LowerCtx *active_ctx = emitter.GetCurrentCtx();
        const analysis::ScopeContext &scope = BuildScope(active_ctx);
        analysis::TypeRef async_type = async_complete.async_type;
        if (!async_type && active_ctx)
        {
          async_type = active_ctx->LookupValueType(async_complete.result);
        }
        analysis::TypeRef source_type =
            active_ctx ? active_ctx->LookupValueType(async_complete.value) : nullptr;
        analysis::TypeRef completed_type = async_complete.result_type;
        if (!completed_type)
        {
          if (const auto sig = analysis::GetAsyncSig(async_type))
          {
            completed_type = sig->result;
          }
        }

        llvm::Type *async_layout_ty = async_type ? emitter.GetLLVMType(async_type) : nullptr;
        llvm::Type *expected_async_ty = ExpectedLLVMType(async_complete.result);
        if (!expected_async_ty)
        {
          expected_async_ty = async_layout_ty;
        }

        llvm::Type *pack_target_ty = async_layout_ty ? async_layout_ty : expected_async_ty;
        if (auto *async_struct = llvm::dyn_cast_or_null<llvm::StructType>(pack_target_ty);
            async_struct && async_struct->getNumElements() >= 1 &&
            async_struct->getElementType(0)->isIntegerTy())
        {
          llvm::Function *current_fn =
              builder.GetInsertBlock() ? builder.GetInsertBlock()->getParent() : nullptr;
          if (current_fn)
          {
            llvm::IRBuilder<> entry_builder(
                &current_fn->getEntryBlock(),
                current_fn->getEntryBlock().begin());
            llvm::AllocaInst *async_slot = entry_builder.CreateAlloca(async_struct);
            builder.CreateStore(llvm::Constant::getNullValue(async_struct), async_slot);

            llvm::Type *disc_ty = async_struct->getElementType(0);
            const AsyncStateDiscs async_discs =
                LoweredAsyncStateDiscs(scope, async_type);
            const std::uint64_t completed_disc = async_discs.completed;
            llvm::Value *disc_ptr = builder.CreateStructGEP(async_struct, async_slot, 0);
            llvm::Value *disc_val = llvm::ConstantInt::get(disc_ty, completed_disc);
            builder.CreateStore(disc_val, disc_ptr);

            if (completed_type &&
                !IsUnitTypeRef(completed_type) &&
                !IsNeverTypeRef(completed_type))
            {
              llvm::Type *completed_ll = emitter.GetLLVMType(completed_type);
              if (completed_ll && !completed_ll->isVoidTy())
              {
                llvm::Value *payload_value = wrapped_value;
                if (!payload_value)
                {
                  payload_value = llvm::Constant::getNullValue(completed_ll);
                }
                else if (payload_value->getType() != completed_ll)
                {
                  if (llvm::Value *coerced = CoerceToTyped(
                          emitter,
                          &builder,
                          payload_value,
                          completed_ll,
                          source_type,
                          completed_type))
                  {
                    payload_value = coerced;
                  }
                  else if (llvm::Value *coerced_plain =
                               CoerceTo(&builder, payload_value, completed_ll))
                  {
                    payload_value = coerced_plain;
                  }
                  else
                  {
                    payload_value = llvm::Constant::getNullValue(completed_ll);
                  }
                }

                llvm::Value *payload_i8 = CreateTaggedPayloadI8Ptr(
                    emitter,
                    &builder,
                    async_struct,
                    async_slot,
                    ::cursive::analysis::layout::kPtrAlign);
                if (payload_i8)
                {
                  llvm::AllocaInst *src_slot =
                      entry_builder.CreateAlloca(payload_value->getType());
                  builder.CreateStore(payload_value, src_slot);

                  llvm::Type *i8_ty = llvm::Type::getInt8Ty(emitter.GetContext());
                  llvm::Type *i64_ty = llvm::Type::getInt64Ty(emitter.GetContext());
                  llvm::Value *src_i8 = builder.CreateBitCast(
                      src_slot,
                      llvm::PointerType::get(i8_ty, 0));
                  const llvm::DataLayout &dl = emitter.GetModule().getDataLayout();
                  const std::uint64_t copy_size = static_cast<std::uint64_t>(
                      dl.getTypeAllocSize(payload_value->getType()));
                  if (copy_size > 0)
                  {
                    builder.CreateMemCpy(
                        payload_i8,
                        llvm::Align(1),
                        src_i8,
                        llvm::Align(1),
                        llvm::ConstantInt::get(i64_ty, copy_size));
                  }
                }
              }
            }

            llvm::Value *packed = builder.CreateLoad(async_struct, async_slot);
            if (expected_async_ty && packed->getType() != expected_async_ty)
            {
              if (llvm::Value *coerced = CoerceTo(&builder, packed, expected_async_ty))
              {
                packed = coerced;
              }
            }
            emitter.SetTempValue(async_complete.result, packed);
            return;
          }
        }

        if (expected_async_ty)
        {
          if (llvm::Value *coerced = CoerceTo(&builder, wrapped_value, expected_async_ty))
          {
            wrapped_value = coerced;
          }
        }
        if (!wrapped_value)
        {
          wrapped_value = DefaultFor(async_complete.result);
        }
        emitter.SetTempValue(async_complete.result, wrapped_value);
      }

      void operator()(const IRAsyncFail &async_fail) const
      {
        llvm::Value *wrapped_value = EvaluateOrDefault(async_fail.value);
        const LowerCtx *active_ctx = emitter.GetCurrentCtx();
        const analysis::ScopeContext &scope = BuildScope(active_ctx);
        analysis::TypeRef async_type = async_fail.async_type;
        if (!async_type && active_ctx)
        {
          async_type = active_ctx->LookupValueType(async_fail.result);
        }
        analysis::TypeRef source_type =
            active_ctx ? active_ctx->LookupValueType(async_fail.value) : nullptr;
        analysis::TypeRef error_type = async_fail.error_type;
        if (!error_type)
        {
          if (const auto sig = analysis::GetAsyncSig(async_type))
          {
            error_type = sig->err;
          }
        }

        llvm::Type *async_layout_ty = async_type ? emitter.GetLLVMType(async_type) : nullptr;
        llvm::Type *expected_async_ty = ExpectedLLVMType(async_fail.result);
        if (!expected_async_ty)
        {
          expected_async_ty = async_layout_ty;
        }

        llvm::Type *pack_target_ty = async_layout_ty ? async_layout_ty : expected_async_ty;
        if (auto *async_struct = llvm::dyn_cast_or_null<llvm::StructType>(pack_target_ty);
            async_struct && async_struct->getNumElements() >= 1 &&
            async_struct->getElementType(0)->isIntegerTy())
        {
          llvm::Function *current_fn =
              builder.GetInsertBlock() ? builder.GetInsertBlock()->getParent() : nullptr;
          if (current_fn)
          {
            llvm::IRBuilder<> entry_builder(
                &current_fn->getEntryBlock(),
                current_fn->getEntryBlock().begin());
            llvm::AllocaInst *async_slot = entry_builder.CreateAlloca(async_struct);
            builder.CreateStore(llvm::Constant::getNullValue(async_struct), async_slot);

            llvm::Type *disc_ty = async_struct->getElementType(0);
            const auto lowered_async = ::cursive::analysis::layout::LowerAsyncType(async_type);
            const AsyncStateDiscs async_discs =
                LoweredAsyncStateDiscs(scope, lowered_async);
            if (!async_discs.failed.has_value())
            {
              // Infallible asyncs have no concrete Failed arm. This path should
              // already be unreachable after typing, so preserve the zero
              // initialized value instead of fabricating a failed discriminator.
              emitter.SetTempValue(async_fail.result,
                                   llvm::Constant::getNullValue(async_struct));
              return;
            }
            const std::uint64_t failed_disc = *async_discs.failed;
            llvm::Value *disc_ptr = builder.CreateStructGEP(async_struct, async_slot, 0);
            llvm::Value *disc_val = llvm::ConstantInt::get(disc_ty, failed_disc);
            builder.CreateStore(disc_val, disc_ptr);

            if (error_type &&
                !IsUnitTypeRef(error_type) &&
                !IsNeverTypeRef(error_type))
            {
              llvm::Type *error_ll = emitter.GetLLVMType(error_type);
              if (error_ll && !error_ll->isVoidTy())
              {
                llvm::Value *payload_value = wrapped_value;
                if (!payload_value)
                {
                  payload_value = llvm::Constant::getNullValue(error_ll);
                }
                else if (payload_value->getType() != error_ll)
                {
                  if (llvm::Value *coerced = CoerceToTyped(
                          emitter,
                          &builder,
                          payload_value,
                          error_ll,
                          source_type,
                          error_type))
                  {
                    payload_value = coerced;
                  }
                  else if (llvm::Value *coerced_plain =
                               CoerceTo(&builder, payload_value, error_ll))
                  {
                    payload_value = coerced_plain;
                  }
                  else
                  {
                    payload_value = llvm::Constant::getNullValue(error_ll);
                  }
                }

                llvm::Value *payload_i8 = CreateTaggedPayloadI8Ptr(
                    emitter,
                    &builder,
                    async_struct,
                    async_slot,
                    ::cursive::analysis::layout::kPtrAlign);
                if (payload_i8)
                {
                  llvm::AllocaInst *src_slot =
                      entry_builder.CreateAlloca(payload_value->getType());
                  builder.CreateStore(payload_value, src_slot);

                  llvm::Type *i8_ty = llvm::Type::getInt8Ty(emitter.GetContext());
                  llvm::Type *i64_ty = llvm::Type::getInt64Ty(emitter.GetContext());
                  llvm::Value *src_i8 = builder.CreateBitCast(
                      src_slot,
                      llvm::PointerType::get(i8_ty, 0));
                  const llvm::DataLayout &dl = emitter.GetModule().getDataLayout();
                  const std::uint64_t copy_size = static_cast<std::uint64_t>(
                      dl.getTypeAllocSize(payload_value->getType()));
                  if (copy_size > 0)
                  {
                    builder.CreateMemCpy(
                        payload_i8,
                        llvm::Align(1),
                        src_i8,
                        llvm::Align(1),
                        llvm::ConstantInt::get(i64_ty, copy_size));
                  }
                }
              }
            }

            llvm::Value *packed = builder.CreateLoad(async_struct, async_slot);
            if (expected_async_ty && packed->getType() != expected_async_ty)
            {
              if (llvm::Value *coerced = CoerceTo(&builder, packed, expected_async_ty))
              {
                packed = coerced;
              }
            }
            emitter.SetTempValue(async_fail.result, packed);
            return;
          }
        }

        if (expected_async_ty)
        {
          if (llvm::Value *coerced = CoerceTo(&builder, wrapped_value, expected_async_ty))
          {
            wrapped_value = coerced;
          }
        }
        if (!wrapped_value)
        {
          wrapped_value = DefaultFor(async_fail.result);
        }
        emitter.SetTempValue(async_fail.result, wrapped_value);
      }
      void operator()(const IRCallVTable &call) const
      {
        const bool debug_vtable_call = core::IsDebugEnabled("obj");
        // Evaluate the dense pointer (base): {data_ptr, vtable_ptr}
        llvm::Value *dense_ptr = EvaluateOrDefault(call.base);
        if (!dense_ptr)
        {
          if (debug_vtable_call)
          {
            std::fprintf(stderr, "[vtable-call] dense_ptr is null for slot=%zu\n",
                         call.slot);
          }
          emitter.SetTempValue(call.result, DefaultFor(call.result));
          return;
        }

        // Debug: print the type of the evaluated dense pointer
        if (debug_vtable_call)
        {
          std::string type_str;
          llvm::raw_string_ostream os(type_str);
          dense_ptr->getType()->print(os);
          os.flush();
          std::fprintf(stderr, "[vtable-call] slot=%zu base_kind=%d base_name=%s dense_ptr_type=%s\n",
                       call.slot,
                       static_cast<int>(call.base.kind),
                       call.base.name.c_str(),
                       type_str.c_str());
        }

        // The dense pointer should be a struct {ptr, ptr}.  If instead we got a
        // pointer (e.g. ByRef parameter), load through it first.
        llvm::Type *ptr_ty = emitter.GetOpaquePtr();
        if (dense_ptr->getType()->isPointerTy())
        {
          llvm::StructType *dyn_ty = GetDynamicType(emitter.GetContext());
          dense_ptr = builder.CreateLoad(dyn_ty, dense_ptr);
          if (debug_vtable_call)
          {
            std::fprintf(stderr,
                         "[vtable-call]   loaded dense_ptr through pointer\n");
          }
        }

        auto *dense_struct_ty = llvm::dyn_cast<llvm::StructType>(dense_ptr->getType());
        if (!dense_struct_ty || dense_struct_ty->getNumElements() < 2)
        {
          if (debug_vtable_call)
          {
            std::fprintf(
                stderr,
                "[vtable-call]   FAIL: dense_ptr is not a struct with >=2 elements\n");
          }
          emitter.SetTempValue(call.result, DefaultFor(call.result));
          return;
        }

        llvm::Value *data_ptr = builder.CreateExtractValue(dense_ptr, {0});
        llvm::Value *vtable_ptr = builder.CreateExtractValue(dense_ptr, {1});

        // Spec vtables carry a 3-word header:
        // [sizeof(T), alignof(T), DropGlueSym(T)] ++ method slots.
        // Dynamic dispatch therefore indexes method slot i at header_offset+i.
        llvm::Type *i64_ty = llvm::Type::getInt64Ty(emitter.GetContext());
        const std::size_t vtable_slot_index = call.slot + 3;
        llvm::Value *slot_ptr = builder.CreateGEP(
            ptr_ty,
            vtable_ptr,
            llvm::ConstantInt::get(i64_ty, vtable_slot_index));
        llvm::Value *fn_ptr = builder.CreateLoad(ptr_ty, slot_ptr);

        // Build call arguments: (data_ptr, ...user_args_including_panic_out)
        // Note: call.args already includes the panic out-parameter appended by
        // method_call.cpp, so we must NOT add it again here.
        std::vector<llvm::Value *> call_args;
        call_args.push_back(data_ptr);
        for (const auto &arg : call.args)
        {
          call_args.push_back(EvaluateOrDefault(arg));
        }

        if (debug_vtable_call)
        {
          std::fprintf(stderr, "[vtable-call]   call_args count=%zu\n",
                       call_args.size());
        }

        // Build the function type: (ptr, arg_types...) -> ret_ty
        // Use the return type from the IRCallVTable (populated by LowerDynCall),
        // fall back to LookupValueType, then try the vtable global entries.
        llvm::Type *ret_ty = nullptr;
        if (call.ret_type)
        {
          ret_ty = emitter.GetLLVMType(call.ret_type);
        }
        if (!ret_ty)
        {
          const LowerCtx *active_ctx = emitter.GetCurrentCtx();
          if (active_ctx)
          {
            if (analysis::TypeRef result_type = active_ctx->LookupValueType(call.result))
            {
              ret_ty = emitter.GetLLVMType(result_type);
            }
          }
        }
        if (!ret_ty)
        {
          // Last resort: try to find the actual function at this vtable slot
          // by examining the vtable global's initializer.
          auto try_vtable_global = [&]() -> llvm::Type *
          {
            // The base is a DynLit whose DerivedValueInfo contains the vtable symbol.
            const LowerCtx *vtable_ctx = emitter.GetCurrentCtx();
            if (!vtable_ctx)
            {
              return nullptr;
            }
            const DerivedValueInfo *derived = vtable_ctx->LookupDerivedValue(call.base);
            if (!derived || derived->vtable_sym.empty())
            {
              return nullptr;
            }
            auto *gv = emitter.GetModule().getNamedGlobal(derived->vtable_sym);
            if (!gv || !gv->hasInitializer())
            {
              return nullptr;
            }
            auto *init = gv->getInitializer();
            auto *arr = llvm::dyn_cast<llvm::ConstantArray>(init);
            llvm::Value *slot_val = nullptr;
            const std::size_t vtable_slot_index = call.slot + 3;
            if (auto *st = llvm::dyn_cast<llvm::ConstantStruct>(init))
            {
              if (vtable_slot_index >= st->getNumOperands())
              {
                return nullptr;
              }
              slot_val = st->getOperand(static_cast<unsigned>(vtable_slot_index));
            }
            else if (auto *arr = llvm::dyn_cast<llvm::ConstantArray>(init))
            {
              if (vtable_slot_index >= arr->getNumOperands())
              {
                return nullptr;
              }
              slot_val = arr->getOperand(static_cast<unsigned>(vtable_slot_index));
            }
            if (!slot_val)
            {
              return nullptr;
            }
            slot_val = slot_val->stripPointerCasts();
            if (auto *fn = llvm::dyn_cast<llvm::Function>(slot_val))
            {
              return fn->getReturnType();
            }
            return nullptr;
          };
          ret_ty = try_vtable_global();
        }
        if (!ret_ty)
        {
          if (debug_vtable_call)
          {
            std::fprintf(
                stderr,
                "[vtable-call]   FAIL: unresolved vtable return type\n");
          }
          if (const LowerCtx *active_ctx = emitter.GetCurrentCtx())
          {
            const_cast<LowerCtx *>(active_ctx)->ReportCodegenFailure();
          }
          emitter.SetTempValue(call.result, DefaultFor(call.result));
          return;
        }
        if (debug_vtable_call)
        {
          std::string rty_str;
          llvm::raw_string_ostream os(rty_str);
          ret_ty->print(os);
          os.flush();
          std::fprintf(stderr, "[vtable-call]   ret_ty=%s has_ir_ret=%d\n",
                       rty_str.c_str(), call.ret_type ? 1 : 0);
        }

        std::vector<llvm::Type *> param_tys;
        param_tys.reserve(call_args.size());
        for (llvm::Value *arg : call_args)
        {
          param_tys.push_back(arg ? arg->getType() : ptr_ty);
        }
        llvm::FunctionType *fn_ty = llvm::FunctionType::get(ret_ty, param_tys, false);

        llvm::CallInst *result = builder.CreateCall(fn_ty, fn_ptr, call_args);
        if (result && !result->getType()->isVoidTy())
        {
          emitter.SetTempValue(call.result, result);
        }
        else
        {
          emitter.SetTempValue(call.result, DefaultFor(call.result));
        }
      }
      void operator()(const IRStoreGlobal &store) const
      {
        std::string symbol = store.symbol;
        if (auto alias = emitter.LookupSymbolAlias(symbol))
        {
          symbol = *alias;
        }

        const LowerCtx *active_ctx = emitter.GetCurrentCtx();
        analysis::TypeRef target_type =
            active_ctx ? active_ctx->LookupStaticType(symbol) : nullptr;
        llvm::GlobalVariable *global_var = nullptr;
        if (llvm::Value *global_value = emitter.GetGlobal(symbol))
        {
          global_var = llvm::dyn_cast<llvm::GlobalVariable>(global_value);
        }
        if (!global_var)
        {
          global_var = emitter.GetModule().getNamedGlobal(symbol);
        }
        const bool has_hosted_state_slot = emitter.HasHostedStateSlot(symbol);
        if (global_var && global_var->isConstant() && !has_hosted_state_slot)
        {
          return;
        }

        llvm::Value *value = EvaluateOrDefault(store.value);
        llvm::Value *target_ptr = nullptr;
        llvm::Type *target_ty = nullptr;
        analysis::TypeRef source_type =
            active_ctx ? active_ctx->LookupValueType(store.value) : nullptr;
        if (!source_type && store.value.kind == IRValue::Kind::Local)
        {
          source_type = emitter.LookupLocalType(store.value.name);
        }

        if (target_type)
        {
          if (llvm::Type *typed_target_ty = emitter.GetLLVMType(target_type))
          {
            target_ty = typed_target_ty;
            target_ptr =
                emitter.GetHostedStatePtr(symbol, typed_target_ty, global_var);
            if (!target_ptr && emitter.HasHostedStateSlot(symbol) && !global_var)
            {
              return;
            }
            if (!target_ptr && global_var)
            {
              target_ptr = builder.CreateBitCast(
                  global_var, llvm::PointerType::get(typed_target_ty, 0));
            }
            llvm::Value *coerced = CoerceToTyped(
                emitter,
                &builder,
                value,
                typed_target_ty,
                source_type,
                target_type);
            if (coerced)
            {
              value = coerced;
            }
          }
        }

        if (!target_ptr)
        {
          if (!global_var)
          {
            return;
          }

          target_ptr = global_var;
          target_ty = global_var->getValueType();
        }

        if (value && value->getType() != target_ty)
        {
          if (llvm::Value *coerced = CoerceTo(&builder, value, target_ty))
          {
            value = coerced;
          }
        }

        if (!value)
        {
          value = llvm::Constant::getNullValue(target_ty);
        }

        llvm::StoreInst *stored = builder.CreateStore(value, target_ptr);
        stored->setAlignment(global_var ? global_var->getAlign().valueOrOne()
                                        : llvm::Align(1));
      }
    };

    std::visit(Visitor{*this, *builder}, ir->node);
    if (ir_perf_enabled && g_ir_proc_perf_ctx && !g_ir_proc_perf_ctx->stack.empty())
    {
      const auto end = Clock::now();
      const IRNodePerfFrame frame = g_ir_proc_perf_ctx->stack.back();
      g_ir_proc_perf_ctx->stack.pop_back();

      const long long elapsed_ms = ElapsedMs(frame.start, end);
      long long self_ms = elapsed_ms - frame.child_ms;
      if (self_ms < 0)
      {
        self_ms = 0;
      }

      if (frame.kind_index < g_ir_proc_perf_ctx->buckets.size())
      {
        auto &bucket = g_ir_proc_perf_ctx->buckets[frame.kind_index];
        bucket.count += 1;
        bucket.total_self_ms += self_ms;
        if (self_ms > bucket.max_self_ms)
        {
          bucket.max_self_ms = self_ms;
        }
      }

      if (!g_ir_proc_perf_ctx->stack.empty())
      {
        g_ir_proc_perf_ctx->stack.back().child_ms += elapsed_ms;
      }
    }
  }

  llvm::Value *LLVMEmitter::GetAddressableStorage(const IRValue &value)
  {
    auto *builder = static_cast<llvm::IRBuilder<> *>(builder_.get());
    if (!builder)
    {
      return nullptr;
    }

    switch (value.kind)
    {
    case IRValue::Kind::Local:
    {
      llvm::Value *local = GetLocalBindStorage(value.name);
      return (local && local->getType()->isPointerTy()) ? local : nullptr;
    }
    case IRValue::Kind::Symbol:
    {
      if (llvm::Value *global = GetGlobal(value.name))
      {
        return global->getType()->isPointerTy() ? global : nullptr;
      }
      if (llvm::GlobalVariable *global = module_->getNamedGlobal(value.name))
      {
        return global;
      }
      return nullptr;
    }
    case IRValue::Kind::Opaque:
    {
      if (llvm::Value *storage = GetTempStorage(value))
      {
        return storage;
      }
      const LowerCtx *ctx = GetCurrentCtx();
      if (!ctx)
      {
        return nullptr;
      }
      const DerivedValueInfo *derived = ctx->LookupDerivedValue(value);
      if (!derived)
      {
        return nullptr;
      }
      switch (derived->kind)
      {
      case DerivedValueInfo::Kind::AddrLocal:
      case DerivedValueInfo::Kind::AddrStatic:
      case DerivedValueInfo::Kind::AddrField:
      case DerivedValueInfo::Kind::AddrTuple:
      case DerivedValueInfo::Kind::AddrIndex:
      case DerivedValueInfo::Kind::AddrDeref:
      {
        llvm::Value *addr = EvaluateIRValue(value);
        return (addr && addr->getType()->isPointerTy()) ? addr : nullptr;
      }
      case DerivedValueInfo::Kind::LoadFromAddr:
        return GetAddressableStorage(derived->base);
      default:
        return nullptr;
      }
    }
    default:
      return nullptr;
    }
  }

  LLVMEmitter::FlowStateSnapshot LLVMEmitter::SaveFlowState() const
  {
    FlowStateSnapshot snapshot;
    snapshot.locals = locals_;
    snapshot.local_home_storage = local_home_storage_;
    snapshot.local_types = local_types_;
    snapshot.values = values_;
    snapshot.storage_values = storage_values_;
    snapshot.preferred_result_storage = preferred_result_storage_;
    snapshot.reusable_aggregate_storage = reusable_aggregate_storage_;
    return snapshot;
  }

  void LLVMEmitter::RestoreFlowState(const FlowStateSnapshot &snapshot)
  {
    const auto persistent_home_storage = local_home_storage_;
    const auto persistent_local_types = local_types_;
    locals_ = snapshot.locals;
    local_home_storage_ = snapshot.local_home_storage;
    local_types_ = snapshot.local_types;
    values_ = snapshot.values;
    storage_values_ = snapshot.storage_values;
    preferred_result_storage_ = snapshot.preferred_result_storage;
    reusable_aggregate_storage_ = snapshot.reusable_aggregate_storage;
    for (const auto &[name, storage] : persistent_home_storage)
    {
      if (storage && !local_home_storage_.contains(name))
      {
        local_home_storage_[name] = storage;
      }
    }
    for (const auto &[name, type] : persistent_local_types)
    {
      if (type && !local_types_.contains(name))
      {
        local_types_[name] = type;
      }
    }
  }

  llvm::AllocaInst *LLVMEmitter::AcquireReusableAggregateStorage(
      llvm::Function *func,
      llvm::Type *ty,
      std::string_view name)
  {
    if (!func || !ty) {
      return nullptr;
    }
    if (!ty->isStructTy() && !ty->isArrayTy()) {
      return nullptr;
    }

    auto func_it = reusable_aggregate_storage_.find(func);
    if (func_it != reusable_aggregate_storage_.end()) {
      auto type_it = func_it->second.find(ty);
      if (type_it != func_it->second.end() && !type_it->second.empty()) {
        llvm::AllocaInst *slot = type_it->second.back();
        type_it->second.pop_back();
        return slot;
      }
    }

    return CreateEntryAlloca(func, ty, std::string(name));
  }

  void LLVMEmitter::ReleaseReusableAggregateStorage(llvm::Value *storage)
  {
    auto *alloca = llvm::dyn_cast_or_null<llvm::AllocaInst>(storage);
    if (!alloca) {
      return;
    }
    llvm::Type *ty = alloca->getAllocatedType();
    if (!ty || (!ty->isStructTy() && !ty->isArrayTy())) {
      return;
    }
    llvm::Function *func = alloca->getFunction();
    if (!func) {
      return;
    }
    reusable_aggregate_storage_[func][ty].push_back(alloca);
  }

  void LLVMEmitter::ForgetTempStorage(const IRValue &value)
  {
    if (value.kind != IRValue::Kind::Opaque) {
      return;
    }
    storage_values_.erase(value.name);
    values_.erase(value.name);
  }

  void LLVMEmitter::ReleaseTempStorage(const IRValue &value)
  {
    if (value.kind != IRValue::Kind::Opaque) {
      return;
    }
    auto it = storage_values_.find(value.name);
    if (it == storage_values_.end()) {
      return;
    }
    ReleaseReusableAggregateStorage(it->second);
    storage_values_.erase(it);
    values_.erase(value.name);
  }

  void LLVMEmitter::ReleaseMoveConsumedStorage(const IRValue &value)
  {
    if (value.kind == IRValue::Kind::Opaque)
    {
      ReleaseTempStorage(value);
      return;
    }

    if (value.kind != IRValue::Kind::Local)
    {
      return;
    }

    auto local_it = locals_.find(value.name);
    if (local_it == locals_.end())
    {
      return;
    }

    llvm::Value *storage = local_it->second;
    auto *alloca = llvm::dyn_cast_or_null<llvm::AllocaInst>(storage);
    if (!alloca)
    {
      return;
    }

    llvm::Type *ty = alloca->getAllocatedType();
    if (!ty || (!ty->isStructTy() && !ty->isArrayTy()))
    {
      return;
    }

    if (GetLocalHomeStorage(value.name) != storage)
    {
      SetLocalHomeStorage(value.name, storage);
    }
    locals_.erase(local_it);
  }

  void LLVMEmitter::RegisterLocalBindStorage(const std::string &name, llvm::Value *val)
  {
    SetLocal(name, val);
    if (val && val->getType()->isPointerTy())
    {
      SetLocalHomeStorage(name, val);
    }
  }

  llvm::Value *LLVMEmitter::GetLocalBindStorage(const std::string &name)
  {
    llvm::Value *local = GetLocal(name);
    if (local && local->getType()->isPointerTy())
    {
      if (GetLocalHomeStorage(name) != local)
      {
        SetLocalHomeStorage(name, local);
      }
      return local;
    }
    return GetLocalHomeStorage(name);
  }

  // T-LLVM-010: Bind local variable
  void LLVMEmitter::EmitBindVar(const IRBindVar &bind)
  {
    auto *builder = static_cast<llvm::IRBuilder<> *>(builder_.get());
    const bool debug_parallel_bind =
        core::IsDebugEnabled("parallel");
    const bool log_this_bind =
        debug_parallel_bind &&
        (bind.name.find("parallel_") != std::string::npos ||
         bind.name.find("spawn_") != std::string::npos ||
         bind.name.find("wait_result") != std::string::npos);
    llvm::Value *init_val = nullptr;
    llvm::Type *ty = nullptr;
    if (bind.type)
    {
      ty = GetLLVMType(bind.type);
    }
    if ((!ty || ty->isVoidTy()) && init_val)
    {
      ty = init_val->getType();
    }
    if (bind.type && init_val)
    {
      analysis::TypeRef stripped = analysis::StripPerm(bind.type);
      if (!stripped)
      {
        stripped = bind.type;
      }
      if (stripped &&
          std::holds_alternative<analysis::TypeFunc>(stripped->node) &&
          IsClosurePairLLVMType(init_val->getType()))
      {
        // Non-capturing closures are represented as (env_ptr, code_ptr) pairs
        // during lowering. Preserve the concrete closure value representation
        // even when the source-level binding is annotated as TypeFunc.
        ty = init_val->getType();
      }
    }
    if (!ty || ty->isVoidTy())
    {
      ty = llvm::Type::getInt64Ty(context_);
    }

    llvm::Function *func = builder->GetInsertBlock()->getParent();
    llvm::IRBuilder<> entry_builder(&func->getEntryBlock(), func->getEntryBlock().begin());
    llvm::Value *bind_slot = nullptr;
    bool adopted_existing_storage = false;
    analysis::TypeRef source_type = nullptr;
    if (const LowerCtx *ctx = GetCurrentCtx())
    {
      source_type = ctx->LookupValueType(bind.value);
    }
    if (!source_type && bind.value.kind == IRValue::Kind::Local)
    {
      const auto it = local_types_.find(bind.value.name);
      if (it != local_types_.end())
      {
        source_type = it->second;
      }
    }
    llvm::Type *source_llvm_ty = source_type ? GetLLVMType(source_type) : nullptr;
    IRBindVar bind_for_slot = bind;
    if (!bind_for_slot.type && source_type)
    {
      bind_for_slot.type = source_type;
    }
    std::optional<BindSlot> bind_slot_info;
    if (current_ctx_)
    {
      bind_slot_info = ResolveBindSlot(bind_for_slot, *current_ctx_);
      if (!bind_slot_info.has_value())
      {
        if (!ty || ty->isVoidTy())
        {
          current_ctx_->ReportCodegenFailure();
          return;
        }
        BindSlot fallback_slot;
        fallback_slot.kind = BindSlot::Kind::Alloca;
        fallback_slot.name = bind.name;
        fallback_slot.type = bind_for_slot.type;
        bind_slot_info = std::move(fallback_slot);
      }
    }
    if (async_state_ && async_state_->info &&
        async_state_->info->slots.contains(bind.name))
    {
      bind_slot = GetLocal(bind.name);
    }

    const bool aggregate_bind_ty = ty && (ty->isStructTy() || ty->isArrayTy());
    const bool use_region_slot =
        bind_slot_info.has_value() &&
        bind_slot_info->kind == BindSlot::Kind::RegionSlot;
    if ((!bind_slot || !bind_slot->getType()->isPointerTy()) &&
        !use_region_slot &&
        aggregate_bind_ty)
    {
      if (llvm::Value *existing_storage = GetAddressableStorage(bind.value))
      {
        bool compatible_storage = (source_llvm_ty == ty);
        if (!compatible_storage)
        {
          if (auto *alloca_inst = llvm::dyn_cast<llvm::AllocaInst>(existing_storage))
          {
            compatible_storage = (alloca_inst->getAllocatedType() == ty);
          }
        }
        if (compatible_storage)
        {
          llvm::Type *slot_ptr_ty = llvm::PointerType::get(ty, 0);
          if (existing_storage->getType() != slot_ptr_ty)
          {
            existing_storage = builder->CreateBitCast(existing_storage, slot_ptr_ty);
          }
          bind_slot = existing_storage;
          adopted_existing_storage = true;
        }
      }
    }
    if ((!bind_slot || !bind_slot->getType()->isPointerTy()) && use_region_slot)
    {
      IRValue region_local;
      region_local.kind = IRValue::Kind::Local;
      region_local.name = bind_slot_info->region;
      llvm::Value *region_value = EvaluateIRValue(region_local);
      if (!region_value)
      {
        if (current_ctx_)
        {
          SPEC_RULE("BindSlot-Err");
          current_ctx_->ReportCodegenFailure();
        }
        return;
      }

      std::uint64_t alloc_size = 0;
      std::uint64_t alloc_align = 1;
      const analysis::ScopeContext &scope = BuildScope(current_ctx_);
      if (bind_slot_info->type)
      {
        if (const auto size = ::cursive::analysis::layout::SizeOf(scope, bind_slot_info->type))
        {
          alloc_size = *size;
        }
        if (const auto align = ::cursive::analysis::layout::AlignOf(scope, bind_slot_info->type))
        {
          alloc_align = *align;
        }
      }
      const llvm::DataLayout &dl = GetModule().getDataLayout();
      if (alloc_size == 0 && !ty->isVoidTy())
      {
        alloc_size = static_cast<std::uint64_t>(dl.getTypeAllocSize(ty));
      }
      if (alloc_align == 0)
      {
        alloc_align = 1;
      }
      if (alloc_align == 1 && !ty->isVoidTy())
      {
        alloc_align = std::max<std::uint64_t>(
            alloc_align,
            static_cast<std::uint64_t>(dl.getABITypeAlign(ty).value()));
      }

      llvm::Value *raw_ptr = nullptr;
      const std::string alloc_sym = BuiltinModalSymRegionAlloc();
      if (std::optional<RuntimeFuncInfo> alloc_info = GetRuntimeFuncInfo(alloc_sym))
      {
        llvm::Function *alloc_fn = GetModule().getFunction(alloc_sym);
        const bool use_c_abi_aggregate_sret = true;
        if (!alloc_fn)
        {
          ABICallResult alloc_abi = ComputeCallABI(
              alloc_info->params,
              alloc_info->ret,
              use_c_abi_aggregate_sret);
          if (alloc_abi.func_type)
          {
            alloc_fn = llvm::Function::Create(
                alloc_abi.func_type,
                llvm::GlobalValue::ExternalLinkage,
                alloc_sym,
                &GetModule());
            alloc_fn->setCallingConv(llvm::CallingConv::C);
          }
        }
        if (alloc_fn)
        {
          llvm::Type *usize_ty = llvm::Type::getInt64Ty(GetContext());
          std::vector<llvm::Value *> alloc_args;
          alloc_args.reserve(3);
          alloc_args.push_back(region_value);
          alloc_args.push_back(llvm::ConstantInt::get(usize_ty, alloc_size));
          alloc_args.push_back(llvm::ConstantInt::get(usize_ty, alloc_align));
          raw_ptr = EmitABICall(
              *this,
              builder,
              alloc_fn,
              alloc_info->params,
              alloc_info->ret,
              alloc_args,
              use_c_abi_aggregate_sret);
        }
      }

      if (!raw_ptr)
      {
        if (current_ctx_)
        {
          SPEC_RULE("BindSlot-Err");
          current_ctx_->ReportCodegenFailure();
        }
        return;
      }

      bind_slot = builder->CreateBitCast(raw_ptr, llvm::PointerType::get(ty, 0));
    }
    if (!bind_slot || !bind_slot->getType()->isPointerTy())
    {
      bind_slot = entry_builder.CreateAlloca(ty, nullptr, bind.name);
    }
    if (bind.type)
    {
      const analysis::ScopeContext &scope = BuildScope(current_ctx_);
      if (const auto align = ::cursive::analysis::layout::AlignOf(scope, bind.type); align.has_value())
      {
        if (auto *alloca_inst = llvm::dyn_cast<llvm::AllocaInst>(bind_slot))
        {
          alloca_inst->setAlignment(llvm::Align(std::max<std::uint64_t>(1, *align)));
        }
      }
    }

    if (!adopted_existing_storage)
    {
      init_val = EvaluateIRValue(bind.value);
    }

    // Store the initial value
    if (!adopted_existing_storage && !init_val)
    {
      init_val = llvm::Constant::getNullValue(ty);
      if (log_this_bind)
      {
        std::fprintf(stderr,
                     "[bind-debug] name=%s init=default-null target=%s\n",
                     bind.name.c_str(),
                     ty->isIntegerTy() ? "int" : ty->isPointerTy() ? "ptr"
                                             : ty->isStructTy()    ? "struct"
                                             : ty->isArrayTy()     ? "array"
                                                                   : "other");
      }
    }
    else if (!adopted_existing_storage)
    {
      if (log_this_bind)
      {
        std::fprintf(stderr,
                     "[bind-debug] name=%s before-coerce source=%s init=%s target=%s\n",
                     bind.name.c_str(),
                     source_type ? analysis::TypeToString(source_type).c_str() : "<null>",
                     init_val->getType()->isIntegerTy() ? "int" : init_val->getType()->isPointerTy() ? "ptr"
                                                              : init_val->getType()->isStructTy()    ? "struct"
                                                              : init_val->getType()->isArrayTy()     ? "array"
                                                                                                     : "other",
                     ty->isIntegerTy() ? "int" : ty->isPointerTy() ? "ptr"
                                             : ty->isStructTy()    ? "struct"
                                             : ty->isArrayTy()     ? "array"
                                                                   : "other");
      }
      if (UnionDebugEnabled())
      {
        analysis::TypeRef target_type = StripPermType(bind.type);
        const bool target_is_union =
            target_type && std::holds_alternative<analysis::TypeUnion>(target_type->node);
        if (target_is_union)
        {
          std::cerr << "[union-debug-bind] name=" << bind.name
                    << " source_kind=" << static_cast<int>(bind.value.kind)
                    << " source_type=" << (source_type ? "known" : "unknown")
                    << " source_llvm="
                    << (init_val->getType()->isIntegerTy() ? "int" : init_val->getType()->isStructTy() ? "struct"
                                                                 : init_val->getType()->isPointerTy()  ? "ptr"
                                                                                                       : "other")
                    << "\n";
        }
      }
      llvm::Value *coerced_init =
          CoerceToTyped(*this, builder, init_val, ty, source_type, bind.type);
      if (!coerced_init)
      {
        if (init_val->getType() == ty)
        {
          coerced_init = init_val;
        }
        else if (llvm::Value *plain = CoerceTo(builder, init_val, ty))
        {
          coerced_init = plain;
        }
      }
      init_val = coerced_init;
      if (!init_val)
      {
        init_val = llvm::Constant::getNullValue(ty);
        if (log_this_bind)
        {
          std::fprintf(stderr,
                       "[bind-debug] name=%s coerce=null-fallback\n",
                       bind.name.c_str());
        }
      }
      else if (log_this_bind)
      {
        std::fprintf(stderr,
                     "[bind-debug] name=%s after-coerce out=%s\n",
                     bind.name.c_str(),
                     init_val->getType()->isIntegerTy() ? "int" : init_val->getType()->isPointerTy() ? "ptr"
                                                              : init_val->getType()->isStructTy()    ? "struct"
                                                              : init_val->getType()->isArrayTy()     ? "array"
                                                                                                     : "other");
      }
    }
    if (!adopted_existing_storage)
    {
      llvm::Value *typed_slot = bind_slot;
      llvm::Type *slot_ptr_ty = llvm::PointerType::get(ty, 0);
      if (typed_slot->getType() != slot_ptr_ty)
      {
        typed_slot = builder->CreateBitCast(typed_slot, slot_ptr_ty);
      }
      builder->CreateStore(init_val, typed_slot);
    }
    else
    {
      storage_values_.erase(bind.value.name);
      values_.erase(bind.value.name);
    }

    if (!adopted_existing_storage)
    {
      ReleaseTempStorage(bind.value);
    }

    RegisterLocalBindStorage(bind.name, bind_slot);
    if (bind.type)
    {
      local_types_[bind.name] = bind.type;
    }
    if (!bind.stable_name.empty() && bind.stable_name != bind.name)
    {
      RegisterLocalBindStorage(bind.stable_name, bind_slot);
      if (bind.type)
      {
        local_types_[bind.stable_name] = bind.type;
      }
    }
  }

  // Evaluate an IRValue to an llvm::Value*
  llvm::Value *LLVMEmitter::EvaluateIRValue(const IRValue &val)
  {
    auto *builder = static_cast<llvm::IRBuilder<> *>(builder_.get());

    auto lookup_value_type = [&](const IRValue &value) -> analysis::TypeRef
    {
      if (const LowerCtx *ctx = GetCurrentCtx())
      {
        if (analysis::TypeRef type = ctx->LookupValueType(value))
        {
          return type;
        }
      }
      if (value.kind == IRValue::Kind::Local)
      {
        const auto it = local_types_.find(value.name);
        if (it != local_types_.end())
        {
          return it->second;
        }
      }
      return nullptr;
    };

    auto default_for = [&](const IRValue &value) -> llvm::Value *
    {
      if (analysis::TypeRef type = lookup_value_type(value))
      {
        if (llvm::Type *llvm_ty = GetLLVMType(type))
        {
          return llvm::Constant::getNullValue(llvm_ty);
        }
      }
      return llvm::ConstantInt::get(llvm::Type::getInt64Ty(context_), 0);
    };

    if (val.kind == IRValue::Kind::Opaque)
    {
      if (llvm::Value *cached = GetTempValue(val))
      {
        return cached;
      }
      if (llvm::Value *storage = GetTempStorage(val))
      {
        analysis::TypeRef storage_type = lookup_value_type(val);
        llvm::Type *storage_llvm_ty = storage_type ? GetLLVMType(storage_type) : nullptr;
        if (storage_llvm_ty && !storage_llvm_ty->isVoidTy())
        {
          llvm::Value *typed_ptr = storage;
          llvm::Type *expected_ptr_ty = llvm::PointerType::get(storage_llvm_ty, 0);
          if (typed_ptr->getType() != expected_ptr_ty)
          {
            typed_ptr = builder->CreateBitCast(typed_ptr, expected_ptr_ty);
          }
          return builder->CreateLoad(storage_llvm_ty, typed_ptr);
        }
      }
    }

    auto eval_key_for = [](const IRValue &value) -> std::string
    {
      std::string key;
      key.reserve(64 + value.name.size() + (value.bytes.size() * 2));
      key += std::to_string(static_cast<int>(value.kind));
      key.push_back(':');
      key += value.name;
      key.push_back(':');
      for (std::uint8_t byte : value.bytes)
      {
        constexpr char kHex[] = "0123456789abcdef";
        key.push_back(kHex[(byte >> 4) & 0x0f]);
        key.push_back(kHex[byte & 0x0f]);
      }
      return key;
    };

    thread_local std::vector<std::string> eval_stack;
    const bool track_eval_cycle = (val.kind == IRValue::Kind::Opaque);
    std::optional<std::string> eval_key;
    if (track_eval_cycle)
    {
      eval_key = eval_key_for(val);
      if (std::find(eval_stack.begin(), eval_stack.end(), *eval_key) != eval_stack.end())
      {
        if (core::IsDebugEnabled("obj"))
        {
          std::cerr << "[cursive] recursive EvaluateIRValue cycle for key=" << *eval_key;
          if (!eval_stack.empty())
          {
            std::cerr << " stack=[";
            for (std::size_t i = 0; i < eval_stack.size(); ++i)
            {
              if (i != 0)
              {
                std::cerr << " -> ";
              }
              std::cerr << eval_stack[i];
            }
            std::cerr << "]";
          }
          std::cerr << "\n";
        }
        return default_for(val);
      }

      eval_stack.push_back(*eval_key);
    }

    struct EvalStackPopGuard
    {
      std::vector<std::string> *stack = nullptr;
      bool active = false;
      ~EvalStackPopGuard()
      {
        if (active && stack && !stack->empty())
        {
          stack->pop_back();
        }
      }
    } eval_stack_pop_guard{&eval_stack, track_eval_cycle};

    auto resolve_symbol = [&](const std::string &name) -> std::string
    {
      if (GetFunction(name) || GetGlobal(name) ||
          module_->getFunction(name) || module_->getNamedGlobal(name))
      {
        return name;
      }
      if (auto alias = LookupSymbolAlias(name))
      {
        if (GetFunction(*alias) || GetGlobal(*alias) ||
            module_->getFunction(*alias) || module_->getNamedGlobal(*alias))
        {
          return *alias;
        }
        if (HasHostedStateSlot(*alias))
        {
          return *alias;
        }
        if (const LowerCtx *active_ctx = GetCurrentCtx())
        {
          if (active_ctx->LookupStaticType(*alias) ||
              active_ctx->LookupProcSig(*alias))
          {
            return *alias;
          }
        }
      }
      return name;
    };

    switch (val.kind)
    {
    case IRValue::Kind::Local:
    {
      llvm::Value *local = GetLocalBindStorage(val.name);
      if (core::IsDebugEnabled("obj") &&
          (val.name == "unit_value" || val.name == "tuple_value" || val.name == "record_value"))
      {
        std::cerr << "[enum-local] name=" << val.name
                  << " found=" << (local ? "yes" : "no");
        if (local)
        {
          std::cerr << " local_ty=" << LLVMValueRepr(local);
        }
        std::cerr << "\n";
      }
      if (!local)
      {
        return nullptr;
      }
      if (auto *alloca = llvm::dyn_cast<llvm::AllocaInst>(local))
      {
        return builder->CreateLoad(alloca->getAllocatedType(), alloca);
      }
      if (local->getType()->isPointerTy())
      {
        const auto it = local_types_.find(val.name);
        if (it != local_types_.end() && it->second)
        {
          if (llvm::Type *local_ty = GetLLVMType(it->second))
          {
            llvm::Value *typed_ptr = local;
            llvm::Type *expected_ptr_ty = llvm::PointerType::get(local_ty, 0);
            if (typed_ptr->getType() != expected_ptr_ty)
            {
              typed_ptr = builder->CreateBitCast(typed_ptr, expected_ptr_ty);
            }
            return builder->CreateLoad(local_ty, typed_ptr);
          }
        }
      }
      return local;
    }

    case IRValue::Kind::Symbol:
    {
      const std::string symbol = resolve_symbol(val.name);
      auto configure_imported_static_decl =
          [&](llvm::GlobalVariable *decl) -> llvm::GlobalVariable * {
        if (!decl) {
          return nullptr;
        }
        const LowerCtx *active_ctx = GetCurrentCtx();
        if (!active_ctx || active_ctx->module_path.empty()) {
          return decl;
        }
        if (project::ObjectFormatOf(target_profile_) != project::ObjectFormat::Coff) {
          return decl;
        }
        const auto *owner_module = active_ctx->LookupStaticModule(symbol);
        if (!owner_module || owner_module->empty()) {
          return decl;
        }
        const std::string &current_root = active_ctx->module_path.front();
        const std::string &owner_root = owner_module->front();
        const bool imported_shared_library_data =
            owner_root != current_root &&
            active_ctx->library_assembly_names.contains(owner_root);
        if (!imported_shared_library_data) {
          return decl;
        }
        decl->setDLLStorageClass(llvm::GlobalValue::DLLImportStorageClass);
        return decl;
      };
      auto load_global_value = [&](llvm::GlobalVariable *global_var) -> llvm::Value *
      {
        if (!global_var)
        {
          return nullptr;
        }
        global_var = configure_imported_static_decl(global_var);
        if (!builder->GetInsertBlock())
        {
          return global_var;
        }

        analysis::TypeRef symbol_type = analysis::StripPerm(lookup_value_type(val));
        if (!symbol_type)
        {
          symbol_type = lookup_value_type(val);
        }
        if (!symbol_type)
        {
          if (const LowerCtx *active_ctx = GetCurrentCtx())
          {
            symbol_type = active_ctx->LookupStaticType(symbol);
          }
        }
        if (symbol_type)
        {
          if (llvm::Type *symbol_ll = GetLLVMType(symbol_type))
          {
            llvm::Value *typed_ptr =
                GetHostedStatePtr(symbol, symbol_ll, global_var);
            if (!typed_ptr && HasHostedStateSlot(symbol) && !global_var)
            {
              return nullptr;
            }
            if (!typed_ptr)
            {
              typed_ptr = builder->CreateBitCast(
                  global_var, llvm::PointerType::get(symbol_ll, 0));
            }
            llvm::LoadInst *loaded = builder->CreateLoad(symbol_ll, typed_ptr);
            loaded->setAlignment(llvm::Align(1));
            return loaded;
          }
        }

        if (llvm::Value *hosted_ptr =
                GetHostedStatePtr(symbol, global_var->getValueType(), global_var))
        {
          llvm::LoadInst *loaded =
              builder->CreateLoad(global_var->getValueType(), hosted_ptr);
          loaded->setAlignment(llvm::Align(1));
          return loaded;
        }
        if (HasHostedStateSlot(symbol) && !global_var)
        {
          return nullptr;
        }

        return builder->CreateLoad(global_var->getValueType(), global_var);
      };

      // Symbol can be a global variable or a function
      if (llvm::Function *func = GetFunction(symbol))
      {
        return func;
      }
      if (llvm::Function *func = module_->getFunction(symbol))
      {
        return func;
      }
      if (llvm::Value *global = GetGlobal(symbol))
      {
        if (auto *global_var = llvm::dyn_cast<llvm::GlobalVariable>(global))
        {
          return load_global_value(global_var);
        }
        return global;
      }
      if (llvm::GlobalVariable *global = module_->getNamedGlobal(symbol))
      {
        return load_global_value(global);
      }
      if (const LowerCtx *active_ctx = GetCurrentCtx())
      {
        analysis::TypeRef static_type = active_ctx->LookupStaticType(symbol);
        static_type = analysis::StripPerm(static_type);
        if (!static_type)
        {
          static_type = active_ctx->LookupStaticType(symbol);
        }
        if (static_type)
        {
          if (llvm::Type *static_ll = GetLLVMType(static_type))
          {
            llvm::GlobalVariable *decl = module_->getNamedGlobal(symbol);
            if (!decl)
            {
              decl = new llvm::GlobalVariable(
                  *module_,
                  static_ll,
                  false,
                  llvm::GlobalValue::ExternalLinkage,
                  nullptr,
                  symbol);
            }
            decl = configure_imported_static_decl(decl);
            return load_global_value(decl);
          }
        }
      }
      return nullptr;
    }

    case IRValue::Kind::Immediate:
    {
      analysis::TypeRef immediate_type;
      immediate_type = analysis::StripPerm(lookup_value_type(val));
      if (!immediate_type)
      {
        immediate_type = lookup_value_type(val);
      }

      auto build_view_literal = [&](llvm::Type *view_ty, LiteralKind literal_kind) -> llvm::Value *
      {
        auto *struct_ty = llvm::dyn_cast_or_null<llvm::StructType>(view_ty);
        if (!struct_ty || struct_ty->getNumElements() < 2)
        {
          return nullptr;
        }

        llvm::Type *ptr_elem_ty = struct_ty->getElementType(0);
        llvm::Type *len_elem_ty = struct_ty->getElementType(1);

        llvm::Constant *data_ptr = llvm::ConstantPointerNull::get(
            llvm::cast<llvm::PointerType>(GetOpaquePtr()));

        if (!val.bytes.empty())
        {
          llvm::ArrayType *arr_ty =
              llvm::ArrayType::get(llvm::Type::getInt8Ty(context_), val.bytes.size());
          const std::string literal_sym = LiteralSym(literal_kind, val.bytes);
          llvm::GlobalVariable *gv = module_->getNamedGlobal(literal_sym);
          if (!gv)
          {
            llvm::Constant *arr_init =
                llvm::ConstantDataArray::get(context_, llvm::ArrayRef<std::uint8_t>(val.bytes));
            gv = new llvm::GlobalVariable(
                *module_,
                arr_ty,
                true,
                llvm::GlobalValue::InternalLinkage,
                arr_init,
                literal_sym);
            gv->setUnnamedAddr(llvm::GlobalValue::UnnamedAddr::Global);
            gv->setAlignment(llvm::Align(1));
          }

          llvm::Constant *zero = llvm::ConstantInt::get(
              llvm::Type::getInt64Ty(context_), 0);
          llvm::Constant *indices[] = {zero, zero};
          llvm::Constant *gep = llvm::ConstantExpr::getInBoundsGetElementPtr(
              arr_ty, gv, indices);
          data_ptr = llvm::ConstantExpr::getPointerCast(gep, GetOpaquePtr());
        }

        if (ptr_elem_ty->isPointerTy() && data_ptr->getType() != ptr_elem_ty)
        {
          data_ptr = llvm::ConstantExpr::getPointerCast(data_ptr, ptr_elem_ty);
        }

        llvm::Constant *len = llvm::ConstantInt::get(
            len_elem_ty, static_cast<std::uint64_t>(val.bytes.size()));
        return llvm::ConstantStruct::get(struct_ty, {data_ptr, len});
      };
      auto build_int_literal = [&](unsigned bit_width) -> llvm::APInt
      {
        if (bit_width == 0)
        {
          bit_width = 64;
        }
        std::vector<std::uint64_t> words((bit_width + 63u) / 64u, 0u);
        const std::size_t max_bytes = words.size() * sizeof(std::uint64_t);
        const std::size_t byte_count = std::min(val.bytes.size(), max_bytes);
        for (std::size_t i = 0; i < byte_count; ++i)
        {
          const std::size_t word_index = i / sizeof(std::uint64_t);
          const std::size_t byte_index = i % sizeof(std::uint64_t);
          words[word_index] |= (static_cast<std::uint64_t>(val.bytes[i]) << (byte_index * 8u));
        }
        return llvm::APInt(bit_width, llvm::ArrayRef<std::uint64_t>(words));
      };

      const bool looks_like_string_literal =
          val.name.size() >= 2 && val.name.front() == '"' && val.name.back() == '"';
      if (looks_like_string_literal)
      {
        if (core::IsDebugEnabled("obj"))
        {
          std::fprintf(stderr,
                       "[llvm-immediate-string] name=%s bytes=%zu\n",
                       val.name.c_str(),
                       val.bytes.size());
        }
        auto *default_view_ty = llvm::StructType::get(
            context_, {GetOpaquePtr(), llvm::Type::getInt64Ty(context_)});
        if (llvm::Value *view = build_view_literal(default_view_ty, LiteralKind::String))
        {
          return view;
        }
      }

      if (immediate_type)
      {
        if (const auto *str_ty = std::get_if<analysis::TypeString>(&immediate_type->node))
        {
          if (str_ty->state.has_value() && *str_ty->state == analysis::StringState::View)
          {
            if (llvm::Type *view_ty = GetLLVMType(immediate_type))
            {
              if (llvm::Value *view = build_view_literal(view_ty, LiteralKind::String))
              {
                return view;
              }
            }
          }
        }

        if (const auto *bytes_ty = std::get_if<analysis::TypeBytes>(&immediate_type->node))
        {
          if (bytes_ty->state.has_value() && *bytes_ty->state == analysis::BytesState::View)
          {
            if (llvm::Type *view_ty = GetLLVMType(immediate_type))
            {
              if (llvm::Value *view = build_view_literal(view_ty, LiteralKind::Bytes))
              {
                return view;
              }
            }
          }
        }
      }

      if (val.name == "true")
      {
        return llvm::ConstantInt::getTrue(context_);
      }
      if (val.name == "false")
      {
        return llvm::ConstantInt::getFalse(context_);
      }
      if (immediate_type)
      {
        if (const auto *prim = std::get_if<analysis::TypePrim>(&immediate_type->node))
        {
          (void)prim;
          if (llvm::Type *prim_ty = GetLLVMType(immediate_type))
          {
            if (prim_ty->isFloatingPointTy())
            {
              std::uint64_t raw = 0;
              for (std::size_t i = 0; i < val.bytes.size() && i < 8; ++i)
              {
                raw |= static_cast<std::uint64_t>(val.bytes[i]) << (8 * i);
              }

              if (prim_ty->isHalfTy())
              {
                const std::uint16_t bits16 = static_cast<std::uint16_t>(raw & 0xFFFFu);
                llvm::APFloat fp(llvm::APFloat::IEEEhalf(),
                                 llvm::APInt(16, bits16));
                return llvm::ConstantFP::get(context_, fp);
              }
              if (prim_ty->isFloatTy())
              {
                const std::uint32_t bits32 = static_cast<std::uint32_t>(raw & 0xFFFFFFFFu);
                float native = 0.0f;
                std::memcpy(&native, &bits32, sizeof(bits32));
                return llvm::ConstantFP::get(prim_ty, static_cast<double>(native));
              }
              if (prim_ty->isDoubleTy())
              {
                double native = 0.0;
                std::memcpy(&native, &raw, sizeof(raw));
                return llvm::ConstantFP::get(prim_ty, native);
              }

              if (!val.name.empty())
              {
                auto strip_float_suffix = [](std::string text) -> std::string
                {
                  constexpr const char *kSuffixes[] = {"f16", "f32", "f64", "f"};
                  for (const char *suffix : kSuffixes)
                  {
                    const std::size_t slen = std::char_traits<char>::length(suffix);
                    if (text.size() >= slen &&
                        text.compare(text.size() - slen, slen, suffix) == 0)
                    {
                      text.resize(text.size() - slen);
                      break;
                    }
                  }
                  return text;
                };
                const std::string core = strip_float_suffix(val.name);
                try
                {
                  const double parsed = std::stod(core);
                  return llvm::ConstantFP::get(prim_ty, parsed);
                }
                catch (...)
                {
                }
              }
            }
            if (auto *int_ty = llvm::dyn_cast<llvm::IntegerType>(prim_ty))
            {
              return llvm::ConstantInt::get(int_ty, build_int_literal(int_ty->getBitWidth()));
            }
          }
        }
      }
      // Create constant from the bytes
      if (val.bytes.empty())
      {
        return llvm::ConstantInt::get(llvm::Type::getInt64Ty(context_), 0);
      }
      // Interpret bytes as an integer value
      std::uint64_t word = 0;
      for (std::size_t i = 0; i < val.bytes.size() && i < 8; ++i)
      {
        word |= static_cast<std::uint64_t>(val.bytes[i]) << (8 * i);
      }
      unsigned bit_width = static_cast<unsigned>(val.bytes.size() * 8);
      if (bit_width == 0)
        bit_width = 64;
      return llvm::ConstantInt::get(
          llvm::Type::getIntNTy(context_, bit_width), word);
    }

    case IRValue::Kind::Opaque:
    {
      const LowerCtx *ctx = GetCurrentCtx();
      if (!ctx)
      {
        return nullptr;
      }
      const DerivedValueInfo *derived = ctx->LookupDerivedValue(val);
      if (!derived)
      {
        if (core::IsDebugEnabled("obj"))
        {
          llvm::Function *fn =
              builder->GetInsertBlock() ? builder->GetInsertBlock()->getParent() : nullptr;
          std::cerr << "[cursive] missing derived value in "
                    << (fn ? fn->getName().str() : std::string("<no-func>"))
                    << " opaque=" << val.name
                    << " type="
                    << (lookup_value_type(val)
                            ? analysis::TypeToString(lookup_value_type(val))
                            : std::string("<null>"))
                    << "\n";
        }
        if (val.name == "null")
        {
          return llvm::ConstantPointerNull::get(
              llvm::cast<llvm::PointerType>(GetOpaquePtr()));
        }
        return nullptr;
      }

      llvm::Value *materialized = nullptr;
      const analysis::ScopeContext &scope = BuildScope(ctx);
      auto pointer_from_value = [&](llvm::Value *value) -> llvm::Value *
      {
        if (!value)
        {
          return nullptr;
        }
        if (value->getType()->isPointerTy())
        {
          return value;
        }
        if (value->getType()->isIntegerTy())
        {
          return builder->CreateIntToPtr(value, GetOpaquePtr());
        }
        return nullptr;
      };
      auto pointee_from_type = [&](analysis::TypeRef type) -> analysis::TypeRef
      {
        analysis::TypeRef current = analysis::StripPerm(type);
        if (!current)
        {
          return nullptr;
        }
        if (const auto *raw = std::get_if<analysis::TypeRawPtr>(&current->node))
        {
          return analysis::StripPerm(raw->element);
        }
        if (const auto *ptr = std::get_if<analysis::TypePtr>(&current->node))
        {
          return analysis::StripPerm(ptr->element);
        }
        return ResolveAliasTypeInScope(scope, current);
      };
      auto field_meta_for = [&](const IRValue &base_value,
                                std::string_view field_name) -> std::optional<FieldAccessMeta>
      {
        auto resolve_from_type = [&](analysis::TypeRef type) -> std::optional<FieldAccessMeta>
        {
          if (!type)
          {
            return std::nullopt;
          }
          analysis::TypeRef candidate = pointee_from_type(type);
          if (!candidate)
          {
            candidate = type;
          }
          return ResolveFieldAccessMeta(scope, candidate, field_name);
        };

        if (analysis::TypeRef base_type = lookup_value_type(base_value))
        {
          if (auto meta = resolve_from_type(base_type))
          {
            return meta;
          }
        }

        const DerivedValueInfo *base_derived = ctx->LookupDerivedValue(base_value);
        while (base_derived)
        {
          if (base_derived->kind == DerivedValueInfo::Kind::AddrLocal)
          {
            if (const BindingState *state = ctx->GetBindingState(base_derived->name))
            {
              if (auto meta = resolve_from_type(state->type))
              {
                return meta;
              }
            }
            break;
          }
          if (base_derived->kind == DerivedValueInfo::Kind::AddrField ||
              base_derived->kind == DerivedValueInfo::Kind::AddrTuple ||
              base_derived->kind == DerivedValueInfo::Kind::AddrIndex ||
              base_derived->kind == DerivedValueInfo::Kind::AddrDeref)
          {
            base_derived = ctx->LookupDerivedValue(base_derived->base);
            continue;
          }
          break;
        }

        return std::nullopt;
      };
      auto strip_perm = [](analysis::TypeRef type) -> analysis::TypeRef
      {
        if (!type)
        {
          return nullptr;
        }
        if (analysis::TypeRef stripped = analysis::StripPerm(type))
        {
          return stripped;
        }
        return type;
      };
      struct MaterializedRangeValue
      {
        IRRangeKind kind = IRRangeKind::Full;
        llvm::Value *lo = nullptr;
        llvm::Value *hi = nullptr;
      };
      auto materialize_range_value = [&](const IRValue &range_value,
                                        llvm::Type *bound_ty,
                                        std::optional<IRRangeKind> fallback_kind = std::nullopt)
          -> std::optional<MaterializedRangeValue>
      {
        auto normalize_range_type = [&](analysis::TypeRef type) -> analysis::TypeRef
        {
          analysis::TypeRef current = strip_perm(type);
          if (!current)
          {
            current = type;
          }
          for (int depth = 0; current && depth < 4; ++depth)
          {
            if (analysis::TypeRef resolved = ResolveAliasTypeInScope(scope, current))
            {
              current = strip_perm(resolved);
              if (!current)
              {
                current = resolved;
              }
              continue;
            }
            break;
          }
          return strip_perm(current);
        };

        analysis::TypeRef range_type =
            normalize_range_type(lookup_value_type(range_value));

        MaterializedRangeValue out;
        std::optional<unsigned> lo_index;
        std::optional<unsigned> hi_index;
        auto configure_for_kind = [&](IRRangeKind kind) -> bool
        {
          out.kind = kind;
          lo_index.reset();
          hi_index.reset();
          switch (kind)
          {
          case IRRangeKind::Full:
            return true;
          case IRRangeKind::From:
            lo_index = 0u;
            return true;
          case IRRangeKind::To:
          case IRRangeKind::ToInclusive:
            hi_index = 0u;
            return true;
          case IRRangeKind::Exclusive:
          case IRRangeKind::Inclusive:
            lo_index = 0u;
            hi_index = 1u;
            return true;
          }
          return false;
        };

        if (range_type && analysis::IsRangeType(range_type))
        {
          if (std::holds_alternative<analysis::TypeRange>(range_type->node))
          {
            if (!configure_for_kind(IRRangeKind::Exclusive))
            {
              return std::nullopt;
            }
          }
          else if (std::holds_alternative<analysis::TypeRangeInclusive>(
                       range_type->node))
          {
            if (!configure_for_kind(IRRangeKind::Inclusive))
            {
              return std::nullopt;
            }
          }
          else if (std::holds_alternative<analysis::TypeRangeFrom>(
                       range_type->node))
          {
            if (!configure_for_kind(IRRangeKind::From))
            {
              return std::nullopt;
            }
          }
          else if (std::holds_alternative<analysis::TypeRangeTo>(
                       range_type->node))
          {
            if (!configure_for_kind(IRRangeKind::To))
            {
              return std::nullopt;
            }
          }
          else if (std::holds_alternative<analysis::TypeRangeToInclusive>(
                       range_type->node))
          {
            if (!configure_for_kind(IRRangeKind::ToInclusive))
            {
              return std::nullopt;
            }
          }
          else if (std::holds_alternative<analysis::TypeRangeFull>(
                       range_type->node))
          {
            if (!configure_for_kind(IRRangeKind::Full))
            {
              return std::nullopt;
            }
          }
          else
          {
            return std::nullopt;
          }
        }
        else if (fallback_kind.has_value())
        {
          if (!configure_for_kind(*fallback_kind))
          {
            return std::nullopt;
          }
        }
        else
        {
          return std::nullopt;
        }

        if (!lo_index.has_value() && !hi_index.has_value())
        {
          return out;
        }

        llvm::Value *raw = EvaluateIRValue(range_value);
        if (!raw)
        {
          return std::nullopt;
        }
        llvm::Type *range_ll = range_type ? GetLLVMType(range_type) : nullptr;
        if (raw->getType()->isPointerTy())
        {
          if (!range_ll)
          {
            return std::nullopt;
          }
          llvm::Value *typed_ptr = raw;
          llvm::Type *expected_ptr_ty = llvm::PointerType::get(range_ll, 0);
          if (typed_ptr->getType() != expected_ptr_ty)
          {
            typed_ptr = builder->CreateBitCast(typed_ptr, expected_ptr_ty);
          }
          raw = builder->CreateLoad(range_ll, typed_ptr);
        }
        else if (range_ll && raw->getType() != range_ll)
        {
          raw = CoerceTo(builder, raw, range_ll);
        }
        if (!raw)
        {
          return std::nullopt;
        }

        auto extract_bound = [&](unsigned index) -> llvm::Value *
        {
          auto *struct_ty = llvm::dyn_cast<llvm::StructType>(raw->getType());
          if (!struct_ty || index >= struct_ty->getNumElements())
          {
            return nullptr;
          }
          llvm::Value *bound = builder->CreateExtractValue(raw, {index});
          if (!bound || !bound->getType()->isIntegerTy() || !bound_ty ||
              !bound_ty->isIntegerTy())
          {
            return nullptr;
          }
          if (bound->getType() != bound_ty)
          {
            bound = builder->CreateIntCast(bound, bound_ty, false);
          }
          return bound;
        };

        if (lo_index.has_value())
        {
          out.lo = extract_bound(*lo_index);
          if (!out.lo)
          {
            return std::nullopt;
          }
        }
        if (hi_index.has_value())
        {
          out.hi = extract_bound(*hi_index);
          if (!out.hi)
          {
            return std::nullopt;
          }
        }
        return out;
      };
      auto enum_decl_for_type = [&](analysis::TypeRef type,
                                    analysis::TypePath *out_path) -> const ast::EnumDecl *
      {
        type = strip_perm(type);
        const auto *path = type ? std::get_if<analysis::TypePathType>(&type->node) : nullptr;
        if (!path)
        {
          return nullptr;
        }
        if (out_path)
        {
          *out_path = path->path;
        }
        if (const ast::EnumDecl* decl = analysis::LookupEnumDecl(scope, path->path))
        {
          return decl;
        }
        if (!scope.current_module.empty() && path->path.size() == 1u)
        {
          analysis::TypePath qualified = scope.current_module;
          qualified.insert(qualified.end(), path->path.begin(), path->path.end());
          if (out_path)
          {
            *out_path = qualified;
          }
          return analysis::LookupEnumDecl(scope, qualified);
        }
        return nullptr;
      };
      auto enum_decl_for_value = [&](const IRValue &value,
                                     analysis::TypePath *out_path) -> const ast::EnumDecl *
      {
        return enum_decl_for_type(lookup_value_type(value), out_path);
      };
      auto enum_decl_for_static_path = [&](const std::vector<std::string> &path,
                                           analysis::TypePath *out_path)
          -> const ast::EnumDecl *
      {
        if (path.empty())
        {
          return nullptr;
        }
        analysis::TypePath resolved_path;
        resolved_path.reserve(path.size());
        for (const auto &seg : path)
        {
          resolved_path.push_back(seg);
        }
        if (out_path)
        {
          *out_path = resolved_path;
        }
        if (const ast::EnumDecl* decl = analysis::LookupEnumDecl(scope, resolved_path))
        {
          return decl;
        }
        if (!scope.current_module.empty() && resolved_path.size() == 1u)
        {
          analysis::TypePath qualified = scope.current_module;
          qualified.insert(qualified.end(), resolved_path.begin(), resolved_path.end());
          if (out_path)
          {
            *out_path = qualified;
          }
          return analysis::LookupEnumDecl(scope, qualified);
        }
        return nullptr;
      };
      auto enum_decl_for_payload_value = [&](const DerivedValueInfo &info,
                                             analysis::TypePath *out_path)
          -> const ast::EnumDecl *
      {
        if (const ast::EnumDecl *decl = enum_decl_for_value(info.base, out_path))
        {
          return decl;
        }
        if (!info.static_path.empty())
        {
          if (const ast::EnumDecl *decl =
                  enum_decl_for_static_path(info.static_path, out_path))
          {
            return decl;
          }
        }
        if (const DerivedValueInfo *base_derived = ctx->LookupDerivedValue(info.base))
        {
          if (!base_derived->static_path.empty())
          {
            if (const ast::EnumDecl *decl =
                    enum_decl_for_static_path(base_derived->static_path, out_path))
            {
              return decl;
            }
          }
        }
        return nullptr;
      };
      auto find_enum_variant = [](const ast::EnumDecl &decl,
                                  std::string_view variant_name) -> const ast::VariantDecl *
      {
        for (const auto &variant : decl.variants)
        {
          if (analysis::IdEq(variant.name, std::string(variant_name)))
          {
            return &variant;
          }
        }
        return nullptr;
      };
      auto enum_variant_disc = [&](const ast::EnumDecl &decl,
                                   std::string_view variant_name) -> std::optional<std::uint64_t>
      {
        const auto discs = analysis::EnumDiscriminants(decl);
        if (!discs.ok || discs.discs.size() != decl.variants.size())
        {
          return std::nullopt;
        }
        for (std::size_t i = 0; i < decl.variants.size(); ++i)
        {
          if (analysis::IdEq(decl.variants[i].name, std::string(variant_name)))
          {
            return discs.discs[i];
          }
        }
        return std::nullopt;
      };
      struct EnumPayloadMemberInfo
      {
        analysis::TypeRef type;
        std::uint64_t offset = 0;
        std::uint64_t payload_size = 0;
        std::uint64_t payload_align = 1;
        bool ok = false;
      };
      auto enum_payload_member_by_index = [&](const ast::EnumDecl &enum_decl,
                                              const ast::VariantDecl &variant,
                                              std::size_t index) -> EnumPayloadMemberInfo
      {
        EnumPayloadMemberInfo out;
        const auto enum_layout =
            ::cursive::analysis::layout::EnumLayoutOf(scope, enum_decl, ::cursive::analysis::layout::ResolveEnumLayoutOptions(enum_decl.attrs));
        if (!enum_layout.has_value())
        {
          return out;
        }
        if (!variant.payload_opt.has_value())
        {
          return out;
        }
        const auto *tuple = std::get_if<ast::VariantPayloadTuple>(&*variant.payload_opt);
        if (!tuple || index >= tuple->elements.size())
        {
          return out;
        }
        std::vector<analysis::TypeRef> field_types;
        field_types.reserve(tuple->elements.size());
        for (const auto &elem : tuple->elements)
        {
          const auto lowered = ::cursive::analysis::layout::LowerTypeForLayout(scope, elem);
          if (!lowered.has_value())
          {
            return out;
          }
          field_types.push_back(*lowered);
        }
        const auto layout = ::cursive::analysis::layout::RecordLayoutOf(scope, field_types);
        if (!layout.has_value() || index >= layout->offsets.size())
        {
          return out;
        }
        out.type = field_types[index];
        out.offset = layout->offsets[index];
        out.payload_size = enum_layout->payload_size;
        out.payload_align = enum_layout->payload_align;
        out.ok = true;
        return out;
      };
      auto enum_payload_member_by_field = [&](const ast::EnumDecl &enum_decl,
                                              const ast::VariantDecl &variant,
                                              std::string_view field_name)
          -> EnumPayloadMemberInfo
      {
        EnumPayloadMemberInfo out;
        const auto enum_layout =
            ::cursive::analysis::layout::EnumLayoutOf(scope, enum_decl, ::cursive::analysis::layout::ResolveEnumLayoutOptions(enum_decl.attrs));
        if (!enum_layout.has_value())
        {
          return out;
        }
        if (!variant.payload_opt.has_value())
        {
          return out;
        }
        const auto *record = std::get_if<ast::VariantPayloadRecord>(&*variant.payload_opt);
        if (!record)
        {
          return out;
        }
        std::vector<analysis::TypeRef> field_types;
        std::vector<std::string> field_names;
        field_types.reserve(record->fields.size());
        field_names.reserve(record->fields.size());
        for (const auto &field : record->fields)
        {
          const auto lowered = ::cursive::analysis::layout::LowerTypeForLayout(scope, field.type);
          if (!lowered.has_value())
          {
            return out;
          }
          field_types.push_back(*lowered);
          field_names.push_back(field.name);
        }
        const auto layout = ::cursive::analysis::layout::RecordLayoutOf(scope, field_types);
        if (!layout.has_value())
        {
          return out;
        }
        for (std::size_t i = 0; i < field_names.size() && i < layout->offsets.size(); ++i)
        {
          if (analysis::IdEq(field_names[i], std::string(field_name)))
          {
            out.type = field_types[i];
            out.offset = layout->offsets[i];
            out.payload_size = enum_layout->payload_size;
            out.payload_align = enum_layout->payload_align;
            out.ok = true;
            break;
          }
        }
        return out;
      };
      auto load_enum_payload_member = [&](llvm::Value *enum_value,
                                          const EnumPayloadMemberInfo &member) -> llvm::Value *
      {
        if (!enum_value || !member.ok || !member.type)
        {
          return nullptr;
        }
        llvm::Type *member_ty = GetLLVMType(member.type);
        auto *enum_ty = llvm::dyn_cast<llvm::StructType>(enum_value->getType());
        if (!member_ty || !enum_ty || enum_ty->getNumElements() < 2)
        {
          return nullptr;
        }
        llvm::Function *current_fn =
            builder->GetInsertBlock() ? builder->GetInsertBlock()->getParent() : nullptr;
        if (!current_fn)
        {
          return nullptr;
        }
        llvm::IRBuilder<> entry_builder(
            &current_fn->getEntryBlock(),
            current_fn->getEntryBlock().begin());
        llvm::AllocaInst *enum_slot = entry_builder.CreateAlloca(enum_ty);
        builder->CreateStore(enum_value, enum_slot);
        llvm::Value *payload_i8 = CreateTaggedPayloadI8Ptr(
            *this,
            builder,
            enum_ty,
            enum_slot,
            member.payload_align);
        if (!payload_i8)
        {
          return nullptr;
        }
        llvm::Type *i8_ty = llvm::Type::getInt8Ty(context_);
        llvm::Type *i64_ty = llvm::Type::getInt64Ty(context_);
        llvm::Value *field_i8 = builder->CreateGEP(
            i8_ty,
            payload_i8,
            llvm::ConstantInt::get(i64_ty, member.offset));
        llvm::Value *field_ptr = builder->CreateBitCast(
            field_i8,
            llvm::PointerType::get(member_ty, 0));
        llvm::LoadInst *load = builder->CreateLoad(member_ty, field_ptr);
        load->setAlignment(llvm::Align(1));
        return load;
      };
      auto modal_decl_for_type = [&](analysis::TypeRef type,
                                     analysis::TypePath *out_path) -> const ast::ModalDecl *
      {
        type = strip_perm(type);
        if (!type)
        {
          return nullptr;
        }
        if (const auto *state = std::get_if<analysis::TypeModalState>(&type->node))
        {
          if (out_path)
          {
            *out_path = state->path;
          }
          return analysis::LookupModalDecl(scope, state->path);
        }
        const auto *path = std::get_if<analysis::TypePathType>(&type->node);
        if (!path)
        {
          return nullptr;
        }
        if (out_path)
        {
          *out_path = path->path;
        }
        return analysis::LookupModalDecl(scope, path->path);
      };
      auto modal_decl_for_value = [&](const IRValue &value,
                                      analysis::TypePath *out_path) -> const ast::ModalDecl *
      {
        return modal_decl_for_type(lookup_value_type(value), out_path);
      };
      auto modal_decl_for_static_path = [&](const std::vector<std::string> &path,
                                            analysis::TypePath *out_path)
          -> const ast::ModalDecl *
      {
        if (path.empty())
        {
          return nullptr;
        }
        analysis::TypePath resolved_path;
        resolved_path.reserve(path.size());
        for (const auto &seg : path)
        {
          resolved_path.push_back(seg);
        }
        if (out_path)
        {
          *out_path = resolved_path;
        }
        return analysis::LookupModalDecl(scope, resolved_path);
      };
      auto modal_decl_for_payload_value = [&](const DerivedValueInfo &info,
                                              analysis::TypePath *out_path)
          -> const ast::ModalDecl *
      {
        if (const ast::ModalDecl *decl = modal_decl_for_value(info.base, out_path))
        {
          return decl;
        }
        if (!info.static_path.empty())
        {
          if (const ast::ModalDecl *decl =
                  modal_decl_for_static_path(info.static_path, out_path))
          {
            return decl;
          }
        }
        if (const DerivedValueInfo *base_derived = ctx->LookupDerivedValue(info.base))
        {
          if (!base_derived->static_path.empty())
          {
            if (const ast::ModalDecl *decl =
                    modal_decl_for_static_path(base_derived->static_path, out_path))
            {
              return decl;
            }
          }
        }
        return nullptr;
      };
      auto find_modal_state = [](const ast::ModalDecl &decl,
                                 std::string_view state_name) -> const ast::StateBlock *
      {
        for (const auto &state : decl.states)
        {
          if (analysis::IdEq(state.name, std::string(state_name)))
          {
            return &state;
          }
        }
        return nullptr;
      };
      struct ModalPayloadMemberInfo
      {
        analysis::TypeRef type;
        std::uint64_t offset = 0;
        std::uint64_t payload_size = 0;
        std::uint64_t payload_align = 1;
        bool tagged = true;
        bool ok = false;
      };
      auto modal_payload_member_by_field = [&](const ast::ModalDecl &modal_decl,
                                               const std::vector<analysis::TypeRef> &modal_args,
                                               std::string_view state_name,
                                               std::string_view field_name)
          -> ModalPayloadMemberInfo
      {
        ModalPayloadMemberInfo out;
        analysis::TypeSubst modal_subst;
        if (modal_decl.generic_params && !modal_decl.generic_params->params.empty())
        {
          if (modal_args.size() > modal_decl.generic_params->params.size())
          {
            return out;
          }
          modal_subst = analysis::BuildSubstitution(
              modal_decl.generic_params->params,
              modal_args);
        }
        const auto modal_layout = ::cursive::analysis::layout::ModalLayoutOf(scope, modal_decl, modal_args);
        if (!modal_layout.has_value())
        {
          return out;
        }
        out.payload_size = modal_layout->payload_size;
        out.payload_align = modal_layout->payload_align;
        out.tagged = modal_layout->disc_type.has_value();

        const ast::StateBlock *state = find_modal_state(modal_decl, state_name);
        if (!state)
        {
          return out;
        }

        std::vector<analysis::TypeRef> field_types;
        std::vector<std::string> field_names;
        for (const auto &member : state->members)
        {
          const auto *field = std::get_if<ast::StateFieldDecl>(&member);
          if (!field)
          {
            continue;
          }
          const auto lowered = ::cursive::analysis::layout::LowerTypeForLayout(scope, field->type);
          if (!lowered.has_value())
          {
            return out;
          }
          analysis::TypeRef field_type = *lowered;
          if (!modal_subst.empty())
          {
            field_type = analysis::InstantiateType(field_type, modal_subst);
          }
          field_types.push_back(field_type);
          field_names.push_back(field->name);
        }

        const auto layout = ::cursive::analysis::layout::RecordLayoutOf(scope, field_types);
        if (!layout.has_value())
        {
          return out;
        }
        for (std::size_t i = 0; i < field_names.size() && i < layout->offsets.size(); ++i)
        {
          if (analysis::IdEq(field_names[i], std::string(field_name)))
          {
            out.type = field_types[i];
            out.offset = layout->offsets[i];
            out.ok = true;
            break;
          }
        }
        return out;
      };
      auto load_modal_payload_member = [&](llvm::Value *modal_value,
                                           const ModalPayloadMemberInfo &member) -> llvm::Value *
      {
        if (!modal_value || !member.ok || !member.type)
        {
          return nullptr;
        }
        llvm::Type *member_ty = GetLLVMType(member.type);
        if (!member_ty)
        {
          return nullptr;
        }
        llvm::Function *current_fn =
            builder->GetInsertBlock() ? builder->GetInsertBlock()->getParent() : nullptr;
        if (!current_fn)
        {
          return nullptr;
        }
        llvm::IRBuilder<> entry_builder(
            &current_fn->getEntryBlock(),
            current_fn->getEntryBlock().begin());
        llvm::AllocaInst *modal_slot = entry_builder.CreateAlloca(modal_value->getType());
        builder->CreateStore(modal_value, modal_slot);

        llvm::Type *i8_ty = llvm::Type::getInt8Ty(context_);
        llvm::Type *i64_ty = llvm::Type::getInt64Ty(context_);
        llvm::Value *payload_i8 = nullptr;
        if (member.tagged)
        {
          auto *modal_ty = llvm::dyn_cast<llvm::StructType>(modal_value->getType());
          if (!modal_ty || modal_ty->getNumElements() < 2)
          {
            return nullptr;
          }
          payload_i8 = CreateTaggedPayloadI8Ptr(
              *this,
              builder,
              modal_ty,
              modal_slot,
              member.payload_align);
        }
        else
        {
          payload_i8 = builder->CreateBitCast(
              modal_slot,
              llvm::PointerType::get(i8_ty, 0));
        }
        if (!payload_i8)
        {
          return nullptr;
        }
        llvm::Value *field_i8 = builder->CreateGEP(
            i8_ty,
            payload_i8,
            llvm::ConstantInt::get(i64_ty, member.offset));
        llvm::Value *field_ptr = builder->CreateBitCast(
            field_i8,
            llvm::PointerType::get(member_ty, 0));
        llvm::LoadInst *load = builder->CreateLoad(member_ty, field_ptr);
        load->setAlignment(llvm::Align(1));
        return load;
      };
      switch (derived->kind)
      {
      case DerivedValueInfo::Kind::Field:
      {
        auto meta = field_meta_for(derived->base, derived->field);
        auto load_field_from_ptr = [&](llvm::Value *base_ptr,
                                       const FieldAccessMeta &field_meta)
            -> llvm::Value *
        {
          if (!base_ptr || !base_ptr->getType()->isPointerTy() ||
              !field_meta.field_type ||
              field_meta.index >= field_meta.aggregate_fields.size())
          {
            return nullptr;
          }
          const auto layout = ::cursive::analysis::layout::RecordLayoutOf(
              scope, field_meta.aggregate_fields, field_meta.layout_options);
          if (!layout.has_value() || field_meta.index >= layout->offsets.size())
          {
            return nullptr;
          }
          llvm::Type *field_ll = GetLLVMType(field_meta.field_type);
          if (!field_ll || field_ll->isVoidTy())
          {
            return nullptr;
          }

          llvm::Value *base_i8 = builder->CreateBitCast(
              base_ptr, llvm::PointerType::get(llvm::Type::getInt8Ty(context_), 0));
          llvm::Value *field_i8 = builder->CreateGEP(
              llvm::Type::getInt8Ty(context_),
              base_i8,
              llvm::ConstantInt::get(
                  llvm::Type::getInt64Ty(context_),
                  layout->offsets[field_meta.index]));
          llvm::Value *field_ptr = builder->CreateBitCast(
              field_i8, llvm::PointerType::get(field_ll, 0));
          llvm::LoadInst *load = builder->CreateLoad(field_ll, field_ptr);
          load->setAlignment(llvm::Align(1));
          return load;
        };
        auto load_field_by_offset = [&](llvm::Value *base_value,
                                        const FieldAccessMeta &field_meta)
            -> llvm::Value *
        {
          if (!base_value)
          {
            return nullptr;
          }
          if (base_value->getType()->isPointerTy())
          {
            return load_field_from_ptr(base_value, field_meta);
          }

          llvm::Function *current_fn =
              builder->GetInsertBlock() ? builder->GetInsertBlock()->getParent() : nullptr;
          if (!current_fn)
          {
            return nullptr;
          }
          llvm::IRBuilder<> entry_builder(
              &current_fn->getEntryBlock(),
              current_fn->getEntryBlock().begin());
          llvm::AllocaInst *base_slot = entry_builder.CreateAlloca(base_value->getType());
          builder->CreateStore(base_value, base_slot);
          return load_field_from_ptr(base_slot, field_meta);
        };

        // Field metadata provides semantic field order (excluding ABI padding).
        // Materialize by byte offset instead of aggregate index to avoid reading
        // synthetic padding members from LLVM struct representations.
        if (meta.has_value())
        {
          if (llvm::Value *base_storage = GetAddressableStorage(derived->base))
          {
            if (llvm::Value *by_offset = load_field_from_ptr(base_storage, *meta))
            {
              materialized = by_offset;
              break;
            }
          }
        }

        llvm::Value *base = EvaluateIRValue(derived->base);
        if (!base)
        {
          break;
        }

        if (meta.has_value())
        {
          if (llvm::Value *by_offset = load_field_by_offset(base, *meta))
          {
            materialized = by_offset;
            break;
          }
        }

        if (auto *struct_ty = llvm::dyn_cast<llvm::StructType>(base->getType()))
        {
          std::optional<std::size_t> index =
              meta.has_value() ? std::optional<std::size_t>(meta->index) : std::nullopt;
          if (!index.has_value())
          {
            if (auto parsed = ParseTupleFieldIndex(derived->field))
            {
              if (*parsed < struct_ty->getNumElements())
              {
                index = *parsed;
              }
            }
            else if (struct_ty->getNumElements() == 1)
            {
              index = 0;
            }
          }
          if (index.has_value() && *index < struct_ty->getNumElements())
          {
            materialized =
                builder->CreateExtractValue(base, {static_cast<unsigned>(*index)});
          }
        }
        else if (auto *arr_ty = llvm::dyn_cast<llvm::ArrayType>(base->getType()))
        {
          std::optional<std::size_t> index =
              meta.has_value() ? std::optional<std::size_t>(meta->index) : std::nullopt;
          if (!index.has_value())
          {
            if (auto parsed = ParseTupleFieldIndex(derived->field))
            {
              if (*parsed < arr_ty->getNumElements())
              {
                index = *parsed;
              }
            }
            else if (arr_ty->getNumElements() == 1)
            {
              index = 0;
            }
          }
          if (index.has_value() && *index < arr_ty->getNumElements())
          {
            materialized =
                builder->CreateExtractValue(base, {static_cast<unsigned>(*index)});
          }
        }
        break;
      }
      case DerivedValueInfo::Kind::AddrStatic:
      {
        auto configure_imported_static_decl =
            [&](llvm::GlobalVariable *decl,
                const std::string &symbol_name) -> llvm::GlobalVariable * {
          if (!decl) {
            return nullptr;
          }
          if (!ctx || ctx->module_path.empty()) {
            return decl;
          }
          if (project::ObjectFormatOf(target_profile_) != project::ObjectFormat::Coff) {
            return decl;
          }
          const auto *owner_module = ctx->LookupStaticModule(symbol_name);
          if (!owner_module || owner_module->empty()) {
            return decl;
          }
          const std::string &current_root = ctx->module_path.front();
          const std::string &owner_root = owner_module->front();
          const bool imported_shared_library_data =
              owner_root != current_root &&
              ctx->library_assembly_names.contains(owner_root);
          if (!imported_shared_library_data) {
            return decl;
          }
          decl->setDLLStorageClass(llvm::GlobalValue::DLLImportStorageClass);
          return decl;
        };
        std::vector<std::string> symbol_candidates;
        symbol_candidates.reserve(4);
        if (!derived->static_path.empty() && !derived->name.empty())
        {
          if (auto* lower_ctx = current_ctx_;
              lower_ctx && lower_ctx->sigma) {
            if (auto addr =
                    StaticAddr(*lower_ctx->sigma,
                               derived->static_path,
                               derived->name)) {
              symbol_candidates.push_back(addr->name);
            } else {
              symbol_candidates.push_back(
                  StaticSymPath(derived->static_path, derived->name));
            }
          } else {
            symbol_candidates.push_back(
                StaticSymPath(derived->static_path, derived->name));
          }
        }
        if (!derived->name.empty())
        {
          symbol_candidates.push_back(derived->name);
          if (auto alias = LookupSymbolAlias(derived->name))
          {
            symbol_candidates.push_back(*alias);
          }
        }

        std::set<std::string> seen_symbols;
        bool hosted_slot_symbol = false;
        for (const auto &symbol_name : symbol_candidates)
        {
          if (symbol_name.empty() || !seen_symbols.insert(symbol_name).second)
          {
            continue;
          }

          analysis::TypeRef static_type = nullptr;
          if (ctx)
          {
            static_type = analysis::StripPerm(ctx->LookupStaticType(symbol_name));
            if (!static_type)
            {
              static_type = ctx->LookupStaticType(symbol_name);
            }
          }

          llvm::Type *static_ll = static_type ? GetLLVMType(static_type) : nullptr;
          if (!static_ll || static_ll->isVoidTy())
          {
            continue;
          }

          llvm::Value *fallback = GetGlobal(symbol_name);
          if (!fallback)
          {
            fallback = module_->getNamedGlobal(symbol_name);
          }
          if (!fallback && static_ll)
          {
            auto *decl = new llvm::GlobalVariable(
                *module_,
                static_ll,
                false,
                llvm::GlobalValue::ExternalLinkage,
                nullptr,
                symbol_name);
            fallback = configure_imported_static_decl(decl, symbol_name);
          }
          else if (auto *global_decl = llvm::dyn_cast<llvm::GlobalVariable>(fallback))
          {
            fallback = configure_imported_static_decl(global_decl, symbol_name);
          }

          llvm::Value *ptr = GetHostedStatePtr(symbol_name, static_ll, fallback);
          if (!ptr && fallback)
          {
            ptr = CoerceTo(builder, fallback, llvm::PointerType::get(static_ll, 0));
            if (!ptr && fallback->getType()->isPointerTy())
            {
              ptr = builder->CreateBitCast(fallback, llvm::PointerType::get(static_ll, 0));
            }
          }
          if (!ptr && HasHostedStateSlot(symbol_name))
          {
            hosted_slot_symbol = true;
          }
          if (ptr)
          {
            materialized = ptr;
            break;
          }
        }

        if (!materialized && hosted_slot_symbol && current_ctx_)
        {
          current_ctx_->ReportCodegenFailure();
        }
        break;
      }
      case DerivedValueInfo::Kind::AddrLocal:
      {
        llvm::Value *local = GetLocalBindStorage(derived->name);
        if (!local)
        {
          if (current_ctx_)
          {
            std::cerr << "[cursive] missing local address storage"
                      << " name=" << derived->name;
            if (module_)
            {
              std::cerr << " module=" << module_->getModuleIdentifier();
            }
            if (builder && builder->GetInsertBlock() &&
                builder->GetInsertBlock()->getParent())
            {
              std::cerr << " function="
                        << builder->GetInsertBlock()->getParent()->getName().str();
            }
            std::cerr << "\n";
            current_ctx_->ReportCodegenFailure();
          }
          break;
        }
        if (auto *alloca = llvm::dyn_cast<llvm::AllocaInst>(local))
        {
          // Panic-out is already a pointer value; load it from the local slot.
          if (derived->name == std::string(kPanicOutName))
          {
            materialized = builder->CreateLoad(alloca->getAllocatedType(), alloca);
          }
          else
          {
            materialized = alloca;
          }
        }
        else
        {
          materialized = local;
        }
        break;
      }
      case DerivedValueInfo::Kind::AddrTuple:
      {
        analysis::TypeRef base_value_type = lookup_value_type(derived->base);
        analysis::TypeRef base_type = pointee_from_type(base_value_type);
        llvm::Value *base = nullptr;
        base = pointer_from_value(EvaluateIRValue(derived->base));
        if (!base)
        {
          base = GetAddressableStorage(derived->base);
        }
        if (!base)
        {
          break;
        }
        analysis::TypeRef elem_type = nullptr;
        std::optional<std::uint64_t> field_offset = derived->byte_offset;
        if (base_type)
        {
          if (const auto *tup = std::get_if<analysis::TypeTuple>(&base_type->node))
          {
            if (derived->tuple_index < tup->elements.size())
            {
              elem_type = tup->elements[derived->tuple_index];
              if (!field_offset.has_value())
              {
                const analysis::ScopeContext &scope = BuildScope(ctx);
                if (const auto layout = ::cursive::analysis::layout::RecordLayoutOf(scope, tup->elements))
                {
                  if (derived->tuple_index < layout->offsets.size())
                  {
                    field_offset = layout->offsets[derived->tuple_index];
                  }
                }
              }
            }
          }
        }
        if (!elem_type)
        {
          elem_type = pointee_from_type(lookup_value_type(val));
        }
        if (!field_offset.has_value())
        {
          break;
        }
        llvm::Value *base_i8 = builder->CreateBitCast(
            base,
            llvm::PointerType::get(llvm::Type::getInt8Ty(context_), 0));
        llvm::Value *field_ptr = builder->CreateGEP(
            llvm::Type::getInt8Ty(context_),
            base_i8,
            llvm::ConstantInt::get(llvm::Type::getInt64Ty(context_), *field_offset));
        if (elem_type)
        {
          if (llvm::Type *elem_ll = GetLLVMType(elem_type))
          {
            field_ptr = builder->CreateBitCast(
                field_ptr,
                llvm::PointerType::get(elem_ll, 0));
          }
        }
        materialized = field_ptr;
        break;
      }
      case DerivedValueInfo::Kind::AddrIndex:
      {
        llvm::Type *i64_ty = llvm::Type::getInt64Ty(context_);
        const bool debug_addr_index = core::IsDebugEnabled("obj");
        auto runtime_range = derived->range_value.has_value()
                                 ? materialize_range_value(*derived->range_value,
                                                           i64_ty,
                                                           derived->range.kind)
                                 : std::nullopt;

        llvm::Value *base_storage = GetAddressableStorage(derived->base);
        llvm::Value *base_value = nullptr;
        llvm::Value *base_ptr = base_storage;
        if (!base_ptr)
        {
          base_value = EvaluateIRValue(derived->base);
          base_ptr = pointer_from_value(base_value);
        }
        if (!base_ptr)
        {
          break;
        }

        llvm::Value *index = EvaluateIRValue(derived->index);
        if ((!index || !index->getType()->isIntegerTy()) &&
            runtime_range.has_value() && runtime_range->lo)
        {
          index = runtime_range->lo;
        }
        if ((!index || !index->getType()->isIntegerTy()) &&
            derived->range.lo.has_value())
        {
          index = EvaluateIRValue(*derived->range.lo);
        }
        if (!index || !index->getType()->isIntegerTy())
        {
          index = llvm::ConstantInt::get(i64_ty, 0);
        }
        if (index->getType()->getIntegerBitWidth() != 64)
        {
          index = builder->CreateIntCast(index, i64_ty, false);
        }

        analysis::TypeRef base_type = pointee_from_type(lookup_value_type(derived->base));
        analysis::TypeRef elem_type = nullptr;
        if (base_type)
        {
          if (const auto *arr = std::get_if<analysis::TypeArray>(&base_type->node))
          {
            elem_type = arr->element;
          }
          else if (const auto *slice = std::get_if<analysis::TypeSlice>(&base_type->node))
          {
            elem_type = slice->element;
          }
        }
        if (!elem_type)
        {
          elem_type = lookup_value_type(val);
        }
        if (!elem_type)
        {
          break;
        }

        llvm::Type *elem_ll = GetLLVMType(elem_type);
        if (!elem_ll)
        {
          break;
        }

        llvm::Value *elem_base_ptr = base_ptr;
        bool used_slice_data_ptr = false;
        if (base_type && std::holds_alternative<analysis::TypeSlice>(base_type->node))
        {
          if (!base_value)
          {
            base_value = EvaluateIRValue(derived->base);
          }
          llvm::Value *data_ptr = nullptr;

          if (base_value && base_value->getType()->isStructTy())
          {
            data_ptr = builder->CreateExtractValue(base_value, {0u});
          }
          else
          {
            llvm::Type *slice_ll = GetLLVMType(base_type);
            if (slice_ll && base_ptr->getType()->isPointerTy())
            {
              llvm::Value *typed_slice_ptr =
                  builder->CreateBitCast(base_ptr, llvm::PointerType::get(slice_ll, 0));
              llvm::Value *loaded_slice = builder->CreateLoad(slice_ll, typed_slice_ptr);
              if (loaded_slice && loaded_slice->getType()->isStructTy())
              {
                data_ptr = builder->CreateExtractValue(loaded_slice, {0u});
              }
            }
          }

          llvm::Value *coerced = pointer_from_value(data_ptr);
          if (coerced)
          {
            elem_base_ptr = coerced;
            used_slice_data_ptr = true;
          }
        }

        if (debug_addr_index)
        {
          llvm::Function *fn =
              builder->GetInsertBlock() ? builder->GetInsertBlock()->getParent() : nullptr;
          std::cerr << "[cursive] AddrIndex materialize in "
                    << (fn ? fn->getName().str() : std::string("<no-func>"))
                    << " result=" << val.name
                    << " base="
                    << (lookup_value_type(derived->base)
                            ? analysis::TypeToString(lookup_value_type(derived->base))
                            : std::string("<null>"))
                    << " base_pointee="
                    << (base_type ? analysis::TypeToString(base_type) : std::string("<null>"))
                    << " elem="
                    << (elem_type ? analysis::TypeToString(elem_type) : std::string("<null>"))
                    << " index_kind="
                    << (derived->index.kind == IRValue::Kind::Immediate ? "imm"
                                                                        : "non-imm")
                    << " used_slice_data_ptr=" << (used_slice_data_ptr ? "true" : "false");
          if (auto *idx_const = llvm::dyn_cast<llvm::ConstantInt>(index))
          {
            std::cerr << " index=" << idx_const->getZExtValue();
          }
          std::cerr << "\n";
        }

        llvm::Value *elem_ptr = builder->CreateBitCast(
            elem_base_ptr, llvm::PointerType::get(elem_ll, 0));
        materialized = builder->CreateGEP(elem_ll, elem_ptr, index);
        break;
      }
      case DerivedValueInfo::Kind::AddrField:
      {
        llvm::Value *base = GetAddressableStorage(derived->base);
        if (!base)
        {
          base = pointer_from_value(EvaluateIRValue(derived->base));
        }
        if (!base)
        {
          break;
        }
        auto meta = field_meta_for(derived->base, derived->field);
        std::optional<std::uint64_t> field_offset;
        analysis::TypeRef field_type = nullptr;
        if (meta.has_value() && meta->index < meta->aggregate_fields.size())
        {
          if (auto layout = ::cursive::analysis::layout::RecordLayoutOf(
                  scope, meta->aggregate_fields, meta->layout_options))
          {
            if (meta->index < layout->offsets.size())
            {
              field_offset = layout->offsets[meta->index];
              field_type = meta->field_type;
            }
          }
        }
        if (!field_offset.has_value())
        {
          // Fallback for unresolved record metadata: single-field aggregate at offset 0.
          if (auto parsed = ParseTupleFieldIndex(derived->field))
          {
            if (*parsed == 0)
            {
              field_offset = 0;
            }
          }
          else
          {
            field_offset = 0;
          }
        }
        if (!field_offset.has_value())
        {
          break;
        }

        llvm::Value *base_i8 = builder->CreateBitCast(
            base, llvm::PointerType::get(llvm::Type::getInt8Ty(context_), 0));
        llvm::Value *field_ptr = builder->CreateGEP(
            llvm::Type::getInt8Ty(context_),
            base_i8,
            llvm::ConstantInt::get(
                llvm::Type::getInt64Ty(context_), *field_offset));
        if (field_type)
        {
          if (llvm::Type *elem_ll = GetLLVMType(field_type))
          {
            field_ptr = builder->CreateBitCast(
                field_ptr, llvm::PointerType::get(elem_ll, 0));
          }
        }
        materialized = field_ptr;
        break;
      }
      case DerivedValueInfo::Kind::AddrDeref:
      {
        materialized = pointer_from_value(EvaluateIRValue(derived->base));
        break;
      }
      case DerivedValueInfo::Kind::LoadFromAddr:
      {
        llvm::Value *base_ptr = pointer_from_value(EvaluateIRValue(derived->base));
        if (!base_ptr)
        {
          break;
        }

        analysis::TypeRef load_type = lookup_value_type(val);
        llvm::Type *load_llvm_ty = load_type ? GetLLVMType(load_type) : nullptr;
        if (!load_llvm_ty || load_llvm_ty->isVoidTy())
        {
          break;
        }

        llvm::Value *typed_ptr = builder->CreateBitCast(
            base_ptr,
            llvm::PointerType::get(load_llvm_ty, 0));
        materialized = builder->CreateLoad(load_llvm_ty, typed_ptr);
        break;
      }
      case DerivedValueInfo::Kind::Tuple:
      {
        llvm::Value *base = EvaluateIRValue(derived->base);
        const bool debug_tuple = core::IsDebugEnabled("obj");
        if (auto *struct_ty = base ? llvm::dyn_cast<llvm::StructType>(base->getType()) : nullptr)
        {
          if (derived->tuple_index < struct_ty->getNumElements())
          {
            materialized = builder->CreateExtractValue(base, {static_cast<unsigned>(derived->tuple_index)});
          }
        }
        else if (llvm::Value *base_ptr = pointer_from_value(base))
        {
          analysis::TypeRef base_type = pointee_from_type(lookup_value_type(derived->base));
          const auto *tup = base_type ? std::get_if<analysis::TypeTuple>(&base_type->node) : nullptr;
          if (tup && derived->tuple_index < tup->elements.size())
          {
            analysis::TypeRef elem_type = tup->elements[derived->tuple_index];
            if (const auto layout = ::cursive::analysis::layout::RecordLayoutOf(scope, tup->elements))
            {
              if (derived->tuple_index < layout->offsets.size())
              {
                const std::uint64_t field_offset = layout->offsets[derived->tuple_index];
                llvm::Value *base_i8 = builder->CreateBitCast(
                    base_ptr,
                    llvm::PointerType::get(llvm::Type::getInt8Ty(context_), 0));
                llvm::Value *field_i8 = builder->CreateGEP(
                    llvm::Type::getInt8Ty(context_),
                    base_i8,
                    llvm::ConstantInt::get(
                        llvm::Type::getInt64Ty(context_),
                        field_offset));
                if (llvm::Type *elem_ll = GetLLVMType(elem_type))
                {
                  llvm::Value *field_ptr = builder->CreateBitCast(
                      field_i8,
                      llvm::PointerType::get(elem_ll, 0));
                  materialized = builder->CreateLoad(elem_ll, field_ptr);
                }
              }
            }
          }
        }
        if (materialized && debug_tuple)
        {
          llvm::Function *fn =
              builder->GetInsertBlock() ? builder->GetInsertBlock()->getParent() : nullptr;
          std::cerr << "[cursive] tuple materialized in "
                    << (fn ? fn->getName().str() : std::string("<no-func>"))
                    << " tuple_index=" << derived->tuple_index
                    << " base_kind="
                    << (base ? (base->getType()->isPointerTy()
                                    ? std::string("ptr")
                                    : (base->getType()->isStructTy() ? std::string("struct")
                                                                     : std::string("non-ptr")))
                             : std::string("<null>"))
                    << " result_kind="
                    << (materialized->getType()->isPointerTy()
                            ? std::string("ptr")
                            : (materialized->getType()->isStructTy()
                                   ? std::string("struct")
                                   : std::string("non-ptr")))
                    << "\n";
        }
        if (!materialized && core::IsDebugEnabled("obj"))
        {
          llvm::Function *fn =
              builder->GetInsertBlock() ? builder->GetInsertBlock()->getParent() : nullptr;
          analysis::TypeRef raw_base_ty = lookup_value_type(derived->base);
          analysis::TypeRef norm_base_ty = pointee_from_type(raw_base_ty);
          std::cerr << "[cursive] tuple materialization failed in "
                    << (fn ? fn->getName().str() : std::string("<no-func>"))
                    << " tuple_index=" << derived->tuple_index
                    << " base_type="
                    << (raw_base_ty ? analysis::TypeToString(raw_base_ty) : std::string("<null>"))
                    << " normalized_base_type="
                    << (norm_base_ty ? analysis::TypeToString(norm_base_ty) : std::string("<null>"))
                    << " base_value="
                    << (base ? (base->getType()->isPointerTy()
                                    ? std::string("ptr")
                                    : (base->getType()->isStructTy() ? std::string("struct")
                                                                     : std::string("non-ptr")))
                             : std::string("<null>"))
                    << "\n";
        }
        break;
      }
      case DerivedValueInfo::Kind::Slice:
      {
        llvm::Type *i64_ty = llvm::Type::getInt64Ty(context_);
        llvm::Value *base = EvaluateIRValue(derived->base);
        if (!base)
        {
          break;
        }

        analysis::TypeRef base_type = strip_perm(lookup_value_type(derived->base));
        analysis::TypeRef slice_type = strip_perm(lookup_value_type(val));
        analysis::TypeRef elem_type = nullptr;
        if (const auto *slice = slice_type ? std::get_if<analysis::TypeSlice>(&slice_type->node)
                                           : nullptr)
        {
          elem_type = slice->element;
        }
        if (!elem_type && base_type)
        {
          if (const auto *arr = std::get_if<analysis::TypeArray>(&base_type->node))
          {
            elem_type = arr->element;
          }
          else if (const auto *slice = std::get_if<analysis::TypeSlice>(&base_type->node))
          {
            elem_type = slice->element;
          }
        }
        if (!elem_type)
        {
          break;
        }

        if (!slice_type)
        {
          slice_type = analysis::MakeTypeSlice(elem_type);
        }
        llvm::Type *slice_ll = slice_type ? GetLLVMType(slice_type) : nullptr;
        auto *slice_struct_ty = llvm::dyn_cast_or_null<llvm::StructType>(slice_ll);
        if (!slice_struct_ty || slice_struct_ty->getNumElements() < 2)
        {
          break;
        }

        llvm::Value *base_len = nullptr;
        if (const auto *arr = base_type ? std::get_if<analysis::TypeArray>(&base_type->node)
                                        : nullptr)
        {
          base_len = llvm::ConstantInt::get(i64_ty, static_cast<std::uint64_t>(arr->length));
        }
        else if (auto *arr_ty = llvm::dyn_cast<llvm::ArrayType>(base->getType()))
        {
          base_len =
              llvm::ConstantInt::get(i64_ty, static_cast<std::uint64_t>(arr_ty->getNumElements()));
        }
        else if (base_type && std::holds_alternative<analysis::TypeSlice>(base_type->node))
        {
          if (base->getType()->isStructTy())
          {
            base_len = builder->CreateExtractValue(base, {1u});
          }
          else if (base->getType()->isPointerTy())
          {
            base_len = EmitIndexLenFromAddr(*this, *builder, base_type, base);
          }
        }
        if (!base_len || !base_len->getType()->isIntegerTy())
        {
          break;
        }
        if (base_len->getType()->getIntegerBitWidth() != 64)
        {
          base_len = builder->CreateIntCast(base_len, i64_ty, false);
        }

        auto bound_or = [&](const std::optional<IRValue> &bound_opt,
                            std::uint64_t default_value) -> llvm::Value *
        {
          if (!bound_opt.has_value())
          {
            return llvm::ConstantInt::get(i64_ty, default_value);
          }
          llvm::Value *bound = EvaluateIRValue(*bound_opt);
          if (!bound || !bound->getType()->isIntegerTy())
          {
            return nullptr;
          }
          if (bound->getType()->getIntegerBitWidth() != 64)
          {
            bound = builder->CreateIntCast(bound, i64_ty, false);
          }
          return bound;
        };

        llvm::Value *start = nullptr;
        llvm::Value *end = nullptr;
        auto runtime_range = derived->range_value.has_value()
                                 ? materialize_range_value(*derived->range_value,
                                                           i64_ty,
                                                           derived->range.kind)
                                 : std::nullopt;
        if (runtime_range.has_value())
        {
          switch (runtime_range->kind)
          {
          case IRRangeKind::Full:
            start = llvm::ConstantInt::get(i64_ty, 0);
            end = base_len;
            break;
          case IRRangeKind::From:
            start = runtime_range->lo
                        ? runtime_range->lo
                        : llvm::ConstantInt::get(i64_ty, 0);
            end = base_len;
            break;
          case IRRangeKind::To:
            start = llvm::ConstantInt::get(i64_ty, 0);
            end = runtime_range->hi
                      ? runtime_range->hi
                      : llvm::ConstantInt::get(i64_ty, 0);
            break;
          case IRRangeKind::ToInclusive:
          {
            start = llvm::ConstantInt::get(i64_ty, 0);
            llvm::Value *hi = runtime_range->hi
                                  ? runtime_range->hi
                                  : llvm::ConstantInt::get(i64_ty, 0);
            end = hi ? builder->CreateAdd(hi, llvm::ConstantInt::get(i64_ty, 1)) : nullptr;
            break;
          }
          case IRRangeKind::Exclusive:
            start = runtime_range->lo
                        ? runtime_range->lo
                        : llvm::ConstantInt::get(i64_ty, 0);
            end = runtime_range->hi
                      ? runtime_range->hi
                      : llvm::ConstantInt::get(i64_ty, 0);
            break;
          case IRRangeKind::Inclusive:
          {
            start = runtime_range->lo
                        ? runtime_range->lo
                        : llvm::ConstantInt::get(i64_ty, 0);
            llvm::Value *hi = runtime_range->hi
                                  ? runtime_range->hi
                                  : llvm::ConstantInt::get(i64_ty, 0);
            end = hi ? builder->CreateAdd(hi, llvm::ConstantInt::get(i64_ty, 1)) : nullptr;
            break;
          }
          }
        }
        else
        {
          switch (derived->range.kind)
          {
          case IRRangeKind::Full:
            start = llvm::ConstantInt::get(i64_ty, 0);
            end = base_len;
            break;
          case IRRangeKind::From:
            start = bound_or(derived->range.lo, 0);
            end = base_len;
            break;
          case IRRangeKind::To:
            start = llvm::ConstantInt::get(i64_ty, 0);
            end = bound_or(derived->range.hi, 0);
            break;
          case IRRangeKind::ToInclusive:
          {
            start = llvm::ConstantInt::get(i64_ty, 0);
            llvm::Value *hi = bound_or(derived->range.hi, 0);
            end = hi ? builder->CreateAdd(hi, llvm::ConstantInt::get(i64_ty, 1)) : nullptr;
            break;
          }
          case IRRangeKind::Exclusive:
            start = bound_or(derived->range.lo, 0);
            end = bound_or(derived->range.hi, 0);
            break;
          case IRRangeKind::Inclusive:
          {
            start = bound_or(derived->range.lo, 0);
            llvm::Value *hi = bound_or(derived->range.hi, 0);
            end = hi ? builder->CreateAdd(hi, llvm::ConstantInt::get(i64_ty, 1)) : nullptr;
            break;
          }
          }
        }
        if (!start || !end)
        {
          break;
        }

        llvm::Type *elem_ll = GetLLVMType(elem_type);
        if (!elem_ll)
        {
          break;
        }

        llvm::Value *base_data_ptr = nullptr;
        if (base_type && std::holds_alternative<analysis::TypeSlice>(base_type->node))
        {
          if (base->getType()->isStructTy())
          {
            base_data_ptr = builder->CreateExtractValue(base, {0u});
          }
          else if (base->getType()->isPointerTy())
          {
            llvm::Type *base_slice_ll = GetLLVMType(base_type);
            if (base_slice_ll)
            {
              llvm::Value *typed_slice_ptr = builder->CreateBitCast(
                  base, llvm::PointerType::get(base_slice_ll, 0));
              llvm::Value *loaded_slice = builder->CreateLoad(base_slice_ll, typed_slice_ptr);
              if (loaded_slice && loaded_slice->getType()->isStructTy())
              {
                base_data_ptr = builder->CreateExtractValue(loaded_slice, {0u});
              }
            }
          }
        }

        if (!base_data_ptr)
        {
          llvm::Value *base_ptr = pointer_from_value(base);
          if (!base_ptr)
          {
            if (auto *arr_ty = llvm::dyn_cast<llvm::ArrayType>(base->getType()))
            {
              llvm::Function *current_fn =
                  builder->GetInsertBlock() ? builder->GetInsertBlock()->getParent() : nullptr;
              if (!current_fn)
              {
                break;
              }
              llvm::IRBuilder<> entry_builder(
                  &current_fn->getEntryBlock(),
                  current_fn->getEntryBlock().begin());
              llvm::AllocaInst *array_slot = entry_builder.CreateAlloca(arr_ty);
              builder->CreateStore(base, array_slot);
              base_ptr = array_slot;
            }
          }
          if (!base_ptr)
          {
            break;
          }
          llvm::Value *elem_base_ptr = builder->CreateBitCast(
              base_ptr, llvm::PointerType::get(elem_ll, 0));
          base_data_ptr = builder->CreateGEP(elem_ll, elem_base_ptr, start);
        }
        else
        {
          llvm::Value *coerced_ptr = pointer_from_value(base_data_ptr);
          if (!coerced_ptr)
          {
            break;
          }
          llvm::Value *elem_base_ptr = builder->CreateBitCast(
              coerced_ptr, llvm::PointerType::get(elem_ll, 0));
          base_data_ptr = builder->CreateGEP(elem_ll, elem_base_ptr, start);
        }

        llvm::Value *slice_ptr = pointer_from_value(base_data_ptr);
        if (!slice_ptr)
        {
          break;
        }
        llvm::Type *slice_ptr_ty = slice_struct_ty->getElementType(0);
        if (slice_ptr->getType() != slice_ptr_ty)
        {
          if (!slice_ptr->getType()->isPointerTy() || !slice_ptr_ty->isPointerTy())
          {
            break;
          }
          slice_ptr = builder->CreateBitCast(slice_ptr, slice_ptr_ty);
        }

        llvm::Value *slice_len = builder->CreateSub(end, start);
        llvm::Type *slice_len_ty = slice_struct_ty->getElementType(1);
        if (slice_len->getType() != slice_len_ty)
        {
          if (!slice_len_ty->isIntegerTy())
          {
            break;
          }
          slice_len = builder->CreateIntCast(slice_len, slice_len_ty, false);
        }

        llvm::Value *slice_value = llvm::UndefValue::get(slice_struct_ty);
        slice_value = builder->CreateInsertValue(slice_value, slice_ptr, {0u});
        slice_value = builder->CreateInsertValue(slice_value, slice_len, {1u});
        materialized = slice_value;
        break;
      }
      case DerivedValueInfo::Kind::Index:
      {
        llvm::Type *i64_ty = llvm::Type::getInt64Ty(context_);
        llvm::Value *base = EvaluateIRValue(derived->base);
        llvm::Value *index = EvaluateIRValue(derived->index);
        if (!base || !index || !index->getType()->isIntegerTy())
        {
          break;
        }
        if (index->getType()->getIntegerBitWidth() != 64)
        {
          index = builder->CreateIntCast(index, i64_ty, false);
        }

        analysis::TypeRef base_type = strip_perm(lookup_value_type(derived->base));
        analysis::TypeRef elem_type = lookup_value_type(val);
        if (!elem_type && base_type)
        {
          if (const auto *arr = std::get_if<analysis::TypeArray>(&base_type->node))
          {
            elem_type = arr->element;
          }
          else if (const auto *slice = std::get_if<analysis::TypeSlice>(&base_type->node))
          {
            elem_type = slice->element;
          }
        }
        llvm::Type *elem_ll = elem_type ? GetLLVMType(elem_type) : nullptr;
        if (!elem_ll)
        {
          break;
        }

        if (auto *arr_ty = llvm::dyn_cast<llvm::ArrayType>(base->getType()))
        {
          if (auto *idx_const = llvm::dyn_cast<llvm::ConstantInt>(index))
          {
            const std::uint64_t idx = idx_const->getZExtValue();
            if (idx < arr_ty->getNumElements())
            {
              materialized = builder->CreateExtractValue(
                  base, {static_cast<unsigned>(idx)});
              break;
            }
          }
          llvm::Function *current_fn =
              builder->GetInsertBlock() ? builder->GetInsertBlock()->getParent() : nullptr;
          if (!current_fn)
          {
            break;
          }
          llvm::IRBuilder<> entry_builder(
              &current_fn->getEntryBlock(),
              current_fn->getEntryBlock().begin());
          llvm::AllocaInst *array_slot = entry_builder.CreateAlloca(arr_ty);
          builder->CreateStore(base, array_slot);
          llvm::Value *elem_ptr = builder->CreateGEP(
              arr_ty,
              array_slot,
              {llvm::ConstantInt::get(i64_ty, 0), index});
          materialized = builder->CreateLoad(elem_ll, elem_ptr);
          break;
        }

        if (base_type && std::holds_alternative<analysis::TypeSlice>(base_type->node) &&
            base->getType()->isStructTy())
        {
          llvm::Value *data_ptr = builder->CreateExtractValue(base, {0u});
          llvm::Value *coerced = pointer_from_value(data_ptr);
          if (!coerced)
          {
            break;
          }
          llvm::Value *elem_base_ptr = builder->CreateBitCast(
              coerced, llvm::PointerType::get(elem_ll, 0));
          llvm::Value *elem_ptr = builder->CreateGEP(
              elem_ll, elem_base_ptr, index);
          materialized = builder->CreateLoad(elem_ll, elem_ptr);
          break;
        }

        llvm::Value *base_ptr = pointer_from_value(base);
        if (!base_ptr)
        {
          break;
        }
        llvm::Value *elem_base_ptr = builder->CreateBitCast(
            base_ptr, llvm::PointerType::get(elem_ll, 0));
        llvm::Value *elem_ptr = builder->CreateGEP(
            elem_ll, elem_base_ptr, index);
        materialized = builder->CreateLoad(elem_ll, elem_ptr);
        break;
      }
      case DerivedValueInfo::Kind::UnionPayload:
      {
        analysis::TypeRef union_type = strip_perm(lookup_value_type(derived->base));
        const auto *uni = union_type ? std::get_if<analysis::TypeUnion>(&union_type->node) : nullptr;
        if (!uni)
        {
          break;
        }
        const auto layout = ::cursive::analysis::layout::UnionLayoutOf(scope, *uni);
        if (!layout.has_value())
        {
          break;
        }
        if (derived->union_index >= layout->member_list.size())
        {
          break;
        }

        analysis::TypeRef member_type = layout->member_list[derived->union_index];
        if (!member_type)
        {
          member_type = lookup_value_type(val);
        }

        llvm::Value *base = EvaluateIRValue(derived->base);
        if (!base || !member_type)
        {
          break;
        }

        analysis::TypeRef stripped_member = analysis::StripPerm(member_type);
        if (stripped_member &&
            std::holds_alternative<analysis::TypePrim>(stripped_member->node) &&
            std::get<analysis::TypePrim>(stripped_member->node).name == "()")
        {
          if (llvm::Type *unit_ty = GetLLVMType(member_type))
          {
            materialized = llvm::Constant::getNullValue(unit_ty);
          }
          break;
        }

        if (layout->niche)
        {
          if (llvm::Type *member_ty = GetLLVMType(member_type))
          {
            materialized = CoerceTo(builder, base, member_ty);
          }
          else
          {
            materialized = base;
          }
          break;
        }

        auto *union_ty = llvm::dyn_cast<llvm::StructType>(base->getType());
        if (!union_ty || union_ty->getNumElements() < 2)
        {
          break;
        }
        llvm::Type *member_ty = GetLLVMType(member_type);
        if (!member_ty)
        {
          break;
        }
        llvm::Function *current_fn =
            builder->GetInsertBlock() ? builder->GetInsertBlock()->getParent() : nullptr;
        if (!current_fn)
        {
          break;
        }
        llvm::IRBuilder<> entry_builder(
            &current_fn->getEntryBlock(),
            current_fn->getEntryBlock().begin());
        llvm::AllocaInst *union_slot = entry_builder.CreateAlloca(union_ty);
        builder->CreateStore(base, union_slot);
        llvm::Value *payload_i8 = CreateTaggedPayloadI8Ptr(
            *this,
            builder,
            union_ty,
            union_slot,
            layout->payload_align);
        if (!payload_i8)
        {
          break;
        }
        llvm::Value *field_ptr = builder->CreateBitCast(
            payload_i8, llvm::PointerType::get(member_ty, 0));
        llvm::LoadInst *load = builder->CreateLoad(member_ty, field_ptr);
        load->setAlignment(llvm::Align(1));
        materialized = load;
        break;
      }
      case DerivedValueInfo::Kind::EnumPayloadIndex:
      {
        analysis::TypePath enum_path;
        const ast::EnumDecl *enum_decl =
            enum_decl_for_payload_value(*derived, &enum_path);
        EnumPayloadMemberInfo member;
        if (enum_decl)
        {
          if (const ast::VariantDecl *variant = find_enum_variant(*enum_decl, derived->variant))
          {
            member = enum_payload_member_by_index(*enum_decl, *variant, derived->tuple_index);
          }
        }
        if (!member.ok)
        {
          member.type = lookup_value_type(val);
          member.offset = 0;
          member.ok = member.type != nullptr;
          if (enum_decl)
          {
            if (const auto enum_layout = ::cursive::analysis::layout::EnumLayoutOf(
                    scope,
                    *enum_decl,
                    ::cursive::analysis::layout::ResolveEnumLayoutOptions(enum_decl->attrs)))
            {
              member.payload_size = enum_layout->payload_size;
              member.payload_align = enum_layout->payload_align;
            }
          }
        }
        llvm::Value *base = EvaluateIRValue(derived->base);
        materialized = load_enum_payload_member(base, member);
        if (core::IsDebugEnabled("obj") && DebugTargetEnumPath(enum_path))
        {
          std::cerr << "[enum-payload-index] path=" << core::StringOfPath(enum_path)
                    << " variant=" << derived->variant
                    << " index=" << derived->tuple_index
                    << " base=" << LLVMValueRepr(base)
                    << " member_ok=" << (member.ok ? "yes" : "no")
                    << " materialized=" << LLVMValueRepr(materialized)
                    << "\n";
        }
        break;
      }
      case DerivedValueInfo::Kind::EnumPayloadField:
      {
        analysis::TypePath enum_path;
        const ast::EnumDecl *enum_decl =
            enum_decl_for_payload_value(*derived, &enum_path);
        EnumPayloadMemberInfo member;
        if (enum_decl)
        {
          if (const ast::VariantDecl *variant = find_enum_variant(*enum_decl, derived->variant))
          {
            member = enum_payload_member_by_field(*enum_decl, *variant, derived->field);
          }
        }
        if (!member.ok)
        {
          member.type = lookup_value_type(val);
          member.offset = 0;
          member.ok = member.type != nullptr;
          if (enum_decl)
          {
            if (const auto enum_layout = ::cursive::analysis::layout::EnumLayoutOf(
                    scope,
                    *enum_decl,
                    ::cursive::analysis::layout::ResolveEnumLayoutOptions(enum_decl->attrs)))
            {
              member.payload_size = enum_layout->payload_size;
              member.payload_align = enum_layout->payload_align;
            }
          }
        }
        llvm::Value *base = EvaluateIRValue(derived->base);
        materialized = load_enum_payload_member(base, member);
        if (core::IsDebugEnabled("obj") && DebugTargetEnumPath(enum_path))
        {
          std::cerr << "[enum-payload-field] path=" << core::StringOfPath(enum_path)
                    << " variant=" << derived->variant
                    << " field=" << derived->field
                    << " base=" << LLVMValueRepr(base)
                    << " member_ok=" << (member.ok ? "yes" : "no")
                    << " materialized=" << LLVMValueRepr(materialized)
                    << "\n";
        }
        break;
      }
      case DerivedValueInfo::Kind::ModalField:
      {
        analysis::TypePath modal_path;
        const ast::ModalDecl *modal_decl =
            modal_decl_for_payload_value(*derived, &modal_path);
        analysis::TypeRef base_modal_type = strip_perm(lookup_value_type(derived->base));
        const auto *base_modal_state =
            base_modal_type
                ? std::get_if<analysis::TypeModalState>(&base_modal_type->node)
                : nullptr;
        const auto *base_modal_path =
            base_modal_type
                ? std::get_if<analysis::TypePathType>(&base_modal_type->node)
                : nullptr;
        const bool base_is_modal_state = (base_modal_state != nullptr);
        const bool base_is_async_modal_state =
            base_modal_state && analysis::IsAsyncType(base_modal_type);
        std::vector<analysis::TypeRef> base_modal_args;
        if (base_modal_state)
        {
          base_modal_args = base_modal_state->generic_args;
        }
        else if (base_modal_path)
        {
          base_modal_args = base_modal_path->generic_args;
        }
        ModalPayloadMemberInfo member;
        if (modal_decl)
        {
          member = modal_payload_member_by_field(
              *modal_decl,
              base_modal_args,
              derived->modal_state,
              derived->field);
          if (base_is_modal_state && !base_is_async_modal_state)
          {
            member.tagged = false;
          }
        }
        if (!member.ok)
        {
          member.type = lookup_value_type(val);
          member.offset = 0;
          member.ok = member.type != nullptr;
          if (modal_decl)
          {
            if (const auto modal_layout = ::cursive::analysis::layout::ModalLayoutOf(scope, *modal_decl, base_modal_args))
            {
              member.payload_size = modal_layout->payload_size;
              member.payload_align = modal_layout->payload_align;
              member.tagged = modal_layout->disc_type.has_value();
            }
          }
          if (base_is_modal_state && !base_is_async_modal_state)
          {
            member.tagged = false;
          }
        }
        llvm::Value *base = EvaluateIRValue(derived->base);
        materialized = load_modal_payload_member(base, member);
        break;
      }
      case DerivedValueInfo::Kind::EnumLit:
      {
        analysis::TypePath enum_path;
        analysis::TypeRef enum_type = lookup_value_type(val);
        const ast::EnumDecl *enum_decl = enum_decl_for_type(enum_type, &enum_path);
        if (!enum_decl && !derived->static_path.empty())
        {
          enum_decl = enum_decl_for_static_path(derived->static_path, &enum_path);
        }
        if (!enum_decl)
        {
          break;
        }
        const ast::VariantDecl *variant = find_enum_variant(*enum_decl, derived->variant);
        const auto disc = enum_variant_disc(*enum_decl, derived->variant);
        const auto enum_layout = ::cursive::analysis::layout::EnumLayoutOf(
            scope,
            *enum_decl,
            ::cursive::analysis::layout::ResolveEnumLayoutOptions(enum_decl->attrs));
        if (!variant || !disc.has_value() || !enum_layout.has_value())
        {
          break;
        }

        llvm::Type *enum_ty = enum_type ? GetLLVMType(enum_type) : nullptr;
        if (enum_layout->payload_size == 0)
        {
          if (!enum_ty)
          {
            break;
          }
          llvm::Value *disc_value = nullptr;
          if (auto *disc_int_ty = llvm::dyn_cast<llvm::IntegerType>(enum_ty))
          {
            disc_value = llvm::ConstantInt::get(disc_int_ty, *disc);
          }
          else
          {
            disc_value = CoerceTo(
                builder,
                llvm::ConstantInt::get(llvm::Type::getInt64Ty(context_), *disc),
                enum_ty);
          }
          materialized = disc_value;
          break;
        }
        auto *enum_struct_ty = llvm::dyn_cast_or_null<llvm::StructType>(enum_ty);
        if (!enum_struct_ty || enum_struct_ty->getNumElements() < 2)
        {
          break;
        }
        llvm::Function *current_fn =
            builder->GetInsertBlock() ? builder->GetInsertBlock()->getParent() : nullptr;
        if (!current_fn)
        {
          break;
        }
        llvm::IRBuilder<> entry_builder(
            &current_fn->getEntryBlock(),
            current_fn->getEntryBlock().begin());
        llvm::AllocaInst *enum_slot = entry_builder.CreateAlloca(enum_struct_ty);
        builder->CreateStore(llvm::Constant::getNullValue(enum_struct_ty), enum_slot);

        llvm::Value *disc_ptr = builder->CreateStructGEP(enum_struct_ty, enum_slot, 0);
        llvm::Type *disc_ty = enum_struct_ty->getElementType(0);
        llvm::Value *disc_value = nullptr;
        if (auto *disc_int_ty = llvm::dyn_cast<llvm::IntegerType>(disc_ty))
        {
          disc_value = llvm::ConstantInt::get(disc_int_ty, *disc);
        }
        else
        {
          disc_value = CoerceTo(
              builder,
              llvm::ConstantInt::get(llvm::Type::getInt64Ty(context_), *disc),
              disc_ty);
        }
        if (disc_value)
        {
          builder->CreateStore(disc_value, disc_ptr);
        }

        llvm::Value *payload_base_i8 = CreateTaggedPayloadI8Ptr(
            *this,
            builder,
            enum_struct_ty,
            enum_slot,
            enum_layout->payload_align);

        auto store_payload_value = [&](const EnumPayloadMemberInfo &member, llvm::Value *value)
        {
          if (!payload_base_i8 || !member.ok || !member.type || !value)
          {
            return;
          }
          llvm::Type *member_ty = GetLLVMType(member.type);
          if (!member_ty)
          {
            return;
          }
          value = CoerceTo(builder, value, member_ty);
          if (!value)
          {
            value = llvm::Constant::getNullValue(member_ty);
          }
          llvm::Type *i8_ty = llvm::Type::getInt8Ty(context_);
          llvm::Type *i64_ty = llvm::Type::getInt64Ty(context_);
          llvm::Value *field_i8 = builder->CreateGEP(
              i8_ty,
              payload_base_i8,
              llvm::ConstantInt::get(i64_ty, member.offset));
          llvm::Value *field_ptr = builder->CreateBitCast(
              field_i8,
              llvm::PointerType::get(member_ty, 0));
          llvm::StoreInst *store = builder->CreateStore(value, field_ptr);
          store->setAlignment(llvm::Align(1));
        };

        if (const auto *tuple_payload =
                variant->payload_opt.has_value()
                    ? std::get_if<ast::VariantPayloadTuple>(&*variant->payload_opt)
                    : nullptr)
        {
          const std::size_t count =
              std::min(tuple_payload->elements.size(), derived->payload_elems.size());
          for (std::size_t i = 0; i < count; ++i)
          {
            const auto member = enum_payload_member_by_index(*enum_decl, *variant, i);
            store_payload_value(member, EvaluateIRValue(derived->payload_elems[i]));
          }
        }
        else if (const auto *record_payload =
                     variant->payload_opt.has_value()
                         ? std::get_if<ast::VariantPayloadRecord>(&*variant->payload_opt)
                         : nullptr)
        {
          (void)record_payload;
          for (const auto &[field_name, field_value] : derived->payload_fields)
          {
            const auto member = enum_payload_member_by_field(*enum_decl, *variant, field_name);
            store_payload_value(member, EvaluateIRValue(field_value));
          }
        }

        materialized = builder->CreateLoad(enum_struct_ty, enum_slot);
        break;
      }
      case DerivedValueInfo::Kind::RangeLit:
      {
        analysis::TypeRef range_type = lookup_value_type(val);
        if (!analysis::IsRangeType(range_type))
        {
          break;
        }
        llvm::Type *range_ty = GetLLVMType(range_type);
        auto *range_struct_ty = llvm::dyn_cast<llvm::StructType>(range_ty);
        if (!range_struct_ty)
        {
          break;
        }

        analysis::TypeRef stripped = range_type;
        while (stripped)
        {
          if (const auto *perm = std::get_if<analysis::TypePerm>(&stripped->node))
          {
            stripped = perm->base;
            continue;
          }
          if (const auto *refine = std::get_if<analysis::TypeRefine>(&stripped->node))
          {
            stripped = refine->base;
            continue;
          }
          break;
        }
        if (!stripped)
        {
          break;
        }

        enum class RangeStructShape
        {
          Full,
          OneLower,
          OneUpper,
          TwoBounds
        };
        RangeStructShape shape = RangeStructShape::Full;
        if (std::holds_alternative<analysis::TypeRange>(stripped->node) ||
            std::holds_alternative<analysis::TypeRangeInclusive>(stripped->node))
        {
          shape = RangeStructShape::TwoBounds;
        }
        else if (std::holds_alternative<analysis::TypeRangeFrom>(stripped->node))
        {
          shape = RangeStructShape::OneLower;
        }
        else if (std::holds_alternative<analysis::TypeRangeTo>(stripped->node) ||
                 std::holds_alternative<analysis::TypeRangeToInclusive>(
                     stripped->node))
        {
          shape = RangeStructShape::OneUpper;
        }
        else if (std::holds_alternative<analysis::TypeRangeFull>(stripped->node))
        {
          shape = RangeStructShape::Full;
        }
        else
        {
          break;
        }

        auto eval_bound = [&](const std::optional<IRValue> &bound_opt,
                              llvm::Type *target_ty) -> llvm::Value *
        {
          if (!target_ty)
          {
            return nullptr;
          }
          if (!bound_opt.has_value())
          {
            return llvm::Constant::getNullValue(target_ty);
          }
          llvm::Value *value = EvaluateIRValue(*bound_opt);
          if (!value)
          {
            return llvm::Constant::getNullValue(target_ty);
          }
          if (value->getType()->isPointerTy() && target_ty->isIntegerTy())
          {
            llvm::Type *load_ty = target_ty;
            if (analysis::TypeRef bound_type = lookup_value_type(*bound_opt))
            {
              if (llvm::Type *bound_ll = GetLLVMType(bound_type))
              {
                if (bound_ll->isIntegerTy())
                {
                  load_ty = bound_ll;
                }
              }
            }
            llvm::Value *typed_ptr = value;
            llvm::Type *ptr_to_load_ty = llvm::PointerType::get(load_ty, 0);
            if (typed_ptr->getType() != ptr_to_load_ty)
            {
              typed_ptr = builder->CreateBitCast(typed_ptr, ptr_to_load_ty);
            }
            value = builder->CreateLoad(load_ty, typed_ptr);
          }
          if (value->getType() != target_ty)
          {
            value = CoerceTo(builder, value, target_ty);
          }
          if (!value)
          {
            return llvm::Constant::getNullValue(target_ty);
          }
          return value;
        };

        llvm::Value *out = llvm::Constant::getNullValue(range_struct_ty);
        switch (shape)
        {
        case RangeStructShape::Full:
          materialized = out;
          break;
        case RangeStructShape::OneLower:
        {
          if (range_struct_ty->getNumElements() < 1)
          {
            break;
          }
          llvm::Type *lo_ty = range_struct_ty->getElementType(0);
          llvm::Value *lo = eval_bound(derived->range.lo, lo_ty);
          out = builder->CreateInsertValue(out, lo, {0u});
          materialized = out;
          break;
        }
        case RangeStructShape::OneUpper:
        {
          if (range_struct_ty->getNumElements() < 1)
          {
            break;
          }
          llvm::Type *hi_ty = range_struct_ty->getElementType(0);
          llvm::Value *hi = eval_bound(derived->range.hi, hi_ty);
          out = builder->CreateInsertValue(out, hi, {0u});
          materialized = out;
          break;
        }
        case RangeStructShape::TwoBounds:
        {
          if (range_struct_ty->getNumElements() < 2)
          {
            break;
          }
          llvm::Type *lo_ty = range_struct_ty->getElementType(0);
          llvm::Type *hi_ty = range_struct_ty->getElementType(1);
          llvm::Value *lo = eval_bound(derived->range.lo, lo_ty);
          llvm::Value *hi = eval_bound(derived->range.hi, hi_ty);
          out = builder->CreateInsertValue(out, lo, {0u});
          out = builder->CreateInsertValue(out, hi, {1u});
          materialized = out;
          break;
        }
        }
        break;
      }
      case DerivedValueInfo::Kind::RecordLit:
      {
        llvm::Type *agg_ty = nullptr;
        analysis::TypeRef record_type = lookup_value_type(val);
        if (record_type)
        {
          agg_ty = GetLLVMType(record_type);
        }
        if (!agg_ty || !agg_ty->isStructTy())
        {
          std::vector<llvm::Type *> inferred_field_tys;
          inferred_field_tys.reserve(derived->fields.size());
          for (const auto &[field_name, field_value] : derived->fields)
          {
            (void)field_name;
            llvm::Value *elem = EvaluateIRValue(field_value);
            inferred_field_tys.push_back(
                elem ? elem->getType() : llvm::Type::getInt64Ty(context_));
          }
          agg_ty = llvm::StructType::get(context_, inferred_field_tys);
        }

        auto *struct_ty = llvm::dyn_cast_or_null<llvm::StructType>(agg_ty);
        if (!struct_ty)
        {
          break;
        }

        struct RecordFieldStore
        {
          std::uint64_t offset = 0;
          analysis::TypeRef field_type;
          IRValue value;
        };
        std::vector<RecordFieldStore> offset_fields;
        bool can_use_offset_mode = record_type != nullptr;
        if (can_use_offset_mode)
        {
          for (const auto &[field_name, field_value] : derived->fields)
          {
            auto meta = ResolveFieldAccessMeta(scope, record_type, field_name);
            if (!meta.has_value() || !meta->field_type ||
                meta->index >= meta->aggregate_fields.size())
            {
              can_use_offset_mode = false;
              break;
            }
            const auto layout = ::cursive::analysis::layout::RecordLayoutOf(
                scope, meta->aggregate_fields, meta->layout_options);
            if (!layout.has_value() || meta->index >= layout->offsets.size())
            {
              can_use_offset_mode = false;
              break;
            }
            RecordFieldStore store;
            store.offset = layout->offsets[meta->index];
            store.field_type = meta->field_type;
            store.value = field_value;
            offset_fields.push_back(std::move(store));
          }
        }
        if (can_use_offset_mode)
        {
          llvm::Function *current_fn =
              builder->GetInsertBlock() ? builder->GetInsertBlock()->getParent() : nullptr;
          if (!current_fn)
          {
            break;
          }
          llvm::IRBuilder<> entry_builder(
              &current_fn->getEntryBlock(),
              current_fn->getEntryBlock().begin());
          llvm::AllocaInst *agg_slot = entry_builder.CreateAlloca(struct_ty);
          builder->CreateStore(llvm::Constant::getNullValue(struct_ty), agg_slot);
          llvm::Value *base_i8 = builder->CreateBitCast(
              agg_slot, llvm::PointerType::get(llvm::Type::getInt8Ty(context_), 0));
          for (const auto &field : offset_fields)
          {
            llvm::Type *field_ll = GetLLVMType(field.field_type);
            if (!field_ll || field_ll->isVoidTy())
            {
              continue;
            }
            llvm::Value *elem = EvaluateIRValue(field.value);
            analysis::TypeRef source_type = lookup_value_type(field.value);
            elem = CoerceToTyped(*this,
                                 builder,
                                 elem,
                                 field_ll,
                                 source_type,
                                 field.field_type);
            if (!elem)
            {
              elem = llvm::Constant::getNullValue(field_ll);
            }
            llvm::Value *field_i8 = builder->CreateGEP(
                llvm::Type::getInt8Ty(context_),
                base_i8,
                llvm::ConstantInt::get(llvm::Type::getInt64Ty(context_), field.offset));
            llvm::Value *field_ptr = builder->CreateBitCast(
                field_i8, llvm::PointerType::get(field_ll, 0));
            llvm::StoreInst *store = builder->CreateStore(elem, field_ptr);
            store->setAlignment(llvm::Align(1));
          }
          materialized = builder->CreateLoad(struct_ty, agg_slot);
          break;
        }

        llvm::Value *agg = llvm::Constant::getNullValue(struct_ty);
        std::vector<bool> field_set(struct_ty->getNumElements(), false);

        auto insert_field = [&](std::size_t index, const IRValue &field_value)
        {
          if (index >= struct_ty->getNumElements())
          {
            return;
          }
          llvm::Type *elem_ty = struct_ty->getElementType(static_cast<unsigned>(index));
          llvm::Value *elem = EvaluateIRValue(field_value);
          elem = CoerceTo(builder, elem, elem_ty);
          if (!elem)
          {
            elem = llvm::Constant::getNullValue(elem_ty);
          }
          agg = builder->CreateInsertValue(agg, elem, {static_cast<unsigned>(index)});
          field_set[index] = true;
        };

        for (const auto &[field_name, field_value] : derived->fields)
        {
          std::optional<std::size_t> index;
          if (record_type)
          {
            if (auto meta = ResolveFieldAccessMeta(scope, record_type, field_name))
            {
              index = meta->index;
            }
          }
          if (!index.has_value())
          {
            for (std::size_t i = 0; i < struct_ty->getNumElements(); ++i)
            {
              if (!field_set[i])
              {
                index = i;
                break;
              }
            }
          }
          if (index.has_value())
          {
            insert_field(*index, field_value);
          }
        }

        materialized = agg;
        break;
      }
      case DerivedValueInfo::Kind::DynLit:
      {
        // Materialize the fat pointer struct {data_ptr, vtable_ptr}
        llvm::Value *data_ptr = EvaluateIRValue(derived->base);
        if (data_ptr && !data_ptr->getType()->isPointerTy())
        {
          data_ptr = builder->CreateIntToPtr(data_ptr, GetOpaquePtr());
        }
        if (!data_ptr)
        {
          data_ptr = llvm::ConstantPointerNull::get(
              llvm::cast<llvm::PointerType>(GetOpaquePtr()));
        }

        llvm::Constant *vtable_ptr = nullptr;
        if (!derived->vtable_sym.empty())
        {
          if (llvm::Value *gv = GetGlobal(derived->vtable_sym))
          {
            vtable_ptr = llvm::dyn_cast<llvm::Constant>(gv);
          }
          if (!vtable_ptr)
          {
            if (auto *gv = module_->getNamedGlobal(derived->vtable_sym))
            {
              vtable_ptr = gv;
            }
          }
          if (!vtable_ptr && current_ctx_ && current_ctx_->sigma)
          {
            analysis::TypeRef lazy_vtable_type = derived->dyn_impl_type;
            analysis::TypePath lazy_class_path = derived->dyn_class_path;
            if ((!lazy_vtable_type || lazy_class_path.empty()))
            {
              if (const auto *info =
                      current_ctx_->LookupRequiredVTable(derived->vtable_sym))
              {
                lazy_vtable_type = info->type;
                lazy_class_path = info->class_path;
              }
            }

            auto find_class_decl =
                [&](const analysis::TypePath &class_path)
                    -> const ast::ClassDecl * {
              if (class_path.empty())
              {
                return nullptr;
              }

              auto lookup_decl =
                  [&](const analysis::TypePath &candidate)
                      -> const ast::ClassDecl * {
                ast::Path class_ast_path(candidate.begin(), candidate.end());
                const auto class_it =
                    current_ctx_->sigma->classes.find(analysis::PathKeyOf(class_ast_path));
                if (class_it == current_ctx_->sigma->classes.end())
                {
                  return nullptr;
                }
                return &class_it->second;
              };

              if (const ast::ClassDecl *decl = lookup_decl(class_path))
              {
                return decl;
              }

              analysis::TypePath module_qualified = current_ctx_->module_path;
              module_qualified.insert(
                  module_qualified.end(), class_path.begin(), class_path.end());
              return lookup_decl(module_qualified);
            };

            if (lazy_vtable_type && !lazy_class_path.empty())
            {
              if (const ast::ClassDecl *class_decl = find_class_decl(lazy_class_path))
              {
                GlobalVTable lazy_vtable = ::cursive::codegen::EmitVTable(
                    lazy_vtable_type, lazy_class_path, *class_decl, *current_ctx_);
                EmitVTable(lazy_vtable);
                if (auto *gv = module_->getNamedGlobal(derived->vtable_sym))
                {
                  vtable_ptr = gv;
                }
              }
            }
          }
        }
        if (!vtable_ptr)
        {
          vtable_ptr = llvm::ConstantPointerNull::get(
              llvm::cast<llvm::PointerType>(GetOpaquePtr()));
        }

        // Fat pointer: {ptr, ptr}
        llvm::Type *ptr_ty = GetOpaquePtr();
        llvm::StructType *fat_ptr_ty = llvm::StructType::get(context_, {ptr_ty, ptr_ty});
        llvm::Value *fat = llvm::Constant::getNullValue(fat_ptr_ty);

        llvm::Value *data_as_ptr = data_ptr;
        if (data_as_ptr->getType() != ptr_ty)
        {
          data_as_ptr = builder->CreateBitCast(data_as_ptr, ptr_ty);
        }
        fat = builder->CreateInsertValue(fat, data_as_ptr, {0});

        llvm::Value *vtable_as_ptr = vtable_ptr;
        if (vtable_as_ptr->getType() != ptr_ty)
        {
          vtable_as_ptr = builder->CreateBitCast(vtable_as_ptr, ptr_ty);
        }
        fat = builder->CreateInsertValue(fat, vtable_as_ptr, {1});

        materialized = fat;
        break;
      }
      case DerivedValueInfo::Kind::ArrayLit:
      case DerivedValueInfo::Kind::ArrayRepeat:
      case DerivedValueInfo::Kind::ArraySegments:
      case DerivedValueInfo::Kind::TupleLit:
      {
        llvm::Type *agg_ty = nullptr;
        if (analysis::TypeRef ty = lookup_value_type(val))
        {
          agg_ty = GetLLVMType(ty);
        }
        if (!agg_ty || (!agg_ty->isArrayTy() && !agg_ty->isStructTy()))
        {
          std::vector<llvm::Type *> inferred_elem_tys;
          if (derived->kind == DerivedValueInfo::Kind::ArrayRepeat)
          {
            llvm::Value *elem = EvaluateIRValue(derived->repeat_value);
            llvm::Type *elem_ty =
                elem ? elem->getType() : llvm::Type::getInt64Ty(context_);
            llvm::Value *count_value = EvaluateIRValue(derived->repeat_count);
            auto *count_int = llvm::dyn_cast_or_null<llvm::ConstantInt>(count_value);
            if (!count_int)
            {
              agg_ty = llvm::StructType::get(context_, inferred_elem_tys);
            }
            else
            {
              for (std::uint64_t i = 0; i < count_int->getZExtValue(); ++i)
              {
                inferred_elem_tys.push_back(elem_ty);
              }
            }
          }
          else if (derived->kind == DerivedValueInfo::Kind::ArraySegments)
          {
            for (const auto &segment : derived->array_segments)
            {
              llvm::Value *elem = EvaluateIRValue(segment.value);
              llvm::Type *elem_ty =
                  elem ? elem->getType() : llvm::Type::getInt64Ty(context_);
              if (segment.kind == DerivedArraySegment::Kind::Element)
              {
                inferred_elem_tys.push_back(elem_ty);
                continue;
              }
              if (!segment.count.has_value())
              {
                agg_ty = llvm::StructType::get(context_, inferred_elem_tys);
                break;
              }
              llvm::Value *count_value = EvaluateIRValue(*segment.count);
              auto *count_int = llvm::dyn_cast_or_null<llvm::ConstantInt>(count_value);
              if (!count_int)
              {
                agg_ty = llvm::StructType::get(context_, inferred_elem_tys);
                break;
              }
              for (std::uint64_t i = 0; i < count_int->getZExtValue(); ++i)
              {
                inferred_elem_tys.push_back(elem_ty);
              }
            }
          }
          else
          {
            inferred_elem_tys.reserve(derived->elements.size());
            for (const auto &elem_value : derived->elements)
            {
              llvm::Value *elem = EvaluateIRValue(elem_value);
              llvm::Type *elem_ty =
                  elem ? elem->getType() : llvm::Type::getInt64Ty(context_);
              inferred_elem_tys.push_back(elem_ty);
            }
          }
          llvm::Type *first_elem_ty = nullptr;
          bool all_same = true;
          for (const auto &elem_ty : inferred_elem_tys)
          {
            if (!first_elem_ty)
            {
              first_elem_ty = elem_ty;
            }
            else if (elem_ty != first_elem_ty)
            {
              all_same = false;
            }
          }
          if ((derived->kind == DerivedValueInfo::Kind::ArrayLit ||
               derived->kind == DerivedValueInfo::Kind::ArrayRepeat ||
               derived->kind == DerivedValueInfo::Kind::ArraySegments) &&
              all_same &&
              first_elem_ty)
          {
            agg_ty = llvm::ArrayType::get(
                first_elem_ty,
                static_cast<std::uint64_t>(inferred_elem_tys.size()));
          }
          else
          {
            agg_ty = llvm::StructType::get(context_, inferred_elem_tys);
          }
        }
        if (agg_ty && (agg_ty->isArrayTy() || agg_ty->isStructTy()))
        {
          llvm::Value *agg = llvm::Constant::getNullValue(agg_ty);
          std::vector<IRValue> materialized_elems;
          if (derived->kind == DerivedValueInfo::Kind::ArrayRepeat)
          {
            llvm::Value *count_value = EvaluateIRValue(derived->repeat_count);
            auto *count_int = llvm::dyn_cast_or_null<llvm::ConstantInt>(count_value);
            if (count_int)
            {
              for (std::uint64_t j = 0; j < count_int->getZExtValue(); ++j)
              {
                materialized_elems.push_back(derived->repeat_value);
              }
            }
          }
          else if (derived->kind == DerivedValueInfo::Kind::ArraySegments)
          {
            for (const auto &segment : derived->array_segments)
            {
              if (segment.kind == DerivedArraySegment::Kind::Element)
              {
                materialized_elems.push_back(segment.value);
                continue;
              }
              if (!segment.count.has_value())
              {
                continue;
              }
              llvm::Value *count_value = EvaluateIRValue(*segment.count);
              auto *count_int = llvm::dyn_cast_or_null<llvm::ConstantInt>(count_value);
              if (!count_int)
              {
                continue;
              }
              for (std::uint64_t j = 0; j < count_int->getZExtValue(); ++j)
              {
                materialized_elems.push_back(segment.value);
              }
            }
          }
          else
          {
            materialized_elems = derived->elements;
          }
          const std::size_t count = materialized_elems.size();
          for (std::size_t i = 0; i < count; ++i)
          {
            llvm::Value *elem = EvaluateIRValue(materialized_elems[i]);
            if (!elem)
            {
              continue;
            }
            llvm::Type *elem_ty = nullptr;
            if (auto *arr_ty = llvm::dyn_cast<llvm::ArrayType>(agg_ty))
            {
              elem_ty = arr_ty->getElementType();
            }
            else if (auto *struct_ty = llvm::dyn_cast<llvm::StructType>(agg_ty))
            {
              if (i < struct_ty->getNumElements())
              {
                elem_ty = struct_ty->getElementType(static_cast<unsigned>(i));
              }
            }
            if (!elem_ty)
            {
              continue;
            }
            elem = CoerceTo(builder, elem, elem_ty);
            if (!elem)
            {
              elem = llvm::Constant::getNullValue(elem_ty);
            }
            agg = builder->CreateInsertValue(agg, elem, {static_cast<unsigned>(i)});
          }
          materialized = agg;
        }
        break;
      }
      default:
        break;
      }

      if (!materialized)
      {
        materialized = default_for(val);
      }
      // Do not memoize block-local instruction values across control-flow joins.
      // Reusing branch-local GEP/load/extract instructions from another block can
      // violate LLVM dominance (observed in nested if/cleanup paths).
      if (llvm::isa<llvm::Constant>(materialized) ||
          llvm::isa<llvm::GlobalValue>(materialized) ||
          llvm::isa<llvm::Argument>(materialized) ||
          llvm::isa<llvm::AllocaInst>(materialized))
      {
        SetTempValue(val, materialized);
      }
      return materialized;
    }

    default:
      return nullptr;
    }
  }

  // Main entry point for T-LLVM-008
  llvm::Module *EmitLLVM(const IRDecls &decls,
                         LowerCtx &ctx,
                         llvm::LLVMContext &llvm_ctx,
                         project::TargetProfile profile)
  {
    auto emitter =
        std::make_unique<LLVMEmitter>(llvm_ctx, "cursive_module", profile);
    llvm::Module *m = emitter->EmitModule(decls, ctx);
    (void)emitter->ReleaseModule();
    return m;
  }

} // namespace cursive::codegen
