// =============================================================================
// MIGRATION MAPPING: tool_resolution.cpp
// =============================================================================
//
// SPEC REFERENCE: CursiveSpecification.md Section 2.6 (lines 1651-1671)
//   - 2.6. Tool Resolution and IR Assembly Inputs
//   - SearchDirs(P) definition
//   - ResolveTool-Ok / ResolveTool-Err-Linker / ResolveTool-Err-IR rules
//
// SOURCE FILE: cursive-bootstrap/src/01_project/tool_resolution.cpp
//   - Lines 1-347 (entire file)
//
// =============================================================================
// CONTENT TO MIGRATE
// =============================================================================
//
// 1. Helper functions (lines 13-92)
//    - GetEnv() (lines 15-21)
//      PURPOSE: Get environment variable value
//
//    - PathSeparator() (lines 23-29)
//      PURPOSE: Get path list separator (';' on Windows, ':' on POSIX)
//
//    - SplitPathList() (lines 31-45)
//      PURPOSE: Split PATH-style list into directory paths
//
//    - EndsWithExe() (lines 53-62)
//      PURPOSE: Check if filename ends with ".exe"
//
//    - ToolCandidates() (lines 64-75)
//      PURPOSE: Generate tool name variants (with/without .exe on Windows)
//
//    - AppendUniquePaths() (lines 77-91)
//      PURPOSE: Append paths avoiding duplicates
//
// 2. SearchDirs() (lines 288-303)
//    - PURPOSE: Get search directories for tool resolution
//    - SPEC RULE:
//      SearchDirs(P) =
//        [ToolchainConfig(P).llvm_bin] if ToolchainConfig(P).llvm_bin != empty
//        [CompilerSidecarToolsDir(P)]  if sidecar tools are present
//        PATHDirs  otherwise
//    - SPEC REF: Lines 1653-1656
//
// 3. ResolveTool() (lines 305-344)
//    - PURPOSE: Resolve tool path from search directories
//    - SPEC RULES:
//      * ResolveTool-Ok (spec lines 1658-1661)
//      * ResolveTool-Err-Linker (spec lines 1663-1666)
//      * ResolveTool-Err-IR (spec lines 1668-1671)
//
// =============================================================================
// DEPENDENCIES
// =============================================================================
//
// Headers required:
//   - "cursive0/01_project/tool_resolution.h"
//   - "cursive0/00_core/assert_spec.h" (SPEC_RULE macro)
//   - "cursive0/00_core/host_primitives.h" (HostPrimFail)
//   - "cursive0/01_project/project.h" (Project)
//   - <algorithm>
//   - <cstdlib>
//   - <string>
//   - <vector>
//   - <optional>
//   - <filesystem>
//   - <system_error>
//
// Types from header (tool_resolution.h):
//   - No custom types, just function declarations
//
// =============================================================================
// REFACTORING NOTES
// =============================================================================
//
// 1. The search order (toolchain llvm_bin > compiler sidecars > PATH) is intentional:
//    - Uses manifest configuration as explicit source of truth
//    - Supports staged compiler-sidecar tools in the compiler distribution
//    - Falls back to system PATH when no explicit/toolchain LLVM is present
//
// 2. Tool candidates include both with and without .exe extension on Windows
//    This handles cases where the extension may or may not be present
//
// =============================================================================
// SPEC RULE ANNOTATIONS (use SPEC_RULE macro)
// =============================================================================
//
// Line 322: SPEC_RULE("ResolveTool-Ok");
// Line 331: SPEC_RULE("ResolveTool-Ok");
// Line 337: HostPrimFail call
// Line 339: SPEC_RULE("ResolveTool-Err-Linker");
// Line 341: SPEC_RULE("ResolveTool-Err-IR");
//
// =============================================================================

#include "01_project/tool_resolution.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_map>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/wait.h>
#include <unistd.h>
#endif

#include "00_core/assert_spec.h"
#include "00_core/host_primitives.h"
#include "00_core/windows_bundle.h"
#include "05_codegen/llvm/llvm_module.h"
#include "01_project/project.h"
#include "01_project/target_profile.h"

