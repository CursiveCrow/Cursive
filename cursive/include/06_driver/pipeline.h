#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "00_core/diagnostics.h"
#include "00_core/source_text.h"
#include "01_project/project.h"
#include "04_analysis/resolve/resolve_items.h"
#include "04_analysis/typing/context.h"
#include "04_analysis/typing/typecheck.h"
#include "04_analysis/typing/types.h"
#include "05_codegen/ir/ir_model.h"
#include "05_codegen/lower/lower_expr.h"
#include "02_source/ast/ast.h"

namespace cursive::driver {

// Module Codegen State
struct ModuleCodegen {
  ast::ModulePath path;
  std::string path_key;
  codegen::IRDecls decls;
  codegen::LowerValueState values;
  std::unordered_map<std::string, codegen::LowerCtx::ProcSigInfo> proc_sigs;
  std::unordered_map<std::string, codegen::LinkageKind> proc_linkages;
  std::unordered_map<std::string, codegen::LowerCtx::AsyncProcInfo> async_procs;
  std::uint64_t temp_counter = 0;
  std::optional<std::string> main_symbol;
};

// Codegen Cache
struct CodegenCache {
  enum class ModuleState : std::uint8_t {
    Pending,
    InProgress,
    Done,
    Failed,
  };

  codegen::LowerCtx ctx;
  const analysis::NameMapBuildResult* name_maps = nullptr;
  std::unordered_map<std::string, codegen::LowerCtx::HostedStateTemplate>
      hosted_state_templates;
  std::vector<codegen::LowerCtx::HostedExportInfo> all_hosted_exports;
  std::optional<analysis::InitPlan> full_init_plan;
  std::vector<std::string> hosted_project_modules;
  std::vector<std::shared_ptr<ModuleCodegen>> modules;
  std::unordered_map<std::string, std::size_t> index;
  std::unordered_map<std::string, const ast::ASTModule*> ast_modules;
  std::unordered_map<std::string, std::size_t> module_order;
  std::unordered_map<std::string, std::shared_ptr<ModuleCodegen>>
      module_entries;
  std::unordered_map<std::string, ModuleState> module_states;
  std::unordered_set<std::string> lowered_proc_symbols;
  std::string active_project_context_key;
  std::uint64_t emit_context_epoch = 0;
  mutable std::mutex module_mu;
  std::condition_variable module_cv;
  std::atomic<bool> ok{true};
};

// Pipeline Result
struct PipelineResult {
  bool phase1_ok = false;
  bool resolve_ok = false;
  bool typecheck_ok = false;
  bool phase4_ok = false;
  core::DiagnosticStream diags;
};

// Inspect source before parsing and return any front-end diagnostics.
core::DiagnosticStream InspectSource(const core::SourceFile& source);

// LLVM initialization
void EnsureLLVMInit();

// File system helpers
bool EnsureDir(const std::filesystem::path& path);
bool WriteFile(const std::filesystem::path& path, std::string_view bytes);

// LLVM module emission
std::optional<std::string> EmitIRForModule(
    const CodegenCache& cache,
    const ModuleCodegen& module,
    const project::Project& project,
    project::TargetProfile target_profile);

std::optional<std::string> EmitObjForModule(
    const CodegenCache& cache,
    const ModuleCodegen& module,
    const project::Project& project,
    project::TargetProfile target_profile);

std::optional<std::string> CodegenObj(
    const CodegenCache& cache,
    const project::ModuleInfo& module,
    const project::Project& project,
    project::TargetProfile target_profile);

std::optional<std::string> CodegenIR(
    const CodegenCache& cache,
    const project::ModuleInfo& module,
    const project::Project& project,
    project::TargetProfile target_profile,
    std::string_view emit_ir);

// Codegen cache building
std::shared_ptr<CodegenCache> BuildCodegenCache(
    const project::Project& project,
    const analysis::ScopeContext& sema_ctx,
    const analysis::NameMapBuildResult& name_maps,
    const analysis::TypecheckResult& typechecked);

std::optional<std::size_t> EnsureCodegenModule(CodegenCache& cache,
                                               std::string_view module_path);
std::shared_ptr<const ModuleCodegen> EnsureCodegenModuleEntry(
    CodegenCache& cache, std::string_view module_path);
bool PopulateCodegenModules(CodegenCache& cache, const project::Project& project);
void ConfigureCodegenContextForProject(CodegenCache& cache,
                                       const project::Project& project);

// Diagnostic helpers
void AppendDiags(core::DiagnosticStream& out, const core::DiagnosticStream& add);
bool HasDiagCode(const core::DiagnosticStream& diags, std::string_view code);

}  // namespace cursive::driver
