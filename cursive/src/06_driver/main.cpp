// =============================================================================
// main.cpp - Compiler entry point
// =============================================================================
//
// SPEC REFERENCE:
//   CursiveSpecification.md §0.3 (lines 186-204) - Observable Compiler Behavior
//   CursiveSpecification.md §5.9.6 (lines 10569-10590) - Program Entry Point
//   CursiveSpecification.md §1.1 (lines 230-240) - Conformance
//
// Phase Orchestration:
//   Phase 0: Build/Project Model (§2)
//   Phase 1: Parse (§3)
//   Phase 2: Compile-Time Execution (§4)
//   Phase 3: Name Resolution + Type Checking (§5)
//   Phase 4: Code Generation (§6)
//
// Exit Code Semantics (from spec §0.3):
//   0 = Compilation succeeded (no errors)
//   1 = Compilation failed (at least one error)
//   2 = CLI parse error (invalid arguments)
//
// =============================================================================

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "06_driver/cli.h"
#include "06_driver/pipeline.h"
#include "06_driver/shared_library_exports.h"
#include "06_driver/version.h"

#include "00_core/assert_spec.h"
#include "00_core/build_log_policy.h"
#include "00_core/crash_debug.h"
#include "00_core/diagnostic_messages.h"
#include "00_core/diagnostic_render.h"
#include "00_core/diagnostics.h"
#include "00_core/hash.h"
#include "00_core/host/services.h"
#include "00_core/ident.h"
#include "00_core/process_config.h"
#include "00_core/source_load.h"
#include "00_core/spec_trace.h"
#include "00_core/symbols.h"
#include "00_core/terminal.h"
#include "00_core/unicode.h"
#include "00_core/compiler_support.h"
#include "01_project/ir_assembly.h"
#include "01_project/assemblies.h"
#include "01_project/assembly_graph.h"
#include "01_project/ffi_library.h"
#include "01_project/link.h"
#include "01_project/manifest.h"
#include "01_project/deterministic_order.h"
#include "01_project/module_discovery.h"
#include "01_project/outputs.h"
#include "01_project/project.h"
#include "01_project/tool_resolution.h"
#include "01_project/unwind_ffi_surface.h"
#include "02_source/ast/ast_dump.h"
#include "03_comptime/comptime.h"
#include "02_source/parser/parse_modules.h"
#include "02_source/parser/parser.h"
#include "02_source/module_paths.h"
#include "04_analysis/caps/authority_model.h"
#include "04_analysis/caps/callgraph_caps.h"
#include "05_codegen/globals/globals.h"
#include "04_analysis/conformance/conformance.h"
#include "04_analysis/attributes/attribute_registry.h"
#include "04_analysis/resolve/resolve_items.h"
#include "04_analysis/resolve/resolver.h"
#include "04_analysis/resolve/scopes.h"
#include "04_analysis/resolve/scopes_lookup.h"
#include "04_analysis/resolve/visibility.h"
#include "04_analysis/typing/context.h"
#include "04_analysis/typing/typecheck.h"
#include "05_codegen/ir/ir_dump.h"
#include "05_codegen/lower/lower_expr.h"
#include "05_codegen/lower/lower_module.h"
#include "05_codegen/intrinsics/builtins.h"

namespace {

std::vector<std::uint8_t> PathFilenameUtf8Bytes(
    const std::filesystem::path& path) {
  const auto utf8 = path.filename().u8string();
  std::vector<std::uint8_t> bytes;
  bytes.reserve(utf8.size());
  for (auto ch : utf8) {
    bytes.push_back(static_cast<std::uint8_t>(ch));
  }
  return bytes;
}

std::string DeriveAssemblyName(const std::filesystem::path& project_dir) {
  std::vector<cursive::core::UnicodeScalar> out;
  const cursive::core::DecodeResult decoded =
      cursive::core::Decode(PathFilenameUtf8Bytes(project_dir));
  bool pending_separator = false;

  auto append_separator = [&]() {
    if (pending_separator) {
      return;
    }
    out.push_back('_');
    pending_separator = true;
  };

  if (decoded.ok) {
    out.reserve(decoded.scalars.size() + 1);
    for (cursive::core::UnicodeScalar scalar : decoded.scalars) {
      if (out.empty()) {
        if (cursive::core::IsIdentStart(scalar)) {
          out.push_back(scalar);
          pending_separator = false;
          continue;
        }
        if (cursive::core::IsIdentContinue(scalar)) {
          out.push_back('_');
          out.push_back(scalar);
          pending_separator = false;
          continue;
        }
        append_separator();
        continue;
      }

      if (cursive::core::IsIdentContinue(scalar)) {
        out.push_back(scalar);
        pending_separator = false;
      } else {
        append_separator();
      }
    }
  }

  std::string name = cursive::core::EncodeUtf8(out);
  if (name.empty()) {
    name = "my_project";
  }
  if (cursive::core::IsKeyword(name)) {
    name.push_back('_');
  }
  if (!cursive::core::IsName(name)) {
    name = "my_project";
  }
  return name;
}

bool HasBlockingErrorsForSema(const cursive::core::DiagnosticStream& diags) {
  for (const auto& diag : diags) {
    if (diag.severity != cursive::core::Severity::Error) {
      continue;
    }
    return true;
  }
  return false;
}

std::size_t CountErrorDiagnostics(const cursive::core::DiagnosticStream& diags) {
  return cursive::analysis::CountErrorLikeDiagnostics(diags);
}

void EmitInternalDiagnostic(cursive::core::DiagnosticStream& diags,
                            cursive::core::Severity severity,
                            const std::optional<cursive::core::Span>& span,
                            const std::string& message) {
  cursive::core::Diagnostic diag;
  diag.severity = severity;
  diag.span = span;
  diag.message = message;
  cursive::core::Emit(diags, diag);
}

void TruncateDiagnosticsToErrorCap(
    cursive::core::DiagnosticStream& diags,
    const cursive::core::ErrorRecoveryPolicy& policy) {
  if (!policy.max_error_count.has_value()) {
    return;
  }

  const std::size_t cap = *policy.max_error_count;
  std::size_t error_count = 0;

  cursive::core::DiagnosticStream truncated;
  truncated.reserve(diags.size());

  for (const auto& diag : diags) {
    const bool is_error_like =
        diag.severity == cursive::core::Severity::Error;
    if (is_error_like) {
      if (error_count >= cap) {
        break;
      }
      ++error_count;
      truncated.push_back(diag);
      if (error_count >= cap) {
        break;
      }
      continue;
    }
    truncated.push_back(diag);
  }

  diags = std::move(truncated);
}

void EmitAuthorityValidationDiagnostic(
    cursive::core::DiagnosticStream& diags,
    const cursive::analysis::AuthorityValidationResult& err) {
  const std::string code = err.error_code.empty() ? "E-CON-0020"
                                                   : err.error_code;
  const std::optional<cursive::core::Span> span =
      err.span.file.empty() ? std::nullopt
                            : std::optional<cursive::core::Span>(err.span);

  if (auto diag = cursive::core::MakeDiagnosticById(code, span)) {
    if (!err.error_message.empty()) {
      diag->message = err.error_message;
    }
    cursive::core::Emit(diags, *diag);
    return;
  }
  EmitInternalDiagnostic(diags, cursive::core::Severity::Error, span,
                         err.error_message.empty()
                             ? "Authority validation failed."
                             : err.error_message);
}

void EmitCapabilityChainErrorDiagnostic(
    cursive::core::DiagnosticStream& diags,
    const cursive::analysis::CapabilityChainError& err) {
  const std::string code = err.code.empty() ? "E-CON-0020" : err.code;

  if (auto diag = cursive::core::MakeDiagnosticById(code, err.span)) {
    if (!err.message.empty()) {
      diag->message = err.message;
    }
    cursive::core::Emit(diags, *diag);
    return;
  }
  EmitInternalDiagnostic(diags, cursive::core::Severity::Error, err.span,
                         err.message.empty()
                             ? "Capability chain validation failed."
                             : err.message);
}

void EmitCapabilityLeakDiagnostic(
    cursive::core::DiagnosticStream& diags,
    const cursive::analysis::CapabilityLeak& leak) {
  const std::string code = leak.code.empty() ? "E-TYP-2623" : leak.code;
  const std::optional<cursive::core::Span> span =
      leak.leak_span.file.empty() ? std::nullopt
                                  : std::optional<cursive::core::Span>(
                                        leak.leak_span);

  if (auto diag = cursive::core::MakeDiagnosticById(code, span)) {
    if (!leak.message.empty()) {
      diag->message = leak.message;
    }
    cursive::core::Emit(diags, *diag);
    return;
  }
  EmitInternalDiagnostic(diags, cursive::core::Severity::Error, span,
                         leak.message.empty()
                             ? "Capability leaked to extern procedure."
                             : leak.message);
}

void EmitAttributeValidationDiagnostic(
    cursive::core::DiagnosticStream& diags,
    const cursive::analysis::AttributeValidationResult& err) {
  const std::string code =
      err.diag_id.has_value() ? std::string(*err.diag_id) : "E-MOD-2450";

  if (auto diag = cursive::core::MakeDiagnosticById(code, err.span)) {
    if (!err.message.empty()) {
      diag->message = err.message;
    }
    cursive::core::Emit(diags, *diag);
    return;
  }
  EmitInternalDiagnostic(diags, cursive::core::Severity::Error, err.span,
                         err.message.empty()
                             ? "Attribute validation failed."
                             : err.message);
}

bool ValidateParsedTypeAttributeLists(
    const std::vector<cursive::ast::ASTModule>& modules,
    cursive::core::DiagnosticStream& diags) {
  auto validate = [&](const auto& attrs, cursive::analysis::AttributeTarget target)
      -> bool {
    if (!cursive::analysis::HasAttribute(attrs,
                                         cursive::analysis::attrs::kDerive)) {
      return true;
    }
    const auto result = cursive::analysis::ValidateAttributes(attrs, target);
    if (result.ok) {
      return true;
    }
    EmitAttributeValidationDiagnostic(diags, result);
    return false;
  };

  for (const auto& module : modules) {
    for (const auto& item : module.items) {
      if (const auto* record = std::get_if<cursive::ast::RecordDecl>(&item)) {
        if (!validate(record->attrs, cursive::analysis::AttributeTarget::Record)) {
          return false;
        }
        continue;
      }
      if (const auto* enum_decl = std::get_if<cursive::ast::EnumDecl>(&item)) {
        if (!validate(enum_decl->attrs, cursive::analysis::AttributeTarget::Enum)) {
          return false;
        }
        continue;
      }
      if (const auto* modal = std::get_if<cursive::ast::ModalDecl>(&item)) {
        if (!validate(modal->attrs, cursive::analysis::AttributeTarget::Modal)) {
          return false;
        }
      }
    }
  }

  return true;
}

bool BuildProgressEnabled() {
  // Priority: CLI --build-progress > manifest [build] progress > default(true)
  const std::optional<bool> override = cursive::core::BuildProgressOverride();
  if (override.has_value()) {
    return *override;
  }
  const std::optional<bool> manifest = cursive::core::ManifestBuildProgress();
  if (manifest.has_value()) {
    return *manifest;
  }
  return true;
}

unsigned long CurrentProcessId() {
  return cursive::core::CurrentHostProcessId();
}

std::filesystem::path g_compiler_executable_path;

std::filesystem::path ResolveCurrentExecutablePath(const char* argv0) {
  const auto current = cursive::core::CurrentExecutablePath();
  if (!current.empty()) {
    return current;
  }

  if (argv0 && argv0[0] != '\0') {
    const std::filesystem::path raw(argv0);
    std::error_code ec;
    if (raw.is_absolute()) {
      return raw;
    }
    const auto abs = std::filesystem::absolute(raw, ec);
    if (!ec) {
      return abs;
    }
    return raw;
  }

  return {};
}

bool IncrementalEnabled() {
  // Priority: CLI --incremental > manifest [build] incremental > default(true)
  const std::optional<bool> override = cursive::core::IncrementalOverride();
  if (override.has_value()) {
    return *override;
  }
  const std::optional<bool> manifest = cursive::core::ManifestIncremental();
  if (manifest.has_value()) {
    return *manifest;
  }
  return true;
}

void MixHashByte(std::uint64_t& hash, std::uint8_t byte) {
  hash ^= static_cast<std::uint64_t>(byte);
  hash *= cursive::core::kFNVPrime64;
}

void MixHashString(std::uint64_t& hash, std::string_view value) {
  for (const unsigned char ch : value) {
    MixHashByte(hash, static_cast<std::uint8_t>(ch));
  }
  MixHashByte(hash, 0xFFU);
}

std::optional<std::string> ComputeFileSourceHash(
    const std::filesystem::path& file,
    cursive::core::DiagnosticStream& diags) {
  const auto bytes = cursive::frontend::ReadBytesDefault(file);
  for (const auto& diag : bytes.diags) {
    cursive::core::Emit(diags, diag);
  }
  if (!bytes.bytes.has_value()) {
    return std::nullopt;
  }

  std::uint64_t hash = cursive::core::kFNVOffset64;
  for (const auto byte : *bytes.bytes) {
    MixHashByte(hash, byte);
  }
  return cursive::core::Hex64(hash);
}

std::optional<std::string> HashFileBytes(const std::filesystem::path& file) {
  std::ifstream in(file, std::ios::binary);
  if (!in) {
    return std::nullopt;
  }
  std::string buffer((std::istreambuf_iterator<char>(in)),
                     std::istreambuf_iterator<char>());
  if (!in && !in.eof()) {
    return std::nullopt;
  }
  return cursive::core::Hex64(cursive::core::FNV1a64(buffer));
}

std::string LinkInputFingerprintField(const std::filesystem::path& path) {
  const std::string normalized = cursive::core::Normalize(path.generic_string());
  const auto hash = HashFileBytes(path);
  if (!hash.has_value()) {
    return normalized + ":missing";
  }
  return normalized + ":" + *hash;
}

bool IsExternalDependencyMarker(std::string_view dep) {
  constexpr std::string_view marker = "__external__:";
  return dep.size() >= marker.size() &&
         dep.compare(0, marker.size(), marker) == 0;
}

std::string CompilerFingerprintField() {
  if (g_compiler_executable_path.empty()) {
    return "compiler=<unknown>";
  }
  std::error_code ec;
  std::filesystem::path normalized =
      std::filesystem::weakly_canonical(g_compiler_executable_path, ec);
  if (ec) {
    ec.clear();
    normalized = std::filesystem::absolute(g_compiler_executable_path, ec);
    if (ec) {
      normalized = g_compiler_executable_path;
    }
  }
  return "compiler=" + LinkInputFingerprintField(normalized);
}

std::string HashFields(const std::vector<std::string>& fields) {
  std::uint64_t hash = cursive::core::kFNVOffset64;
  for (const auto& field : fields) {
    MixHashString(hash, field);
  }
  return cursive::core::Hex64(hash);
}

std::optional<std::string> ComputeModuleSourceHash(
    const cursive::project::ModuleInfo& module,
    cursive::core::DiagnosticStream& diags) {
  const auto unit = cursive::project::CompilationUnit(module.dir);
  for (const auto& diag : unit.diags) {
    cursive::core::Emit(diags, diag);
  }
  if (cursive::core::HasError(unit.diags)) {
    return std::nullopt;
  }

  std::uint64_t hash = cursive::core::kFNVOffset64;
  MixHashString(hash, module.path);

  for (const auto& file : unit.files) {
    MixHashString(hash, cursive::core::Normalize(file.generic_string()));
    const auto file_hash = ComputeFileSourceHash(file, diags);
    if (!file_hash.has_value()) {
      return std::nullopt;
    }
    MixHashString(hash, *file_hash);
    MixHashByte(hash, 0x00U);
  }

  return cursive::core::Hex64(hash);
}

std::optional<std::string> ResolveImportedModule(
    const cursive::ast::ImportDecl& import,
    const std::unordered_set<std::string>& known_modules) {
  if (import.path.empty()) {
    return std::nullopt;
  }

  std::string candidate;
  std::optional<std::string> best;
  for (std::size_t i = 0; i < import.path.size(); ++i) {
    if (i > 0) {
      candidate.append("::");
    }
    candidate.append(import.path[i]);
    if (known_modules.find(candidate) != known_modules.end()) {
      best = candidate;
    }
  }
  return best;
}

std::unordered_map<std::string, std::vector<std::string>> BuildModuleDeps(
    const std::vector<cursive::ast::ASTModule>& modules,
    const std::unordered_set<std::string>& known_modules) {
  std::unordered_map<std::string, std::vector<std::string>> deps_by_module;
  deps_by_module.reserve(modules.size());

  for (const auto& module : modules) {
    const std::string module_path = cursive::core::StringOfPath(module.path);
    if (known_modules.find(module_path) == known_modules.end()) {
      continue;
    }

    std::unordered_set<std::string> dep_set;
    for (const auto& item : module.items) {
      const auto* import = std::get_if<cursive::ast::ImportDecl>(&item);
      if (!import) {
        continue;
      }
      const auto target = ResolveImportedModule(*import, known_modules);
      if (!target.has_value()) {
        dep_set.insert("__external__:" + cursive::core::StringOfPath(import->path));
        continue;
      }
      if (*target == module_path) {
        continue;
      }
      dep_set.insert(*target);
    }

    std::vector<std::string> deps(dep_set.begin(), dep_set.end());
    std::sort(deps.begin(), deps.end());
    deps_by_module[module_path] = std::move(deps);
  }

  return deps_by_module;
}

cursive::source::ModuleNames ModuleNamesForAssemblies(
    const std::vector<cursive::project::Assembly>& assemblies) {
  cursive::source::ModuleNames names;
  for (const auto& assembly : assemblies) {
    for (const auto& module : assembly.modules) {
      names.insert(module.path);
    }
  }
  return names;
}

std::unordered_map<std::string, std::string> ModuleOwnerMapForAssemblies(
    const std::vector<cursive::project::Assembly>& assemblies) {
  std::unordered_map<std::string, std::string> owners;
  for (const auto& assembly : assemblies) {
    for (const auto& module : assembly.modules) {
      owners.emplace(module.path, assembly.name);
    }
  }
  return owners;
}

std::optional<std::string> ResolveImportedAssemblyName(
    const cursive::ast::ImportDecl& import,
    const cursive::ast::ModulePath& current_module,
    const cursive::source::ModuleNames& module_names,
    const std::unordered_map<std::string, std::string>& module_owner) {
  const auto resolved =
      cursive::source::ResolveImportModulePath(current_module,
                                              module_names,
                                              import.path);
  if (!resolved.has_value()) {
    return std::nullopt;
  }
  const auto owner_it = module_owner.find(cursive::core::StringOfPath(*resolved));
  if (owner_it == module_owner.end()) {
    return std::nullopt;
  }
  return owner_it->second;
}

std::optional<std::string> ResolveModuleAssemblyName(
    const cursive::ast::ModulePath& module_path,
    const std::unordered_map<std::string, std::string>& module_owner) {
  const auto owner_it =
      module_owner.find(cursive::core::StringOfPath(module_path));
  if (owner_it == module_owner.end()) {
    return std::nullopt;
  }
  return owner_it->second;
}

std::optional<cursive::ast::ModulePath> ResolveUsingModulePath(
    const cursive::ast::UsingClause& clause,
    const cursive::ast::ModulePath& current_module,
    const cursive::source::ModuleNames& module_names) {
  return std::visit(
      [&](const auto& node) -> std::optional<cursive::ast::ModulePath> {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, cursive::ast::UsingItem>) {
          return cursive::source::ResolveImportModulePath(current_module,
                                                          module_names,
                                                          node.module_path);
        } else if constexpr (std::is_same_v<T, cursive::ast::UsingWildcard>) {
          return cursive::source::ResolveImportModulePath(current_module,
                                                          module_names,
                                                          node.module_path);
        } else {
          return cursive::source::ResolveImportModulePath(current_module,
                                                          module_names,
                                                          node.module_path);
        }
      },
      clause);
}

