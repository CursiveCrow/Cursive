#include "cursive0/project/link.h"

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <unordered_set>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

#include "cursive0/core/assert_spec.h"
#include "cursive0/core/diagnostic_messages.h"
#include "cursive0/core/host_primitives.h"
#include "cursive0/core/symbols.h"
#include "cursive0/project/outputs.h"
#include "cursive0/project/tool_resolution.h"

namespace cursive0::project {

namespace {

std::optional<core::Diagnostic> MakeExternalDiag(std::string_view code) {
  auto diag = core::MakeDiagnostic(code);
  if (diag) {
    diag->span.reset();
  }
  return diag;
}

void EmitExternal(core::DiagnosticStream& diags, std::string_view code) {
  if (auto diag = MakeExternalDiag(code)) {
    diags = core::Emit(diags, *diag);
  }
}

std::optional<std::string> ReadFileBytes(const std::filesystem::path& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    return std::nullopt;
  }
  std::ostringstream oss;
  oss << in.rdbuf();
  return oss.str();
}

uint16_t ReadU16(const unsigned char* data) {
  return static_cast<uint16_t>(data[0] | (data[1] << 8));
}

uint32_t ReadU32(const unsigned char* data) {
  return static_cast<uint32_t>(data[0] | (data[1] << 8) | (data[2] << 16) |
                               (data[3] << 24));
}

bool ParseDecimal(std::string_view field, std::size_t* value) {
  std::size_t acc = 0;
  bool any = false;
  for (char c : field) {
    if (c >= '0' && c <= '9') {
      any = true;
      acc = acc * 10 + static_cast<std::size_t>(c - '0');
    } else if (c == ' ' || c == '\0') {
      if (any) {
        break;
      }
    } else {
      if (any) {
        break;
      }
      return false;
    }
  }
  if (!any) {
    return false;
  }
  *value = acc;
  return true;
}

std::string TrimArchiveName(std::string_view name_field) {
  std::string name(name_field);
  while (!name.empty() && name.back() == ' ') {
    name.pop_back();
  }
  if (name.size() > 1 && name.back() == '/') {
    name.pop_back();
  }
  return name;
}

bool IsSpecialArchiveMember(std::string_view name) {
  return name == "/" || name == "//";
}

std::optional<std::string> CoffSymbolName(std::string_view bytes,
                                          std::size_t entry_offset,
                                          std::size_t string_table_offset,
                                          std::size_t string_table_size) {
  if (entry_offset + 8 > bytes.size()) {
    return std::nullopt;
  }
  const unsigned char* data =
      reinterpret_cast<const unsigned char*>(bytes.data() + entry_offset);
  bool long_name = data[0] == 0 && data[1] == 0 && data[2] == 0 && data[3] == 0;
  if (!long_name) {
    std::string name;
    name.reserve(8);
    for (std::size_t i = 0; i < 8; ++i) {
      char c = static_cast<char>(data[i]);
      if (c == '\0') {
        break;
      }
      name.push_back(c);
    }
    if (name.empty()) {
      return std::nullopt;
    }
    return name;
  }
  if (entry_offset + 8 > bytes.size()) {
    return std::nullopt;
  }
  const uint32_t offset = ReadU32(data + 4);
  if (offset < 4) {
    return std::nullopt;
  }
  const std::size_t start = string_table_offset + offset;
  const std::size_t end = string_table_offset + string_table_size;
  if (start >= bytes.size()) {
    return std::nullopt;
  }
  std::size_t limit = bytes.size();
  if (end > start && end <= bytes.size()) {
    limit = end;
  }
  std::string name;
  for (std::size_t i = start; i < limit; ++i) {
    char c = bytes[i];
    if (c == '\0') {
      break;
    }
    name.push_back(c);
  }
  if (name.empty()) {
    return std::nullopt;
  }
  return name;
}

bool ParseCoffSymbols(std::string_view bytes,
                      std::vector<std::string>& symbols) {
  if (bytes.size() < 20) {
    return false;
  }
  const unsigned char* data =
      reinterpret_cast<const unsigned char*>(bytes.data());
  const uint16_t sig1 = ReadU16(data);
  const uint16_t sig2 = ReadU16(data + 2);
  if (sig1 == 0 && sig2 == 0xFFFFu) {
    return true;
  }
  const uint32_t sym_table = ReadU32(data + 8);
  const uint32_t sym_count = ReadU32(data + 12);
  if (sym_table == 0 || sym_count == 0) {
    return true;
  }
  const std::size_t sym_table_end =
      sym_table + static_cast<std::size_t>(sym_count) * 18;
  if (sym_table_end > bytes.size()) {
    return false;
  }
  std::size_t string_table_offset = sym_table_end;
  std::size_t string_table_size = 0;
  if (string_table_offset + 4 <= bytes.size()) {
    string_table_size = ReadU32(reinterpret_cast<const unsigned char*>(
        bytes.data() + string_table_offset));
  }

  uint32_t i = 0;
  while (i < sym_count) {
    const std::size_t entry_offset = sym_table + i * 18;
    if (entry_offset + 18 > bytes.size()) {
      return false;
    }
    const unsigned char* entry =
        reinterpret_cast<const unsigned char*>(bytes.data() + entry_offset);
    const int16_t section = static_cast<int16_t>(ReadU16(entry + 12));
    const uint8_t aux = entry[17];
    if (section > 0 || section == static_cast<int16_t>(-1)) {
      const auto name = CoffSymbolName(bytes, entry_offset, string_table_offset,
                                       string_table_size);
      if (name.has_value()) {
        symbols.push_back(*name);
      }
    }
    i += 1 + static_cast<uint32_t>(aux);
  }
  return true;
}

bool ParseArchiveSymbols(std::string_view bytes,
                         std::vector<std::string>& symbols) {
  if (bytes.size() < 8 || bytes.substr(0, 8) != "!<arch>\n") {
    return false;
  }
  std::size_t offset = 8;
  while (offset + 60 <= bytes.size()) {
    const std::string_view header(bytes.data() + offset, 60);
    const std::string name = TrimArchiveName(header.substr(0, 16));
    std::size_t size = 0;
    if (!ParseDecimal(header.substr(48, 10), &size)) {
      return false;
    }
    const std::size_t data_offset = offset + 60;
    if (data_offset + size > bytes.size()) {
      return false;
    }
    if (!IsSpecialArchiveMember(name)) {
      const std::string_view member(bytes.data() + data_offset, size);
      if (!ParseCoffSymbols(member, symbols)) {
        return false;
      }
    }
    offset = data_offset + size;
    if (offset % 2 == 1) {
      ++offset;
    }
  }
  return true;
}

std::optional<std::vector<std::string>> LinkerSymsForInputs(
    const std::vector<std::filesystem::path>& inputs) {
  std::unordered_set<std::string> seen;
  for (const auto& input : inputs) {
    const auto bytes = ReadFileBytes(input);
    if (!bytes.has_value()) {
      return std::nullopt;
    }
    std::vector<std::string> symbols;
    if (bytes->size() >= 8 && bytes->substr(0, 8) == "!<arch>\n") {
      if (!ParseArchiveSymbols(*bytes, symbols)) {
        return std::nullopt;
      }
    } else {
      if (!ParseCoffSymbols(*bytes, symbols)) {
        return std::nullopt;
      }
    }
    for (const auto& sym : symbols) {
      seen.insert(sym);
    }
  }
  std::vector<std::string> out;
  out.reserve(seen.size());
  for (const auto& sym : seen) {
    out.push_back(sym);
  }
  std::sort(out.begin(), out.end());
  return out;
}

bool MissingRuntimeSym(const std::vector<std::string>& syms) {
  const auto required = RuntimeRequiredSyms();
  std::unordered_set<std::string> table;
  table.reserve(syms.size());
  for (const auto& sym : syms) {
    table.insert(sym);
  }
  for (const auto& req : required) {
    if (table.find(req) == table.end()) {
      return true;
    }
  }
  return false;
}

std::optional<std::filesystem::path> ResolveToolDefault(
    const Project& project,
    std::string_view tool) {
  return ResolveTool(project, tool);
}

bool InvokeLinkerDefault(const std::filesystem::path& tool,
                         const std::vector<std::filesystem::path>& inputs,
                         const std::filesystem::path& exe);

}  // namespace

