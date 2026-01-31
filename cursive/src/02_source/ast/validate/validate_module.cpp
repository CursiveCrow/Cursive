// ===========================================================================
// MIGRATION MAPPING: validate_module.cpp
// ===========================================================================
//
// PURPOSE:
//   Validation and documentation attachment for module-level AST structures.
//   Handles ASTFile and ASTModule validation, doc comment attachment, and
//   module-level constraint checking.
//
// ===========================================================================
// SPEC REFERENCE: C:\Dev\Cursive\C0updated.md
// ===========================================================================
//   Section 3.3.11 - Documentation Association (line 2639)
//   Defines how doc comments are attached to AST items.
//
//   Rules:
//   - Module docs (//!) attach to module/file level
//   - Line docs (///) attach to following item
//   - Block docs (/** */) attach to following item
//   - Doc ordering is preserved
//
// ---------------------------------------------------------------------------
// SOURCE FILE: C:\Dev\Cursive\cursive-bootstrap\src\02_syntax\parser_docs.cpp
// ---------------------------------------------------------------------------
//   Lines 59-71: ModuleDocs function
//     - Filters DocComments with kind == ModuleDoc
//     - SPEC_RULE("Attach-Doc-Module")
//
//   Lines 73-106: AttachLineDocs function
//     - Attaches line docs to following AST items
//     - Iterates items and docs together
//     - Skips ErrorItems
//     - SPEC_RULE("Attach-Doc-Line")
//
//   Lines 13-16: ItemStartOffset helper
//     - Extracts start offset from any ASTItem variant
//     - Uses std::visit
//
//   Lines 18-20: IsErrorItem helper
//     - Checks if item is ErrorItem variant
//
//   Lines 22-55: SetItemDoc helper
//     - Sets doc field on item (handles each variant)
//
// ---------------------------------------------------------------------------
// DEPENDENCIES
// ---------------------------------------------------------------------------
//   Internal:
//   - ast/nodes/ast_module.h: ASTFile, ASTModule
//   - ast/nodes/ast_items.h: ASTItem, ErrorItem
//   - ast/nodes/ast_common.h: DocList
//   - validate_common.cpp: ValidationResult, ValidationContext
//   - validate_items.cpp: validate_item
//   - lexer: DocComment type
//   - 00_core/span.h: Span for locations
//
// ---------------------------------------------------------------------------
// REFACTORING NOTES
// ---------------------------------------------------------------------------
//   1. Doc attachment modifies the AST (not pure validation)
//   2. Module validation coordinates all item validation
//   3. Unsafe span collection enables safety analysis
//   4. File path validation is platform-dependent
//
// ===========================================================================

// TODO: Migrate implementation from source files listed above
//
// namespace cursive::ast::validate {
//
// // ---------------------------------------------------------------------------
// // TOP-LEVEL MODULE VALIDATION
// // ---------------------------------------------------------------------------
//
// /// Validates an ASTModule and all its contents.
// /// @param module Module to validate
// /// @param ctx Validation context
// ValidationResult validate_module(const ASTModule& module, ValidationContext& ctx);
//
// /// Validates an ASTFile and all its contents.
// /// @param file File to validate
// /// @param ctx Validation context
// ValidationResult validate_file(const ASTFile& file, ValidationContext& ctx);
//
// // ---------------------------------------------------------------------------
// // DOCUMENTATION ATTACHMENT
// // ---------------------------------------------------------------------------
//
// /// Attaches module-level documentation to file.
// /// Module docs use //! comment syntax.
// /// Filters DocComments with kind == ModuleDoc.
// /// SPEC_RULE("Attach-Doc-Module")
// /// @param file File to attach docs to (modified)
// /// @param docs All doc comments from parsing
// void attach_module_docs(ASTFile& file, const std::vector<DocComment>& docs);
//
// /// Attaches line documentation to items.
// /// Line docs (///) attach to the following item.
// /// Iterates items and docs together by position.
// /// Skips ErrorItems.
// /// SPEC_RULE("Attach-Doc-Line")
// /// @param items Items to attach docs to (modified)
// /// @param docs All doc comments from parsing
// void attach_line_docs(std::vector<ASTItem>& items, const std::vector<DocComment>& docs);
//
// // ---------------------------------------------------------------------------
// // DOCUMENTATION HELPERS
// // ---------------------------------------------------------------------------
//
// /// Extracts start offset from any ASTItem variant.
// /// Uses std::visit to handle all item types.
// /// @param item Item to get offset from
// /// @return Start offset of the item's span
// size_t item_start_offset(const ASTItem& item);
//
// /// Checks if an item is an ErrorItem.
// /// ErrorItems are skipped during doc attachment.
// /// @param item Item to check
// bool is_error_item(const ASTItem& item);
//
// /// Sets the doc field on an item.
// /// Handles each ASTItem variant.
// /// @param item Item to modify
// /// @param docs Documentation to attach
// void set_item_doc(ASTItem& item, DocList docs);
//
// /// Filters doc comments to get module docs only.
// /// @param docs All doc comments
// /// @return Only module-level docs (//!)
// std::vector<DocComment> filter_module_docs(const std::vector<DocComment>& docs);
//
// /// Filters doc comments to get item docs only.
// /// @param docs All doc comments
// /// @return Only item-level docs (/// and /** */)
// std::vector<DocComment> filter_item_docs(const std::vector<DocComment>& docs);
//
// // ---------------------------------------------------------------------------
// // MODULE STRUCTURE VALIDATION
// // ---------------------------------------------------------------------------
//
// /// Validates module path.
// /// @param path Module path to validate
// /// @param ctx Validation context
// bool is_valid_module_path(const Path& path, ValidationContext& ctx);
//
// /// Validates file path.
// /// @param path File path to validate
// /// @param ctx Validation context
// bool is_valid_file_path(const std::string& path, ValidationContext& ctx);
//
// // ---------------------------------------------------------------------------
// // UNSAFE SPAN COLLECTION
// // ---------------------------------------------------------------------------
//
// /// Collects all unsafe spans from a file.
// /// Used for safety analysis passes.
// /// @param file File to analyze
// /// @return List of spans where unsafe blocks appear
// std::vector<Span> collect_unsafe_spans(const ASTFile& file);
//
// /// Validates unsafe span consistency.
// /// Checks:
// /// - Spans are valid (start <= end)
// /// - Spans are within file bounds
// /// - No overlapping unsafe blocks
// /// @param spans Unsafe spans to validate
// /// @param ctx Validation context
// bool validate_unsafe_spans(const std::vector<Span>& spans, ValidationContext& ctx);
//
// // ---------------------------------------------------------------------------
// // ERROR ITEM HANDLING
// // ---------------------------------------------------------------------------
//
// /// Collects all error items from a file.
// /// Error items indicate parser errors that were recovered from.
// /// @param file File to analyze
// /// @return List of error items with their spans
// std::vector<std::pair<Span, ErrorItem>> collect_error_items(const ASTFile& file);
//
// /// Reports error items as diagnostics.
// /// @param errors Error items to report
// /// @param ctx Validation context
// void report_error_items(const std::vector<std::pair<Span, ErrorItem>>& errors, ValidationContext& ctx);
//
// } // namespace cursive::ast::validate

