#include "cursive0/03_analysis/types/type_equiv.h"

#include <array>
#include <cstdint>
#include <limits>
#include <optional>
#include <string_view>
#include <type_traits>

#include "cursive0/00_core/assert_spec.h"
#include "cursive0/00_core/int128.h"
#include "cursive0/00_core/symbols.h"
#include "cursive0/03_analysis/contracts/verification.h"
#include "cursive0/03_analysis/resolve/collect_toplevel.h"
#include "cursive0/03_analysis/resolve/scopes.h"
#include "cursive0/03_analysis/resolve/scopes_lookup.h"
#include "cursive0/03_analysis/resolve/visibility.h"

namespace cursive0::analysis {

namespace {

static inline void SpecDefsTypeEquiv() {
  SPEC_DEF("TypeEqJudg", "5.2.1");
  SPEC_DEF("ConstLenJudg", "5.2.1");
  SPEC_DEF("MembersEq", "5.2.1");
}

static constexpr unsigned kPointerSizeBytes = 8;
static constexpr unsigned kPointerSizeBits = 8 * kPointerSizeBytes;

static constexpr std::array<std::string_view, 12> kIntSuffixes = {
    "i128", "u128", "isize", "usize", "i64", "u64",
    "i32",  "u32",  "i16",   "u16",  "i8",  "u8"};

static bool EndsWith(std::string_view value, std::string_view suffix) {
  if (suffix.size() > value.size()) {
    return false;
  }
  return value.substr(value.size() - suffix.size()) == suffix;
}

static std::string_view StripIntSuffix(std::string_view lexeme) {
  for (const auto suffix : kIntSuffixes) {
    if (EndsWith(lexeme, suffix)) {
      const std::size_t core_len = lexeme.size() - suffix.size();
      if (core_len == 0) {
        continue;
      }
      return lexeme.substr(0, core_len);
    }
  }
  return lexeme;
}

static bool DigitValue(char c, unsigned base, unsigned* out) {
  if (c >= '0' && c <= '9') {
    unsigned digit = static_cast<unsigned>(c - '0');
    if (digit < base) {
      *out = digit;
      return true;
    }
    return false;
  }
  if (base <= 10) {
    return false;
  }
  if (c >= 'a' && c <= 'f') {
    unsigned digit = static_cast<unsigned>(10 + (c - 'a'));
    if (digit < base) {
      *out = digit;
      return true;
    }
    return false;
  }
  if (c >= 'A' && c <= 'F') {
    unsigned digit = static_cast<unsigned>(10 + (c - 'A'));
    if (digit < base) {
      *out = digit;
      return true;
    }
    return false;
  }
  return false;
}

static bool ParseIntCore(std::string_view core,
                         core::UInt128& value_out) {
  unsigned base = 10;
  std::string_view digits = core;
  if (core.size() >= 2 && core[0] == '0') {
    const char prefix = core[1];
    if (prefix == 'x' || prefix == 'X') {
      base = 16;
      digits = core.substr(2);
    } else if (prefix == 'o' || prefix == 'O') {
      base = 8;
      digits = core.substr(2);
    } else if (prefix == 'b' || prefix == 'B') {
      base = 2;
      digits = core.substr(2);
    }
  }
  if (digits.empty()) {
    return false;
  }
  core::UInt128 value = core::UInt128FromU64(0);
  const core::UInt128 max = core::UInt128Max();
  const core::UInt128 base128 = core::UInt128FromU64(base);
  bool saw_digit = false;
  for (char c : digits) {
    if (c == '_') {
      continue;
    }
    unsigned digit = 0;
    if (!DigitValue(c, base, &digit)) {
      return false;
    }
    saw_digit = true;
    const core::UInt128 digit128 = core::UInt128FromU64(digit);
    // Check for overflow: value > (max - digit) / base
    const core::UInt128 max_minus_digit = core::UInt128Sub(max, digit128);
    const core::UInt128 threshold = core::UInt128Div(max_minus_digit, base128);
    if (core::UInt128Greater(value, threshold)) {
      return false;
    }
    value = core::UInt128Add(core::UInt128Mul(value, base128), digit128);
  }
  if (!saw_digit) {
    return false;
  }
  value_out = value;
  return true;
}

static std::optional<unsigned> IntWidthOf(std::string_view name) {
  if (name == "i8" || name == "u8") {
    return 8;
  }
  if (name == "i16" || name == "u16") {
    return 16;
  }
  if (name == "i32" || name == "u32") {
    return 32;
  }
  if (name == "i64" || name == "u64") {
    return 64;
  }
  if (name == "i128" || name == "u128") {
    return 128;
  }
  if (name == "isize" || name == "usize") {
    return kPointerSizeBits;
  }
  return std::nullopt;
}

static bool IsUnsignedIntType(std::string_view name) {
  return name == "u8" || name == "u16" || name == "u32" || name == "u64" ||
         name == "u128" || name == "usize";
}

static bool IsSignedIntType(std::string_view name) {
  return name == "i8" || name == "i16" || name == "i32" || name == "i64" ||
         name == "i128" || name == "isize";
}

static bool InRangeUnsigned(core::UInt128 value, unsigned width) {
  if (width >= 128) {
    return true;
  }
  const core::UInt128 one = core::UInt128FromU64(1);
  const core::UInt128 max =
      core::UInt128Sub(core::UInt128ShiftLeft(one, width), one);
  return core::UInt128LessOrEqual(value, max);
}

static bool InRangeSigned(core::UInt128 value, unsigned width) {
  if (width == 0) {
    return false;
  }
  if (width >= 128) {
    const core::UInt128 one = core::UInt128FromU64(1);
    const core::UInt128 max =
        core::UInt128Sub(core::UInt128ShiftLeft(one, 127), one);
    return core::UInt128LessOrEqual(value, max);
  }
  const core::UInt128 one = core::UInt128FromU64(1);
  const core::UInt128 max =
      core::UInt128Sub(core::UInt128ShiftLeft(one, width - 1), one);
  return core::UInt128LessOrEqual(value, max);
}

static bool InRangeInt(core::UInt128 value, std::string_view name) {
  const auto width = IntWidthOf(name);
  if (!width.has_value()) {
    return false;
  }
  if (IsUnsignedIntType(name)) {
    return InRangeUnsigned(value, *width);
  }
  if (IsSignedIntType(name)) {
    return InRangeSigned(value, *width);
  }
  return false;
}

static std::optional<std::uint64_t> ParseIntLiteralUsize(
    std::string_view lexeme) {
  const std::string_view core = StripIntSuffix(lexeme);
  core::UInt128 value = core::UInt128FromU64(0);
  if (!ParseIntCore(core, value)) {
    return std::nullopt;
  }
  if (!InRangeInt(value, "usize")) {
    return std::nullopt;
  }
  if (!core::UInt128FitsU64(value)) {
    return std::nullopt;
  }
  return core::UInt128ToU64(value);
}

static bool TypePathEq(const TypePath& lhs, const TypePath& rhs) {
  if (lhs.size() != rhs.size()) {
    return false;
  }
  for (std::size_t i = 0; i < lhs.size(); ++i) {
    if (IdKeyOf(lhs[i]) != IdKeyOf(rhs[i])) {
      return false;
    }
  }
  return true;
}

static bool SpanEq(const core::Span& lhs, const core::Span& rhs) {
  return lhs.file == rhs.file &&
         lhs.start_offset == rhs.start_offset &&
         lhs.end_offset == rhs.end_offset;
}

static const syntax::ASTModule* FindModule(const ScopeContext& ctx,
                                           const syntax::ModulePath& path) {
  for (const auto& mod : ctx.sigma.mods) {
    if (PathEq(mod.path, path)) {
      return &mod;
    }
  }
  return nullptr;
}

static std::optional<syntax::ExprPtr> FindStaticInit(
    const syntax::ASTModule& module,
    std::string_view name) {
  for (const auto& item : module.items) {
    const auto* decl = std::get_if<syntax::StaticDecl>(&item);
    if (!decl || !decl->binding.pat) {
      continue;
    }
    const auto& pat = *decl->binding.pat;
    const auto* ident = std::get_if<syntax::IdentifierPattern>(&pat.node);
    if (!ident) {
      continue;
    }
    if (!IdEq(ident->name, name)) {
      continue;
    }
    if (decl->binding.op.kind != syntax::TokenKind::Operator ||
        decl->binding.op.lexeme != "=") {
      continue;
    }
    return decl->binding.init;
  }
  return std::nullopt;
}

static ModuleNames ModuleNamesForConstLen(const ScopeContext& ctx) {
  if (ctx.project) {
    return ModuleNamesOf(*ctx.project);
  }
  ModuleNames names;
  names.reserve(ctx.sigma.mods.size());
  for (const auto& mod : ctx.sigma.mods) {
    names.push_back(core::StringOfPath(mod.path));
  }
  return names;
}

static std::optional<std::pair<syntax::ModulePath, syntax::Identifier>>
ResolveValuePathForConstLen(const ScopeContext& ctx,
                            const syntax::ModulePath& path,
                            std::string_view name) {
  ScopeContext local = ctx;
  const auto current_module = local.current_module;
  const auto name_maps = CollectNameMaps(local);
  local.current_module = current_module;
  const auto module_names = ModuleNamesForConstLen(local);
  const auto value = ResolveQualified(local, name_maps.name_maps, module_names,
                                      path, name, EntityKind::Value, CanAccess);
  if (!value.ok || !value.entity || !value.entity->origin_opt) {
    return std::nullopt;
  }
  syntax::Identifier resolved_name =
      value.entity->target_opt ? *value.entity->target_opt
                               : syntax::Identifier(name);
  return std::make_pair(*value.entity->origin_opt, std::move(resolved_name));
}

}  // namespace

ConstLenResult ConstLen(const ScopeContext& ctx, const syntax::ExprPtr& expr) {
  SpecDefsTypeEquiv();
  if (!expr) {
    SPEC_RULE("ConstLen-Err");
    return {false, "ConstLen-Err", std::nullopt};
  }
  return std::visit(
      [&](const auto& node) -> ConstLenResult {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::LiteralExpr>) {
          if (node.literal.kind != syntax::TokenKind::IntLiteral) {
            SPEC_RULE("ConstLen-Err");
            return {false, "ConstLen-Err", std::nullopt};
          }
          const auto value = ParseIntLiteralUsize(node.literal.lexeme);
          if (!value.has_value()) {
            SPEC_RULE("ConstLen-Err");
            return {false, "ConstLen-Err", std::nullopt};
          }
          SPEC_RULE("ConstLen-Lit");
          return {true, std::nullopt, *value};
        } else if constexpr (std::is_same_v<T, syntax::IdentifierExpr>) {
          const auto ent = ResolveValueName(ctx, node.name);
          if (!ent || !ent->origin_opt) {
            SPEC_RULE("ConstLen-Err");
            return {false, "ConstLen-Err", std::nullopt};
          }
          const auto* module = FindModule(ctx, *ent->origin_opt);
          if (!module) {
            SPEC_RULE("ConstLen-Err");
            return {false, "ConstLen-Err", std::nullopt};
          }
          const auto resolved_name = ent->target_opt.value_or(node.name);
          const auto init = FindStaticInit(*module, resolved_name);
          if (!init.has_value()) {
            SPEC_RULE("ConstLen-Err");
            return {false, "ConstLen-Err", std::nullopt};
          }
          const auto nested = ConstLen(ctx, *init);
          if (!nested.ok) {
            return nested;
          }
          SPEC_RULE("ConstLen-Path");
          return nested;
        } else if constexpr (std::is_same_v<T, syntax::PathExpr>) {
          const auto resolved =
              ResolveValuePathForConstLen(ctx, node.path, node.name);
          if (!resolved.has_value()) {
            SPEC_RULE("ConstLen-Err");
            return {false, "ConstLen-Err", std::nullopt};
          }
          const auto* module = FindModule(ctx, resolved->first);
          if (!module) {
            SPEC_RULE("ConstLen-Err");
            return {false, "ConstLen-Err", std::nullopt};
          }
          const auto init = FindStaticInit(*module, resolved->second);
          if (!init.has_value()) {
            SPEC_RULE("ConstLen-Err");
            return {false, "ConstLen-Err", std::nullopt};
          }
          const auto nested = ConstLen(ctx, *init);
          if (!nested.ok) {
            return nested;
          }
          SPEC_RULE("ConstLen-Path");
          return nested;
        } else {
          SPEC_RULE("ConstLen-Err");
          return {false, "ConstLen-Err", std::nullopt};
        }
      },
      expr->node);
}

