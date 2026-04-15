#include "01_project/ir_assembly.h"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/wait.h>
#include <unistd.h>
#endif

#include "00_core/assert_spec.h"
#include "00_core/crash_debug.h"
#include "00_core/host_primitives.h"

namespace cursive::project {

namespace {

std::filesystem::path MakeTempAsmPath(std::string_view extension) {
  static std::atomic<unsigned long long> counter{0};
  const auto stamp = std::chrono::high_resolution_clock::now()
                         .time_since_epoch()
                         .count();
  const auto id = counter.fetch_add(1, std::memory_order_relaxed);
  auto path = std::filesystem::temp_directory_path() /
              ("cursive-llvm-as-" + std::to_string(stamp) + "-" +
               std::to_string(id));
  path.replace_extension(std::string(extension));
  return path;
}

bool WriteBytes(const std::filesystem::path& path, std::string_view bytes) {
  std::ofstream out(path, std::ios::binary | std::ios::trunc);
  if (!out) {
    return false;
  }
  out.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
  return out.good();
}

std::optional<std::string> ReadBytes(const std::filesystem::path& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    return std::nullopt;
  }
  std::string bytes((std::istreambuf_iterator<char>(in)),
                    std::istreambuf_iterator<char>());
  if (!in.good() && !in.eof()) {
    return std::nullopt;
  }
  return bytes;
}

void RemoveTempFile(const std::filesystem::path& path) {
  std::error_code ec;
  std::filesystem::remove(path, ec);
}

bool RunAssemblerProcess(const std::filesystem::path& tool,
                         const std::filesystem::path& input_path,
                         const std::filesystem::path& output_path) {
#ifdef _WIN32
  if (core::CrashReportingEnabled()) {
    core::DebugRunOptions options;
    options.program = tool;
    options.working_directory = output_path.parent_path();
    options.report_root = core::DefaultTargetCrashReportRoot(output_path);
    options.tool_name = "cursive-llvm-as";
    options.arguments.push_back(input_path.generic_string());
    options.arguments.push_back("-o");
    options.arguments.push_back(output_path.generic_string());
    const auto result = core::DebugRunProcess(options);
    return result.launched && result.exit_code == 0;
  }
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
  args.push_back(input_path.wstring());
  args.push_back(L"-o");
  args.push_back(output_path.wstring());

  std::wstring cmd;
  for (std::size_t i = 0; i < args.size(); ++i) {
    if (i != 0) {
      cmd.push_back(L' ');
    }
    cmd += quote_arg(args[i]);
  }

  STARTUPINFOW si;
  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(si);
  PROCESS_INFORMATION pi;
  ZeroMemory(&pi, sizeof(pi));

  std::vector<wchar_t> cmd_buf(cmd.begin(), cmd.end());
  cmd_buf.push_back(L'\0');

  const BOOL ok = CreateProcessW(tool.wstring().c_str(), cmd_buf.data(),
                                 nullptr, nullptr, FALSE, CREATE_NO_WINDOW,
                                 nullptr, nullptr, &si, &pi);
  if (!ok) {
    return false;
  }

  WaitForSingleObject(pi.hProcess, INFINITE);
  DWORD exit_code = 1;
  GetExitCodeProcess(pi.hProcess, &exit_code);
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);
  return exit_code == 0;
#else
  std::vector<std::string> args;
  args.push_back(tool.string());
  args.push_back(input_path.string());
  args.push_back("-o");
  args.push_back(output_path.string());

  std::vector<char*> argv;
  argv.reserve(args.size() + 1);
  for (auto& arg : args) {
    argv.push_back(arg.data());
  }
  argv.push_back(nullptr);

  const pid_t pid = fork();
  if (pid < 0) {
    return false;
  }
  if (pid == 0) {
    execv(argv[0], argv.data());
    std::perror("execv");
    _exit(127);
  }

  int status = 0;
  if (waitpid(pid, &status, 0) < 0) {
    return false;
  }
  return WIFEXITED(status) && WEXITSTATUS(status) == 0;
#endif
}

std::optional<std::string> AssembleIRViaTool(const std::filesystem::path& tool,
                                             std::string_view ir_text) {
  const auto input_path = MakeTempAsmPath(".ll");
  const auto output_path = MakeTempAsmPath(".bc");

  if (!WriteBytes(input_path, ir_text)) {
    RemoveTempFile(input_path);
    RemoveTempFile(output_path);
    return std::nullopt;
  }

  if (!RunAssemblerProcess(tool, input_path, output_path)) {
    RemoveTempFile(input_path);
    RemoveTempFile(output_path);
    return std::nullopt;
  }

  auto bytes = ReadBytes(output_path);
  RemoveTempFile(input_path);
  RemoveTempFile(output_path);
  return bytes;
}

}  // namespace

std::optional<std::string> AssembleIRWithDeps(const std::filesystem::path& tool,
                                              std::string_view ir_text,
                                              InvokeFn invoke) {
  std::optional<std::string> bytes;
  if (invoke != nullptr) {
    bytes = invoke(tool, ir_text);
  } else {
    bytes = AssembleIRViaTool(tool, ir_text);
  }
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
  return AssembleIRWithDeps(tool, ir_text, nullptr);
}

}  // namespace cursive::project
