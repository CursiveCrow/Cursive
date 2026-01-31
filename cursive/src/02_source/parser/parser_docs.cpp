// =============================================================================
// MIGRATION MAPPING: parser_docs.cpp
// =============================================================================
//
// DESTINATION: cursive/src/02_source/parser/parser_docs.cpp
// PURPOSE: Documentation comment association with AST items
//
// =============================================================================
// SPEC REFERENCES
// =============================================================================
//
// C0updated.md Section 3.3.11 (Lines 6433-6450): Doc Comment Association (Phase 1)
//
// **DocSeq and ItemSeq (lines 6435-6436)**:
//   DocSeq(D) = D
//   ItemSeq(Items) = Items
//
// **Attach-Doc-Line (lines 6438-6443)**:
//   d.kind = LineDoc    Items = [i_1, ..., i_k]    j = min{ t | d.span.end_offset <= i_t.span.start_offset }
//   ----------------------------------------
//   AttachDoc(d, i_j)
//
//   LineDocTarget(d, Items) = i_j <=> AttachDoc(d, i_j)
//   LineDocsFor(i, D, Items) = [d in D | d.kind = LineDoc && LineDocTarget(d, Items) = i]
//
// **Attach-Doc-Module (lines 6445-6448)**:
//   d.kind = ModuleDoc
//   ----------------------------------------
//   AttachModuleDoc(d)
//
//   ModuleDocs(D) = [d in D | d.kind = ModuleDoc]
//
// C0updated.md Section 3.2.1: Documentation Comments
//   DocKind = {LineDoc, ModuleDoc}
//   - LineDoc: /// comments that document the following item
//   - ModuleDoc: //! comments that document the containing module
//
// =============================================================================
// SOURCE FILES
// =============================================================================
//
// PRIMARY SOURCE: cursive-bootstrap/src/02_syntax/parser_docs.cpp
//   Lines 1-108: Complete file
//
// FUNCTIONS TO MIGRATE:
//
// 1. Anonymous namespace helpers (lines 11-56):
//
//    a. ItemStartOffset (lines 13-15):
//       Gets the start offset of an ASTItem using std::visit
//
//       Source:
//         std::size_t ItemStartOffset(const ASTItem& item) {
//           return std::visit([](const auto& node) { return node.span.start_offset; }, item);
//         }
//
//    b. IsErrorItem (lines 17-19):
//       Checks if item is an ErrorItem
//
//       Source:
//         bool IsErrorItem(const ASTItem& item) {
//           return std::holds_alternative<ErrorItem>(item);
//         }
//
//    c. SetItemDoc (lines 21-55):
//       Attaches doc list to an item (type-specific visitor)
//
//       Source:
//         void SetItemDoc(ASTItem& item, DocList docs) {
//           if (auto* decl = std::get_if<UsingDecl>(&item)) {
//             decl->doc = std::move(docs);
//             return;
//           }
//           if (auto* decl = std::get_if<StaticDecl>(&item)) {
//             decl->doc = std::move(docs);
//             return;
//           }
//           if (auto* decl = std::get_if<ProcedureDecl>(&item)) {
//             decl->doc = std::move(docs);
//             return;
//           }
//           if (auto* decl = std::get_if<RecordDecl>(&item)) {
//             decl->doc = std::move(docs);
//             return;
//           }
//           if (auto* decl = std::get_if<EnumDecl>(&item)) {
//             decl->doc = std::move(docs);
//             return;
//           }
//           if (auto* decl = std::get_if<ModalDecl>(&item)) {
//             decl->doc = std::move(docs);
//             return;
//           }
//           if (auto* decl = std::get_if<ClassDecl>(&item)) {
//             decl->doc = std::move(docs);
//             return;
//           }
//           if (auto* decl = std::get_if<TypeAliasDecl>(&item)) {
//             decl->doc = std::move(docs);
//             return;
//           }
//         }
//
// 2. ModuleDocs (lines 59-71):
//    SPEC: Attach-Doc-Module (line 66), ModuleDocs predicate (line 6450)
//    Extracts module documentation comments from doc stream
//
//    Source:
//      std::vector<DocComment> ModuleDocs(const std::vector<DocComment>* docs) {
//        std::vector<DocComment> out;
//        if (!docs) { return out; }
//        for (const auto& doc : *docs) {
//          if (doc.kind == DocKind::ModuleDoc) {
//            SPEC_RULE("Attach-Doc-Module");
//            out.push_back(doc);
//          }
//        }
//        return out;
//      }
//
// 3. AttachLineDocs (lines 73-106):
//    SPEC: Attach-Doc-Line (line 96)
//    Associates line documentation comments with their target items
//
//    Source:
//      void AttachLineDocs(std::vector<ASTItem>& items,
//                          const std::vector<DocComment>* docs) {
//        if (!docs || items.empty()) { return; }
//        std::vector<DocList> item_docs(items.size());
//        std::size_t item_index = 0;
//        for (const auto& doc : *docs) {
//          if (doc.kind != DocKind::LineDoc) { continue; }
//          while (item_index < items.size() &&
//                 ItemStartOffset(items[item_index]) < doc.span.end_offset) {
//            item_index++;
//          }
//          if (item_index >= items.size()) { continue; }
//          if (IsErrorItem(items[item_index])) { continue; }
//          SPEC_RULE("Attach-Doc-Line");
//          item_docs[item_index].push_back(doc);
//        }
//        for (std::size_t i = 0; i < items.size(); ++i) {
//          if (item_docs[i].empty()) { continue; }
//          SetItemDoc(items[i], std::move(item_docs[i]));
//        }
//      }
//
// =============================================================================
// DEPENDENCIES
// =============================================================================
//
// REQUIRED HEADERS:
//   - cursive/include/02_source/parser/parser.hpp
//   - cursive/include/02_source/ast/ast.hpp
//   - cursive/include/02_source/ast/doc_comment.hpp
//   - cursive/include/00_core/assert_spec.hpp
//   - <vector>
//   - <variant>
//   - <utility>
//
// TYPES REQUIRED:
//   - ASTItem (std::variant of all item types)
//   - DocComment, DocKind, DocList
//   - UsingDecl, StaticDecl, ProcedureDecl, RecordDecl, EnumDecl, ModalDecl, ClassDecl, TypeAliasDecl
//   - ErrorItem
//
// =============================================================================
// REFACTORING NOTES
// =============================================================================
//
// 1. SPEC RULE MARKERS:
//    All SPEC_RULE calls must be preserved:
//    - "Attach-Doc-Module" (line 66)
//    - "Attach-Doc-Line" (line 96)
//
// 2. VARIANT VISITATION:
//    SetItemDoc uses a chain of std::get_if calls. Consider using std::visit
//    with an overloaded visitor for cleaner code:
//
//      std::visit(overloaded{
//        [&](UsingDecl& d) { d.doc = std::move(docs); },
//        [&](StaticDecl& d) { d.doc = std::move(docs); },
//        // ... etc
//        [](auto&) { /* no doc field */ }
//      }, item);
//
// 3. DOC ASSOCIATION ALGORITHM:
//    The algorithm in AttachLineDocs is O(n + m) where n = items, m = docs:
//    - It scans docs in order
//    - For each LineDoc, finds first item starting after doc ends
//    - Skips ErrorItem nodes (they can't receive docs)
//
// 4. ERROR ITEMS:
//    ErrorItem nodes are skipped during doc association. This prevents
//    documentation from being attached to error recovery nodes.
//
// 5. DOC KINDS:
//    - LineDoc (///): Documents the following item
//    - ModuleDoc (//!): Documents the containing module
//    Module docs are collected separately and stored in ASTFile.module_doc.
//
// 6. MISSING ITEM TYPES:
//    SetItemDoc doesn't handle ImportDecl or ExternBlock. Consider whether
//    these should also receive documentation.
//
// 7. FORWARD DECLARATION:
//    ModuleDocs and AttachLineDocs are forward-declared in parser.cpp
//    (lines 17-19 of source). Ensure declarations match in header.
//
// 8. NAMESPACE:
//    Source uses cursive0::syntax. Update to target namespace.
//
// =============================================================================
