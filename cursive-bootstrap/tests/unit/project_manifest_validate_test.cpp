#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>

#include "cursive0/core/assert_spec.h"
#include "cursive0/core/path.h"
#include "cursive0/project/project.h"

namespace {

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

}  // namespace

int main() {
  {
    SPEC_COV("WF-Project-Root");
    const auto dir = MakeTempDir("project_root");
    WriteFile(dir / "Cursive.toml", "assembly = {}\n");
    assert(cursive0::project::IsProjectRoot(dir));
    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
  }

  {
    SPEC_COV("Resolve-Canonical");
    const auto resolved = cursive0::core::Resolve("/root", "src");
    assert(resolved.has_value());
    assert(resolved->root == "/root");
    assert(resolved->path == "/root/src");
  }

  {
    SPEC_COV("Resolve-Canonical-Err");
    const auto resolved = cursive0::core::Resolve("/root", "../src");
    assert(!resolved.has_value());
  }

  return 0;
}
