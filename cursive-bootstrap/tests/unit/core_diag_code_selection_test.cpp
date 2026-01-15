#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>

#include "cursive0/core/assert_spec.h"
#include "cursive0/core/diagnostic_codes.h"

namespace {

using cursive0::core::Code;
using cursive0::core::DiagCodeMap;

static int RunDiagExtractorCheck() {
  const std::filesystem::path tool_path =
      std::filesystem::path(__FILE__).parent_path().parent_path().parent_path() / "tools" /
      "diag_extractor.py";
  const std::string tool = tool_path.string();
  const std::string cmd = "python3 \"" + tool + "\" --check";
  int rc = std::system(cmd.c_str());
  if (rc != 0) {
    const std::string fallback = "python \"" + tool + "\" --check";
    rc = std::system(fallback.c_str());
  }
  return rc;
}

}  // namespace

int main() {
  SPEC_COV("Code-Spec");
  SPEC_COV("Code-C0");

  {
    DiagCodeMap spec;
    DiagCodeMap c0;
    spec["Rule-A"] = "E-AAA-0001";
    c0["Rule-A"] = "E-BBB-0002";
    c0["Rule-B"] = "W-CCC-0003";

    const auto code_a = Code(spec, c0, "Rule-A");
    assert(code_a.has_value());
    assert(*code_a == "E-AAA-0001");

    const auto code_b = Code(spec, c0, "Rule-B");
    assert(code_b.has_value());
    assert(*code_b == "W-CCC-0003");

    const auto code_missing = Code(spec, c0, "Missing");
    assert(!code_missing.has_value());
  }

  {
    const int rc = RunDiagExtractorCheck();
    assert(rc == 0);
  }

  return 0;
}