std::filesystem::path RuntimeLibPath(const Project& project) {
  return project.root / "runtime" / "cursive0_rt.lib";
}

std::vector<std::string> RuntimeRequiredSyms() {
  std::vector<std::string> syms;
  syms.push_back(core::PathSig({"cursive", "runtime", "panic"}));
  syms.push_back(core::PathSig({"cursive", "runtime", "string", "drop_managed"}));
  syms.push_back(core::PathSig({"cursive", "runtime", "bytes", "drop_managed"}));
  syms.push_back(core::PathSig({"cursive", "runtime", "context_init"}));
  syms.push_back(core::PathSig({"cursive", "runtime", "spec_trace", "emit"}));

  const std::string_view region_procs[] = {
      "new_scoped",
      "alloc",
      "reset_unchecked",
      "freeze",
      "thaw",
      "free_unchecked",
  };
  for (const auto& proc : region_procs) {
    syms.push_back(core::PathSig({"cursive", "runtime", "region", proc}));
  }

  const std::string_view string_builtins[] = {
      "from",
      "as_view",
      "to_managed",
      "clone_with",
      "append",
      "length",
      "is_empty",
  };
  for (const auto& name : string_builtins) {
    syms.push_back(core::PathSig({"cursive", "runtime", "string", name}));
  }

  const std::string_view bytes_builtins[] = {
      "with_capacity",
      "from_slice",
      "as_view",
      "to_managed",
      "view",
      "view_string",
      "append",
      "length",
      "is_empty",
  };
  for (const auto& name : bytes_builtins) {
    syms.push_back(core::PathSig({"cursive", "runtime", "bytes", name}));
  }

  const std::string_view fs_methods[] = {
      "open_read",
      "open_write",
      "open_append",
      "create_write",
      "read_file",
      "read_bytes",
      "write_file",
      "write_stdout",
      "write_stderr",
      "exists",
      "remove",
      "open_dir",
      "create_dir",
      "ensure_dir",
      "kind",
      "restrict",
  };
  for (const auto& name : fs_methods) {
    syms.push_back(core::PathSig({"cursive", "runtime", "fs", name}));
  }

  const std::string_view heap_methods[] = {
      "with_quota",
      "alloc_raw",
      "dealloc_raw",
  };
  for (const auto& name : heap_methods) {
    syms.push_back(core::PathSig({"cursive", "runtime", "heap", name}));
  }

  std::sort(syms.begin(), syms.end());
  syms.erase(std::unique(syms.begin(), syms.end()), syms.end());
  return syms;
}

