#include <cstddef>
#include <cstdint>
#include <vector>

#include "cursive0/core/source_load.h"
#include "cursive0/syntax/lexer.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  std::vector<uint8_t> bytes(data, data + size);
  const auto loaded = cursive0::core::LoadSource("fuzz.cursive", bytes);
  if (!loaded.source) {
    return 0;
  }
  const auto result = cursive0::syntax::Tokenize(*loaded.source);
  (void)result;
  return 0;
}
