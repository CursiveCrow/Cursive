// =============================================================================
// MIGRATION MAPPING: item/visibility.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 4: Module System - Visibility
//   - Visibility modifiers: public, internal, private, protected
//   - Visibility inheritance
//   - Field visibility
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/types/visibility.cpp
//   Visibility checking
//
// KEY CONTENT TO MIGRATE:
//   VISIBILITY CHECKING:
//   1. Determine declared visibility of item
//   2. Check access from requesting context
//   3. Apply inheritance rules
//   4. Report visibility errors
//
//   VISIBILITY LEVELS:
//   - public: visible everywhere
//   - internal: visible within the assembly (default)
//   - private: visible only in declaring module
//   - protected: visible in declaring module and submodules
//
//   VISIBILITY RULES:
//   IsVisible(item, from_context):
//   - public: always true
//   - internal: same_assembly(item, from_context)
//   - protected: same_module_or_submodule(item, from_context)
//   - private: same_module(item, from_context)
//
//   FIELD VISIBILITY:
//   - Fields inherit item visibility by default
//   - May have explicit visibility override
//   - Private fields accessible only in declaring module
//
//   VISIBILITY COERCION:
//   - Cannot expose private type in public signature
//   - Return types must be at least as visible as procedure
//   - Parameter types must be at least as visible
//
//   DEFAULT VISIBILITY:
//   - Items: internal (assembly-wide)
//   - Fields: same as containing type
//   - Methods: same as containing type
//
// DEPENDENCIES:
//   - ModuleHierarchy for module relationships
//   - AssemblyContext for internal checks
//   - TypeVisibility for nested type checks
//
// SPEC RULES:
//   - Visibility semantics
//   - Visibility coercion rules
//
// RESULT:
//   bool indicating visibility
//   Diagnostics for visibility violations
//
// =============================================================================

