#pragma once

#include <cstdint>

namespace cursive0::codegen {

constexpr std::uint64_t kAsyncFrameResumeStateOffset = 0;
constexpr std::uint64_t kAsyncFrameResumeFnOffset = 8;
constexpr std::uint64_t kAsyncFrameHeaderSize = 16;
constexpr std::uint64_t kAsyncFrameHeaderAlign = 8;

}  // namespace cursive0::codegen
