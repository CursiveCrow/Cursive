#include "cursive0/analysis/types/type_expr.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <unordered_map>
#include <utility>
#include <vector>

#include "cursive0/core/assert_spec.h"
#include "cursive0/core/diagnostic_messages.h"
#include "cursive0/analysis/composite/arrays_slices.h"
#include "cursive0/analysis/composite/classes.h"
#include "cursive0/analysis/caps/cap_filesystem.h"
#include "cursive0/analysis/caps/cap_heap.h"
#include "cursive0/analysis/caps/cap_system.h"
#include "cursive0/analysis/caps/cap_concurrency.h"
#include "cursive0/analysis/composite/enums.h"
#include "cursive0/analysis/composite/function_types.h"
#include "cursive0/analysis/contracts/contract_check.h"
#include "cursive0/analysis/contracts/verification.h"
#include "cursive0/analysis/types/literals.h"
#include "cursive0/analysis/modal/modal.h"
#include "cursive0/analysis/modal/modal_fields.h"
#include "cursive0/analysis/modal/modal_transitions.h"
#include "cursive0/analysis/modal/modal_widen.h"
#include "cursive0/analysis/composite/records.h"
#include "cursive0/analysis/composite/record_methods.h"
#include "cursive0/analysis/composite/tuples.h"
#include "cursive0/analysis/resolve/scopes.h"
#include "cursive0/analysis/resolve/scopes_lookup.h"
#include "cursive0/analysis/memory/safe_ptr.h"
#include "cursive0/analysis/types/subtyping.h"
#include "cursive0/analysis/types/type_equiv.h"
#include "cursive0/analysis/types/type_infer.h"
#include "cursive0/analysis/types/type_match.h"
#include "cursive0/analysis/types/type_stmt.h"
#include "cursive0/analysis/memory/regions.h"

namespace cursive0::analysis {

namespace {

// §5.2.12 definitions
static inline void SpecDefsTypeExpr() {
  SPEC_DEF("ExprJudg", "5.2.12");
  SPEC_DEF("UnresolvedExpr", "5.2.12");
  SPEC_DEF("StripPerm", "5.2.12");
  SPEC_DEF("BitcopyType", "5.2.12");
  SPEC_DEF("SignedIntTypes", "5.2.12");
  SPEC_DEF("NumericTypes", "5.2.12");
  SPEC_DEF("IntTypes", "5.2.12");
  SPEC_DEF("FloatTypes", "5.2.12");
  SPEC_DEF("EqType", "5.2.12");
  SPEC_DEF("OrdType", "5.2.12");
  SPEC_DEF("ValuePathType", "5.2.12");
  SPEC_DEF("UnsafeSpan", "5.2.12");
  SPEC_DEF("ArithOps", "5.2.12");
  SPEC_DEF("BitOps", "5.2.12");
  SPEC_DEF("ShiftOps", "5.2.12");
  SPEC_DEF("CmpOps", "5.2.12");
  SPEC_DEF("LogicOps", "5.2.12");
  SPEC_DEF("CastValid", "5.2.12");
  SPEC_DEF("AddrOfOk", "5.2.12");
  SPEC_DEF("SuccessMember", "5.2.12");
  SPEC_DEF("FieldVis", "5.2.12");
  SPEC_DEF("FieldVisible", "5.2.12");
  SPEC_DEF("FieldNames", "5.2.12");
  SPEC_DEF("FieldInitNames", "5.2.12");
  SPEC_DEF("FieldNameSet", "5.2.12");
  SPEC_DEF("FieldInitSet", "5.2.12");
  SPEC_DEF("RegionActiveType", "5.2.17");
  SPEC_DEF("InnermostActiveRegion", "5.2.17");
  SpecDefsSafePtr();
}

// Type sets
static constexpr std::array<std::string_view, 12> kIntTypes = {
    "i8", "i16", "i32", "i64", "i128", "isize",
    "u8", "u16", "u32", "u64", "u128", "usize"};

static constexpr std::array<std::string_view, 6> kSignedIntTypes = {
    "i8", "i16", "i32", "i64", "i128", "isize"};

static constexpr std::array<std::string_view, 3> kFloatTypes = {"f16", "f32",
                                                                "f64"};

static bool IsIntType(std::string_view name) {
  return std::find(kIntTypes.begin(), kIntTypes.end(), name) != kIntTypes.end();
}

static bool IsSignedIntType(std::string_view name) {
  return std::find(kSignedIntTypes.begin(), kSignedIntTypes.end(), name) !=
         kSignedIntTypes.end();
}

static bool IsFloatType(std::string_view name) {
  return std::find(kFloatTypes.begin(), kFloatTypes.end(), name) !=
         kFloatTypes.end();
}

static bool IsNumericType(std::string_view name) {
  return IsIntType(name) || IsFloatType(name);
}

static bool IsPrimTypeName(std::string_view name) {
  return IsIntType(name) || IsFloatType(name) || name == "bool" ||
         name == "char" || name == "()" || name == "!";
}

struct TypeLowerResult {
  bool ok = false;
  std::optional<std::string_view> diag_id;
  TypeRef type;
};

static TypeLowerResult LowerType(const ScopeContext& ctx,
                                 const std::shared_ptr<syntax::Type>& type);

static constexpr std::uint64_t kPtrSize = 8;
static constexpr std::uint64_t kPtrAlign = 8;

static std::optional<std::uint64_t> PrimSize(std::string_view name) {
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

static std::optional<std::uint64_t> PrimAlign(std::string_view name) {
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

static std::uint64_t AlignUp(std::uint64_t value, std::uint64_t align) {
  if (align == 0) {
    return value;
  }
  const std::uint64_t rem = value % align;
  if (rem == 0) {
    return value;
  }
  return value + (align - rem);
}

static std::optional<std::string_view> DiscTypeName(
    std::uint64_t max_disc) {
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

static std::optional<std::pair<std::uint64_t, std::uint64_t>> DiscTypeLayout(
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

static std::optional<std::pair<std::uint64_t, std::uint64_t>>
LayoutOfSeq(const ScopeContext& ctx, const std::vector<TypeRef>& elems);

static std::optional<std::pair<std::uint64_t, std::uint64_t>>
LayoutOf(const ScopeContext& ctx, const TypeRef& type) {
  if (!type) {
    return std::nullopt;
  }
  if (const auto* perm = std::get_if<TypePerm>(&type->node)) {
    return LayoutOf(ctx, perm->base);
  }
  if (const auto* refine = std::get_if<TypeRefine>(&type->node)) {
    return LayoutOf(ctx, refine->base);
  }

  auto modal_layout = [&](std::uint64_t managed_size,
                          std::uint64_t view_size)
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
  };

  auto state_layout = [&](const syntax::StateBlock& state)
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
  };

  auto modal_state_layout = [&](const syntax::ModalDecl& decl,
                                std::string_view state_name)
      -> std::optional<std::pair<std::uint64_t, std::uint64_t>> {
    for (const auto& state : decl.states) {
      if (IdEq(state.name, state_name)) {
        return state_layout(state);
      }
    }
    return std::nullopt;
  };

  auto modal_decl_layout = [&](const syntax::ModalDecl& decl)
      -> std::optional<std::pair<std::uint64_t, std::uint64_t>> {
    if (decl.states.empty()) {
      return std::nullopt;
    }
    const auto payload_state = PayloadState(ctx, decl);
    if (payload_state.has_value()) {
      return modal_state_layout(decl, *payload_state);
    }
    std::uint64_t max_size = 0;
    std::uint64_t max_align = 1;
    for (const auto& state : decl.states) {
      const auto layout = state_layout(state);
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
  };

  auto enum_decl_layout = [&](const syntax::EnumDecl& decl)
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
                     std::get_if<syntax::VariantPayloadTuple>(
                         &*variant.payload_opt)) {
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
                     std::get_if<syntax::VariantPayloadRecord>(
                         &*variant.payload_opt)) {
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

    const std::uint64_t align =
        std::max(disc_layout->second, payload_align);
    const std::uint64_t size =
        AlignUp(disc_layout->first + payload_size, align);
    return std::make_pair(size, align);
  };

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
      return modal_layout(3 * kPtrSize, 2 * kPtrSize);
    }
    if (*str->state == StringState::Managed) {
      return std::make_pair(3 * kPtrSize, kPtrAlign);
    }
    return std::make_pair(2 * kPtrSize, kPtrAlign);
  }
  if (const auto* bytes = std::get_if<TypeBytes>(&type->node)) {
    if (!bytes->state.has_value()) {
      return modal_layout(3 * kPtrSize, 2 * kPtrSize);
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
    const std::uint64_t align =
        std::max(disc_layout->second, payload_align);
    const std::uint64_t size =
        AlignUp(disc_layout->first + payload_size, align);
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
    return modal_state_layout(*decl, modal->state);
  }
  if (const auto* path = std::get_if<TypePathType>(&type->node)) {
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
      return enum_decl_layout(*enum_decl);
    }
    if (const auto* modal_decl = std::get_if<syntax::ModalDecl>(&it->second)) {
      return modal_decl_layout(*modal_decl);
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

static std::optional<std::pair<std::uint64_t, std::uint64_t>>
LayoutOfSeq(const ScopeContext& ctx, const std::vector<TypeRef>& elems) {
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

static std::optional<std::uint64_t> SizeOfInternal(const ScopeContext& ctx,
                                           const TypeRef& type) {
  const auto layout = LayoutOf(ctx, type);
  if (!layout.has_value()) {
    return std::nullopt;
  }
  return layout->first;
}

static std::optional<std::uint64_t> AlignOfInternal(const ScopeContext& ctx,
                                            const TypeRef& type) {
  const auto layout = LayoutOf(ctx, type);
  if (!layout.has_value()) {
    return std::nullopt;
  }
  return layout->second;
}

static bool IsPrimType(const TypeRef& type, std::string_view name) {
  if (!type) {
    return false;
  }
  const auto* prim = std::get_if<TypePrim>(&type->node);
  return prim && prim->name == name;
}

static std::optional<std::string> GetPrimName(const TypeRef& type) {
  if (!type) {
    return std::nullopt;
  }
  const auto stripped = StripPerm(type);
  if (!stripped) {
    return std::nullopt;
  }
  const auto* prim = std::get_if<TypePrim>(&stripped->node);
  if (!prim) {
    return std::nullopt;
  }
  return prim->name;
}

// Parse a tuple index from an IntLiteral token
static std::optional<std::size_t> ParseTupleIndex(const syntax::Token& token) {
  if (token.kind != syntax::TokenKind::IntLiteral) {
    return std::nullopt;
  }
  // Simple decimal parsing (tuple indices are always small decimal numbers)
  std::size_t value = 0;
  for (char c : token.lexeme) {
    if (c == '_') continue;
    if (c < '0' || c > '9') return std::nullopt;
    value = value * 10 + static_cast<std::size_t>(c - '0');
  }
  return value;
}

// Forward declarations for mutual recursion
static ExprTypeResult TypeExprImpl(const ScopeContext& ctx,
                                   const StmtTypeContext& type_ctx,
                                   const syntax::ExprPtr& expr,
                                   const TypeEnv& env);

static PlaceTypeResult TypePlaceImpl(const ScopeContext& ctx,
                                     const StmtTypeContext& type_ctx,
                                     const syntax::ExprPtr& expr,
                                     const TypeEnv& env);

static ExprTypeResult TypeTransmuteExprImpl(const ScopeContext& ctx,
                                 const StmtTypeContext& type_ctx,
                                 const syntax::TransmuteExpr& expr,
                                 const TypeEnv& env,
                                 const core::Span& span);

static std::optional<ParamMode> LowerParamMode(
    const std::optional<syntax::ParamMode>& mode);

static syntax::ExprPtr MakeExpr(const core::Span& span, syntax::ExprNode node) {
  auto expr = std::make_shared<syntax::Expr>();
  expr->span = span;
  expr->node = std::move(node);
  return expr;
}

static syntax::ExprPtr SubstituteIdent(const syntax::ExprPtr& expr,
                                       std::string_view name,
                                       const syntax::ExprPtr& replacement) {
  if (!expr) {
    return expr;
  }
  if (const auto* ident = std::get_if<syntax::IdentifierExpr>(&expr->node)) {
    if (IdEq(ident->name, name)) {
      return replacement;
    }
    return expr;
  }
  return std::visit(
      [&](const auto& node) -> syntax::ExprPtr {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::BinaryExpr>) {
          auto out = node;
          out.lhs = SubstituteIdent(node.lhs, name, replacement);
          out.rhs = SubstituteIdent(node.rhs, name, replacement);
          return MakeExpr(expr->span, out);
        } else if constexpr (std::is_same_v<T, syntax::UnaryExpr>) {
          auto out = node;
          out.value = SubstituteIdent(node.value, name, replacement);
          return MakeExpr(expr->span, out);
        } else if constexpr (std::is_same_v<T, syntax::FieldAccessExpr>) {
          auto out = node;
          out.base = SubstituteIdent(node.base, name, replacement);
          return MakeExpr(expr->span, out);
        } else if constexpr (std::is_same_v<T, syntax::TupleAccessExpr>) {
          auto out = node;
          out.base = SubstituteIdent(node.base, name, replacement);
          return MakeExpr(expr->span, out);
        } else if constexpr (std::is_same_v<T, syntax::IndexAccessExpr>) {
          auto out = node;
          out.base = SubstituteIdent(node.base, name, replacement);
          out.index = SubstituteIdent(node.index, name, replacement);
          return MakeExpr(expr->span, out);
        } else if constexpr (std::is_same_v<T, syntax::CallExpr>) {
          auto out = node;
          out.callee = SubstituteIdent(node.callee, name, replacement);
          for (auto& arg : out.args) {
            arg.value = SubstituteIdent(arg.value, name, replacement);
          }
          return MakeExpr(expr->span, out);
        } else if constexpr (std::is_same_v<T, syntax::QualifiedApplyExpr>) {
          auto out = node;
          if (std::holds_alternative<syntax::ParenArgs>(node.args)) {
            auto paren = std::get<syntax::ParenArgs>(node.args);
            for (auto& arg : paren.args) {
              arg.value = SubstituteIdent(arg.value, name, replacement);
            }
            out.args = paren;
          } else {
            auto brace = std::get<syntax::BraceArgs>(node.args);
            for (auto& field : brace.fields) {
              field.value = SubstituteIdent(field.value, name, replacement);
            }
            out.args = brace;
          }
          return MakeExpr(expr->span, out);
        } else if constexpr (std::is_same_v<T, syntax::MethodCallExpr>) {
          auto out = node;
          out.receiver = SubstituteIdent(node.receiver, name, replacement);
          for (auto& arg : out.args) {
            arg.value = SubstituteIdent(arg.value, name, replacement);
          }
          return MakeExpr(expr->span, out);
        } else if constexpr (std::is_same_v<T, syntax::CastExpr>) {
          auto out = node;
          out.value = SubstituteIdent(node.value, name, replacement);
          return MakeExpr(expr->span, out);
        } else if constexpr (std::is_same_v<T, syntax::RangeExpr>) {
          auto out = node;
          out.lhs = SubstituteIdent(node.lhs, name, replacement);
          out.rhs = SubstituteIdent(node.rhs, name, replacement);
          return MakeExpr(expr->span, out);
        } else if constexpr (std::is_same_v<T, syntax::DerefExpr>) {
          auto out = node;
          out.value = SubstituteIdent(node.value, name, replacement);
          return MakeExpr(expr->span, out);
        } else if constexpr (std::is_same_v<T, syntax::AddressOfExpr>) {
          auto out = node;
          out.place = SubstituteIdent(node.place, name, replacement);
          return MakeExpr(expr->span, out);
        } else if constexpr (std::is_same_v<T, syntax::MoveExpr>) {
          auto out = node;
          out.place = SubstituteIdent(node.place, name, replacement);
          return MakeExpr(expr->span, out);
        } else if constexpr (std::is_same_v<T, syntax::AllocExpr>) {
          auto out = node;
          out.value = SubstituteIdent(node.value, name, replacement);
          return MakeExpr(expr->span, out);
        } else if constexpr (std::is_same_v<T, syntax::TupleExpr>) {
          auto out = node;
          for (auto& elem : out.elements) {
            elem = SubstituteIdent(elem, name, replacement);
          }
          return MakeExpr(expr->span, out);
        } else if constexpr (std::is_same_v<T, syntax::ArrayExpr>) {
          auto out = node;
          for (auto& elem : out.elements) {
            elem = SubstituteIdent(elem, name, replacement);
          }
          return MakeExpr(expr->span, out);
        } else if constexpr (std::is_same_v<T, syntax::RecordExpr>) {
          auto out = node;
          for (auto& field : out.fields) {
            field.value = SubstituteIdent(field.value, name, replacement);
          }
          return MakeExpr(expr->span, out);
        } else if constexpr (std::is_same_v<T, syntax::EnumLiteralExpr>) {
          auto out = node;
          if (out.payload_opt.has_value()) {
            if (std::holds_alternative<syntax::EnumPayloadParen>(*out.payload_opt)) {
              auto paren = std::get<syntax::EnumPayloadParen>(*out.payload_opt);
              for (auto& elem : paren.elements) {
                elem = SubstituteIdent(elem, name, replacement);
              }
              out.payload_opt = paren;
            } else {
              auto brace = std::get<syntax::EnumPayloadBrace>(*out.payload_opt);
              for (auto& field : brace.fields) {
                field.value = SubstituteIdent(field.value, name, replacement);
              }
              out.payload_opt = brace;
            }
          }
          return MakeExpr(expr->span, out);
        } else if constexpr (std::is_same_v<T, syntax::IfExpr>) {
          auto out = node;
          out.cond = SubstituteIdent(node.cond, name, replacement);
          out.then_expr = SubstituteIdent(node.then_expr, name, replacement);
          out.else_expr = SubstituteIdent(node.else_expr, name, replacement);
          return MakeExpr(expr->span, out);
        } else if constexpr (std::is_same_v<T, syntax::MatchExpr>) {
          auto out = node;
          out.value = SubstituteIdent(node.value, name, replacement);
          for (auto& arm : out.arms) {
            arm.guard_opt = SubstituteIdent(arm.guard_opt, name, replacement);
            arm.body = SubstituteIdent(arm.body, name, replacement);
          }
          return MakeExpr(expr->span, out);
        } else if constexpr (std::is_same_v<T, syntax::PropagateExpr>) {
          auto out = node;
          out.value = SubstituteIdent(node.value, name, replacement);
          return MakeExpr(expr->span, out);
        } else if constexpr (std::is_same_v<T, syntax::EntryExpr>) {
          auto out = node;
          out.expr = SubstituteIdent(node.expr, name, replacement);
          return MakeExpr(expr->span, out);
        } else {
          return expr;
        }
      },
      expr->node);
}

static bool ExprUsesOnlyEnvBindings(const syntax::ExprPtr& expr,
                                    const TypeEnv& env) {
  if (!expr) {
    return true;
  }
  if (const auto* ident = std::get_if<syntax::IdentifierExpr>(&expr->node)) {
    return BindOf(env, ident->name).has_value();
  }
  if (std::holds_alternative<syntax::ResultExpr>(expr->node)) {
    return false;
  }
  return std::visit(
      [&](const auto& node) -> bool {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::BinaryExpr>) {
          return ExprUsesOnlyEnvBindings(node.lhs, env) &&
                 ExprUsesOnlyEnvBindings(node.rhs, env);
        } else if constexpr (std::is_same_v<T, syntax::UnaryExpr>) {
          return ExprUsesOnlyEnvBindings(node.value, env);
        } else if constexpr (std::is_same_v<T, syntax::FieldAccessExpr>) {
          return ExprUsesOnlyEnvBindings(node.base, env);
        } else if constexpr (std::is_same_v<T, syntax::TupleAccessExpr>) {
          return ExprUsesOnlyEnvBindings(node.base, env);
        } else if constexpr (std::is_same_v<T, syntax::IndexAccessExpr>) {
          return ExprUsesOnlyEnvBindings(node.base, env) &&
                 ExprUsesOnlyEnvBindings(node.index, env);
        } else if constexpr (std::is_same_v<T, syntax::CallExpr>) {
          if (!ExprUsesOnlyEnvBindings(node.callee, env)) {
            return false;
          }
          for (const auto& arg : node.args) {
            if (!ExprUsesOnlyEnvBindings(arg.value, env)) {
              return false;
            }
          }
          return true;
        } else if constexpr (std::is_same_v<T, syntax::QualifiedApplyExpr>) {
          return false;
        } else if constexpr (std::is_same_v<T, syntax::QualifiedNameExpr>) {
          return false;
        } else if constexpr (std::is_same_v<T, syntax::PathExpr>) {
          return false;
        } else if constexpr (std::is_same_v<T, syntax::MethodCallExpr>) {
          if (!ExprUsesOnlyEnvBindings(node.receiver, env)) {
            return false;
          }
          for (const auto& arg : node.args) {
            if (!ExprUsesOnlyEnvBindings(arg.value, env)) {
              return false;
            }
          }
          return true;
        } else if constexpr (std::is_same_v<T, syntax::CastExpr>) {
          return ExprUsesOnlyEnvBindings(node.value, env);
        } else if constexpr (std::is_same_v<T, syntax::RangeExpr>) {
          return ExprUsesOnlyEnvBindings(node.lhs, env) &&
                 ExprUsesOnlyEnvBindings(node.rhs, env);
        } else if constexpr (std::is_same_v<T, syntax::DerefExpr>) {
          return ExprUsesOnlyEnvBindings(node.value, env);
        } else if constexpr (std::is_same_v<T, syntax::AddressOfExpr>) {
          return ExprUsesOnlyEnvBindings(node.place, env);
        } else if constexpr (std::is_same_v<T, syntax::MoveExpr>) {
          return ExprUsesOnlyEnvBindings(node.place, env);
        } else if constexpr (std::is_same_v<T, syntax::AllocExpr>) {
          return ExprUsesOnlyEnvBindings(node.value, env);
        } else if constexpr (std::is_same_v<T, syntax::TupleExpr>) {
          for (const auto& elem : node.elements) {
            if (!ExprUsesOnlyEnvBindings(elem, env)) {
              return false;
            }
          }
          return true;
        } else if constexpr (std::is_same_v<T, syntax::ArrayExpr>) {
          for (const auto& elem : node.elements) {
            if (!ExprUsesOnlyEnvBindings(elem, env)) {
              return false;
            }
          }
          return true;
        } else if constexpr (std::is_same_v<T, syntax::RecordExpr>) {
          for (const auto& field : node.fields) {
            if (!ExprUsesOnlyEnvBindings(field.value, env)) {
              return false;
            }
          }
          return true;
        } else if constexpr (std::is_same_v<T, syntax::EnumLiteralExpr>) {
          if (!node.payload_opt.has_value()) {
            return true;
          }
          if (std::holds_alternative<syntax::EnumPayloadParen>(*node.payload_opt)) {
            const auto& paren = std::get<syntax::EnumPayloadParen>(*node.payload_opt);
            for (const auto& elem : paren.elements) {
              if (!ExprUsesOnlyEnvBindings(elem, env)) {
                return false;
              }
            }
            return true;
          }
          const auto& brace = std::get<syntax::EnumPayloadBrace>(*node.payload_opt);
          for (const auto& field : brace.fields) {
            if (!ExprUsesOnlyEnvBindings(field.value, env)) {
              return false;
            }
          }
          return true;
        } else if constexpr (std::is_same_v<T, syntax::IfExpr>) {
          return ExprUsesOnlyEnvBindings(node.cond, env) &&
                 ExprUsesOnlyEnvBindings(node.then_expr, env) &&
                 ExprUsesOnlyEnvBindings(node.else_expr, env);
        } else if constexpr (std::is_same_v<T, syntax::MatchExpr>) {
          if (!ExprUsesOnlyEnvBindings(node.value, env)) {
            return false;
          }
          for (const auto& arm : node.arms) {
            if (!ExprUsesOnlyEnvBindings(arm.guard_opt, env)) {
              return false;
            }
            if (!ExprUsesOnlyEnvBindings(arm.body, env)) {
              return false;
            }
          }
          return true;
        } else if constexpr (std::is_same_v<T, syntax::PropagateExpr>) {
          return ExprUsesOnlyEnvBindings(node.value, env);
        } else if constexpr (std::is_same_v<T, syntax::EntryExpr>) {
          return ExprUsesOnlyEnvBindings(node.expr, env);
        } else {
          return true;
        }
      },
      expr->node);
}

static bool ProveRefinePredicate(const syntax::ExprPtr& value,
                                 const TypeRefine& refine,
                                 std::optional<std::string_view>& diag_id) {
  if (!refine.predicate) {
    return false;
  }
  const auto substituted =
      SubstituteIdent(refine.predicate, "self", value);
  StaticProofContext proof_ctx;
  const auto proof = StaticProof(proof_ctx, substituted);
  if (!proof.provable) {
    diag_id = "E-TYP-1953";
    return false;
  }
  return true;
}

static Permission PermOfType(const TypeRef& type) {
  if (!type) {
    return Permission::Const;
  }
  if (const auto* perm = std::get_if<TypePerm>(&type->node)) {
    return perm->perm;
  }
  return Permission::Const;
}

static bool IsCapabilityType(const TypeRef& type) {
  const auto stripped = StripPerm(type);
  if (!stripped) {
    return false;
  }
  const auto* dyn = std::get_if<TypeDynamic>(&stripped->node);
  if (!dyn) {
    return false;
  }
  return IsFileSystemClassPath(dyn->path) ||
         IsHeapAllocatorClassPath(dyn->path) ||
         IsReactorClassPath(dyn->path) ||
         IsExecutionDomainClassPath(dyn->path);
}

static bool IsImpureType(const TypeRef& type) {
  if (!type) {
    return false;
  }
  if (PermOfType(type) == Permission::Unique) {
    return true;
  }
  if (IsCapabilityType(type)) {
    return true;
  }
  return false;
}

static bool ParamsPure(const ScopeContext& ctx,
                       const std::vector<TypeFuncParam>& params) {
  (void)ctx;
  for (const auto& param : params) {
    if (IsImpureType(param.type)) {
      return false;
    }
  }
  return true;
}

static bool ParamsPure(const ScopeContext& ctx,
                       const std::vector<syntax::Param>& params,
                       const std::function<TypeLowerResult(
                           const std::shared_ptr<syntax::Type>&)>& lower_type) {
  for (const auto& param : params) {
    const auto lowered = lower_type(param.type);
    if (!lowered.ok) {
      return true;
    }
    if (IsImpureType(lowered.type)) {
      return false;
    }
  }
  return true;
}

static TypeRef SubstSelfType(const TypeRef& self, const TypeRef& type) {
  if (!type) {
    return type;
  }
  return std::visit(
      [&](const auto& node) -> TypeRef {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, TypePathType>) {
          if (node.path.size() == 1 && node.path[0] == "Self") {
            return self;
          }
          return type;
        } else if constexpr (std::is_same_v<T, TypePerm>) {
          return MakeTypePerm(node.perm, SubstSelfType(self, node.base));
        } else if constexpr (std::is_same_v<T, TypeTuple>) {
          std::vector<TypeRef> elems;
          elems.reserve(node.elements.size());
          for (const auto& elem : node.elements) {
            elems.push_back(SubstSelfType(self, elem));
          }
          return MakeTypeTuple(std::move(elems));
        } else if constexpr (std::is_same_v<T, TypeArray>) {
          return MakeTypeArray(SubstSelfType(self, node.element), node.length);
        } else if constexpr (std::is_same_v<T, TypeSlice>) {
          return MakeTypeSlice(SubstSelfType(self, node.element));
        } else if constexpr (std::is_same_v<T, TypeUnion>) {
          std::vector<TypeRef> members;
          members.reserve(node.members.size());
          for (const auto& member : node.members) {
            members.push_back(SubstSelfType(self, member));
          }
          return MakeTypeUnion(std::move(members));
        } else if constexpr (std::is_same_v<T, TypeFunc>) {
          std::vector<TypeFuncParam> params;
          params.reserve(node.params.size());
          for (const auto& param : node.params) {
            params.push_back(
                TypeFuncParam{param.mode, SubstSelfType(self, param.type)});
          }
          return MakeTypeFunc(std::move(params), SubstSelfType(self, node.ret));
        } else if constexpr (std::is_same_v<T, TypePtr>) {
          return MakeTypePtr(SubstSelfType(self, node.element), node.state);
        } else if constexpr (std::is_same_v<T, TypeRawPtr>) {
          return MakeTypeRawPtr(node.qual, SubstSelfType(self, node.element));
        } else if constexpr (std::is_same_v<T, TypeRefine>) {
          return MakeTypeRefine(SubstSelfType(self, node.base), node.predicate);
        } else if constexpr (std::is_same_v<T, TypeModalState>) {
          std::vector<TypeRef> args;
          args.reserve(node.generic_args.size());
          for (const auto& arg : node.generic_args) {
            args.push_back(SubstSelfType(self, arg));
          }
          return MakeTypeModalState(node.path, node.state, std::move(args));
        } else {
          return type;
        }
      },
      type->node);
}

static bool PermSub(Permission lhs, Permission rhs) {
  if (lhs == Permission::Const && rhs == Permission::Const) {
    SPEC_RULE("Perm-Const");
    return true;
  }
  if (lhs == Permission::Unique && rhs == Permission::Unique) {
    SPEC_RULE("Perm-Unique");
    return true;
  }
  if (lhs == Permission::Unique && rhs == Permission::Const) {
    SPEC_RULE("Perm-Unique-Const");
    return true;
  }
  return false;
}

struct AddrOfOkResult {
  bool ok = false;
  std::optional<std::string_view> diag_id;
};

static AddrOfOkResult AddrOfOk(const syntax::ExprPtr& expr,
                               const ExprTypeFn& type_expr) {
  if (!IsPlaceExpr(expr)) {
    return {false, std::nullopt};
  }
  const auto* index = std::get_if<syntax::IndexAccessExpr>(&expr->node);
  if (!index) {
    return {true, std::nullopt};
  }
  const auto idx_type = type_expr(index->index);
  if (!idx_type.ok) {
    return {false, idx_type.diag_id};
  }
  const auto idx_stripped = StripPerm(idx_type.type);
  if (IsPrimType(idx_stripped, "usize")) {
    return {true, std::nullopt};
  }

  const auto base_type = type_expr(index->base);
  if (!base_type.ok) {
    return {false, base_type.diag_id};
  }
  const auto stripped = StripPerm(base_type.type);
  if (stripped) {
    if (std::holds_alternative<TypeArray>(stripped->node)) {
      return {false, "Index-Array-NonUsize"};
    }
    if (std::holds_alternative<TypeSlice>(stripped->node)) {
      return {false, "Index-Slice-NonUsize"};
    }
  }
  return {false, "Index-NonIndexable"};
}


// Record field lookup helpers
static syntax::Path FullPath(const syntax::ModulePath& path,
                             std::string_view name) {
  syntax::Path out = path;
  out.emplace_back(name);
  return out;
}

static const syntax::RecordDecl* LookupRecordDecl(const ScopeContext& ctx,
                                                  const TypePath& path) {
  syntax::Path syntax_path;
  syntax_path.reserve(path.size());
  for (const auto& comp : path) {
    syntax_path.push_back(comp);
  }
  const auto it = ctx.sigma.types.find(PathKeyOf(syntax_path));
  if (it == ctx.sigma.types.end()) {
    return nullptr;
  }
  return std::get_if<syntax::RecordDecl>(&it->second);
}

static const syntax::EnumDecl* LookupEnumDecl(const ScopeContext& ctx,
                                              const TypePath& path) {
  syntax::Path syntax_path;
  syntax_path.reserve(path.size());
  for (const auto& comp : path) {
    syntax_path.push_back(comp);
  }
  const auto it = ctx.sigma.types.find(PathKeyOf(syntax_path));
  if (it == ctx.sigma.types.end()) {
    return nullptr;
  }
  return std::get_if<syntax::EnumDecl>(&it->second);
}

static std::optional<TypeRef> FieldType(const syntax::RecordDecl& record,
                                        std::string_view field_name,
                                        const ScopeContext& ctx);

static syntax::Visibility FieldVis(const syntax::RecordDecl& record,
                                   std::string_view field_name) {
  SpecDefsTypeExpr();
  for (const auto& member : record.members) {
    if (const auto* field = std::get_if<syntax::FieldDecl>(&member)) {
      if (IdEq(field->name, field_name)) {
        return field->vis;
      }
    }
  }
  return syntax::Visibility::Private;
}

static bool FieldExists(const syntax::RecordDecl& record,
                        std::string_view field_name) {
  for (const auto& member : record.members) {
    if (const auto* field = std::get_if<syntax::FieldDecl>(&member)) {
      if (IdEq(field->name, field_name)) {
        return true;
      }
    }
  }
  return false;
}

static syntax::ModulePath ModuleOfRecordPath(const TypePath& path) {
  if (path.size() <= 1) {
    return {};
  }
  return syntax::ModulePath(path.begin(), path.end() - 1);
}

static bool FieldVisible(const ScopeContext& ctx,
                         const syntax::RecordDecl& record,
                         std::string_view field_name,
                         const TypePath& record_path) {
  SpecDefsTypeExpr();
  const auto vis = FieldVis(record, field_name);
  if (vis == syntax::Visibility::Public ||
      vis == syntax::Visibility::Internal) {
    return true;
  }
  const auto record_module = ModuleOfRecordPath(record_path);
  return PathEq(record_module, ctx.current_module);
}

// Type lowering helpers
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
        } else if constexpr (std::is_same_v<T, syntax::TypeOpaque>) {
          return {true, std::nullopt,
                  MakeTypeOpaque(node.path, type.get(), type->span)};
        } else if constexpr (std::is_same_v<T, syntax::TypeRefine>) {
          const auto base = LowerType(ctx, node.base);
          if (!base.ok) {
            return base;
          }
          return {true, std::nullopt,
                  MakeTypeRefine(base.type, node.predicate)};
        } else if constexpr (std::is_same_v<T, syntax::TypeModalState>) {
          std::vector<TypeRef> args;
          args.reserve(node.generic_args.size());
          for (const auto& arg : node.generic_args) {
            const auto lower_result = LowerType(ctx, arg);
            if (!lower_result.ok) {
              return lower_result;
            }
            args.push_back(lower_result.type);
          }
          return {true, std::nullopt,
                  MakeTypeModalState(node.path, node.state, std::move(args))};
        } else if constexpr (std::is_same_v<T, syntax::TypePathType>) {
          auto is_builtin_generic = [&](const syntax::TypePath& path) {
            if (path.size() != 1) {
              return false;
            }
            return IdEq(path[0], "SpawnHandle") ||
                   IdEq(path[0], "FutureHandle") ||
                   IdEq(path[0], "Async") ||
                   IdEq(path[0], "Sequence") ||
                   IdEq(path[0], "Future") ||
                   IdEq(path[0], "Stream") ||
                   IdEq(path[0], "Pipe") ||
                   IdEq(path[0], "Exchange") ||
                   IdEq(path[0], "Ptr");
          };
          if (!node.generic_args.empty() && is_builtin_generic(node.path)) {
            std::vector<TypeRef> lowered_args;
            for (const auto& arg : node.generic_args) {
              const auto lower_result = LowerType(ctx, arg);
              if (!lower_result.ok) {
                return lower_result;
              }
              lowered_args.push_back(lower_result.type);
            }
            TypePathType result_type;
            result_type.path = node.path;
            result_type.generic_args = std::move(lowered_args);
            return {true, std::nullopt, MakeType(result_type)};
          }
          return {true, std::nullopt, MakeTypePath(node.path)};
        } else {
          return {false, std::nullopt, {}};
        }
      },
      type->node);
}