std::optional<std::filesystem::path> ResolveRuntimeLib(const Project& project) {
  const auto path = RuntimeLibPath(project);
  const auto bytes = ReadFileBytes(path);
  if (!bytes.has_value()) {
    SPEC_RULE("ResolveRuntimeLib-Err");
    core::HostPrimFail(core::HostPrim::ResolveRuntimeLib, true);
    return std::nullopt;
  }
  SPEC_RULE("ResolveRuntimeLib-Ok");
  return path;
}

std::optional<std::vector<std::string>> LinkerSyms(
    const std::filesystem::path&,
    const std::vector<std::filesystem::path>& inputs,
    const std::filesystem::path&) {
  return LinkerSymsForInputs(inputs);
}

bool InvokeLinker(const std::filesystem::path& tool,
                  const std::vector<std::filesystem::path>& inputs,
                  const std::filesystem::path& exe) {
#ifdef _WIN32
  auto quote_arg = [](std::wstring_view arg) -> std::wstring {
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
  };

  std::vector<std::wstring> args;
  args.reserve(inputs.size() + 5);
  args.push_back(tool.wstring());
  args.push_back(L"/OUT:" + exe.wstring());
  args.push_back(L"/ENTRY:main");
  args.push_back(L"/SUBSYSTEM:CONSOLE");
  args.push_back(L"/NODEFAULTLIB");
  for (const auto& input : inputs) {
    args.push_back(input.wstring());
  }

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

  const std::wstring tool_path = tool.wstring();
  const BOOL ok = CreateProcessW(tool_path.c_str(), cmd_buf.data(), nullptr,
                                 nullptr, TRUE, CREATE_NO_WINDOW, nullptr,
                                 nullptr, &si, &pi);
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
  args.reserve(inputs.size() + 5);
  args.push_back(tool.string());
  args.push_back("/OUT:" + exe.string());
  args.push_back("/ENTRY:main");
  args.push_back("/SUBSYSTEM:CONSOLE");
  args.push_back("/NODEFAULTLIB");
  for (const auto& input : inputs) {
    args.push_back(input.string());
  }

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
    _exit(127);
  }
  int status = 0;
  if (waitpid(pid, &status, 0) < 0) {
    return false;
  }
  return WIFEXITED(status) && WEXITSTATUS(status) == 0;
