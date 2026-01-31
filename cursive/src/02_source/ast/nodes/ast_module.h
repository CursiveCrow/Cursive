// ===========================================================================
// MIGRATION MAPPING: ast_module.h
// ===========================================================================
//
// PURPOSE:
//   Module and file-level AST structures. Contains ASTModule and ASTFile
//   definitions that represent complete parsed compilation units, along
//   with supporting types like UnsafeSpanSet.
//
// ---------------------------------------------------------------------------
// SPEC REFERENCE: C:\Dev\Cursive\C0updated.md
// ---------------------------------------------------------------------------
//   Section 3.3.2.2 - Module and File Structures (Lines 2655-2669)
//
//   ASTModule = (path: ModulePath, items: [ASTItem], module_doc: DocList)
//                                                       (Line 2659)
//   ASTFile = (path: string, items: [ASTItem], module_doc: DocList)
//                                                       (Line 2666)
//
//   Additional fields (implementation extensions):
//   - unsafe_spans: tracks locations of unsafe blocks for verification
//
// ---------------------------------------------------------------------------
// SOURCE FILE: C:\Dev\Cursive\cursive-bootstrap\include\cursive0\02_syntax\ast.h
// ---------------------------------------------------------------------------
//
// 1. UNSAFE SPAN TRACKING (Lines 1287-1290)
//    ─────────────────────────────────────────────────────────────────────────
//    struct UnsafeSpanSet {
//      string path;           // File path
//      vector<Span> spans;    // Locations of unsafe blocks
//    };
//
// 2. AST MODULE (Lines 1292-1297)
//    ─────────────────────────────────────────────────────────────────────────
//    struct ASTModule {
//      Path path;                        // Module path (e.g., ["mylib", "utils"])
//      vector<ASTItem> items;            // Top-level declarations
//      DocList module_doc;               // //! module documentation
//      vector<UnsafeSpanSet> unsafe_spans; // Unsafe block locations per file
//    };
//
// 3. AST FILE (Lines 1299-1304)
//    ─────────────────────────────────────────────────────────────────────────
//    struct ASTFile {
//      string path;                      // File system path
//      vector<ASTItem> items;            // Top-level declarations
//      DocList module_doc;               // //! module documentation
//      vector<Span> unsafe_spans;        // Unsafe block locations
//    };
//
// ---------------------------------------------------------------------------
// DEPENDENCIES
// ---------------------------------------------------------------------------
//   From ast_fwd.h:
//     - Path, ModulePath
//     - DocList
//
//   From ast_items.h:
//     - ASTItem variant
//
//   From 00_core/span.h:
//     - core::Span
//
// ---------------------------------------------------------------------------
// USAGE CONTEXT
// ---------------------------------------------------------------------------
//   ASTFile represents a single source file after parsing:
//   - path: the file system path to the source
//   - items: parsed declarations in order of appearance
//   - module_doc: accumulated //! comments at file start
//   - unsafe_spans: locations of unsafe { } blocks for tracking
//
//   ASTModule represents a logical module (possibly spanning files):
//   - path: the module path in the namespace hierarchy
//   - items: merged declarations from all files in the module
//   - module_doc: merged module documentation
//   - unsafe_spans: unsafe locations grouped by source file
//
// ---------------------------------------------------------------------------
// REFACTORING NOTES
// ---------------------------------------------------------------------------
//   1. ASTFile is produced by the parser for each source file.
//      ASTModule is produced by the module loader that aggregates files.
//
//   2. The unsafe_spans field is used by:
//      - The C0 conformance checker (to warn about unsafe usage)
//      - The safety analyzer (to track unsafe context)
//      - The documentation generator (to annotate unsafe code)
//
//   3. Consider adding more file-level metadata:
//      - Import graph edges (dependencies)
//      - Parse diagnostics count
//      - File modification time (for incremental compilation)
//
//   4. ASTModule could have additional fields:
//      - submodules: [ASTModule] for nested modules
//      - exports: visibility-filtered item list
//      - dependencies: list of imported modules
//
//   5. The path representation differs:
//      - ASTFile uses string (file system path)
//      - ASTModule uses Path (module namespace path)
//      Consider unifying or making the distinction clearer.
//
//   6. For incremental compilation, consider:
//      - Splitting ASTFile into parsed structure + source reference
//      - Adding hash/fingerprint for change detection
//      - Storing parse tree alongside AST for error recovery
//
// ===========================================================================

// TODO: Migrate module definitions from ast.h lines 1287-1306

