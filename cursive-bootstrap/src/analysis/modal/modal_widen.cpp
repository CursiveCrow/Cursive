#include "cursive0/analysis/modal/modal_widen.h"

#include <optional>
#include <memory>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "cursive0/core/assert_spec.h"
#include "cursive0/analysis/modal/modal.h"
#include "cursive0/analysis/resolve/scopes.h"
#include "cursive0/analysis/resolve/scopes_lookup.h"
#include "cursive0/analysis/types/type_equiv.h"
#include "cursive0/analysis/types/type_expr.h"

namespace cursive0::analysis {

namespace {

struct TypeLowerResult {
  bool ok = false;
  std::optional<std::string_view> diag_id;
  TypeRef type;
};

static Permission LowerPermission(syntax::TypePerm perm) {
  switch (perm) {
    case syntax::TypePerm::Const:
      return Permission::Const;
    case syntax::TypePerm::Unique:
      return Permission::Unique;
    case syntax::TypePerm::Shared:
      return Permission::Shared;
  }
  return Permission::Const;
}

static std::optional<ParamMode> LowerParamMode(
    const std::optional<syntax::ParamMode>& mode) {
  if (!mode.has_value()) {
    return std::nullopt;
  }
  return ParamMode::Move;
}

static RawPtrQual LowerRawPtrQual(syntax::RawPtrQual qual) {
  switch (qual) {
    case syntax::RawPtrQual::Imm:
      return RawPtrQual::Imm;
    case syntax::RawPtrQual::Mut:
      return RawPtrQual::Mut;
  }
  return RawPtrQual::Imm;
}

static std::optional<StringState> LowerStringState(
    const std::optional<syntax::StringState>& state) {
  if (!state.has_value()) {
    return std::nullopt;
  }
  switch (*state) {
    case syntax::StringState::Managed:
      return StringState::Managed;
    case syntax::StringState::View:
      return StringState::View;
  }
  return std::nullopt;
}

static std::optional<BytesState> LowerBytesState(
    const std::optional<syntax::BytesState>& state) {
  if (!state.has_value()) {
    return std::nullopt;
  }
  switch (*state) {
    case syntax::BytesState::Managed:
      return BytesState::Managed;
    case syntax::BytesState::View:
      return BytesState::View;
  }
  return std::nullopt;
}

static std::optional<PtrState> LowerPtrState(
    const std::optional<syntax::PtrState>& state) {
  if (!state.has_value()) {
    return std::nullopt;
  }
  switch (*state) {
    case syntax::PtrState::Valid:
      return PtrState::Valid;
    case syntax::PtrState::Null:
      return PtrState::Null;
    case syntax::PtrState::Expired:
      return PtrState::Expired;
  }
  return std::nullopt;
}

static TypeLowerResult LowerType(const ScopeContext& ctx,
                                 const std::shared_ptr<syntax::Type>& type);

static TypeLowerResult LowerType(const ScopeContext& ctx,
                                 const std::shared_ptr<syntax::Type>& type) {
  if (!type) {
    return {false, std::nullopt, {}};
  }
  return std::visit(
      [&](const auto& node) -> TypeLowerResult {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::TypePrim>) {
          return {true, std::nullopt, MakeTypePrim(node.name)};
        } else if constexpr (std::is_same_v<T, syntax::TypePermType>) {
          const auto base = LowerType(ctx, node.base);
          if (!base.ok) {
            return base;
          }
          return {true, std::nullopt,
                  MakeTypePerm(LowerPermission(node.perm), base.type)};
        } else if constexpr (std::is_same_v<T, syntax::TypeUnion>) {
          std::vector<TypeRef> members;
          members.reserve(node.types.size());
          for (const auto& elem : node.types) {
            const auto lowered = LowerType(ctx, elem);
            if (!lowered.ok) {
              return lowered;
            }
            members.push_back(lowered.type);
          }
          return {true, std::nullopt, MakeTypeUnion(std::move(members))};
        } else if constexpr (std::is_same_v<T, syntax::TypeFunc>) {
          std::vector<TypeFuncParam> params;
          params.reserve(node.params.size());
          for (const auto& param : node.params) {
            const auto lowered = LowerType(ctx, param.type);
            if (!lowered.ok) {
              return lowered;
            }
            params.push_back(
                TypeFuncParam{LowerParamMode(param.mode), lowered.type});
          }
          const auto ret = LowerType(ctx, node.ret);
          if (!ret.ok) {
            return ret;
          }
          return {true, std::nullopt,
                  MakeTypeFunc(std::move(params), ret.type)};
        } else if constexpr (std::is_same_v<T, syntax::TypeTuple>) {
          std::vector<TypeRef> elements;
          elements.reserve(node.elements.size());
          for (const auto& elem : node.elements) {
            const auto lowered = LowerType(ctx, elem);
            if (!lowered.ok) {
              return lowered;
            }
            elements.push_back(lowered.type);
          }
          return {true, std::nullopt, MakeTypeTuple(std::move(elements))};
        } else if constexpr (std::is_same_v<T, syntax::TypeArray>) {
          const auto elem = LowerType(ctx, node.element);
          if (!elem.ok) {
            return elem;
          }
          const auto len = ConstLen(ctx, node.length);
          if (!len.ok || !len.value.has_value()) {
            return {false, len.diag_id, {}};
          }
          return {true, std::nullopt, MakeTypeArray(elem.type, *len.value)};
        } else if constexpr (std::is_same_v<T, syntax::TypeSlice>) {
          const auto elem = LowerType(ctx, node.element);
          if (!elem.ok) {
            return elem;
          }
          return {true, std::nullopt, MakeTypeSlice(elem.type)};
        } else if constexpr (std::is_same_v<T, syntax::TypePtr>) {
          const auto elem = LowerType(ctx, node.element);
          if (!elem.ok) {
            return elem;
          }
          return {true, std::nullopt,
                  MakeTypePtr(elem.type, LowerPtrState(node.state))};
        } else if constexpr (std::is_same_v<T, syntax::TypeRawPtr>) {
          const auto elem = LowerType(ctx, node.element);
          if (!elem.ok) {
            return elem;
          }
          return {true, std::nullopt,
                  MakeTypeRawPtr(LowerRawPtrQual(node.qual), elem.type)};
        } else if constexpr (std::is_same_v<T, syntax::TypeString>) {
          return {true, std::nullopt,
                  MakeTypeString(LowerStringState(node.state))};
        } else if constexpr (std::is_same_v<T, syntax::TypeBytes>) {
          return {true, std::nullopt,
                  MakeTypeBytes(LowerBytesState(node.state))};
        } else if constexpr (std::is_same_v<T, syntax::TypeDynamic>) {
          return {true, std::nullopt, MakeTypeDynamic(node.path)};
        } else if constexpr (std::is_same_v<T, syntax::TypeModalState>) {
          return {true, std::nullopt,
                  MakeTypeModalState(node.path, node.state)};
        } else if constexpr (std::is_same_v<T, syntax::TypePathType>) {
          return {true, std::nullopt, MakeTypePath(node.path)};
        } else {
          return {false, std::nullopt, {}};
        }
      },
      type->node);
}

static std::size_t StateFieldCount(const syntax::StateBlock& state) {
  std::size_t count = 0;
  for (const auto& member : state.members) {
    if (std::holds_alternative<syntax::StateFieldDecl>(member)) {
      ++count;
    }
  }
  return count;
}

static const syntax::StateFieldDecl* SingleFieldPayload(
    const syntax::StateBlock& state) {
  const syntax::StateFieldDecl* field = nullptr;
  for (const auto& member : state.members) {
    const auto* payload = std::get_if<syntax::StateFieldDecl>(&member);
    if (!payload) {
      continue;
    }
    if (field) {
      return nullptr;
    }
    field = payload;
  }
  return field;
}

static bool EmptyState(const syntax::StateBlock& state) {
  return StateFieldCount(state) == 0;
}

static std::uint64_t NicheCount(const ScopeContext& ctx, const TypeRef& type) {
  (void)ctx;
  if (!type) {
    return 0;
  }
  const auto* ptr = std::get_if<TypePtr>(&type->node);
  if (!ptr) {
    return 0;
  }
  return ptr->state == PtrState::Valid ? 1 : 0;
}

}  // namespace