std::optional<std::string> ResolveUsingAssemblyName(
    const cursive::ast::UsingDecl& using_decl,
    const cursive::ast::ModulePath& current_module,
    const cursive::source::ModuleNames& module_names,
    const std::unordered_map<std::string, std::string>& module_owner) {
  const auto resolved_module =
      ResolveUsingModulePath(using_decl.clause, current_module, module_names);
  if (!resolved_module.has_value()) {
    return std::nullopt;
  }
  return ResolveModuleAssemblyName(*resolved_module, module_owner);
}

std::filesystem::path RuntimeLogsDir(const cursive::project::Project& project) {
  // Keep runtime logs inside the active output root to avoid stray build dirs.
  return project.outputs.root / "logs" / "runtime";
}

std::filesystem::path ConformanceLogsDir(
    const std::filesystem::path& output_root) {
  return output_root / "logs" / "conformance";
}

std::filesystem::path FallbackConformanceLogsDir(
    const std::filesystem::path& project_root) {
  return project_root / "build" / "main" / "logs" / "conformance";
}

std::filesystem::path ResolveLogFileName(
    const std::optional<std::string>& requested_path,
    std::string_view fallback_name) {
  std::filesystem::path file_name;
  if (requested_path.has_value()) {
    file_name = std::filesystem::path(*requested_path).filename();
  }
  if (file_name.empty()) {
    file_name = std::filesystem::path(std::string(fallback_name));
  }
  return file_name;
}

std::string EffectiveRuntimeLogFilePath(
    const cursive::project::Project& project,
    const cursive::driver::CliOptions& opts) {
  if (!opts.log_to_file) {
    return {};
  }

  if (opts.log_file_path.has_value() && !opts.log_file_path->empty()) {
    return std::filesystem::path(*opts.log_file_path).generic_string();
  }

  const std::filesystem::path file_name =
      ResolveLogFileName(std::nullopt, project.assembly.name + ".runtime.log");

  return (RuntimeLogsDir(project) / file_name).generic_string();
}

std::string EffectiveConformancePath(
    const std::filesystem::path& conformance_logs_dir,
    const cursive::driver::CliOptions& opts) {
  const std::filesystem::path file_name =
      ResolveLogFileName(opts.conformance_path, "compile.conformance.log");
  return (conformance_logs_dir / file_name).generic_string();
}

std::vector<cursive::ast::ASTModule> FilterAstModulesForProject(
    const std::vector<cursive::ast::ASTModule>& modules,
    const cursive::project::Project& project) {
  std::unordered_set<std::string> wanted;
  wanted.reserve(project.modules.size());
  for (const auto& module : project.modules) {
    wanted.insert(module.path);
  }

  std::vector<cursive::ast::ASTModule> filtered;
  filtered.reserve(project.modules.size());
  for (const auto& module : modules) {
    const std::string module_path = cursive::core::StringOfPath(module.path);
    if (wanted.find(module_path) == wanted.end()) {
      continue;
    }
    filtered.push_back(module);
  }
  return filtered;
}


void EmitExternalCode(cursive::core::DiagnosticStream& diags,
                      std::string_view code,
                      const std::optional<std::string>& note = std::nullopt) {
  if (auto diag = cursive::core::MakeExternalDiagnostic(code)) {
    if (note.has_value() && !note->empty()) {
      cursive::core::SubDiagnostic child;
      child.kind = cursive::core::SubDiagnosticKind::Note;
      child.message = *note;
      diag->children.push_back(std::move(child));
    }
    cursive::core::Emit(diags, *diag);
    return;
  }
  cursive::core::EmitExternalDiagnostic(diags, code);
}

void EnsureCompilerLogDirectory(const std::filesystem::path& logs_root) {
  std::error_code ec;
  std::filesystem::create_directories(logs_root, ec);
}

