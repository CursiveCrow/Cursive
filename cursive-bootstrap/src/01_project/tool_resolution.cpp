#include "cursive0/01_project/tool_resolution.h"

#include <algorithm>
#include <cstdlib>
#include <string>

#include "cursive0/00_core/assert_spec.h"
#include "cursive0/00_core/host_primitives.h"
#include "cursive0/01_project/project.h"

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

void AppendUniquePaths(std::vector<std::filesystem::path>& out,
                       const std::vector<std::filesystem::path>& extra) {
  for (const auto& candidate : extra) {
    bool exists = false;
    for (const auto& current : out) {
      if (current == candidate) {
        exists = true;
        break;
      }
    }
    if (!exists) {
      out.push_back(candidate);
    }
  }
}

#ifdef _WIN32
std::vector<int> ParseVersionParts(std::string_view text) {
  std::vector<int> parts;
  int value = 0;
  bool active = false;
  for (char ch : text) {
    if (ch >= '0' && ch <= '9') {
      active = true;
      value = value * 10 + static_cast<int>(ch - '0');
    } else if (active) {
      parts.push_back(value);
      value = 0;
      active = false;
    }
  }
  if (active) {
    parts.push_back(value);
  }
  if (parts.empty()) {
    parts.push_back(0);
  }
  return parts;
}

int CompareVersionParts(const std::vector<int>& lhs,
                        const std::vector<int>& rhs) {
  const std::size_t count = std::max(lhs.size(), rhs.size());
  for (std::size_t i = 0; i < count; ++i) {
    const int left = i < lhs.size() ? lhs[i] : 0;
    const int right = i < rhs.size() ? rhs[i] : 0;
    if (left < right) {
      return -1;
    }
    if (left > right) {
      return 1;
    }
  }
  return 0;
}

struct LinkCandidate {
  std::filesystem::path path;
  std::vector<int> version;
  int preference = 0;
};

bool IsBetterCandidate(const LinkCandidate& lhs,
                       const LinkCandidate& rhs) {
  const int version_cmp = CompareVersionParts(lhs.version, rhs.version);
  if (version_cmp != 0) {
    return version_cmp > 0;
  }
  return lhs.preference < rhs.preference;
}

void AddLinkCandidate(std::vector<LinkCandidate>& candidates,
                      const std::filesystem::path& path,
                      const std::vector<int>& version,
                      int preference) {
  std::error_code ec;
  if (std::filesystem::exists(path, ec) && !ec) {
    candidates.push_back({path, version, preference});
  }
}

std::vector<std::filesystem::path> VisualStudioRoots() {
  std::vector<std::filesystem::path> roots;
  if (auto program_files = GetEnv("ProgramFiles")) {
    roots.push_back(std::filesystem::path(*program_files) /
                    "Microsoft Visual Studio");
  }
  if (auto program_files_x86 = GetEnv("ProgramFiles(x86)")) {
    std::filesystem::path root =
        std::filesystem::path(*program_files_x86) /
        "Microsoft Visual Studio";
    bool duplicate = false;
    for (const auto& existing : roots) {
      if (existing == root) {
        duplicate = true;
        break;
      }
    }
    if (!duplicate) {
      roots.push_back(std::move(root));
    }
  }
  return roots;
}

std::optional<std::filesystem::path> FindLatestMsvcLinker() {
  std::vector<LinkCandidate> candidates;

  if (auto vctools = GetEnv("VCToolsInstallDir")) {
    const std::filesystem::path base(*vctools);
    const auto version = ParseVersionParts(base.filename().string());
    AddLinkCandidate(candidates,
                     base / "bin" / "Hostx64" / "x64" / "link.exe",
                     version, 0);
    AddLinkCandidate(candidates,
                     base / "bin" / "Hostx64" / "x86" / "link.exe",
                     version, 1);
    AddLinkCandidate(candidates,
                     base / "bin" / "Hostx86" / "x64" / "link.exe",
                     version, 2);
    AddLinkCandidate(candidates,
                     base / "bin" / "Hostx86" / "x86" / "link.exe",
                     version, 3);
  }

  const auto roots = VisualStudioRoots();
  for (const auto& root : roots) {
    std::error_code root_ec;
    if (!std::filesystem::is_directory(root, root_ec) || root_ec) {
      continue;
    }
    std::error_code year_ec;
    for (auto year_it = std::filesystem::directory_iterator(root, year_ec);
         year_it != std::filesystem::directory_iterator();
         year_it.increment(year_ec)) {
      if (year_ec) {
        year_ec.clear();
        continue;
      }
      std::error_code year_dir_ec;
      if (!year_it->is_directory(year_dir_ec) || year_dir_ec) {
        continue;
      }
      std::error_code edition_ec;
      for (auto edition_it =
               std::filesystem::directory_iterator(year_it->path(),
                                                   edition_ec);
           edition_it != std::filesystem::directory_iterator();
           edition_it.increment(edition_ec)) {
        if (edition_ec) {
          edition_ec.clear();
          continue;
        }
        std::error_code edition_dir_ec;
        if (!edition_it->is_directory(edition_dir_ec) || edition_dir_ec) {
          continue;
        }
        const auto msvc_root = edition_it->path() / "VC" / "Tools" / "MSVC";
        std::error_code msvc_ec;
        if (!std::filesystem::is_directory(msvc_root, msvc_ec) || msvc_ec) {
          continue;
        }
        std::error_code version_ec;
        for (auto version_it =
                 std::filesystem::directory_iterator(msvc_root, version_ec);
             version_it != std::filesystem::directory_iterator();
             version_it.increment(version_ec)) {
          if (version_ec) {
            version_ec.clear();
            continue;
          }
          std::error_code version_dir_ec;
          if (!version_it->is_directory(version_dir_ec) || version_dir_ec) {
            continue;
          }
          const auto version_name = version_it->path().filename().string();
          const auto version = ParseVersionParts(version_name);
          const auto base = version_it->path() / "bin";
          AddLinkCandidate(candidates,
                           base / "Hostx64" / "x64" / "link.exe",
                           version, 0);
          AddLinkCandidate(candidates,
                           base / "Hostx64" / "x86" / "link.exe",
                           version, 1);
          AddLinkCandidate(candidates,
                           base / "Hostx86" / "x64" / "link.exe",
                           version, 2);
          AddLinkCandidate(candidates,
                           base / "Hostx86" / "x86" / "link.exe",
                           version, 3);
        }
      }
    }
  }

  if (candidates.empty()) {
    return std::nullopt;
  }

  LinkCandidate best = candidates.front();
  for (std::size_t i = 1; i < candidates.size(); ++i) {
    if (IsBetterCandidate(candidates[i], best)) {
      best = candidates[i];
    }
  }
  return best.path;
}
#endif

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
  auto dirs = SearchDirs(project);
#ifdef _WIN32
  if (tool == "link") {
    const auto path_env = GetEnv("PATH");
    if (path_env.has_value() && !path_env->empty()) {
      AppendUniquePaths(dirs, SplitPathList(*path_env));
    }
  }
#endif
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

#ifdef _WIN32
  if (tool == "link") {
    if (auto linker = FindLatestMsvcLinker()) {
      SPEC_RULE("ResolveTool-Ok");
      return linker;
    }
  }
#endif

  core::HostPrimFail(core::HostPrim::ResolveTool, true);
  if (tool == "lld-link") {
    SPEC_RULE("ResolveTool-Err-Linker");
  } else if (tool == "llvm-as") {
    SPEC_RULE("ResolveTool-Err-IR");
  }
  return std::nullopt;
}

}  // namespace cursive0::project
