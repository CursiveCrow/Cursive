#include "01_project/link.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <unordered_set>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

#include "00_core/assert_spec.h"
#include "00_core/crash_debug.h"
#include "00_core/diagnostic_messages.h"
#include "00_core/host_primitives.h"
#include "00_core/process_config.h"
#include "00_core/windows_bundle.h"
#include "01_project/outputs.h"
#include "01_project/project.h"
#include "01_project/target_profile.h"
#include "01_project/tool_resolution.h"
#include "05_codegen/intrinsics/builtins.h"

namespace cursive::project {

namespace {

constexpr std::string_view kLibraryEntrySym = "__cursive_library_entry";
constexpr std::string_view kDelayImpLibraryName = "delayimp.lib";
constexpr std::uint64_t kWindowsExeStackReserveBytes = 1ull << 20;
constexpr std::uint64_t kWindowsExeStackCommitBytes = 64ull << 10;

#ifdef _WIN32
std::filesystem::path DelayImpLibraryPath() {
  if (const auto bundled = core::BundledWindowsDelayImpLibPath();
      bundled.has_value()) {
    return *bundled;
  }
  return std::filesystem::path(std::string(kDelayImpLibraryName));
}
#endif
constexpr std::string_view kRuntimeInitPrefix =
    "cursive_x3a_x3aruntime_x3a_x3ainit_x3a_x3a";
constexpr std::string_view kRuntimeDeinitPrefix =
    "cursive_x3a_x3aruntime_x3a_x3adeinit_x3a_x3a";

bool IsHiddenSharedLibraryExportSymbolImpl(std::string_view symbol) {
  return symbol == kLibraryEntrySym ||
         symbol.rfind("__cx_", 0) == 0 ||
         symbol.rfind(kRuntimeInitPrefix, 0) == 0 ||
         symbol.rfind(kRuntimeDeinitPrefix, 0) == 0 ||
         symbol.ends_with("$resume");
}

bool LinkDebugEnabled() {
  return core::LinkDebugOverride().value_or(core::IsDebugEnabled("link"));
}

void EmitExternal(core::DiagnosticStream& diags, std::string_view code) {
  core::EmitExternalDiagnostic(diags, code);
}

bool IsSharedLibraryProject(const Project& project) {
  return project.assembly.kind == "library" &&
         project.assembly.link_kind == "shared";
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

bool CanReadFile(const std::filesystem::path& path) {
  std::ifstream in(path, std::ios::binary);
  return static_cast<bool>(in);
}

bool IsLinkerResolvedLibraryName(const std::filesystem::path& path) {
  return !path.empty() && !path.has_parent_path() && !path.is_absolute();
}

uint16_t ReadU16(const unsigned char* data) {
  return static_cast<uint16_t>(data[0] | (data[1] << 8));
}

uint32_t ReadU32(const unsigned char* data) {
  return static_cast<uint32_t>(data[0] | (data[1] << 8) | (data[2] << 16) |
                               (data[3] << 24));
}

uint64_t ReadU64(const unsigned char* data) {
  return static_cast<uint64_t>(data[0]) |
         (static_cast<uint64_t>(data[1]) << 8) |
         (static_cast<uint64_t>(data[2]) << 16) |
         (static_cast<uint64_t>(data[3]) << 24) |
         (static_cast<uint64_t>(data[4]) << 32) |
         (static_cast<uint64_t>(data[5]) << 40) |
         (static_cast<uint64_t>(data[6]) << 48) |
         (static_cast<uint64_t>(data[7]) << 56);
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

constexpr uint8_t kCoffStorageClassExternal = 2;

bool IsCoffBackendHelperSymbolForDuplicateScan(std::string_view sym) {
  return sym.rfind("__real@", 0) == 0 ||
         sym.rfind("__xmm@", 0) == 0 ||
         sym.rfind("__ymm@", 0) == 0 ||
         sym.rfind("__zmm@", 0) == 0;
}

bool IsArchiveBytes(std::string_view bytes) {
  return bytes.size() >= 8 && bytes.substr(0, 8) == "!<arch>\n";
}

bool IsElfBytes(std::string_view bytes) {
  return bytes.size() >= 4 &&
         static_cast<unsigned char>(bytes[0]) == 0x7F &&
         bytes[1] == 'E' && bytes[2] == 'L' && bytes[3] == 'F';
}

bool ParseCoffSymbols(std::string_view bytes,
                      std::vector<std::string>& symbols,
                      bool defined_external_only) {
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
    const uint8_t storage_class = entry[16];
    const uint8_t aux = entry[17];
    const bool is_defined = section > 0 || section == static_cast<int16_t>(-1);
    const bool include_symbol =
        !defined_external_only || storage_class == kCoffStorageClassExternal;
    if (is_defined && include_symbol) {
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

bool ParseElfSymbols(std::string_view bytes,
                     std::vector<std::string>& symbols,
                     bool defined_external_only) {
  if (!IsElfBytes(bytes) || bytes.size() < 64) {
    return false;
  }
  const unsigned char* data =
      reinterpret_cast<const unsigned char*>(bytes.data());
  constexpr std::size_t kEIClass = 4;
  constexpr std::size_t kEIData = 5;
  constexpr unsigned char kElfClass64 = 2;
  constexpr unsigned char kElfDataLittle = 1;
  constexpr uint32_t kSHTSymTab = 2;
  constexpr uint32_t kSHTDynSym = 11;
  constexpr uint16_t kSHNUndef = 0;
  constexpr uint8_t kSTBLocal = 0;
  constexpr uint8_t kSTBGlobal = 1;
  constexpr uint8_t kSTBWeak = 2;

  if (data[kEIClass] != kElfClass64 || data[kEIData] != kElfDataLittle) {
    return false;
  }

  const std::size_t shoff = static_cast<std::size_t>(ReadU64(data + 40));
  const std::size_t shentsize = ReadU16(data + 58);
  const std::size_t shnum = ReadU16(data + 60);
  if (shoff > bytes.size() ||
      shentsize == 0 ||
      shoff + shentsize * shnum > bytes.size()) {
    return false;
  }

  for (std::size_t i = 0; i < shnum; ++i) {
    const std::size_t sh_offset = shoff + i * shentsize;
    const unsigned char* sh = data + sh_offset;
    const uint32_t type = ReadU32(sh + 4);
    if (type != kSHTSymTab && type != kSHTDynSym) {
      continue;
    }

    const std::size_t section_offset = static_cast<std::size_t>(ReadU64(sh + 24));
    const std::size_t section_size = static_cast<std::size_t>(ReadU64(sh + 32));
    const uint32_t link = ReadU32(sh + 40);
    const std::size_t entsize = static_cast<std::size_t>(ReadU64(sh + 56));
    if (section_offset > bytes.size() ||
        section_size > bytes.size() - section_offset ||
        entsize == 0 ||
        section_size % entsize != 0 ||
        link >= shnum) {
      return false;
    }

    const unsigned char* linked_section =
        data + shoff + static_cast<std::size_t>(link) * shentsize;
    const std::size_t str_offset =
        static_cast<std::size_t>(ReadU64(linked_section + 24));
    const std::size_t str_size =
        static_cast<std::size_t>(ReadU64(linked_section + 32));
    if (str_offset > bytes.size() || str_size > bytes.size() - str_offset) {
      return false;
    }
    const char* strtab = bytes.data() + str_offset;

    const std::size_t count = section_size / entsize;
    for (std::size_t sym_index = 0; sym_index < count; ++sym_index) {
      const unsigned char* sym =
          data + section_offset + sym_index * entsize;
      const uint32_t name_offset = ReadU32(sym);
      const uint8_t info = sym[4];
      const uint16_t shndx = ReadU16(sym + 6);
      if (name_offset >= str_size || shndx == kSHNUndef) {
        continue;
      }
      const uint8_t binding = static_cast<uint8_t>(info >> 4);
      if (defined_external_only &&
          binding != kSTBGlobal &&
          binding != kSTBWeak &&
          binding != kSTBLocal) {
        continue;
      }
      if (defined_external_only &&
          binding != kSTBGlobal &&
          binding != kSTBWeak) {
        continue;
      }

      std::string name;
      for (std::size_t pos = name_offset; pos < str_size; ++pos) {
        const char c = strtab[pos];
        if (c == '\0') {
          break;
        }
        name.push_back(c);
      }
      if (!name.empty()) {
        symbols.push_back(std::move(name));
      }
    }
  }
  return true;
}

bool ParseObjectSymbols(std::string_view bytes,
                        std::vector<std::string>& symbols,
                        bool defined_external_only) {
  if (IsElfBytes(bytes)) {
    return ParseElfSymbols(bytes, symbols, defined_external_only);
  }
  return ParseCoffSymbols(bytes, symbols, defined_external_only);
}

bool ParseArchiveSymbols(std::string_view bytes,
                         std::vector<std::string>& symbols,
                         bool defined_external_only) {
  if (!IsArchiveBytes(bytes)) {
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
      if (!ParseObjectSymbols(member, symbols, defined_external_only)) {
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

std::optional<ObjectFormat> DetectObjectFormatForBytes(std::string_view bytes) {
  if (IsElfBytes(bytes)) {
    return ObjectFormat::Elf;
  }

  std::vector<std::string> ignored;
  if (ParseCoffSymbols(bytes, ignored, false)) {
    return ObjectFormat::Coff;
  }
  return std::nullopt;
}

bool ArchiveMembersMatchObjectFormat(std::string_view bytes,
                                     ObjectFormat expected_format) {
  if (!IsArchiveBytes(bytes)) {
    return false;
  }

  bool saw_member = false;
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
      const auto member_format = DetectObjectFormatForBytes(member);
      if (!member_format.has_value() || *member_format != expected_format) {
        return false;
      }
      saw_member = true;
    }
    offset = data_offset + size;
    if (offset % 2 == 1) {
      ++offset;
    }
  }
  return saw_member;
}

bool LinkInputMatchesObjectFormat(const std::filesystem::path& input,
                                  ObjectFormat expected_format) {
  const auto bytes = ReadFileBytes(input);
  if (!bytes.has_value()) {
    return false;
  }
  if (IsArchiveBytes(*bytes)) {
    return ArchiveMembersMatchObjectFormat(*bytes, expected_format);
  }
  const auto actual_format = DetectObjectFormatForBytes(*bytes);
  return actual_format.has_value() && *actual_format == expected_format;
}

std::optional<std::vector<std::string>> LinkerSymsForInputs(
    const std::vector<std::filesystem::path>& inputs) {
  std::unordered_set<std::string> seen;
  for (const auto& input : inputs) {
    const auto bytes = ReadFileBytes(input);
    if (!bytes.has_value()) {
      if (IsLinkerResolvedLibraryName(input)) {
        if (LinkDebugEnabled()) {
          std::fprintf(stderr,
                       "[link-debug] symbol-scan-skip-linker-name path=%s\n",
                       input.string().c_str());
        }
        continue;
      }
      if (LinkDebugEnabled()) {
        std::fprintf(stderr,
                     "[link-debug] symbol-scan-read-fail path=%s\n",
                     input.string().c_str());
      }
      return std::nullopt;
    }
    std::vector<std::string> symbols;
    if (IsArchiveBytes(*bytes)) {
      if (!ParseArchiveSymbols(*bytes, symbols, true)) {
        if (LinkDebugEnabled()) {
          std::fprintf(stderr,
                       "[link-debug] symbol-scan-archive-parse-fail path=%s\n",
                       input.string().c_str());
        }
        return std::nullopt;
      }
    } else {
      if (!ParseObjectSymbols(*bytes, symbols, true)) {
        if (LinkDebugEnabled()) {
          std::fprintf(stderr,
                       "[link-debug] symbol-scan-object-parse-fail path=%s\n",
                       input.string().c_str());
        }
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

std::optional<std::vector<std::string>> DuplicateDefinedExternalSymbolsForObjectInputs(
    const std::vector<std::filesystem::path>& inputs) {
  std::unordered_set<std::string> seen;
  std::unordered_set<std::string> duplicate_set;
  for (const auto& input : inputs) {
    const auto bytes = ReadFileBytes(input);
    if (!bytes.has_value()) {
      return std::nullopt;
    }
    if (IsArchiveBytes(*bytes)) {
      continue;
    }

    std::vector<std::string> symbols;
    if (!ParseObjectSymbols(*bytes, symbols, true)) {
      return std::nullopt;
    }
    for (const auto& sym : symbols) {
      if (IsCoffBackendHelperSymbolForDuplicateScan(sym)) {
        continue;
      }
      if (!seen.insert(sym).second) {
        duplicate_set.insert(sym);
      }
    }
  }

  std::vector<std::string> duplicates;
  duplicates.reserve(duplicate_set.size());
  for (const auto& symbol : duplicate_set) {
    duplicates.push_back(symbol);
  }
  std::sort(duplicates.begin(), duplicates.end());
  return duplicates;
}

std::optional<std::vector<std::string>> DefinedExternalSymbolsForObjectInputs(
    const std::vector<std::filesystem::path>& inputs) {
  std::unordered_set<std::string> seen;
  for (const auto& input : inputs) {
    const auto bytes = ReadFileBytes(input);
    if (!bytes.has_value()) {
      return std::nullopt;
    }
    if (IsArchiveBytes(*bytes)) {
      continue;
    }

    std::vector<std::string> symbols;
    if (!ParseObjectSymbols(*bytes, symbols, true)) {
      return std::nullopt;
    }
    for (const auto& sym : symbols) {
      seen.insert(sym);
    }
  }

  std::vector<std::string> defined;
  defined.reserve(seen.size());
  for (const auto& symbol : seen) {
    defined.push_back(symbol);
  }
  std::sort(defined.begin(), defined.end());
  return defined;
}

std::optional<std::string> FirstMissingRuntimeSym(
    const std::vector<std::string>& syms) {
  const auto required = RuntimeRequiredSyms();
  for (const auto& req : required) {
    if (!std::binary_search(syms.begin(), syms.end(), req)) {
      return req;
    }
  }
  return std::nullopt;
}

bool IsMissingExplicitLibraryInput(const std::filesystem::path& input) {
  if (input.empty()) {
    return false;
  }
  if (!input.has_parent_path() && !input.is_absolute()) {
    return false;
  }
  std::error_code ec;
  return !std::filesystem::exists(input, ec);
}

std::filesystem::path MaterializeLinkInputForTool(
    const Project& project,
    TargetProfile target_profile,
    const std::filesystem::path& input) {
  if (target_profile != TargetProfile::X86_64Win64 ||
      input.empty() ||
      (!input.has_parent_path() && !input.is_absolute()) ||
      input.extension() != SharedLibSuffix(target_profile)) {
    return input;
  }

  auto candidate = input;
  candidate.replace_extension(ImportLibSuffix(target_profile));
  if (CanReadFile(candidate)) {
    return candidate;
  }

  if (input.has_parent_path() && input.parent_path().filename() == "bin") {
    candidate = input.parent_path().parent_path() / "lib" / input.filename();
    candidate.replace_extension(ImportLibSuffix(target_profile));
    if (CanReadFile(candidate)) {
      return candidate;
    }
  }

  return input;
}

std::string LowerAscii(std::string_view text) {
  std::string out;
  out.reserve(text.size());
  for (unsigned char ch : text) {
    out.push_back(static_cast<char>(std::tolower(ch)));
  }
  return out;
}

bool OutputSuggestsMissingLibrary(std::string_view output) {
  static constexpr std::string_view kNeedles[] = {
      "unable to find library",
      "could not open",
      "cannot open",
      "can't open",
      "no such file",
      "file not found",
      "cannot find",
      "could not find",
  };
  for (const auto needle : kNeedles) {
    if (output.find(needle) != std::string_view::npos) {
      return true;
    }
  }
  return false;
}

bool OutputMentionsLibrary(std::string_view output,
                           const std::filesystem::path& input) {
  const std::string generic = LowerAscii(input.generic_string());
  const std::string filename = LowerAscii(input.filename().generic_string());
  const std::string stem = LowerAscii(input.stem().generic_string());
  return (!generic.empty() && output.find(generic) != std::string_view::npos) ||
         (!filename.empty() &&
          output.find(filename) != std::string_view::npos) ||
         (!stem.empty() && output.find(stem) != std::string_view::npos);
}

bool IsMissingNamedLibraryFailure(
    const std::vector<std::filesystem::path>& inputs,
    std::string_view output) {
  if (output.empty()) {
    return false;
  }
  const std::string lowered = LowerAscii(output);
  if (!OutputSuggestsMissingLibrary(lowered)) {
    return false;
  }
  for (const auto& input : inputs) {
    if (input.empty() || IsMissingExplicitLibraryInput(input)) {
      continue;
    }
    if (OutputMentionsLibrary(lowered, input)) {
      return true;
    }
  }
  return false;
}

#ifdef _WIN32
std::vector<std::wstring> BuildWindowsLinkArgs(
    const std::filesystem::path& tool,
    const std::vector<std::filesystem::path>& inputs,
    const std::filesystem::path& output,
    const std::optional<std::filesystem::path>& import_lib,
    const LinkPlan& plan) {
  const bool shared_library = plan.output_kind == LinkOutputKind::SharedLibrary;
  std::vector<std::wstring> args;
  args.reserve(inputs.size() + plan.export_symbols.size() +
               plan.data_export_symbols.size() +
               plan.delay_load_dlls.size() + 8);
  args.push_back(tool.wstring());
  args.push_back(L"/NOLOGO");
  args.push_back(L"/OUT:" + output.wstring());
  auto map_output = output;
  map_output.replace_extension(L".map");
  if (shared_library) {
    args.push_back(L"/DLL");
    if (plan.shared_library_lifecycle_mode ==
        SharedLibraryLifecycleMode::WindowsEntry) {
      const std::string entry_symbol =
          plan.entry_symbol.value_or(std::string(kLibraryEntrySym));
      args.push_back(L"/ENTRY:" +
                     std::wstring(entry_symbol.begin(), entry_symbol.end()));
    }
    if (import_lib.has_value()) {
      args.push_back(L"/IMPLIB:" + import_lib->wstring());
    }
  } else {
    args.push_back(L"/ENTRY:main");
    args.push_back(L"/SUBSYSTEM:CONSOLE");
    args.push_back(L"/STACK:" +
                   std::to_wstring(kWindowsExeStackReserveBytes) +
                   L"," +
                   std::to_wstring(kWindowsExeStackCommitBytes));
  }
  args.push_back(L"/MAP:" + map_output.wstring());
  args.push_back(L"/NODEFAULTLIB");
  if (!plan.delay_load_dlls.empty()) {
    args.push_back(DelayImpLibraryPath().wstring());
  }
  for (const auto& input : inputs) {
    args.push_back(input.wstring());
  }
  for (const auto& dll_name : plan.delay_load_dlls) {
    args.push_back(L"/DELAYLOAD:" +
                   std::wstring(dll_name.begin(), dll_name.end()));
  }

  std::vector<std::string> export_symbols = plan.export_symbols;
  std::vector<std::string> data_export_symbols = plan.data_export_symbols;
  if (shared_library) {
    export_symbols.erase(
        std::remove_if(export_symbols.begin(),
                       export_symbols.end(),
                       [](const std::string& symbol) {
                         return IsHiddenSharedLibraryExportSymbolImpl(symbol);
                       }),
        export_symbols.end());
    data_export_symbols.erase(
        std::remove_if(data_export_symbols.begin(),
                       data_export_symbols.end(),
                       [](const std::string& symbol) {
                         return IsHiddenSharedLibraryExportSymbolImpl(symbol);
                       }),
        data_export_symbols.end());
  }
  std::sort(export_symbols.begin(), export_symbols.end());
  export_symbols.erase(
      std::unique(export_symbols.begin(), export_symbols.end()),
      export_symbols.end());
  std::sort(data_export_symbols.begin(), data_export_symbols.end());
  data_export_symbols.erase(
      std::unique(data_export_symbols.begin(), data_export_symbols.end()),
      data_export_symbols.end());
  for (const auto& symbol : data_export_symbols) {
    export_symbols.erase(
        std::remove(export_symbols.begin(), export_symbols.end(), symbol),
        export_symbols.end());
  }
  for (const auto& symbol : export_symbols) {
    args.push_back(L"/EXPORT:" +
                   std::wstring(symbol.begin(), symbol.end()));
  }
  for (const auto& symbol : data_export_symbols) {
    args.push_back(L"/EXPORT:" +
                   std::wstring(symbol.begin(), symbol.end()) +
                   L",DATA");
  }
  return args;
}

std::vector<std::wstring> BuildPosixLinkArgsWide(
    const std::filesystem::path& tool,
    const std::vector<std::filesystem::path>& inputs,
    const std::filesystem::path& output,
    const std::optional<std::filesystem::path>& import_lib,
    const LinkPlan& plan) {
  (void)import_lib;
  std::vector<std::wstring> args;
  args.reserve(inputs.size() + 5);
  args.push_back(tool.wstring());
  args.push_back(L"-o");
  args.push_back(output.wstring());
  if (plan.output_kind == LinkOutputKind::SharedLibrary) {
    args.push_back(L"--shared");
  } else {
    args.push_back(L"--entry=main");
  }
  args.push_back(L"--nostdlib");
  for (const auto& input : inputs) {
    args.push_back(input.wstring());
  }
  return args;
}
#else
std::vector<std::string> BuildPosixLinkArgs(
    const std::filesystem::path& tool,
    const std::vector<std::filesystem::path>& inputs,
    const std::filesystem::path& output,
    const std::optional<std::filesystem::path>& import_lib,
    const LinkPlan& plan) {
  (void)import_lib;
  std::vector<std::string> args;
  args.reserve(inputs.size() + 5 + plan.delay_load_dlls.size());
  args.push_back(tool.string());
  args.push_back("-o");
  args.push_back(output.string());
  if (plan.output_kind == LinkOutputKind::SharedLibrary) {
    args.push_back("--shared");
  } else {
    args.push_back("--entry=main");
  }
  args.push_back("--nostdlib");
  for (const auto& input : inputs) {
    args.push_back(input.string());
  }
  return args;
}
#endif

// Get the directory containing the compiler executable
std::filesystem::path GetCompilerDir() {
#ifdef _WIN32
  wchar_t path[MAX_PATH];
  DWORD len = GetModuleFileNameW(nullptr, path, MAX_PATH);
  if (len > 0 && len < MAX_PATH) {
    return std::filesystem::path(path).parent_path();
  }
#else
  // On Unix, read /proc/self/exe
  char path[4096];
  ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
  if (len > 0) {
    path[len] = '\0';
    return std::filesystem::path(path).parent_path();
  }
#endif
  return std::filesystem::path();
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

std::filesystem::path DefaultRuntimeLibPath(const Project& project,
                                            TargetProfile target_profile) {
#ifdef _WIN32
  if (const auto bundled_runtime = core::BundledWindowsRuntimeLibPath();
      bundled_runtime.has_value()) {
    return *bundled_runtime;
  }
#endif

  const std::string runtime_name(
      RuntimeLibNameFor(target_profile));
  const auto compiler_dir = GetCompilerDir();
  if (!compiler_dir.empty()) {
    const auto packaged_runtime = compiler_dir / runtime_name;
    if (CanReadFile(packaged_runtime)) {
      return packaged_runtime;
    }

    if (HasCompilerSupportLayout(compiler_dir)) {
      return compiler_dir / "runtime" / runtime_name;
    }

    const auto parent = compiler_dir.parent_path();
    if (HasCompilerSupportLayout(parent)) {
      return parent / "runtime" / runtime_name;
    }
  }

  std::filesystem::path build_root = project.outputs.root;
  if (build_root.empty()) {
    build_root = project.root / "build";
  }
  return build_root / "runtime" / runtime_name;
}

}  // namespace

std::filesystem::path RuntimeLibPath(const Project& project,
                                     TargetProfile target_profile) {
  // Spec rule:
  // 1. CLI/manifest override
  // 2. Compiler-provided runtime (packaged root runtime or legacy runtime/)
  if (const auto override_lib = core::RuntimeLibOverride();
      override_lib.has_value() && !override_lib->empty()) {
    return std::filesystem::path(*override_lib);
  }

  if (const auto manifest_lib = core::ManifestRuntimeLib();
      manifest_lib.has_value() && !manifest_lib->empty()) {
    return std::filesystem::path(*manifest_lib);
  }

  return DefaultRuntimeLibPath(project, target_profile);
}

std::vector<std::string> RuntimeRequiredSyms() {
  return codegen::RuntimeLinkRequiredSyms();
}

bool IsHiddenSharedLibraryExportSymbol(std::string_view symbol) {
  return IsHiddenSharedLibraryExportSymbolImpl(symbol);
}

std::optional<std::filesystem::path> ResolveRuntimeLib(
    const Project& project,
    TargetProfile target_profile) {
  const auto path = RuntimeLibPath(project, target_profile);
  if (!CanReadFile(path)) {
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

std::optional<std::vector<std::filesystem::path>> ArchiveMembers(
    const std::filesystem::path& archive) {
  if (!CanReadFile(archive)) {
    return std::nullopt;
  }
  return std::vector<std::filesystem::path>{archive};
}

LinkInvocationResult InvokeLinker(
    const std::filesystem::path& tool,
    const std::vector<std::filesystem::path>& inputs,
    const std::filesystem::path& output,
    const std::optional<std::filesystem::path>& import_lib,
    const LinkPlan& plan) {
  LinkInvocationResult result;
#ifdef _WIN32
  const std::optional<bool> debug_override = core::LinkDebugOverride();
  const bool debug_link =
      debug_override.has_value() ? *debug_override
                                 : core::IsDebugEnabled("link");
  auto wide_to_utf8_lossy = [](std::wstring_view text) {
    if (text.empty()) {
      return std::string{};
    }
    const int size = WideCharToMultiByte(CP_UTF8, 0, text.data(),
                                         static_cast<int>(text.size()),
                                         nullptr, 0, nullptr, nullptr);
    if (size <= 0) {
      std::string out;
      out.reserve(text.size());
      for (const wchar_t ch : text) {
        out.push_back((ch >= 0 && ch <= 0x7F) ? static_cast<char>(ch) : '?');
      }
      return out;
    }
    std::string out(size, '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()),
                        out.data(), size, nullptr, nullptr);
    return out;
  };
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

  const bool target_is_coff =
      ObjectFormatOf(plan.target_profile) == ObjectFormat::Coff;
  std::vector<std::wstring> args =
      target_is_coff ? BuildWindowsLinkArgs(tool, inputs, output, import_lib,
                                            plan)
                     : BuildPosixLinkArgsWide(tool, inputs, output, import_lib,
                                              plan);

  if (core::CrashReportingEnabled()) {
    core::DebugRunOptions run_options;
    run_options.program = tool;
    run_options.working_directory = output.parent_path();
    run_options.report_root = core::DefaultTargetCrashReportRoot(output);
    run_options.tool_name = "cursive-link";
    for (std::size_t i = 1; i < args.size(); ++i) {
      run_options.arguments.push_back(wide_to_utf8_lossy(args[i]));
    }
    const auto debug_result = core::DebugRunProcess(run_options);
    result.ok = debug_result.launched && debug_result.exit_code == 0;
    result.crashed = debug_result.crashed;
    result.exit_code = debug_result.exit_code;
    result.output = debug_result.stdout_text;
    result.output += debug_result.stderr_text;
    if (debug_result.crash_report.has_value()) {
      result.crash_report_json_path =
          debug_result.crash_report->artifacts.json_path;
      result.crash_kind = debug_result.crash_report->kind;
    }
    if (!debug_result.launched) {
      result.output = "debug launch failed";
    }
    return result;
  }

  std::wstring cmd;
  for (std::size_t i = 0; i < args.size(); ++i) {
    if (i != 0) {
      cmd.push_back(L' ');
    }
    cmd += quote_arg(args[i]);
  }

  SECURITY_ATTRIBUTES sa;
  ZeroMemory(&sa, sizeof(sa));
  sa.nLength = sizeof(sa);
  sa.bInheritHandle = TRUE;
  HANDLE read_pipe = nullptr;
  HANDLE write_pipe = nullptr;
  if (!CreatePipe(&read_pipe, &write_pipe, &sa, 0)) {
    result.output = "CreatePipe failed";
    return result;
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

  std::vector<wchar_t> cmd_buf(cmd.begin(), cmd.end());
  cmd_buf.push_back(L'\0');

  if (debug_link) {
    std::fprintf(stderr, "[link-debug] tool=%ls\n", tool.wstring().c_str());
    std::fprintf(stderr, "[link-debug] out=%ls\n", output.wstring().c_str());
    std::fprintf(stderr, "[link-debug] input_count=%zu\n", inputs.size());
    std::fprintf(stderr, "[link-debug] cmd=%ls\n", cmd.c_str());
  }

  const BOOL ok = CreateProcessW(tool.wstring().c_str(), cmd_buf.data(),
                                 nullptr, nullptr, TRUE, CREATE_NO_WINDOW,
                                 nullptr, nullptr, &si, &pi);
  CloseHandle(write_pipe);
  if (!ok) {
    CloseHandle(read_pipe);
    result.output = "CreateProcessW failed err=" +
                    std::to_string(static_cast<unsigned long>(GetLastError()));
    if (debug_link) {
      std::fprintf(stderr, "[link-debug] CreateProcessW failed err=%lu\n",
                   static_cast<unsigned long>(GetLastError()));
    }
    return result;
  }

  char buffer[4096];
  DWORD bytes_read = 0;
  while (ReadFile(read_pipe, buffer, sizeof(buffer), &bytes_read, nullptr) &&
         bytes_read > 0) {
    result.output.append(buffer, buffer + bytes_read);
  }
  CloseHandle(read_pipe);

  WaitForSingleObject(pi.hProcess, INFINITE);
  DWORD exit_code = 1;
  GetExitCodeProcess(pi.hProcess, &exit_code);
  result.exit_code = static_cast<int>(exit_code);
  result.ok = exit_code == 0;
  if (debug_link) {
    std::fprintf(stderr, "[link-debug] exit=%lu\n",
                 static_cast<unsigned long>(exit_code));
    if (!result.output.empty()) {
      std::fprintf(stderr, "[link-debug] output=%s\n", result.output.c_str());
    }
  }
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);
  return result;
#else
  std::vector<std::string> args =
      BuildPosixLinkArgs(tool, inputs, output, import_lib, plan);

  std::vector<char*> argv;
  argv.reserve(args.size() + 1);
  for (auto& arg : args) {
    argv.push_back(arg.data());
  }
  argv.push_back(nullptr);

  int pipe_fds[2] = {-1, -1};
  if (pipe(pipe_fds) != 0) {
    result.output = "pipe failed";
    return result;
  }

  const pid_t pid = fork();
  if (pid < 0) {
    close(pipe_fds[0]);
    close(pipe_fds[1]);
    result.output = "fork failed";
    return result;
  }
  if (pid == 0) {
    close(pipe_fds[0]);
    dup2(pipe_fds[1], STDOUT_FILENO);
    dup2(pipe_fds[1], STDERR_FILENO);
    close(pipe_fds[1]);
    execv(argv[0], argv.data());
    std::perror("execv");
    _exit(127);
  }
  close(pipe_fds[1]);
  char buffer[4096];
  ssize_t bytes_read = 0;
  while ((bytes_read = read(pipe_fds[0], buffer, sizeof(buffer))) > 0) {
    result.output.append(buffer, buffer + bytes_read);
  }
  close(pipe_fds[0]);
  int status = 0;
  if (waitpid(pid, &status, 0) < 0) {
    return result;
  }
  if (WIFEXITED(status)) {
    result.exit_code = WEXITSTATUS(status);
    result.ok = result.exit_code == 0;
  }
  return result;
#endif
}

bool InvokeArchiver(const std::filesystem::path& tool,
                    const std::vector<std::filesystem::path>& inputs,
                    const std::filesystem::path& output) {
#ifdef _WIN32
  if (core::CrashReportingEnabled()) {
    core::DebugRunOptions run_options;
    run_options.program = tool;
    run_options.working_directory = output.parent_path();
    run_options.report_root = core::DefaultTargetCrashReportRoot(output);
    run_options.tool_name = "cursive-archiver";
    run_options.arguments.push_back("/NOLOGO");
    run_options.arguments.push_back("/OUT:" + output.generic_string());
    for (const auto& input : inputs) {
      run_options.arguments.push_back(input.generic_string());
    }
    const auto debug_result = core::DebugRunProcess(run_options);
    return debug_result.launched && debug_result.exit_code == 0;
  }
  const bool debug_link =
      core::LinkDebugOverride().value_or(core::IsDebugEnabled("link"));
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
  args.reserve(inputs.size() + 3);
  args.push_back(tool.wstring());
  args.push_back(L"/NOLOGO");
  args.push_back(L"/OUT:" + output.wstring());
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
  if (debug_link) {
    si.dwFlags |= STARTF_USESTDHANDLES;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    si.hStdOutput = GetStdHandle(STD_ERROR_HANDLE);
    si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
  }
  PROCESS_INFORMATION pi;
  ZeroMemory(&pi, sizeof(pi));

  std::vector<wchar_t> cmd_buf(cmd.begin(), cmd.end());
  cmd_buf.push_back(L'\0');

  if (debug_link) {
    std::fprintf(stderr, "[link-debug] archiver=%ls\n", tool.wstring().c_str());
    std::fprintf(stderr, "[link-debug] archive-out=%ls\n", output.wstring().c_str());
    std::fprintf(stderr, "[link-debug] archive-input-count=%zu\n", inputs.size());
    std::fprintf(stderr, "[link-debug] archive-cmd=%ls\n", cmd.c_str());
  }

  const BOOL ok = CreateProcessW(tool.wstring().c_str(), cmd_buf.data(),
                                 nullptr, nullptr, TRUE, CREATE_NO_WINDOW,
                                 nullptr, nullptr, &si, &pi);
  if (!ok) {
    if (debug_link) {
      std::fprintf(stderr, "[link-debug] archive CreateProcessW failed err=%lu\n",
                   static_cast<unsigned long>(GetLastError()));
    }
    return false;
  }

  WaitForSingleObject(pi.hProcess, INFINITE);
  DWORD exit_code = 1;
  GetExitCodeProcess(pi.hProcess, &exit_code);
  if (debug_link) {
    std::fprintf(stderr, "[link-debug] archive exit=%lu\n",
                 static_cast<unsigned long>(exit_code));
  }
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);
  return exit_code == 0;
#else
  std::vector<std::string> args;
  args.reserve(inputs.size() + 2);
  args.push_back(tool.string());
  args.push_back("rcs");
  args.push_back(output.string());
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

std::vector<std::filesystem::path> MaterializeLinkInputsForTool(
    const Project& project,
    TargetProfile target_profile,
    const std::vector<std::filesystem::path>& inputs) {
  std::vector<std::filesystem::path> materialized;
  materialized.reserve(inputs.size());
  for (const auto& input : inputs) {
    materialized.push_back(
        MaterializeLinkInputForTool(project, target_profile, input));
  }
  return materialized;
}

LinkResult Link(const std::vector<std::filesystem::path>& objs,
                const std::vector<std::filesystem::path>& extra_inputs,
                const Project& project,
                const LinkPlan& plan,
                const LinkDeps& deps) {
  LinkResult result;
  const auto output_path = PrimaryArtifactPath(project, plan.target_profile);
  const auto import_lib = ImportLibPath(project, plan.target_profile);
  if (!output_path.has_value()) {
    result.status = LinkStatus::Fail;
    return result;
  }

  for (const auto& input : extra_inputs) {
    if (IsMissingExplicitLibraryInput(input)) {
      SPEC_RULE("Link-Library-NotFound");
      EmitExternal(result.diags, "E-SYS-3347");
      result.status = LinkStatus::Fail;
      return result;
    }
  }

  const std::string_view linker_name =
      LinkerToolName(plan.target_profile);
  const auto tool = deps.resolve_tool(project, plan.target_profile, linker_name);
  if (!tool.has_value()) {
    SPEC_RULE("Link-NotFound");
    if (auto diag = core::MakeExternalDiagnostic("E-OUT-0405")) {
      core::SubDiagnostic guidance_note;
      guidance_note.kind = core::SubDiagnosticKind::Note;
      guidance_note.message = "set llvm_bin in [toolchain] in Cursive.toml";
      diag->children.push_back(std::move(guidance_note));

      core::SubDiagnostic search_note;
      search_note.kind = core::SubDiagnosticKind::Note;
      search_note.message = FormatSearchedPaths(project, linker_name);
      diag->children.push_back(std::move(search_note));
      core::Emit(result.diags, *diag);
    }
    result.status = LinkStatus::NotFound;
    return result;
  }
  const std::filesystem::path tool_path = *tool;

  const auto materialized_extra_inputs =
      MaterializeLinkInputsForTool(project, plan.target_profile, extra_inputs);

  const auto runtime_lib =
      deps.resolve_runtime_lib(project, plan.target_profile);
  if (!runtime_lib.has_value()) {
    SPEC_RULE("Link-Runtime-Missing");
    if (auto diag = core::MakeExternalDiagnostic("E-OUT-0407")) {
      core::SubDiagnostic guidance_note;
      guidance_note.kind = core::SubDiagnosticKind::Note;
      guidance_note.message =
          "set --runtime-lib <path> or add runtime_lib to [toolchain] in "
          "Cursive.toml";
      diag->children.push_back(std::move(guidance_note));

      // Collect runtime lib search locations
      const auto rt_path = RuntimeLibPath(project, plan.target_profile);
      std::string search_info = "searched for runtime library at: " +
                                rt_path.string();
      core::SubDiagnostic search_note;
      search_note.kind = core::SubDiagnosticKind::Note;
      search_note.message = std::move(search_info);
      diag->children.push_back(std::move(search_note));
      core::Emit(result.diags, *diag);
    }
    result.status = LinkStatus::RuntimeMissing;
    return result;
  }

  if (!LinkInputMatchesObjectFormat(
          *runtime_lib, ObjectFormatOf(plan.target_profile))) {
    if (LinkDebugEnabled()) {
      std::fprintf(stderr,
                   "[link-debug] runtime-format-mismatch path=%s target=%s\n",
                   runtime_lib->string().c_str(),
                   std::string(TargetProfileName(plan.target_profile)).c_str());
    }
    if (auto diag = core::MakeExternalDiagnostic("E-OUT-0408")) {
      core::SubDiagnostic note;
      note.kind = core::SubDiagnosticKind::Note;
      note.message = "runtime library `" + runtime_lib->string() +
                     "` does not match target object format for `" +
                     std::string(TargetProfileName(plan.target_profile)) +
                     "`";
      diag->children.push_back(std::move(note));
      core::Emit(result.diags, *diag);
    } else {
      EmitExternal(result.diags, "E-OUT-0408");
    }
    result.status = LinkStatus::RuntimeIncompatible;
    return result;
  }

  std::vector<std::filesystem::path> sym_inputs = objs;
  sym_inputs.insert(sym_inputs.end(), materialized_extra_inputs.begin(),
                    materialized_extra_inputs.end());
  sym_inputs.push_back(*runtime_lib);

  std::vector<std::filesystem::path> inputs = objs;
  inputs.reserve(objs.size() + materialized_extra_inputs.size() + 1);
  for (const auto& input : materialized_extra_inputs) {
    inputs.push_back(input);
  }
  inputs.push_back(*runtime_lib);

  std::vector<std::filesystem::path> duplicate_symbol_inputs = objs;
  duplicate_symbol_inputs.reserve(objs.size() + materialized_extra_inputs.size());
  for (const auto& input : materialized_extra_inputs) {
    duplicate_symbol_inputs.push_back(input);
  }
  const auto duplicate_symbols =
      DuplicateDefinedExternalSymbolsForObjectInputs(duplicate_symbol_inputs);
  if (duplicate_symbols.has_value() && !duplicate_symbols->empty()) {
    if (auto diag = core::MakeExternalDiagnostic("E-SYS-3342")) {
      core::SubDiagnostic note;
      note.kind = core::SubDiagnosticKind::Note;
      note.message = "duplicate link symbol: " + duplicate_symbols->front();
      diag->children.push_back(std::move(note));
      core::Emit(result.diags, *diag);
    } else {
      EmitExternal(result.diags, "E-SYS-3342");
    }
    result.status = LinkStatus::Fail;
    return result;
  }

  const auto syms = deps.linker_syms(tool_path, sym_inputs, *output_path);
  const auto missing_runtime_sym =
      syms.has_value() ? FirstMissingRuntimeSym(*syms) : std::nullopt;
  if (!syms.has_value() || missing_runtime_sym.has_value()) {
    if (LinkDebugEnabled()) {
      if (!syms.has_value()) {
        std::fprintf(stderr,
                     "[link-debug] runtime-symbol-scan-failed input_count=%zu\n",
                     sym_inputs.size());
      } else {
        std::fprintf(stderr,
                     "[link-debug] runtime-symbol-missing symbol=%s\n",
                     missing_runtime_sym->c_str());
      }
    }
    SPEC_RULE("Link-Runtime-Incompatible");
    EmitExternal(result.diags, "E-OUT-0408");
    result.status = LinkStatus::RuntimeIncompatible;
    return result;
  }

  const auto link_result =
      deps.invoke_linker(tool_path, inputs, *output_path, import_lib, plan);
  if (!link_result.ok) {
    core::HostPrimFail(core::HostPrim::InvokeLinker, true);
    const bool missing_library =
        IsMissingNamedLibraryFailure(extra_inputs, link_result.output);
    if (auto diag = core::MakeExternalDiagnostic(missing_library
                                                     ? "E-SYS-3347"
                                                     : "E-OUT-0404")) {
      if (!link_result.crash_report_json_path.empty()) {
        core::SubDiagnostic note;
        note.kind = core::SubDiagnosticKind::Note;
        note.message =
            "linker crash report: " +
            link_result.crash_report_json_path.generic_string();
        diag->children.push_back(std::move(note));
      }
      if (link_result.crashed && !link_result.crash_kind.empty()) {
        core::SubDiagnostic note;
        note.kind = core::SubDiagnosticKind::Note;
        note.message = "linker crash kind: " + link_result.crash_kind;
        diag->children.push_back(std::move(note));
      }
      core::Emit(result.diags, *diag);
    } else if (missing_library) {
      EmitExternal(result.diags, "E-SYS-3347");
    } else {
      EmitExternal(result.diags, "E-OUT-0404");
    }
    if (missing_library) {
      SPEC_RULE("Link-Library-NotFound");
    } else {
      SPEC_RULE("Link-Fail");
    }
    result.status = LinkStatus::Fail;
    return result;
  }

  SPEC_RULE("Link-Ok");
  result.status = LinkStatus::Ok;
  return result;
}

LinkResult Archive(const std::vector<std::filesystem::path>& objs,
                   const Project& project,
                   TargetProfile target_profile,
                   const LinkDeps& deps) {
  LinkResult result;
  const auto output_path = PrimaryArtifactPath(project, target_profile);
  if (!output_path.has_value()) {
    result.status = LinkStatus::Fail;
    return result;
  }

  const std::string_view archiver_name =
      ArchiverToolName(target_profile);
  const auto tool =
      deps.resolve_tool(project, target_profile, archiver_name);
  if (!tool.has_value()) {
    SPEC_RULE("Archive-NotFound");
    if (auto diag = core::MakeExternalDiagnostic("E-OUT-0405")) {
      core::SubDiagnostic guidance_note;
      guidance_note.kind = core::SubDiagnosticKind::Note;
      guidance_note.message = "set llvm_bin in [toolchain] in Cursive.toml";
      diag->children.push_back(std::move(guidance_note));

      core::SubDiagnostic search_note;
      search_note.kind = core::SubDiagnosticKind::Note;
      search_note.message = FormatSearchedPaths(project, archiver_name);
      diag->children.push_back(std::move(search_note));
      core::Emit(result.diags, *diag);
    }
    result.status = LinkStatus::NotFound;
    return result;
  }
  const std::filesystem::path tool_path = *tool;

  std::vector<std::filesystem::path> inputs = objs;

  if (!deps.invoke_archiver(tool_path, inputs, *output_path)) {
    core::HostPrimFail(core::HostPrim::InvokeArchiver, true);
    SPEC_RULE("Archive-Fail");
    EmitExternal(result.diags, "E-OUT-0404");
    result.status = LinkStatus::Fail;
    return result;
  }

  SPEC_RULE("Archive-Ok");
  result.status = LinkStatus::Ok;
  return result;
}
}  // namespace cursive::project