static std::optional<TypeRef> FieldType(const syntax::RecordDecl& record,
                                        std::string_view field_name,
                                        const ScopeContext& ctx) {
  for (const auto& member : record.members) {
    if (const auto* field = std::get_if<syntax::FieldDecl>(&member)) {
      if (IdEq(field->name, field_name)) {
        const auto lowered = LowerType(ctx, field->type);
        if (lowered.ok) {
          return lowered.type;
        }
        return std::nullopt;
      }
    }
  }
  return std::nullopt;
}

// Operator classification
static bool IsArithOp(std::string_view op) {
  return op == "+" || op == "-" || op == "*" || op == "/" || op == "%" ||
         op == "**";
}

static bool IsBitOp(std::string_view op) {
  return op == "&" || op == "|" || op == "^";
}

static bool IsShiftOp(std::string_view op) {
  return op == "<<" || op == ">>";
}

static bool IsEqOp(std::string_view op) { return op == "==" || op == "!="; }

static bool IsOrdOp(std::string_view op) {
  return op == "<" || op == "<=" || op == ">" || op == ">=";
}

static bool IsSelfTypePath(const TypePath& path) {
  return path.size() == 1 && IdEq(path[0], "Self");
}

static bool IsBuiltinCapClassPath(const TypePath& path) {
  return path.size() == 1 &&
         (IdEq(path[0], "FileSystem") || IdEq(path[0], "HeapAllocator") ||
          IdEq(path[0], "Reactor"));
}

static bool IsLogicOp(std::string_view op) { return op == "&&" || op == "||"; }

//=============================================================================
// Expression Typing Implementations
//=============================================================================

// (T-Ident) and (P-Ident)
ExprTypeResult TypeIdentifierExprImpl(const ScopeContext& ctx,
                                      const syntax::IdentifierExpr& expr,
                                      const TypeEnv& env) {
  SpecDefsTypeExpr();
  ExprTypeResult result;

  const auto binding = BindOf(env, expr.name);
  if (binding.has_value()) {
    if (BitcopyType(ctx, binding->type)) {
      SPEC_RULE("T-Ident");
      result.ok = true;
      result.type = binding->type;
      return result;
    }
    SPEC_RULE("ValueUse-NonBitcopyPlace");
    result.diag_id = "ValueUse-NonBitcopyPlace";
    return result;
  }

  const auto ent = ResolveValueName(ctx, expr.name);
  if (ent.has_value() && ent->origin_opt.has_value()) {
    const auto target_name = ent->target_opt.value_or(expr.name);
    const auto value_type =
        ValuePathType(ctx, *ent->origin_opt, target_name);
    if (!value_type.ok) {
      result.diag_id = value_type.diag_id;
      return result;
    }
    if (value_type.type) {
      if (BitcopyType(ctx, value_type.type)) {
        SPEC_RULE("T-Ident");
        result.ok = true;
        result.type = value_type.type;
        return result;
      }
      SPEC_RULE("ValueUse-NonBitcopyPlace");
      result.diag_id = "ValueUse-NonBitcopyPlace";
      return result;
    }
  }

  result.diag_id = "ResolveExpr-Ident-Err";
  return result;
}

