#include "cursive0/codegen/layout/layout.h"

#include <algorithm>

#include "cursive0/core/assert_spec.h"
#include "cursive0/analysis/modal/modal_widen.h"
#include "cursive0/analysis/resolve/scopes.h"

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

std::optional<RecordLayout> StatePayloadLayout(
    const cursive0::analysis::ScopeContext& ctx,
    const cursive0::syntax::StateBlock& state) {
  std::vector<cursive0::analysis::TypeRef> fields;
  for (const auto& member : state.members) {
    if (const auto* field =
            std::get_if<cursive0::syntax::StateFieldDecl>(&member)) {
      const auto lowered = LowerTypeForLayout(ctx, field->type);
      if (!lowered.has_value()) {
        return std::nullopt;
      }
      fields.push_back(*lowered);
    }
  }
  return RecordLayoutOf(ctx, fields);
}

}  // namespace

std::optional<ModalLayout> ModalLayoutOf(
    const cursive0::analysis::ScopeContext& ctx,
    const cursive0::syntax::ModalDecl& decl) {
  if (decl.states.empty()) {
    return std::nullopt;
  }

  const auto payload_state = cursive0::analysis::PayloadState(ctx, decl);
  if (payload_state.has_value()) {
    for (const auto& state : decl.states) {
      if (cursive0::analysis::IdEq(state.name, *payload_state)) {
        const auto layout = StatePayloadLayout(ctx, state);
        if (!layout.has_value()) {
          return std::nullopt;
        }
        SPEC_RULE("Layout-Modal-Niche");
        ModalLayout out;
        out.niche = true;
        out.layout = layout->layout;
        out.niche_payload_layout = layout->layout;
        out.payload_size = layout->layout.size;
        return out;
      }
    }
  }

  std::uint64_t max_size = 0;
  std::uint64_t max_align = 1;
  for (const auto& state : decl.states) {
    const auto layout = StatePayloadLayout(ctx, state);
    if (!layout.has_value()) {
      return std::nullopt;
    }
    max_size = std::max(max_size, layout->layout.size);
    max_align = std::max(max_align, layout->layout.align);
  }

  const auto disc = DiscTypeLayout(decl.states.size() - 1);
  if (!disc.has_value()) {
    return std::nullopt;
  }
  const std::uint64_t align = std::max(disc->align, max_align);
  const std::uint64_t size = AlignUp(disc->size + max_size, align);

  SPEC_RULE("Layout-Modal-Tagged");
  ModalLayout out;
  out.niche = false;
  out.layout = Layout{size, align};
  out.payload_size = max_size;
  out.disc_type = DiscTypeName(decl.states.size() - 1);
  return out;
}

}  // namespace cursive0::codegen
