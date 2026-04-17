#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "00_core/diagnostics.h"
#include "01_project/project.h"
#include "02_source/ast/ast.h"

namespace cursive::project {

struct AssemblyImportGraph {
  std::unordered_map<std::string, const Assembly*> assemblies;
  std::unordered_map<std::string, std::vector<std::string>> imports;
};

AssemblyImportGraph BuildAssemblyImportGraph(
    const Project& project,
    const std::vector<ast::ASTModule>& modules);

bool ValidateAssemblyImportGraphStructure(
    const Project& project,
    const AssemblyImportGraph& graph,
    core::DiagnosticStream& diags);

bool ValidateHostedLibraryImportGraph(
    const Project& project,
    const AssemblyImportGraph& graph,
    const std::vector<ast::ASTModule>& modules,
    core::DiagnosticStream& diags);

std::vector<ModuleInfo> ComputeEmitModules(std::string_view assembly_name,
                                           const AssemblyImportGraph& graph);

std::vector<std::string> ComputeLibraryClosure(
    std::string_view assembly_name,
    const AssemblyImportGraph& graph);
std::vector<std::string> ImportedLibraries(
    std::string_view assembly_name,
    const AssemblyImportGraph& graph);

std::optional<Project> BuildOutputProjectForAssembly(
    const Project& base_project,
    const AssemblyImportGraph& graph,
    std::string_view assembly_name);

}  // namespace cursive::project
