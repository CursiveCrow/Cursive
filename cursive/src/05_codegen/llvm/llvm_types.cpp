// =============================================================================
// MIGRATION MAPPING: llvm_types.cpp
// =============================================================================
//
// SPEC REFERENCE: CursiveSpecification.md
//   - Section 6.12.3 LLVMTy Judgment (lines 17416-17560)
//   - LLVMTy-Prim rule (lines 17456-17459)
//   - LLVMTy-Perm rule (lines 17461-17464)
//   - LLVMTy-Ptr rule (lines 17466-17469)
//   - LLVMTy-RawPtr rule (lines 17471-17474)
//   - LLVMTy-Func rule (lines 17476-17479)
//   - LLVMTy-Record rule (lines 17493-17496)
//   - LLVMTy-Tuple rule (lines 17498-17501)
//   - LLVMTy-Array rule (lines 17503-17506)
//   - LLVMTy-Slice, LLVMTy-Union, LLVMTy-Modal, etc.
//
// SOURCE FILE: cursive-bootstrap/src/04_codegen/llvm/llvm_types.cpp
//   - Lines 1-100: Type mapping helpers
//   - Lines 19-39: BuildScope, IsUnitType helpers
//   - Lines 41-50: AlignUp helper
//   - Lines 52-98: AppendPad, AppendStructElems
//   - Lines 100+: GetLLVMType implementation
//
// DEPENDENCIES:
//   - cursive/include/05_codegen/llvm/llvm_types.h
//   - cursive/include/05_codegen/llvm/llvm_emit.h (LLVMEmitter)
//   - cursive/include/05_codegen/layout/layout.h (SizeOf, AlignOf, RecordLayoutOf)
//   - llvm/IR/DerivedTypes.h
//   - llvm/IR/Type.h
// =============================================================================

#include "05_codegen/llvm/llvm_types.h"

#include "00_core/spec_trace.h"
#include "05_codegen/layout/layout.h"
#include "05_codegen/llvm/llvm_emit.h"

#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Type.h"

#include <algorithm>
#include <cstdint>

