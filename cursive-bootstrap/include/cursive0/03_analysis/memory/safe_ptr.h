#pragma once

#include <optional>

#include "cursive0/03_analysis/types/types.h"

namespace cursive0::analysis {

void SpecDefsSafePtr();

const TypePtr* AsSafePtr(const TypeRef& type);
bool IsSafePtrType(const TypeRef& type);
std::optional<PtrState> PtrStateOf(const TypeRef& type);

}  // namespace cursive0::analysis
