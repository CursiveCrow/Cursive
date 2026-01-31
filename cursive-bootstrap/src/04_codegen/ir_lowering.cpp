#include "cursive0/04_codegen/llvm/llvm_emit.h"
#include "cursive0/04_codegen/llvm/llvm_ir_utils.h"
#include "cursive0/00_core/assert_spec.h"
#include "cursive0/runtime/runtime_interface.h"

#include "cursive0/04_codegen/checks.h"
#include "cursive0/04_codegen/globals.h"
#include "cursive0/04_codegen/cleanup.h"
#include "cursive0/04_codegen/async_frame.h"
#include "cursive0/04_codegen/layout/layout.h"
#include "cursive0/04_codegen/mangle.h"
#include "cursive0/00_core/symbols.h"
#include "cursive0/03_analysis/composite/enums.h"
#include "cursive0/03_analysis/generics/monomorphize.h"
#include "cursive0/03_analysis/modal/modal_widen.h"
#include "cursive0/03_analysis/resolve/scopes.h"
#include "cursive0/03_analysis/types/type_equiv.h"

// LLVM Includes
#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/APInt.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"

#include <algorithm>
#include <cstring>
#include <iostream>
#include <limits>

namespace cursive0::codegen {

static const syntax::ModalDecl* LookupModalDecl(const analysis::ScopeContext& scope,
                                                const analysis::TypePath& path);
static const syntax::StateBlock* LookupModalState(const syntax::ModalDecl& decl,
                                                  std::string_view state);


namespace {

// Utility functions moved to llvm_ir_utils.cpp:
// BuildScope, AlignUp, APIntFromBytes, AllZero, ConstBytes,
// StripPerm, IsUnsignedPrim, IsUnsignedType, IsUnitType,
// PtrElementType, AsUSize, CreateEntryAlloca, ByteGEP,
// LoadAtOffset, StoreAtOffset

llvm::Value* LoadPanicOutPtr(LLVMEmitter& emitter,
                             llvm::IRBuilder<>* builder);

llvm::Value* CallAsyncResume(LLVMEmitter& emitter,
                             llvm::IRBuilder<>* builder,
                             LowerCtx* ctx,
                             llvm::Value* frame_ptr,
                             llvm::Value* input_ptr,
                             const analysis::TypeRef& async_type) {
  if (!builder || !frame_ptr || !async_type) {
    return nullptr;
  }
  (void)ctx;

  llvm::Value* resume_fn = LoadAtOffset(emitter, builder, frame_ptr,
                                        kAsyncFrameResumeFnOffset,
                                        emitter.GetOpaquePtr());
  if (!resume_fn) {
    return nullptr;
  }

  std::vector<IRParam> params;
  IRParam frame_param;
  frame_param.mode = analysis::ParamMode::Move;
  frame_param.name = "frame";
  frame_param.type = analysis::MakeTypePtr(analysis::MakeTypePrim("u8"),
                                           analysis::PtrState::Valid);
  params.push_back(frame_param);

  IRParam input_param;
  input_param.mode = analysis::ParamMode::Move;
  input_param.name = "input";
  input_param.type = analysis::MakeTypePtr(analysis::MakeTypePrim("u8"),
                                           analysis::PtrState::Valid);
  params.push_back(input_param);

  IRParam panic_param;
  panic_param.mode = analysis::ParamMode::Move;
  panic_param.name = std::string(kPanicOutName);
  panic_param.type = PanicOutType();
  params.push_back(panic_param);

  ABICallResult abi = emitter.ComputeCallABI(params, async_type);
  if (!abi.func_type) {
    return nullptr;
  }

  llvm::Function* func = builder->GetInsertBlock()->getParent();
  std::vector<llvm::Value*> call_args(abi.func_type->getNumParams(), nullptr);
  llvm::AllocaInst* sret_alloca = nullptr;
  if (abi.has_sret) {
    llvm::Type* ret_ty = emitter.GetLLVMType(async_type);
    sret_alloca = CreateEntryAlloca(emitter, builder, ret_ty, "sret_async");
    call_args[0] = sret_alloca;
  }

  llvm::Value* in_ptr = input_ptr ? input_ptr
                                  : llvm::Constant::getNullValue(emitter.GetOpaquePtr());
  llvm::Value* panic_ptr = LoadPanicOutPtr(emitter, builder);
  if (!panic_ptr) {
    panic_ptr = llvm::Constant::getNullValue(emitter.GetOpaquePtr());
  }

  std::vector<llvm::Value*> args = {frame_ptr, in_ptr, panic_ptr};

  for (std::size_t i = 0; i < params.size(); ++i) {
    if (i >= abi.param_indices.size()) {
      break;
    }
    if (!abi.param_indices[i].has_value()) {
      continue;
    }
    unsigned idx = *abi.param_indices[i];
    if (idx >= call_args.size() || i >= args.size()) {
      continue;
    }
    llvm::Value* arg = args[i];
    if (!arg) {
      continue;
    }
    llvm::Type* target_ty = abi.param_types[idx];
    if (abi.param_kinds[i] == PassKind::ByRef) {
      if (!arg->getType()->isPointerTy()) {
        llvm::Type* elem_ty = emitter.GetLLVMType(params[i].type);
        llvm::AllocaInst* slot = CreateEntryAlloca(emitter, builder, elem_ty, "byref_async");
        if (slot) {
          builder->CreateStore(arg, slot);
          call_args[idx] = slot;
        }
        continue;
      }
      if (target_ty && arg->getType() != target_ty) {
        if (arg->getType()->isPointerTy() && target_ty->isPointerTy()) {
          arg = builder->CreateBitCast(arg, target_ty);
        }
      }
      call_args[idx] = arg;
      continue;
    }
    if (target_ty && arg->getType() != target_ty) {
      if (arg->getType()->isPointerTy() && target_ty->isPointerTy()) {
        arg = builder->CreateBitCast(arg, target_ty);
      }
    }
    call_args[idx] = arg;
  }

  for (std::size_t i = 0; i < call_args.size(); ++i) {
    if (!call_args[i]) {
      call_args[i] = llvm::Constant::getNullValue(abi.param_types[i]);
    }
  }

  llvm::Value* callee = resume_fn;
  llvm::Type* callee_ty = abi.func_type->getPointerTo();
  if (callee->getType() != callee_ty && callee->getType()->isPointerTy()) {
    callee = builder->CreateBitCast(callee, callee_ty);
  }

  llvm::CallInst* call_inst = builder->CreateCall(abi.func_type, callee, call_args);
  call_inst->setCallingConv(llvm::CallingConv::C);

  if (abi.has_sret && sret_alloca) {
    return builder->CreateLoad(emitter.GetLLVMType(async_type), sret_alloca);
  }
  if (!call_inst->getType()->isVoidTy()) {
    return call_inst;
  }
  return llvm::Constant::getNullValue(emitter.GetLLVMType(async_type));
}

std::string ResolvePathSymbol(const std::vector<std::string>& path,
                              const std::string& name) {
  std::vector<std::string> full = path;
  full.push_back(name);
  const std::string qualified = core::StringOfPath(full);
  const std::string builtin = BuiltinSym(qualified);
  if (!builtin.empty()) {
    return builtin;
  }
  return core::Mangle(qualified);
}

struct FieldInfo {
  std::uint64_t offset = 0;
  analysis::TypeRef type;
};

std::optional<std::pair<std::vector<std::string>, std::vector<analysis::TypeRef>>>
CollectRecordFields(const analysis::ScopeContext& scope,
                    const syntax::RecordDecl& record) {
  std::vector<std::string> names;
  std::vector<analysis::TypeRef> types;
  for (const auto& member : record.members) {
    const auto* field = std::get_if<syntax::FieldDecl>(&member);
    if (!field) {
      continue;
    }
    if (!field->type) {
      return std::nullopt;
    }
    const auto lowered = LowerTypeForLayout(scope, field->type);
    if (!lowered.has_value()) {
      return std::nullopt;
    }
    names.push_back(field->name);
    types.push_back(*lowered);
  }
  return std::make_pair(std::move(names), std::move(types));
}

std::optional<std::pair<std::vector<std::string>, std::vector<analysis::TypeRef>>>
CollectModalStateFields(const analysis::ScopeContext& scope,
                        const syntax::StateBlock& state) {
  std::vector<std::string> names;
  std::vector<analysis::TypeRef> types;
  for (const auto& member : state.members) {
    const auto* field = std::get_if<syntax::StateFieldDecl>(&member);
    if (!field) {
      continue;
    }
    if (!field->type) {
      return std::nullopt;
    }
    const auto lowered = LowerTypeForLayout(scope, field->type);
    if (!lowered.has_value()) {
      return std::nullopt;
    }
    names.push_back(field->name);
    types.push_back(*lowered);
  }
  return std::make_pair(std::move(names), std::move(types));
}

std::optional<FieldInfo> LookupRecordField(const analysis::ScopeContext& scope,
                                           const analysis::TypePathType& path_type,
                                           const std::string& field_name) {
  const auto& sigma = scope.sigma;
  syntax::Path syntax_path;
  syntax_path.reserve(path_type.path.size());
  for (const auto& seg : path_type.path) {
    syntax_path.push_back(seg);
  }
  const auto it = sigma.types.find(analysis::PathKeyOf(syntax_path));
  if (it == sigma.types.end()) {
    return std::nullopt;
  }
  const auto* record = std::get_if<syntax::RecordDecl>(&it->second);
  if (!record) {
    return std::nullopt;
  }
  auto fields_opt = CollectRecordFields(scope, *record);
  if (!fields_opt.has_value()) {
    return std::nullopt;
  }
  auto& names = fields_opt->first;
  auto& types = fields_opt->second;

  // §13.1 Instantiate: Substitute type parameters in field types for generic records
  // e.g., for Container<i32>, substitute T -> i32 in field type "T"
  if (record->generic_params.has_value() && !path_type.generic_args.empty()) {
    analysis::TypeSubst type_subst = analysis::BuildSubstitution(
        record->generic_params->params, path_type.generic_args);
    for (auto& field_type : types) {
      field_type = analysis::InstantiateType(field_type, type_subst);
    }
  }

  auto layout = RecordLayoutOf(scope, types);
  if (!layout.has_value()) {
    return std::nullopt;
  }
  for (std::size_t i = 0; i < names.size(); ++i) {
    if (names[i] == field_name) {
      return FieldInfo{layout->offsets[i], types[i]};
    }
  }
  return std::nullopt;
}

std::optional<FieldInfo> LookupTupleField(const analysis::ScopeContext& scope,
                                          const analysis::TypeTuple& tuple,
                                          std::size_t index) {
  if (index >= tuple.elements.size()) {
    return std::nullopt;
  }
  const auto layout = RecordLayoutOf(scope, tuple.elements);
  if (!layout.has_value()) {
    return std::nullopt;
  }
  return FieldInfo{layout->offsets[index], tuple.elements[index]};
}

struct RangeCalc {
  llvm::Value* start = nullptr;
  llvm::Value* end = nullptr;
  llvm::Value* ok = nullptr;
};

RangeCalc ComputeRangeBounds(const IRRange& range,
                             llvm::Value* len,
                             LLVMEmitter& emitter,
                             llvm::IRBuilder<>* builder) {
  llvm::LLVMContext& ctx = emitter.GetContext();
  auto* usize_ty = llvm::Type::getInt64Ty(ctx);
  llvm::Value* len_val = AsUSize(builder, len, ctx);
  if (!len_val) {
    len_val = llvm::ConstantInt::get(usize_ty, 0);
  }

  auto eval_opt = [&](const std::optional<IRValue>& opt) -> llvm::Value* {
    if (!opt.has_value()) {
      return nullptr;
    }
    llvm::Value* v = emitter.EvaluateIRValue(*opt);
    return AsUSize(builder, v, ctx);
  };

  llvm::Value* lo = eval_opt(range.lo);
  llvm::Value* hi = eval_opt(range.hi);

  if (!lo) {
    lo = llvm::ConstantInt::get(usize_ty, 0);
  }
  if (!hi) {
    hi = len_val;
  }

  llvm::Value* start = nullptr;
  llvm::Value* end = nullptr;
  llvm::Value* ok = nullptr;

  switch (range.kind) {
    case syntax::RangeKind::Full:
      start = llvm::ConstantInt::get(usize_ty, 0);
      end = len_val;
      ok = llvm::ConstantInt::getTrue(ctx);
      break;
    case syntax::RangeKind::From: {
      start = lo;
      end = len_val;
      ok = builder->CreateICmpULE(lo, len_val);
      break;
    }
    case syntax::RangeKind::To: {
      start = llvm::ConstantInt::get(usize_ty, 0);
      end = hi;
      ok = builder->CreateICmpULE(hi, len_val);
      break;
    }
    case syntax::RangeKind::ToInclusive: {
      start = llvm::ConstantInt::get(usize_ty, 0);
      end = builder->CreateAdd(hi, llvm::ConstantInt::get(usize_ty, 1));
      ok = builder->CreateICmpULT(hi, len_val);
      break;
    }
    case syntax::RangeKind::Exclusive: {
      start = lo;
      end = hi;
      auto le = builder->CreateICmpULE(lo, hi);
      auto in = builder->CreateICmpULE(hi, len_val);
      ok = builder->CreateAnd(le, in);
      break;
    }
    case syntax::RangeKind::Inclusive: {
      start = lo;
      end = builder->CreateAdd(hi, llvm::ConstantInt::get(usize_ty, 1));
      auto le = builder->CreateICmpULE(lo, hi);
      auto in = builder->CreateICmpULT(hi, len_val);
      auto max_val = llvm::ConstantInt::get(usize_ty, std::numeric_limits<std::uint64_t>::max());
      auto not_max = builder->CreateICmpNE(hi, max_val);
      ok = builder->CreateAnd(builder->CreateAnd(le, in), not_max);
      break;
    }
  }

  return RangeCalc{start, end, ok};
}

// SliceLenFromValue moved to llvm_ir_utils.cpp

struct BaseAddress {
  llvm::Value* ptr = nullptr;
  analysis::TypeRef type;
};

std::optional<BaseAddress> GetBaseAddress(LLVMEmitter& emitter,
                                          llvm::IRBuilder<>* builder,
                                          const IRValue& base,
                                          analysis::TypeRef base_type) {
  auto stripped = StripPerm(base_type);
  if (!stripped) {
    return std::nullopt;
  }
  if (const auto* ptr = std::get_if<analysis::TypePtr>(&stripped->node)) {
    llvm::Value* addr = emitter.EvaluateIRValue(base);
    if (!addr) {
      return std::nullopt;
    }
    analysis::TypeRef elem = ptr->element;
    if (const auto stripped_elem = StripPerm(elem)) {
      elem = stripped_elem;
    }
    return BaseAddress{addr, elem};
  }
  if (const auto* raw = std::get_if<analysis::TypeRawPtr>(&stripped->node)) {
    llvm::Value* addr = emitter.EvaluateIRValue(base);
    if (!addr) {
      return std::nullopt;
    }
    analysis::TypeRef elem = raw->element;
    if (const auto stripped_elem = StripPerm(elem)) {
      elem = stripped_elem;
    }
    return BaseAddress{addr, elem};
  }

  llvm::Value* val = emitter.EvaluateIRValue(base);
  if (!val) {
    return std::nullopt;
  }
  llvm::Type* llvm_ty = emitter.GetLLVMType(stripped);
  auto* alloca = CreateEntryAlloca(emitter, builder, llvm_ty, "tmp_addr");
  if (!alloca) {
    return std::nullopt;
  }
  builder->CreateStore(val, alloca);
  return BaseAddress{alloca, stripped};
}

// ExtractSlicePtr and ExtractSliceLen moved to llvm_ir_utils.cpp

llvm::Value* MaterializeDerivedAddress(LLVMEmitter& emitter,
                                       llvm::IRBuilder<>* builder,
                                       const DerivedValueInfo& info) {
  LowerCtx* ctx = emitter.GetCurrentCtx();
  if (!ctx) {
    return nullptr;
  }
  const auto scope = BuildScope(ctx);

  switch (info.kind) {
    case DerivedValueInfo::Kind::AddrLocal: {
      return emitter.GetLocal(info.name);
    }
    case DerivedValueInfo::Kind::AddrStatic: {
      const std::string sym = ResolvePathSymbol(info.static_path, info.name);
      if (auto* f = emitter.GetFunction(sym)) {
        return f;
      }
      if (auto* g = emitter.GetGlobal(sym)) {
        return g;
      }
      return nullptr;
    }
    case DerivedValueInfo::Kind::AddrField: {
      auto base_type = ctx->LookupValueType(info.base);
      auto base_opt = GetBaseAddress(emitter, builder, info.base, base_type);
      if (!base_opt) {
        return nullptr;
      }
      auto base = *base_opt;
      if (auto* path = std::get_if<analysis::TypePathType>(&base.type->node)) {
        auto field_info = LookupRecordField(scope, *path, info.field);
        if (!field_info.has_value()) {
          return nullptr;
        }
        return field_info->offset == 0
                   ? base.ptr
                   : ByteGEP(emitter, builder, base.ptr, field_info->offset);
      }
      return nullptr;
    }
    case DerivedValueInfo::Kind::AddrTuple: {
      auto base_type = ctx->LookupValueType(info.base);
      auto base_opt = GetBaseAddress(emitter, builder, info.base, base_type);
      if (!base_opt) {
        return nullptr;
      }
      auto base = *base_opt;
      if (auto* tuple = std::get_if<analysis::TypeTuple>(&base.type->node)) {
        auto field_info = LookupTupleField(scope, *tuple, info.tuple_index);
        if (!field_info.has_value()) {
          return nullptr;
        }
        return field_info->offset == 0
                   ? base.ptr
                   : ByteGEP(emitter, builder, base.ptr, field_info->offset);
      }
      return nullptr;
    }
    case DerivedValueInfo::Kind::AddrIndex: {
      auto base_type = ctx->LookupValueType(info.base);
      auto base_opt = GetBaseAddress(emitter, builder, info.base, base_type);
      if (!base_opt) {
        return nullptr;
      }
      auto base = *base_opt;
      llvm::Value* data_ptr = nullptr;
      llvm::Value* len_val = nullptr;
      analysis::TypeRef elem_type;

      if (const auto* arr = std::get_if<analysis::TypeArray>(&base.type->node)) {
        elem_type = arr->element;
        data_ptr = base.ptr;
        len_val = llvm::ConstantInt::get(llvm::Type::getInt64Ty(emitter.GetContext()), arr->length);
      } else if (const auto* slice = std::get_if<analysis::TypeSlice>(&base.type->node)) {
        elem_type = slice->element;
        llvm::Type* slice_ty = emitter.GetLLVMType(base.type);
        llvm::Value* slice_val = builder->CreateLoad(slice_ty, base.ptr);
        data_ptr = ExtractSlicePtr(emitter, builder, slice_val);
        len_val = ExtractSliceLen(emitter, builder, slice_val);
      } else if (const auto* str = std::get_if<analysis::TypeString>(&base.type->node)) {
        elem_type = analysis::MakeTypePrim("u8");
        llvm::Type* str_ty = emitter.GetLLVMType(base.type);
        llvm::Value* str_val = builder->CreateLoad(str_ty, base.ptr);
        data_ptr = ExtractSlicePtr(emitter, builder, str_val);
        len_val = ExtractSliceLen(emitter, builder, str_val);
      } else if (const auto* bytes = std::get_if<analysis::TypeBytes>(&base.type->node)) {
        elem_type = analysis::MakeTypePrim("u8");
        llvm::Type* bytes_ty = emitter.GetLLVMType(base.type);
        llvm::Value* bytes_val = builder->CreateLoad(bytes_ty, base.ptr);
        data_ptr = ExtractSlicePtr(emitter, builder, bytes_val);
        len_val = ExtractSliceLen(emitter, builder, bytes_val);
      }

      if (!data_ptr || !elem_type) {
        return nullptr;
      }

      llvm::Value* idx = nullptr;
      if (info.index.kind != IRValue::Kind::Opaque || !info.index.name.empty() ||
          !info.index.bytes.empty()) {
        idx = emitter.EvaluateIRValue(info.index);
        idx = AsUSize(builder, idx, emitter.GetContext());
      } else if (info.range.kind != syntax::RangeKind::Full || info.range.lo.has_value() || info.range.hi.has_value()) {
        if (!len_val) {
          return nullptr;
        }
        auto bounds = ComputeRangeBounds(info.range, len_val, emitter, builder);
        idx = bounds.start;
      }

      if (!idx) {
        return nullptr;
      }

      const auto size_opt = SizeOf(scope, elem_type);
      if (!size_opt.has_value()) {
        return nullptr;
      }
      llvm::Value* stride = llvm::ConstantInt::get(llvm::Type::getInt64Ty(emitter.GetContext()), *size_opt);
      llvm::Value* offset = builder->CreateMul(idx, stride);
      return ByteGEP(emitter, builder, data_ptr, offset);
    }
    case DerivedValueInfo::Kind::AddrDeref: {
      return emitter.EvaluateIRValue(info.base);
    }
    default:
      return nullptr;
  }
}

llvm::Value* MaterializeDerivedValue(LLVMEmitter& emitter,
                                     llvm::IRBuilder<>* builder,
                                     const IRValue& value,
                                     const DerivedValueInfo& info) {
  LowerCtx* ctx = emitter.GetCurrentCtx();
  if (!ctx) {
    return nullptr;
  }
  const auto scope = BuildScope(ctx);

  auto load_typed = [&](llvm::Value* addr, const analysis::TypeRef& ty) -> llvm::Value* {
    if (!addr || !ty) {
      return nullptr;
    }
    llvm::Type* llvm_ty = emitter.GetLLVMType(ty);
    return builder->CreateLoad(llvm_ty, addr);
  };

  switch (info.kind) {
    case DerivedValueInfo::Kind::Field: {
      auto base_type = ctx->LookupValueType(info.base);
      auto base_opt = GetBaseAddress(emitter, builder, info.base, base_type);
      if (!base_opt) {
        return nullptr;
      }
      auto base = *base_opt;
      if (auto* path = std::get_if<analysis::TypePathType>(&base.type->node)) {
        auto field_info = LookupRecordField(scope, *path, info.field);
        if (!field_info.has_value()) {
          return nullptr;
        }
        llvm::Value* addr = field_info->offset == 0
                                ? base.ptr
                                : ByteGEP(emitter, builder, base.ptr, field_info->offset);
        return load_typed(addr, field_info->type);
      }
      return nullptr;
    }
    case DerivedValueInfo::Kind::Tuple: {
      auto base_type = ctx->LookupValueType(info.base);
      auto base_opt = GetBaseAddress(emitter, builder, info.base, base_type);
      if (!base_opt) {
        return nullptr;
      }
      auto base = *base_opt;
      if (auto* tuple = std::get_if<analysis::TypeTuple>(&base.type->node)) {
        auto field_info = LookupTupleField(scope, *tuple, info.tuple_index);
        if (!field_info.has_value()) {
          return nullptr;
        }
        llvm::Value* addr = field_info->offset == 0
                                ? base.ptr
                                : ByteGEP(emitter, builder, base.ptr, field_info->offset);
        return load_typed(addr, field_info->type);
      }
      return nullptr;
    }
    case DerivedValueInfo::Kind::Index: {
      auto base_type = ctx->LookupValueType(info.base);
      auto base_opt = GetBaseAddress(emitter, builder, info.base, base_type);
      if (!base_opt) {
        return nullptr;
      }
      auto base = *base_opt;
      llvm::Value* data_ptr = nullptr;
      analysis::TypeRef elem_type;
      if (const auto* arr = std::get_if<analysis::TypeArray>(&base.type->node)) {
        elem_type = arr->element;
        data_ptr = base.ptr;
      } else if (const auto* slice = std::get_if<analysis::TypeSlice>(&base.type->node)) {
        elem_type = slice->element;
        llvm::Type* slice_ty = emitter.GetLLVMType(base.type);
        llvm::Value* slice_val = builder->CreateLoad(slice_ty, base.ptr);
        data_ptr = ExtractSlicePtr(emitter, builder, slice_val);
      } else if (const auto* str = std::get_if<analysis::TypeString>(&base.type->node)) {
        elem_type = analysis::MakeTypePrim("u8");
        llvm::Type* str_ty = emitter.GetLLVMType(base.type);
        llvm::Value* str_val = builder->CreateLoad(str_ty, base.ptr);
        data_ptr = ExtractSlicePtr(emitter, builder, str_val);
      } else if (const auto* bytes = std::get_if<analysis::TypeBytes>(&base.type->node)) {
        elem_type = analysis::MakeTypePrim("u8");
        llvm::Type* bytes_ty = emitter.GetLLVMType(base.type);
        llvm::Value* bytes_val = builder->CreateLoad(bytes_ty, base.ptr);
        data_ptr = ExtractSlicePtr(emitter, builder, bytes_val);
      }

      if (!data_ptr || !elem_type) {
        return nullptr;
      }
      llvm::Value* idx = emitter.EvaluateIRValue(info.index);
      idx = AsUSize(builder, idx, emitter.GetContext());
      if (!idx) {
        return nullptr;
      }
      const auto size_opt = SizeOf(scope, elem_type);
      if (!size_opt.has_value()) {
        return nullptr;
      }
      llvm::Value* stride = llvm::ConstantInt::get(llvm::Type::getInt64Ty(emitter.GetContext()), *size_opt);
      llvm::Value* offset = builder->CreateMul(idx, stride);
      llvm::Value* addr = ByteGEP(emitter, builder, data_ptr, offset);
      return load_typed(addr, elem_type);
    }
    case DerivedValueInfo::Kind::Slice: {
      auto base_type = ctx->LookupValueType(info.base);
      auto base_opt = GetBaseAddress(emitter, builder, info.base, base_type);
      if (!base_opt) {
        return nullptr;
      }
      auto base = *base_opt;
      llvm::Value* data_ptr = nullptr;
      llvm::Value* len_val = nullptr;
      analysis::TypeRef elem_type;
      if (const auto* arr = std::get_if<analysis::TypeArray>(&base.type->node)) {
        elem_type = arr->element;
        data_ptr = base.ptr;
        len_val = llvm::ConstantInt::get(llvm::Type::getInt64Ty(emitter.GetContext()), arr->length);
      } else if (const auto* slice = std::get_if<analysis::TypeSlice>(&base.type->node)) {
        elem_type = slice->element;
        llvm::Type* slice_ty = emitter.GetLLVMType(base.type);
        llvm::Value* slice_val = builder->CreateLoad(slice_ty, base.ptr);
        data_ptr = ExtractSlicePtr(emitter, builder, slice_val);
        len_val = ExtractSliceLen(emitter, builder, slice_val);
      } else if (const auto* str = std::get_if<analysis::TypeString>(&base.type->node)) {
        elem_type = analysis::MakeTypePrim("u8");
        llvm::Type* str_ty = emitter.GetLLVMType(base.type);
        llvm::Value* str_val = builder->CreateLoad(str_ty, base.ptr);
        data_ptr = ExtractSlicePtr(emitter, builder, str_val);
        len_val = ExtractSliceLen(emitter, builder, str_val);
      } else if (const auto* bytes = std::get_if<analysis::TypeBytes>(&base.type->node)) {
        elem_type = analysis::MakeTypePrim("u8");
        llvm::Type* bytes_ty = emitter.GetLLVMType(base.type);
        llvm::Value* bytes_val = builder->CreateLoad(bytes_ty, base.ptr);
        data_ptr = ExtractSlicePtr(emitter, builder, bytes_val);
        len_val = ExtractSliceLen(emitter, builder, bytes_val);
      }

      if (!data_ptr || !elem_type || !len_val) {
        return nullptr;
      }

      auto bounds = ComputeRangeBounds(info.range, len_val, emitter, builder);
      llvm::Value* start = bounds.start;
      llvm::Value* end = bounds.end;

      const auto size_opt = SizeOf(scope, elem_type);
      if (!size_opt.has_value()) {
        return nullptr;
      }
      llvm::Value* stride = llvm::ConstantInt::get(llvm::Type::getInt64Ty(emitter.GetContext()), *size_opt);
      llvm::Value* offset = builder->CreateMul(start, stride);
      llvm::Value* slice_ptr = ByteGEP(emitter, builder, data_ptr, offset);
      llvm::Value* slice_len = builder->CreateSub(end, start);

      analysis::TypeRef slice_type = ctx->LookupValueType(value);
      llvm::Type* slice_ty = emitter.GetLLVMType(slice_type);
      auto* alloca = CreateEntryAlloca(emitter, builder, slice_ty, "slice");
      if (!alloca) {
        return nullptr;
      }
      builder->CreateStore(llvm::Constant::getNullValue(slice_ty), alloca);

      const auto layout = RecordLayoutOf(scope, {analysis::MakeTypeRawPtr(analysis::RawPtrQual::Imm, analysis::MakeTypePrim("()")),
                                                 analysis::MakeTypePrim("usize")});
      if (!layout.has_value() || layout->offsets.size() < 2) {
        return nullptr;
      }
      StoreAtOffset(emitter, builder, alloca, layout->offsets[0], slice_ptr);
      StoreAtOffset(emitter, builder, alloca, layout->offsets[1], slice_len);

      return builder->CreateLoad(slice_ty, alloca);
    }
    case DerivedValueInfo::Kind::EnumPayloadIndex:
    case DerivedValueInfo::Kind::EnumPayloadField: {
      auto base_type = ctx->LookupValueType(info.base);
      auto stripped = StripPerm(base_type);
      auto* path = stripped ? std::get_if<analysis::TypePathType>(&stripped->node) : nullptr;
      if (!path) {
        return nullptr;
      }
      syntax::Path syntax_path;
      syntax_path.reserve(path->path.size());
      for (const auto& seg : path->path) {
        syntax_path.push_back(seg);
      }
      const auto it = scope.sigma.types.find(analysis::PathKeyOf(syntax_path));
      if (it == scope.sigma.types.end()) {
        return nullptr;
      }
      const auto* enum_decl = std::get_if<syntax::EnumDecl>(&it->second);
      if (!enum_decl) {
        return nullptr;
      }
      const syntax::VariantDecl* variant = nullptr;
      for (const auto& v : enum_decl->variants) {
        if (v.name == info.variant) {
          variant = &v;
          break;
        }
      }
      if (!variant || !variant->payload_opt.has_value()) {
        return nullptr;
      }
      auto base_opt = GetBaseAddress(emitter, builder, info.base, base_type);
      if (!base_opt) {
        return nullptr;
      }
      auto base = *base_opt;
      const auto enum_layout = EnumLayoutOf(scope, *enum_decl);
      if (!enum_layout.has_value()) {
        return nullptr;
      }
      const auto disc_type = analysis::MakeTypePrim(enum_layout->disc_type);
      const auto disc_size = SizeOf(scope, disc_type).value_or(0);
      const std::uint64_t payload_offset = AlignUp(disc_size, enum_layout->payload_align);

      if (const auto* tup = std::get_if<syntax::VariantPayloadTuple>(&*variant->payload_opt)) {
        if (info.kind != DerivedValueInfo::Kind::EnumPayloadIndex) {
          return nullptr;
        }
        if (info.tuple_index >= tup->elements.size()) {
          return nullptr;
        }
        std::vector<analysis::TypeRef> elems;
        elems.reserve(tup->elements.size());
        for (const auto& elem : tup->elements) {
          const auto lowered = LowerTypeForLayout(scope, elem);
          if (!lowered.has_value()) {
            return nullptr;
          }
          elems.push_back(*lowered);
        }
        const auto layout = RecordLayoutOf(scope, elems);
        if (!layout.has_value()) {
          return nullptr;
        }
        const std::uint64_t elem_offset =
            payload_offset + layout->offsets[info.tuple_index];
        llvm::Value* addr = elem_offset == 0
                                ? base.ptr
                                : ByteGEP(emitter, builder, base.ptr, elem_offset);
        return load_typed(addr, elems[info.tuple_index]);
      }

      const auto* rec = std::get_if<syntax::VariantPayloadRecord>(&*variant->payload_opt);
      if (!rec || info.kind != DerivedValueInfo::Kind::EnumPayloadField) {
        return nullptr;
      }
      std::vector<std::string> names;
      std::vector<analysis::TypeRef> types;
      names.reserve(rec->fields.size());
      types.reserve(rec->fields.size());
      for (const auto& field : rec->fields) {
        const auto lowered = LowerTypeForLayout(scope, field.type);
        if (!lowered.has_value()) {
          return nullptr;
        }
        names.push_back(field.name);
        types.push_back(*lowered);
      }
      const auto layout = RecordLayoutOf(scope, types);
      if (!layout.has_value()) {
        return nullptr;
      }
      for (std::size_t i = 0; i < names.size(); ++i) {
        if (names[i] == info.field) {
          const std::uint64_t field_offset =
              payload_offset + layout->offsets[i];
          llvm::Value* addr = field_offset == 0
                                  ? base.ptr
                                  : ByteGEP(emitter, builder, base.ptr, field_offset);
          return load_typed(addr, types[i]);
        }
      }
      return nullptr;
    }
    case DerivedValueInfo::Kind::ModalField: {
      auto base_type = ctx->LookupValueType(info.base);
      auto stripped = StripPerm(base_type);
      if (!stripped) {
        return nullptr;
      }
      const syntax::ModalDecl* decl = nullptr;
      std::string state_name = info.modal_state;
      if (const auto* path = std::get_if<analysis::TypePathType>(&stripped->node)) {
        syntax::Path syntax_path;
        syntax_path.reserve(path->path.size());
        for (const auto& seg : path->path) {
          syntax_path.push_back(seg);
        }
        const auto it = scope.sigma.types.find(analysis::PathKeyOf(syntax_path));
        if (it == scope.sigma.types.end()) {
          return nullptr;
        }
        decl = std::get_if<syntax::ModalDecl>(&it->second);
      } else if (const auto* modal_state = std::get_if<analysis::TypeModalState>(&stripped->node)) {
        syntax::Path syntax_path;
        syntax_path.reserve(modal_state->path.size());
        for (const auto& seg : modal_state->path) {
          syntax_path.push_back(seg);
        }
        const auto it = scope.sigma.types.find(analysis::PathKeyOf(syntax_path));
        if (it == scope.sigma.types.end()) {
          return nullptr;
        }
        decl = std::get_if<syntax::ModalDecl>(&it->second);
        state_name = modal_state->state;
      }
      if (!decl || state_name.empty()) {
        return nullptr;
      }

      const syntax::StateBlock* state = nullptr;
      for (const auto& st : decl->states) {
        if (analysis::IdEq(st.name, state_name)) {
          state = &st;
          break;
        }
      }
      if (!state) {
        return nullptr;
      }

      std::vector<std::string> names;
      std::vector<analysis::TypeRef> types;
      for (const auto& member : state->members) {
        const auto* field = std::get_if<syntax::StateFieldDecl>(&member);
        if (!field) {
          continue;
        }
        const auto lowered = LowerTypeForLayout(scope, field->type);
        if (!lowered.has_value()) {
          return nullptr;
        }
        names.push_back(field->name);
        types.push_back(*lowered);
      }
      const auto layout = RecordLayoutOf(scope, types);
      if (!layout.has_value()) {
        return nullptr;
      }
      std::optional<std::size_t> field_index;
      for (std::size_t i = 0; i < names.size(); ++i) {
        if (names[i] == info.field) {
          field_index = i;
          break;
        }
      }
      if (!field_index.has_value()) {
        return nullptr;
      }

      std::uint64_t payload_offset = 0;
      if (std::holds_alternative<analysis::TypePathType>(stripped->node)) {
        const auto modal_layout = ModalLayoutOf(scope, *decl);
        if (!modal_layout.has_value()) {
          return nullptr;
        }
        if (!modal_layout->niche && modal_layout->disc_type.has_value()) {
          std::uint64_t payload_align = 1;
          for (const auto& st : decl->states) {
            std::vector<analysis::TypeRef> st_types;
            for (const auto& member : st.members) {
              const auto* field = std::get_if<syntax::StateFieldDecl>(&member);
              if (!field) {
                continue;
              }
              const auto lowered = LowerTypeForLayout(scope, field->type);
              if (!lowered.has_value()) {
                return nullptr;
              }
              st_types.push_back(*lowered);
            }
            const auto st_layout = RecordLayoutOf(scope, st_types);
            if (!st_layout.has_value()) {
              return nullptr;
            }
            payload_align = std::max(payload_align, st_layout->layout.align);
          }
          const auto disc_type = analysis::MakeTypePrim(*modal_layout->disc_type);
          const auto disc_size = SizeOf(scope, disc_type).value_or(0);
          payload_offset = AlignUp(disc_size, payload_align);
        }
      }

      auto base_opt = GetBaseAddress(emitter, builder, info.base, base_type);
      if (!base_opt) {
        return nullptr;
      }
      auto base = *base_opt;
      const std::uint64_t field_offset =
          payload_offset + layout->offsets[*field_index];
      llvm::Value* addr = field_offset == 0
                              ? base.ptr
                              : ByteGEP(emitter, builder, base.ptr, field_offset);
      return load_typed(addr, types[*field_index]);
    }
    case DerivedValueInfo::Kind::UnionPayload: {
      auto base_type = ctx->LookupValueType(info.base);
      auto stripped = StripPerm(base_type);
      if (!stripped) {
        return nullptr;
      }
      const auto* uni = std::get_if<analysis::TypeUnion>(&stripped->node);
      if (!uni) {
        return nullptr;
      }
      const auto layout = UnionLayoutOf(scope, *uni);
      if (!layout.has_value()) {
        return nullptr;
      }
      if (info.union_index >= layout->member_list.size()) {
        return nullptr;
      }
      const auto member_type = layout->member_list[info.union_index];
      auto base_opt = GetBaseAddress(emitter, builder, info.base, base_type);
      if (!base_opt) {
        return nullptr;
      }
      auto base = *base_opt;
      std::uint64_t payload_offset = 0;
      if (!layout->niche && layout->disc_type.has_value()) {
        const auto disc_type = analysis::MakeTypePrim(*layout->disc_type);
        const auto disc_size = SizeOf(scope, disc_type).value_or(0);
        payload_offset = AlignUp(disc_size, layout->payload_align);
      }
      if (IsUnitType(member_type)) {
        llvm::Type* llvm_ty = emitter.GetLLVMType(member_type);
        return llvm::Constant::getNullValue(llvm_ty);
      }
      llvm::Value* addr = payload_offset == 0
                              ? base.ptr
                              : ByteGEP(emitter, builder, base.ptr, payload_offset);
      return load_typed(addr, member_type);
    }
    case DerivedValueInfo::Kind::TupleLit: {
      auto value_type = ctx->LookupValueType(value);
      auto stripped = StripPerm(value_type);
      if (!stripped) {
        return nullptr;
      }
      auto* tuple = std::get_if<analysis::TypeTuple>(&stripped->node);
      if (!tuple) {
        return nullptr;
      }
      auto layout = RecordLayoutOf(scope, tuple->elements);
      if (!layout.has_value()) {
        return nullptr;
      }
      llvm::Type* llvm_ty = emitter.GetLLVMType(stripped);
      auto* alloca = CreateEntryAlloca(emitter, builder, llvm_ty, "tuple");
      if (!alloca) {
        return nullptr;
      }
      builder->CreateStore(llvm::Constant::getNullValue(llvm_ty), alloca);
      for (std::size_t i = 0; i < info.elements.size() && i < tuple->elements.size(); ++i) {
        const auto size_opt = SizeOf(scope, tuple->elements[i]);
        if (size_opt.has_value() && *size_opt == 0) {
          continue;
        }
        llvm::Value* elem = emitter.EvaluateIRValue(info.elements[i]);
        if (!elem) {
          continue;
        }
        StoreAtOffset(emitter, builder, alloca, layout->offsets[i], elem);
      }
      return builder->CreateLoad(llvm_ty, alloca);
    }
    case DerivedValueInfo::Kind::ArrayLit: {
      auto value_type = ctx->LookupValueType(value);
      auto stripped = StripPerm(value_type);
      if (!stripped) {
        return nullptr;
      }
      auto* arr = std::get_if<analysis::TypeArray>(&stripped->node);
      if (!arr) {
        return nullptr;
      }
      llvm::Type* llvm_ty = emitter.GetLLVMType(stripped);
      auto* alloca = CreateEntryAlloca(emitter, builder, llvm_ty, "array");
      if (!alloca) {
        return nullptr;
      }
      builder->CreateStore(llvm::Constant::getNullValue(llvm_ty), alloca);
      const auto elem_size = SizeOf(scope, arr->element).value_or(0);
      for (std::size_t i = 0; i < info.elements.size() && i < arr->length; ++i) {
        if (elem_size == 0) {
          continue;
        }
        llvm::Value* elem = emitter.EvaluateIRValue(info.elements[i]);
        if (!elem) {
          continue;
        }
        const std::uint64_t offset = static_cast<std::uint64_t>(i) * elem_size;
        StoreAtOffset(emitter, builder, alloca, offset, elem);
      }
      return builder->CreateLoad(llvm_ty, alloca);
    }
    case DerivedValueInfo::Kind::ArrayRepeat: {
      auto value_type = ctx->LookupValueType(value);
      auto stripped = StripPerm(value_type);
      if (!stripped) {
        return nullptr;
      }
      auto* arr = std::get_if<analysis::TypeArray>(&stripped->node);
      if (!arr) {
        return nullptr;
      }
      llvm::Type* llvm_ty = emitter.GetLLVMType(stripped);
      auto* alloca = CreateEntryAlloca(emitter, builder, llvm_ty, "array_repeat");
      if (!alloca) {
        return nullptr;
      }
      builder->CreateStore(llvm::Constant::getNullValue(llvm_ty), alloca);
      const auto elem_size = SizeOf(scope, arr->element).value_or(0);
      llvm::Value* repeat_elem = emitter.EvaluateIRValue(info.repeat_value);
      if (!repeat_elem || elem_size == 0) {
        return builder->CreateLoad(llvm_ty, alloca);
      }
      for (std::uint64_t i = 0; i < arr->length; ++i) {
        const std::uint64_t offset = i * elem_size;
        StoreAtOffset(emitter, builder, alloca, offset, repeat_elem);
      }
      return builder->CreateLoad(llvm_ty, alloca);
    }
    case DerivedValueInfo::Kind::RecordLit: {
      auto value_type = ctx->LookupValueType(value);
      auto stripped = StripPerm(value_type);
      if (!stripped) {
        return nullptr;
      }
      std::optional<std::pair<std::vector<std::string>, std::vector<analysis::TypeRef>>> fields_opt;
      if (auto* path = std::get_if<analysis::TypePathType>(&stripped->node)) {
        fields_opt = [&, path]() -> std::optional<std::pair<std::vector<std::string>, std::vector<analysis::TypeRef>>> {
          syntax::Path syntax_path;
          syntax_path.reserve(path->path.size());
          for (const auto& seg : path->path) {
            syntax_path.push_back(seg);
          }
          const auto it = scope.sigma.types.find(analysis::PathKeyOf(syntax_path));
          if (it == scope.sigma.types.end()) {
            return std::nullopt;
          }
          const auto* record = std::get_if<syntax::RecordDecl>(&it->second);
          if (!record) {
            return std::nullopt;
          }
          auto result = CollectRecordFields(scope, *record);
          // §13.1 Instantiate: Substitute type parameters in field types for generic records
          if (result.has_value() && record->generic_params.has_value() && !path->generic_args.empty()) {
            analysis::TypeSubst type_subst = analysis::BuildSubstitution(
                record->generic_params->params, path->generic_args);
            for (auto& field_type : result->second) {
              field_type = analysis::InstantiateType(field_type, type_subst);
            }
          }
          return result;
        }();
      } else if (auto* modal = std::get_if<analysis::TypeModalState>(&stripped->node)) {
        const auto* decl = LookupModalDecl(scope, modal->path);
        if (!decl) {
          return nullptr;
        }
        const auto* state = LookupModalState(*decl, modal->state);
        if (!state) {
          return nullptr;
        }
        fields_opt = CollectModalStateFields(scope, *state);
      } else {
        return nullptr;
      }
      if (!fields_opt.has_value()) {
        return nullptr;
      }
      auto& names = fields_opt->first;
      auto& types = fields_opt->second;
      auto layout = RecordLayoutOf(scope, types);
      if (!layout.has_value()) {
        return nullptr;
      }
      llvm::Type* llvm_ty = emitter.GetLLVMType(stripped);
      auto* alloca = CreateEntryAlloca(emitter, builder, llvm_ty, "record");
      if (!alloca) {
        return nullptr;
      }
      builder->CreateStore(llvm::Constant::getNullValue(llvm_ty), alloca);
      for (std::size_t i = 0; i < names.size(); ++i) {
        const auto size_opt = SizeOf(scope, types[i]);
        if (size_opt.has_value() && *size_opt == 0) {
          continue;
        }
        auto it = std::find_if(info.fields.begin(), info.fields.end(),
                               [&](const auto& entry) { return entry.first == names[i]; });
        if (it == info.fields.end()) {
          continue;
        }
        llvm::Value* field_val = emitter.EvaluateIRValue(it->second);
        if (!field_val) {
          continue;
        }
        StoreAtOffset(emitter, builder, alloca, layout->offsets[i], field_val);
      }
      return builder->CreateLoad(llvm_ty, alloca);
    }
    case DerivedValueInfo::Kind::DynLit: {
      auto value_type = ctx->LookupValueType(value);
      auto stripped = StripPerm(value_type);
      if (!stripped || !std::holds_alternative<analysis::TypeDynamic>(stripped->node)) {
        return nullptr;
      }
      llvm::Type* llvm_ty = emitter.GetLLVMType(stripped);
      if (!llvm_ty || !llvm_ty->isStructTy()) {
        return nullptr;
      }

      llvm::Value* data_ptr = emitter.EvaluateIRValue(info.base);
      if (!data_ptr) {
        return nullptr;
      }

      std::string sym = info.vtable_sym;
      if (auto alias = emitter.LookupSymbolAlias(sym)) {
        sym = *alias;
      }
      llvm::Value* vtable_ptr = nullptr;
      if (!sym.empty()) {
        if (auto* g = emitter.GetGlobal(sym)) {
          vtable_ptr = g;
        } else if (auto* f = emitter.GetFunction(sym)) {
          vtable_ptr = f;
        }
      }
      if (!vtable_ptr) {
        return nullptr;
      }

      llvm::Value* data_cast = builder->CreateBitCast(data_ptr, emitter.GetOpaquePtr());
      llvm::Value* vtable_cast = builder->CreateBitCast(vtable_ptr, emitter.GetOpaquePtr());

      llvm::Value* dyn_val = llvm::UndefValue::get(llvm_ty);
      dyn_val = builder->CreateInsertValue(dyn_val, data_cast, {0});
      dyn_val = builder->CreateInsertValue(dyn_val, vtable_cast, {1});
      return dyn_val;
    }
    case DerivedValueInfo::Kind::DynData: {
      llvm::Value* dyn_val = emitter.EvaluateIRValue(info.base);
      if (!dyn_val || !dyn_val->getType()->isStructTy()) {
        return nullptr;
      }
      return builder->CreateExtractValue(dyn_val, {0});
    }
    case DerivedValueInfo::Kind::DynVTableDrop: {
      llvm::Value* dyn_val = emitter.EvaluateIRValue(info.base);
      if (!dyn_val || !dyn_val->getType()->isStructTy()) {
        return nullptr;
      }
      llvm::Value* vtable_ptr = builder->CreateExtractValue(dyn_val, {1});
      if (!vtable_ptr || !vtable_ptr->getType()->isPointerTy()) {
        return nullptr;
      }
      llvm::Value* drop_addr = ByteGEP(
          emitter, builder, vtable_ptr,
          static_cast<std::uint64_t>(2 * kPtrSize));
      llvm::Type* ptr_ty = emitter.GetOpaquePtr();
      return builder->CreateLoad(ptr_ty, drop_addr);
    }
    case DerivedValueInfo::Kind::EnumLit: {
      auto value_type = ctx->LookupValueType(value);
      auto stripped = StripPerm(value_type);
      auto* path = stripped ? std::get_if<analysis::TypePathType>(&stripped->node) : nullptr;
      if (!path) {
        return nullptr;
      }
      syntax::Path syntax_path;
      syntax_path.reserve(path->path.size());
      for (const auto& seg : path->path) {
        syntax_path.push_back(seg);
      }
      const auto it = scope.sigma.types.find(analysis::PathKeyOf(syntax_path));
      if (it == scope.sigma.types.end()) {
        return nullptr;
      }
      const auto* enum_decl = std::get_if<syntax::EnumDecl>(&it->second);
      if (!enum_decl) {
        return nullptr;
      }
      const auto discs = analysis::EnumDiscriminants(*enum_decl);
      if (!discs.ok || discs.discs.size() != enum_decl->variants.size()) {
        return nullptr;
      }
      std::optional<std::size_t> variant_index;
      for (std::size_t i = 0; i < enum_decl->variants.size(); ++i) {
        if (enum_decl->variants[i].name == info.variant) {
          variant_index = i;
          break;
        }
      }
      if (!variant_index.has_value()) {
        return nullptr;
      }
      const auto enum_layout = EnumLayoutOf(scope, *enum_decl);
      if (!enum_layout.has_value()) {
        return nullptr;
      }
      const auto disc_type = analysis::MakeTypePrim(enum_layout->disc_type);
      const auto disc_size = SizeOf(scope, disc_type).value_or(0);
      const std::uint64_t payload_offset = AlignUp(disc_size, enum_layout->payload_align);

      llvm::Type* llvm_ty = emitter.GetLLVMType(stripped);
      auto* alloca = CreateEntryAlloca(emitter, builder, llvm_ty, "enum");
      if (!alloca) {
        return nullptr;
      }
      builder->CreateStore(llvm::Constant::getNullValue(llvm_ty), alloca);

      llvm::Type* disc_ty = emitter.GetLLVMType(disc_type);
      llvm::Value* disc_val = llvm::ConstantInt::get(disc_ty, discs.discs[*variant_index]);
      StoreAtOffset(emitter, builder, alloca, 0, disc_val);

      const auto& variant = enum_decl->variants[*variant_index];
      if (variant.payload_opt.has_value()) {
        if (const auto* tup = std::get_if<syntax::VariantPayloadTuple>(&*variant.payload_opt)) {
          std::vector<analysis::TypeRef> elems;
          elems.reserve(tup->elements.size());
          for (const auto& elem : tup->elements) {
            const auto lowered = LowerTypeForLayout(scope, elem);
            if (!lowered.has_value()) {
              return nullptr;
            }
            elems.push_back(*lowered);
          }
          const auto layout = RecordLayoutOf(scope, elems);
          if (!layout.has_value()) {
            return nullptr;
          }
          for (std::size_t i = 0; i < elems.size() && i < info.payload_elems.size(); ++i) {
            const auto size_opt = SizeOf(scope, elems[i]);
            if (size_opt.has_value() && *size_opt == 0) {
              continue;
            }
            llvm::Value* payload_val = emitter.EvaluateIRValue(info.payload_elems[i]);
            if (!payload_val) {
              continue;
            }
            StoreAtOffset(emitter, builder, alloca, payload_offset + layout->offsets[i], payload_val);
          }
        } else if (const auto* rec = std::get_if<syntax::VariantPayloadRecord>(&*variant.payload_opt)) {
          std::vector<std::string> names;
          std::vector<analysis::TypeRef> types;
          names.reserve(rec->fields.size());
          types.reserve(rec->fields.size());
          for (const auto& field : rec->fields) {
            const auto lowered = LowerTypeForLayout(scope, field.type);
            if (!lowered.has_value()) {
              return nullptr;
            }
            names.push_back(field.name);
            types.push_back(*lowered);
          }
          const auto layout = RecordLayoutOf(scope, types);
          if (!layout.has_value()) {
            return nullptr;
          }
          for (std::size_t i = 0; i < names.size(); ++i) {
            const auto size_opt = SizeOf(scope, types[i]);
            if (size_opt.has_value() && *size_opt == 0) {
              continue;
            }
            auto itv = std::find_if(info.payload_fields.begin(), info.payload_fields.end(),
                                    [&](const auto& entry) { return entry.first == names[i]; });
            if (itv == info.payload_fields.end()) {
              continue;
            }
            llvm::Value* payload_val = emitter.EvaluateIRValue(itv->second);
            if (!payload_val) {
              continue;
            }
            StoreAtOffset(emitter, builder, alloca, payload_offset + layout->offsets[i], payload_val);
          }
        }
      }

      return builder->CreateLoad(llvm_ty, alloca);
    }
    case DerivedValueInfo::Kind::RangeLit: {
      auto value_type = ctx->LookupValueType(value);
      auto stripped = StripPerm(value_type);
      if (!stripped) {
        return nullptr;
      }
      llvm::Type* llvm_ty = emitter.GetLLVMType(stripped);
      auto* alloca = CreateEntryAlloca(emitter, builder, llvm_ty, "range");
      if (!alloca) {
        return nullptr;
      }
      builder->CreateStore(llvm::Constant::getNullValue(llvm_ty), alloca);

      std::uint8_t tag = 2;
      switch (info.range.kind) {
        case syntax::RangeKind::To: tag = 0; break;
        case syntax::RangeKind::ToInclusive: tag = 1; break;
        case syntax::RangeKind::Full: tag = 2; break;
        case syntax::RangeKind::From: tag = 3; break;
        case syntax::RangeKind::Exclusive: tag = 4; break;
        case syntax::RangeKind::Inclusive: tag = 5; break;
      }

      llvm::Value* lo_val = nullptr;
      llvm::Value* hi_val = nullptr;
      if (info.range.lo.has_value()) {
        lo_val = emitter.EvaluateIRValue(*info.range.lo);
      }
      if (info.range.hi.has_value()) {
        hi_val = emitter.EvaluateIRValue(*info.range.hi);
      }
      llvm::LLVMContext& ctx_ll = emitter.GetContext();
      auto* usize_ty = llvm::Type::getInt64Ty(ctx_ll);
      lo_val = AsUSize(builder, lo_val, ctx_ll);
      hi_val = AsUSize(builder, hi_val, ctx_ll);
      if (!lo_val) {
        lo_val = llvm::ConstantInt::get(usize_ty, 0);
      }
      if (!hi_val) {
        hi_val = llvm::ConstantInt::get(usize_ty, 0);
      }

      std::vector<analysis::TypeRef> fields;
      fields.push_back(analysis::MakeTypePrim("u8"));
      fields.push_back(analysis::MakeTypePrim("usize"));
      fields.push_back(analysis::MakeTypePrim("usize"));
      const auto layout = RecordLayoutOf(scope, fields);
      if (!layout.has_value() || layout->offsets.size() < 3) {
        return nullptr;
      }
      llvm::Value* tag_val = llvm::ConstantInt::get(llvm::Type::getInt8Ty(ctx_ll), tag);
      StoreAtOffset(emitter, builder, alloca, layout->offsets[0], tag_val);
      StoreAtOffset(emitter, builder, alloca, layout->offsets[1], lo_val);
      StoreAtOffset(emitter, builder, alloca, layout->offsets[2], hi_val);
      return builder->CreateLoad(llvm_ty, alloca);
    }
    case DerivedValueInfo::Kind::LoadFromAddr: {
      analysis::TypeRef value_type = ctx->LookupValueType(value);
      if (!value_type) {
        return nullptr;
      }
      llvm::Value* addr = emitter.EvaluateIRValue(info.base);
      if (!addr) {
        return nullptr;
      }
      llvm::Type* llvm_ty = emitter.GetLLVMType(value_type);
      if (!llvm_ty) {
        return nullptr;
      }
      if (!addr->getType()->isPointerTy()) {
        return nullptr;
      }
      llvm::Value* typed_ptr = builder->CreateBitCast(addr, llvm_ty->getPointerTo());
      return builder->CreateLoad(llvm_ty, typed_ptr);
    }
    default:
      break;
  }
  return nullptr;
}

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

bool IsInitFunction(LLVMEmitter& emitter, llvm::Function* func);
void StoreInitPanicRecord(LLVMEmitter& emitter,
                          llvm::IRBuilder<>* builder);

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

bool IsInitFunction(LLVMEmitter& emitter, llvm::Function* func) {
  if (!func) {
    return false;
  }
  const std::string prefix =
      core::Mangle(core::StringOfPath({"cursive", "runtime", "init"}));
  return func->getName().starts_with(prefix);
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


}  // namespace
// T-LLVM-009: IR Operation Lowering

llvm::Value* LLVMEmitter::EvaluateIRValue(const IRValue& val) {
  SPEC_RULE("LowerIR-Value");

  auto* builder = static_cast<llvm::IRBuilder<>*>(builder_.get());
  LowerCtx* ctx = GetCurrentCtx();

  if (val.kind == IRValue::Kind::Local) {
    llvm::Value* slot = GetLocal(val.name);
    if (!slot) {
      return nullptr;
    }
    if (auto* alloca = llvm::dyn_cast<llvm::AllocaInst>(slot)) {
      return builder->CreateLoad(alloca->getAllocatedType(), alloca);
    }
    if (ctx) {
      if (auto ty = ctx->LookupValueType(val)) {
        return builder->CreateLoad(GetLLVMType(ty), slot);
      }
    }
    if (slot->getType()->isPointerTy()) {
      return builder->CreateLoad(GetOpaquePtr(), slot);
    }
    return slot;
  }

  if (val.kind == IRValue::Kind::Symbol) {
    std::string sym = val.name;
    if (auto alias = LookupSymbolAlias(val.name)) {
      sym = *alias;
    }

    auto load_global = [&](const std::string& name) -> llvm::Value* {
      if (auto* g = GetGlobal(name)) {
        if (auto* gv = llvm::dyn_cast<llvm::GlobalVariable>(g)) {
          return builder->CreateLoad(gv->getValueType(), gv);
        }
        if (g->getType()->isPointerTy()) {
          return builder->CreateLoad(GetOpaquePtr(), g);
        }
        return g;
      }
      if (auto* gv = module_->getGlobalVariable(name, true)) {
        globals_[name] = gv;
        return builder->CreateLoad(gv->getValueType(), gv);
      }
      if (ctx) {
        auto type = ctx->LookupStaticType(name);
        if (type) {
          llvm::Type* llvm_ty = GetLLVMType(type);
          auto* gv = new llvm::GlobalVariable(
              *module_,
              llvm_ty,
              false,
              llvm::GlobalValue::ExternalLinkage,
              nullptr,
              name);
          const auto scope = BuildScope(ctx);
          const auto align = AlignOf(scope, type).value_or(1);
          gv->setAlignment(llvm::Align(align));
          globals_[name] = gv;
          return builder->CreateLoad(gv->getValueType(), gv);
        }
      }
      return nullptr;
    };

    if (auto* f = GetFunction(sym)) {
      return f;
    }
    if (auto* g = load_global(sym)) {
      return g;
    }

    if (auto* f = GetFunction(val.name)) {
      return f;
    }
    if (auto* g = load_global(val.name)) {
      return g;
    }
    return nullptr;
  }

  if (val.kind == IRValue::Kind::Immediate) {
    llvm::Type* llvm_ty = nullptr;
    analysis::TypeRef value_type;
    if (ctx) {
      value_type = ctx->LookupValueType(val);
      if (value_type) {
        llvm_ty = GetLLVMType(value_type);
      }
    }
    if (!value_type && !val.name.empty() && val.name.size() >= 2 &&
        val.name.front() == '"' && val.name.back() == '"') {
      value_type = analysis::MakeTypeString(analysis::StringState::View);
      llvm_ty = GetLLVMType(value_type);
    }
    if (value_type) {
      if (const auto* str = std::get_if<analysis::TypeString>(&value_type->node)) {
        if (str->state.has_value() &&
            *str->state == analysis::StringState::View &&
            llvm_ty) {
          SPEC_RULE("EmitLiteral-String");
          const std::string sym = MangleLiteral("string", val.bytes);
          if (!GetGlobal(sym)) {
            GlobalConst gc;
            gc.symbol = sym;
            gc.bytes = val.bytes;
            EmitGlobalConst(gc);
          }
          auto* gv = llvm::cast_or_null<llvm::GlobalVariable>(GetGlobal(sym));
          if (!gv) {
            return nullptr;
          }
          llvm::Type* ptr_ty = GetOpaquePtr();
          llvm::Type* usize_ty = GetLLVMType(analysis::MakeTypePrim("usize"));
          auto* zero = llvm::ConstantInt::get(usize_ty, 0);
          llvm::Constant* idxs[] = {zero, zero};
          auto* gep = llvm::ConstantExpr::getInBoundsGetElementPtr(
              gv->getValueType(), gv, llvm::ArrayRef<llvm::Constant*>(idxs));
          auto* ptr_const = llvm::ConstantExpr::getBitCast(gep, ptr_ty);
          auto* len_const = llvm::ConstantInt::get(usize_ty, val.bytes.size());
          auto* struct_ty = llvm::cast<llvm::StructType>(llvm_ty);
          return llvm::ConstantStruct::get(struct_ty, {ptr_const, len_const});
        }
      }
    }
    if (llvm_ty) {
      return ConstBytes(llvm_ty, val.bytes, *this, ctx);
    }
    if (!val.bytes.empty()) {
      auto int_ty = llvm::Type::getIntNTy(context_, static_cast<unsigned>(val.bytes.size() * 8));
      return llvm::ConstantInt::get(context_, APIntFromBytes(val.bytes));
    }
    return llvm::Constant::getNullValue(llvm::Type::getInt8Ty(context_));
  }

  if (val.kind == IRValue::Kind::Opaque) {
    if (val.name == "unit" || val.name == "()") {
      llvm::Type* unit_ty = GetLLVMType(analysis::MakeTypePrim("()"));
      return llvm::Constant::getNullValue(unit_ty);
    }
    if (ctx) {
      if (const auto* derived = ctx->LookupDerivedValue(val)) {
        llvm::Value* material = nullptr;
        switch (derived->kind) {
          case DerivedValueInfo::Kind::AddrLocal:
          case DerivedValueInfo::Kind::AddrStatic:
          case DerivedValueInfo::Kind::AddrField:
          case DerivedValueInfo::Kind::AddrTuple:
          case DerivedValueInfo::Kind::AddrIndex:
          case DerivedValueInfo::Kind::AddrDeref:
            material = MaterializeDerivedAddress(*this, builder, *derived);
            break;
          default:
            material = MaterializeDerivedValue(*this, builder, val, *derived);
            break;
        }
        if (material) {
          return material;
        }
      }
    }
    if (auto* cached = GetTempValue(val)) {
      return cached;
    }
    return nullptr;
  }

  return nullptr;
}

struct MatchValue {
  llvm::Value* addr = nullptr;
  analysis::TypeRef type;
};

static std::optional<MatchValue> MakeMatchValue(LLVMEmitter& emitter,
                                                llvm::IRBuilder<>* builder,
                                                LowerCtx* ctx,
                                                const IRValue& value,
                                                const analysis::TypeRef& type_hint) {
  if (!ctx) {
    return std::nullopt;
  }
  analysis::TypeRef type = type_hint ? type_hint : ctx->LookupValueType(value);
  if (!type) {
    if (std::getenv("CURSIVE0_DEBUG_OBJ")) {
      std::cerr << "[cursivec0] match scrutinee type missing for "
                << value.name << "\n";
    }
    return std::nullopt;
  }
  llvm::Value* val = emitter.EvaluateIRValue(value);
  if (!val) {
    if (std::getenv("CURSIVE0_DEBUG_OBJ")) {
      std::cerr << "[cursivec0] match scrutinee value missing for "
                << value.name << "\n";
    }
    return std::nullopt;
  }
  llvm::Type* llvm_ty = emitter.GetLLVMType(type);
  auto* alloca = CreateEntryAlloca(emitter, builder, llvm_ty, "match_val");
  if (!alloca) {
    return std::nullopt;
  }
  builder->CreateStore(val, alloca);
  MatchValue out;
  out.addr = alloca;
  out.type = StripPerm(type);
  return out;
}

static llvm::Value* LoadMatchValue(LLVMEmitter& emitter,
                                   llvm::IRBuilder<>* builder,
                                   const MatchValue& value) {
  if (!value.addr || !value.type) {
    return nullptr;
  }
  llvm::Type* llvm_ty = emitter.GetLLVMType(value.type);
  return builder->CreateLoad(llvm_ty, value.addr);
}

static const syntax::EnumDecl* LookupEnumDecl(const analysis::ScopeContext& scope,
                                              const analysis::TypePathType& path_type) {
  syntax::Path syntax_path;
  syntax_path.reserve(path_type.path.size());
  for (const auto& seg : path_type.path) {
    syntax_path.push_back(seg);
  }
  const auto it = scope.sigma.types.find(analysis::PathKeyOf(syntax_path));
  if (it == scope.sigma.types.end()) {
    return nullptr;
  }
  return std::get_if<syntax::EnumDecl>(&it->second);
}

static const syntax::ModalDecl* LookupModalDecl(const analysis::ScopeContext& scope,
                                                const analysis::TypePath& path) {
  syntax::Path syntax_path;
  syntax_path.reserve(path.size());
  for (const auto& seg : path) {
    syntax_path.push_back(seg);
  }
  const auto it = scope.sigma.types.find(analysis::PathKeyOf(syntax_path));
  if (it == scope.sigma.types.end()) {
    return nullptr;
  }
  return std::get_if<syntax::ModalDecl>(&it->second);
}

static const syntax::StateBlock* LookupModalState(const syntax::ModalDecl& decl,
                                                  std::string_view state) {
  for (const auto& st : decl.states) {
    if (analysis::IdEq(st.name, state)) {
      return &st;
    }
  }
  return nullptr;
}

static std::optional<std::vector<analysis::TypeRef>> ModalStateFieldTypes(
    const analysis::ScopeContext& scope,
    const syntax::StateBlock& state) {
  std::vector<analysis::TypeRef> types;
  for (const auto& member : state.members) {
    const auto* field = std::get_if<syntax::StateFieldDecl>(&member);
    if (!field) {
      continue;
    }
    const auto lowered = LowerTypeForLayout(scope, field->type);
    if (!lowered.has_value()) {
      return std::nullopt;
    }
    types.push_back(*lowered);
  }
  return types;
}

static std::optional<std::uint64_t> ModalPayloadAlign(const analysis::ScopeContext& scope,
                                                      const syntax::ModalDecl& decl) {
  std::uint64_t max_align = 1;
  for (const auto& state : decl.states) {
    const auto types = ModalStateFieldTypes(scope, state);
    if (!types.has_value()) {
      return std::nullopt;
    }
    const auto layout = RecordLayoutOf(scope, *types);
    if (!layout.has_value()) {
      return std::nullopt;
    }
    max_align = std::max(max_align, layout->layout.align);
  }
  return max_align;
}

static std::optional<MatchValue> RecordFieldValue(LLVMEmitter& emitter,
                                                  llvm::IRBuilder<>* builder,
                                                  const analysis::ScopeContext& scope,
                                                  const MatchValue& base,
                                                  std::string_view field) {
  if (!base.type) {
    return std::nullopt;
  }
  const auto* path = std::get_if<analysis::TypePathType>(&base.type->node);
  if (!path) {
    return std::nullopt;
  }
  auto field_info = LookupRecordField(scope, *path, std::string(field));
  if (!field_info.has_value()) {
    return std::nullopt;
  }
  llvm::Value* addr = field_info->offset == 0
                          ? base.addr
                          : ByteGEP(emitter, builder, base.addr, field_info->offset);
  MatchValue out;
  out.addr = addr;
  out.type = field_info->type;
  return out;
}

static std::optional<MatchValue> TupleElementValue(LLVMEmitter& emitter,
                                                   llvm::IRBuilder<>* builder,
                                                   const analysis::ScopeContext& scope,
                                                   const MatchValue& base,
                                                   std::size_t index) {
  if (!base.type) {
    return std::nullopt;
  }
  const auto* tuple = std::get_if<analysis::TypeTuple>(&base.type->node);
  if (!tuple) {
    return std::nullopt;
  }
  auto field_info = LookupTupleField(scope, *tuple, index);
  if (!field_info.has_value()) {
    return std::nullopt;
  }
  llvm::Value* addr = field_info->offset == 0
                          ? base.addr
                          : ByteGEP(emitter, builder, base.addr, field_info->offset);
  MatchValue out;
  out.addr = addr;
  out.type = field_info->type;
  return out;
}

static std::optional<MatchValue> EnumPayloadIndexValue(LLVMEmitter& emitter,
                                                       llvm::IRBuilder<>* builder,
                                                       const analysis::ScopeContext& scope,
                                                       const MatchValue& base,
                                                       const syntax::EnumDecl& decl,
                                                       const syntax::VariantDecl& variant,
                                                       std::size_t index) {
  if (!base.type) {
    return std::nullopt;
  }
  const auto layout = EnumLayoutOf(scope, decl);
  if (!layout.has_value()) {
    return std::nullopt;
  }
  const auto disc_type = analysis::MakeTypePrim(layout->disc_type);
  const auto disc_size = SizeOf(scope, disc_type).value_or(0);
  const std::uint64_t payload_offset = AlignUp(disc_size, layout->payload_align);

  const auto* tuple_payload =
      std::get_if<syntax::VariantPayloadTuple>(&*variant.payload_opt);
  if (!tuple_payload || index >= tuple_payload->elements.size()) {
    return std::nullopt;
  }
  std::vector<analysis::TypeRef> elems;
  elems.reserve(tuple_payload->elements.size());
  for (const auto& elem : tuple_payload->elements) {
    const auto lowered = LowerTypeForLayout(scope, elem);
    if (!lowered.has_value()) {
      return std::nullopt;
    }
    elems.push_back(*lowered);
  }
  const auto layout_rec = RecordLayoutOf(scope, elems);
  if (!layout_rec.has_value()) {
    return std::nullopt;
  }
  llvm::Value* addr = ByteGEP(emitter, builder, base.addr,
                              payload_offset + layout_rec->offsets[index]);
  MatchValue out;
  out.addr = addr;
  out.type = elems[index];
  return out;
}

static std::optional<MatchValue> EnumPayloadFieldValue(LLVMEmitter& emitter,
                                                       llvm::IRBuilder<>* builder,
                                                       const analysis::ScopeContext& scope,
                                                       const MatchValue& base,
                                                       const syntax::EnumDecl& decl,
                                                       const syntax::VariantDecl& variant,
                                                       std::string_view field) {
  if (!base.type) {
    return std::nullopt;
  }
  const auto layout = EnumLayoutOf(scope, decl);
  if (!layout.has_value()) {
    return std::nullopt;
  }
  const auto disc_type = analysis::MakeTypePrim(layout->disc_type);
  const auto disc_size = SizeOf(scope, disc_type).value_or(0);
  const std::uint64_t payload_offset = AlignUp(disc_size, layout->payload_align);

  const auto* record_payload =
      std::get_if<syntax::VariantPayloadRecord>(&*variant.payload_opt);
  if (!record_payload) {
    return std::nullopt;
  }
  std::vector<std::string> names;
  std::vector<analysis::TypeRef> types;
  names.reserve(record_payload->fields.size());
  types.reserve(record_payload->fields.size());
  for (const auto& f : record_payload->fields) {
    const auto lowered = LowerTypeForLayout(scope, f.type);
    if (!lowered.has_value()) {
      return std::nullopt;
    }
    names.push_back(f.name);
    types.push_back(*lowered);
  }
  const auto layout_rec = RecordLayoutOf(scope, types);
  if (!layout_rec.has_value()) {
    return std::nullopt;
  }
  for (std::size_t i = 0; i < names.size(); ++i) {
    if (names[i] == field) {
      llvm::Value* addr = ByteGEP(emitter, builder, base.addr,
                                  payload_offset + layout_rec->offsets[i]);
      MatchValue out;
      out.addr = addr;
      out.type = types[i];
      return out;
    }
  }
  return std::nullopt;
}

static std::optional<MatchValue> ModalFieldValue(LLVMEmitter& emitter,
                                                 llvm::IRBuilder<>* builder,
                                                 const analysis::ScopeContext& scope,
                                                 const MatchValue& base,
                                                 const syntax::ModalDecl& decl,
                                                 std::string_view state_name,
                                                 std::string_view field_name) {
  const auto* state = LookupModalState(decl, state_name);
  if (!state) {
    return std::nullopt;
  }
  std::vector<std::string> names;
  std::vector<analysis::TypeRef> types;
  for (const auto& member : state->members) {
    const auto* field = std::get_if<syntax::StateFieldDecl>(&member);
    if (!field) {
      continue;
    }
    const auto lowered = LowerTypeForLayout(scope, field->type);
    if (!lowered.has_value()) {
      return std::nullopt;
    }
    names.push_back(field->name);
    types.push_back(*lowered);
  }
  const auto layout_rec = RecordLayoutOf(scope, types);
  if (!layout_rec.has_value()) {
    return std::nullopt;
  }
  std::optional<std::size_t> field_index;
  for (std::size_t i = 0; i < names.size(); ++i) {
    if (names[i] == field_name) {
      field_index = i;
      break;
    }
  }
  if (!field_index.has_value()) {
    return std::nullopt;
  }

  std::uint64_t payload_offset = 0;
  if (std::holds_alternative<analysis::TypePathType>(base.type->node)) {
    const auto modal_layout = ModalLayoutOf(scope, decl);
    if (!modal_layout.has_value()) {
      return std::nullopt;
    }
    if (!modal_layout->niche && modal_layout->disc_type.has_value()) {
      const auto payload_align = ModalPayloadAlign(scope, decl);
      if (!payload_align.has_value()) {
        return std::nullopt;
      }
      const auto disc_type = analysis::MakeTypePrim(*modal_layout->disc_type);
      const auto disc_size = SizeOf(scope, disc_type).value_or(0);
      payload_offset = AlignUp(disc_size, *payload_align);
    }
  }

  llvm::Value* addr = ByteGEP(emitter, builder, base.addr,
                              payload_offset + layout_rec->offsets[*field_index]);
  MatchValue out;
  out.addr = addr;
  out.type = types[*field_index];
  return out;
}

static llvm::Value* LiteralConstValue(LLVMEmitter& emitter,
                                      LowerCtx* ctx,
                                      const analysis::TypeRef& type,
                                      const syntax::Token& lit) {
  if (!type) {
    return nullptr;
  }
  const auto bytes = EncodeConst(type, lit);
  if (!bytes.has_value()) {
    return nullptr;
  }
  llvm::Type* llvm_ty = emitter.GetLLVMType(type);
  return ConstBytes(llvm_ty, *bytes, emitter, ctx);
}

static void LowerMatchIR(LLVMEmitter& emitter,
                         llvm::IRBuilder<>* builder,
                         LowerCtx* ctx,
                         const IRMatch& match) {
  SPEC_RULE("LowerIR-Match");
  SPEC_RULE("Lower-MatchIR");
  if (!ctx) {
    return;
  }
  const auto scope = BuildScope(ctx);
  auto scrut_opt = MakeMatchValue(emitter, builder, ctx, match.scrutinee,
                                  match.scrutinee_type);
  if (!scrut_opt.has_value()) {
    ctx->ReportCodegenFailure();
    return;
  }
  MatchValue scrut = *scrut_opt;

  llvm::Function* func = builder->GetInsertBlock()->getParent();
  llvm::BasicBlock* merge_bb =
      llvm::BasicBlock::Create(emitter.GetContext(), "match_merge", func);
  llvm::BasicBlock* fail_bb =
      llvm::BasicBlock::Create(emitter.GetContext(), "match_fail", func);

  int match_fail_code = -1;
  if (std::getenv("CURSIVE0_DEBUG_MATCH_FAIL")) {
    static int match_fail_id = 0;
    match_fail_code = 40000 + match_fail_id++;
    if (std::getenv("CURSIVE0_DEBUG_MATCH_FAIL_LOG")) {
      std::cerr << "[cursivec0] match_fail_code=" << match_fail_code
                << " func=" << (func ? func->getName().str() : "<unknown>") << "\n";
    }
  }
  const bool debug_match_fail_val =
      std::getenv("CURSIVE0_DEBUG_MATCH_FAIL_VAL") != nullptr;

  std::vector<std::pair<llvm::Value*, llvm::BasicBlock*>> incoming;
  analysis::TypeRef result_type = ctx ? ctx->LookupValueType(match.result) : nullptr;
  llvm::Type* result_llvm_ty = result_type ? emitter.GetLLVMType(result_type) : nullptr;
  bool result_is_zst = false;
  if (result_type) {
    if (auto size_opt = SizeOf(scope, result_type)) {
      result_is_zst = (*size_opt == 0);
    }
  }

  auto emit_pattern = [&](const auto& self,
                          const syntax::Pattern& pat,
                          const MatchValue& value,
                          llvm::BasicBlock* match_bb,
                          llvm::BasicBlock* no_match_bb) -> void {
    std::visit(
        [&](const auto& node) {
          using T = std::decay_t<decltype(node)>;
          if constexpr (std::is_same_v<T, syntax::WildcardPattern>) {
            SPEC_RULE("Match-Wildcard");
            builder->CreateBr(match_bb);
          } else if constexpr (std::is_same_v<T, syntax::IdentifierPattern>) {
            SPEC_RULE("Match-Ident");
            builder->CreateBr(match_bb);
          } else if constexpr (std::is_same_v<T, syntax::TypedPattern>) {
            SPEC_RULE("Match-Typed");
            if (!value.type) {
              builder->CreateBr(no_match_bb);
              return;
            }
            const auto* uni = std::get_if<analysis::TypeUnion>(&value.type->node);
            if (!uni) {
              builder->CreateBr(no_match_bb);
              return;
            }
            const auto layout = UnionLayoutOf(scope, *uni);
            if (!layout.has_value()) {
              builder->CreateBr(no_match_bb);
              return;
            }
            const auto& members = layout->member_list;
            std::optional<std::size_t> member_index;
            std::optional<analysis::TypeRef> target;
            if (node.type) {
              target = LowerTypeForLayout(scope, node.type);
            }
            if (target.has_value()) {
              for (std::size_t i = 0; i < members.size(); ++i) {
                if (analysis::TypeEquiv(*target, members[i]).equiv) {
                  member_index = i;
                  break;
                }
              }
            }
            if (!member_index.has_value() && !node.type) {
              constexpr std::string_view kCasePrefix = "__case";
              if (node.name.size() > kCasePrefix.size() &&
                  node.name.rfind(kCasePrefix, 0) == 0) {
                std::size_t idx = 0;
                bool ok = true;
                for (std::size_t i = kCasePrefix.size(); i < node.name.size(); ++i) {
                  char c = node.name[i];
                  if (c < '0' || c > '9') {
                    ok = false;
                    break;
                  }
                  idx = idx * 10 + static_cast<std::size_t>(c - '0');
                  if (idx >= members.size()) {
                    ok = false;
                    break;
                  }
                }
                if (ok) {
                  member_index = idx;
                }
              }
            }
            if (!member_index.has_value()) {
              builder->CreateBr(no_match_bb);
              return;
            }
            if (layout->niche) {
              std::optional<std::size_t> payload_index;
              for (std::size_t i = 0; i < members.size(); ++i) {
                if (!IsUnitType(members[i])) {
                  payload_index = i;
                  break;
                }
              }
              if (!payload_index.has_value()) {
                builder->CreateBr(no_match_bb);
                return;
              }
              const auto payload_type = members[*payload_index];
              llvm::Value* payload_val =
                  builder->CreateLoad(emitter.GetLLVMType(payload_type), value.addr);
              if (!payload_val || !payload_val->getType()->isPointerTy()) {
                builder->CreateBr(no_match_bb);
                return;
              }
              llvm::Value* is_null = builder->CreateICmpEQ(
                  payload_val,
                  llvm::ConstantPointerNull::get(
                      llvm::cast<llvm::PointerType>(payload_val->getType())));
              llvm::Value* cond = (*member_index == *payload_index)
                                      ? builder->CreateNot(is_null)
                                      : is_null;
              builder->CreateCondBr(cond, match_bb, no_match_bb);
              return;
            }
            if (!layout->disc_type.has_value()) {
              builder->CreateBr(no_match_bb);
              return;
            }
            const auto disc_type = analysis::MakeTypePrim(*layout->disc_type);
            llvm::Type* disc_ty = emitter.GetLLVMType(disc_type);
            llvm::Value* disc =
                LoadAtOffset(emitter, builder, value.addr, 0, disc_ty);
            llvm::Value* expected =
                llvm::ConstantInt::get(disc_ty, *member_index);
            if (disc && expected && disc->getType() != expected->getType()) {
              if (disc->getType()->isIntegerTy() &&
                  expected->getType()->isIntegerTy()) {
                expected = builder->CreateIntCast(expected, disc->getType(), false);
              } else if (disc->getType()->isPointerTy() &&
                         expected->getType()->isPointerTy()) {
                expected = builder->CreateBitCast(expected, disc->getType());
              }
            }
            llvm::Value* cond = builder->CreateICmpEQ(disc, expected);
            builder->CreateCondBr(cond, match_bb, no_match_bb);
          } else if constexpr (std::is_same_v<T, syntax::LiteralPattern>) {
            SPEC_RULE("Match-Literal");
            llvm::Value* val = LoadMatchValue(emitter, builder, value);
            if (!val || !value.type) {
              builder->CreateBr(no_match_bb);
              return;
            }
            llvm::Value* lit =
                LiteralConstValue(emitter, ctx, value.type, node.literal);
            if (!lit) {
              builder->CreateBr(no_match_bb);
              return;
            }
            llvm::Value* cond = nullptr;
            if (val->getType()->isFloatingPointTy()) {
              cond = builder->CreateFCmpOEQ(val, lit);
            } else if (val->getType()->isIntegerTy() ||
                       val->getType()->isPointerTy()) {
              cond = builder->CreateICmpEQ(val, lit);
            } else if (const auto* str =
                           std::get_if<analysis::TypeString>(&value.type->node)) {
              if (str->state.has_value() &&
                  *str->state == analysis::StringState::View &&
                  val->getType()->isStructTy()) {
                llvm::Value* ptr_a = builder->CreateExtractValue(val, {0});
                llvm::Value* len_a = builder->CreateExtractValue(val, {1});
                llvm::Value* ptr_b = builder->CreateExtractValue(lit, {0});
                llvm::Value* len_b = builder->CreateExtractValue(lit, {1});
                llvm::Value* eq_ptr = builder->CreateICmpEQ(ptr_a, ptr_b);
                llvm::Value* eq_len = builder->CreateICmpEQ(len_a, len_b);
                cond = builder->CreateAnd(eq_ptr, eq_len);
              }
            } else if (const auto* bytes =
                           std::get_if<analysis::TypeBytes>(&value.type->node)) {
              if (bytes->state.has_value() &&
                  *bytes->state == analysis::BytesState::View &&
                  val->getType()->isStructTy()) {
                llvm::Value* ptr_a = builder->CreateExtractValue(val, {0});
                llvm::Value* len_a = builder->CreateExtractValue(val, {1});
                llvm::Value* ptr_b = builder->CreateExtractValue(lit, {0});
                llvm::Value* len_b = builder->CreateExtractValue(lit, {1});
                llvm::Value* eq_ptr = builder->CreateICmpEQ(ptr_a, ptr_b);
                llvm::Value* eq_len = builder->CreateICmpEQ(len_a, len_b);
                cond = builder->CreateAnd(eq_ptr, eq_len);
              }
            }
            if (!cond) {
              builder->CreateBr(no_match_bb);
              return;
            }
            builder->CreateCondBr(cond, match_bb, no_match_bb);
          } else if constexpr (std::is_same_v<T, syntax::TuplePattern>) {
            SPEC_RULE("Match-Tuple");
            if (node.elements.empty()) {
              builder->CreateBr(match_bb);
              return;
            }
            llvm::BasicBlock* next_bb = nullptr;
            for (std::size_t i = 0; i < node.elements.size(); ++i) {
              auto elem =
                  TupleElementValue(emitter, builder, scope, value, i);
              if (!elem.has_value()) {
                builder->CreateBr(no_match_bb);
                return;
              }
              if (i + 1 < node.elements.size()) {
                next_bb = llvm::BasicBlock::Create(
                    emitter.GetContext(), "pat_tuple_next", func);
                self(self, *node.elements[i], *elem, next_bb, no_match_bb);
                builder->SetInsertPoint(next_bb);
              } else {
                self(self, *node.elements[i], *elem, match_bb, no_match_bb);
              }
            }
          } else if constexpr (std::is_same_v<T, syntax::RecordPattern>) {
            SPEC_RULE("Match-Record");
            std::vector<const syntax::FieldPattern*> explicit_fields;
            for (const auto& field : node.fields) {
              if (field.pattern_opt) {
                explicit_fields.push_back(&field);
              }
            }
            if (explicit_fields.empty()) {
              builder->CreateBr(match_bb);
              return;
            }
            llvm::BasicBlock* next_bb = nullptr;
            for (std::size_t i = 0; i < explicit_fields.size(); ++i) {
              const auto& field = *explicit_fields[i];
              auto field_val =
                  RecordFieldValue(emitter, builder, scope, value, field.name);
              if (!field_val.has_value()) {
                builder->CreateBr(no_match_bb);
                return;
              }
              if (i + 1 < explicit_fields.size()) {
                next_bb = llvm::BasicBlock::Create(
                    emitter.GetContext(), "pat_record_next", func);
                self(self, *field.pattern_opt, *field_val, next_bb, no_match_bb);
                builder->SetInsertPoint(next_bb);
              } else {
                self(self, *field.pattern_opt, *field_val, match_bb, no_match_bb);
              }
            }
          } else if constexpr (std::is_same_v<T, syntax::EnumPattern>) {
            if (!value.type) {
              builder->CreateBr(no_match_bb);
              return;
            }
            const auto* path =
                std::get_if<analysis::TypePathType>(&value.type->node);
            if (!path) {
              builder->CreateBr(no_match_bb);
              return;
            }
            const auto* enum_decl = LookupEnumDecl(scope, *path);
            if (!enum_decl) {
              builder->CreateBr(no_match_bb);
              return;
            }
            const syntax::VariantDecl* variant = nullptr;
            for (const auto& v : enum_decl->variants) {
              if (analysis::IdEq(v.name, node.name)) {
                variant = &v;
                break;
              }
            }
            if (!variant) {
              builder->CreateBr(no_match_bb);
              return;
            }
            const auto discs = analysis::EnumDiscriminants(*enum_decl);
            if (!discs.ok || discs.discs.size() != enum_decl->variants.size()) {
              builder->CreateBr(no_match_bb);
              return;
            }
            std::optional<std::size_t> variant_index;
            for (std::size_t i = 0; i < enum_decl->variants.size(); ++i) {
              if (analysis::IdEq(enum_decl->variants[i].name, node.name)) {
                variant_index = i;
                break;
              }
            }
            if (!variant_index.has_value()) {
              builder->CreateBr(no_match_bb);
              return;
            }
            const auto layout = EnumLayoutOf(scope, *enum_decl);
            if (!layout.has_value()) {
              builder->CreateBr(no_match_bb);
              return;
            }
            const auto disc_type = analysis::MakeTypePrim(layout->disc_type);
            llvm::Type* disc_ty = emitter.GetLLVMType(disc_type);
            llvm::Value* disc =
                LoadAtOffset(emitter, builder, value.addr, 0, disc_ty);
            llvm::Value* expected =
                llvm::ConstantInt::get(disc_ty, discs.discs[*variant_index]);
            if (disc && expected && disc->getType() != expected->getType()) {
              if (disc->getType()->isIntegerTy() &&
                  expected->getType()->isIntegerTy()) {
                expected = builder->CreateIntCast(expected, disc->getType(), false);
              } else if (disc->getType()->isPointerTy() &&
                         expected->getType()->isPointerTy()) {
                expected = builder->CreateBitCast(expected, disc->getType());
              }
            }
            llvm::Value* tag_match = builder->CreateICmpEQ(disc, expected);

            if (!variant->payload_opt.has_value()) {
              SPEC_RULE("Match-Enum-Unit");
              builder->CreateCondBr(tag_match, match_bb, no_match_bb);
              return;
            }

            if (std::holds_alternative<syntax::VariantPayloadTuple>(
                    *variant->payload_opt)) {
              SPEC_RULE("Match-Enum-Tuple");
              if (!node.payload_opt.has_value()) {
                builder->CreateBr(no_match_bb);
                return;
              }
              const auto* tuple_pat =
                  std::get_if<syntax::TuplePayloadPattern>(&*node.payload_opt);
              if (!tuple_pat) {
                builder->CreateBr(no_match_bb);
                return;
              }
              llvm::BasicBlock* payload_bb = llvm::BasicBlock::Create(
                  emitter.GetContext(), "pat_enum_payload", func);
              builder->CreateCondBr(tag_match, payload_bb, no_match_bb);
              builder->SetInsertPoint(payload_bb);
              llvm::BasicBlock* next_bb = nullptr;
              for (std::size_t i = 0; i < tuple_pat->elements.size(); ++i) {
                auto elem = EnumPayloadIndexValue(emitter, builder, scope, value,
                                                  *enum_decl, *variant, i);
                if (!elem.has_value()) {
                  builder->CreateBr(no_match_bb);
                  return;
                }
                if (i + 1 < tuple_pat->elements.size()) {
                  next_bb = llvm::BasicBlock::Create(
                      emitter.GetContext(), "pat_enum_next", func);
                  self(self, *tuple_pat->elements[i], *elem, next_bb,
                       no_match_bb);
                  builder->SetInsertPoint(next_bb);
                } else {
                  self(self, *tuple_pat->elements[i], *elem, match_bb,
                       no_match_bb);
                }
              }
              return;
            }

            SPEC_RULE("Match-Enum-Record");
            if (!node.payload_opt.has_value()) {
              builder->CreateBr(no_match_bb);
              return;
            }
            const auto* record_pat =
                std::get_if<syntax::RecordPayloadPattern>(&*node.payload_opt);
            if (!record_pat) {
              builder->CreateBr(no_match_bb);
              return;
            }
            llvm::BasicBlock* payload_bb = llvm::BasicBlock::Create(
                emitter.GetContext(), "pat_enum_payload", func);
            builder->CreateCondBr(tag_match, payload_bb, no_match_bb);
            builder->SetInsertPoint(payload_bb);
            std::vector<const syntax::FieldPattern*> explicit_fields;
            for (const auto& field : record_pat->fields) {
              if (field.pattern_opt) {
                explicit_fields.push_back(&field);
              }
            }
            if (explicit_fields.empty()) {
              builder->CreateBr(match_bb);
              return;
            }
            llvm::BasicBlock* next_bb = nullptr;
            for (std::size_t i = 0; i < explicit_fields.size(); ++i) {
              const auto& field = *explicit_fields[i];
              auto field_val =
                  EnumPayloadFieldValue(emitter, builder, scope, value, *enum_decl,
                                        *variant, field.name);
              if (!field_val.has_value()) {
                builder->CreateBr(no_match_bb);
                return;
              }
              if (i + 1 < explicit_fields.size()) {
                next_bb = llvm::BasicBlock::Create(
                    emitter.GetContext(), "pat_enum_next", func);
                self(self, *field.pattern_opt, *field_val, next_bb,
                     no_match_bb);
                builder->SetInsertPoint(next_bb);
              } else {
                self(self, *field.pattern_opt, *field_val, match_bb,
                     no_match_bb);
              }
            }
          } else if constexpr (std::is_same_v<T, syntax::ModalPattern>) {
            if (!value.type) {
              builder->CreateBr(no_match_bb);
              return;
            }
            const syntax::ModalDecl* decl = nullptr;
            bool is_state_specific = false;
            if (const auto* path =
                    std::get_if<analysis::TypePathType>(&value.type->node)) {
              decl = LookupModalDecl(scope, path->path);
            } else if (const auto* state =
                           std::get_if<analysis::TypeModalState>(&value.type->node)) {
              decl = LookupModalDecl(scope, state->path);
              is_state_specific = true;
              if (!analysis::IdEq(state->state, node.state)) {
                builder->CreateBr(no_match_bb);
                return;
              }
            }
            if (!decl) {
              builder->CreateBr(no_match_bb);
              return;
            }
            if (!is_state_specific) {
              SPEC_RULE("Match-Modal-General");
            } else {
              SPEC_RULE("Match-Modal-State");
            }
            const bool has_fields = node.fields_opt.has_value();
            if (!is_state_specific) {
              if (has_fields) {
                SPEC_RULE("Match-Modal-Record");
              } else {
                SPEC_RULE("Match-Modal-Empty");
              }
            }

            llvm::Value* state_cond = nullptr;
            if (!is_state_specific) {
              const auto layout = ModalLayoutOf(scope, *decl);
              if (!layout.has_value()) {
                builder->CreateBr(no_match_bb);
                return;
              }
              if (layout->niche) {
                const auto payload_state = analysis::PayloadState(scope, *decl);
                if (!payload_state.has_value()) {
                  builder->CreateBr(no_match_bb);
                  return;
                }
                const auto* payload_block =
                    LookupModalState(*decl, *payload_state);
                if (!payload_block) {
                  builder->CreateBr(no_match_bb);
                  return;
                }
                std::optional<std::string_view> empty_state;
                for (const auto& st : decl->states) {
                  if (!analysis::IdEq(st.name, *payload_state)) {
                    empty_state = st.name;
                    break;
                  }
                }
                const auto payload_fields =
                    ModalStateFieldTypes(scope, *payload_block);
                if (!payload_fields.has_value() || payload_fields->empty()) {
                  builder->CreateBr(no_match_bb);
                  return;
                }
                llvm::Value* payload_val = builder->CreateLoad(
                    emitter.GetLLVMType((*payload_fields)[0]), value.addr);
                if (!payload_val || !payload_val->getType()->isPointerTy()) {
                  builder->CreateBr(no_match_bb);
                  return;
                }
                llvm::Value* is_null = builder->CreateICmpEQ(
                    payload_val,
                    llvm::ConstantPointerNull::get(
                        llvm::cast<llvm::PointerType>(payload_val->getType())));
                if (analysis::IdEq(node.state, *payload_state)) {
                  state_cond = builder->CreateNot(is_null);
                } else if (empty_state.has_value() &&
                           analysis::IdEq(node.state, *empty_state)) {
                  state_cond = is_null;
                } else {
                  state_cond = llvm::ConstantInt::getFalse(emitter.GetContext());
                }
              } else {
                if (!layout->disc_type.has_value()) {
                  builder->CreateBr(no_match_bb);
                  return;
                }
                std::size_t index = 0;
                bool found = false;
                for (std::size_t i = 0; i < decl->states.size(); ++i) {
                  if (analysis::IdEq(decl->states[i].name, node.state)) {
                    index = i;
                    found = true;
                    break;
                  }
                }
                if (!found) {
                  builder->CreateBr(no_match_bb);
                  return;
                }
                const auto disc_type = analysis::MakeTypePrim(*layout->disc_type);
                llvm::Type* disc_ty = emitter.GetLLVMType(disc_type);
                llvm::Value* disc =
                    LoadAtOffset(emitter, builder, value.addr, 0, disc_ty);
                llvm::Value* expected = llvm::ConstantInt::get(disc_ty, index);
                if (disc && expected && disc->getType() != expected->getType()) {
                  if (disc->getType()->isIntegerTy() &&
                      expected->getType()->isIntegerTy()) {
                    expected = builder->CreateIntCast(expected, disc->getType(), false);
                  } else if (disc->getType()->isPointerTy() &&
                             expected->getType()->isPointerTy()) {
                    expected = builder->CreateBitCast(expected, disc->getType());
                  }
                }
                state_cond = builder->CreateICmpEQ(disc, expected);
              }
            }

            if (!has_fields) {
              if (is_state_specific) {
                builder->CreateBr(match_bb);
              } else {
                builder->CreateCondBr(state_cond, match_bb, no_match_bb);
              }
              return;
            }

            llvm::BasicBlock* payload_bb = llvm::BasicBlock::Create(
                emitter.GetContext(), "pat_modal_payload", func);
            if (is_state_specific) {
              builder->CreateBr(payload_bb);
            } else {
              builder->CreateCondBr(state_cond, payload_bb, no_match_bb);
            }
            builder->SetInsertPoint(payload_bb);

            std::vector<const syntax::FieldPattern*> explicit_fields;
            for (const auto& field : node.fields_opt->fields) {
              if (field.pattern_opt) {
                explicit_fields.push_back(&field);
              }
            }
            if (explicit_fields.empty()) {
              builder->CreateBr(match_bb);
              return;
            }
            llvm::BasicBlock* next_bb = nullptr;
            for (std::size_t i = 0; i < explicit_fields.size(); ++i) {
              const auto& field = *explicit_fields[i];
              auto field_val = ModalFieldValue(emitter, builder, scope, value,
                                               *decl, node.state, field.name);
              if (!field_val.has_value()) {
                builder->CreateBr(no_match_bb);
                return;
              }
              if (i + 1 < explicit_fields.size()) {
                next_bb = llvm::BasicBlock::Create(
                    emitter.GetContext(), "pat_modal_next", func);
                self(self, *field.pattern_opt, *field_val, next_bb,
                     no_match_bb);
                builder->SetInsertPoint(next_bb);
              } else {
                self(self, *field.pattern_opt, *field_val, match_bb,
                     no_match_bb);
              }
            }
          } else if constexpr (std::is_same_v<T, syntax::RangePattern>) {
            SPEC_RULE("Match-Range");
            if (!value.type) {
              builder->CreateBr(no_match_bb);
              return;
            }
            if (!node.lo || !node.hi) {
              builder->CreateBr(no_match_bb);
              return;
            }
            const auto* lo_lit =
                std::get_if<syntax::LiteralPattern>(&node.lo->node);
            const auto* hi_lit =
                std::get_if<syntax::LiteralPattern>(&node.hi->node);
            if (!lo_lit || !hi_lit) {
              builder->CreateBr(no_match_bb);
              return;
            }
            llvm::Value* val = LoadMatchValue(emitter, builder, value);
            llvm::Value* lo =
                LiteralConstValue(emitter, ctx, value.type, lo_lit->literal);
            llvm::Value* hi =
                LiteralConstValue(emitter, ctx, value.type, hi_lit->literal);
            if (!val || !lo || !hi || !val->getType()->isIntegerTy()) {
              builder->CreateBr(no_match_bb);
              return;
            }
            bool is_unsigned = IsUnsignedType(value.type);
            llvm::Value* ge = is_unsigned ? builder->CreateICmpUGE(val, lo)
                                          : builder->CreateICmpSGE(val, lo);
            llvm::Value* lt = nullptr;
            if (node.kind == syntax::RangeKind::Inclusive) {
              lt = is_unsigned ? builder->CreateICmpULE(val, hi)
                               : builder->CreateICmpSLE(val, hi);
            } else {
              lt = is_unsigned ? builder->CreateICmpULT(val, hi)
                               : builder->CreateICmpSLT(val, hi);
            }
            llvm::Value* cond = builder->CreateAnd(ge, lt);
            builder->CreateCondBr(cond, match_bb, no_match_bb);
          } else {
            builder->CreateBr(no_match_bb);
          }
        },
        pat.node);
  };

  llvm::BasicBlock* next_bb = builder->GetInsertBlock();
  for (std::size_t i = 0; i < match.arms.size(); ++i) {
    llvm::BasicBlock* arm_body =
        llvm::BasicBlock::Create(emitter.GetContext(), "match_arm", func);
    llvm::BasicBlock* arm_next =
        (i + 1 < match.arms.size())
            ? llvm::BasicBlock::Create(emitter.GetContext(), "match_next", func)
            : fail_bb;

    builder->SetInsertPoint(next_bb);
    if (!match.arms[i].pattern) {
      builder->CreateBr(arm_body);
    } else {
      emit_pattern(emit_pattern, *match.arms[i].pattern, scrut, arm_body,
                   arm_next);
    }

    builder->SetInsertPoint(arm_body);
    if (ctx) {
      ctx->PushScope(false, false);
    }
    emitter.EmitIR(match.arms[i].body);
    if (ctx) {
      ctx->PopScope();
    }
    llvm::BasicBlock* arm_end = builder->GetInsertBlock();
    llvm::Value* arm_val = emitter.EvaluateIRValue(match.arms[i].value);
    if (!arm_val && result_llvm_ty) {
      if (result_is_zst) {
        arm_val = llvm::Constant::getNullValue(result_llvm_ty);
      } else {
        arm_val = llvm::UndefValue::get(result_llvm_ty);
        if (ctx) {
          ctx->ReportCodegenFailure();
        }
      }
    }
    if (arm_end && !arm_end->getTerminator()) {
      builder->CreateBr(merge_bb);
      if (arm_val) {
        incoming.push_back({arm_val, arm_end});
      }
    }

    next_bb = arm_next;
  }

  builder->SetInsertPoint(fail_bb);
  if (!fail_bb->getTerminator()) {
    if (llvm::Function* panic_fn = emitter.GetFunction(RuntimePanicSym())) {
      if (debug_match_fail_val) {
        llvm::Value* disc_val = nullptr;
        if (scrut.addr && scrut.type) {
          auto stripped = StripPerm(scrut.type);
          if (stripped) {
            if (const auto* uni = std::get_if<analysis::TypeUnion>(&stripped->node)) {
              if (const auto layout = UnionLayoutOf(scope, *uni)) {
                if (layout->disc_type.has_value()) {
                  const auto disc_type = analysis::MakeTypePrim(*layout->disc_type);
                  llvm::Type* disc_ty = emitter.GetLLVMType(disc_type);
                  disc_val = LoadAtOffset(emitter, builder, scrut.addr, 0, disc_ty);
                }
              }
            } else if (const auto* path =
                           std::get_if<analysis::TypePathType>(&stripped->node)) {
              syntax::Path syntax_path;
              syntax_path.reserve(path->path.size());
              for (const auto& seg : path->path) {
                syntax_path.push_back(seg);
              }
              const auto it = scope.sigma.types.find(analysis::PathKeyOf(syntax_path));
              if (it != scope.sigma.types.end()) {
                if (const auto* enum_decl = std::get_if<syntax::EnumDecl>(&it->second)) {
                  if (const auto layout = EnumLayoutOf(scope, *enum_decl)) {
                    const auto disc_type = analysis::MakeTypePrim(layout->disc_type);
                    llvm::Type* disc_ty = emitter.GetLLVMType(disc_type);
                    disc_val = LoadAtOffset(emitter, builder, scrut.addr, 0, disc_ty);
                  }
                }
              }
            }
          }
        }

        if (disc_val) {
          llvm::Type* i32_ty = llvm::Type::getInt32Ty(emitter.GetContext());
          llvm::Value* disc_i32 = disc_val;
          if (disc_val->getType() != i32_ty) {
            disc_i32 = builder->CreateZExtOrTrunc(disc_val, i32_ty);
          }
          llvm::Value* base =
              llvm::ConstantInt::get(i32_ty, 50000);
          llvm::Value* code_val = builder->CreateAdd(base, disc_i32);
          builder->CreateCall(panic_fn, {code_val});
        } else if (match_fail_code >= 0) {
          llvm::Value* code_val = llvm::ConstantInt::get(
              llvm::Type::getInt32Ty(emitter.GetContext()), match_fail_code);
          builder->CreateCall(panic_fn, {code_val});
        }
      } else if (match_fail_code >= 0) {
        llvm::Value* code_val = llvm::ConstantInt::get(
            llvm::Type::getInt32Ty(emitter.GetContext()), match_fail_code);
        builder->CreateCall(panic_fn, {code_val});
      }
    }
    builder->CreateUnreachable();
  }

  builder->SetInsertPoint(merge_bb);
  if (incoming.empty()) {
    if (result_llvm_ty && result_is_zst) {
      emitter.SetTempValue(match.result,
                           llvm::Constant::getNullValue(result_llvm_ty));
    }
    return;
  }
  if (!incoming.empty()) {
    llvm::Type* phi_ty = result_llvm_ty ? result_llvm_ty
                                        : incoming.front().first->getType();
    bool is_unsigned = IsUnsignedType(result_type);
    llvm::PHINode* phi = builder->CreatePHI(phi_ty, incoming.size());
    auto coerce_in_block = [&](llvm::Value* val, llvm::BasicBlock* pred) -> llvm::Value* {
      if (!val || val->getType() == phi_ty) {
        return val;
      }
      llvm::BasicBlock* saved_bb = builder->GetInsertBlock();
      auto saved_ip = builder->GetInsertPoint();
      llvm::Instruction* term = pred ? pred->getTerminator() : nullptr;
      if (term) {
        builder->SetInsertPoint(term);
      } else if (pred) {
        builder->SetInsertPoint(pred);
      }
      llvm::Value* casted = val;
      if (val->getType()->isPointerTy() && phi_ty->isPointerTy()) {
        casted = builder->CreateBitCast(val, phi_ty);
      } else if (val->getType()->isIntegerTy() && phi_ty->isIntegerTy()) {
        unsigned src_bits = val->getType()->getIntegerBitWidth();
        unsigned dst_bits = phi_ty->getIntegerBitWidth();
        if (dst_bits > src_bits) {
          casted = is_unsigned ? builder->CreateZExt(val, phi_ty)
                               : builder->CreateSExt(val, phi_ty);
        } else if (dst_bits < src_bits) {
          casted = builder->CreateTrunc(val, phi_ty);
        }
      } else if (val->getType()->isFloatingPointTy() && phi_ty->isFloatingPointTy()) {
        unsigned src_bits = val->getType()->getPrimitiveSizeInBits();
        unsigned dst_bits = phi_ty->getPrimitiveSizeInBits();
        if (dst_bits > src_bits) {
          casted = builder->CreateFPExt(val, phi_ty);
        } else if (dst_bits < src_bits) {
          casted = builder->CreateFPTrunc(val, phi_ty);
        }
      } else if (val->getType()->isIntegerTy() && phi_ty->isFloatingPointTy()) {
        casted = is_unsigned ? builder->CreateUIToFP(val, phi_ty)
                             : builder->CreateSIToFP(val, phi_ty);
      } else if (val->getType()->isFloatingPointTy() && phi_ty->isIntegerTy()) {
        casted = is_unsigned ? builder->CreateFPToUI(val, phi_ty)
                             : builder->CreateFPToSI(val, phi_ty);
      }
      builder->SetInsertPoint(saved_bb, saved_ip);
      if (casted && casted->getType() != phi_ty) {
        return nullptr;
      }
      return casted;
    };
    for (auto& entry : incoming) {
      llvm::Value* val = coerce_in_block(entry.first, entry.second);
      if (!val) {
        continue;
      }
      phi->addIncoming(val, entry.second);
    }
    emitter.SetTempValue(match.result, phi);
  }
}

// Visitor for IR node dispatch
struct IRVisitor {
  LLVMEmitter& emitter;
  llvm::IRBuilder<>* builder;
  LowerCtx* ctx;

