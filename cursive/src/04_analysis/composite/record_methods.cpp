// =============================================================================
// MIGRATION MAPPING: record_methods.cpp
// =============================================================================
//
// SPEC REFERENCE: CursiveSpecification.md
// - Section 5.3.2 "Record Methods" (lines 12156-12259)
//   - Fields(R), Methods(R), Self_R, SelfType (lines 12160-12163)
//   - Recv-Explicit (lines 12167-12170)
//   - Record-Method-RecvSelf-Err (lines 12172-12175)
//   - Recv-Const (lines 12177-12179)
//   - Recv-Unique (lines 12181-12183)
//   - Recv-Shared (lines 12185-12187)
//   - ParamNames (line 12189)
//   - WF-Record-Method (lines 12191-12194)
//   - T-Record-Method-Body (lines 12196-12199)
//   - T-Record-Method-Builtin (lines 12201-12204)
//   - MethodNames (line 12206)
//   - WF-Record-Methods (lines 12208-12211)
//   - Record-Method-Dup (lines 12213-12216)
//   - LookupMethodRules (line 12220)
//   - ArgsOkJudg (line 12224)
//   - RecvBaseType (line 12226)
//   - Args-Empty (lines 12228-12230)
//   - Args-Cons (lines 12232-12235)
//   - Args-Cons-Ref (lines 12237-12240)
//   - RecvArgOk (line 12242)
//   - T-Record-MethodCall (lines 12245-12248)
// - Section 5.3.1 "Classes" for common method definitions (lines 11764-12155)
//   - RecvType (lines 11791-11794)
//   - RecvMode (lines 11796-11797)
//   - PermOf (lines 11799-11800)
//   - RecvPerm (line 11802)
//   - SubstSelf (lines 11775-11789)
//   - Sig_T (line 11808)
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/composite/record_methods.cpp
// - Lines 1-440 (entire file)
//
// Key source functions to migrate:
// - RecvTypeForReceiver (lines 124-150): Compute receiver type
// - RecvModeOf (lines 152-164): Extract receiver mode
// - RecvBaseType (lines 166-195): Get base type from receiver expression
// - RecvArgOk (lines 197-219): Validate receiver argument
// - ArgsOk (lines 221-343): Validate all arguments
// - LookupMethodStatic (lines 345-437): Static method lookup
//
// Supporting helpers:
// - LowerReceiverPerm (lines 36-46): Convert receiver permission
// - AddrOfOkResult struct (lines 48-51): Address-of validation result
// - AddrOfOk (lines 53-85): Check if address-of is valid
// - MakeExpr (lines 87-92): Construct expression node
// - MovedArgExpr (lines 94-106): Create moved argument expression
// - FindRecordMethod (lines 108-120): Find method in record declaration
//
// DEPENDENCIES:
// - cursive/src/04_analysis/composite/classes.h (ClassMethodTable)
// - cursive/src/04_analysis/resolve/scopes.h (ScopeContext)
// - cursive/src/04_analysis/resolve/scopes_lookup.h (name lookup)
// - cursive/src/04_analysis/types/subtyping.h (Subtyping)
// - cursive/src/04_analysis/types/type_expr.h (type operations)
// - cursive/src/00_core/assert_spec.h (SPEC_DEF, SPEC_RULE)
//
// REFACTORING NOTES:
// 1. Receiver type computation handles both shorthand (~, ~!, ~%) and explicit forms
// 2. RecvArgOk validates that address-of is valid for reference params
// 3. ArgsOk validates argument count, move modifiers, types, and place expressions
// 4. Method lookup first checks record methods, then class default methods
// 5. LookupMethodStatic handles opaque type underlying lookups
// 6. Ambiguous default methods from multiple classes produce an error
// 7. SubstSelf is used to substitute Self type in method signatures
// =============================================================================

#include "04_analysis/composite/record_methods.h"

#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "00_core/assert_spec.h"
#include "04_analysis/composite/classes.h"
#include "04_analysis/generics/monomorphize.h"
#include "04_analysis/resolve/scopes.h"
#include "04_analysis/resolve/scopes_lookup.h"
#include "04_analysis/typing/subtyping.h"
#include "04_analysis/typing/type_expr.h"
#include "04_analysis/typing/type_lower.h"

