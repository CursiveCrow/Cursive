#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>

#include "cursive0/core/assert_spec.h"
#include "cursive0/core/diagnostics.h"
#include "cursive0/core/path.h"
#include "cursive0/project/module_discovery.h"

namespace {

using cursive0::core::DiagnosticStream;
using cursive0::core::HasError;
using cursive0::project::CompilationUnit;
using cursive0::project::Modules;
using cursive0::project::ModulesResult;

bool HasCode(const DiagnosticStream& diags, std::string_view code) {
  for (const auto& diag : diags) {
    if (diag.code == code) {
      return true;
    }
  }
  return false;
}

std::filesystem::path MakeTempDir() {
  std::error_code ec;
  std::filesystem::path base = std::filesystem::temp_directory_path(ec);
  if (ec || base.empty()) {
    base = ".";
  }
  static std::size_t counter = 0;
  const auto stamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
  std::filesystem::path candidate = base / ("cursive0_moddisc_" + std::to_string(stamp) +
                                            "_" + std::to_string(counter++));
  std::filesystem::create_directories(candidate, ec);
  return candidate;
}

void WriteFile(const std::filesystem::path& path, std::string_view contents) {
  std::filesystem::create_directories(path.parent_path());
  std::ofstream out(path, std::ios::binary);
  out << contents;
}

bool HasModulePath(const ModulesResult& result, std::string_view path) {
  for (const auto& mod : result.modules) {
    if (mod.path == path) {
      return true;
    }
  }
  return false;
}

}  // namespace

int main() {
  {
    const std::string ext1 = cursive0::core::FileExt("file");
    const std::string ext2 = cursive0::core::FileExt(".hidden");
    const std::string ext3 = cursive0::core::FileExt("a.");
    const std::string ext4 = cursive0::core::FileExt("a.b.c");
    assert(ext1.empty());
    assert(ext2.empty());
    assert(ext3 == ".");
    assert(ext4 == ".c");
  }

  {
    SPEC_COV("Disc-Start");
    SPEC_COV("Disc-Add");
    SPEC_COV("Disc-Skip");
    SPEC_COV("Disc-Done");
    SPEC_COV("Module-Dir");
    SPEC_COV("Module-Path-Root");
    SPEC_COV("Module-Path-Rel");
    SPEC_COV("WF-Module-Path-Ok");
    SPEC_COV("Modules-Ok");

    const std::filesystem::path root = MakeTempDir();
    WriteFile(root / "root.cursive", "procedure demo() {}\n");
    WriteFile(root / "skip" / "note.txt", "skip\n");
    WriteFile(root / "alpha" / "beta" / "mod.cursive", "procedure demo() {}\n");

    const ModulesResult result = Modules(root, "demo");
    assert(!HasError(result.diags));
    assert(HasModulePath(result, "demo"));
    assert(HasModulePath(result, "alpha::beta"));
    assert(!HasModulePath(result, "skip"));
  }

  {
    SPEC_COV("WF-Module-Path-Ident-Err");
    SPEC_COV("Disc-Invalid-Component");
    SPEC_COV("Modules-Err");

    const std::filesystem::path root = MakeTempDir();
    WriteFile(root / "1bad" / "mod.cursive", "procedure demo() {}\n");

    const ModulesResult result = Modules(root, "demo");
    assert(HasError(result.diags));
    assert(HasCode(result.diags, "E-MOD-1106"));
  }

  {
    SPEC_COV("WF-Module-Path-Reserved");

    const std::filesystem::path root = MakeTempDir();
    WriteFile(root / "if" / "mod.cursive", "procedure demo() {}\n");

    const ModulesResult result = Modules(root, "demo");
    assert(HasError(result.diags));
    assert(HasCode(result.diags, "E-MOD-1105"));
  }

  {
    SPEC_COV("WF-Module-Path-Collision");
    SPEC_COV("Disc-Collision");
    SPEC_COV("Modules-Err");

    const std::filesystem::path root = MakeTempDir();
    const auto upper = root / "Foo";
    const auto lower = root / "foo";
    WriteFile(upper / "mod.cursive", "procedure demo() {}\n");
    WriteFile(lower / "mod.cursive", "procedure demo() {}\n");

    std::error_code ec;
    const bool same = std::filesystem::equivalent(upper, lower, ec);

    const ModulesResult result = Modules(root, "demo");
    if (same) {
      assert(!HasError(result.diags));
    } else {
      assert(HasError(result.diags));
      assert(HasCode(result.diags, "E-MOD-1104"));
      assert(HasCode(result.diags, "W-MOD-1101"));
    }
  }

  {
    SPEC_COV("Module-Path-Rel-Fail");
    SPEC_COV("Disc-Rel-Fail");
    SPEC_COV("Modules-Err");

    const std::filesystem::path root = MakeTempDir();
    WriteFile(root / "root.cursive", "procedure demo() {}\n");

    const std::filesystem::path weird = root / ".." / root.filename();
    const ModulesResult result = Modules(weird, "demo");
    assert(HasError(result.diags));
    assert(HasCode(result.diags, "E-PRJ-0304"));
  }

  {
    SPEC_COV("CompilationUnit-Rel-Fail");

    const std::filesystem::path root = MakeTempDir();
    const std::filesystem::path module_dir = root / "mod";
    WriteFile(module_dir / "unit.cursive", "procedure demo() {}\n");

    const std::filesystem::path weird = module_dir / ".." / module_dir.filename();
    const auto result = CompilationUnit(weird);
    assert(HasError(result.diags));
    assert(HasCode(result.diags, "E-PRJ-0303"));
  }

  return 0;
}
