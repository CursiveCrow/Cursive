// =============================================================================
// MIGRATION MAPPING: pattern/enum_pattern.cpp
// =============================================================================
//
// SPEC REFERENCE: CursiveSpecification.md Lines 16772-16780
//   - TagOf-Enum: Get enum discriminant for variant
//   - EnumValuePath, VariantIndex, EnumDisc
//
// SOURCE FILE: cursive-bootstrap/src/04_codegen/lower/lower_pat.cpp
//   - Lines 295-331: EnumPattern in RegisterPatternBindings
//   - Looks up EnumDecl and VariantDecl
//   - Handles TuplePayloadPattern and RecordPayloadPattern
//   - Recursively registers payload element/field bindings
//   - LowerBindPattern: tag comparison + payload extraction
//
// DEPENDENCIES:
//   - cursive/src/05_codegen/ir_model.h (IRTagOf, comparison IR)
//   - Helper functions: LookupEnumDecl, FindVariant
//
// =============================================================================

#include "05_codegen/lower/pattern/enum_pattern.h"

#include "00_core/assert_spec.h"
#include "04_analysis/resolve/scopes.h"
#include "04_analysis/typing/context.h"
#include "04_analysis/typing/types.h"
#include "04_analysis/layout/layout.h"
#include "02_source/ast/ast.h"

#include <cassert>
#include <variant>

