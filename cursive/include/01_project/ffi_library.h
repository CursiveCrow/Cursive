#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "01_project/target_profile.h"
#include "02_source/ast/ast.h"

namespace cursive::project {

struct FfiLibrarySpec {
  std::string name;
  std::string kind;
};

std::optional<FfiLibrarySpec> NormalizeLibraryAttribute(
    const ast::AttributeItem& attr);

std::vector<FfiLibrarySpec> CollectExternLibrarySpecs(
    const std::vector<ast::ASTModule>& modules);

bool IsLibraryKindSupportedForCurrentTarget(std::string_view kind,
                                            TargetProfile profile);

std::optional<std::string> ResolveLibraryNameForCurrentTarget(
    std::string_view name,
    std::string_view kind,
    TargetProfile profile);

std::optional<std::filesystem::path> ResolveLibraryLinkInputForCurrentTarget(
    std::string_view name,
    std::string_view kind,
    TargetProfile profile);

std::vector<std::filesystem::path> ResolveExternLibraryInputs(
    const std::vector<FfiLibrarySpec>& specs,
    TargetProfile profile);

}  // namespace cursive::project
