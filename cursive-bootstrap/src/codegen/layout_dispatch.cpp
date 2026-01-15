#include "cursive0/codegen/layout.h"

#include <algorithm>
#include <type_traits>

#include "cursive0/core/assert_spec.h"
#include "cursive0/sema/modal_widen.h"
#include "cursive0/sema/scopes.h"
#include "cursive0/sema/type_equiv.h"

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

std::optional<cursive0::sema::Permission> LowerPermission(
    cursive0::syntax::TypePerm perm) {
  switch (perm) {
    case cursive0::syntax::TypePerm::Const:
      return cursive0::sema::Permission::Const;
    case cursive0::syntax::TypePerm::Unique:
      return cursive0::sema::Permission::Unique;
    case cursive0::syntax::TypePerm::Shared:
      return cursive0::sema::Permission::Shared;
  }
  return std::nullopt;
}

std::optional<cursive0::sema::ParamMode> LowerParamMode(
    const std::optional<cursive0::syntax::ParamMode>& mode) {
  if (!mode.has_value()) {
    return std::nullopt;
  }
  return cursive0::sema::ParamMode::Move;
}

cursive0::sema::RawPtrQual LowerRawPtrQual(cursive0::syntax::RawPtrQual qual) {
  return qual == cursive0::syntax::RawPtrQual::Imm
             ? cursive0::sema::RawPtrQual::Imm
             : cursive0::sema::RawPtrQual::Mut;
}

std::optional<cursive0::sema::StringState> LowerStringState(
    const std::optional<cursive0::syntax::StringState>& state) {
  if (!state.has_value()) {
    return std::nullopt;
  }
  return *state == cursive0::syntax::StringState::Managed
             ? cursive0::sema::StringState::Managed
             : cursive0::sema::StringState::View;
}

std::optional<cursive0::sema::BytesState> LowerBytesState(
    const std::optional<cursive0::syntax::BytesState>& state) {
  if (!state.has_value()) {
    return std::nullopt;
  }
  return *state == cursive0::syntax::BytesState::Managed
             ? cursive0::sema::BytesState::Managed
             : cursive0::sema::BytesState::View;
}

std::optional<cursive0::sema::PtrState> LowerPtrState(
    const std::optional<cursive0::syntax::PtrState>& state) {
  if (!state.has_value()) {
    return std::nullopt;
  }
  switch (*state) {
    case cursive0::syntax::PtrState::Valid:
      return cursive0::sema::PtrState::Valid;
    case cursive0::syntax::PtrState::Null:
      return cursive0::sema::PtrState::Null;
    case cursive0::syntax::PtrState::Expired:
      return cursive0::sema::PtrState::Expired;
  }
  return std::nullopt;
}

std::optional<Layout> ModalStringBytesLayout(std::uint64_t managed_size,
                                             std::uint64_t view_size) {
  const auto disc = DiscTypeLayout(1);
  if (!disc.has_value()) {
    return std::nullopt;
  }
  const std::uint64_t max_size = std::max(managed_size, view_size);
  const std::uint64_t align = std::max(disc->align, kPtrAlign);
  const std::uint64_t size = AlignUp(disc->size + max_size, align);
  return Layout{size, align};
}

}  // namespace

