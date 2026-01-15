#include <cassert>
#include <filesystem>
#include <string_view>

#include "cursive0/core/assert_spec.h"
#include "cursive0/core/diagnostics.h"
#include "cursive0/project/deterministic_order.h"

namespace {

using cursive0::core::DiagnosticStream;
using cursive0::core::HasError;
using cursive0::project::DirSeq;
using cursive0::project::DirSeqFrom;
using cursive0::project::FileKey;
using cursive0::project::Fold;
using cursive0::project::FoldPath;

bool HasCode(const DiagnosticStream& diags, std::string_view code) {
  for (const auto& diag : diags) {
    if (diag.code == code) {
      return true;
    }
  }
  return false;
}

std::filesystem::path MissingTempDir() {
  std::error_code ec;
  std::filesystem::path base = std::filesystem::temp_directory_path(ec);
  if (ec) {
    base = ".";
  }
  std::filesystem::path candidate = base / "cursive0_missing_dir_test";
  ec.clear();
  if (!std::filesystem::exists(candidate, ec) && !ec) {
    return candidate;
  }
  for (int i = 0; i < 10; ++i) {
    std::filesystem::path attempt = candidate;
    attempt += std::to_string(i);
    ec.clear();
    if (!std::filesystem::exists(attempt, ec) && !ec) {
      return attempt;
    }
  }
  return candidate / "still_missing";
}

}  // namespace

int main() {
  {
    SPEC_COV("FileOrder-Rel-Fail");
    DiagnosticStream diags;
    (void)FileKey("/root/other/file.cursive", "/root/src", diags);
    assert(HasError(diags));
    assert(HasCode(diags, "E-PRJ-0303"));
  }

  {
    SPEC_COV("DirSeq-Rel-Fail");
    const std::vector<std::filesystem::path> dirs = {"/outside"};
    const auto result = DirSeqFrom("/root/src", dirs);
    assert(HasError(result.diags));
    assert(HasCode(result.diags, "E-PRJ-0303"));
  }

  {
    SPEC_COV("DirSeq-Read-Err");
    const std::filesystem::path missing = MissingTempDir();
    const auto result = DirSeq(missing);
    assert(HasError(result.diags));
    assert(HasCode(result.diags, "E-PRJ-0305"));
  }

  {
    const std::string folded_path = FoldPath("Foo/Bar");
    assert(folded_path == "foo/bar");
    const std::string folded_module = Fold("Ab::CD");
    assert(folded_module == "ab::cd");
  }

  return 0;
}
