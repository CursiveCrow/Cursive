#include "cursive0/analysis/composite/record_methods.h"

#include <unordered_set>
#include <utility>
#include <vector>

#include "cursive0/core/assert_spec.h"
#include "cursive0/analysis/composite/classes.h"
#include "cursive0/analysis/resolve/scopes.h"
#include "cursive0/analysis/resolve/scopes_lookup.h"
#include "cursive0/analysis/types/subtyping.h"

namespace cursive0::analysis {

namespace {

static inline void SpecDefsRecordMethods() {
  SPEC_DEF("Methods", "5.3.2");
  SPEC_DEF("MethodNames", "5.3.2");
  SPEC_DEF("Self_R", "5.3.2");
  SPEC_DEF("SelfType", "5.3.2");
  SPEC_DEF("RecvType", "5.3.1");
  SPEC_DEF("RecvMode", "5.3.1");
  SPEC_DEF("RecvPerm", "5.3.1");
  SPEC_DEF("PermOf", "5.3.1");
  SPEC_DEF("RecvBaseType", "5.3.2");
  SPEC_DEF("RecvArgOk", "5.3.2");
  SPEC_DEF("ArgsOkJudg", "5.3.2");
  SPEC_DEF("LookupMethodRules", "5.3.2");
  SPEC_DEF("StripPerm", "5.2.12");
  SPEC_DEF("AddrOfOk", "5.2.12");
  SPEC_DEF("IsPlace", "3.3.3");
}

static Permission LowerReceiverPerm(syntax::ReceiverPerm perm) {
  switch (perm) {
    case syntax::ReceiverPerm::Const:
      return Permission::Const;
    case syntax::ReceiverPerm::Unique:
      return Permission::Unique;
    case syntax::ReceiverPerm::Shared:
      return Permission::Shared;
  }
  return Permission::Const;
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
        } else {
          return type;
        }
      },
      type->node);
}