PlaceTypeResult TypeIdentifierPlaceImpl(const ScopeContext& /*ctx*/,
                                        const syntax::IdentifierExpr& expr,
                                        const TypeEnv& env) {
  SpecDefsTypeExpr();
  PlaceTypeResult result;

  const auto binding = BindOf(env, expr.name);
  if (binding.has_value()) {
    SPEC_RULE("P-Ident");
    result.ok = true;
    result.type = binding->type;
    return result;
  }

  result.diag_id = "ResolveExpr-Ident-Err";
  return result;
}

// (T-Path-Value)
ExprTypeResult TypePathExprImpl(const ScopeContext& ctx,
                                const syntax::PathExpr& expr) {
  SpecDefsTypeExpr();
  ExprTypeResult result;

  const auto typed = ValuePathType(ctx, expr.path, expr.name);
  if (!typed.ok) {
    result.diag_id = typed.diag_id;
    return result;
  }
  if (!typed.type) {
    result.diag_id = "ResolveExpr-Ident-Err";
    return result;
  }

  SPEC_RULE("T-Path-Value");
  result.ok = true;
  result.type = typed.type;
  return result;
}

// (T-Field-Record), (T-Field-Record-Perm), (FieldAccess-*)
ExprTypeResult TypeFieldAccessExprImpl(const ScopeContext& ctx,
                                       const StmtTypeContext& type_ctx,
                                       const syntax::FieldAccessExpr& expr,
                                       const TypeEnv& env) {
  SpecDefsTypeExpr();
  ExprTypeResult result;

  // Field access uses place typing for the base expression, since accessing
  // a field doesn't consume the base - it just reads through it. This allows
  // field access on non-Bitcopy types (e.g., self.field in methods).
  const auto place_result = TypePlace(ctx, type_ctx, expr.base, env);
  TypeRef base_type;
  if (place_result.ok) {
    base_type = place_result.type;
  } else {
    // Fall back to value typing for non-place expressions
    const auto base_result = TypeExpr(ctx, type_ctx, expr.base, env);
    if (!base_result.ok) {
      result.diag_id = base_result.diag_id;
      return result;
    }
    base_type = base_result.type;
  }

  const auto stripped = StripPerm(base_type);
  if (!stripped) {
    return result;
  }

  if (std::holds_alternative<TypeUnion>(stripped->node)) {
    SPEC_RULE("Union-DirectAccess-Err");
    result.diag_id = "Union-DirectAccess-Err";
    return result;
  }
  if (std::holds_alternative<TypeOpaque>(stripped->node)) {
    SPEC_RULE("T-Opaque-Project");
    result.diag_id = "E-TYP-2510";
    return result;
  }

  if (const auto* modal_state = std::get_if<TypeModalState>(&stripped->node)) {
    const auto* modal = LookupModalDecl(ctx, modal_state->path);
    if (!modal) {
      return result;
    }
    const auto* field_decl =
        LookupModalFieldDecl(*modal, modal_state->state, expr.name);
    if (!field_decl) {
      SPEC_RULE("Modal-Field-Missing");
      result.diag_id = "Modal-Field-Missing";
      return result;
    }
    if (!ModalFieldVisible(ctx, modal_state->path)) {
      SPEC_RULE("Modal-Field-NotVisible");
      result.diag_id = "Modal-Field-NotVisible";
      return result;
    }
    const auto lowered = LowerType(ctx, field_decl->type);
    if (!lowered.ok) {
      return result;
    }
    TypeRef field_type = lowered.type;
    if (const auto* perm_type = std::get_if<TypePerm>(&base_type->node)) {
      field_type = MakeTypePerm(perm_type->perm, field_type);
      if (BitcopyType(ctx, field_type)) {
        SPEC_RULE("T-Modal-Field-Perm");
        result.ok = true;
        result.type = field_type;
        return result;
      }
      SPEC_RULE("ValueUse-NonBitcopyPlace");
      result.diag_id = "ValueUse-NonBitcopyPlace";
      return result;
    }

    if (BitcopyType(ctx, field_type)) {
      SPEC_RULE("T-Modal-Field");
      result.ok = true;
      result.type = field_type;
      return result;
    }

    SPEC_RULE("ValueUse-NonBitcopyPlace");
    result.diag_id = "ValueUse-NonBitcopyPlace";
    return result;
  }

  const auto* path_type = std::get_if<TypePathType>(&stripped->node);
  if (!path_type) {
    return result;
  }

  if (LookupModalDecl(ctx, path_type->path)) {
    SPEC_RULE("Modal-Field-General-Err");
    result.diag_id = "Modal-Field-General-Err";
    return result;
  }

  if (LookupEnumDecl(ctx, path_type->path)) {
    SPEC_RULE("FieldAccess-Enum");
    result.diag_id = "FieldAccess-Unknown";
    return result;
  }

  const auto* record = LookupRecordDecl(ctx, path_type->path);
  if (!record) {
    return result;
  }

  if (!FieldExists(*record, expr.name)) {
    SPEC_RULE("FieldAccess-Unknown");
    result.diag_id = "FieldAccess-Unknown";
    return result;
  }

  if (!FieldVisible(ctx, *record, expr.name, path_type->path)) {
    SPEC_RULE("FieldAccess-NotVisible");
    result.diag_id = "FieldAccess-NotVisible";
    return result;
  }

  const auto field_type_opt = FieldType(*record, expr.name, ctx);
  if (!field_type_opt.has_value()) {
    return result;
  }

  TypeRef field_type = *field_type_opt;

  if (const auto* perm_type = std::get_if<TypePerm>(&base_type->node)) {
    field_type = MakeTypePerm(perm_type->perm, field_type);
    if (BitcopyType(ctx, field_type)) {
      SPEC_RULE("T-Field-Record-Perm");
      result.ok = true;
      result.type = field_type;
      return result;
    }
    SPEC_RULE("ValueUse-NonBitcopyPlace");
    result.diag_id = "ValueUse-NonBitcopyPlace";
    return result;
  }

  if (BitcopyType(ctx, field_type)) {
    SPEC_RULE("T-Field-Record");
    result.ok = true;
    result.type = field_type;
    return result;
  }

  SPEC_RULE("ValueUse-NonBitcopyPlace");
  result.diag_id = "ValueUse-NonBitcopyPlace";
  return result;
}


PlaceTypeResult TypeFieldAccessPlaceImpl(const ScopeContext& ctx,
                                         const StmtTypeContext& type_ctx,
                                         const syntax::FieldAccessExpr& expr,
                                         const TypeEnv& env) {
  SpecDefsTypeExpr();
  PlaceTypeResult result;

  const auto base_result = TypePlace(ctx, type_ctx, expr.base, env);
  if (!base_result.ok) {
    result.diag_id = base_result.diag_id;
    return result;
  }

  const auto stripped = StripPerm(base_result.type);
  if (!stripped) {
    return result;
  }

  if (std::holds_alternative<TypeUnion>(stripped->node)) {
    SPEC_RULE("Union-DirectAccess-Err");
    result.diag_id = "Union-DirectAccess-Err";
    return result;
  }

  if (const auto* modal_state = std::get_if<TypeModalState>(&stripped->node)) {
    const auto* modal = LookupModalDecl(ctx, modal_state->path);
    if (!modal) {
      return result;
    }
    const auto* field_decl =
        LookupModalFieldDecl(*modal, modal_state->state, expr.name);
    if (!field_decl) {
      SPEC_RULE("Modal-Field-Missing");
      result.diag_id = "Modal-Field-Missing";
      return result;
    }
    if (!ModalFieldVisible(ctx, modal_state->path)) {
      SPEC_RULE("Modal-Field-NotVisible");
      result.diag_id = "Modal-Field-NotVisible";
      return result;
    }
    const auto lowered = LowerType(ctx, field_decl->type);
    if (!lowered.ok) {
      return result;
    }
    TypeRef field_type = lowered.type;
    if (const auto* perm_type = std::get_if<TypePerm>(&base_result.type->node)) {
      field_type = MakeTypePerm(perm_type->perm, field_type);
    }
    result.ok = true;
    result.type = field_type;
    return result;
  }

  const auto* path_type = std::get_if<TypePathType>(&stripped->node);
  if (!path_type) {
    return result;
  }

  if (LookupModalDecl(ctx, path_type->path)) {
    SPEC_RULE("Modal-Field-General-Err");
    result.diag_id = "Modal-Field-General-Err";
    return result;
  }

  if (LookupEnumDecl(ctx, path_type->path)) {
    SPEC_RULE("FieldAccess-Enum");
    result.diag_id = "FieldAccess-Unknown";
    return result;
  }

  const auto* record = LookupRecordDecl(ctx, path_type->path);
  if (!record) {
    return result;
  }

  if (!FieldExists(*record, expr.name)) {
    SPEC_RULE("FieldAccess-Unknown");
    result.diag_id = "FieldAccess-Unknown";
    return result;
  }

  if (!FieldVisible(ctx, *record, expr.name, path_type->path)) {
    SPEC_RULE("FieldAccess-NotVisible");
    result.diag_id = "FieldAccess-NotVisible";
    return result;
  }

  const auto field_type_opt = FieldType(*record, expr.name, ctx);
  if (!field_type_opt.has_value()) {
    return result;
  }

  TypeRef field_type = *field_type_opt;

  if (const auto* perm_type = std::get_if<TypePerm>(&base_result.type->node)) {
    field_type = MakeTypePerm(perm_type->perm, field_type);
    SPEC_RULE("P-Field-Record-Perm");
  } else {
    SPEC_RULE("P-Field-Record");
  }

  result.ok = true;
  result.type = field_type;
  return result;
}


static void WarnWidenLargePayload(const ScopeContext& ctx,
                                  const StmtTypeContext& type_ctx,
                                  const core::Span& span,
                                  const TypePath& modal_path,
                                  std::string_view state) {
  SpecDefsTypeExpr();
  if (!WidenWarnCond(ctx, modal_path, state)) {
    SPEC_RULE("Warn-Widen-Ok");
    return;
  }
  SPEC_RULE("Warn-Widen-LargePayload");
  if (!type_ctx.diags) {
    return;
  }
  if (auto diag = core::MakeDiagnostic("W-SYS-4010", span)) {
    *type_ctx.diags = core::Emit(*type_ctx.diags, *diag);
  }
}


// (T-Not-Bool), (T-Not-Int), (T-Neg)
ExprTypeResult TypeUnaryExprImpl(const ScopeContext& ctx,
                                 const StmtTypeContext& type_ctx,
                                 const syntax::UnaryExpr& expr,
                                 const TypeEnv& env,
                                 const core::Span& span) {
  SpecDefsTypeExpr();
  ExprTypeResult result;

  const auto operand = TypeExpr(ctx, type_ctx, expr.value, env);
  if (!operand.ok) {
    result.diag_id = operand.diag_id;
    return result;
  }

  const std::string_view op = expr.op;
  if (op == "widen") {
    const auto stripped = StripPerm(operand.type);
    if (!stripped) {
      return result;
    }
    if (const auto* modal = std::get_if<TypeModalState>(&stripped->node)) {
      const auto* decl = LookupModalDecl(ctx, modal->path);
      if (!decl || !HasState(*decl, modal->state)) {
        return result;
      }
      WarnWidenLargePayload(ctx, type_ctx, span, modal->path, modal->state);
      if (const auto* perm = std::get_if<TypePerm>(&operand.type->node)) {
        SPEC_RULE("T-Modal-Widen-Perm");
        result.ok = true;
        result.type = MakeTypePerm(perm->perm, MakeTypePath(modal->path));
        return result;
      }
      SPEC_RULE("T-Modal-Widen");
      result.ok = true;
      result.type = MakeTypePath(modal->path);
      return result;
    }
    if (const auto* path = std::get_if<TypePathType>(&stripped->node)) {
      if (LookupModalDecl(ctx, path->path)) {
        SPEC_RULE("Widen-AlreadyGeneral");
        result.diag_id = "Widen-AlreadyGeneral";
        return result;
      }
    }
    SPEC_RULE("Widen-NonModal");
    result.diag_id = "Widen-NonModal";
    return result;
  }

  const auto* prim = std::get_if<TypePrim>(&operand.type->node);
  if (!prim) {
    return result;
  }
  const std::string_view name = prim->name;
  if (op == "!") {
    if (name == "bool") {
      SPEC_RULE("T-Not-Bool");
      result.ok = true;
      result.type = MakeTypePrim("bool");
      return result;
    }
    if (IsIntType(name)) {
      SPEC_RULE("T-Not-Int");
      result.ok = true;
      result.type = MakeTypePrim(std::string(name));
      return result;
    }
    return result;
  }

  if (op == "-") {
    if (IsSignedIntType(name) || IsFloatType(name)) {
      SPEC_RULE("T-Neg");
      result.ok = true;
      result.type = MakeTypePrim(std::string(name));
      return result;
    }
    return result;
  }

  return result;
}


// (T-Arith), (T-Bitwise), (T-Shift), (T-Compare-Eq), (T-Compare-Ord),
// (T-Logical)
ExprTypeResult TypeBinaryExprImpl(const ScopeContext& ctx,
                                  const StmtTypeContext& type_ctx,
                                  const syntax::BinaryExpr& expr,
                                  const TypeEnv& env) {
  SpecDefsTypeExpr();
  ExprTypeResult result;

  const auto lhs = TypeExpr(ctx, type_ctx, expr.lhs, env);
  if (!lhs.ok) {
    result.diag_id = lhs.diag_id;
    return result;
  }

  const auto rhs = TypeExpr(ctx, type_ctx, expr.rhs, env);
  if (!rhs.ok) {
    result.diag_id = rhs.diag_id;
    return result;
  }

  const auto lhs_name = GetPrimName(lhs.type);
  const auto rhs_name = GetPrimName(rhs.type);
  const std::string_view op = expr.op;

  if (IsArithOp(op)) {
    if (!lhs_name.has_value() || !rhs_name.has_value()) {
      return result;
    }
    const auto equiv = TypeEquiv(lhs.type, rhs.type);
    if (!equiv.ok) {
      result.diag_id = equiv.diag_id;
      return result;
    }
    if (!equiv.equiv) {
      return result;
    }
    if (!IsNumericType(*lhs_name)) {
      return result;
    }
    SPEC_RULE("T-Arith");
    result.ok = true;
    result.type = MakeTypePrim(std::string(*lhs_name));
    return result;
  }

  if (IsBitOp(op)) {
    if (!lhs_name.has_value() || !rhs_name.has_value()) {
      return result;
    }
    const auto equiv = TypeEquiv(lhs.type, rhs.type);
    if (!equiv.ok) {
      result.diag_id = equiv.diag_id;
      return result;
    }
    if (!equiv.equiv) {
      return result;
    }
    if (!IsIntType(*lhs_name)) {
      return result;
    }
    SPEC_RULE("T-Bitwise");
    result.ok = true;
    result.type = MakeTypePrim(std::string(*lhs_name));
    return result;
  }

  if (IsShiftOp(op)) {
    if (!lhs_name.has_value()) {
      return result;
    }
    if (!IsIntType(*lhs_name)) {
      return result;
    }
    if (!IsPrimType(rhs.type, "u32")) {
      return result;
    }
    SPEC_RULE("T-Shift");
    result.ok = true;
    result.type = MakeTypePrim(std::string(*lhs_name));
    return result;
  }

  if (IsEqOp(op)) {
    if (!EqType(lhs.type)) {
      return result;
    }
    const auto equiv = TypeEquiv(lhs.type, rhs.type);
    if (!equiv.ok) {
      result.diag_id = equiv.diag_id;
      return result;
    }
    if (!equiv.equiv) {
      return result;
    }
    SPEC_RULE("T-Compare-Eq");
    result.ok = true;
    result.type = MakeTypePrim("bool");
    return result;
  }

  if (IsOrdOp(op)) {
    if (!OrdType(lhs.type)) {
      return result;
    }
    const auto equiv = TypeEquiv(lhs.type, rhs.type);
    if (!equiv.ok) {
      result.diag_id = equiv.diag_id;
      return result;
    }
    if (!equiv.equiv) {
      return result;
    }
    SPEC_RULE("T-Compare-Ord");
    result.ok = true;
    result.type = MakeTypePrim("bool");
    return result;
  }

  if (IsLogicOp(op)) {
    if (!IsPrimType(lhs.type, "bool") || !IsPrimType(rhs.type, "bool")) {
      return result;
    }
    SPEC_RULE("T-Logical");
    result.ok = true;
    result.type = MakeTypePrim("bool");
    return result;
  }

  return result;
}


// (T-Cast)
ExprTypeResult TypeCastExprImpl(const ScopeContext& ctx,
                                const StmtTypeContext& type_ctx,
                                const syntax::CastExpr& expr,
                                const TypeEnv& env) {
  SpecDefsTypeExpr();
  ExprTypeResult result;

  const auto target = LowerType(ctx, expr.type);
  if (!target.ok) {
    result.diag_id = target.diag_id;
    return result;
  }
  const auto target_wf = TypeWF(ctx, target.type);
  if (!target_wf.ok) {
    result.diag_id = target_wf.diag_id;
    return result;
  }

  if (std::holds_alternative<TypeDynamic>(target.type->node)) {
    const auto* dyn = std::get_if<TypeDynamic>(&target.type->node);
    if (!dyn) {
      return result;
    }
    if (ctx.sigma.classes.find(PathKeyOf(dyn->path)) == ctx.sigma.classes.end()) {
      result.diag_id = "Superclass-Undefined";
      return result;
    }

    const auto place = TypePlace(ctx, type_ctx, expr.value, env);
    if (!place.ok) {
      result.diag_id = place.diag_id;
      return result;
    }

    auto type_expr = [&](const syntax::ExprPtr& inner) {
      return TypeExpr(ctx, type_ctx, inner, env);
    };
    const auto addr_ok = AddrOfOk(expr.value, type_expr);
    if (!addr_ok.ok) {
      result.diag_id = addr_ok.diag_id;
      return result;
    }

    const auto stripped = StripPerm(place.type);
    if (!TypeImplementsClass(ctx, stripped, dyn->path)) {
      return result;
    }

    if (!ClassDispatchable(ctx, dyn->path)) {
      SPEC_RULE("Dynamic-NonDispatchable");
      result.diag_id = "Dynamic-NonDispatchable";
      return result;
    }

    SPEC_RULE("T-Dynamic-Form");
    result.ok = true;
    result.type = target.type;
    return result;
  }


  const auto source = TypeExpr(ctx, type_ctx, expr.value, env);
  if (!source.ok) {
    result.diag_id = source.diag_id;
    return result;
  }

  if (const auto* refine = std::get_if<TypeRefine>(&target.type->node)) {
    if (!CastValid(source.type, refine->base)) {
      return result;
    }
    if (!ProveRefinePredicate(expr.value, *refine, result.diag_id)) {
      return result;
    }
    SPEC_RULE("T-Refine-Intro");
    result.ok = true;
    result.type = target.type;
    return result;
  }

  if (!CastValid(source.type, target.type)) {
    return result;
  }

  SPEC_RULE("T-Cast");
  result.ok = true;
  result.type = target.type;
  return result;
}