std::optional<std::string_view> PayloadState(const ScopeContext& ctx,
                                             const syntax::ModalDecl& decl) {
  std::optional<std::string_view> candidate;
  std::uint64_t niche_count = 0;
  for (const auto& state : decl.states) {
    const auto* payload = SingleFieldPayload(state);
    if (!payload) {
      continue;
    }
    const auto lowered = LowerType(ctx, payload->type);
    if (!lowered.ok) {
      return std::nullopt;
    }
    const auto count = NicheCount(ctx, lowered.type);
    if (count == 0) {
      continue;
    }
    if (candidate.has_value()) {
      return std::nullopt;
    }
    candidate = state.name;
    niche_count = count;
  }

  if (!candidate.has_value()) {
    return std::nullopt;
  }

  for (const auto& state : decl.states) {
    if (IdEq(state.name, *candidate)) {
      continue;
    }
    if (!EmptyState(state)) {
      return std::nullopt;
    }
  }

  const std::uint64_t required =
      decl.states.empty() ? 0ull : static_cast<std::uint64_t>(decl.states.size() - 1);
  if (niche_count < required) {
    return std::nullopt;
  }

  return candidate;
}

bool NicheApplies(const ScopeContext& ctx, const syntax::ModalDecl& decl) {
  return PayloadState(ctx, decl).has_value();
}

bool NicheCompatible(const ScopeContext& ctx,
                     const TypePath& modal_path,
                     std::string_view state) {
  const auto* decl = LookupModalDecl(ctx, modal_path);
  if (!decl) {
    return false;
  }
  if (!HasState(*decl, state)) {
    return false;
  }
  const auto payload_state = PayloadState(ctx, *decl);
  if (!payload_state.has_value() || !IdEq(*payload_state, state)) {
    return false;
  }

  const auto state_type = MakeTypeModalState(modal_path, std::string(state));
  const auto modal_type = MakeTypePath(modal_path);
  const auto state_size = SizeOf(ctx, state_type);
  const auto modal_size = SizeOf(ctx, modal_type);
  if (!state_size.has_value() || !modal_size.has_value()) {
    return false;
  }
  if (*state_size != *modal_size) {
    return false;
  }
  const auto state_align = AlignOf(ctx, state_type);
  const auto modal_align = AlignOf(ctx, modal_type);
  if (!state_align.has_value() || !modal_align.has_value()) {
    return false;
  }
  return *state_align == *modal_align;
}

bool WidenWarnCond(const ScopeContext& ctx,
                   const TypePath& modal_path,
                   std::string_view state) {
  const auto* decl = LookupModalDecl(ctx, modal_path);
  if (!decl) {
    return false;
  }
  if (!HasState(*decl, state)) {
    return false;
  }
  const auto state_type = MakeTypeModalState(modal_path, std::string(state));
  const auto size = SizeOf(ctx, state_type);
  if (!size.has_value()) {
    return false;
  }
  if (*size <= kWidenLargePayloadThresholdBytes) {
    return false;
  }
  return !NicheCompatible(ctx, modal_path, state);
}

}  // namespace cursive0::analysis
