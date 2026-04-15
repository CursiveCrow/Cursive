#include "00_core/crash_debug.h"

#include <algorithm>
#include <atomic>
#include <cctype>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <functional>
#include <mutex>
#include <optional>
#include <sstream>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <utility>

#ifdef _WIN32
#include <windows.h>
#include <dbghelp.h>
#include <malloc.h>
#include <process.h>
#pragma comment(lib, "dbghelp.lib")
#endif

namespace cursive::core {

namespace {

struct RuntimeState {
  CrashRuntimeOptions options;
  bool installed = false;
};

RuntimeState& State() {
  static RuntimeState state;
  return state;
}

std::mutex& StateMutex() {
  static std::mutex mu;
  return mu;
}

std::atomic<bool>& HandlingCrash() {
  static std::atomic<bool> handling{false};
  return handling;
}

CrashRuntimeOptions CrashOptionsSnapshot() {
  std::lock_guard<std::mutex> lock(StateMutex());
  return State().options;
}

std::string EscapeJson(std::string_view value) {
  std::string out;
  out.reserve(value.size() + 8);
  for (const char ch : value) {
    switch (ch) {
      case '\\':
        out += "\\\\";
        break;
      case '"':
        out += "\\\"";
        break;
      case '\n':
        out += "\\n";
        break;
      case '\r':
        out += "\\r";
        break;
      case '\t':
        out += "\\t";
        break;
      default:
        if (static_cast<unsigned char>(ch) < 0x20) {
          char buffer[7];
          std::snprintf(buffer, sizeof(buffer), "\\u%04X",
                        static_cast<unsigned>(static_cast<unsigned char>(ch)));
          out += buffer;
        } else {
          out.push_back(ch);
        }
        break;
    }
  }
  return out;
}

std::string NowUtcString() {
  const auto now = std::chrono::system_clock::now();
  const std::time_t now_time = std::chrono::system_clock::to_time_t(now);
  std::tm tm_utc{};
#ifdef _WIN32
  gmtime_s(&tm_utc, &now_time);
#else
  gmtime_r(&now_time, &tm_utc);
#endif
  char buffer[32];
  std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &tm_utc);
  return buffer;
}

std::string TimestampFileStem() {
  const auto now = std::chrono::system_clock::now();
  const std::time_t now_time = std::chrono::system_clock::to_time_t(now);
  std::tm tm_local{};
#ifdef _WIN32
  localtime_s(&tm_local, &now_time);
#else
  localtime_r(&now_time, &tm_local);
#endif
  char buffer[32];
  std::strftime(buffer, sizeof(buffer), "%Y%m%d_%H%M%S", &tm_local);
  return buffer;
}

std::string PathString(const std::filesystem::path& path) {
  return path.empty() ? std::string{} : path.generic_string();
}

std::filesystem::path TempCrashRoot() {
  std::error_code ec;
  const auto temp = std::filesystem::temp_directory_path(ec);
  if (!ec) {
    return temp / "cursive" / "crash";
  }
  return std::filesystem::path("crash");
}

void EnsureDirectory(const std::filesystem::path& path) {
  if (path.empty()) {
    return;
  }
  std::error_code ec;
  std::filesystem::create_directories(path, ec);
}

void WriteTextFile(const std::filesystem::path& path, std::string_view text) {
  if (path.empty()) {
    return;
  }
  EnsureDirectory(path.parent_path());
  std::ofstream out(path, std::ios::binary | std::ios::trunc);
  if (!out) {
    return;
  }
  out.write(text.data(), static_cast<std::streamsize>(text.size()));
}

std::string JoinArguments(const std::vector<std::string>& args) {
  std::ostringstream oss;
  for (std::size_t i = 0; i < args.size(); ++i) {
    if (i != 0) {
      oss << ' ';
    }
    oss << args[i];
  }
  return oss.str();
}

std::string Hex32(std::uint32_t value) {
  char buffer[16];
  std::snprintf(buffer, sizeof(buffer), "0x%08X", value);
  return buffer;
}

std::string Hex64(std::uint64_t value) {
  char buffer[32];
  std::snprintf(buffer, sizeof(buffer), "0x%016llX",
                static_cast<unsigned long long>(value));
  return buffer;
}

std::string HexCompact64(std::uint64_t value) {
  char buffer[32];
  std::snprintf(buffer, sizeof(buffer), "0x%llX",
                static_cast<unsigned long long>(value));
  return buffer;
}

std::string Trim(std::string_view text) {
  std::size_t start = 0;
  while (start < text.size() &&
         std::isspace(static_cast<unsigned char>(text[start])) != 0) {
    ++start;
  }
  std::size_t end = text.size();
  while (end > start &&
         std::isspace(static_cast<unsigned char>(text[end - 1])) != 0) {
    --end;
  }
  return std::string(text.substr(start, end - start));
}

bool IsHexString(std::string_view text) {
  if (text.empty()) {
    return false;
  }
  for (const unsigned char ch : text) {
    if (std::isxdigit(ch) == 0) {
      return false;
    }
  }
  return true;
}

std::optional<std::uint64_t> ParseHexU64(std::string_view text) {
  if (!IsHexString(text)) {
    return std::nullopt;
  }
  try {
    return static_cast<std::uint64_t>(
        std::stoull(std::string(text), nullptr, 16));
  } catch (...) {
    return std::nullopt;
  }
}

#ifdef _WIN32

std::string WideToUtf8Lossy(std::wstring_view text) {
  if (text.empty()) {
    return {};
  }
  const int size = WideCharToMultiByte(CP_UTF8, 0, text.data(),
                                       static_cast<int>(text.size()), nullptr,
                                       0, nullptr, nullptr);
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
}

std::wstring Utf8ToWide(std::string_view text) {
  if (text.empty()) {
    return {};
  }
  const int size = MultiByteToWideChar(CP_UTF8, 0, text.data(),
                                       static_cast<int>(text.size()), nullptr,
                                       0);
  if (size <= 0) {
    std::wstring out;
    out.reserve(text.size());
    for (const char ch : text) {
      out.push_back(static_cast<unsigned char>(ch));
    }
    return out;
  }
  std::wstring out(size, L'\0');
  MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()),
                      out.data(), size);
  return out;
}