// (T-If), (T-If-No-Else)
ExprTypeResult TypeIfExprImpl(const ScopeContext& ctx,
                              const StmtTypeContext& type_ctx,
                              const syntax::IfExpr& expr,
                              const TypeEnv& env) {
  SpecDefsTypeExpr();
  ExprTypeResult result;

  const auto cond = TypeExpr(ctx, type_ctx, expr.cond, env);
  if (!cond.ok) {
    result.diag_id = cond.diag_id;
    return result;
  }
  if (!IsPrimType(cond.type, "bool")) {
    return result;
  }

  const auto then_result = TypeExpr(ctx, type_ctx, expr.then_expr, env);
  if (!then_result.ok) {
    result.diag_id = then_result.diag_id;
    return result;
  }

  if (!expr.else_expr) {
    if (!IsPrimType(then_result.type, "()")) {
      return result;
    }
    SPEC_RULE("T-If-No-Else");
    result.ok = true;
    result.type = MakeTypePrim("()");
    return result;
  }

  const auto else_result = TypeExpr(ctx, type_ctx, expr.else_expr, env);
  if (!else_result.ok) {
    result.diag_id = else_result.diag_id;
    return result;
  }

  const auto equiv = TypeEquiv(then_result.type, else_result.type);
  if (!equiv.ok) {
    result.diag_id = equiv.diag_id;
    return result;
  }
  if (!equiv.equiv) {
    return result;
  }

  SPEC_RULE("T-If");
  result.ok = true;
  result.type = then_result.type;
  return result;
}


// (Chk-If), (Chk-If-No-Else)
CheckResult CheckIfExprImpl(const ScopeContext& ctx,
                            const StmtTypeContext& type_ctx,
                            const syntax::IfExpr& expr,
                            const TypeRef& expected,
                            const TypeEnv& env) {
  SpecDefsTypeExpr();
  CheckResult result;

  const auto cond = TypeExpr(ctx, type_ctx, expr.cond, env);
  if (!cond.ok) {
    result.diag_id = cond.diag_id;
    return result;
  }
  if (!IsPrimType(cond.type, "bool")) {
    return result;
  }

  const auto then_check =
      CheckExprAgainst(ctx, type_ctx, expr.then_expr, expected, env);
  if (!then_check.ok) {
    result.diag_id = then_check.diag_id;
    return result;
  }

  if (!expr.else_expr) {
    if (!IsPrimType(expected, "()")) {
      return result;
    }
    SPEC_RULE("Chk-If-No-Else");
    result.ok = true;
    return result;
  }

  const auto else_check =
      CheckExprAgainst(ctx, type_ctx, expr.else_expr, expected, env);
  if (!else_check.ok) {
    result.diag_id = else_check.diag_id;
    return result;
  }

  SPEC_RULE("Chk-If");
  result.ok = true;
  return result;
}


// Range expressions (Range-Full, Range-To, etc.)
ExprTypeResult TypeRangeExprImpl(const ScopeContext& ctx,
                                 const StmtTypeContext& type_ctx,
                                 const syntax::RangeExpr& expr,
                                 const TypeEnv& env) {
  SpecDefsTypeExpr();
  ExprTypeResult result;

  // Helper to check if type is an integer type (for C0X dispatch ranges)
  auto is_integer_type = [](const TypeRef& type) -> bool {
    if (!type) return false;
    const auto* prim = std::get_if<TypePrim>(&type->node);
    if (!prim) return false;
    return prim->name == "usize" || prim->name == "isize" ||
           prim->name == "i8" || prim->name == "i16" ||
           prim->name == "i32" || prim->name == "i64" ||
           prim->name == "u8" || prim->name == "u16" ||
           prim->name == "u32" || prim->name == "u64";
  };

  // Check bounds are usize (or integer types in parallel context for C0X)
  if (expr.lhs) {
    const auto start = TypeExpr(ctx, type_ctx, expr.lhs, env);
    if (!start.ok) {
      result.diag_id = start.diag_id;
      return result;
    }
    // C0X Extension: Allow integer types in parallel context for dispatch
    if (!IsPrimType(start.type, "usize") &&
        !(type_ctx.in_parallel && is_integer_type(start.type))) {
      SPEC_RULE("Range-NonIndex-Err");
      result.diag_id = "Range-NonIndex-Err";
      return result;
    }
  }

  if (expr.rhs) {
    const auto end = TypeExpr(ctx, type_ctx, expr.rhs, env);
    if (!end.ok) {
      result.diag_id = end.diag_id;
      return result;
    }
    // C0X Extension: Allow integer types in parallel context for dispatch
    if (!IsPrimType(end.type, "usize") &&
        !(type_ctx.in_parallel && is_integer_type(end.type))) {
      SPEC_RULE("Range-NonIndex-Err");
      result.diag_id = "Range-NonIndex-Err";
      return result;
    }
  }

  // Determine range kind and apply appropriate rule
  switch (expr.kind) {
    case syntax::RangeKind::Full:
      SPEC_RULE("Range-Full");
      break;
    case syntax::RangeKind::To:
      SPEC_RULE("Range-To");
      break;
    case syntax::RangeKind::ToInclusive:
      SPEC_RULE("Range-ToInclusive");
      break;
    case syntax::RangeKind::From:
      SPEC_RULE("Range-From");
      break;
    case syntax::RangeKind::Exclusive:
      SPEC_RULE("Range-Exclusive");
      break;
    case syntax::RangeKind::Inclusive:
      SPEC_RULE("Range-Inclusive");
      break;
  }

  result.ok = true;
  result.type = MakeTypeRange();
  return result;
}

// (T-AddrOf)
ExprTypeResult TypeAddressOfExprImpl(const ScopeContext& ctx,
                                     const StmtTypeContext& type_ctx,
                                     const syntax::AddressOfExpr& expr,
                                     const TypeEnv& env) {
  SpecDefsTypeExpr();
  ExprTypeResult result;

  // Must be a place expression
  if (!IsPlaceExpr(expr.place)) {
    SPEC_RULE("AddrOf-NonPlace");
    result.diag_id = "AddrOf-NonPlace";
    return result;
  }

  if (expr.place) {
    if (const auto* index = std::get_if<syntax::IndexAccessExpr>(&expr.place->node)) {
      const auto idx_type = TypeExpr(ctx, type_ctx, index->index, env);
      if (!idx_type.ok) {
        result.diag_id = idx_type.diag_id;
        return result;
      }
      const auto idx_stripped = StripPerm(idx_type.type);
      if (!IsPrimType(idx_stripped, "usize")) {
        const auto base_type = TypePlace(ctx, type_ctx, index->base, env);
        if (!base_type.ok) {
          result.diag_id = base_type.diag_id;
          return result;
        }
        const auto stripped = StripPerm(base_type.type);
        if (stripped && std::holds_alternative<TypeArray>(stripped->node)) {
          SPEC_RULE("AddrOf-Index-Array-NonUsize");
          result.diag_id = "Index-Array-NonUsize";
          return result;
        }
        if (stripped && std::holds_alternative<TypeSlice>(stripped->node)) {
          SPEC_RULE("AddrOf-Index-Slice-NonUsize");
          result.diag_id = "Index-Slice-NonUsize";
          return result;
        }
        result.diag_id = "Index-NonIndexable";
        return result;
      }

      if (!index->index ||
          !std::holds_alternative<syntax::RangeExpr>(index->index->node)) {
        const auto base_place = TypePlace(ctx, type_ctx, index->base, env);
        if (!base_place.ok) {
          result.diag_id = base_place.diag_id;
          return result;
        }
        const auto stripped = StripPerm(base_place.type);
        if (const auto* array = stripped ? std::get_if<TypeArray>(&stripped->node)
                                         : nullptr) {
          const auto index_const = ConstLen(ctx, index->index);
          if (!index_const.ok || !index_const.value.has_value()) {
            result.diag_id = "Index-Array-NonConst-Err";
            return result;
          }
          if (*index_const.value >= array->length) {
            result.diag_id = "Index-Array-OOB-Err";
            return result;
          }
          TypeRef out_type = array->element;
          if (const auto* perm = std::get_if<TypePerm>(&base_place.type->node)) {
            out_type = MakeTypePerm(perm->perm, out_type);
          }
          SPEC_RULE("T-AddrOf");
          result.ok = true;
          result.type = MakeTypePtr(out_type, PtrState::Valid);
          return result;
        }
      }
    }
  }

  const auto place = TypePlace(ctx, type_ctx, expr.place, env);
  if (!place.ok) {
    result.diag_id = place.diag_id;
    return result;
  }

  SPEC_RULE("T-AddrOf");
  result.ok = true;
  result.type = MakeTypePtr(place.type, PtrState::Valid);
  return result;
}

// (T-Deref-Ptr), (T-Deref-Raw)
ExprTypeResult TypeDerefExprImpl(const ScopeContext& ctx,
                                 const StmtTypeContext& type_ctx,
                                 const syntax::DerefExpr& expr,
                                 const TypeEnv& env,
                                 const core::Span& span) {
  SpecDefsTypeExpr();
  ExprTypeResult result;

  const auto ptr = TypeExpr(ctx, type_ctx, expr.value, env);
  if (!ptr.ok) {
    result.diag_id = ptr.diag_id;
    return result;
  }

  const auto stripped = StripPerm(ptr.type);
  if (!stripped) {
    return result;
  }

  if (const auto* type_ptr = std::get_if<TypePtr>(&stripped->node)) {
    if (type_ptr->state == PtrState::Null) {
      SPEC_RULE("Deref-Null");
      result.diag_id = "Deref-Null";
      return result;
    }
    if (type_ptr->state == PtrState::Expired) {
      SPEC_RULE("Deref-Expired");
      result.diag_id = "Deref-Expired";
      return result;
    }
    if (!BitcopyType(ctx, type_ptr->element)) {
      SPEC_RULE("ValueUse-NonBitcopyPlace");
      result.diag_id = "ValueUse-NonBitcopyPlace";
      return result;
    }
    SPEC_RULE("T-Deref-Ptr");
    result.ok = true;
    result.type = type_ptr->element;
    return result;
  }

  if (const auto* raw_ptr = std::get_if<TypeRawPtr>(&stripped->node)) {
    if (!IsInUnsafeSpan(ctx, span)) {
      SPEC_RULE("Deref-Raw-Unsafe");
      result.diag_id = "Deref-Raw-Unsafe";
      return result;
    }
    if (!BitcopyType(ctx, raw_ptr->element)) {
      SPEC_RULE("ValueUse-NonBitcopyPlace");
      result.diag_id = "ValueUse-NonBitcopyPlace";
      return result;
    }
    SPEC_RULE("T-Deref-Raw");
    result.ok = true;
    result.type = raw_ptr->element;
    return result;
  }

  return result;
}


// (P-Deref-Ptr), (P-Deref-Raw-Imm), (P-Deref-Raw-Mut)
PlaceTypeResult TypeDerefPlaceImpl(const ScopeContext& ctx,
                                   const StmtTypeContext& type_ctx,
                                   const syntax::DerefExpr& expr,
                                   const TypeEnv& env,
                                   const core::Span& span) {
  SpecDefsTypeExpr();
  PlaceTypeResult result;

  const auto ptr = TypeExpr(ctx, type_ctx, expr.value, env);
  if (!ptr.ok) {
    result.diag_id = ptr.diag_id;
    return result;
  }

  const auto stripped = StripPerm(ptr.type);
  if (!stripped) {
    return result;
  }

  if (const auto* type_ptr = std::get_if<TypePtr>(&stripped->node)) {
    if (type_ptr->state == PtrState::Null) {
      SPEC_RULE("Deref-Null");
      result.diag_id = "Deref-Null";
      return result;
    }
    if (type_ptr->state == PtrState::Expired) {
      SPEC_RULE("Deref-Expired");
      result.diag_id = "Deref-Expired";
      return result;
    }
    SPEC_RULE("P-Deref-Ptr");
    result.ok = true;
    result.type = type_ptr->element;
    return result;
  }

  if (const auto* raw_ptr = std::get_if<TypeRawPtr>(&stripped->node)) {
    if (!IsInUnsafeSpan(ctx, span)) {
      SPEC_RULE("Deref-Raw-Unsafe");
      result.diag_id = "Deref-Raw-Unsafe";
      return result;
    }
    if (raw_ptr->qual == RawPtrQual::Imm) {
      SPEC_RULE("P-Deref-Raw-Imm");
      result.ok = true;
      result.type = MakeTypePerm(Permission::Const, raw_ptr->element);
      return result;
    }
    SPEC_RULE("P-Deref-Raw-Mut");
    result.ok = true;
    result.type = MakeTypePerm(Permission::Unique, raw_ptr->element);
    return result;
  }

  return result;
}


// (T-Move)
ExprTypeResult TypeMoveExprImpl(const ScopeContext& ctx,
                                const StmtTypeContext& type_ctx,
                                const syntax::MoveExpr& expr,
                                const TypeEnv& env) {
  SpecDefsTypeExpr();
  ExprTypeResult result;
  if (type_ctx.require_pure) {
    result.diag_id = "E-SEM-2802";
    return result;
  }

  const auto place = TypePlace(ctx, type_ctx, expr.place, env);
  if (!place.ok) {
    result.diag_id = place.diag_id;
    return result;
  }

  SPEC_RULE("T-Move");
  result.ok = true;
  result.type = place.type;
  return result;
}

// (T-ErrorExpr)
ExprTypeResult TypeErrorExprImpl(const ScopeContext& /*ctx*/,
                                 const syntax::ErrorExpr& /*expr*/) {
  SpecDefsTypeExpr();
  ExprTypeResult result;
  SPEC_RULE("T-ErrorExpr");
  result.ok = true;
  result.type = MakeTypePrim("!");
  return result;
}

// (T-Propagate)


ExprTypeResult TypePropagateExprImpl(const ScopeContext& ctx,
                                     const StmtTypeContext& type_ctx,
                                     const syntax::PropagateExpr& expr,
                                     const TypeEnv& env) {
  SpecDefsTypeExpr();
  ExprTypeResult result;

  const auto inner = TypeExpr(ctx, type_ctx, expr.value, env);
  if (!inner.ok) {
    result.diag_id = inner.diag_id;
    return result;
  }

  const auto stripped = StripPerm(inner.type);
  if (!stripped) {
    return result;
  }

  const auto* union_type = std::get_if<TypeUnion>(&stripped->node);
  if (!union_type) {
    return result;
  }

  if (union_type->members.empty()) {
    return result;
  }

  if (!type_ctx.return_type) {
    return result;
  }

  std::optional<TypeRef> success;
  for (const auto& member : union_type->members) {
    const auto sub = Subtyping(ctx, member, type_ctx.return_type);
    if (!sub.ok) {
      result.diag_id = sub.diag_id;
      return result;
    }
    if (sub.subtype) {
      continue;
    }
    if (success.has_value()) {
      return result;
    }
    success = member;
  }

  if (!success.has_value()) {
    return result;
  }

  SPEC_RULE("T-Propagate");
  result.ok = true;
  result.type = *success;
  return result;
}


// Tuple access typing
ExprTypeResult TypeTupleAccessExprImpl(const ScopeContext& ctx,
                                       const StmtTypeContext& type_ctx,
                                       const syntax::TupleAccessExpr& expr,
                                       const TypeEnv& env) {
  SpecDefsTypeExpr();
  auto type_expr = [&](const syntax::ExprPtr& inner) {
    return TypeExpr(ctx, type_ctx, inner, env);
  };
  PlaceTypeFn type_place = [&](const syntax::ExprPtr& inner) {
    return TypePlace(ctx, type_ctx, inner, env);
  };
  auto lower_type = [&](const std::shared_ptr<syntax::Type>& type) -> LowerTypeResult {
    const auto lowered = LowerType(ctx, type);
    if (!lowered.ok) {
      return {false, lowered.diag_id, {}};
    }
    return {true, std::nullopt, lowered.type};
  };
  return TypeTupleAccessValue(ctx, expr, type_expr);
}

PlaceTypeResult TypeTupleAccessPlaceImpl(const ScopeContext& ctx,
                                         const StmtTypeContext& type_ctx,
                                         const syntax::TupleAccessExpr& expr,
                                         const TypeEnv& env) {
  SpecDefsTypeExpr();
  PlaceTypeFn type_place = [&](const syntax::ExprPtr& inner) {
    return TypePlace(ctx, type_ctx, inner, env);
  };
  return TypeTupleAccessPlace(ctx, expr, type_place);
}

