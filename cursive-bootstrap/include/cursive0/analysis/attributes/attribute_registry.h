#pragma once

#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include "cursive0/core/span.h"
#include "cursive0/syntax/ast.h"

namespace cursive0::analysis {

// C0X Extension: Attribute System - Registry

// Attribute target (where attribute can be applied)
enum class AttributeTarget {
  Procedure,
  Record,
  Enum,
  Modal,
  Class,
  Field,
  Method,
  Param,
  TypeAlias,
  Static,
  Statement,
  KeyBlock,
};

// Attribute argument specification
struct AttributeArgSpec {
  std::string name;
  bool required = false;
  bool accepts_expr = false;  // Can take expression value
  std::optional<std::string> default_value;
};

// Attribute specification
struct AttributeSpec {
  std::string_view name;
  std::set<AttributeTarget> valid_targets;
  std::vector<AttributeArgSpec> args;
  bool deprecated = false;
  std::optional<std::string_view> deprecated_message;
};

// Built-in attribute names
namespace attrs {
  // Key system attributes
  constexpr std::string_view kDynamic = "dynamic";
  constexpr std::string_view kOrder = "order";
  
  // Contract attributes
  constexpr std::string_view kDebugContract = "debug_contract";
  constexpr std::string_view kReleaseContract = "release_contract";
  
  // Class attributes
  constexpr std::string_view kStaticDispatchOnly = "static_dispatch_only";
  
  // Memory attributes
  constexpr std::string_view kRepr = "repr";
  constexpr std::string_view kAlign = "align";
  constexpr std::string_view kPacked = "packed";
  
  // Diagnostic control
  constexpr std::string_view kAllow = "allow";
  constexpr std::string_view kWarn = "warn";
  constexpr std::string_view kDeny = "deny";
  constexpr std::string_view kForbid = "forbid";
  
  // Testing
  constexpr std::string_view kTest = "test";
  constexpr std::string_view kBench = "bench";
  constexpr std::string_view kIgnore = "ignore";
  
  // Visibility/linking
  constexpr std::string_view kInline = "inline";
  constexpr std::string_view kCold = "cold";
  constexpr std::string_view kHot = "hot";
  constexpr std::string_view kNoMangle = "no_mangle";
  constexpr std::string_view kExport = "export";
}

// Attribute registry
class AttributeRegistry {
 public:
  AttributeRegistry();
  
  // Register a new attribute
  void Register(const AttributeSpec& spec);
  
  // Lookup attribute specification
  const AttributeSpec* Lookup(std::string_view name) const;
  
  // Check if attribute is valid for target
  bool IsValidForTarget(std::string_view name, AttributeTarget target) const;
  
  // Get all registered attributes
  const std::map<std::string, AttributeSpec>& Specs() const { return specs_; }
  
 private:
  std::map<std::string, AttributeSpec> specs_;
};

// Global registry instance
const AttributeRegistry& GetAttributeRegistry();

// Attribute validation result
struct AttributeValidationResult {
  bool ok = true;
  std::optional<std::string_view> diag_id;
  std::optional<core::Span> span;
  std::string message;
};

// Validate attribute list
AttributeValidationResult ValidateAttributes(
    const syntax::AttributeList& attrs,
    AttributeTarget target);

// Check for specific attribute
bool HasAttribute(const syntax::AttributeList& attrs, std::string_view name);

// Get attribute value (first argument or named arg)
std::optional<std::string> GetAttributeValue(
    const syntax::AttributeList& attrs,
    std::string_view name,
    std::string_view arg_name = "");

}  // namespace cursive0::analysis
