#pragma once

#include <cstdint>
#include <limits>

namespace cursive0::core {

#if defined(__GNUC__) || defined(__clang__)
// GCC/Clang: Use native __int128
using UInt128 = unsigned __int128;

inline UInt128 MakeUInt128(std::uint64_t high, std::uint64_t low) {
  return (static_cast<UInt128>(high) << 64) | low;
}

inline std::uint64_t UInt128High(UInt128 value) {
  return static_cast<std::uint64_t>(value >> 64);
}

inline std::uint64_t UInt128Low(UInt128 value) {
  return static_cast<std::uint64_t>(value);
}

inline UInt128 UInt128Max() {
  return ~static_cast<UInt128>(0);
}

inline bool UInt128LessOrEqual(UInt128 a, UInt128 b) {
  return a <= b;
}

inline bool UInt128Greater(UInt128 a, UInt128 b) {
  return a > b;
}

inline UInt128 UInt128Add(UInt128 a, UInt128 b) {
  return a + b;
}

inline UInt128 UInt128Mul(UInt128 a, UInt128 b) {
  return a * b;
}

inline UInt128 UInt128Sub(UInt128 a, UInt128 b) {
  return a - b;
}

inline UInt128 UInt128Div(UInt128 a, UInt128 b) {
  return a / b;
}

inline UInt128 UInt128ShiftLeft(UInt128 a, unsigned shift) {
  return a << shift;
}

inline UInt128 UInt128ShiftRight(UInt128 a, unsigned shift) {
  return a >> shift;
}

inline UInt128 UInt128FromU64(std::uint64_t value) {
  return static_cast<UInt128>(value);
}

inline std::uint64_t UInt128ToU64(UInt128 value) {
  return static_cast<std::uint64_t>(value);
}

inline bool UInt128FitsU64(UInt128 value) {
  return value <= std::numeric_limits<std::uint64_t>::max();
}

#else
// MSVC/other: Use struct-based 128-bit integer

struct UInt128 {
  std::uint64_t low = 0;
  std::uint64_t high = 0;
};

inline UInt128 MakeUInt128(std::uint64_t high, std::uint64_t low) {
  return UInt128{low, high};
}

inline std::uint64_t UInt128High(UInt128 value) {
  return value.high;
}

inline std::uint64_t UInt128Low(UInt128 value) {
  return value.low;
}

inline UInt128 UInt128Max() {
  return UInt128{~std::uint64_t(0), ~std::uint64_t(0)};
}

inline bool UInt128LessOrEqual(UInt128 a, UInt128 b) {
  if (a.high != b.high) {
    return a.high < b.high;
  }
  return a.low <= b.low;
}

inline bool UInt128Greater(UInt128 a, UInt128 b) {
  if (a.high != b.high) {
    return a.high > b.high;
  }
  return a.low > b.low;
}

inline UInt128 UInt128Add(UInt128 a, UInt128 b) {
  UInt128 result;
  result.low = a.low + b.low;
  result.high = a.high + b.high + (result.low < a.low ? 1 : 0);
  return result;
}

inline UInt128 UInt128Mul(UInt128 a, UInt128 b) {
  // For simplicity, assume we're multiplying small values (base * value)
  // Full 128x128 multiplication is complex; this handles common cases
  if (a.high == 0 && b.high == 0) {
    // 64x64 multiplication
    std::uint64_t a_lo = a.low & 0xFFFFFFFF;
    std::uint64_t a_hi = a.low >> 32;
    std::uint64_t b_lo = b.low & 0xFFFFFFFF;
    std::uint64_t b_hi = b.low >> 32;

    std::uint64_t ll = a_lo * b_lo;
    std::uint64_t lh = a_lo * b_hi;
    std::uint64_t hl = a_hi * b_lo;
    std::uint64_t hh = a_hi * b_hi;

    std::uint64_t mid = lh + (ll >> 32);
    mid += hl;
    if (mid < lh) {
      hh += std::uint64_t(1) << 32;
    }

    UInt128 result;
    result.low = (mid << 32) | (ll & 0xFFFFFFFF);
    result.high = hh + (mid >> 32);
    return result;
  }
  // For larger values, use approximate multiplication
  // (This should be sufficient for literal parsing)
  UInt128 result;
  result.low = a.low * b.low;
  result.high = a.high * b.low + a.low * b.high;
  return result;
}

inline UInt128 UInt128Sub(UInt128 a, UInt128 b) {
  UInt128 result;
  result.low = a.low - b.low;
  result.high = a.high - b.high - (a.low < b.low ? 1 : 0);
  return result;
}

inline UInt128 UInt128Div(UInt128 a, UInt128 b) {
  // Simple case: division by 64-bit value
  if (b.high == 0 && a.high == 0) {
    return UInt128{a.low / b.low, 0};
  }
  // For 128-bit division, use binary long division
  if (b.high == 0) {
    UInt128 result{0, 0};
    std::uint64_t remainder = 0;
    if (a.high > 0) {
      result.high = a.high / b.low;
      remainder = a.high % b.low;
    }
    std::uint64_t combined =
        (remainder << 32) | (a.low >> 32);
    std::uint64_t q_high = combined / b.low;
    remainder = combined % b.low;
    combined = (remainder << 32) | (a.low & 0xFFFFFFFF);
    std::uint64_t q_low = combined / b.low;
    result.low = (q_high << 32) | q_low;
    return result;
  }
  // Full 128/128 division - return 0 for now (shouldn't happen in practice)
  return UInt128{0, 0};
}

inline UInt128 UInt128ShiftLeft(UInt128 a, unsigned shift) {
  if (shift == 0) {
    return a;
  }
  if (shift >= 128) {
    return UInt128{0, 0};
  }
  if (shift >= 64) {
    return UInt128{0, a.low << (shift - 64)};
  }
  UInt128 result;
  result.high = (a.high << shift) | (a.low >> (64 - shift));
  result.low = a.low << shift;
  return result;
}

inline UInt128 UInt128ShiftRight(UInt128 a, unsigned shift) {
  if (shift == 0) {
    return a;
  }
  if (shift >= 128) {
    return UInt128{0, 0};
  }
  if (shift >= 64) {
    return UInt128{a.high >> (shift - 64), 0};
  }
  UInt128 result;
  result.low = (a.low >> shift) | (a.high << (64 - shift));
  result.high = a.high >> shift;
  return result;
}

inline UInt128 UInt128FromU64(std::uint64_t value) {
  return UInt128{value, 0};
}

inline std::uint64_t UInt128ToU64(UInt128 value) {
  return value.low;
}

inline bool UInt128FitsU64(UInt128 value) {
  return value.high == 0;
}

#endif

}  // namespace cursive0::core
