// =================================================================
// File: 04_codegen/llvm/llvm_ir_utils.cpp
// Construct: LLVM IR Emission Utilities
// Spec Section: 6.12
// Spec Rules: ConstBytes, TypeUtils
// =================================================================
#include "cursive0/04_codegen/llvm/llvm_ir_utils.h"

#include "cursive0/00_core/assert_spec.h"
#include "cursive0/00_core/symbols.h"
#include "cursive0/04_codegen/llvm/llvm_emit.h"
#include "cursive0/04_codegen/layout/layout.h"
#include "cursive0/04_codegen/mangle.h"
#include "cursive0/03_analysis/generics/monomorphize.h"

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/APInt.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"

#include <algorithm>
#include <cstring>
#include <limits>

namespace cursive0::codegen {

analysis::ScopeContext BuildScope(const LowerCtx* ctx) {
  analysis::ScopeContext scope;
  if (ctx && ctx->sigma) {
    scope.sigma = *ctx->sigma;
    scope.current_module = ctx->module_path;
  }
  return scope;
}

std::uint64_t AlignUp(std::uint64_t value, std::uint64_t align) {
  if (align == 0) {
    return value;
  }
  const std::uint64_t rem = value % align;
  if (rem == 0) {
    return value;
  }
  return value + (align - rem);
}

llvm::APInt APIntFromBytes(const std::vector<std::uint8_t>& bytes) {
  const unsigned bitwidth = static_cast<unsigned>(bytes.size() * 8);
  if (bitwidth == 0) {
    return llvm::APInt(1, 0);
  }
  std::vector<std::uint64_t> words;
  words.reserve((bytes.size() + 7) / 8);
  for (std::size_t i = 0; i < bytes.size(); i += 8) {
    std::uint64_t word = 0;
    const std::size_t limit = std::min<std::size_t>(8, bytes.size() - i);
    for (std::size_t j = 0; j < limit; ++j) {
      word |= static_cast<std::uint64_t>(bytes[i + j]) << (8 * j);
    }
    words.push_back(word);
  }
  return llvm::APInt(bitwidth, words);
}

bool AllZero(const std::vector<std::uint8_t>& bytes) {
  for (auto b : bytes) {
    if (b != 0) {
      return false;
    }
  }
  return true;
}

llvm::Constant* ConstBytes(llvm::Type* ty,
                           const std::vector<std::uint8_t>& bytes,
                           LLVMEmitter& emitter,
                           LowerCtx* lower_ctx) {
  if (!ty) {
    return nullptr;
  }

  if (bytes.empty()) {
    return llvm::Constant::getNullValue(ty);
  }

  if (auto* arr = llvm::dyn_cast<llvm::ArrayType>(ty)) {
    if (arr->getElementType()->isIntegerTy(8) &&
        arr->getNumElements() == bytes.size()) {
      return llvm::ConstantDataArray::get(
          emitter.GetContext(), llvm::ArrayRef<uint8_t>(bytes.data(), bytes.size()));
    }
  }

  if (auto* int_ty = llvm::dyn_cast<llvm::IntegerType>(ty)) {
    const unsigned bits = int_ty->getBitWidth();
    if (bits == bytes.size() * 8) {
      return llvm::ConstantInt::get(emitter.GetContext(), APIntFromBytes(bytes));
    }
  }

  if (ty->isHalfTy() || ty->isFloatTy() || ty->isDoubleTy()) {
    const unsigned bits = ty->getPrimitiveSizeInBits();
    if (bits == bytes.size() * 8) {
      auto int_ty = llvm::IntegerType::get(emitter.GetContext(), bits);
      auto int_c = llvm::ConstantInt::get(emitter.GetContext(), APIntFromBytes(bytes));
      return llvm::ConstantExpr::getBitCast(int_c, ty);
    }
  }

  if (ty->isPointerTy() && AllZero(bytes)) {
    return llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(ty));
  }

  if (lower_ctx) {
    lower_ctx->ReportCodegenFailure();
  }
  return llvm::Constant::getNullValue(ty);
}

analysis::TypeRef StripPerm(const analysis::TypeRef& type) {
  if (!type) {
    return type;
  }
  if (const auto* perm = std::get_if<analysis::TypePerm>(&type->node)) {
    return StripPerm(perm->base);
  }
  return type;
}

bool IsUnsignedPrim(std::string_view name) {
  return name == "u8" || name == "u16" || name == "u32" ||
         name == "u64" || name == "u128" || name == "usize" ||
         name == "bool";
}

bool IsUnsignedType(const analysis::TypeRef& type) {
  auto stripped = StripPerm(type);
  if (!stripped) {
    return false;
  }
  if (const auto* prim = std::get_if<analysis::TypePrim>(&stripped->node)) {
    return IsUnsignedPrim(prim->name);
  }
  return false;
}

bool IsUnitType(const analysis::TypeRef& type) {
  auto stripped = StripPerm(type);
  if (!stripped) {
    return false;
  }
  if (const auto* prim = std::get_if<analysis::TypePrim>(&stripped->node)) {
    return prim->name == "()";
  }
  return false;
}

