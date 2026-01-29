#include "cursive0/analysis/types/type_pattern.h"

#include <array>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

#include "cursive0/core/assert_spec.h"
#include "cursive0/core/int128.h"
#include "cursive0/analysis/resolve/collect_toplevel.h"
#include "cursive0/analysis/types/literals.h"
#include "cursive0/analysis/modal/modal.h"
#include "cursive0/analysis/resolve/scopes.h"
#include "cursive0/analysis/types/subtyping.h"
#include "cursive0/analysis/types/type_equiv.h"
#include "cursive0/analysis/types/types.h"

namespace cursive0::analysis {

namespace {

static inline void SpecDefsTypePattern() {
  SPEC_DEF("MatchJudg", "5.2.13");
  SPEC_DEF("StripPerm", "5.2.12");
  SPEC_DEF("ConstPatInt", "5.2.13");
  SPEC_DEF("PatternEffectRules", "5.2.13");
}

static TypeRef StripPerm(const TypeRef& type) {
  if (!type) {
    return type;
  }
  if (const auto* perm = std::get_if<TypePerm>(&type->node)) {
    return perm->base;
  }
  return type;
}


static void ApplyPermToBindings(const TypeRef& expected,
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

static bool IsPrimType(const TypeRef& type, std::string_view name) {
  if (!type) {
    return false;
  }
  const auto* prim = std::get_if<TypePrim>(&type->node);
  return prim && prim->name == name;
}

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

static std::optional<core::UInt128> ParseIntLiteralValue(
    std::string_view lexeme) {
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

static std::optional<core::UInt128> ConstPatInt(
    const syntax::PatternPtr& pattern) {
  if (!pattern) {
    return std::nullopt;
  }
  const auto* lit = std::get_if<syntax::LiteralPattern>(&pattern->node);
  if (!lit) {
    return std::nullopt;
  }
  if (lit->literal.kind != syntax::TokenKind::IntLiteral) {
    return std::nullopt;
  }
  return ParseIntLiteralValue(lit->literal.lexeme);
}

struct IntroResult {
  bool ok = false;
  std::optional<std::string_view> diag_id;
  TypeEnv env;
};

struct TypeLowerResult {
  bool ok = false;
  std::optional<std::string_view> diag_id;
  TypeRef type;
};

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

static RawPtrQual LowerRawPtrQual(syntax::RawPtrQual qual) {
  switch (qual) {
    case syntax::RawPtrQual::Imm:
      return RawPtrQual::Imm;
    case syntax::RawPtrQual::Mut:
      return RawPtrQual::Mut;
  }
  return RawPtrQual::Imm;
}

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
            params.push_back(TypeFuncParam{LowerParamMode(param.mode),
                                           lowered.type});
          }
          const auto ret = LowerType(ctx, node.ret);
          if (!ret.ok) {
            return ret;
          }
          return {true, std::nullopt, MakeTypeFunc(std::move(params), ret.type)};
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
          return {true, std::nullopt, MakeTypeString(LowerStringState(node.state))};
        } else if constexpr (std::is_same_v<T, syntax::TypeBytes>) {
          return {true, std::nullopt, MakeTypeBytes(LowerBytesState(node.state))};
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
            const auto lowered = LowerType(ctx, arg);
            if (!lowered.ok) {
              return lowered;
            }
            args.push_back(lowered.type);
          }
          return {true, std::nullopt,
                  MakeTypeModalState(node.path, node.state, std::move(args))};
        } else if constexpr (std::is_same_v<T, syntax::TypePathType>) {
          // §5.2.9, §13.1: Generic type instantiation lowering
          // Per WF-Apply (§5.2.3), type arguments MUST be preserved
          if (!node.generic_args.empty()) {
            std::vector<TypeRef> lowered_args;
            lowered_args.reserve(node.generic_args.size());
            for (const auto& arg : node.generic_args) {
              const auto lower_result = LowerType(ctx, arg);
              if (!lower_result.ok) {
                return lower_result;
              }
              lowered_args.push_back(lower_result.type);
            }
            return {true, std::nullopt,
                    MakeTypePath(node.path, std::move(lowered_args))};
          }
          return {true, std::nullopt, MakeTypePath(node.path)};
        } else {
          return {false, std::nullopt, {}};
        }
      },
      type->node);
}

