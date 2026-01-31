#include "cursive0/01_project/ir_assembly.h"

#include <atomic>
#include <string>
#include <thread>
#include <vector>

#include "cursive0/00_core/assert_spec.h"
#include "cursive0/00_core/host_primitives.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace cursive0::project {

namespace {

#ifdef _WIN32

std::wstring QuoteWinArg(std::wstring_view arg) {
  if (arg.empty()) {
    return L"\"\"";
  }
  bool needs_quotes = false;
  for (wchar_t c : arg) {
    if (c == L' ' || c == L'\t' || c == L'\"') {
      needs_quotes = true;
      break;
    }
  }
  if (!needs_quotes) {
    return std::wstring(arg);
  }
  std::wstring out;
  out.push_back(L'\"');
  int backslashes = 0;
  for (wchar_t c : arg) {
    if (c == L'\\') {
      ++backslashes;
      continue;
    }
    if (c == L'\"') {
      out.append(backslashes * 2 + 1, L'\\');
      out.push_back(L'\"');
      backslashes = 0;
      continue;
    }
    if (backslashes > 0) {
      out.append(backslashes, L'\\');
      backslashes = 0;
    }
    out.push_back(c);
  }
  if (backslashes > 0) {
    out.append(backslashes * 2, L'\\');
  }
  out.push_back(L'\"');
  return out;
}

std::wstring BuildCommandLine(const std::filesystem::path& tool) {
  const std::wstring tool_arg = tool.wstring();
  const std::wstring args[] = {tool_arg, L"-o", L"-", L"-"};
  std::wstring cmd;
  for (std::size_t i = 0; i < 4; ++i) {
    if (i != 0) {
      cmd.push_back(L' ');
    }
    cmd += QuoteWinArg(args[i]);
  }
  return cmd;
}

bool WriteAll(HANDLE handle, std::string_view data) {
  std::size_t offset = 0;
  while (offset < data.size()) {
    const std::size_t remaining = data.size() - offset;
    const DWORD chunk = static_cast<DWORD>(
        remaining > 1 << 20 ? 1 << 20 : remaining);
    DWORD written = 0;
    if (!WriteFile(handle, data.data() + offset, chunk, &written, nullptr)) {
      return false;
    }
    if (written == 0) {
      return false;
    }
    offset += written;
  }
  return true;
}

std::optional<std::string> InvokeDefault(const std::filesystem::path& tool,
                                         std::string_view input) {
  SECURITY_ATTRIBUTES sa;
  sa.nLength = sizeof(SECURITY_ATTRIBUTES);
  sa.bInheritHandle = TRUE;
  sa.lpSecurityDescriptor = nullptr;

  HANDLE stdin_read = nullptr;
  HANDLE stdin_write = nullptr;
  HANDLE stdout_read = nullptr;
  HANDLE stdout_write = nullptr;

  if (!CreatePipe(&stdin_read, &stdin_write, &sa, 0)) {
    core::HostPrimFail(core::HostPrim::Invoke, true);
    return std::nullopt;
  }
  if (!CreatePipe(&stdout_read, &stdout_write, &sa, 0)) {
    CloseHandle(stdin_read);
    CloseHandle(stdin_write);
    core::HostPrimFail(core::HostPrim::Invoke, true);
    return std::nullopt;
  }

  if (!SetHandleInformation(stdin_write, HANDLE_FLAG_INHERIT, 0) ||
      !SetHandleInformation(stdout_read, HANDLE_FLAG_INHERIT, 0)) {
    CloseHandle(stdin_read);
    CloseHandle(stdin_write);
    CloseHandle(stdout_read);
    CloseHandle(stdout_write);
    core::HostPrimFail(core::HostPrim::Invoke, true);
    return std::nullopt;
  }

  HANDLE stderr_handle = CreateFileW(L"NUL", GENERIC_WRITE,
                                     FILE_SHARE_READ | FILE_SHARE_WRITE, &sa,
                                     OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                                     nullptr);
  if (stderr_handle == INVALID_HANDLE_VALUE) {
    CloseHandle(stdin_read);
    CloseHandle(stdin_write);
    CloseHandle(stdout_read);
    CloseHandle(stdout_write);
    core::HostPrimFail(core::HostPrim::Invoke, true);
    return std::nullopt;
  }

  STARTUPINFOW si;
  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(si);
  si.dwFlags = STARTF_USESTDHANDLES;
  si.hStdInput = stdin_read;
  si.hStdOutput = stdout_write;
  si.hStdError = stderr_handle;

  PROCESS_INFORMATION pi;
  ZeroMemory(&pi, sizeof(pi));

  std::wstring cmd = BuildCommandLine(tool);
  std::vector<wchar_t> cmd_buf(cmd.begin(), cmd.end());
  cmd_buf.push_back(L'\0');

  const std::wstring tool_path = tool.wstring();
  const BOOL ok = CreateProcessW(tool_path.c_str(), cmd_buf.data(), nullptr,
                                 nullptr, TRUE, CREATE_NO_WINDOW, nullptr,
                                 nullptr, &si, &pi);

  CloseHandle(stdin_read);
  CloseHandle(stdout_write);
  CloseHandle(stderr_handle);

  if (!ok) {
    CloseHandle(stdin_write);
    CloseHandle(stdout_read);
    core::HostPrimFail(core::HostPrim::Invoke, true);
    return std::nullopt;
  }

  std::string output;
  std::atomic<bool> read_ok(true);
  std::thread reader([&]() {
    char buffer[4096];
    DWORD read = 0;
    while (true) {
      const BOOL r = ReadFile(stdout_read, buffer, sizeof(buffer), &read, nullptr);
      if (!r) {
        const DWORD err = GetLastError();
        if (err == ERROR_BROKEN_PIPE) {
          break;
        }
        read_ok.store(false);
        break;
      }
      if (read == 0) {
        break;
      }
      output.append(buffer, buffer + read);
    }
  });

  const bool write_ok = WriteAll(stdin_write, input);
  CloseHandle(stdin_write);

  WaitForSingleObject(pi.hProcess, INFINITE);
  DWORD exit_code = 1;
  GetExitCodeProcess(pi.hProcess, &exit_code);
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);

