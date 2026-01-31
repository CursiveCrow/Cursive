// =================================================================
// File: 03_analysis/types/type_layout.cpp
// Construct: Type Layout Computation
// Spec Section: 5.2.12
// =================================================================
#include "cursive0/03_analysis/types/type_layout.h"

#include <algorithm>
#include <string>
#include <string_view>
#include <variant>

#include "cursive0/03_analysis/composite/enums.h"
#include "cursive0/03_analysis/modal/modal.h"
#include "cursive0/03_analysis/modal/modal_widen.h"
#include "cursive0/03_analysis/resolve/scopes.h"
#include "cursive0/03_analysis/types/type_lower.h"
#include "cursive0/03_analysis/types/types.h"

namespace cursive0::analysis {

// Forward declarations for internal functions (not exported in header)
static std::optional<std::pair<std::uint64_t, std::uint64_t>> LayoutOf(
    const ScopeContext& ctx, const TypeRef& type);
static std::optional<std::pair<std::uint64_t, std::uint64_t>> LayoutOfSeq(
    const ScopeContext& ctx, const std::vector<TypeRef>& elems);

std::optional<std::uint64_t> PrimSize(std::string_view name) {
  if (name == "i8" || name == "u8") return 1;
  if (name == "i16" || name == "u16") return 2;
  if (name == "i32" || name == "u32") return 4;
  if (name == "i64" || name == "u64") return 8;
  if (name == "i128" || name == "u128") return 16;
  if (name == "f16") return 2;
  if (name == "f32") return 4;
  if (name == "f64") return 8;
  if (name == "bool") return 1;
  if (name == "char") return 4;
  if (name == "usize" || name == "isize") return kPtrSize;
  if (name == "()" || name == "!") return 0;
  return std::nullopt;
}

std::optional<std::uint64_t> PrimAlign(std::string_view name) {
  if (name == "i8" || name == "u8") return 1;
  if (name == "i16" || name == "u16") return 2;
  if (name == "i32" || name == "u32") return 4;
  if (name == "i64" || name == "u64") return 8;
  if (name == "i128" || name == "u128") return 16;
  if (name == "f16") return 2;
  if (name == "f32") return 4;
  if (name == "f64") return 8;
  if (name == "bool") return 1;
  if (name == "char") return 4;
  if (name == "usize" || name == "isize") return kPtrAlign;
  if (name == "()" || name == "!") return 1;
  return std::nullopt;
}

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

std::optional<std::string_view> DiscTypeName(std::uint64_t max_disc) {
  if (max_disc <= 0xFFu) {
    return "u8";
  }
  if (max_disc <= 0xFFFFu) {
    return "u16";
  }
  if (max_disc <= 0xFFFFFFFFu) {
    return "u32";
  }
  return "u64";
}

std::optional<std::pair<std::uint64_t, std::uint64_t>> DiscTypeLayout(
    std::uint64_t max_disc) {
  const auto name = DiscTypeName(max_disc);
  if (!name.has_value()) {
    return std::nullopt;
  }
  const auto size = PrimSize(*name);
  const auto align = PrimAlign(*name);
  if (!size.has_value() || !align.has_value()) {
    return std::nullopt;
  }
  return std::make_pair(*size, *align);
}

namespace {

static auto ModalLayout(std::uint64_t managed_size, std::uint64_t view_size)
    -> std::optional<std::pair<std::uint64_t, std::uint64_t>> {
  const auto disc = DiscTypeLayout(1);
  if (!disc.has_value()) {
    return std::nullopt;
  }
  const std::uint64_t max_size = std::max(managed_size, view_size);
  const std::uint64_t max_align = kPtrAlign;
  const std::uint64_t align = std::max(disc->second, max_align);
  const std::uint64_t size = AlignUp(disc->first + max_size, align);
  return std::make_pair(size, align);
}

static auto StateLayout(const ScopeContext& ctx, const syntax::StateBlock& state)
    -> std::optional<std::pair<std::uint64_t, std::uint64_t>> {
  std::vector<TypeRef> fields;
  for (const auto& member : state.members) {
    if (const auto* field = std::get_if<syntax::StateFieldDecl>(&member)) {
      const auto lowered = LowerType(ctx, field->type);
      if (!lowered.ok || !lowered.type) {
        return std::nullopt;
      }
      fields.push_back(lowered.type);
    }
  }
  return LayoutOfSeq(ctx, fields);
}

static auto ModalStateLayout(const ScopeContext& ctx,
                             const syntax::ModalDecl& decl,
                             std::string_view state_name)
    -> std::optional<std::pair<std::uint64_t, std::uint64_t>> {
  for (const auto& state : decl.states) {
    if (IdEq(state.name, state_name)) {
      return StateLayout(ctx, state);
    }
  }
  return std::nullopt;
}

static auto ModalDeclLayout(const ScopeContext& ctx,
                            const syntax::ModalDecl& decl)
    -> std::optional<std::pair<std::uint64_t, std::uint64_t>> {
  if (decl.states.empty()) {
    return std::nullopt;
  }
  const auto payload_state = PayloadState(ctx, decl);
  if (payload_state.has_value()) {
    return ModalStateLayout(ctx, decl, *payload_state);
  }
  std::uint64_t max_size = 0;
  std::uint64_t max_align = 1;
  for (const auto& state : decl.states) {
    const auto layout = StateLayout(ctx, state);
    if (!layout.has_value()) {
      return std::nullopt;
    }
    max_size = std::max(max_size, layout->first);
    max_align = std::max(max_align, layout->second);
  }
  const auto disc = DiscTypeLayout(decl.states.size() - 1);
  if (!disc.has_value()) {
    return std::nullopt;
  }
  const std::uint64_t align = std::max(disc->second, max_align);
  const std::uint64_t size = AlignUp(disc->first + max_size, align);
  return std::make_pair(size, align);
}

static auto EnumDeclLayout(const ScopeContext& ctx, const syntax::EnumDecl& decl)
    -> std::optional<std::pair<std::uint64_t, std::uint64_t>> {
  if (decl.variants.empty()) {
    return std::nullopt;
  }
  const auto discs = EnumDiscriminants(decl);
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
    std::optional<std::pair<std::uint64_t, std::uint64_t>> layout;
    if (!variant.payload_opt.has_value()) {
      layout = std::make_pair(0ull, 1ull);
    } else if (const auto* tuple =
                   std::get_if<syntax::VariantPayloadTuple>(&*variant.payload_opt)) {
      std::vector<TypeRef> elements;
      elements.reserve(tuple->elements.size());
      for (const auto& elem : tuple->elements) {
        const auto lowered = LowerType(ctx, elem);
        if (!lowered.ok) {
          return std::nullopt;
        }
        elements.push_back(lowered.type);
      }
      layout = LayoutOfSeq(ctx, elements);
    } else if (const auto* rec =
                   std::get_if<syntax::VariantPayloadRecord>(&*variant.payload_opt)) {
      std::vector<TypeRef> fields;
      fields.reserve(rec->fields.size());
      for (const auto& field : rec->fields) {
        const auto lowered = LowerType(ctx, field.type);
        if (!lowered.ok) {
          return std::nullopt;
        }
        fields.push_back(lowered.type);
      }
      layout = LayoutOfSeq(ctx, fields);
    }
    if (!layout.has_value()) {
      return std::nullopt;
    }
    payload_size = std::max(payload_size, layout->first);
    payload_align = std::max(payload_align, layout->second);
  }

