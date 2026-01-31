#include "cursive0/04_codegen/layout/layout.h"

#include <algorithm>

#include "cursive0/00_core/assert_spec.h"
#include "cursive0/03_analysis/modal/modal_widen.h"
#include "cursive0/03_analysis/resolve/scopes.h"
#include "cursive0/03_analysis/types/type_equiv.h"
#include "cursive0/03_analysis/types/types.h"

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

bool IsUnitType(const cursive0::analysis::TypeRef& type) {
  if (!type) {
    return false;
  }
  if (const auto* prim = std::get_if<cursive0::analysis::TypePrim>(&type->node)) {
    return prim->name == "()";
  }
  return false;
}

std::uint64_t NicheCount(const cursive0::analysis::ScopeContext& ctx,
                         const cursive0::analysis::TypeRef& type) {
  if (!type) {
    return 0;
  }
  if (const auto* perm = std::get_if<cursive0::analysis::TypePerm>(&type->node)) {
    return NicheCount(ctx, perm->base);
  }
  if (const auto* ptr = std::get_if<cursive0::analysis::TypePtr>(&type->node)) {
    if (ptr->state == cursive0::analysis::PtrState::Valid) {
      return 1;
    }
    return 0;
  }
  if (const auto* path =
          std::get_if<cursive0::analysis::TypePathType>(&type->node)) {
    cursive0::syntax::Path syntax_path;
    syntax_path.reserve(path->path.size());
    for (const auto& comp : path->path) {
      syntax_path.push_back(comp);
    }
    const auto it =
        ctx.sigma.types.find(cursive0::analysis::PathKeyOf(syntax_path));
    if (it != ctx.sigma.types.end()) {
      if (const auto* alias =
              std::get_if<cursive0::syntax::TypeAliasDecl>(&it->second)) {
        const auto lowered = LowerTypeForLayout(ctx, alias->type);
        if (!lowered.has_value()) {
          return 0;
        }
        return NicheCount(ctx, *lowered);
      }
    }
  }
  return 0;
}

}  // namespace

std::optional<UnionLayout> UnionLayoutOf(
    const cursive0::analysis::ScopeContext& ctx,
    const cursive0::analysis::TypeUnion& uni) {
  if (uni.members.empty()) {
    return std::nullopt;
  }

  const auto member_list = cursive0::analysis::SortUnionMembers(uni.members);

  const std::uint64_t required = member_list.empty()
                                    ? 0ull
                                    : static_cast<std::uint64_t>(member_list.size() - 1);

  std::optional<std::size_t> payload_index;
  std::uint64_t niche_count = 0;

  for (std::size_t i = 0; i < member_list.size(); ++i) {
    if (IsUnitType(member_list[i])) {
      continue;
    }
    const auto count = NicheCount(ctx, member_list[i]);
    if (count == 0) {
      continue;
    }
    if (payload_index.has_value()) {
      payload_index.reset();
      break;
    }
    payload_index = i;
    niche_count = count;
  }

  bool niche_applies = payload_index.has_value();
  if (niche_applies && payload_index.has_value()) {
    for (std::size_t i = 0; i < member_list.size(); ++i) {
      if (i == *payload_index) {
        continue;
      }
      if (!IsUnitType(member_list[i])) {
        niche_applies = false;
        break;
      }
    }
    if (niche_count < required) {
      niche_applies = false;
    }
  }

  UnionLayout out;
  out.member_list = member_list;

  if (niche_applies && payload_index.has_value()) {
    SPEC_RULE("Layout-Union-Niche");
    const auto payload_layout = LayoutOf(ctx, member_list[*payload_index]);
    if (!payload_layout.has_value()) {
      return std::nullopt;
    }
    out.niche = true;
    out.layout = *payload_layout;
    out.niche_payload_layout = *payload_layout;
    out.payload_size = payload_layout->size;
    out.payload_align = payload_layout->align;
    return out;
  }

  SPEC_RULE("Layout-Union-Tagged");
  std::uint64_t payload_size = 0;
  std::uint64_t payload_align = 1;
  for (const auto& member : member_list) {
    const auto size = SizeOf(ctx, member);
    const auto align = AlignOf(ctx, member);
    if (!size.has_value() || !align.has_value()) {
      return std::nullopt;
    }
    payload_size = std::max(payload_size, *size);
    payload_align = std::max(payload_align, *align);
  }

  const auto disc_layout = DiscTypeLayout(member_list.size() - 1);
  if (!disc_layout.has_value()) {
    return std::nullopt;
  }

  const std::uint64_t align = std::max(disc_layout->align, payload_align);
  const std::uint64_t size = AlignUp(disc_layout->size + payload_size, align);
  out.niche = false;
  out.layout = Layout{size, align};
  out.payload_size = payload_size;
  out.payload_align = payload_align;
  out.disc_type = DiscTypeName(member_list.size() - 1);
  return out;
}

}  // namespace cursive0::codegen
