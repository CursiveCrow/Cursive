#include "04_analysis/typing/dynamic_context.h"

#include "04_analysis/attributes/attribute_registry.h"

namespace cursive::analysis {

namespace {

bool HasStaticOrTrust(const ast::AttributeList& attrs_list) {
  return HasAttribute(attrs_list, attrs::kStatic) ||
         HasAttribute(attrs_list, attrs::kTrust);
}

}  // namespace

bool FindInnermostDynamic(
    std::span<const ast::AttributeList* const> ancestors) {
  for (auto it = ancestors.rbegin(); it != ancestors.rend(); ++it) {
    if (!*it) {
      continue;
    }
    if (HasStaticOrTrust(**it)) {
      return false;
    }
    if (HasAttribute(**it, attrs::kDynamic)) {
      return true;
    }
  }
  return false;
}

bool ComputeDynamicContext(
    std::span<const ast::AttributeList* const> ancestors,
    bool fallback_dynamic) {
  if (FindInnermostDynamic(ancestors)) {
    return true;
  }
  return fallback_dynamic;
}

}  // namespace cursive::analysis

