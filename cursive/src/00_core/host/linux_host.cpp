#include "host_internal.h"

#ifndef _WIN32

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <array>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace cursive::core {

namespace {

bool FileExists(const std::filesystem::path& path) {
  std::error_code ec;
  return std::filesystem::is_regular_file(path, ec) && !ec;
}

HostProcessResult RunProcessImpl(const HostProcessSpec& spec) {
  HostProcessResult result;
  if (spec.program.empty()) {
    result.error_message = "process program path is empty";
    return result;
  }

  int pipe_fds[2] = {-1, -1};
  if (spec.output_mode == HostProcessOutputMode::CaptureMerged) {
    if (pipe(pipe_fds) != 0) {
      result.error_message = "pipe failed";
      return result;
    }
  }

  std::vector<std::string> args;
  args.reserve(spec.arguments.size() + 1);
  args.push_back(spec.program.string());
  args.insert(args.end(), spec.arguments.begin(), spec.arguments.end());

  std::vector<char*> argv;
  argv.reserve(args.size() + 1);
  for (auto& arg : args) {
    argv.push_back(arg.data());
  }
  argv.push_back(nullptr);

  const pid_t pid = fork();
  if (pid < 0) {
    if (pipe_fds[0] >= 0) {
      close(pipe_fds[0]);
      close(pipe_fds[1]);
    }
    result.error_message = "fork failed";
    return result;
  }

  if (pid == 0) {
    if (spec.working_directory.has_value()) {
      (void)chdir(spec.working_directory->c_str());
    }
    if (spec.output_mode == HostProcessOutputMode::CaptureMerged) {
      close(pipe_fds[0]);
      dup2(pipe_fds[1], STDOUT_FILENO);
      dup2(pipe_fds[1], STDERR_FILENO);
      close(pipe_fds[1]);
    }
    execv(argv[0], argv.data());
    _exit(127);
  }

  result.launched = true;

  if (spec.output_mode == HostProcessOutputMode::CaptureMerged) {
    close(pipe_fds[1]);
    std::array<char, 4096> buffer{};
    ssize_t bytes_read = 0;
    while ((bytes_read = read(pipe_fds[0], buffer.data(), buffer.size())) > 0) {
      result.output.append(buffer.data(),
                           buffer.data() + static_cast<std::size_t>(bytes_read));
    }
    close(pipe_fds[0]);
  }

  int status = 0;
  if (waitpid(pid, &status, 0) < 0) {
    result.error_message = "waitpid failed";
    return result;
  }
  if (WIFEXITED(status)) {
    result.exit_code = WEXITSTATUS(status);
  } else {
    result.exit_code = -1;
  }
  return result;
}

HostTerminalInfo QueryTerminalImpl(FILE* stream) {
  HostTerminalInfo info;
  const int fd = fileno(stream);
  if (fd < 0 || isatty(fd) == 0) {
    return info;
  }
  info.is_tty = true;
  info.ansi_enabled = true;

  struct winsize w {};
  if (ioctl(fd, TIOCGWINSZ, &w) == 0 && w.ws_col > 0) {
    info.width = static_cast<int>(w.ws_col);
  }
  return info;
}

std::filesystem::path CompilerSidecarBinDirImpl(
    CompilerSupportLayoutKind layout,
    const std::filesystem::path& support_root) {
  if (layout == CompilerSupportLayoutKind::None || support_root.empty()) {
    return {};
  }
  if (layout == CompilerSupportLayoutKind::PackagedOut) {
    return support_root / "linux" / "bin";
  }
  return support_root / "bin";
}

}  // namespace

HostServices BuildLinuxHostServices() {
  HostServices services;
  services.getenv_utf8 = [](std::string_view name) -> std::optional<std::string> {
    if (name.empty()) {
      return std::nullopt;
    }
    const std::string name_copy(name);
    const char* value = std::getenv(name_copy.c_str());
    if (value == nullptr) {
      return std::nullopt;
    }
    return std::string(value);
  };
  services.path_list_separator = []() { return ':'; };
  services.tool_name_candidates = [](std::string_view tool) {
    return std::vector<std::string>{std::string(tool)};
  };
  services.current_executable_path = []() {
    std::array<char, 4096> path{};
    const ssize_t len = readlink("/proc/self/exe", path.data(),
                                 path.size() - 1);
    if (len <= 0) {
      return std::filesystem::path();
    }
    path[static_cast<std::size_t>(len)] = '\0';
    return std::filesystem::path(path.data());
  };
  services.current_process_id = []() {
    return static_cast<unsigned long>(getpid());
  };
  services.current_thread_id = []() {
    return static_cast<std::uint64_t>(syscall(SYS_gettid));
  };
  services.run_process = RunProcessImpl;
  services.terminal_info = QueryTerminalImpl;
  services.compiler_sidecar_bin_dir = CompilerSidecarBinDirImpl;
  services.configure_compiler_sidecar_bin_dir =
      [](const std::filesystem::path& sidecar_bin_dir,
         std::string* error_message) {
        const auto sidecar_lib_dir = sidecar_bin_dir.parent_path() / "lib";
        const auto icu_data = sidecar_lib_dir / "icudt72l.dat";
        if (FileExists(icu_data) &&
            setenv("ICU_DATA", sidecar_lib_dir.string().c_str(), 1) != 0) {
          if (error_message != nullptr) {
            *error_message = "setenv(ICU_DATA) failed";
          }
          return false;
        }
        if (error_message != nullptr) {
          error_message->clear();
        }
        return true;
      };
  return services;
}

const HostServices& NativeHostServices() {
  static const HostServices services = BuildLinuxHostServices();
  return services;
}

}  // namespace cursive::core

#endif