void EnsureRuntimeLogDirectory(const cursive::project::Project& project,
                               const cursive::driver::CliOptions& opts) {
  if (!opts.log_to_file) {
    return;
  }
  std::error_code ec;
  std::filesystem::path logs_root = RuntimeLogsDir(project);
  if (opts.log_file_path.has_value() && !opts.log_file_path->empty()) {
    logs_root = std::filesystem::path(*opts.log_file_path).parent_path();
  }
  if (!logs_root.empty()) {
    std::filesystem::create_directories(logs_root, ec);
  }
}

std::optional<cursive::project::TargetProfile> ResolveSelectedTargetProfile(
    const cursive::driver::CliOptions& opts,
    const cursive::project::Project& project,
    cursive::core::DiagnosticStream& diags) {
  if (opts.target_profile_override.has_value()) {
    return *opts.target_profile_override;
  }
  if (project.toolchain.target_profile.has_value()) {
    return *project.toolchain.target_profile;
  }
  cursive::core::EmitExternalDiagnostic(diags, "E-PRJ-0112");
  return std::nullopt;
}

std::string BuildIncrementalBuildKey(const cursive::project::Project& project,
                                     cursive::project::TargetProfile target_profile,
                                     const cursive::driver::CliOptions& opts) {
  std::vector<std::string> fields;
  fields.reserve(18);
  fields.push_back("v3");
  fields.push_back(cursive::driver::GetVersionString());
  fields.push_back(CompilerFingerprintField());
  fields.push_back(project.assembly.name);
  fields.push_back(project.assembly.kind);
  fields.push_back(project.assembly.link_kind.value_or("none"));
  fields.push_back(std::string(
      cursive::project::TargetProfileName(target_profile)));
  fields.push_back(project.root.generic_string());
  fields.push_back(project.source_root.generic_string());
  fields.push_back(project.outputs.root.generic_string());
  fields.push_back(project.assembly.emit_ir.value_or("none"));
  fields.push_back(std::string("log_enabled=") + (opts.log_enabled ? "1" : "0"));
  fields.push_back(std::string("log_to_console=") +
                   (opts.log_to_console ? "1" : "0"));
  fields.push_back(std::string("log_to_file=") + (opts.log_to_file ? "1" : "0"));
  fields.push_back(std::string("trace=") + (opts.trace ? "1" : "0"));
  fields.push_back("trace_filter_mask=" +
                   std::to_string(opts.trace_filter_mask.value_or(0u)));
  fields.push_back("trace_min_level=" +
                   std::to_string(opts.trace_min_level.value_or(0u)));
  fields.push_back("log_file=" + EffectiveRuntimeLogFilePath(project, opts));
  if (const auto runtime_lib =
          cursive::project::ResolveRuntimeLib(project, target_profile);
      runtime_lib.has_value()) {
    fields.push_back("runtime_lib=" + LinkInputFingerprintField(*runtime_lib));
  } else {
    fields.push_back("runtime_lib=<missing>");
  }
  return HashFields(fields);
}

struct IncrementalBuildDataResult {
  bool ok = false;
  std::string build_key;
  std::unordered_map<std::string, cursive::project::IncrementalModuleInfo>
      modules;
};

IncrementalBuildDataResult BuildIncrementalBuildData(
    const cursive::project::Project& project,
    cursive::project::TargetProfile target_profile,
    const std::vector<cursive::ast::ASTModule>& resolved_modules,
    const cursive::driver::CliOptions& opts,
    cursive::core::DiagnosticStream& diags) {
  IncrementalBuildDataResult result;

  std::unordered_set<std::string> module_set;
  module_set.reserve(std::max(project.modules.size(), resolved_modules.size()));
  for (const auto& module : resolved_modules) {
    module_set.insert(cursive::core::StringOfPath(module.path));
  }
  for (const auto& module : project.modules) {
    module_set.insert(module.path);
  }

  const auto deps_by_module = BuildModuleDeps(resolved_modules, module_set);

  std::unordered_map<std::string, std::string> source_hashes;
  source_hashes.reserve(project.modules.size());
  for (const auto& module : project.modules) {
    const auto source_hash = ComputeModuleSourceHash(module, diags);
    if (!source_hash.has_value()) {
      return result;
    }
    source_hashes[module.path] = *source_hash;
  }

  result.build_key = BuildIncrementalBuildKey(project, target_profile, opts);
  result.modules.reserve(project.modules.size());

  std::unordered_map<std::string, std::string> full_hash_cache;
  full_hash_cache.reserve(project.modules.size());
  std::unordered_set<std::string> full_hash_visiting;

  std::function<std::string(const std::string&)> compute_full_hash =
      [&](const std::string& module_path) -> std::string {
    const auto cached_it = full_hash_cache.find(module_path);
    if (cached_it != full_hash_cache.end()) {
      return cached_it->second;
    }

    const auto source_it = source_hashes.find(module_path);
    const std::string source_hash =
        source_it != source_hashes.end() ? source_it->second : "missing";

    if (full_hash_visiting.find(module_path) != full_hash_visiting.end()) {
      std::vector<std::string> cycle_fields;
      cycle_fields.reserve(5);
      cycle_fields.push_back("v2");
      cycle_fields.push_back(result.build_key);
      cycle_fields.push_back(module_path);
      cycle_fields.push_back("source=" + source_hash);
      cycle_fields.push_back("cycle=1");
      const std::string cycle_hash = HashFields(cycle_fields);
      full_hash_cache[module_path] = cycle_hash;
      return cycle_hash;
    }

    full_hash_visiting.insert(module_path);

    std::vector<std::string> fields;
    const auto dep_it = deps_by_module.find(module_path);
    const std::size_t dep_count =
        dep_it != deps_by_module.end() ? dep_it->second.size() : 0;
    fields.reserve(6 + dep_count);
    fields.push_back("v2");
    fields.push_back(result.build_key);
    fields.push_back(module_path);
    fields.push_back("source=" + source_hash);

    if (dep_it != deps_by_module.end()) {
      for (const auto& dep : dep_it->second) {
        const auto dep_source_it = source_hashes.find(dep);
        if (dep_source_it == source_hashes.end()) {
          fields.push_back("dep=" + dep + ":missing");
          continue;
        }
        fields.push_back("dep=" + dep + ":" + compute_full_hash(dep));
      }
    }

    const std::string out = HashFields(fields);
    full_hash_visiting.erase(module_path);
    full_hash_cache[module_path] = out;
    return out;
  };

  for (const auto& module : project.modules) {
    cursive::project::IncrementalModuleInfo info;
    info.source_hash = source_hashes[module.path];
    info.public_hash = info.source_hash;

    const auto dep_it = deps_by_module.find(module.path);
    if (dep_it != deps_by_module.end()) {
      info.dependencies = dep_it->second;
    }

    info.full_hash = compute_full_hash(module.path);

    result.modules[module.path] = std::move(info);
  }

  result.ok = true;
  return result;
}

struct IncrementalManifestModuleState {
  cursive::project::IncrementalModuleInfo info;
  std::string obj_hash;
  std::string ir_hash;
};

struct IncrementalManifestState {
  std::string format = "1";
  std::string assembly;
  std::string build_key;
  std::string emit_ir;
  std::string kind;
  std::string link_fingerprint;
  std::unordered_map<std::string, IncrementalManifestModuleState> modules;
};

std::vector<std::string> SplitByChar(std::string_view text, char sep) {
  std::vector<std::string> out;
  std::size_t start = 0;
  while (start <= text.size()) {
    const std::size_t pos = text.find(sep, start);
    if (pos == std::string_view::npos) {
      out.emplace_back(text.substr(start));
      break;
    }
    out.emplace_back(text.substr(start, pos - start));
    start = pos + 1;
  }
  return out;
}

std::filesystem::path IncrementalManifestPath(
    const cursive::project::Project& project) {
  return project.outputs.root / ".cursive-incremental" /
         (project.assembly.name + ".manifest");
}

std::optional<IncrementalManifestState> LoadIncrementalManifest(
    const std::filesystem::path& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    return std::nullopt;
  }

  IncrementalManifestState state;
  std::string line;
  while (std::getline(in, line)) {
    if (line.empty()) {
      continue;
    }
    const auto fields = SplitByChar(line, '\t');
    if (fields.empty()) {
      continue;
    }
    if (fields[0] == "H" && fields.size() >= 3) {
      if (fields[1] == "format") {
        state.format = fields[2];
      } else if (fields[1] == "assembly") {
        state.assembly = fields[2];
      } else if (fields[1] == "build_key") {
        state.build_key = fields[2];
      } else if (fields[1] == "emit_ir") {
        state.emit_ir = fields[2];
      } else if (fields[1] == "kind") {
        state.kind = fields[2];
      } else if (fields[1] == "link") {
        state.link_fingerprint = fields[2];
      }
      continue;
    }
    if (fields[0] != "M" || fields.size() < 8) {
      continue;
    }

    IncrementalManifestModuleState module_state;
    module_state.info.source_hash = fields[2];
    module_state.info.public_hash = fields[3];
    module_state.info.full_hash = fields[4];
    module_state.obj_hash = fields[5];
    module_state.ir_hash = fields[6];
    if (!fields[7].empty()) {
      module_state.info.dependencies = SplitByChar(fields[7], ',');
    }
    state.modules[fields[1]] = std::move(module_state);
  }

  if (!in && !in.eof()) {
    return std::nullopt;
  }
  return state;
}

std::string EffectiveEmitIR(const cursive::project::Project& project) {
  if (project.assembly.emit_ir.has_value()) {
    return *project.assembly.emit_ir;
  }
  return project.assembly.kind == "executable" ? "none" : "ll";
}

std::string ComputeLinkFingerprint(
    const cursive::project::Project& project,
    cursive::project::TargetProfile target_profile,
    const std::string& build_key,
    const std::unordered_map<std::string, IncrementalManifestModuleState>& modules,
    const std::vector<std::filesystem::path>& link_inputs,
    const std::optional<std::filesystem::path>& runtime_lib,
    std::string_view emit_ir) {
  std::vector<std::string> fields;
  fields.reserve(8 + project.modules.size() + link_inputs.size());
  fields.push_back("v5");
  fields.push_back(build_key);
  fields.push_back(project.assembly.name);
  fields.push_back(project.assembly.kind);
  fields.push_back(project.assembly.link_kind.value_or("none"));
  fields.push_back(
      std::string(cursive::project::TargetProfileName(target_profile)));
  fields.push_back(std::string(emit_ir));
  if (runtime_lib.has_value()) {
    fields.push_back("runtime=" + LinkInputFingerprintField(*runtime_lib));
  }
  if (const auto map_path = cursive::project::MapPath(project, target_profile);
      map_path.has_value()) {
    fields.push_back("map=windows-sidecar");
    fields.push_back("map_path=" +
                     cursive::core::Normalize(map_path->generic_string()));
  }
  for (const auto& input : link_inputs) {
    fields.push_back("input=" + LinkInputFingerprintField(input));
  }

  for (const auto& module : project.modules) {
    const auto it = modules.find(module.path);
    if (it == modules.end()) {
      fields.push_back(module.path + ":missing");
      continue;
    }
    const auto& mod = it->second;
    fields.push_back(module.path + ":" + mod.info.full_hash + ":" +
                     mod.obj_hash + ":" + mod.ir_hash);
  }

  return HashFields(fields);
}

struct IncrementalNoopCheckResult {
  bool reusable = false;
  std::size_t modules = 0;
  std::string reason;
};