std::string ExceptionName(DWORD code) {
  switch (code) {
    case EXCEPTION_ACCESS_VIOLATION:
      return "ACCESS_VIOLATION";
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
      return "ARRAY_BOUNDS_EXCEEDED";
    case EXCEPTION_BREAKPOINT:
      return "BREAKPOINT";
    case EXCEPTION_DATATYPE_MISALIGNMENT:
      return "DATATYPE_MISALIGNMENT";
    case EXCEPTION_FLT_DENORMAL_OPERAND:
      return "FLT_DENORMAL_OPERAND";
    case EXCEPTION_FLT_DIVIDE_BY_ZERO:
      return "FLT_DIVIDE_BY_ZERO";
    case EXCEPTION_FLT_INEXACT_RESULT:
      return "FLT_INEXACT_RESULT";
    case EXCEPTION_FLT_INVALID_OPERATION:
      return "FLT_INVALID_OPERATION";
    case EXCEPTION_FLT_OVERFLOW:
      return "FLT_OVERFLOW";
    case EXCEPTION_FLT_STACK_CHECK:
      return "FLT_STACK_CHECK";
    case EXCEPTION_FLT_UNDERFLOW:
      return "FLT_UNDERFLOW";
    case EXCEPTION_ILLEGAL_INSTRUCTION:
      return "ILLEGAL_INSTRUCTION";
    case EXCEPTION_IN_PAGE_ERROR:
      return "IN_PAGE_ERROR";
    case EXCEPTION_INT_DIVIDE_BY_ZERO:
      return "INT_DIVIDE_BY_ZERO";
    case EXCEPTION_INT_OVERFLOW:
      return "INT_OVERFLOW";
    case EXCEPTION_INVALID_DISPOSITION:
      return "INVALID_DISPOSITION";
    case EXCEPTION_NONCONTINUABLE_EXCEPTION:
      return "NONCONTINUABLE_EXCEPTION";
    case EXCEPTION_PRIV_INSTRUCTION:
      return "PRIV_INSTRUCTION";
    case EXCEPTION_SINGLE_STEP:
      return "SINGLE_STEP";
    case EXCEPTION_STACK_OVERFLOW:
      return "STACK_OVERFLOW";
    default:
      return "UNKNOWN_EXCEPTION";
  }
}

std::string CrashKindFromException(DWORD code) {
  switch (code) {
    case EXCEPTION_STACK_OVERFLOW:
      return "stack-overflow";
    case EXCEPTION_ACCESS_VIOLATION:
      return "access-violation";
    case EXCEPTION_INT_DIVIDE_BY_ZERO:
    case EXCEPTION_FLT_DIVIDE_BY_ZERO:
      return "divide-by-zero";
    default:
      return "seh-exception";
  }
}

std::string DescribeWin32Error(DWORD error_code) {
  if (error_code == 0) {
    return {};
  }
  LPWSTR message_buffer = nullptr;
  const DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER |
                      FORMAT_MESSAGE_FROM_SYSTEM |
                      FORMAT_MESSAGE_IGNORE_INSERTS;
  const DWORD length = FormatMessageW(flags, nullptr, error_code, 0,
                                      reinterpret_cast<LPWSTR>(&message_buffer),
                                      0, nullptr);
  std::string message;
  if (length != 0 && message_buffer != nullptr) {
    message = Trim(WideToUtf8Lossy(std::wstring_view(message_buffer, length)));
  }
  if (message_buffer != nullptr) {
    LocalFree(message_buffer);
  }
  if (message.empty()) {
    message = "Windows error";
  }
  return message + " (" + Hex32(error_code) + ")";
}

std::string QuoteArg(std::wstring_view arg) {
  if (arg.empty()) {
    return "\"\"";
  }
  bool needs_quotes = false;
  for (const wchar_t ch : arg) {
    if (ch == L' ' || ch == L'\t' || ch == L'"') {
      needs_quotes = true;
      break;
    }
  }
  if (!needs_quotes) {
    return WideToUtf8Lossy(arg);
  }

  std::wstring out;
  out.push_back(L'"');
  int backslashes = 0;
  for (const wchar_t ch : arg) {
    if (ch == L'\\') {
      ++backslashes;
      continue;
    }
    if (ch == L'"') {
      out.append(backslashes * 2 + 1, L'\\');
      out.push_back(L'"');
      backslashes = 0;
      continue;
    }
    if (backslashes > 0) {
      out.append(backslashes, L'\\');
      backslashes = 0;
    }
    out.push_back(ch);
  }
  if (backslashes > 0) {
    out.append(backslashes * 2, L'\\');
  }
  out.push_back(L'"');
  return WideToUtf8Lossy(out);
}

std::wstring QuoteWideArg(std::wstring_view arg) {
  return Utf8ToWide(QuoteArg(arg));
}

std::wstring BuildCommandLine(const std::filesystem::path& program,
                              const std::vector<std::string>& arguments) {
  std::wstring cmd = QuoteWideArg(program.wstring());
  for (const auto& arg : arguments) {
    cmd.push_back(L' ');
    const std::wstring wide_arg = Utf8ToWide(arg);
    cmd += QuoteWideArg(wide_arg);
  }
  return cmd;
}

std::string BuildSymbolSearchPath(const std::filesystem::path& executable_path) {
  std::vector<std::string> parts;
  if (!executable_path.empty()) {
    parts.push_back(executable_path.parent_path().string());
    if (executable_path.parent_path().filename() == "bin") {
      parts.push_back(executable_path.parent_path().parent_path().string());
      parts.push_back((executable_path.parent_path().parent_path() / "obj").string());
    }
  }
  wchar_t buffer[32767];
  const DWORD len = GetEnvironmentVariableW(L"_NT_SYMBOL_PATH", buffer,
                                            static_cast<DWORD>(sizeof(buffer) / sizeof(buffer[0])));
  if (len > 0 && len < (sizeof(buffer) / sizeof(buffer[0]))) {
    parts.push_back(WideToUtf8Lossy(std::wstring_view(buffer, len)));
  }
  std::ostringstream oss;
  for (std::size_t i = 0; i < parts.size(); ++i) {
    if (i != 0) {
      oss << ';';
    }
    oss << parts[i];
  }
  return oss.str();
}

struct MapSymbolEntry {
  std::uint64_t rva = 0;
  std::string name;
};

struct MapSymbolIndex {
  std::filesystem::path map_path;
  std::uint64_t preferred_base = 0;
  std::vector<MapSymbolEntry> symbols;
};

constexpr auto kMapFreshnessTolerance = std::chrono::seconds(5);

