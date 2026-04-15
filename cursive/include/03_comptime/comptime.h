#pragma once

#include <filesystem>
#include <optional>
#include <vector>

#include "00_core/diagnostics.h"
#include "02_source/ast/ast.h"

namespace cursive::frontend {

struct ComptimeResult {
  std::optional<std::vector<ast::ASTModule>> modules;
  core::DiagnosticStream diags;
};

ComptimeResult ComptimePass(const std::vector<ast::ASTModule>& modules,
                            const std::filesystem::path& project_root,
                            const std::filesystem::path& source_root);

ComptimeResult ExecuteComptime(const std::vector<ast::ASTModule>& modules,
                               const std::filesystem::path& project_root,
                               const std::filesystem::path& source_root);

}  // namespace cursive::frontend