namespace cursive::project {

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

std::optional<std::string> RunToolVersionCommand(
    const std::filesystem::path& tool) {
#ifdef _WIN32
  auto quote_arg = [](std::wstring_view arg) {
    std::wstring out;
    out.push_back(L'"');
    for (const wchar_t ch : arg) {
      if (ch == L'"') {
        out.push_back(L'\\');
      }
      out.push_back(ch);
    }
    out.push_back(L'"');
    return out;
  };

  std::vector<std::wstring> args;
  args.push_back(tool.wstring());
  args.push_back(L"--version");

  std::wstring cmd_line;
  for (std::size_t i = 0; i < args.size(); ++i) {
    if (i != 0) {
      cmd_line.push_back(L' ');
    }
    cmd_line += quote_arg(args[i]);
  }

  SECURITY_ATTRIBUTES sa;
  ZeroMemory(&sa, sizeof(sa));
  sa.nLength = sizeof(sa);
  sa.bInheritHandle = TRUE;

  HANDLE read_pipe = nullptr;
  HANDLE write_pipe = nullptr;
  if (!CreatePipe(&read_pipe, &write_pipe, &sa, 0)) {
    return std::nullopt;
  }
  SetHandleInformation(read_pipe, HANDLE_FLAG_INHERIT, 0);

  STARTUPINFOW si;
  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(si);
  si.dwFlags |= STARTF_USESTDHANDLES;
  si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
  si.hStdOutput = write_pipe;
  si.hStdError = write_pipe;

  PROCESS_INFORMATION pi;
  ZeroMemory(&pi, sizeof(pi));

  std::vector<wchar_t> cmd_buf(cmd_line.begin(), cmd_line.end());
  cmd_buf.push_back(L'\0');

  const BOOL ok = CreateProcessW(tool.wstring().c_str(), cmd_buf.data(),
                                 nullptr, nullptr, TRUE, CREATE_NO_WINDOW,
                                 nullptr, nullptr, &si, &pi);
  CloseHandle(write_pipe);
  if (!ok) {
    CloseHandle(read_pipe);
    return std::nullopt;
  }

  std::string output;
  char buffer[4096];
  DWORD bytes_read = 0;
  while (ReadFile(read_pipe, buffer, sizeof(buffer), &bytes_read, nullptr) &&
         bytes_read > 0) {
    output.append(buffer, buffer + bytes_read);
  }
  CloseHandle(read_pipe);

  WaitForSingleObject(pi.hProcess, INFINITE);
  DWORD exit_code = 1;
  GetExitCodeProcess(pi.hProcess, &exit_code);
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);
  if (exit_code != 0) {
    return std::nullopt;
  }
  return output;
#else
  int pipefd[2];
  if (pipe(pipefd) != 0) {
    return std::nullopt;
  }

  const pid_t pid = fork();
  if (pid < 0) {
    close(pipefd[0]);
    close(pipefd[1]);
    return std::nullopt;
  }
  if (pid == 0) {
    dup2(pipefd[1], STDOUT_FILENO);
    dup2(pipefd[1], STDERR_FILENO);
    close(pipefd[0]);
    close(pipefd[1]);

    std::string tool_str = tool.string();
    std::string version_arg = "--version";
    char* argv[] = {tool_str.data(), version_arg.data(), nullptr};
    execv(argv[0], argv);
    _exit(127);
  }

  close(pipefd[1]);
  std::string output;
  char buffer[4096];
  ssize_t count = 0;
  while ((count = read(pipefd[0], buffer, sizeof(buffer))) > 0) {
    output.append(buffer, buffer + count);
  }
  close(pipefd[0]);

  int status = 0;
  if (waitpid(pid, &status, 0) < 0) {
    return std::nullopt;
  }
  if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
    return std::nullopt;
  }
  return output;
#endif
}

bool IsPinnedLlvmAssembler(std::string_view tool) {
  return tool == "llvm-as";
}

bool CandidateMatchesLLVMToolchain(const std::filesystem::path& candidate) {
  static std::mutex cache_mu;
  static std::unordered_map<std::string, bool> cache;

  const std::string cache_key = candidate.lexically_normal().generic_string();
  {
    const std::lock_guard<std::mutex> lock(cache_mu);
    const auto it = cache.find(cache_key);
    if (it != cache.end()) {
      return it->second;
    }
  }

  const auto version_output = RunToolVersionCommand(candidate);
  const bool matches =
      version_output.has_value() &&
      version_output->find(std::string(codegen::GetLLVMToolchainVersion())) !=
          std::string::npos;

  const std::lock_guard<std::mutex> lock(cache_mu);
  cache.insert_or_assign(cache_key, matches);
  return matches;
}

std::filesystem::path CurrentExecutableDir() {
#ifdef _WIN32
  wchar_t path[MAX_PATH];
  const DWORD len = GetModuleFileNameW(nullptr, path, MAX_PATH);
  if (len == 0 || len >= MAX_PATH) {
    return std::filesystem::path();
  }
  return std::filesystem::path(path).parent_path();
#else
  char path[4096];
  const ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
  if (len <= 0) {
    return std::filesystem::path();
  }
  path[len] = '\0';
  return std::filesystem::path(path).parent_path();
#endif
}