std::uint64_t FileTimeCacheStamp(const std::filesystem::path& path) {
  std::error_code ec;
  const auto time = std::filesystem::last_write_time(path, ec);
  if (ec) {
    return 0;
  }
  const auto ticks = time.time_since_epoch().count();
  if (ticks < 0) {
    return 0;
  }
  return static_cast<std::uint64_t>(ticks);
}

std::string DecodeMapSymbol(std::string_view symbol) {
  std::string out;
  out.reserve(symbol.size());
  for (std::size_t i = 0; i < symbol.size(); ++i) {
    if (i + 4 < symbol.size() && symbol[i] == '_' && symbol[i + 1] == 'x' &&
        std::isxdigit(static_cast<unsigned char>(symbol[i + 2])) != 0 &&
        std::isxdigit(static_cast<unsigned char>(symbol[i + 3])) != 0 &&
        symbol[i + 4] == '_') {
      const auto value = ParseHexU64(symbol.substr(i + 2, 2));
      if (value.has_value() && *value <= 0x7F) {
        out.push_back(static_cast<char>(*value));
        i += 4;
        continue;
      }
    }
    if (i + 2 < symbol.size() && symbol[i] == 'x' &&
        std::isxdigit(static_cast<unsigned char>(symbol[i + 1])) != 0 &&
        std::isxdigit(static_cast<unsigned char>(symbol[i + 2])) != 0 &&
        i > 0 && symbol[i - 1] == '_') {
      const bool has_suffix_boundary =
          (i + 3 == symbol.size()) || symbol[i + 3] == '_' ||
          std::isupper(static_cast<unsigned char>(symbol[i + 3])) != 0;
      if (has_suffix_boundary) {
        const auto value = ParseHexU64(symbol.substr(i + 1, 2));
        if (value.has_value() && *value <= 0x7F) {
          out.push_back(static_cast<char>(*value));
          i += 2;
          if (i + 1 < symbol.size() && symbol[i + 1] == '_') {
            ++i;
          }
          continue;
        }
      }
    }
    out.push_back(symbol[i]);
  }
  return out;
}

std::optional<MapSymbolIndex> LoadMapSymbolIndex(
    const std::filesystem::path& image_path) {
  if (image_path.empty()) {
    return std::nullopt;
  }
  std::error_code ec;
  const auto image_write_time = std::filesystem::last_write_time(image_path, ec);
  if (ec) {
    return std::nullopt;
  }
  std::filesystem::path map_path = image_path;
  map_path.replace_extension(".map");
  if (!std::filesystem::exists(map_path, ec) || ec) {
    return std::nullopt;
  }
  const auto map_write_time = std::filesystem::last_write_time(map_path, ec);
  if (ec) {
    return std::nullopt;
  }
  if (map_write_time + kMapFreshnessTolerance < image_write_time) {
    return std::nullopt;
  }

  std::ifstream in(map_path, std::ios::binary);
  if (!in) {
    return std::nullopt;
  }

  MapSymbolIndex index;
  index.map_path = map_path;

  std::string line;
  while (std::getline(in, line)) {
    const std::string trimmed = Trim(line);
    static constexpr std::string_view preferred_prefix =
        "Preferred load address is ";
    if (trimmed.rfind(preferred_prefix, 0) == 0) {
      const auto parsed =
          ParseHexU64(trimmed.substr(preferred_prefix.size()));
      if (parsed.has_value()) {
        index.preferred_base = *parsed;
      }
      continue;
    }

    std::istringstream iss(trimmed);
    std::string address_token;
    std::string symbol_token;
    std::string absolute_token;
    if (!(iss >> address_token >> symbol_token >> absolute_token)) {
      continue;
    }
    if (address_token.find(':') == std::string::npos ||
        !IsHexString(absolute_token)) {
      continue;
    }
    const auto absolute = ParseHexU64(absolute_token);
    if (!absolute.has_value() || *absolute < index.preferred_base) {
      continue;
    }
    MapSymbolEntry entry;
    entry.rva = *absolute - index.preferred_base;
    entry.name = DecodeMapSymbol(symbol_token);
    index.symbols.push_back(std::move(entry));
  }

  if (index.symbols.empty()) {
    return std::nullopt;
  }
  std::sort(index.symbols.begin(), index.symbols.end(),
            [](const MapSymbolEntry& lhs, const MapSymbolEntry& rhs) {
              return lhs.rva < rhs.rva;
            });
  return index;
}

const MapSymbolIndex* GetCachedMapSymbolIndex(
    const std::filesystem::path& image_path) {
  static std::mutex cache_mutex;
  static std::unordered_map<std::string, std::optional<MapSymbolIndex>> cache;

  const std::string key =
      image_path.lexically_normal().generic_string() + "|" +
      std::to_string(FileTimeCacheStamp(image_path)) + "|" +
      std::to_string(FileTimeCacheStamp(std::filesystem::path(image_path)
                                            .replace_extension(".map")));
  std::lock_guard<std::mutex> lock(cache_mutex);
  auto it = cache.find(key);
  if (it == cache.end()) {
    it = cache.emplace(key, LoadMapSymbolIndex(image_path)).first;
  }
  return it->second.has_value() ? &*it->second : nullptr;
}

void TryResolveFrameFromMap(CrashFrame* frame) {
  if (frame == nullptr || frame->module_path.empty() || frame->module_base == 0 ||
      frame->address < frame->module_base) {
    return;
  }
  const MapSymbolIndex* index =
      GetCachedMapSymbolIndex(std::filesystem::path(frame->module_path));
  if (index == nullptr) {
    return;
  }
  const std::uint64_t rva = frame->address - frame->module_base;
  const auto it = std::upper_bound(
      index->symbols.begin(), index->symbols.end(), rva,
      [](std::uint64_t value, const MapSymbolEntry& entry) {
        return value < entry.rva;
      });
  if (it == index->symbols.begin()) {
    return;
  }
  const MapSymbolEntry& entry = *std::prev(it);
  frame->symbol = entry.name;
  frame->offset = rva - entry.rva;
}

struct SymbolSession {
  HANDLE process = nullptr;
  bool active = false;

