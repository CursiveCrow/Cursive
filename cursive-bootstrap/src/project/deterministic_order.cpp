#include "cursive0/project/deterministic_order.h"

#include <algorithm>
#include <optional>
#include <system_error>

#include "cursive0/core/assert_spec.h"
#include "cursive0/core/diagnostic_messages.h"
#include "cursive0/core/path.h"
#include "cursive0/core/unicode.h"

namespace cursive0::project {

namespace {

void EmitExternal(core::DiagnosticStream& diags, std::string_view code) {
  auto diag = core::MakeDiagnostic(code);
  if (diag) {
    diag->span.reset();
    diags = core::Emit(diags, *diag);
  }
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

std::string JoinModulePath(const std::vector<std::string>& comps) {
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

}  // namespace

std::string FoldPath(std::string_view path) {
  const auto comps = core::PathComps(path);
  std::vector<std::string> folded;
  folded.reserve(comps.size());
  for (const auto& comp : comps) {
    folded.push_back(core::CaseFold(core::NFC(comp)));
  }
  return core::JoinComp(folded);
}

std::string Fold(std::string_view module_path) {
  const auto comps = SplitModulePath(module_path);
  std::vector<std::string> folded;
  folded.reserve(comps.size());
  for (const auto& comp : comps) {
    folded.push_back(core::CaseFold(core::NFC(comp)));
  }
  return JoinModulePath(folded);
}

bool Utf8LexLess(std::string_view a, std::string_view b) {
  return std::lexicographical_compare(
      a.begin(), a.end(), b.begin(), b.end(),
      [](unsigned char lhs, unsigned char rhs) { return lhs < rhs; });
}

bool KeyLess(const OrderKey& a, const OrderKey& b) {
  if (Utf8LexLess(a.folded, b.folded)) {
    return true;
  }
  if (a.folded == b.folded) {
    return Utf8LexLess(a.raw, b.raw);
  }
  return false;
}

OrderKey FileKey(const std::filesystem::path& file,
                 const std::filesystem::path& dir,
                 core::DiagnosticStream& diags) {
  const std::string file_str = file.generic_string();
  const std::string dir_str = dir.generic_string();
  const std::optional<std::string> rel = core::Relative(file_str, dir_str);
  if (!rel.has_value()) {
    SPEC_RULE("FileOrder-Rel-Fail");
    EmitExternal(diags, "E-PRJ-0303");
    return OrderKey{"", core::Basename(file_str)};
  }
  return OrderKey{FoldPath(*rel), *rel};
}

OrderKey DirKey(const std::filesystem::path& dir,
                const std::filesystem::path& base,
                core::DiagnosticStream& diags) {
  const std::string dir_str = dir.generic_string();
  const std::string base_str = base.generic_string();
  const std::optional<std::string> rel = core::Relative(dir_str, base_str);
  if (!rel.has_value()) {
    SPEC_RULE("DirSeq-Rel-Fail");
    EmitExternal(diags, "E-PRJ-0303");
    return OrderKey{"", core::Basename(dir_str)};
  }
  return OrderKey{FoldPath(*rel), *rel};
}

DirSeqResult DirSeqFrom(const std::filesystem::path& root,
                        const std::vector<std::filesystem::path>& dirs) {
  DirSeqResult result;
  struct DirEntry {
    std::filesystem::path path;
    OrderKey key;
  };
  std::vector<DirEntry> entries;
  entries.reserve(dirs.size());
  for (const auto& dir : dirs) {
    entries.push_back(DirEntry{dir, DirKey(dir, root, result.diags)});
  }
  std::stable_sort(entries.begin(), entries.end(),
                   [](const DirEntry& lhs, const DirEntry& rhs) {
                     return KeyLess(lhs.key, rhs.key);
                   });
  result.dirs.reserve(entries.size());
  for (const auto& entry : entries) {
    result.dirs.push_back(entry.path);
  }
  return result;
}

DirSeqResult DirSeq(const std::filesystem::path& root) {
  DirSeqResult result;
  std::error_code ec;
  std::filesystem::directory_iterator it(root, ec);
  if (ec) {
    SPEC_RULE("DirSeq-Read-Err");
    EmitExternal(result.diags, "E-PRJ-0305");
    return result;
  }
  std::vector<std::filesystem::path> dirs;
  const std::filesystem::directory_iterator end;
  for (; it != end; it.increment(ec)) {
    if (ec) {
      SPEC_RULE("DirSeq-Read-Err");
      EmitExternal(result.diags, "E-PRJ-0305");
      return result;
    }
    std::error_code type_ec;
    if (it->is_directory(type_ec)) {
      if (type_ec) {
        SPEC_RULE("DirSeq-Read-Err");
        EmitExternal(result.diags, "E-PRJ-0305");
        return result;
      }
      dirs.push_back(it->path());
    }
  }
  return DirSeqFrom(root, dirs);
}

}  // namespace cursive0::project