bool HasCompilerSupportLayout(const std::filesystem::path& candidate) {
  if (candidate.empty()) {
    return false;
  }
  std::error_code ec;
  for (const char* rel : {"runtime", "tools", "bin", "lib"}) {
    ec.clear();
    if (std::filesystem::is_directory(candidate / rel, ec) && !ec) {
      return true;
    }
  }
  return false;
}

std::optional<std::filesystem::path> CompilerSidecarToolsDir() {
#ifdef _WIN32
  if (const auto bundled_tools = core::BundledWindowsToolsDirPath();
      bundled_tools.has_value()) {
    return bundled_tools;
  }
#endif

  const auto compiler_dir = CurrentExecutableDir();
  if (HasCompilerSupportLayout(compiler_dir)) {
    const auto tools_dir = compiler_dir / "tools";
    std::error_code ec;
    if (std::filesystem::is_directory(tools_dir, ec) && !ec) {
      return tools_dir;
    }
  }

  const auto parent = compiler_dir.parent_path();
  if (HasCompilerSupportLayout(parent)) {
    const auto tools_dir = parent / "tools";
    std::error_code ec;
    if (std::filesystem::is_directory(tools_dir, ec) && !ec) {
      return tools_dir;
    }
  }

  return std::nullopt;
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
  if (!EndsWithExe(tool)) {
    std::string with_exe(tool);
    with_exe += ".exe";
    out.push_back(std::move(with_exe));
  }
  return out;
}

void AddUniquePath(std::vector<std::filesystem::path>& out,
                   const std::filesystem::path& candidate) {
  if (candidate.empty()) {
    return;
  }
  if (std::find(out.begin(), out.end(), candidate) == out.end()) {
    out.push_back(candidate);
  }
}

}  // namespace

std::vector<std::filesystem::path> SearchDirs(const Project& project) {
  if (project.toolchain.llvm_bin.has_value() &&
      !project.toolchain.llvm_bin->empty()) {
    return {std::filesystem::path(*project.toolchain.llvm_bin)};
  }

  if (const auto sidecar_tools = CompilerSidecarToolsDir();
      sidecar_tools.has_value()) {
    return {*sidecar_tools};
  }

  std::vector<std::filesystem::path> dirs;
  const auto path_env = GetEnv("PATH");
  if (path_env.has_value() && !path_env->empty()) {
    for (const auto& path : SplitPathList(*path_env)) {
      AddUniquePath(dirs, path);
    }
  }
  return dirs;
}

std::optional<std::filesystem::path> ResolveTool(const Project& project,
                                                 TargetProfile target_profile,
                                                 std::string_view tool) {
  const auto dirs = SearchDirs(project);
  const auto candidates = ToolCandidates(tool);
  for (const auto& dir : dirs) {
    for (const auto& name : candidates) {
      const auto candidate = dir / name;
      std::error_code ec;
      if (std::filesystem::exists(candidate, ec) && !ec) {
        if (IsPinnedLlvmAssembler(tool) &&
            !CandidateMatchesLLVMToolchain(candidate)) {
          continue;
        }
        SPEC_RULE("ResolveTool-Ok");
        return candidate;
      }
    }
  }

  core::HostPrimFail(core::HostPrim::ResolveTool, true);
  if (tool == LinkerToolName(target_profile)) {
    SPEC_RULE("ResolveTool-Err-Linker");
  } else if (tool == ArchiverToolName(target_profile)) {
    SPEC_RULE("ResolveTool-Err-Archiver");
  } else if (tool == "llvm-as") {
    SPEC_RULE("ResolveTool-Err-IR");
  }
  return std::nullopt;
}

std::string FormatSearchedPaths(const Project& project,
                                std::string_view tool) {
  const auto dirs = SearchDirs(project);
  const auto candidates = ToolCandidates(tool);
  std::string result = "searched for '" + std::string(tool) + "' in:";
  if (dirs.empty()) {
    result += " (no search directories found)";
    return result;
  }
  for (const auto& dir : dirs) {
    result += "\n  ";
    result += dir.string();
    for (const auto& name : candidates) {
      const auto candidate = dir / name;
      std::error_code ec;
      const bool exists = std::filesystem::exists(candidate, ec) && !ec;
      result += "\n    ";
      result += name;
      if (!exists) {
        result += " (not found)";
      }
    }
  }
  return result;
}

}  // namespace cursive::project
