#include "cursive0/syntax/parse_modules.h"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <string_view>

#include "cursive0/core/assert_spec.h"
#include "cursive0/core/diagnostic_messages.h"
#include "cursive0/core/host_primitives.h"
#include "cursive0/core/diagnostics.h"
#include "cursive0/analysis/types/conformance.h"
#include "cursive0/syntax/keyword_policy.h"

namespace cursive0::frontend {


ReadBytesResult ReadBytesDefault(const std::filesystem::path& path) {
  ReadBytesResult result;
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    SPEC_RULE("ReadBytes-Err");
    core::HostPrimFail(core::HostPrim::ReadBytes, true);
    if (auto diag = core::MakeDiagnostic("E-SRC-0102")) {
      diag->span.reset();
      result.diags = core::Emit(result.diags, *diag);
    }
    return result;
  }

  std::string buffer((std::istreambuf_iterator<char>(in)),
                     std::istreambuf_iterator<char>());
  if (!in && !in.eof()) {
    SPEC_RULE("ReadBytes-Err");
    core::HostPrimFail(core::HostPrim::ReadBytes, true);
    if (auto diag = core::MakeDiagnostic("E-SRC-0102")) {
      diag->span.reset();
      result.diags = core::Emit(result.diags, *diag);
    }
    return result;
  }

  std::vector<std::uint8_t> bytes(buffer.begin(), buffer.end());
  SPEC_RULE("ReadBytes-Ok");
  result.bytes = std::move(bytes);
  return result;
}

namespace {

void AppendDiags(core::DiagnosticStream& out,
                 const core::DiagnosticStream& add) {
  for (const auto& diag : add) {
    out = core::Emit(out, diag);
  }
}

bool HasDiagCode(const core::DiagnosticStream& diags,
                 std::string_view code) {
  for (const auto& diag : diags) {
    if (diag.code == code) {
      return true;
    }
  }
  return false;
}

std::vector<std::string> SplitModulePath(std::string_view path) {
  std::vector<std::string> parts;
  std::size_t start = 0;
  while (start <= path.size()) {
    const std::size_t pos = path.find("::", start);
    if (pos == std::string_view::npos) {
      parts.emplace_back(path.substr(start));
      break;
    }
    parts.emplace_back(path.substr(start, pos - start));
    start = pos + 2;
  }
  return parts;
}

std::filesystem::path DirOf(std::string_view module_path,
                            const std::filesystem::path& source_root,
                            std::string_view assembly_name) {
  if (module_path == assembly_name) {
    SPEC_RULE("DirOf-Root");
    return source_root;
  }
  SPEC_RULE("DirOf-Rel");
  const auto comps = SplitModulePath(module_path);
  std::filesystem::path dir = source_root;
  for (const auto& comp : comps) {
    dir /= comp;
  }
  return dir;
}

ParseModuleDeps DefaultDeps() {
  ParseModuleDeps deps;
  deps.compilation_unit = project::CompilationUnit;
  deps.read_bytes = ReadBytesDefault;
  deps.load_source = core::LoadSource;
  deps.parse_file = syntax::ParseFile;
  return deps;
}

}  // namespace

