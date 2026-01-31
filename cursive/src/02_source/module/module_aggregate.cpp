// ===========================================================================
// MIGRATION MAPPING: module_aggregate.cpp
// ===========================================================================
//
// PURPOSE:
//   Module aggregation: loading, parsing, and combining source files into
//   modules. Implements the ParseModule and ParseModules judgments that
//   collect all files in a compilation unit and aggregate them into an
//   ASTModule structure.
//
// ---------------------------------------------------------------------------
// SPEC REFERENCE: C:\Dev\Cursive\C0updated.md
// ---------------------------------------------------------------------------
//   Section 3.4 - Module Aggregation
//   Lines 6520-6648
//
//   Key definitions:
//   - Lines 6522-6531: Inputs, Outputs, and Invariants
//     * ModuleAggInputs(P) = (P.modules, P.source_root, CompilationUnit sets) (6524)
//     * ModuleAggOutputs(P) = (ParseModule results, NameMaps, G, InitOrder, InitPlan) (6525)
//     * ModuleMap(P, p) = M iff ParseModules(P) => Ms and M in Ms and M.path = p (6526)
//     * PathOfModule(p), StringOfPath definitions (6527-6528)
//     * NameCollectAfterParse (6529)
//
//   - Lines 6533-6548: Module Directory Computation
//     * DirOf-Root rule (6537-6540): When p = A, DirOf(p, S) = S
//     * DirOf-Rel rule (6542-6545): When p = c1::...::cn, DirOf(p, S) = S/c1/.../cn
//     * A = P.assembly.name (6547)
//
//   - Lines 6549-6597: ParseModule (Big-Step)
//     * U = CompilationUnit(DirOf(p, S)) (6550)
//     * ReadBytes-Ok, ReadBytes-Err rules (6554-6562)
//     * ParseModule-Ok rule (6566-6569): Aggregates F_i.items and F_i.module_doc
//     * ParseModule-Err-Read rule (6571-6574)
//     * ParseModule-Err-Load rule (6576-6579)
//     * LoadSource Short-Circuit (6581-6582): Don't continue if LoadSource fails
//     * ParseModule-Err-Unit rule (6584-6587)
//     * ParseModule-Err-Parse rule (6589-6592)
//     * ParseFileBestEffort, ParseFileOk, ParseFileDiag definitions (6594-6597)
//
//   - Lines 6599-6634: ParseModule (Small-Step)
//     * ModState = {ModStart, ModScan, ModDone, Error} (6600)
//     * Mod-Start rule (6602-6605)
//     * Mod-Start-Err-Unit rule (6607-6610)
//     * Mod-Scan rule (6612-6615): Process one file
//     * Mod-Scan-Err-Read, Mod-Scan-Err-Load, Mod-Scan-Err-Parse rules (6617-6630)
//     * Mod-Done rule (6632-6634): Final aggregation
//
//   - Lines 6636-6648: ParseModules (Big-Step)
//     * P.modules = [p_1, ..., p_k] (6638)
//     * ParseModules-Ok rule (6640-6643): All modules parsed successfully
//     * ParseModules-Err rule (6645-6648): Any module fails
//
// ---------------------------------------------------------------------------
// SOURCE FILE: C:\Dev\Cursive\cursive-bootstrap\src\02_syntax\parse_modules.cpp
// ---------------------------------------------------------------------------
//   Lines 20-49: ReadBytesDefault function
//     - Opens file in binary mode
//     - Reads entire file contents
//     - SPEC_RULE("ReadBytes-Ok"), SPEC_RULE("ReadBytes-Err")
//     - Returns ReadBytesResult with bytes or diagnostics
//
//   Lines 51-58: AppendDiags helper
//     - Appends diagnostics from one stream to another
//
//   Lines 60-68: HasDiagCode helper
//     - Checks if specific diagnostic code exists in stream
//
//   Lines 70-83: SplitModulePath function
//     - Splits "a::b::c" into ["a", "b", "c"]
//
//   Lines 85-99: DirOf function
//     - Implements DirOf-Root and DirOf-Rel rules
//     - Converts module path to filesystem directory
//     - SPEC_RULE("DirOf-Root"), SPEC_RULE("DirOf-Rel")
//
//   Lines 101-108: DefaultDeps function
//     - Creates default ParseModuleDeps with standard functions
//     - compilation_unit, read_bytes, load_source, parse_file
//
//   Lines 112-232: ParseModuleWithDeps function
//     - Main module parsing implementation
//     - Lines 117-122: Debug logging
//     - Lines 124-135: CompilationUnit lookup with error handling
//       * SPEC_RULE("Mod-Start"), SPEC_RULE("Mod-Start-Err-Unit")
//     - Lines 140-221: File iteration loop
//       * Lines 142-148: ReadBytes with Mod-Scan-Err-Read
//       * Lines 149-156: LoadSource with Mod-Scan-Err-Load
//       * Lines 158-166: Optional source inspection
//       * Lines 168-208: ParseFile with error handling
//         - C0 subset checking (177-182)
//         - Diagnostic deduplication (184-202)
//       * Lines 210-220: Item aggregation
//         - CheckMethodContext validation
//         - SPEC_RULE("Mod-Scan")
//     - Lines 223-231: Final module construction
//       * SPEC_RULE("Mod-Done"), SPEC_RULE("ParseModule-Ok")
//
//   Lines 234-262: ParseModulesWithDeps function
//     - Iterates all modules in project
//     - Collects parsed modules
//     - SPEC_RULE("ParseModules-Ok"), SPEC_RULE("ParseModules-Err")
//     - SPEC_RULE("Phase1-Complete"), SPEC_RULE("Phase1-Declarations"), SPEC_RULE("Phase1-Forward-Refs")
//
//   Lines 265-277: ParseModule/ParseModules convenience wrappers
//     - Use default dependencies
//
// ---------------------------------------------------------------------------
// DEPENDENCIES
// ---------------------------------------------------------------------------
//   Required types:
//   - ASTModule, ASTFile, ASTItem from ast_nodes.h
//   - SourceFile from source_text.h
//   - DiagnosticStream from diagnostics.h
//   - filesystem::path for file operations
//
//   Required functions:
//   - CompilationUnit(dir) - returns list of source files in directory
//   - ReadBytes(path) - reads file contents
//   - LoadSource(path, bytes) - creates SourceFile from bytes
//   - ParseFile(source) - parses SourceFile into ASTFile
//
//   External dependencies:
//   - std::ifstream for file reading
//   - std::filesystem for path operations
//
// ---------------------------------------------------------------------------
// REFACTORING NOTES
// ---------------------------------------------------------------------------
//   1. Dependency injection is already well-designed with ParseModuleDeps.
//      Keep this pattern for testability.
//
//   2. Consider async file loading:
//      - Files in a compilation unit are independent
//      - Could use std::async for parallel loading
//      - Aggregate diagnostics at end
//
//   3. Error handling improvements:
//      - Accumulate all errors instead of failing on first
//      - Continue parsing remaining files after one fails
//      - Report all errors at end for better user experience
//
//   4. Caching layer:
//      - Cache parsed ASTFiles by path + modification time
//      - Reuse cached AST for unchanged files
//      - Significant speedup for incremental compilation
//
//   5. The diagnostic deduplication logic (lines 184-202) is complex.
//      Consider extracting to a dedicated deduplication module.
//
//   6. CompilationUnit discovery is external (project.h).
//      Ensure clear interface for locating source files.
//
//   7. Consider progress reporting for large projects:
//      - Callback for each file processed
//      - Enable progress bars in CLI/IDE
//
//   8. Module path validation:
//      - Check for reserved prefixes (cursive::, gen_)
//      - Validate path components are valid identifiers
//
// ===========================================================================