std::optional<cursive0::sema::TypeRef> LowerTypeForLayout(
    const cursive0::sema::ScopeContext& ctx,
    const std::shared_ptr<cursive0::syntax::Type>& type) {
  if (!type) {
    return std::nullopt;
  }
  return std::visit(
      [&](const auto& node) -> std::optional<cursive0::sema::TypeRef> {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, cursive0::syntax::TypePrim>) {
          return cursive0::sema::MakeTypePrim(node.name);
        } else if constexpr (std::is_same_v<T, cursive0::syntax::TypePermType>) {
          const auto base = LowerTypeForLayout(ctx, node.base);
          const auto perm = LowerPermission(node.perm);
          if (!base.has_value() || !perm.has_value()) {
            return std::nullopt;
          }
          return cursive0::sema::MakeTypePerm(*perm, *base);
        } else if constexpr (std::is_same_v<T, cursive0::syntax::TypeUnion>) {
          std::vector<cursive0::sema::TypeRef> members;
          members.reserve(node.types.size());
          for (const auto& elem : node.types) {
            const auto lowered = LowerTypeForLayout(ctx, elem);
            if (!lowered.has_value()) {
              return std::nullopt;
            }
            members.push_back(*lowered);
          }
          return cursive0::sema::MakeTypeUnion(std::move(members));
        } else if constexpr (std::is_same_v<T, cursive0::syntax::TypeFunc>) {
          std::vector<cursive0::sema::TypeFuncParam> params;
          params.reserve(node.params.size());
          for (const auto& param : node.params) {
            const auto lowered = LowerTypeForLayout(ctx, param.type);
            if (!lowered.has_value()) {
              return std::nullopt;
            }
            params.push_back(cursive0::sema::TypeFuncParam{
                LowerParamMode(param.mode), *lowered});
          }
          const auto ret = LowerTypeForLayout(ctx, node.ret);
          if (!ret.has_value()) {
            return std::nullopt;
          }
          return cursive0::sema::MakeTypeFunc(std::move(params), *ret);
        } else if constexpr (std::is_same_v<T, cursive0::syntax::TypeTuple>) {
          std::vector<cursive0::sema::TypeRef> elements;
          elements.reserve(node.elements.size());
          for (const auto& elem : node.elements) {
            const auto lowered = LowerTypeForLayout(ctx, elem);
            if (!lowered.has_value()) {
              return std::nullopt;
            }
            elements.push_back(*lowered);
          }
          return cursive0::sema::MakeTypeTuple(std::move(elements));
        } else if constexpr (std::is_same_v<T, cursive0::syntax::TypeArray>) {
          const auto elem = LowerTypeForLayout(ctx, node.element);
          if (!elem.has_value()) {
            return std::nullopt;
          }
          const auto len = cursive0::sema::ConstLen(ctx, node.length);
          if (!len.ok || !len.value.has_value()) {
            return std::nullopt;
          }
          return cursive0::sema::MakeTypeArray(*elem, *len.value);
        } else if constexpr (std::is_same_v<T, cursive0::syntax::TypeSlice>) {
          const auto elem = LowerTypeForLayout(ctx, node.element);
          if (!elem.has_value()) {
            return std::nullopt;
          }
          return cursive0::sema::MakeTypeSlice(*elem);
        } else if constexpr (std::is_same_v<T, cursive0::syntax::TypePtr>) {
          const auto elem = LowerTypeForLayout(ctx, node.element);
          if (!elem.has_value()) {
            return std::nullopt;
          }
          return cursive0::sema::MakeTypePtr(*elem, LowerPtrState(node.state));
        } else if constexpr (std::is_same_v<T, cursive0::syntax::TypeRawPtr>) {
          const auto elem = LowerTypeForLayout(ctx, node.element);
          if (!elem.has_value()) {
            return std::nullopt;
          }
          return cursive0::sema::MakeTypeRawPtr(LowerRawPtrQual(node.qual),
                                                *elem);
        } else if constexpr (std::is_same_v<T, cursive0::syntax::TypeString>) {
          return cursive0::sema::MakeTypeString(LowerStringState(node.state));
        } else if constexpr (std::is_same_v<T, cursive0::syntax::TypeBytes>) {
          return cursive0::sema::MakeTypeBytes(LowerBytesState(node.state));
        } else if constexpr (std::is_same_v<T, cursive0::syntax::TypeDynamic>) {
          return cursive0::sema::MakeTypeDynamic(node.path);
        } else if constexpr (std::is_same_v<T,
                                           cursive0::syntax::TypeModalState>) {
          return cursive0::sema::MakeTypeModalState(node.path, node.state);
        } else if constexpr (std::is_same_v<T,
                                           cursive0::syntax::TypePathType>) {
          return cursive0::sema::MakeTypePath(node.path);
        } else {
          return std::nullopt;
        }
      },
      type->node);
}