  struct AsyncPayloadLayout;

  IRVisitor(LLVMEmitter& e)
      : emitter(e),
        builder(static_cast<llvm::IRBuilder<>*>(e.GetBuilderRaw())),
        ctx(e.GetCurrentCtx()) {}

  llvm::Value* AsI1(llvm::Value* v) {
    if (!v) {
      return nullptr;
    }
    if (v->getType()->isIntegerTy(1)) {
      return v;
    }
    if (v->getType()->isIntegerTy()) {
      return builder->CreateICmpNE(v, llvm::ConstantInt::get(v->getType(), 0));
    }
    if (v->getType()->isPointerTy()) {
      return builder->CreateICmpNE(
          v, llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(v->getType())));
    }
    return v;
  }

  llvm::Value* AsBool(llvm::Value* v) {
    if (!v) {
      return nullptr;
    }
    llvm::Type* bool_ty = emitter.GetLLVMType(analysis::MakeTypePrim("bool"));
    if (v->getType() == bool_ty) {
      return v;
    }
    llvm::Value* i1 = AsI1(v);
    if (!i1) {
      return v;
    }
    return builder->CreateZExt(i1, bool_ty);
  }

  llvm::Value* CoerceToType(llvm::Value* v, llvm::Type* target, bool is_unsigned) {
    if (!v || !target) {
      return v;
    }
    if (v->getType() == target) {
      return v;
    }
    if (v->getType()->isPointerTy() && target->isPointerTy()) {
      return builder->CreateBitCast(v, target);
    }
    if (v->getType()->isIntegerTy() && target->isPointerTy()) {
      return builder->CreateIntToPtr(v, target);
    }
    if (v->getType()->isPointerTy() && target->isIntegerTy()) {
      return builder->CreatePtrToInt(v, target);
    }
    if (v->getType()->isIntegerTy() && target->isIntegerTy()) {
      unsigned src_bits = v->getType()->getIntegerBitWidth();
      unsigned dst_bits = target->getIntegerBitWidth();
      if (dst_bits > src_bits) {
        return is_unsigned ? builder->CreateZExt(v, target) : builder->CreateSExt(v, target);
      }
      if (dst_bits < src_bits) {
        return builder->CreateTrunc(v, target);
      }
      return v;
    }
    if (v->getType()->isFloatingPointTy() && target->isFloatingPointTy()) {
      unsigned src_bits = v->getType()->getPrimitiveSizeInBits();
      unsigned dst_bits = target->getPrimitiveSizeInBits();
      if (dst_bits > src_bits) {
        return builder->CreateFPExt(v, target);
      }
      if (dst_bits < src_bits) {
        return builder->CreateFPTrunc(v, target);
      }
      return v;
    }
    if (v->getType()->isIntegerTy() && target->isFloatingPointTy()) {
      return is_unsigned ? builder->CreateUIToFP(v, target) : builder->CreateSIToFP(v, target);
    }
    if (v->getType()->isFloatingPointTy() && target->getTypeID() == llvm::Type::IntegerTyID) {
      return is_unsigned ? builder->CreateFPToUI(v, target) : builder->CreateFPToSI(v, target);
    }
    return v;
  }

