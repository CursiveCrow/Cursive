// =============================================================================
// MIGRATION MAPPING: load_project.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md Section 2.1 (lines 1030-1159)
//   - Project Load (Small-Step): Step-Parse, Step-Validate, Step-Asm-*
//   - Assembly Selection: Select-Only, Select-By-Name, Select-Err
//   - Assembly Build (Big-Step): BuildAssembly-Ok/Err-*
//   - Project Load (Big-Step): LoadProject-Ok/Err
//
// SOURCE FILE: cursive-bootstrap/src/01_project/load_project.cpp
//   - Lines 1-211 (entire file)
//
// =============================================================================
// CONTENT TO MIGRATE
// =============================================================================
//
// 1. MakeExternalDiag() helper (lines 17-23)
//    - PURPOSE: Create diagnostic without source span
//    - REFACTORING: Consolidate into core diagnostics module
//
// 2. EmitExternal() helper (lines 25-29)
//    - PURPOSE: Emit external diagnostic to stream
//
// 3. IsDirDefault() helper (lines 31-38)
//    - PURPOSE: Check if path is a directory (default implementation)
//    - DEPENDENCIES: std::filesystem::is_directory
//
// 4. BuildAssembly() (lines 40-96)
//    - PURPOSE: Build Assembly record from validated spec
//    - SPEC RULES:
//      * BuildAssembly-Ok (spec lines 1122-1125)
//      * BuildAssembly-Err-Resolve (spec lines 1127-1130)
//      * BuildAssembly-Err-Root (spec lines 1132-1135)
//      * BuildAssembly-Err-Modules (spec lines 1137-1140)
//      * WF-Source-Root / WF-Source-Root-Err (spec lines 1215-1223)
//      * ModuleList-Ok / ModuleList-Err (spec lines 1376-1384)
//    - DIAGNOSTICS:
//      * E-PRJ-0302: Source root not a directory
//      * E-PRJ-0304: Path resolution error
//    - DEPENDENCIES: deps.resolve, deps.is_dir, deps.modules, deps.outputs
//
// 5. SelectAssembly() (lines 98-120)
//    - PURPOSE: Select assembly from list by target name
//    - SPEC RULES:
//      * Select-Only (spec lines 1105-1108)
//      * Select-By-Name (spec lines 1110-1113)
//      * Select-Err (spec lines 1115-1118)
//    - DIAGNOSTICS: E-PRJ-0205
//
// 6. LoadProjectImpl() (lines 122-183)
//    - PURPOSE: Main project loading implementation
//    - SPEC RULES:
//      * Step-Parse / Step-Parse-Err (spec lines 1035-1043)
//      * Step-Validate / Step-Validate-Err (spec lines 1045-1053)
//      * Step-Asm-Init (spec lines 1078-1081)
//      * Step-Asm-Cons (spec lines 1083-1086)
//      * Step-Asm-Err (spec lines 1088-1091)
//      * Step-Asm-Done / Step-Asm-Done-Err (spec lines 1093-1101)
//      * LoadProject-Ok (spec lines 1144-1147)
//      * LoadProject-Err (spec lines 1149-1152)
//    - DEPENDENCIES: deps.parse, deps.validate, BuildAssembly, SelectAssembly
//
// 7. LoadProjectWithDeps() (lines 187-191)
//    - PURPOSE: Public entry point with dependency injection
//
// 8. LoadProject() overloads (lines 193-208)
//    - PURPOSE: Public entry points with default dependencies
//    - DEPENDENCIES: ParseManifest, ValidateManifest, core::Resolve,
//                    IsDirDefault, Modules, ComputeOutputPaths
//
// =============================================================================
// DEPENDENCIES
// =============================================================================
//
// Headers required:
//   - "cursive0/01_project/project.h" (types: Project, Assembly, LoadProjectResult, LoadProjectDeps)
//   - "cursive0/00_core/assert_spec.h" (SPEC_RULE macro)
//   - "cursive0/00_core/diagnostic_messages.h" (MakeDiagnostic)
//   - "cursive0/00_core/diagnostics.h" (DiagnosticStream, Emit, HasError)
//   - "cursive0/01_project/deterministic_order.h" (Fold, Utf8LexLess)
//   - "cursive0/01_project/module_discovery.h" (Modules, ModulesResult)
//   - "cursive0/01_project/outputs.h" (ComputeOutputPaths)
//   - <algorithm>
//   - <string_view>
//   - <filesystem>
//
// Types from header (project.h):
//   - Project { root, assemblies, assembly, source_root, outputs, modules }
//   - Assembly { name, kind, root, out_dir, emit_ir, source_root, outputs, modules }
//   - LoadProjectResult { std::optional<Project> project; DiagnosticStream diags; }
//   - LoadProjectDeps { parse, validate, resolve, is_dir, modules, outputs }
//
// =============================================================================
// REFACTORING NOTES
// =============================================================================
//
// 1. The LoadProjectDeps struct enables dependency injection for testing
//    RECOMMENDATION: Maintain this pattern for testability
//
// 2. Module sorting logic (lines 73-81) duplicates ordering logic
//    RECOMMENDATION: Extract to deterministic_order module
//
// 3. Consider adding progress callbacks for large projects
//
// 4. The spec defines small-step and big-step semantics for project loading
//    The implementation follows the big-step pattern with early returns on error
//
// =============================================================================
// SPEC RULE ANNOTATIONS (use SPEC_RULE macro)
// =============================================================================
//
// Line 48: SPEC_RULE("BuildAssembly-Err-Resolve");
// Line 55-56: SPEC_RULE("WF-Source-Root-Err"); SPEC_RULE("BuildAssembly-Err-Root");
// Line 60: SPEC_RULE("WF-Source-Root");
// Line 67-68: SPEC_RULE("ModuleList-Err"); SPEC_RULE("BuildAssembly-Err-Modules");
// Line 82: SPEC_RULE("ModuleList-Ok");
// Line 94: SPEC_RULE("BuildAssembly-Ok");
// Line 104: SPEC_RULE("Select-Only");
// Line 107: SPEC_RULE("Select-Err");
// Line 113: SPEC_RULE("Select-By-Name");
// Line 117: SPEC_RULE("Select-Err");
// Line 132-133: SPEC_RULE("Step-Parse-Err"); SPEC_RULE("LoadProject-Err");
// Line 136: SPEC_RULE("Step-Parse");
// Line 144-145: SPEC_RULE("Step-Validate-Err"); SPEC_RULE("LoadProject-Err");
// Line 148: SPEC_RULE("Step-Validate");
// Line 150: SPEC_RULE("Step-Asm-Init");
// Line 154: SPEC_RULE("Step-Asm-Cons");
// Line 157-158: SPEC_RULE("Step-Asm-Err"); SPEC_RULE("LoadProject-Err");
// Line 166-167: SPEC_RULE("Step-Asm-Done-Err"); SPEC_RULE("LoadProject-Err");
// Line 180-181: SPEC_RULE("Step-Asm-Done"); SPEC_RULE("LoadProject-Ok");
//
// =============================================================================

