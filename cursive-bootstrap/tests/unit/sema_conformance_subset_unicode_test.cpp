#include <cassert>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "cursive0/core/source_load.h"
#include "cursive0/core/span.h"
#include "cursive0/sema/conformance.h"
#include "cursive0/syntax/ast.h"

namespace {

cursive0::core::SourceFile LoadSourceChecked(std::string_view text) {
  std::vector<std::uint8_t> bytes(text.begin(), text.end());
  const auto loaded = cursive0::core::LoadSource("unicode.cursive", bytes);
  assert(loaded.source.has_value());
  return *loaded.source;
}

}  // namespace

int main() {
  {
    const std::string content = "let shared\xCE\xA9 = 1\n";
    const auto source = LoadSourceChecked(content);
    const auto perms = cursive0::sema::CheckC0SubsetPermSyntax(source);
    assert(perms.subset_ok);
    assert(perms.diags.empty());
  }

  {
    const std::string content = "let comptime\xCE\xB1 = 1\n";
    const auto source = LoadSourceChecked(content);
    const auto uns = cursive0::sema::CheckC0UnsupportedConstructs(source);
    assert(uns.subset_ok);
    assert(uns.diags.empty());
  }

  {
    const std::string content = "let x: shared i32 = 0\n";
    const auto source = LoadSourceChecked(content);
    const auto perms = cursive0::sema::CheckC0SubsetPermSyntax(source);
    assert(!perms.subset_ok);
    assert(!perms.diags.empty());
  }

  {
    const std::string content = "~%\n";
    const auto source = LoadSourceChecked(content);
    const auto perms = cursive0::sema::CheckC0SubsetPermSyntax(source);
    assert(!perms.subset_ok);
    assert(!perms.diags.empty());
  }

  {
    const std::string content = "comptime { }\n";
    const auto source = LoadSourceChecked(content);
    const auto uns = cursive0::sema::CheckC0UnsupportedConstructs(source);
    assert(!uns.subset_ok);
    assert(!uns.diags.empty());
  }

  {
    const std::string content = "where\n";
    const auto source = LoadSourceChecked(content);
    cursive0::syntax::ASTFile ast;
    ast.path = source.path;
    cursive0::syntax::ErrorItem item;
    item.span = cursive0::core::SpanOf(source, 0, source.byte_len);
    ast.items = {item};
    ast.module_doc = {};
    ast.unsafe_spans = {};
    const auto uns = cursive0::sema::CheckC0UnsupportedFormsAst(ast, source);
    assert(!uns.subset_ok);
    assert(!uns.diags.empty());
  }

  return 0;
}
