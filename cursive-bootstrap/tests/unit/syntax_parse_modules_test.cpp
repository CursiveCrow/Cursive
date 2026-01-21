#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

#include "cursive0/core/assert_spec.h"
#include "cursive0/core/diagnostic_messages.h"
#include "cursive0/core/diagnostics.h"
#include "cursive0/syntax/parse_modules.h"
#include "cursive0/project/module_discovery.h"

namespace {

using cursive0::core::DiagnosticStream;
using cursive0::core::Emit;
using cursive0::core::HasError;
using cursive0::core::MakeDiagnostic;
using cursive0::frontend::ParseModuleDeps;
using cursive0::frontend::ParseModuleResult;
using cursive0::frontend::ParseModulesResult;
using cursive0::frontend::ReadBytesResult;
using cursive0::project::CompilationUnitResult;
using cursive0::project::ModuleInfo;

std::filesystem::path g_last_dir;
std::vector<std::uint8_t> g_last_bytes;

int g_inspect_calls = 0;

void ResetLastDir() {
  g_last_dir.clear();
}

void ResetLastBytes() {
  g_last_bytes.clear();
}

void ResetInspectCalls() {
  g_inspect_calls = 0;
}

cursive0::frontend::InspectResult InspectSourceSpy(
    const cursive0::core::SourceFile&) {
  ++g_inspect_calls;
  cursive0::frontend::InspectResult result;
  result.subset_ok = false;
  return result;
}

CompilationUnitResult UnitTwoFiles(const std::filesystem::path& dir) {
  g_last_dir = dir;
  CompilationUnitResult result;
  result.files = {dir / "a.cursive", dir / "b.cursive"};
  return result;
}

CompilationUnitResult UnitEmpty(const std::filesystem::path& dir) {
  g_last_dir = dir;
  CompilationUnitResult result;
  return result;
}

CompilationUnitResult UnitOneFile(const std::filesystem::path& dir) {
  g_last_dir = dir;
  CompilationUnitResult result;
  result.files = {dir / "only.cursive"};
  return result;
}

CompilationUnitResult UnitFail(const std::filesystem::path& dir) {
  g_last_dir = dir;
  CompilationUnitResult result;
  auto diag = MakeDiagnostic("E-PRJ-0303");
  assert(diag.has_value());
  diag->span.reset();
  result.diags = Emit(result.diags, *diag);
  return result;
}

ReadBytesResult ReadBytesOk(const std::filesystem::path&) {
  ReadBytesResult result;
  result.bytes = std::vector<std::uint8_t>{0x41};
  return result;
}

ReadBytesResult ReadBytesErr(const std::filesystem::path&) {
  ReadBytesResult result;
  auto diag = MakeDiagnostic("E-SRC-0102");
  assert(diag.has_value());
  diag->span.reset();
  result.diags = Emit(result.diags, *diag);
  return result;
}

ReadBytesResult ReadBytesMaybe(const std::filesystem::path& path) {
  if (path.generic_string().find("bad") != std::string::npos) {
    return ReadBytesErr(path);
  }
  return ReadBytesOk(path);
}

cursive0::core::SourceLoadResult LoadSourceOk(std::string_view path,
                                              const std::vector<std::uint8_t>& bytes) {
  cursive0::core::SourceLoadResult result;
  cursive0::core::SourceFile file;
  file.path = std::string(path);
  file.bytes = bytes;
  result.source = std::move(file);
  return result;
}

cursive0::core::SourceLoadResult LoadSourceCapture(std::string_view path,
                                                   const std::vector<std::uint8_t>& bytes) {
  g_last_bytes = bytes;
  return LoadSourceOk(path, bytes);
}

cursive0::core::SourceLoadResult LoadSourceErr(std::string_view,
                                               const std::vector<std::uint8_t>&) {
  cursive0::core::SourceLoadResult result;
  auto diag = MakeDiagnostic("E-SRC-0101");
  assert(diag.has_value());
  diag->span.reset();
  result.diags = Emit(result.diags, *diag);
  return result;
}

cursive0::syntax::ParseFileResult ParseFileStub(const cursive0::core::SourceFile& source) {
  cursive0::syntax::ParseFileResult result;
  cursive0::syntax::ASTFile file;
  file.path = source.path;

  if (source.path.find("b.cursive") != std::string::npos) {
    file.items.push_back(cursive0::syntax::ErrorItem{});
    file.items.push_back(cursive0::syntax::ErrorItem{});
  } else {
    file.items.push_back(cursive0::syntax::ErrorItem{});
  }

  cursive0::syntax::DocComment doc;
  doc.kind = cursive0::syntax::DocKind::ModuleDoc;
  doc.text = source.path;
  file.module_doc.push_back(doc);

  result.file = std::move(file);
  return result;
}

cursive0::syntax::ParseFileResult ParseFileErr(const cursive0::core::SourceFile&) {
  cursive0::syntax::ParseFileResult result;
  auto diag = MakeDiagnostic("E-SRC-0101");
  assert(diag.has_value());
  diag->span.reset();
  result.diags = Emit(result.diags, *diag);
  return result;
}

}  // namespace