std::optional<Layout> LayoutOf(const cursive0::sema::ScopeContext& ctx,
                               const cursive0::sema::TypeRef& type) {
  if (!type) {
    return std::nullopt;
  }

  if (const auto* perm = std::get_if<cursive0::sema::TypePerm>(&type->node)) {
    SPEC_RULE("Layout-Perm");
    return LayoutOf(ctx, perm->base);
  }

  if (const auto* prim = std::get_if<cursive0::sema::TypePrim>(&type->node)) {
    SPEC_RULE("Layout-Prim");
    const auto size = PrimSize(prim->name);
    const auto align = PrimAlign(prim->name);
    if (!size.has_value() || !align.has_value()) {
      return std::nullopt;
    }
    return Layout{*size, *align};
  }

  if (std::holds_alternative<cursive0::sema::TypePtr>(type->node)) {
    SPEC_RULE("Layout-Ptr");
    return Layout{kPtrSize, kPtrAlign};
  }
  if (std::holds_alternative<cursive0::sema::TypeRawPtr>(type->node)) {
    SPEC_RULE("Layout-RawPtr");
    return Layout{kPtrSize, kPtrAlign};
  }
  if (std::holds_alternative<cursive0::sema::TypeFunc>(type->node)) {
    SPEC_RULE("Layout-Func");
    return Layout{kPtrSize, kPtrAlign};
  }

  if (std::holds_alternative<cursive0::sema::TypeDynamic>(type->node)) {
    const auto dyn = DynLayoutOf();
    SPEC_RULE("Layout-DynamicClass");
    return dyn.layout;
  }

  if (std::holds_alternative<cursive0::sema::TypeSlice>(type->node)) {
    SPEC_RULE("Layout-Slice");
    return Layout{2 * kPtrSize, kPtrAlign};
  }

  if (const auto* str = std::get_if<cursive0::sema::TypeString>(&type->node)) {
    if (!str->state.has_value()) {
      const auto modal = ModalStringBytesLayout(3 * kPtrSize, 2 * kPtrSize);
      return modal;
    }
    if (*str->state == cursive0::sema::StringState::Managed) {
      SPEC_RULE("Layout-String-Managed");
      return Layout{3 * kPtrSize, kPtrAlign};
    }
    SPEC_RULE("Layout-String-View");
    return Layout{2 * kPtrSize, kPtrAlign};
  }

  if (const auto* bytes =
          std::get_if<cursive0::sema::TypeBytes>(&type->node)) {
    if (!bytes->state.has_value()) {
      const auto modal = ModalStringBytesLayout(3 * kPtrSize, 2 * kPtrSize);
      return modal;
    }
    if (*bytes->state == cursive0::sema::BytesState::Managed) {
      SPEC_RULE("Layout-Bytes-Managed");
      return Layout{3 * kPtrSize, kPtrAlign};
    }
    SPEC_RULE("Layout-Bytes-View");
    return Layout{2 * kPtrSize, kPtrAlign};
  }

  if (std::holds_alternative<cursive0::sema::TypeRange>(type->node)) {
    SPEC_RULE("Layout-Range-SizeAlign");
    const auto layout = RangeLayoutOf(ctx);
    if (!layout.has_value()) {
      return std::nullopt;
    }
    return layout->layout;
  }

  if (const auto* array = std::get_if<cursive0::sema::TypeArray>(&type->node)) {
    SPEC_RULE("Layout-Array");
    const auto elem_layout = LayoutOf(ctx, array->element);
    if (!elem_layout.has_value()) {
      return std::nullopt;
    }
    const auto size = elem_layout->size * array->length;
    return Layout{size, elem_layout->align};
  }

  if (const auto* tuple = std::get_if<cursive0::sema::TypeTuple>(&type->node)) {
    if (tuple->elements.empty()) {
      SPEC_RULE("Layout-Tuple-Empty");
    } else {
      SPEC_RULE("Layout-Tuple-Cons");
    }
    const auto layout = RecordLayoutOf(ctx, tuple->elements);
    if (!layout.has_value()) {
      return std::nullopt;
    }
    SPEC_RULE("Layout-Tuple");
    return layout->layout;
  }

  if (const auto* uni = std::get_if<cursive0::sema::TypeUnion>(&type->node)) {
    const auto layout = UnionLayoutOf(ctx, *uni);
    if (!layout.has_value()) {
      return std::nullopt;
    }
    SPEC_RULE("Layout-Union");
    return layout->layout;
  }

  if (const auto* modal =
          std::get_if<cursive0::sema::TypeModalState>(&type->node)) {
    SPEC_RULE("Layout-ModalState");
    cursive0::syntax::Path syntax_path;
    syntax_path.reserve(modal->path.size());
    for (const auto& comp : modal->path) {
      syntax_path.push_back(comp);
    }
    const auto it = ctx.sigma.types.find(cursive0::sema::PathKeyOf(syntax_path));
    if (it == ctx.sigma.types.end()) {
      return std::nullopt;
    }
    const auto* decl = std::get_if<cursive0::syntax::ModalDecl>(&it->second);
    if (!decl) {
      return std::nullopt;
    }
    for (const auto& state : decl->states) {
      if (cursive0::sema::IdEq(state.name, modal->state)) {
        std::vector<cursive0::sema::TypeRef> fields;
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
        const auto layout = RecordLayoutOf(ctx, fields);
        if (!layout.has_value()) {
          return std::nullopt;
        }
        return layout->layout;
      }
    }
    return std::nullopt;
  }

  if (const auto* path = std::get_if<cursive0::sema::TypePathType>(&type->node)) {
    cursive0::syntax::Path syntax_path;
    syntax_path.reserve(path->path.size());
    for (const auto& comp : path->path) {
      syntax_path.push_back(comp);
    }
    const auto it = ctx.sigma.types.find(cursive0::sema::PathKeyOf(syntax_path));
    if (it == ctx.sigma.types.end()) {
      return std::nullopt;
    }
    if (const auto* record = std::get_if<cursive0::syntax::RecordDecl>(&it->second)) {
      std::vector<cursive0::sema::TypeRef> fields;
      for (const auto& member : record->members) {
        if (const auto* field =
                std::get_if<cursive0::syntax::FieldDecl>(&member)) {
          const auto lowered = LowerTypeForLayout(ctx, field->type);
          if (!lowered.has_value()) {
            return std::nullopt;
          }
          fields.push_back(*lowered);
        }
      }
      const auto layout = RecordLayoutOf(ctx, fields);
      if (!layout.has_value()) {
        return std::nullopt;
      }
      SPEC_RULE("Layout-Record");
      return layout->layout;
    }
    if (const auto* enum_decl = std::get_if<cursive0::syntax::EnumDecl>(&it->second)) {
      const auto layout = EnumLayoutOf(ctx, *enum_decl);
      if (!layout.has_value()) {
        return std::nullopt;
      }
      SPEC_RULE("Layout-Enum");
      return layout->layout;
    }
    if (const auto* modal_decl = std::get_if<cursive0::syntax::ModalDecl>(&it->second)) {
      const auto layout = ModalLayoutOf(ctx, *modal_decl);
      if (!layout.has_value()) {
        return std::nullopt;
      }
      SPEC_RULE("Layout-Modal");
      return layout->layout;
    }
    if (const auto* alias = std::get_if<cursive0::syntax::TypeAliasDecl>(&it->second)) {
      const auto lowered = LowerTypeForLayout(ctx, alias->type);
      if (!lowered.has_value()) {
        return std::nullopt;
      }
      SPEC_RULE("Layout-Alias");
      return LayoutOf(ctx, *lowered);
    }
  }

  return std::nullopt;
}