  const std::uint64_t align = std::max(disc_layout->second, payload_align);
  const std::uint64_t size = AlignUp(disc_layout->first + payload_size, align);
  return std::make_pair(size, align);
}

static auto ComputeAsyncLayout(const ScopeContext& ctx,
                               const std::vector<TypeRef>& async_args)
    -> std::optional<std::pair<std::uint64_t, std::uint64_t>> {
  std::uint64_t max_payload_size = 0;
  std::uint64_t max_payload_align = 1;

  auto add_payload_layout = [&](const std::optional<std::pair<std::uint64_t, std::uint64_t>>& layout_opt) {
    if (!layout_opt.has_value() || layout_opt->first == 0) {
      return;
    }
    max_payload_size = std::max(max_payload_size, layout_opt->first);
    max_payload_align = std::max(max_payload_align, layout_opt->second);
  };

  auto record_layout = [&](const TypeRef& a, const TypeRef& b)
      -> std::optional<std::pair<std::uint64_t, std::uint64_t>> {
    const auto la = LayoutOf(ctx, a);
    const auto lb = LayoutOf(ctx, b);
    if (!la.has_value() || !lb.has_value()) {
      return std::nullopt;
    }
    const std::uint64_t align = std::max(la->second, lb->second);
    const std::uint64_t b_off = AlignUp(la->first, lb->second);
    const std::uint64_t size = AlignUp(b_off + lb->first, align);
    return std::make_pair(size, align);
  };

  const auto out_type = async_args.size() > 0 ? async_args[0] : MakeTypeTuple({});
  const auto frame_ptr = MakeTypePtr(MakeTypePrim("u8"), PtrState::Valid);
  const auto suspended_layout = record_layout(out_type, frame_ptr);
  if (!suspended_layout.has_value()) {
    return std::nullopt;
  }
  add_payload_layout(suspended_layout);

  if (async_args.size() > 2 && async_args[2]) {
    if (const auto* prim = std::get_if<TypePrim>(&async_args[2]->node)) {
      if (prim->name != "!") {
        add_payload_layout(LayoutOf(ctx, async_args[2]));
      }
    } else if (const auto* tup = std::get_if<TypeTuple>(&async_args[2]->node)) {
      if (!tup->elements.empty()) {
        add_payload_layout(LayoutOf(ctx, async_args[2]));
      }
    } else {
      add_payload_layout(LayoutOf(ctx, async_args[2]));
    }
  }

  if (async_args.size() > 3 && async_args[3]) {
    if (const auto* prim = std::get_if<TypePrim>(&async_args[3]->node)) {
      if (prim->name != "!") {
        add_payload_layout(LayoutOf(ctx, async_args[3]));
      }
    } else if (const auto* tup = std::get_if<TypeTuple>(&async_args[3]->node)) {
      if (!tup->elements.empty()) {
        add_payload_layout(LayoutOf(ctx, async_args[3]));
      }
    } else {
      add_payload_layout(LayoutOf(ctx, async_args[3]));
    }
  }

  const std::uint64_t disc_size = 1;
  const std::uint64_t disc_align = 1;
  const std::uint64_t align = std::max(disc_align, max_payload_align);
  const std::uint64_t size = AlignUp(disc_size + max_payload_size, align);
  return std::make_pair(size, align);
}

}  // namespace

