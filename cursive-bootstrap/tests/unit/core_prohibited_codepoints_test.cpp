#include <cassert>
#include <cstdint>
#include <optional>
#include <vector>

#include "cursive0/core/assert_spec.h"
#include "cursive0/core/unicode.h"

namespace {

using cursive0::core::FirstProhibitedOutsideLiteral;
using cursive0::core::NoProhibited;
using cursive0::core::UnicodeScalar;

std::vector<UnicodeScalar> Scalars(std::initializer_list<UnicodeScalar> list) {
  return std::vector<UnicodeScalar>(list);
}

void ExpectFirstIndex(const std::vector<UnicodeScalar>& scalars,
                      std::size_t index) {
  const auto result = FirstProhibitedOutsideLiteral(scalars);
  assert(result.has_value());
  assert(*result == index);
}

}  // namespace

int main() {
  SPEC_COV("WF-Prohibited");

  {
    assert(NoProhibited(Scalars({0x09})));
    assert(NoProhibited(Scalars({0x0A})));
    assert(NoProhibited(Scalars({0x0C})));
    assert(NoProhibited(Scalars({0x0D})));
  }

  {
    ExpectFirstIndex(Scalars({'a', 0x00, 'b'}), 1);
    ExpectFirstIndex(Scalars({'a', 0x7F, 'b'}), 1);
    ExpectFirstIndex(Scalars({'a', 0x85, 'b'}), 1);
    ExpectFirstIndex(Scalars({0x20AC, 0x9F}), 1);
  }

  {
    const auto scalars = Scalars({0x22, 0x00, 0x22});
    assert(NoProhibited(scalars));
  }

  {
    const auto scalars = Scalars({0x27, 0x01, 0x27});
    assert(NoProhibited(scalars));
  }

  {
    ExpectFirstIndex(Scalars({0x22, 0x00, 'a'}), 1);
    ExpectFirstIndex(Scalars({0x27, 0x01}), 1);
  }

  return 0;
}