  SymbolSession(HANDLE process_handle,
                const std::filesystem::path& executable_path)
      : process(process_handle) {
    SymSetOptions(SYMOPT_DEFERRED_LOADS | SYMOPT_UNDNAME | SYMOPT_LOAD_LINES |
                  SYMOPT_FAIL_CRITICAL_ERRORS);
    const std::string search_path = BuildSymbolSearchPath(executable_path);
    active = SymInitialize(process,
                           search_path.empty() ? nullptr : search_path.c_str(),
                           TRUE) == TRUE;
  }

  ~SymbolSession() {
    if (active) {
      SymCleanup(process);
    }
  }
};

CrashArtifacts MakeArtifacts(const std::filesystem::path& root,
                             std::string_view kind,
                             DWORD pid) {
  EnsureDirectory(root);
  const std::string stem =
      TimestampFileStem() + "_" + std::to_string(static_cast<unsigned long>(pid));
  const std::filesystem::path report_dir =
      root / (stem + "_" + std::string(kind));
  EnsureDirectory(report_dir);
  CrashArtifacts artifacts;
  artifacts.report_dir = report_dir;
  artifacts.text_path = report_dir / "report.txt";
  artifacts.json_path = report_dir / "report.json";
  artifacts.minidump_path = report_dir / "crash.dmp";
  artifacts.stdout_path = report_dir / "stdout.txt";
  artifacts.stderr_path = report_dir / "stderr.txt";
  return artifacts;
}

bool WriteMinidump(const CrashArtifacts& artifacts,
                   HANDLE process,
                   DWORD process_id,
                   DWORD thread_id,
                   EXCEPTION_RECORD* exception_record,
                   CONTEXT* context) {
  if (artifacts.minidump_path.empty()) {
    return false;
  }
  EnsureDirectory(artifacts.minidump_path.parent_path());
  HANDLE file = CreateFileW(artifacts.minidump_path.wstring().c_str(),
                            GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL, nullptr);
  if (file == INVALID_HANDLE_VALUE) {
    return false;
  }

  MINIDUMP_EXCEPTION_INFORMATION exception_info{};
  MINIDUMP_EXCEPTION_INFORMATION* exception_info_ptr = nullptr;
  EXCEPTION_POINTERS pointers{};
  if (exception_record != nullptr && context != nullptr) {
    pointers.ExceptionRecord = exception_record;
    pointers.ContextRecord = context;
    exception_info.ThreadId = thread_id;
    exception_info.ExceptionPointers = &pointers;
    exception_info.ClientPointers = FALSE;
    exception_info_ptr = &exception_info;
  }

  const BOOL ok = MiniDumpWriteDump(
      process, process_id, file,
      static_cast<MINIDUMP_TYPE>(MiniDumpWithThreadInfo |
                                 MiniDumpWithUnloadedModules |
                                 MiniDumpWithDataSegs),
      exception_info_ptr, nullptr, nullptr);
  CloseHandle(file);
  return ok == TRUE;
}

CrashFrame CaptureFrame(HANDLE process,
                        std::uint64_t address,
                        std::size_t index,
                        const std::filesystem::path& executable_path) {
  CrashFrame out;
  out.index = index;
  out.address = address;

  char symbol_buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME];
  std::memset(symbol_buffer, 0, sizeof(symbol_buffer));
  auto* symbol = reinterpret_cast<SYMBOL_INFO*>(symbol_buffer);
  symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
  symbol->MaxNameLen = MAX_SYM_NAME;
  DWORD64 displacement = 0;
  if (SymFromAddr(process, address, &displacement, symbol) == TRUE) {
    out.symbol = symbol->Name;
    out.offset = displacement;
  }

  IMAGEHLP_LINE64 line{};
  line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
  DWORD line_displacement = 0;
  if (SymGetLineFromAddr64(process, address, &line_displacement, &line) ==
      TRUE) {
    if (line.FileName != nullptr) {
      out.file = line.FileName;
    }
    out.line = line.LineNumber;
  }

  IMAGEHLP_MODULE64 module{};
  module.SizeOfStruct = sizeof(IMAGEHLP_MODULE64);
  if (SymGetModuleInfo64(process, address, &module) == TRUE) {
    out.module_base = module.BaseOfImage;
    if (address >= module.BaseOfImage) {
      out.module_offset = address - module.BaseOfImage;
    }
    if (module.ModuleName[0] != '\0') {
      out.module = module.ModuleName;
    }
    if (module.LoadedImageName[0] != '\0') {
      out.module_path = module.LoadedImageName;
    } else if (module.ImageName[0] != '\0') {
      out.module_path = module.ImageName;
    }
  }

  if (out.module_path.empty() && !executable_path.empty()) {
    out.module_path = executable_path.generic_string();
  }
  if (out.module.empty() && !out.module_path.empty()) {
    out.module = std::filesystem::path(out.module_path).stem().string();
  }
  if (out.symbol.empty()) {
    TryResolveFrameFromMap(&out);
  }
  return out;
}

std::vector<CrashFrame> CaptureFrames(HANDLE process,
                                      HANDLE thread,
                                      CONTEXT context,
                                      std::size_t max_frames,
                                      const std::filesystem::path& executable_path) {
  std::vector<CrashFrame> frames;
  SymbolSession symbols(process, executable_path);
  if (!symbols.active || max_frames == 0) {
    return frames;
  }

  STACKFRAME64 frame{};
  DWORD machine_type = IMAGE_FILE_MACHINE_AMD64;
  frame.AddrPC.Offset = context.Rip;
  frame.AddrPC.Mode = AddrModeFlat;
  frame.AddrFrame.Offset = context.Rbp;
  frame.AddrFrame.Mode = AddrModeFlat;
  frame.AddrStack.Offset = context.Rsp;
  frame.AddrStack.Mode = AddrModeFlat;

  frames.push_back(CaptureFrame(process, context.Rip, 0, executable_path));

  std::size_t next_index = 1;
  while (frames.size() < max_frames) {
    const BOOL ok =
        StackWalk64(machine_type, process, thread, &frame, &context, nullptr,
                    SymFunctionTableAccess64, SymGetModuleBase64, nullptr);
    if (!ok || frame.AddrPC.Offset == 0) {
      break;
    }
    if (!frames.empty() && frames.back().address == frame.AddrPC.Offset) {
      continue;
    }
    frames.push_back(CaptureFrame(process, frame.AddrPC.Offset, next_index,
                                  executable_path));
    ++next_index;
  }
  return frames;
}

std::string HumanMessageForException(DWORD code) {
  if (code == EXCEPTION_STACK_OVERFLOW) {
    return "Unhandled stack overflow.";
  }
  return "Unhandled structured exception.";
}

