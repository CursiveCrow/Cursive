#include "00_core/windows_bundle.h"

#include <filesystem>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>

#ifdef _WIN32
#include <windows.h>
#endif

namespace cursive::core {

namespace {

#ifdef _WIN32

struct BundleState {
  bool initialized = false;
  bool success = false;
  std::filesystem::path support_root;
  std::filesystem::path dll_dir;
  std::filesystem::path tools_dir;
  std::filesystem::path linker_path;
  std::filesystem::path archiver_path;
  std::filesystem::path assembler_path;
  std::filesystem::path runtime_lib;
  std::filesystem::path delayimp_lib;
  std::string error_message;
};

std::once_flag g_bundle_once;
BundleState g_bundle_state;

enum class SupportLayoutKind {
  PackagedOut,
  LegacyBuildTree,
};

struct SupportLayout {
  SupportLayoutKind kind = SupportLayoutKind::PackagedOut;
  std::filesystem::path root;
};

std::string WideToNarrowLossy(std::wstring_view text) {
  std::string out;
  out.reserve(text.size());
  for (const wchar_t ch : text) {
    out.push_back((ch >= 0 && ch <= 0x7F) ? static_cast<char>(ch) : '?');
  }
  return out;
}

std::string WinErrorMessage(const std::string& context, DWORD error) {
  LPWSTR raw = nullptr;
  const DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER |
                      FORMAT_MESSAGE_FROM_SYSTEM |
                      FORMAT_MESSAGE_IGNORE_INSERTS;
  const DWORD len =
      FormatMessageW(flags, nullptr, error, 0, reinterpret_cast<LPWSTR>(&raw),
                     0, nullptr);

  std::string message = context;
  message += " (win32=";
  message += std::to_string(static_cast<unsigned long>(error));
  message += ")";

  if (len != 0 && raw != nullptr) {
    std::wstring_view wide(raw, len);
    while (!wide.empty() &&
           (wide.back() == L'\r' || wide.back() == L'\n' ||
            wide.back() == L' ')) {
      wide.remove_suffix(1);
    }
    message += ": ";
    message += WideToNarrowLossy(wide);
  }

  if (raw != nullptr) {
    LocalFree(raw);
  }
  return message;
}

std::filesystem::path CurrentExecutableDir() {
  wchar_t path[MAX_PATH];
  const DWORD len = GetModuleFileNameW(nullptr, path, MAX_PATH);
  if (len == 0 || len >= MAX_PATH) {
    return std::filesystem::path();
  }
  return std::filesystem::path(path).parent_path();
}

bool IsSupportRootCandidate(const std::filesystem::path& candidate) {
  if (candidate.empty()) {
    return false;
  }
  std::error_code ec;
  for (const char* rel : {"bin", "tools", "runtime", "lib"}) {
    ec.clear();
    if (std::filesystem::is_directory(candidate / rel, ec) && !ec) {
      return true;
    }
  }
  return false;
}

bool FileExists(const std::filesystem::path& path) {
  std::error_code ec;
  return std::filesystem::is_regular_file(path, ec) && !ec;
}

bool IsPackagedSupportRootCandidate(const std::filesystem::path& candidate) {
  if (candidate.empty()) {
    return false;
  }

  std::error_code ec;
  if (std::filesystem::is_directory(candidate / "windows", ec) && !ec) {
    return true;
  }

  return FileExists(candidate / "cursive0_rt.lib");
}

SupportLayout ResolveSupportLayout() {
  const auto executable_dir = CurrentExecutableDir();
  if (IsPackagedSupportRootCandidate(executable_dir)) {
    return {SupportLayoutKind::PackagedOut, executable_dir};
  }
  if (IsSupportRootCandidate(executable_dir)) {
    return {SupportLayoutKind::LegacyBuildTree, executable_dir};
  }
  const auto parent = executable_dir.parent_path();
  if (IsSupportRootCandidate(parent)) {
    return {SupportLayoutKind::LegacyBuildTree, parent};
  }
  return {SupportLayoutKind::PackagedOut, executable_dir};
}

bool ConfigureSidecarSupport(BundleState& state, std::string* error_message) {
  const SupportLayout layout = ResolveSupportLayout();
  state.support_root = layout.root;
  if (state.support_root.empty()) {
    if (error_message != nullptr) {
      *error_message = "Failed to resolve compiler sidecar root.";
    }
    return false;
  }

  if (layout.kind == SupportLayoutKind::PackagedOut) {
    const auto windows_root = state.support_root / "windows";
    state.dll_dir = windows_root / "bin";
    state.tools_dir = windows_root / "tools";
    state.linker_path = state.tools_dir / "lld-link.exe";
    state.archiver_path = state.tools_dir / "llvm-lib.exe";
    state.assembler_path = state.tools_dir / "llvm-as.exe";
    state.runtime_lib = state.support_root / "cursive0_rt.lib";
    state.delayimp_lib = windows_root / "lib" / "delayimp.lib";
  } else {
    state.dll_dir = state.support_root / "bin";
    state.tools_dir = state.support_root / "tools";
    state.linker_path = state.tools_dir / "lld-link.exe";
    state.archiver_path = state.tools_dir / "llvm-lib.exe";
    state.assembler_path = state.tools_dir / "llvm-as.exe";
    state.runtime_lib = state.support_root / "runtime" / "cursive0_rt.lib";
    state.delayimp_lib = state.support_root / "lib" / "delayimp.lib";
  }

  if (!std::filesystem::is_directory(state.dll_dir)) {
    if (error_message != nullptr) {
      *error_message = "Missing compiler sidecar directory: " +
                       state.dll_dir.string();
    }
    return false;
  }

  for (const auto& dll : {"icudt72.dll", "icuuc72.dll", "icuin72.dll"}) {
    const auto dll_path = state.dll_dir / dll;
    if (!FileExists(dll_path)) {
      if (error_message != nullptr) {
        *error_message = "Missing compiler sidecar file: " + dll_path.string();
      }
      return false;
    }
  }

  if (!SetDllDirectoryW(state.dll_dir.wstring().c_str())) {
    if (error_message != nullptr) {
      *error_message = WinErrorMessage(
          "Failed to configure compiler sidecar DLL directory '" +
              state.dll_dir.string() + "'.",
          GetLastError());
    }
    return false;
  }

  return true;
}

BundleState& BundleStateInstance() {
  std::call_once(g_bundle_once, []() {
    g_bundle_state.initialized = true;
    std::string error_message;
    g_bundle_state.success =
        ConfigureSidecarSupport(g_bundle_state, &error_message);
    if (!g_bundle_state.success) {
      g_bundle_state.error_message = std::move(error_message);
    }
  });
  return g_bundle_state;
}

#endif

}  // namespace

bool EnsureBundledWindowsCompilerSupport(std::string* error_message) {
#ifdef _WIN32
  BundleState& state = BundleStateInstance();
  if (!state.success && error_message != nullptr) {
    *error_message = state.error_message;
  }
  return state.success;
#else
  if (error_message != nullptr) {
    error_message->clear();
  }
  return true;
#endif
}

std::optional<std::filesystem::path> BundledWindowsToolsDirPath() {
#ifdef _WIN32
  BundleState& state = BundleStateInstance();
  if (!state.success || !std::filesystem::is_directory(state.tools_dir)) {
    return std::nullopt;
  }
  return state.tools_dir;
#else
  return std::nullopt;
#endif
}

std::optional<std::filesystem::path> BundledWindowsLinkerPath() {
#ifdef _WIN32
  BundleState& state = BundleStateInstance();
  if (!state.success || !FileExists(state.linker_path)) {
    return std::nullopt;
  }
  return state.linker_path;
#else
  return std::nullopt;
#endif
}

std::optional<std::filesystem::path> BundledWindowsArchiverPath() {
#ifdef _WIN32
  BundleState& state = BundleStateInstance();
  if (!state.success || !FileExists(state.archiver_path)) {
    return std::nullopt;
  }
  return state.archiver_path;
#else
  return std::nullopt;
#endif
}

std::optional<std::filesystem::path> BundledWindowsAssemblerPath() {
#ifdef _WIN32
  BundleState& state = BundleStateInstance();
  if (!state.success || !FileExists(state.assembler_path)) {
    return std::nullopt;
  }
  return state.assembler_path;
#else
  return std::nullopt;
#endif
}

std::optional<std::filesystem::path> BundledWindowsRuntimeLibPath() {
#ifdef _WIN32
  BundleState& state = BundleStateInstance();
  if (!state.success || !FileExists(state.runtime_lib)) {
    return std::nullopt;
  }
  return state.runtime_lib;
#else
  return std::nullopt;
#endif
}

std::optional<std::filesystem::path> BundledWindowsDelayImpLibPath() {
#ifdef _WIN32
  BundleState& state = BundleStateInstance();
  if (!state.success || !FileExists(state.delayimp_lib)) {
    return std::nullopt;
  }
  return state.delayimp_lib;
#else
  return std::nullopt;
#endif
}

}  // namespace cursive::core