bool IsNeverType(const analysis::TypeRef& type) {
  auto stripped = StripPerm(type);
  if (!stripped) {
    return false;
  }
  if (const auto* prim = std::get_if<analysis::TypePrim>(&stripped->node)) {
    return prim->name == "!";
  }
  return false;
}

analysis::TypeRef PtrElementType(const analysis::TypeRef& type) {
  auto stripped = StripPerm(type);
  if (!stripped) {
    return nullptr;
  }
  if (const auto* ptr = std::get_if<analysis::TypePtr>(&stripped->node)) {
    return ptr->element;
  }
  if (const auto* raw = std::get_if<analysis::TypeRawPtr>(&stripped->node)) {
    return raw->element;
  }
  return nullptr;
}

llvm::Value* AsUSize(llvm::IRBuilder<>* builder,
                     llvm::Value* value,
                     llvm::LLVMContext& ctx) {
  if (!value || !value->getType()->isIntegerTy()) {
    return nullptr;
  }
  auto* usize_ty = llvm::Type::getInt64Ty(ctx);
  const unsigned bits = value->getType()->getIntegerBitWidth();
  if (bits == 64) {
    return value;
  }
  if (bits < 64) {
    return builder->CreateZExt(value, usize_ty);
  }
  return builder->CreateTrunc(value, usize_ty);
}

llvm::AllocaInst* CreateEntryAlloca(LLVMEmitter& emitter,
                                    llvm::IRBuilder<>* builder,
                                    llvm::Type* ty,
                                    const std::string& name) {
  if (!ty) {
    return nullptr;
  }
  llvm::Function* func = builder->GetInsertBlock()->getParent();
  llvm::IRBuilder<> entry_builder(&func->getEntryBlock(), func->getEntryBlock().begin());
  return entry_builder.CreateAlloca(ty, nullptr, name);
}

llvm::Value* ByteGEP(LLVMEmitter& emitter,
                     llvm::IRBuilder<>* builder,
                     llvm::Value* base_ptr,
                     std::uint64_t offset) {
  llvm::LLVMContext& ctx = emitter.GetContext();
  llvm::Value* idx = llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx), offset);
  return builder->CreateGEP(llvm::Type::getInt8Ty(ctx), base_ptr, idx);
}

llvm::Value* ByteGEP(LLVMEmitter& emitter,
                     llvm::IRBuilder<>* builder,
                     llvm::Value* base_ptr,
                     llvm::Value* offset) {
  llvm::LLVMContext& ctx = emitter.GetContext();
  return builder->CreateGEP(llvm::Type::getInt8Ty(ctx), base_ptr, offset);
}

llvm::Value* LoadAtOffset(LLVMEmitter& emitter,
                          llvm::IRBuilder<>* builder,
                          llvm::Value* base_ptr,
                          std::uint64_t offset,
                          llvm::Type* field_ty) {
  if (!base_ptr || !field_ty) {
    return nullptr;
  }
  llvm::Value* ptr = offset == 0 ? base_ptr : ByteGEP(emitter, builder, base_ptr, offset);
  return builder->CreateLoad(field_ty, ptr);
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

llvm::Value* SliceLenFromValue(LLVMEmitter& emitter,
                               llvm::IRBuilder<>* builder,
                               const IRValue& value,
                               const analysis::TypeRef& type) {
  auto stripped = StripPerm(type);
  if (!stripped) {
    return nullptr;
  }
  llvm::LLVMContext& ctx = emitter.GetContext();
  auto* usize_ty = llvm::Type::getInt64Ty(ctx);

  if (const auto* arr = std::get_if<analysis::TypeArray>(&stripped->node)) {
    return llvm::ConstantInt::get(usize_ty, arr->length);
  }

  if (std::holds_alternative<analysis::TypeSlice>(stripped->node) ||
      std::holds_alternative<analysis::TypeString>(stripped->node) ||
      std::holds_alternative<analysis::TypeBytes>(stripped->node)) {
    llvm::Value* val = emitter.EvaluateIRValue(value);
    if (!val) {
      return nullptr;
    }
    if (val->getType()->isStructTy()) {
      llvm::Value* len = builder->CreateExtractValue(val, {1});
      return AsUSize(builder, len, ctx);
    }
  }

  return nullptr;
}

llvm::Value* ExtractSlicePtr(LLVMEmitter& emitter,
                             llvm::IRBuilder<>* builder,
                             llvm::Value* slice_val) {
  if (!slice_val || !slice_val->getType()->isStructTy()) {
    return nullptr;
  }
  return builder->CreateExtractValue(slice_val, {0});
}

llvm::Value* ExtractSliceLen(LLVMEmitter& emitter,
                             llvm::IRBuilder<>* builder,
                             llvm::Value* slice_val) {
  if (!slice_val || !slice_val->getType()->isStructTy()) {
    return nullptr;
  }
  llvm::Value* len = builder->CreateExtractValue(slice_val, {1});
  return AsUSize(builder, len, emitter.GetContext());
}

}  // namespace cursive0::codegen
