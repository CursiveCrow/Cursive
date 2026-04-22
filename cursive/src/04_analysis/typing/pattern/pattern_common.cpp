// =============================================================================
// pattern_common.cpp - Pattern Type Checking Common Utilities
// =============================================================================
//
// SPEC REFERENCE: CursiveSpecification.md
//   Section 5.4: If-Is Pattern Case Analysis
//   - PatJudg (line 9727): Pattern judgment
//   - Pat-Dup-Err (lines 9729-9732): Duplicate pattern name
//   - Distinct(PatNames(pat)) requirement
//   - Irrefutable vs refutable patterns
//
// =============================================================================

#include "04_analysis/typing/type_pattern.h"

#include <array>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

#include "00_core/assert_spec.h"
#include "00_core/int128.h"
#include "04_analysis/modal/modal.h"
#include "04_analysis/generics/monomorphize.h"
#include "04_analysis/resolve/resolve_items.h"
#include "04_analysis/resolve/scopes.h"
#include "04_analysis/typing/literals.h"
#include "04_analysis/typing/subtyping.h"
#include "04_analysis/typing/type_equiv.h"
#include "04_analysis/typing/type_expr.h"
#include "04_analysis/typing/type_lower.h"
#include "04_analysis/typing/types.h"

namespace cursive::analysis {

namespace {

static inline void SpecDefsTypePattern() {
  SPEC_DEF("CaseJudg", "5.2.13");
  SPEC_DEF("StripPerm", "5.2.12");
  SPEC_DEF("ConstPatInt", "5.2.13");
  SPEC_DEF("PatternEffectRules", "5.2.13");
}

// -----------------------------------------------------------------------------
// Permission Stripping
// -----------------------------------------------------------------------------

static TypeRef StripPermOnce(const TypeRef& type) {
  if (!type) {
    return type;
  }
  if (const auto* perm = std::get_if<TypePerm>(&type->node)) {
    return perm->base;
  }
  return type;
}

static TypeEquivResult TypeEquivIgnorePerm(const TypeRef& lhs, const TypeRef& rhs) {
  const TypeRef lhs_base = StripPermOnce(lhs);
  const TypeRef rhs_base = StripPermOnce(rhs);
  return TypeEquiv(lhs_base, rhs_base);
}

static void ApplyPermToBindings(
    const TypeRef& expected,
    std::vector<std::pair<std::string, TypeRef>>& bindings) {
  if (!expected) {
    return;
  }
  const auto* perm = std::get_if<TypePerm>(&expected->node);
  if (!perm) {
    return;
  }
  for (auto& [name, type] : bindings) {
    type = MakeTypePerm(perm->perm, type);
  }
}

// -----------------------------------------------------------------------------
// Integer Literal Parsing for Range Patterns
// -----------------------------------------------------------------------------

static constexpr std::array<std::string_view, 12> kIntTypes = {
    "i8", "i16", "i32", "i64", "i128", "isize",
    "u8", "u16", "u32", "u64", "u128", "usize"};

static bool IsIntTypeName(std::string_view name) {
  for (const auto& t : kIntTypes) {
    if (name == t) {
      return true;
    }
  }
  return false;
}

static constexpr std::array<std::string_view, 12> kIntSuffixes = {
    "i128", "u128", "isize", "usize", "i64", "u64",
    "i32",  "u32",  "i16",  "u16",  "i8",  "u8"};

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

static bool ParseIntCore(std::string_view core, core::UInt128& value_out) {
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

static std::optional<core::UInt128> ParseIntLiteralValue(std::string_view lexeme) {
  if (lexeme.empty() || lexeme[0] == '-') {
    return std::nullopt;
  }
  const std::string_view core = StripIntSuffix(lexeme);
  if (core.empty()) {
    return std::nullopt;
  }
  core::UInt128 value = core::UInt128FromU64(0);
  if (!ParseIntCore(core, value)) {
    return std::nullopt;
  }
  return value;
}

static std::optional<core::UInt128> ConstPatInt(const ast::PatternPtr& pattern) {
  SPEC_RULE("ConstPatInt");
  if (!pattern) {
    return std::nullopt;
  }
  const auto* lit = std::get_if<ast::LiteralPattern>(&pattern->node);
  if (!lit) {
    return std::nullopt;
  }
  if (lit->literal.kind != ast::TokenKind::IntLiteral) {
    return std::nullopt;
  }
  return ParseIntLiteralValue(lit->literal.lexeme);
}

// -----------------------------------------------------------------------------
// Type Lookup Helpers
// -----------------------------------------------------------------------------

static const ast::RecordDecl* LookupRecordDecl(const ScopeContext& ctx,
                                               const ast::TypePath& path) {
  const auto it = ctx.sigma.types.find(PathKeyOf(path));
  if (it == ctx.sigma.types.end()) {
    return nullptr;
  }
  return std::get_if<ast::RecordDecl>(&it->second);
}

static const ast::EnumDecl* LookupEnumDecl(const ScopeContext& ctx,
                                           const ast::TypePath& path) {
  const auto it = ctx.sigma.types.find(PathKeyOf(path));
  if (it == ctx.sigma.types.end()) {
    return nullptr;
  }
  return std::get_if<ast::EnumDecl>(&it->second);
}

static const ast::FieldDecl* LookupFieldDecl(const ast::RecordDecl& record,
                                             std::string_view name) {
  for (const auto& member : record.members) {
    const auto* field = std::get_if<ast::FieldDecl>(&member);
    if (!field) {
      continue;
    }
    if (IdKeyOf(field->name) == IdKeyOf(name)) {
      return field;
    }
  }
  return nullptr;
}

static bool FieldVisible(const ast::ModulePath& current,
                         const ast::TypePath& record_path,
                         const ast::FieldDecl& field) {
  if (field.vis == ast::Visibility::Public ||
      field.vis == ast::Visibility::Internal) {
    return true;
  }
  if (record_path.empty()) {
    return false;
  }
  ast::ModulePath module_path = record_path;
  module_path.pop_back();
  // Private and protected: visible in same module
  if (module_path == current) {
    return true;
  }
  return false;
}

static std::optional<TypeRef> EnumFieldType(
    const ast::VariantPayloadRecord& payload,
    const ScopeContext& ctx,
    std::string_view name) {
  for (const auto& field : payload.fields) {
    if (IdKeyOf(field.name) != IdKeyOf(name)) {
      continue;
    }
    const auto lowered = LowerType(ctx, field.type);
    if (!lowered.ok) {
      return std::nullopt;
    }
    return lowered.type;
  }
  return std::nullopt;
}

static std::optional<TypeRef> ModalFieldType(const ast::ModalDecl& decl,
                                              std::string_view state,
                                              const ScopeContext& ctx,
                                              std::string_view name,
                                              const std::vector<TypeRef>& modal_args) {
  const auto state_key = IdKeyOf(state);
  TypeSubst modal_subst;
  if (decl.generic_params.has_value()) {
    if (modal_args.size() > decl.generic_params->params.size()) {
      return std::nullopt;
    }
    modal_subst =
        BuildModalRefSubstitution(decl.generic_params->params, modal_args);
  }
  for (const auto& block : decl.states) {
    if (IdKeyOf(block.name) != state_key) {
      continue;
    }
    for (const auto& member : block.members) {
      const auto* field = std::get_if<ast::StateFieldDecl>(&member);
      if (!field) {
        continue;
      }
      if (IdKeyOf(field->name) != IdKeyOf(name)) {
        continue;
      }
      const auto lowered = LowerType(ctx, field->type);
      if (!lowered.ok) {
        return std::nullopt;
      }
      TypeRef field_type = lowered.type;
      field_type = InstantiateType(field_type, modal_subst);
      return field_type;
    }
  }
  return std::nullopt;
}

// -----------------------------------------------------------------------------
// Duplicate Name Check
// -----------------------------------------------------------------------------

static bool DistinctPatNames(const std::vector<ast::Identifier>& names) {
  SPEC_RULE("Pat-Dup-Err");
  std::unordered_set<IdKey> seen;
  for (const auto& name : names) {
    if (!seen.insert(IdKeyOf(name)).second) {
      return false;
    }
  }
  return true;
}

// -----------------------------------------------------------------------------
// Pattern Typing Implementation
// -----------------------------------------------------------------------------

static PatternTypeResult TypePatternAgainstTypeImpl(const ScopeContext& ctx,
                                              const ast::PatternPtr& pattern,
                                              const TypeRef& expected) {
  SpecDefsTypePattern();
  PatternTypeResult result;
  if (!pattern || !expected) {
    return result;
  }

  const auto names = PatNames(pattern);
  if (!DistinctPatNames(names)) {
    SPEC_RULE("Pat-Dup-R-Err");
    result.diag_id = "Pat-Dup-Err";
    return result;
  }

  return std::visit(
      [&](const auto& node) -> PatternTypeResult {
        using T = std::decay_t<decltype(node)>;

        // Wildcard Pattern
        if constexpr (std::is_same_v<T, ast::WildcardPattern>) {
          SPEC_RULE("Pat-Wildcard-R");
          return {true, std::nullopt, {}};
        }

        // Identifier Pattern
        else if constexpr (std::is_same_v<T, ast::IdentifierPattern>) {
          SPEC_RULE("Pat-Ident-R");
          return {true, std::nullopt, {{std::string(node.name), expected}}};
        }

        // Literal Pattern
        else if constexpr (std::is_same_v<T, ast::LiteralPattern>) {
          ast::LiteralExpr expr{node.literal};
          const auto typed = TypeLiteralExpr(ctx, expr);
          if (!typed.ok) {
            return {false, typed.diag_id, {}};
          }
          const auto sub = Subtyping(ctx, typed.type, expected);
          if (!sub.ok) {
            return {false, sub.diag_id, {}};
          }
          if (!sub.subtype) {
            return {false, std::nullopt, {}};
          }
          SPEC_RULE("Pat-Literal-R");
          return {true, std::nullopt, {}};
        }

        // Typed Pattern
        else if constexpr (std::is_same_v<T, ast::TypedPattern>) {
          const auto lowered = LowerType(ctx, node.type);
          if (!lowered.ok) {
            return {false, lowered.diag_id, {}};
          }
          const auto* uni = std::get_if<TypeUnion>(&expected->node);
          if (!uni) {
            // Non-union case: check type equivalence
            const auto equiv = TypeEquiv(lowered.type, expected);
            if (!equiv.ok) {
              return {false, equiv.diag_id, {}};
            }
            if (equiv.equiv) {
              SPEC_RULE("Pat-Typed-Exact-R");
              return {true, std::nullopt, {{std::string(node.name), lowered.type}}};
            }

            // Modal-state typed patterns can refine a general modal reference:
            // x: M@S is accepted when expected type is modal ref M.
            if (const auto* lowered_modal =
                    std::get_if<TypeModalState>(&lowered.type->node)) {
                const auto* expected_path = AppliedTypePath(*expected);
                const auto* expected_args = AppliedTypeArgs(*expected);
                if (expected_path && expected_args) {
                  if (lowered_modal->path == *expected_path &&
                      lowered_modal->generic_args.size() == expected_args->size()) {
                    bool args_ok = true;
                    for (std::size_t i = 0;
                         i < lowered_modal->generic_args.size(); ++i) {
                      const auto arg_equiv = TypeEquiv(
                          lowered_modal->generic_args[i],
                          (*expected_args)[i]);
                    if (!arg_equiv.ok) {
                      return {false, arg_equiv.diag_id, {}};
                    }
                    if (!arg_equiv.equiv) {
                      args_ok = false;
                      break;
                    }
                  }
                  if (args_ok) {
                    SPEC_RULE("Pat-Typed-Exact-R");
                    return {true, std::nullopt,
                            {{std::string(node.name), lowered.type}}};
                  }
                }
              }
            }
            return {false, std::nullopt, {}};
          }
          // Union case: check if pattern type is a member
          bool member = false;
          for (const auto& member_type : uni->members) {
            const auto equiv = TypeEquivIgnorePerm(lowered.type, member_type);
            if (!equiv.ok) {
              return {false, equiv.diag_id, {}};
            }
            if (equiv.equiv) {
              member = true;
              break;
            }
          }
          if (!member) {
            return {false, std::nullopt, {}};
          }
          SPEC_RULE("Pat-Typed-Union-R");
          return {true, std::nullopt, {{std::string(node.name), lowered.type}}};
        }

        // Tuple Pattern
        else if constexpr (std::is_same_v<T, ast::TuplePattern>) {
          if (node.elements.empty()) {
            if (IsPrimType(expected, "()")) {
              SPEC_RULE("Pat-Unit");
              return {true, std::nullopt, {}};
            }
            return {false, std::nullopt, {}};
          }
          const auto* tuple = std::get_if<TypeTuple>(&expected->node);
          if (!tuple) {
            return {false, std::nullopt, {}};
          }
          if (tuple->elements.size() != node.elements.size()) {
            SPEC_RULE("Pat-Tuple-R-Arity-Err");
            return {false, "Pat-Tuple-Arity-Err", {}};
          }
          std::vector<std::pair<std::string, TypeRef>> binds;
          for (std::size_t i = 0; i < node.elements.size(); ++i) {
            const auto sub = TypePatternAgainstTypeImpl(ctx, node.elements[i],
                                                  tuple->elements[i]);
            if (!sub.ok) {
              return sub;
            }
            binds.insert(binds.end(), sub.bindings.begin(), sub.bindings.end());
          }
          SPEC_RULE("Pat-Tuple-R");
          return {true, std::nullopt, std::move(binds)};
        }

        // Record Pattern
        else if constexpr (std::is_same_v<T, ast::RecordPattern>) {
          const auto* path_type = std::get_if<TypePathType>(&expected->node);
          if (!path_type || path_type->path != node.path) {
            return {false, std::nullopt, {}};
          }
          const auto* record = LookupRecordDecl(ctx, node.path);
          if (!record) {
            return {false, std::nullopt, {}};
          }
          std::vector<std::pair<std::string, TypeRef>> binds;
          for (const auto& field : node.fields) {
            const auto* decl = LookupFieldDecl(*record, field.name);
            if (!decl) {
              SPEC_RULE("RecordPattern-UnknownField");
              return {false, "RecordPattern-UnknownField", {}};
            }
            if (!FieldVisible(ctx.current_module, node.path, *decl)) {
              return {false, std::nullopt, {}};
            }
            const auto lowered = LowerType(ctx, decl->type);
            if (!lowered.ok) {
              return {false, lowered.diag_id, {}};
            }
            ast::PatternPtr pat = field.pattern_opt;
            if (!pat) {
              auto implicit = std::make_shared<ast::Pattern>();
              implicit->node = ast::IdentifierPattern{field.name};
              pat = implicit;
            }
            const auto sub = TypePatternAgainstTypeImpl(ctx, pat, lowered.type);
            if (!sub.ok) {
              return sub;
            }
            binds.insert(binds.end(), sub.bindings.begin(), sub.bindings.end());
          }
          SPEC_RULE("Pat-Record-R");
          return {true, std::nullopt, std::move(binds)};
        }

        // Enum Pattern
        else if constexpr (std::is_same_v<T, ast::EnumPattern>) {
          const auto* path_type = std::get_if<TypePathType>(&expected->node);
          if (!path_type || path_type->path != node.path) {
            return {false, std::nullopt, {}};
          }
          const auto* decl = LookupEnumDecl(ctx, node.path);
          if (!decl) {
            return {false, std::nullopt, {}};
          }
          const ast::VariantDecl* variant = nullptr;
          for (const auto& v : decl->variants) {
            if (IdEq(v.name, node.name)) {
              variant = &v;
              break;
            }
          }
          if (!variant) {
            return {false, std::nullopt, {}};
          }
          if (!variant->payload_opt.has_value()) {
            if (node.payload_opt.has_value()) {
              return {false, std::nullopt, {}};
            }
            SPEC_RULE("Pat-Enum-Unit-R");
            return {true, std::nullopt, {}};
          }
          if (!node.payload_opt.has_value()) {
            return {false, std::nullopt, {}};
          }
          if (std::holds_alternative<ast::VariantPayloadTuple>(
                  *variant->payload_opt)) {
            const auto& tuple_payload =
                std::get<ast::VariantPayloadTuple>(*variant->payload_opt);
            if (!std::holds_alternative<ast::TuplePayloadPattern>(
                    *node.payload_opt)) {
              return {false, std::nullopt, {}};
            }
            const auto& tuple =
                std::get<ast::TuplePayloadPattern>(*node.payload_opt);
            if (tuple.elements.size() != tuple_payload.elements.size()) {
              return {false, std::nullopt, {}};
            }
            std::vector<std::pair<std::string, TypeRef>> binds;
            for (std::size_t i = 0; i < tuple.elements.size(); ++i) {
              const auto lowered = LowerType(ctx, tuple_payload.elements[i]);
              if (!lowered.ok) {
                return {false, lowered.diag_id, {}};
              }
              const auto sub = TypePatternAgainstTypeImpl(ctx, tuple.elements[i],
                                                    lowered.type);
              if (!sub.ok) {
                return sub;
              }
              binds.insert(binds.end(), sub.bindings.begin(), sub.bindings.end());
            }
            SPEC_RULE("Pat-Enum-Tuple-R");
            return {true, std::nullopt, std::move(binds)};
          }
          const auto& record_payload =
              std::get<ast::VariantPayloadRecord>(*variant->payload_opt);
          if (!std::holds_alternative<ast::RecordPayloadPattern>(
                  *node.payload_opt)) {
            return {false, std::nullopt, {}};
          }
          const auto& record =
              std::get<ast::RecordPayloadPattern>(*node.payload_opt);
          std::vector<std::pair<std::string, TypeRef>> binds;
          for (const auto& field : record.fields) {
            const auto field_type = EnumFieldType(record_payload, ctx, field.name);
            if (!field_type.has_value()) {
              return {false, std::nullopt, {}};
            }
            ast::PatternPtr pat = field.pattern_opt;
            if (!pat) {
              auto implicit = std::make_shared<ast::Pattern>();
              implicit->node = ast::IdentifierPattern{field.name};
              pat = implicit;
            }
            const auto sub = TypePatternAgainstTypeImpl(ctx, pat, *field_type);
            if (!sub.ok) {
              return sub;
            }
            binds.insert(binds.end(), sub.bindings.begin(), sub.bindings.end());
          }
          SPEC_RULE("Pat-Enum-Record-R");
          return {true, std::nullopt, std::move(binds)};
        }

          // Modal Pattern
          else if constexpr (std::is_same_v<T, ast::ModalPattern>) {
            const auto* path_type = AppliedTypePath(*expected);
            const auto* path_args = AppliedTypeArgs(*expected);
            const auto* modal_state = std::get_if<TypeModalState>(&expected->node);
            const ast::ModalDecl* decl = nullptr;
            std::vector<TypeRef> modal_args;
            std::string_view state = node.state;
            if (path_type) {
              decl = LookupModalDecl(ctx, *path_type);
              if (!decl || !HasState(*decl, state)) {
                return {false, std::nullopt, {}};
              }
              if (path_args) {
                modal_args = *path_args;
              }
          } else if (modal_state) {
            if (!IdEq(modal_state->state, state)) {
              return {false, std::nullopt, {}};
            }
            decl = LookupModalDecl(ctx, modal_state->path);
            if (!decl || !HasState(*decl, state)) {
              return {false, std::nullopt, {}};
            }
            modal_args = modal_state->generic_args;
          } else {
            return {false, std::nullopt, {}};
          }
          const std::vector<ast::FieldPattern>* fields = nullptr;
          std::vector<ast::FieldPattern> empty;
          if (node.fields_opt.has_value()) {
            fields = &node.fields_opt->fields;
          } else {
            fields = &empty;
          }
          std::vector<std::pair<std::string, TypeRef>> binds;
          for (const auto& field : *fields) {
            const auto field_type = ModalFieldType(*decl, state, ctx, field.name,
                                                   modal_args);
            if (!field_type.has_value()) {
              return {false, std::nullopt, {}};
            }
            ast::PatternPtr pat = field.pattern_opt;
            if (!pat) {
              auto implicit = std::make_shared<ast::Pattern>();
              implicit->node = ast::IdentifierPattern{field.name};
              pat = implicit;
            }
            const auto sub = TypePatternAgainstTypeImpl(ctx, pat, *field_type);
            if (!sub.ok) {
              return sub;
            }
            binds.insert(binds.end(), sub.bindings.begin(), sub.bindings.end());
          }
          if (path_type) {
            SPEC_RULE("Pat-Modal-R");
          } else {
            SPEC_RULE("Pat-Modal-State-R");
          }
          return {true, std::nullopt, std::move(binds)};
        }

        // Range Pattern
        else if constexpr (std::is_same_v<T, ast::RangePattern>) {
          const auto lo = ConstPatInt(node.lo);
          const auto hi = ConstPatInt(node.hi);
          if (!lo.has_value() || !hi.has_value()) {
            SPEC_RULE("RangePattern-NonConst");
            return {false, "RangePattern-NonConst", {}};
          }
          const auto* prim = std::get_if<TypePrim>(&expected->node);
          if (!prim || !IsIntTypeName(prim->name)) {
            return {false, std::nullopt, {}};
          }
          const bool less = core::UInt128Greater(*hi, *lo);
          const bool leq = core::UInt128LessOrEqual(*lo, *hi);
          if (node.kind == ast::RangeKind::Exclusive) {
            if (!less) {
              SPEC_RULE("RangePattern-Empty");
              return {false, "RangePattern-Empty", {}};
            }
          } else if (node.kind == ast::RangeKind::Inclusive) {
            if (!leq) {
              SPEC_RULE("RangePattern-Empty");
              return {false, "RangePattern-Empty", {}};
            }
          } else {
            return {false, std::nullopt, {}};
          }
          SPEC_RULE("Pat-Range-R");
          return {true, std::nullopt, {}};
        }

        else {
          return {false, std::nullopt, {}};
        }
      },
      pattern->node);
}

}  // namespace

// =============================================================================
// Public Interface
// =============================================================================

PatternTypeResult TypePatternAgainstType(const ScopeContext& ctx,
                                   const ast::PatternPtr& pattern,
                                   const TypeRef& expected) {
  SpecDefsTypePattern();
  PatternTypeResult result;
  if (!pattern || !expected) {
    return result;
  }

  const auto* perm = std::get_if<TypePerm>(&expected->node);
  const auto base = StripPermOnce(expected);
  result = TypePatternAgainstTypeImpl(ctx, pattern, base);
  if (result.ok && perm) {
    ApplyPermToBindings(expected, result.bindings);
    SPEC_RULE("Pat-StripPerm");
  }
  return result;
}

bool IrrefutablePattern(const ScopeContext& ctx,
                        const ast::PatternPtr& pattern,
                        const TypeRef& expected) {
  SpecDefsTypePattern();
  if (!pattern || !expected) {
    return false;
  }

  const auto base = StripPermOnce(expected);
  if (!base) {
    return false;
  }

  return std::visit(
      [&](const auto& node) -> bool {
        using T = std::decay_t<decltype(node)>;

        if constexpr (std::is_same_v<T, ast::WildcardPattern>) {
          return true;
        }

        else if constexpr (std::is_same_v<T, ast::IdentifierPattern>) {
          return true;
        }

        else if constexpr (std::is_same_v<T, ast::TuplePattern>) {
          if (node.elements.empty()) {
            return IsPrimType(base, "()");
          }
          const auto* tuple = std::get_if<TypeTuple>(&base->node);
          if (!tuple || tuple->elements.size() != node.elements.size()) {
            return false;
          }
          for (std::size_t i = 0; i < node.elements.size(); ++i) {
            if (!IrrefutablePattern(ctx, node.elements[i], tuple->elements[i])) {
              return false;
            }
          }
          return true;
        }

        else if constexpr (std::is_same_v<T, ast::RecordPattern>) {
          const auto* path_type = std::get_if<TypePathType>(&base->node);
          if (!path_type || path_type->path != node.path) {
            return false;
          }
          const auto* record = LookupRecordDecl(ctx, node.path);
          if (!record) {
            return false;
          }
          for (const auto& field : node.fields) {
            const auto* decl = LookupFieldDecl(*record, field.name);
            if (!decl) {
              return false;
            }
            const auto lowered = LowerType(ctx, decl->type);
            if (!lowered.ok) {
              return false;
            }
            ast::PatternPtr pat = field.pattern_opt;
            if (!pat) {
              auto implicit = std::make_shared<ast::Pattern>();
              implicit->node = ast::IdentifierPattern{field.name};
              pat = implicit;
            }
            if (!IrrefutablePattern(ctx, pat, lowered.type)) {
              return false;
            }
          }
          return true;
        }

        else if constexpr (std::is_same_v<T, ast::ModalPattern>) {
          const auto* modal_state = std::get_if<TypeModalState>(&base->node);
          if (!modal_state || !IdEq(modal_state->state, node.state)) {
            return false;
          }
          const auto* decl = LookupModalDecl(ctx, modal_state->path);
          if (!decl || !HasState(*decl, node.state)) {
            return false;
          }
          const std::vector<ast::FieldPattern>* fields = nullptr;
          std::vector<ast::FieldPattern> empty;
          if (node.fields_opt.has_value()) {
            fields = &node.fields_opt->fields;
          } else {
            fields = &empty;
          }
          for (const auto& field : *fields) {
            const auto field_type =
                ModalFieldType(*decl, node.state, ctx, field.name,
                               modal_state->generic_args);
            if (!field_type.has_value()) {
              return false;
            }
            ast::PatternPtr pat = field.pattern_opt;
            if (!pat) {
              auto implicit = std::make_shared<ast::Pattern>();
              implicit->node = ast::IdentifierPattern{field.name};
              pat = implicit;
            }
            if (!IrrefutablePattern(ctx, pat, *field_type)) {
              return false;
            }
          }
          return true;
        }

        else if constexpr (std::is_same_v<T, ast::TypedPattern>) {
          // _: T is irrefutable when T matches the expected type exactly
          // (non-union case). For union narrowing, it remains refutable.
          const auto lowered = LowerType(ctx, node.type);
          if (!lowered.ok || !lowered.type) {
            return false;
          }
          const auto equiv = TypeEquiv(lowered.type, base);
          if (equiv.ok && equiv.equiv) {
            return true;
          }
          return false;
        }

        else {
          // Literal, Enum, Range are refutable
          return false;
        }
      },
      pattern->node);
}

}  // namespace cursive::analysis