CrashReport BuildCrashReport(const CrashRuntimeOptions& options,
                             std::string_view kind,
                             DWORD process_id,
                             DWORD thread_id,
                             DWORD exception_code,
                             std::string exception_name,
                             std::string message,
                             const CrashArtifacts& artifacts,
                             std::vector<CrashFrame> frames) {
  CrashReport report;
  report.tool = options.tool_name;
  report.version = options.tool_version;
  report.timestamp_utc = NowUtcString();
  report.kind = std::string(kind);
  report.process_id = process_id;
  report.thread_id = thread_id;
  report.exception_code_value = exception_code;
  report.exception_name = std::move(exception_name);
  report.message = std::move(message);
  report.arguments = options.arguments;
  report.working_directory = options.working_directory;
  report.executable_path = options.executable_path;
  report.artifacts = artifacts;
  report.frames = std::move(frames);
  return report;
}

CrashReport CaptureCrashReport(const CrashRuntimeOptions& options,
                               std::string_view kind,
                               HANDLE process,
                               DWORD process_id,
                               HANDLE thread,
                               DWORD thread_id,
                               DWORD exception_code,
                               const std::string& message,
                               EXCEPTION_RECORD* exception_record,
                               CONTEXT* context) {
  const CrashArtifacts artifacts =
      MakeArtifacts(options.report_root.empty() ? TempCrashRoot()
                                                : options.report_root,
                    kind, process_id);
  std::vector<CrashFrame> frames;
  if (context != nullptr && thread != nullptr) {
    frames = CaptureFrames(process, thread, *context, options.max_frames,
                           options.executable_path);
  }
  if (options.write_minidump) {
    WriteMinidump(artifacts, process, process_id, thread_id, exception_record,
                  context);
  }
  return BuildCrashReport(options, kind, process_id, thread_id, exception_code,
                          exception_code == 0 ? std::string{}
                                              : ExceptionName(exception_code),
                          message, artifacts, std::move(frames));
}

void DrainPipe(HANDLE pipe, std::string* output) {
  char buffer[4096];
  DWORD bytes_read = 0;
  while (ReadFile(pipe, buffer, sizeof(buffer), &bytes_read, nullptr) &&
         bytes_read > 0) {
    output->append(buffer, buffer + bytes_read);
  }
  CloseHandle(pipe);
}

DebugRunResult RunProcessWithoutDebugger(const DebugRunOptions& options) {
  DebugRunResult result;
  STARTUPINFOW si{};
  si.cb = sizeof(si);
  PROCESS_INFORMATION pi{};
  std::wstring cmd = BuildCommandLine(options.program, options.arguments);
  std::vector<wchar_t> cmd_buf(cmd.begin(), cmd.end());
  cmd_buf.push_back(L'\0');
  const std::wstring cwd =
      options.working_directory.empty()
          ? options.program.parent_path().wstring()
          : options.working_directory.wstring();

  const BOOL ok = CreateProcessW(
      options.program.wstring().c_str(), cmd_buf.data(), nullptr, nullptr, FALSE,
      0, nullptr, cwd.empty() ? nullptr : cwd.c_str(), &si, &pi);
  if (!ok) {
    result.launch_error = DescribeWin32Error(GetLastError());
    return result;
  }

  result.launched = true;
  WaitForSingleObject(pi.hProcess, INFINITE);
  DWORD exit_code = 1;
  GetExitCodeProcess(pi.hProcess, &exit_code);
  result.exit_code = static_cast<int>(exit_code);
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);
  return result;
}

bool IsBenignFirstChance(DWORD code) {
  return code == EXCEPTION_BREAKPOINT || code == EXCEPTION_SINGLE_STEP;
}

