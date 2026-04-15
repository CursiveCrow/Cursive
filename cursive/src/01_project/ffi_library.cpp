#include "01_project/ffi_library.h"

#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace cursive::project {

namespace {

std::string NormalizeAttributeStringLiteral(std::string value) {
  if (value.size() >= 2 &&
      ((value.front() == '"' && value.back() == '"') ||
       (value.front() == '\'' && value.back() == '\''))) {
    return value.substr(1, value.size() - 2);
  }
  return value;
}

std::optional<std::string> AttributeStringArgByKey(
    const ast::AttributeItem& attr,
    std::string_view key) {
  const auto token = ast::get_attr_token_arg(attr, key);
  if (!token.has_value()) {
    return std::nullopt;
  }
  return NormalizeAttributeStringLiteral(token->lexeme);
}

}  // namespace

std::optional<FfiLibrarySpec> NormalizeLibraryAttribute(
    const ast::AttributeItem& attr) {
  if (attr.name != "library") {
    return std::nullopt;
  }

  std::optional<std::string> name;
  std::optional<std::string> kind;

  for (std::size_t i = 0; i < attr.args.size(); ++i) {
    const auto& arg = attr.args[i];
    if (!arg.key.has_value()) {
      return std::nullopt;
    }
    if (*arg.key != "name" && *arg.key != "kind") {
      return std::nullopt;
    }
    const auto* token = std::get_if<ast::Token>(&arg.value);
    if (!token) {
      return std::nullopt;
    }
    const std::string normalized = NormalizeAttributeStringLiteral(token->lexeme);
    if (*arg.key == "name") {
      if (name.has_value() || normalized.empty() || i != 0) {
        return std::nullopt;
      }
      name = normalized;
    } else {
      if (kind.has_value() || normalized.empty() || !name.has_value() ||
          i != 1) {
        return std::nullopt;
      }
      kind = normalized;
    }
  }
  if (!name.has_value() || name->empty()) {
    return std::nullopt;
  }

  return FfiLibrarySpec{*name, kind.value_or("dylib")};
}

std::vector<FfiLibrarySpec> CollectExternLibrarySpecs(
    const std::vector<ast::ASTModule>& modules) {
  std::vector<FfiLibrarySpec> out;
  std::unordered_set<std::string> seen;
  for (const auto& module : modules) {
    for (const auto& item : module.items) {
      const auto* block = std::get_if<ast::ExternBlock>(&item);
      if (!block) {
        continue;
      }
      for (const auto& attr : ast::AttrListOf(*block)) {
        const auto spec = NormalizeLibraryAttribute(attr);
        if (!spec.has_value()) {
          continue;
        }
        const std::string key = spec->kind + "|" + spec->name;
        if (!seen.insert(key).second) {
          continue;
        }
        out.push_back(*spec);
      }
    }
  }
  return out;
}

bool IsLibraryKindSupportedForCurrentTarget(std::string_view kind,
                                            TargetProfile profile) {
  return LibraryKindSupported(kind, profile);
}

std::optional<std::string> ResolveLibraryNameForCurrentTarget(
    std::string_view name,
    std::string_view kind,
    TargetProfile profile) {
  return ResolveLibraryName(kind, name, profile);
}

std::optional<std::filesystem::path> ResolveLibraryLinkInputForCurrentTarget(
    std::string_view name,
    std::string_view kind,
    TargetProfile profile) {
  const auto resolved = ResolveLibraryNameForCurrentTarget(name, kind, profile);
  if (!resolved.has_value()) {
    return std::nullopt;
  }

  std::filesystem::path candidate(*resolved);
  const std::filesystem::path original_name{std::string(name)};
  if (profile == TargetProfile::X86_64Win64 &&
      !original_name.has_extension() &&
      (kind == "dylib" || kind == "raw-dylib") &&
      candidate.extension() == ".dll") {
    candidate.replace_extension(".lib");
  }
  return candidate;
}

std::vector<std::filesystem::path> ResolveExternLibraryInputs(
    const std::vector<FfiLibrarySpec>& specs,
    TargetProfile profile) {
  std::vector<std::filesystem::path> out;
  std::unordered_set<std::string> seen;
  out.reserve(specs.size());
  for (const auto& spec : specs) {
    const auto resolved =
        ResolveLibraryLinkInputForCurrentTarget(spec.name, spec.kind, profile);
    if (!resolved.has_value()) {
      continue;
    }
    const std::string key = resolved->generic_string();
    if (!seen.insert(key).second) {
      continue;
    }
    out.push_back(*resolved);
  }
  return out;
}

}  // namespace cursive::project