static const syntax::RecordDecl* LookupRecordDecl(const ScopeContext& ctx,
                                                  const syntax::TypePath& path) {
  const auto it = ctx.sigma.types.find(PathKeyOf(path));
  if (it == ctx.sigma.types.end()) {
    return nullptr;
  }
  return std::get_if<syntax::RecordDecl>(&it->second);
}

static const syntax::EnumDecl* LookupEnumDecl(const ScopeContext& ctx,
                                              const syntax::TypePath& path) {
  const auto it = ctx.sigma.types.find(PathKeyOf(path));
  if (it == ctx.sigma.types.end()) {
    return nullptr;
  }
  return std::get_if<syntax::EnumDecl>(&it->second);
}

static const syntax::FieldDecl* LookupFieldDecl(
    const syntax::RecordDecl& record,
    std::string_view name) {
  for (const auto& member : record.members) {
    const auto* field = std::get_if<syntax::FieldDecl>(&member);
    if (!field) {
      continue;
    }
    if (IdKeyOf(field->name) == IdKeyOf(name)) {
      return field;
    }
  }
  return nullptr;
}

static bool FieldVisible(const syntax::ModulePath& current,
                         const syntax::TypePath& record_path,
                         const syntax::FieldDecl& field) {
  if (field.vis == syntax::Visibility::Public ||
      field.vis == syntax::Visibility::Internal) {
    return true;
  }
  if (record_path.empty()) {
    return false;
  }
  syntax::ModulePath module_path = record_path;
  module_path.pop_back();
  return module_path == current;
}

