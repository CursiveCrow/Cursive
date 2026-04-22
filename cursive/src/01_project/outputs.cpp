#include "01_project/outputs.h"

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <thread>
#include <optional>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

#include "00_core/assert_spec.h"
#include "00_core/build_log_policy.h"
#include "00_core/diagnostic_messages.h"
#include "00_core/diagnostics.h"
#include "00_core/host/services.h"
#include "00_core/host_primitives.h"
#include "00_core/path.h"
#include "00_core/process_config.h"
#include "00_core/hash.h"
#include "00_core/symbols.h"
#include "01_project/compiler_support_paths.h"
#include "01_project/ir_assembly.h"
#include "01_project/assembly_graph.h"
#include "01_project/assemblies.h"
#include "01_project/deterministic_order.h"
#include "01_project/ffi_library.h"
#include "01_project/link.h"
#include "01_project/project.h"
#include "01_project/target_profile.h"
#include "01_project/tool_resolution.h"
#include "02_source/ast/ast.h"
#include "02_source/parser/parse_modules.h"
#include "03_comptime/comptime.h"
#include "04_analysis/attributes/attribute_registry.h"

namespace cursive::project {

namespace {

void EmitExternal(core::DiagnosticStream& diags, std::string_view code) {
  core::EmitExternalDiagnostic(diags, code);
}

unsigned long CurrentProcessId() {
  return core::CurrentHostProcessId();
}

core::BuildLogMode ResolveOutputLogMode() {
  const bool debug_output =
      core::IsDebugEnabled("pipeline") || core::IsDebugEnabled("output");
  core::BuildLogResolveOptions options;
  options.debug_enabled = debug_output;
  options.cli_progress = core::BuildProgressOverride();
  options.manifest_progress = core::ManifestBuildProgress();
  // Build progress defaults to enabled; keep normal mode concise via summary.
  options.default_enabled = true;
  return core::ResolveBuildLogMode(options);
}

void LogBuildProgress(const std::string& message) {
  const core::BuildLogMode mode = ResolveOutputLogMode();
  if (mode == core::BuildLogMode::None) {
    return;
  }
  if (mode == core::BuildLogMode::Summary &&
      !core::ShouldEmitSummaryBuildLog(core::BuildLogChannel::Output,
                                       message)) {
    return;
  }

  const bool debug_output =
      core::IsDebugEnabled("pipeline") || core::IsDebugEnabled("output");
  if (debug_output) {
    std::cerr << "[trace][build] output pid=" << CurrentProcessId() << " "
              << message << "\n";
  } else {
    std::cerr << "[info][build] output " << message << "\n";
  }
  std::cerr.flush();
}


std::string_view EmitIrMode(const Project& project) {
  if (project.assembly.emit_ir.has_value()) {
    return *project.assembly.emit_ir;
  }
  return "none";
}

bool IsExecutable(const Project& project) {
  return project.assembly.kind == "executable";
}

bool IsLibrary(const Project& project) {
  return project.assembly.kind == "library";
}

bool IsDependency(const Project& project) {
  return project.assembly.kind == "dependency";
}

bool IsSharedLibrary(const Project& project) {
  return IsLibrary(project) && project.assembly.link_kind == "shared";
}

bool IsStaticLibrary(const Project& project) {
  return IsLibrary(project) && project.assembly.link_kind == "static";
}

bool IsDistinct(const std::vector<std::filesystem::path>& paths) {
  std::unordered_set<std::string> seen;
  seen.reserve(paths.size());
  for (const auto& path : paths) {
    const std::string key = path.generic_string();
    if (!seen.insert(key).second) {
      return false;
    }
  }
  return true;
}

bool UnderPath(const std::filesystem::path& path,
               const std::filesystem::path& root) {
  const std::string path_norm = core::Normalize(path.generic_string());
  const std::string root_norm = core::Normalize(root.generic_string());
  return core::Prefix(root_norm, path_norm);
}

std::string JoinList(const std::vector<std::string>& items,
                     std::string_view sep) {
  std::ostringstream oss;
  bool first = true;
  for (const auto& item : items) {
    if (!first) {
      oss << sep;
    }
    first = false;
    oss << item;
  }
  return oss.str();
}

std::string RenderList(const std::vector<std::string>& items) {
  std::ostringstream oss;
  oss << "[";
  oss << JoinList(items, ", ");
  oss << "]";
  return oss.str();
}

void EmitInternalDiagnostic(core::DiagnosticStream& diags,
                            const std::string& message) {
  core::Diagnostic diag;
  diag.severity = core::Severity::Error;
  diag.message = message;
  core::Emit(diags, diag);
}

std::optional<std::vector<ast::ASTModule>> FilterAstModulesForProject(
    const Project& project,
    const std::vector<ast::ASTModule>& all_modules,
    core::DiagnosticStream& diags) {
  std::unordered_map<std::string, const ast::ASTModule*> by_path;
  by_path.reserve(all_modules.size());
  for (const auto& module : all_modules) {
    by_path.emplace(core::StringOfPath(module.path), &module);
  }

  std::vector<ast::ASTModule> selected;
  selected.reserve(project.modules.size());
  for (const auto& module_info : project.modules) {
    const auto it = by_path.find(module_info.path);
    if (it == by_path.end() || it->second == nullptr) {
      EmitInternalDiagnostic(diags,
                             "Parsed AST module missing for `" +
                                 module_info.path + "`");
      return std::nullopt;
    }
    selected.push_back(*it->second);
  }
  return selected;
}

bool ContainsHostExports(const std::vector<ast::ASTModule>& ast_modules) {
  for (const auto& module : ast_modules) {
    for (const auto& item : module.items) {
      const auto* proc = std::get_if<ast::ProcedureDecl>(&item);
      if (proc &&
          analysis::HasAttribute(proc->attrs, analysis::attrs::kHostExport)) {
        return true;
      }
    }
  }
  return false;
}

bool CopyBundledRuntimeSidecars(const std::filesystem::path& bin_dir,
                                TargetProfile target_profile,
                                core::DiagnosticStream& diags) {
  const auto sidecars = CompilerExecutableSidecarPaths(target_profile);
  if (sidecars.empty()) {
    return true;
  }

  std::error_code ec;
  std::filesystem::create_directories(bin_dir, ec);
  if (ec) {
    EmitInternalDiagnostic(diags,
                           "Failed to create artifact bin directory `" +
                               bin_dir.generic_string() + "`");
    return false;
  }

  for (const auto& source : sidecars) {
    const std::filesystem::path destination = bin_dir / source.filename();
    ec.clear();
    std::filesystem::copy_file(source, destination,
                               std::filesystem::copy_options::overwrite_existing,
                               ec);
    if (ec) {
      EmitInternalDiagnostic(diags,
                             "Failed to stage runtime sidecar `" +
                                 source.generic_string() + "` into `" +
                                 destination.generic_string() + "`");
      return false;
    }
  }
  return true;
}

std::vector<std::filesystem::path> ImportedSharedLibraryArtifacts(
    const std::vector<std::filesystem::path>& inputs,
    TargetProfile target_profile) {
  const std::filesystem::path shared_suffix(SharedLibSuffix(target_profile));
  if (shared_suffix.empty()) {
    return {};
  }

  std::vector<std::filesystem::path> artifacts;
  artifacts.reserve(inputs.size());
  std::unordered_set<std::string> seen;
  seen.reserve(inputs.size());

  for (const auto& input : inputs) {
    if (input.extension() != shared_suffix) {
      continue;
    }
    const std::string normalized = core::Normalize(input.generic_string());
    if (!seen.insert(normalized).second) {
      continue;
    }
    artifacts.push_back(input);
  }

  return artifacts;
}

bool CopyImportedSharedLibraryArtifacts(
    const std::filesystem::path& bin_dir,
    const std::vector<std::filesystem::path>& inputs,
    TargetProfile target_profile,
    core::DiagnosticStream& diags) {
  const auto artifacts =
      ImportedSharedLibraryArtifacts(inputs, target_profile);
  if (artifacts.empty()) {
    return true;
  }

  std::error_code ec;
  std::filesystem::create_directories(bin_dir, ec);
  if (ec) {
    EmitInternalDiagnostic(diags,
                           "Failed to create artifact bin directory `" +
                               bin_dir.generic_string() + "`");
    return false;
  }

  for (const auto& source : artifacts) {
    const std::filesystem::path destination = bin_dir / source.filename();
    if (core::Normalize(source.generic_string()) ==
        core::Normalize(destination.generic_string())) {
      continue;
    }

    ec.clear();
    std::filesystem::copy_file(source, destination,
                               std::filesystem::copy_options::overwrite_existing,
                               ec);
    if (ec) {
      EmitInternalDiagnostic(diags,
                             "Failed to stage imported shared library `" +
                                 source.generic_string() + "` into `" +
                                 destination.generic_string() + "`");
      return false;
    }
  }

  return true;
}

void EmitUnsupportedArtifactDiagnostic(
    core::DiagnosticStream& diags,
    const Project& project,
    TargetProfile target_profile,
    std::string_view artifact,
    std::string_view detail,
    std::string_view supported_profile = "x86_64-win64") {
  auto diag = core::MakeExternalDiagnostic("E-OUT-0409");
  if (!diag.has_value()) {
    return;
  }

  core::SubDiagnostic requested_note;
  requested_note.kind = core::SubDiagnosticKind::Note;
  requested_note.message =
      "requested " + std::string(artifact) + " for target profile `" +
      std::string(TargetProfileName(target_profile)) + "`";
  diag->children.push_back(std::move(requested_note));

  core::SubDiagnostic detail_note;
  detail_note.kind = core::SubDiagnosticKind::Note;
  detail_note.message = std::string(detail);
  diag->children.push_back(std::move(detail_note));

  if (!supported_profile.empty()) {
    core::SubDiagnostic support_note;
    support_note.kind = core::SubDiagnosticKind::Note;
    support_note.message =
        "currently supported target profile: `" +
        std::string(supported_profile) + "`";
    diag->children.push_back(std::move(support_note));
  }

  core::Emit(diags, *diag);
}

LinkPlan BaseLinkPlan(const Project& project, TargetProfile target_profile) {
  LinkPlan plan;
  plan.target_profile = target_profile;
  if (const auto link_mode = LinkMode(project); link_mode.has_value()) {
    plan.output_kind = *link_mode;
  }
  if (plan.output_kind == LinkOutputKind::SharedLibrary) {
    if (ObjectFormatOf(target_profile) == ObjectFormat::Coff) {
      plan.shared_library_lifecycle_mode =
          SharedLibraryLifecycleMode::WindowsEntry;
      plan.entry_symbol = "__cursive_library_entry";
    } else {
      plan.shared_library_lifecycle_mode =
          SharedLibraryLifecycleMode::PosixCtorDtor;
      plan.entry_symbol.reset();
    }
  }
  return plan;
}

void NormalizeSharedLibraryExports(SharedLibraryExports& exports) {
  auto normalize = [](std::vector<std::string>& symbols) {
    std::stable_sort(symbols.begin(), symbols.end(),
                     [](const std::string& lhs, const std::string& rhs) {
                       return Utf8LexLess(lhs, rhs);
                     });
    symbols.erase(std::unique(symbols.begin(), symbols.end()), symbols.end());
  };

  exports.export_symbols.erase(
      std::remove_if(exports.export_symbols.begin(), exports.export_symbols.end(),
                     [](const std::string& symbol) {
                       return IsHiddenSharedLibraryExportSymbol(symbol);
                     }),
      exports.export_symbols.end());
  normalize(exports.export_symbols);
  normalize(exports.data_export_symbols);
}

bool IncrementalEnabled() {
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

void HashMixByte(std::uint64_t& hash, std::uint8_t byte) {
  hash ^= static_cast<std::uint64_t>(byte);
  hash *= core::kFNVPrime64;
}

void HashMixString(std::uint64_t& hash, std::string_view value) {
  for (const unsigned char ch : value) {
    HashMixByte(hash, static_cast<std::uint8_t>(ch));
  }
  HashMixByte(hash, 0xFFU);
}

std::string HashFields(const std::vector<std::string>& fields) {
  std::uint64_t hash = core::kFNVOffset64;
  for (const auto& field : fields) {
    HashMixString(hash, field);
  }
  return core::Hex64(hash);
}

std::string HashBytes(std::string_view bytes) {
  return core::Hex64(core::FNV1a64(bytes));
}

std::optional<std::string> HashFileBytes(const std::filesystem::path& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    return std::nullopt;
  }
  std::string buffer((std::istreambuf_iterator<char>(in)),
                     std::istreambuf_iterator<char>());
  if (!in && !in.eof()) {
    return std::nullopt;
  }
  return HashBytes(buffer);
}

std::string LinkInputFingerprintField(const std::filesystem::path& path) {
  const std::string normalized = core::Normalize(path.generic_string());
  const auto hash = HashFileBytes(path);
  if (!hash.has_value()) {
    return normalized + ":missing";
  }
  return normalized + ":" + *hash;
}

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

std::string JoinByChar(const std::vector<std::string>& values, char sep) {
  std::ostringstream oss;
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i > 0) {
      oss << sep;
    }
    oss << values[i];
  }
  return oss.str();
}

