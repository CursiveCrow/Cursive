#pragma once

#include <optional>

#include "cursive0/00_core/diagnostic_codes.h"

namespace cursive0::core {

enum class BehaviorClass {
  Specified,
  UVB,
};

enum class RawPtrPermission {
  Imm,
  Mut,
};

BehaviorClass BehaviorOfDynamicUndefined(bool dynamic_undefined);

bool DynamicUndefinedReadPtr(RawPtrPermission perm, bool read_addr_defined);
bool DynamicUndefinedWritePtr(RawPtrPermission perm);

std::optional<DiagCode> StaticUndefinedCode(const DiagCodeMap& spec_map,
                                            const DiagCodeMap& c0_map,
                                            const DiagId& id);

}  // namespace cursive0::core