std::optional<std::uint64_t> SizeOf(const cursive0::sema::ScopeContext& ctx,
                                    const cursive0::sema::TypeRef& type) {
  if (!type) {
    return std::nullopt;
  }
  if (const auto* perm = std::get_if<cursive0::sema::TypePerm>(&type->node)) {
    SPEC_RULE("Size-Perm");
    return SizeOf(ctx, perm->base);
  }
  if (const auto* prim = std::get_if<cursive0::sema::TypePrim>(&type->node)) {
    SPEC_RULE("Size-Prim");
    return PrimSize(prim->name);
  }
  if (std::holds_alternative<cursive0::sema::TypePtr>(type->node)) {
    SPEC_RULE("Size-Ptr");
    return kPtrSize;
  }
  if (std::holds_alternative<cursive0::sema::TypeRawPtr>(type->node)) {
    SPEC_RULE("Size-RawPtr");
    return kPtrSize;
  }
  if (std::holds_alternative<cursive0::sema::TypeFunc>(type->node)) {
    SPEC_RULE("Size-Func");
    return kPtrSize;
  }
  if (std::holds_alternative<cursive0::sema::TypeDynamic>(type->node)) {
    SPEC_RULE("Size-DynamicClass");
    return 2 * kPtrSize;
  }
  if (std::holds_alternative<cursive0::sema::TypeSlice>(type->node)) {
    SPEC_RULE("Size-Slice");
    return 2 * kPtrSize;
  }
  if (const auto* str = std::get_if<cursive0::sema::TypeString>(&type->node)) {
    if (!str->state.has_value()) {
      SPEC_RULE("Size-String-Modal");
      const auto layout = ModalStringBytesLayout(3 * kPtrSize, 2 * kPtrSize);
      if (!layout.has_value()) {
        return std::nullopt;
      }
      return layout->size;
    }
    if (*str->state == cursive0::sema::StringState::Managed) {
      SPEC_RULE("Size-String-Managed");
      return 3 * kPtrSize;
    }
    SPEC_RULE("Size-String-View");
    return 2 * kPtrSize;
  }
  if (const auto* bytes =
          std::get_if<cursive0::sema::TypeBytes>(&type->node)) {
    if (!bytes->state.has_value()) {
      SPEC_RULE("Size-Bytes-Modal");
      const auto layout = ModalStringBytesLayout(3 * kPtrSize, 2 * kPtrSize);
      if (!layout.has_value()) {
        return std::nullopt;
      }
      return layout->size;
    }
    if (*bytes->state == cursive0::sema::BytesState::Managed) {
      SPEC_RULE("Size-Bytes-Managed");
      return 3 * kPtrSize;
    }
    SPEC_RULE("Size-Bytes-View");
    return 2 * kPtrSize;
  }
  if (std::holds_alternative<cursive0::sema::TypeRange>(type->node)) {
    SPEC_RULE("Size-Range");
    const auto layout = LayoutOf(ctx, type);
    if (!layout.has_value()) {
      return std::nullopt;
    }
    return layout->size;
  }
  if (const auto* array = std::get_if<cursive0::sema::TypeArray>(&type->node)) {
    SPEC_RULE("Size-Array");
    const auto elem = SizeOf(ctx, array->element);
    if (!elem.has_value()) {
      return std::nullopt;
    }
    return (*elem) * array->length;
  }
  if (const auto* tuple = std::get_if<cursive0::sema::TypeTuple>(&type->node)) {
    (void)tuple;
    SPEC_RULE("Size-Tuple");
    const auto layout = LayoutOf(ctx, type);
    if (!layout.has_value()) {
      return std::nullopt;
    }
    return layout->size;
  }
  if (const auto* uni = std::get_if<cursive0::sema::TypeUnion>(&type->node)) {
    (void)uni;
    SPEC_RULE("Size-Union");
    const auto layout = UnionLayoutOf(ctx, *uni);
    if (!layout.has_value()) {
      return std::nullopt;
    }
    return layout->layout.size;
  }
  if (std::holds_alternative<cursive0::sema::TypeModalState>(type->node)) {
    SPEC_RULE("Size-ModalState");
    const auto layout = LayoutOf(ctx, type);
    if (!layout.has_value()) {
      return std::nullopt;
    }
    return layout->size;
  }
  if (const auto* path = std::get_if<cursive0::sema::TypePathType>(&type->node)) {
    cursive0::syntax::Path syntax_path;
    syntax_path.reserve(path->path.size());
    for (const auto& comp : path->path) {
      syntax_path.push_back(comp);
    }
    const auto it = ctx.sigma.types.find(cursive0::sema::PathKeyOf(syntax_path));
    if (it == ctx.sigma.types.end()) {
      return std::nullopt;
    }
    if (std::holds_alternative<cursive0::syntax::RecordDecl>(it->second)) {
      SPEC_RULE("Size-Record");
      const auto layout = LayoutOf(ctx, type);
      if (!layout.has_value()) {
        return std::nullopt;
      }
      return layout->size;
    }
    if (std::holds_alternative<cursive0::syntax::EnumDecl>(it->second)) {
      SPEC_RULE("Size-Enum");
      const auto layout = LayoutOf(ctx, type);
      if (!layout.has_value()) {
        return std::nullopt;
      }
      return layout->size;
    }
    if (std::holds_alternative<cursive0::syntax::ModalDecl>(it->second)) {
      SPEC_RULE("Size-Modal");
      const auto layout = LayoutOf(ctx, type);
      if (!layout.has_value()) {
        return std::nullopt;
      }
      return layout->size;
    }
    if (const auto* alias = std::get_if<cursive0::syntax::TypeAliasDecl>(&it->second)) {
      SPEC_RULE("Size-Alias");
      const auto lowered = LowerTypeForLayout(ctx, alias->type);
      if (!lowered.has_value()) {
        return std::nullopt;
      }
      return SizeOf(ctx, *lowered);
    }
  }
  return std::nullopt;
}

