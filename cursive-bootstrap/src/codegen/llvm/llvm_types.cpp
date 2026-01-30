#include "cursive0/codegen/llvm/llvm_emit.h"
#include "cursive0/codegen/layout/layout.h"
#include "cursive0/core/assert_spec.h"
#include "cursive0/analysis/generics/monomorphize.h"
#include "cursive0/analysis/modal/modal_widen.h"
#include "cursive0/analysis/resolve/scopes.h"
#include "cursive0/analysis/types/types.h"
#include "cursive0/analysis/caps/cap_concurrency.h"

// LLVM Includes
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Type.h"

#include <algorithm>
#include <cstdint>

namespace cursive0::codegen {

namespace {

std::optional<analysis::ScopeContext> BuildScope(const LowerCtx* ctx) {
  if (!ctx || !ctx->sigma) {
    return std::nullopt;
  }
  analysis::ScopeContext scope;
  scope.sigma = *ctx->sigma;
  scope.current_module = ctx->module_path;
  return scope;
}

bool IsUnitType(const analysis::TypeRef& type) {
  if (!type) {
    return false;
  }
  if (const auto* prim = std::get_if<analysis::TypePrim>(&type->node)) {
    return prim->name == "()";
  }
  return false;
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

void AppendPad(std::vector<llvm::Type*>& elems,
               llvm::LLVMContext& ctx,
               std::uint64_t pad) {
  if (pad == 0) {
    return;
  }
  elems.push_back(llvm::ArrayType::get(llvm::Type::getInt8Ty(ctx), pad));
}

void AppendStructElems(std::vector<llvm::Type*>& elems,
                       const std::vector<analysis::TypeRef>& fields,
                       const std::vector<std::uint64_t>& offsets,
                       std::uint64_t size,
                       LLVMEmitter& emitter,
                       const analysis::ScopeContext& scope,
                       LowerCtx* lower_ctx) {
  if (fields.size() != offsets.size()) {
    if (lower_ctx) {
      lower_ctx->ReportCodegenFailure();
    }
    return;
  }

  std::uint64_t prev_end = 0;
  for (std::size_t i = 0; i < fields.size(); ++i) {
    const std::uint64_t offset = offsets[i];
    if (offset > prev_end) {
      AppendPad(elems, emitter.GetContext(), offset - prev_end);
    }

    elems.push_back(emitter.GetLLVMType(fields[i]));

    const auto field_size = SizeOf(scope, fields[i]);
    if (!field_size.has_value()) {
      if (lower_ctx) {
        lower_ctx->ReportCodegenFailure();
      }
      prev_end = offset;
      continue;
    }
    prev_end = offset + *field_size;
  }

  if (size > prev_end) {
    AppendPad(elems, emitter.GetContext(), size - prev_end);
  }
}

llvm::Type* AlignmentMarkerType(std::uint64_t align, LLVMEmitter& emitter) {
  llvm::LLVMContext& ctx = emitter.GetContext();
  switch (align) {
    case 1:
      return llvm::Type::getInt8Ty(ctx);
    case 2:
      return llvm::Type::getInt16Ty(ctx);
    case 4:
      return llvm::Type::getInt32Ty(ctx);
    case 8:
      return llvm::Type::getInt64Ty(ctx);
    case 16:
      return llvm::Type::getInt128Ty(ctx);
    default:
      return nullptr;
  }
}

llvm::Type* IntTypeForSize(std::uint64_t size, LLVMEmitter& emitter) {
  llvm::LLVMContext& ctx = emitter.GetContext();
  switch (size) {
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

llvm::Type* TaggedBlobType(std::uint64_t size,
                           std::uint64_t align,
                           LLVMEmitter& emitter) {
  llvm::LLVMContext& ctx = emitter.GetContext();
  if (size == 0) {
    return llvm::StructType::get(ctx, {});
  }
  llvm::Type* byte = llvm::Type::getInt8Ty(ctx);
  llvm::Type* bytes = llvm::ArrayType::get(byte, size);
  std::vector<llvm::Type*> fields;
  fields.push_back(bytes);
  if (align > 1) {
    if (llvm::Type* marker = AlignmentMarkerType(align, emitter)) {
      fields.push_back(llvm::ArrayType::get(marker, 0));
    }
  }
  return llvm::StructType::get(ctx, fields, /*isPacked=*/false);
}

llvm::Type* TaggedABIType(std::uint64_t size,
                          std::uint64_t align,
                          LLVMEmitter& emitter) {
  if (llvm::Type* int_ty = IntTypeForSize(size, emitter)) {
    return int_ty;
  }
  return TaggedBlobType(size, align, emitter);
}

llvm::Type* TaggedStructType(const analysis::TypeRef& disc_type,
                             std::uint64_t payload_size,
                             std::uint64_t payload_align,
                             std::uint64_t size,
                             LLVMEmitter& emitter,
                             const analysis::ScopeContext& scope,
                             LowerCtx* lower_ctx) {
  llvm::LLVMContext& ctx = emitter.GetContext();
  std::vector<llvm::Type*> elems;
  elems.push_back(emitter.GetLLVMType(disc_type));

  const auto disc_size = SizeOf(scope, disc_type);
  if (!disc_size.has_value()) {
    if (lower_ctx) {
      lower_ctx->ReportCodegenFailure();
    }
    return llvm::StructType::get(ctx, elems, /*isPacked=*/false);
  }

  const std::uint64_t payload_off = AlignUp(*disc_size, payload_align);
  const std::uint64_t pad_mid = payload_off - *disc_size;
  AppendPad(elems, ctx, pad_mid);

  llvm::Type* byte = llvm::Type::getInt8Ty(ctx);
  elems.push_back(llvm::ArrayType::get(byte, payload_size));

  const std::uint64_t payload_end = payload_off + payload_size;
  std::uint64_t pad_tail = 0;
  if (size < payload_end) {
    if (lower_ctx) {
      lower_ctx->ReportCodegenFailure();
    }
  } else {
    pad_tail = size - payload_end;
  }
  AppendPad(elems, ctx, pad_tail);

  return llvm::StructType::get(ctx, elems, /*isPacked=*/false);
}

struct AsyncLayoutInfo {
  std::uint64_t size = 0;
  std::uint64_t align = 1;
  std::uint64_t payload_size = 0;
  std::uint64_t payload_align = 1;
};

static bool IsNeverType(const analysis::TypeRef& type) {
  if (!type) return false;
  const auto* prim = std::get_if<analysis::TypePrim>(&type->node);
  return prim && prim->name == "!";
}

static std::optional<AsyncLayoutInfo> AsyncLayoutOf(
    const analysis::ScopeContext& scope,
    const std::vector<analysis::TypeRef>& generic_args) {
  std::uint64_t max_payload_size = 0;
  std::uint64_t max_payload_align = 1;

  auto add_payload_layout = [&](const std::optional<Layout>& layout_opt) {
    if (!layout_opt.has_value() || layout_opt->size == 0) {
      return;
    }
    max_payload_size = std::max(max_payload_size, layout_opt->size);
    max_payload_align = std::max(max_payload_align, layout_opt->align);
  };

  // Suspended payload is { output: Out, frame: Ptr<u8> } (hidden frame field)
  const auto out_type = generic_args.size() > 0
                            ? generic_args[0]
                            : analysis::MakeTypePrim("()");
  const auto frame_ptr = analysis::MakeTypePtr(analysis::MakeTypePrim("u8"),
                                               analysis::PtrState::Valid);
  const auto suspended_layout = RecordLayoutOf(scope, {out_type, frame_ptr});
  if (!suspended_layout.has_value()) {
    return std::nullopt;
  }
  add_payload_layout(suspended_layout->layout);

  // Completed payload (Result)
  if (generic_args.size() > 2 && generic_args[2] &&
      !IsNeverType(generic_args[2]) && !IsUnitType(generic_args[2])) {
    add_payload_layout(LayoutOf(scope, generic_args[2]));
  }

  // Failed payload (Error)
  if (generic_args.size() > 3 && generic_args[3] &&
      !IsNeverType(generic_args[3]) && !IsUnitType(generic_args[3])) {
    add_payload_layout(LayoutOf(scope, generic_args[3]));
  }

  const std::uint64_t disc_size = 1;
  const std::uint64_t disc_align = 1;
  const std::uint64_t align = std::max(disc_align, max_payload_align);
  const std::uint64_t size = AlignUp(disc_size + max_payload_size, align);

  AsyncLayoutInfo out;
  out.size = size;
  out.align = align;
  out.payload_size = max_payload_size;
  out.payload_align = max_payload_align;
  return out;
}

// §5.4.5: Build LLVM type for Async<Out, In, Result, E>
// Layout matches ModalLayout: tagged discriminant + max payload blob.
llvm::Type* BuildAsyncLLVMType(const analysis::ScopeContext& scope,
                               const std::vector<analysis::TypeRef>& generic_args,
                               LLVMEmitter& emitter,
                               LowerCtx* lower_ctx) {
  auto layout = AsyncLayoutOf(scope, generic_args);
  if (!layout.has_value()) {
    if (lower_ctx) {
      lower_ctx->ReportCodegenFailure();
    }
    return llvm::StructType::get(emitter.GetContext(), {});
  }
  const auto disc_type = analysis::MakeTypePrim("u8");
  return TaggedStructType(disc_type,
                          layout->payload_size,
                          layout->payload_align,
                          layout->size,
                          emitter,
                          scope,
                          lower_ctx);
}

std::optional<analysis::TypeRef> ModalNichePayloadType(
    const analysis::ScopeContext& scope,
    const syntax::ModalDecl& decl) {
  const auto payload_state = analysis::PayloadState(scope, decl);
  if (!payload_state.has_value()) {
    return std::nullopt;
  }

  for (const auto& state : decl.states) {
    if (!analysis::IdEq(state.name, *payload_state)) {
      continue;
    }
    const syntax::StateFieldDecl* field = nullptr;
    for (const auto& member : state.members) {
      const auto* candidate = std::get_if<syntax::StateFieldDecl>(&member);
      if (!candidate) {
        continue;
      }
      if (field) {
        return std::nullopt;
      }
      field = candidate;
    }
    if (!field) {
      return std::nullopt;
    }
    return LowerTypeForLayout(scope, field->type);
  }

  return std::nullopt;
}

std::vector<analysis::TypeRef> RecordFieldTypes(const analysis::ScopeContext& scope,
                                            const syntax::RecordDecl& record) {
  std::vector<analysis::TypeRef> fields;
  for (const auto& member : record.members) {
    const auto* field = std::get_if<syntax::FieldDecl>(&member);
    if (!field) {
      continue;
    }
    const auto lowered = LowerTypeForLayout(scope, field->type);
    if (!lowered.has_value()) {
      return {};
    }
    fields.push_back(*lowered);
  }
  return fields;
}

std::vector<analysis::TypeRef> ModalStateFields(const analysis::ScopeContext& scope,
                                            const syntax::ModalDecl& decl,
                                            std::string_view state_name) {
  for (const auto& state : decl.states) {
    if (!analysis::IdEq(state.name, state_name)) {
      continue;
    }
    std::vector<analysis::TypeRef> fields;
    for (const auto& member : state.members) {
      const auto* field = std::get_if<syntax::StateFieldDecl>(&member);
      if (!field) {
        continue;
      }
      const auto lowered = LowerTypeForLayout(scope, field->type);
      if (!lowered.has_value()) {
        return {};
      }
      fields.push_back(*lowered);
    }
    return fields;
  }
  return {};
}

std::optional<std::uint64_t> MaxModalPayloadAlign(const analysis::ScopeContext& scope,
                                                  const syntax::ModalDecl& decl) {
  std::uint64_t max_align = 1;
  for (const auto& state : decl.states) {
    std::vector<analysis::TypeRef> fields;
    for (const auto& member : state.members) {
      const auto* field = std::get_if<syntax::StateFieldDecl>(&member);
      if (!field) {
        continue;
      }
      const auto lowered = LowerTypeForLayout(scope, field->type);
      if (!lowered.has_value()) {
        return std::nullopt;
      }
      fields.push_back(*lowered);
    }
    const auto layout = RecordLayoutOf(scope, fields);
    if (!layout.has_value()) {
      return std::nullopt;
    }
    max_align = std::max(max_align, layout->layout.align);
  }
  return max_align;
}

}  // namespace

// T-LLVM-002: Opaque Pointer Model
llvm::Type* LLVMEmitter::GetOpaquePtr() {
  SPEC_DEF("OpaquePointerModel", "§6.12.2");
  return llvm::PointerType::get(context_, 0); // Address space 0
}

// T-LLVM-007: LLVM Type Mapping
llvm::Type* LLVMEmitter::GetLLVMType(analysis::TypeRef type) {
  if (!type) {
    SPEC_RULE("LLVMTy-Err");
    return llvm::Type::getVoidTy(context_); // Error/Void fallback
  }

  if (type_cache_.count(type)) {
    return type_cache_[type];
  }

  llvm::Type* ll_ty = nullptr;
  const auto scope_opt = BuildScope(current_ctx_);

  if (const auto* prim = std::get_if<analysis::TypePrim>(&type->node)) {
    SPEC_RULE("LLVMTy-Prim");
    const std::string& name = prim->name;
    if (name == "bool") {
      ll_ty = llvm::Type::getInt8Ty(context_);
    } else if (name == "char") {
      ll_ty = llvm::Type::getInt32Ty(context_);
    } else if (name == "i8" || name == "u8") {
      ll_ty = llvm::Type::getInt8Ty(context_);
    } else if (name == "i16" || name == "u16") {
      ll_ty = llvm::Type::getInt16Ty(context_);
    } else if (name == "i32" || name == "u32") {
      ll_ty = llvm::Type::getInt32Ty(context_);
    } else if (name == "i64" || name == "u64" || name == "usize" || name == "isize") {
      ll_ty = llvm::Type::getInt64Ty(context_);
    } else if (name == "i128" || name == "u128") {
      ll_ty = llvm::Type::getInt128Ty(context_);
    } else if (name == "f16") {
      ll_ty = llvm::Type::getHalfTy(context_);
    } else if (name == "f32") {
      ll_ty = llvm::Type::getFloatTy(context_);
    } else if (name == "f64") {
      ll_ty = llvm::Type::getDoubleTy(context_);
    } else if (name == "unit" || name == "()" || name == "never" || name == "!") {
      ll_ty = llvm::StructType::get(context_, {}); // ZST
    } else {
      ll_ty = llvm::Type::getInt8Ty(context_);
    }
  } else if (const auto* perm = std::get_if<analysis::TypePerm>(&type->node)) {
    SPEC_RULE("LLVMTy-Perm");
    ll_ty = GetLLVMType(perm->base);
  } else if (std::holds_alternative<analysis::TypePtr>(type->node)) {
    SPEC_RULE("LLVMTy-Ptr");
    ll_ty = GetOpaquePtr();
  } else if (std::holds_alternative<analysis::TypeRawPtr>(type->node)) {
    SPEC_RULE("LLVMTy-RawPtr");
    ll_ty = GetOpaquePtr();
  } else if (std::holds_alternative<analysis::TypeFunc>(type->node)) {
    SPEC_RULE("LLVMTy-Func");
    ll_ty = GetOpaquePtr();
  } else if (const auto* tuple = std::get_if<analysis::TypeTuple>(&type->node)) {
    SPEC_RULE("LLVMTy-Tuple");
    if (!scope_opt.has_value()) {
      if (current_ctx_) {
        current_ctx_->ReportCodegenFailure();
      }
      ll_ty = llvm::StructType::get(context_, {});
    } else {
      const auto layout = RecordLayoutOf(*scope_opt, tuple->elements);
      std::vector<llvm::Type*> elems;
      if (layout.has_value()) {
        AppendStructElems(elems, tuple->elements, layout->offsets, layout->layout.size,
                          *this, *scope_opt, current_ctx_);
      }
      ll_ty = llvm::StructType::get(context_, elems);
    }
  } else if (const auto* arr = std::get_if<analysis::TypeArray>(&type->node)) {
    SPEC_RULE("LLVMTy-Array");
    llvm::Type* elem_ty = GetLLVMType(arr->element);
    ll_ty = llvm::ArrayType::get(elem_ty, arr->length);
  } else if (const auto* slice = std::get_if<analysis::TypeSlice>(&type->node)) {
    SPEC_RULE("LLVMTy-Slice");
    llvm::Type* ptr_ty = GetOpaquePtr();
    llvm::Type* len_ty = GetLLVMType(analysis::MakeTypePrim("usize"));
    ll_ty = llvm::StructType::get(context_, {ptr_ty, len_ty});
  } else if (const auto* str = std::get_if<analysis::TypeString>(&type->node)) {
    if (str->state.has_value() && *str->state == analysis::StringState::View) {
      SPEC_RULE("LLVMTy-StringView");
      llvm::Type* ptr_ty = GetOpaquePtr();
      llvm::Type* len_ty = GetLLVMType(analysis::MakeTypePrim("usize"));
      ll_ty = llvm::StructType::get(context_, {ptr_ty, len_ty});
    } else if (str->state.has_value() && *str->state == analysis::StringState::Managed) {
      SPEC_RULE("LLVMTy-StringManaged");
      llvm::Type* ptr_ty = GetOpaquePtr();
      llvm::Type* len_ty = GetLLVMType(analysis::MakeTypePrim("usize"));
      ll_ty = llvm::StructType::get(context_, {ptr_ty, len_ty, len_ty});
    } else if (scope_opt.has_value()) {
      SPEC_RULE("LLVMTy-Modal-StringBytes");
      const std::uint64_t disc_size = 1;
      const std::uint64_t disc_align = 1;
      const std::uint64_t payload_size = 3 * kPtrSize;
      const std::uint64_t payload_align = kPtrAlign;
      const std::uint64_t align = std::max(disc_align, payload_align);
      const std::uint64_t size = AlignUp(disc_size + payload_size, align);
      ll_ty = TaggedABIType(size, align, *this);
    }
  } else if (const auto* bytes = std::get_if<analysis::TypeBytes>(&type->node)) {
    if (bytes->state.has_value() && *bytes->state == analysis::BytesState::View) {
      SPEC_RULE("LLVMTy-BytesView");
      llvm::Type* ptr_ty = GetOpaquePtr();
      llvm::Type* len_ty = GetLLVMType(analysis::MakeTypePrim("usize"));
      ll_ty = llvm::StructType::get(context_, {ptr_ty, len_ty});
    } else if (bytes->state.has_value() && *bytes->state == analysis::BytesState::Managed) {
      SPEC_RULE("LLVMTy-BytesManaged");
      llvm::Type* ptr_ty = GetOpaquePtr();
      llvm::Type* len_ty = GetLLVMType(analysis::MakeTypePrim("usize"));
      ll_ty = llvm::StructType::get(context_, {ptr_ty, len_ty, len_ty});
    } else if (scope_opt.has_value()) {
      SPEC_RULE("LLVMTy-Modal-StringBytes");
      const std::uint64_t disc_size = 1;
      const std::uint64_t disc_align = 1;
      const std::uint64_t payload_size = 3 * kPtrSize;
      const std::uint64_t payload_align = kPtrAlign;
      const std::uint64_t align = std::max(disc_align, payload_align);
      const std::uint64_t size = AlignUp(disc_size + payload_size, align);
      ll_ty = TaggedABIType(size, align, *this);
    }
  } else if (std::holds_alternative<analysis::TypeDynamic>(type->node)) {
    SPEC_RULE("LLVMTy-Dynamic");
    auto dyn = DynLayoutOf();
    std::vector<llvm::Type*> elems;
    elems.reserve(dyn.fields.size());
    for (const auto& field : dyn.fields) {
      elems.push_back(GetLLVMType(field));
    }
    ll_ty = llvm::StructType::get(context_, elems);
  } else if (std::holds_alternative<analysis::TypeModalState>(type->node)) {
    SPEC_RULE("LLVMTy-ModalState");
    if (!scope_opt.has_value()) {
      if (current_ctx_) {
        current_ctx_->ReportCodegenFailure();
      }
      ll_ty = llvm::StructType::get(context_, {});
    } else {
      const auto* modal = std::get_if<analysis::TypeModalState>(&type->node);
      const bool is_async = modal && modal->path.size() == 1 &&
                            analysis::IdEq(modal->path[0], "Async");
      if (modal && (analysis::IsSpawnedTypePath(modal->path) ||
                    analysis::IsCancelTokenTypePath(modal->path) ||
                    analysis::IsTrackedTypePath(modal->path))) {
        ll_ty = GetOpaquePtr();
        type_cache_[type] = ll_ty;
        return ll_ty;
      }
      // §5.4.5: Build concrete struct type for Async modal state
      if (is_async && !modal->generic_args.empty()) {
        SPEC_RULE("LLVMTy-AsyncState");
        ll_ty = BuildAsyncLLVMType(*scope_opt, modal->generic_args, *this, current_ctx_);
        type_cache_[type] = ll_ty;
        return ll_ty;
      } else if (is_async) {
        ll_ty = GetOpaquePtr();
        type_cache_[type] = ll_ty;
        return ll_ty;
      }
      syntax::Path syntax_path;
      syntax_path.reserve(modal->path.size());
      for (const auto& comp : modal->path) {
        syntax_path.push_back(comp);
      }
      const auto it = scope_opt->sigma.types.find(analysis::PathKeyOf(syntax_path));
      if (it != scope_opt->sigma.types.end()) {
        if (const auto* decl = std::get_if<syntax::ModalDecl>(&it->second)) {
          const auto fields = ModalStateFields(*scope_opt, *decl, modal->state);
          const auto layout = RecordLayoutOf(*scope_opt, fields);
          std::vector<llvm::Type*> elems;
          if (layout.has_value()) {
            AppendStructElems(elems, fields, layout->offsets, layout->layout.size,
                              *this, *scope_opt, current_ctx_);
          }
          ll_ty = llvm::StructType::get(context_, elems);
        }
      }
    }
  } else if (std::holds_alternative<analysis::TypeRange>(type->node)) {
    SPEC_RULE("LLVMTy-Range");
    if (!scope_opt.has_value()) {
      if (current_ctx_) {
        current_ctx_->ReportCodegenFailure();
      }
      ll_ty = llvm::StructType::get(context_, {});
    } else {
      std::vector<analysis::TypeRef> fields;
      fields.push_back(analysis::MakeTypePrim("u8"));
      fields.push_back(analysis::MakeTypePrim("usize"));
      fields.push_back(analysis::MakeTypePrim("usize"));
      const auto layout = RangeLayoutOf(*scope_opt);
      std::vector<llvm::Type*> elems;
      if (layout.has_value()) {
        AppendStructElems(elems, fields, layout->offsets, layout->layout.size,
                          *this, *scope_opt, current_ctx_);
      }
      ll_ty = llvm::StructType::get(context_, elems);
    }
  } else if (const auto* uni = std::get_if<analysis::TypeUnion>(&type->node)) {
    if (!scope_opt.has_value()) {
      SPEC_RULE("LLVMTy-Union-Tagged");
      ll_ty = llvm::StructType::get(context_, {});
    } else {
      const auto layout = UnionLayoutOf(*scope_opt, *uni);
      if (layout.has_value() && layout->niche) {
        SPEC_RULE("LLVMTy-Union-Niche");
        analysis::TypeRef payload;
        for (const auto& member : layout->member_list) {
          if (IsUnitType(member)) {
            continue;
          }
          payload = member;
          break;
        }
        if (payload) {
          ll_ty = GetLLVMType(payload);
        }
      }
      if (!ll_ty && layout.has_value()) {
        SPEC_RULE("LLVMTy-Union-Tagged");
        if (layout->disc_type.has_value()) {
          ll_ty = TaggedABIType(layout->layout.size,
                                layout->layout.align,
                                *this);
        }
      }
    }
  } else if (const auto* path = std::get_if<analysis::TypePathType>(&type->node)) {
    if (!scope_opt.has_value()) {
      SPEC_RULE("LLVMTy-Err");
      ll_ty = GetOpaquePtr();
    } else {
      const bool is_async = path->path.size() == 1 &&
                            analysis::IdEq(path->path[0], "Async");
      if (analysis::IsSpawnedTypePath(path->path) ||
          analysis::IsCancelTokenTypePath(path->path) ||
          analysis::IsTrackedTypePath(path->path)) {
        ll_ty = GetOpaquePtr();
        type_cache_[type] = ll_ty;
        return ll_ty;
      }
      // §5.4.5: Build concrete struct type for Async<Out, In, Result, E>
      if (is_async && !path->generic_args.empty()) {
        SPEC_RULE("LLVMTy-Async");
        ll_ty = BuildAsyncLLVMType(*scope_opt, path->generic_args, *this, current_ctx_);
        type_cache_[type] = ll_ty;
        return ll_ty;
      } else if (is_async) {
        // Async without generic args - return opaque pointer as fallback
        ll_ty = GetOpaquePtr();
        type_cache_[type] = ll_ty;
        return ll_ty;
      }

      // §5.4.5: Handle async type aliases - expand to Async and build struct type
      // Future<T; E = !> = Async<(), (), T, E>
      // Sequence<T> = Async<T, (), (), !>
      // Stream<T; E> = Async<T, (), (), E>
      // Pipe<In; Out> = Async<Out, In, (), !>
      // Exchange<T> = Async<T, T, T, !>
      const std::string& type_name = path->path.empty() ? "" : path->path.back();
      if (analysis::IdEq(type_name, "Future") && !path->generic_args.empty()) {
        SPEC_RULE("LLVMTy-Future");
        auto unit_type = analysis::MakeTypeTuple({});
        auto never_type = analysis::MakeTypePrim("!");
        std::vector<analysis::TypeRef> async_args = {
            unit_type,  // Out
            unit_type,  // In
            path->generic_args[0],  // Result (T)
            path->generic_args.size() > 1 ? path->generic_args[1] : never_type  // E
        };
        ll_ty = BuildAsyncLLVMType(*scope_opt, async_args, *this, current_ctx_);
        type_cache_[type] = ll_ty;
        return ll_ty;
      }
      if (analysis::IdEq(type_name, "Sequence") && !path->generic_args.empty()) {
        SPEC_RULE("LLVMTy-Sequence");
        auto unit_type = analysis::MakeTypeTuple({});
        auto never_type = analysis::MakeTypePrim("!");
        std::vector<analysis::TypeRef> async_args = {
            path->generic_args[0],  // Out (T)
            unit_type,  // In
            unit_type,  // Result
            never_type  // E
        };
        ll_ty = BuildAsyncLLVMType(*scope_opt, async_args, *this, current_ctx_);
        type_cache_[type] = ll_ty;
        return ll_ty;
      }
      if (analysis::IdEq(type_name, "Stream") && path->generic_args.size() >= 2) {
        SPEC_RULE("LLVMTy-Stream");
        auto unit_type = analysis::MakeTypeTuple({});
        std::vector<analysis::TypeRef> async_args = {
            path->generic_args[0],  // Out (T)
            unit_type,  // In
            unit_type,  // Result
            path->generic_args[1]  // E
        };
        ll_ty = BuildAsyncLLVMType(*scope_opt, async_args, *this, current_ctx_);
        type_cache_[type] = ll_ty;
        return ll_ty;
      }
      if (analysis::IdEq(type_name, "Pipe") && path->generic_args.size() >= 2) {
        SPEC_RULE("LLVMTy-Pipe");
        auto unit_type = analysis::MakeTypeTuple({});
        auto never_type = analysis::MakeTypePrim("!");
        std::vector<analysis::TypeRef> async_args = {
            path->generic_args[1],  // Out
            path->generic_args[0],  // In
            unit_type,  // Result
            never_type  // E
        };
        ll_ty = BuildAsyncLLVMType(*scope_opt, async_args, *this, current_ctx_);
        type_cache_[type] = ll_ty;
        return ll_ty;
      }
      if (analysis::IdEq(type_name, "Exchange") && !path->generic_args.empty()) {
        SPEC_RULE("LLVMTy-Exchange");
        auto never_type = analysis::MakeTypePrim("!");
        std::vector<analysis::TypeRef> async_args = {
            path->generic_args[0],  // Out (T)
            path->generic_args[0],  // In (T)
            path->generic_args[0],  // Result (T)
            never_type  // E
        };
        ll_ty = BuildAsyncLLVMType(*scope_opt, async_args, *this, current_ctx_);
        type_cache_[type] = ll_ty;
        return ll_ty;
      }
      syntax::Path syntax_path;
      syntax_path.reserve(path->path.size());
      for (const auto& comp : path->path) {
        syntax_path.push_back(comp);
      }
      const auto it = scope_opt->sigma.types.find(analysis::PathKeyOf(syntax_path));
      if (it != scope_opt->sigma.types.end()) {
        if (const auto* alias = std::get_if<syntax::TypeAliasDecl>(&it->second)) {
          SPEC_RULE("LLVMTy-Alias");
          const auto lowered = LowerTypeForLayout(*scope_opt, alias->type);
          if (lowered.has_value()) {
            // §13.1: Apply generic substitution if alias has params and type has args
            if (alias->generic_params && !alias->generic_params->params.empty() &&
                !path->generic_args.empty()) {
              analysis::TypeSubst subst = analysis::BuildSubstitution(
                  alias->generic_params->params, path->generic_args);
              analysis::TypeRef instantiated = analysis::InstantiateType(*lowered, subst);
              ll_ty = GetLLVMType(instantiated);
            } else {
              ll_ty = GetLLVMType(*lowered);
            }
          }
        } else if (const auto* record = std::get_if<syntax::RecordDecl>(&it->second)) {
          SPEC_RULE("LLVMTy-Record");
          const auto fields = RecordFieldTypes(*scope_opt, *record);
          const auto layout = RecordLayoutOf(*scope_opt, fields);
          std::vector<llvm::Type*> elems;
          if (layout.has_value()) {
            AppendStructElems(elems, fields, layout->offsets, layout->layout.size,
                              *this, *scope_opt, current_ctx_);
          }
          ll_ty = llvm::StructType::get(context_, elems);
        } else if (const auto* enum_decl = std::get_if<syntax::EnumDecl>(&it->second)) {
          SPEC_RULE("LLVMTy-Enum");
          const auto layout = EnumLayoutOf(*scope_opt, *enum_decl);
          if (layout.has_value()) {
            ll_ty = TaggedABIType(layout->layout.size,
                                  layout->layout.align,
                                  *this);
          }
        } else if (const auto* modal_decl = std::get_if<syntax::ModalDecl>(&it->second)) {
          if (scope_opt.has_value()) {
            const auto layout = ModalLayoutOf(*scope_opt, *modal_decl);
            if (layout.has_value() && layout->niche) {
              SPEC_RULE("LLVMTy-Modal-Niche");
              const auto payload = ModalNichePayloadType(*scope_opt, *modal_decl);
              if (payload.has_value()) {
                ll_ty = GetLLVMType(*payload);
              }
            }
            if (!ll_ty && layout.has_value()) {
              SPEC_RULE("LLVMTy-Modal-Tagged");
              if (layout->disc_type.has_value()) {
                const auto payload_align = MaxModalPayloadAlign(*scope_opt, *modal_decl);
                if (!payload_align.has_value() && current_ctx_) {
                  current_ctx_->ReportCodegenFailure();
                }
                const auto disc_type = analysis::MakeTypePrim(*layout->disc_type);
                ll_ty = TaggedStructType(disc_type,
                                         layout->payload_size,
                                         payload_align.value_or(1),
                                         layout->layout.size,
                                         *this,
                                         *scope_opt,
                                         current_ctx_);
              }
            }
          }
        }
      }
    }
  }

  if (!ll_ty) {
    SPEC_RULE("LLVMTy-Err");
    ll_ty = llvm::Type::getInt8Ty(context_);
  }

  type_cache_[type] = ll_ty;
  return ll_ty;
}

} // namespace cursive0::codegen
