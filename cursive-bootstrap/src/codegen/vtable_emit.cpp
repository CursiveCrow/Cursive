#include "cursive0/codegen/llvm/llvm_emit.h"
#include "cursive0/codegen/layout/layout.h"
#include "cursive0/core/assert_spec.h"

// LLVM Includes
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Module.h"

namespace cursive0::codegen {

// T-LLVM-012: VTable Emission
// VTable layout: { usize size, usize align, ptr drop, ptr slots... }

void LLVMEmitter::EmitVTable(const GlobalVTable& vtable) {
  SPEC_RULE("EmitVTable");
  SPEC_RULE("EmitVTable-Header");
  SPEC_RULE("EmitVTable-Slots");
  SPEC_RULE("LowerIRDecl-VTable");

  llvm::Type* usize_ty = GetLLVMType(analysis::MakeTypePrim("usize"));
  llvm::Type* ptr_ty = GetOpaquePtr();
  if (!usize_ty || !ptr_ty) {
    SPEC_RULE("EmitVTable-Err");
    if (current_ctx_) {
      current_ctx_->ReportCodegenFailure();
    }
    return;
  }

  std::vector<llvm::Type*> field_types;
  field_types.reserve(3 + vtable.slots.size());
  field_types.push_back(usize_ty);
  field_types.push_back(usize_ty);
  field_types.push_back(ptr_ty);
  for (std::size_t i = 0; i < vtable.slots.size(); ++i) {
    field_types.push_back(ptr_ty);
  }

  llvm::StructType* vtable_ty = llvm::StructType::get(context_, field_types);

  std::vector<llvm::Constant*> elements;
  elements.reserve(field_types.size());

  elements.push_back(llvm::ConstantInt::get(usize_ty, vtable.header.size));
  elements.push_back(llvm::ConstantInt::get(usize_ty, vtable.header.align));

  llvm::Constant* drop_ptr = nullptr;
  if (vtable.header.drop_sym.empty()) {
    drop_ptr = llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(ptr_ty));
  } else if (auto* drop_fn = GetFunction(vtable.header.drop_sym)) {
    drop_ptr = llvm::ConstantExpr::getBitCast(drop_fn, ptr_ty);
  } else if (auto* drop_gv = GetGlobal(vtable.header.drop_sym)) {
    drop_ptr = llvm::ConstantExpr::getBitCast(llvm::cast<llvm::GlobalValue>(drop_gv), ptr_ty);
  } else {
    SPEC_RULE("EmitVTable-Err");
    if (current_ctx_) {
      current_ctx_->ReportCodegenFailure();
    }
    drop_ptr = llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(ptr_ty));
  }
  elements.push_back(drop_ptr);

  for (const auto& slot_sym : vtable.slots) {
    llvm::Constant* slot_ptr = nullptr;
    if (auto* slot_fn = GetFunction(slot_sym)) {
      slot_ptr = llvm::ConstantExpr::getBitCast(slot_fn, ptr_ty);
    } else if (auto* slot_gv = GetGlobal(slot_sym)) {
      slot_ptr = llvm::ConstantExpr::getBitCast(llvm::cast<llvm::GlobalValue>(slot_gv), ptr_ty);
    } else {
      SPEC_RULE("EmitVTable-Err");
      if (current_ctx_) {
        current_ctx_->ReportCodegenFailure();
      }
      slot_ptr = llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(ptr_ty));
    }
    elements.push_back(slot_ptr);
  }

  llvm::Constant* init = llvm::ConstantStruct::get(vtable_ty, elements);

  auto* gvar = new llvm::GlobalVariable(
      *module_,
      vtable_ty,
      true,
      llvm::GlobalValue::InternalLinkage,
      init,
      vtable.symbol);

  gvar->setAlignment(llvm::Align(kPtrAlign));
  gvar->setUnnamedAddr(llvm::GlobalValue::UnnamedAddr::Global);

  globals_[vtable.symbol] = gvar;
}

} // namespace cursive0::codegen
