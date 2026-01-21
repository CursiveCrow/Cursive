#include <cassert>
#include <string>
#include <vector>

#include "cursive0/codegen/checks.h"
#include "cursive0/codegen/ir_dump.h"
#include "cursive0/codegen/lower/lower_expr.h"
#include "cursive0/core/assert_spec.h"
#include "cursive0/syntax/ast.h"

namespace {

using cursive0::codegen::DumpIR;
using cursive0::codegen::InitPanicHandle;
using cursive0::codegen::LowerCtx;
using cursive0::syntax::ModulePath;

bool Contains(const std::string& text, const std::string& needle) {
  return text.find(needle) != std::string::npos;
}

}  // namespace

int main() {
  SPEC_COV("PoisonCheck");
  SPEC_COV("PoisonFlag-Get");
  SPEC_COV("PoisonFlag-Set");
  SPEC_COV("PoisonFlag-Clear");
  SPEC_COV("PoisonFlag-OnMove");

  SPEC_COV("Init-Start");
  SPEC_COV("Init-Step");
  SPEC_COV("Init-Next-Module");
  SPEC_COV("Init-Panic");
  SPEC_COV("Init-Done");
  SPEC_COV("Init-Ok");
  SPEC_COV("Init-Fail");
  SPEC_COV("Deinit-Ok");
  SPEC_COV("Deinit-Panic");

  {
    LowerCtx ctx;
    ctx.init_modules = {ModulePath{"a"}, ModulePath{"b"}, ModulePath{"c"}};
    ctx.init_eager_edges = {{0, 2}, {1, 2}};

    const auto ir = InitPanicHandle("c", ctx);
    const std::string dump = DumpIR(ir);
    assert(Contains(dump, "init_panic_handle c"));
    assert(Contains(dump, "[a, b, c]"));
  }

  {
    LowerCtx ctx;
    const auto ir = InitPanicHandle("main", ctx);
    const std::string dump = DumpIR(ir);
    assert(Contains(dump, "init_panic_handle main"));
    assert(Contains(dump, "[main]"));
  }

  return 0;
}