  analysis::TypeRef ValueType(const IRValue& value) const {
    if (!ctx) {
      return nullptr;
    }
    return ctx->LookupValueType(value);
  }

  bool IsUnsignedValue(const IRValue& value) const {
    return IsUnsignedType(ValueType(value));
  }

  void StoreTemp(const IRValue& value, llvm::Value* v) {
    emitter.SetTempValue(value, v);
  }

  llvm::Value* BuildUnionValue(const analysis::TypeRef& union_type,
                               const analysis::TypeRef& member_type,
                               llvm::Value* member_val) {
    if (!union_type) {
      return member_val;
    }
    auto stripped = StripPerm(union_type);
    if (!stripped) {
      return member_val;
    }

    auto is_unit_like = [](const analysis::TypeRef& type) -> bool {
      if (!type) {
        return false;
      }
      auto base = StripPerm(type);
      if (!base) {
        return false;
      }
      if (const auto* prim = std::get_if<analysis::TypePrim>(&base->node)) {
        return prim->name == "()";
      }
      if (const auto* tup = std::get_if<analysis::TypeTuple>(&base->node)) {
        return tup->elements.empty();
      }
      return false;
    };

    const auto* uni = std::get_if<analysis::TypeUnion>(&stripped->node);
    llvm::Type* union_llvm_ty = emitter.GetLLVMType(stripped);
    if (!uni) {
      if (!union_llvm_ty) {
        return member_val;
      }
      if (!member_val) {
        return llvm::Constant::getNullValue(union_llvm_ty);
      }
      if (member_val->getType() != union_llvm_ty) {
        member_val = CoerceToType(member_val, union_llvm_ty, IsUnsignedType(member_type));
      }
      return member_val;
    }

    const auto scope = BuildScope(ctx);
    const auto layout = UnionLayoutOf(scope, *uni);
    if (!layout.has_value()) {
      return nullptr;
    }

    const auto& members = layout->member_list;
    std::optional<std::size_t> member_index;
    if (member_type) {
      for (std::size_t i = 0; i < members.size(); ++i) {
        if (analysis::TypeEquiv(member_type, members[i]).equiv) {
          member_index = i;
          break;
        }
      }
    }
    if (!member_index.has_value() && is_unit_like(member_type)) {
      for (std::size_t i = 0; i < members.size(); ++i) {
        if (is_unit_like(members[i])) {
          member_index = i;
          break;
        }
      }
    }
    if (!member_index.has_value() && members.size() == 1) {
      member_index = 0;
    }
    if (!member_index.has_value()) {
      return nullptr;
    }

    if (layout->niche) {
      std::optional<std::size_t> payload_index;
      for (std::size_t i = 0; i < members.size(); ++i) {
        if (!is_unit_like(members[i])) {
          payload_index = i;
          break;
        }
      }
      if (!payload_index.has_value()) {
        return nullptr;
      }
      const auto payload_type = members[*payload_index];
      llvm::Type* payload_llvm_ty = emitter.GetLLVMType(payload_type);
      if (!payload_llvm_ty) {
        return nullptr;
      }
      if (is_unit_like(member_type) || !member_val) {
        return llvm::Constant::getNullValue(payload_llvm_ty);
      }
      if (member_val->getType() != payload_llvm_ty) {
        member_val = CoerceToType(member_val, payload_llvm_ty,
                                  IsUnsignedType(member_type));
      }
      return member_val;
    }

    if (!layout->disc_type.has_value()) {
      return nullptr;
    }

    if (!union_llvm_ty) {
      return nullptr;
    }
    auto* alloca = CreateEntryAlloca(emitter, builder, union_llvm_ty, "union");
    if (!alloca) {
      return nullptr;
    }
    builder->CreateStore(llvm::Constant::getNullValue(union_llvm_ty), alloca);

    const auto disc_type = analysis::MakeTypePrim(*layout->disc_type);
    llvm::Type* disc_llvm_ty = emitter.GetLLVMType(disc_type);
    if (!disc_llvm_ty) {
      return nullptr;
    }
    llvm::Value* disc_val = llvm::ConstantInt::get(disc_llvm_ty, *member_index);
    StoreAtOffset(emitter, builder, alloca, 0, disc_val);

    const auto member = members[*member_index];
    if (!is_unit_like(member) && !IsNeverType(member)) {
      const auto size_opt = SizeOf(scope, member);
      if (size_opt.has_value() && *size_opt > 0) {
        llvm::Type* member_llvm_ty = emitter.GetLLVMType(member);
        if (!member_llvm_ty) {
          return nullptr;
        }
        if (!member_val) {
          member_val = llvm::Constant::getNullValue(member_llvm_ty);
        } else if (member_val->getType() != member_llvm_ty) {
          member_val = CoerceToType(member_val, member_llvm_ty,
                                    IsUnsignedType(member_type));
        }
        const std::uint64_t disc_size = SizeOf(scope, disc_type).value_or(0);
        const std::uint64_t payload_offset =
            AlignUp(disc_size, layout->payload_align);
        StoreAtOffset(emitter, builder, alloca, payload_offset, member_val);
      }
    }

    return builder->CreateLoad(union_llvm_ty, alloca);
  }

