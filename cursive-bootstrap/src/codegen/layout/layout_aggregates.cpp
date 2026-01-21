#include "cursive0/codegen/layout/layout.h"

#include <algorithm>

#include "cursive0/core/assert_spec.h"

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

std::optional<std::string> DiscTypeName(std::uint64_t max_disc) {
  if (max_disc <= 0xFFu) {
    return std::string("u8");
  }
  if (max_disc <= 0xFFFFu) {
    return std::string("u16");
  }
  if (max_disc <= 0xFFFFFFFFu) {
    return std::string("u32");
  }
  return std::string("u64");
}

std::optional<Layout> DiscTypeLayout(std::uint64_t max_disc) {
  const auto name = DiscTypeName(max_disc);
  if (!name.has_value()) {
    return std::nullopt;
  }
  const auto size = PrimSize(*name);
  const auto align = PrimAlign(*name);
  if (!size.has_value() || !align.has_value()) {
    return std::nullopt;
  }
  return Layout{*size, *align};
}

std::optional<RecordLayout> RangeLayoutInternal(
    const cursive0::analysis::ScopeContext& ctx) {
  SPEC_RULE("Layout-Range");
  std::vector<cursive0::analysis::TypeRef> fields;
  fields.push_back(cursive0::analysis::MakeTypePrim("u8"));
  fields.push_back(cursive0::analysis::MakeTypePrim("usize"));
  fields.push_back(cursive0::analysis::MakeTypePrim("usize"));
  return RecordLayoutOf(ctx, fields);
}

}  // namespace

std::optional<RecordLayout> RangeLayoutOf(
    const cursive0::analysis::ScopeContext& ctx) {
  return RangeLayoutInternal(ctx);
}


std::optional<EnumLayout> EnumLayoutOf(
    const cursive0::analysis::ScopeContext& ctx,
    const cursive0::syntax::EnumDecl& decl) {
  if (decl.variants.empty()) {
    return std::nullopt;
  }
  const auto discs = cursive0::analysis::EnumDiscriminants(decl);
  if (!discs.ok || discs.discs.empty()) {
    return std::nullopt;
  }
  const auto disc_layout = DiscTypeLayout(discs.max_disc);
  if (!disc_layout.has_value()) {
    return std::nullopt;
  }

  std::uint64_t payload_size = 0;
  std::uint64_t payload_align = 1;

  for (const auto& variant : decl.variants) {
    std::optional<RecordLayout> layout;
    if (!variant.payload_opt.has_value()) {
      layout = RecordLayout{Layout{0, 1}, {}};
    } else if (const auto* tuple =
                   std::get_if<cursive0::syntax::VariantPayloadTuple>(
                       &*variant.payload_opt)) {
      std::vector<cursive0::analysis::TypeRef> elems;
      elems.reserve(tuple->elements.size());
      for (const auto& elem : tuple->elements) {
        const auto lowered = LowerTypeForLayout(ctx, elem);
        if (!lowered.has_value()) {
          return std::nullopt;
        }
        elems.push_back(*lowered);
      }
      layout = RecordLayoutOf(ctx, elems);
    } else if (const auto* rec =
                   std::get_if<cursive0::syntax::VariantPayloadRecord>(
                       &*variant.payload_opt)) {
      std::vector<cursive0::analysis::TypeRef> fields;
      fields.reserve(rec->fields.size());
      for (const auto& field : rec->fields) {
        const auto lowered = LowerTypeForLayout(ctx, field.type);
        if (!lowered.has_value()) {
          return std::nullopt;
        }
        fields.push_back(*lowered);
      }
      layout = RecordLayoutOf(ctx, fields);
    }

    if (!layout.has_value()) {
      return std::nullopt;
    }
    payload_size = std::max(payload_size, layout->layout.size);
    payload_align = std::max(payload_align, layout->layout.align);
  }

  SPEC_RULE("Layout-Enum-Tagged");
  const std::uint64_t align =
      std::max(disc_layout->align, payload_align);
  const std::uint64_t size = AlignUp(disc_layout->size + payload_size, align);

  EnumLayout out;
  out.layout = Layout{size, align};
  out.disc_type = *DiscTypeName(discs.max_disc);
  out.payload_size = payload_size;
  out.payload_align = payload_align;
  return out;
}

}  // namespace cursive0::codegen