IncrementalNoopCheckResult CheckIncrementalNoopReuse(
    const cursive::project::Project& project,
    cursive::project::TargetProfile target_profile,
    const std::vector<cursive::ast::ASTModule>& parsed_modules,
    const cursive::driver::CliOptions& opts,
    cursive::core::DiagnosticStream& diags) {
  IncrementalNoopCheckResult result;

  if (!IncrementalEnabled()) {
    result.reason = "disabled";
    return result;
  }
  if (project.assembly.kind != "executable") {
    result.reason = "non-executable";
    return result;
  }
  if (project.modules.empty()) {
    result.reason = "empty-module-set";
    return result;
  }
  if (project.modules.size() != project.assembly.modules.size()) {
    result.reason = "multi-assembly-graph";
    return result;
  }

  const auto manifest_path = IncrementalManifestPath(project);
  const auto manifest = LoadIncrementalManifest(manifest_path);
  if (!manifest.has_value()) {
    result.reason = "manifest-missing";
    return result;
  }

  const std::string build_key =
      BuildIncrementalBuildKey(project, target_profile, opts);
  const std::string emit_ir = EffectiveEmitIR(project);
  const bool compatible = manifest->format == "1" &&
                          manifest->assembly == project.assembly.name &&
                          manifest->kind == project.assembly.kind &&
                          manifest->emit_ir == emit_ir &&
                          manifest->build_key == build_key;
  if (!compatible) {
    result.reason = "manifest-incompatible";
    return result;
  }

  if (manifest->modules.size() != project.modules.size()) {
    result.reason = "module-count-mismatch";
    return result;
  }

  std::error_code exe_ec;
  const auto exe_path = cursive::project::ExePath(project, target_profile);
  if (!std::filesystem::exists(exe_path, exe_ec) || exe_ec) {
    result.reason = "exe-missing";
    return result;
  }
  if (const auto map_path = cursive::project::MapPath(project, target_profile);
      map_path.has_value()) {
    std::error_code map_ec;
    if (!std::filesystem::exists(*map_path, map_ec) || map_ec) {
      result.reason = "map-missing";
      return result;
    }
  }

  auto current_incremental =
      BuildIncrementalBuildData(project, target_profile, parsed_modules, opts,
                                diags);
  if (!current_incremental.ok) {
    result.reason = "fingerprint-failed";
    return result;
  }
  if (current_incremental.modules.size() != project.modules.size()) {
    result.reason = "fingerprint-module-count-mismatch";
    return result;
  }

  for (const auto& [module_path, info] : current_incremental.modules) {
    for (const auto& dep : info.dependencies) {
      if (IsExternalDependencyMarker(dep)) {
        continue;
      }
      if (current_incremental.modules.find(dep) ==
          current_incremental.modules.end()) {
        result.reason = "external-dependency:" + dep;
        return result;
      }
    }
  }

  for (const auto& module : project.modules) {
    const auto curr_it = current_incremental.modules.find(module.path);
    if (curr_it == current_incremental.modules.end()) {
      result.reason = "fingerprint-module-missing";
      return result;
    }
    const auto prev_it = manifest->modules.find(module.path);
    if (prev_it == manifest->modules.end()) {
      result.reason = "manifest-module-missing";
      return result;
    }
    if (prev_it->second.info.full_hash != curr_it->second.full_hash) {
      result.reason = "module-changed:" + module.path;
      return result;
    }
  }

  if (manifest->link_fingerprint.empty()) {
    result.reason = "link-fingerprint-missing";
    return result;
  }

  const auto extern_libraries =
      cursive::project::CollectExternLibrarySpecs(parsed_modules);
  const auto link_inputs =
      cursive::project::ResolveExternLibraryInputs(extern_libraries,
                                                   target_profile);
  const auto runtime_lib =
      cursive::project::ResolveRuntimeLib(project, target_profile);
  const std::string link_fingerprint =
      ComputeLinkFingerprint(project, target_profile, build_key,
                             manifest->modules, link_inputs,
                             runtime_lib, emit_ir);
  if (link_fingerprint != manifest->link_fingerprint) {
    result.reason = "link-fingerprint-mismatch";
    return result;
  }

  result.reusable = true;
  result.modules = project.modules.size();
  result.reason = "hit";
  return result;
}

}  // namespace

// ============================================================================
// Main Entry Point
// ============================================================================

