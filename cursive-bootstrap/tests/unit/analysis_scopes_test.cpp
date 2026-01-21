#include <cassert>
#include <string>

#include "cursive0/analysis/types/context.h"
#include "cursive0/analysis/resolve/scopes.h"

namespace {

using cursive0::analysis::EntityKind;
using cursive0::analysis::IdEq;
using cursive0::analysis::IdKeyOf;
using cursive0::analysis::KeywordKey;
using cursive0::analysis::ReservedCursive;
using cursive0::analysis::ReservedGen;
using cursive0::analysis::ReservedId;
using cursive0::analysis::ReservedModulePath;
using cursive0::analysis::UniverseBindings;
using cursive0::syntax::ModulePath;

}  // namespace

int main() {
  {
    const std::string decomposed = "e\xCC\x81";
    const std::string composed = "\xC3\xA9";
    assert(IdKeyOf(decomposed) == IdKeyOf(composed));
    assert(IdEq(decomposed, composed));
  }

  {
    assert(ReservedGen("gen_tmp"));
    assert(!ReservedGen("gen"));
    assert(ReservedCursive("cursive"));
    assert(!ReservedCursive("cursivex"));
    assert(ReservedId("gen_tmp"));
    assert(ReservedId("cursive"));
    assert(!ReservedId("user"));
  }

  {
    ModulePath p1 = {"cursive", "io"};
    ModulePath p2 = {"foo", "gen_tmp"};
    ModulePath p3 = {"foo", "bar"};
    assert(ReservedModulePath(p1));
    assert(ReservedModulePath(p2));
    assert(!ReservedModulePath(p3));
  }

  {
    assert(KeywordKey(IdKeyOf("let")));
    assert(!KeywordKey(IdKeyOf("not_keyword")));
  }

  {
    const auto universe = UniverseBindings();
    const auto i32_key = IdKeyOf("i32");
    const auto cursive_key = IdKeyOf("cursive");
    const auto it_type = universe.find(i32_key);
    assert(it_type != universe.end());
    assert(it_type->second.kind == EntityKind::Type);

    const auto it_module = universe.find(cursive_key);
    assert(it_module != universe.end());
    assert(it_module->second.kind == EntityKind::ModuleAlias);
    assert(it_module->second.origin_opt.has_value());
    assert(it_module->second.origin_opt->size() == 1);
    assert((*it_module->second.origin_opt)[0] == "cursive");
  }

  return 0;
}
