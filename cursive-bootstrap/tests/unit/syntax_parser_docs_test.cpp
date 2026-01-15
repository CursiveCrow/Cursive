#include <cassert>
#include <string>
#include <vector>

#include "cursive0/core/assert_spec.h"
#include "cursive0/core/span.h"
#include "cursive0/syntax/ast.h"
#include "cursive0/syntax/parser.h"

namespace cursive0::syntax {
std::vector<DocComment> ModuleDocs(const std::vector<DocComment>* docs);
void AttachLineDocs(std::vector<ASTItem>& items,
                    const std::vector<DocComment>* docs);
}  // namespace cursive0::syntax

namespace {

using cursive0::core::Span;
using cursive0::syntax::ASTItem;
using cursive0::syntax::DocComment;
using cursive0::syntax::DocKind;
using cursive0::syntax::ErrorItem;
using cursive0::syntax::ModuleDocs;
using cursive0::syntax::ProcedureDecl;
using cursive0::syntax::StaticDecl;
using cursive0::syntax::AttachLineDocs;

static Span MakeSpan(std::size_t start, std::size_t end) {
  Span sp;
  sp.file = "docs.cursive";
  sp.start_offset = start;
  sp.end_offset = end;
  return sp;
}

static DocComment MakeDoc(DocKind kind,
                          std::string text,
                          std::size_t start,
                          std::size_t end) {
  DocComment doc;
  doc.kind = kind;
  doc.text = std::move(text);
  doc.span = MakeSpan(start, end);
  return doc;
}

}  // namespace

int main() {
  SPEC_COV("Attach-Doc-Line");
  SPEC_COV("Attach-Doc-Module");

  {
    std::vector<DocComment> docs;
    docs.push_back(MakeDoc(DocKind::ModuleDoc, "module a", 0, 1));
    docs.push_back(MakeDoc(DocKind::LineDoc, "first", 2, 5));
    docs.push_back(MakeDoc(DocKind::LineDoc, "second", 18, 19));
    docs.push_back(MakeDoc(DocKind::LineDoc, "third", 20, 21));
    docs.push_back(MakeDoc(DocKind::ModuleDoc, "module b", 16, 17));
    docs.push_back(MakeDoc(DocKind::LineDoc, "trailing", 40, 45));

    StaticDecl first{};
    first.span = MakeSpan(10, 15);
    first.doc = {};
    ProcedureDecl second{};
    second.span = MakeSpan(30, 35);
    second.doc = {};

    std::vector<ASTItem> items;
    items.emplace_back(first);
    items.emplace_back(second);

    AttachLineDocs(items, &docs);
    const auto module_docs = ModuleDocs(&docs);

    assert(module_docs.size() == 2);
    assert(module_docs[0].text == "module a");
    assert(module_docs[1].text == "module b");

    const auto& item0 = std::get<StaticDecl>(items[0]);
    assert(item0.doc.size() == 1);
    assert(item0.doc[0].text == "first");

    const auto& item1 = std::get<ProcedureDecl>(items[1]);
    assert(item1.doc.size() == 2);
    assert(item1.doc[0].text == "second");
    assert(item1.doc[1].text == "third");
  }

  {
    std::vector<DocComment> docs;
    docs.push_back(MakeDoc(DocKind::LineDoc, "drop", 5, 8));

    ErrorItem err{};
    err.span = MakeSpan(10, 11);
    ProcedureDecl ok{};
    ok.span = MakeSpan(20, 25);
    ok.doc = {};

    std::vector<ASTItem> items;
    items.emplace_back(err);
    items.emplace_back(ok);

    AttachLineDocs(items, &docs);

    const auto& proc = std::get<ProcedureDecl>(items[1]);
    assert(proc.doc.empty());
  }

  return 0;
}