static std::optional<TypeRef> EnumFieldType(const syntax::VariantPayloadRecord& payload,
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

static std::optional<TypeRef> ModalFieldType(const syntax::ModalDecl& decl,
                                             std::string_view state,
                                             const ScopeContext& ctx,
                                             std::string_view name) {
  const auto state_key = IdKeyOf(state);
  for (const auto& block : decl.states) {
    if (IdKeyOf(block.name) != state_key) {
      continue;
    }
    for (const auto& member : block.members) {
      const auto* field = std::get_if<syntax::StateFieldDecl>(&member);
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
      return lowered.type;
    }
  }
  return std::nullopt;
}

static bool DistinctPatNames(const std::vector<syntax::Identifier>& names) {
  std::unordered_set<IdKey> seen;
  for (const auto& name : names) {
    if (!seen.insert(IdKeyOf(name)).second) {
      return false;
    }
  }
  return true;
}

static PatternTypeResult TypeMatchPatternImpl(const ScopeContext& ctx,
                                              const syntax::PatternPtr& pattern,
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
        if constexpr (std::is_same_v<T, syntax::WildcardPattern>) {
          SPEC_RULE("Pat-Wildcard-R");
          return {true, std::nullopt, {}};
        } else if constexpr (std::is_same_v<T, syntax::IdentifierPattern>) {
          SPEC_RULE("Pat-Ident-R");
          return {true, std::nullopt, {{std::string(node.name), expected}}};
        } else if constexpr (std::is_same_v<T, syntax::LiteralPattern>) {
          syntax::LiteralExpr expr{node.literal};
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
        } else if constexpr (std::is_same_v<T, syntax::TypedPattern>) {
          const auto lowered = LowerType(ctx, node.type);
          if (!lowered.ok) {
            return {false, lowered.diag_id, {}};
          }
          const auto* uni = std::get_if<TypeUnion>(&expected->node);
          if (!uni) {
            return {false, std::nullopt, {}};
          }
          bool member = false;
          for (const auto& member_type : uni->members) {
            const auto equiv = TypeEquiv(lowered.type, member_type);
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
          return {true, std::nullopt, {{std::string(node.name), lowered.type}}};
        } else if constexpr (std::is_same_v<T, syntax::TuplePattern>) {
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
            const auto sub = TypeMatchPatternImpl(ctx, node.elements[i],
                                                  tuple->elements[i]);
            if (!sub.ok) {
              return sub;
            }
            binds.insert(binds.end(), sub.bindings.begin(), sub.bindings.end());
          }
          SPEC_RULE("Pat-Tuple-R");
          return {true, std::nullopt, std::move(binds)};
        } else if constexpr (std::is_same_v<T, syntax::RecordPattern>) {
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
            syntax::PatternPtr pat = field.pattern_opt;
            if (!pat) {
              auto implicit = std::make_shared<syntax::Pattern>();
              implicit->node = syntax::IdentifierPattern{field.name};
              pat = implicit;
            }
            const auto sub = TypeMatchPatternImpl(ctx, pat, lowered.type);
            if (!sub.ok) {
              return sub;
            }
            binds.insert(binds.end(), sub.bindings.begin(), sub.bindings.end());
          }
          SPEC_RULE("Pat-Record-R");
          return {true, std::nullopt, std::move(binds)};
        } else if constexpr (std::is_same_v<T, syntax::EnumPattern>) {
          const auto* path_type = std::get_if<TypePathType>(&expected->node);
          if (!path_type || path_type->path != node.path) {
            return {false, std::nullopt, {}};
          }
          const auto* decl = LookupEnumDecl(ctx, node.path);
          if (!decl) {
            return {false, std::nullopt, {}};
          }
          const syntax::VariantDecl* variant = nullptr;
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
          if (std::holds_alternative<syntax::VariantPayloadTuple>(
                  *variant->payload_opt)) {
            const auto& tuple_payload =
                std::get<syntax::VariantPayloadTuple>(*variant->payload_opt);
            if (!std::holds_alternative<syntax::TuplePayloadPattern>(
                    *node.payload_opt)) {
              return {false, std::nullopt, {}};
            }
            const auto& tuple =
                std::get<syntax::TuplePayloadPattern>(*node.payload_opt);
            if (tuple.elements.size() != tuple_payload.elements.size()) {
              return {false, std::nullopt, {}};
            }
            std::vector<std::pair<std::string, TypeRef>> binds;
            for (std::size_t i = 0; i < tuple.elements.size(); ++i) {
              const auto lowered = LowerType(ctx, tuple_payload.elements[i]);
              if (!lowered.ok) {
                return {false, lowered.diag_id, {}};
              }
              const auto sub = TypeMatchPatternImpl(ctx, tuple.elements[i],
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
              std::get<syntax::VariantPayloadRecord>(*variant->payload_opt);
          if (!std::holds_alternative<syntax::RecordPayloadPattern>(
                  *node.payload_opt)) {
            return {false, std::nullopt, {}};
          }
          const auto& record =
              std::get<syntax::RecordPayloadPattern>(*node.payload_opt);
          std::vector<std::pair<std::string, TypeRef>> binds;
          for (const auto& field : record.fields) {
            const auto field_type = EnumFieldType(record_payload, ctx, field.name);
            if (!field_type.has_value()) {
              return {false, std::nullopt, {}};
            }
            syntax::PatternPtr pat = field.pattern_opt;
            if (!pat) {
              auto implicit = std::make_shared<syntax::Pattern>();
              implicit->node = syntax::IdentifierPattern{field.name};
              pat = implicit;
            }
            const auto sub = TypeMatchPatternImpl(ctx, pat, *field_type);
            if (!sub.ok) {
              return sub;
            }
            binds.insert(binds.end(), sub.bindings.begin(), sub.bindings.end());
          }
          SPEC_RULE("Pat-Enum-Record-R");
          return {true, std::nullopt, std::move(binds)};
        } else if constexpr (std::is_same_v<T, syntax::ModalPattern>) {
          const auto* path_type = std::get_if<TypePathType>(&expected->node);
          const auto* modal_state = std::get_if<TypeModalState>(&expected->node);
          const syntax::ModalDecl* decl = nullptr;
          std::string_view state = node.state;
          if (path_type) {
            decl = LookupModalDecl(ctx, path_type->path);
            if (!decl || !HasState(*decl, state)) {
              return {false, std::nullopt, {}};
            }
          } else if (modal_state) {
            if (!IdEq(modal_state->state, state)) {
              return {false, std::nullopt, {}};
            }
            decl = LookupModalDecl(ctx, modal_state->path);
            if (!decl || !HasState(*decl, state)) {
              return {false, std::nullopt, {}};
            }
          } else {
            return {false, std::nullopt, {}};
          }
          const std::vector<syntax::FieldPattern>* fields = nullptr;
          std::vector<syntax::FieldPattern> empty;
          if (node.fields_opt.has_value()) {
            fields = &node.fields_opt->fields;
          } else {
            fields = &empty;
          }
          std::vector<std::pair<std::string, TypeRef>> binds;
          for (const auto& field : *fields) {
            const auto field_type =
                ModalFieldType(*decl, state, ctx, field.name);
            if (!field_type.has_value()) {
              return {false, std::nullopt, {}};
            }
            syntax::PatternPtr pat = field.pattern_opt;
            if (!pat) {
              auto implicit = std::make_shared<syntax::Pattern>();
              implicit->node = syntax::IdentifierPattern{field.name};
              pat = implicit;
            }
            const auto sub = TypeMatchPatternImpl(ctx, pat, *field_type);
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
        } else if constexpr (std::is_same_v<T, syntax::RangePattern>) {
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
          if (node.kind == syntax::RangeKind::Exclusive) {
            if (!less) {
              SPEC_RULE("RangePattern-Empty");
              return {false, "RangePattern-Empty", {}};
            }
          } else if (node.kind == syntax::RangeKind::Inclusive) {
            if (!leq) {
              SPEC_RULE("RangePattern-Empty");
              return {false, "RangePattern-Empty", {}};
            }
          } else {
            return {false, std::nullopt, {}};
          }
          SPEC_RULE("Pat-Range-R");
          return {true, std::nullopt, {}};
        } else {
          return {false, std::nullopt, {}};
        }
      },
      pattern->node);
}

}  // namespace

PatternTypeResult TypeMatchPattern(const ScopeContext& ctx,
                                   const syntax::PatternPtr& pattern,
                                   const TypeRef& expected) {
  SpecDefsTypePattern();
  PatternTypeResult result;
  if (!pattern || !expected) {
    return result;
  }

  const auto* perm = std::get_if<TypePerm>(&expected->node);
  const auto base = StripPerm(expected);
  result = TypeMatchPatternImpl(ctx, pattern, base);
  if (result.ok && perm) {
    ApplyPermToBindings(expected, result.bindings);
    SPEC_RULE("Pat-StripPerm");
  }
  return result;
}

bool IrrefutablePattern(const ScopeContext& ctx,
                        const syntax::PatternPtr& pattern,
                        const TypeRef& expected) {
  SpecDefsTypePattern();
  if (!pattern || !expected) {
    return false;
  }

  const auto base = StripPerm(expected);
  if (!base) {
    return false;
  }

  return std::visit(
      [&](const auto& node) -> bool {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::WildcardPattern>) {
          return true;
        } else if constexpr (std::is_same_v<T, syntax::IdentifierPattern>) {
          return true;
        } else if constexpr (std::is_same_v<T, syntax::TuplePattern>) {
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
        } else if constexpr (std::is_same_v<T, syntax::RecordPattern>) {
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
            syntax::PatternPtr pat = field.pattern_opt;
            if (!pat) {
              auto implicit = std::make_shared<syntax::Pattern>();
              implicit->node = syntax::IdentifierPattern{field.name};
              pat = implicit;
            }
            if (!IrrefutablePattern(ctx, pat, lowered.type)) {
              return false;
            }
          }
          return true;
        } else if constexpr (std::is_same_v<T, syntax::ModalPattern>) {
          const auto* modal_state = std::get_if<TypeModalState>(&base->node);
          if (!modal_state || !IdEq(modal_state->state, node.state)) {
            return false;
          }
          const auto* decl = LookupModalDecl(ctx, modal_state->path);
          if (!decl || !HasState(*decl, node.state)) {
            return false;
          }
          const std::vector<syntax::FieldPattern>* fields = nullptr;
          std::vector<syntax::FieldPattern> empty;
          if (node.fields_opt.has_value()) {
            fields = &node.fields_opt->fields;
          } else {
            fields = &empty;
          }
          for (const auto& field : *fields) {
            const auto field_type =
                ModalFieldType(*decl, node.state, ctx, field.name);
            if (!field_type.has_value()) {
              return false;
            }
            syntax::PatternPtr pat = field.pattern_opt;
            if (!pat) {
              auto implicit = std::make_shared<syntax::Pattern>();
              implicit->node = syntax::IdentifierPattern{field.name};
              pat = implicit;
            }
            if (!IrrefutablePattern(ctx, pat, *field_type)) {
              return false;
            }
          }
          return true;
        } else {
          return false;
        }
      },
      pattern->node);
}

}  // namespace cursive0::analysis
