#include "cursive0/core/path.h"

#include <cstddef>

#include "cursive0/core/assert_spec.h"

namespace cursive0::core {

bool IsWinSep(char c) {
  return c == '/' || c == '\\';
}

bool IsAsciiLetter(char c) {
  return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

static bool StartsWith(std::string_view s, std::string_view prefix) {
  return s.size() >= prefix.size() && s.substr(0, prefix.size()) == prefix;
}

bool DriveRooted(std::string_view p) {
  return p.size() >= 3 && IsAsciiLetter(p[0]) && p[1] == ':' && IsWinSep(p[2]);
}

bool UNC(std::string_view p) {
  return StartsWith(p, "//") || StartsWith(p, "\\\\");
}

bool RootRelative(std::string_view p) {
  return !p.empty() && IsWinSep(p[0]) && !UNC(p) && !DriveRooted(p);
}

bool AbsPath(std::string_view p) {
  return DriveRooted(p) || UNC(p) || RootRelative(p);
}

bool IsRelative(std::string_view p) {
  return !AbsPath(p);
}

std::string RootTag(std::string_view p) {
  if (DriveRooted(p)) {
    return std::string(p.substr(0, 2));
  }
  if (UNC(p)) {
    return "//";
  }
  if (RootRelative(p)) {
    return "/";
  }
  return "";
}

std::string Tail(std::string_view p) {
  if (DriveRooted(p)) {
    return std::string(p.substr(3));
  }
  if (UNC(p)) {
    return std::string(p.substr(2));
  }
  if (RootRelative(p)) {
    return std::string(p.substr(1));
  }
  return std::string(p);
}

std::vector<std::string> Segs(std::string_view p) {
  std::vector<std::string> segs;
  std::size_t i = 0;
  while (i < p.size()) {
    while (i < p.size() && IsWinSep(p[i])) {
      ++i;
    }
    const std::size_t start = i;
    while (i < p.size() && !IsWinSep(p[i])) {
      ++i;
    }
    if (i > start) {
      segs.emplace_back(p.substr(start, i - start));
    }
  }
  return segs;
}

std::vector<std::string> PathComps(std::string_view p) {
  const std::string root = RootTag(p);
  if (root.empty()) {
    return Segs(p);
  }
  std::vector<std::string> comps;
  comps.push_back(root);
  const auto tail = Tail(p);
  auto segs = Segs(tail);
  comps.insert(comps.end(), segs.begin(), segs.end());
  return comps;
}

static std::string JoinCompRec(const std::vector<std::string>& comps,
                               std::size_t index) {
  if (index >= comps.size()) {
    return "";
  }
  if (index + 1 == comps.size()) {
    return comps[index];
  }
  const std::string& c = comps[index];
  const std::string rest = JoinCompRec(comps, index + 1);
  if (c == "/" || c == "//") {
    return c + rest;
  }
  return c + "/" + rest;
}

std::string JoinComp(const std::vector<std::string>& comps) {
  return JoinCompRec(comps, 0);
}

std::string Join(std::string_view a, std::string_view b) {
  if (AbsPath(b)) {
    return std::string(b);
  }
  auto comps = PathComps(a);
  auto tail = PathComps(b);
  comps.insert(comps.end(), tail.begin(), tail.end());
  return JoinComp(comps);
}

std::string Normalize(std::string_view p) {
  auto comps = PathComps(p);
  std::vector<std::string> filtered;
  filtered.reserve(comps.size());
  for (const auto& c : comps) {
    if (c == ".") {
      continue;
    }
    filtered.push_back(c);
  }
  return JoinComp(filtered);
}

std::optional<std::string> Canon(std::string_view p) {
  const std::string norm = Normalize(p);
  const auto comps = PathComps(norm);
  for (const auto& c : comps) {
    if (c == "..") {
      return std::nullopt;
    }
  }
  return norm;
}

bool PathPrefix(const std::vector<std::string>& path,
                const std::vector<std::string>& pref) {
  if (pref.size() > path.size()) {
    return false;
  }
  for (std::size_t i = 0; i < pref.size(); ++i) {
    if (path[i] != pref[i]) {
      return false;
    }
  }
  return true;
}

bool Prefix(std::string_view p, std::string_view q) {
  return PathPrefix(PathComps(p), PathComps(q));
}

std::optional<ResolveResult> Resolve(std::string_view root, std::string_view p) {
  const std::string joined = Normalize(Join(root, p));
  const auto canon_root = Canon(root);
  const auto canon_path = Canon(joined);
  if (!canon_root.has_value() || !canon_path.has_value()) {
    SPEC_RULE("Resolve-Canonical-Err");
    return std::nullopt;
  }
  SPEC_RULE("Resolve-Canonical");
  return ResolveResult{*canon_root, *canon_path};
}

std::optional<std::string> Relative(std::string_view p, std::string_view base) {
  const auto canon_path = Canon(p);
  const auto canon_base = Canon(base);
  if (!canon_path.has_value() || !canon_base.has_value()) {
    return std::nullopt;
  }
  const auto path_comps = PathComps(*canon_path);
  const auto base_comps = PathComps(*canon_base);
  if (!PathPrefix(path_comps, base_comps)) {
    return std::nullopt;
  }
  std::vector<std::string> rel_comps;
  rel_comps.insert(rel_comps.end(),
                   path_comps.begin() + static_cast<std::ptrdiff_t>(base_comps.size()),
                   path_comps.end());
  return JoinComp(rel_comps);
}

std::string Basename(std::string_view p) {
  const auto comps = PathComps(p);
  if (comps.empty()) {
    return "";
  }
  return comps.back();
}

std::string FileExt(std::string_view p) {
  const std::string base = Basename(p);
  const auto pos = base.rfind('.');
  if (pos == std::string::npos) {
    return "";
  }
  if (pos == 0) {
    return "";
  }
  return base.substr(pos);
}

}  // namespace cursive0::core