  void operator()(const IRSeq& seq) {
    SPEC_RULE("LowerIR-Seq");
    SPEC_RULE("LowerIRInstr-Seq");
    for (const auto& item : seq.items) {
      emitter.EmitIR(item);
    }
  }

  void operator()(const IRBindVar& bind) {
    SPEC_RULE("Lower-BindVarIR");
    emitter.EmitBindVar(bind);
  }

  void operator()(const IRStoreVar& store) {
    SPEC_RULE("LowerIR-StoreVar");
    SPEC_RULE("Lower-StoreVarIR");
    llvm::Value* val = emitter.EvaluateIRValue(store.value);
    llvm::Value* slot = emitter.GetLocal(store.name);
    if (!slot && ctx && ctx->resolve_name) {
      auto resolved = ctx->resolve_name(store.name);
      if (resolved.has_value() && !resolved->empty()) {
        std::vector<std::string> full = *resolved;
        const std::string resolved_name = full.back();
        full.pop_back();
        const std::string sym = StaticSymPath(full, resolved_name);
        if (ctx->LookupStaticType(sym)) {
          SPEC_RULE("BindSlot-Static");
          slot = emitter.GetGlobal(sym);
          if (!slot) {
            analysis::TypeRef ty = ctx->LookupStaticType(sym);
            llvm::Type* llvm_ty = ty ? emitter.GetLLVMType(ty) : nullptr;
            if (llvm_ty) {
              auto* gv = new llvm::GlobalVariable(
                  emitter.GetModule(),
                  llvm_ty,
                  false,
                  llvm::GlobalValue::ExternalLinkage,
                  nullptr,
                  sym);
              const auto scope = BuildScope(ctx);
              const auto align = AlignOf(scope, ty).value_or(1);
              gv->setAlignment(llvm::Align(align));
              slot = gv;
            }
          }
        }
      }
    }
    if (!val || !slot) {
      SPEC_RULE("BindSlot-Err");
      if (ctx) {
        ctx->ReportCodegenFailure();
      }
      return;
    }
    bool is_unsigned = IsUnsignedValue(store.value);
    if (auto* alloca = llvm::dyn_cast<llvm::AllocaInst>(slot)) {
      llvm::Type* dst_ty = alloca->getAllocatedType();
      val = CoerceToType(val, dst_ty, is_unsigned);
      builder->CreateStore(val, alloca);
      return;
    }
    if (slot->getType()->isPointerTy()) {
      llvm::Type* dst_ty = nullptr;
      if (auto* gv = llvm::dyn_cast<llvm::GlobalVariable>(slot)) {
        dst_ty = gv->getValueType();
      }
      if (!dst_ty) {
        analysis::TypeRef ty = nullptr;
        if (ctx) {
          if (const auto* state = ctx->GetBindingState(store.name)) {
            ty = state->type;
          }
        }
        dst_ty = ty ? emitter.GetLLVMType(ty) : val->getType();
      }
      val = CoerceToType(val, dst_ty, is_unsigned);
      builder->CreateStore(val, slot);
    }
  }

  void operator()(const IRStoreVarNoDrop& store) {
    SPEC_RULE("LowerIR-StoreVarNoDrop");
    SPEC_RULE("Lower-StoreVarNoDropIR");
    llvm::Value* val = emitter.EvaluateIRValue(store.value);
    llvm::Value* slot = emitter.GetLocal(store.name);
    if (!val || !slot) {
      SPEC_RULE("BindSlot-Err");
      if (ctx) {
        ctx->ReportCodegenFailure();
      }
      return;
    }
    bool is_unsigned = IsUnsignedValue(store.value);
    if (auto* alloca = llvm::dyn_cast<llvm::AllocaInst>(slot)) {
      llvm::Type* dst_ty = alloca->getAllocatedType();
      val = CoerceToType(val, dst_ty, is_unsigned);
      builder->CreateStore(val, alloca);
      return;
    }
    if (slot->getType()->isPointerTy()) {
      analysis::TypeRef ty = ctx ? ctx->LookupValueType(store.value) : nullptr;
      llvm::Type* dst_ty = ty ? emitter.GetLLVMType(ty) : val->getType();
      val = CoerceToType(val, dst_ty, is_unsigned);
      builder->CreateStore(val, slot);
    }
  }

  void operator()(const IRReadVar& read) {
    SPEC_RULE("LowerIR-ReadVar");
    if (!ctx) {
      return;
    }
    if (!emitter.GetLocal(read.name)) {
      SPEC_RULE("BindSlot-Err");
      ctx->ReportCodegenFailure();
      return;
    }
    const BindValidity validity = GetBindValidity(read.name, *ctx);
    if (validity != BindValidity::Valid) {
      SPEC_RULE("Lower-ReadVarIR-Err");
      ctx->ReportCodegenFailure();
      return;
    }
    SPEC_RULE("Lower-ReadVarIR");
  }

  void operator()(const IRStoreGlobal& store) {
    SPEC_RULE("LowerIR-StoreGlobal");
    SPEC_RULE("Lower-StoreGlobal");
    llvm::Value* val = emitter.EvaluateIRValue(store.value);
    llvm::Value* global = emitter.GetGlobal(store.symbol);
    if (!val || !global) {
      return;
    }
    llvm::Type* dst_ty = nullptr;
    if (auto* gv = llvm::dyn_cast<llvm::GlobalVariable>(global)) {
      dst_ty = gv->getValueType();
    }
    bool is_unsigned = IsUnsignedValue(store.value);
    if (dst_ty) {
      val = CoerceToType(val, dst_ty, is_unsigned);
      builder->CreateStore(val, global);
    }
  }

