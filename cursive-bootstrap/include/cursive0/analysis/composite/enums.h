#pragma once

#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

#include "cursive0/core/span.h"
#include "cursive0/syntax/ast.h"

namespace cursive0::analysis {

struct EnumDiscResult {
  bool ok = false;
  std::optional<std::string_view> diag_id;
  std::optional<core::Span> span;
  std::vector<std::uint64_t> discs;
  std::uint64_t max_disc = 0;
};

EnumDiscResult EnumDiscriminants(const syntax::EnumDecl& decl);

}  // namespace cursive0::analysis