// TODO: Migrate implementation from source files listed above
//
// Suggested public API:
//
// namespace cursive::module {
//
// // File reading
// struct ReadBytesResult {
//   optional<vector<uint8_t>> bytes;
//   DiagnosticStream diags;
// };
// ReadBytesResult read_bytes(const fs::path& path);
//
// // Module path utilities
// vector<string> split_module_path(string_view path);
// fs::path dir_of(string_view module_path, const fs::path& source_root, string_view assembly_name);
//
// // Dependency injection for testing
// struct ParseModuleDeps {
//   function<CompilationUnitResult(const fs::path&)> compilation_unit;
//   function<ReadBytesResult(const fs::path&)> read_bytes;
//   function<SourceLoadResult(string_view, const vector<uint8_t>&)> load_source;
//   function<ParseFileResult(const SourceFile&)> parse_file;
//   function<InspectResult(const SourceFile&)> inspect_source;  // optional
// };
//
// // Single module parsing
// struct ParseModuleResult {
//   optional<ASTModule> module;
//   DiagnosticStream diags;
//   bool subset_ok = true;
// };
// ParseModuleResult parse_module(string_view module_path, const fs::path& source_root, string_view assembly_name);
// ParseModuleResult parse_module_with_deps(string_view module_path, const fs::path& source_root, string_view assembly_name, const ParseModuleDeps& deps);
//
// // Multiple module parsing
// struct ParseModulesResult {
//   vector<ASTModule> modules;
//   DiagnosticStream diags;
//   bool subset_ok = true;
// };
// ParseModulesResult parse_modules(const vector<ModuleInfo>& modules, const fs::path& source_root, string_view assembly_name);
// ParseModulesResult parse_modules_with_deps(const vector<ModuleInfo>& modules, const fs::path& source_root, string_view assembly_name, const ParseModuleDeps& deps);
//
// } // namespace cursive::module