namespace cursive::analysis {

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

struct AliasNormalizeResult {
  bool ok = true;
  std::optional<std::string_view> diag_id;
  TypeRef type;
};

struct AliasExpandResult {
  bool ok = true;
  std::optional<std::string_view> diag_id;
  bool expanded = false;
  TypeRef type;
};

static const ast::TypeAliasDecl* LookupTypeAliasDecl(const ScopeContext& ctx,
                                                     const TypePath& path) {
  if (path.empty()) {
    return nullptr;
  }
  if (path.size() > 1) {
    ast::Path full;
    full.reserve(path.size());
    for (const auto& seg : path) {
      full.push_back(seg);
    }
    const auto it = ctx.sigma.types.find(PathKeyOf(full));
    if (it == ctx.sigma.types.end()) {
      return nullptr;
    }
    return std::get_if<ast::TypeAliasDecl>(&it->second);
  }

  const auto ent = ResolveTypeName(ctx, path[0]);
  if (!ent.has_value() || !ent->origin_opt.has_value()) {
    return nullptr;
  }

  ast::Path resolved = *ent->origin_opt;
  const std::string resolved_name =
      ent->target_opt.has_value() ? *ent->target_opt : path[0];
  resolved.push_back(resolved_name);
  const auto resolved_it = ctx.sigma.types.find(PathKeyOf(resolved));
  if (resolved_it == ctx.sigma.types.end()) {
    return nullptr;
  }
  return std::get_if<ast::TypeAliasDecl>(&resolved_it->second);
}

struct TypeDeclLookupResult {
  const TypeDecl* decl = nullptr;
  TypePath resolved_path;
};

static TypeDeclLookupResult LookupTypeDeclByPath(const ScopeContext& ctx,
                                                 const TypePath& path) {
  if (path.empty()) {
    return {};
  }

  auto lookup_full = [&](const ast::Path& full) -> TypeDeclLookupResult {
    const auto it = ctx.sigma.types.find(PathKeyOf(full));
    if (it == ctx.sigma.types.end()) {
      return {};
    }
    TypeDeclLookupResult result;
    result.decl = &it->second;
    result.resolved_path.reserve(full.size());
    for (const auto& seg : full) {
      result.resolved_path.push_back(seg);
    }
    return result;
  };

  ast::Path direct;
  direct.reserve(path.size());
  for (const auto& seg : path) {
    direct.push_back(seg);
  }
  if (const auto direct_lookup = lookup_full(direct); direct_lookup.decl) {
    return direct_lookup;
  }

  if (path.size() > 1) {
    ast::Path without_root;
    without_root.reserve(path.size() - 1);
    for (std::size_t i = 1; i < path.size(); ++i) {
      without_root.push_back(path[i]);
    }
    if (const auto local_lookup = lookup_full(without_root); local_lookup.decl) {
      return local_lookup;
    }
  }

  if (path.size() != 1) {
    return {};
  }

  const auto ent = ResolveTypeName(ctx, path[0]);
  if (!ent.has_value() || !ent->origin_opt.has_value()) {
    return {};
  }

  ast::Path resolved = *ent->origin_opt;
  resolved.push_back(ent->target_opt.has_value() ? *ent->target_opt : path[0]);
  return lookup_full(resolved);
}

static AliasExpandResult ExpandTypeAliasApply(const ScopeContext& ctx,
                                              const TypePathType& applied) {
  AliasExpandResult result;
  const auto* alias = LookupTypeAliasDecl(ctx, applied.path);
  if (!alias) {
    return result;
  }

  const auto lowered = LowerType(ctx, alias->type);
  if (!lowered.ok) {
    result.ok = false;
    result.diag_id = lowered.diag_id;
    return result;
  }

  if (!alias->generic_params.has_value()) {
    if (!applied.generic_args.empty()) {
      return result;
    }
    result.type = lowered.type;
    result.expanded = true;
    return result;
  }

  const auto& params = alias->generic_params->params;
  if (applied.generic_args.size() > params.size()) {
    return result;
  }

  const auto subst = BuildSubstitution(params, applied.generic_args);
  result.type = InstantiateType(lowered.type, subst);
  result.expanded = result.type != nullptr;
  return result;
}

static AliasNormalizeResult NormalizeMethodBaseType(const ScopeContext& ctx,
                                                    const TypeRef& type) {
  AliasNormalizeResult out;
  out.type = type;
  for (int i = 0; i < 16; ++i) {
    if (!out.type) {
      return out;
    }
    const auto* path = AppliedTypePath(*out.type);
    const auto* args = AppliedTypeArgs(*out.type);
    if (!path || !args) {
      return out;
    }
    const auto expanded = ExpandTypeAliasApply(ctx, TypePathType{*path, *args});
    if (!expanded.ok) {
      out.ok = false;
      out.diag_id = expanded.diag_id;
      return out;
    }
    if (!expanded.expanded) {
      return out;
    }
    out.type = StripPerm(expanded.type);
  }
  return out;
}

static Permission LowerReceiverPerm(ast::ReceiverPerm perm) {
  switch (perm) {
    case ast::ReceiverPerm::Const:
      return Permission::Const;
    case ast::ReceiverPerm::Unique:
      return Permission::Unique;
    case ast::ReceiverPerm::Shared:
      return Permission::Shared;
  }
  return Permission::Const;
}

struct AddrOfOkResult {
  bool ok = false;
  std::optional<std::string_view> diag_id;
};

static AddrOfOkResult AddrOfOk(const ast::ExprPtr& expr,
                               const ExprTypeFn& type_expr) {
  if (!IsPlaceExpr(expr)) {
    return {false, std::nullopt};
  }
  const auto* index = std::get_if<ast::IndexAccessExpr>(&expr->node);
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

static const ast::MethodDecl* FindRecordMethod(const ast::RecordDecl& record,
                                               std::string_view name) {
  for (const auto& member : record.members) {
    const auto* method = std::get_if<ast::MethodDecl>(&member);
    if (!method) {
      continue;
    }
    if (IdEq(method->name, name)) {
      return method;
    }
  }
  return nullptr;
}

static bool IsExplicitSelfReceiverType(const TypeRef& type) {
  if (!type) {
    return false;
  }
  if (const auto* path = std::get_if<TypePathType>(&type->node)) {
    return IsSelfVarPath(path->path);
  }
  if (const auto* perm = std::get_if<TypePerm>(&type->node)) {
    return IsExplicitSelfReceiverType(perm->base);
  }
  if (const auto* refine = std::get_if<TypeRefine>(&type->node)) {
    return IsExplicitSelfReceiverType(refine->base);
  }
  return false;
}

}  // namespace

RecvTypeResult RecvTypeForReceiver(const ScopeContext& ctx,
                                   const TypeRef& base,
                                   const ast::Receiver& receiver,
                                   const LowerTypeFn& lower_type) {
  SpecDefsRecordMethods();
  (void)ctx;
  RecvTypeResult result;
  return std::visit(
      [&](const auto& recv) -> RecvTypeResult {
        using R = std::decay_t<decltype(recv)>;
        if constexpr (std::is_same_v<R, ast::ReceiverShorthand>) {
          result.ok = true;
          result.type = MakeTypePerm(LowerReceiverPerm(recv.perm), base);
          return result;
        } else {
          const auto lowered = lower_type(recv.type);
          if (!lowered.ok) {
            result.diag_id = lowered.diag_id;
            return result;
          }
          if (!IsExplicitSelfReceiverType(lowered.type)) {
            result.diag_id = "Record-Method-RecvSelf-Err";
            return result;
          }
          result.ok = true;
          result.type = SubstSelfType(base, lowered.type);
          return result;
        }
      },
      receiver);
}

std::optional<ParamMode> RecvModeOf(const ast::Receiver& receiver) {
  SpecDefsRecordMethods();
  return std::visit(
      [&](const auto& recv) -> std::optional<ParamMode> {
        using R = std::decay_t<decltype(recv)>;
        if constexpr (std::is_same_v<R, ast::ReceiverShorthand>) {
          return std::nullopt;
        } else {
          return LowerParamMode(recv.mode_opt);
        }
      },
      receiver);
}

RecvBaseTypeResult RecvBaseType(const ast::ExprPtr& base,
                                const std::optional<ParamMode>& mode,
                                const PlaceTypeFn& type_place,
                                const ExprTypeFn& type_expr) {
  SpecDefsRecordMethods();
  RecvBaseTypeResult result;
  if (!base) {
    return result;
  }
  if (!mode.has_value()) {
    if (HasSourceProvenance(base)) {
      if (!IsPlaceExprForCall(base)) {
        result.diag_id = "Call-Arg-NotPlace";
        return result;
      }
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

RecvArgOkResult RecvArgOk(const ast::ExprPtr& base,
                          const std::optional<ParamMode>& mode,
                          const ExprTypeFn& type_expr) {
  SpecDefsRecordMethods();
  RecvArgOkResult result;
  if (!base) {
    return result;
  }
  if (!mode.has_value()) {
    if (HasSourceProvenance(base) && !IsPlaceExprForCall(base)) {
      result.diag_id = "Call-Arg-NotPlace";
      return result;
    }
    if (HasSourceProvenance(base)) {
      const auto addr_ok = AddrOfOk(base, type_expr);
      if (!addr_ok.ok) {
        result.diag_id = addr_ok.diag_id;
        return result;
      }
    }
    result.ok = true;
    return result;
  }
  if (std::holds_alternative<ast::MoveExpr>(base->node)) {
    result.ok = true;
    return result;
  }
  return result;
}

ArgsOkResult ArgsOk(const ScopeContext& ctx,
                    const std::vector<ast::Param>& params,
                    const std::vector<ast::Arg>& args,
                    const ExprTypeFn& type_expr,
                    const PlaceTypeFn* type_place,
                    const LowerTypeFn& lower_type,
                    const ArgCheckFn* check_expr) {
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
    if (MissingRequiredMoveForConsuming(lowered_params[i].mode, args[i])) {
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
    if (!lowered_params[i].mode.has_value()) {
      const bool has_source_prov = HasSourceProvenance(arg.value);
      if (has_source_prov && !IsPlaceExprForCall(arg.value)) {
        SPEC_RULE("Call-Arg-NotPlace");
        result.diag_id = "Call-Arg-NotPlace";
        return result;
      }
      if (has_source_prov && type_place) {
        const auto place_type = (*type_place)(arg.value);
        if (!place_type.ok) {
          result.diag_id = place_type.diag_id;
          return result;
        }
        arg_types.push_back(place_type.type);
      } else {
        if (!has_source_prov && check_expr) {
          const auto checked = (*check_expr)(arg.value, lowered_params[i].type);
          if (checked.ok) {
            arg_types.push_back(lowered_params[i].type);
            continue;
          }
          if (checked.diag_id.has_value()) {
            result.diag_id = checked.diag_id;
            return result;
          }
        }
        const auto arg_type = type_expr(arg.value);
        if (!arg_type.ok) {
          result.diag_id = arg_type.diag_id;
          return result;
        }
        arg_types.push_back(arg_type.type);
      }
      continue;
    }
    const auto arg_expr = MovedArgExpr(arg);
    if (!HasSourceProvenance(arg.value) && check_expr) {
      const auto checked = (*check_expr)(arg_expr, lowered_params[i].type);
      if (checked.ok) {
        arg_types.push_back(lowered_params[i].type);
        continue;
      }
      if (checked.diag_id.has_value()) {
        result.diag_id = checked.diag_id;
        return result;
      }
    }
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
    if (!lowered_params[i].mode.has_value() &&
        HasSourceProvenance(args[i].value) &&
        !IsPlaceExprForCall(args[i].value)) {
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
      if (HasSourceProvenance(args[i].value)) {
        const auto addr_ok = AddrOfOk(args[i].value, type_expr);
        if (!addr_ok.ok) {
          result.diag_id = addr_ok.diag_id;
          return result;
        }
      }
      SPEC_RULE("Args-Cons-Ref");
    }
  }

  result.ok = true;
  return result;
}

ArgsOkResult ArgsOkWithSubst(const ScopeContext& ctx,
                             const std::vector<ast::Param>& params,
                             const std::vector<ast::Arg>& args,
                             const ExprTypeFn& type_expr,
                             const PlaceTypeFn* type_place,
                             const LowerTypeFn& lower_type,
                             const TypeSubst& subst,
                             const ArgCheckFn* check_expr) {
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
    TypeRef param_type = lowered.type;
    if (!subst.empty()) {
      param_type = InstantiateType(param_type, subst);
    }
    lowered_params.push_back(
        TypeFuncParam{LowerParamMode(param.mode), param_type});
  }

  for (std::size_t i = 0; i < args.size(); ++i) {
    if (MissingRequiredMoveForConsuming(lowered_params[i].mode, args[i])) {
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
    if (!lowered_params[i].mode.has_value()) {
      const bool has_source_prov = HasSourceProvenance(arg.value);
      if (has_source_prov && !IsPlaceExprForCall(arg.value)) {
        SPEC_RULE("Call-Arg-NotPlace");
        result.diag_id = "Call-Arg-NotPlace";
        return result;
      }
      if (has_source_prov && type_place) {
        const auto place_type = (*type_place)(arg.value);
        if (!place_type.ok) {
          result.diag_id = place_type.diag_id;
          return result;
        }
        arg_types.push_back(place_type.type);
      } else {
        if (!has_source_prov && check_expr) {
          const auto checked = (*check_expr)(arg.value, lowered_params[i].type);
          if (checked.ok) {
            arg_types.push_back(lowered_params[i].type);
            continue;
          }
          if (checked.diag_id.has_value()) {
            result.diag_id = checked.diag_id;
            return result;
          }
        }
        const auto arg_type = type_expr(arg.value);
        if (!arg_type.ok) {
          result.diag_id = arg_type.diag_id;
          return result;
        }
        arg_types.push_back(arg_type.type);
      }
      continue;
    }
    const auto arg_expr = MovedArgExpr(arg);
    if (!HasSourceProvenance(arg.value) && check_expr) {
      const auto checked = (*check_expr)(arg_expr, lowered_params[i].type);
      if (checked.ok) {
        arg_types.push_back(lowered_params[i].type);
        continue;
      }
      if (checked.diag_id.has_value()) {
        result.diag_id = checked.diag_id;
        return result;
      }
    }
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
    if (!lowered_params[i].mode.has_value() &&
        HasSourceProvenance(args[i].value) &&
        !IsPlaceExprForCall(args[i].value)) {
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
      if (HasSourceProvenance(args[i].value)) {
        const auto addr_ok = AddrOfOk(args[i].value, type_expr);
        if (!addr_ok.ok) {
          result.diag_id = addr_ok.diag_id;
          return result;
        }
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

  const auto normalized = NormalizeMethodBaseType(ctx, lookup_base);
  if (!normalized.ok) {
    result.diag_id = normalized.diag_id;
    return result;
  }
  if (normalized.type) {
    lookup_base = normalized.type;
  }
  result.normalized_base = lookup_base;

  const auto* path = lookup_base ? AppliedTypePath(*lookup_base) : nullptr;
  const ast::RecordDecl* record = nullptr;
  std::vector<ast::ClassPath> implements;
  if (path) {
    const auto type_decl_lookup = LookupTypeDeclByPath(ctx, *path);
    if (const auto* type_decl = type_decl_lookup.decl) {
      if (const auto* record_decl = std::get_if<ast::RecordDecl>(type_decl)) {
        record = record_decl;
        implements = record_decl->implements;
        result.record_path = type_decl_lookup.resolved_path;
      } else if (const auto* enum_decl =
                     std::get_if<ast::EnumDecl>(type_decl)) {
        implements = enum_decl->implements;
      } else if (const auto* modal_decl =
                     std::get_if<ast::ModalDecl>(type_decl)) {
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
    const ast::ClassMethodDecl* method = nullptr;
    ast::ClassPath owner;
  };
  std::vector<DefaultMethod> defaults;
  std::unordered_set<const ast::ClassMethodDecl*> seen_defaults;
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

}  // namespace cursive::analysis
