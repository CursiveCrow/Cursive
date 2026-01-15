#pragma once

#include <filesystem>
#include <optional>
#include <string_view>
#include <vector>

namespace cursive0::project {

struct Project;

std::vector<std::filesystem::path> SearchDirs(const Project& project);

std::optional<std::filesystem::path> ResolveTool(const Project& project,
                                                 std::string_view tool);

}  // namespace cursive0::project