  void operator()(const IRCall& call) {
    SPEC_RULE("LowerIR-Call");
    llvm::Value* callee = emitter.EvaluateIRValue(call.callee);
    if (!callee) {
      return;
    }

    std::string sym;
    if (call.callee.kind == IRValue::Kind::Symbol) {
      sym = call.callee.name;
      if (auto alias = emitter.LookupSymbolAlias(call.callee.name)) {
        sym = *alias;
      }
    }

    if (ctx && !sym.empty()) {
      if (const auto* mod = ctx->LookupProcModule(sym)) {
        llvm::GlobalVariable* flag = GetOrCreatePoisonFlag(emitter, *mod);
        if (!flag) {
          SPEC_RULE("CheckPoison-Err");
          ctx->ReportCodegenFailure();
          return;
        }
        llvm::Type* bool_ty = emitter.GetLLVMType(analysis::MakeTypePrim("bool"));
        if (!bool_ty) {
          SPEC_RULE("CheckPoison-Err");
          ctx->ReportCodegenFailure();
          return;
        }
        llvm::Value* flag_val = builder->CreateLoad(bool_ty, flag);
        llvm::Value* ok = builder->CreateICmpEQ(
            flag_val, llvm::Constant::getNullValue(bool_ty));
        EmitPanicIfFalse(emitter, builder, ok, PanicCode(PanicReason::InitPanic));
      }
    }

    std::vector<llvm::Value*> arg_values;
    arg_values.reserve(call.args.size());
    for (const auto& arg : call.args) {
      arg_values.push_back(emitter.EvaluateIRValue(arg));
    }

    if (!sym.empty() && sym == BuiltinSymAsyncResume()) {
      if (arg_values.size() < 2) {
        if (ctx) {
          ctx->ReportCodegenFailure();
        }
        return;
      }

      analysis::TypeRef async_type;
      if (ctx) {
        async_type = ctx->LookupValueType(call.args[0]);
        if (!async_type) {
          async_type = ctx->LookupValueType(call.result);
        }
      }
      if (!async_type) {
        if (ctx) {
          ctx->ReportCodegenFailure();
        }
        return;
      }

      const auto scope = BuildScope(ctx);
      const auto payload_layout = AsyncPayloadLayoutOf(scope, async_type);
      if (!payload_layout.has_value()) {
        if (ctx) {
          ctx->ReportCodegenFailure();
        }
        return;
      }

      llvm::Value* async_val = arg_values[0];
      if (!async_val) {
        if (ctx) {
          ctx->ReportCodegenFailure();
        }
        return;
      }

      llvm::Type* async_llvm_ty = emitter.GetLLVMType(async_type);
      llvm::Value* async_struct_ptr = async_val;
      if (!async_val->getType()->isPointerTy()) {
        llvm::AllocaInst* async_alloca =
            CreateEntryAlloca(emitter, builder, async_llvm_ty, "async_resume");
        if (!async_alloca) {
          if (ctx) {
            ctx->ReportCodegenFailure();
          }
          return;
        }
        builder->CreateStore(async_val, async_alloca);
        async_struct_ptr = async_alloca;
      }

      llvm::Value* frame_ptr = LoadAtOffset(emitter, builder, async_struct_ptr,
                                            payload_layout->payload_offset +
                                                payload_layout->suspended_frame_offset,
                                            emitter.GetOpaquePtr());
      if (!frame_ptr) {
        if (ctx) {
          ctx->ReportCodegenFailure();
        }
        return;
      }

      llvm::Value* input_ptr = llvm::Constant::getNullValue(emitter.GetOpaquePtr());
      if (auto sig = analysis::GetAsyncSig(async_type)) {
        if (sig->in && !IsUnitType(sig->in)) {
          llvm::Type* in_llvm_ty = emitter.GetLLVMType(sig->in);
          llvm::AllocaInst* input_alloca =
              CreateEntryAlloca(emitter, builder, in_llvm_ty, "async_input");
          if (!input_alloca) {
            if (ctx) {
              ctx->ReportCodegenFailure();
            }
            return;
          }
          llvm::Value* input_val = arg_values[1];
          if (input_val) {
            input_val = CoerceToType(input_val, in_llvm_ty, IsUnsignedValue(call.args[1]));
            builder->CreateStore(input_val, input_alloca);
            input_ptr = builder->CreateBitCast(input_alloca, emitter.GetOpaquePtr());
          }
        }
      }

      llvm::Value* resumed = CallAsyncResume(emitter, builder, ctx,
                                             frame_ptr, input_ptr, async_type);
      if (!resumed) {
        if (ctx) {
          ctx->ReportCodegenFailure();
        }
        return;
      }
      StoreTemp(call.result, resumed);
      return;
    }

    llvm::Value* result = nullptr;
    const LowerCtx::ProcSigInfo* sig = nullptr;
    if (ctx && !sym.empty()) {
      sig = ctx->LookupProcSig(sym);
    }

    if (sig) {
      SPEC_RULE("Lower-CallIR-Func");
      ABICallResult abi = emitter.ComputeCallABI(sig->params, sig->ret);
      std::vector<llvm::Value*> call_args(abi.func_type->getNumParams(), nullptr);
      llvm::AllocaInst* sret_alloca = nullptr;
      if (abi.has_sret) {
        llvm::Function* func = builder->GetInsertBlock()->getParent();
        llvm::IRBuilder<> entry_builder(&func->getEntryBlock(), func->getEntryBlock().begin());
        llvm::Type* ret_ty = emitter.GetLLVMType(sig->ret);
        sret_alloca = entry_builder.CreateAlloca(ret_ty, nullptr, "sret");
        call_args[0] = sret_alloca;
      }

      for (std::size_t i = 0; i < sig->params.size(); ++i) {
        if (i >= abi.param_indices.size()) {
          break;
        }
        if (!abi.param_indices[i].has_value()) {
          continue;
        }
        unsigned idx = *abi.param_indices[i];
        if (idx >= call_args.size() || i >= arg_values.size()) {
          continue;
        }
        llvm::Value* arg = arg_values[i];
        if (!arg) {
          continue;
        }
        bool is_unsigned = IsUnsignedType(sig->params[i].type);
        analysis::TypeRef param_type = sig->params[i].type;
        analysis::TypeRef arg_type = ValueType(call.args[i]);
        const bool param_is_ptr = PtrElementType(param_type) != nullptr;
        bool arg_is_ptr = PtrElementType(arg_type) != nullptr;
        if (!arg_is_ptr && arg && arg->getType()->isPointerTy()) {
          arg_is_ptr = true;
        }
        if (param_is_ptr && !arg_is_ptr) {
          llvm::Type* elem_ty = arg_type ? emitter.GetLLVMType(arg_type) : nullptr;
          if (!elem_ty) {
            elem_ty = emitter.GetLLVMType(sig->params[i].type);
          }
          llvm::AllocaInst* slot = CreateEntryAlloca(emitter, builder, elem_ty, "ptr_arg");
          if (slot) {
            llvm::Value* stored = CoerceToType(arg, elem_ty, is_unsigned);
            builder->CreateStore(stored, slot);
            call_args[idx] = slot;
          }
          continue;
        }
        if (abi.param_kinds[i] == PassKind::ByRef) {
          analysis::TypeRef arg_elem = PtrElementType(arg_type);
          if (arg_elem && param_type) {
            analysis::TypeRef arg_base = StripPerm(arg_elem);
            analysis::TypeRef param_base = StripPerm(param_type);
            const auto equiv = analysis::TypeEquiv(arg_base ? arg_base : arg_elem,
                                               param_base ? param_base : param_type);
            if (equiv.ok && equiv.equiv) {
              llvm::Value* ptr_arg = arg;
              llvm::Type* target_ty = abi.param_types[idx];
              if (ptr_arg && target_ty && ptr_arg->getType() != target_ty) {
                if (ptr_arg->getType()->isPointerTy() && target_ty->isPointerTy()) {
                  ptr_arg = builder->CreateBitCast(ptr_arg, target_ty);
                } else {
                  ptr_arg = CoerceToType(ptr_arg, target_ty, true);
                }
              }
              call_args[idx] = ptr_arg;
              continue;
            }
          }
          // If arg is already a pointer (e.g., address of a place expression),
          // pass it directly for true pass-by-reference semantics
          if (arg->getType()->isPointerTy()) {
            llvm::Value* ptr_arg = arg;
            llvm::Type* target_ty = abi.param_types[idx];
            if (ptr_arg && target_ty && ptr_arg->getType() != target_ty) {
              if (target_ty->isPointerTy()) {
                ptr_arg = builder->CreateBitCast(ptr_arg, target_ty);
              }
            }
            call_args[idx] = ptr_arg;
            continue;
          }
          // Fallback: create a byref slot for non-pointer values
          llvm::Type* elem_ty = emitter.GetLLVMType(sig->params[i].type);
          llvm::AllocaInst* slot = CreateEntryAlloca(emitter, builder, elem_ty, "byref_arg");
          if (slot) {
            llvm::Value* stored = CoerceToType(arg, elem_ty, is_unsigned);
            builder->CreateStore(stored, slot);
            call_args[idx] = slot;
          }
          continue;
        }
        llvm::Type* target_ty = abi.param_types[idx];
        arg = CoerceToType(arg, target_ty, is_unsigned);
        call_args[idx] = arg;
      }

      for (std::size_t i = 0; i < call_args.size(); ++i) {
        if (!call_args[i]) {
          call_args[i] = llvm::Constant::getNullValue(abi.param_types[i]);
        }
      }

      llvm::CallInst* call_inst = builder->CreateCall(abi.func_type, callee, call_args);
      call_inst->setCallingConv(llvm::CallingConv::C);

      if (abi.has_sret && sret_alloca) {
        result = builder->CreateLoad(emitter.GetLLVMType(sig->ret), sret_alloca);
      } else if (!call_inst->getType()->isVoidTy()) {
        result = call_inst;
      } else {
        result = llvm::Constant::getNullValue(emitter.GetLLVMType(sig->ret));
      }
    } else {
      std::vector<llvm::Type*> arg_tys;
      arg_tys.reserve(arg_values.size());
      for (auto* arg : arg_values) {
        arg_tys.push_back(arg ? arg->getType() : emitter.GetLLVMType(analysis::MakeTypePrim("()")));
      }
      analysis::TypeRef ret_type = ValueType(call.result);
      llvm::Type* ret_ty = ret_type ? emitter.GetLLVMType(ret_type)
                                    : llvm::Type::getVoidTy(emitter.GetContext());
      llvm::FunctionType* ft = llvm::FunctionType::get(ret_ty, arg_tys, false);
      llvm::CallInst* call_inst = builder->CreateCall(ft, callee, arg_values);
      if (!ret_ty->isVoidTy()) {
        result = call_inst;
      } else if (ret_type) {
        result = llvm::Constant::getNullValue(emitter.GetLLVMType(ret_type));
      }
    }

    if (result) {
      StoreTemp(call.result, result);
    }
  }

  void operator()(const IRCallVTable& call) {
    SPEC_RULE("LowerIR-CallVTable");
    SPEC_RULE("Lower-CallVTable");
    llvm::Value* base_val = emitter.EvaluateIRValue(call.base);
    if (!base_val || !base_val->getType()->isStructTy()) {
      return;
    }

    llvm::Value* data_ptr = builder->CreateExtractValue(base_val, {0});
    llvm::Value* vtable_ptr = builder->CreateExtractValue(base_val, {1});
    if (!data_ptr || !vtable_ptr) {
      return;
    }

    std::uint64_t slot_index = static_cast<std::uint64_t>(call.slot + 3);
    llvm::Value* offset = llvm::ConstantInt::get(
        llvm::Type::getInt64Ty(emitter.GetContext()), slot_index * kPtrSize);
    llvm::Value* slot_addr = ByteGEP(emitter, builder, vtable_ptr, offset);
    llvm::Value* slot_fn = builder->CreateLoad(emitter.GetOpaquePtr(), slot_addr);

    std::vector<llvm::Value*> args;
    args.reserve(call.args.size() + 1);
    args.push_back(data_ptr);
    for (const auto& arg : call.args) {
      args.push_back(emitter.EvaluateIRValue(arg));
    }

    analysis::TypeRef ret_type = ValueType(call.result);
    llvm::Type* ret_ty = ret_type ? emitter.GetLLVMType(ret_type)
                                  : llvm::Type::getVoidTy(emitter.GetContext());
    llvm::FunctionType* ft = llvm::FunctionType::get(ret_ty, {}, false);
    if (!args.empty()) {
      std::vector<llvm::Type*> arg_tys;
      arg_tys.reserve(args.size());
      for (auto* arg : args) {
        arg_tys.push_back(arg ? arg->getType() : emitter.GetOpaquePtr());
      }
      ft = llvm::FunctionType::get(ret_ty, arg_tys, false);
    }
    llvm::CallInst* call_inst = builder->CreateCall(ft, slot_fn, args);
    if (ret_ty->isVoidTy()) {
      if (ret_type) {
        StoreTemp(call.result, llvm::Constant::getNullValue(emitter.GetLLVMType(ret_type)));
      }
    } else {
      StoreTemp(call.result, call_inst);
    }
  }

  void operator()(const IRReturn& ret) {
    SPEC_RULE("LowerIR-Return");
    llvm::Function* func = builder->GetInsertBlock()->getParent();
    llvm::Value* val = emitter.EvaluateIRValue(ret.value);

    if (auto* async_state = emitter.GetAsyncState()) {
      if (async_state->info && async_state->info->is_resume && async_state->frame_ptr) {
        llvm::Function* free_fn = emitter.GetFunction(BuiltinSymAsyncFreeFrame());
        if (!free_fn) {
          free_fn = emitter.GetModule().getFunction(BuiltinSymAsyncFreeFrame());
        }
        if (free_fn) {
          builder->CreateCall(free_fn, {async_state->frame_ptr});
        } else if (ctx) {
          ctx->ReportCodegenFailure();
        }
      }
    }

    if (func->arg_size() > 0 && func->hasParamAttribute(0, llvm::Attribute::StructRet)) {
      llvm::Argument* sret = func->getArg(0);
      if (val) {
        builder->CreateStore(val, sret);
      }
      builder->CreateRetVoid();
      return;
    }

    llvm::Type* ret_ty = func->getReturnType();
    if (ret_ty->isVoidTy()) {
      builder->CreateRetVoid();
      return;
    }
    if (!val) {
      builder->CreateRet(llvm::Constant::getNullValue(ret_ty));
      return;
    }
    if (val->getType() != ret_ty) {
      val = CoerceToType(val, ret_ty, IsUnsignedValue(ret.value));
    }
    builder->CreateRet(val);
  }

  void operator()(const IRResult& /*res*/) {
    SPEC_RULE("LowerIR-Result");
  }

  void operator()(const IRBreak& /*brk*/) {
    SPEC_RULE("LowerIR-Break");
  }

  void operator()(const IRContinue&) {
    SPEC_RULE("LowerIR-Continue");
  }

  void operator()(const IRUnaryOp& op) {
    SPEC_RULE("LowerIR-UnaryOp");
    llvm::Value* operand = emitter.EvaluateIRValue(op.operand);
    if (!operand) {
      return;
    }
    llvm::Value* result = nullptr;
    bool is_unsigned = IsUnsignedValue(op.operand);

    if (op.op == "widen") {
      LowerCtx* lower_ctx = emitter.GetCurrentCtx();
      if (!lower_ctx) {
        return;
      }
      const auto scope = BuildScope(lower_ctx);
      analysis::TypeRef operand_type = lower_ctx->LookupValueType(op.operand);
      analysis::TypeRef result_type = lower_ctx->LookupValueType(op.result);
      auto stripped_operand = StripPerm(operand_type);
      auto stripped_result = StripPerm(result_type);
      if (!stripped_operand) {
        return;
      }

      auto widen_string_bytes = [&](bool managed, const analysis::TypeRef& out_type) -> llvm::Value* {
        llvm::Type* out_ty = out_type ? emitter.GetLLVMType(out_type) : nullptr;
        if (!out_ty) {
          return nullptr;
        }
        auto base_opt = GetBaseAddress(emitter, builder, op.operand, operand_type);
        if (!base_opt) {
          return nullptr;
        }
        llvm::Value* base_ptr = base_opt->ptr;
        llvm::Type* ptr_ty = emitter.GetOpaquePtr();
        llvm::Type* usize_ty = emitter.GetLLVMType(analysis::MakeTypePrim("usize"));
        llvm::Value* ptr_val = LoadAtOffset(emitter, builder, base_ptr, 0, ptr_ty);
        llvm::Value* len_val = LoadAtOffset(emitter, builder, base_ptr, kPtrSize, usize_ty);
        llvm::Value* cap_val = managed
                                   ? LoadAtOffset(emitter, builder, base_ptr, 2 * kPtrSize, usize_ty)
                                   : llvm::ConstantInt::get(usize_ty, 0);
        if (!ptr_val || !len_val || !cap_val) {
          return nullptr;
        }
        auto* alloca = CreateEntryAlloca(emitter, builder, out_ty, "widen_sb");
        if (!alloca) {
          return nullptr;
        }
        builder->CreateStore(llvm::Constant::getNullValue(out_ty), alloca);

        llvm::Type* disc_ty = emitter.GetLLVMType(analysis::MakeTypePrim("u8"));
        llvm::Value* disc_val = llvm::ConstantInt::get(disc_ty, managed ? 0 : 1);
        StoreAtOffset(emitter, builder, alloca, 0, disc_val);
        const std::uint64_t payload_offset = AlignUp(1, kPtrAlign);
        StoreAtOffset(emitter, builder, alloca, payload_offset, ptr_val);
        StoreAtOffset(emitter, builder, alloca, payload_offset + kPtrSize, len_val);
        StoreAtOffset(emitter, builder, alloca, payload_offset + 2 * kPtrSize, cap_val);
        return builder->CreateLoad(out_ty, alloca);
      };

      if (const auto* str = std::get_if<analysis::TypeString>(&stripped_operand->node)) {
        if (str->state.has_value() && stripped_result &&
            std::holds_alternative<analysis::TypeString>(stripped_result->node)) {
          const auto* out_str = std::get_if<analysis::TypeString>(&stripped_result->node);
          if (out_str && !out_str->state.has_value()) {
            analysis::TypeRef out_type = result_type ? result_type : stripped_result;
            if (!out_type) {
              out_type = analysis::MakeTypeString(std::nullopt);
            }
            result = widen_string_bytes(*str->state == analysis::StringState::Managed, out_type);
          }
        }
      } else if (const auto* bytes = std::get_if<analysis::TypeBytes>(&stripped_operand->node)) {
        if (bytes->state.has_value() && stripped_result &&
            std::holds_alternative<analysis::TypeBytes>(stripped_result->node)) {
          const auto* out_bytes = std::get_if<analysis::TypeBytes>(&stripped_result->node);
          if (out_bytes && !out_bytes->state.has_value()) {
            analysis::TypeRef out_type = result_type ? result_type : stripped_result;
            if (!out_type) {
              out_type = analysis::MakeTypeBytes(std::nullopt);
            }
            result = widen_string_bytes(*bytes->state == analysis::BytesState::Managed, out_type);
          }
        }
      } else if (const auto* modal_state =
                     std::get_if<analysis::TypeModalState>(&stripped_operand->node)) {
        const auto* decl = LookupModalDecl(scope, modal_state->path);
        if (!decl) {
          return;
        }
        const auto* state = LookupModalState(*decl, modal_state->state);
        if (!state) {
          return;
        }
        const auto modal_layout = ModalLayoutOf(scope, *decl);
        if (!modal_layout.has_value()) {
          return;
        }

        if (modal_layout->niche) {
          const auto payload_state = analysis::PayloadState(scope, *decl);
          if (!payload_state.has_value()) {
            return;
          }
          const auto* payload_block = LookupModalState(*decl, *payload_state);
          if (!payload_block) {
            return;
          }
          const auto payload_types = ModalStateFieldTypes(scope, *payload_block);
          if (!payload_types.has_value() || payload_types->empty()) {
            return;
          }
          const auto payload_layout = RecordLayoutOf(scope, *payload_types);
          if (!payload_layout.has_value()) {
            return;
          }

          const analysis::TypeRef payload_type = (*payload_types)[0];
          llvm::Type* payload_llvm = emitter.GetLLVMType(payload_type);
          if (analysis::IdEq(modal_state->state, *payload_state)) {
            auto base_opt = GetBaseAddress(emitter, builder, op.operand, operand_type);
            if (!base_opt) {
              return;
            }
            const std::uint64_t offset = payload_layout->offsets.empty()
                                             ? 0
                                             : payload_layout->offsets[0];
            result = LoadAtOffset(emitter, builder, base_opt->ptr, offset, payload_llvm);
          } else {
            Value unit_payload;
            unit_payload.node = UnitVal{};
            auto payload_ptr = std::make_shared<Value>(std::move(unit_payload));
            Value modal_value;
            modal_value.node =
                ModalVal{std::string(modal_state->state), std::move(payload_ptr)};
            const auto bits = ValueBits(scope,
                                        analysis::MakeTypePath(modal_state->path),
                                        modal_value);
            if (!bits.has_value()) {
              return;
            }
            result = ConstBytes(payload_llvm, *bits, emitter, lower_ctx);
          }
        } else {
          if (!modal_layout->disc_type.has_value()) {
            return;
          }
          llvm::Type* out_ty = result_type ? emitter.GetLLVMType(result_type)
                                           : emitter.GetLLVMType(analysis::MakeTypePath(modal_state->path));
          if (!out_ty) {
            return;
          }
          auto* alloca = CreateEntryAlloca(emitter, builder, out_ty, "widen_modal");
          if (!alloca) {
            return;
          }
          builder->CreateStore(llvm::Constant::getNullValue(out_ty), alloca);

          std::size_t state_index = 0;
          bool found = false;
          for (std::size_t i = 0; i < decl->states.size(); ++i) {
            if (analysis::IdEq(decl->states[i].name, modal_state->state)) {
              state_index = i;
              found = true;
              break;
            }
          }
          if (!found) {
            return;
          }
          const auto disc_type = analysis::MakeTypePrim(*modal_layout->disc_type);
          llvm::Type* disc_ty = emitter.GetLLVMType(disc_type);
          llvm::Value* disc_val = llvm::ConstantInt::get(disc_ty, state_index);
          StoreAtOffset(emitter, builder, alloca, 0, disc_val);

          const auto payload_align = ModalPayloadAlign(scope, *decl);
          if (!payload_align.has_value()) {
            return;
          }
          const auto disc_size = SizeOf(scope, disc_type).value_or(0);
          const std::uint64_t payload_offset = AlignUp(disc_size, *payload_align);

          const auto field_types = ModalStateFieldTypes(scope, *state);
          if (!field_types.has_value()) {
            return;
          }
          const auto field_layout = RecordLayoutOf(scope, *field_types);
          if (!field_layout.has_value()) {
            return;
          }
          auto base_opt = GetBaseAddress(emitter, builder, op.operand, operand_type);
          if (!base_opt) {
            return;
          }
          for (std::size_t i = 0; i < field_types->size(); ++i) {
            const auto size_opt = SizeOf(scope, (*field_types)[i]);
            if (size_opt.has_value() && *size_opt == 0) {
              continue;
            }
            llvm::Type* field_ty = emitter.GetLLVMType((*field_types)[i]);
            llvm::Value* field_val = LoadAtOffset(
                emitter, builder, base_opt->ptr, field_layout->offsets[i], field_ty);
            StoreAtOffset(emitter, builder, alloca, payload_offset + field_layout->offsets[i], field_val);
          }

          result = builder->CreateLoad(out_ty, alloca);
        }
      }
    } else if (op.op == "-") {
      if (operand->getType()->isIntegerTy()) {
        llvm::Value* zero = llvm::ConstantInt::get(operand->getType(), 0);
        result = emitter.EmitCheckedSub(zero, operand, !is_unsigned);
      } else if (operand->getType()->isFloatingPointTy()) {
        result = builder->CreateFNeg(operand);
      }
    } else if (op.op == "!") {
      result = AsBool(builder->CreateNot(AsI1(operand)));
    } else if (op.op == "~") {
      result = builder->CreateNot(operand);
    }

    if (result) {
      StoreTemp(op.result, result);
    }
  }

  void operator()(const IRBinaryOp& op) {
    SPEC_RULE("LowerIR-BinaryOp");
    llvm::Value* lhs = emitter.EvaluateIRValue(op.lhs);
    llvm::Value* rhs = emitter.EvaluateIRValue(op.rhs);
    if (!lhs || !rhs) {
      return;
    }

    bool is_unsigned = IsUnsignedValue(op.lhs);
    llvm::Value* result = nullptr;

    if (op.op == "+") {
      result = emitter.EmitCheckedAdd(lhs, rhs, !is_unsigned);
    } else if (op.op == "-") {
      result = emitter.EmitCheckedSub(lhs, rhs, !is_unsigned);
    } else if (op.op == "*") {
      result = emitter.EmitCheckedMul(lhs, rhs, !is_unsigned);
    } else if (op.op == "/") {
      result = emitter.EmitCheckedDiv(lhs, rhs, !is_unsigned);
    } else if (op.op == "%") {
      result = emitter.EmitCheckedRem(lhs, rhs, !is_unsigned);
    } else if (op.op == "<<") {
      result = emitter.EmitCheckedShl(lhs, rhs);
    } else if (op.op == ">>") {
      result = emitter.EmitCheckedShr(lhs, rhs, !is_unsigned);
    } else if (op.op == "&") {
      result = builder->CreateAnd(lhs, rhs);
    } else if (op.op == "|") {
      result = builder->CreateOr(lhs, rhs);
    } else if (op.op == "^") {
      result = builder->CreateXor(lhs, rhs);
    } else if (op.op == "==" || op.op == "!=" || op.op == "<" ||
               op.op == "<=" || op.op == ">" || op.op == ">=") {
      if (lhs->getType() != rhs->getType()) {
        rhs = CoerceToType(rhs, lhs->getType(), is_unsigned);
      }
      llvm::Value* cmp = nullptr;
      if (lhs->getType()->isFloatingPointTy() || rhs->getType()->isFloatingPointTy()) {
        if (op.op == "==") cmp = builder->CreateFCmpOEQ(lhs, rhs);
        else if (op.op == "!=") cmp = builder->CreateFCmpONE(lhs, rhs);
        else if (op.op == "<") cmp = builder->CreateFCmpOLT(lhs, rhs);
        else if (op.op == "<=") cmp = builder->CreateFCmpOLE(lhs, rhs);
        else if (op.op == ">") cmp = builder->CreateFCmpOGT(lhs, rhs);
        else if (op.op == ">=") cmp = builder->CreateFCmpOGE(lhs, rhs);
      } else {
        if (op.op == "==") cmp = builder->CreateICmpEQ(lhs, rhs);
        else if (op.op == "!=") cmp = builder->CreateICmpNE(lhs, rhs);
        else if (op.op == "<") cmp = is_unsigned ? builder->CreateICmpULT(lhs, rhs)
                                                   : builder->CreateICmpSLT(lhs, rhs);
        else if (op.op == "<=") cmp = is_unsigned ? builder->CreateICmpULE(lhs, rhs)
                                                   : builder->CreateICmpSLE(lhs, rhs);
        else if (op.op == ">") cmp = is_unsigned ? builder->CreateICmpUGT(lhs, rhs)
                                                  : builder->CreateICmpSGT(lhs, rhs);
        else if (op.op == ">=") cmp = is_unsigned ? builder->CreateICmpUGE(lhs, rhs)
                                                   : builder->CreateICmpSGE(lhs, rhs);
      }
      result = AsBool(cmp);
    } else if (op.op == "&&" || op.op == "||") {
      llvm::Value* lhs_i1 = AsI1(lhs);
      llvm::Value* rhs_i1 = AsI1(rhs);
      llvm::Value* bool_val = (op.op == "&&") ? builder->CreateAnd(lhs_i1, rhs_i1)
                                               : builder->CreateOr(lhs_i1, rhs_i1);
      result = AsBool(bool_val);
    }

    if (result) {
      StoreTemp(op.result, result);
    }
  }

  void operator()(const IRCast& cast) {
    SPEC_RULE("LowerIR-Cast");
    llvm::Value* val = emitter.EvaluateIRValue(cast.value);
    llvm::Type* target = emitter.GetLLVMType(cast.target);
    if (!val || !target) {
      return;
    }

    bool is_unsigned = IsUnsignedType(cast.target);
    llvm::Value* result = nullptr;

    if (target->isIntegerTy(8) && cast.target &&
        std::holds_alternative<analysis::TypePrim>(cast.target->node) &&
        std::get<analysis::TypePrim>(cast.target->node).name == "bool") {
      result = AsBool(val);
    } else {
      result = CoerceToType(val, target, is_unsigned);
    }

    if (result) {
      StoreTemp(cast.result, result);
    }
  }

  void operator()(const IRTransmute& trans) {
    SPEC_RULE("LowerIR-Transmute");
    llvm::Value* val = emitter.EvaluateIRValue(trans.value);
    llvm::Type* target = emitter.GetLLVMType(trans.to);
    if (!val || !target) {
      return;
    }

    llvm::Value* result = nullptr;
    if (val->getType()->isPointerTy() && target->isPointerTy()) {
      result = builder->CreateBitCast(val, target);
    } else if (val->getType()->isPointerTy() && target->isIntegerTy()) {
      result = builder->CreatePtrToInt(val, target);
    } else if (val->getType()->isIntegerTy() && target->isPointerTy()) {
      result = builder->CreateIntToPtr(val, target);
    } else if (val->getType()->getPrimitiveSizeInBits() ==
               target->getPrimitiveSizeInBits()) {
      result = builder->CreateBitCast(val, target);
    } else {
      if (ctx) {
        ctx->ReportCodegenFailure();
      }
      result = builder->CreateBitCast(val, target);
    }

    if (result) {
      StoreTemp(trans.result, result);
    }
  }

  void operator()(const IRReadPtr& read) {
    SPEC_RULE("LowerIR-ReadPtr");
    llvm::Value* ptr = emitter.EvaluateIRValue(read.ptr);
    if (!ptr) {
      return;
    }

    analysis::TypeRef ptr_type = ValueType(read.ptr);
    if (ptr_type) {
      auto stripped = StripPerm(ptr_type);
      if (stripped) {
        if (std::holds_alternative<analysis::TypePtr>(stripped->node)) {
          SPEC_RULE("Lower-ReadPtrIR");
        } else if (std::holds_alternative<analysis::TypeRawPtr>(stripped->node)) {
          SPEC_RULE("Lower-ReadPtrIR-Raw");
        }
      }
    }
    if (ptr_type) {
      auto stripped = StripPerm(ptr_type);
      if (stripped) {
        if (const auto* ptr_info = std::get_if<analysis::TypePtr>(&stripped->node)) {
          if (ptr_info->state.has_value()) {
            if (*ptr_info->state == analysis::PtrState::Null) {
              SPEC_RULE("Lower-ReadPtrIR-Null");
              StorePanicRecord(emitter, builder, PanicCode(PanicReason::NullDeref));
              EmitReturn(emitter, builder);
              return;
            }
            if (*ptr_info->state == analysis::PtrState::Expired) {
              SPEC_RULE("Lower-ReadPtrIR-Expired");
              StorePanicRecord(emitter, builder, PanicCode(PanicReason::ExpiredDeref));
              EmitReturn(emitter, builder);
              return;
            }
          }
          if (!ptr_info->state.has_value() ||
              *ptr_info->state == analysis::PtrState::Valid) {
            llvm::Function* check_fn = emitter.GetFunction(RegionSymAddrIsActive());
            if (!check_fn) {
              if (ctx) {
                ctx->ReportCodegenFailure();
              }
              return;
            }
            llvm::Value* cast_ptr = builder->CreateBitCast(ptr, emitter.GetOpaquePtr());
            llvm::Value* active = builder->CreateCall(check_fn, {cast_ptr});
            llvm::Value* ok = active;
            if (active->getType()->isIntegerTy() &&
                active->getType()->getIntegerBitWidth() != 1) {
              ok = builder->CreateICmpNE(active,
                                         llvm::ConstantInt::get(active->getType(), 0));
            }
            EmitPanicReturnIfFalse(emitter, builder, ok,
                                   PanicCode(PanicReason::ExpiredDeref));
          }
        }
      }
    }
    analysis::TypeRef elem_type = PtrElementType(ptr_type);
    if (!elem_type || IsUnitType(elem_type)) {
      elem_type = ValueType(read.result);
    }

    llvm::Type* llvm_ty = elem_type ? emitter.GetLLVMType(elem_type)
                                    : llvm::Type::getInt8Ty(emitter.GetContext());
    llvm::Value* loaded = builder->CreateLoad(llvm_ty, ptr);
    StoreTemp(read.result, loaded);
  }

  void operator()(const IRWritePtr& write) {
    SPEC_RULE("LowerIR-WritePtr");
    llvm::Value* ptr = emitter.EvaluateIRValue(write.ptr);
    llvm::Value* val = emitter.EvaluateIRValue(write.value);
    if (!ptr || !val) {
      return;
    }

    analysis::TypeRef ptr_type = ValueType(write.ptr);
    analysis::TypeRef elem_type = PtrElementType(ptr_type);
    analysis::TypeRef val_type = ValueType(write.value);

    if (ptr_type) {
      auto stripped = StripPerm(ptr_type);
      if (!stripped) {
        return;
      }
      if (const auto* ptr_info = std::get_if<analysis::TypePtr>(&stripped->node)) {
        if (ptr_info->state == analysis::PtrState::Null) {
          SPEC_RULE("Lower-WritePtrIR-Null");
          StorePanicRecord(emitter, builder, PanicCode(PanicReason::NullDeref));
          EmitReturn(emitter, builder);
          return;
        }
        if (ptr_info->state == analysis::PtrState::Expired) {
          SPEC_RULE("Lower-WritePtrIR-Expired");
          StorePanicRecord(emitter, builder, PanicCode(PanicReason::ExpiredDeref));
          EmitReturn(emitter, builder);
          return;
        }
        SPEC_RULE("Lower-WritePtrIR");
        if (!ptr_info->state.has_value() ||
            ptr_info->state == analysis::PtrState::Valid) {
          llvm::Function* check_fn = emitter.GetFunction(RegionSymAddrIsActive());
          if (!check_fn) {
            if (ctx) {
              ctx->ReportCodegenFailure();
            }
            return;
          }
          llvm::Value* cast_ptr = builder->CreateBitCast(ptr, emitter.GetOpaquePtr());
          llvm::Value* active = builder->CreateCall(check_fn, {cast_ptr});
          llvm::Value* ok = active;
          if (active->getType()->isIntegerTy() &&
              active->getType()->getIntegerBitWidth() != 1) {
            ok = builder->CreateICmpNE(active,
                                       llvm::ConstantInt::get(active->getType(), 0));
          }
          EmitPanicReturnIfFalse(emitter, builder, ok,
                                 PanicCode(PanicReason::ExpiredDeref));
        }
      }
      if (const auto* raw_info = std::get_if<analysis::TypeRawPtr>(&stripped->node)) {
        if (raw_info->qual == analysis::RawPtrQual::Imm) {
          SPEC_RULE("Lower-WritePtrIR-Raw-Err");
          if (ctx) {
            ctx->ReportCodegenFailure();
          }
          return;
        }
        SPEC_RULE("Lower-WritePtrIR-Raw");
      }
    }

    bool needs_memmove = false;
    llvm::Value* src_ptr = nullptr;
    llvm::Value* len_val = nullptr;

    if (val_type) {
      auto stripped = StripPerm(val_type);
      if (std::holds_alternative<analysis::TypeSlice>(stripped->node) ||
          std::holds_alternative<analysis::TypeString>(stripped->node) ||
          std::holds_alternative<analysis::TypeBytes>(stripped->node)) {
        needs_memmove = true;
        src_ptr = ExtractSlicePtr(emitter, builder, val);
        len_val = ExtractSliceLen(emitter, builder, val);
      } else if (const auto* arr = std::get_if<analysis::TypeArray>(&stripped->node)) {
        needs_memmove = true;
        llvm::Type* llvm_ty = emitter.GetLLVMType(stripped);
        llvm::Function* func = builder->GetInsertBlock()->getParent();
        llvm::IRBuilder<> entry_builder(&func->getEntryBlock(), func->getEntryBlock().begin());
        auto* temp = entry_builder.CreateAlloca(llvm_ty, nullptr, "array_tmp");
        builder->CreateStore(val, temp);
        src_ptr = temp;
        len_val = llvm::ConstantInt::get(llvm::Type::getInt64Ty(emitter.GetContext()), arr->length);
      }
    }

    if (needs_memmove && elem_type && src_ptr && len_val) {
      const auto scope = BuildScope(ctx);
      const auto size_opt = SizeOf(scope, elem_type);
      if (!size_opt.has_value()) {
        return;
      }
      llvm::Value* stride = llvm::ConstantInt::get(
          llvm::Type::getInt64Ty(emitter.GetContext()), *size_opt);
      llvm::Value* bytes = builder->CreateMul(len_val, stride);
      emitter.EmitMemMove(ptr, src_ptr, bytes, AlignOf(scope, elem_type).value_or(1));
      return;
    }

    llvm::Type* dst_ty = elem_type ? emitter.GetLLVMType(elem_type) : val->getType();
    val = CoerceToType(val, dst_ty, IsUnsignedType(elem_type));
    builder->CreateStore(val, ptr);
  }

