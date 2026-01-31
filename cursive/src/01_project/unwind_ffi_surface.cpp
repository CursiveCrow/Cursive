// =============================================================================
// MIGRATION MAPPING: unwind_ffi_surface.cpp (NEW FILE)
// =============================================================================
//
// SPEC REFERENCE: C0updated.md Section 2.7 (lines 1683-1701)
//   - 2.7. Unwind and FFI Surface
//   - FFI Boundary definition
//   - Unwind Modes
//   - Boundary Effects
//   - Safety requirements for extern calls
//
// SOURCE FILES: This is new functionality for Phase 0
//   - Partially related to extern handling in Phase 3/Phase 4
//   - No direct bootstrap source - implement from spec
//
// =============================================================================
// RATIONALE FOR NEW FILE
// =============================================================================
//
// Section 2.7 of the spec defines FFI and unwind semantics that affect:
// 1. How extern procedures are handled at the project level
// 2. How exported procedures are prepared for foreign callers
// 3. Unwind behavior configuration
//
// This is a build/project concern because it affects:
// - Symbol visibility decisions
// - Attribute processing
// - Linkage requirements
//
// =============================================================================
// CONTENT TO IMPLEMENT
// =============================================================================
//
// 1. FFI Boundary Detection
//    - SPEC RULE: FFIBoundary(proc) <=>
//        proc = ExternProcDecl(...) or
//        (proc = ProcedureDecl(...) and ExportAttr(proc) defined)
//    - SPEC REF: Lines 1687-1688
//
// 2. Unwind Mode Extraction
//    - SPEC RULE: UnwindMode(proc) = m iff UnwindAttr(proc) = m
//    - SPEC RULE: UnwindMode(proc) = "abort" iff UnwindAttr(proc) undefined
//    - SPEC REF: Lines 1690-1694
//    - Attribute format: [[unwind("abort")]] or [[unwind("catch")]]
//
// 3. FFI Safety Validation (Project-Level)
//    - SPEC RULE: Calls to extern procedures MUST occur within unsafe block
//    - SPEC REF: Line 1701
//    - NOTE: Full validation is in Phase 3, but project-level tooling
//      may need to identify extern declarations
//
// =============================================================================
// PROPOSED FUNCTIONS
// =============================================================================
//
// 1. bool IsFfiBoundary(const ProcedureDecl& proc)
//    - PURPOSE: Check if procedure crosses FFI boundary
//    - INPUT: Procedure declaration AST node
//    - OUTPUT: true if extern or has [[export]] attribute
//
// 2. std::string GetUnwindMode(const ProcedureDecl& proc)
//    - PURPOSE: Get unwind mode for FFI procedure
//    - INPUT: Procedure declaration with potential [[unwind]] attribute
//    - OUTPUT: "abort" (default) or "catch"
//
// 3. bool HasExportAttribute(const ProcedureDecl& proc)
//    - PURPOSE: Check for [[export]] attribute
//    - INPUT: Procedure declaration
//    - OUTPUT: true if [[export]] present
//
// 4. bool HasUnwindAttribute(const ProcedureDecl& proc, std::string* mode_out)
//    - PURPOSE: Check for [[unwind]] attribute and extract mode
//    - INPUT: Procedure declaration
//    - OUTPUT: true if [[unwind]] present, mode in mode_out
//
// 5. struct FfiSurfaceInfo { ... }
//    - PURPOSE: Collect FFI surface information for a module
//    - FIELDS:
//      * std::vector<ExternProcInfo> imports
//      * std::vector<ExportProcInfo> exports
//
// 6. FfiSurfaceInfo CollectFfiSurface(const ASTModule& module)
//    - PURPOSE: Collect all FFI declarations from a module
//    - INPUT: Parsed module AST
//    - OUTPUT: FFI surface information
//
// =============================================================================
// BOUNDARY EFFECTS (from spec)
// =============================================================================
//
// SPEC RULES (Lines 1696-1699):
//
// 1. If Cursive panic or foreign unwind attempts to cross FFI boundary
//    with UnwindMode(proc) = "abort", program MUST abort.
//
// 2. If UnwindMode(proc) = "catch":
//    - Imported procedures: foreign unwinds converted to Cursive panics
//    - Exported procedures: Cursive panics caught and converted to error indicator
//    - See Section 5.13.6.4 and Section 21.2 for details
//
// =============================================================================
// DEPENDENCIES
// =============================================================================
//
// Headers required:
//   - "cursive0/01_project/unwind_ffi_surface.h" (to be created)
//   - "cursive0/02_source/ast.h" (AST node types)
//   - <string>
//   - <string_view>
//   - <vector>
//   - <optional>
//
// Types needed:
//   - ProcedureDecl (from ast.h)
//   - ExternProcDecl (from ast.h)
//   - AttributeList (from ast.h)
//   - ASTModule (from ast.h)
//
// =============================================================================
// REFACTORING NOTES
// =============================================================================
//
// 1. This is new functionality for the refactored compiler
//    RECOMMENDATION: Implement incrementally as needed
//
// 2. The FFI surface analysis may be needed for:
//    - Link-time validation of runtime library compatibility
//    - Symbol export decisions
//    - ABI compatibility checking
//
// 3. The unwind mode affects code generation
//    RECOMMENDATION: Store in a project-level data structure
//
// 4. The spec references sections not yet implemented:
//    - Section 5.13.6.4: Attribute details
//    - Section 21.2: FFI boundary behavior
//    RECOMMENDATION: Add TODO markers for cross-references
//
// =============================================================================
// SPEC RULE ANNOTATIONS (to be added during implementation)
// =============================================================================
//
// No existing source to annotate - this is a new file.
// Implement with appropriate SPEC_RULE annotations.
//
// Suggested annotations:
//   SPEC_RULE("FFIBoundary");           // When checking if procedure is FFI
//   SPEC_RULE("UnwindMode-Default");    // When returning default "abort"
//   SPEC_RULE("UnwindMode-Explicit");   // When extracting from attribute
//   SPEC_RULE("Call-Extern-Unsafe-Err"); // When validating unsafe requirement
//
// =============================================================================

