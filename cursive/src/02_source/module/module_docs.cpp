// ===========================================================================
// MIGRATION MAPPING: module_docs.cpp
// ===========================================================================
//
// PURPOSE:
//   Documentation comment handling for modules: extracting module-level docs,
//   attaching line docs to declarations, and managing doc comment association
//   with AST items.
//
// ---------------------------------------------------------------------------
// SPEC REFERENCE: C:\Dev\Cursive\C0updated.md
// ---------------------------------------------------------------------------
//   Section 3.2.1 - Doc Comment definitions (referenced at line 2644)
//
//   Section 3.3.11 - Documentation Association (referenced at line 2639)
//   DocAssociationRef = {"3.3.11"}
//
//   Key definitions from Section 3.2.1 (lines 2017-2036):
//   - DocComment = (kind, text, span) (2019)
//   - DocKind = {LineDoc, ModuleDoc} (2021)
//   - StripLeadingSpace(s) = s[1..|s|) if |s| > 0 and At(s, 0) = U+0020, else s (2022-2024)
//   - DocBody(T, i, j) = StripLeadingSpace(T[i+3..j)) (2025)
//   - DocMarker(T, i) = LineDoc if T[i..i+3] = "///", ModuleDoc if T[i..i+3] = "//!", else bottom (2026-2029)
//
//   Documentation attachment rules (implied from parser):
//   - Module docs (//!) apply to the entire module/file
//   - Line docs (///) attach to the immediately following item
//   - Multiple consecutive line docs merge into a DocList
//   - Docs cannot "skip over" items or blank lines
//
// ---------------------------------------------------------------------------
// SOURCE FILE: C:\Dev\Cursive\cursive-bootstrap\src\02_syntax\parser_docs.cpp
// ---------------------------------------------------------------------------
//   Full file (109 lines)
//
//   Lines 1-8: Includes
//     - parser.h, utility headers
//
//   Lines 10-17: Internal helpers (namespace)
//
//   Lines 13-16: ItemStartOffset function
//     - Extracts start_offset from ASTItem variant using std::visit
//     - Returns node.span.start_offset
//
//   Lines 18-20: IsErrorItem function
//     - Checks if item is ErrorItem using std::holds_alternative
//
//   Lines 22-55: SetItemDoc function
//     - Sets doc field on an ASTItem
//     - Handles each variant type:
//       * UsingDecl (23-26)
//       * StaticDecl (27-30)
//       * ProcedureDecl (31-34)
//       * RecordDecl (35-38)
//       * EnumDecl (39-42)
//       * ModalDecl (43-46)
//       * ClassDecl (47-50)
//       * TypeAliasDecl (51-54)
//     - Note: ImportDecl, ExternBlock, ErrorItem don't get docs set here
//
//   Lines 59-71: ModuleDocs function
//     - Input: const vector<DocComment>* docs
//     - Output: vector<DocComment> (filtered)
//     - Filters docs where kind == DocKind::ModuleDoc
//     - SPEC_RULE("Attach-Doc-Module")
//
//   Lines 73-106: AttachLineDocs function
//     - Input: vector<ASTItem>& items (mutable), const vector<DocComment>* docs
//     - Output: items modified with doc fields set
//     - Algorithm:
//       1. Create item_docs vector same size as items (line 79)
//       2. Track current item_index (line 80)
//       3. For each doc with kind == LineDoc (lines 82-98):
//          a. Skip items whose start_offset < doc.span.end_offset (lines 86-89)
//          b. Skip if past end of items (lines 90-91)
//          c. Skip if current item is ErrorItem (lines 93-95)
//          d. Add doc to current item's doc list (line 97)
//          e. SPEC_RULE("Attach-Doc-Line")
//       4. Apply collected docs to items (lines 100-105)
//     - Key insight: docs and items are both sorted by position
//
// ---------------------------------------------------------------------------
// SOURCE FILE: C:\Dev\Cursive\cursive-bootstrap\src\02_syntax\lexer_ws.cpp
// ---------------------------------------------------------------------------
//   Contains doc comment scanning during lexical analysis.
//   DocComment creation happens during tokenization.
//
//   (Specific line numbers depend on file content - lexer creates
//   DocComment records with kind, text, and span during comment scanning)
//
// ---------------------------------------------------------------------------
// DEPENDENCIES
// ---------------------------------------------------------------------------
//   Required types:
//   - ASTItem variant from ast_nodes.h
//   - DocComment { kind, text, span } from lexer
//   - DocKind { LineDoc, ModuleDoc } enum from lexer
//   - DocList = vector<DocComment>
//
//   Required helpers:
//   - std::visit for variant traversal
//   - std::holds_alternative for type checking
//
// ---------------------------------------------------------------------------
// DOCUMENTATION ATTACHMENT SEMANTICS
// ---------------------------------------------------------------------------
//
//   Module Documentation (//!):
//   ```cursive
//   //! This is module-level documentation.
//   //! It describes the entire module.
//
//   public procedure foo() -> () { }
//   ```
//   The //! comments become module_doc, not attached to foo.
//
//   Line Documentation (///):
//   ```cursive
//   /// This documents the procedure.
//   /// It can span multiple lines.
//   public procedure foo() -> () { }
//   ```
//   The /// comments attach to foo as its doc field.
//
//   Interleaved Comments:
//   ```cursive
//   /// Doc for foo
//   public procedure foo() -> () { }
//
//   /// Doc for bar
//   public procedure bar() -> () { }
//   ```
//   Each /// block attaches to its following item.
//
//   Orphaned Comments:
//   ```cursive
//   /// This comment has no following item
//   ```
//   Comments at end of file with no following item are discarded.
//
// ---------------------------------------------------------------------------
// REFACTORING NOTES
// ---------------------------------------------------------------------------
//   1. The current algorithm assumes docs and items are position-sorted.
//      Add assertions to verify this invariant.
//
//   2. Consider handling orphaned docs:
//      - Emit warning for docs with no following item
//      - Store orphaned docs separately for tooling
//
//   3. The SetItemDoc function uses explicit variant checks.
//      Consider using a visitor or adding a common interface.
//
//   4. Doc merging for multi-line comments:
//      - Current approach creates DocList
//      - Consider merging text into single string for display
//
//   5. Support for doc comment attributes:
//      - Parse @param, @return, @throws annotations
//      - Store structured doc info for IDE support
//
//   6. Integration with documentation generator:
//      - Export docs in standard formats (markdown, HTML)
//      - Support cross-references between items
//
//   7. Handle doc comments on nested items:
//      - Fields in records
//      - Variants in enums
//      - States in modals
//      - Methods (already handled via doc_opt)
//
//   8. The current implementation modifies items in-place.
//      Consider returning new items with docs for immutability.
//
// ===========================================================================

// TODO: Migrate implementation from source files listed above
//
// Suggested public API:
//
// namespace cursive::module {
//
// // Module doc extraction
// // Filters docs to only module-level (//!) comments
// vector<DocComment> extract_module_docs(const vector<DocComment>& all_docs);
//
// // Line doc attachment
// // Attaches /// comments to their following items
// // Modifies items in-place
// void attach_line_docs(vector<ASTItem>& items, const vector<DocComment>& all_docs);
//
// // Combined doc processing
// // Returns module docs and attaches line docs to items
// vector<DocComment> process_docs(vector<ASTItem>& items, const vector<DocComment>& all_docs);
//
// // Doc extraction helpers
// bool is_module_doc(const DocComment& doc);
// bool is_line_doc(const DocComment& doc);
//
// // Item doc accessors (read-only)
// optional<DocList> get_item_docs(const ASTItem& item);
//
// // Doc text utilities
// string strip_leading_space(string_view text);
// string merge_doc_list(const DocList& docs);
//
// } // namespace cursive::module