// Record literal typing
ExprTypeResult TypeRecordExprImpl(const ScopeContext& ctx,
                                  const StmtTypeContext& type_ctx,
                                  const syntax::RecordExpr& expr,
                                  const TypeEnv& env) {
  SpecDefsTypeExpr();
  ExprTypeResult result;

  if (const auto* modal = std::get_if<syntax::ModalStateRef>(&expr.target)) {
    if (modal->path.size() == 1 &&
        (modal->path[0] == "File" || modal->path[0] == "DirIter")) {
      SPEC_RULE("Record-FileDir-Err");
      result.diag_id = "Record-FileDir-Err";
      return result;
    }
    const auto* decl = LookupModalDecl(ctx, modal->path);
    if (!decl) {
      return result;
    }
    const auto* state = LookupModalState(*decl, modal->state);
    if (!state) {
      return result;
    }

    SPEC_RULE("WF-ModalState");
    SPEC_RULE("State-Specific-WF");

    std::unordered_set<IdKey> seen;
    for (const auto& field_init : expr.fields) {
      const auto key = IdKeyOf(field_init.name);
      if (!seen.insert(key).second) {
        SPEC_RULE("Record-FieldInit-Dup");
        result.diag_id = "Record-FieldInit-Dup";
        return result;
      }
    }

    std::unordered_map<IdKey, const syntax::StateFieldDecl*> payload_fields;
    for (const auto& member : state->members) {
      if (const auto* field = std::get_if<syntax::StateFieldDecl>(&member)) {
        payload_fields.emplace(IdKeyOf(field->name), field);
      }
    }

    for (const auto& field_init : expr.fields) {
      if (payload_fields.find(IdKeyOf(field_init.name)) == payload_fields.end()) {
        SPEC_RULE("Record-Field-Unknown");
        result.diag_id = "Record-Field-Unknown";
        return result;
      }
    }

    for (const auto& member : state->members) {
      if (const auto* field = std::get_if<syntax::StateFieldDecl>(&member)) {
        const auto key = IdKeyOf(field->name);
        if (seen.find(key) == seen.end()) {
          SPEC_RULE("Record-FieldInit-Missing");
          result.diag_id = "Record-FieldInit-Missing";
          return result;
        }
      }
    }

    auto type_expr = [&](const syntax::ExprPtr& inner) {
      return TypeExpr(ctx, type_ctx, inner, env);
    };
    auto type_ident = [&](std::string_view name) -> ExprTypeResult {
      return TypeIdentifierExprImpl(ctx, syntax::IdentifierExpr{std::string(name)},
                                    env);
    };
    PlaceTypeFn type_place = [&](const syntax::ExprPtr& inner) {
      return TypePlace(ctx, type_ctx, inner, env);
    };
    auto match_check = [&](const syntax::MatchExpr& match,
                           const TypeRef& expected) -> CheckResult {
      return CheckMatchExpr(ctx, type_ctx, match, env, expected);
    };

    for (const auto& field_init : expr.fields) {
      const auto it = payload_fields.find(IdKeyOf(field_init.name));
      if (it == payload_fields.end() || !it->second) {
        return result;
      }
      const auto lowered = LowerType(ctx, it->second->type);
      if (!lowered.ok) {
        result.diag_id = lowered.diag_id;
        return result;
      }
      const auto check = CheckExpr(
          ctx, field_init.value, lowered.type, type_expr, type_place, type_ident,
          match_check);
      if (!check.ok) {
        result.diag_id = check.diag_id;
        return result;
      }
    }

    SPEC_RULE("T-Modal-State-Intro");
    result.ok = true;
    result.type = MakeTypeModalState(modal->path, modal->state);
    return result;
  }

  const auto* path = std::get_if<syntax::TypePath>(&expr.target);
  if (!path) {
    return result;
  }

  const TypePath type_path = *path;
  const auto* record = LookupRecordDecl(ctx, type_path);
  if (!record) {
    return result;
  }

  std::unordered_set<IdKey> seen;
  for (const auto& field_init : expr.fields) {
    const auto key = IdKeyOf(field_init.name);
    if (!seen.insert(key).second) {
      SPEC_RULE("Record-FieldInit-Dup");
      result.diag_id = "Record-FieldInit-Dup";
      return result;
    }
  }

  for (const auto& field_init : expr.fields) {
    if (!FieldExists(*record, field_init.name)) {
      SPEC_RULE("Record-Field-Unknown");
      result.diag_id = "Record-Field-Unknown";
      return result;
    }
    if (!FieldVisible(ctx, *record, field_init.name, type_path)) {
      SPEC_RULE("Record-Field-NotVisible");
      result.diag_id = "Record-Field-NotVisible";
      return result;
    }
  }

  std::unordered_set<IdKey> provided;
  for (const auto& field_init : expr.fields) {
    provided.insert(IdKeyOf(field_init.name));
  }
  for (const auto& member : record->members) {
    if (const auto* field = std::get_if<syntax::FieldDecl>(&member)) {
      if (provided.find(IdKeyOf(field->name)) == provided.end()) {
        SPEC_RULE("Record-FieldInit-Missing");
        result.diag_id = "Record-FieldInit-Missing";
        return result;
      }
    }
  }

  auto type_expr = [&](const syntax::ExprPtr& inner) {
    return TypeExpr(ctx, type_ctx, inner, env);
  };
  auto type_ident = [&](std::string_view name) -> ExprTypeResult {
    return TypeIdentifierExprImpl(ctx, syntax::IdentifierExpr{std::string(name)},
                                  env);
  };
  PlaceTypeFn type_place = [&](const syntax::ExprPtr& inner) {
    return TypePlace(ctx, type_ctx, inner, env);
  };
  auto match_check = [&](const syntax::MatchExpr& match,
                         const TypeRef& expected) -> CheckResult {
    return CheckMatchExpr(ctx, type_ctx, match, env, expected);
  };

  for (const auto& field_init : expr.fields) {
    const auto field_type_opt = FieldType(*record, field_init.name, ctx);
    if (!field_type_opt.has_value()) {
      return result;
    }

    if (!BitcopyType(ctx, *field_type_opt) && IsPlaceExpr(field_init.value) &&
        (!field_init.value ||
         !std::holds_alternative<syntax::MoveExpr>(field_init.value->node))) {
      SPEC_RULE("Record-Field-NonBitcopy-Move");
      result.diag_id = "Record-Field-NonBitcopy-Move";
      return result;
    }

    const auto check = CheckExpr(
        ctx, field_init.value, *field_type_opt, type_expr, type_place, type_ident,
        match_check);
    if (!check.ok) {
      result.diag_id = check.diag_id;
      return result;
    }
  }

  SPEC_RULE("T-Record-Literal");
  result.ok = true;
  result.type = MakeTypePath(type_path);
  return result;
}


// Enum literal typing
ExprTypeResult TypeEnumLiteralExprImpl(const ScopeContext& ctx,
                                       const StmtTypeContext& type_ctx,
                                       const syntax::EnumLiteralExpr& expr,
                                       const TypeEnv& env) {
  SpecDefsTypeExpr();
  ExprTypeResult result;

  if (expr.path.empty()) {
    return result;
  }

  TypePath enum_path;
  std::string variant_name;
  if (expr.path.size() == 1) {
    return result;
  }
  enum_path.assign(expr.path.begin(), expr.path.end() - 1);
  variant_name = expr.path.back();

  const auto* enum_decl = LookupEnumDecl(ctx, enum_path);
  if (!enum_decl) {
    return result;
  }

  const syntax::VariantDecl* variant = nullptr;
  for (const auto& v : enum_decl->variants) {
    if (IdEq(v.name, variant_name)) {
      variant = &v;
      break;
    }
  }
  if (!variant) {
    return result;
  }

  auto type_expr = [&](const syntax::ExprPtr& inner) {
    return TypeExpr(ctx, type_ctx, inner, env);
  };
  auto type_ident = [&](std::string_view name) -> ExprTypeResult {
    return TypeIdentifierExprImpl(ctx, syntax::IdentifierExpr{std::string(name)},
                                  env);
  };
  PlaceTypeFn type_place = [&](const syntax::ExprPtr& inner) {
    return TypePlace(ctx, type_ctx, inner, env);
  };
  auto match_check = [&](const syntax::MatchExpr& match,
                         const TypeRef& expected) -> CheckResult {
    return CheckMatchExpr(ctx, type_ctx, match, env, expected);
  };

  if (!variant->payload_opt.has_value()) {
    if (expr.payload_opt.has_value()) {
      return result;
    }
    SPEC_RULE("T-Enum-Lit-Unit");
  } else {
    if (!expr.payload_opt.has_value()) {
      return result;
    }

    const auto& decl_payload = *variant->payload_opt;
    const auto& expr_payload = *expr.payload_opt;

    if (std::holds_alternative<syntax::VariantPayloadTuple>(decl_payload)) {
      const auto& tuple_payload =
          std::get<syntax::VariantPayloadTuple>(decl_payload);
      if (!std::holds_alternative<syntax::EnumPayloadParen>(expr_payload)) {
        return result;
      }
      const auto& paren = std::get<syntax::EnumPayloadParen>(expr_payload);

      if (paren.elements.size() != tuple_payload.elements.size()) {
        return result;
      }

      for (std::size_t i = 0; i < paren.elements.size(); ++i) {
        const auto elem_type_lowered =
            LowerType(ctx, tuple_payload.elements[i]);
        if (!elem_type_lowered.ok) {
          result.diag_id = elem_type_lowered.diag_id;
          return result;
        }
        const auto check = CheckExpr(
            ctx, paren.elements[i], elem_type_lowered.type, type_expr,
            type_place, type_ident, match_check);
        if (!check.ok) {
          result.diag_id = check.diag_id;
          return result;
        }
      }
      SPEC_RULE("T-Enum-Lit-Tuple");
    } else {
      const auto& record_payload =
          std::get<syntax::VariantPayloadRecord>(decl_payload);
      if (!std::holds_alternative<syntax::EnumPayloadBrace>(expr_payload)) {
        return result;
      }
      const auto& brace = std::get<syntax::EnumPayloadBrace>(expr_payload);

      std::unordered_set<IdKey> seen;
      for (const auto& field_init : brace.fields) {
        const auto key = IdKeyOf(field_init.name);
        if (!seen.insert(key).second) {
          return result;
        }
      }

      std::unordered_map<IdKey, TypeRef> field_types;
      for (const auto& field_decl : record_payload.fields) {
        const auto lowered = LowerType(ctx, field_decl.type);
        if (!lowered.ok) {
          result.diag_id = lowered.diag_id;
          return result;
        }
        field_types.emplace(IdKeyOf(field_decl.name), lowered.type);
      }

      if (field_types.size() != seen.size()) {
        return result;
      }

      for (const auto& field_init : brace.fields) {
        const auto it = field_types.find(IdKeyOf(field_init.name));
        if (it == field_types.end()) {
          return result;
        }
        const auto check = CheckExpr(
            ctx, field_init.value, it->second, type_expr, type_place, type_ident,
            match_check);
        if (!check.ok) {
          result.diag_id = check.diag_id;
          return result;
        }
      }
      SPEC_RULE("T-Enum-Lit-Record");
    }
  }

  result.ok = true;
  result.type = MakeTypePath(enum_path);
  return result;
}


