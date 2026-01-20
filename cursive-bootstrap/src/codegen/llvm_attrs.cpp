#include "cursive0/codegen/llvm_emit.h"
#include "cursive0/codegen/layout.h"
#include "cursive0/core/assert_spec.h"
#include "cursive0/sema/scopes.h"
#include "cursive0/sema/types.h"

// LLVM Includes
#include "llvm/IR/Attributes.h"
#include "llvm/IR/Function.h"

namespace cursive0::codegen {

namespace {

std::optional<sema::ScopeContext> BuildScope(const LowerCtx* ctx) {
  if (!ctx || !ctx->sigma) {
    return std::nullopt;
  }
  sema::ScopeContext scope;
  scope.sigma = *ctx->sigma;
  scope.current_module = ctx->module_path;
  return scope;
}

sema::TypeRef StripPerm(const sema::TypeRef& type) {
  if (!type) {
    return type;
  }
  if (const auto* perm = std::get_if<sema::TypePerm>(&type->node)) {
    SPEC_RULE("PtrStateOf-Perm");
    return StripPerm(perm->base);
  }
  return type;
}

sema::Permission PermOf(const sema::TypeRef& type) {
  if (!type) {
    return sema::Permission::Shared;
  }
  if (const auto* perm = std::get_if<sema::TypePerm>(&type->node)) {
    return perm->perm;
  }
  return sema::Permission::Shared;
}

void AddArgAttrs(llvm::AttrBuilder& b, const sema::TypeRef& type) {
  const auto stripped = StripPerm(type);
  if (!stripped) {
    return;
  }
  if (std::holds_alternative<sema::TypePtr>(stripped->node) ||
      std::holds_alternative<sema::TypeFunc>(stripped->node)) {
    const auto perm = PermOf(type);
    if (perm == sema::Permission::Unique) {
      b.addAttribute(llvm::Attribute::NoAlias);
    }
    if (perm == sema::Permission::Const) {
      b.addAttribute(llvm::Attribute::ReadOnly);
    }
  }
}

void AddPtrAttrs(llvm::AttrBuilder& b,
                 const sema::TypeRef& type,
                 const std::optional<sema::ScopeContext>& scope_opt) {
  const auto stripped = StripPerm(type);
  if (!stripped) {
    return;
  }
  const auto* ptr = std::get_if<sema::TypePtr>(&stripped->node);
  if (ptr) {
    if (!ptr->state.has_value() || *ptr->state != sema::PtrState::Valid) {
      SPEC_RULE("LLVM-PtrAttrs-Other");
      return;
    }
  } else if (std::holds_alternative<sema::TypeRawPtr>(stripped->node)) {
    SPEC_RULE("LLVM-PtrAttrs-RawPtr");
    return;
  } else {
    return;
  }

  b.addAttribute(llvm::Attribute::NonNull);
  b.addAttribute(llvm::Attribute::NoUndef);

  if (!scope_opt.has_value()) {
    return;
  }

  const auto size = SizeOf(*scope_opt, ptr->element);
  const auto align = AlignOf(*scope_opt, ptr->element);
  if (size.has_value()) {
    b.addDereferenceableAttr(*size);
  }
  if (align.has_value() && *align > 0) {
    b.addAlignmentAttr(llvm::Align(*align));
  }
}

}  // namespace

// T-LLVM-003: LLVM attribute mapping
void LLVMEmitter::AddPtrAttributes(llvm::Function* func,
                                   unsigned arg_idx,
                                   sema::TypeRef type) {
  SPEC_RULE("LLVM-PtrAttrs-Valid");
  SPEC_RULE("LLVM-ArgAttrs-Ptr");
  SPEC_RULE("LLVM-ArgAttrs-RawPtr");
  SPEC_RULE("LLVM-ArgAttrs-NonPtr");

  if (!func || !type) {
    return;
  }

  llvm::AttrBuilder b(context_);
  const auto scope_opt = BuildScope(current_ctx_);

  AddArgAttrs(b, type);
  AddPtrAttrs(b, type, scope_opt);

  if (b.hasAttributes()) {
    func->addParamAttrs(arg_idx, b);
  }
}

} // namespace cursive0::codegen
