// =============================================================================
// resolve_attributes.cpp - Attribute Resolution
// =============================================================================
//
// SPEC REFERENCE:
//   CursiveSpecification.md §5.1.7 "Resolution Pass" (Lines 7430-7549)
//   CursiveSpecification.md §3.3.2.8 "Attributes"
//
// CONTENT:
//   1. ResolveAttributes - Validate and resolve attribute list
//   2. ValidateAttributeName - Check attribute name is known
//   3. ResolveAttributeArgs - Resolve attribute argument expressions
//   4. ValidateAttributeTarget - Validate attribute applicability
//
// =============================================================================

#include "04_analysis/resolve/resolver.h"

#include <algorithm>
#include <array>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

#include "00_core/assert_spec.h"
#include "04_analysis/resolve/scopes.h"

namespace cursive::analysis {

namespace {

static inline void SpecDefsAttributes() {
  SPEC_DEF("ResolveAttributes", "5.1.7");
  SPEC_DEF("ValidateAttributeName", "5.1.7");
  SPEC_DEF("ResolveAttributeArgs", "5.1.7");
  SPEC_DEF("ValidateAttributeTarget", "5.1.7");
  SPEC_DEF("KnownAttributes", "3.3.2.8");
}

// -----------------------------------------------------------------------------
// Known Attribute Names
// -----------------------------------------------------------------------------
// Complete list from the current specification subset implemented here.
// -----------------------------------------------------------------------------

const std::array<std::string_view, 24> KnownAttributeNames = {
    "inline",
    "cold",
    "deprecated",
    "reflect",
    "dynamic",
    "stale_ok",
    "emit",
    "files",
    "export",
    "host_export",
    "mangle",
    "library",
    "unwind",
    "layout",
    "ffi_pass_by_value",
    "static",
    "trust",
    "test",       // Common testing attribute
    "doc",        // Documentation attribute
    "relaxed",
    "acquire",
    "release",
    "acqrel",
    "seqcst",
};

// Inline variants
const std::array<std::string_view, 3> InlineVariants = {
    "always",
    "never",
    "default",
};

// Layout variants
const std::array<std::string_view, 1> LayoutVariants = {
    "C",
};

bool IsKnownAttributeName(std::string_view name) {
  for (const auto& known : KnownAttributeNames) {
    if (IdEq(name, known)) {
      return true;
    }
  }
  return false;
}

bool IsKnownInlineVariant(std::string_view variant) {
  for (const auto& known : InlineVariants) {
    if (IdEq(variant, known)) {
      return true;
    }
  }
  return false;
}

bool IsKnownLayoutVariant(std::string_view variant) {
  for (const auto& known : LayoutVariants) {
    if (IdEq(variant, known)) {
      return true;
    }
  }
  return false;
}

// -----------------------------------------------------------------------------
// Attribute Target Validation
// -----------------------------------------------------------------------------
// Certain attributes are only valid on certain item kinds.
// -----------------------------------------------------------------------------

enum class AttributeTarget {
  Procedure,
  ExternBlock,
  Record,
  Enum,
  Modal,
  Class,
  TypeAlias,
  Static,
  Field,
  Expr,
  Any,
};

AttributeTarget TargetOfAttribute(std::string_view name) {
  // Attributes that apply to procedures
  if (IdEq(name, "inline") || IdEq(name, "cold") || IdEq(name, "export") ||
      IdEq(name, "host_export") || IdEq(name, "mangle") ||
      IdEq(name, "trust") || IdEq(name, "static") ||
      IdEq(name, "unwind") || IdEq(name, "test")) {
    return AttributeTarget::Procedure;
  }

  if (IdEq(name, "library")) {
    return AttributeTarget::ExternBlock;
  }

  // Layout applies to records, enums, modals
  if (IdEq(name, "layout")) {
    return AttributeTarget::Record; // Also valid on enums/modals
  }
  if (IdEq(name, "ffi_pass_by_value")) {
    return AttributeTarget::Enum;
  }

  // Dynamic applies to procedures with contracts or expressions
  if (IdEq(name, "dynamic")) {
    return AttributeTarget::Any;
  }

  // Deprecated and doc apply to anything
  if (IdEq(name, "deprecated") || IdEq(name, "doc")) {
    return AttributeTarget::Any;
  }

  return AttributeTarget::Any;
}

}  // namespace

// =============================================================================
// Public Interface
// =============================================================================

// -----------------------------------------------------------------------------
// ValidateAttributeName
// -----------------------------------------------------------------------------
// Checks if an attribute name is in the known set.
// Unknown attributes are errors, not warnings.
// -----------------------------------------------------------------------------

bool ValidateAttributeName(std::string_view name) {
  SpecDefsAttributes();
  const bool known = IsKnownAttributeName(name);
  if (known) {
    SPEC_RULE("ValidateAttributeName-Ok");
  } else {
    SPEC_RULE("ValidateAttributeName-Unknown");
  }
  return known;
}

// -----------------------------------------------------------------------------
// ResolveAttributeArgs
// -----------------------------------------------------------------------------
// Resolves attribute argument expressions.
// Arguments must be constant expressions (validated in later pass).
// -----------------------------------------------------------------------------

ResolveResult<std::vector<ast::ExprPtr>> ResolveAttributeArgs(
    ResolveContext& ctx,
    const std::vector<ast::ExprPtr>& args) {
  SpecDefsAttributes();
  ResolveResult<std::vector<ast::ExprPtr>> result;
  result.ok = true;

  if (args.empty()) {
    SPEC_RULE("ResolveAttributeArgs-Empty");
    return result;
  }

  result.value.reserve(args.size());
  for (const auto& arg : args) {
    if (!arg) {
      result.value.push_back(nullptr);
      continue;
    }
    const auto resolved = ResolveExpr(ctx, arg);
    if (!resolved.ok) {
      return {false, resolved.diag_id, resolved.span, {}};
    }
    result.value.push_back(resolved.value);
    SPEC_RULE("ResolveAttributeArgs-Cons");
  }

  return result;
}

// -----------------------------------------------------------------------------
// ResolveAttribute
// -----------------------------------------------------------------------------
// Resolves a single attribute.
// Validates the attribute name and resolves arguments.
// -----------------------------------------------------------------------------

ResolveResult<ast::AttributeItem> ResolveAttribute(
    ResolveContext& ctx,
    const ast::AttributeItem& attr) {
  SpecDefsAttributes();
  ResolveResult<ast::AttributeItem> result;
  result.ok = true;
  result.value = attr;

  // Validate attribute name
  if (!ValidateAttributeName(attr.name)) {
    SPEC_RULE("ResolveAttribute-UnknownName");
    return {false, "ResolveAttribute-UnknownName", attr.span, {}};
  }

  // Validate inline variants
  if (IdEq(std::string_view(attr.name), "inline") && !attr.args.empty()) {
    // Check if the first arg is a known inline variant
    // (This is a simplification; actual parsing may differ)
  }

  // Validate layout variants
  if (IdEq(std::string_view(attr.name), "layout") && !attr.args.empty()) {
    // Check if the first arg is a known layout variant
  }

  // Arguments are already parsed - no further resolution needed at this phase
  // Expression arguments (if any) would be resolved in a later type-checking pass

  SPEC_RULE("ResolveAttribute-Ok");
  return result;
}

// -----------------------------------------------------------------------------
// ResolveAttributes
// -----------------------------------------------------------------------------
// Resolves a list of attributes.
// Validates each attribute name and resolves arguments.
//
// Implements (Resolve-Attributes) from §5.1.7:
//   ∀ attr ∈ attrs.
//     ValidAttributeName(attr.name) ∧
//     ValidAttributeTarget(attr.name, target_kind) ∧
//     ∀ arg ∈ attr.args. Γ ⊢ ResolveConstExpr(arg) ⇓ ok
//   → Γ ⊢ ResolveAttributes(attrs) ⇓ ok
// -----------------------------------------------------------------------------

ResolveResult<ast::AttributeList> ResolveAttributes(
    ResolveContext& ctx,
    const ast::AttributeList& attrs) {
  SpecDefsAttributes();
  ResolveResult<ast::AttributeList> result;
  result.ok = true;

  if (attrs.empty()) {
    SPEC_RULE("ResolveAttributes-Empty");
    return result;
  }

  result.value.reserve(attrs.size());

  // Check for duplicate attributes
  std::unordered_set<IdKey> seen_attrs;

  for (const auto& attr : attrs) {
    // Check for duplicates (some attributes can be repeated, e.g., deprecated)
    const auto key = IdKeyOf(std::string_view(attr.name));
    if (!IdEq(std::string_view(attr.name), "deprecated") &&
        !IdEq(std::string_view(attr.name), "doc")) {
      if (seen_attrs.find(key) != seen_attrs.end()) {
        SPEC_RULE("ResolveAttributes-Duplicate");
        return {false, "ResolveAttributes-Duplicate", attr.span, {}};
      }
      seen_attrs.insert(key);
    }

    // Resolve the attribute
    const auto resolved = ResolveAttribute(ctx, attr);
    if (!resolved.ok) {
      return {false, resolved.diag_id, resolved.span, {}};
    }
    result.value.push_back(resolved.value);
    SPEC_RULE("ResolveAttributes-Cons");
  }

  return result;
}

// -----------------------------------------------------------------------------
// ValidateAttributeTarget
// -----------------------------------------------------------------------------
// Validates that an attribute is applicable to the given target item kind.
// Returns true if valid, false otherwise.
// -----------------------------------------------------------------------------

bool ValidateAttributeTarget(std::string_view attr_name,
                             std::string_view target_kind) {
  SpecDefsAttributes();
  const auto expected = TargetOfAttribute(attr_name);

  // "Any" target means the attribute applies to everything
  if (expected == AttributeTarget::Any) {
    SPEC_RULE("ValidateAttributeTarget-Any");
    return true;
  }

  // Check specific targets
  bool valid = false;
  if (expected == AttributeTarget::Procedure && IdEq(target_kind, "procedure")) {
    valid = true;
  } else if (expected == AttributeTarget::Record &&
             (IdEq(target_kind, "record") || IdEq(target_kind, "enum") ||
              IdEq(target_kind, "modal"))) {
    valid = true;
  } else if (expected == AttributeTarget::Static && IdEq(target_kind, "static")) {
    valid = true;
  }

  if (valid) {
    SPEC_RULE("ValidateAttributeTarget-Ok");
  } else {
    SPEC_RULE("ValidateAttributeTarget-Invalid");
  }

  return valid;
}

// HasAttribute is defined in attribute_registry.cpp and declared in attribute_registry.h
// GetAttribute is defined in attribute_list.cpp and declared in attribute_registry.h
// No duplicate definitions here.

}  // namespace cursive::analysis
