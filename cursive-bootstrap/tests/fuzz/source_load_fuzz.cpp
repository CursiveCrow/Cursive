#include <cstddef>
#include <cstdint>
#include <vector>

#include "cursive0/core/source_load.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  std::vector<uint8_t> bytes(data, data + size);
  const auto result = cursive0::core::LoadSource("fuzz.cursive", bytes);
  (void)result;
  return 0;
}