ExprTypeResult TypeMethodCallExprImpl(const ScopeContext& ctx,
                                        const StmtTypeContext& type_ctx,
                                        const syntax::MethodCallExpr& expr,
                                        const TypeEnv& env,
                                        const core::Span& span) {
  SpecDefsTypeExpr();
  ExprTypeResult result;

  if (!expr.receiver) {
    return result;
  }

  auto type_expr = [&](const syntax::ExprPtr& inner) {
    return TypeExpr(ctx, type_ctx, inner, env);
  };
  PlaceTypeFn type_place = [&](const syntax::ExprPtr& inner) {
    return TypePlace(ctx, type_ctx, inner, env);
  };
  auto lower_type = [&](const std::shared_ptr<syntax::Type>& type) -> LowerTypeResult {
    const auto lowered = LowerType(ctx, type);
    if (!lowered.ok) {
      return {false, lowered.diag_id, {}};
    }
    return {true, std::nullopt, lowered.type};
  };

  TypeRef lookup_base;
  Permission caller_perm = Permission::Const;
  const auto place = TypePlace(ctx, type_ctx, expr.receiver, env);
  if (place.ok) {
    lookup_base = StripPerm(place.type);
    caller_perm = PermOfType(place.type);
  } else {
    const auto base_expr = TypeExpr(ctx, type_ctx, expr.receiver, env);
    if (!base_expr.ok) {
      result.diag_id = base_expr.diag_id;
      return result;
    }
    lookup_base = StripPerm(base_expr.type);
    caller_perm = PermOfType(base_expr.type);
  }

  if (!lookup_base) {
    return result;
  }
  if (type_ctx.require_pure && IsCapabilityType(lookup_base)) {
    result.diag_id = "E-SEM-2802";
    return result;
  }

  if (const auto* opaque = std::get_if<TypeOpaque>(&lookup_base->node)) {
    const auto class_table = ClassMethodTable(ctx, opaque->class_path);
    if (!class_table.ok) {
      result.diag_id = class_table.diag_id;
      return result;
    }

    const syntax::ClassMethodDecl* method = nullptr;
    syntax::ClassPath owner;
    for (const auto& entry : class_table.methods) {
      if (!entry.method) {
        continue;
      }
      if (!IdEq(entry.method->name, expr.name)) {
        continue;
      }
      method = entry.method;
      owner = entry.owner;
      break;
    }
    if (!method) {
      SPEC_RULE("T-Opaque-Project");
      result.diag_id = "E-TYP-2510";
      return result;
    }

    if (!owner.empty() && IdEq(owner.back(), "Drop") &&
        IdEq(method->name, "drop")) {
      SPEC_RULE("Drop-Call-Err-Dyn");
      result.diag_id = "Drop-Call-Err-Dyn";
      return result;
    }

    auto lower_type_self = [&](const std::shared_ptr<syntax::Type>& type)
        -> LowerTypeResult {
      const auto lowered = LowerType(ctx, type);
      if (!lowered.ok) {
        return {false, lowered.diag_id, {}};
      }
      return {true, std::nullopt, SubstSelfType(lookup_base, lowered.type)};
    };

    const auto mode = RecvModeOf(method->receiver);
    const auto recv_base =
        RecvBaseType(expr.receiver, mode, type_place, type_expr);
    if (!recv_base.ok) {
      result.diag_id = recv_base.diag_id;
      return result;
    }
    const auto recv_type =
        RecvTypeForReceiver(ctx, lookup_base, method->receiver, lower_type_self);
    if (!recv_type.ok) {
      result.diag_id = recv_type.diag_id;
      return result;
    }
    const auto method_perm = PermOfType(recv_type.type);
    if (!PermSub(recv_base.perm, method_perm)) {
      SPEC_RULE("MethodCall-RecvPerm-Err");
      result.diag_id = "MethodCall-RecvPerm-Err";
      return result;
    }

    const auto recv_arg = RecvArgOk(expr.receiver, mode, type_expr);
    if (!recv_arg.ok) {
      result.diag_id = recv_arg.diag_id;
      return result;
    }

    const auto args_ok =
        ArgsOk(ctx, method->params, expr.args, type_expr, &type_place,
               lower_type_self);
    if (!args_ok.ok) {
      result.diag_id = args_ok.diag_id;
      return result;
    }

    TypeLowerResult ret_type;
    if (!method->return_type_opt) {
      ret_type = {true, std::nullopt, MakeTypePrim("()")} ;
    } else {
      ret_type = lower_type_self(method->return_type_opt);
    }
    if (!ret_type.ok) {
      result.diag_id = ret_type.diag_id;
      return result;
    }

    SPEC_RULE("T-Opaque-Project");
    result.ok = true;
    result.type = ret_type.type;
    return result;
  }

  if (const auto* modal = std::get_if<TypeModalState>(&lookup_base->node)) {
    if (modal->path.size() == 1 && IdEq(modal->path[0], "Region") &&
        (IdEq(expr.name, "reset_unchecked") ||
         IdEq(expr.name, "free_unchecked")) &&
        (IdEq(modal->state, "Active") || IdEq(modal->state, "Frozen")) &&
        !IsInUnsafeSpan(ctx, span)) {
      SPEC_RULE("Region-Unchecked-Unsafe-Err");
      result.diag_id = "Region-Unchecked-Unsafe-Err";
      return result;
    }

    const auto* decl = LookupModalDecl(ctx, modal->path);
    if (!decl) {
      return result;
    }

    if (const auto* transition =
            LookupTransitionDecl(*decl, modal->state, expr.name)) {
      if (caller_perm != Permission::Unique) {
        SPEC_RULE("Transition-Source-Err");
        result.diag_id = "Transition-Source-Err";
        return result;
      }
      if (!StateMemberVisible(ctx, modal->path, transition->vis)) {
        SPEC_RULE("Transition-NotVisible");
        result.diag_id = "Transition-NotVisible";
        return result;
      }
      const auto args_ok =
          ArgsOk(ctx, transition->params, expr.args, type_expr, &type_place,
                 lower_type);
      if (!args_ok.ok) {
        result.diag_id = args_ok.diag_id;
        return result;
      }
      const auto recv_arg =
          RecvArgOk(expr.receiver, ParamMode::Move, type_expr);
      if (!recv_arg.ok) {
        if (recv_arg.diag_id.has_value()) {
          result.diag_id = recv_arg.diag_id;
        } else {
          SPEC_RULE("Call-Move-Missing");
          result.diag_id = "Call-Move-Missing";
        }
        return result;
      }
      SPEC_RULE("T-Modal-Transition");
      result.ok = true;
      result.type = MakeTypeModalState(modal->path, transition->target_state);
      return result;
    }

    const auto* method =
        LookupStateMethodDecl(*decl, modal->state, expr.name);
    if (!method) {
      SPEC_RULE("Modal-Method-NotFound");
      result.diag_id = "Modal-Method-NotFound";
      return result;
    }
    if (!StateMemberVisible(ctx, modal->path, method->vis)) {
      SPEC_RULE("Modal-Method-NotVisible");
      result.diag_id = "Modal-Method-NotVisible";
      return result;
    }
    if (!PermSub(caller_perm, Permission::Const)) {
      SPEC_RULE("MethodCall-RecvPerm-Err");
      result.diag_id = "MethodCall-RecvPerm-Err";
      return result;
    }
    const auto args_ok =
        ArgsOk(ctx, method->params, expr.args, type_expr, &type_place,
               lower_type);
    if (!args_ok.ok) {
      result.diag_id = args_ok.diag_id;
      return result;
    }
    TypeLowerResult ret_type;
    if (!method->return_type_opt) {
      ret_type = {true, std::nullopt, MakeTypePrim("()")} ;
    } else {
      ret_type = LowerType(ctx, method->return_type_opt);
    }
    if (!ret_type.ok) {
      result.diag_id = ret_type.diag_id;
      return result;
    }
    SPEC_RULE("T-Modal-Method");
    result.ok = true;
    result.type = ret_type.type;
    return result;
  }

  if (const auto* dyn = std::get_if<TypeDynamic>(&lookup_base->node)) {
    if (IsFileSystemClassPath(dyn->path)) {
      const auto sig = LookupFileSystemMethodSig(expr.name);
      if (!sig.has_value()) {
        SPEC_RULE("LookupClassMethod-NotFound");
        result.diag_id = "LookupMethod-NotFound";
        return result;
      }

      const auto recv_base =
          RecvBaseType(expr.receiver, std::nullopt, type_place, type_expr);
      if (!recv_base.ok) {
        result.diag_id = recv_base.diag_id;
        return result;
      }
      if (!recv_base.base ||
          !std::holds_alternative<TypeDynamic>(recv_base.base->node)) {
        return result;
      }
      if (!PermSub(recv_base.perm, sig->recv_perm)) {
        SPEC_RULE("MethodCall-RecvPerm-Err");
        result.diag_id = "MethodCall-RecvPerm-Err";
        return result;
      }
      const auto recv_arg = RecvArgOk(expr.receiver, std::nullopt, type_expr);
      if (!recv_arg.ok) {
        result.diag_id = recv_arg.diag_id;
        return result;
      }
      const auto args_ok =
          ArgsOk(ctx, sig->params, expr.args, type_expr, &type_place,
                 lower_type);
      if (!args_ok.ok) {
        result.diag_id = args_ok.diag_id;
        return result;
      }

      SPEC_RULE("T-Dynamic-MethodCall");
      result.ok = true;
      result.type = sig->ret;
      return result;
    }
    if (IsHeapAllocatorClassPath(dyn->path)) {
      const auto sig = LookupHeapAllocatorMethodSig(expr.name);
      if (!sig.has_value()) {
        SPEC_RULE("LookupClassMethod-NotFound");
        result.diag_id = "LookupMethod-NotFound";
        return result;
      }

      if (sig->kind == HeapAllocatorMethodKind::AllocRaw &&
          !IsInUnsafeSpan(ctx, span)) {
        SPEC_RULE("AllocRaw-Unsafe-Err");
        result.diag_id = "AllocRaw-Unsafe-Err";
        return result;
      }
      if (sig->kind == HeapAllocatorMethodKind::DeallocRaw &&
          !IsInUnsafeSpan(ctx, span)) {
        SPEC_RULE("DeallocRaw-Unsafe-Err");
        result.diag_id = "DeallocRaw-Unsafe-Err";
        return result;
      }

      const auto recv_base =
          RecvBaseType(expr.receiver, std::nullopt, type_place, type_expr);
      if (!recv_base.ok) {
        result.diag_id = recv_base.diag_id;
        return result;
      }
      if (!recv_base.base ||
          !std::holds_alternative<TypeDynamic>(recv_base.base->node)) {
        return result;
      }
      if (!PermSub(recv_base.perm, sig->recv_perm)) {
        SPEC_RULE("MethodCall-RecvPerm-Err");
        result.diag_id = "MethodCall-RecvPerm-Err";
        return result;
      }
      const auto recv_arg = RecvArgOk(expr.receiver, std::nullopt, type_expr);
      if (!recv_arg.ok) {
        result.diag_id = recv_arg.diag_id;
        return result;
      }

      if (sig->kind == HeapAllocatorMethodKind::AllocRaw) {
        const auto args_ok =
            ArgsOk(ctx, sig->params, expr.args, type_expr, &type_place,
                   lower_type);
        if (!args_ok.ok) {
          result.diag_id = args_ok.diag_id;
          return result;
        }
        // alloc_raw requires an expected raw pointer type for its element.
        return result;
      }

      if (sig->kind == HeapAllocatorMethodKind::DeallocRaw) {
        if (expr.args.size() != 2) {
          SPEC_RULE("Call-ArgCount-Err");
          result.diag_id = "Call-ArgCount-Err";
          return result;
        }
        if (expr.args[0].moved || expr.args[1].moved) {
          SPEC_RULE("Call-Move-Unexpected");
          result.diag_id = "Call-Move-Unexpected";
          return result;
        }
        if (!IsPlaceExpr(expr.args[0].value) ||
            !IsPlaceExpr(expr.args[1].value)) {
          SPEC_RULE("Call-Arg-NotPlace");
          result.diag_id = "Call-Arg-NotPlace";
          return result;
        }
        const auto ptr_type = type_expr(expr.args[0].value);
        if (!ptr_type.ok) {
          result.diag_id = ptr_type.diag_id;
          return result;
        }
        const auto ptr_strip = StripPerm(ptr_type.type);
        const auto* raw_ptr = ptr_strip
                                  ? std::get_if<TypeRawPtr>(&ptr_strip->node)
                                  : nullptr;
        if (!raw_ptr || raw_ptr->qual != RawPtrQual::Mut) {
          SPEC_RULE("Call-ArgType-Err");
          result.diag_id = "Call-ArgType-Err";
          return result;
        }
        const auto count_type = type_expr(expr.args[1].value);
        if (!count_type.ok) {
          result.diag_id = count_type.diag_id;
          return result;
        }
        const auto sub = Subtyping(ctx, count_type.type, MakeTypePrim("usize"));
        if (!sub.ok) {
          result.diag_id = sub.diag_id;
          return result;
        }
        if (!sub.subtype) {
          SPEC_RULE("Call-ArgType-Err");
          result.diag_id = "Call-ArgType-Err";
          return result;
        }
        SPEC_RULE("T-Dynamic-MethodCall");
        result.ok = true;
        result.type = sig->ret;
        return result;
      }

      const auto args_ok =
          ArgsOk(ctx, sig->params, expr.args, type_expr, &type_place,
                 lower_type);
      if (!args_ok.ok) {
        result.diag_id = args_ok.diag_id;
        return result;
      }

      SPEC_RULE("T-Dynamic-MethodCall");
      result.ok = true;
      result.type = sig->ret;
      return result;
    }
    if (IsReactorClassPath(dyn->path)) {
      if (!(IdEq(expr.name, "run") || IdEq(expr.name, "register"))) {
        SPEC_RULE("LookupClassMethod-NotFound");
        result.diag_id = "LookupMethod-NotFound";
        return result;
      }

      const auto recv_base =
          RecvBaseType(expr.receiver, std::nullopt, type_place, type_expr);
      if (!recv_base.ok) {
        result.diag_id = recv_base.diag_id;
        return result;
      }
      if (!recv_base.base ||
          !std::holds_alternative<TypeDynamic>(recv_base.base->node)) {
        return result;
      }
      if (!PermSub(recv_base.perm, Permission::Const)) {
        SPEC_RULE("MethodCall-RecvPerm-Err");
        result.diag_id = "MethodCall-RecvPerm-Err";
        return result;
      }
      const auto recv_arg = RecvArgOk(expr.receiver, std::nullopt, type_expr);
      if (!recv_arg.ok) {
        result.diag_id = recv_arg.diag_id;
        return result;
      }
      if (expr.args.size() != 1) {
        SPEC_RULE("Call-ArgCount-Err");
        result.diag_id = "Call-ArgCount-Err";
        return result;
      }
      if (expr.args[0].moved) {
        SPEC_RULE("Call-Move-Unexpected");
        result.diag_id = "Call-Move-Unexpected";
        return result;
      }
      const auto arg_type = type_expr(expr.args[0].value);
      if (!arg_type.ok) {
        result.diag_id = arg_type.diag_id;
        return result;
      }
      const auto async_sig = AsyncSigOf(ctx, arg_type.type);
      if (!async_sig.has_value() ||
          !IsPrimType(async_sig->out, "()") ||
          !IsPrimType(async_sig->in, "()")) {
        SPEC_RULE("Call-ArgType-Err");
        result.diag_id = "Call-ArgType-Err";
        return result;
      }

      SPEC_RULE("T-Dynamic-MethodCall");
      result.ok = true;
      if (IdEq(expr.name, "run")) {
        result.type = MakeTypeUnion({async_sig->result, async_sig->err});
      } else {
        result.type = MakeFutureHandleType(async_sig->result, async_sig->err);
      }
      return result;
    }
    if (IsExecutionDomainTypePath(dyn->path)) {
      const auto sig = LookupExecutionDomainMethodSig(expr.name);
      if (!sig.has_value()) {
        SPEC_RULE("LookupClassMethod-NotFound");
        result.diag_id = "LookupMethod-NotFound";
        return result;
      }

      const auto recv_base =
          RecvBaseType(expr.receiver, std::nullopt, type_place, type_expr);
      if (!recv_base.ok) {
        result.diag_id = recv_base.diag_id;
        return result;
      }
      if (!recv_base.base ||
          !std::holds_alternative<TypeDynamic>(recv_base.base->node)) {
        return result;
      }
      if (!PermSub(recv_base.perm, sig->recv_perm)) {
        SPEC_RULE("MethodCall-RecvPerm-Err");
        result.diag_id = "MethodCall-RecvPerm-Err";
        return result;
      }
      const auto recv_arg = RecvArgOk(expr.receiver, std::nullopt, type_expr);
      if (!recv_arg.ok) {
        result.diag_id = recv_arg.diag_id;
        return result;
      }
      const auto args_ok =
          ArgsOk(ctx, sig->params, expr.args, type_expr, &type_place,
                 lower_type);
      if (!args_ok.ok) {
        result.diag_id = args_ok.diag_id;
        return result;
      }

      SPEC_RULE("T-Dynamic-MethodCall");
      result.ok = true;
      result.type = sig->ret;
      return result;
    }
    const auto table = ClassMethodTable(ctx, dyn->path);
    if (!table.ok) {
      result.diag_id = table.diag_id;
      return result;
    }

    const syntax::ClassMethodDecl* method = nullptr;
    syntax::ClassPath owner;
    for (const auto& entry : table.methods) {
      if (!entry.method) {
        continue;
      }
      if (!IdEq(entry.method->name, expr.name)) {
        continue;
      }
      method = entry.method;
      owner = entry.owner;
      break;
    }
    if (!method) {
      SPEC_RULE("LookupClassMethod-NotFound");
      result.diag_id = "LookupMethod-NotFound";
      return result;
    }

    if (!owner.empty() && IdEq(owner.back(), "Drop") &&
        IdEq(method->name, "drop")) {
      SPEC_RULE("Drop-Call-Err-Dyn");
      result.diag_id = "Drop-Call-Err-Dyn";
      return result;
    }

    const auto mode = RecvModeOf(method->receiver);
    const auto recv_base =
        RecvBaseType(expr.receiver, mode, type_place, type_expr);
    if (!recv_base.ok) {
      result.diag_id = recv_base.diag_id;
      return result;
    }
    if (!recv_base.base ||
        !std::holds_alternative<TypeDynamic>(recv_base.base->node)) {
      return result;
    }

    const auto self_var = MakeTypePath({"Self"});
    const auto recv_type =
        RecvTypeForReceiver(ctx, self_var, method->receiver, lower_type);
    if (!recv_type.ok) {
      result.diag_id = recv_type.diag_id;
      return result;
    }
    const auto method_perm = PermOfType(recv_type.type);
    if (type_ctx.require_pure) {
      if (method_perm != Permission::Const ||
          !ParamsPure(ctx, method->params, lower_type)) {
        result.diag_id = "E-SEM-2802";
        return result;
      }
    }
    if (!PermSub(recv_base.perm, method_perm)) {
      SPEC_RULE("MethodCall-RecvPerm-Err");
      result.diag_id = "MethodCall-RecvPerm-Err";
      return result;
    }

    const auto recv_arg = RecvArgOk(expr.receiver, mode, type_expr);
    if (!recv_arg.ok) {
      result.diag_id = recv_arg.diag_id;
      return result;
    }

    const auto args_ok =
        ArgsOk(ctx, method->params, expr.args, type_expr, &type_place,
               lower_type);
    if (!args_ok.ok) {
      result.diag_id = args_ok.diag_id;
      return result;
    }

    TypeLowerResult ret_type;
    if (!method->return_type_opt) {
      ret_type = {true, std::nullopt, MakeTypePrim("()")} ;
    } else {
      ret_type = LowerType(ctx, method->return_type_opt);
    }
    if (!ret_type.ok) {
      result.diag_id = ret_type.diag_id;
      return result;
    }

    SPEC_RULE("T-Dynamic-MethodCall");
    result.ok = true;
    result.type = ret_type.type;
    return result;
  }

  if (const auto* path = std::get_if<TypePathType>(&lookup_base->node)) {
    if (path->path.size() == 1 && IdEq(path->path[0], "System")) {
      const auto sig = LookupSystemMethodSig(expr.name);
      if (!sig.has_value()) {
        SPEC_RULE("LookupMethod-NotFound");
        result.diag_id = "LookupMethod-NotFound";
        return result;
      }
      if (type_ctx.require_pure) {
        if (sig->recv_perm != Permission::Const ||
            !ParamsPure(ctx, sig->params, lower_type)) {
          result.diag_id = "E-SEM-2802";
          return result;
        }
      }

      const auto recv_base =
          RecvBaseType(expr.receiver, std::nullopt, type_place, type_expr);
      if (!recv_base.ok) {
        result.diag_id = recv_base.diag_id;
        return result;
      }
      const auto* recv_path = recv_base.base
                                  ? std::get_if<TypePathType>(&recv_base.base->node)
                                  : nullptr;
      if (!recv_path || recv_path->path.size() != 1 ||
          !IdEq(recv_path->path[0], "System")) {
        return result;
      }
      if (!PermSub(recv_base.perm, sig->recv_perm)) {
        SPEC_RULE("MethodCall-RecvPerm-Err");
        result.diag_id = "MethodCall-RecvPerm-Err";
        return result;
      }

      const auto recv_arg = RecvArgOk(expr.receiver, std::nullopt, type_expr);
      if (!recv_arg.ok) {
        result.diag_id = recv_arg.diag_id;
        return result;
      }

      const auto args_ok =
          ArgsOk(ctx, sig->params, expr.args, type_expr, &type_place, lower_type);
      if (!args_ok.ok) {
        result.diag_id = args_ok.diag_id;
        return result;
      }

      SPEC_RULE("T-Record-MethodCall");
      SPEC_RULE("Step-MethodCall");
      result.ok = true;
      result.type = sig->ret;
      return result;
    }

    // C0X Extension: Context method lookup (§18.2)
    if (path->path.size() == 1 && IdEq(path->path[0], "Context")) {
      const auto sig = LookupContextMethodSig(expr.name);
      if (sig.has_value()) {
        if (type_ctx.require_pure) {
          if (sig->recv_perm != Permission::Const ||
              !ParamsPure(ctx, sig->params, lower_type)) {
            result.diag_id = "E-SEM-2802";
            return result;
          }
        }
        const auto recv_base =
            RecvBaseType(expr.receiver, std::nullopt, type_place, type_expr);
        if (!recv_base.ok) {
          result.diag_id = recv_base.diag_id;
          return result;
        }
        const auto* recv_path = recv_base.base
                                    ? std::get_if<TypePathType>(&recv_base.base->node)
                                    : nullptr;
        if (!recv_path || recv_path->path.size() != 1 ||
            !IdEq(recv_path->path[0], "Context")) {
          return result;
        }
        if (!PermSub(recv_base.perm, sig->recv_perm)) {
          SPEC_RULE("MethodCall-RecvPerm-Err");
          result.diag_id = "MethodCall-RecvPerm-Err";
          return result;
        }

        const auto recv_arg = RecvArgOk(expr.receiver, std::nullopt, type_expr);
        if (!recv_arg.ok) {
          result.diag_id = recv_arg.diag_id;
          return result;
        }

        const auto args_ok =
            ArgsOk(ctx, sig->params, expr.args, type_expr, &type_place, lower_type);
        if (!args_ok.ok) {
          result.diag_id = args_ok.diag_id;
          return result;
        }

        SPEC_RULE("T-Context-MethodCall");
        result.ok = true;
        result.type = sig->ret;
        return result;
      }
    }
  }

  const auto lookup = LookupMethodStatic(ctx, lookup_base, expr.name);
  if (!lookup.ok) {
    result.diag_id = lookup.diag_id;
    return result;
  }

  const auto* record_method = lookup.record_method;
  const auto* class_method = lookup.class_method;
  if (!record_method && !class_method) {
    return result;
  }

  if (IdEq(expr.name, "drop")) {
    const syntax::ClassPath drop_path = {"Drop"};
    if (TypeImplementsClass(ctx, lookup_base, drop_path)) {
      SPEC_RULE("Drop-Call-Err");
      result.diag_id = "Drop-Call-Err";
      return result;
    }
  }

  if (class_method) {
    if (!lookup.owner_class.empty() && IdEq(lookup.owner_class.back(), "Drop") &&
        IdEq(class_method->name, "drop")) {
      SPEC_RULE("Drop-Call-Err");
      result.diag_id = "Drop-Call-Err";
      return result;
    }
  }

  const syntax::Receiver& receiver =
      record_method ? record_method->receiver : class_method->receiver;
  const auto mode = RecvModeOf(receiver);
  const auto recv_base =
      RecvBaseType(expr.receiver, mode, type_place, type_expr);
  if (!recv_base.ok) {
    result.diag_id = recv_base.diag_id;
    return result;
  }

  const auto recv_type = RecvTypeForReceiver(ctx, lookup_base, receiver, lower_type);
  if (!recv_type.ok) {
    result.diag_id = recv_type.diag_id;
    return result;
  }
  const auto method_perm = PermOfType(recv_type.type);
  if (type_ctx.require_pure) {
    if (method_perm != Permission::Const ||
        !ParamsPure(ctx, params, lower_type)) {
      result.diag_id = "E-SEM-2802";
      return result;
    }
  }
  if (!PermSub(recv_base.perm, method_perm)) {
    SPEC_RULE("MethodCall-RecvPerm-Err");
    result.diag_id = "MethodCall-RecvPerm-Err";
    return result;
  }

  const auto recv_arg = RecvArgOk(expr.receiver, mode, type_expr);
  if (!recv_arg.ok) {
    result.diag_id = recv_arg.diag_id;
    return result;
  }

  const auto& params = record_method ? record_method->params
                                     : class_method->params;
  const auto args_ok =
      ArgsOk(ctx, params, expr.args, type_expr, &type_place, lower_type);
  if (!args_ok.ok) {
    result.diag_id = args_ok.diag_id;
    return result;
  }

  TypeLowerResult ret_type;
  const auto ret_opt = record_method ? record_method->return_type_opt
                                     : class_method->return_type_opt;
  if (!ret_opt) {
    ret_type = {true, std::nullopt, MakeTypePrim("()")} ;
  } else {
    ret_type = LowerType(ctx, ret_opt);
  }
  if (!ret_type.ok) {
    result.diag_id = ret_type.diag_id;
    return result;
  }

  if (record_method) {
    SPEC_RULE("T-Record-MethodCall");
    SPEC_RULE("Step-MethodCall");
  } else {
    SPEC_RULE("T-MethodCall");
  }
  result.ok = true;
  result.type = ret_type.type;
  return result;
}

