#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>

#include "cursive0/core/assert_spec.h"
#include "cursive0/project/manifest.h"

namespace {

using cursive0::core::DiagnosticStream;
using cursive0::project::ManifestParseResult;
using cursive0::project::ParseManifest;

std::filesystem::path MakeTempDir(std::string_view tag) {
  const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
  std::filesystem::path base = std::filesystem::temp_directory_path();
  std::string name = std::string("cursivec0_") + std::string(tag) + "_" + std::to_string(now);
  std::filesystem::path dir = base / name;
  std::error_code ec;
  std::filesystem::create_directories(dir, ec);
  assert(!ec);
  return dir;
}

void WriteFile(const std::filesystem::path& path, const std::string& contents) {
  std::ofstream out(path, std::ios::binary);
  assert(out.is_open());
  out << contents;
}

bool HasCode(const DiagnosticStream& diags, std::string_view code) {
  for (const auto& diag : diags) {
    if (diag.code == code) {
      return true;
    }
  }
  return false;
}

}  // namespace

int main() {
  {
    SPEC_COV("Parse-Manifest-Missing");
    const auto dir = MakeTempDir("manifest_missing");
    const ManifestParseResult result = ParseManifest(dir);
    assert(!result.table.has_value());
    assert(HasCode(result.diags, "E-PRJ-0101"));
    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
  }

  {
    SPEC_COV("Parse-Manifest-Err");
    const auto dir = MakeTempDir("manifest_invalid");
    WriteFile(dir / "Cursive.toml", "assembly = [\n");
    const ManifestParseResult result = ParseManifest(dir);
    assert(!result.table.has_value());
    assert(HasCode(result.diags, "E-PRJ-0102"));
    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
  }

  {
    SPEC_COV("Parse-Manifest-Ok");
    const auto dir = MakeTempDir("manifest_ok");
    WriteFile(dir / "Cursive.toml", "assembly = {}\n");
    const ManifestParseResult result = ParseManifest(dir);
    assert(result.table.has_value());
    assert(result.diags.empty());
    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
  }

  return 0;
}
