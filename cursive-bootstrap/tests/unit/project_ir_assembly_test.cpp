#include <cassert>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

#include "cursive0/core/assert_spec.h"
#include "cursive0/project/ir_assembly.h"

namespace {

std::optional<std::string> InvokeOk(const std::filesystem::path&,
                                    std::string_view) {
  return std::string("BC");
}

std::optional<std::string> InvokeFail(const std::filesystem::path&,
                                      std::string_view) {
  return std::nullopt;
}

}  // namespace

int main() {
  {
    SPEC_COV("AssembleIR-Ok");
    const auto out =
        cursive0::project::AssembleIRWithDeps("/tools/llvm-as", "IR", InvokeOk);
    assert(out.has_value());
    assert(*out == "BC");
  }

  {
    SPEC_COV("AssembleIR-Err");
    const auto out =
        cursive0::project::AssembleIRWithDeps("/tools/llvm-as", "IR", InvokeFail);
    assert(!out.has_value());
  }

  return 0;
}