DebugRunResult DebugRunWindows(const DebugRunOptions& options) {
  DebugRunResult result;
  if (options.program.empty()) {
    return result;
  }
  if (!options.enabled) {
    return RunProcessWithoutDebugger(options);
  }

  SECURITY_ATTRIBUTES sa{};
  sa.nLength = sizeof(sa);
  sa.bInheritHandle = TRUE;

  HANDLE stdout_read = nullptr;
  HANDLE stdout_write = nullptr;
  HANDLE stderr_read = nullptr;
  HANDLE stderr_write = nullptr;
  if (!CreatePipe(&stdout_read, &stdout_write, &sa, 0)) {
    result.launch_error = DescribeWin32Error(GetLastError());
    return result;
  }
  if (!CreatePipe(&stderr_read, &stderr_write, &sa, 0)) {
    result.launch_error = DescribeWin32Error(GetLastError());
    CloseHandle(stdout_read);
    CloseHandle(stdout_write);
    return result;
  }
  SetHandleInformation(stdout_read, HANDLE_FLAG_INHERIT, 0);
  SetHandleInformation(stderr_read, HANDLE_FLAG_INHERIT, 0);

  STARTUPINFOW si{};
  si.cb = sizeof(si);
  si.dwFlags |= STARTF_USESTDHANDLES;
  si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
  si.hStdOutput = stdout_write;
  si.hStdError = stderr_write;

  PROCESS_INFORMATION pi{};
  std::wstring cmd = BuildCommandLine(options.program, options.arguments);
  std::vector<wchar_t> cmd_buf(cmd.begin(), cmd.end());
  cmd_buf.push_back(L'\0');
  const std::wstring cwd =
      options.working_directory.empty()
          ? options.program.parent_path().wstring()
          : options.working_directory.wstring();

  const BOOL ok = CreateProcessW(
      options.program.wstring().c_str(), cmd_buf.data(), nullptr, nullptr, TRUE,
      DEBUG_ONLY_THIS_PROCESS | CREATE_NO_WINDOW, nullptr,
      cwd.empty() ? nullptr : cwd.c_str(), &si, &pi);

  CloseHandle(stdout_write);
  CloseHandle(stderr_write);

  if (!ok) {
    result.launch_error = DescribeWin32Error(GetLastError());
    CloseHandle(stdout_read);
    CloseHandle(stderr_read);
    return result;
  }

  result.launched = true;
  std::thread stdout_thread(DrainPipe, stdout_read, &result.stdout_text);
  std::thread stderr_thread(DrainPipe, stderr_read, &result.stderr_text);

  while (true) {
    DEBUG_EVENT event{};
    if (!WaitForDebugEvent(&event, INFINITE)) {
      break;
    }

    DWORD continue_status = DBG_CONTINUE;
    switch (event.dwDebugEventCode) {
      case CREATE_PROCESS_DEBUG_EVENT:
        if (event.u.CreateProcessInfo.hFile != nullptr) {
          CloseHandle(event.u.CreateProcessInfo.hFile);
        }
        break;
      case LOAD_DLL_DEBUG_EVENT:
        if (event.u.LoadDll.hFile != nullptr) {
          CloseHandle(event.u.LoadDll.hFile);
        }
        break;
      case EXCEPTION_DEBUG_EVENT: {
        const auto& exception = event.u.Exception.ExceptionRecord;
        const bool first_chance = event.u.Exception.dwFirstChance != 0;
        if (first_chance && IsBenignFirstChance(exception.ExceptionCode)) {
          continue_status = DBG_CONTINUE;
          break;
        }
        if (first_chance) {
          continue_status = DBG_EXCEPTION_NOT_HANDLED;
          break;
        }

        HANDLE thread =
            OpenThread(THREAD_GET_CONTEXT | THREAD_SUSPEND_RESUME |
                           THREAD_QUERY_INFORMATION,
                       FALSE, event.dwThreadId);
        CONTEXT context{};
        context.ContextFlags = CONTEXT_FULL;
        if (thread != nullptr) {
          GetThreadContext(thread, &context);
        }

        const CrashRuntimeOptions runtime_options = {
            options.enabled,
            options.write_minidump,
            options.emit_stderr_summary,
            false,
            options.max_frames,
            options.report_root.empty()
                ? DefaultTargetCrashReportRoot(options.program)
                : options.report_root,
            options.tool_name,
            options.tool_version,
            options.arguments,
            PathString(options.working_directory.empty() ? options.program.parent_path()
                                                         : options.working_directory),
            options.program};
        const CrashArtifacts artifacts = MakeArtifacts(
            runtime_options.report_root, CrashKindFromException(exception.ExceptionCode),
            pi.dwProcessId);
        auto frames = thread != nullptr
                          ? CaptureFrames(pi.hProcess, thread, context,
                                          options.max_frames, options.program)
                          : std::vector<CrashFrame>{};
        EXCEPTION_RECORD record_copy = exception;
        CONTEXT context_copy = context;
        if (options.write_minidump && thread != nullptr) {
          WriteMinidump(artifacts, pi.hProcess, pi.dwProcessId, event.dwThreadId,
                        &record_copy, &context_copy);
        }
        result.crashed = true;
        result.crash_report = BuildCrashReport(
            runtime_options, CrashKindFromException(exception.ExceptionCode),
            pi.dwProcessId, event.dwThreadId, exception.ExceptionCode,
            ExceptionName(exception.ExceptionCode),
            HumanMessageForException(exception.ExceptionCode), artifacts,
            std::move(frames));
        if (thread != nullptr) {
          CloseHandle(thread);
        }
        continue_status = DBG_EXCEPTION_NOT_HANDLED;
        break;
      }
      case EXIT_PROCESS_DEBUG_EVENT:
        result.exit_code = static_cast<int>(event.u.ExitProcess.dwExitCode);
        ContinueDebugEvent(event.dwProcessId, event.dwThreadId, DBG_CONTINUE);
        goto done;
      default:
        break;
    }
    ContinueDebugEvent(event.dwProcessId, event.dwThreadId, continue_status);
  }

done:
  WaitForSingleObject(pi.hProcess, INFINITE);
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);
  stdout_thread.join();
  stderr_thread.join();

  if (result.crash_report.has_value()) {
    WriteTextFile(result.crash_report->artifacts.stdout_path, result.stdout_text);
    WriteTextFile(result.crash_report->artifacts.stderr_path, result.stderr_text);
    WriteTextFile(result.crash_report->artifacts.text_path,
                  CrashSummary(*result.crash_report));
    WriteTextFile(result.crash_report->artifacts.json_path,
                  CrashReportToJson(*result.crash_report));
    if (options.emit_stderr_summary) {
      const std::string summary = CrashSummary(*result.crash_report);
      std::fwrite(summary.data(), 1, summary.size(), stderr);
      std::fflush(stderr);
    }
  }

  return result;
}

CrashReport CaptureCurrentProcessCrash(const CrashRuntimeOptions& options,
                                       std::string_view kind,
                                       DWORD exception_code,
                                       const std::string& message,
                                       EXCEPTION_RECORD* exception_record,
                                       CONTEXT* context) {
  return CaptureCrashReport(options, kind, GetCurrentProcess(),
                            GetCurrentProcessId(), GetCurrentThread(),
                            GetCurrentThreadId(), exception_code, message,
                            exception_record, context);
}

void EmitCrashOutputs(const CrashRuntimeOptions& options,
                      const CrashReport& report) {
  WriteTextFile(report.artifacts.text_path, CrashSummary(report));
  WriteTextFile(report.artifacts.json_path, CrashReportToJson(report));
  if (options.emit_stderr_summary) {
    const std::string summary = CrashSummary(report);
    std::fwrite(summary.data(), 1, summary.size(), stderr);
    std::fflush(stderr);
  }
  if (options.emit_json_stdout) {
    const std::string json = CrashEnvelopeToJson(report);
    std::fwrite(json.data(), 1, json.size(), stdout);
    std::fwrite("\n", 1, 1, stdout);
    std::fflush(stdout);
  }
}

struct DeferredCrashCaptureRequest {
  HANDLE completion_event = nullptr;
  DWORD thread_id = 0;
  DWORD exception_code = 0;
  bool has_exception_record = false;
  bool has_context = false;
  EXCEPTION_RECORD exception_record{};
  CONTEXT context{};
};

unsigned __stdcall DeferredCrashCaptureThreadProc(void* raw_request) {
  auto* request = static_cast<DeferredCrashCaptureRequest*>(raw_request);
  if (request == nullptr) {
    return 0;
  }

  const CrashRuntimeOptions options = CrashOptionsSnapshot();
  HANDLE crash_thread =
      OpenThread(THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION, FALSE,
                 request->thread_id);
  CrashReport report = CaptureCrashReport(
      options, CrashKindFromException(request->exception_code),
      GetCurrentProcess(), GetCurrentProcessId(), crash_thread,
      request->thread_id, request->exception_code,
      HumanMessageForException(request->exception_code),
      request->has_exception_record ? &request->exception_record : nullptr,
      request->has_context ? &request->context : nullptr);
  if (crash_thread != nullptr) {
    CloseHandle(crash_thread);
  }
  EmitCrashOutputs(options, report);
  if (request->completion_event != nullptr) {
    SetEvent(request->completion_event);
  }
  return 0;
}