#endif
}

namespace {

bool InvokeLinkerDefault(const std::filesystem::path& tool,
                         const std::vector<std::filesystem::path>& inputs,
                         const std::filesystem::path& exe) {
  return InvokeLinker(tool, inputs, exe);
}

}  // namespace

LinkResult LinkWithDeps(const std::vector<std::filesystem::path>& objs,
                        const Project& project,
                        const LinkDeps& deps) {
  LinkResult result;

  auto tool = deps.resolve_tool(project, "lld-link");
  if (!tool.has_value()) {
    tool = deps.resolve_tool(project, "link");
  }
  if (!tool.has_value()) {
    SPEC_RULE("Link-NotFound");
    EmitExternal(result.diags, "E-OUT-0405");
    result.status = LinkStatus::NotFound;
    return result;
  }

  const auto runtime_lib = deps.resolve_runtime_lib(project);
  if (!runtime_lib.has_value()) {
    SPEC_RULE("Link-Runtime-Missing");
    EmitExternal(result.diags, "E-OUT-0407");
    result.status = LinkStatus::RuntimeMissing;
    return result;
  }

  std::vector<std::filesystem::path> inputs = objs;
  inputs.push_back(*runtime_lib);
  const auto exe = ExePath(project);

  const auto syms = deps.linker_syms(*tool, inputs, exe);
  if (!syms.has_value() || MissingRuntimeSym(*syms)) {
    SPEC_RULE("Link-Runtime-Incompatible");
    EmitExternal(result.diags, "E-OUT-0408");
    result.status = LinkStatus::RuntimeIncompatible;
    return result;
  }

  if (!deps.invoke_linker(*tool, inputs, exe)) {
    core::HostPrimFail(core::HostPrim::InvokeLinker, true);
    SPEC_RULE("Link-Fail");
    EmitExternal(result.diags, "E-OUT-0404");
    result.status = LinkStatus::Fail;
    return result;
  }

  SPEC_RULE("Link-Ok");
  result.status = LinkStatus::Ok;
  return result;
}

LinkResult Link(const std::vector<std::filesystem::path>& objs,
                const Project& project) {
  const LinkDeps deps = {
      ResolveToolDefault,
      ResolveRuntimeLib,
      LinkerSyms,
      InvokeLinkerDefault,
  };
  return LinkWithDeps(objs, project, deps);
}

}  // namespace cursive0::project
