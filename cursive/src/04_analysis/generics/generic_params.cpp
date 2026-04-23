/*
 * =============================================================================
 * generic_params.cpp - Generic Parameter Validation Implementation
 * =============================================================================
 *
 * SPEC REFERENCE:
 *   - CursiveSpecification.md, Section 13 "Generics" (lines 22300-22600)
 *   - CursiveSpecification.md, Section 13.1 "Generic Parameters" (line 22328)
 *   - CursiveSpecification.md, Section 13.2 "Generic Arguments" (lines 22380-22450)
 *   - CursiveSpecification.md, Section 13.3 "Type Bounds" (lines 22460-22520)
 *
 * This file implements the generic parameter validation system:
 *   - ValidateGenericParams: Main validation entry point
 *   - CheckParamUniqueness: Ensure no duplicate parameter names
 *   - ParseTypeParam: Parse and validate individual type parameters
 *   - ParseConstParam: Parse const generic parameters
 *   - ValidateDefaultValues: Validate default type argument ordering
 *   - BuildParamScope: Create scope containing type parameter bindings
 *
 * CRITICAL SYNTAX:
 *   - Generic parameters use SEMICOLONS: <T; U>
 *   - Generic arguments use COMMAS: <T, U>
 *   - Type bounds use <: syntax: <T <: Comparable>
 *   - Defaults use = syntax: <T; Alloc = DefaultAlloc>
 *
 * =============================================================================
 */

#include "04_analysis/generics/generic_params.h"

#include <algorithm>
#include <charconv>
#include <set>
#include <string_view>

#include "00_core/assert_spec.h"
#include "00_core/diagnostic_messages.h"
#include "04_analysis/resolve/scopes.h"
#include "04_analysis/typing/type_lower.h"