std::optional<std::uint64_t> AlignOf(const cursive0::sema::ScopeContext& ctx,
                                     const cursive0::sema::TypeRef& type) {
  if (!type) {
    return std::nullopt;
  }
  if (const auto* perm = std::get_if<cursive0::sema::TypePerm>(&type->node)) {
    SPEC_RULE("Align-Perm");
    return AlignOf(ctx, perm->base);
  }
  if (const auto* prim = std::get_if<cursive0::sema::TypePrim>(&type->node)) {
    SPEC_RULE("Align-Prim");
    return PrimAlign(prim->name);
  }
  if (std::holds_alternative<cursive0::sema::TypePtr>(type->node)) {
    SPEC_RULE("Align-Ptr");
    return kPtrAlign;
  }
  if (std::holds_alternative<cursive0::sema::TypeRawPtr>(type->node)) {
    SPEC_RULE("Align-RawPtr");
    return kPtrAlign;
  }
  if (std::holds_alternative<cursive0::sema::TypeFunc>(type->node)) {
    SPEC_RULE("Align-Func");
    return kPtrAlign;
  }
  if (std::holds_alternative<cursive0::sema::TypeDynamic>(type->node)) {
    SPEC_RULE("Align-DynamicClass");
    return kPtrAlign;
  }
  if (std::holds_alternative<cursive0::sema::TypeSlice>(type->node)) {
    SPEC_RULE("Align-Slice");
    return kPtrAlign;
  }
  if (const auto* str = std::get_if<cursive0::sema::TypeString>(&type->node)) {
    if (!str->state.has_value()) {
      SPEC_RULE("Align-String-Modal");
      const auto layout = ModalStringBytesLayout(3 * kPtrSize, 2 * kPtrSize);
      if (!layout.has_value()) {
        return std::nullopt;
      }
      return layout->align;
    }
    if (*str->state == cursive0::sema::StringState::Managed) {
      SPEC_RULE("Align-String-Managed");
      return kPtrAlign;
    }
    SPEC_RULE("Align-String-View");
    return kPtrAlign;
  }
  if (const auto* bytes =
          std::get_if<cursive0::sema::TypeBytes>(&type->node)) {
    if (!bytes->state.has_value()) {
      SPEC_RULE("Align-Bytes-Modal");
      const auto layout = ModalStringBytesLayout(3 * kPtrSize, 2 * kPtrSize);
      if (!layout.has_value()) {
        return std::nullopt;
      }
      return layout->align;
    }
    if (*bytes->state == cursive0::sema::BytesState::Managed) {
      SPEC_RULE("Align-Bytes-Managed");
      return kPtrAlign;
    }
    SPEC_RULE("Align-Bytes-View");
    return kPtrAlign;
  }
  if (std::holds_alternative<cursive0::sema::TypeRange>(type->node)) {
    SPEC_RULE("Align-Range");
    const auto layout = LayoutOf(ctx, type);
    if (!layout.has_value()) {
      return std::nullopt;
    }
    return layout->align;
  }
  if (const auto* array = std::get_if<cursive0::sema::TypeArray>(&type->node)) {
    SPEC_RULE("Align-Array");
    return AlignOf(ctx, array->element);
  }
  if (const auto* tuple = std::get_if<cursive0::sema::TypeTuple>(&type->node)) {
    (void)tuple;
    SPEC_RULE("Align-Tuple");
    const auto layout = LayoutOf(ctx, type);
    if (!layout.has_value()) {
      return std::nullopt;
    }
    return layout->align;
  }
  if (const auto* uni = std::get_if<cursive0::sema::TypeUnion>(&type->node)) {
    (void)uni;
    SPEC_RULE("Align-Union");
    const auto layout = UnionLayoutOf(ctx, *uni);
    if (!layout.has_value()) {
      return std::nullopt;
    }
    return layout->layout.align;
  }
  if (std::holds_alternative<cursive0::sema::TypeModalState>(type->node)) {
    SPEC_RULE("Align-ModalState");
    const auto layout = LayoutOf(ctx, type);
    if (!layout.has_value()) {
      return std::nullopt;
    }
    return layout->align;
  }
  if (const auto* path = std::get_if<cursive0::sema::TypePathType>(&type->node)) {
    cursive0::syntax::Path syntax_path;
    syntax_path.reserve(path->path.size());
    for (const auto& comp : path->path) {
      syntax_path.push_back(comp);
    }
    const auto it = ctx.sigma.types.find(cursive0::sema::PathKeyOf(syntax_path));
    if (it == ctx.sigma.types.end()) {
      return std::nullopt;
    }
    if (std::holds_alternative<cursive0::syntax::RecordDecl>(it->second)) {
      SPEC_RULE("Align-Record");
      const auto layout = LayoutOf(ctx, type);
      if (!layout.has_value()) {
        return std::nullopt;
      }
      return layout->align;
    }
    if (std::holds_alternative<cursive0::syntax::EnumDecl>(it->second)) {
      SPEC_RULE("Align-Enum");
      const auto layout = LayoutOf(ctx, type);
      if (!layout.has_value()) {
        return std::nullopt;
      }
      return layout->align;
    }
    if (std::holds_alternative<cursive0::syntax::ModalDecl>(it->second)) {
      SPEC_RULE("Align-Modal");
      const auto layout = LayoutOf(ctx, type);
      if (!layout.has_value()) {
        return std::nullopt;
      }
      return layout->align;
    }
    if (const auto* alias = std::get_if<cursive0::syntax::TypeAliasDecl>(&it->second)) {
      SPEC_RULE("Align-Alias");
      const auto lowered = LowerTypeForLayout(ctx, alias->type);
      if (!lowered.has_value()) {
        return std::nullopt;
      }
      return AlignOf(ctx, *lowered);
    }
  }
  return std::nullopt;
}

}  // namespace cursive0::codegen