int main(int argc, char** argv) {
  using namespace cursive;
  using namespace cursive::driver;

  g_compiler_executable_path =
      ResolveCurrentExecutablePath((argc > 0) ? argv[0] : nullptr);

  {
    core::CrashRuntimeOptions crash_options;
    crash_options.tool_name = "Cursive";
    crash_options.tool_version = GetVersionString();
    crash_options.executable_path = g_compiler_executable_path;
    crash_options.arguments.reserve(argc > 1 ? static_cast<std::size_t>(argc - 1)
                                             : 0u);
    for (int i = 1; i < argc; ++i) {
      crash_options.arguments.push_back(argv[i]);
    }
    std::error_code ec;
    crash_options.working_directory =
        std::filesystem::current_path(ec).generic_string();
    core::ConfigureCrashRuntime(crash_options);
    core::InstallCrashHandlers();
  }

  {
    std::string bundle_error;
    if (!core::EnsureBundledHostCompilerSupport(&bundle_error)) {
      if (bundle_error.empty()) {
        bundle_error = "Failed to initialize compiler sidecar support.";
      }
      std::cerr << "error: " << bundle_error << "\n";
      return 1;
    }
  }

  SpecDefsDriver();

  const auto parse_result = ParseArgs(argc, argv);
  if (!parse_result.options.has_value()) {
    if (!parse_result.error_message.empty()) {
      std::cerr << "error: " << parse_result.error_message << "\n";
    }
    PrintUsage();
    return 2;
  }
  const auto& opts = parse_result.options;
  core::SetCrashEnabled(!opts->no_crash_report);
  core::SetCrashJsonStdout(opts->diag_json);
  core::MaybeTriggerCrashFixtureFromEnv();

  core::SetBuildProgressOverride(opts->build_progress);
  core::SetIncrementalOverride(opts->incremental);
  core::SetRuntimeLibOverride(opts->runtime_lib_path);
  core::SetLinkDebugOverride(opts->link_debug);
  core::SetMaxErrorsOverride(opts->max_errors_override);
  core::SetOutDirOverride(opts->out_dir);
  if (!opts->debug_subsystems.empty()) {
    core::SetDebugSubsystems(opts->debug_subsystems);
  }
  const auto effective_error_policy =
      core::MaxErrorsOverride().value_or(core::DefaultErrorRecoveryPolicy());

  // Map CLI Verbosity to core Verbosity and store in process config.
  {
    core::Verbosity core_verbosity = core::Verbosity::Normal;
    switch (opts->verbosity) {
      case Verbosity::Quiet: core_verbosity = core::Verbosity::Quiet; break;
      case Verbosity::Normal: core_verbosity = core::Verbosity::Normal; break;
      case Verbosity::Verbose: core_verbosity = core::Verbosity::Verbose; break;
    }
    core::SetVerbosity(core_verbosity);
  }

  // Compute color override early so init/clean subcommands can use it.
  const auto color_override = [&]() -> core::ColorOverride {
    switch (opts->color_mode) {
      case ColorMode::Always: return core::ColorOverride::ForceOn;
      case ColorMode::Never: return core::ColorOverride::ForceOff;
      case ColorMode::Auto: return core::ColorOverride::Auto;
    }
    return core::ColorOverride::Auto;
  }();

  if (opts->show_help) {
    PrintHelp();
    return 0;
  }
  if (opts->show_debug_help) {
    PrintDebugHelp();
    return 0;
  }
  if (opts->show_version) {
    PrintVersion();
    return 0;
  }

  // ========================================================================
  // init subcommand
  // ========================================================================
  if (opts->do_init) {
    namespace fs = std::filesystem;
    const fs::path project_dir = fs::absolute(fs::path(opts->input_path));
    const auto toml_path = project_dir / "Cursive.toml";

    std::error_code ec;
    if (fs::exists(toml_path, ec) && !ec) {
      std::cerr << "error: Cursive.toml already exists in "
                << project_dir.string() << "\n";
      return 1;
    }

    // Derive project name from directory name
    std::string project_label = project_dir.filename().string();
    if (project_label.empty() || project_label == "." || project_label == "..") {
      project_label = "my_project";
    }
    const std::string project_name = DeriveAssemblyName(project_dir);

    // Create project directory if it doesn't exist
    if (!fs::exists(project_dir, ec)) {
      fs::create_directories(project_dir, ec);
      if (ec) {
        std::cerr << "error: could not create directory: "
                  << project_dir.string() << "\n";
        return 1;
      }
    }

    // Create Cursive.toml
    {
      std::ofstream out(toml_path, std::ios::binary);
      if (!out) {
        std::cerr << "error: could not create Cursive.toml\n";
        return 1;
      }
      out << "[assembly]\n"
          << "name = \"" << project_name << "\"\n"
          << "kind = \"executable\"\n"
          << "root = \"src\"\n";
    }

    // Create src/ directory
    const auto src_dir = project_dir / "src";
    fs::create_directories(src_dir, ec);
    if (ec) {
      std::cerr << "error: could not create src directory\n";
      return 1;
    }

    // Create src/main.cursive
    {
      std::ofstream out(src_dir / "main.cursive", std::ios::binary);
      if (!out) {
        std::cerr << "error: could not create src/main.cursive\n";
        return 1;
      }
      out << "public procedure main(move ctx: Context) -> i32 {\n"
          << "    return 0\n"
          << "}\n";
    }

    const bool init_color = core::IsColorEnabledWithOverride(stderr, color_override);
    constexpr std::size_t kLabelWidth = 12;
    const std::string_view label = "Created";
    const std::size_t pad = kLabelWidth - label.size();
    std::cerr << std::string(pad, ' ')
              << core::Colorize(label, core::Color::BoldGreen, init_color)
              << "  " << project_label;
    if (project_name != project_label) {
      std::cerr << " (cursive project, assembly " << project_name << ")\n";
    } else {
      std::cerr << " (cursive project)\n";
    }
    return 0;
  }

  // ========================================================================
  // clean subcommand
  // ========================================================================
  if (opts->do_clean) {
    namespace fs = std::filesystem;
    const fs::path input_path_fs = opts->input_path;
    const auto project_root = project::FindProjectRoot(input_path_fs);
    const auto assembly_target =
        project::ParseAssemblyTarget(opts->assembly_target);
    if (!assembly_target.has_value()) {
      core::DiagnosticStream target_diags;
      core::EmitExternalDiagnostic(target_diags, "E-PRJ-0205");
      for (const auto& diag : target_diags) {
        std::cerr << core::Render(diag) << "\n";
      }
      return 1;
    }
    const auto project_result =
        project::LoadProject(project_root, *assembly_target);

    if (!project_result.project.has_value()) {
      for (const auto& diag : project_result.diags) {
        std::cerr << core::Render(diag) << "\n";
      }
      return 1;
    }

    const auto& proj = *project_result.project;
    const auto& out_root = proj.outputs.root;
    const std::string project_name = proj.assembly.name;
    const bool clean_color = core::IsColorEnabledWithOverride(stderr, color_override);
    constexpr std::size_t kLabelWidth = 12;
    const std::string_view label = "Cleaned";
    const std::size_t pad = kLabelWidth - label.size();

    std::error_code ec;
    if (fs::exists(out_root, ec) && !ec) {
      fs::remove_all(out_root, ec);
      if (ec) {
        std::cerr << "error: could not remove " << out_root.string() << ": "
                  << ec.message() << "\n";
        return 1;
      }
      std::cerr << std::string(pad, ' ')
                << core::Colorize(label, core::Color::BoldGreen, clean_color)
                << "  " << project_name << " (removed "
                << out_root.string() << ")\n";
    } else {
      std::cerr << std::string(pad, ' ')
                << core::Colorize(label, core::Color::BoldGreen, clean_color)
                << "  " << project_name << " (nothing to clean)\n";
    }
    return 0;
  }

  const std::filesystem::path input_path = opts->input_path;
  const auto project_root = project::FindProjectRoot(input_path);

  core::DiagnosticStream diags;
  const bool is_quiet = opts->verbosity == Verbosity::Quiet;
  const bool is_verbose = opts->verbosity == Verbosity::Verbose;
  const bool show_build_progress =
      is_quiet ? false : (is_verbose ? true : BuildProgressEnabled());
  const bool use_color = core::IsColorEnabledWithOverride(stderr, color_override);
  const auto build_start = std::chrono::steady_clock::now();

  // Human-friendly progress: right-aligned colored label + detail.
  const auto progress = [&](const char* label, const std::string& detail,
                            core::Color color = core::Color::BoldGreen) {
    if (!show_build_progress) return;
    constexpr std::size_t kLabelWidth = 12;
    const std::size_t label_len = std::strlen(label);
    const std::size_t pad =
        (kLabelWidth > label_len) ? (kLabelWidth - label_len) : 0;
    std::cerr << std::string(pad, ' ')
              << core::Colorize(label, color, use_color) << "  " << detail
              << "\n";
    std::cerr.flush();
  };

  // Machine-format logging preserved under --debug pipeline.
  const bool debug_pipeline = core::IsDebugEnabled("pipeline");
  core::BuildLogResolveOptions phase_log_options;
  phase_log_options.channel_enabled = show_build_progress;
  phase_log_options.debug_enabled = debug_pipeline;
  phase_log_options.default_enabled = true;
  const core::BuildLogMode phase_log_mode =
      core::ResolveBuildLogMode(phase_log_options);
  const auto log_machine = [&](const std::string& message) {
    if (debug_pipeline) {
      std::cerr << "[trace][build] pid=" << CurrentProcessId() << " "
                << message << "\n";
      std::cerr.flush();
      return;
    }
    if (phase_log_mode == core::BuildLogMode::None) return;
    if (phase_log_mode == core::BuildLogMode::Summary &&
        !core::ShouldEmitSummaryBuildLog(core::BuildLogChannel::Phase,
                                         message)) {
      return;
    }
    std::cerr << "[info][build] " << message << "\n";
    std::cerr.flush();
  };

  log_machine("event=start input=" + opts->input_path +
              (opts->assembly_target.has_value()
                   ? " assembly=" + *opts->assembly_target
                   : ""));

  bool phase1_ok = false;
  bool phase2_ok = false;
  bool resolve_ok = false;
  bool typecheck_ok = false;
  bool phase4_ok = false;
  bool incremental_noop_reused = false;

  // Per-phase timing (for --verbose)
  long long parse_ms = 0;
  long long check_ms = 0;
  long long codegen_ms = 0;
  long long link_ms = 0;

  log_machine("phase=project-load");
  const auto assembly_target =
      project::ParseAssemblyTarget(opts->assembly_target);
  if (!assembly_target.has_value()) {
    core::EmitExternalDiagnostic(diags, "E-PRJ-0205");
    for (const auto& diag : diags) {
      std::cerr << core::Render(diag) << "\n";
    }
    return 1;
  }
  const auto project_result =
      project::LoadProject(project_root, *assembly_target);
  if (project_result.project.has_value()) {
    core::UpdateCrashReportRoot(
        core::DefaultCrashReportRoot(project_result.project->outputs.root));
  }

  if (opts->conformance_path.has_value()) {
    std::filesystem::path conformance_logs_dir =
        FallbackConformanceLogsDir(project_root);
    if (project_result.project.has_value()) {
      conformance_logs_dir =
          ConformanceLogsDir(project_result.project->outputs.root);
    }
    EnsureCompilerLogDirectory(conformance_logs_dir);
    core::Conformance::Init(
        EffectiveConformancePath(conformance_logs_dir, *opts), "compile");
    core::Conformance::SetRoot(project_root.string());
    core::Conformance::SetPhase("project-load");
  }

  for (const auto& diag : project_result.diags) {
    core::Emit(diags, diag);
  }

  std::optional<project::TargetProfile> selected_target_profile;
  if (!core::HasError(diags) && project_result.project.has_value()) {
    selected_target_profile = ResolveSelectedTargetProfile(
        *opts, *project_result.project, diags);
  }

  if (!core::HasError(diags) && project_result.project.has_value() &&
      selected_target_profile.has_value()) {
    const auto& proj = *project_result.project;
    const auto target_profile = *selected_target_profile;
    progress("Loading",
             project_root.filename().string() + " (" +
                 std::to_string(proj.modules.size()) + " modules)");
    if (is_verbose) {
      for (const auto& module : proj.modules) {
        std::cerr << "       module: " << module.path << "\n";
      }
      std::cerr.flush();
    }
    EnsureRuntimeLogDirectory(proj, *opts);

    frontend::ParseModuleDeps deps;
    deps.compilation_unit = project::CompilationUnit;
    deps.read_bytes = frontend::ReadBytesDefault;
    deps.load_source = core::LoadSource;
    deps.parse_file = ast::ParseFile;
    deps.inspect_source = [](const core::SourceFile& source) {
      return InspectSource(source);
    };

    log_machine("phase=parse-modules");
    core::Conformance::SetPhase("parse");
    const auto parse_start = std::chrono::steady_clock::now();
    std::unordered_map<std::string, const project::Assembly*> assembly_by_name;
    assembly_by_name.reserve(proj.assemblies.size());
    for (const auto& assembly : proj.assemblies) {
      assembly_by_name.emplace(assembly.name, &assembly);
    }
    const auto all_module_names = ModuleNamesForAssemblies(proj.assemblies);
    const auto module_owner = ModuleOwnerMapForAssemblies(proj.assemblies);

    std::vector<std::string> pending_assemblies = {proj.assembly.name};
    std::unordered_set<std::string> seen_assemblies = {proj.assembly.name};
    std::vector<project::ModuleInfo> reachable_modules;
    std::vector<ast::ASTModule> parsed_modules;
    std::optional<std::vector<ast::ASTModule>> parsed_project_module_set;
    frontend::UnsafeSpanMap parsed_unsafe_spans_by_file;
    core::DiagnosticStream parse_phase_diags;
    core::DiagnosticStream comptime_phase_diags;
    bool parse_ok = true;
    bool comptime_ok = true;

    for (std::size_t i = 0; i < pending_assemblies.size() && parse_ok && comptime_ok;
         ++i) {
      const auto asm_it = assembly_by_name.find(pending_assemblies[i]);
      if (asm_it == assembly_by_name.end()) {
        continue;
      }
      const auto& assembly = *asm_it->second;
      progress("Parsing",
               assembly.name + " (" +
                   std::to_string(assembly.modules.size()) + " modules)");
      log_machine("phase=parse-modules assembly-start name=" + assembly.name +
                  " modules=" + std::to_string(assembly.modules.size()) +
                  " source_root=" + assembly.source_root.generic_string());

      auto parsed_chunk = frontend::ParseModulesWithDeps(
          assembly.modules, assembly.source_root, assembly.name, deps);
      for (const auto& diag : parsed_chunk.diags) {
        core::Emit(parse_phase_diags, diag);
      }
      const bool parse_chunk_has_errors = core::HasError(parsed_chunk.diags);
      if (!parsed_chunk.modules.has_value() || parse_chunk_has_errors) {
        log_machine("phase=parse-modules assembly-finish name=" + assembly.name +
                    " ok=false parsed_modules=" +
                    std::to_string(parsed_chunk.modules.has_value()
                                       ? parsed_chunk.modules->size()
                                       : 0) +
                    " emitted_diags=" +
                    std::to_string(parsed_chunk.diags.size()));
        parse_ok = false;
        break;
      }
      reachable_modules.insert(reachable_modules.end(), assembly.modules.begin(),
                               assembly.modules.end());

      std::vector<ast::ASTModule> stage_modules = std::move(*parsed_chunk.modules);
      for (auto& [path, spans] : parsed_chunk.unsafe_spans_by_file) {
        parsed_unsafe_spans_by_file.insert_or_assign(std::move(path),
                                                     std::move(spans));
      }
      if (!ValidateParsedTypeAttributeLists(stage_modules, parse_phase_diags)) {
        log_machine("phase=parse-modules assembly-finish name=" + assembly.name +
                    " ok=false attr-validation=true parsed_modules=" +
                    std::to_string(stage_modules.size()) +
                    " emitted_diags=" +
                    std::to_string(parsed_chunk.diags.size()));
        parse_ok = false;
        break;
      }
      log_machine("phase=parse-modules assembly-finish name=" + assembly.name +
                  " ok=true parsed_modules=" +
                  std::to_string(stage_modules.size()) +
                  " emitted_diags=" +
                  std::to_string(parsed_chunk.diags.size()));
      for (const auto& module : stage_modules) {
        for (const auto& item : module.items) {
          const auto* import = std::get_if<ast::ImportDecl>(&item);
          if (import) {
            const auto imported_assembly = ResolveImportedAssemblyName(
                *import, module.path, all_module_names, module_owner);
            if (!imported_assembly.has_value() ||
                assembly_by_name.find(*imported_assembly) ==
                    assembly_by_name.end()) {
              continue;
            }
            if (seen_assemblies.insert(*imported_assembly).second) {
              pending_assemblies.push_back(*imported_assembly);
            }
            continue;
          }
          const auto* using_decl = std::get_if<ast::UsingDecl>(&item);
          if (!using_decl) {
            continue;
          }
          const auto using_assembly = ResolveUsingAssemblyName(
              *using_decl, module.path, all_module_names, module_owner);
          if (!using_assembly.has_value() ||
              assembly_by_name.find(*using_assembly) == assembly_by_name.end()) {
            continue;
          }
          if (seen_assemblies.insert(*using_assembly).second) {
            pending_assemblies.push_back(*using_assembly);
          }
        }
      }

      if (!opts->phase1_only) {
        core::Conformance::SetPhase("comptime");
        log_machine("phase=comptime assembly-start name=" + assembly.name +
                    " modules=" + std::to_string(stage_modules.size()) +
                    " source_root=" + assembly.source_root.generic_string());
        auto expanded_chunk =
            frontend::ExecuteComptime(stage_modules, proj.root, assembly.source_root);
        for (const auto& diag : expanded_chunk.diags) {
          core::Emit(comptime_phase_diags, diag);
        }
        const bool comptime_chunk_has_errors = core::HasError(expanded_chunk.diags);
        if (!expanded_chunk.modules.has_value() || comptime_chunk_has_errors) {
          log_machine("phase=comptime assembly-finish name=" + assembly.name +
                      " ok=false expanded_modules=" +
                      std::to_string(expanded_chunk.modules.has_value()
                                         ? expanded_chunk.modules->size()
                                         : 0) +
                      " emitted_diags=" +
                      std::to_string(expanded_chunk.diags.size()));
          comptime_ok = false;
          core::Conformance::SetPhase("parse");
          break;
        }
        log_machine("phase=comptime assembly-finish name=" + assembly.name +
                    " ok=true expanded_modules=" +
                    std::to_string(expanded_chunk.modules->size()) +
                    " emitted_diags=" +
                    std::to_string(expanded_chunk.diags.size()));
        stage_modules = std::move(*expanded_chunk.modules);
        core::Conformance::SetPhase("parse");
      }

      for (auto& module : stage_modules) {
        parsed_modules.push_back(std::move(module));
      }
    }

    if (parse_ok && comptime_ok) {
      auto project_modules = parsed_modules;
      for (const auto& assembly : proj.assemblies) {
        if (seen_assemblies.find(assembly.name) != seen_assemblies.end()) {
          continue;
        }
        progress("Parsing",
                 assembly.name + " (" +
                     std::to_string(assembly.modules.size()) + " modules)");
        log_machine("phase=parse-modules assembly-start name=" + assembly.name +
                    " modules=" + std::to_string(assembly.modules.size()) +
                    " source_root=" + assembly.source_root.generic_string());

        auto parsed_chunk = frontend::ParseModulesWithDeps(
            assembly.modules, assembly.source_root, assembly.name, deps);
        for (const auto& diag : parsed_chunk.diags) {
          core::Emit(parse_phase_diags, diag);
        }
        const bool parse_chunk_has_errors = core::HasError(parsed_chunk.diags);
        if (!parsed_chunk.modules.has_value() || parse_chunk_has_errors) {
          log_machine("phase=parse-modules assembly-finish name=" +
                      assembly.name + " ok=false parsed_modules=" +
                      std::to_string(parsed_chunk.modules.has_value()
                                         ? parsed_chunk.modules->size()
                                         : 0) +
                      " emitted_diags=" +
                      std::to_string(parsed_chunk.diags.size()));
          parse_ok = false;
          break;
        }
        std::vector<ast::ASTModule> stage_modules = std::move(*parsed_chunk.modules);
        for (auto& [path, spans] : parsed_chunk.unsafe_spans_by_file) {
          parsed_unsafe_spans_by_file.insert_or_assign(std::move(path),
                                                       std::move(spans));
        }
        if (!ValidateParsedTypeAttributeLists(stage_modules, parse_phase_diags)) {
          log_machine("phase=parse-modules assembly-finish name=" +
                      assembly.name + " ok=false attr-validation=true parsed_modules=" +
                      std::to_string(stage_modules.size()) +
                      " emitted_diags=" +
                      std::to_string(parsed_chunk.diags.size()));
          parse_ok = false;
          break;
        }
        log_machine("phase=parse-modules assembly-finish name=" + assembly.name +
                    " ok=true parsed_modules=" +
                    std::to_string(stage_modules.size()) +
                    " emitted_diags=" +
                    std::to_string(parsed_chunk.diags.size()));
        if (!opts->phase1_only) {
          core::Conformance::SetPhase("comptime");
          log_machine("phase=comptime assembly-start name=" + assembly.name +
                      " modules=" + std::to_string(stage_modules.size()) +
                      " source_root=" + assembly.source_root.generic_string());
          auto expanded_chunk = frontend::ExecuteComptime(stage_modules, proj.root,
                                                          assembly.source_root);
          for (const auto& diag : expanded_chunk.diags) {
            core::Emit(comptime_phase_diags, diag);
          }
          const bool comptime_chunk_has_errors = core::HasError(expanded_chunk.diags);
          if (!expanded_chunk.modules.has_value() || comptime_chunk_has_errors) {
            log_machine("phase=comptime assembly-finish name=" + assembly.name +
                        " ok=false expanded_modules=" +
                        std::to_string(expanded_chunk.modules.has_value()
                                           ? expanded_chunk.modules->size()
                                           : 0) +
                        " emitted_diags=" +
                        std::to_string(expanded_chunk.diags.size()));
            comptime_ok = false;
            core::Conformance::SetPhase("parse");
            break;
          }
          log_machine("phase=comptime assembly-finish name=" + assembly.name +
                      " ok=true expanded_modules=" +
                      std::to_string(expanded_chunk.modules->size()) +
                      " emitted_diags=" +
                      std::to_string(expanded_chunk.diags.size()));
          stage_modules = std::move(*expanded_chunk.modules);
          core::Conformance::SetPhase("parse");
        }

        for (auto& module : stage_modules) {
          project_modules.push_back(std::move(module));
        }
      }
      if (parse_ok && comptime_ok) {
        parsed_project_module_set = std::move(project_modules);
      }
    }

    parse_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - parse_start).count();
    phase1_ok = parse_ok;
    phase2_ok = parse_ok && comptime_ok;
    const std::size_t parse_phase_error_count =
        CountErrorDiagnostics(diags) + CountErrorDiagnostics(parse_phase_diags);
    if (core::AbortOnErrorCount(effective_error_policy,
                                parse_phase_error_count)) {
      log_machine("phase=parse-modules abort-on-error-count errors=" +
                  std::to_string(parse_phase_error_count));
      phase1_ok = false;
      parse_ok = false;
    }
    std::optional<std::vector<ast::ASTModule>> parsed_module_set;
    project::Project sema_project = proj;
    if (phase1_ok) {
      for (const auto& diag : parse_phase_diags) {
        core::Emit(diags, diag);
      }
      for (const auto& diag : comptime_phase_diags) {
        core::Emit(diags, diag);
      }
      sema_project.modules = std::move(reachable_modules);
    } else {
      analysis::ResolveContext parse_fail_res_ctx;
      parse_fail_res_ctx.parse_ok = false;
      parse_fail_res_ctx.parse_diags = &parse_phase_diags;
      const auto parse_failed_resolution =
          analysis::ResolveModules(parse_fail_res_ctx);
      resolve_ok = parse_failed_resolution.ok;
      for (const auto& diag : parse_failed_resolution.diags) {
        core::Emit(diags, diag);
      }
    }

    if (phase2_ok) {
      parsed_module_set = std::move(parsed_modules);
    }

    if (parsed_module_set.has_value()) {
      std::size_t ffi_import_count = 0;
      std::size_t ffi_export_count = 0;
      for (const auto& module : *parsed_module_set) {
        const auto surface = project::CollectFfiSurface(module);
        ffi_import_count += surface.imports.size();
        ffi_export_count += surface.exports.size();
      }
      log_machine("phase=parse-modules step=ffi-surface imports=" +
                  std::to_string(ffi_import_count) +
                  " exports=" + std::to_string(ffi_export_count));
    }

    if (opts->dump_ast && parsed_module_set.has_value()) {
      ast::DumpOptions dump_opts;
      dump_opts.include_spans = true;
      for (const auto& module :
           FilterAstModulesForProject(*parsed_module_set, sema_project)) {
        std::cout << "module " << core::StringOfPath(module.path) << "\n";
        for (const auto& item : module.items) {
          std::cout << "  " << ast::to_string(item, dump_opts) << "\n";
        }
      }
    }

    if (!HasBlockingErrorsForSema(diags) && parsed_module_set.has_value() &&
        !opts->phase1_only && !opts->check_only &&
        !opts->emit_ir && !opts->no_output) {
      const auto noop_check =
          CheckIncrementalNoopReuse(sema_project, target_profile,
                                    *parsed_module_set, *opts, diags);
      log_machine("phase=incremental-fastpath reusable=" +
                  std::string(noop_check.reusable ? "true" : "false") +
                  " reason=" + noop_check.reason + " modules=" +
                  std::to_string(noop_check.modules));
      if (is_verbose) {
        if (noop_check.reusable) {
          std::cerr << "  incremental: cache hit (no changes)\n";
        } else {
          std::cerr << "  incremental: rebuild required (" << noop_check.reason << ")\n";
        }
        std::cerr.flush();
      }
      if (noop_check.reusable) {
        resolve_ok = true;
        typecheck_ok = true;
        phase4_ok = true;
        incremental_noop_reused = true;
      }
    }

    if (!HasBlockingErrorsForSema(diags) && parsed_module_set.has_value() &&
        !opts->phase1_only && !incremental_noop_reused) {
      const auto sema_start = std::chrono::steady_clock::now();
      progress("Checking", proj.assembly.name);
      log_machine("phase=sema");
      core::Conformance::SetPhase("resolve");

      analysis::ScopeContext ctx;
      ctx.project = &sema_project;
      ctx.target_profile = target_profile;
      ctx.sigma.mods = *parsed_module_set;
      ctx.sigma.unsafe_spans_by_file = parsed_unsafe_spans_by_file;
      ctx.scopes = {analysis::Scope{}, analysis::Scope{}, analysis::Scope{}};
      log_machine("phase=sema step=context-init modules=" +
                  std::to_string(parsed_module_set->size()));

      std::size_t visibility_index = 0;
      for (const auto& module : *parsed_module_set) {
        ++visibility_index;
        ctx.current_module = module.path;
        const std::string module_name = core::StringOfPath(module.path);
        log_machine("phase=sema step=visibility-check-start index=" +
                    std::to_string(visibility_index) + "/" +
                    std::to_string(parsed_module_set->size()) + " module=" +
                    module_name);
        const auto vis_diags = analysis::CheckModuleVisibility(ctx, module);
        for (const auto& diag : vis_diags) {
          core::Emit(diags, diag);
        }
        log_machine("phase=sema step=visibility-check-finish index=" +
                    std::to_string(visibility_index) + "/" +
                    std::to_string(parsed_module_set->size()) + " module=" +
                    module_name + " emitted_diags=" +
                    std::to_string(vis_diags.size()));
      }

      log_machine("phase=sema step=name-map-collect-start modules=" +
                  std::to_string(ctx.sigma.mods.size()));
      const auto name_maps = analysis::CollectNameMaps(ctx);
      for (const auto& diag : name_maps.diags) {
        core::Emit(diags, diag);
      }
      log_machine("phase=sema step=name-map-collect-finish emitted_diags=" +
                  std::to_string(name_maps.diags.size()));

      if (!HasBlockingErrorsForSema(diags)) {
        log_machine("phase=sema step=populate-sigma-start modules=" +
                    std::to_string(ctx.sigma.mods.size()));
        analysis::PopulateSigma(ctx);
        log_machine("phase=sema step=populate-sigma-finish");
        const auto module_names = analysis::ModuleNamesOf(sema_project);

        analysis::ResolveContext res_ctx;
        res_ctx.ctx = &ctx;
        res_ctx.name_maps = &name_maps.name_maps;
        res_ctx.module_names = &module_names;
        res_ctx.can_access = analysis::CanAccess;
        res_ctx.parse_ok = phase1_ok;
        res_ctx.parse_diags = &parse_phase_diags;

        log_machine("phase=sema step=resolve-start modules=" +
                    std::to_string(ctx.sigma.mods.size()));
        const auto resolved = analysis::ResolveModules(res_ctx);
        resolve_ok = resolved.ok;
        for (const auto& diag : resolved.diags) {
          core::Emit(diags, diag);
        }
        log_machine("phase=sema step=resolve-finish ok=" +
                    std::string(resolved.ok ? "true" : "false") +
                    " emitted_diags=" + std::to_string(resolved.diags.size()) +
                    " resolved_modules=" +
                    std::to_string(resolved.modules.size()));

        if (resolved.ok) {
          ctx.sigma.mods = resolved.modules;
          log_machine("phase=sema step=resolve-apply-start modules=" +
                      std::to_string(ctx.sigma.mods.size()));
          analysis::PopulateSigma(ctx);
          log_machine("phase=sema step=resolve-apply-finish");
        }

        std::optional<project::AssemblyImportGraph> assembly_graph;
        bool assembly_graph_ok = false;
        if (!HasBlockingErrorsForSema(diags) && resolve_ok) {
          typecheck_ok = true;
          const auto& graph_modules = parsed_project_module_set.has_value()
                                          ? *parsed_project_module_set
                                          : ctx.sigma.mods;
          assembly_graph =
              project::BuildAssemblyImportGraph(sema_project, graph_modules);
          assembly_graph_ok =
              project::ValidateAssemblyImportGraphStructure(sema_project,
                                                           *assembly_graph,
                                                           diags);
        }

        if (!HasBlockingErrorsForSema(diags) && resolve_ok &&
            assembly_graph_ok) {
          const auto& graph_modules = parsed_project_module_set.has_value()
                                          ? *parsed_project_module_set
                                          : ctx.sigma.mods;
          if (!project::ValidateHostedLibraryImportGraph(sema_project,
                                                         *assembly_graph,
                                                         graph_modules,
                                                         diags)) {
            typecheck_ok = false;
          }
        }

        if (!HasBlockingErrorsForSema(diags) && resolve_ok &&
            assembly_graph_ok && typecheck_ok) {
          if (opts->dump_project) {
            auto output_project = project::BuildOutputProjectForAssembly(
                sema_project, *assembly_graph, sema_project.assembly.name);
            if (!output_project.has_value()) {
              EmitInternalDiagnostic(
                  diags, cursive::core::Severity::Error, std::nullopt,
                  "Failed to construct output project for dump-project: " +
                      sema_project.assembly.name);
              typecheck_ok = false;
            } else {
              const auto lines =
                  project::DumpProject(*output_project, target_profile,
                                       true);
              for (const auto& line : lines) {
                std::cout << line << "\n";
              }
              return 0;
            }
          }

          core::Conformance::SetPhase("typecheck");
          const auto typecheck_start = std::chrono::steady_clock::now();
          log_machine("phase=sema step=typecheck-start modules=" +
                      std::to_string(ctx.sigma.mods.size()));
          const auto typechecked =
              analysis::TypecheckModules(ctx, ctx.sigma.mods,
                                         &name_maps.name_maps);
          for (const auto& diag : typechecked.diags) {
            core::Emit(diags, diag);
          }
          typecheck_ok = typechecked.ok;

          if (typecheck_ok) {
            std::vector<const ast::ASTModule*> cap_modules;
            cap_modules.reserve(ctx.sigma.mods.size());
            for (const auto& module : ctx.sigma.mods) {
              cap_modules.push_back(&module);
            }

            auto call_graph = analysis::BuildCallGraph(ctx, cap_modules);
            analysis::PropagateCapabilityRequirements(call_graph);
            analysis::AnnotateCapabilityFlow(call_graph);
            const auto cap_chain = analysis::ValidateCapabilityChain(
                ctx, call_graph, &typechecked.expr_types);
            if (!cap_chain.valid) {
              typecheck_ok = false;
              std::size_t emitted_cap_diags = 0;
              for (const auto& err : cap_chain.errors) {
                EmitCapabilityChainErrorDiagnostic(diags, err);
                ++emitted_cap_diags;
              }
              for (const auto& leak : cap_chain.leaks) {
                EmitCapabilityLeakDiagnostic(diags, leak);
                ++emitted_cap_diags;
              }
              log_machine("phase=sema step=capability-chain-finish ok=false errors=" +
                          std::to_string(cap_chain.errors.size()) +
                          " leaks=" + std::to_string(cap_chain.leaks.size()) +
                          " emitted_diags=" + std::to_string(emitted_cap_diags));
            } else {
              log_machine("phase=sema step=capability-chain-finish ok=true");
            }

            log_machine("phase=sema step=authority-model-start modules=" +
                        std::to_string(cap_modules.size()));
            const auto authority = analysis::ValidateModuleAuthority(
                ctx, cap_modules, &typechecked.expr_types);
            if (!authority.valid) {
              typecheck_ok = false;
              std::size_t emitted_authority_diags = 0;
              for (const auto& err : authority.errors) {
                EmitAuthorityValidationDiagnostic(diags, err);
                ++emitted_authority_diags;
              }
              log_machine(
                  "phase=sema step=authority-model-finish ok=false errors=" +
                  std::to_string(authority.errors.size()) +
                  " emitted_diags=" +
                  std::to_string(emitted_authority_diags));
            } else {
              log_machine("phase=sema step=authority-model-finish ok=true");
            }
          }

          const auto typecheck_elapsed =
              std::chrono::duration_cast<std::chrono::milliseconds>(
                  std::chrono::steady_clock::now() - typecheck_start)
                  .count();
          log_machine("phase=sema step=typecheck-finish ok=" +
                      std::string(typecheck_ok ? "true" : "false") +
                      " emitted_diags=" +
                      std::to_string(typechecked.diags.size()) +
                      " elapsed_ms=" + std::to_string(typecheck_elapsed));

          check_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
              std::chrono::steady_clock::now() - sema_start).count();
          if (typecheck_ok && opts->check_only) {
            phase4_ok = true;  // --check: skip codegen, report success
          } else if (typecheck_ok) {
            const auto codegen_start = std::chrono::steady_clock::now();
            progress("Compiling",
                     sema_project.assembly.name + " (" +
                         std::to_string(ctx.sigma.mods.size()) + " modules)");
            log_machine("phase=codegen");
            core::Conformance::SetPhase("codegen");

            codegen::LowerCtx lower_ctx;
            lower_ctx.sigma = &ctx.sigma;
            lower_ctx.executable_project =
                sema_project.assembly.kind == "executable";
            lower_ctx.shared_library_project =
                sema_project.assembly.kind == "library" &&
                sema_project.assembly.link_kind.value_or("shared") == "shared";
            lower_ctx.log_enabled = opts->log_enabled;
            lower_ctx.log_to_console = opts->log_to_console;
            lower_ctx.log_to_file = opts->log_to_file;
            lower_ctx.log_file_path = EffectiveRuntimeLogFilePath(sema_project, *opts);
            lower_ctx.trace = opts->trace;
            lower_ctx.trace_filter_mask = opts->trace_filter_mask;
            lower_ctx.trace_min_level = opts->trace_min_level;
            lower_ctx.trace_root = sema_project.root.generic_string();

            const auto* expr_types = &typechecked.expr_types;
            const auto* dynamic_refine_checks =
                &typechecked.dynamic_refine_checks;
            lower_ctx.expr_types =
                const_cast<analysis::ExprTypeMap*>(expr_types);
            lower_ctx.dynamic_refine_checks =
                const_cast<analysis::DynamicRefineExprMap*>(
                    dynamic_refine_checks);
            lower_ctx.expr_type =
                [expr_types](const ast::Expr& expr) -> analysis::TypeRef {
              if (!expr_types) {
                return nullptr;
              }
              const auto it = expr_types->find(&expr);
              if (it == expr_types->end()) {
                return nullptr;
              }
              return it->second;
            };

            lower_ctx.resolve_name =
                [&](const std::string& name)
                    -> std::optional<std::vector<std::string>> {
              const auto module_key =
                  analysis::PathKeyOf(lower_ctx.module_path);
              const auto map_it = name_maps.name_maps.find(module_key);
              if (map_it == name_maps.name_maps.end()) {
                if (!codegen::BuiltinSym(name).empty()) {
                  return std::vector<std::string>{name};
                }
                return std::nullopt;
              }
              const auto ent_it =
                  map_it->second.find(analysis::IdKeyOf(name));
              if (ent_it == map_it->second.end()) {
                if (!codegen::BuiltinSym(name).empty()) {
                  return std::vector<std::string>{name};
                }
                return std::nullopt;
              }
              const auto& ent = ent_it->second;
              if (ent.kind != analysis::EntityKind::Value) {
                if (codegen::BuiltinSym(name).empty()) {
                  return std::nullopt;
                }
                return std::vector<std::string>{name};
              }
              const std::string resolved_name = ent.target_opt.value_or(name);
              if (!ent.origin_opt.has_value()) {
                if (codegen::BuiltinSym(resolved_name).empty()) {
                  return std::nullopt;
                }
                return std::vector<std::string>{resolved_name};
              }
              std::vector<std::string> full = *ent.origin_opt;
              full.push_back(resolved_name);
              return full;
            };

            lower_ctx.resolve_type_name =
                [&](const std::string& name)
                    -> std::optional<std::vector<std::string>> {
              const auto module_key =
                  analysis::PathKeyOf(lower_ctx.module_path);
              const auto map_it = name_maps.name_maps.find(module_key);
              if (map_it == name_maps.name_maps.end()) {
                return std::nullopt;
              }
              const auto ent_it =
                  map_it->second.find(analysis::IdKeyOf(name));
              if (ent_it == map_it->second.end()) {
                return std::nullopt;
              }
              const auto& ent = ent_it->second;
              if (ent.kind != analysis::EntityKind::Type ||
                  !ent.origin_opt.has_value()) {
                return std::nullopt;
              }
              std::vector<std::string> full = *ent.origin_opt;
              const std::string resolved_name = ent.target_opt.value_or(name);
              full.push_back(resolved_name);
              return full;
            };

            if (typechecked.init_plan.has_value()) {
              lower_ctx.init_order = typechecked.init_plan->init_order;
              lower_ctx.init_modules = typechecked.init_plan->graph.modules;
              lower_ctx.init_eager_edges =
                  typechecked.init_plan->graph.eager_edges;
            }

            if (opts->emit_ir) {
              for (const ast::ASTModule& module : ctx.sigma.mods) {
                lower_ctx.module_path = module.path;
                lower_ctx.resolve_failed = false;
                lower_ctx.codegen_failed = false;
                lower_ctx.resolve_failures.clear();
                auto decls = codegen::LowerModule(module, lower_ctx);
                if (lower_ctx.resolve_failed || lower_ctx.codegen_failed) {
                  if (const auto diag = core::MakeDiagnosticById("E-OUT-0403")) {
                    core::Emit(diags, *diag);
                  }
                  phase4_ok = false;
                  break;
                }
                std::cout << codegen::DumpIR(decls) << "\n";
              }
              if (!core::HasError(diags)) {
                phase4_ok = true;
              }
            } else if (!opts->no_output) {
              if (!assembly_graph.has_value()) {
                phase4_ok = false;
              } else {
                const auto* ctx_ptr = &ctx;
                const auto* name_maps_ptr = &name_maps;
                const auto* typechecked_ptr = &typechecked;
                const bool log_enabled = lower_ctx.log_enabled;
                const bool log_to_console = lower_ctx.log_to_console;
                const bool log_to_file = lower_ctx.log_to_file;
                const bool trace = lower_ctx.trace;
                const auto trace_filter_mask = lower_ctx.trace_filter_mask;
                const auto trace_min_level = lower_ctx.trace_min_level;
                const std::string trace_root = lower_ctx.trace_root;
                const std::string log_file_path = lower_ctx.log_file_path;
                auto lazy_caches = std::make_shared<std::unordered_map<
                    std::string, std::shared_ptr<CodegenCache>>>();
                auto lazy_cache_mu = std::make_shared<std::mutex>();
                auto ensure_cache =
                    [lazy_caches,
                     lazy_cache_mu,
                     ctx_ptr,
                     name_maps_ptr,
                     typechecked_ptr,
                     log_enabled,
                     log_to_console,
                     log_to_file,
                     trace,
                     trace_filter_mask,
                     trace_min_level,
                     trace_root,
                     log_file_path,
                     &log_machine](
                        const project::Project& p)
                        -> std::shared_ptr<CodegenCache> {
                  const std::string cache_key =
                      p.assembly.name + "|" + p.assembly.kind + "|" +
                      p.assembly.link_kind.value_or("none");
                  {
                    std::lock_guard<std::mutex> lock(*lazy_cache_mu);
                    auto cache_it = lazy_caches->find(cache_key);
                    if (cache_it == lazy_caches->end()) {
                      const auto cache_start = std::chrono::steady_clock::now();
                      log_machine("phase=codegen cache=build-start key=" +
                                  cache_key + " modules=" +
                                  std::to_string(p.modules.size()));
                      auto cache = BuildCodegenCache(p,
                                                     *ctx_ptr,
                                                     *name_maps_ptr,
                                                     *typechecked_ptr);
                      if (cache) {
                        cache->ctx.log_enabled = log_enabled;
                        cache->ctx.log_to_console = log_to_console;
                        cache->ctx.log_to_file = log_to_file;
                        cache->ctx.trace = trace;
                        cache->ctx.trace_filter_mask = trace_filter_mask;
                        cache->ctx.trace_min_level = trace_min_level;
                        cache->ctx.trace_root = trace_root;
                        cache->ctx.log_file_path = log_file_path;
                      }
                      (*lazy_caches)[cache_key] = cache;
                      const auto elapsed =
                          std::chrono::duration_cast<std::chrono::milliseconds>(
                              std::chrono::steady_clock::now() - cache_start)
                              .count();
                      const bool cache_ok = cache && cache->ok.load();
                      const std::size_t lowered_count =
                          cache ? cache->modules.size() : 0;
                      log_machine("phase=codegen cache=build-finish key=" +
                                  cache_key + " ok=" +
                                  std::string(cache_ok ? "true" : "false") +
                                  " lowered=" +
                                  std::to_string(lowered_count) +
                                  " elapsed_ms=" + std::to_string(elapsed));
                    }
                  }
                  return (*lazy_caches)[cache_key];
                };

                if (IncrementalEnabled()) {
                  log_machine("phase=codegen incremental=deferred-per-artifact");
                }
                auto output_project_opt =
                    cursive::project::BuildOutputProjectForAssembly(
                        sema_project, *assembly_graph, sema_project.assembly.name);
                if (!output_project_opt.has_value()) {
                  EmitInternalDiagnostic(
                      diags, core::Severity::Error, std::nullopt,
                      "Failed to construct output project for assembly: " +
                          sema_project.assembly.name);
                  phase4_ok = false;
                } else {
                  auto output_project = std::move(*output_project_opt);
                  auto incremental_cache = std::make_shared<std::unordered_map<
                      std::string, IncrementalBuildDataResult>>();
                  auto incremental_mu = std::make_shared<std::mutex>();
                  auto ensure_incremental =
                      [incremental_cache,
                       incremental_mu,
                       &diags,
                       &log_machine,
                       opts,
                       &ctx,
                       target_profile](const project::Project& p)
                          -> std::optional<IncrementalBuildDataResult> {
                    if (!IncrementalEnabled()) {
                      return std::nullopt;
                    }
                    const std::string cache_key =
                        p.assembly.name + "|" + p.assembly.kind + "|" +
                        p.assembly.link_kind.value_or("none");
                    {
                      std::lock_guard<std::mutex> lock(*incremental_mu);
                      const auto it = incremental_cache->find(cache_key);
                      if (it != incremental_cache->end()) {
                        return it->second;
                      }
                    }

                    const auto build_ast_modules =
                        FilterAstModulesForProject(ctx.sigma.mods, p);
                    const auto fingerprint_start =
                        std::chrono::steady_clock::now();
                    log_machine(
                        "phase=codegen incremental=fingerprint-start assembly=" +
                        p.assembly.name + " modules=" +
                        std::to_string(p.modules.size()));
                    auto incremental = BuildIncrementalBuildData(
                        p, target_profile, build_ast_modules, *opts,
                        diags);
                    const auto elapsed =
                        std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::steady_clock::now() - fingerprint_start)
                            .count();
                    log_machine(
                        "phase=codegen incremental=fingerprint-finish assembly=" +
                        p.assembly.name + " ok=" +
                        std::string(incremental.ok ? "true" : "false") +
                        " modules=" +
                        std::to_string(incremental.ok
                                           ? incremental.modules.size()
                                           : 0) +
                        " elapsed_ms=" + std::to_string(elapsed));
                    if (!incremental.ok) {
                      return std::nullopt;
                    }

                    std::lock_guard<std::mutex> lock(*incremental_mu);
                    (*incremental_cache)[cache_key] = incremental;
                    return incremental;
                  };

                  progress("Compiling",
                           output_project.assembly.name + " (" +
                               std::to_string(output_project.modules.size()) +
                               " modules)");

                  project::OutputPipelineDeps out_deps;
                  out_deps.ensure_dir = EnsureDir;
                  out_deps.codegen_obj =
                      [ensure_cache, target_profile](const project::ModuleInfo& module,
                                            const project::Project& p)
                          -> std::optional<std::string> {
                    const auto cache = ensure_cache(p);
                    if (!cache || !cache->ok.load()) {
                      return std::nullopt;
                    }
                    return CodegenObj(*cache, module, p, target_profile);
                  };
                  out_deps.codegen_ir =
                      [ensure_cache, target_profile](const project::ModuleInfo& module,
                                            const project::Project& p,
                                            std::string_view emit_ir)
                          -> std::optional<std::string> {
                    const auto cache = ensure_cache(p);
                    if (!cache || !cache->ok.load()) {
                      return std::nullopt;
                    }
                    return CodegenIR(*cache, module, p, target_profile, emit_ir);
                  };
                  out_deps.write_file = WriteFile;
                  out_deps.resolve_tool = project::ResolveTool;
                  out_deps.assemble_ir = project::AssembleIR;
                  out_deps.resolve_runtime_lib = project::ResolveRuntimeLib;
                  out_deps.invoke_linker = project::InvokeLinker;
                  out_deps.linker_syms = project::LinkerSyms;
                  out_deps.archive_members = project::ArchiveMembers;
                  out_deps.invoke_archiver = project::InvokeArchiver;
                  out_deps.resolve_ast_modules =
                      [&ctx](const project::Project& p)
                          -> frontend::ParseModulesResult {
                    frontend::ParseModulesResult parsed;
                    parsed.modules =
                        FilterAstModulesForProject(ctx.sigma.mods, p);
                    return parsed;
                  };
                  out_deps.resolve_shared_library_exports =
                      [ensure_cache](const project::Project& p)
                          -> std::optional<project::SharedLibraryExports> {
                    const auto cache = ensure_cache(p);
                    if (!cache || !cache->ok.load()) {
                      return std::nullopt;
                    }
                    if (!PopulateCodegenModules(*cache, p)) {
                      return std::nullopt;
                    }
                    return ResolveSharedLibraryExports(p, *cache);
                  };
                  out_deps.prepare_codegen_context =
                      [ensure_cache](const project::Project& p,
                                     const project::SharedLibraryExports& exports)
                          -> bool {
                    const auto cache = ensure_cache(p);
                    if (!cache || !cache->ok.load()) {
                      return false;
                    }
                    return PrepareSharedLibraryCodegenContext(p, *cache, exports);
                  };
                  out_deps.incremental_module =
                      [ensure_incremental](const project::ModuleInfo& module,
                                           const project::Project& p)
                          -> std::optional<project::IncrementalModuleInfo> {
                    const auto incremental = ensure_incremental(p);
                    if (!incremental.has_value()) {
                      return std::nullopt;
                    }
                    const auto it = incremental->modules.find(module.path);
                    if (it == incremental->modules.end()) {
                      return std::nullopt;
                    }
                    return it->second;
                  };
                  out_deps.incremental_build_key =
                      [ensure_incremental](const project::Project& p)
                          -> std::optional<std::string> {
                    const auto incremental = ensure_incremental(p);
                    if (!incremental.has_value()) {
                      return std::nullopt;
                    }
                    return incremental->build_key;
                  };
                  out_deps.codegen_obj_thread_safe = true;

                  const auto codegen_cache = ensure_cache(output_project);
                  if (!codegen_cache || !codegen_cache->ok.load()) {
                    EmitInternalDiagnostic(
                        diags, core::Severity::Error, std::nullopt,
                        "Failed to build codegen cache for assembly: " +
                            output_project.assembly.name);
                    phase4_ok = false;
                  } else {
                    if (!PopulateCodegenModules(*codegen_cache, output_project)) {
                      EmitInternalDiagnostic(
                          diags, core::Severity::Error, std::nullopt,
                          "Failed to lower complete module set for assembly: " +
                              output_project.assembly.name);
                      phase4_ok = false;
                    } else {
                      auto output =
                          project::OutputPipeline(output_project,
                                                  target_profile,
                                                  out_deps);
                      AppendDiags(diags, output.diags);
                      phase4_ok = output.artifacts.has_value();
                    }
                  }
                }
              }
            } else {
              phase4_ok = true;
            }
            codegen_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - codegen_start).count();
          }
        }
      }
    }
  }

  // ========================================================================
  // Conformance Checking
  // ========================================================================

  TruncateDiagnosticsToErrorCap(diags, effective_error_policy);

  analysis::PhaseOrderResult phases;
  phases.phase1_ok = phase1_ok;
  phases.phase2_ok = phase2_ok;
  phases.phase3_ok = opts->phase1_only
                         ? phase1_ok
                         : (phase1_ok && resolve_ok && typecheck_ok);
  phases.phase4_ok = opts->phase1_only ? false : phase4_ok;

  analysis::ConformanceInput input;
  input.phase_orders = phases;
  analysis::ConformanceJudgmentEvidence evidence;
  evidence.project_bound = project_result.project.has_value();
  evidence.parse_modules_ok = phase1_ok;
  evidence.execute_comptime_ok = phase2_ok;
  evidence.phase3_checks.resolve_modules_ok = resolve_ok;
  evidence.phase3_checks.decl_typing_ok = typecheck_ok;
  evidence.phase3_checks.main_check_ok = typecheck_ok;
  evidence.output_pipeline_ok = phase4_ok;
  input.evidence = evidence;
  input.error_count = CountErrorDiagnostics(diags);
  input.error_policy = effective_error_policy;

  const bool rejected =
      opts->phase1_only ? false : analysis::RejectIllFormed(input);

  // ========================================================================
  // Output Diagnostics
  // ========================================================================

  // Lazy source file registry: caches file contents for diagnostic rendering.
  std::unordered_map<std::string, std::optional<std::string>> source_cache;
  core::SourceRegistry source_registry =
      [&source_cache](const std::string& path)
          -> std::optional<std::string> {
    auto it = source_cache.find(path);
    if (it != source_cache.end()) {
      return it->second;
    }
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
      source_cache[path] = std::nullopt;
      return std::nullopt;
    }
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    source_cache[path] = content;
    return content;
  };

  if (opts->diag_json) {
    const auto ordered_json = core::Order(diags);
    std::cout << DiagnosticStreamToJson(ordered_json, source_registry) << "\n";
  } else {
    const auto ordered = core::Order(diags);

    // Separate progress output from diagnostics.
    if (!ordered.empty() && show_build_progress) {
      std::cerr << "\n";
    }

    core::RenderOptions render_opts;
    render_opts.color = use_color;
    render_opts.terminal_width = core::TerminalWidth();
    render_opts.context_lines = is_quiet ? 0 : 1;
    for (const auto& diag : ordered) {
      std::cerr << core::RenderRich(diag, source_registry, render_opts)
                << "\n";
    }

    const auto summary = core::DiagnosticSummary(ordered, use_color);
    if (!summary.empty()) {
      std::cerr << "\n" << summary << "\n";
    }
  }

  // ========================================================================
  // Exit Code Determination
  // ========================================================================

  const core::CompileStatusResult status = core::CompileStatus(diags);
  const bool ok = status == core::CompileStatusResult::Ok && !rejected;

  if (core::IsDebugEnabled("pipeline")) {
    std::cerr << "[trace][build] pipeline phase1_ok=" << phase1_ok
              << " phase2_ok=" << phase2_ok
              << " resolve_ok=" << resolve_ok
              << " typecheck_ok=" << typecheck_ok << " phase4_ok=" << phase4_ok
              << " diags=" << diags.size() << " rejected=" << rejected
              << " status=" << static_cast<int>(status) << "\n";
  }

  // Machine-format finish event (debug only).
  if (debug_pipeline) {
    std::size_t error_count = 0;
    for (const auto& diag : diags) {
      if (diag.severity == core::Severity::Error) {
        ++error_count;
      }
    }
    std::cerr << "[trace][build] pid=" << CurrentProcessId()
              << " event=finish ok=" << (ok ? "true" : "false")
              << " phase1_ok=" << phase1_ok
              << " phase2_ok=" << phase2_ok
              << " resolve_ok=" << resolve_ok
              << " typecheck_ok=" << typecheck_ok
              << " phase4_ok=" << phase4_ok << " errors=" << error_count
              << " rejected=" << rejected
              << " status=" << static_cast<int>(status) << "\n";
  }

  // Verbose per-phase timing breakdown.
  if (is_verbose && !opts->diag_json) {
    std::cerr << "\n  Phase timing:\n";
    std::cerr << "    parse:   " << parse_ms << "ms\n";
    std::cerr << "    check:   " << check_ms << "ms\n";
    std::cerr << "    codegen: " << codegen_ms << "ms\n";
    std::cerr.flush();
  }

  // Human-friendly build result line.
  if (!opts->diag_json && show_build_progress) {
    const auto build_elapsed =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - build_start);
    const auto ms = build_elapsed.count();
    std::string elapsed_str;
    if (ms < 1000) {
      elapsed_str = std::to_string(ms) + "ms";
    } else {
      const auto secs = ms / 1000;
      const auto frac = (ms % 1000) / 10;
      elapsed_str = std::to_string(secs) + "." +
                    (frac < 10 ? "0" : "") + std::to_string(frac) + "s";
    }

    if (ok) {
      std::string detail = "build succeeded";
      if (incremental_noop_reused) {
        detail += " (no changes)";
      }
      detail += " in " + elapsed_str;
      progress("Finished", detail);
    } else {
      progress("Finished", "build failed in " + elapsed_str,
               core::Color::BoldRed);
    }
  }

  return ok ? 0 : 1;
}
