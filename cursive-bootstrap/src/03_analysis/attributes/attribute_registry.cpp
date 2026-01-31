#include "cursive0/03_analysis/attributes/attribute_registry.h"

#include "cursive0/00_core/assert_spec.h"

namespace cursive0::analysis {

namespace {

static inline void SpecDefsAttributes() {
  SPEC_DEF("Attribute", "C0X.6.A");
  SPEC_DEF("AttrTarget", "C0X.6.A");
  SPEC_DEF("AttrValidation", "C0X.6.A");
  SPEC_DEF("AttrRegistry", "C0X.6.A");
}

// Initialize the global registry with built-in attributes
AttributeRegistry InitializeRegistry() {
  SpecDefsAttributes();
  SPEC_RULE("AttrRegistry-Init");
  
  AttributeRegistry registry;
  
  // [[dynamic]] - Dynamic verification scope
  {
    AttributeSpec spec;
    spec.name = attrs::kDynamic;
    spec.valid_targets = {AttributeTarget::Procedure, AttributeTarget::Record,
                          AttributeTarget::Enum, AttributeTarget::Modal,
                          AttributeTarget::Class, AttributeTarget::Method,
                          AttributeTarget::Static, AttributeTarget::Binding,
                          AttributeTarget::Statement, AttributeTarget::Expression,
                          AttributeTarget::KeyBlock};
    registry.Register(spec);
  }
  
  // [[order(path1, path2, ...)]] - Key ordering
  {
    AttributeSpec spec;
    spec.name = attrs::kOrder;
    spec.valid_targets = {AttributeTarget::KeyBlock};
    spec.args.push_back({"paths", false, true, std::nullopt});
    registry.Register(spec);
  }
  
  // [[debug_contract]] - Contract only in debug mode
  {
    AttributeSpec spec;
    spec.name = attrs::kDebugContract;
    spec.valid_targets = {AttributeTarget::Procedure, AttributeTarget::Method};
    registry.Register(spec);
  }
  
  // [[release_contract]] - Contract also in release mode
  {
    AttributeSpec spec;
    spec.name = attrs::kReleaseContract;
    spec.valid_targets = {AttributeTarget::Procedure, AttributeTarget::Method};
    registry.Register(spec);
  }
  
  // [[static_dispatch_only]] - Class method not vtable eligible
  {
    AttributeSpec spec;
    spec.name = attrs::kStaticDispatchOnly;
    spec.valid_targets = {AttributeTarget::Method};
    registry.Register(spec);
  }
  
  // [[layout(...)]] - Memory representation
  {
    AttributeSpec spec;
    spec.name = attrs::kLayout;
    spec.valid_targets = {AttributeTarget::Record, AttributeTarget::Enum};
    spec.args.push_back({"kind", true, false, std::nullopt});  // C, packed, transparent
    registry.Register(spec);
  }
  
  // [[align(N)]] - Alignment
  {
    AttributeSpec spec;
    spec.name = attrs::kAlign;
    spec.valid_targets = {AttributeTarget::Record, AttributeTarget::Field};
    spec.args.push_back({"alignment", true, true, std::nullopt});
    registry.Register(spec);
  }
  
  // [[packed]] - Pack fields
  {
    AttributeSpec spec;
    spec.name = attrs::kPacked;
    spec.valid_targets = {AttributeTarget::Record};
    registry.Register(spec);
  }

  // Memory order attributes
  {
    AttributeSpec spec;
    spec.name = attrs::kRelaxed;
    spec.valid_targets = {AttributeTarget::Expression};
    registry.Register(spec);
  }
  {
    AttributeSpec spec;
    spec.name = attrs::kAcquire;
    spec.valid_targets = {AttributeTarget::Expression};
    registry.Register(spec);
  }
  {
    AttributeSpec spec;
    spec.name = attrs::kRelease;
    spec.valid_targets = {AttributeTarget::Expression};
    registry.Register(spec);
  }
  {
    AttributeSpec spec;
    spec.name = attrs::kAcqRel;
    spec.valid_targets = {AttributeTarget::Expression};
    registry.Register(spec);
  }
  {
    AttributeSpec spec;
    spec.name = attrs::kSeqCst;
    spec.valid_targets = {AttributeTarget::Expression};
    registry.Register(spec);
  }
  
  // [[allow(lint_name)]] - Allow lint
  {
    AttributeSpec spec;
    spec.name = attrs::kAllow;
    spec.valid_targets = {
        AttributeTarget::Procedure, AttributeTarget::Record,
        AttributeTarget::Enum, AttributeTarget::Modal,
        AttributeTarget::Class, AttributeTarget::Field,
        AttributeTarget::Method, AttributeTarget::Statement};
    spec.args.push_back({"lint", true, false, std::nullopt});
    registry.Register(spec);
  }
  
  // [[warn(lint_name)]] - Warn on lint
  {
    AttributeSpec spec;
    spec.name = attrs::kWarn;
    spec.valid_targets = {
        AttributeTarget::Procedure, AttributeTarget::Record,
        AttributeTarget::Enum, AttributeTarget::Modal,
        AttributeTarget::Class, AttributeTarget::Field,
        AttributeTarget::Method, AttributeTarget::Statement};
    spec.args.push_back({"lint", true, false, std::nullopt});
    registry.Register(spec);
  }
  
  // [[deny(lint_name)]] - Deny lint (error)
  {
    AttributeSpec spec;
    spec.name = attrs::kDeny;
    spec.valid_targets = {
        AttributeTarget::Procedure, AttributeTarget::Record,
        AttributeTarget::Enum, AttributeTarget::Modal,
        AttributeTarget::Class, AttributeTarget::Field,
        AttributeTarget::Method, AttributeTarget::Statement};
    spec.args.push_back({"lint", true, false, std::nullopt});
    registry.Register(spec);
  }
  
  // [[forbid(lint_name)]] - Forbid lint (cannot be overridden)
  {
    AttributeSpec spec;
    spec.name = attrs::kForbid;
    spec.valid_targets = {
        AttributeTarget::Procedure, AttributeTarget::Record,
        AttributeTarget::Enum, AttributeTarget::Modal,
        AttributeTarget::Class, AttributeTarget::Field,
        AttributeTarget::Method, AttributeTarget::Statement};
    spec.args.push_back({"lint", true, false, std::nullopt});
    registry.Register(spec);
  }
  
  // [[test]] - Test function
  {
    AttributeSpec spec;
    spec.name = attrs::kTest;
    spec.valid_targets = {AttributeTarget::Procedure};
    registry.Register(spec);
  }
  
  // [[bench]] - Benchmark function
  {
    AttributeSpec spec;
    spec.name = attrs::kBench;
    spec.valid_targets = {AttributeTarget::Procedure};
    registry.Register(spec);
  }
  
  // [[ignore]] - Ignored test
  {
    AttributeSpec spec;
    spec.name = attrs::kIgnore;
    spec.valid_targets = {AttributeTarget::Procedure};
    registry.Register(spec);
  }

  // [[deprecated]] - Deprecated declaration
  {
    AttributeSpec spec;
    spec.name = attrs::kDeprecated;
    spec.valid_targets = {AttributeTarget::Procedure, AttributeTarget::Record,
                          AttributeTarget::Enum, AttributeTarget::Modal,
                          AttributeTarget::Class, AttributeTarget::Field,
                          AttributeTarget::Method, AttributeTarget::TypeAlias,
                          AttributeTarget::Static};
    spec.args.push_back({"message", false, false, std::nullopt});
    registry.Register(spec);
  }

  // [[stale_ok]] - Suppress shared staleness warnings
  {
    AttributeSpec spec;
    spec.name = attrs::kStaleOk;
    spec.valid_targets = {AttributeTarget::Binding};
    registry.Register(spec);
  }
  
  // [[inline]] - Inline hint
  {
    AttributeSpec spec;
    spec.name = attrs::kInline;
    spec.valid_targets = {AttributeTarget::Procedure, AttributeTarget::Method};
    spec.args.push_back({"kind", false, false, "always"});  // always, never, hint
    registry.Register(spec);
  }
  
  // [[cold]] - Cold function (unlikely to be called)
  {
    AttributeSpec spec;
    spec.name = attrs::kCold;
    spec.valid_targets = {AttributeTarget::Procedure, AttributeTarget::Method};
    registry.Register(spec);
  }
  
  // [[hot]] - Hot function (frequently called)
  {
    AttributeSpec spec;
    spec.name = attrs::kHot;
    spec.valid_targets = {AttributeTarget::Procedure, AttributeTarget::Method};
    registry.Register(spec);
  }
  
  // [[no_mangle]] - Don't mangle name
  {
    AttributeSpec spec;
    spec.name = attrs::kNoMangle;
    spec.valid_targets = {AttributeTarget::Procedure, AttributeTarget::Static};
    registry.Register(spec);
  }
  
  // [[export]] - Export symbol
  {
    AttributeSpec spec;
    spec.name = attrs::kExport;
    spec.valid_targets = {AttributeTarget::Procedure, AttributeTarget::Static};
    spec.args.push_back({"link_name", false, false, std::nullopt});
    registry.Register(spec);
  }

  // [[symbol("name")]] - Explicit symbol name (FFI)
  {
    AttributeSpec spec;
    spec.name = attrs::kSymbol;
    spec.valid_targets = {AttributeTarget::Procedure};
    spec.args.push_back({"name", true, false, std::nullopt});
    registry.Register(spec);
  }

  // [[library("name")]] - Link library (extern block)
  {
    AttributeSpec spec;
    spec.name = attrs::kLibrary;
    spec.valid_targets = {AttributeTarget::ExternBlock};
    spec.args.push_back({"name", true, false, std::nullopt});
    registry.Register(spec);
  }

  // [[unwind(mode)]] - FFI unwind mode
  {
    AttributeSpec spec;
    spec.name = attrs::kUnwind;
    spec.valid_targets = {AttributeTarget::Procedure};
    spec.args.push_back({"mode", false, false, "abort"});
    registry.Register(spec);
  }

  // [[ffi_pass_by_value]] - Force by-value FFI passing
  {
    AttributeSpec spec;
    spec.name = attrs::kFfiPassByValue;
    spec.valid_targets = {AttributeTarget::Procedure};
    registry.Register(spec);
  }

  // Verification-mode attributes
  {
    AttributeSpec spec;
    spec.name = attrs::kStatic;
    spec.valid_targets = {AttributeTarget::ExternBlock, AttributeTarget::Procedure};
    registry.Register(spec);
  }
  {
    AttributeSpec spec;
    spec.name = attrs::kAssume;
    spec.valid_targets = {AttributeTarget::ExternBlock, AttributeTarget::Procedure};
    registry.Register(spec);
  }
  {
    AttributeSpec spec;
    spec.name = attrs::kTrust;
    spec.valid_targets = {AttributeTarget::ExternBlock, AttributeTarget::Procedure};
    registry.Register(spec);
  }
  
  return registry;
}

}  // namespace

AttributeRegistry::AttributeRegistry() = default;

void AttributeRegistry::Register(const AttributeSpec& spec) {
  specs_[std::string(spec.name)] = spec;
}

const AttributeSpec* AttributeRegistry::Lookup(std::string_view name) const {
  auto it = specs_.find(std::string(name));
  if (it == specs_.end()) {
    return nullptr;
  }
  return &it->second;
}

bool AttributeRegistry::IsValidForTarget(std::string_view name,
                                         AttributeTarget target) const {
  const auto* spec = Lookup(name);
  if (!spec) {
    return false;  // Unknown attribute
  }
  return spec->valid_targets.count(target) > 0;
}

const AttributeRegistry& GetAttributeRegistry() {
  static const AttributeRegistry registry = InitializeRegistry();
  return registry;
}

AttributeValidationResult ValidateAttributes(
    const syntax::AttributeList& attrs,
    AttributeTarget target) {
  SpecDefsAttributes();
  SPEC_RULE("AttrValidation");
  
  AttributeValidationResult result;
  const auto& registry = GetAttributeRegistry();
  
  for (const auto& attr : attrs) {
    const auto* spec = registry.Lookup(attr.name);
    
    // Unknown attribute
    if (!spec) {
      result.ok = false;
      result.diag_id = "E-SEM-2901";  // Unknown attribute
      result.span = attr.span;
      result.message = "Unknown attribute: " + attr.name;
      return result;
    }
    
    // Deprecated attribute
    if (spec->deprecated) {
      // Emit warning (non-fatal)
    }
    
    // Invalid target
    if (!registry.IsValidForTarget(attr.name, target)) {
      result.ok = false;
      result.diag_id = "E-SEM-2902";  // Invalid attribute target
      result.span = attr.span;
      result.message = "Attribute '" + attr.name + "' cannot be applied here";
      return result;
    }
    
    // Validate arguments
    for (const auto& arg_spec : spec->args) {
      if (arg_spec.required) {
        bool found = false;
        for (const auto& arg : attr.args) {
          if (arg.key && *arg.key == arg_spec.name) {
            found = true;
            break;
          }
          // Positional arg for first required arg
          if (!arg.key && &arg_spec == &spec->args[0]) {
            found = true;
            break;
          }
        }
        if (!found) {
          result.ok = false;
          result.diag_id = "E-SEM-2903";  // Missing required argument
          result.span = attr.span;
          result.message = "Missing required argument: " + arg_spec.name;
          return result;
        }
      }
    }
  }
  
  return result;
}

bool HasAttribute(const syntax::AttributeList& attrs, std::string_view name) {
  for (const auto& attr : attrs) {
    if (attr.name == name) {
      return true;
    }
  }
  return false;
}

std::optional<std::string> GetAttributeValue(
    const syntax::AttributeList& attrs,
    std::string_view name,
    std::string_view arg_name) {
  for (const auto& attr : attrs) {
    if (attr.name != name) {
      continue;
    }
    
    for (const auto& arg : attr.args) {
      // Named argument
      if (!arg_name.empty() && arg.key && *arg.key == arg_name) {
        if (const auto* token = std::get_if<syntax::Token>(&arg.value)) {
          return std::string(token->lexeme);
        }
      }
      // First positional argument
      if (arg_name.empty() && !arg.key) {
        if (const auto* token = std::get_if<syntax::Token>(&arg.value)) {
          return std::string(token->lexeme);
        }
      }
    }
  }
  
  return std::nullopt;
}

}  // namespace cursive0::analysis