  void operator()(const IRAddrOf& addr) {
    SPEC_RULE("LowerIR-AddrOf");
    SPEC_RULE("Lower-AddrOfIR");
    llvm::Value* result = emitter.EvaluateIRValue(addr.result);
    if (result) {
      StoreTemp(addr.result, result);
    }
  }

  void operator()(const IRIf& if_node) {
    SPEC_RULE("LowerIR-If");
    SPEC_RULE("Lower-IfIR");
    llvm::Value* cond = emitter.EvaluateIRValue(if_node.cond);
    if (!cond) {
      return;
    }

    llvm::Function* func = builder->GetInsertBlock()->getParent();
    llvm::BasicBlock* then_bb = llvm::BasicBlock::Create(emitter.GetContext(), "then", func);
    llvm::BasicBlock* else_bb = llvm::BasicBlock::Create(emitter.GetContext(), "else", func);
    llvm::BasicBlock* merge_bb = llvm::BasicBlock::Create(emitter.GetContext(), "ifcont", func);

    builder->CreateCondBr(AsI1(cond), then_bb, else_bb);

    builder->SetInsertPoint(then_bb);
    emitter.EmitIR(if_node.then_ir);
    llvm::Value* then_val = emitter.EvaluateIRValue(if_node.then_value);
    llvm::BasicBlock* then_end = builder->GetInsertBlock();
    if (then_end && !then_end->getTerminator()) {
      builder->CreateBr(merge_bb);
    }

    builder->SetInsertPoint(else_bb);
    emitter.EmitIR(if_node.else_ir);
    llvm::Value* else_val = emitter.EvaluateIRValue(if_node.else_value);
    llvm::BasicBlock* else_end = builder->GetInsertBlock();
    if (else_end && !else_end->getTerminator()) {
      builder->CreateBr(merge_bb);
    }

    builder->SetInsertPoint(merge_bb);
    if (then_val && else_val && then_end && else_end) {
      analysis::TypeRef result_type = ctx ? ctx->LookupValueType(if_node.result) : nullptr;
      llvm::Type* phi_ty = result_type ? emitter.GetLLVMType(result_type)
                                       : then_val->getType();
      bool is_unsigned = IsUnsignedType(result_type);
      auto coerce_in_block = [&](llvm::Value* val, llvm::BasicBlock* pred) -> llvm::Value* {
        if (!val || val->getType() == phi_ty) {
          return val;
        }
        llvm::BasicBlock* saved_bb = builder->GetInsertBlock();
        auto saved_ip = builder->GetInsertPoint();
        llvm::Instruction* term = pred ? pred->getTerminator() : nullptr;
        if (term) {
          builder->SetInsertPoint(term);
        } else if (pred) {
          builder->SetInsertPoint(pred);
        }
        llvm::Value* casted = CoerceToType(val, phi_ty, is_unsigned);
        builder->SetInsertPoint(saved_bb, saved_ip);
        return casted;
      };
      llvm::PHINode* phi = builder->CreatePHI(phi_ty, 2);
      phi->addIncoming(coerce_in_block(then_val, then_end), then_end);
      phi->addIncoming(coerce_in_block(else_val, else_end), else_end);
      StoreTemp(if_node.result, phi);
    }
  }

  void operator()(const IRBlock& block) {
    SPEC_RULE("LowerIR-Block");
    SPEC_RULE("Lower-BlockIR");
    if (ctx) {
      ctx->PushScope(false, false);
    }
    emitter.EmitIR(block.setup);
    emitter.EmitIR(block.body);
    if (ctx) {
      ctx->PopScope();
    }
  }

  void operator()(const IRLoop& loop) {
    SPEC_RULE("LowerIR-Loop");
    SPEC_RULE("Lower-LoopIR");
    llvm::Function* func = builder->GetInsertBlock()->getParent();
    llvm::BasicBlock* loop_bb = llvm::BasicBlock::Create(emitter.GetContext(), "loop", func);
    llvm::BasicBlock* exit_bb = llvm::BasicBlock::Create(emitter.GetContext(), "loopexit", func);

    builder->CreateBr(loop_bb);
    builder->SetInsertPoint(loop_bb);

    if (loop.kind == IRLoopKind::Conditional && loop.cond_ir) {
      emitter.EmitIR(loop.cond_ir);
      llvm::Value* cond = emitter.EvaluateIRValue(*loop.cond_value);
      if (cond) {
        llvm::BasicBlock* body_bb = llvm::BasicBlock::Create(emitter.GetContext(), "loopbody", func);
        builder->CreateCondBr(AsI1(cond), body_bb, exit_bb);
        builder->SetInsertPoint(body_bb);
      }
    }

    emitter.EmitIR(loop.body_ir);
    if (!builder->GetInsertBlock()->getTerminator()) {
      builder->CreateBr(loop_bb);
    }

    builder->SetInsertPoint(exit_bb);
  }

  void operator()(const IRMatch& match) {
    LowerMatchIR(emitter, builder, ctx, match);
  }

  void operator()(const IRAlloc& alloc) {
    SPEC_RULE("LowerIR-Alloc");
    llvm::Value* val = emitter.EvaluateIRValue(alloc.value);
    const IRValue* region_value = nullptr;
    if (alloc.region.has_value()) {
      region_value = &*alloc.region;
    } else {
      region_value = emitter.CurrentActiveRegion();
    }

    if (!region_value) {
      if (ctx) {
        ctx->ReportCodegenFailure();
      }
      return;
    }

    llvm::Value* region = emitter.EvaluateIRValue(*region_value);
    if (!region) {
      if (ctx) {
        ctx->ReportCodegenFailure();
      }
      return;
    }

    analysis::TypeRef value_type = alloc.type;
    if (!value_type && ctx) {
      value_type = ctx->LookupValueType(alloc.value);
    }
    if (!value_type) {
      if (ctx) {
        ctx->ReportCodegenFailure();
      }
      return;
    }

    const auto scope = BuildScope(ctx);
    const auto size_opt = SizeOf(scope, value_type);
    const auto align_opt = AlignOf(scope, value_type);
    if (!size_opt.has_value()) {
      if (ctx) {
        ctx->ReportCodegenFailure();
      }
      return;
    }
    const std::uint64_t size = *size_opt;
    const std::uint64_t align = align_opt.value_or(1);

    llvm::Function* alloc_fn = emitter.GetFunction(RegionSymAlloc());
    if (!alloc_fn) {
      if (ctx) {
        ctx->ReportCodegenFailure();
      }
      return;
    }
    llvm::Type* usize_ty = llvm::Type::getInt64Ty(emitter.GetContext());
    llvm::Value* size_val = llvm::ConstantInt::get(usize_ty, size);
    llvm::Value* align_val = llvm::ConstantInt::get(usize_ty, align);

    SPEC_RULE("Lower-AllocIR");
    llvm::Value* raw_ptr = builder->CreateCall(alloc_fn, {region, size_val, align_val});
    StoreTemp(alloc.result, raw_ptr);

    if (!val) {
      return;
    }
    llvm::Type* llvm_ty = emitter.GetLLVMType(value_type);
    if (!llvm_ty) {
      if (ctx) {
        ctx->ReportCodegenFailure();
      }
      return;
    }
    bool is_unsigned = IsUnsignedValue(alloc.value);
    val = CoerceToType(val, llvm_ty, is_unsigned);
    llvm::Value* typed_ptr = builder->CreateBitCast(raw_ptr, llvm_ty->getPointerTo());
    builder->CreateStore(val, typed_ptr);
  }

  void operator()(const IRRegion& region) {
    SPEC_RULE("LowerIR-Region");
    SPEC_RULE("Lower-RegionIR");
    if (!ctx) {
      emitter.EmitIR(region.body);
      return;
    }

    IRValue region_value = ctx->FreshTempValue("region");
    IRCall create;
    create.callee.kind = IRValue::Kind::Symbol;
    create.callee.name = RegionSymNewScoped();
    create.args = {region.owner};
    create.result = region_value;
    (*this)(create);

    emitter.PushActiveRegion(region_value);

    std::optional<llvm::Value*> prior_alias;
    if (region.alias.has_value()) {
      const std::string& alias = *region.alias;
      if (auto* prev = emitter.GetLocal(alias)) {
        prior_alias = prev;
      }
      analysis::TypeRef alias_type;
      if (const auto* state = ctx->GetBindingState(alias)) {
        alias_type = state->type;
      } else {
        alias_type = analysis::MakeTypeModalState({"Region"}, "Active");
      }
      llvm::Type* llvm_ty = emitter.GetLLVMType(alias_type);
      if (llvm_ty) {
        llvm::AllocaInst* slot = CreateEntryAlloca(emitter, builder, llvm_ty, alias);
        if (slot) {
          llvm::Value* reg_val = emitter.EvaluateIRValue(region_value);
          if (reg_val) {
            builder->CreateStore(reg_val, slot);
          }
          emitter.SetLocal(alias, slot);
          if (ctx) {
            IRValue local_val;
            local_val.kind = IRValue::Kind::Local;
            local_val.name = alias;
            ctx->RegisterValueType(local_val, alias_type);
            ctx->RegisterVar(alias, alias_type, false, true);
          }
        }
      }
    }

    emitter.EmitIR(region.body);

    emitter.PopActiveRegion();

    IRValue released = ctx->FreshTempValue("region_free");
    IRCall free_call;
    free_call.callee.kind = IRValue::Kind::Symbol;
    free_call.callee.name = RegionSymFreeUnchecked();
    free_call.args = {region_value};
    free_call.result = released;
    (*this)(free_call);

    if (region.alias.has_value()) {
      const std::string& alias = *region.alias;
      if (prior_alias.has_value()) {
        emitter.SetLocal(alias, *prior_alias);
      } else {
        emitter.RemoveLocal(alias);
      }
    }
  }

  void operator()(const IRFrame& frame) {
    SPEC_RULE("LowerIR-Frame");
    SPEC_RULE("Lower-FrameIR");
    emitter.EmitIR(frame.body);
  }

  void operator()(const IRBranch& /*branch*/) {
    SPEC_RULE("LowerIR-Branch");
    SPEC_RULE("Lower-BranchIR");
  }

  void operator()(const IRPhi& /*phi*/) {
    SPEC_RULE("LowerIR-Phi");
    SPEC_RULE("Lower-PhiIR");
  }

  void operator()(const IRClearPanic&) {
    SPEC_RULE("LowerIR-ClearPanic");
    SPEC_RULE("LowerIRInstr-ClearPanic");
    ClearPanicRecord(emitter, builder);
  }

  void operator()(const IRPanicCheck& check) {
    SPEC_RULE("LowerIR-PanicCheck");
    SPEC_RULE("LowerIRInstr-PanicCheck");
    llvm::Value* ptr = LoadPanicOutPtr(emitter, builder);
    if (!ptr) {
      return;
    }
    llvm::Type* bool_ty = emitter.GetLLVMType(analysis::MakeTypePrim("bool"));
    llvm::Value* flag = LoadAtOffset(emitter, builder, ptr, 0, bool_ty);
    if (!flag) {
      return;
    }
    llvm::Value* is_panic = builder->CreateICmpNE(
        flag, llvm::Constant::getNullValue(flag->getType()));

    llvm::Function* func = builder->GetInsertBlock()->getParent();
    llvm::BasicBlock* ok_bb = llvm::BasicBlock::Create(emitter.GetContext(), "panic_ok", func);
    llvm::BasicBlock* fail_bb = llvm::BasicBlock::Create(emitter.GetContext(), "panic_fail", func);
    builder->CreateCondBr(is_panic, fail_bb, ok_bb);

    builder->SetInsertPoint(fail_bb);
    if (check.cleanup_ir) {
      emitter.EmitIR(check.cleanup_ir);
    }
    EmitReturn(emitter, builder);

    builder->SetInsertPoint(ok_bb);
    llvm::BasicBlock* cont_bb =
        llvm::BasicBlock::Create(emitter.GetContext(), "panic_cont", func);
    builder->CreateBr(cont_bb);
    builder->SetInsertPoint(cont_bb);
  }

  void operator()(const IRInitPanicHandle& handle) {
    llvm::Value* ptr = LoadPanicOutPtr(emitter, builder);
    if (!ptr) {
      return;
    }
    llvm::Type* bool_ty = emitter.GetLLVMType(analysis::MakeTypePrim("bool"));
    llvm::Value* flag = LoadAtOffset(emitter, builder, ptr, 0, bool_ty);
    if (!flag) {
      return;
    }
    llvm::Value* is_panic = builder->CreateICmpNE(
        flag, llvm::Constant::getNullValue(flag->getType()));

    llvm::Function* func = builder->GetInsertBlock()->getParent();
    llvm::BasicBlock* panic_bb =
        llvm::BasicBlock::Create(emitter.GetContext(), "init_panic", func);
    llvm::BasicBlock* ok_bb =
        llvm::BasicBlock::Create(emitter.GetContext(), "init_ok", func);
    builder->CreateCondBr(is_panic, panic_bb, ok_bb);

    builder->SetInsertPoint(panic_bb);
    const auto& poison = handle.poison_modules;
    if (!poison.empty()) {
      SPEC_RULE("SetPoison-OnInitFail");
      for (const auto& module_name : poison) {
        const auto path = SplitModulePathString(module_name);
        llvm::GlobalVariable* flag_var = GetOrCreatePoisonFlag(emitter, path);
        if (!flag_var) {
          SPEC_RULE("SetPoison-Err");
          if (ctx) {
            ctx->ReportCodegenFailure();
          }
          return;
        }
        llvm::Value* val = llvm::ConstantInt::get(bool_ty, 1);
        builder->CreateStore(val, flag_var);
      }
    } else if (!handle.module.empty()) {
      SPEC_RULE("SetPoison-OnInitFail");
      const auto path = SplitModulePathString(handle.module);
      llvm::GlobalVariable* flag_var = GetOrCreatePoisonFlag(emitter, path);
      if (!flag_var) {
        SPEC_RULE("SetPoison-Err");
        if (ctx) {
          ctx->ReportCodegenFailure();
        }
        return;
      }
      llvm::Value* val = llvm::ConstantInt::get(bool_ty, 1);
      builder->CreateStore(val, flag_var);
    }

    StorePanicRecord(emitter, builder, PanicCode(PanicReason::InitPanic));
    EmitReturn(emitter, builder);

    builder->SetInsertPoint(ok_bb);
    llvm::BasicBlock* cont_bb =
        llvm::BasicBlock::Create(emitter.GetContext(), "init_cont", func);
    builder->CreateBr(cont_bb);
    builder->SetInsertPoint(cont_bb);
  }

  void operator()(const IRCheckPoison& check) {
    SPEC_RULE("LowerIR-CheckPoison");
    SPEC_RULE("LowerIRInstr-CheckPoison");
    SPEC_RULE("CheckPoison-Use");
    const std::vector<std::string> path = SplitModulePathString(check.module);
    llvm::GlobalVariable* flag = GetOrCreatePoisonFlag(emitter, path);
    if (!flag) {
      SPEC_RULE("CheckPoison-Err");
      if (ctx) {
        ctx->ReportCodegenFailure();
      }
      return;
    }
    llvm::Type* bool_ty = emitter.GetLLVMType(analysis::MakeTypePrim("bool"));
    if (!bool_ty) {
      SPEC_RULE("CheckPoison-Err");
      if (ctx) {
        ctx->ReportCodegenFailure();
      }
      return;
    }
    llvm::Value* flag_val = builder->CreateLoad(bool_ty, flag);
    llvm::Value* ok = builder->CreateICmpEQ(
        flag_val, llvm::Constant::getNullValue(bool_ty));
    EmitPanicIfFalse(emitter, builder, ok, PanicCode(PanicReason::InitPanic));
  }

  void operator()(const IRLowerPanic& panic) {
    SPEC_RULE("LowerIR-LowerPanic");
    SPEC_RULE("LowerIRInstr-LowerPanic");
    StorePanicRecord(emitter, builder, PanicCodeFromString(panic.reason));
    if (panic.cleanup_ir) {
      emitter.EmitIR(panic.cleanup_ir);
    }
    EmitReturn(emitter, builder);
    llvm::Function* func = builder->GetInsertBlock()->getParent();
    llvm::BasicBlock* cont_bb =
        llvm::BasicBlock::Create(emitter.GetContext(), "panic_cont", func);
    builder->SetInsertPoint(cont_bb);
  }

  void operator()(const IRCheckIndex& check) {
    SPEC_RULE("LowerIR-CheckIndex");
    analysis::TypeRef base_type = ValueType(check.base);
    llvm::Value* len = SliceLenFromValue(emitter, builder, check.base, base_type);
    if (!len) {
      return;
    }
    llvm::Value* idx = emitter.EvaluateIRValue(check.index);
    idx = AsUSize(builder, idx, emitter.GetContext());
    if (!idx) {
      return;
    }
    llvm::Value* ok = builder->CreateICmpULT(idx, len);
    EmitPanicIfFalse(emitter, builder, ok, PanicCode(PanicReason::Bounds));
  }

  void operator()(const IRCheckRange& check) {
    SPEC_RULE("LowerIR-CheckRange");
    analysis::TypeRef base_type = ValueType(check.base);
    llvm::Value* len = SliceLenFromValue(emitter, builder, check.base, base_type);
    if (!len) {
      return;
    }
    auto bounds = ComputeRangeBounds(check.range, len, emitter, builder);
    EmitPanicIfFalse(emitter, builder, bounds.ok, PanicCode(PanicReason::Bounds));
  }

  void operator()(const IRCheckSliceLen& check) {
    SPEC_RULE("LowerIR-CheckSliceLen");
    analysis::TypeRef base_type = ValueType(check.base);
    analysis::TypeRef val_type = ValueType(check.value);
    llvm::Value* len = SliceLenFromValue(emitter, builder, check.base, base_type);
    llvm::Value* val_len = SliceLenFromValue(emitter, builder, check.value, val_type);
    if (!len || !val_len) {
      return;
    }
    auto bounds = ComputeRangeBounds(check.range, len, emitter, builder);
    llvm::Value* expected = builder->CreateSub(bounds.end, bounds.start);
    llvm::Value* ok = builder->CreateICmpEQ(val_len, expected);
    EmitPanicIfFalse(emitter, builder, ok, PanicCode(PanicReason::Bounds));
  }

  void operator()(const IRCheckOp& check) {
    SPEC_RULE("LowerIR-CheckOp");
    llvm::Value* lhs = emitter.EvaluateIRValue(check.lhs);
    llvm::Value* rhs = check.rhs.has_value() ? emitter.EvaluateIRValue(*check.rhs) : nullptr;
    if (!lhs) {
      return;
    }

    if (check.op == "addr_active") {
      llvm::Value* ok = lhs;
      if (lhs->getType()->isIntegerTy() &&
          lhs->getType()->getIntegerBitWidth() != 1) {
        ok = builder->CreateICmpNE(
            lhs, llvm::ConstantInt::get(lhs->getType(), 0));
      }
      EmitPanicIfFalse(emitter, builder, ok, PanicCodeFromString(check.reason));
      return;
    }

    llvm::Value* ok = nullptr;
    bool is_unsigned = IsUnsignedValue(check.lhs);

    if (check.op == "/" || check.op == "%") {
      if (rhs) {
        ok = builder->CreateICmpNE(rhs, llvm::ConstantInt::get(rhs->getType(), 0));
        if (!is_unsigned && lhs->getType()->isIntegerTy()) {
          auto bits = lhs->getType()->getIntegerBitWidth();
          auto min = llvm::APInt::getSignedMinValue(bits);
          auto neg_one = llvm::APInt::getAllOnes(bits);
          llvm::Value* is_min = builder->CreateICmpEQ(lhs, llvm::ConstantInt::get(lhs->getType(), min));
          llvm::Value* is_neg1 = builder->CreateICmpEQ(rhs, llvm::ConstantInt::get(rhs->getType(), neg_one));
          llvm::Value* overflow = builder->CreateAnd(is_min, is_neg1);
          ok = builder->CreateAnd(ok, builder->CreateNot(overflow));
        }
      }
    } else if (check.op == "<<" || check.op == ">>") {
      if (rhs && lhs->getType()->isIntegerTy()) {
        unsigned bits = lhs->getType()->getIntegerBitWidth();
        llvm::Value* rhs_cast = CoerceToType(rhs, lhs->getType(), true);
        llvm::Value* limit = llvm::ConstantInt::get(lhs->getType(), bits);
        ok = builder->CreateICmpULT(rhs_cast, limit);
      }
    } else if (check.op == "+" || check.op == "-" || check.op == "*") {
      if (rhs && lhs->getType()->isIntegerTy()) {
        llvm::Intrinsic::ID id = llvm::Intrinsic::sadd_with_overflow;
        if (check.op == "+") {
          id = is_unsigned ? llvm::Intrinsic::uadd_with_overflow : llvm::Intrinsic::sadd_with_overflow;
        } else if (check.op == "-") {
          id = is_unsigned ? llvm::Intrinsic::usub_with_overflow : llvm::Intrinsic::ssub_with_overflow;
        } else if (check.op == "*") {
          id = is_unsigned ? llvm::Intrinsic::umul_with_overflow : llvm::Intrinsic::smul_with_overflow;
        }
        llvm::Function* intrinsic = llvm::Intrinsic::getDeclaration(&emitter.GetModule(), id, {lhs->getType()});
        llvm::Value* res = builder->CreateCall(intrinsic, {lhs, rhs});
        llvm::Value* overflow = builder->CreateExtractValue(res, 1);
        ok = builder->CreateNot(overflow);
      }
    } else if (check.op == "-" && !rhs) {
      if (lhs->getType()->isIntegerTy() && !is_unsigned) {
        auto bits = lhs->getType()->getIntegerBitWidth();
        auto min = llvm::APInt::getSignedMinValue(bits);
        ok = builder->CreateICmpNE(lhs, llvm::ConstantInt::get(lhs->getType(), min));
      } else {
        ok = llvm::ConstantInt::getTrue(emitter.GetContext());
      }
    }

    if (ok) {
      EmitPanicIfFalse(emitter, builder, ok, PanicCodeFromString(check.reason));
    }
  }

  void operator()(const IRCheckCast& check) {
    SPEC_RULE("LowerIR-CheckCast");
    analysis::TypeRef src_type = ValueType(check.value);
    analysis::TypeRef dst_type = check.target;
    if (!src_type || !dst_type) {
      return;
    }

    auto stripped_src = StripPerm(src_type);
    auto stripped_dst = StripPerm(dst_type);
    if (!stripped_src || !stripped_dst) {
      return;
    }

    auto* src_prim = std::get_if<analysis::TypePrim>(&stripped_src->node);
    auto* dst_prim = std::get_if<analysis::TypePrim>(&stripped_dst->node);
    if (!src_prim || !dst_prim) {
      return;
    }

    llvm::Value* val = emitter.EvaluateIRValue(check.value);
    if (!val) {
      return;
    }

    auto float_check = [&](bool is_unsigned) {
      if (!val->getType()->isFloatingPointTy()) {
        return;
      }
      llvm::Type* dst_ty = emitter.GetLLVMType(dst_type);
      if (!dst_ty->isIntegerTy()) {
        return;
      }
      unsigned bits = dst_ty->getIntegerBitWidth();
      llvm::APInt min_int(bits, 0);
      llvm::APInt max_int(bits, 0);
      if (is_unsigned) {
        min_int = llvm::APInt::getMinValue(bits);
        max_int = llvm::APInt::getMaxValue(bits);
      } else {
        min_int = llvm::APInt::getSignedMinValue(bits);
        max_int = llvm::APInt::getSignedMaxValue(bits);
      }
      llvm::APFloat min_f(val->getType()->getFltSemantics());
      llvm::APFloat max_f(val->getType()->getFltSemantics());
      min_f.convertFromAPInt(min_int, !is_unsigned, llvm::APFloat::rmNearestTiesToEven);
      max_f.convertFromAPInt(max_int, !is_unsigned, llvm::APFloat::rmNearestTiesToEven);
      llvm::Value* min_c = llvm::ConstantFP::get(val->getType(), min_f);
      llvm::Value* max_c = llvm::ConstantFP::get(val->getType(), max_f);

      llvm::Value* isnan = builder->CreateFCmpUNO(val, val);
      llvm::Value* ge = builder->CreateFCmpOGE(val, min_c);
      llvm::Value* le = builder->CreateFCmpOLE(val, max_c);
      llvm::Value* ok = builder->CreateAnd(builder->CreateAnd(ge, le), builder->CreateNot(isnan));
      EmitPanicIfFalse(emitter, builder, ok, PanicCode(PanicReason::Cast));
    };

    if (src_prim->name == "u32" && dst_prim->name == "char") {
      llvm::Value* max_cp = llvm::ConstantInt::get(val->getType(), 0x10FFFFu);
      llvm::Value* ge_min = builder->CreateICmpULE(val, max_cp);
      llvm::Value* lo_sur = builder->CreateICmpULT(val, llvm::ConstantInt::get(val->getType(), 0xD800u));
      llvm::Value* hi_sur = builder->CreateICmpUGT(val, llvm::ConstantInt::get(val->getType(), 0xDFFFu));
      llvm::Value* not_sur = builder->CreateOr(lo_sur, hi_sur);
      llvm::Value* ok = builder->CreateAnd(ge_min, not_sur);
      EmitPanicIfFalse(emitter, builder, ok, PanicCode(PanicReason::Cast));
      return;
    }

    if ((src_prim->name == "f16" || src_prim->name == "f32" || src_prim->name == "f64") &&
        (dst_prim->name == "i8" || dst_prim->name == "i16" || dst_prim->name == "i32" ||
         dst_prim->name == "i64" || dst_prim->name == "i128" || dst_prim->name == "isize")) {
      float_check(false);
      return;
    }

    if ((src_prim->name == "f16" || src_prim->name == "f32" || src_prim->name == "f64") &&
        (dst_prim->name == "u8" || dst_prim->name == "u16" || dst_prim->name == "u32" ||
         dst_prim->name == "u64" || dst_prim->name == "u128" || dst_prim->name == "usize" ||
         dst_prim->name == "bool")) {
      float_check(true);
      return;
    }
  }

  void operator()(const IRDefer&) {
    SPEC_RULE("LowerIR-Defer");
  }

  void operator()(const IRMoveState&) {
    SPEC_RULE("LowerIR-MoveState");
    SPEC_RULE("Lower-MoveStateIR");
  }

  void operator()(const IRReadPlace&) {
    SPEC_RULE("LowerIR-ReadPlace");
    SPEC_RULE("Lower-ReadPlaceIR");
  }

  void operator()(const IRWritePlace&) {
    SPEC_RULE("LowerIR-WritePlace");
    SPEC_RULE("Lower-WritePlaceIR");
  }

  void operator()(const IRReadPath& read) {
    SPEC_RULE("LowerIR-ReadPath");
    std::vector<std::string> full = read.path;
    full.push_back(read.name);
    const std::string qualified = core::StringOfPath(full);
    const std::string builtin = BuiltinSym(qualified);
    std::string sym = builtin.empty() ? core::Mangle(qualified) : builtin;
    emitter.SetSymbolAlias(read.name, sym);

    if (!builtin.empty()) {
      SPEC_RULE("Lower-ReadPathIR-Runtime");
    }

    if (ctx) {
      if (builtin.empty() && ctx->sigma) {
        syntax::Path record_path = read.path;
        record_path.push_back(read.name);
        const auto it = ctx->sigma->types.find(analysis::PathKeyOf(record_path));
        if (it != ctx->sigma->types.end() &&
            std::holds_alternative<syntax::RecordDecl>(it->second)) {
          SPEC_RULE("Lower-ReadPathIR-Record");
          ctx->RegisterRecordCtor(sym, record_path);
          llvm::GlobalVariable* flag = GetOrCreatePoisonFlag(emitter, read.path);
          if (!flag) {
            SPEC_RULE("CheckPoison-Err");
            ctx->ReportCodegenFailure();
            return;
          }
          llvm::Type* bool_ty = emitter.GetLLVMType(analysis::MakeTypePrim("bool"));
          if (!bool_ty) {
            SPEC_RULE("CheckPoison-Err");
            ctx->ReportCodegenFailure();
            return;
          }
          llvm::Value* flag_val = builder->CreateLoad(bool_ty, flag);
          llvm::Value* ok = builder->CreateICmpEQ(
              flag_val, llvm::Constant::getNullValue(bool_ty));
          EmitPanicIfFalse(emitter, builder, ok, PanicCode(PanicReason::InitPanic));
          return;
        }
      }

      if (ctx->LookupStaticType(sym)) {
        if (ctx->LookupStaticModule(sym)) {
          SPEC_RULE("Lower-ReadPathIR-Static-User");
          llvm::GlobalVariable* flag = GetOrCreatePoisonFlag(emitter, read.path);
          if (!flag) {
            SPEC_RULE("CheckPoison-Err");
            ctx->ReportCodegenFailure();
            return;
          }
          llvm::Type* bool_ty = emitter.GetLLVMType(analysis::MakeTypePrim("bool"));
          if (!bool_ty) {
            SPEC_RULE("CheckPoison-Err");
            ctx->ReportCodegenFailure();
            return;
          }
          llvm::Value* flag_val = builder->CreateLoad(bool_ty, flag);
          llvm::Value* ok = builder->CreateICmpEQ(
              flag_val, llvm::Constant::getNullValue(bool_ty));
          EmitPanicIfFalse(emitter, builder, ok, PanicCode(PanicReason::InitPanic));
        } else {
          SPEC_RULE("Lower-ReadPathIR-Static-Gen");
        }
      } else if (const auto* mod = ctx->LookupProcModule(sym)) {
        SPEC_RULE("Lower-ReadPathIR-Proc-User");
        llvm::GlobalVariable* flag = GetOrCreatePoisonFlag(emitter, *mod);
        if (!flag) {
          SPEC_RULE("CheckPoison-Err");
          ctx->ReportCodegenFailure();
          return;
        }
        llvm::Type* bool_ty = emitter.GetLLVMType(analysis::MakeTypePrim("bool"));
        if (!bool_ty) {
          SPEC_RULE("CheckPoison-Err");
          ctx->ReportCodegenFailure();
          return;
        }
        llvm::Value* flag_val = builder->CreateLoad(bool_ty, flag);
        llvm::Value* ok = builder->CreateICmpEQ(
            flag_val, llvm::Constant::getNullValue(bool_ty));
        EmitPanicIfFalse(emitter, builder, ok, PanicCode(PanicReason::InitPanic));
      } else if (builtin.empty() && ctx->LookupProcSig(sym)) {
        SPEC_RULE("Lower-ReadPathIR-Proc-Gen");
      }
    }
  }

