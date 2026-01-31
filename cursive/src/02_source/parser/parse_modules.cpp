// =============================================================================
// MIGRATION MAPPING: parse_modules.cpp
// =============================================================================
//
// DESTINATION: cursive/src/02_source/parser/parse_modules.cpp
// PURPOSE: Module-level parsing orchestration (ParseModule, ParseModules)
//
// =============================================================================
// SPEC REFERENCES
// =============================================================================
//
// C0updated.md Section 3.4.1 (Lines 6522-6531): Module Aggregation Inputs/Outputs
//
// **ModuleAggInputs (line 6524)**:
//   ModuleAggInputs(P) = <P.modules, P.source_root, { CompilationUnit(DirOf(p, P.source_root)) | p in P.modules }>
//
// **ModuleAggOutputs (line 6525)**:
//   ModuleAggOutputs(P) = <{ ParseModule(p, P.source_root) | p in P.modules }, { NameMap(P, p) | p in P.modules }, G, InitOrder, InitPlan>
//
// C0updated.md Section 3.4.2 (Lines 6533-6582): Module Aggregation (ParseModule)
//
// **DirOf-Root (lines 6537-6540)**:
//   p = A
//   ----------------------------------------
//   DirOf(p, S) = S
//
// **DirOf-Rel (lines 6542-6545)**:
//   p = c_1 :: ... :: c_n    n >= 1
//   ----------------------------------------
//   DirOf(p, S) = S / c_1 / ... / c_n
//
// **ReadBytes-Ok (lines 6554-6557)**:
//   read_ok(f) = B
//   ----------------------------------------
//   ReadBytes(f) => B
//
// **ReadBytes-Err (lines 6559-6562)**:
//   read_ok(f) error    c = Code(ReadBytes-Err)
//   ----------------------------------------
//   ReadBytes(f) error c
//
// **ParseModule-Ok (lines 6566-6569)**:
//   forall i, ReadBytes(f_i) => B_i    LoadSource(f_i, B_i) => S_i    ParseFile(S_i) => F_i
//   ----------------------------------------
//   ParseModule(p, S) => <p, F_1.items ++ ... ++ F_n.items, F_1.module_doc ++ ... ++ F_n.module_doc>
//
// **ParseModule-Err-Read (lines 6571-6574)**:
//   exists i, ReadBytes(f_i) error c
//   ----------------------------------------
//   ParseModule(p, S) error c
//
// **ParseModule-Err-Load (lines 6576-6579)**:
//   exists i, ReadBytes(f_i) => B_i    LoadSource(f_i, B_i) error c
//   ----------------------------------------
//   ParseModule(p, S) error c
//
// =============================================================================
// SOURCE FILES
// =============================================================================
//
// PRIMARY SOURCE: cursive-bootstrap/src/02_syntax/parse_modules.cpp
//   Lines 1-279: Complete file
//
// FUNCTIONS TO MIGRATE:
//
// 1. ReadBytesDefault (lines 20-49):
//    SPEC: ReadBytes-Ok (line 46), ReadBytes-Err (lines 24, 36)
//    Default implementation using std::ifstream
//
//    Source:
//      ReadBytesResult ReadBytesDefault(const std::filesystem::path& path) {
//        ReadBytesResult result;
//        std::ifstream in(path, std::ios::binary);
//        if (!in) {
//          SPEC_RULE("ReadBytes-Err");
//          core::HostPrimFail(core::HostPrim::ReadBytes, true);
//          if (auto diag = core::MakeDiagnostic("E-SRC-0102")) {
//            diag->span.reset();
//            result.diags = core::Emit(result.diags, *diag);
//          }
//          return result;
//        }
//        std::string buffer((std::istreambuf_iterator<char>(in)),
//                           std::istreambuf_iterator<char>());
//        if (!in && !in.eof()) {
//          SPEC_RULE("ReadBytes-Err");
//          core::HostPrimFail(core::HostPrim::ReadBytes, true);
//          if (auto diag = core::MakeDiagnostic("E-SRC-0102")) {
//            diag->span.reset();
//            result.diags = core::Emit(result.diags, *diag);
//          }
//          return result;
//        }
//        std::vector<std::uint8_t> bytes(buffer.begin(), buffer.end());
//        SPEC_RULE("ReadBytes-Ok");
//        result.bytes = std::move(bytes);
//        return result;
//      }
//
// 2. Anonymous namespace helpers (lines 51-109):
//
//    a. AppendDiags (lines 53-58):
//       Merges diagnostic streams (duplicate of parser.cpp helper)
//
//    b. HasDiagCode (lines 60-68):
//       Checks if diagnostic stream contains a specific code
//
//    c. SplitModulePath (lines 70-83):
//       Splits "a::b::c" into ["a", "b", "c"]
//
//       Source:
//         std::vector<std::string> SplitModulePath(std::string_view path) {
//           std::vector<std::string> parts;
//           std::size_t start = 0;
//           while (start <= path.size()) {
//             const std::size_t pos = path.find("::", start);
//             if (pos == std::string_view::npos) {
//               parts.emplace_back(path.substr(start));
//               break;
//             }
//             parts.emplace_back(path.substr(start, pos - start));
//             start = pos + 2;
//           }
//           return parts;
//         }
//
//    d. DirOf (lines 85-99):
//       SPEC: DirOf-Root (line 89), DirOf-Rel (line 92)
//       Computes directory path for a module
//
//       Source:
//         std::filesystem::path DirOf(std::string_view module_path,
//                                     const std::filesystem::path& source_root,
//                                     std::string_view assembly_name) {
//           if (module_path == assembly_name) {
//             SPEC_RULE("DirOf-Root");
//             return source_root;
//           }
//           SPEC_RULE("DirOf-Rel");
//           const auto comps = SplitModulePath(module_path);
//           std::filesystem::path dir = source_root;
//           for (const auto& comp : comps) {
//             dir /= comp;
//           }
//           return dir;
//         }
//
//    e. DefaultDeps (lines 101-108):
//       Creates default ParseModuleDeps with standard functions
//
// 3. ParseModuleWithDeps (lines 112-232):
//    SPEC: Mod-Start (line 124), Mod-Scan (line 212), Mod-Done (line 223)
//    SPEC: ParseModule-Ok (line 230), ParseModule-Err-* variants
//    Main module parsing with dependency injection
//
//    Key sections:
//    - Lines 124-135: Module directory and compilation unit lookup
//    - Lines 137-139: Initialize result containers
//    - Lines 140-221: File loop (read, load, inspect, parse each file)
//    - Lines 223-231: Assemble ASTModule result
//
// 4. ParseModulesWithDeps (lines 234-263):
//    SPEC: ParseModules-Ok (line 257), ParseModules-Err (line 251)
//    Parses multiple modules in sequence
//
// 5. ParseModule (lines 265-270):
//    Public entry point using default dependencies
//
// 6. ParseModules (lines 272-277):
//    Public entry point for multiple modules using default dependencies
//
// =============================================================================
// HEADER FILE: cursive-bootstrap/include/cursive0/02_syntax/parse_modules.h
// =============================================================================
//
// TYPES (lines 17-48):
//
// ReadBytesResult:
//   struct ReadBytesResult {
//     std::optional<std::vector<std::uint8_t>> bytes;
//     core::DiagnosticStream diags;
//   };
//
// InspectResult:
//   struct InspectResult {
//     core::DiagnosticStream diags;
//     bool subset_ok = true;
//   };
//
// ParseModuleDeps (dependency injection struct):
//   struct ParseModuleDeps {
//     project::CompilationUnitResult (*compilation_unit)(const std::filesystem::path&);
//     ReadBytesResult (*read_bytes)(const std::filesystem::path&);
//     core::SourceLoadResult (*load_source)(std::string_view, const std::vector<std::uint8_t>&);
//     syntax::ParseFileResult (*parse_file)(const core::SourceFile&);
//     InspectResult (*inspect_source)(const core::SourceFile&) = nullptr;
//   };
//
// ParseModuleResult:
//   struct ParseModuleResult {
//     std::optional<syntax::ASTModule> module;
//     core::DiagnosticStream diags;
//     bool subset_ok = true;
//   };
//
// ParseModulesResult:
//   struct ParseModulesResult {
//     std::optional<std::vector<syntax::ASTModule>> modules;
//     core::DiagnosticStream diags;
//     bool subset_ok = true;
//   };
//
// =============================================================================
// DEPENDENCIES
// =============================================================================
//
// REQUIRED HEADERS:
//   - cursive/include/02_source/parser/parse_modules.hpp
//   - cursive/include/02_source/parser/parser.hpp
//   - cursive/include/01_project/module_discovery.hpp
//   - cursive/include/00_core/diagnostics.hpp
//   - cursive/include/00_core/source_load.hpp
//   - cursive/include/00_core/host_primitives.hpp
//   - <filesystem>
//   - <fstream>
//   - <vector>
//   - <string>
//   - <string_view>
//   - <optional>
//   - <cstdint>
//   - <cstdlib>
//   - <iostream>
//
// REQUIRED FUNCTIONS:
//   - project::CompilationUnit (01_project/module_discovery.cpp)
//   - core::LoadSource (00_core/source_load.cpp)
//   - syntax::ParseFile (parser.cpp)
//   - syntax::CheckMethodContext (keyword_policy.cpp or similar)
//   - analysis::CheckC0UnsupportedFormsAst (03_analysis/types/conformance.cpp)
//   - core::HostPrimFail (00_core/host_primitives.cpp)
//   - core::MakeDiagnostic (00_core/diagnostic_messages.cpp)
//   - core::Emit, core::HasError (00_core/diagnostics.cpp)
//
// TYPES REQUIRED:
//   - ASTModule, ASTItem, DocComment, UnsafeSpanSet
//   - project::ModuleInfo, project::CompilationUnitResult
//   - core::SourceFile, core::SourceLoadResult
//   - core::DiagnosticStream
//
// =============================================================================
// REFACTORING NOTES
// =============================================================================
//
// 1. SPEC RULE MARKERS:
//    All SPEC_RULE calls must be preserved:
//    - "ReadBytes-Err" (lines 24, 36)
//    - "ReadBytes-Ok" (line 46)
//    - "DirOf-Root" (line 89)
//    - "DirOf-Rel" (line 92)
//    - "Mod-Start" (line 124)
//    - "Mod-Start-Err-Unit" (line 132)
//    - "ParseModule-Err-Unit" (line 133)
//    - "Mod-Scan-Err-Read" (line 145)
//    - "ParseModule-Err-Read" (line 146)
//    - "Mod-Scan-Err-Load" (line 153)
//    - "ParseModule-Err-Load" (line 154)
//    - "Mod-Scan-Err-Parse" (line 205)
//    - "ParseModule-Err-Parse" (line 206)
//    - "Mod-Scan" (line 212)
//    - "Mod-Done" (line 223)
//    - "ParseModule-Ok" (line 230)
//    - "ParseModules-Err" (line 251)
//    - "ParseModules-Ok" (line 257)
//    - "Phase1-Complete" (line 258)
//    - "Phase1-Declarations" (line 259)
//    - "Phase1-Forward-Refs" (line 260)
//
// 2. NAMESPACE:
//    Source uses cursive0::frontend. This may need to be adjusted in the
//    refactored codebase.
//
// 3. DEPENDENCY INJECTION:
//    ParseModuleDeps allows testing with mock implementations. This pattern
//    should be preserved for testability.
//
// 4. DEBUG OUTPUT:
//    Uses CURSIVE0_DEBUG_PHASES and CURSIVE0_DEBUG_PARSE environment variables.
//    Consider consolidating into a debug configuration system.
//
// 5. SUBSET CHECKING:
//    The subset_ok field tracks whether the parsed code conforms to Cursive0.
//    This involves calling CheckC0UnsupportedFormsAst which is in analysis.
//
// 6. DIAGNOSTIC SUPPRESSION:
//    Lines 184-202 filter duplicate diagnostics based on parse-time errors.
//    This prevents cascading of related errors.
//
// 7. UNSAFE SPANS:
//    UnsafeSpanSet is collected per-file and aggregated into the module.
//    This tracks `unsafe { }` block locations for later analysis.
//
// 8. APP_DIAGS DUPLICATION:
//    AppendDiags is duplicated from parser.cpp. Should be in a shared utility.
//
// 9. FILE LOOP STRUCTURE:
//    The file processing loop (lines 140-221) could be extracted into a
//    helper function for clarity:
//      ProcessModuleFile(path, deps, result) -> optional<FileData>
//
// 10. HOST PRIMITIVE TRACKING:
//     HostPrimFail is called for ReadBytes failures. This is for conformance
//     testing to track which host primitives are exercised.
//
// =============================================================================
