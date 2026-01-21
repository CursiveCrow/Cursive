#include "cursive0/analysis/memory/safe_ptr.h"

#include "cursive0/core/assert_spec.h"

namespace cursive0::analysis {

namespace {

static TypeRef StripPermOnce(const TypeRef& type) {
  if (!type) {
    return type;
  }
  if (const auto* perm = std::get_if<TypePerm>(&type->node)) {
    return perm->base;
  }
  return type;
}

}  // namespace

void SpecDefsSafePtr() {
  SPEC_DEF("PtrState", "5.2.16");
  SPEC_DEF("Ptr<T>", "5.2.16");
  SPEC_DEF("Ptr<T>@s", "5.2.16");
  SPEC_DEF("PtrDiagRefs", "5.2.16");
}

const TypePtr* AsSafePtr(const TypeRef& type) {
  SpecDefsSafePtr();
  if (!type) {
    return nullptr;
  }
  const auto stripped = StripPermOnce(type);
  if (!stripped) {
    return nullptr;
  }
  return std::get_if<TypePtr>(&stripped->node);
}

bool IsSafePtrType(const TypeRef& type) {
  return AsSafePtr(type) != nullptr;
}

std::optional<PtrState> PtrStateOf(const TypeRef& type) {
  const auto* ptr = AsSafePtr(type);
  if (!ptr) {
    return std::nullopt;
  }
  return ptr->state;
}

}  // namespace cursive0::analysis
