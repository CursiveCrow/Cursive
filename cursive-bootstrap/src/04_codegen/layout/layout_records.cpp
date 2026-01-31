#include "cursive0/04_codegen/layout/layout.h"

#include <algorithm>

#include "cursive0/00_core/assert_spec.h"

namespace cursive0::codegen {
namespace {

std::uint64_t AlignUp(std::uint64_t value, std::uint64_t align) {
  if (align == 0) {
    return value;
  }
  const std::uint64_t rem = value % align;
  if (rem == 0) {
    return value;
  }
  return value + (align - rem);
}

}  // namespace

std::optional<RecordLayout> RecordLayoutOf(
    const cursive0::analysis::ScopeContext& ctx,
    const std::vector<cursive0::analysis::TypeRef>& fields) {
  if (fields.empty()) {
    SPEC_RULE("Layout-Record-Empty");
    return RecordLayout{Layout{0, 1}, {}};
  }
  SPEC_RULE("Layout-Record-Cons");
  std::vector<std::uint64_t> offsets;
  offsets.reserve(fields.size());
  std::uint64_t offset = 0;
  std::uint64_t max_align = 1;
  for (std::size_t i = 0; i < fields.size(); ++i) {
    const auto align = AlignOf(ctx, fields[i]);
    const auto size = SizeOf(ctx, fields[i]);
    if (!align.has_value() || !size.has_value()) {
      return std::nullopt;
    }
    if (i == 0) {
      offset = 0;
    } else {
      offset = AlignUp(offset, *align);
    }
    offsets.push_back(offset);
    max_align = std::max(max_align, *align);
    offset += *size;
  }
  const std::uint64_t size = AlignUp(offset, max_align);
  return RecordLayout{Layout{size, max_align}, offsets};
}

}  // namespace cursive0::codegen
