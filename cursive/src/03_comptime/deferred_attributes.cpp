// =============================================================================
// MIGRATION MAPPING: deferred_attributes.cpp
// =============================================================================
//
// SPEC REFERENCE:
//   C0updated.md §5.13.1 (lines 13918-13920)
//   "Deferred attributes. `reflect`, `derive`, `emit`, and `files` require compile-time
//    execution and are deferred in Cursive0 (Phase 2). Their use is rejected as
//    Unsupported-Construct."
//
//   C0updated.md §5.13.1 (lines 13861-13867)
//   R_spec attribute list including deferred attributes:
//     layout, inline, cold, static_dispatch_only, deprecated,
//     dynamic, stale_ok,
//     relaxed, acquire, release, acqrel, seqcst,
//     static, assume, trust,
//     symbol, library, no_mangle, unwind,
//     reflect, derive, emit, files,    <-- DEFERRED IN CURSIVE0
//     export, ffi_pass_by_value
//
//   C0updated.md §4 (lines 6650-6653)
//   "Phase 2: Compile-Time Execution (Deferred in Cursive0)"
//
// SOURCE FILE: NEW (deferred functionality)
//   No direct source in cursive-bootstrap. This file handles deferred
//   compile-time attributes that require Phase 2 execution.
//
// DEPENDENCIES:
//   - cursive/include/00_core/diagnostics.h (DiagnosticStream, Emit, Severity)
//   - cursive/include/00_core/diagnostic_codes.h (diagnostic code definitions)
//   - cursive/include/04_analysis/attributes/attribute_registry.h (AttributeKind)
//   - cursive/include/00_core/span.h (Span)
//
// PURPOSE:
//   This module handles attributes that require compile-time execution and are
//   therefore deferred in Cursive0. The deferred attributes are:
//
//   1. [[reflect]] - Runtime reflection metadata generation
//      Requires comptime to generate type metadata at compile time.
//
//   2. [[derive]] - Automatic implementation derivation
//      Requires comptime to synthesize method implementations.
//
//   3. [[emit]] - Code emission/generation
//      Requires comptime to generate arbitrary code.
//
//   4. [[files]] - File embedding
//      Requires comptime to read and embed external files.
//
//   All of these attributes are syntactically valid but semantically rejected
//   in Cursive0 with an Unsupported-Construct diagnostic.
//
// REFACTORING NOTES:
//   - Keep the deferred attribute list in sync with R_spec in the spec
//   - When Phase 2 is implemented, this module will be replaced by actual
//     attribute handlers
//   - Consider a registry pattern to allow gradual enablement of individual
//     deferred attributes
//
// IMPLEMENTATION REQUIREMENTS:
//   1. DeferredAttributes() -> std::set<std::string>
//      Returns the set of deferred attribute names: {reflect, derive, emit, files}
//
//   2. IsDeferredAttribute(name: std::string_view) -> bool
//      Returns true if the attribute name is in the deferred set.
//
//   3. CheckDeferredAttribute(attr, diags) -> void
//      Emits E-UNS diagnostic if attr is a deferred attribute.
//      The diagnostic should indicate that the attribute requires Phase 2.
//
//   4. ValidateDeferredAttributeUsage(attr_list, diags) -> void
//      Scans an attribute list and emits diagnostics for any deferred attributes.
//
// ATTRIBUTE TARGET COMPATIBILITY (from spec §5.13.1):
//   AttrTargets for deferred attributes are not specified in Cursive0 since
//   they are rejected before target checking. When enabled:
//   - reflect: typically applies to Record, Enum, Modal
//   - derive: typically applies to Record, Enum
//   - emit: typically applies to Module or Expression
//   - files: typically applies to Module or Binding
//
// DIAGNOSTIC CODES (from spec §8.7):
//   E-UNS-0101: Unsupported construct (generic)
//   Specific codes for deferred attributes may use:
//   E-UNS-01XX where XX is attribute-specific
//
// =============================================================================

// TODO: Implement deferred attribute handling
// This is a deferred feature in Cursive0 - all deferred attributes are rejected.

