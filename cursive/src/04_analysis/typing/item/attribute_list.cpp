// =============================================================================
// MIGRATION MAPPING: item/attribute_list.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section: Attributes
//   - Attribute syntax: [[attr]], [[attr(args)]]
//   - Complete attribute list in spec
//   - Attribute validation
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/types/attributes.cpp
//   Attribute processing
//
// KEY CONTENT TO MIGRATE:
//   ATTRIBUTE PROCESSING:
//   1. Parse attribute list
//   2. Validate each attribute
//   3. Check attribute applies to target
//   4. Extract attribute arguments
//   5. Register for later phases
//
//   VALID ATTRIBUTES:
//   - [[inline]], [[inline(always)]], [[inline(never)]], [[inline(hint)]]
//   - [[cold]]
//   - [[export]]
//   - [[no_mangle]]
//   - [[symbol("name")]]
//   - [[unwind]]
//   - [[layout(C)]]
//   - [[ffi_pass_by_value]]
//   - [[weak]]
//   - [[dynamic]]
//   - [[static_dispatch_only]]
//
//   ATTRIBUTE TARGETS:
//   - Procedures: inline, cold, export, no_mangle, symbol, unwind
//   - Types: layout(C), ffi_pass_by_value
//   - Contracts: dynamic
//   - Generic methods: static_dispatch_only
//
//   VALIDATION:
//   - Attribute must be known
//   - Attribute must apply to target kind
//   - Arguments must be valid for attribute
//   - Some attributes are mutually exclusive
//
//   EFFECT ON TYPING:
//   - [[dynamic]]: allows runtime contract verification
//   - [[layout(C)]]: enables FfiSafe for type
//   - [[static_dispatch_only]]: prevents dynamic dispatch
//
// DEPENDENCIES:
//   - AttributeRegistry for known attributes
//   - TargetKindCheck for applicability
//   - ArgumentValidation for attribute args
//
// SPEC RULES:
//   - Attribute syntax and semantics
//   - Target applicability rules
//
// RESULT:
//   Validated attribute list
//   Diagnostics for invalid attributes
//
// =============================================================================

