#include "cursive0/project/tool_resolution.h"

#include <cstdlib>
#include <string>

#include "cursive0/core/assert_spec.h"
#include "cursive0/core/host_primitives.h"
#include "cursive0/project/project.h"

namespace cursive0::project {

namespace {

std::optional<std::string> GetEnv(const char* name) {
  const char* value = std::getenv(name);
  if (!value) {
    return std::nullopt;
  }
  return std::string(value);
}

char PathSeparator() {
#ifdef _WIN32
  return ';';
#else
  return ':';
#endif
}

std::vector<std::filesystem::path> SplitPathList(std::string_view path_list) {
  std::vector<std::filesystem::path> out;
  const char sep = PathSeparator();
  std::size_t start = 0;
  for (std::size_t i = 0; i <= path_list.size(); ++i) {
    if (i == path_list.size() || path_list[i] == sep) {
      const std::string_view segment = path_list.substr(start, i - start);
      if (!segment.empty()) {
        out.emplace_back(std::string(segment));
      }
      start = i + 1;
    }
  }
  return out;
}

bool IsRepoLLVM(const Project& project) {
  std::error_code ec;
  const auto repo = project.root / "llvm" / "llvm-21.1.8-x86_64" / "bin";
  return std::filesystem::is_directory(repo, ec) && !ec;
}

bool EndsWithExe(std::string_view name) {
  if (name.size() < 4) {
    return false;
  }
  const auto tail = name.substr(name.size() - 4);
  return tail[0] == '.' &&
         (tail[1] == 'e' || tail[1] == 'E') &&
         (tail[2] == 'x' || tail[2] == 'X') &&
         (tail[3] == 'e' || tail[3] == 'E');
}

std::vector<std::string> ToolCandidates(std::string_view tool) {
  std::vector<std::string> out;
  out.emplace_back(tool);
#ifdef _WIN32
  if (!EndsWithExe(tool)) {
    std::string with_exe(tool);
    with_exe += ".exe";
    out.push_back(std::move(with_exe));
  }
#endif
  return out;
}

}  // namespace

std::vector<std::filesystem::path> SearchDirs(const Project& project) {
  const auto env = GetEnv("C0_LLVM_BIN");
  if (env.has_value() && !env->empty()) {
    return {std::filesystem::path(*env)};
  }

  if (IsRepoLLVM(project)) {
    return {project.root / "llvm" / "llvm-21.1.8-x86_64" / "bin"};
  }

  const auto path_env = GetEnv("PATH");
  if (!path_env.has_value() || path_env->empty()) {
    return {};
  }
  return SplitPathList(*path_env);
}

std::optional<std::filesystem::path> ResolveTool(const Project& project,
                                                 std::string_view tool) {
  const auto dirs = SearchDirs(project);
  const auto candidates = ToolCandidates(tool);
  for (const auto& dir : dirs) {
    for (const auto& name : candidates) {
      const auto candidate = dir / name;
      std::error_code ec;
      if (std::filesystem::exists(candidate, ec) && !ec) {
        SPEC_RULE("ResolveTool-Ok");
        return candidate;
      }
    }
  }

  core::HostPrimFail(core::HostPrim::ResolveTool, true);
  if (tool == "lld-link") {
    SPEC_RULE("ResolveTool-Err-Linker");
  } else if (tool == "llvm-as") {
    SPEC_RULE("ResolveTool-Err-IR");
  }
  return std::nullopt;
}

}  // namespace cursive0::project