static std::optional<ParamMode> LowerParamMode(
    const std::optional<syntax::ParamMode>& mode) {
  if (!mode.has_value()) {
    return std::nullopt;
  }
  return ParamMode::Move;
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

static TypeRef StripPerm(const TypeRef& type) {
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

static bool IsPlaceExpr(const syntax::ExprPtr& expr) {
  if (!expr) {
    return false;
  }
  if (std::holds_alternative<syntax::IdentifierExpr>(expr->node)) {
    return true;
  }
  if (std::holds_alternative<syntax::FieldAccessExpr>(expr->node)) {
    return true;
  }
  if (std::holds_alternative<syntax::TupleAccessExpr>(expr->node)) {
    return true;
  }
  if (std::holds_alternative<syntax::IndexAccessExpr>(expr->node)) {
    return true;
  }
  if (const auto* deref = std::get_if<syntax::DerefExpr>(&expr->node)) {
    return IsPlaceExpr(deref->value);
  }
  return false;
}

static bool IsPrimType(const TypeRef& type, std::string_view name) {
  if (!type) {
    return false;
  }
  const auto* prim = std::get_if<TypePrim>(&type->node);
  return prim && prim->name == name;
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

static syntax::ExprPtr MakeExpr(const core::Span& span, syntax::ExprNode node) {
  auto expr = std::make_shared<syntax::Expr>();
  expr->span = span;
  expr->node = std::move(node);
  return expr;
}

static syntax::ExprPtr MovedArgExpr(const syntax::Arg& arg) {
  if (!arg.moved || !IsPlaceExpr(arg.value)) {
    return arg.value;
  }
  core::Span span = arg.span;
  if (!span.file.empty()) {
    return MakeExpr(span, syntax::MoveExpr{arg.value});
  }
  if (arg.value) {
    return MakeExpr(arg.value->span, syntax::MoveExpr{arg.value});
  }
  return MakeExpr(core::Span{}, syntax::MoveExpr{arg.value});
}

static const syntax::MethodDecl* FindRecordMethod(const syntax::RecordDecl& record,
                                                  std::string_view name) {
  for (const auto& member : record.members) {
    const auto* method = std::get_if<syntax::MethodDecl>(&member);
    if (!method) {
      continue;
    }
    if (IdEq(method->name, name)) {
      return method;
    }
  }
  return nullptr;
}

}  // namespace

RecvTypeResult RecvTypeForReceiver(const ScopeContext& ctx,
                                   const TypeRef& base,
                                   const syntax::Receiver& receiver,
                                   const LowerTypeFn& lower_type) {
  SpecDefsRecordMethods();
  (void)ctx;
  RecvTypeResult result;
  return std::visit(
      [&](const auto& recv) -> RecvTypeResult {
        using R = std::decay_t<decltype(recv)>;
        if constexpr (std::is_same_v<R, syntax::ReceiverShorthand>) {
          result.ok = true;
          result.type = MakeTypePerm(LowerReceiverPerm(recv.perm), base);
          return result;
        } else {
          const auto lowered = lower_type(recv.type);
          if (!lowered.ok) {
            result.diag_id = lowered.diag_id;
            return result;
          }
          result.ok = true;
          result.type = SubstSelfType(base, lowered.type);
          return result;
        }
      },
      receiver);
}

std::optional<ParamMode> RecvModeOf(const syntax::Receiver& receiver) {
  SpecDefsRecordMethods();
  return std::visit(
      [&](const auto& recv) -> std::optional<ParamMode> {
        using R = std::decay_t<decltype(recv)>;
        if constexpr (std::is_same_v<R, syntax::ReceiverShorthand>) {
          return std::nullopt;
        } else {
          return LowerParamMode(recv.mode_opt);
        }
      },
      receiver);
}

RecvBaseTypeResult RecvBaseType(const syntax::ExprPtr& base,
                                const std::optional<ParamMode>& mode,
                                const PlaceTypeFn& type_place,
                                const ExprTypeFn& type_expr) {
  SpecDefsRecordMethods();
  RecvBaseTypeResult result;
  if (!base) {
    return result;
  }
  if (!mode.has_value()) {
    const auto place = type_place(base);
    if (!place.ok) {
      result.diag_id = place.diag_id;
      return result;
    }
    result.ok = true;
    result.perm = PermOfType(place.type);
    result.base = StripPerm(place.type);
    return result;
  }
  const auto expr = type_expr(base);
  if (!expr.ok) {
    result.diag_id = expr.diag_id;
    return result;
  }
  result.ok = true;
  result.perm = PermOfType(expr.type);
  result.base = StripPerm(expr.type);
  return result;
}

RecvArgOkResult RecvArgOk(const syntax::ExprPtr& base,
                          const std::optional<ParamMode>& mode,
                          const ExprTypeFn& type_expr) {
  SpecDefsRecordMethods();
  RecvArgOkResult result;
  if (!base) {
    return result;
  }
  if (!mode.has_value()) {
    const auto addr_ok = AddrOfOk(base, type_expr);
    if (!addr_ok.ok) {
      result.diag_id = addr_ok.diag_id;
      return result;
    }
    result.ok = true;
    return result;
  }
  if (std::holds_alternative<syntax::MoveExpr>(base->node)) {
    result.ok = true;
    return result;
  }
  return result;
}

ArgsOkResult ArgsOk(const ScopeContext& ctx,
                    const std::vector<syntax::Param>& params,
                    const std::vector<syntax::Arg>& args,
                    const ExprTypeFn& type_expr,
                    const PlaceTypeFn* type_place,
                    const LowerTypeFn& lower_type) {
  SpecDefsRecordMethods();
  ArgsOkResult result;

  if (params.size() != args.size()) {
    SPEC_RULE("Call-ArgCount-Err");
    result.diag_id = "Call-ArgCount-Err";
    return result;
  }

  std::vector<TypeFuncParam> lowered_params;
  lowered_params.reserve(params.size());
  for (const auto& param : params) {
    const auto lowered = lower_type(param.type);
    if (!lowered.ok) {
      result.diag_id = lowered.diag_id;
      return result;
    }
    lowered_params.push_back(
        TypeFuncParam{LowerParamMode(param.mode), lowered.type});
  }

  for (std::size_t i = 0; i < args.size(); ++i) {
    if (lowered_params[i].mode == ParamMode::Move && !args[i].moved) {
      SPEC_RULE("Call-Move-Missing");
      result.diag_id = "Call-Move-Missing";
      return result;
    }
  }

  for (std::size_t i = 0; i < args.size(); ++i) {
    if (!lowered_params[i].mode.has_value() && args[i].moved) {
      SPEC_RULE("Call-Move-Unexpected");
      result.diag_id = "Call-Move-Unexpected";
      return result;
    }
  }

  std::vector<TypeRef> arg_types;
  arg_types.reserve(args.size());
  for (std::size_t i = 0; i < args.size(); ++i) {
    const auto& arg = args[i];
    if (!lowered_params[i].mode.has_value() && type_place &&
        IsPlaceExpr(arg.value)) {
      const auto place_type = (*type_place)(arg.value);
      if (!place_type.ok) {
        result.diag_id = place_type.diag_id;
        return result;
      }
      arg_types.push_back(place_type.type);
      continue;
    }
    const auto arg_expr = MovedArgExpr(arg);
    const auto arg_type = type_expr(arg_expr);
    if (!arg_type.ok) {
      result.diag_id = arg_type.diag_id;
      return result;
    }
    arg_types.push_back(arg_type.type);
  }

  for (std::size_t i = 0; i < args.size(); ++i) {
    const auto sub = Subtyping(ctx, arg_types[i], lowered_params[i].type);
    if (!sub.ok) {
      result.diag_id = sub.diag_id;
      return result;
    }
    if (!sub.subtype) {
      SPEC_RULE("Call-ArgType-Err");
      result.diag_id = "Call-ArgType-Err";
      return result;
    }
  }

  for (std::size_t i = 0; i < args.size(); ++i) {
    if (!lowered_params[i].mode.has_value() && !IsPlaceExpr(args[i].value)) {
      SPEC_RULE("Call-Arg-NotPlace");
      result.diag_id = "Call-Arg-NotPlace";
      return result;
    }
  }

  if (lowered_params.empty()) {
    SPEC_RULE("Args-Empty");
  } else {
    for (std::size_t i = 0; i < lowered_params.size(); ++i) {
      if (lowered_params[i].mode == ParamMode::Move) {
        const auto moved = MovedArgExpr(args[i]);
        const auto moved_type = type_expr(moved);
        if (!moved_type.ok) {
          result.diag_id = moved_type.diag_id;
          return result;
        }
        const auto sub = Subtyping(ctx, moved_type.type, lowered_params[i].type);
        if (!sub.ok) {
          result.diag_id = sub.diag_id;
          return result;
        }
        if (!sub.subtype) {
          SPEC_RULE("Call-ArgType-Err");
          result.diag_id = "Call-ArgType-Err";
          return result;
        }
        SPEC_RULE("Args-Cons");
        continue;
      }
      const auto addr_ok = AddrOfOk(args[i].value, type_expr);
      if (!addr_ok.ok) {
        result.diag_id = addr_ok.diag_id;
        return result;
      }
      SPEC_RULE("Args-Cons-Ref");
    }
  }

  result.ok = true;
  return result;
}

StaticMethodLookup LookupMethodStatic(const ScopeContext& ctx,
                                      const TypeRef& base,
                                      std::string_view name) {
  SpecDefsRecordMethods();
  StaticMethodLookup result;
  if (!base) {
    return result;
  }

  TypeRef lookup_base = base;
  if (const auto* opaque = std::get_if<TypeOpaque>(&base->node)) {
    if (opaque->origin) {
      const auto it = ctx.sigma.opaque_underlying.find(opaque->origin);
      if (it != ctx.sigma.opaque_underlying.end()) {
        lookup_base = it->second;
      }
    }
  }

  const auto* path_type = std::get_if<TypePathType>(&lookup_base->node);
  const syntax::RecordDecl* record = nullptr;
  std::vector<syntax::ClassPath> implements;
  if (path_type) {
    syntax::Path syntax_path;
    syntax_path.reserve(path_type->path.size());
    for (const auto& comp : path_type->path) {
      syntax_path.push_back(comp);
    }
    const auto it = ctx.sigma.types.find(PathKeyOf(syntax_path));
    if (it != ctx.sigma.types.end()) {
      if (const auto* record_decl = std::get_if<syntax::RecordDecl>(&it->second)) {
        record = record_decl;
        implements = record_decl->implements;
      } else if (const auto* enum_decl =
                     std::get_if<syntax::EnumDecl>(&it->second)) {
        implements = enum_decl->implements;
      } else if (const auto* modal_decl =
                     std::get_if<syntax::ModalDecl>(&it->second)) {
        implements = modal_decl->implements;
      }
    }
  }

  if (record) {
    const auto* method = FindRecordMethod(*record, name);
    if (method) {
      result.ok = true;
      result.record_method = method;
      return result;
    }
  }

  struct DefaultMethod {
    const syntax::ClassMethodDecl* method = nullptr;
    syntax::ClassPath owner;
  };
  std::vector<DefaultMethod> defaults;
  std::unordered_set<const syntax::ClassMethodDecl*> seen_defaults;
  for (const auto& class_path : implements) {
    const auto table = ClassMethodTable(ctx, class_path);
    if (!table.ok) {
      result.diag_id = table.diag_id;
      return result;
    }
    for (const auto& entry : table.methods) {
      if (!entry.method || !entry.method->body_opt) {
        continue;
      }
      if (!IdEq(entry.method->name, name)) {
        continue;
      }
      if (seen_defaults.insert(entry.method).second) {
        defaults.push_back(DefaultMethod{entry.method, entry.owner});
      }
    }
  }

  if (defaults.empty()) {
    SPEC_RULE("LookupMethod-NotFound");
    result.diag_id = "LookupMethod-NotFound";
    return result;
  }
  if (defaults.size() > 1) {
    SPEC_RULE("LookupMethod-Ambig");
    result.diag_id = "LookupMethod-Ambig";
    return result;
  }

  result.ok = true;
  result.class_method = defaults[0].method;
  result.owner_class = defaults[0].owner;
  return result;
}

}  // namespace cursive0::analysis