namespace cursive::codegen {

namespace {

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

void AppendPad(std::vector<llvm::Type*>& elems,
               llvm::LLVMContext& ctx,
               std::uint64_t pad) {
  if (pad == 0) {
    return;
  }
  elems.push_back(llvm::ArrayType::get(llvm::Type::getInt8Ty(ctx), pad));
}

}  // namespace

// =============================================================================
// §6.12.2 Opaque Pointer Model (LLVM 21)
// =============================================================================

llvm::PointerType* GetOpaquePointerType(llvm::LLVMContext& context,
                                        unsigned address_space) {
  SPEC_DEF("OpaquePointerModel", "§6.12.2");
  return llvm::PointerType::get(context, address_space);
}

// =============================================================================
// §6.12.8 LLVM Type Mapping
// =============================================================================

llvm::StructType* GetZSTType(llvm::LLVMContext& context) {
  return llvm::StructType::get(context, {});
}

// -----------------------------------------------------------------------------
// Primitive Type Mapping
// -----------------------------------------------------------------------------

llvm::Type* GetPrimType(llvm::LLVMContext& context, std::string_view name) {
  SPEC_RULE("LLVMTy-Prim");

  if (name == "bool") {
    return llvm::Type::getInt8Ty(context);
  }
  if (name == "char") {
    return llvm::Type::getInt32Ty(context);
  }
  if (name == "i8" || name == "u8") {
    return llvm::Type::getInt8Ty(context);
  }
  if (name == "i16" || name == "u16") {
    return llvm::Type::getInt16Ty(context);
  }
  if (name == "i32" || name == "u32") {
    return llvm::Type::getInt32Ty(context);
  }
  if (name == "i64" || name == "u64" || name == "usize" || name == "isize") {
    return llvm::Type::getInt64Ty(context);
  }
  if (name == "i128" || name == "u128") {
    return llvm::Type::getInt128Ty(context);
  }
  if (name == "f16") {
    return llvm::Type::getHalfTy(context);
  }
  if (name == "f32") {
    return llvm::Type::getFloatTy(context);
  }
  if (name == "f64") {
    return llvm::Type::getDoubleTy(context);
  }
  if (name == "unit" || name == "()" || name == "never" || name == "!") {
    return GetZSTType(context);  // Zero-sized type
  }

  // Default fallback
  return llvm::Type::getInt8Ty(context);
}

bool IsValidPrimName(std::string_view name) {
  return name == "bool" || name == "char" ||
         name == "i8" || name == "u8" ||
         name == "i16" || name == "u16" ||
         name == "i32" || name == "u32" ||
         name == "i64" || name == "u64" ||
         name == "i128" || name == "u128" ||
         name == "isize" || name == "usize" ||
         name == "f16" || name == "f32" || name == "f64" ||
         name == "()" || name == "unit" ||
         name == "!" || name == "never";
}

// -----------------------------------------------------------------------------
// Aggregate Type Construction
// -----------------------------------------------------------------------------

llvm::StructType* CreateStructType(llvm::LLVMContext& context,
                                   const std::vector<llvm::Type*>& elements,
                                   bool is_packed) {
  return llvm::StructType::get(context, elements, is_packed);
}

llvm::ArrayType* CreateArrayType(llvm::Type* element_type, std::uint64_t count) {
  return llvm::ArrayType::get(element_type, count);
}

llvm::Type* CreatePaddingType(llvm::LLVMContext& context, std::uint64_t bytes) {
  if (bytes == 0) {
    return nullptr;
  }
  return llvm::ArrayType::get(llvm::Type::getInt8Ty(context), bytes);
}

// -----------------------------------------------------------------------------
// Layout-Aware Struct Construction
// -----------------------------------------------------------------------------

std::vector<llvm::Type*> ComputeStructElements(
    LLVMEmitter& emitter,
    const std::vector<analysis::TypeRef>& fields,
    const std::vector<std::uint64_t>& offsets,
    std::uint64_t total_size,
    std::uint64_t required_align) {
  std::vector<llvm::Type*> elems;
  llvm::LLVMContext& ctx = emitter.GetContext();

  if (fields.size() != offsets.size()) {
    return elems;
  }

  std::uint64_t prev_end = 0;
  std::uint64_t natural_align = 1;
  for (std::size_t i = 0; i < fields.size(); ++i) {
    const std::uint64_t offset = offsets[i];
    if (offset > prev_end) {
      AppendPad(elems, ctx, offset - prev_end);
    }

    elems.push_back(emitter.GetLLVMType(fields[i]));

    // Get the scope context from emitter
    if (emitter.GetCurrentCtx() && emitter.GetCurrentCtx()->sigma) {
      analysis::ScopeContext scope;
      scope.sigma = *emitter.GetCurrentCtx()->sigma;
      scope.current_module = emitter.GetCurrentCtx()->module_path;
      const auto field_size = SizeOf(scope, fields[i]);
      const auto field_align = AlignOf(scope, fields[i]);
      if (field_size.has_value()) {
        prev_end = offset + *field_size;
      } else {
        prev_end = offset;
      }
      if (field_align.has_value()) {
        natural_align = std::max(natural_align, *field_align);
      }
    } else {
      prev_end = offset;
    }
  }

  if (total_size > prev_end) {
    AppendPad(elems, ctx, total_size - prev_end);
  }

  if (required_align > natural_align) {
    if (llvm::Type* marker = GetAlignmentMarkerType(ctx, required_align)) {
      elems.push_back(llvm::ArrayType::get(marker, 0));
    }
  }

  return elems;
}

std::vector<llvm::Type*> ComputeTaggedElements(
    LLVMEmitter& emitter,
    const analysis::TypeRef& disc_type,
    std::uint64_t payload_size,
    std::uint64_t payload_align,
    std::uint64_t total_size) {
  llvm::LLVMContext& ctx = emitter.GetContext();
  std::vector<llvm::Type*> elems;

  // Add discriminant type
  elems.push_back(emitter.GetLLVMType(disc_type));

  // Compute discriminant size
  std::uint64_t disc_size = 1;
  if (emitter.GetCurrentCtx() && emitter.GetCurrentCtx()->sigma) {
    analysis::ScopeContext scope;
    scope.sigma = *emitter.GetCurrentCtx()->sigma;
    scope.current_module = emitter.GetCurrentCtx()->module_path;
    const auto size_opt = SizeOf(scope, disc_type);
    if (size_opt.has_value()) {
      disc_size = *size_opt;
    }
  }

  // Add padding between discriminant and payload
  const std::uint64_t payload_off = AlignUp(disc_size, payload_align);
  const std::uint64_t pad_mid = payload_off - disc_size;
  AppendPad(elems, ctx, pad_mid);

  // Add payload blob
  llvm::Type* byte = llvm::Type::getInt8Ty(ctx);
  elems.push_back(llvm::ArrayType::get(byte, payload_size));

  // Add tail padding
  const std::uint64_t payload_end = payload_off + payload_size;
  if (total_size > payload_end) {
    AppendPad(elems, ctx, total_size - payload_end);
  }

  return elems;
}

// -----------------------------------------------------------------------------
// Composite Type Helpers
// -----------------------------------------------------------------------------

llvm::StructType* GetSliceType(llvm::LLVMContext& context) {
  SPEC_RULE("LLVMTy-Slice");
  llvm::Type* ptr_ty = GetOpaquePointerType(context);
  llvm::Type* len_ty = llvm::Type::getInt64Ty(context);
  return llvm::StructType::get(context, {ptr_ty, len_ty});
}

llvm::StructType* GetStringViewType(llvm::LLVMContext& context) {
  SPEC_RULE("LLVMTy-StringView");
  llvm::Type* ptr_ty = GetOpaquePointerType(context);
  llvm::Type* len_ty = llvm::Type::getInt64Ty(context);
  return llvm::StructType::get(context, {ptr_ty, len_ty});
}

llvm::StructType* GetBytesViewType(llvm::LLVMContext& context) {
  SPEC_RULE("LLVMTy-BytesView");
  llvm::Type* ptr_ty = GetOpaquePointerType(context);
  llvm::Type* len_ty = llvm::Type::getInt64Ty(context);
  return llvm::StructType::get(context, {ptr_ty, len_ty});
}

llvm::StructType* GetStringManagedType(llvm::LLVMContext& context) {
  SPEC_RULE("LLVMTy-StringManaged");
  llvm::Type* ptr_ty = GetOpaquePointerType(context);
  llvm::Type* len_ty = llvm::Type::getInt64Ty(context);
  return llvm::StructType::get(context, {ptr_ty, len_ty, len_ty});
}

llvm::StructType* GetBytesManagedType(llvm::LLVMContext& context) {
  SPEC_RULE("LLVMTy-BytesManaged");
  llvm::Type* ptr_ty = GetOpaquePointerType(context);
  llvm::Type* len_ty = llvm::Type::getInt64Ty(context);
  return llvm::StructType::get(context, {ptr_ty, len_ty, len_ty});
}

llvm::StructType* GetRangeType(llvm::LLVMContext& context) {
  SPEC_RULE("LLVMTy-Range");
  llvm::Type* kind_ty = llvm::Type::getInt8Ty(context);
  llvm::Type* bound_ty = llvm::Type::getInt64Ty(context);
  return llvm::StructType::get(context, {kind_ty, bound_ty, bound_ty});
}

llvm::StructType* GetDynamicType(llvm::LLVMContext& context) {
  SPEC_RULE("LLVMTy-Dynamic");
  llvm::Type* ptr_ty = GetOpaquePointerType(context);
  return llvm::StructType::get(context, {ptr_ty, ptr_ty});
}

// -----------------------------------------------------------------------------
// Type Size and Alignment
// -----------------------------------------------------------------------------

std::uint64_t GetTypeSize(llvm::Type* type) {
  // Note: This is a simplified implementation
  // For accurate sizes, use DataLayout::getTypeAllocSize
  if (!type) {
    return 0;
  }
  if (type->isIntegerTy()) {
    return (type->getIntegerBitWidth() + 7) / 8;
  }
  if (type->isHalfTy()) {
    return 2;
  }
  if (type->isFloatTy()) {
    return 4;
  }
  if (type->isDoubleTy()) {
    return 8;
  }
  if (type->isPointerTy()) {
    return 8;  // x86_64
  }
  // For complex types, would need DataLayout
  return 0;
}

std::uint64_t GetTypeAlignment(llvm::Type* type) {
  // Note: This is a simplified implementation
  // For accurate alignment, use DataLayout::getABITypeAlign
  if (!type) {
    return 1;
  }
  if (type->isIntegerTy()) {
    std::uint64_t size = (type->getIntegerBitWidth() + 7) / 8;
    return std::min(size, static_cast<std::uint64_t>(8));
  }
  if (type->isHalfTy()) {
    return 2;
  }
  if (type->isFloatTy()) {
    return 4;
  }
  if (type->isDoubleTy()) {
    return 8;
  }
  if (type->isPointerTy()) {
    return 8;  // x86_64
  }
  return 1;
}

llvm::Type* GetIntTypeForSize(llvm::LLVMContext& context, std::uint64_t size) {
  switch (size) {
    case 1:
      return llvm::Type::getInt8Ty(context);
    case 2:
      return llvm::Type::getInt16Ty(context);
    case 4:
      return llvm::Type::getInt32Ty(context);
    case 8:
      return llvm::Type::getInt64Ty(context);
    default:
      return nullptr;
  }
}

llvm::Type* GetAlignmentMarkerType(llvm::LLVMContext& context, std::uint64_t align) {
  switch (align) {
    case 1:
      return llvm::Type::getInt8Ty(context);
    case 2:
      return llvm::Type::getInt16Ty(context);
    case 4:
      return llvm::Type::getInt32Ty(context);
    case 8:
      return llvm::Type::getInt64Ty(context);
    case 16:
      return llvm::Type::getInt128Ty(context);
    default:
      return nullptr;
  }
}

// -----------------------------------------------------------------------------
// Tagged Type Helpers
// -----------------------------------------------------------------------------

llvm::StructType* CreateTaggedBlobType(llvm::LLVMContext& context,
                                       std::uint64_t size,
                                       std::uint64_t align) {
  if (size == 0) {
    return llvm::StructType::get(context, {});
  }

  llvm::Type* byte = llvm::Type::getInt8Ty(context);
  llvm::Type* bytes = llvm::ArrayType::get(byte, size);
  std::vector<llvm::Type*> fields;
  fields.push_back(bytes);

  if (align > 1) {
    if (llvm::Type* marker = GetAlignmentMarkerType(context, align)) {
      fields.push_back(llvm::ArrayType::get(marker, 0));
    }
  }

  return llvm::StructType::get(context, fields, /*isPacked=*/false);
}

llvm::Type* CreateTaggedABIType(llvm::LLVMContext& context,
                                std::uint64_t size,
                                std::uint64_t align) {
  // Use integer type if size matches standard int sizes for better ABI
  if (llvm::Type* int_ty = GetIntTypeForSize(context, size)) {
    return int_ty;
  }
  return CreateTaggedBlobType(context, size, align);
}

llvm::StructType* CreateTaggedStructType(LLVMEmitter& emitter,
                                         const analysis::TypeRef& disc_type,
                                         std::uint64_t payload_size,
                                         std::uint64_t payload_align,
                                         std::uint64_t total_size) {
  std::vector<llvm::Type*> elems = ComputeTaggedElements(
      emitter, disc_type, payload_size, payload_align, total_size);
  return llvm::StructType::get(emitter.GetContext(), elems, /*isPacked=*/false);
}

// -----------------------------------------------------------------------------
// Async Type Layout (§5.4.5)
// -----------------------------------------------------------------------------

llvm::Type* BuildAsyncLLVMType(LLVMEmitter& emitter,
                               const std::vector<analysis::TypeRef>& generic_args) {
  SPEC_RULE("LLVMTy-Async");

  llvm::LLVMContext& ctx = emitter.GetContext();

  // If no context, return minimal struct
  if (!emitter.GetCurrentCtx() || !emitter.GetCurrentCtx()->sigma) {
    return llvm::StructType::get(ctx, {});
  }

  analysis::ScopeContext scope;
  scope.sigma = *emitter.GetCurrentCtx()->sigma;
  scope.current_module = emitter.GetCurrentCtx()->module_path;

  analysis::AsyncSig async_sig{};
  async_sig.out =
      !generic_args.empty() ? generic_args[0] : analysis::MakeTypePrim("()");
  async_sig.in =
      generic_args.size() > 1 ? generic_args[1] : analysis::MakeTypePrim("()");
  async_sig.result =
      generic_args.size() > 2 ? generic_args[2] : analysis::MakeTypePrim("()");
  async_sig.err =
      generic_args.size() > 3 ? generic_args[3] : analysis::MakeTypePrim("!");
  const auto lowered_async = LowerAsyncType(async_sig);
  const bool has_failed_state =
      lowered_async.has_value() &&
      std::find(lowered_async->states.begin(),
                lowered_async->states.end(),
                "Failed") != lowered_async->states.end();

  // Compute async layout similar to modal layout
  std::uint64_t max_payload_size = 0;
  std::uint64_t max_payload_align = 1;

  auto add_payload_layout = [&](const std::optional<Layout>& layout_opt) {
    if (!layout_opt.has_value() || layout_opt->size == 0) {
      return;
    }
    max_payload_size = std::max(max_payload_size, layout_opt->size);
    max_payload_align = std::max(max_payload_align, layout_opt->align);
  };
  auto add_payload_type = [&](const analysis::TypeRef& type) {
    if (!type) {
      return;
    }
    add_payload_layout(LayoutOf(scope, type));
  };
  auto is_unit_type = [](const analysis::TypeRef& type) {
    if (!type) {
      return false;
    }
    if (const auto* prim = std::get_if<analysis::TypePrim>(&type->node)) {
      return prim->name == "()";
    }
    if (const auto* tuple = std::get_if<analysis::TypeTuple>(&type->node)) {
      return tuple->elements.empty();
    }
    return false;
  };
  auto is_never_type = [](const analysis::TypeRef& type) {
    if (!type) {
      return false;
    }
    if (const auto* prim = std::get_if<analysis::TypePrim>(&type->node)) {
      return prim->name == "!";
    }
    return false;
  };

  // Suspended payload: { output: Out, frame: Ptr<u8> }.
  const analysis::TypeRef out_type = async_sig.out;
  const analysis::TypeRef frame_ptr = analysis::MakeTypePtr(
      analysis::MakeTypePrim("u8"),
      analysis::PtrState::Valid);
  const auto suspended_layout = RecordLayoutOf(scope, {out_type, frame_ptr});
  if (!suspended_layout.has_value()) {
    return llvm::StructType::get(ctx, {});
  }
  add_payload_layout(suspended_layout->layout);

  // Completed payload: Result (if inhabited and non-empty).
  if (async_sig.result &&
      !is_never_type(async_sig.result) &&
      !is_unit_type(async_sig.result)) {
    add_payload_type(async_sig.result);
  }

  // Failed payload: E (if inhabited and non-empty).
  if (has_failed_state &&
      async_sig.err &&
      !is_never_type(async_sig.err) &&
      !is_unit_type(async_sig.err)) {
    add_payload_type(async_sig.err);
  }

  // Runtime async frame extraction assumes suspended payload stores a hidden
  // frame pointer at byte offset 8. Keep LLVM Async payload large/aligned
  // enough for that contract, including Out = ().
  constexpr std::uint64_t kAsyncFramePtrPayloadOffset = 8;
  const std::uint64_t min_suspended_payload =
      kAsyncFramePtrPayloadOffset + kPtrSize;
  max_payload_size = std::max(max_payload_size, min_suspended_payload);
  max_payload_align = std::max(max_payload_align, kPtrAlign);

  // Build tagged struct
  const std::uint64_t disc_size = 1;
  const std::uint64_t disc_align = 1;
  const std::uint64_t align = std::max(disc_align, max_payload_align);
  const std::uint64_t size = AlignUp(disc_size + max_payload_size, align);

  auto disc_type = analysis::MakeTypePrim("u8");
  return CreateTaggedStructType(emitter, disc_type, max_payload_size, max_payload_align, size);
}

}  // namespace cursive::codegen
