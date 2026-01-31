// ===========================================================================
// MIGRATION MAPPING: build_module.cpp
// ===========================================================================
//
// PURPOSE:
//   Factory functions for constructing module-level AST structures.
//   Provides builders for ASTFile and ASTModule defined in ast_module.h.
//
// ===========================================================================
// SPEC REFERENCE: C:\Dev\Cursive\C0updated.md
// ===========================================================================
//   Section 3.3.9 - ParseModule rules
//   Defines parsing rules for complete source files and modules.
//
//   Module structures (from spec Section 3.3.2):
//   - ASTFile: Represents a single source file
//     - path: File path
//     - items: Top-level declarations
//     - module_doc: Module-level documentation
//     - unsafe_spans: Spans of unsafe blocks (for analysis)
//
//   - ASTModule: Represents a complete module (possibly multiple files)
//     - path: Module path (e.g., mylib::networking)
//     - items: All declarations in the module
//     - module_doc: Module documentation
//     - unsafe_spans: Collected unsafe spans from all files
//
// ---------------------------------------------------------------------------
// SOURCE FILE: C:\Dev\Cursive\cursive-bootstrap\src\02_syntax\parser.cpp
// ---------------------------------------------------------------------------
//   Module construction:
//   - parse_file(): Parses a single source file into ASTFile
//   - Module aggregation from multiple files
//   - Module documentation extraction
//   - Unsafe span collection for analysis passes
//
// ---------------------------------------------------------------------------
// DEPENDENCIES
// ---------------------------------------------------------------------------
//   Internal:
//   - ast/nodes/ast_fwd.h: Path
//   - ast/nodes/ast_common.h: DocList, UnsafeSpanSet
//   - ast/nodes/ast_items.h: ASTItem
//   - ast/nodes/ast_module.h: ASTFile, ASTModule
//   - build_common.cpp: span utilities
//   - 00_core/span.h: Span
//
// ---------------------------------------------------------------------------
// REFACTORING NOTES
// ---------------------------------------------------------------------------
//   1. ASTFile is the result of parsing a single source file
//   2. ASTModule may be constructed from multiple ASTFiles
//   3. Module documentation (//!) is attached to module, not items
//   4. Unsafe span collection enables safety analysis passes
//
// ===========================================================================

// TODO: Migrate implementation from source files listed above
//
// namespace cursive::ast {
//
// // ---------------------------------------------------------------------------
// // FILE CONSTRUCTION
// // ---------------------------------------------------------------------------
//
// /// Creates an ASTFile from parsed content.
// /// @param path Source file path
// /// @param items Top-level declarations
// /// @param module_doc Module-level documentation comments (//!)
// /// @param unsafe_spans Spans of all unsafe blocks in the file
// ASTFile make_ast_file(
//     string path,
//     vector<ASTItem> items,
//     DocList module_doc,
//     vector<Span> unsafe_spans
// );
//
// // ---------------------------------------------------------------------------
// // MODULE CONSTRUCTION
// // ---------------------------------------------------------------------------
//
// /// Creates an ASTModule from module content.
// /// May aggregate content from multiple source files.
// /// @param path Module path (e.g., mylib::networking)
// /// @param items All declarations in the module
// /// @param module_doc Module documentation
// /// @param unsafe_spans Unsafe spans from all files in module
// ASTModule make_ast_module(
//     Path path,
//     vector<ASTItem> items,
//     DocList module_doc,
//     vector<UnsafeSpanSet> unsafe_spans
// );
//
// // ---------------------------------------------------------------------------
// // MODULE AGGREGATION HELPERS
// // ---------------------------------------------------------------------------
//
// /// Merges multiple ASTFiles into a single ASTModule.
// /// @param module_path Module path
// /// @param files Source files belonging to the module
// ASTModule merge_files_to_module(Path module_path, vector<ASTFile> files);
//
// /// Extracts module documentation from file items.
// /// Module documentation uses //! comments at file start.
// /// @param file Source file
// DocList extract_module_doc(const ASTFile& file);
//
// /// Collects all unsafe spans from a file.
// /// Used for safety analysis passes.
// /// @param file Source file
// vector<Span> collect_unsafe_spans(const ASTFile& file);
//
// } // namespace cursive::ast