static std::optional<std::pair<std::uint64_t, std::uint64_t>> LayoutOfSeq(
    const ScopeContext& ctx, const std::vector<TypeRef>& elems) {
  if (elems.empty()) {
    return std::make_pair(0ull, 1ull);
  }
  std::uint64_t offset = 0;
  std::uint64_t max_align = 1;
  for (const auto& elem : elems) {
    const auto layout = LayoutOf(ctx, elem);
    if (!layout.has_value()) {
      return std::nullopt;
    }
    const auto size = layout->first;
    const auto align = layout->second;
    max_align = std::max(max_align, align);
    offset = AlignUp(offset, align);
    offset += size;
  }
  const auto total = AlignUp(offset, max_align);
  return std::make_pair(total, max_align);
}

static std::optional<std::pair<std::uint64_t, std::uint64_t>> LayoutOf(
    const ScopeContext& ctx, const TypeRef& type) {
  if (!type) {
    return std::nullopt;
  }
  if (const auto* perm = std::get_if<TypePerm>(&type->node)) {
    return LayoutOf(ctx, perm->base);
  }
  if (const auto* refine = std::get_if<TypeRefine>(&type->node)) {
    return LayoutOf(ctx, refine->base);
  }

  if (const auto* prim = std::get_if<TypePrim>(&type->node)) {
    const auto size = PrimSize(prim->name);
    const auto align = PrimAlign(prim->name);
    if (!size.has_value() || !align.has_value()) {
      return std::nullopt;
    }
    return std::make_pair(*size, *align);
  }
  if (std::holds_alternative<TypePtr>(type->node) ||
      std::holds_alternative<TypeRawPtr>(type->node) ||
      std::holds_alternative<TypeFunc>(type->node)) {
    return std::make_pair(kPtrSize, kPtrAlign);
  }
  if (std::holds_alternative<TypeDynamic>(type->node)) {
    return std::make_pair(2 * kPtrSize, kPtrAlign);
  }
  if (const auto* slice = std::get_if<TypeSlice>(&type->node)) {
    (void)slice;
    return std::make_pair(2 * kPtrSize, kPtrAlign);
  }
  if (const auto* str = std::get_if<TypeString>(&type->node)) {
    if (!str->state.has_value()) {
      return ModalLayout(3 * kPtrSize, 2 * kPtrSize);
    }
    if (*str->state == StringState::Managed) {
      return std::make_pair(3 * kPtrSize, kPtrAlign);
    }
    return std::make_pair(2 * kPtrSize, kPtrAlign);
  }
  if (const auto* bytes = std::get_if<TypeBytes>(&type->node)) {
    if (!bytes->state.has_value()) {
      return ModalLayout(3 * kPtrSize, 2 * kPtrSize);
    }
    if (*bytes->state == BytesState::Managed) {
      return std::make_pair(3 * kPtrSize, kPtrAlign);
    }
    return std::make_pair(2 * kPtrSize, kPtrAlign);
  }
  if (const auto* range = std::get_if<TypeRange>(&type->node)) {
    (void)range;
    std::vector<TypeRef> fields;
    fields.push_back(MakeTypePrim("u8"));
    fields.push_back(MakeTypePrim("usize"));
    fields.push_back(MakeTypePrim("usize"));
    return LayoutOfSeq(ctx, fields);
  }
  if (const auto* array = std::get_if<TypeArray>(&type->node)) {
    const auto elem_layout = LayoutOf(ctx, array->element);
    if (!elem_layout.has_value()) {
      return std::nullopt;
    }
    const auto size = elem_layout->first * array->length;
    return std::make_pair(size, elem_layout->second);
  }
  if (const auto* tuple = std::get_if<TypeTuple>(&type->node)) {
    return LayoutOfSeq(ctx, tuple->elements);
  }
  if (const auto* uni = std::get_if<TypeUnion>(&type->node)) {
    if (uni->members.empty()) {
      return std::nullopt;
    }
    std::uint64_t payload_size = 0;
    std::uint64_t payload_align = 1;
    for (const auto& member : uni->members) {
      const auto layout = LayoutOf(ctx, member);
      if (!layout.has_value()) {
        return std::nullopt;
      }
      payload_size = std::max(payload_size, layout->first);
      payload_align = std::max(payload_align, layout->second);
    }
    const auto disc_layout = DiscTypeLayout(uni->members.size() - 1);
    if (!disc_layout.has_value()) {
      return std::nullopt;
    }
    const std::uint64_t align = std::max(disc_layout->second, payload_align);
    const std::uint64_t size = AlignUp(disc_layout->first + payload_size, align);
    return std::make_pair(size, align);
  }
  if (const auto* modal = std::get_if<TypeModalState>(&type->node)) {
    syntax::Path syntax_path;
    syntax_path.reserve(modal->path.size());
    for (const auto& comp : modal->path) {
      syntax_path.push_back(comp);
    }
    const auto it = ctx.sigma.types.find(PathKeyOf(syntax_path));
    if (it == ctx.sigma.types.end()) {
      return std::nullopt;
    }
    const auto* decl = std::get_if<syntax::ModalDecl>(&it->second);
    if (!decl) {
      return std::nullopt;
    }
    return ModalStateLayout(ctx, *decl, modal->state);
  }
  if (const auto* path = std::get_if<TypePathType>(&type->node)) {
    const std::string& type_name = path->path.empty() ? "" : path->path.back();

    // Check for Async<Out, In, Result, E>
    if (IdEq(type_name, "Async") && !path->generic_args.empty()) {
      return ComputeAsyncLayout(ctx, path->generic_args);
    }
    // Future<T; E = !> = Async<(), (), T, E>
    if (IdEq(type_name, "Future") && !path->generic_args.empty()) {
      auto unit_type = MakeTypeTuple({});
      auto never_type = MakeTypePrim("!");
      std::vector<TypeRef> async_args = {
          unit_type,
          unit_type,
          path->generic_args[0],
          path->generic_args.size() > 1 ? path->generic_args[1] : never_type
      };
      return ComputeAsyncLayout(ctx, async_args);
    }
    // Sequence<T> = Async<T, (), (), !>
    if (IdEq(type_name, "Sequence") && !path->generic_args.empty()) {
      auto unit_type = MakeTypeTuple({});
      auto never_type = MakeTypePrim("!");
      std::vector<TypeRef> async_args = {
          path->generic_args[0],
          unit_type,
          unit_type,
          never_type
      };
      return ComputeAsyncLayout(ctx, async_args);
    }
    // Stream<T; E> = Async<T, (), (), E>
    if (IdEq(type_name, "Stream") && path->generic_args.size() >= 2) {
      auto unit_type = MakeTypeTuple({});
      std::vector<TypeRef> async_args = {
          path->generic_args[0],
          unit_type,
          unit_type,
          path->generic_args[1]
      };
      return ComputeAsyncLayout(ctx, async_args);
    }
    // Pipe<In; Out> = Async<Out, In, (), !>
    if (IdEq(type_name, "Pipe") && path->generic_args.size() >= 2) {
      auto unit_type = MakeTypeTuple({});
      auto never_type = MakeTypePrim("!");
      std::vector<TypeRef> async_args = {
          path->generic_args[1],
          path->generic_args[0],
          unit_type,
          never_type
      };
      return ComputeAsyncLayout(ctx, async_args);
    }
    // Exchange<T> = Async<T, T, T, !>
    if (IdEq(type_name, "Exchange") && !path->generic_args.empty()) {
      auto never_type = MakeTypePrim("!");
      std::vector<TypeRef> async_args = {
          path->generic_args[0],
          path->generic_args[0],
          path->generic_args[0],
          never_type
      };
      return ComputeAsyncLayout(ctx, async_args);
    }

    syntax::Path syntax_path;
    syntax_path.reserve(path->path.size());
    for (const auto& comp : path->path) {
      syntax_path.push_back(comp);
    }
    const auto it = ctx.sigma.types.find(PathKeyOf(syntax_path));
    if (it == ctx.sigma.types.end()) {
      return std::nullopt;
    }
    if (const auto* record = std::get_if<syntax::RecordDecl>(&it->second)) {
      std::vector<TypeRef> fields;
      for (const auto& member : record->members) {
        if (const auto* field = std::get_if<syntax::FieldDecl>(&member)) {
          const auto lowered = LowerType(ctx, field->type);
          if (!lowered.ok || !lowered.type) {
            return std::nullopt;
          }
          fields.push_back(lowered.type);
        }
      }
      return LayoutOfSeq(ctx, fields);
    }
    if (const auto* enum_decl = std::get_if<syntax::EnumDecl>(&it->second)) {
      return EnumDeclLayout(ctx, *enum_decl);
    }
    if (const auto* modal_decl = std::get_if<syntax::ModalDecl>(&it->second)) {
      return ModalDeclLayout(ctx, *modal_decl);
    }
    if (const auto* alias = std::get_if<syntax::TypeAliasDecl>(&it->second)) {
      const auto lowered = LowerType(ctx, alias->type);
      if (!lowered.ok) {
        return std::nullopt;
      }
      return LayoutOf(ctx, lowered.type);
    }
  }
  return std::nullopt;
}

std::optional<std::uint64_t> SizeOf(const ScopeContext& ctx,
                                    const TypeRef& type) {
  const auto layout = LayoutOf(ctx, type);
  if (!layout.has_value()) {
    return std::nullopt;
  }
  return layout->first;
}

std::optional<std::uint64_t> AlignOf(const ScopeContext& ctx,
                                     const TypeRef& type) {
  const auto layout = LayoutOf(ctx, type);
  if (!layout.has_value()) {
    return std::nullopt;
  }
  return layout->second;
}

}  // namespace cursive0::analysis
