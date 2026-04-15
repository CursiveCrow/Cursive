#pragma once

#include <filesystem>
#include <optional>
#include <string>

namespace cursive::core {

bool EnsureBundledWindowsCompilerSupport(std::string* error_message = nullptr);

std::optional<std::filesystem::path> BundledWindowsToolsDirPath();
std::optional<std::filesystem::path> BundledWindowsLinkerPath();
std::optional<std::filesystem::path> BundledWindowsArchiverPath();
std::optional<std::filesystem::path> BundledWindowsAssemblerPath();

std::optional<std::filesystem::path> BundledWindowsRuntimeLibPath();
std::optional<std::filesystem::path> BundledWindowsDelayImpLibPath();

}  // namespace cursive::core