  reader.join();
  CloseHandle(stdout_read);

  if (!write_ok || !read_ok.load() || exit_code != 0) {
    core::HostPrimFail(core::HostPrim::Invoke, true);
    return std::nullopt;
  }

  return output;
}

#else

bool WriteAll(int fd, std::string_view data) {
  std::size_t offset = 0;
  while (offset < data.size()) {
    const std::size_t remaining = data.size() - offset;
    const std::size_t chunk = remaining > 1 << 20 ? 1 << 20 : remaining;
    const ssize_t written = write(fd, data.data() + offset, chunk);
    if (written < 0) {
      if (errno == EINTR) {
        continue;
      }
      return false;
    }
    if (written == 0) {
      return false;
    }
    offset += static_cast<std::size_t>(written);
  }
  return true;
}

std::optional<std::string> InvokeDefault(const std::filesystem::path& tool,
                                         std::string_view input) {
  int stdin_pipe[2] = {-1, -1};
  int stdout_pipe[2] = {-1, -1};
  if (pipe(stdin_pipe) != 0) {
    core::HostPrimFail(core::HostPrim::Invoke, true);
    return std::nullopt;
  }
  if (pipe(stdout_pipe) != 0) {
    close(stdin_pipe[0]);
    close(stdin_pipe[1]);
    core::HostPrimFail(core::HostPrim::Invoke, true);
    return std::nullopt;
  }

  const pid_t pid = fork();
  if (pid < 0) {
    close(stdin_pipe[0]);
    close(stdin_pipe[1]);
    close(stdout_pipe[0]);
    close(stdout_pipe[1]);
    core::HostPrimFail(core::HostPrim::Invoke, true);
    return std::nullopt;
  }

  if (pid == 0) {
    dup2(stdin_pipe[0], STDIN_FILENO);
    dup2(stdout_pipe[1], STDOUT_FILENO);
    const int devnull = open("/dev/null", O_WRONLY);
    if (devnull >= 0) {
      dup2(devnull, STDERR_FILENO);
      close(devnull);
    }
    close(stdin_pipe[0]);
    close(stdin_pipe[1]);
    close(stdout_pipe[0]);
    close(stdout_pipe[1]);

    std::string tool_str = tool.string();
    std::vector<std::string> args;
    args.push_back(tool_str);
    args.push_back("-o");
    args.push_back("-");
    args.push_back("-");
    std::vector<char*> argv;
    argv.reserve(args.size() + 1);
    for (auto& arg : args) {
      argv.push_back(arg.data());
    }
    argv.push_back(nullptr);
    execv(argv[0], argv.data());
    _exit(127);
  }

  close(stdin_pipe[0]);
  close(stdout_pipe[1]);

  std::string output;
  std::atomic<bool> read_ok(true);
  std::thread reader([&]() {
    char buffer[4096];
    while (true) {
      const ssize_t n = read(stdout_pipe[0], buffer, sizeof(buffer));
      if (n > 0) {
        output.append(buffer, buffer + n);
        continue;
      }
      if (n == 0) {
        break;
      }
      if (errno == EINTR) {
        continue;
      }
      read_ok.store(false);
      break;
    }
  });

  const bool write_ok = WriteAll(stdin_pipe[1], input);
  close(stdin_pipe[1]);

  int status = 0;
  waitpid(pid, &status, 0);

  reader.join();
  close(stdout_pipe[0]);

  if (!write_ok || !read_ok.load() || !WIFEXITED(status) || WEXITSTATUS(status) != 0) {
    core::HostPrimFail(core::HostPrim::Invoke, true);
    return std::nullopt;
  }

  return output;
}

#endif

}  // namespace

std::optional<std::string> AssembleIRWithDeps(const std::filesystem::path& tool,
                                              std::string_view ir_text,
                                              InvokeFn invoke) {
  const auto bytes = invoke(tool, ir_text);
  if (!bytes.has_value()) {
    core::HostPrimFail(core::HostPrim::AssembleIR, true);
    SPEC_RULE("AssembleIR-Err");
    return std::nullopt;
  }
  SPEC_RULE("AssembleIR-Ok");
  return bytes;
}

std::optional<std::string> AssembleIR(const std::filesystem::path& tool,
                                      std::string_view ir_text) {
  return AssembleIRWithDeps(tool, ir_text, InvokeDefault);
}

}  // namespace cursive0::project