// Main expression typing dispatcher
static ExprTypeResult TypeExprImpl(const ScopeContext& ctx,
                                   const StmtTypeContext& type_ctx,
                                   const syntax::ExprPtr& expr,
                                   const TypeEnv& env) {
  SpecDefsTypeExpr();
  ExprTypeResult result;

  if (!expr) {
    return result;
  }

  auto type_expr = [&](const syntax::ExprPtr& inner) {
    return TypeExpr(ctx, type_ctx, inner, env);
  };
  PlaceTypeFn type_place = [&](const syntax::ExprPtr& inner) {
    return TypePlace(ctx, type_ctx, inner, env);
  };
  auto type_ident = [&](std::string_view name) -> ExprTypeResult {
    return TypeIdentifierExprImpl(ctx, syntax::IdentifierExpr{std::string(name)},
                                  env);
  };

  return std::visit(
      [&](const auto& node) -> ExprTypeResult {
        using T = std::decay_t<decltype(node)>;

        if constexpr (std::is_same_v<T, syntax::ErrorExpr>) {
          return TypeErrorExprImpl(ctx, node);
        } else if constexpr (std::is_same_v<T, syntax::LiteralExpr>) {
          return TypeLiteralExpr(ctx, node);
        } else if constexpr (std::is_same_v<T, syntax::PtrNullExpr>) {
          SPEC_RULE("Syn-PtrNull-Err");
          ExprTypeResult r;
          r.diag_id = "PtrNull-Infer-Err";
          return r;
        } else if constexpr (std::is_same_v<T, syntax::IdentifierExpr>) {
          return TypeIdentifierExprImpl(ctx, node, env);
        } else if constexpr (std::is_same_v<T, syntax::PathExpr>) {
          return TypePathExprImpl(ctx, node);
        } else if constexpr (std::is_same_v<T, syntax::FieldAccessExpr>) {
          return TypeFieldAccessExprImpl(ctx, type_ctx, node, env);
        } else if constexpr (std::is_same_v<T, syntax::TupleAccessExpr>) {
          return TypeTupleAccessExprImpl(ctx, type_ctx, node, env);
        } else if constexpr (std::is_same_v<T, syntax::IndexAccessExpr>) {
          return TypeIndexAccessValue(ctx, node, type_expr);
        } else if constexpr (std::is_same_v<T, syntax::BinaryExpr>) {
          return TypeBinaryExprImpl(ctx, type_ctx, node, env);
        } else if constexpr (std::is_same_v<T, syntax::UnaryExpr>) {
          return TypeUnaryExprImpl(ctx, type_ctx, node, env, expr->span);
        } else if constexpr (std::is_same_v<T, syntax::CastExpr>) {
          return TypeCastExprImpl(ctx, type_ctx, node, env);
        } else if constexpr (std::is_same_v<T, syntax::IfExpr>) {
          return TypeIfExprImpl(ctx, type_ctx, node, env);
        } else if constexpr (std::is_same_v<T, syntax::MatchExpr>) {
          return TypeMatchExpr(ctx, type_ctx, node, env);
        } else if constexpr (std::is_same_v<T, syntax::RangeExpr>) {
          return TypeRangeExprImpl(ctx, type_ctx, node, env);
        } else if constexpr (std::is_same_v<T, syntax::AddressOfExpr>) {
          return TypeAddressOfExprImpl(ctx, type_ctx, node, env);
        } else if constexpr (std::is_same_v<T, syntax::DerefExpr>) {
          return TypeDerefExprImpl(ctx, type_ctx, node, env, expr->span);
        } else if constexpr (std::is_same_v<T, syntax::MoveExpr>) {
          return TypeMoveExprImpl(ctx, type_ctx, node, env);
        } else if constexpr (std::is_same_v<T, syntax::PropagateExpr>) {
          return TypePropagateExprImpl(ctx, type_ctx, node, env);
        } else if constexpr (std::is_same_v<T, syntax::ResultExpr>) {
          if (type_ctx.contract_phase != ContractPhase::Postcondition) {
            ExprTypeResult r;
            r.diag_id = "E-SEM-2806";
            return r;
          }
          ExprTypeResult r;
          r.ok = true;
          r.type = type_ctx.return_type ? type_ctx.return_type : MakeTypePrim("()");
          return r;
        } else if constexpr (std::is_same_v<T, syntax::EntryExpr>) {
          if (type_ctx.contract_phase != ContractPhase::Postcondition) {
            ExprTypeResult r;
            r.diag_id = "E-SEM-2852";
            return r;
          }
          if (!node.expr) {
            return ExprTypeResult{};
          }
          if (!ExprUsesOnlyEnvBindings(node.expr, env)) {
            ExprTypeResult r;
            r.diag_id = "E-SEM-2852";
            return r;
          }
          const auto typed = TypeExpr(ctx, type_ctx, node.expr, env);
          if (!typed.ok) {
            ExprTypeResult r;
            r.diag_id = typed.diag_id;
            return r;
          }
          if (!BitcopyType(ctx, typed.type) && !CloneType(ctx, typed.type)) {
            ExprTypeResult r;
            r.diag_id = "E-SEM-2805";
            return r;
          }
          ExprTypeResult r;
          r.ok = true;
          r.type = typed.type;
          return r;
        } else if constexpr (std::is_same_v<T, syntax::RecordExpr>) {
          return TypeRecordExprImpl(ctx, type_ctx, node, env);
        } else if constexpr (std::is_same_v<T, syntax::EnumLiteralExpr>) {
          return TypeEnumLiteralExprImpl(ctx, type_ctx, node, env);
        } else if constexpr (std::is_same_v<T, syntax::TupleExpr>) {
          return TypeTupleExpr(ctx, node, type_expr);
        } else if constexpr (std::is_same_v<T, syntax::ArrayExpr>) {
          return TypeArrayExpr(ctx, node, type_expr);
        } else if constexpr (std::is_same_v<T, syntax::CallExpr>) {
          if (node.callee && node.args.empty()) {
            if (const auto* ident =
                    std::get_if<syntax::IdentifierExpr>(&node.callee->node);
                ident && IdEq(ident->name, "RegionOptions")) {
              ExprTypeResult r;
              r.ok = true;
              r.type = MakeTypePath({"RegionOptions"});
              SPEC_RULE("T-Record-Default");
              return r;
            }
          }
          const auto record =
              TypeRecordDefaultCall(ctx, node.callee, node.args, type_expr);
          if (record.ok || record.diag_id.has_value()) {
            return record;
          }
          PlaceTypeFn type_place = [&](const syntax::ExprPtr& inner) {
            return TypePlace(ctx, type_ctx, inner, env);
          };
          const auto call =
              TypeCall(ctx, node.callee, node.args, type_expr, &type_place);
          ExprTypeResult r;
          if (!call.ok) {
            r.diag_id = call.diag_id;
            return r;
          }
          if (type_ctx.require_pure) {
            const auto callee_type = type_expr(node.callee);
            if (callee_type.ok) {
              const auto* func =
                  std::get_if<TypeFunc>(&StripPerm(callee_type.type)->node);
              if (func && !ParamsPure(ctx, func->params)) {
                r.diag_id = "E-SEM-2802";
                return r;
              }
            }
          }
          r.ok = true;
          r.type = call.type;
          return r;
        } else if constexpr (std::is_same_v<T, syntax::MethodCallExpr>) {
          return TypeMethodCallExprImpl(ctx, type_ctx, node, env, expr->span);
        } else if constexpr (std::is_same_v<T, syntax::BlockExpr>) {
          return TypeBlockExpr(ctx, type_ctx, node, env, type_expr, type_ident,
                               type_place);
        } else if constexpr (std::is_same_v<T, syntax::UnsafeBlockExpr>) {
          return TypeUnsafeBlockExpr(ctx, type_ctx, node, env, type_expr,
                                     type_ident, type_place);
        } else if constexpr (std::is_same_v<T, syntax::LoopInfiniteExpr>) {
          return TypeLoopInfiniteExpr(ctx, type_ctx, node, env, type_expr,
                                      type_ident, type_place);
        } else if constexpr (std::is_same_v<T, syntax::LoopConditionalExpr>) {
          return TypeLoopConditionalExpr(ctx, type_ctx, node, env, type_expr,
                                         type_ident, type_place);
        } else if constexpr (std::is_same_v<T, syntax::LoopIterExpr>) {
          return TypeLoopIterExpr(ctx, type_ctx, node, env, type_expr,
                                  type_ident, type_place);
        } else if constexpr (std::is_same_v<T, syntax::AllocExpr>) {
          return TypeAllocExpr(ctx, type_ctx, node, env);
        } else if constexpr (std::is_same_v<T, syntax::TransmuteExpr>) {
          return TypeTransmuteExprImpl(ctx, type_ctx, node, env, expr->span);
        // C0X Extension: Async expressions (§19)
        } else if constexpr (std::is_same_v<T, syntax::YieldExpr>) {
          return TypeYieldExpr(ctx, type_ctx, node, env, type_expr);
        } else if constexpr (std::is_same_v<T, syntax::YieldFromExpr>) {
          return TypeYieldFromExpr(ctx, type_ctx, node, env, type_expr);
        } else if constexpr (std::is_same_v<T, syntax::SyncExpr>) {
          return TypeSyncExpr(ctx, type_ctx, node, env, type_expr);
        } else if constexpr (std::is_same_v<T, syntax::RaceExpr>) {
          return TypeRaceExpr(ctx, type_ctx, node, env, type_expr, type_ident,
                              type_place);
        } else if constexpr (std::is_same_v<T, syntax::AllExpr>) {
          return TypeAllExpr(ctx, type_ctx, node, env, type_expr);
        } else if constexpr (std::is_same_v<T, syntax::QualifiedNameExpr>) {
          SPEC_RULE("Expr-Unresolved-Err");
          ExprTypeResult r;
          r.diag_id = "ResolveExpr-Ident-Err";
          return r;
        } else if constexpr (std::is_same_v<T, syntax::QualifiedApplyExpr>) {
          SPEC_RULE("Expr-Unresolved-Err");
          ExprTypeResult r;
          r.diag_id = "ResolveExpr-Ident-Err";
          return r;
        // C0X Extension: Structured Concurrency (§10, §18)
        } else if constexpr (std::is_same_v<T, syntax::ParallelExpr>) {
          return TypeParallelExpr(ctx, type_ctx, node, env, type_expr,
                                  type_ident, type_place);
        } else if constexpr (std::is_same_v<T, syntax::SpawnExpr>) {
          return TypeSpawnExpr(ctx, type_ctx, node, env, type_expr,
                               type_ident, type_place);
        } else if constexpr (std::is_same_v<T, syntax::WaitExpr>) {
          return TypeWaitExpr(ctx, type_ctx, node, env, type_expr, type_place);
        } else if constexpr (std::is_same_v<T, syntax::DispatchExpr>) {
          return TypeDispatchExpr(ctx, type_ctx, node, env, type_expr,
                                  type_ident, type_place);
        } else {
          return ExprTypeResult{};
        }
      },
      expr->node);
}

// Main place typing dispatcher
static PlaceTypeResult TypePlaceImpl(const ScopeContext& ctx,
                                     const StmtTypeContext& type_ctx,
                                     const syntax::ExprPtr& expr,
                                     const TypeEnv& env) {
  SpecDefsTypeExpr();
  PlaceTypeResult result;
  struct PlaceTypeRecorder {
    const ScopeContext& ctx;
    const syntax::ExprPtr& expr;
    PlaceTypeResult& result;
    ~PlaceTypeRecorder() {
      if (result.ok && ctx.expr_types && expr) {
        (*ctx.expr_types)[expr.get()] = result.type;
      }
    }
  } recorder{ctx, expr, result};

  if (!expr) {
    return result;
  }

  PlaceTypeFn type_place = [&](const syntax::ExprPtr& inner) {
    return TypePlace(ctx, type_ctx, inner, env);
  };
  auto type_expr = [&](const syntax::ExprPtr& inner) {
    return TypeExpr(ctx, type_ctx, inner, env);
  };

  return std::visit(
      [&](const auto& node) -> PlaceTypeResult {
        using T = std::decay_t<decltype(node)>;

        if constexpr (std::is_same_v<T, syntax::IdentifierExpr>) {
          return TypeIdentifierPlaceImpl(ctx, node, env);
        } else if constexpr (std::is_same_v<T, syntax::FieldAccessExpr>) {
          return TypeFieldAccessPlaceImpl(ctx, type_ctx, node, env);
        } else if constexpr (std::is_same_v<T, syntax::TupleAccessExpr>) {
          return TypeTupleAccessPlaceImpl(ctx, type_ctx, node, env);
        } else if constexpr (std::is_same_v<T, syntax::DerefExpr>) {
          return TypeDerefPlaceImpl(ctx, type_ctx, node, env, expr->span);
        } else if constexpr (std::is_same_v<T, syntax::IndexAccessExpr>) {
          return TypeIndexAccessPlace(ctx, node, type_place, type_expr);
        } else {
          return PlaceTypeResult{};
        }
      },
      expr->node);
}


ExprTypeResult TypeTransmuteExprImpl(const ScopeContext& ctx,
                                 const StmtTypeContext& type_ctx,
                                 const syntax::TransmuteExpr& expr,
                                 const TypeEnv& env,
                                 const core::Span& span) {
  SpecDefsTypeExpr();
  ExprTypeResult result;
  if (type_ctx.require_pure) {
    result.diag_id = "E-SEM-2802";
    return result;
  }
  if (!IsInUnsafeSpan(ctx, span)) {
    SPEC_RULE("Transmute-Unsafe-Err");
    result.diag_id = "Transmute-Unsafe-Err";
    return result;
  }
  if (!expr.from || !expr.to || !expr.value) {
    return result;
  }

  const auto from = LowerType(ctx, expr.from);
  if (!from.ok) {
    result.diag_id = from.diag_id;
    return result;
  }
  const auto from_wf = TypeWF(ctx, from.type);
  if (!from_wf.ok) {
    result.diag_id = from_wf.diag_id;
    return result;
  }
  const auto to = LowerType(ctx, expr.to);
  if (!to.ok) {
    result.diag_id = to.diag_id;
    return result;
  }
  const auto to_wf = TypeWF(ctx, to.type);
  if (!to_wf.ok) {
    result.diag_id = to_wf.diag_id;
    return result;
  }

  const auto from_size = SizeOfInternal(ctx, from.type);
  const auto to_size = SizeOfInternal(ctx, to.type);
  if (!from_size.has_value() || !to_size.has_value()) {
    return result;
  }
  if (*from_size != *to_size) {
    SPEC_RULE("T-Transmute-SizeEq");
    result.diag_id = "T-Transmute-SizeEq";
    return result;
  }
  SPEC_RULE("T-Transmute-SizeEq");

  const auto from_align = AlignOfInternal(ctx, from.type);
  const auto to_align = AlignOfInternal(ctx, to.type);
  if (!from_align.has_value() || !to_align.has_value()) {
    return result;
  }
  if (*from_align != *to_align) {
    SPEC_RULE("T-Transmute-AlignEq");
    result.diag_id = "T-Transmute-AlignEq";
    return result;
  }
  SPEC_RULE("T-Transmute-AlignEq");

  const auto value = TypeExpr(ctx, type_ctx, expr.value, env);
  if (!value.ok) {
    result.diag_id = value.diag_id;
    return result;
  }
  const auto equiv = TypeEquiv(value.type, from.type);
  if (!equiv.ok) {
    result.diag_id = equiv.diag_id;
    return result;
  }
  if (!equiv.equiv) {
    return result;
  }

  SPEC_RULE("T-Transmute");
  result.ok = true;
  result.type = to.type;
  return result;
}

TypeWfResult TypeWFImpl(const ScopeContext& ctx, const TypeRef& type) {
  SpecDefsTypeExpr();
  if (!type) {
    return {};
  }
  return std::visit(
      [&](const auto& node) -> TypeWfResult {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, TypePrim>) {
          if (!IsPrimTypeName(node.name)) {
            return {};
          }
          SPEC_RULE("WF-Prim");
          return {true, std::nullopt};
        } else if constexpr (std::is_same_v<T, TypePerm>) {
          const auto base = TypeWFImpl(ctx, node.base);
          if (!base.ok) {
            return base;
          }
          SPEC_RULE("WF-Perm");
          return {true, std::nullopt};
        } else if constexpr (std::is_same_v<T, TypeTuple>) {
          for (const auto& elem : node.elements) {
            const auto wf = TypeWFImpl(ctx, elem);
            if (!wf.ok) {
              return wf;
            }
          }
          SPEC_RULE("WF-Tuple");
          return {true, std::nullopt};
        } else if constexpr (std::is_same_v<T, TypeArray>) {
          const auto wf = TypeWFImpl(ctx, node.element);
          if (!wf.ok) {
            return wf;
          }
          SPEC_RULE("WF-Array");
          return {true, std::nullopt};
        } else if constexpr (std::is_same_v<T, TypeSlice>) {
          const auto wf = TypeWFImpl(ctx, node.element);
          if (!wf.ok) {
            return wf;
          }
          SPEC_RULE("WF-Slice");
          return {true, std::nullopt};
        } else if constexpr (std::is_same_v<T, TypeUnion>) {
          if (node.members.size() < 2) {
            SPEC_RULE("WF-Union-TooFew");
            return {false, "WF-Union-TooFew"};
          }
          for (const auto& member : node.members) {
            const auto wf = TypeWFImpl(ctx, member);
            if (!wf.ok) {
              return wf;
            }
          }
          SPEC_RULE("WF-Union");
          return {true, std::nullopt};
        } else if constexpr (std::is_same_v<T, TypeFunc>) {
          for (const auto& param : node.params) {
            const auto wf = TypeWFImpl(ctx, param.type);
            if (!wf.ok) {
              return wf;
            }
          }
          const auto ret = TypeWFImpl(ctx, node.ret);
          if (!ret.ok) {
            return ret;
          }
          SPEC_RULE("WF-Func");
          return {true, std::nullopt};
        } else if constexpr (std::is_same_v<T, TypePathType>) {
          if (IsSelfTypePath(node.path)) {
            SPEC_RULE("WF-Path");
            return {true, std::nullopt};
          }
          if (ctx.sigma.types.find(PathKeyOf(node.path)) == ctx.sigma.types.end()) {
            return {};
          }
          SPEC_RULE("WF-Path");
          return {true, std::nullopt};
        } else if constexpr (std::is_same_v<T, TypeDynamic>) {
          if (IsBuiltinCapClassPath(node.path)) {
            SPEC_RULE("WF-Dynamic");
            return {true, std::nullopt};
          }
          if (ctx.sigma.classes.find(PathKeyOf(node.path)) == ctx.sigma.classes.end()) {
            SPEC_RULE("WF-Dynamic-Err");
            return {false, "Superclass-Undefined"};
          }
          SPEC_RULE("WF-Dynamic");
          return {true, std::nullopt};
        } else if constexpr (std::is_same_v<T, TypeOpaque>) {
          syntax::Path class_path;
          class_path.reserve(node.class_path.size());
          for (const auto& comp : node.class_path) {
            class_path.push_back(comp);
          }
          if (ctx.sigma.classes.find(PathKeyOf(class_path)) == ctx.sigma.classes.end()) {
            SPEC_RULE("WF-Opaque-Err");
            return {false, "Superclass-Undefined"};
          }
          SPEC_RULE("WF-Opaque");
          return {true, std::nullopt};
        } else if constexpr (std::is_same_v<T, TypeRefine>) {
          const auto base = TypeWFImpl(ctx, node.base);
          if (!base.ok) {
            return base;
          }
          if (!node.predicate) {
            return {false, std::nullopt};
          }
          TypeEnv env;
          TypeScope scope;
          scope.emplace(IdKeyOf("self"),
                        TypeBinding{syntax::Mutability::Let, node.base});
          env.scopes.push_back(std::move(scope));
          StmtTypeContext type_ctx;
          type_ctx.return_type = MakeTypePrim("bool");
          const auto pred_type = TypeExpr(ctx, type_ctx, node.predicate, env);
          if (!pred_type.ok) {
            return {false, pred_type.diag_id};
          }
          if (!IsPrimType(pred_type.type, "bool")) {
            return {false, "E-TYP-1955"};
          }
          const auto purity = CheckPurity(node.predicate);
          if (!purity.ok) {
            return {false, std::optional<std::string_view>{"E-TYP-1954"}};
          }
          SPEC_RULE("WF-Refine-Type");
          return {true, std::nullopt};
        } else if constexpr (std::is_same_v<T, TypeString>) {
          SPEC_RULE("WF-String");
          return {true, std::nullopt};
        } else if constexpr (std::is_same_v<T, TypeBytes>) {
          SPEC_RULE("WF-Bytes");
          return {true, std::nullopt};
        } else if constexpr (std::is_same_v<T, TypePtr>) {
          const auto wf = TypeWFImpl(ctx, node.element);
          if (!wf.ok) {
            return wf;
          }
          SPEC_RULE("WF-Ptr");
          return {true, std::nullopt};
        } else if constexpr (std::is_same_v<T, TypeRawPtr>) {
          const auto wf = TypeWFImpl(ctx, node.element);
          if (!wf.ok) {
            return wf;
          }
          SPEC_RULE("WF-RawPtr");
          return {true, std::nullopt};
        } else if constexpr (std::is_same_v<T, TypeModalState>) {
          const auto* decl = LookupModalDecl(ctx, node.path);
          if (!decl || !HasState(*decl, node.state)) {
            return {};
          }
          SPEC_RULE("WF-ModalState");
          return {true, std::nullopt};
        } else if constexpr (std::is_same_v<T, TypeRange>) {
          return {true, std::nullopt};
        } else {
          return {};
        }
      },
      type->node);
}
}  // namespace

//=============================================================================
// Public API
//=============================================================================

TypeRef StripPerm(const TypeRef& type) {
  SpecDefsTypeExpr();
  if (!type) {
    return type;
  }
  if (const auto* perm = std::get_if<TypePerm>(&type->node)) {
    return StripPerm(perm->base);
  }
  if (const auto* refine = std::get_if<TypeRefine>(&type->node)) {
    return StripPerm(refine->base);
  }
  return type;
}

TypeWfResult TypeWF(const ScopeContext& ctx, const TypeRef& type) {
  return TypeWFImpl(ctx, type);
}

std::optional<std::uint64_t> SizeOf(const ScopeContext& ctx,
                                   const TypeRef& type) {
  SpecDefsTypeExpr();
  return SizeOfInternal(ctx, type);
}

std::optional<std::uint64_t> AlignOf(const ScopeContext& ctx,
                                    const TypeRef& type) {
  SpecDefsTypeExpr();
  return AlignOfInternal(ctx, type);
}

