#include "cursive0/00_core/symbols.h"

#include <cstddef>

#include "cursive0/00_core/unicode.h"

namespace cursive0::core {

namespace {

bool IsAsciiAlnum(unsigned char c) {
  return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') ||
         (c >= 'a' && c <= 'z');
}

char HexDigit(unsigned int value) {
  const unsigned int digit = value & 0xF;
  if (digit < 10) {
    return static_cast<char>('0' + digit);
  }
  return static_cast<char>('a' + (digit - 10));
}

std::string JoinWithDoubleColon(const std::vector<std::string>& comps) {
  if (comps.empty()) {
    return "";
  }
  std::string out = comps[0];
  for (std::size_t i = 1; i < comps.size(); ++i) {
    out.append("::");
    out.append(comps[i]);
  }
  return out;
}

std::vector<std::string> SplitModulePath(std::string_view path) {
  std::vector<std::string> parts;
  std::size_t start = 0;
  while (start <= path.size()) {
    const std::size_t pos = path.find("::", start);
    if (pos == std::string_view::npos) {
      parts.emplace_back(path.substr(start));
      break;
    }
    parts.emplace_back(path.substr(start, pos - start));
    start = pos + 2;
  }
  return parts;
}

}  // namespace

std::string StringOfPath(const std::vector<std::string>& comps) {
  return JoinWithDoubleColon(comps);
}

std::string StringOfPath(std::initializer_list<std::string_view> comps) {
  if (comps.size() == 0) {
    return "";
  }
  std::string out;
  bool first = true;
  for (const auto& comp : comps) {
    if (!first) {
      out.append("::");
    }
    out.append(comp.begin(), comp.end());
    first = false;
  }
  return out;
}

std::string PathToPrefix(std::string_view s) {
  const std::string nfc = NFC(s);
  std::string out;
  out.reserve(nfc.size());
  for (unsigned char c : nfc) {
    if (IsAsciiAlnum(c)) {
      out.push_back(static_cast<char>(c));
    } else {
      out.append("_x");
      out.push_back(HexDigit(c >> 4));
      out.push_back(HexDigit(c));
    }
  }
  return out;
}

std::string Mangle(std::string_view s) {
  return PathToPrefix(s);
}

std::string PathSig(std::initializer_list<std::string_view> comps) {
  return Mangle(StringOfPath(comps));
}

std::string MangleModulePath(std::string_view module_path) {
  auto parts = SplitModulePath(module_path);
  for (auto& part : parts) {
    part = NFC(part);
  }
  return Mangle(StringOfPath(parts));
}

}  // namespace cursive0::core
