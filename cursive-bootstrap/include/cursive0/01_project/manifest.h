#pragma once

#include <filesystem>
#include <optional>

#include <toml++/toml.hpp>

#include "cursive0/00_core/diagnostics.h"

namespace cursive0::project {

using TOMLTable = toml::table;

struct ManifestParseResult {
  std::optional<TOMLTable> table;
  core::DiagnosticStream diags;
};

std::filesystem::path FindProjectRoot(const std::filesystem::path& input_path);

ManifestParseResult ParseManifest(const std::filesystem::path& project_root);

}  // namespace cursive0::project
