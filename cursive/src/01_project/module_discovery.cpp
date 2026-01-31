// =============================================================================
// MIGRATION MAPPING: module_discovery.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md Section 2.4 (lines 1209-1327)
//   - 2.4. Module Discovery
//   - Module Discovery (Big-Step): Modules-Ok/Err
//   - Module Discovery (Small-Step): Disc-Start, Disc-Skip, Disc-Add, etc.
//   - Module Path computation
//   - Compilation Unit ordering
//
// SOURCE FILE: cursive-bootstrap/src/01_project/module_discovery.cpp
//   - Lines 1-337 (entire file)
//
// =============================================================================
// CONTENT TO MIGRATE
// =============================================================================
//
// 1. MakeExternalDiag() / EmitExternal() helpers (lines 23-35)
//    - PURPOSE: Emit diagnostics without source span
//    - REFACTORING: Consolidate with other files
//
// 2. HasCode() helper (lines 37-44)
//    - PURPOSE: Check if diagnostic stream contains specific code
//
// 3. DirListResult struct and CollectDirsRecursive() (lines 46-80)
//    - PURPOSE: Recursively collect all directories under root
//    - SPEC RULE: Dirs(S) = { d | is_dir(d) and relative(d, S) ok }
//    - SPEC REF: Lines 1211-1213
//    - DIAGNOSTIC: E-PRJ-0305 (DirSeq-Read-Err)
//
// 4. ModuleDirCheck struct and CheckModuleDir() (lines 82-123)
//    - PURPOSE: Check if directory contains .cursive files
//    - SPEC RULE: Module-Dir: exists f in Files(d) : FileExt(f) = ".cursive"
//    - SPEC REF: Lines 1225-1228
//    - DIAGNOSTIC: E-PRJ-0305
//
// 5. SplitModulePath() / JoinModulePath() helpers (lines 125-150)
//    - PURPOSE: Split/join module path on "::" delimiter
//    - REFACTORING: Duplicate with deterministic_order.cpp
//
// 6. ModulePathResult and ModulePathFor() (lines 152-184)
//    - PURPOSE: Compute module path from directory path
//    - SPEC RULES:
//      * Module-Path-Root (spec lines 1256-1259)
//      * Module-Path-Rel (spec lines 1261-1264)
//      * Module-Path-Rel-Fail (spec lines 1266-1269)
//    - DIAGNOSTIC: E-PRJ-0304 (Disc-Rel-Fail)
//
// 7. ValidateModulePath() (lines 186-202)
//    - PURPOSE: Validate module path components
//    - SPEC RULES:
//      * WF-Module-Path-Ok (spec lines 1273-1276)
//      * WF-Module-Path-Reserved (spec lines 1278-1281)
//      * WF-Module-Path-Ident-Err (spec lines 1283-1286)
//    - DIAGNOSTICS:
//      * E-MOD-1105: Reserved keyword in module path
//      * E-MOD-1106: Invalid identifier in module path
//    - DEPENDENCIES: IsIdentifier(), IsKeyword() from ident.h
//
// 8. CompilationUnit() (lines 206-261)
//    - PURPOSE: Get sorted list of .cursive files in module directory
//    - SPEC RULE: CompilationUnit(d) = sort_{<_file}(Files(d))
//    - SPEC REF: Line 1246
//    - SPEC RULE: CompilationUnit-Rel-Fail (spec lines 1248-1251)
//    - DIAGNOSTIC: E-PRJ-0305, E-PRJ-0303
//
// 9. Modules() (lines 263-334)
//    - PURPOSE: Main module discovery entry point
//    - SPEC RULES:
//      * Disc-Start (spec lines 1296-1298)
//      * Disc-Skip (spec lines 1300-1303)
//      * Disc-Add (spec lines 1305-1308)
//      * Disc-Collision (spec lines 1310-1313)
//      * Disc-Invalid-Component (spec lines 1315-1318)
//      * Disc-Rel-Fail (spec lines 1320-1323)
//      * Disc-Done (spec lines 1325-1327)
//      * Modules-Ok / Modules-Err (spec lines 1234-1242)
//      * WF-Module-Path-Collision (spec lines 1288-1291)
//    - DIAGNOSTICS:
//      * E-MOD-1104: Module path collision
//      * W-MOD-1101: Case-insensitive collision warning
//
// =============================================================================
// DEPENDENCIES
// =============================================================================
//
// Headers required:
//   - "cursive0/01_project/module_discovery.h" (types)
//   - "cursive0/00_core/assert_spec.h" (SPEC_RULE macro)
//   - "cursive0/00_core/diagnostic_messages.h" (MakeDiagnostic)
//   - "cursive0/00_core/diagnostics.h" (DiagnosticStream, Emit, HasError)
//   - "cursive0/00_core/path.h" (FileExt, Relative, PathComps)
//   - "cursive0/01_project/deterministic_order.h" (FileKey, KeyLess, Fold, DirSeqFrom)
//   - "cursive0/01_project/ident.h" (IsIdentifier, IsKeyword)
//   - <algorithm>
//   - <filesystem>
//   - <optional>
//   - <string>
//   - <string_view>
//   - <system_error>
//   - <unordered_map>
//   - <vector>
//
// Types from header (module_discovery.h):
//   - ModuleInfo { std::string path; std::filesystem::path dir; }
//   - ModulesResult { std::vector<ModuleInfo> modules; DiagnosticStream diags; }
//   - CompilationUnitResult { std::vector<std::filesystem::path> files; DiagnosticStream diags; }
//
// =============================================================================
// REFACTORING NOTES
// =============================================================================
//
// 1. SplitModulePath/JoinModulePath are duplicated in deterministic_order.cpp
//    RECOMMENDATION: Move to shared module_path utility
//
// 2. The discovery algorithm uses std::unordered_map<string, string> for
//    collision detection (folded path -> original path)
//
// 3. The spec defines both small-step and big-step semantics:
//    - Small-step: DiscState machine transitions
//    - Big-step: Modules-Ok/Err
//    The implementation follows the big-step pattern
//
// 4. Consider parallel directory enumeration for large projects
//
// =============================================================================
// SPEC RULE ANNOTATIONS (use SPEC_RULE macro)
// =============================================================================
//
// Lines 58, 65, 72, 93, 101, 109: SPEC_RULE("DirSeq-Read-Err");
// Line 167-168: SPEC_RULE("Module-Path-Rel-Fail"); SPEC_RULE("Disc-Rel-Fail");
// Line 173: SPEC_RULE("Module-Path-Root");
// Line 179: SPEC_RULE("Module-Path-Rel");
// Line 190: SPEC_RULE("WF-Module-Path-Ident-Err");
// Line 195: SPEC_RULE("WF-Module-Path-Reserved");
// Line 200: SPEC_RULE("WF-Module-Path-Ok");
// Line 211, 225, 233: SPEC_RULE("DirSeq-Read-Err");
// Line 245: SPEC_RULE("CompilationUnit-Rel-Fail");
// Line 267: SPEC_RULE("Disc-Start");
// Line 274, 288, 301, 306-307, 317, 327: SPEC_RULE("Modules-Err");
// Line 292: SPEC_RULE("Disc-Skip");
// Line 295: SPEC_RULE("Module-Dir");
// Line 305: SPEC_RULE("Disc-Invalid-Component");
// Line 313-314: SPEC_RULE("Disc-Collision"); SPEC_RULE("WF-Module-Path-Collision");
// Line 323: SPEC_RULE("Disc-Add");
// Line 331-332: SPEC_RULE("Disc-Done"); SPEC_RULE("Modules-Ok");
//
// =============================================================================