int main() {
  {
    SPEC_COV("DirOf-Root");
    SPEC_COV("ReadBytes-Ok");
    SPEC_COV("Mod-Start");
    SPEC_COV("Mod-Scan");
    SPEC_COV("Mod-Done");
    SPEC_COV("ParseModule-Ok");
    SPEC_COV("Phase1-Complete");
    SPEC_COV("Phase1-Declarations");
    SPEC_COV("Phase1-Forward-Refs");

    ResetLastDir();
    ParseModuleDeps deps;
    deps.compilation_unit = UnitTwoFiles;
    deps.read_bytes = ReadBytesOk;
    deps.load_source = LoadSourceOk;
    deps.parse_file = ParseFileStub;

    const ParseModuleResult result = cursive0::frontend::ParseModuleWithDeps(
        "app", "/src", "app", deps);
    assert(result.module.has_value());
    assert(g_last_dir == std::filesystem::path("/src"));
    assert(result.module->path.size() == 1);
    assert(result.module->path[0] == "app");
    assert(result.module->items.size() == 3);
    assert(result.module->module_doc.size() == 2);
    assert(result.module->module_doc[0].text.find("a.cursive") != std::string::npos);
    assert(result.module->module_doc[1].text.find("b.cursive") != std::string::npos);
  }

  {
    SPEC_COV("DirOf-Rel");

    ResetLastDir();
    ParseModuleDeps deps;
    deps.compilation_unit = UnitEmpty;
    deps.read_bytes = ReadBytesOk;
    deps.load_source = LoadSourceOk;
    deps.parse_file = ParseFileStub;

    const ParseModuleResult result = cursive0::frontend::ParseModuleWithDeps(
        "foo::bar", "/src", "app", deps);
    assert(result.module.has_value());
    assert(g_last_dir == std::filesystem::path("/src/foo/bar"));
    assert(result.module->path.size() == 2);
    assert(result.module->path[0] == "foo");
    assert(result.module->path[1] == "bar");
  }

  {
    SPEC_COV("ReadBytes-Err");
    SPEC_COV("Mod-Scan-Err-Read");
    SPEC_COV("ParseModule-Err-Read");

    ParseModuleDeps deps;
    deps.compilation_unit = UnitOneFile;
    deps.read_bytes = ReadBytesErr;
    deps.load_source = LoadSourceOk;
    deps.parse_file = ParseFileStub;

    const ParseModuleResult result = cursive0::frontend::ParseModuleWithDeps(
        "app", "/src", "app", deps);
    assert(!result.module.has_value());
    assert(HasError(result.diags));
  }

  {
    SPEC_COV("Mod-Scan-Err-Load");
    SPEC_COV("ParseModule-Err-Load");

    ParseModuleDeps deps;
    deps.compilation_unit = UnitOneFile;
    deps.read_bytes = ReadBytesOk;
    deps.load_source = LoadSourceErr;
    deps.parse_file = ParseFileStub;

    const ParseModuleResult result = cursive0::frontend::ParseModuleWithDeps(
        "app", "/src", "app", deps);
    assert(!result.module.has_value());
    assert(HasError(result.diags));
  }

  {
    ResetInspectCalls();
    ParseModuleDeps deps;
    deps.compilation_unit = UnitOneFile;
    deps.read_bytes = ReadBytesOk;
    deps.load_source = LoadSourceErr;
    deps.parse_file = ParseFileStub;
    deps.inspect_source = InspectSourceSpy;

    const ParseModuleResult result = cursive0::frontend::ParseModuleWithDeps(
        "app", "/src", "app", deps);
    assert(!result.module.has_value());
    assert(HasError(result.diags));
    assert(g_inspect_calls == 0);
  }

  {
    SPEC_COV("Mod-Scan-Err-Parse");
    SPEC_COV("ParseModule-Err-Parse");

    ParseModuleDeps deps;
    deps.compilation_unit = UnitOneFile;
    deps.read_bytes = ReadBytesOk;
    deps.load_source = LoadSourceOk;
    deps.parse_file = ParseFileErr;

    const ParseModuleResult result = cursive0::frontend::ParseModuleWithDeps(
        "app", "/src", "app", deps);
    assert(!result.module.has_value());
    assert(HasError(result.diags));
  }

  {
    SPEC_COV("ParseModules-Ok");

    ParseModuleDeps deps;
    deps.compilation_unit = UnitEmpty;
    deps.read_bytes = ReadBytesOk;
    deps.load_source = LoadSourceOk;
    deps.parse_file = ParseFileStub;

    std::vector<ModuleInfo> modules;
    modules.push_back(ModuleInfo{"app", "/src"});
    modules.push_back(ModuleInfo{"foo::bar", "/src/foo/bar"});

    const ParseModulesResult result = cursive0::frontend::ParseModulesWithDeps(
        modules, "/src", "app", deps);
    assert(result.modules.has_value());
    assert(result.modules->size() == 2);
  }

  {
    SPEC_COV("ParseModules-Err");

    ParseModuleDeps deps;
    deps.compilation_unit = UnitOneFile;
    deps.read_bytes = ReadBytesMaybe;
    deps.load_source = LoadSourceOk;
    deps.parse_file = ParseFileStub;

    std::vector<ModuleInfo> modules;
    modules.push_back(ModuleInfo{"app", "/src"});
    modules.push_back(ModuleInfo{"bad", "/src/bad"});

    const ParseModulesResult result = cursive0::frontend::ParseModulesWithDeps(
        modules, "/src", "app", deps);
    assert(!result.modules.has_value());
    assert(HasError(result.diags));
  }

  {
    SPEC_COV("Mod-Start-Err-Unit");
    SPEC_COV("ParseModule-Err-Unit");

    ParseModuleDeps deps;
    deps.compilation_unit = UnitFail;
    deps.read_bytes = ReadBytesOk;
    deps.load_source = LoadSourceOk;
    deps.parse_file = ParseFileStub;

    const ParseModuleResult result = cursive0::frontend::ParseModuleWithDeps(
        "app", "/src", "app", deps);
    assert(!result.module.has_value());
    assert(HasError(result.diags));
  }

  {
    SPEC_COV("ReadBytes-Ok");

    ResetLastBytes();
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto temp_dir = std::filesystem::temp_directory_path() /
        ("cursive_readbytes_" + std::to_string(stamp));
    std::filesystem::create_directories(temp_dir);
    const auto source_path = temp_dir / "only.cursive";
    const std::string content =
        "# SPEC_COV: ReadBytes-Ok\n"
        "# ASSEMBLY: app\n"
        "let x = 1\n";
    {
      std::ofstream out(source_path, std::ios::binary);
      assert(out);
      out << content;
    }

    ParseModuleDeps deps;
    deps.compilation_unit = UnitOneFile;
    deps.read_bytes = cursive0::frontend::ReadBytesDefault;
    deps.load_source = LoadSourceCapture;
    deps.parse_file = ParseFileStub;

    const ParseModuleResult result = cursive0::frontend::ParseModuleWithDeps(
        "app", temp_dir, "app", deps);
    assert(result.module.has_value());
    const std::vector<std::uint8_t> expected(content.begin(), content.end());
    assert(g_last_bytes == expected);

    std::filesystem::remove(source_path);
    std::filesystem::remove(temp_dir);
  }

  return 0;
}
