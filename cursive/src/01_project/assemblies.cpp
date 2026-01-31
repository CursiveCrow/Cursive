// =============================================================================
// MIGRATION MAPPING: assemblies.cpp (NEW FILE)
// =============================================================================
//
// SPEC REFERENCE: C0updated.md Section 2.2 (lines 1161-1166)
//   - 2.2. Assemblies
//   - AssemblyKind definition
//   - Assembly record validation
//
// SOURCE FILES: Content extracted from multiple sources
//   - cursive-bootstrap/src/01_project/load_project.cpp (Assembly handling)
//   - cursive-bootstrap/src/01_project/project_validate.cpp (ValidatedAssembly)
//
// =============================================================================
// RATIONALE FOR NEW FILE
// =============================================================================
//
// This file consolidates assembly-related functionality that is currently
// scattered across load_project.cpp and project_validate.cpp. The spec
// defines assemblies as a distinct concept in Section 2.2, warranting
// a dedicated implementation file.
//
// =============================================================================
// CONTENT TO IMPLEMENT
// =============================================================================
//
// 1. AssemblyKind validation
//    - SPEC RULE: AssemblyKind = {`executable`, `library`}
//    - SPEC REF: Lines 748-749
//    - SPEC RULE: A_0.kind in AssemblyKind => A_0 : Assembly
//    - SPEC REF: Lines 1163-1165
//
// 2. Assembly record accessors
//    - SPEC RULE: Assemblies(P) = P.assemblies
//    - SPEC RULE: Assembly(P) = P.assembly
//    - SPEC RULE: AsmNames(P) = [A.name | A in Assemblies(P)]
//    - SPEC RULE: AsmByName(P, n) = A iff A in Assemblies(P) and A.name = n
//    - SPEC REF: Lines 756-759
//
// 3. Assembly record structure
//    - SPEC: Assembly = <name, kind, root, out_dir, emit_ir, source_root, outputs, modules>
//    - SPEC REF: Line 752
//
// =============================================================================
// CONTENT TO EXTRACT FROM EXISTING FILES
// =============================================================================
//
// From load_project.cpp:
//   - Assembly struct definition (currently in project.h)
//   - BuildAssembly() function creates Assembly instances (lines 40-96)
//   - Module list handling for assembly.modules field
//
// From project_validate.cpp:
//   - ValidatedAssembly struct (intermediate validation result)
//   - AssemblyKind validation (lines 215-219)
//
// =============================================================================
// PROPOSED FUNCTIONS
// =============================================================================
//
// 1. bool IsValidAssemblyKind(std::string_view kind)
//    - PURPOSE: Check if kind is "executable" or "library"
//    - SPEC RULE: k in AssemblyKind
//
// 2. bool IsExecutable(const Assembly& assembly)
//    - PURPOSE: Check if assembly is an executable
//    - SPEC RULE: Executable(P) <=> P.assembly.kind = `executable`
//    - SPEC REF: Line 194
//
// 3. bool IsLibrary(const Assembly& assembly)
//    - PURPOSE: Check if assembly is a library
//
// 4. std::vector<std::string> GetAssemblyNames(const Project& project)
//    - PURPOSE: Get list of assembly names
//    - SPEC RULE: AsmNames(P) = [A.name | A in Assemblies(P)]
//
// 5. std::optional<Assembly> GetAssemblyByName(const Project& project, std::string_view name)
//    - PURPOSE: Find assembly by name
//    - SPEC RULE: AsmByName(P, n) = A iff A.name = n
//
// =============================================================================
// DEPENDENCIES
// =============================================================================
//
// Headers required:
//   - "cursive0/01_project/assemblies.h" (to be created)
//   - "cursive0/01_project/project.h" (Project, Assembly types)
//   - <string>
//   - <string_view>
//   - <vector>
//   - <optional>
//
// Types needed:
//   - Assembly struct (from project.h)
//   - Project struct (from project.h)
//
// =============================================================================
// REFACTORING NOTES
// =============================================================================
//
// 1. This file consolidates assembly handling from multiple places
//    RECOMMENDATION: Move related code here during migration
//
// 2. The spec treats assemblies as first-class entities
//    RECOMMENDATION: Ensure type definitions match spec structure
//
// 3. Consider adding validation functions for Assembly records
//
// 4. The IsExecutable() helper is currently duplicated in outputs.cpp
//    RECOMMENDATION: Move here as the canonical location
//
// =============================================================================
// SPEC RULE ANNOTATIONS (to be added during implementation)
// =============================================================================
//
// No existing source to annotate - this is a new file.
// Implement with appropriate SPEC_RULE annotations.
//
// =============================================================================