bool IsPlaceExpr(const syntax::ExprPtr& expr) {
  SpecDefsTypeExpr();
  if (!expr) {
    return false;
  }
  return std::visit(
      [](const auto& node) -> bool {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::IdentifierExpr>) {
          return true;
        } else if constexpr (std::is_same_v<T, syntax::FieldAccessExpr>) {
          return true;
        } else if constexpr (std::is_same_v<T, syntax::TupleAccessExpr>) {
          return true;
        } else if constexpr (std::is_same_v<T, syntax::IndexAccessExpr>) {
          return true;
        } else if constexpr (std::is_same_v<T, syntax::DerefExpr>) {
          return IsPlaceExpr(node.value);
        } else {
          return false;
        }
      },
      expr->node);
}

static bool BuiltinBitcopyType(const TypeRef& type) {
  if (!type) {
    return false;
  }
  return std::visit(
      [](const auto& node) -> bool {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, TypePrim>) {
          return IsPrimTypeName(node.name);
        } else if constexpr (std::is_same_v<T, TypePtr> ||
                             std::is_same_v<T, TypeRawPtr> ||
                             std::is_same_v<T, TypeSlice> ||
                             std::is_same_v<T, TypeFunc> ||
                             std::is_same_v<T, TypeDynamic> ||
                             std::is_same_v<T, TypeRange>) {
          return true;
        } else if constexpr (std::is_same_v<T, TypeString>) {
          return node.state.has_value() &&
                 *node.state == StringState::View;
        } else if constexpr (std::is_same_v<T, TypeBytes>) {
          return node.state.has_value() &&
                 *node.state == BytesState::View;
        } else {
          return false;
        }
      },
      type->node);
}

static bool IsBitcopyClassPath(const syntax::ClassPath& path) {
  return !path.empty() && IdEq(path.back(), "Bitcopy");
}

static bool ImplementsBitcopy(const ScopeContext& ctx, const TypeRef& type) {
  const auto* path_type = std::get_if<TypePathType>(&type->node);
  if (!path_type) {
    return false;
  }

  syntax::Path syntax_path;
  syntax_path.reserve(path_type->path.size());
  for (const auto& comp : path_type->path) {
    syntax_path.push_back(comp);
  }
  const auto it = ctx.sigma.types.find(PathKeyOf(syntax_path));
  if (it == ctx.sigma.types.end()) {
    return false;
  }

  auto has_bitcopy = [](const std::vector<syntax::ClassPath>& impls) {
    for (const auto& impl : impls) {
      if (IsBitcopyClassPath(impl)) {
        return true;
      }
    }
    return false;
  };

  if (const auto* record = std::get_if<syntax::RecordDecl>(&it->second)) {
    return has_bitcopy(record->implements);
  }
  if (const auto* enum_decl = std::get_if<syntax::EnumDecl>(&it->second)) {
    return has_bitcopy(enum_decl->implements);
  }
  if (const auto* modal_decl = std::get_if<syntax::ModalDecl>(&it->second)) {
    return has_bitcopy(modal_decl->implements);
  }
  return false;
}

bool BitcopyType(const ScopeContext& ctx, const TypeRef& type) {
  SpecDefsTypeExpr();
  if (!type) {
    return false;
  }
  if (const auto* perm = std::get_if<TypePerm>(&type->node)) {
    if (perm->perm == Permission::Unique) {
      return false;
    }
    return BitcopyType(ctx, perm->base);
  }
  if (const auto* refine = std::get_if<TypeRefine>(&type->node)) {
    return BitcopyType(ctx, refine->base);
  }
  if (const auto* opaque = std::get_if<TypeOpaque>(&type->node)) {
    if (opaque->origin) {
      const auto it = ctx.sigma.opaque_underlying.find(opaque->origin);
      if (it != ctx.sigma.opaque_underlying.end()) {
        return BitcopyType(ctx, it->second);
      }
    }
    return false;
  }
  if (const auto* path = std::get_if<TypePathType>(&type->node)) {
    syntax::Path syntax_path;
    syntax_path.reserve(path->path.size());
    for (const auto& comp : path->path) {
      syntax_path.push_back(comp);
    }
    const auto it = ctx.sigma.types.find(PathKeyOf(syntax_path));
    if (it != ctx.sigma.types.end()) {
      if (const auto* alias = std::get_if<syntax::TypeAliasDecl>(&it->second)) {
        const auto lowered = LowerType(ctx, alias->type);
        if (lowered.ok && lowered.type) {
          return BitcopyType(ctx, lowered.type);
        }
      }
    }
  }
  if (const auto* tuple = std::get_if<TypeTuple>(&type->node)) {
    for (const auto& elem : tuple->elements) {
      if (!BitcopyType(ctx, elem)) {
        return false;
      }
    }
    return true;
  }
  if (const auto* array = std::get_if<TypeArray>(&type->node)) {
    return BitcopyType(ctx, array->element);
  }
  return BuiltinBitcopyType(type) || ImplementsBitcopy(ctx, type);
}

bool CloneType(const ScopeContext& ctx, const TypeRef& type) {
  SpecDefsTypeExpr();
  if (!type) {
    return false;
  }
  if (const auto* perm = std::get_if<TypePerm>(&type->node)) {
    return CloneType(ctx, perm->base);
  }
  if (const auto* refine = std::get_if<TypeRefine>(&type->node)) {
    return CloneType(ctx, refine->base);
  }
  if (const auto* opaque = std::get_if<TypeOpaque>(&type->node)) {
    if (opaque->origin) {
      const auto it = ctx.sigma.opaque_underlying.find(opaque->origin);
      if (it != ctx.sigma.opaque_underlying.end()) {
        return CloneType(ctx, it->second);
      }
    }
    return false;
  }
  if (BitcopyType(ctx, type)) {
    return true;
  }
  const syntax::ClassPath clone_path = {"Clone"};
  return TypeImplementsClass(ctx, type, clone_path);
}

bool EqType(const TypeRef& type) {
  SpecDefsTypeExpr();
  const auto stripped = StripPerm(type);
  if (!stripped) {
    return false;
  }

  if (const auto* prim = std::get_if<TypePrim>(&stripped->node)) {
    if (IsNumericType(prim->name) || prim->name == "bool" ||
        prim->name == "char") {
      return true;
    }
  }

  if (std::holds_alternative<TypePtr>(stripped->node)) {
    return true;
  }

  if (std::holds_alternative<TypeRawPtr>(stripped->node)) {
    return true;
  }

  if (std::holds_alternative<TypeString>(stripped->node)) {
    return true;
  }

  if (std::holds_alternative<TypeBytes>(stripped->node)) {
    return true;
  }

  return false;
}

bool OrdType(const TypeRef& type) {
  SpecDefsTypeExpr();
  const auto stripped = StripPerm(type);
  if (!stripped) {
    return false;
  }

  const auto* prim = std::get_if<TypePrim>(&stripped->node);
  if (!prim) {
    return false;
  }

  return IsIntType(prim->name) || IsFloatType(prim->name) ||
         prim->name == "char";
}

bool CastValid(const TypeRef& source, const TypeRef& target) {
  SpecDefsTypeExpr();
  const auto src = StripPerm(source);
  const auto tgt = StripPerm(target);

  if (!src || !tgt) {
    return false;
  }

  const auto* src_prim = std::get_if<TypePrim>(&src->node);
  const auto* tgt_prim = std::get_if<TypePrim>(&tgt->node);

  if (!src_prim || !tgt_prim) {
    return false;
  }

  // Numeric to numeric
  if (IsNumericType(src_prim->name) && IsNumericType(tgt_prim->name)) {
    return true;
  }

  // Bool to int
  if (src_prim->name == "bool" && IsIntType(tgt_prim->name)) {
    return true;
  }

  // Int to bool
  if (IsIntType(src_prim->name) && tgt_prim->name == "bool") {
    return true;
  }

  // Char to u32
  if (src_prim->name == "char" && tgt_prim->name == "u32") {
    return true;
  }

  // U32 to char
  if (src_prim->name == "u32" && tgt_prim->name == "char") {
    return true;
  }

  return false;
}


bool IsInUnsafeSpan(const ScopeContext& ctx, const core::Span& span) {
  SpecDefsTypeExpr();
  if (span.file.empty()) {
    return false;
  }

  const auto range = core::SpanRange(span);
  for (const auto& module : ctx.sigma.mods) {
    for (const auto& file_spans : module.unsafe_spans) {
      if (file_spans.path != span.file) {
        continue;
      }
      for (const auto& sp : file_spans.spans) {
        const auto sp_range = core::SpanRange(sp);
        if (range.first >= sp_range.first && range.second <= sp_range.second) {
          return true;
        }
      }
    }
  }

  return false;
}


ExprTypeResult TypeExpr(const ScopeContext& ctx,
                        const StmtTypeContext& type_ctx,
                        const syntax::ExprPtr& expr,
                        const TypeEnv& env) {
  SpecDefsTypeExpr();
  auto result = TypeExprImpl(ctx, type_ctx, expr, env);
  if (result.ok) {
    if (ctx.expr_types && expr) {
      (*ctx.expr_types)[expr.get()] = result.type;
    }
    SPEC_RULE("Lift-Expr");
  }
  return result;
}

CheckResult CheckExprAgainst(const ScopeContext& ctx,
                             const StmtTypeContext& type_ctx,
                             const syntax::ExprPtr& expr,
                             const TypeRef& expected,
                             const TypeEnv& env) {
  SpecDefsTypeExpr();
  CheckResult result;

  if (!expr || !expected) {
    return result;
  }

  if (const auto* if_expr = std::get_if<syntax::IfExpr>(&expr->node)) {
    return CheckIfExprImpl(ctx, type_ctx, *if_expr, expected, env);
  }

  auto type_expr = [&](const syntax::ExprPtr& inner) {
    return TypeExpr(ctx, type_ctx, inner, env);
  };
  auto type_ident = [&](std::string_view name) -> ExprTypeResult {
    return TypeIdentifierExprImpl(ctx, syntax::IdentifierExpr{std::string(name)},
                                  env);
  };
  PlaceTypeFn type_place = [&](const syntax::ExprPtr& inner) {
    return TypePlace(ctx, type_ctx, inner, env);
  };
  auto match_check = [&](const syntax::MatchExpr& match,
                         const TypeRef& expected_type) -> CheckResult {
    return CheckMatchExpr(ctx, type_ctx, match, env, expected_type);
  };

  const auto check =
      CheckExpr(ctx, expr, expected, type_expr, type_place, type_ident, match_check);
  if (!check.ok) {
    result.diag_id = check.diag_id;
    return result;
  }

  result.ok = true;
  return result;
}


CheckResult CheckPlaceAgainst(const ScopeContext& ctx,
                              const StmtTypeContext& type_ctx,
                              const syntax::ExprPtr& expr,
                              const TypeRef& expected,
                              const TypeEnv& env) {
  SpecDefsTypeExpr();
  CheckResult result;

  if (!expr || !expected) {
    return result;
  }

  const auto place = TypePlace(ctx, type_ctx, expr, env);
  if (!place.ok) {
    result.diag_id = place.diag_id;
    return result;
  }

  const auto sub = Subtyping(ctx, place.type, expected);
  if (!sub.ok) {
    result.diag_id = sub.diag_id;
    return result;
  }
  if (!sub.subtype) {
    return result;
  }

  SPEC_RULE("Place-Check");
  result.ok = true;
  return result;
}

PlaceTypeResult TypePlace(const ScopeContext& ctx,
                          const StmtTypeContext& type_ctx,
                          const syntax::ExprPtr& expr,
                          const TypeEnv& env) {
  auto result = TypePlaceImpl(ctx, type_ctx, expr, env);
  if (result.ok) {
    if (ctx.expr_types && expr) {
      (*ctx.expr_types)[expr.get()] = result.type;
    }
  }
  return result;
}

// Individual expression form handlers (public wrappers)
ExprTypeResult TypeIdentifierExpr(const ScopeContext& ctx,
                                  const syntax::IdentifierExpr& expr,
                                  const TypeEnv& env) {
  return TypeIdentifierExprImpl(ctx, expr, env);
}

ExprTypeResult TypePathExpr(const ScopeContext& ctx,
                            const syntax::PathExpr& expr) {
  return TypePathExprImpl(ctx, expr);
}

ExprTypeResult TypeFieldAccessExpr(const ScopeContext& ctx,
                                   const StmtTypeContext& type_ctx,
                                   const syntax::FieldAccessExpr& expr,
                                   const TypeEnv& env) {
  return TypeFieldAccessExprImpl(ctx, type_ctx, expr, env);
}

PlaceTypeResult TypeFieldAccessPlace(const ScopeContext& ctx,
                                     const StmtTypeContext& type_ctx,
                                     const syntax::FieldAccessExpr& expr,
                                     const TypeEnv& env) {
  return TypeFieldAccessPlaceImpl(ctx, type_ctx, expr, env);
}

ExprTypeResult TypeTupleAccessExpr(const ScopeContext& ctx,
                                   const StmtTypeContext& type_ctx,
                                   const syntax::TupleAccessExpr& expr,
                                   const TypeEnv& env) {
  return TypeTupleAccessExprImpl(ctx, type_ctx, expr, env);
}

PlaceTypeResult TypeTupleAccessPlace(const ScopeContext& ctx,
                                     const StmtTypeContext& type_ctx,
                                     const syntax::TupleAccessExpr& expr,
                                     const TypeEnv& env) {
  return TypeTupleAccessPlaceImpl(ctx, type_ctx, expr, env);
}

ExprTypeResult TypeBinaryExpr(const ScopeContext& ctx,
                              const StmtTypeContext& type_ctx,
                              const syntax::BinaryExpr& expr,
                              const TypeEnv& env) {
  return TypeBinaryExprImpl(ctx, type_ctx, expr, env);
}

ExprTypeResult TypeUnaryExpr(const ScopeContext& ctx,
                             const StmtTypeContext& type_ctx,
                             const syntax::UnaryExpr& expr,
                             const TypeEnv& env,
                             const core::Span& span) {
  return TypeUnaryExprImpl(ctx, type_ctx, expr, env, span);
}

ExprTypeResult TypeCastExpr(const ScopeContext& ctx,
                            const StmtTypeContext& type_ctx,
                            const syntax::CastExpr& expr,
                            const TypeEnv& env) {
  return TypeCastExprImpl(ctx, type_ctx, expr, env);
}

ExprTypeResult TypeIfExpr(const ScopeContext& ctx,
                          const StmtTypeContext& type_ctx,
                          const syntax::IfExpr& expr,
                          const TypeEnv& env) {
  return TypeIfExprImpl(ctx, type_ctx, expr, env);
}

CheckResult CheckIfExpr(const ScopeContext& ctx,
                        const StmtTypeContext& type_ctx,
                        const syntax::IfExpr& expr,
                        const TypeRef& expected,
                        const TypeEnv& env) {
  return CheckIfExprImpl(ctx, type_ctx, expr, expected, env);
}

ExprTypeResult TypeRangeExpr(const ScopeContext& ctx,
                             const StmtTypeContext& type_ctx,
                             const syntax::RangeExpr& expr,
                             const TypeEnv& env) {
  return TypeRangeExprImpl(ctx, type_ctx, expr, env);
}

ExprTypeResult TypeAddressOfExpr(const ScopeContext& ctx,
                                 const StmtTypeContext& type_ctx,
                                 const syntax::AddressOfExpr& expr,
                                 const TypeEnv& env) {
  return TypeAddressOfExprImpl(ctx, type_ctx, expr, env);
}

ExprTypeResult TypeDerefExpr(const ScopeContext& ctx,
                             const StmtTypeContext& type_ctx,
                             const syntax::DerefExpr& expr,
                             const TypeEnv& env) {
  return TypeDerefExprImpl(ctx, type_ctx, expr, env, core::Span{});
}

PlaceTypeResult TypeDerefPlace(const ScopeContext& ctx,
                               const StmtTypeContext& type_ctx,
                               const syntax::DerefExpr& expr,
                               const TypeEnv& env) {
  return TypeDerefPlaceImpl(ctx, type_ctx, expr, env, core::Span{});
}

ExprTypeResult TypeMoveExpr(const ScopeContext& ctx,
                            const StmtTypeContext& type_ctx,
                            const syntax::MoveExpr& expr,
                            const TypeEnv& env) {
  return TypeMoveExprImpl(ctx, type_ctx, expr, env);
}

ExprTypeResult TypeAllocExpr(const ScopeContext& ctx,
                             const StmtTypeContext& type_ctx,
                             const syntax::AllocExpr& expr,
                             const TypeEnv& env) {
  SpecDefsTypeExpr();
  ExprTypeResult result;
  if (type_ctx.require_pure) {
    result.diag_id = "E-SEM-2802";
    return result;
  }
  if (!expr.value) {
    return result;
  }

  if (expr.region_opt.has_value()) {
    const auto binding = BindOf(env, *expr.region_opt);
    if (!binding.has_value()) {
      result.diag_id = "ResolveExpr-Ident-Err";
      return result;
    }
    if (!RegionActiveType(binding->type)) {
      SPEC_RULE("Alloc-Region-NotFound-Err");
      result.diag_id = "Alloc-Region-NotFound-Err";
      return result;
    }
    const auto inner = TypeExpr(ctx, type_ctx, expr.value, env);
    if (!inner.ok) {
      result.diag_id = inner.diag_id;
      return result;
    }
    SPEC_RULE("T-Alloc-Explicit");
    result.ok = true;
    result.type = inner.type;
    return result;
  }

  const auto region = InnermostActiveRegion(env);
  if (!region.has_value()) {
    SPEC_RULE("Alloc-Implicit-NoRegion-Err");
    result.diag_id = "Alloc-Implicit-NoRegion-Err";
    return result;
  }

  const auto inner = TypeExpr(ctx, type_ctx, expr.value, env);
  if (!inner.ok) {
    result.diag_id = inner.diag_id;
    return result;
  }
  SPEC_RULE("T-Alloc-Implicit");
  result.ok = true;
  result.type = inner.type;
  return result;
}




ExprTypeResult TypeTransmuteExpr(const ScopeContext& ctx,
                                 const StmtTypeContext& type_ctx,
                                 const syntax::TransmuteExpr& expr,
                                 const TypeEnv& env) {
  return TypeTransmuteExprImpl(ctx, type_ctx, expr, env, core::Span{});
}

ExprTypeResult TypePropagateExpr(const ScopeContext& ctx,
                                 const StmtTypeContext& type_ctx,
                                 const syntax::PropagateExpr& expr,
                                 const TypeEnv& env) {
  return TypePropagateExprImpl(ctx, type_ctx, expr, env);
}

ExprTypeResult TypeRecordExpr(const ScopeContext& ctx,
                              const StmtTypeContext& type_ctx,
                              const syntax::RecordExpr& expr,
                              const TypeEnv& env) {
  return TypeRecordExprImpl(ctx, type_ctx, expr, env);
}

ExprTypeResult TypeEnumLiteralExpr(const ScopeContext& ctx,
                                   const StmtTypeContext& type_ctx,
                                   const syntax::EnumLiteralExpr& expr,
                                   const TypeEnv& env) {
  return TypeEnumLiteralExprImpl(ctx, type_ctx, expr, env);
}

ExprTypeResult TypeErrorExpr(const ScopeContext& ctx,
                             const syntax::ErrorExpr& expr) {
  return TypeErrorExprImpl(ctx, expr);
}

}  // namespace cursive0::analysis
