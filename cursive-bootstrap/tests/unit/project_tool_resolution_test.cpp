#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>

#include "cursive0/core/assert_spec.h"
#include "cursive0/project/outputs.h"
#include "cursive0/project/project.h"
#include "cursive0/project/tool_resolution.h"

namespace {

using cursive0::project::Assembly;
using cursive0::project::Project;
using cursive0::project::ValidatedAssembly;

#ifdef _WIN32
static constexpr char kPathSep = ';';
#else
static constexpr char kPathSep = ':';
#endif

struct EnvGuard {
  explicit EnvGuard(const char* name) : name_(name) {
    const char* value = std::getenv(name);
    if (value) {
      old_ = std::string(value);
    }
  }

  ~EnvGuard() {
    if (old_.has_value()) {
      Set(old_.value());
    } else {
      Unset();
    }
  }

  void Set(const std::string& value) const {
#ifdef _WIN32
    _putenv_s(name_, value.c_str());
#else
    setenv(name_, value.c_str(), 1);
#endif
  }

  void Unset() const {
#ifdef _WIN32
    _putenv_s(name_, "");
#else
    unsetenv(name_);
#endif
  }

  const char* name_;
  std::optional<std::string> old_;
};

std::filesystem::path MakeTempDir(std::string_view name) {
  static int counter = 0;
  const auto root = std::filesystem::temp_directory_path();
  const std::string dir_name =
      std::string("cursive0_") + std::string(name) + "_" +
      std::to_string(++counter);
  const auto path = root / dir_name;
  std::error_code ec;
  std::filesystem::create_directories(path, ec);
  return path;
}

void Touch(const std::filesystem::path& path) {
  std::filesystem::create_directories(path.parent_path());
  std::ofstream out(path, std::ios::binary);
  out << "stub";
}

Project MakeProject(const std::filesystem::path& root) {
  Project project;
  project.root = root;
  project.source_root = root / "src";
  const ValidatedAssembly spec{"app", "executable", "src",
                               std::nullopt, std::nullopt};
  Assembly assembly;
  assembly.name = spec.name;
  assembly.kind = spec.kind;
  assembly.root = spec.root;
  assembly.out_dir = spec.out_dir;
  assembly.emit_ir = spec.emit_ir;
  assembly.source_root = project.source_root;
  assembly.outputs = cursive0::project::ComputeOutputPaths(project.root, spec);
  project.outputs = assembly.outputs;
  project.assembly = assembly;
  project.assemblies = {assembly};
  return project;
}

}  // namespace

int main() {
  EnvGuard env_llvm("C0_LLVM_BIN");
  EnvGuard env_path("PATH");

  {
    SPEC_COV("ResolveTool-Ok");
    const auto env_dir = MakeTempDir("llvm_env");
    Touch(env_dir / "llvm-as");

    const auto project_root = MakeTempDir("project_env");
    const auto repo_dir = project_root / "llvm" / "llvm-21.1.8-x86_64" / "bin";
    Touch(repo_dir / "llvm-as");

    env_llvm.Set(env_dir.string());
    env_path.Set(std::string("/tmp") + kPathSep + std::string("/bin"));

    const Project project = MakeProject(project_root);
    const auto tool = cursive0::project::ResolveTool(project, "llvm-as");
    assert(tool.has_value());
    assert(tool->parent_path() == env_dir);
  }

  {
    SPEC_COV("ResolveTool-Err-IR");
    const auto env_dir = MakeTempDir("llvm_missing");
    env_llvm.Set(env_dir.string());
    env_path.Set(std::string("/tmp"));

    const Project project = MakeProject(MakeTempDir("project_missing"));
    const auto tool = cursive0::project::ResolveTool(project, "llvm-as");
    assert(!tool.has_value());
  }

  {
    SPEC_COV("ResolveTool-Err-Linker");
    const auto env_dir = MakeTempDir("linker_missing");
    env_llvm.Set(env_dir.string());
    env_path.Set(std::string("/tmp"));

    const Project project = MakeProject(MakeTempDir("project_linker_missing"));
    const auto tool = cursive0::project::ResolveTool(project, "lld-link");
    assert(!tool.has_value());
  }

  {
    const auto project_root = MakeTempDir("project_repo");
    const auto repo_dir = project_root / "llvm" / "llvm-21.1.8-x86_64" / "bin";
    Touch(repo_dir / "llvm-as");

    env_llvm.Unset();
    env_path.Set("");

    const Project project = MakeProject(project_root);
    const auto tool = cursive0::project::ResolveTool(project, "llvm-as");
    assert(tool.has_value());
    assert(tool->parent_path() == repo_dir);
  }

  {
    const auto dir_a = MakeTempDir("path_a");
    const auto dir_b = MakeTempDir("path_b");
    Touch(dir_a / "lld-link");
    Touch(dir_b / "lld-link");

    env_llvm.Unset();
    env_path.Set(dir_a.string() + kPathSep + dir_b.string());

    const Project project = MakeProject(MakeTempDir("project_path"));
    const auto tool = cursive0::project::ResolveTool(project, "lld-link");
    assert(tool.has_value());
    assert(tool->parent_path() == dir_a);
  }

#ifdef _WIN32
  {
    const auto env_dir = MakeTempDir("win_suffix");
    Touch(env_dir / "llvm-as");
    Touch(env_dir / "llvm-as.exe");

    env_llvm.Set(env_dir.string());
    env_path.Set("");

    const Project project = MakeProject(MakeTempDir("project_win"));
    const auto tool = cursive0::project::ResolveTool(project, "llvm-as");
    assert(tool.has_value());
    assert(tool->filename() == "llvm-as");
  }
#endif

  return 0;
}
