#pragma once

#include <filesystem>
#include <optional>

#include <toml++/toml.hpp>

#include "00_core/diagnostics.h"

namespace cursive::project {

using TOMLTable = toml::table;

struct ManifestParseResult {
  std::optional<TOMLTable> table;
  core::DiagnosticStream diags;
};

std::filesystem::path FindProjectRoot(const std::filesystem::path& input_path);

ManifestParseResult ParseManifest(const std::filesystem::path& project_root);

}  // namespace cursive::project