TypeEquivResult TypeEquiv(const TypeRef& lhs, const TypeRef& rhs) {
  SpecDefsTypeEquiv();
  if (!lhs || !rhs) {
    return {true, std::nullopt, false};
  }
  if (lhs.get() == rhs.get()) {
    return {true, std::nullopt, true};
  }
  return std::visit(
      [&](const auto& node) -> TypeEquivResult {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, TypePrim>) {
          const auto* other = std::get_if<TypePrim>(&rhs->node);
          if (!other) {
            return {true, std::nullopt, false};
          }
          SPEC_RULE("T-Equiv-Prim");
          return {true, std::nullopt, node.name == other->name};
        } else if constexpr (std::is_same_v<T, TypePerm>) {
          const auto* other = std::get_if<TypePerm>(&rhs->node);
          if (!other) {
            return {true, std::nullopt, false};
          }
          SPEC_RULE("T-Equiv-Perm");
          if (node.perm != other->perm) {
            return {true, std::nullopt, false};
          }
          return TypeEquiv(node.base, other->base);
        } else if constexpr (std::is_same_v<T, TypeTuple>) {
          const auto* other = std::get_if<TypeTuple>(&rhs->node);
          if (!other) {
            return {true, std::nullopt, false};
          }
          SPEC_RULE("T-Equiv-Tuple");
          if (node.elements.size() != other->elements.size()) {
            return {true, std::nullopt, false};
          }
          for (std::size_t i = 0; i < node.elements.size(); ++i) {
            const auto res = TypeEquiv(node.elements[i], other->elements[i]);
            if (!res.ok || !res.equiv) {
              return res.ok ? TypeEquivResult{true, std::nullopt, false} : res;
            }
          }
          return {true, std::nullopt, true};
        } else if constexpr (std::is_same_v<T, TypeArray>) {
          const auto* other = std::get_if<TypeArray>(&rhs->node);
          if (!other) {
            return {true, std::nullopt, false};
          }
          SPEC_RULE("T-Equiv-Array");
          if (node.length != other->length) {
            return {true, std::nullopt, false};
          }
          return TypeEquiv(node.element, other->element);
        } else if constexpr (std::is_same_v<T, TypeSlice>) {
          const auto* other = std::get_if<TypeSlice>(&rhs->node);
          if (!other) {
            return {true, std::nullopt, false};
          }
          SPEC_RULE("T-Equiv-Slice");
          return TypeEquiv(node.element, other->element);
        } else if constexpr (std::is_same_v<T, TypeFunc>) {
          const auto* other = std::get_if<TypeFunc>(&rhs->node);
          if (!other) {
            return {true, std::nullopt, false};
          }
          SPEC_RULE("T-Equiv-Func");
          if (node.params.size() != other->params.size()) {
            return {true, std::nullopt, false};
          }
          for (std::size_t i = 0; i < node.params.size(); ++i) {
            const auto& lp = node.params[i];
            const auto& rp = other->params[i];
            if (lp.mode != rp.mode) {
              return {true, std::nullopt, false};
            }
            const auto res = TypeEquiv(lp.type, rp.type);
            if (!res.ok || !res.equiv) {
              return res.ok ? TypeEquivResult{true, std::nullopt, false} : res;
            }
          }
          return TypeEquiv(node.ret, other->ret);
        } else if constexpr (std::is_same_v<T, TypeUnion>) {
          const auto* other = std::get_if<TypeUnion>(&rhs->node);
          if (!other) {
            return {true, std::nullopt, false};
          }
          SPEC_RULE("T-Equiv-Union");
          const auto lhs_sorted = SortUnionMembers(node.members);
          const auto rhs_sorted = SortUnionMembers(other->members);
          if (lhs_sorted.size() != rhs_sorted.size()) {
            return {true, std::nullopt, false};
          }
          for (std::size_t i = 0; i < lhs_sorted.size(); ++i) {
            const auto res = TypeEquiv(lhs_sorted[i], rhs_sorted[i]);
            if (!res.ok || !res.equiv) {
              return res.ok ? TypeEquivResult{true, std::nullopt, false} : res;
            }
          }
          return {true, std::nullopt, true};
        } else if constexpr (std::is_same_v<T, TypePathType>) {
          const auto* other = std::get_if<TypePathType>(&rhs->node);
          if (!other) {
            return {true, std::nullopt, false};
          }
          SPEC_RULE("T-Equiv-Path");
          if (!PathEq(node.path, other->path)) {
            return {true, std::nullopt, false};
          }
          if (node.generic_args.size() != other->generic_args.size()) {
            return {true, std::nullopt, false};
          }
          for (std::size_t i = 0; i < node.generic_args.size(); ++i) {
            const auto res = TypeEquiv(node.generic_args[i], other->generic_args[i]);
            if (!res.ok || !res.equiv) {
              return res.ok ? TypeEquivResult{true, std::nullopt, false} : res;
            }
          }
          return {true, std::nullopt, true};
        } else if constexpr (std::is_same_v<T, TypeModalState>) {
          const auto* other = std::get_if<TypeModalState>(&rhs->node);
          if (!other) {
            return {true, std::nullopt, false};
          }
          SPEC_RULE("T-Equiv-ModalState");
          if (!PathEq(node.path, other->path) || node.state != other->state) {
            return {true, std::nullopt, false};
          }
          if (node.generic_args.size() != other->generic_args.size()) {
            return {true, std::nullopt, false};
          }
          for (std::size_t i = 0; i < node.generic_args.size(); ++i) {
            const auto res = TypeEquiv(node.generic_args[i], other->generic_args[i]);
            if (!res.ok || !res.equiv) {
              return res.ok ? TypeEquivResult{true, std::nullopt, false} : res;
            }
          }
          return {true, std::nullopt, true};
        } else if constexpr (std::is_same_v<T, TypeString>) {
          const auto* other = std::get_if<TypeString>(&rhs->node);
          if (!other) {
            return {true, std::nullopt, false};
          }
          SPEC_RULE("T-Equiv-String");
          return {true, std::nullopt, node.state == other->state};
        } else if constexpr (std::is_same_v<T, TypeBytes>) {
          const auto* other = std::get_if<TypeBytes>(&rhs->node);
          if (!other) {
            return {true, std::nullopt, false};
          }
          SPEC_RULE("T-Equiv-Bytes");
          return {true, std::nullopt, node.state == other->state};
        } else if constexpr (std::is_same_v<T, TypeRange>) {
          const auto* other = std::get_if<TypeRange>(&rhs->node);
          if (!other) {
            return {true, std::nullopt, false};
          }
          SPEC_RULE("T-Equiv-Range");
          return {true, std::nullopt, true};
        } else if constexpr (std::is_same_v<T, TypePtr>) {
          const auto* other = std::get_if<TypePtr>(&rhs->node);
          if (!other) {
            return {true, std::nullopt, false};
          }
          SPEC_RULE("T-Equiv-Ptr");
          if (node.state != other->state) {
            return {true, std::nullopt, false};
          }
          return TypeEquiv(node.element, other->element);
        } else if constexpr (std::is_same_v<T, TypeRawPtr>) {
          const auto* other = std::get_if<TypeRawPtr>(&rhs->node);
          if (!other) {
            return {true, std::nullopt, false};
          }
          SPEC_RULE("T-Equiv-RawPtr");
          if (node.qual != other->qual) {
            return {true, std::nullopt, false};
          }
          return TypeEquiv(node.element, other->element);
        } else if constexpr (std::is_same_v<T, TypeDynamic>) {
          const auto* other = std::get_if<TypeDynamic>(&rhs->node);
          if (!other) {
            return {true, std::nullopt, false};
          }
          SPEC_RULE("T-Equiv-Dynamic");
          return {true, std::nullopt, PathEq(node.path, other->path)};
        } else if constexpr (std::is_same_v<T, TypeOpaque>) {
          const auto* other = std::get_if<TypeOpaque>(&rhs->node);
          if (!other) {
            return {true, std::nullopt, false};
          }
          SPEC_RULE("T-Equiv-Opaque");
          if (!TypePathEq(node.class_path, other->class_path)) {
            return {true, std::nullopt, false};
          }
          return {true, std::nullopt, SpanEq(node.origin_span, other->origin_span)};
        } else if constexpr (std::is_same_v<T, TypeRefine>) {
          const auto* other = std::get_if<TypeRefine>(&rhs->node);
          if (!other) {
            return {true, std::nullopt, false};
          }
          const auto base = TypeEquiv(node.base, other->base);
          if (!base.ok || !base.equiv) {
            return base.ok ? TypeEquivResult{true, std::nullopt, false} : base;
          }
          SPEC_RULE("T-Equiv-Refine");
          return {true, std::nullopt,
                  ExprStructEqual(node.predicate, other->predicate)};
        } else {
          return {true, std::nullopt, false};
        }
      },
      lhs->node);
}

}  // namespace cursive0::analysis
