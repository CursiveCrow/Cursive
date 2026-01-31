#include "cursive0/01_project/manifest.h"

#include <exception>
#include <filesystem>
#include <system_error>

#include "cursive0/00_core/assert_spec.h"
#include "cursive0/00_core/diagnostic_messages.h"
#include "cursive0/00_core/diagnostics.h"
#include "cursive0/00_core/host_primitives.h"

namespace cursive0::project {

namespace {

std::filesystem::path StartDirForInput(const std::filesystem::path& input_path) {
  std::filesystem::path dir = input_path;
  if (dir.has_filename()) {
    dir = dir.parent_path();
  }
  if (dir.empty()) {
    std::error_code ec;
    const auto cwd = std::filesystem::current_path(ec);
    if (!ec) {
      dir = cwd;
    }
  }
  if (dir.empty()) {
    dir = ".";
  }
  return dir;
}

std::optional<core::Diagnostic> MakeExternalDiag(std::string_view code) {
  auto diag = core::MakeDiagnostic(code);
  if (diag) {
    diag->span.reset();
  }
  return diag;
}

}  // namespace

std::filesystem::path FindProjectRoot(const std::filesystem::path& input_path) {
  const std::filesystem::path start = StartDirForInput(input_path);
  std::filesystem::path current = start;

  for (;;) {
    std::error_code ec;
    const auto manifest_path = current / "Cursive.toml";
    const bool exists = std::filesystem::exists(manifest_path, ec);
    if (ec) {
      return current;
    }
    if (exists) {
      return current;
    }
    const auto parent = current.parent_path();
    if (parent.empty() || parent == current) {
      break;
    }
    current = parent;
  }

  return start;
}

ManifestParseResult ParseManifest(const std::filesystem::path& project_root) {
  ManifestParseResult result;
  const auto manifest_path = project_root / "Cursive.toml";

  std::error_code ec;
  const bool exists = std::filesystem::exists(manifest_path, ec);
  if (ec) {
    SPEC_RULE("Parse-Manifest-Err");
    if (auto diag = MakeExternalDiag("E-PRJ-0102")) {
      result.diags = core::Emit(result.diags, *diag);
    }
    core::HostPrimFail(core::HostPrim::ParseTOML, true);
    return result;
  }

  if (!exists) {
    SPEC_RULE("Parse-Manifest-Missing");
    if (auto diag = MakeExternalDiag("E-PRJ-0101")) {
      result.diags = core::Emit(result.diags, *diag);
    }
    return result;
  }

  try {
    result.table = toml::parse_file(manifest_path.string());
    SPEC_RULE("Parse-Manifest-Ok");
    return result;
  } catch (const toml::parse_error&) {
    // Fall through to error handling.
  } catch (const std::exception&) {
    // Fall through to error handling.
  }

  SPEC_RULE("Parse-Manifest-Err");
  if (auto diag = MakeExternalDiag("E-PRJ-0102")) {
    result.diags = core::Emit(result.diags, *diag);
  }
  core::HostPrimFail(core::HostPrim::ParseTOML, true);
  return result;
}

}  // namespace cursive0::project
