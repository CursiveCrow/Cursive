#include <cassert>

#include "cursive0/core/assert_spec.h"
#include "cursive0/core/ub_model.h"

namespace {

using cursive0::core::BehaviorClass;
using cursive0::core::BehaviorOfDynamicUndefined;
using cursive0::core::DiagCodeMap;
using cursive0::core::DynamicUndefinedReadPtr;
using cursive0::core::DynamicUndefinedWritePtr;
using cursive0::core::RawPtrPermission;
using cursive0::core::StaticUndefinedCode;

}  // namespace

int main() {
  SPEC_COV("Dynamic-Undefined-UVB");
  SPEC_COV("Static-Undefined");
  SPEC_COV("Static-Undefined-NoCode");

  {
    assert(BehaviorOfDynamicUndefined(true) == BehaviorClass::UVB);
    assert(BehaviorOfDynamicUndefined(false) == BehaviorClass::Specified);
  }

  {
    assert(DynamicUndefinedReadPtr(RawPtrPermission::Imm, false));
    assert(DynamicUndefinedReadPtr(RawPtrPermission::Mut, false));
    assert(!DynamicUndefinedReadPtr(RawPtrPermission::Imm, true));
  }

  {
    assert(DynamicUndefinedWritePtr(RawPtrPermission::Imm));
    assert(!DynamicUndefinedWritePtr(RawPtrPermission::Mut));
  }

  {
    DiagCodeMap spec;
    DiagCodeMap c0;
    spec["Rule-A"] = "E-AAA-0001";
    c0["Rule-B"] = "W-BBB-0002";

    const auto code_a = StaticUndefinedCode(spec, c0, "Rule-A");
    assert(code_a.has_value());
    assert(*code_a == "E-AAA-0001");

    const auto code_b = StaticUndefinedCode(spec, c0, "Rule-B");
    assert(code_b.has_value());
    assert(*code_b == "W-BBB-0002");

    const auto code_missing = StaticUndefinedCode(spec, c0, "Missing");
    assert(!code_missing.has_value());
  }

  return 0;
}
