#include "cursive0/syntax/parser.h"

#include <utility>
#include <variant>
#include <vector>

#include "cursive0/core/assert_spec.h"

namespace cursive0::syntax {

namespace {

std::size_t ItemStartOffset(const ASTItem& item) {
  return std::visit([](const auto& node) { return node.span.start_offset; },
                    item);
}

bool IsErrorItem(const ASTItem& item) {
  return std::holds_alternative<ErrorItem>(item);
}

void SetItemDoc(ASTItem& item, DocList docs) {
  if (auto* decl = std::get_if<UsingDecl>(&item)) {
    decl->doc = std::move(docs);
    return;
  }
  if (auto* decl = std::get_if<StaticDecl>(&item)) {
    decl->doc = std::move(docs);
    return;
  }
  if (auto* decl = std::get_if<ProcedureDecl>(&item)) {
    decl->doc = std::move(docs);
    return;
  }
  if (auto* decl = std::get_if<RecordDecl>(&item)) {
    decl->doc = std::move(docs);
    return;
  }
  if (auto* decl = std::get_if<EnumDecl>(&item)) {
    decl->doc = std::move(docs);
    return;
  }
  if (auto* decl = std::get_if<ModalDecl>(&item)) {
    decl->doc = std::move(docs);
    return;
  }
  if (auto* decl = std::get_if<ClassDecl>(&item)) {
    decl->doc = std::move(docs);
    return;
  }
  if (auto* decl = std::get_if<TypeAliasDecl>(&item)) {
    decl->doc = std::move(docs);
    return;
  }
}

}  // namespace

std::vector<DocComment> ModuleDocs(const std::vector<DocComment>* docs) {
  std::vector<DocComment> out;
  if (!docs) {
    return out;
  }
  for (const auto& doc : *docs) {
    if (doc.kind == DocKind::ModuleDoc) {
      SPEC_RULE("Attach-Doc-Module");
      out.push_back(doc);
    }
  }
  return out;
}

void AttachLineDocs(std::vector<ASTItem>& items,
                    const std::vector<DocComment>* docs) {
  if (!docs || items.empty()) {
    return;
  }

  std::vector<DocList> item_docs(items.size());
  std::size_t item_index = 0;

  for (const auto& doc : *docs) {
    if (doc.kind != DocKind::LineDoc) {
      continue;
    }
    while (item_index < items.size() &&
           ItemStartOffset(items[item_index]) < doc.span.end_offset) {
      item_index++;
    }
    if (item_index >= items.size()) {
      continue;
    }
    if (IsErrorItem(items[item_index])) {
      continue;
    }
    SPEC_RULE("Attach-Doc-Line");
    item_docs[item_index].push_back(doc);
  }

  for (std::size_t i = 0; i < items.size(); ++i) {
    if (item_docs[i].empty()) {
      continue;
    }
    SetItemDoc(items[i], std::move(item_docs[i]));
  }
}

}  // namespace cursive0::syntax