void CaptureCrashOffThread(EXCEPTION_POINTERS* info) {
  auto* request = static_cast<DeferredCrashCaptureRequest*>(
      HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                sizeof(DeferredCrashCaptureRequest)));
  if (request == nullptr) {
    return;
  }

  request->thread_id = GetCurrentThreadId();
  if (info != nullptr && info->ExceptionRecord != nullptr) {
    request->exception_code = info->ExceptionRecord->ExceptionCode;
    request->exception_record = *info->ExceptionRecord;
    request->has_exception_record = true;
  }
  if (info != nullptr && info->ContextRecord != nullptr) {
    request->context = *info->ContextRecord;
    request->has_context = true;
  }

  request->completion_event = CreateEventW(nullptr, TRUE, FALSE, nullptr);
  if (request->completion_event == nullptr) {
    HeapFree(GetProcessHeap(), 0, request);
    return;
  }

  unsigned thread_id = 0;
  HANDLE worker = reinterpret_cast<HANDLE>(
      _beginthreadex(nullptr, 0, DeferredCrashCaptureThreadProc, request, 0,
                     &thread_id));
  if (worker == nullptr) {
    CloseHandle(request->completion_event);
    HeapFree(GetProcessHeap(), 0, request);
    return;
  }

  WaitForSingleObject(request->completion_event, 30000);
  WaitForSingleObject(worker, 30000);
  CloseHandle(worker);
  CloseHandle(request->completion_event);
  HeapFree(GetProcessHeap(), 0, request);
}

LONG WINAPI UnhandledExceptionFilterThunk(EXCEPTION_POINTERS* info) {
  if (!State().options.enabled) {
    return EXCEPTION_CONTINUE_SEARCH;
  }
  bool expected = false;
  if (!HandlingCrash().compare_exchange_strong(expected, true)) {
    return EXCEPTION_EXECUTE_HANDLER;
  }

  if (info != nullptr && info->ExceptionRecord != nullptr &&
      info->ExceptionRecord->ExceptionCode == EXCEPTION_STACK_OVERFLOW) {
    _resetstkoflw();
  }
  CaptureCrashOffThread(info);
  return EXCEPTION_EXECUTE_HANDLER;
}

[[noreturn]] void TerminateHandlerThunk() {
  if (!State().options.enabled) {
    std::abort();
  }
  bool expected = false;
  if (!HandlingCrash().compare_exchange_strong(expected, true)) {
    std::_Exit(3);
  }
  CONTEXT context{};
  RtlCaptureContext(&context);
  CrashReport report = CaptureCurrentProcessCrash(
      State().options, "terminate", 0u,
      "std::terminate was invoked.", nullptr, &context);
  EmitCrashOutputs(State().options, report);
  std::_Exit(3);
}

void AbortSignalHandlerThunk(int) {
  if (!State().options.enabled) {
    std::_Exit(134);
  }
  bool expected = false;
  if (!HandlingCrash().compare_exchange_strong(expected, true)) {
    std::_Exit(134);
  }
  CONTEXT context{};
  RtlCaptureContext(&context);
  CrashReport report = CaptureCurrentProcessCrash(
      State().options, "abort", 0u, "abort() was invoked.", nullptr, &context);
  EmitCrashOutputs(State().options, report);
  std::_Exit(134);
}

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4717)
#endif
[[noreturn]] void TriggerStackOverflowFixture() {
  alignas(16) volatile char padding[4096]{};
  padding[0] = 1;
  (void)padding;
  TriggerStackOverflowFixture();
}
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif  // _WIN32

}  // namespace

std::filesystem::path DefaultCrashReportRoot(
    const std::filesystem::path& output_root) {
  if (output_root.empty()) {
    return TempCrashRoot();
  }
  return output_root / "logs" / "crash";
}

std::filesystem::path DefaultTargetCrashReportRoot(
    const std::filesystem::path& program_path) {
  if (program_path.empty()) {
    return TempCrashRoot();
  }
  const std::filesystem::path parent = program_path.parent_path();
  if (parent.filename() == "bin") {
    return parent.parent_path() / "logs" / "crash";
  }
  return parent / "logs" / "crash";
}

void ConfigureCrashRuntime(const CrashRuntimeOptions& options) {
  std::lock_guard<std::mutex> lock(StateMutex());
  State().options = options;
  if (State().options.report_root.empty()) {
    State().options.report_root = TempCrashRoot();
  }
  if (State().options.working_directory.empty()) {
    std::error_code ec;
    State().options.working_directory =
        std::filesystem::current_path(ec).generic_string();
  }
}

void UpdateCrashReportRoot(const std::filesystem::path& report_root) {
  std::lock_guard<std::mutex> lock(StateMutex());
  State().options.report_root =
      report_root.empty() ? TempCrashRoot() : report_root;
}

void SetCrashJsonStdout(bool enabled) {
  std::lock_guard<std::mutex> lock(StateMutex());
  State().options.emit_json_stdout = enabled;
}

void SetCrashEnabled(bool enabled) {
  std::lock_guard<std::mutex> lock(StateMutex());
  State().options.enabled = enabled;
}

bool CrashReportingEnabled() {
  std::lock_guard<std::mutex> lock(StateMutex());
  return State().options.enabled;
}

bool CrashCaptureSupported() {
#ifdef _WIN32
  return true;
#else
  return false;
#endif
}

void InstallCrashHandlers() {
#ifdef _WIN32
  std::lock_guard<std::mutex> lock(StateMutex());
  if (State().installed) {
    return;
  }
  ULONG stack_guarantee = 64 * 1024;
  SetThreadStackGuarantee(&stack_guarantee);
  SetUnhandledExceptionFilter(UnhandledExceptionFilterThunk);
  std::set_terminate(TerminateHandlerThunk);
  std::signal(SIGABRT, AbortSignalHandlerThunk);
  State().installed = true;
#endif
}

