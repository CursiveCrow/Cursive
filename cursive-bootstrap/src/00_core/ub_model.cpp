#include "cursive0/00_core/ub_model.h"

#include "cursive0/00_core/assert_spec.h"

namespace cursive0::core {

static inline void SpecDefsUBModel() {
  SPEC_DEF("BehaviorClass", "1.2");
  SPEC_DEF("UVBRel", "1.2");
  SPEC_DEF("DynamicUndefined", "1.2");
  SPEC_DEF("Behavior", "1.2");
  SPEC_DEF("StaticUndefined", "1.2");
  SPEC_DEF("DiagIdOf", "1.2");
}

BehaviorClass BehaviorOfDynamicUndefined(bool dynamic_undefined) {
  SpecDefsUBModel();
  if (dynamic_undefined) {
    SPEC_RULE("Dynamic-Undefined-UVB");
    return BehaviorClass::UVB;
  }
  return BehaviorClass::Specified;
}

bool DynamicUndefinedReadPtr(RawPtrPermission perm, bool read_addr_defined) {
  SpecDefsUBModel();
  (void)perm;
  return !read_addr_defined;
}

bool DynamicUndefinedWritePtr(RawPtrPermission perm) {
  SpecDefsUBModel();
  return perm == RawPtrPermission::Imm;
}

std::optional<DiagCode> StaticUndefinedCode(const DiagCodeMap& spec_map,
                                            const DiagCodeMap& c0_map,
                                            const DiagId& id) {
  SpecDefsUBModel();
  const auto code = Code(spec_map, c0_map, id);
  if (code.has_value()) {
    SPEC_RULE("Static-Undefined");
    return code;
  }
  SPEC_RULE("Static-Undefined-NoCode");
  return std::nullopt;
}

}  // namespace cursive0::core