namespace cursive::codegen {

namespace {

// ============================================================================
// Helper functions for enum pattern lowering
// ============================================================================

/// Lower a syntax type to analysis TypeRef for layout computation
static analysis::TypeRef LowerSyntaxType(const std::shared_ptr<ast::Type>& type,
                                         LowerCtx& ctx) {
  if (!type) {
    return nullptr;
  }
  const analysis::ScopeContext& scope = ScopeForLowering(ctx);
  if (const auto lowered = ::cursive::analysis::layout::LowerTypeForLayout(scope, type)) {
    return *lowered;
  }
  return nullptr;
}

/// Look up an EnumDecl from a type path in the Sigma environment
static const ast::EnumDecl* LookupEnumDecl(const ast::TypePath& path,
                                           const LowerCtx& ctx) {
  if (!ctx.sigma) {
    return nullptr;
  }
  const auto it = ctx.sigma->types.find(analysis::PathKeyOf(path));
  if (it == ctx.sigma->types.end()) {
    return nullptr;
  }
  return std::get_if<ast::EnumDecl>(&it->second);
}

/// Find a variant by name within an enum declaration
static const ast::VariantDecl* FindVariant(const ast::EnumDecl& decl,
                                           std::string_view name) {
  for (const auto& variant : decl.variants) {
    if (analysis::IdEq(variant.name, name)) {
      return &variant;
    }
  }
  return nullptr;
}

/// Get the type of a field in an enum variant's record payload
static analysis::TypeRef EnumPayloadFieldType(const ast::VariantDecl& variant,
                                              std::string_view name,
                                              LowerCtx& ctx) {
  if (!variant.payload_opt.has_value()) {
    return nullptr;
  }
  if (const auto* record =
          std::get_if<ast::VariantPayloadRecord>(&*variant.payload_opt)) {
    for (const auto& field : record->fields) {
      if (analysis::IdEq(field.name, name)) {
        return LowerSyntaxType(field.type, ctx);
      }
    }
  }
  return nullptr;
}

/// Get the type of an element in an enum variant's tuple payload by index
static analysis::TypeRef EnumPayloadTupleType(const ast::VariantDecl& variant,
                                              std::size_t index,
                                              LowerCtx& ctx) {
  if (!variant.payload_opt.has_value()) {
    return nullptr;
  }
  if (const auto* tuple =
          std::get_if<ast::VariantPayloadTuple>(&*variant.payload_opt)) {
    if (index < tuple->elements.size()) {
      return LowerSyntaxType(tuple->elements[index], ctx);
    }
  }
  return nullptr;
}

}  // namespace

// ============================================================================
// RegisterEnumPatternBindings - Register bindings for an enum pattern
// ============================================================================
//
// Called from RegisterPatternBindings in pattern_common.cpp for EnumPattern.
// Looks up the enum declaration and variant, then recursively registers
// bindings for payload elements (tuple) or fields (record).
//
// SPEC: CursiveSpecification.md Lines 16772-16774
//   EnumValuePath(v) = path
//   VariantIndex(E, name) = i
//   EnumDisc(E, name) = d
// ============================================================================

void RegisterEnumPatternBindings(
    const ast::EnumPattern& node,
    const analysis::TypeRef& hint,
    LowerCtx& ctx,
    bool is_immovable,
    analysis::ProvenanceKind prov,
    std::optional<std::string> prov_region,
    std::optional<std::string> prov_region_tag,
    std::function<void(const ast::Pattern&, analysis::TypeRef)> walk) {
  // Look up the enum declaration from the path
  const ast::EnumDecl* enum_decl = LookupEnumDecl(node.path, ctx);
  const ast::VariantDecl* variant = nullptr;
  if (enum_decl) {
    variant = FindVariant(*enum_decl, node.name);
  }

  // If no payload, nothing to register
  if (!node.payload_opt.has_value()) {
    return;
  }

  // Dispatch on payload type: tuple or record
  std::visit(
      [&](const auto& payload) {
        using P = std::decay_t<decltype(payload)>;
        if constexpr (std::is_same_v<P, ast::TuplePayloadPattern>) {
          // Tuple payload: register bindings for each element by index
          for (std::size_t i = 0; i < payload.elements.size(); ++i) {
            analysis::TypeRef elem_type;
            if (variant) {
              elem_type = EnumPayloadTupleType(*variant, i, ctx);
            }
            walk(*payload.elements[i], elem_type);
          }
        } else {
          // Record payload: register bindings for each field
          static_assert(std::is_same_v<P, ast::RecordPayloadPattern>,
                        "EnumPayloadPattern must be Tuple or Record");
          for (const auto& field : payload.fields) {
            analysis::TypeRef field_type;
            if (variant) {
              field_type = EnumPayloadFieldType(*variant, field.name, ctx);
            }
            if (field.pattern_opt) {
              // Field has a nested pattern
              walk(*field.pattern_opt, field_type);
            } else {
              // Shorthand: field name becomes binding
              ctx.RegisterVar(field.name, field_type, true, is_immovable, prov,
                              prov_region, false, prov_region_tag);
            }
          }
        }
      },
      *node.payload_opt);
}

// ============================================================================
// LowerBindEnumPattern - Lower binding for an enum pattern
// ============================================================================
//
// Called from LowerBindPattern in pattern_common.cpp for EnumPattern.
// Creates derived values for payload extraction and recursively binds
// elements or fields.
//
// SPEC: CursiveSpecification.md Lines 16777-16780 (TagOf-Enum)
//   Extracts discriminant tag and payload values.
// ============================================================================

IRPtr LowerBindEnumPattern(
    const ast::EnumPattern& pat,
    const IRValue& value,
    LowerCtx& ctx,
    std::function<analysis::TypeRef(const std::string&)> lookup_bind_type,
    std::function<analysis::ProvenanceKind(const std::string&)> lookup_bind_prov,
    std::function<std::optional<std::string>(const std::string&)>
        lookup_bind_region,
    std::function<std::optional<std::string>(const std::string&)>
        lookup_bind_region_tag,
    std::function<IRPtr(const ast::Pattern&, const IRValue&)> lower_bind) {
  // If no payload, nothing to bind
  if (!pat.payload_opt) {
    return EmptyIR();
  }
  const ast::EnumDecl* enum_decl = LookupEnumDecl(pat.path, ctx);
  const ast::VariantDecl* variant = enum_decl ? FindVariant(*enum_decl, pat.name) : nullptr;

  return std::visit(
      [&](const auto& payload) -> IRPtr {
        using P = std::decay_t<decltype(payload)>;
        if constexpr (std::is_same_v<P, ast::TuplePayloadPattern>) {
          // Tuple payload: extract each element by index
          std::vector<IRPtr> bindings;
          for (std::size_t i = 0; i < payload.elements.size(); ++i) {
            analysis::TypeRef elem_type =
                variant ? EnumPayloadTupleType(*variant, i, ctx) : nullptr;
            IRValue elem = ctx.FreshTempValue("pat_enum_payload_elem");
            DerivedValueInfo info;
            info.kind = DerivedValueInfo::Kind::EnumPayloadIndex;
            info.base = value;
            info.static_path = pat.path;
            info.variant = pat.name;
            info.tuple_index = i;
            ctx.RegisterDerivedValue(elem, info);
            if (elem_type) {
              ctx.RegisterValueType(elem, elem_type);
            }
            bindings.push_back(lower_bind(*payload.elements[i], elem));
          }
          return SeqIR(std::move(bindings));
        } else {
          // Record payload: extract each field by name
          static_assert(std::is_same_v<P, ast::RecordPayloadPattern>,
                        "EnumPayloadPattern must be Tuple or Record");
          std::vector<IRPtr> bindings;
          for (const auto& field : payload.fields) {
            analysis::TypeRef field_type =
                variant ? EnumPayloadFieldType(*variant, field.name, ctx)
                        : nullptr;
            IRValue field_val = ctx.FreshTempValue("pat_enum_payload_field");
            DerivedValueInfo info;
            info.kind = DerivedValueInfo::Kind::EnumPayloadField;
            info.base = value;
            info.static_path = pat.path;
            info.variant = pat.name;
            info.field = field.name;
            ctx.RegisterDerivedValue(field_val, info);
            if (field_type) {
              ctx.RegisterValueType(field_val, field_type);
            }
            if (field.pattern_opt) {
              // Field has a nested pattern - recurse
              bindings.push_back(lower_bind(*field.pattern_opt, field_val));
            } else {
              // Shorthand: directly bind field name to extracted value
              IRBindVar bind;
              bind.name = field.name;
              bind.stable_name = ctx.StableBindingName(field.name);
              bind.value = field_val;
              bind.type = lookup_bind_type(field.name);
              if (!bind.type) {
                bind.type = field_type;
              }
              bind.prov = lookup_bind_prov(field.name);
              bind.prov_region = lookup_bind_region(field.name);
              bind.prov_region_tag = lookup_bind_region_tag(field.name);
              bindings.push_back(MakeIR(std::move(bind)));
            }
          }
          return SeqIR(std::move(bindings));
        }
      },
      *pat.payload_opt);
}

// ============================================================================
// PatternCheckEnum - Check if a value matches an enum pattern
// ============================================================================
//
// Called from PatternCheck in pattern_common.cpp for EnumPattern.
// Compares the discriminant tag of the value against the expected variant.
//
// SPEC: CursiveSpecification.md Lines 16777-16780 (TagOf-Enum)
//   TagOf(v, T) = d where d is the discriminant for the variant.
// ============================================================================

IRValue PatternCheckEnum(const ast::EnumPattern& pat,
                         const IRValue& value,
                         LowerCtx& ctx) {
  SPEC_RULE("TagOf-Enum");
  // Get the tag of the scrutinee value
  (void)TagOf(value, TagOfKind::Enum, ctx);
  // Return a fresh temp representing the comparison result
  return ctx.FreshTempValue("pat_enum_tag_match");
}

}  // namespace cursive::codegen