ParseModuleResult ParseModuleWithDeps(std::string_view module_path,
                                      const std::filesystem::path& source_root,
                                      std::string_view assembly_name,
                                      const ParseModuleDeps& deps) {
  ParseModuleResult result;
  const bool debug_phases = std::getenv("CURSIVE0_DEBUG_PHASES") != nullptr;
  const auto log_phase = [&](const char* label, const std::filesystem::path& path) {
    if (debug_phases) {
      std::cerr << "[cursivec0] parse: " << label << " " << path.string() << "\n";
    }
  };

  SPEC_RULE("Mod-Start");
  const std::filesystem::path module_dir =
      DirOf(module_path, source_root, assembly_name);

  const project::CompilationUnitResult unit =
      deps.compilation_unit(module_dir);
  AppendDiags(result.diags, unit.diags);
  if (core::HasError(unit.diags)) {
    SPEC_RULE("Mod-Start-Err-Unit");
    SPEC_RULE("ParseModule-Err-Unit");
    return result;
  }

  std::vector<syntax::ASTItem> items;
  std::vector<syntax::DocComment> docs;
  std::vector<syntax::UnsafeSpanSet> unsafe_spans;
  for (const auto& file : unit.files) {
    log_phase("read", file);
    const ReadBytesResult bytes = deps.read_bytes(file);
    AppendDiags(result.diags, bytes.diags);
    if (!bytes.bytes.has_value()) {
      SPEC_RULE("Mod-Scan-Err-Read");
      SPEC_RULE("ParseModule-Err-Read");
      return result;
    }
    const core::SourceLoadResult load =
        deps.load_source(file.generic_string(), *bytes.bytes);
    AppendDiags(result.diags, load.diags);
    if (!load.source.has_value()) {
      SPEC_RULE("Mod-Scan-Err-Load");
      SPEC_RULE("ParseModule-Err-Load");
      return result;
    }

    core::DiagnosticStream inspect_diags;
    if (deps.inspect_source) {
      log_phase("inspect", file);
      const InspectResult inspected = deps.inspect_source(*load.source);
      AppendDiags(inspect_diags, inspected.diags);
      if (!inspected.subset_ok) {
        result.subset_ok = false;
      }
    }

    log_phase("parse", file);
    const syntax::ParseFileResult parsed = deps.parse_file(*load.source);
    AppendDiags(result.diags, parsed.diags);
    if (parsed.file.has_value()) {
      const auto ast_subset =
          cursive0::analysis::CheckC0UnsupportedFormsAst(*parsed.file, *load.source);
      AppendDiags(inspect_diags, ast_subset.diags);
      if (!ast_subset.subset_ok) {
        result.subset_ok = false;
      }
    }
    const bool has_parse_uns_0101 = HasDiagCode(parsed.diags, "E-UNS-0101");
    const bool has_parse_uns_0110 = HasDiagCode(parsed.diags, "E-UNS-0110");
    const bool has_parse_uns_0112 = HasDiagCode(parsed.diags, "E-UNS-0112");
    const bool has_parse_uns_0113 = HasDiagCode(parsed.diags, "E-UNS-0113");
    const bool suppress_generic =
        has_parse_uns_0101 || has_parse_uns_0110 || has_parse_uns_0112;
    if (suppress_generic || has_parse_uns_0113) {
      core::DiagnosticStream filtered;
      for (const auto& diag : inspect_diags) {
        if (diag.code == "E-UNS-0101" && suppress_generic) {
          continue;
        }
        if (diag.code == "E-UNS-0113" && has_parse_uns_0113) {
          continue;
        }
        filtered = core::Emit(filtered, diag);
      }
      inspect_diags = std::move(filtered);
    }
    AppendDiags(result.diags, inspect_diags);
    if (!parsed.file.has_value()) {
      SPEC_RULE("Mod-Scan-Err-Parse");
      SPEC_RULE("ParseModule-Err-Parse");
      return result;
    }

    syntax::CheckMethodContext(*parsed.file, result.diags);

    SPEC_RULE("Mod-Scan");
    items.insert(items.end(), parsed.file->items.begin(),
                 parsed.file->items.end());
    docs.insert(docs.end(), parsed.file->module_doc.begin(),
                parsed.file->module_doc.end());
    syntax::UnsafeSpanSet file_spans;
    file_spans.path = parsed.file->path;
    file_spans.spans = parsed.file->unsafe_spans;
    unsafe_spans.push_back(std::move(file_spans));
  }

  SPEC_RULE("Mod-Done");
  syntax::ASTModule module;
  module.path = SplitModulePath(module_path);
  module.items = std::move(items);
  module.module_doc = std::move(docs);
  module.unsafe_spans = std::move(unsafe_spans);
  result.module = std::move(module);
  SPEC_RULE("ParseModule-Ok");
  return result;
}

ParseModulesResult ParseModulesWithDeps(
    const std::vector<project::ModuleInfo>& modules,
    const std::filesystem::path& source_root,
    std::string_view assembly_name,
    const ParseModuleDeps& deps) {
  ParseModulesResult result;

  std::vector<syntax::ASTModule> parsed_modules;
  parsed_modules.reserve(modules.size());
  for (const auto& module : modules) {
    const ParseModuleResult parsed =
        ParseModuleWithDeps(module.path, source_root, assembly_name, deps);
    AppendDiags(result.diags, parsed.diags);
    if (!parsed.subset_ok) {
      result.subset_ok = false;
    }
    if (!parsed.module.has_value()) {
      SPEC_RULE("ParseModules-Err");
      return result;
    }
    parsed_modules.push_back(*parsed.module);
  }

  SPEC_RULE("ParseModules-Ok");
  SPEC_RULE("Phase1-Complete");
  SPEC_RULE("Phase1-Declarations");
  SPEC_RULE("Phase1-Forward-Refs");
  result.modules = std::move(parsed_modules);
  return result;
}

ParseModuleResult ParseModule(std::string_view module_path,
                              const std::filesystem::path& source_root,
                              std::string_view assembly_name) {
  return ParseModuleWithDeps(module_path, source_root, assembly_name,
                             DefaultDeps());
}

ParseModulesResult ParseModules(const std::vector<project::ModuleInfo>& modules,
                                const std::filesystem::path& source_root,
                                std::string_view assembly_name) {
  return ParseModulesWithDeps(modules, source_root, assembly_name,
                              DefaultDeps());
}

}  // namespace cursive0::frontend
