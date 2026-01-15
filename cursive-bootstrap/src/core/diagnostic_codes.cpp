#include "cursive0/core/diagnostic_codes.h"

#include "cursive0/core/assert_spec.h"

namespace cursive0::core {

static inline void SpecDefsDiagCodeTypes() {
  SPEC_DEF("DiagId", "1.2");
  SPEC_DEF("DiagCode", "0.4");
}

std::optional<DiagCode> SpecCode(const DiagCodeMap& spec_map, const DiagId& id) {
  SpecDefsDiagCodeTypes();
  auto it = spec_map.find(id);
  if (it == spec_map.end()) {
    return std::nullopt;
  }
  return it->second;
}

std::optional<DiagCode> C0Code(const DiagCodeMap& c0_map, const DiagId& id) {
  SpecDefsDiagCodeTypes();
  auto it = c0_map.find(id);
  if (it == c0_map.end()) {
    return std::nullopt;
  }
  return it->second;
}

std::optional<DiagCode> Code(const DiagCodeMap& spec_map,
                             const DiagCodeMap& c0_map,
                             const DiagId& id) {
  SpecDefsDiagCodeTypes();
  if (const auto spec = SpecCode(spec_map, id)) {
    SPEC_RULE("Code-Spec");
    return spec;
  }
  if (const auto c0 = C0Code(c0_map, id)) {
    SPEC_RULE("Code-C0");
    return c0;
  }
  return std::nullopt;
}

}  // namespace cursive0::core