  void operator()(const IROpaque&) {
    SPEC_RULE("LowerIR-Opaque");
    SPEC_RULE("LowerIRInstr-Empty");
  }

  // C0X Extension: Structured Concurrency IR handlers (§18)
  
  // §18.1 Parallel block lowering
  // Emits: ctx = cursive0_parallel_begin(domain, cancel_token, name)
  //        <body>
  //        cursive0_parallel_join(ctx)
  void operator()(const IRParallel& par) {
    SPEC_RULE("LowerIR-Parallel");
    SPEC_RULE("Lower-ParallelIR");
    
    if (!ctx) {
      // No codegen context - just emit the body sequentially
      if (par.body) {
        emitter.EmitIR(par.body);
      }
      return;
    }
    
    // §18.1.1: Call cursive0_parallel_begin(domain, cancel_token, name)
    IRValue ctx_value = ctx->FreshTempValue("parallel_ctx");
    {
      IRCall begin_call;
      begin_call.callee.kind = IRValue::Kind::Symbol;
      begin_call.callee.name = "cursive0_parallel_begin";
      begin_call.args.push_back(par.domain);
      if (par.cancel_token.has_value()) {
        begin_call.args.push_back(*par.cancel_token);
      } else {
        // Pass null for cancel_token
        IRValue null_val;
        null_val.kind = IRValue::Kind::Immediate;
        null_val.bytes = {0, 0, 0, 0, 0, 0, 0, 0};  // null pointer
        begin_call.args.push_back(null_val);
      }
      // Pass name string or null
      IRValue name_val;
      name_val.kind = IRValue::Kind::Immediate;
      name_val.bytes = {0, 0, 0, 0, 0, 0, 0, 0};  // null for now
      begin_call.args.push_back(name_val);
      begin_call.result = ctx_value;
      (*this)(begin_call);
    }
    
    // Store parallel context for nested spawn/dispatch to access
    emitter.PushParallelContext(ctx_value);
    
    // Emit the body
    if (par.body) {
      emitter.EmitIR(par.body);
    }
    
    emitter.PopParallelContext();
    
    // §18.1.2: Call cursive0_parallel_join(ctx) - waits and propagates panics
    {
      IRCall join_call;
      join_call.callee.kind = IRValue::Kind::Symbol;
      join_call.callee.name = "cursive0_parallel_join";
      join_call.args.push_back(ctx_value);
      join_call.result = par.result;
      (*this)(join_call);
    }
  }

  // §18.4 Spawn expression lowering
  // For inline execution (§18.2.4 InlineDomain):
  //   1. Execute body immediately
  //   2. Store result via cursive0_spawn_inline_complete(result_ptr, result_size)
  //   3. Return handle in @Ready state
  void operator()(const IRSpawn& spawn) {
    SPEC_RULE("LowerIR-Spawn");
    SPEC_RULE("Lower-SpawnIR");

    if (spawn.captured_env) {
      emitter.EmitIR(spawn.captured_env);
    }

    IRCall create_call;
    create_call.callee.kind = IRValue::Kind::Symbol;
    create_call.callee.name = "cursive0_spawn_create";
    create_call.args = {spawn.env_ptr, spawn.env_size, spawn.body_fn, spawn.result_size};
    create_call.result = spawn.result;
    (*this)(create_call);
  }

  // §10.3 Wait expression lowering
  // Extracts result value T from Spawned<T>
  void operator()(const IRWait& wait) {
    SPEC_RULE("LowerIR-Wait");
    SPEC_RULE("Lower-WaitIR");

    IRValue ptr_val;
    if (ctx) {
      ptr_val = ctx->FreshTempValue("wait_ptr");
    } else {
      ptr_val.kind = IRValue::Kind::Opaque;
      ptr_val.name = "wait_ptr";
    }

    IRCall wait_call;
    wait_call.callee.kind = IRValue::Kind::Symbol;
    wait_call.callee.name = "cursive0_spawn_wait";
    wait_call.args.push_back(wait.handle);
    wait_call.result = ptr_val;
    (*this)(wait_call);

    analysis::TypeRef result_type = ctx ? ctx->LookupValueType(wait.result) : nullptr;
    if (!result_type || IsUnitType(result_type)) {
      llvm::Type* unit_ty = emitter.GetLLVMType(analysis::MakeTypePrim("()"));
      if (unit_ty) {
        StoreTemp(wait.result, llvm::Constant::getNullValue(unit_ty));
      }
      return;
    }

    llvm::Value* ptr = emitter.EvaluateIRValue(ptr_val);
    llvm::Type* elem_ty = emitter.GetLLVMType(result_type);
    if (!ptr || !elem_ty) {
      return;
    }
    llvm::Value* casted = builder->CreateBitCast(ptr, elem_ty->getPointerTo());
    llvm::Value* loaded = builder->CreateLoad(elem_ty, casted);
    StoreTemp(wait.result, loaded);
  }

  // §18.5 Dispatch expression lowering
  // For inline execution: sequential iteration with optional reduction
  void operator()(const IRDispatch& dispatch) {
    SPEC_RULE("LowerIR-Dispatch");
    SPEC_RULE("Lower-DispatchIR");

    if (dispatch.captured_env) {
      emitter.EmitIR(dispatch.captured_env);
    }

    IRValue reduce_op_val;
    reduce_op_val.kind = IRValue::Kind::Immediate;
    if (dispatch.reduce_op.has_value()) {
      const std::string& text = *dispatch.reduce_op;
      reduce_op_val.name = "\"" + text + "\"";
      reduce_op_val.bytes.assign(text.begin(), text.end());
    } else {
      reduce_op_val.name = "\"\"";
      reduce_op_val.bytes.clear();
    }

    const bool has_reduce =
        dispatch.reduce_op.has_value() || dispatch.reduce_fn.has_value();

    IRValue reduce_fn_val;
    if (dispatch.reduce_fn.has_value()) {
      reduce_fn_val = *dispatch.reduce_fn;
    } else {
      reduce_fn_val.kind = IRValue::Kind::Immediate;
      reduce_fn_val.bytes = {0};
    }

    IRValue ordered_val;
    ordered_val.kind = IRValue::Kind::Immediate;
    ordered_val.bytes = {static_cast<std::uint8_t>(dispatch.ordered ? 1 : 0)};

    IRValue chunk_val;
    if (dispatch.chunk_size.has_value()) {
      chunk_val = *dispatch.chunk_size;
    } else {
      chunk_val.kind = IRValue::Kind::Immediate;
      chunk_val.bytes = {0};
    }

    IRCall call;
    call.callee.kind = IRValue::Kind::Symbol;
    call.callee.name = "cursive0_dispatch_run";
    call.args = {dispatch.range,
                 dispatch.elem_size,
                 dispatch.result_size,
                 dispatch.body_fn,
                 dispatch.env_ptr,
                 reduce_op_val,
                 dispatch.result_ptr,
                 reduce_fn_val,
                 ordered_val,
                 chunk_val};
    if (ctx) {
      call.result = ctx->FreshTempValue("dispatch_call");
    } else {
      call.result.kind = IRValue::Kind::Opaque;
      call.result.name = "dispatch_call";
    }
    (*this)(call);

    if (has_reduce) {
      IRReadPtr read;
      read.ptr = dispatch.result_ptr;
      read.result = dispatch.result;
      (*this)(read);
    } else {
      llvm::Type* unit_ty = emitter.GetLLVMType(analysis::MakeTypePrim("()"));
      if (unit_ty) {
        StoreTemp(dispatch.result, llvm::Constant::getNullValue(unit_ty));
      }
    }
  }

  // C0X Extension: Asynchronous Operations IR nodes (§19)

  // §19.2.2 Yield expression lowering
  // Stores output value in async frame, sets state, returns @Suspended
  void operator()(const IRYield& yield) {
    SPEC_RULE("LowerIR-Yield");
    SPEC_RULE("Lower-YieldIR");

    auto* async_state = emitter.GetAsyncState();
    if (!async_state || !async_state->info || !async_state->frame_ptr) {
      if (ctx) {
        ctx->ReportCodegenFailure();
      }
      return;
    }

    analysis::TypeRef async_type = async_state->info->async_type;
    if (!async_type && ctx) {
      async_type = ctx->proc_ret_type;
    }
    if (!async_type) {
      if (ctx) {
        ctx->ReportCodegenFailure();
      }
      return;
    }

    const auto scope = BuildScope(ctx);
    const auto payload_layout = AsyncPayloadLayoutOf(scope, async_type);
    if (!payload_layout.has_value()) {
      if (ctx) {
        ctx->ReportCodegenFailure();
      }
      return;
    }

    llvm::Type* async_llvm_ty = emitter.GetLLVMType(async_type);
    if (!async_llvm_ty) {
      if (ctx) {
        ctx->ReportCodegenFailure();
      }
      return;
    }

    auto* alloca = CreateEntryAlloca(emitter, builder, async_llvm_ty, "async_yield");
    if (!alloca) {
      if (ctx) {
        ctx->ReportCodegenFailure();
      }
      return;
    }
    builder->CreateStore(llvm::Constant::getNullValue(async_llvm_ty), alloca);

    llvm::IntegerType* disc_ty =
        llvm::cast<llvm::IntegerType>(emitter.GetLLVMType(analysis::MakeTypePrim("u8")));
    llvm::Value* disc_val = llvm::ConstantInt::get(disc_ty, 0);
    StoreAtOffset(emitter, builder, alloca, 0, disc_val);

    analysis::TypeRef out_type = async_state->info->out_type;
    llvm::Value* out_val = emitter.EvaluateIRValue(yield.value);
    if (out_type) {
      const auto size_opt = SizeOf(scope, out_type);
      if (size_opt.has_value() && *size_opt > 0 && out_val) {
        llvm::Type* out_llvm_ty = emitter.GetLLVMType(out_type);
        out_val = CoerceToType(out_val, out_llvm_ty, IsUnsignedValue(yield.value));
        StoreAtOffset(emitter, builder, alloca,
                      payload_layout->payload_offset + payload_layout->suspended_output_offset,
                      out_val);
      }
    }

    StoreAtOffset(emitter, builder, alloca,
                  payload_layout->payload_offset + payload_layout->suspended_frame_offset,
                  async_state->frame_ptr);

    llvm::Value* state_val = llvm::ConstantInt::get(
        llvm::Type::getInt64Ty(emitter.GetContext()),
        static_cast<std::uint64_t>(yield.state_index));
    StoreAtOffset(emitter, builder, async_state->frame_ptr,
                  kAsyncFrameResumeStateOffset, state_val);

    llvm::Value* async_val = builder->CreateLoad(async_llvm_ty, alloca);

    llvm::Function* func = builder->GetInsertBlock()->getParent();
    if (func->arg_size() > 0 && func->hasParamAttribute(0, llvm::Attribute::StructRet)) {
      llvm::Argument* sret = func->getArg(0);
      builder->CreateStore(async_val, sret);
      builder->CreateRetVoid();
    } else {
      builder->CreateRet(async_val);
    }

    llvm::BasicBlock* resume_bb = nullptr;
    auto resume_it = async_state->resume_blocks.find(yield.state_index);
    if (resume_it != async_state->resume_blocks.end()) {
      resume_bb = resume_it->second;
    } else {
      resume_bb = llvm::BasicBlock::Create(
          emitter.GetContext(), "resume_state", func);
      async_state->resume_blocks[yield.state_index] = resume_bb;
      if (async_state->resume_switch) {
        llvm::IntegerType* state_ty =
            llvm::Type::getInt64Ty(emitter.GetContext());
        llvm::ConstantInt* case_val = llvm::ConstantInt::get(
            state_ty, static_cast<std::uint64_t>(yield.state_index));
        async_state->resume_switch->addCase(case_val, resume_bb);
      }
    }

    builder->SetInsertPoint(resume_bb);

    analysis::TypeRef in_type = async_state->info->in_type;
    llvm::Value* input_val = nullptr;
    if (in_type && !IsUnitType(in_type)) {
      llvm::Type* in_llvm_ty = emitter.GetLLVMType(in_type);
      llvm::Value* input_ptr = async_state->input_ptr
                                   ? async_state->input_ptr
                                   : llvm::Constant::getNullValue(emitter.GetOpaquePtr());
      llvm::Value* typed_ptr = builder->CreateBitCast(input_ptr,
                                                      in_llvm_ty->getPointerTo());
      input_val = builder->CreateLoad(in_llvm_ty, typed_ptr);
    } else {
      llvm::Type* unit_ty = emitter.GetLLVMType(analysis::MakeTypePrim("()"));
      input_val = llvm::Constant::getNullValue(unit_ty);
    }

    if (input_val) {
      StoreTemp(yield.result, input_val);
    }
  }

  // §19.2.3 Yield-from expression lowering
  // Delegates to source async, forwarding yields and collecting result
  void operator()(const IRYieldFrom& yf) {
    SPEC_RULE("LowerIR-YieldFrom");
    SPEC_RULE("Lower-YieldFromIR");

    auto* async_state = emitter.GetAsyncState();
    if (!async_state || !async_state->info || !async_state->frame_ptr) {
      if (ctx) {
        ctx->ReportCodegenFailure();
      }
      return;
    }

    analysis::TypeRef outer_async_type = async_state->info->async_type;
    if (!outer_async_type && ctx) {
      outer_async_type = ctx->proc_ret_type;
    }
    if (!outer_async_type) {
      if (ctx) {
        ctx->ReportCodegenFailure();
      }
      return;
    }

    analysis::TypeRef source_type = yf.source_type;
    if (!source_type && ctx) {
      source_type = ctx->LookupValueType(yf.source);
    }
    if (!source_type) {
      if (ctx) {
        ctx->ReportCodegenFailure();
      }
      return;
    }

    const auto scope = BuildScope(ctx);
    const auto outer_payload_layout = AsyncPayloadLayoutOf(scope, outer_async_type);
    const auto source_payload_layout = AsyncPayloadLayoutOf(scope, source_type);
    if (!outer_payload_layout.has_value() || !source_payload_layout.has_value()) {
      if (ctx) {
        ctx->ReportCodegenFailure();
      }
      return;
    }

    llvm::Type* outer_async_llvm_ty = emitter.GetLLVMType(outer_async_type);
    llvm::Type* source_llvm_ty = emitter.GetLLVMType(source_type);
    if (!outer_async_llvm_ty || !source_llvm_ty) {
      if (ctx) {
        ctx->ReportCodegenFailure();
      }
      return;
    }

    llvm::Value* source_ptr = nullptr;
    if (yf.source.kind == IRValue::Kind::Local) {
      source_ptr = emitter.GetLocal(yf.source.name);
    }
    if (!source_ptr) {
      llvm::Value* source_val = emitter.EvaluateIRValue(yf.source);
      llvm::AllocaInst* source_alloca =
          CreateEntryAlloca(emitter, builder, source_llvm_ty, "yield_from_source");
      if (!source_alloca) {
        if (ctx) {
          ctx->ReportCodegenFailure();
        }
        return;
      }
      if (!source_val) {
        source_val = llvm::Constant::getNullValue(source_llvm_ty);
      } else if (source_val->getType() != source_llvm_ty) {
        source_val = CoerceToType(source_val, source_llvm_ty, IsUnsignedValue(yf.source));
      }
      builder->CreateStore(source_val, source_alloca);
      source_ptr = source_alloca;
    }

    const auto source_sig = analysis::GetAsyncSig(source_type);
    if (!source_sig.has_value()) {
      if (ctx) {
        ctx->ReportCodegenFailure();
      }
      return;
    }

    llvm::Function* func = builder->GetInsertBlock()->getParent();
    llvm::LLVMContext& llvm_ctx = emitter.GetContext();
    llvm::BasicBlock* loop_bb =
        llvm::BasicBlock::Create(llvm_ctx, "yield_from_loop", func);
    llvm::BasicBlock* suspended_bb =
        llvm::BasicBlock::Create(llvm_ctx, "yield_from_suspended", func);
    llvm::BasicBlock* completed_bb =
        llvm::BasicBlock::Create(llvm_ctx, "yield_from_completed", func);
    llvm::BasicBlock* failed_bb =
        llvm::BasicBlock::Create(llvm_ctx, "yield_from_failed", func);
    llvm::BasicBlock* exit_bb =
        llvm::BasicBlock::Create(llvm_ctx, "yield_from_exit", func);

    builder->CreateBr(loop_bb);
    builder->SetInsertPoint(loop_bb);

    llvm::IntegerType* disc_ty =
        llvm::cast<llvm::IntegerType>(emitter.GetLLVMType(analysis::MakeTypePrim("u8")));
    llvm::Value* disc = LoadAtOffset(emitter, builder, source_ptr, 0, disc_ty);
    if (!disc) {
      if (ctx) {
        ctx->ReportCodegenFailure();
      }
      return;
    }

    llvm::SwitchInst* sw = builder->CreateSwitch(disc, failed_bb, 3);
    sw->addCase(llvm::ConstantInt::get(disc_ty, 0), suspended_bb);  // @Suspended
    sw->addCase(llvm::ConstantInt::get(disc_ty, 1), completed_bb);  // @Completed
    sw->addCase(llvm::ConstantInt::get(disc_ty, 2), failed_bb);     // @Failed

    // Handle @Suspended: yield output and suspend outer async
    builder->SetInsertPoint(suspended_bb);
    {
      analysis::TypeRef out_type = async_state->info->out_type;
      if (!out_type) {
        out_type = source_sig->out;
      }
      llvm::Value* out_val = nullptr;
      if (out_type && !IsUnitType(out_type)) {
        llvm::Type* out_llvm_ty = emitter.GetLLVMType(out_type);
        if (out_llvm_ty) {
          out_val = LoadAtOffset(emitter, builder, source_ptr,
                                 source_payload_layout->payload_offset +
                                     source_payload_layout->suspended_output_offset,
                                 out_llvm_ty);
        }
      }
      if (!out_val) {
        llvm::Type* unit_ty = emitter.GetLLVMType(analysis::MakeTypePrim("()"));
        if (unit_ty) {
          out_val = llvm::Constant::getNullValue(unit_ty);
        }
      }

      auto* alloca =
          CreateEntryAlloca(emitter, builder, outer_async_llvm_ty, "async_yield_from");
      if (!alloca) {
        if (ctx) {
          ctx->ReportCodegenFailure();
        }
        return;
      }
      builder->CreateStore(llvm::Constant::getNullValue(outer_async_llvm_ty), alloca);

      llvm::Value* disc_val = llvm::ConstantInt::get(disc_ty, 0);
      StoreAtOffset(emitter, builder, alloca, 0, disc_val);

      if (out_type) {
        const auto size_opt = SizeOf(scope, out_type);
        if (size_opt.has_value() && *size_opt > 0 && out_val) {
          llvm::Type* out_llvm_ty = emitter.GetLLVMType(out_type);
          out_val = CoerceToType(out_val, out_llvm_ty, IsUnsignedType(out_type));
          StoreAtOffset(emitter, builder, alloca,
                        outer_payload_layout->payload_offset +
                            outer_payload_layout->suspended_output_offset,
                        out_val);
        }
      }

      StoreAtOffset(emitter, builder, alloca,
                    outer_payload_layout->payload_offset +
                        outer_payload_layout->suspended_frame_offset,
                    async_state->frame_ptr);

      llvm::Value* state_val = llvm::ConstantInt::get(
          llvm::Type::getInt64Ty(emitter.GetContext()),
          static_cast<std::uint64_t>(yf.state_index));
      StoreAtOffset(emitter, builder, async_state->frame_ptr,
                    kAsyncFrameResumeStateOffset, state_val);

      llvm::Value* async_val = builder->CreateLoad(outer_async_llvm_ty, alloca);

      if (func->arg_size() > 0 &&
          func->hasParamAttribute(0, llvm::Attribute::StructRet)) {
        llvm::Argument* sret = func->getArg(0);
        builder->CreateStore(async_val, sret);
        builder->CreateRetVoid();
      } else {
        builder->CreateRet(async_val);
      }
    }

    // Resume block for yield-from
    llvm::BasicBlock* resume_bb = nullptr;
    auto resume_it = async_state->resume_blocks.find(yf.state_index);
    if (resume_it != async_state->resume_blocks.end()) {
      resume_bb = resume_it->second;
    } else {
      resume_bb = llvm::BasicBlock::Create(
          emitter.GetContext(), "resume_state", func);
      async_state->resume_blocks[yf.state_index] = resume_bb;
      if (async_state->resume_switch) {
        llvm::IntegerType* state_ty =
            llvm::Type::getInt64Ty(emitter.GetContext());
        llvm::ConstantInt* case_val = llvm::ConstantInt::get(
            state_ty, static_cast<std::uint64_t>(yf.state_index));
        async_state->resume_switch->addCase(case_val, resume_bb);
      }
    }

    builder->SetInsertPoint(resume_bb);

    llvm::Value* input_ptr = async_state->input_ptr
                                 ? async_state->input_ptr
                                 : llvm::Constant::getNullValue(emitter.GetOpaquePtr());
    llvm::Value* source_frame_ptr =
        LoadAtOffset(emitter, builder, source_ptr,
                     source_payload_layout->payload_offset +
                         source_payload_layout->suspended_frame_offset,
                     emitter.GetOpaquePtr());
    if (!source_frame_ptr) {
      if (ctx) {
        ctx->ReportCodegenFailure();
      }
      return;
    }
    llvm::Value* resumed = CallAsyncResume(emitter, builder, ctx,
                                           source_frame_ptr, input_ptr, source_type);
    if (!resumed) {
      if (ctx) {
        ctx->ReportCodegenFailure();
      }
      return;
    }
    if (resumed->getType() != source_llvm_ty) {
      resumed = CoerceToType(resumed, source_llvm_ty, IsUnsignedType(source_type));
    }
    builder->CreateStore(resumed, source_ptr);
    builder->CreateBr(loop_bb);

    // Handle @Completed: produce final result
    builder->SetInsertPoint(completed_bb);
    {
      analysis::TypeRef result_type = source_sig->result;
      llvm::Value* result_val = nullptr;
      if (result_type) {
        llvm::Type* result_llvm_ty = emitter.GetLLVMType(result_type);
        if (result_llvm_ty) {
          result_val = LoadAtOffset(emitter, builder, source_ptr,
                                    source_payload_layout->payload_offset,
                                    result_llvm_ty);
          if (!result_val) {
            result_val = llvm::Constant::getNullValue(result_llvm_ty);
          }
        }
      }
      if (result_val) {
        StoreTemp(yf.result, result_val);
      }
      builder->CreateBr(exit_bb);
    }

    // Handle @Failed: propagate error from outer async
    builder->SetInsertPoint(failed_bb);
    {
      analysis::TypeRef err_type = source_sig->err;
      if (IsNeverType(err_type)) {
        EmitPanicIfFalse(emitter, builder,
                         llvm::ConstantInt::getFalse(llvm_ctx),
                         PanicCode(PanicReason::AsyncFailed));
        builder->CreateUnreachable();
        return;
      }

      llvm::Value* err_val = nullptr;
      if (err_type) {
        llvm::Type* err_llvm_ty = emitter.GetLLVMType(err_type);
        if (err_llvm_ty) {
          err_val = LoadAtOffset(emitter, builder, source_ptr,
                                 source_payload_layout->payload_offset,
                                 err_llvm_ty);
          if (!err_val) {
            err_val = llvm::Constant::getNullValue(err_llvm_ty);
          }
        }
      }

      auto* alloca =
          CreateEntryAlloca(emitter, builder, outer_async_llvm_ty, "async_fail");
      if (!alloca) {
        if (ctx) {
          ctx->ReportCodegenFailure();
        }
        return;
      }
      builder->CreateStore(llvm::Constant::getNullValue(outer_async_llvm_ty), alloca);

      llvm::Value* disc_val = llvm::ConstantInt::get(disc_ty, 2);
      StoreAtOffset(emitter, builder, alloca, 0, disc_val);

      analysis::TypeRef outer_err_type = async_state->info->err_type;
      if (!outer_err_type) {
        if (auto outer_sig = analysis::GetAsyncSig(outer_async_type)) {
          outer_err_type = outer_sig->err;
        }
      }
      if (outer_err_type) {
        const auto size_opt = SizeOf(scope, outer_err_type);
        if (size_opt.has_value() && *size_opt > 0 && err_val) {
          llvm::Type* err_llvm_ty = emitter.GetLLVMType(outer_err_type);
          if (err_llvm_ty) {
            err_val = CoerceToType(err_val, err_llvm_ty, IsUnsignedType(outer_err_type));
            StoreAtOffset(emitter, builder, alloca,
                          outer_payload_layout->payload_offset, err_val);
          }
        }
      }

      llvm::Value* async_val = builder->CreateLoad(outer_async_llvm_ty, alloca);

      if (async_state->info && async_state->info->is_resume &&
          async_state->frame_ptr) {
        llvm::Function* free_fn = emitter.GetFunction(BuiltinSymAsyncFreeFrame());
        if (!free_fn) {
          free_fn = emitter.GetModule().getFunction(BuiltinSymAsyncFreeFrame());
        }
        if (free_fn) {
          builder->CreateCall(free_fn, {async_state->frame_ptr});
        } else if (ctx) {
          ctx->ReportCodegenFailure();
        }
      }

      if (func->arg_size() > 0 &&
          func->hasParamAttribute(0, llvm::Attribute::StructRet)) {
        llvm::Argument* sret = func->getArg(0);
        builder->CreateStore(async_val, sret);
        builder->CreateRetVoid();
      } else {
        builder->CreateRet(async_val);
      }
      return;
    }

    builder->SetInsertPoint(exit_bb);
  }

  // §19.3.3 Sync expression lowering
  // Runs async to completion synchronously
    void operator()(const IRSync& sync) {
    SPEC_RULE("LowerIR-Sync");
    SPEC_RULE("Lower-SyncIR");

    // Get the async value
    llvm::Value* async_val = emitter.EvaluateIRValue(sync.async_value);
    if (!async_val) {
      if (ctx) {
        ctx->ReportCodegenFailure();
      }
      return;
    }

    // Async modal type layout:
    // - Discriminant: u8 at offset 0 (0=Suspended, 1=Completed, 2=Failed)
    // - Payload: after discriminant, aligned
    //
    // ?19.3.3 Sync semantics:
    // Loop: check state
    //   @Suspended -> call resume(())
    //   @Completed -> return value
    //   @Failed -> propagate error

    llvm::Function* func = builder->GetInsertBlock()->getParent();
    llvm::LLVMContext& llvm_ctx = emitter.GetContext();

    const auto scope = BuildScope(ctx);
    analysis::TypeRef async_type = sync.async_type;
    if (!async_type) {
      if (ctx) {
        ctx->ReportCodegenFailure();
      }
      return;
    }

    const auto payload_layout = AsyncPayloadLayoutOf(scope, async_type);
    if (!payload_layout.has_value()) {
      if (ctx) {
        ctx->ReportCodegenFailure();
      }
      return;
    }

    analysis::TypeRef result_type = sync.result_type;
    analysis::TypeRef error_type = sync.error_type;
    if (!result_type || !error_type) {
      if (auto sig = analysis::GetAsyncSig(async_type)) {
        if (!result_type) result_type = sig->result;
        if (!error_type) error_type = sig->err;
      }
    }

    analysis::TypeRef out_type = ctx ? ctx->LookupValueType(sync.result) : nullptr;
    if (!out_type) {
      if (error_type && !IsNeverType(error_type)) {
        out_type = analysis::MakeTypeUnion({result_type, error_type});
      } else {
        out_type = result_type;
      }
    }

    llvm::Type* result_llvm_ty = result_type ? emitter.GetLLVMType(result_type) : nullptr;
    if (!result_llvm_ty) {
      result_llvm_ty = llvm::StructType::get(llvm_ctx, {});  // Unit type
    }

    llvm::Type* out_llvm_ty = out_type ? emitter.GetLLVMType(out_type) : result_llvm_ty;
    if (!out_llvm_ty) {
      if (ctx) {
        ctx->ReportCodegenFailure();
      }
      return;
    }

    llvm::Type* async_llvm_ty = emitter.GetLLVMType(async_type);
    if (!async_llvm_ty) {
      if (ctx) {
        ctx->ReportCodegenFailure();
      }
      return;
    }

    llvm::AllocaInst* result_slot =
        CreateEntryAlloca(emitter, builder, out_llvm_ty, "sync_result");
    if (!result_slot) {
      if (ctx) {
        ctx->ReportCodegenFailure();
      }
      return;
    }
    builder->CreateStore(llvm::Constant::getNullValue(out_llvm_ty), result_slot);

    // Create basic blocks for the sync loop
    llvm::BasicBlock* loop_bb = llvm::BasicBlock::Create(llvm_ctx, "sync_loop", func);
    llvm::BasicBlock* suspended_bb = llvm::BasicBlock::Create(llvm_ctx, "sync_suspended", func);
    llvm::BasicBlock* completed_bb = llvm::BasicBlock::Create(llvm_ctx, "sync_completed", func);
    llvm::BasicBlock* failed_bb = llvm::BasicBlock::Create(llvm_ctx, "sync_failed", func);
    llvm::BasicBlock* exit_bb = llvm::BasicBlock::Create(llvm_ctx, "sync_exit", func);

    // If async_val is not a pointer, allocate and store it to get an address.
    llvm::Value* async_struct_ptr = async_val;
    if (!async_val->getType()->isPointerTy()) {
      llvm::AllocaInst* async_alloca =
          CreateEntryAlloca(emitter, builder, async_llvm_ty, "async_slot");
      if (!async_alloca) {
        if (ctx) {
          ctx->ReportCodegenFailure();
        }
        return;
      }
      builder->CreateStore(async_val, async_alloca);
      async_struct_ptr = async_alloca;
    }

    // Jump to loop
    builder->CreateBr(loop_bb);
    builder->SetInsertPoint(loop_bb);

    // Load discriminant at offset 0
    llvm::IntegerType* disc_ty =
        llvm::cast<llvm::IntegerType>(emitter.GetLLVMType(analysis::MakeTypePrim("u8")));
    llvm::Value* disc = LoadAtOffset(emitter, builder, async_struct_ptr, 0, disc_ty);
    if (!disc) {
      if (ctx) {
        ctx->ReportCodegenFailure();
      }
      return;
    }

    // Create switch on discriminant
    llvm::SwitchInst* sw = builder->CreateSwitch(disc, failed_bb, 2);
    sw->addCase(llvm::ConstantInt::get(disc_ty, 0), suspended_bb);  // @Suspended
    sw->addCase(llvm::ConstantInt::get(disc_ty, 1), completed_bb);  // @Completed

    // Handle @Suspended: call resume and loop back
    builder->SetInsertPoint(suspended_bb);
    {
      llvm::Value* frame_ptr = LoadAtOffset(emitter, builder, async_struct_ptr,
                                            payload_layout->payload_offset +
                                                payload_layout->suspended_frame_offset,
                                            emitter.GetOpaquePtr());
      if (!frame_ptr) {
        if (ctx) {
          ctx->ReportCodegenFailure();
        }
        return;
      }
      llvm::Value* input_ptr = llvm::Constant::getNullValue(emitter.GetOpaquePtr());
      llvm::Value* resumed = CallAsyncResume(emitter, builder, ctx,
                                             frame_ptr, input_ptr, async_type);
      if (!resumed) {
        if (ctx) {
          ctx->ReportCodegenFailure();
        }
        return;
      }
      builder->CreateStore(resumed, async_struct_ptr);
      builder->CreateBr(loop_bb);
    }

    // Handle @Completed: extract value and exit
    builder->SetInsertPoint(completed_bb);
    {
      llvm::Value* completed_val = LoadAtOffset(emitter, builder, async_struct_ptr,
                                                 payload_layout->payload_offset,
                                                 result_llvm_ty);
      if (!completed_val) {
        completed_val = llvm::Constant::getNullValue(result_llvm_ty);
      }
      llvm::Value* out_val = BuildUnionValue(out_type, result_type, completed_val);
      if (!out_val) {
        out_val = llvm::Constant::getNullValue(out_llvm_ty);
      }
      builder->CreateStore(out_val, result_slot);
      builder->CreateBr(exit_bb);
    }

    // Handle @Failed: propagate error or panic
    builder->SetInsertPoint(failed_bb);
    {
      if (IsNeverType(error_type)) {
        EmitPanicIfFalse(emitter, builder,
                         llvm::ConstantInt::getFalse(llvm_ctx),
                         PanicCode(PanicReason::AsyncFailed));
        builder->CreateUnreachable();
      } else {
        llvm::Type* err_llvm_ty = error_type ? emitter.GetLLVMType(error_type) : nullptr;
        llvm::Value* err_val = nullptr;
        if (err_llvm_ty) {
          err_val = LoadAtOffset(emitter, builder, async_struct_ptr,
                                 payload_layout->payload_offset, err_llvm_ty);
          if (!err_val) {
            err_val = llvm::Constant::getNullValue(err_llvm_ty);
          }
        }
        llvm::Value* out_val = BuildUnionValue(out_type, error_type, err_val);
        if (!out_val) {
          out_val = llvm::Constant::getNullValue(out_llvm_ty);
        }
        builder->CreateStore(out_val, result_slot);
        builder->CreateBr(exit_bb);
      }
    }

    // Exit block: load result
    builder->SetInsertPoint(exit_bb);
    llvm::Value* out_val = builder->CreateLoad(out_llvm_ty, result_slot);
    StoreTemp(sync.result, out_val);
  }