void MaybeTriggerCrashFixtureFromEnv() {
#ifdef _WIN32
  char* value = nullptr;
  std::size_t value_len = 0;
  if (_dupenv_s(&value, &value_len, "CURSIVE_CRASH_TEST") != 0 ||
      value == nullptr || value_len == 0) {
    return;
  }

  const std::string mode(value);
  std::free(value);

  if (mode == "stack-overflow") {
    TriggerStackOverflowFixture();
  }
  if (mode == "access-violation") {
    *static_cast<volatile int*>(nullptr) = 1;
  }
  if (mode == "abort") {
    std::abort();
  }
  if (mode == "terminate") {
    std::terminate();
  }
#endif
}

std::string CrashReportToJson(const CrashReport& report) {
  std::ostringstream oss;
  oss << "{";
  oss << "\"tool\":\"" << EscapeJson(report.tool) << "\",";
  oss << "\"version\":\"" << EscapeJson(report.version) << "\",";
  oss << "\"timestamp_utc\":\"" << EscapeJson(report.timestamp_utc) << "\",";
  oss << "\"kind\":\"" << EscapeJson(report.kind) << "\",";
  oss << "\"process_id\":" << report.process_id << ",";
  oss << "\"thread_id\":" << report.thread_id << ",";
  oss << "\"exception_code\":" << report.exception_code_value << ",";
  oss << "\"exception_name\":\"" << EscapeJson(report.exception_name) << "\",";
  oss << "\"message\":\"" << EscapeJson(report.message) << "\",";
  oss << "\"working_directory\":\"" << EscapeJson(report.working_directory) << "\",";
  oss << "\"executable_path\":\"" << EscapeJson(PathString(report.executable_path))
      << "\",";
  oss << "\"arguments\":[";
  for (std::size_t i = 0; i < report.arguments.size(); ++i) {
    if (i != 0) {
      oss << ",";
    }
    oss << "\"" << EscapeJson(report.arguments[i]) << "\"";
  }
  oss << "],";
  oss << "\"artifacts\":{";
  oss << "\"report_dir\":\"" << EscapeJson(PathString(report.artifacts.report_dir))
      << "\",";
  oss << "\"text_path\":\"" << EscapeJson(PathString(report.artifacts.text_path))
      << "\",";
  oss << "\"json_path\":\"" << EscapeJson(PathString(report.artifacts.json_path))
      << "\",";
  oss << "\"minidump_path\":\""
      << EscapeJson(PathString(report.artifacts.minidump_path)) << "\",";
  oss << "\"stdout_path\":\""
      << EscapeJson(PathString(report.artifacts.stdout_path)) << "\",";
  oss << "\"stderr_path\":\""
      << EscapeJson(PathString(report.artifacts.stderr_path)) << "\"";
  oss << "},";
  oss << "\"frames\":[";
  for (std::size_t i = 0; i < report.frames.size(); ++i) {
    if (i != 0) {
      oss << ",";
    }
    const auto& frame = report.frames[i];
    oss << "{";
    oss << "\"index\":" << frame.index << ",";
    oss << "\"module\":\"" << EscapeJson(frame.module) << "\",";
    oss << "\"module_path\":\"" << EscapeJson(frame.module_path) << "\",";
    oss << "\"symbol\":\"" << EscapeJson(frame.symbol) << "\",";
    oss << "\"file\":\"" << EscapeJson(frame.file) << "\",";
    oss << "\"line\":" << frame.line << ",";
    oss << "\"address\":\"" << EscapeJson(Hex64(frame.address)) << "\",";
    oss << "\"module_base\":\"" << EscapeJson(Hex64(frame.module_base)) << "\",";
    oss << "\"module_offset\":" << frame.module_offset << ",";
    oss << "\"offset\":" << frame.offset << ",";
    oss << "\"inline\":" << (frame.inline_frame ? "true" : "false");
    oss << "}";
  }
  oss << "]";
  oss << "}";
  return oss.str();
}

std::string CrashEnvelopeToJson(const CrashReport& report) {
  std::ostringstream oss;
  oss << "{";
  oss << "\"diagnostics\":[],";
  oss << "\"crash\":" << CrashReportToJson(report);
  oss << "}";
  return oss.str();
}

std::string CrashSummary(const CrashReport& report) {
  std::ostringstream oss;
  oss << "fatal: " << report.message;
  if (!report.exception_name.empty()) {
    oss << " (" << report.exception_name;
    if (report.exception_code_value != 0) {
      oss << ", " << Hex32(report.exception_code_value);
    }
    oss << ")";
  }
  oss << "\n";
  if (!report.tool.empty()) {
    oss << "tool: " << report.tool;
    if (!report.version.empty()) {
      oss << " " << report.version;
    }
    oss << "\n";
  }
  if (!report.executable_path.empty()) {
    oss << "executable: " << report.executable_path.generic_string() << "\n";
  }
  if (!report.working_directory.empty()) {
    oss << "cwd: " << report.working_directory << "\n";
  }
  if (!report.artifacts.report_dir.empty()) {
    oss << "report_dir: " << report.artifacts.report_dir.generic_string() << "\n";
  }
  if (!report.artifacts.json_path.empty()) {
    oss << "report_json: " << report.artifacts.json_path.generic_string() << "\n";
  }
  if (!report.artifacts.minidump_path.empty()) {
    oss << "minidump: " << report.artifacts.minidump_path.generic_string() << "\n";
  }
  if (!report.arguments.empty()) {
    oss << "args: " << JoinArguments(report.arguments) << "\n";
  }
  if (!report.frames.empty()) {
    oss << "stacktrace:\n";
    const std::size_t limit = std::min<std::size_t>(report.frames.size(), 32);
    for (std::size_t i = 0; i < limit; ++i) {
      const auto& frame = report.frames[i];
      oss << "  [" << frame.index << "] "
          << (frame.module.empty() ? "<unknown>" : frame.module) << "!"
          << (frame.symbol.empty() ? "<unknown>" : frame.symbol)
          << " +" << HexCompact64(frame.symbol.empty() ? frame.module_offset
                                                       : frame.offset)
          << " @ " << Hex64(frame.address);
      if (!frame.file.empty() && frame.line != 0) {
        oss << " (" << frame.file << ":" << frame.line << ")";
      }
      oss << "\n";
    }
  }
  return oss.str();
}

DebugRunResult DebugRunProcess(const DebugRunOptions& options) {
#ifdef _WIN32
  if (!options.enabled) {
    return RunProcessWithoutDebugger(options);
  }
  return DebugRunWindows(options);
#else
  (void)options;
  return {};
#endif
}

}  // namespace cursive::core
