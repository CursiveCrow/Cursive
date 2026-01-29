#include "cursive0/analysis/composite/records.h"

#include <optional>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

#include "cursive0/core/assert_spec.h"
#include "cursive0/analysis/resolve/scopes.h"
#include "cursive0/analysis/resolve/scopes_lookup.h"
#include "cursive0/analysis/types/subtyping.h"
#include "cursive0/analysis/types/type_equiv.h"

namespace cursive0::analysis {

namespace {

struct TypeLowerResult {
  bool ok = false;
  std::optional<std::string_view> diag_id;
  TypeRef type;
};

static inline void SpecDefsRecords() {
  SPEC_DEF("Fields", "5.3.2");
  SPEC_DEF("InitOk", "5.2.8");
  SPEC_DEF("DefaultConstructible", "5.2.8");
  SPEC_DEF("RecordPath", "5.2.8");
  SPEC_DEF("RecordCallee", "5.2.8");
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
          return {true, std::nullopt,
                  MakeTypeArray(elem.type, *len.value)};
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

static std::vector<const syntax::FieldDecl*> RecordFields(
    const syntax::RecordDecl& record) {
  std::vector<const syntax::FieldDecl*> fields;
  for (const auto& member : record.members) {
    if (const auto* field = std::get_if<syntax::FieldDecl>(&member)) {
      fields.push_back(field);
    }
  }
  return fields;
}

static bool DefaultConstructible(
    const std::vector<const syntax::FieldDecl*>& fields) {
  for (const auto* field : fields) {
    if (!field || !field->init_opt) {
      return false;
    }
  }
  return true;
}

static syntax::Path FullPath(const syntax::ModulePath& path,
                             std::string_view name) {
  syntax::Path out = path;
  out.emplace_back(name);
  return out;
}

static const syntax::RecordDecl* LookupRecordDecl(const ScopeContext& ctx,
                                                  const syntax::Path& path) {
  const auto it = ctx.sigma.types.find(PathKeyOf(path));
  if (it == ctx.sigma.types.end()) {
    return nullptr;
  }
  return std::get_if<syntax::RecordDecl>(&it->second);
}

struct RecordCalleeResult {
  const syntax::RecordDecl* record = nullptr;
  std::optional<syntax::Path> path;
};

static RecordCalleeResult ResolveRecordCallee(const ScopeContext& ctx,
                                              const syntax::ExprPtr& callee,
                                              const std::vector<syntax::Arg>& args) {
  if (!callee || !args.empty()) {
    return {};
  }
  return std::visit(
      [&](const auto& node) -> RecordCalleeResult {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::IdentifierExpr>) {
          if (IdEq(node.name, "RegionOptions")) {
            syntax::Path full;
            full.emplace_back("RegionOptions");
            const auto* record = LookupRecordDecl(ctx, full);
            if (record) {
              return {record, std::move(full)};
            }
          }
          const auto ent = ResolveTypeName(ctx, node.name);
          if (!ent.has_value()) {
            return {};
          }
          const auto name = ent->target_opt.value_or(node.name);
          syntax::ModulePath module;
          if (ent->origin_opt.has_value()) {
            module = *ent->origin_opt;
          }
          syntax::Path full = FullPath(module, name);
          const auto* record = LookupRecordDecl(ctx, full);
          if (!record) {
            return {};
          }
          return {record, std::move(full)};
        } else if constexpr (std::is_same_v<T, syntax::PathExpr>) {
          syntax::Path full = FullPath(node.path, node.name);
          const auto* record = LookupRecordDecl(ctx, full);
          if (!record) {
            return {};
          }
          return {record, std::move(full)};
        }
        return {};
      },
      callee->node);
}

}  // namespace

RecordWfResult CheckRecordWf(const ScopeContext& ctx,
                             const syntax::RecordDecl& record,
                             const ExprTypeFn& type_expr) {
  SpecDefsRecords();
  RecordWfResult result;
  const auto fields = RecordFields(record);
  std::unordered_set<IdKey> seen;
  seen.reserve(fields.size());
  for (const auto* field : fields) {
    if (!field) {
      continue;
    }
    const auto key = IdKeyOf(field->name);
    if (!seen.insert(key).second) {
      SPEC_RULE("WF-Record-DupField");
      result.diag_id = "WF-Record-DupField";
      return result;
    }
  }

  for (const auto* field : fields) {
    if (!field || !field->init_opt) {
      continue;
    }
    const auto init_type = type_expr(field->init_opt);
    if (!init_type.ok) {
      result.diag_id = init_type.diag_id;
      return result;
    }
    const auto field_type = LowerType(ctx, field->type);
    if (!field_type.ok) {
      result.diag_id = field_type.diag_id;
      return result;
    }
    const auto sub = Subtyping(ctx, init_type.type, field_type.type);
    if (!sub.ok) {
      result.diag_id = sub.diag_id;
      return result;
    }
    if (!sub.subtype) {
      return result;
    }
  }

  SPEC_RULE("WF-Record");
  result.ok = true;
  return result;
}

ExprTypeResult TypeRecordDefaultCall(const ScopeContext& ctx,
                                     const syntax::ExprPtr& callee,
                                     const std::vector<syntax::Arg>& args,
                                     const ExprTypeFn& type_expr) {
  SpecDefsRecords();
  ExprTypeResult result;
  const auto callee_result = ResolveRecordCallee(ctx, callee, args);
  if (!callee_result.record || !callee_result.path.has_value()) {
    return result;
  }

  const auto wf = CheckRecordWf(ctx, *callee_result.record, type_expr);
  if (!wf.ok) {
    result.diag_id = wf.diag_id;
    return result;
  }

  const auto fields = RecordFields(*callee_result.record);
  if (!DefaultConstructible(fields)) {
    SPEC_RULE("Record-Default-Init-Err");
    result.diag_id = "Record-Default-Init-Err";
    return result;
  }

  SPEC_RULE("T-Record-Default");
  result.ok = true;
  result.type = MakeTypePath(*callee_result.path);
  return result;
}

}  // namespace cursive0::analysis