  // §19.3.4 Race expression (first-completion mode) lowering
    void operator()(const IRRaceReturn& race) {
    SPEC_RULE("LowerIR-RaceReturn");
    SPEC_RULE("Lower-RaceReturnIR");

    const auto scope = BuildScope(ctx);
    analysis::TypeRef union_type = ctx ? ctx->LookupValueType(race.result) : nullptr;
    if (!union_type) {
      union_type = race.result_type;
    }
    llvm::Type* union_llvm_ty = union_type ? emitter.GetLLVMType(union_type) : nullptr;
    if (!union_llvm_ty) {
      if (ctx) {
        ctx->ReportCodegenFailure();
      }
      return;
    }

    llvm::AllocaInst* result_slot =
        CreateEntryAlloca(emitter, builder, union_llvm_ty, "race_result");
    if (!result_slot) {
      if (ctx) {
        ctx->ReportCodegenFailure();
      }
      return;
    }
    builder->CreateStore(llvm::Constant::getNullValue(union_llvm_ty), result_slot);

    struct ArmState {
      const IRRaceArm* arm = nullptr;
      analysis::TypeRef async_type;
      analysis::TypeRef result_type;
      analysis::TypeRef err_type;
      llvm::Type* async_llvm_ty = nullptr;
      llvm::AllocaInst* slot = nullptr;
      AsyncPayloadLayout payload;
    };

    std::vector<ArmState> arms;
    arms.reserve(race.arms.size());
    for (const auto& arm : race.arms) {
      emitter.EmitIR(arm.async_ir);
      analysis::TypeRef async_type = ctx ? ctx->LookupValueType(arm.async_value) : nullptr;
      if (!async_type) {
        if (ctx) {
          ctx->ReportCodegenFailure();
        }
        return;
      }
      const auto sig = analysis::GetAsyncSig(async_type);
      if (!sig.has_value()) {
        if (ctx) {
          ctx->ReportCodegenFailure();
        }
        return;
      }
      const auto payload_layout = AsyncPayloadLayoutOf(scope, async_type);
      if (!payload_layout.has_value()) {
        if (ctx) {
          ctx->ReportCodegenFailure();
        }
        return;
      }
      llvm::Type* async_llvm_ty = emitter.GetLLVMType(async_type);
      if (!async_llvm_ty) {
        if (ctx) {
          ctx->ReportCodegenFailure();
        }
        return;
      }
      llvm::AllocaInst* slot =
          CreateEntryAlloca(emitter, builder, async_llvm_ty, "race_async");
      if (!slot) {
        if (ctx) {
          ctx->ReportCodegenFailure();
        }
        return;
      }
      llvm::Value* async_val = emitter.EvaluateIRValue(arm.async_value);
      if (!async_val) {
        if (ctx) {
          ctx->ReportCodegenFailure();
        }
        return;
      }
      if (async_val->getType() != async_llvm_ty) {
        async_val = CoerceToType(async_val, async_llvm_ty, IsUnsignedValue(arm.async_value));
      }
      builder->CreateStore(async_val, slot);

      ArmState state;
      state.arm = &arm;
      state.async_type = async_type;
      state.result_type = sig->result;
      state.err_type = sig->err;
      state.async_llvm_ty = async_llvm_ty;
      state.slot = slot;
      state.payload = *payload_layout;
      arms.push_back(state);
    }

    llvm::Function* func = builder->GetInsertBlock()->getParent();
    llvm::LLVMContext& llvm_ctx = emitter.GetContext();
    llvm::BasicBlock* loop_bb = llvm::BasicBlock::Create(llvm_ctx, "race_loop", func);
    llvm::BasicBlock* resume_bb = llvm::BasicBlock::Create(llvm_ctx, "race_resume", func);
    llvm::BasicBlock* exit_bb = llvm::BasicBlock::Create(llvm_ctx, "race_exit", func);

    std::vector<llvm::BasicBlock*> check_bbs;
    std::vector<llvm::BasicBlock*> completed_bbs;
    std::vector<llvm::BasicBlock*> failed_bbs;
    check_bbs.reserve(arms.size());
    completed_bbs.reserve(arms.size());
    failed_bbs.reserve(arms.size());
    for (std::size_t i = 0; i < arms.size(); ++i) {
      check_bbs.push_back(llvm::BasicBlock::Create(llvm_ctx, "race_check", func));
      completed_bbs.push_back(llvm::BasicBlock::Create(llvm_ctx, "race_completed", func));
      failed_bbs.push_back(llvm::BasicBlock::Create(llvm_ctx, "race_failed", func));
    }

    builder->CreateBr(loop_bb);
    builder->SetInsertPoint(loop_bb);
    if (check_bbs.empty()) {
      builder->CreateBr(exit_bb);
    } else {
      builder->CreateBr(check_bbs[0]);
    }

    llvm::IntegerType* disc_ty =
        llvm::cast<llvm::IntegerType>(emitter.GetLLVMType(analysis::MakeTypePrim("u8")));

    for (std::size_t i = 0; i < arms.size(); ++i) {
      const auto& arm = arms[i];
      llvm::BasicBlock* next_bb = (i + 1 < arms.size()) ? check_bbs[i + 1] : resume_bb;

      builder->SetInsertPoint(check_bbs[i]);
      llvm::Value* disc = LoadAtOffset(emitter, builder, arm.slot, 0, disc_ty);
      if (!disc) {
        if (ctx) {
          ctx->ReportCodegenFailure();
        }
        return;
      }
      llvm::SwitchInst* sw = builder->CreateSwitch(disc, next_bb, 2);
      sw->addCase(llvm::ConstantInt::get(disc_ty, 1), completed_bbs[i]);
      sw->addCase(llvm::ConstantInt::get(disc_ty, 2), failed_bbs[i]);

      // Completed arm: bind pattern, run handler, return result
      builder->SetInsertPoint(completed_bbs[i]);
      {
        llvm::Value* result_val = nullptr;
        if (arm.result_type) {
          llvm::Type* result_llvm_ty = emitter.GetLLVMType(arm.result_type);
          if (result_llvm_ty) {
            result_val = LoadAtOffset(emitter, builder, arm.slot,
                                      arm.payload.payload_offset, result_llvm_ty);
            if (!result_val) {
              result_val = llvm::Constant::getNullValue(result_llvm_ty);
            }
          }
        }
        if (result_val) {
          StoreTemp(arm.arm->match_value, result_val);
        }
        emitter.EmitIR(arm.arm->handler_ir);
        llvm::Value* handler_val = emitter.EvaluateIRValue(arm.arm->handler_result);
        analysis::TypeRef handler_type = ctx ? ctx->LookupValueType(arm.arm->handler_result)
                                             : race.result_type;
        llvm::Value* out_val = BuildUnionValue(union_type, handler_type, handler_val);
        if (!out_val) {
          out_val = llvm::Constant::getNullValue(union_llvm_ty);
        }
        builder->CreateStore(out_val, result_slot);
        if (!builder->GetInsertBlock()->getTerminator()) {
          builder->CreateBr(exit_bb);
        }
      }

      // Failed arm: propagate error
      builder->SetInsertPoint(failed_bbs[i]);
      {
        if (IsNeverType(arm.err_type)) {
          EmitPanicIfFalse(emitter, builder,
                           llvm::ConstantInt::getFalse(llvm_ctx),
                           PanicCode(PanicReason::AsyncFailed));
          builder->CreateUnreachable();
        } else {
          llvm::Value* err_val = nullptr;
          if (arm.err_type) {
            llvm::Type* err_llvm_ty = emitter.GetLLVMType(arm.err_type);
            if (err_llvm_ty) {
              err_val = LoadAtOffset(emitter, builder, arm.slot,
                                     arm.payload.payload_offset, err_llvm_ty);
              if (!err_val) {
                err_val = llvm::Constant::getNullValue(err_llvm_ty);
              }
            }
          }
          llvm::Value* out_val = BuildUnionValue(union_type, arm.err_type, err_val);
          if (!out_val) {
            out_val = llvm::Constant::getNullValue(union_llvm_ty);
          }
          builder->CreateStore(out_val, result_slot);
          if (!builder->GetInsertBlock()->getTerminator()) {
            builder->CreateBr(exit_bb);
          }
        }
      }
    }

    // Resume all suspended arms and loop
    builder->SetInsertPoint(resume_bb);
    {
      llvm::Value* input_ptr = llvm::Constant::getNullValue(emitter.GetOpaquePtr());
      for (const auto& arm : arms) {
        llvm::Value* frame_ptr = LoadAtOffset(emitter, builder, arm.slot,
                                               arm.payload.payload_offset +
                                                   arm.payload.suspended_frame_offset,
                                               emitter.GetOpaquePtr());
        if (!frame_ptr) {
          if (ctx) {
            ctx->ReportCodegenFailure();
          }
          return;
        }
        llvm::Value* resumed = CallAsyncResume(emitter, builder, ctx,
                                               frame_ptr, input_ptr, arm.async_type);
        if (!resumed) {
          if (ctx) {
            ctx->ReportCodegenFailure();
          }
          return;
        }
        if (resumed->getType() != arm.async_llvm_ty) {
          resumed = CoerceToType(resumed, arm.async_llvm_ty, IsUnsignedType(arm.async_type));
        }
        builder->CreateStore(resumed, arm.slot);
      }
      builder->CreateBr(loop_bb);
    }

    builder->SetInsertPoint(exit_bb);
    llvm::Value* out_val = builder->CreateLoad(union_llvm_ty, result_slot);
    StoreTemp(race.result, out_val);
  }

  // §19.3.4 Race expression (streaming mode) lowering
  void operator()(const IRRaceYield& race) {
    SPEC_RULE("LowerIR-RaceYield");
    SPEC_RULE("Lower-RaceYieldIR");

    // TODO: Implement race (streaming) emission
    // 1. Initiate all async values
    // 2. When any yields: execute handler, emit value, resume arm
    // 3. When arm completes: remove from race
    // 4. When all complete: stream completes
    // 5. If any fails: propagate error, cancel others

    (void)race;  // Suppress unused warning until implemented
  }

  // §19.3.5 All expression lowering
    void operator()(const IRAll& all) {
    SPEC_RULE("LowerIR-All");
    SPEC_RULE("Lower-AllIR");

    const auto scope = BuildScope(ctx);
    analysis::TypeRef union_type = ctx ? ctx->LookupValueType(all.result) : nullptr;
    llvm::Type* union_llvm_ty = union_type ? emitter.GetLLVMType(union_type) : nullptr;
    if (!union_llvm_ty) {
      if (ctx) {
        ctx->ReportCodegenFailure();
      }
      return;
    }

    llvm::AllocaInst* result_slot =
        CreateEntryAlloca(emitter, builder, union_llvm_ty, "all_result");
    if (!result_slot) {
      if (ctx) {
        ctx->ReportCodegenFailure();
      }
      return;
    }
    builder->CreateStore(llvm::Constant::getNullValue(union_llvm_ty), result_slot);

    struct ArmState {
      analysis::TypeRef async_type;
      analysis::TypeRef result_type;
      analysis::TypeRef err_type;
      llvm::Type* async_llvm_ty = nullptr;
      llvm::Type* result_llvm_ty = nullptr;
      llvm::AllocaInst* slot = nullptr;
      llvm::AllocaInst* done_slot = nullptr;
      llvm::AllocaInst* value_slot = nullptr;
      AsyncPayloadLayout payload;
    };

    std::vector<ArmState> arms;
    arms.reserve(all.async_values.size());
    std::vector<analysis::TypeRef> tuple_elems;
    tuple_elems.reserve(all.async_values.size());

    for (std::size_t i = 0; i < all.async_values.size(); ++i) {
      if (i < all.async_irs.size()) {
        emitter.EmitIR(all.async_irs[i]);
      }
      analysis::TypeRef async_type = ctx ? ctx->LookupValueType(all.async_values[i]) : nullptr;
      if (!async_type) {
        if (ctx) {
          ctx->ReportCodegenFailure();
        }
        return;
      }
      const auto sig = analysis::GetAsyncSig(async_type);
      if (!sig.has_value()) {
        if (ctx) {
          ctx->ReportCodegenFailure();
        }
        return;
      }
      const auto payload_layout = AsyncPayloadLayoutOf(scope, async_type);
      if (!payload_layout.has_value()) {
        if (ctx) {
          ctx->ReportCodegenFailure();
        }
        return;
      }
      llvm::Type* async_llvm_ty = emitter.GetLLVMType(async_type);
      if (!async_llvm_ty) {
        if (ctx) {
          ctx->ReportCodegenFailure();
        }
        return;
      }
      llvm::AllocaInst* slot =
          CreateEntryAlloca(emitter, builder, async_llvm_ty, "all_async");
      if (!slot) {
        if (ctx) {
          ctx->ReportCodegenFailure();
        }
        return;
      }
      llvm::Value* async_val = emitter.EvaluateIRValue(all.async_values[i]);
      if (!async_val) {
        if (ctx) {
          ctx->ReportCodegenFailure();
        }
        return;
      }
      if (async_val->getType() != async_llvm_ty) {
        async_val = CoerceToType(async_val, async_llvm_ty, IsUnsignedValue(all.async_values[i]));
      }
      builder->CreateStore(async_val, slot);

      llvm::AllocaInst* done_slot =
          CreateEntryAlloca(emitter, builder, llvm::Type::getInt1Ty(emitter.GetContext()), "all_done");
      if (!done_slot) {
        if (ctx) {
          ctx->ReportCodegenFailure();
        }
        return;
      }
      builder->CreateStore(llvm::ConstantInt::getFalse(emitter.GetContext()), done_slot);

      llvm::Type* result_llvm_ty = sig->result ? emitter.GetLLVMType(sig->result) : nullptr;
      llvm::AllocaInst* value_slot = nullptr;
      if (result_llvm_ty) {
        value_slot = CreateEntryAlloca(emitter, builder, result_llvm_ty, "all_value");
        if (value_slot) {
          builder->CreateStore(llvm::Constant::getNullValue(result_llvm_ty), value_slot);
        }
      }

      ArmState state;
      state.async_type = async_type;
      state.result_type = sig->result;
      state.err_type = sig->err;
      state.async_llvm_ty = async_llvm_ty;
      state.result_llvm_ty = result_llvm_ty;
      state.slot = slot;
      state.done_slot = done_slot;
      state.value_slot = value_slot;
      state.payload = *payload_layout;
      arms.push_back(state);
      tuple_elems.push_back(sig->result);
    }

    analysis::TypeRef tuple_type = all.tuple_type;
    if (!tuple_type) {
      tuple_type = analysis::MakeTypeTuple(tuple_elems);
    }

    llvm::Function* func = builder->GetInsertBlock()->getParent();
    llvm::LLVMContext& llvm_ctx = emitter.GetContext();
    llvm::BasicBlock* loop_bb = llvm::BasicBlock::Create(llvm_ctx, "all_loop", func);
    llvm::BasicBlock* after_bb = llvm::BasicBlock::Create(llvm_ctx, "all_after", func);
    llvm::BasicBlock* success_bb = llvm::BasicBlock::Create(llvm_ctx, "all_success", func);
    llvm::BasicBlock* exit_bb = llvm::BasicBlock::Create(llvm_ctx, "all_exit", func);

    std::vector<llvm::BasicBlock*> check_bbs;
    std::vector<llvm::BasicBlock*> proc_bbs;
    std::vector<llvm::BasicBlock*> suspended_bbs;
    std::vector<llvm::BasicBlock*> completed_bbs;
    std::vector<llvm::BasicBlock*> failed_bbs;
    check_bbs.reserve(arms.size());
    proc_bbs.reserve(arms.size());
    suspended_bbs.reserve(arms.size());
    completed_bbs.reserve(arms.size());
    failed_bbs.reserve(arms.size());
    for (std::size_t i = 0; i < arms.size(); ++i) {
      check_bbs.push_back(llvm::BasicBlock::Create(llvm_ctx, "all_check", func));
      proc_bbs.push_back(llvm::BasicBlock::Create(llvm_ctx, "all_proc", func));
      suspended_bbs.push_back(llvm::BasicBlock::Create(llvm_ctx, "all_suspend", func));
      completed_bbs.push_back(llvm::BasicBlock::Create(llvm_ctx, "all_complete", func));
      failed_bbs.push_back(llvm::BasicBlock::Create(llvm_ctx, "all_failed", func));
    }

    builder->CreateBr(loop_bb);
    builder->SetInsertPoint(loop_bb);
    if (check_bbs.empty()) {
      builder->CreateBr(success_bb);
    } else {
      builder->CreateBr(check_bbs[0]);
    }

    llvm::IntegerType* disc_ty =
        llvm::cast<llvm::IntegerType>(emitter.GetLLVMType(analysis::MakeTypePrim("u8")));

    for (std::size_t i = 0; i < arms.size(); ++i) {
      const auto& arm = arms[i];
      llvm::BasicBlock* next_bb = (i + 1 < arms.size()) ? check_bbs[i + 1] : after_bb;

      builder->SetInsertPoint(check_bbs[i]);
      llvm::Value* done_val = builder->CreateLoad(llvm::Type::getInt1Ty(llvm_ctx), arm.done_slot);
      builder->CreateCondBr(done_val, next_bb, proc_bbs[i]);

      builder->SetInsertPoint(proc_bbs[i]);
      llvm::Value* disc = LoadAtOffset(emitter, builder, arm.slot, 0, disc_ty);
      if (!disc) {
        if (ctx) {
          ctx->ReportCodegenFailure();
        }
        return;
      }
      llvm::SwitchInst* sw = builder->CreateSwitch(disc, suspended_bbs[i], 2);
      sw->addCase(llvm::ConstantInt::get(disc_ty, 1), completed_bbs[i]);
      sw->addCase(llvm::ConstantInt::get(disc_ty, 2), failed_bbs[i]);

      // Suspended: resume and continue
      builder->SetInsertPoint(suspended_bbs[i]);
      {
        llvm::Value* frame_ptr = LoadAtOffset(emitter, builder, arm.slot,
                                               arm.payload.payload_offset +
                                                   arm.payload.suspended_frame_offset,
                                               emitter.GetOpaquePtr());
        if (!frame_ptr) {
          if (ctx) {
            ctx->ReportCodegenFailure();
          }
          return;
        }
        llvm::Value* input_ptr = llvm::Constant::getNullValue(emitter.GetOpaquePtr());
        llvm::Value* resumed = CallAsyncResume(emitter, builder, ctx,
                                               frame_ptr, input_ptr, arm.async_type);
        if (!resumed) {
          if (ctx) {
            ctx->ReportCodegenFailure();
          }
          return;
        }
        if (resumed->getType() != arm.async_llvm_ty) {
          resumed = CoerceToType(resumed, arm.async_llvm_ty, IsUnsignedType(arm.async_type));
        }
        builder->CreateStore(resumed, arm.slot);
        builder->CreateBr(next_bb);
      }

      // Completed: store result and mark done
      builder->SetInsertPoint(completed_bbs[i]);
      {
        if (arm.result_llvm_ty && arm.value_slot) {
          llvm::Value* result_val = LoadAtOffset(emitter, builder, arm.slot,
                                                 arm.payload.payload_offset, arm.result_llvm_ty);
          if (!result_val) {
            result_val = llvm::Constant::getNullValue(arm.result_llvm_ty);
          }
          builder->CreateStore(result_val, arm.value_slot);
        }
        builder->CreateStore(llvm::ConstantInt::getTrue(llvm_ctx), arm.done_slot);
        builder->CreateBr(next_bb);
      }

      // Failed: propagate error
      builder->SetInsertPoint(failed_bbs[i]);
      {
        if (IsNeverType(arm.err_type)) {
          EmitPanicIfFalse(emitter, builder,
                           llvm::ConstantInt::getFalse(llvm_ctx),
                           PanicCode(PanicReason::AsyncFailed));
          builder->CreateUnreachable();
        } else {
          llvm::Value* err_val = nullptr;
          if (arm.err_type) {
            llvm::Type* err_llvm_ty = emitter.GetLLVMType(arm.err_type);
            if (err_llvm_ty) {
              err_val = LoadAtOffset(emitter, builder, arm.slot,
                                     arm.payload.payload_offset, err_llvm_ty);
              if (!err_val) {
                err_val = llvm::Constant::getNullValue(err_llvm_ty);
              }
            }
          }
          llvm::Value* out_val = BuildUnionValue(union_type, arm.err_type, err_val);
          if (!out_val) {
            out_val = llvm::Constant::getNullValue(union_llvm_ty);
          }
          builder->CreateStore(out_val, result_slot);
          if (!builder->GetInsertBlock()->getTerminator()) {
            builder->CreateBr(exit_bb);
          }
        }
      }
    }

    // After processing all arms, check completion
    builder->SetInsertPoint(after_bb);
    llvm::Value* all_done = llvm::ConstantInt::getTrue(llvm_ctx);
    for (const auto& arm : arms) {
      llvm::Value* done_val = builder->CreateLoad(llvm::Type::getInt1Ty(llvm_ctx), arm.done_slot);
      all_done = builder->CreateAnd(all_done, done_val);
    }
    builder->CreateCondBr(all_done, success_bb, loop_bb);

    // Success: build tuple and wrap in union
    builder->SetInsertPoint(success_bb);
    {
      llvm::Type* tuple_llvm_ty = tuple_type ? emitter.GetLLVMType(tuple_type) : nullptr;
      llvm::Value* tuple_val = nullptr;
      if (tuple_llvm_ty) {
        auto* tuple_alloca = CreateEntryAlloca(emitter, builder, tuple_llvm_ty, "all_tuple");
        if (!tuple_alloca) {
          if (ctx) {
            ctx->ReportCodegenFailure();
          }
          return;
        }
        builder->CreateStore(llvm::Constant::getNullValue(tuple_llvm_ty), tuple_alloca);
        const auto layout = RecordLayoutOf(scope, tuple_elems);
        if (!layout.has_value() || layout->offsets.size() < tuple_elems.size()) {
          if (ctx) {
            ctx->ReportCodegenFailure();
          }
          return;
        }
        for (std::size_t i = 0; i < arms.size() && i < tuple_elems.size(); ++i) {
          if (!arms[i].result_llvm_ty || !arms[i].value_slot) {
            continue;
          }
          llvm::Value* elem_val = builder->CreateLoad(arms[i].result_llvm_ty, arms[i].value_slot);
          StoreAtOffset(emitter, builder, tuple_alloca, layout->offsets[i], elem_val);
        }
        tuple_val = builder->CreateLoad(tuple_llvm_ty, tuple_alloca);
      }
      llvm::Value* out_val = BuildUnionValue(union_type, tuple_type, tuple_val);
      if (!out_val) {
        out_val = llvm::Constant::getNullValue(union_llvm_ty);
      }
      builder->CreateStore(out_val, result_slot);
      builder->CreateBr(exit_bb);
    }

    builder->SetInsertPoint(exit_bb);
    llvm::Value* out_val = builder->CreateLoad(union_llvm_ty, result_slot);
    StoreTemp(all.result, out_val);
  }

    struct AsyncPayloadLayout {
    std::uint64_t payload_offset = 0;
    std::uint64_t payload_size = 0;
    std::uint64_t payload_align = 1;
    std::uint64_t suspended_output_offset = 0;
    std::uint64_t suspended_frame_offset = 0;
  };

  static bool IsUnitType(const analysis::TypeRef& type) {
    if (!type) {
      return false;
    }
    if (const auto* prim = std::get_if<analysis::TypePrim>(&type->node)) {
      return prim->name == "()";
    }
    if (const auto* tup = std::get_if<analysis::TypeTuple>(&type->node)) {
      return tup->elements.empty();
    }
    return false;
  }

  static bool IsNeverType(const analysis::TypeRef& type) {
    if (!type) {
      return false;
    }
    if (const auto* prim = std::get_if<analysis::TypePrim>(&type->node)) {
      return prim->name == "!";
    }
    return false;
  }

  static std::optional<AsyncPayloadLayout> AsyncPayloadLayoutOf(
      const analysis::ScopeContext& scope,
      const analysis::TypeRef& async_type) {
    const auto sig = analysis::GetAsyncSig(async_type);
    if (!sig.has_value()) {
      return std::nullopt;
    }

    std::uint64_t max_payload_size = 0;
    std::uint64_t max_payload_align = 1;

    auto add_payload_layout = [&](const std::optional<Layout>& layout_opt) {
      if (!layout_opt.has_value() || layout_opt->size == 0) {
        return;
      }
      max_payload_size = std::max(max_payload_size, layout_opt->size);
      max_payload_align = std::max(max_payload_align, layout_opt->align);
    };

    const auto frame_ptr = analysis::MakeTypePtr(analysis::MakeTypePrim("u8"),
                                                 analysis::PtrState::Valid);
    const auto suspended_layout = RecordLayoutOf(scope, {sig->out, frame_ptr});
    if (!suspended_layout.has_value() || suspended_layout->offsets.size() < 2) {
      return std::nullopt;
    }
    add_payload_layout(suspended_layout->layout);

    if (!IsUnitType(sig->result) && !IsNeverType(sig->result)) {
      add_payload_layout(LayoutOf(scope, sig->result));
    }
    if (!IsUnitType(sig->err) && !IsNeverType(sig->err)) {
      add_payload_layout(LayoutOf(scope, sig->err));
    }

    const auto disc_type = analysis::MakeTypePrim("u8");
    const auto disc_size = SizeOf(scope, disc_type).value_or(1);
    const std::uint64_t payload_offset = AlignUp(disc_size, max_payload_align);

    AsyncPayloadLayout out;
    out.payload_offset = payload_offset;
    out.payload_size = max_payload_size;
    out.payload_align = max_payload_align;
    out.suspended_output_offset = suspended_layout->offsets[0];
    out.suspended_frame_offset = suspended_layout->offsets[1];
    return out;
  }

  // §19.1.3 Async@Completed value construction
  // Used for return statements in async procedures
  void operator()(const IRAsyncComplete& async_complete) {
    SPEC_RULE("LowerIR-AsyncComplete");
    SPEC_RULE("Lower-AsyncCompleteIR");

    // Get the value to wrap
    llvm::Value* value = emitter.EvaluateIRValue(async_complete.value);

    const auto scope = BuildScope(ctx);
    const auto async_type = async_complete.async_type;
    if (!async_type) {
      if (ctx) {
        ctx->ReportCodegenFailure();
      }
      return;
    }

    const auto payload_layout = AsyncPayloadLayoutOf(scope, async_type);
    if (!payload_layout.has_value()) {
      if (ctx) {
        ctx->ReportCodegenFailure();
      }
      return;
    }

    llvm::Type* async_llvm_ty = emitter.GetLLVMType(async_type);
    if (!async_llvm_ty) {
      if (ctx) {
        ctx->ReportCodegenFailure();
      }
      return;
    }

    auto* alloca = CreateEntryAlloca(emitter, builder, async_llvm_ty, "async_complete");
    if (!alloca) {
      if (ctx) {
        ctx->ReportCodegenFailure();
      }
      return;
    }
    builder->CreateStore(llvm::Constant::getNullValue(async_llvm_ty), alloca);

    llvm::Type* disc_ty = emitter.GetLLVMType(analysis::MakeTypePrim("u8"));
    llvm::Value* disc_val = llvm::ConstantInt::get(disc_ty, 1);
    StoreAtOffset(emitter, builder, alloca, 0, disc_val);

    analysis::TypeRef result_type = async_complete.result_type;
    if (!result_type) {
      if (auto sig = analysis::GetAsyncSig(async_type)) {
        result_type = sig->result;
      }
    }

    if (result_type) {
      const auto size_opt = SizeOf(scope, result_type);
      if (size_opt.has_value() && *size_opt > 0) {
        llvm::Type* result_llvm_ty = emitter.GetLLVMType(result_type);
        if (value && result_llvm_ty) {
          value = CoerceToType(value, result_llvm_ty, IsUnsignedValue(async_complete.value));
          StoreAtOffset(emitter, builder, alloca,
                        payload_layout->payload_offset, value);
        }
      }
    }

    llvm::Value* async_val = builder->CreateLoad(async_llvm_ty, alloca);
    StoreTemp(async_complete.result, async_val);
  }
};
void LLVMEmitter::EmitIR(const IRPtr& ir) {
    if (!ir) return;
    if (current_ctx_ && current_ctx_->codegen_failed) {
      SPEC_RULE("LowerIRInstr-Err");
      return;
    }

    std::visit(IRVisitor{*this}, ir->node);

    if (current_ctx_ && current_ctx_->codegen_failed) {
      SPEC_RULE("LowerIRInstr-Err");
    }
}

} // namespace cursive0::codegen
