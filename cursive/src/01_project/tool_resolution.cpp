// =============================================================================
// MIGRATION MAPPING: tool_resolution.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md Section 2.6 (lines 1651-1671)
//   - 2.6. Tool Resolution and IR Assembly Inputs
//   - SearchDirs(P) definition
//   - ResolveTool-Ok / ResolveTool-Err-Linker / ResolveTool-Err-IR rules
//
// SOURCE FILE: cursive-bootstrap/src/01_project/tool_resolution.cpp
//   - Lines 1-347 (entire file)
//
// =============================================================================
// CONTENT TO MIGRATE
// =============================================================================
//
// 1. Helper functions (lines 13-92)
//    - GetEnv() (lines 15-21)
//      PURPOSE: Get environment variable value
//
//    - PathSeparator() (lines 23-29)
//      PURPOSE: Get path list separator (';' on Windows, ':' on POSIX)
//
//    - SplitPathList() (lines 31-45)
//      PURPOSE: Split PATH-style list into directory paths
//
//    - IsRepoLLVM() (lines 47-51)
//      PURPOSE: Check if LLVM is present in repo at llvm/llvm-21.1.8-x86_64/bin
//      SPEC RULE: RepoLLVM(P) predicate
//      SPEC REF: Line 1655
//
//    - EndsWithExe() (lines 53-62)
//      PURPOSE: Check if filename ends with ".exe"
//
//    - ToolCandidates() (lines 64-75)
//      PURPOSE: Generate tool name variants (with/without .exe on Windows)
//
//    - AppendUniquePaths() (lines 77-91)
//      PURPOSE: Append paths avoiding duplicates
//
// 2. Windows-specific Visual Studio linker discovery (lines 93-284, #ifdef _WIN32)
//    - ParseVersionParts() (lines 94-115)
//      PURPOSE: Parse version string like "14.37.12345" into [14, 37, 12345]
//
//    - CompareVersionParts() (lines 117-131)
//      PURPOSE: Compare version tuples
//
//    - LinkCandidate struct (lines 133-137)
//      PURPOSE: Store candidate linker path with version and preference
//
//    - IsBetterCandidate() (lines 139-146)
//      PURPOSE: Compare candidates (prefer newer version, then preference)
//
//    - AddLinkCandidate() (lines 148-156)
//      PURPOSE: Add candidate if file exists
//
//    - VisualStudioRoots() (lines 158-179)
//      PURPOSE: Get Visual Studio installation root directories
//      Checks ProgramFiles and ProgramFiles(x86)
//
//    - FindLatestMsvcLinker() (lines 181-283)
//      PURPOSE: Find latest MSVC link.exe by scanning Visual Studio installations
//      Search order:
//        1. VCToolsInstallDir environment variable
//        2. Visual Studio installations under Program Files
//      Prefers: Hostx64/x64 > Hostx64/x86 > Hostx86/x64 > Hostx86/x86
//
// 3. SearchDirs() (lines 288-303)
//    - PURPOSE: Get search directories for tool resolution
//    - SPEC RULE:
//      SearchDirs(P) =
//        [Env(`C0_LLVM_BIN`)]  if Env(`C0_LLVM_BIN`) != empty
//        [P.root/`llvm/llvm-21.1.8-x86_64/bin`]  if RepoLLVM(P)
//        PATHDirs  otherwise
//    - SPEC REF: Lines 1653-1656
//
// 4. ResolveTool() (lines 305-344)
//    - PURPOSE: Resolve tool path from search directories
//    - SPEC RULES:
//      * ResolveTool-Ok (spec lines 1658-1661)
//      * ResolveTool-Err-Linker (spec lines 1663-1666)
//      * ResolveTool-Err-IR (spec lines 1668-1671)
//    - Special handling for "link" tool on Windows:
//      Appends PATH directories and falls back to FindLatestMsvcLinker()
//
// =============================================================================
// DEPENDENCIES
// =============================================================================
//
// Headers required:
//   - "cursive0/01_project/tool_resolution.h"
//   - "cursive0/00_core/assert_spec.h" (SPEC_RULE macro)
//   - "cursive0/00_core/host_primitives.h" (HostPrimFail)
//   - "cursive0/01_project/project.h" (Project)
//   - <algorithm>
//   - <cstdlib>
//   - <string>
//   - <vector>
//   - <optional>
//   - <filesystem>
//   - <system_error>
//
// Types from header (tool_resolution.h):
//   - No custom types, just function declarations
//
// =============================================================================
// REFACTORING NOTES
// =============================================================================
//
// 1. The Windows Visual Studio linker discovery is complex but necessary
//    for users who don't have lld-link in PATH
//    RECOMMENDATION: Keep this as-is for Windows compatibility
//
// 2. The search order (C0_LLVM_BIN > repo LLVM > PATH) is intentional:
//    - Allows explicit override via environment variable
//    - Supports vendored LLVM in repository
//    - Falls back to system PATH
//
// 3. The FindLatestMsvcLinker() function scans Visual Studio installations
//    RECOMMENDATION: Consider caching the result for repeated calls
//
// 4. Tool candidates include both with and without .exe extension on Windows
//    This handles cases where the extension may or may not be present
//
// =============================================================================
// SPEC RULE ANNOTATIONS (use SPEC_RULE macro)
// =============================================================================
//
// Line 322: SPEC_RULE("ResolveTool-Ok");
// Line 331: SPEC_RULE("ResolveTool-Ok");
// Line 337: HostPrimFail call
// Line 339: SPEC_RULE("ResolveTool-Err-Linker");
// Line 341: SPEC_RULE("ResolveTool-Err-IR");
//
// =============================================================================