struct IncrementalManifestModuleState {
  IncrementalModuleInfo info;
  std::string obj_hash;
  std::string ir_hash;
};

struct IncrementalManifestState {
  std::string format = "1";
  std::string assembly;
  std::string build_key;
  std::string emit_ir;
  std::string kind;
  std::string link_kind;
  std::string link_fingerprint;
  std::unordered_map<std::string, IncrementalManifestModuleState> modules;
};

std::filesystem::path IncrementalDirPath(const Project& project) {
  return project.outputs.root / ".cursive-incremental";
}

std::filesystem::path IncrementalManifestPath(const Project& project) {
  return IncrementalDirPath(project) / (project.assembly.name + ".manifest");
}

bool FileExists(const std::filesystem::path& path) {
  std::error_code ec;
  return std::filesystem::exists(path, ec) && !ec;
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
    const auto fields = SplitByChar(line, '	');
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
      } else if (fields[1] == "link_kind") {
        state.link_kind = fields[2];
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

bool SaveIncrementalManifest(const Project& project,
                             const OutputPipelineDeps& deps,
                             const IncrementalManifestState& state) {
  const auto dir = IncrementalDirPath(project);
  if (!deps.ensure_dir(dir)) {
    return false;
  }

  std::ostringstream out;
  out << "H	format	" << state.format << "\n";
  out << "H	assembly	" << state.assembly << "\n";
  out << "H	build_key	" << state.build_key << "\n";
  out << "H	emit_ir	" << state.emit_ir << "\n";
  out << "H	kind	" << state.kind << "\n";
  out << "H	link_kind	" << state.link_kind << "\n";
  out << "H	link	" << state.link_fingerprint << "\n";

  for (const auto& module : project.modules) {
    const auto it = state.modules.find(module.path);
    if (it == state.modules.end()) {
      continue;
    }
    const auto& mod = it->second;
    out << "M	" << module.path << "	" << mod.info.source_hash << "	"
        << mod.info.public_hash << "	" << mod.info.full_hash << "	"
        << mod.obj_hash << "	" << mod.ir_hash << "	"
        << JoinByChar(mod.info.dependencies, ',') << "\n";
  }

  const auto manifest = IncrementalManifestPath(project);
  return deps.write_file(manifest, out.str());
}

std::string ComputeLinkFingerprint(
    const Project& project,
    TargetProfile target_profile,
    const std::string& build_key,
    const std::unordered_map<std::string, IncrementalManifestModuleState>& modules,
    const std::vector<std::filesystem::path>& link_inputs,
    const std::optional<std::filesystem::path>& runtime_lib,
    const LinkPlan& plan,
    std::string_view emit_ir) {
  std::vector<std::string> fields;
  fields.reserve(10 + project.modules.size() + link_inputs.size() +
                 plan.export_symbols.size() + plan.data_export_symbols.size());
  fields.push_back("v6");
  fields.push_back(build_key);
  fields.push_back("target=" + std::string(TargetProfileName(plan.target_profile)));
  fields.push_back(project.assembly.name);
  fields.push_back(project.assembly.kind);
  fields.push_back(project.assembly.link_kind.value_or("none"));
  fields.push_back(std::string(emit_ir));
  if (IsStaticLibrary(project)) {
    fields.push_back("output=archive");
  } else {
    fields.push_back(plan.output_kind == LinkOutputKind::SharedLibrary
                         ? "output=shared"
                         : "output=exe");
  }
  switch (plan.shared_library_lifecycle_mode) {
    case SharedLibraryLifecycleMode::None:
      fields.push_back("lifecycle=none");
      break;
    case SharedLibraryLifecycleMode::WindowsEntry:
      fields.push_back("lifecycle=windows-entry");
      break;
    case SharedLibraryLifecycleMode::PosixCtorDtor:
      fields.push_back("lifecycle=posix-ctor-dtor");
      break;
  }
  fields.push_back("entry=" + plan.entry_symbol.value_or(""));
  std::vector<std::string> exports = plan.export_symbols;
  std::sort(exports.begin(), exports.end());
  exports.erase(std::unique(exports.begin(), exports.end()), exports.end());
  for (const auto& symbol : exports) {
    fields.push_back("export=" + symbol);
  }
  std::vector<std::string> data_exports = plan.data_export_symbols;
  std::sort(data_exports.begin(), data_exports.end());
  data_exports.erase(std::unique(data_exports.begin(), data_exports.end()),
                     data_exports.end());
  for (const auto& symbol : data_exports) {
    fields.push_back("export-data=" + symbol);
  }
  if (runtime_lib.has_value()) {
    fields.push_back("runtime=" + LinkInputFingerprintField(*runtime_lib));
  }
  if (const auto map_path = MapPath(project, target_profile);
      map_path.has_value()) {
    fields.push_back("map=windows-sidecar");
    fields.push_back("map_path=" + core::Normalize(map_path->generic_string()));
  }
  const auto materialized_link_inputs =
      MaterializeLinkInputsForTool(project, target_profile, link_inputs);
  for (const auto& input : materialized_link_inputs) {
    fields.push_back("input=" + LinkInputFingerprintField(input));
  }

  for (const auto& module : project.modules) {
    auto it = modules.find(module.path);
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

}  // namespace

OutputPaths ComputeOutputPaths(const std::filesystem::path& project_root,
                               const ValidatedAssembly& assembly) {
  const auto cli_out_dir = core::OutDirOverride();
  const std::filesystem::path root =
      cli_out_dir.has_value()      ? (project_root / *cli_out_dir)
      : assembly.out_dir.has_value() ? (project_root / *assembly.out_dir)
                                     : (project_root / "build");
  OutputPaths paths;
  paths.root = root;
  paths.obj_dir = root / "obj";
  paths.ir_dir = root / "ir";
  paths.bin_dir = root / "bin";
  paths.lib_dir = root / "lib";
  return paths;
}

std::filesystem::path ObjPath(const Project& project,
                              TargetProfile target_profile,
                              const ModuleInfo& module) {
  const std::string mangled = core::MangleModulePath(module.path);
  return project.outputs.obj_dir /
         (mangled + std::string(ObjExt(target_profile)));
}

std::filesystem::path IRPath(const Project& project,
                             TargetProfile,
                             const ModuleInfo& module,
                             std::string_view emit_ir) {
  std::string ext = ".ll";
  if (emit_ir == "bc") {
    ext = ".bc";
  }
  const std::string mangled = core::MangleModulePath(module.path);
  return project.outputs.ir_dir / (mangled + ext);
}

std::filesystem::path ExePath(const Project& project,
                              TargetProfile target_profile) {
  return project.outputs.bin_dir /
         (project.assembly.name + std::string(ExeSuffix(target_profile)));
}

std::filesystem::path SharedLibPath(const Project& project,
                                    TargetProfile target_profile) {
  return project.outputs.bin_dir /
         (std::string(LibraryPrefix(target_profile)) +
          project.assembly.name +
          std::string(SharedLibSuffix(target_profile)));
}

std::filesystem::path StaticLibPath(const Project& project,
                                    TargetProfile target_profile) {
  return project.outputs.lib_dir /
         (std::string(LibraryPrefix(target_profile)) +
          project.assembly.name +
          std::string(StaticLibSuffix(target_profile)));
}

std::optional<std::filesystem::path> ImportLibPath(const Project& project,
                                                   TargetProfile target_profile) {
  if (!IsSharedLibrary(project) || !EmitsImportLib(target_profile)) {
    return std::nullopt;
  }
  return project.outputs.lib_dir /
         (std::string(LibraryPrefix(target_profile)) +
          project.assembly.name +
          std::string(ImportLibSuffix(target_profile)));
}

std::optional<std::filesystem::path> MapPath(const Project& project,
                                             TargetProfile target_profile) {
  if (ObjectFormatOf(target_profile) != ObjectFormat::Coff) {
    return std::nullopt;
  }
  if (!(IsExecutable(project) || IsSharedLibrary(project))) {
    return std::nullopt;
  }
  const auto primary = PrimaryArtifactPath(project, target_profile);
  if (!primary.has_value()) {
    return std::nullopt;
  }
  auto map_path = *primary;
  map_path.replace_extension(".map");
  return map_path;
}

std::optional<std::filesystem::path> PrimaryArtifactPath(
    const Project& project,
    TargetProfile target_profile) {
  if (IsExecutable(project)) {
    return ExePath(project, target_profile);
  }
  if (IsSharedLibrary(project)) {
    return SharedLibPath(project, target_profile);
  }
  if (IsStaticLibrary(project)) {
    return StaticLibPath(project, target_profile);
  }
  return std::nullopt;
}

std::vector<std::filesystem::path> LibraryArtifactInputs(
    const std::vector<std::filesystem::path>& inputs) {
  return inputs;
}

std::optional<LinkOutputKind> LinkMode(const Project& project) {
  if (IsExecutable(project)) {
    return LinkOutputKind::Executable;
  }
  if (IsSharedLibrary(project)) {
    return LinkOutputKind::SharedLibrary;
  }
  return std::nullopt;
}

std::optional<std::filesystem::path> LinkOutputPath(
    const Project& project,
    TargetProfile target_profile) {
  if (IsExecutable(project)) {
    return ExePath(project, target_profile);
  }
  if (IsSharedLibrary(project)) {
    return SharedLibPath(project, target_profile);
  }
  return std::nullopt;
}

bool UsesBinDir(const Project& project, TargetProfile) {
  return IsExecutable(project) || IsSharedLibrary(project);
}

bool UsesLibDir(const Project& project, TargetProfile target_profile) {
  return IsStaticLibrary(project) ||
         ImportLibPath(project, target_profile).has_value();
}

std::vector<std::filesystem::path> ObjPaths(
    const Project& project,
    TargetProfile target_profile,
    const std::vector<ModuleInfo>& modules) {
  std::vector<std::filesystem::path> out;
  out.reserve(modules.size());
  for (const auto& module : modules) {
    out.push_back(ObjPath(project, target_profile, module));
  }
  return out;
}

std::vector<std::filesystem::path> IRPaths(
    const Project& project,
    TargetProfile target_profile,
    const std::vector<ModuleInfo>& modules,
    std::string_view emit_ir) {
  std::vector<std::filesystem::path> out;
  if (!(emit_ir == "ll" || emit_ir == "bc")) {
    return out;
  }
  out.reserve(modules.size());
  for (const auto& module : modules) {
    out.push_back(IRPath(project, target_profile, module, emit_ir));
  }
  return out;
}

std::vector<std::filesystem::path> RequiredOutputs(const Project& project,
                                                   TargetProfile target_profile) {
  std::vector<std::filesystem::path> out;
  const auto objs = ObjPaths(project, target_profile, project.modules);
  out.insert(out.end(), objs.begin(), objs.end());
  const std::string_view emit_ir = EmitIrMode(project);
  if (emit_ir == "ll" || emit_ir == "bc") {
    const auto irs = IRPaths(project, target_profile, project.modules, emit_ir);
    out.insert(out.end(), irs.begin(), irs.end());
  }
  if (const auto primary = PrimaryArtifactPath(project, target_profile);
      primary.has_value()) {
    out.push_back(*primary);
  }
  if (const auto import_lib = ImportLibPath(project, target_profile);
      import_lib.has_value()) {
    out.push_back(*import_lib);
  }
  return out;
}

bool OutputHygiene(const Project& project, TargetProfile target_profile) {
  const auto required = RequiredOutputs(project, target_profile);
  for (const auto& path : required) {
    if (!UnderPath(path, project.outputs.root)) {
      return false;
    }
  }
  return true;
}

std::vector<std::string> DumpProject(const Project& project,
                                     TargetProfile target_profile,
                                     bool dump_files) {
  std::vector<std::string> out;
  out.reserve(9 + project.modules.size());

  auto render_opt = [](const std::optional<std::string>& value) -> std::string {
    return value.has_value() ? *value : "<bottom>";
  };

  auto render_opt_path =
      [](const std::optional<std::filesystem::path>& value) -> std::string {
    return value.has_value() ? value->generic_string() : "<bottom>";
  };

  std::vector<std::string> assembly_names;
  assembly_names.reserve(project.assemblies.size());
  for (const auto& assembly : project.assemblies) {
    assembly_names.push_back(assembly.name);
  }

  std::vector<std::string> module_names;
  module_names.reserve(project.modules.size());
  for (const auto& module : project.modules) {
    module_names.push_back(module.path);
  }

  out.push_back("<project_root, " + project.root.generic_string() + ">");
  out.push_back("<assemblies, " + RenderList(assembly_names) + ">");
  out.push_back("<assembly_name, " + project.assembly.name + ">");
  out.push_back("<assembly_kind, " + project.assembly.kind + ">");
  out.push_back("<link_kind, " + render_opt(project.assembly.link_kind) + ">");
  out.push_back("<source_root, " + project.source_root.generic_string() + ">");
  out.push_back("<output_root, " + project.outputs.root.generic_string() + ">");
  out.push_back("<module_list, " + RenderList(module_names) + ">");

  if (IsExecutable(project) || IsLibrary(project)) {
    const auto primary = PrimaryArtifactPath(project, target_profile);
    const auto import_lib = ImportLibPath(project, target_profile);
    out.push_back("<artifact, " + render_opt_path(primary) + ", import_lib, " +
                  render_opt_path(import_lib) + ">");
  }

  const bool emit_ir_enabled =
      project.assembly.emit_ir.has_value() && *project.assembly.emit_ir != "none";
  const std::string_view emit_ir = emit_ir_enabled ? *project.assembly.emit_ir : "";
  for (const auto& module : project.modules) {
    const std::string obj_path =
        ObjPath(project, target_profile, module).generic_string();
    const std::string ir_value =
        emit_ir_enabled
            ? IRPath(project, target_profile, module, emit_ir).generic_string()
                        : "<bottom>";
    out.push_back("<module, " + module.path + ", obj, " + obj_path + ", ir, " +
                  ir_value + ">");
  }

  if (dump_files) {
    for (const auto& module : project.modules) {
      const auto unit = CompilationUnit(module.dir);
      for (const auto& file : unit.files) {
        std::string line = "file:";
        line.append(file.generic_string());
        out.push_back(std::move(line));
      }
    }
  }

  return out;
}

struct ParsedAstModulesResult {
  std::optional<std::vector<ast::ASTModule>> modules;
  core::DiagnosticStream diags;
};

ParsedAstModulesResult ParseAstModulesForProject(const Project& project,
                                                 const OutputPipelineDeps& deps) {
  if (deps.resolve_ast_modules) {
    auto parsed = deps.resolve_ast_modules(project);
    return {std::move(parsed.modules), std::move(parsed.diags)};
  }

  auto parsed =
      frontend::ParseModules(project.modules, project.source_root, project.assembly.name);
  if (!parsed.modules.has_value() || core::HasError(parsed.diags)) {
    return {std::move(parsed.modules), std::move(parsed.diags)};
  }

  return {std::move(parsed.modules), std::move(parsed.diags)};
}

Project AssemblyProject(const Project& base_project, const Assembly& assembly) {
  Project project = base_project;
  project.assembly = assembly;
  project.source_root = assembly.source_root;
  project.outputs = assembly.outputs;
  project.modules = assembly.modules;
  return project;
}

frontend::ComptimePassOptions BuildComptimeOptions(const Project& project) {
  frontend::ComptimePassOptions options;
  options.project_root = project.root;
  options.fallback_source_root = project.source_root;
  options.source_roots_by_assembly.reserve(project.assemblies.size());
  for (const auto& assembly : project.assemblies) {
    options.source_roots_by_assembly[assembly.name] = assembly.source_root;
  }
  return options;
}

LinkPlan BuildOutputLinkPlan(const Project& project,
                             TargetProfile target_profile,
                             const std::vector<ast::ASTModule>& ast_modules,
                             const OutputPipelineDeps& deps,
                             core::DiagnosticStream& diags) {
  LinkPlan plan = BaseLinkPlan(project, target_profile);
  const bool hosted_library = ContainsHostExports(ast_modules);

  if (hosted_library && !IsSharedLibrary(project)) {
    EmitUnsupportedArtifactDiagnostic(
        diags, project, target_profile, "hosted library",
        "`[[host_export]]` requires a shared-library final artifact");
    return plan;
  }

  if (IsSharedLibrary(project) &&
      !SupportsSharedLibraries(target_profile)) {
    EmitUnsupportedArtifactDiagnostic(
        diags, project, target_profile, "shared library",
        "shared-library outputs are not yet implemented for this target profile");
    return plan;
  }

  if (hosted_library &&
      !SupportsHostedLibraries(target_profile)) {
    EmitUnsupportedArtifactDiagnostic(
        diags, project, target_profile, "hosted library",
        "hosted-library lifecycle exports and session runtime are not yet implemented for this target profile");
    return plan;
  }

  if (!IsSharedLibrary(project)) {
    return plan;
  }
  if (!deps.resolve_shared_library_exports) {
    core::Diagnostic diag;
    diag.severity = core::Severity::Error;
    diag.message = "shared-library export resolver missing";
    core::Emit(diags, diag);
    return plan;
  }

  const auto exports = deps.resolve_shared_library_exports(project);
  if (!exports.has_value()) {
    core::Diagnostic diag;
    diag.severity = core::Severity::Error;
    diag.message = "shared-library export resolution failed";
    core::Emit(diags, diag);
    return plan;
  }

  auto normalized_exports = *exports;
  NormalizeSharedLibraryExports(normalized_exports);

  if (!deps.prepare_codegen_context) {
    core::Diagnostic diag;
    diag.severity = core::Severity::Error;
    diag.message = "shared-library codegen preparation hook missing";
    core::Emit(diags, diag);
    return plan;
  }
  if (!deps.prepare_codegen_context(project, normalized_exports)) {
    core::Diagnostic diag;
    diag.severity = core::Severity::Error;
    diag.message = "shared-library codegen preparation failed";
    core::Emit(diags, diag);
  }

  plan.export_symbols = normalized_exports.export_symbols;
  plan.data_export_symbols = normalized_exports.data_export_symbols;

  return plan;
}

std::vector<std::filesystem::path> AppendUniquePaths(
    std::vector<std::filesystem::path> paths,
    const std::vector<std::filesystem::path>& extras) {
  std::unordered_set<std::string> seen;
  seen.reserve(paths.size() + extras.size());
  for (const auto& path : paths) {
    seen.insert(path.generic_string());
  }
  for (const auto& extra : extras) {
    if (!seen.insert(extra.generic_string()).second) {
      continue;
    }
    paths.push_back(extra);
  }
  return paths;
}

struct OutputCoordinatorState {
  TargetProfile target_profile;
  const OutputPipelineDeps& deps;
  const std::vector<ast::ASTModule>* project_ast_modules = nullptr;
  std::optional<std::vector<ast::ASTModule>> owned_project_ast_modules;
  std::unordered_map<std::string, std::vector<ast::ASTModule>> ast_by_assembly;
  std::unordered_map<std::string, OutputArtifacts> built_artifacts;
  std::optional<AssemblyImportGraph> graph;
};

struct ArtifactBuildPlan {
  std::vector<std::string> assembly_order;
};

OutputPipelineResult OutputPipelineSingleAssembly(
    const Project& project,
    TargetProfile target_profile,
    const OutputPipelineDeps& deps,
    const std::vector<std::filesystem::path>& extra_link_inputs,
    const LinkPlan& link_plan);

std::optional<Project> AssemblyProjectView(const Project& base_project,
                                           std::string_view assembly_name) {
  for (const auto& assembly : base_project.assemblies) {
    if (assembly.name != assembly_name) {
      continue;
    }
    return AssemblyProject(base_project, assembly);
  }
  return std::nullopt;
}

std::vector<std::string> ReverseDependentAssemblies(
    std::string_view assembly_name,
    const AssemblyImportGraph& graph) {
  std::unordered_map<std::string, std::vector<std::string>> reverse_imports;
  reverse_imports.reserve(graph.imports.size());
  for (const auto& [importer_name, deps] : graph.imports) {
    for (const auto& dep_name : deps) {
      reverse_imports[dep_name].push_back(importer_name);
    }
  }
  for (auto& [_, importers] : reverse_imports) {
    std::stable_sort(importers.begin(), importers.end(),
                     [](const std::string& lhs, const std::string& rhs) {
                       return Utf8LexLess(lhs, rhs);
                     });
    importers.erase(std::unique(importers.begin(), importers.end()),
                    importers.end());
  }

  std::vector<std::string> discovered;
  std::unordered_set<std::string> seen;
  std::vector<std::string> pending = {std::string(assembly_name)};
  seen.insert(std::string(assembly_name));

  for (std::size_t i = 0; i < pending.size(); ++i) {
    const auto reverse_it = reverse_imports.find(pending[i]);
    if (reverse_it == reverse_imports.end()) {
      continue;
    }
    for (const auto& importer_name : reverse_it->second) {
      if (!seen.insert(importer_name).second) {
        continue;
      }
      discovered.push_back(importer_name);
      pending.push_back(importer_name);
    }
  }

  std::stable_sort(discovered.begin(), discovered.end(),
                   [](const std::string& lhs, const std::string& rhs) {
                     return Utf8LexLess(lhs, rhs);
                   });
  discovered.erase(std::unique(discovered.begin(), discovered.end()),
                   discovered.end());
  return discovered;
}

bool RestageSharedLibraryForExistingConsumers(
    const Project& root_project,
    const AssemblyImportGraph& graph,
    const Project& provider_project,
    const OutputArtifacts& provider_artifacts,
    TargetProfile target_profile,
    core::DiagnosticStream& diags) {
  if (!IsSharedLibrary(provider_project) ||
      !provider_artifacts.primary_artifact.has_value()) {
    return true;
  }

  const auto reverse_dependents =
      ReverseDependentAssemblies(provider_project.assembly.name, graph);
  if (reverse_dependents.empty()) {
    return true;
  }

  const std::vector<std::filesystem::path> provider_runtime_artifacts = {
      *provider_artifacts.primary_artifact};
  for (const auto& consumer_name : reverse_dependents) {
    const auto consumer_project =
        AssemblyProjectView(root_project, consumer_name);
    if (!consumer_project.has_value() ||
        !IsLinkable(consumer_project->assembly) ||
        !UsesBinDir(*consumer_project, target_profile)) {
      continue;
    }

    std::error_code ec;
    if (!std::filesystem::exists(consumer_project->outputs.bin_dir, ec) || ec) {
      continue;
    }

    if (!CopyImportedSharedLibraryArtifacts(consumer_project->outputs.bin_dir,
                                            provider_runtime_artifacts,
                                            target_profile,
                                            diags)) {
      EmitInternalDiagnostic(
          diags,
          "Failed to restage shared library `" +
              provider_artifacts.primary_artifact->generic_string() + "` into `" +
              consumer_project->outputs.bin_dir.generic_string() + "`");
      return false;
    }
  }

  return true;
}

bool EnsureProjectAstModulesLoaded(const Project& base_project,
                                   OutputCoordinatorState& state,
                                   core::DiagnosticStream& diags) {
  if (state.project_ast_modules != nullptr) {
    return true;
  }

  if (state.deps.resolve_project_ast_modules) {
    auto full_modules = state.deps.resolve_project_ast_modules(base_project);
    if (full_modules.has_value()) {
      state.project_ast_modules = &full_modules->get();
      return true;
    }
  }

  std::vector<ast::ASTModule> phase1_modules;
  for (const auto& assembly : base_project.assemblies) {
    const Project assembly_project = AssemblyProject(base_project, assembly);
    auto parsed = ParseAstModulesForProject(assembly_project, state.deps);
    for (const auto& diag : parsed.diags) {
      core::Emit(diags, diag);
    }
    if (!parsed.modules.has_value() || core::HasError(parsed.diags)) {
      return false;
    }
    phase1_modules.insert(phase1_modules.end(),
                          std::make_move_iterator(parsed.modules->begin()),
                          std::make_move_iterator(parsed.modules->end()));
  }

  auto expanded =
      frontend::ExecuteComptime(phase1_modules, BuildComptimeOptions(base_project));
  for (const auto& diag : expanded.diags) {
    core::Emit(diags, diag);
  }
  if (!expanded.modules.has_value() || core::HasError(expanded.diags)) {
    return false;
  }

  state.owned_project_ast_modules = std::move(*expanded.modules);
  state.project_ast_modules = &*state.owned_project_ast_modules;
  return true;
}

bool LoadAstForAssembly(const Project& base_project,
                        std::string_view assembly_name,
                        OutputCoordinatorState& state,
                        core::DiagnosticStream& diags) {
  if (state.ast_by_assembly.find(std::string(assembly_name)) !=
      state.ast_by_assembly.end()) {
    return true;
  }

  const auto assembly_project = AssemblyProjectView(base_project, assembly_name);
  if (!assembly_project.has_value()) {
    return false;
  }
  if (!EnsureProjectAstModulesLoaded(base_project, state, diags)) {
    return false;
  }

  const auto filtered =
      FilterAstModulesForProject(*assembly_project, *state.project_ast_modules, diags);
  if (!filtered.has_value()) {
    return false;
  }
  state.ast_by_assembly.emplace(std::string(assembly_name), *filtered);
  return true;
}

LinkResult FinalizeArtifacts(
    const Project& project,
    TargetProfile target_profile,
    const std::vector<std::filesystem::path>& objs,
    const std::vector<std::filesystem::path>& extra_link_inputs,
    const LinkPlan& link_plan,
    const OutputPipelineDeps& deps) {
  const LinkDeps link_deps = {
      deps.resolve_tool,
      deps.resolve_runtime_lib,
      deps.linker_syms,
      deps.archive_members,
      deps.invoke_linker,
      deps.invoke_archiver,
  };

  if (IsStaticLibrary(project)) {
    LinkResult result = Archive(objs, project, target_profile, link_deps);
    if (result.status == LinkStatus::Ok) {
      SPEC_RULE("Finalize-Archive");
    }
    return result;
  }

  LinkResult result = Link(objs, extra_link_inputs, project, link_plan, link_deps);
  if (result.status == LinkStatus::Ok) {
    SPEC_RULE("Finalize-Link");
  }
  return result;
}

bool EnsureAssemblyGraphLoaded(const Project& project,
                              OutputCoordinatorState& state,
                              core::DiagnosticStream& diags) {
  if (state.graph.has_value()) {
    return true;
  }

  if (state.project_ast_modules == nullptr &&
      state.deps.resolve_project_ast_modules) {
    auto full_modules = state.deps.resolve_project_ast_modules(project);
    if (full_modules.has_value()) {
      state.project_ast_modules = &full_modules->get();
    }
  }

  if (state.project_ast_modules == nullptr) {
    std::vector<ast::ASTModule> all_modules;
    for (const auto& assembly : project.assemblies) {
      if (!LoadAstForAssembly(project, assembly.name, state, diags)) {
        return false;
      }
      const auto ast_it = state.ast_by_assembly.find(assembly.name);
      if (ast_it != state.ast_by_assembly.end()) {
        all_modules.insert(all_modules.end(), ast_it->second.begin(),
                           ast_it->second.end());
      }
    }
    state.owned_project_ast_modules = std::move(all_modules);
    state.project_ast_modules = &*state.owned_project_ast_modules;
  }

  state.graph = BuildAssemblyImportGraph(project, *state.project_ast_modules);
  if (!ValidateAssemblyImportGraphStructure(project, *state.graph, diags)) {
    return false;
  }
  if (!ValidateHostedLibraryImportGraph(project, *state.graph, *state.project_ast_modules,
                                        diags)) {
    return false;
  }
  return true;
}

ArtifactBuildPlan BuildArtifactBuildPlan(
    const Project& project,
    const AssemblyImportGraph& graph) {
  ArtifactBuildPlan plan;
  const auto libraries = ImportedLibraries(project.assembly.name, graph);
  plan.assembly_order.reserve(libraries.size() + 1);
  for (const auto& assembly_name : libraries) {
    plan.assembly_order.push_back(assembly_name);
  }
  plan.assembly_order.push_back(project.assembly.name);
  return plan;
}

OutputPipelineResult OutputPipelineSingleAssembly(
    const Project& project,
    TargetProfile target_profile,
    const OutputPipelineDeps& deps,
    const std::vector<std::filesystem::path>& extra_link_inputs,
    const LinkPlan& link_plan) {
  OutputPipelineResult result;

  SPEC_RULE("Out-Start");

  const std::string_view emit_ir = EmitIrMode(project);
  const bool executable = IsExecutable(project);
  const bool dependency = IsDependency(project);
  const bool shared_library = IsSharedLibrary(project);
  const bool static_library = IsStaticLibrary(project);
  const bool linkable = !dependency;
  const core::BuildLogMode output_log_mode = ResolveOutputLogMode();
  const bool show_build_progress = output_log_mode != core::BuildLogMode::None;
  const bool show_detailed_progress =
      output_log_mode == core::BuildLogMode::Detailed;

  if (show_build_progress) {
    std::cerr << "   Compiling  " << project.assembly.name << " ("
              << project.modules.size() << " modules)\n";
    std::cerr.flush();
    std::ostringstream oss;
    oss << "start assembly=" << project.assembly.name
        << " kind=" << project.assembly.kind
        << " modules=" << project.modules.size()
        << " emit_ir=" << emit_ir;
    LogBuildProgress(oss.str());
  }

  if (!deps.ensure_dir(project.outputs.root) ||
      !deps.ensure_dir(project.outputs.obj_dir) ||
      (UsesBinDir(project, target_profile) &&
       !deps.ensure_dir(project.outputs.bin_dir)) ||
      (UsesLibDir(project, target_profile) &&
       !deps.ensure_dir(project.outputs.lib_dir)) ||
      ((emit_ir == "ll" || emit_ir == "bc") &&
       !deps.ensure_dir(project.outputs.ir_dir))) {
    if (show_build_progress) {
      LogBuildProgress("dirs-error");
    }
    SPEC_RULE("Out-Dirs-Err");
    EmitExternal(result.diags, "E-OUT-0401");
    SPEC_RULE("Output-Pipeline-Err");
    return result;
  }
  SPEC_RULE("Out-Dirs-Ok");

  if (show_build_progress) {
    std::ostringstream oss;
    oss << "dirs-ok root=" << project.outputs.root.generic_string()
        << " obj=" << project.outputs.obj_dir.generic_string();
    if (UsesBinDir(project, target_profile)) {
      oss << " bin=" << project.outputs.bin_dir.generic_string();
    }
    if (UsesLibDir(project, target_profile)) {
      oss << " lib=" << project.outputs.lib_dir.generic_string();
    }
    if (emit_ir == "ll" || emit_ir == "bc") {
      oss << " ir=" << project.outputs.ir_dir.generic_string();
    }
    LogBuildProgress(oss.str());
  }

  if (deps.prepare_codegen_context) {
    SharedLibraryExports exports;
    exports.export_symbols = link_plan.export_symbols;
    exports.data_export_symbols = link_plan.data_export_symbols;
    if (!deps.prepare_codegen_context(project, exports)) {
      if (show_build_progress) {
        LogBuildProgress("codegen-context-error");
      }
      core::Diagnostic diag;
      diag.severity = core::Severity::Error;
      diag.message = "project codegen context preparation failed";
      core::Emit(result.diags, diag);
      return result;
    }
  }

  const bool has_incremental_callbacks =
      static_cast<bool>(deps.incremental_module) &&
      static_cast<bool>(deps.incremental_build_key);
  bool incremental_enabled = IncrementalEnabled() && has_incremental_callbacks;
  std::optional<std::string> incremental_build_key;
  IncrementalManifestState previous_manifest;
  bool have_previous_manifest = false;
  bool manifest_compatible = false;
  IncrementalManifestState next_manifest;

  if (incremental_enabled) {
    incremental_build_key = deps.incremental_build_key(project);
    if (!incremental_build_key.has_value() || incremental_build_key->empty()) {
      incremental_enabled = false;
      if (show_build_progress) {
        LogBuildProgress("incremental-disabled reason=missing-build-key");
      }
    }
  }

  if (incremental_enabled) {
    const auto manifest_path = IncrementalManifestPath(project);
    if (const auto loaded = LoadIncrementalManifest(manifest_path)) {
      previous_manifest = *loaded;
      have_previous_manifest = true;
    }

    next_manifest.format = "1";
    next_manifest.assembly = project.assembly.name;
    next_manifest.build_key = *incremental_build_key;
    next_manifest.emit_ir = std::string(emit_ir);
    next_manifest.kind = project.assembly.kind;
    next_manifest.link_kind = project.assembly.link_kind.value_or("none");

    manifest_compatible =
        have_previous_manifest && previous_manifest.format == "1" &&
        previous_manifest.assembly == next_manifest.assembly &&
        previous_manifest.build_key == next_manifest.build_key &&
        previous_manifest.emit_ir == next_manifest.emit_ir &&
        previous_manifest.kind == next_manifest.kind &&
        previous_manifest.link_kind == next_manifest.link_kind;

    if (show_build_progress) {
      std::ostringstream oss;
      oss << "incremental mode=on manifest="
          << (have_previous_manifest ? "loaded" : "new")
          << " compatible=" << (manifest_compatible ? "true" : "false");
      LogBuildProgress(oss.str());
    }
  }

  const auto obj_paths = ObjPaths(project, target_profile, project.modules);
  if (!IsDistinct(obj_paths)) {
    if (show_build_progress) {
      LogBuildProgress("obj-collision");
    }
    SPEC_RULE("Out-Obj-Collision");
    EmitExternal(result.diags, "E-OUT-0406");
    SPEC_RULE("Output-Pipeline-Err");
    return result;
  }

  bool any_obj_rebuilt = false;
  bool any_ir_rebuilt = false;
  std::vector<std::filesystem::path> objs;
  objs.reserve(project.modules.size());
  struct ObjEmitState {
    std::filesystem::path obj_path;
    std::optional<IncrementalModuleInfo> module_info;
    bool reused = false;
    std::string reused_hash;
    std::optional<std::string> bytes;
    bool codegen_failed = false;
  };
  std::vector<ObjEmitState> obj_states(project.modules.size());
  std::vector<std::size_t> obj_codegen_indices;
  obj_codegen_indices.reserve(project.modules.size());
  const std::size_t obj_total = project.modules.size();
  std::size_t obj_index = 0;
  std::size_t obj_reused = 0;
  std::size_t obj_rebuilt = 0;
  constexpr std::size_t kSummaryProgressStep = 8;
  auto maybe_log_obj_progress = [&]() {
    if (!show_build_progress || show_detailed_progress || obj_total == 0) {
      return;
    }
    const bool on_step = (obj_index % kSummaryProgressStep) == 0;
    if (!on_step && obj_index != obj_total) {
      return;
    }
    std::ostringstream oss;
    oss << "obj-progress index=" << obj_index << "/" << obj_total
        << " reused=" << obj_reused << " rebuilt=" << obj_rebuilt;
    LogBuildProgress(oss.str());
  };

  if (show_build_progress) {
    std::ostringstream oss;
    oss << "obj-phase-start total=" << obj_total;
    LogBuildProgress(oss.str());
  }

  for (std::size_t i = 0; i < project.modules.size(); ++i) {
    const auto& module = project.modules[i];
    auto& state = obj_states[i];
    state.obj_path = ObjPath(project, target_profile, module);

    if (incremental_enabled) {
      state.module_info = deps.incremental_module(module, project);
    }

    if (incremental_enabled && state.module_info.has_value() &&
        manifest_compatible) {
      const auto prev_it = previous_manifest.modules.find(module.path);
      if (prev_it != previous_manifest.modules.end() &&
          prev_it->second.info.full_hash == state.module_info->full_hash &&
          FileExists(state.obj_path)) {
        state.reused = true;
        state.reused_hash = prev_it->second.obj_hash;
        if (state.reused_hash.empty()) {
          const auto obj_hash = HashFileBytes(state.obj_path);
          if (obj_hash.has_value()) {
            state.reused_hash = *obj_hash;
          }
        }
      }
    }

    if (!state.reused) {
      obj_codegen_indices.push_back(i);
    }
  }

  if (deps.codegen_obj_thread_safe && obj_codegen_indices.size() > 1) {
    const std::size_t hw = std::thread::hardware_concurrency();
    const std::size_t workers = std::min<std::size_t>(
        obj_codegen_indices.size(), hw == 0 ? 4 : hw);
    std::atomic<std::size_t> next_job{0};
    std::vector<std::thread> pool;
    pool.reserve(workers);

    for (std::size_t w = 0; w < workers; ++w) {
      pool.emplace_back([&]() {
        for (;;) {
          const std::size_t slot = next_job.fetch_add(1);
          if (slot >= obj_codegen_indices.size()) {
            break;
          }
          const std::size_t module_idx = obj_codegen_indices[slot];
          const auto& module = project.modules[module_idx];
          auto bytes = deps.codegen_obj(module, project);
          if (!bytes.has_value()) {
            obj_states[module_idx].codegen_failed = true;
            continue;
          }
          obj_states[module_idx].bytes = std::move(*bytes);
        }
      });
    }
    for (auto& worker : pool) {
      worker.join();
    }
  } else {
    for (const auto module_idx : obj_codegen_indices) {
      const auto& module = project.modules[module_idx];
      auto bytes = deps.codegen_obj(module, project);
      if (!bytes.has_value()) {
        obj_states[module_idx].codegen_failed = true;
        continue;
      }
      obj_states[module_idx].bytes = std::move(*bytes);
    }
  }

  for (std::size_t i = 0; i < project.modules.size(); ++i) {
    ++obj_index;
    const auto& module = project.modules[i];
    const auto& state = obj_states[i];
    const auto& obj_path = state.obj_path;

    if (state.reused) {
      if (show_detailed_progress) {
        std::ostringstream oss;
        oss << "obj-reuse module=" << module.path
            << " index=" << obj_index << "/" << obj_total
            << " path=" << obj_path.generic_string();
        LogBuildProgress(oss.str());
      }
      SPEC_RULE("Emit-Objects-Cons");
      SPEC_RULE("Out-Obj-Cons");
      objs.push_back(obj_path);

      IncrementalManifestModuleState manifest_state;
      manifest_state.info = *state.module_info;
      manifest_state.obj_hash = state.reused_hash;
      next_manifest.modules[module.path] = std::move(manifest_state);
      ++obj_reused;
      maybe_log_obj_progress();
      continue;
    }

    if (show_detailed_progress) {
      std::ostringstream oss;
      oss << "obj-codegen module=" << module.path
          << " index=" << obj_index << "/" << obj_total
          << " path=" << obj_path.generic_string();
      LogBuildProgress(oss.str());
    }

    if (state.codegen_failed || !state.bytes.has_value()) {
      if (show_build_progress) {
        std::ostringstream oss;
        oss << "obj-error module=" << module.path;
        LogBuildProgress(oss.str());
      }
      SPEC_RULE("Out-Obj-Err");
      EmitExternal(result.diags, "E-OUT-0402");
      SPEC_RULE("Output-Pipeline-Err");
      return result;
    }

    const std::string& bytes = *state.bytes;
    SPEC_RULE("CodegenObj-LLVM");
    if (!deps.write_file(obj_path, bytes)) {
      if (show_build_progress) {
        std::ostringstream oss;
        oss << "obj-write-error module=" << module.path
            << " path=" << obj_path.generic_string();
        LogBuildProgress(oss.str());
      }
      core::HostPrimFail(core::HostPrim::WriteFile, true);
      SPEC_RULE("Out-Obj-Err");
      EmitExternal(result.diags, "E-OUT-0402");
      SPEC_RULE("Output-Pipeline-Err");
      return result;
    }
    if (show_detailed_progress) {
      std::ostringstream oss;
      oss << "obj-written module=" << module.path
          << " index=" << obj_index << "/" << obj_total
          << " path=" << obj_path.generic_string() << " bytes="
          << bytes.size();
      LogBuildProgress(oss.str());
    }
    SPEC_RULE("Emit-Objects-Cons");
    SPEC_RULE("Out-Obj-Cons");
    objs.push_back(obj_path);
    any_obj_rebuilt = true;

    if (incremental_enabled) {
      IncrementalManifestModuleState manifest_state;
      if (state.module_info.has_value()) {
        manifest_state.info = *state.module_info;
      } else {
        manifest_state.info.full_hash = HashBytes(bytes);
        manifest_state.info.source_hash = manifest_state.info.full_hash;
        manifest_state.info.public_hash = manifest_state.info.full_hash;
      }
      manifest_state.obj_hash = HashBytes(bytes);
      next_manifest.modules[module.path] = std::move(manifest_state);
    }
    ++obj_rebuilt;
    maybe_log_obj_progress();
  }

  if (project.modules.empty()) {
    SPEC_RULE("Emit-Objects-Empty");
    if (show_build_progress) {
      LogBuildProgress("obj-none");
    }
  }
  if (show_build_progress) {
    std::ostringstream oss;
    oss << "obj-phase-finish total=" << obj_total << " reused=" << obj_reused
        << " rebuilt=" << obj_rebuilt;
    LogBuildProgress(oss.str());
  }
  SPEC_RULE("Out-Obj-Done");

  std::vector<std::filesystem::path> irs;
  if (emit_ir == "none") {
    SPEC_RULE("Emit-IR-None");
    if (show_build_progress) {
      LogBuildProgress("ir-skip mode=none");
    }
    if (linkable) {
      SPEC_RULE("Out-IR-None-Finalize");
    } else {
      SPEC_RULE("Out-IR-None-NoFinalize");
    }
  } else {
    const auto ir_paths =
        IRPaths(project, target_profile, project.modules, emit_ir);
    if (!IsDistinct(ir_paths)) {
      if (show_build_progress) {
        LogBuildProgress("ir-collision");
      }
      SPEC_RULE("Out-IR-Collision");
      EmitExternal(result.diags, "E-OUT-0406");
      SPEC_RULE("Output-Pipeline-Err");
      return result;
    }

    irs.reserve(project.modules.size());
    const std::size_t ir_total = project.modules.size();
    std::size_t ir_index = 0;
    std::size_t ir_reused = 0;
    std::size_t ir_rebuilt = 0;
    auto maybe_log_ir_progress = [&]() {
      if (!show_build_progress || show_detailed_progress || ir_total == 0) {
        return;
      }
      const bool on_step = (ir_index % kSummaryProgressStep) == 0;
      if (!on_step && ir_index != ir_total) {
        return;
      }
      std::ostringstream oss;
      oss << "ir-progress mode=" << emit_ir << " index=" << ir_index << "/"
          << ir_total << " reused=" << ir_reused
          << " rebuilt=" << ir_rebuilt;
      LogBuildProgress(oss.str());
    };
    if (show_build_progress) {
      std::ostringstream oss;
      oss << "ir-phase-start mode=" << emit_ir << " total=" << ir_total;
      LogBuildProgress(oss.str());
    }
    for (const auto& module : project.modules) {
      ++ir_index;
      const auto ir_path = IRPath(project, target_profile, module, emit_ir);

      std::optional<IncrementalModuleInfo> module_info;
      if (incremental_enabled) {
        module_info = deps.incremental_module(module, project);
      }

      bool reused_ir = false;
      std::string reused_ir_hash;
      if (incremental_enabled && module_info.has_value() && manifest_compatible) {
        const auto prev_it = previous_manifest.modules.find(module.path);
        if (prev_it != previous_manifest.modules.end() &&
            prev_it->second.info.full_hash == module_info->full_hash &&
            FileExists(ir_path)) {
          reused_ir = true;
          reused_ir_hash = prev_it->second.ir_hash;
          if (reused_ir_hash.empty()) {
            const auto hash = HashFileBytes(ir_path);
            if (hash.has_value()) {
              reused_ir_hash = *hash;
            }
          }
        }
      }

      if (reused_ir) {
        if (show_detailed_progress) {
          std::ostringstream oss;
          oss << "ir-reuse mode=" << emit_ir << " module=" << module.path
              << " index=" << ir_index << "/" << ir_total
              << " path=" << ir_path.generic_string();
          LogBuildProgress(oss.str());
        }
        if (emit_ir == "ll") {
          SPEC_RULE("Emit-IR-Cons-LL");
          SPEC_RULE("Out-IR-Cons-LL");
        } else {
          SPEC_RULE("Emit-IR-Cons-BC");
          SPEC_RULE("Out-IR-Cons-BC");
        }
        irs.push_back(ir_path);

        IncrementalManifestModuleState& state = next_manifest.modules[module.path];
        if (state.info.full_hash.empty() && module_info.has_value()) {
          state.info = *module_info;
        }
        state.ir_hash = reused_ir_hash;
        ++ir_reused;
        maybe_log_ir_progress();
        continue;
      }

      if (emit_ir == "ll") {
        if (show_detailed_progress) {
          std::ostringstream oss;
          oss << "ir-codegen mode=ll module=" << module.path
              << " index=" << ir_index << "/" << ir_total
              << " path=" << ir_path.generic_string();
          LogBuildProgress(oss.str());
        }
        const auto ll_bytes = deps.codegen_ir(module, project, "ll");
        if (!ll_bytes.has_value()) {
          if (show_build_progress) {
            std::ostringstream oss;
            oss << "ir-error mode=ll module=" << module.path;
            LogBuildProgress(oss.str());
          }
          SPEC_RULE("Emit-IR-Err");
          SPEC_RULE("Out-IR-Err");
          EmitExternal(result.diags, "E-OUT-0403");
          SPEC_RULE("Output-Pipeline-Err");
          return result;
        }
        SPEC_RULE("CodegenIR-LLVM");
        if (!deps.write_file(ir_path, *ll_bytes)) {
          if (show_build_progress) {
            std::ostringstream oss;
            oss << "ir-write-error mode=ll module=" << module.path
                << " path=" << ir_path.generic_string();
            LogBuildProgress(oss.str());
          }
          core::HostPrimFail(core::HostPrim::WriteFile, true);
          SPEC_RULE("Emit-IR-Err");
          SPEC_RULE("Out-IR-Err");
          EmitExternal(result.diags, "E-OUT-0403");
          SPEC_RULE("Output-Pipeline-Err");
          return result;
        }
        if (show_detailed_progress) {
          std::ostringstream oss;
          oss << "ir-written mode=ll module=" << module.path
              << " index=" << ir_index << "/" << ir_total
              << " path=" << ir_path.generic_string() << " bytes="
              << ll_bytes->size();
          LogBuildProgress(oss.str());
        }
        SPEC_RULE("Emit-IR-Cons-LL");
        SPEC_RULE("Out-IR-Cons-LL");
        irs.push_back(ir_path);
        any_ir_rebuilt = true;

        if (incremental_enabled) {
          IncrementalManifestModuleState& state = next_manifest.modules[module.path];
          if (state.info.full_hash.empty()) {
            if (module_info.has_value()) {
              state.info = *module_info;
            } else {
              state.info.full_hash = HashBytes(*ll_bytes);
              state.info.source_hash = state.info.full_hash;
              state.info.public_hash = state.info.full_hash;
            }
          }
          state.ir_hash = HashBytes(*ll_bytes);
        }
        ++ir_rebuilt;
        maybe_log_ir_progress();
      } else {
        if (show_detailed_progress) {
          std::ostringstream oss;
          oss << "ir-codegen mode=bc module=" << module.path
              << " index=" << ir_index << "/" << ir_total
              << " path=" << ir_path.generic_string();
          LogBuildProgress(oss.str());
        }
        const auto ll_bytes = deps.codegen_ir(module, project, "ll");
        if (!ll_bytes.has_value()) {
          if (show_build_progress) {
            std::ostringstream oss;
            oss << "ir-error mode=bc module=" << module.path
                << " stage=codegen";
            LogBuildProgress(oss.str());
          }
          SPEC_RULE("Emit-IR-Err");
          SPEC_RULE("Out-IR-Err");
          EmitExternal(result.diags, "E-OUT-0403");
          SPEC_RULE("Output-Pipeline-Err");
          return result;
        }
        SPEC_RULE("CodegenIR-LLVM");
        const auto tool = deps.resolve_tool(project, target_profile, "llvm-as");
        if (!tool.has_value()) {
          if (show_build_progress) {
            LogBuildProgress("ir-error mode=bc stage=resolve-llvm-as");
          }
          SPEC_RULE("Emit-IR-Err");
          SPEC_RULE("Out-IR-Err");
          EmitExternal(result.diags, "E-OUT-0403");
          SPEC_RULE("Output-Pipeline-Err");
          return result;
        }
        const auto bc_bytes = deps.assemble_ir(*tool, *ll_bytes);
        if (!bc_bytes.has_value()) {
          if (show_build_progress) {
            std::ostringstream oss;
            oss << "ir-error mode=bc module=" << module.path
                << " stage=assemble";
            LogBuildProgress(oss.str());
          }
          SPEC_RULE("Emit-IR-Err");
          SPEC_RULE("Out-IR-Err");
          EmitExternal(result.diags, "E-OUT-0403");
          SPEC_RULE("Output-Pipeline-Err");
          return result;
        }
        if (!deps.write_file(ir_path, *bc_bytes)) {
          if (show_build_progress) {
            std::ostringstream oss;
            oss << "ir-write-error mode=bc module=" << module.path
                << " path=" << ir_path.generic_string();
            LogBuildProgress(oss.str());
          }
          core::HostPrimFail(core::HostPrim::WriteFile, true);
          SPEC_RULE("Emit-IR-Err");
          SPEC_RULE("Out-IR-Err");
          EmitExternal(result.diags, "E-OUT-0403");
          SPEC_RULE("Output-Pipeline-Err");
          return result;
        }
        if (show_detailed_progress) {
          std::ostringstream oss;
          oss << "ir-written mode=bc module=" << module.path
              << " index=" << ir_index << "/" << ir_total
              << " path=" << ir_path.generic_string() << " bytes="
              << bc_bytes->size();
          LogBuildProgress(oss.str());
        }
        SPEC_RULE("Emit-IR-Cons-BC");
        SPEC_RULE("Out-IR-Cons-BC");
        irs.push_back(ir_path);
        any_ir_rebuilt = true;

        if (incremental_enabled) {
          IncrementalManifestModuleState& state = next_manifest.modules[module.path];
          if (state.info.full_hash.empty()) {
            if (module_info.has_value()) {
              state.info = *module_info;
            } else {
              state.info.full_hash = HashBytes(*bc_bytes);
              state.info.source_hash = state.info.full_hash;
              state.info.public_hash = state.info.full_hash;
            }
          }
          state.ir_hash = HashBytes(*bc_bytes);
        }
        ++ir_rebuilt;
        maybe_log_ir_progress();
      }
    }
    if (show_build_progress) {
      std::ostringstream oss;
      oss << "ir-phase-finish mode=" << emit_ir << " total=" << ir_total
          << " reused=" << ir_reused << " rebuilt=" << ir_rebuilt;
      LogBuildProgress(oss.str());
    }
    if (linkable) {
      SPEC_RULE("Out-IR-Done-Finalize");
    } else {
      SPEC_RULE("Out-IR-Done-NoFinalize");
    }
  }

  auto persist_manifest_if_enabled = [&]() {
    if (!incremental_enabled) {
      return;
    }
    if (!SaveIncrementalManifest(project, deps, next_manifest) &&
        show_build_progress) {
      LogBuildProgress("incremental-warning reason=manifest-write-failed");
    }
  };

  if (dependency) {
    OutputArtifacts artifacts;
    artifacts.objs = std::move(objs);
    artifacts.irs = std::move(irs);
    artifacts.primary_artifact.reset();
    artifacts.import_lib.reset();
    artifacts.map_file.reset();
    result.artifacts = std::move(artifacts);

    persist_manifest_if_enabled();

    SPEC_RULE("Output-Pipeline-Dependency");
    if (show_build_progress) {
      std::ostringstream oss;
      oss << "finish kind=dependency objs=" << result.artifacts->objs.size()
          << " irs=" << result.artifacts->irs.size();
      LogBuildProgress(oss.str());
    }
    return result;
  }

  const auto primary_artifact = PrimaryArtifactPath(project, target_profile);
  if (!primary_artifact.has_value()) {
    SPEC_RULE("Output-Pipeline-Err");
    return result;
  }

  const auto import_lib = ImportLibPath(project, target_profile);
  const auto map_path = MapPath(project, target_profile);
  bool reuse_final = false;
  if (incremental_enabled && manifest_compatible && !any_obj_rebuilt &&
      !any_ir_rebuilt && FileExists(*primary_artifact) &&
      (!import_lib.has_value() || FileExists(*import_lib)) &&
      (!map_path.has_value() || FileExists(*map_path))) {
    const auto runtime_lib = deps.resolve_runtime_lib(project, target_profile);
    const std::string link_fingerprint = ComputeLinkFingerprint(
        project, target_profile, *incremental_build_key, next_manifest.modules,
        extra_link_inputs, runtime_lib, link_plan, emit_ir);
    if (!previous_manifest.link_fingerprint.empty() &&
        previous_manifest.link_fingerprint == link_fingerprint) {
      reuse_final = true;
      next_manifest.link_fingerprint = link_fingerprint;
    }
  }

  if (!reuse_final) {
    if (show_build_progress) {
      std::ostringstream oss;
      oss << "finalize-start output=" << primary_artifact->generic_string()
          << " kind=" << project.assembly.kind;
      if (project.assembly.link_kind.has_value()) {
        oss << " link_kind=" << *project.assembly.link_kind;
      }
      oss << " input_count=" << (objs.size() + extra_link_inputs.size() + 1);
      LogBuildProgress(oss.str());
    }

    const LinkResult link_result =
        FinalizeArtifacts(project, target_profile, objs, extra_link_inputs,
                          link_plan, deps);
    for (const auto& diag : link_result.diags) {
      core::Emit(result.diags, diag);
    }

    if (static_library) {
      switch (link_result.status) {
        case LinkStatus::Ok:
          SPEC_RULE("Out-Final-Archive-Ok");
          if (show_build_progress) {
            LogBuildProgress("finalize-ok mode=archive");
          }
          break;
        case LinkStatus::NotFound:
          if (show_build_progress) {
            LogBuildProgress("finalize-error mode=archive reason=tool-not-found");
          }
          SPEC_RULE("Out-Final-Archive-Err");
          SPEC_RULE("Output-Pipeline-Err");
          return result;
        case LinkStatus::RuntimeMissing:
          if (show_build_progress) {
            LogBuildProgress("finalize-error mode=archive reason=runtime-missing");
          }
          SPEC_RULE("Out-Final-Archive-Err");
          SPEC_RULE("Output-Pipeline-Err");
          return result;
        case LinkStatus::RuntimeIncompatible:
        case LinkStatus::Fail:
          if (show_build_progress) {
            LogBuildProgress("finalize-error mode=archive reason=archive-failed");
          }
          SPEC_RULE("Out-Final-Archive-Err");
          SPEC_RULE("Output-Pipeline-Err");
          return result;
      }
    } else {
      switch (link_result.status) {
        case LinkStatus::Ok:
          SPEC_RULE("Out-Final-Link-Ok");
          if (show_build_progress) {
            LogBuildProgress("finalize-ok mode=link");
          }
          break;
        case LinkStatus::NotFound:
          if (show_build_progress) {
            LogBuildProgress("finalize-error mode=link reason=tool-not-found");
          }
          SPEC_RULE("Out-Final-Link-Err");
          SPEC_RULE("Output-Pipeline-Err");
          return result;
        case LinkStatus::RuntimeMissing:
          if (show_build_progress) {
            LogBuildProgress("finalize-error mode=link reason=runtime-missing");
          }
          SPEC_RULE("Out-Final-Link-Err");
          SPEC_RULE("Output-Pipeline-Err");
          return result;
        case LinkStatus::RuntimeIncompatible:
          if (show_build_progress) {
            LogBuildProgress(
                "finalize-error mode=link reason=runtime-incompatible");
          }
          SPEC_RULE("Out-Final-Link-Err");
          SPEC_RULE("Output-Pipeline-Err");
          return result;
        case LinkStatus::Fail:
          if (show_build_progress) {
            LogBuildProgress("finalize-error mode=link reason=link-failed");
          }
          SPEC_RULE("Out-Final-Link-Err");
          SPEC_RULE("Output-Pipeline-Err");
          return result;
      }
    }

    if (IsExecutable(project) || IsSharedLibrary(project)) {
      if (!CopyBundledRuntimeSidecars(project.outputs.bin_dir, target_profile,
                                      result.diags)) {
        if (show_build_progress) {
          LogBuildProgress("finalize-error mode=stage-runtime-sidecars");
        }
        SPEC_RULE("Output-Pipeline-Err");
        return result;
      }
      if (!CopyImportedSharedLibraryArtifacts(project.outputs.bin_dir,
                                             extra_link_inputs,
                                             target_profile,
                                             result.diags)) {
        if (show_build_progress) {
          LogBuildProgress("finalize-error mode=stage-shared-library-sidecars");
        }
        SPEC_RULE("Output-Pipeline-Err");
        return result;
      }
    }

    if (incremental_enabled) {
      const auto runtime_lib = deps.resolve_runtime_lib(project, target_profile);
      next_manifest.link_fingerprint = ComputeLinkFingerprint(
          project, target_profile, *incremental_build_key,
          next_manifest.modules,
          extra_link_inputs, runtime_lib, link_plan, emit_ir);
    }
  } else {
    SPEC_RULE(static_library ? "Out-Final-Archive-Ok" : "Out-Final-Link-Ok");
    if (show_build_progress) {
      LogBuildProgress("finalize-reuse");
    }
  }

  OutputArtifacts artifacts;
  artifacts.objs = std::move(objs);
  artifacts.irs = std::move(irs);
  artifacts.primary_artifact = *primary_artifact;
  artifacts.import_lib = import_lib;
  artifacts.map_file = map_path;
  result.artifacts = std::move(artifacts);

  persist_manifest_if_enabled();

  SPEC_RULE("Output-Pipeline-Linkable");
  if (show_build_progress) {
    std::ostringstream oss;
    oss << "finish kind=" << project.assembly.kind
        << " objs=" << result.artifacts->objs.size()
        << " irs=" << result.artifacts->irs.size()
        << " artifact="
        << result.artifacts->primary_artifact->generic_string();
    if (result.artifacts->import_lib.has_value()) {
      oss << " import_lib="
          << result.artifacts->import_lib->generic_string();
    }
    if (result.artifacts->map_file.has_value()) {
      oss << " map=" << result.artifacts->map_file->generic_string();
    }
    LogBuildProgress(oss.str());
  }
  return result;
}

OutputPipelineResult OutputPipeline(const Project& project,
                                    TargetProfile target_profile,
                                    const OutputPipelineDeps& deps) {
  OutputCoordinatorState state{target_profile, deps};
  OutputPipelineResult result;
  if (!EnsureAssemblyGraphLoaded(project, state, result.diags)) {
    return result;
  }
  const ArtifactBuildPlan plan = BuildArtifactBuildPlan(project, *state.graph);
  for (const auto& assembly_name : plan.assembly_order) {
    if (!LoadAstForAssembly(project, assembly_name, state, result.diags)) {
      return result;
    }

    const auto output_project =
        BuildOutputProjectForAssembly(project, *state.graph, assembly_name);
    if (!output_project.has_value()) {
      EmitInternalDiagnostic(result.diags,
                             "Unable to build output project for `" +
                                 assembly_name + "`");
      return result;
    }

    if (state.project_ast_modules == nullptr) {
      EmitInternalDiagnostic(result.diags,
                             "Assembly graph loaded without project AST modules");
      return result;
    }
    const auto ast_modules =
        FilterAstModulesForProject(*output_project,
                                   *state.project_ast_modules,
                                   result.diags);
    if (!ast_modules.has_value()) {
      return result;
    }

    std::vector<std::filesystem::path> library_inputs;
    const auto imported_libraries =
        ImportedLibraries(output_project->assembly.name, *state.graph);
    library_inputs.reserve(imported_libraries.size());
    for (const auto& lib_name : imported_libraries) {
      const auto built_it = state.built_artifacts.find(lib_name);
      if (built_it == state.built_artifacts.end() ||
          !built_it->second.primary_artifact.has_value()) {
        EmitInternalDiagnostic(result.diags,
                               "Missing built library artifact for `" +
                                   lib_name + "`");
        return result;
      }
      library_inputs.push_back(*built_it->second.primary_artifact);
    }

    const auto extern_specs = CollectExternLibrarySpecs(*ast_modules);
    const auto extern_inputs =
        ResolveExternLibraryInputs(extern_specs, state.target_profile);
    const auto extra_inputs =
        AppendUniquePaths(library_inputs, extern_inputs);
    const LinkPlan link_plan =
        BuildOutputLinkPlan(*output_project, state.target_profile, *ast_modules,
                            state.deps, result.diags);
    if (core::HasError(result.diags)) {
      return result;
    }

    auto assembly_result = OutputPipelineSingleAssembly(*output_project,
                                                        state.target_profile,
                                                        state.deps,
                                                        extra_inputs,
                                                        link_plan);
    for (const auto& diag : assembly_result.diags) {
      core::Emit(result.diags, diag);
    }
    if (!assembly_result.artifacts.has_value()) {
      return result;
    }
    state.built_artifacts[output_project->assembly.name] =
        *assembly_result.artifacts;
    if (!RestageSharedLibraryForExistingConsumers(project,
                                                  *state.graph,
                                                  *output_project,
                                                  *assembly_result.artifacts,
                                                  state.target_profile,
                                                  result.diags)) {
      return result;
    }
    if (output_project->assembly.name == project.assembly.name) {
      result.artifacts = assembly_result.artifacts;
    }
  }
  return result;
}


}  // namespace cursive::project