namespace cursive::analysis {

namespace {

static inline void SpecDefsGenericParams() {
  SPEC_DEF("GenericParams", "C0X.13.1");
  SPEC_DEF("TypeParam", "C0X.13.1");
  SPEC_DEF("TypeBound", "C0X.13.1");
  SPEC_DEF("BindTypeParams", "C0X.13.1");
  SPEC_DEF("DefaultArgs", "C0X.13.1.3");
}

core::Diagnostic MakeInternalGenericDiagnostic(
    core::Severity severity,
    const std::optional<core::Span>& span,
    const std::string& message) {
  core::Diagnostic diag;
  diag.severity = severity;
  diag.span = span;
  diag.message = message;
  return diag;
}

// Integral types valid for const generic parameters
const std::set<std::string> kConstParamTypes = {
    "i8", "i16", "i32", "i64", "i128",
    "u8", "u16", "u32", "u64", "u128",
    "isize", "usize"
};

// Check if a type name is a valid const generic parameter type
bool IsIntegralTypeName(const std::string& name) {
  return kConstParamTypes.find(name) != kConstParamTypes.end();
}

bool ParseI64Literal(std::string_view text, std::int64_t& out) {
  if (text.empty()) {
    return false;
  }

  const char* begin = text.data();
  const char* end = begin + text.size();
  auto [ptr, ec] = std::from_chars(begin, end, out, 10);
  return ec == std::errc{} && ptr == end;
}

std::optional<std::int64_t> ParseConstDefaultValue(
    const std::shared_ptr<ast::Type>& default_type) {
  if (!default_type) {
    return std::nullopt;
  }

  return std::visit(
      [](const auto& node) -> std::optional<std::int64_t> {
        using T = std::decay_t<decltype(node)>;
        std::int64_t value = 0;

        if constexpr (std::is_same_v<T, ast::TypePrim>) {
          if (ParseI64Literal(node.name, value)) {
            return value;
          }
        } else if constexpr (std::is_same_v<T, ast::TypePathType>) {
          if (node.path.size() == 1 && node.generic_args.empty() &&
              ParseI64Literal(node.path.front(), value)) {
            return value;
          }
        }

        return std::nullopt;
      },
      default_type->node);
}

// Lower an AST type to a TypeRef for validation
TypeRef LowerTypeForValidation(const ScopeContext& ctx,
                                const std::shared_ptr<ast::Type>& ast_type) {
  if (!ast_type) {
    return nullptr;
  }

  return std::visit(
      [&](const auto& node) -> TypeRef {
        using T = std::decay_t<decltype(node)>;

        if constexpr (std::is_same_v<T, ast::TypePrim>) {
          return MakeTypePrim(node.name);
        } else if constexpr (std::is_same_v<T, ast::TypePathType>) {
          TypePath path;
          path.reserve(node.path.size());
          for (const auto& ident : node.path) {
            path.push_back(ident);
          }

          // Handle generic arguments if present
          if (!node.generic_args.empty()) {
            std::vector<TypeRef> args;
            args.reserve(node.generic_args.size());
            for (const auto& arg : node.generic_args) {
              args.push_back(LowerTypeForValidation(ctx, arg));
            }
            return MakeTypePath(path, std::move(args));
          }

          return MakeTypePath(path);
        } else if constexpr (std::is_same_v<T, ast::TypeTuple>) {
          std::vector<TypeRef> elements;
          elements.reserve(node.elements.size());
          for (const auto& elem : node.elements) {
            elements.push_back(LowerTypeForValidation(ctx, elem));
          }
          return MakeTypeTuple(std::move(elements));
        } else if constexpr (std::is_same_v<T, ast::TypeArray>) {
          return MakeTypeArray(
              LowerTypeForValidation(ctx, node.element),
              0);  // Length computed separately
        } else if constexpr (std::is_same_v<T, ast::TypeSlice>) {
          return MakeTypeSlice(LowerTypeForValidation(ctx, node.element));
        } else if constexpr (std::is_same_v<T, ast::TypePermType>) {
          Permission perm;
          switch (node.perm) {
            case ast::TypePerm::Const:
              perm = Permission::Const;
              break;
            case ast::TypePerm::Unique:
              perm = Permission::Unique;
              break;
            case ast::TypePerm::Shared:
              perm = Permission::Shared;
              break;
          }
          return MakeTypePerm(perm, LowerTypeForValidation(ctx, node.base));
        } else if constexpr (std::is_same_v<T, ast::TypeUnion>) {
          std::vector<TypeRef> members;
          members.reserve(node.types.size());
          for (const auto& ty : node.types) {
            members.push_back(LowerTypeForValidation(ctx, ty));
          }
          return MakeTypeUnion(std::move(members));
        } else if constexpr (std::is_same_v<T, ast::TypeFunc>) {
          std::vector<TypeFuncParam> params;
          params.reserve(node.params.size());
          for (const auto& param : node.params) {
            std::optional<ParamMode> mode;
            if (param.mode) {
              mode = ParamMode::Move;
            }
            params.push_back(TypeFuncParam{
                mode,
                LowerTypeForValidation(ctx, param.type)});
          }
          return MakeTypeFunc(std::move(params),
                              LowerTypeForValidation(ctx, node.ret));
        } else if constexpr (std::is_same_v<T, ast::TypeClosure>) {
          std::vector<std::pair<bool, TypeRef>> params;
          params.reserve(node.params.size());
          for (const auto& param : node.params) {
            const bool is_move = param.mode.has_value();
            params.emplace_back(is_move,
                                LowerTypeForValidation(ctx, param.type));
          }
          TypeRef ret = LowerTypeForValidation(ctx, node.ret);
          std::optional<std::vector<SharedDep>> deps_opt;
          if (node.deps_opt.has_value()) {
            std::vector<SharedDep> deps;
            deps.reserve(node.deps_opt->size());
            for (const auto& dep : *node.deps_opt) {
              SharedDep lowered;
              lowered.name = dep.name;
              lowered.type = LowerTypeForValidation(ctx, dep.type);
              deps.push_back(std::move(lowered));
            }
            deps_opt = std::move(deps);
          }
          return MakeTypeClosure(std::move(params), ret, std::move(deps_opt));
        } else if constexpr (std::is_same_v<T, ast::TypeSafePtr>) {
          std::optional<PtrState> state;
          if (node.state) {
            switch (*node.state) {
              case ast::PtrState::Valid:
                state = PtrState::Valid;
                break;
              case ast::PtrState::Null:
                state = PtrState::Null;
                break;
              case ast::PtrState::Expired:
                state = PtrState::Expired;
                break;
            }
          }
          return MakeTypePtr(LowerTypeForValidation(ctx, node.element), state);
        } else if constexpr (std::is_same_v<T, ast::TypeRawPtr>) {
          RawPtrQual qual = node.qual == ast::RawPtrQual::Imm
                                ? RawPtrQual::Imm
                                : RawPtrQual::Mut;
          return MakeTypeRawPtr(qual, LowerTypeForValidation(ctx, node.element));
        } else if constexpr (std::is_same_v<T, ast::TypeString>) {
          std::optional<StringState> state;
          if (node.state) {
            state = *node.state == ast::StringState::Managed
                        ? StringState::Managed
                        : StringState::View;
          }
          return MakeTypeString(state);
        } else if constexpr (std::is_same_v<T, ast::TypeBytes>) {
          std::optional<BytesState> state;
          if (node.state) {
            state = *node.state == ast::BytesState::Managed
                        ? BytesState::Managed
                        : BytesState::View;
          }
          return MakeTypeBytes(state);
        } else if constexpr (std::is_same_v<T, ast::TypeDynamic>) {
          return MakeTypeDynamic(node.path);
        } else if constexpr (std::is_same_v<T, ast::TypeModalState>) {
          std::vector<TypeRef> args;
          args.reserve(node.generic_args.size());
          for (const auto& arg : node.generic_args) {
            args.push_back(LowerTypeForValidation(ctx, arg));
          }
          return MakeTypeModalState(node.path, node.state, std::move(args));
        } else if constexpr (std::is_same_v<T, ast::TypeOpaque>) {
          return MakeTypeOpaque(node.path, nullptr, core::Span{});
        } else if constexpr (std::is_same_v<T, ast::TypeRefine>) {
          return MakeTypeRefine(
              LowerTypeForValidation(ctx, node.base),
              node.predicate);
        } else {
          return nullptr;
        }
      },
      ast_type->node);
}

}  // namespace

// =============================================================================
// Validation Functions
// =============================================================================

GenericParamsResult ValidateGenericParams(
    const ScopeContext& ctx,
    const std::optional<ast::GenericParams>& params_opt) {
  SpecDefsGenericParams();
  SPEC_RULE("Validate-GenericParams");

  GenericParamsResult result;

  // No parameters is valid
  if (!params_opt || params_opt->params.empty()) {
    return result;
  }

  // SPEC: CursiveSpecification.md Section 13.1
  // "Generic parameters MUST have unique names within the parameter list."

  // Check uniqueness first
  auto uniqueness = CheckParamUniqueness(params_opt);
  if (!uniqueness.ok) {
    result.ok = false;
    result.diag_id = uniqueness.diag_id;
    result.diagnostics = std::move(uniqueness.diagnostics);
    return result;
  }

  // Validate default ordering
  auto defaults = ValidateDefaultValues(ctx, params_opt);
  if (!defaults.ok) {
    result.ok = false;
    result.diag_id = defaults.diag_id;
    result.diagnostics = std::move(defaults.diagnostics);
    return result;
  }

  // Parse each parameter
  result.type_params.reserve(params_opt->params.size());
  for (const auto& param : params_opt->params) {
    result.type_params.push_back(ParseTypeParam(ctx, param));
  }

  return result;
}

ParamUniquenessResult CheckParamUniqueness(
    const std::optional<ast::GenericParams>& params_opt) {
  SpecDefsGenericParams();
  SPEC_RULE("Check-ParamUniqueness");

  ParamUniquenessResult result;

  if (!params_opt || params_opt->params.empty()) {
    return result;
  }

  // SPEC: E-TYP-2304 "Duplicate type parameter name"
  std::set<std::string> seen_names;

  for (const auto& param : params_opt->params) {
    if (seen_names.find(param.name) != seen_names.end()) {
      result.ok = false;
      result.diag_id = "E-TYP-2304";
      result.duplicate_name = param.name;
      if (auto diag = core::MakeDiagnosticById("E-TYP-2304", param.span)) {
        diag->message = "Duplicate type parameter name: " + param.name;
        result.diagnostics.push_back(*diag);
      } else {
        result.diagnostics.push_back(MakeInternalGenericDiagnostic(
            core::Severity::Error, param.span,
            "Internal error: unresolved diagnostic id 'E-TYP-2304'"));
      }

      return result;
    }
    seen_names.insert(param.name);
  }

  return result;
}

TypeParamInfo ParseTypeParam(
    const ScopeContext& ctx,
    const ast::TypeParam& param) {
  SpecDefsGenericParams();
  SPEC_RULE("Parse-TypeParam");

  // SPEC: CursiveSpecification.md (Parse-TypeParam) rule
  // TypeParam = <name, bounds, default_opt, variance>

  TypeParamInfo info;
  info.name = param.name;
  info.span = param.span;

  // Process bounds: T <: Class1, Class2
  for (const auto& bound : param.bounds) {
    info.class_bounds.push_back(bound);
  }

  // Process default type if present
  if (param.default_type) {
    info.default_type = LowerTypeForValidation(ctx, param.default_type);
  }

  return info;
}

ConstParamInfo ParseConstParam(
    const ScopeContext& ctx,
    const ast::TypeParam& param,
    const TypeRef& param_type) {
  SpecDefsGenericParams();
  SPEC_RULE("Parse-ConstParam");
  (void)ctx;

  // SPEC: CursiveSpecification.md Section 13.1.2 "Const Generic Parameters"
  // Const parameters must have integral type

  ConstParamInfo info;
  info.name = param.name;
  info.span = param.span;
  info.type = IsValidConstParamType(param_type) ? param_type : nullptr;

  // If the parser provides a representable const default literal shape, store it.
  // Otherwise, keep default_value empty.
  info.default_value = ParseConstDefaultValue(param.default_type);

  return info;
}

DefaultValueResult ValidateDefaultValues(
    const ScopeContext& ctx,
    const std::optional<ast::GenericParams>& params_opt) {
  SpecDefsGenericParams();
  SPEC_RULE("Validate-DefaultValues");

  DefaultValueResult result;

  if (!params_opt || params_opt->params.empty()) {
    return result;
  }

  // SPEC: CursiveSpecification.md Section 13.1.3 "Default Type Arguments"
  // "Parameters with defaults MUST appear after all parameters without defaults."
  // E-TYP-2303: Default after non-default parameter

  bool seen_default = false;

  for (const auto& param : params_opt->params) {
    bool has_default = param.default_type != nullptr;

    if (seen_default && !has_default) {
      // Parameter without default after parameter with default
      result.ok = false;
      result.diag_id = "E-TYP-2303";
      result.param_name = param.name;

      if (auto diag = core::MakeDiagnosticById("E-TYP-2303", param.span)) {
        diag->message = "Parameter '" + param.name +
                        "' without default follows parameter with default";
        result.diagnostics.push_back(*diag);
      } else {
        result.diagnostics.push_back(MakeInternalGenericDiagnostic(
            core::Severity::Error, param.span,
            "Internal error: unresolved diagnostic id 'E-TYP-2303'"));
      }

      return result;
    }

    if (has_default) {
      if (!LowerTypeForValidation(ctx, param.default_type)) {
        result.ok = false;
        result.diag_id.reset();
        result.param_name = param.name;
        result.diagnostics.push_back(MakeInternalGenericDiagnostic(
            core::Severity::Error, param.span,
            "Internal error: unable to lower default type for generic parameter '" +
                param.name + "'"));

        return result;
      }
      seen_default = true;
    }
  }

  return result;
}

// =============================================================================
// Scope Building
// =============================================================================

Scope BuildParamScope(
    const ScopeContext& ctx,
    const std::optional<ast::GenericParams>& params_opt) {
  SpecDefsGenericParams();
  SPEC_RULE("Build-ParamScope");

  // SPEC: BindTypeParams(Gamma, params)
  // Each type parameter is bound as a type entity

  Scope scope;

  if (!params_opt || params_opt->params.empty()) {
    return scope;
  }

  for (const auto& param : params_opt->params) {
    // Create an entity for the type parameter
    Entity entity;
    entity.kind = EntityKind::Type;
    entity.source = EntitySource::Decl;
    // Type parameters don't have an origin module path - they're local
    entity.origin_opt = std::nullopt;
    entity.target_opt = param.name;

    // Bind the parameter name in the scope
    IdKey key = IdKeyOf(param.name);
    scope[key] = entity;
  }

  return scope;
}

// =============================================================================
// Helper Functions
// =============================================================================

bool IsValidConstParamType(const TypeRef& type) {
  SpecDefsGenericParams();
  SPEC_RULE("ConstParam-Type-Check");

  if (!type) {
    return false;
  }

  // SPEC: CursiveSpecification.md Section 13.1.2
  // "Const generic parameters MUST have integral type."

  if (const auto* prim = std::get_if<TypePrim>(&type->node)) {
    return IsIntegralTypeName(prim->name);
  }

  return false;
}

std::size_t RequiredParamCount(
    const std::optional<ast::GenericParams>& params_opt) {
  if (!params_opt || params_opt->params.empty()) {
    return 0;
  }

  std::size_t count = 0;
  for (const auto& param : params_opt->params) {
    if (!param.default_type) {
      ++count;
    }
  }
  return count;
}

std::size_t TotalParamCount(
    const std::optional<ast::GenericParams>& params_opt) {
  if (!params_opt) {
    return 0;
  }
  return params_opt->params.size();
}

bool HasDefaultParams(const std::optional<ast::GenericParams>& params_opt) {
  if (!params_opt || params_opt->params.empty()) {
    return false;
  }

  for (const auto& param : params_opt->params) {
    if (param.default_type) {
      return true;
    }
  }
  return false;
}

}  // namespace cursive::analysis
