#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>

#ifndef CURSIVE_TEST_COMPILER_PATH
#error "CURSIVE_TEST_COMPILER_PATH must be defined"
#endif

#ifndef CURSIVE_TEST_RUNTIME_LIB_PATH
#error "CURSIVE_TEST_RUNTIME_LIB_PATH must be defined"
#endif

#ifndef CURSIVE_TEST_TARGET_PROFILE
#error "CURSIVE_TEST_TARGET_PROFILE must be defined"
#endif

#ifndef CURSIVE_TEST_EXECUTABLE_SUFFIX
#error "CURSIVE_TEST_EXECUTABLE_SUFFIX must be defined"
#endif

#ifndef CURSIVE_TEST_WORK_ROOT
#error "CURSIVE_TEST_WORK_ROOT must be defined"
#endif

#ifndef CURSIVE_TEST_LLD_LINK_PATH
#error "CURSIVE_TEST_LLD_LINK_PATH must be defined"
#endif

#ifndef CURSIVE_TEST_LLVM_LIB_PATH
#error "CURSIVE_TEST_LLVM_LIB_PATH must be defined"
#endif

#ifndef CURSIVE_TEST_LLVM_AS_PATH
#error "CURSIVE_TEST_LLVM_AS_PATH must be defined"
#endif

namespace {

std::string Quote(std::string_view value) {
#ifdef _WIN32
  std::string out = "\"";
  for (char c : value) {
    if (c == '"') {
      out += "\\\"";
    } else {
      out += c;
    }
  }
  out += "\"";
  return out;
#else
  std::string out = "'";
  for (char c : value) {
    if (c == '\'') {
      out += "'\\''";
    } else {
      out += c;
    }
  }
  out += "'";
  return out;
#endif
}

bool WriteFile(const std::filesystem::path& path, const std::string& text) {
  std::ofstream out(path, std::ios::binary);
  if (!out) {
    std::cerr << "failed to open " << path << " for writing\n";
    return false;
  }
  out << text;
  return out.good();
}

std::optional<std::string> ReadFile(const std::filesystem::path& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    std::cerr << "failed to open " << path << " for reading\n";
    return std::nullopt;
  }
  std::ostringstream text;
  text << in.rdbuf();
  return text.str();
}

std::optional<std::string_view> FunctionBody(
    std::string_view module_ir,
    std::string_view signature) {
  const std::size_t start = module_ir.find(signature);
  if (start == std::string_view::npos) {
    return std::nullopt;
  }
  const std::size_t next = module_ir.find("\ndefine ", start + signature.size());
  const std::size_t end = next == std::string_view::npos ? module_ir.size() : next;
  return module_ir.substr(start, end - start);
}

bool ContainsBefore(std::string_view body,
                    std::string_view needle,
                    std::string_view marker) {
  const std::size_t needle_pos = body.find(needle);
  const std::size_t marker_pos = body.find(marker);
  return needle_pos != std::string_view::npos &&
         marker_pos != std::string_view::npos &&
         needle_pos < marker_pos;
}

bool ContainsAfterLast(std::string_view body,
                       std::string_view needle,
                       std::string_view marker) {
  const std::size_t marker_pos = body.rfind(marker);
  if (marker_pos == std::string_view::npos) {
    return false;
  }
  return body.find(needle, marker_pos + marker.size()) != std::string_view::npos;
}

int RunCommand(const std::string& command) {
  return std::system(command.c_str());
}

bool InstallTool(const std::filesystem::path& tool_root,
                 const std::filesystem::path& source,
                 std::string_view name) {
  std::error_code ec;
  std::filesystem::create_directories(tool_root, ec);
  if (ec) {
    std::cerr << "failed to create tool directory " << tool_root << ": "
              << ec.message() << "\n";
    return false;
  }
  if (!std::filesystem::exists(source, ec) || ec) {
    std::cerr << "required test tool is missing: " << source << "\n";
    return false;
  }

  const std::filesystem::path target = tool_root / std::filesystem::path(name);
  std::filesystem::remove(target, ec);
  ec.clear();
  std::filesystem::create_symlink(source, target, ec);
  if (!ec) {
    return true;
  }

  ec.clear();
  std::filesystem::copy_file(
      source,
      target,
      std::filesystem::copy_options::overwrite_existing,
      ec);
  if (ec) {
    std::cerr << "failed to install test tool " << source << " as " << target
              << ": " << ec.message() << "\n";
    return false;
  }
  return true;
}

std::string CommandWithToolPath(const std::filesystem::path& tool_root,
                                const std::string& command) {
#ifdef _WIN32
  return "set \"PATH=" + tool_root.generic_string() + ";%PATH%\" && " +
         command;
#else
  return "PATH=" + Quote(tool_root.generic_string()) + ":$PATH " + command;
#endif
}

std::string RunExecutableCommand(const std::filesystem::path& executable,
                                 const std::filesystem::path& run_log) {
#ifdef _WIN32
  return "cd /d " + Quote(executable.parent_path().generic_string()) + " && " +
         Quote(executable.filename().string()) + " > " +
         Quote(run_log.generic_string()) + " 2>&1";
#else
  return Quote(executable.generic_string()) + " > " +
         Quote(run_log.generic_string()) + " 2>&1";
#endif
}

std::string FixtureManifest() {
  std::ostringstream out;
  out << "[toolchain]\n";
  out << "runtime_lib = \"" << CURSIVE_TEST_RUNTIME_LIB_PATH << "\"\n";
  out << "target_profile = \"" << CURSIVE_TEST_TARGET_PROFILE << "\"\n\n";
  out << "[[assembly]]\n";
  out << "name = \"diagnostic_path\"\n";
  out << "kind = \"executable\"\n";
  out << "root = \"src\"\n";
  out << "out_dir = \"build/diagnostic_path\"\n";
  out << "emit_ir = \"ll\"\n";
  return out.str();
}

std::string FixtureSource() {
  return R"cursive(public modal DiagnosticSeverity {
    @Error {
        public procedure isError(~) -> bool {
            return true
        }
    }

    @Info {
        public procedure isError(~) -> bool {
            return false
        }
    }
}

public type DiagnosticSeverityValue =
    DiagnosticSeverity@Error | DiagnosticSeverity@Info

public record DiagnosticCode {
    public text: string@View
}

public modal DiagnosticCodeOption {
    @Absent {}

    @Present {
        public code: DiagnosticCode
    }
}

public type DiagnosticCodeOptionValue =
    DiagnosticCodeOption@Absent | DiagnosticCodeOption@Present

public let CODE_CLI_UNKNOWN_COMMAND: string@View = "E-CLI-0001"
public let CODE_CLI_PIPELINE_UNAVAILABLE: string@View = "E-CLI-0002"
public let CODE_CLI_OUTPUT_WRITE_FAILED: string@View = "E-CLI-0003"

public record SourceSpan {
    public file_path: string@View
    public start_line: usize
    public start_column: usize
    public end_line: usize
    public end_column: usize
}

public modal DiagnosticSource {
    @Unknown {}

    @Known {
        public span: SourceSpan
    }
}

public type DiagnosticSourceValue =
    DiagnosticSource@Unknown | DiagnosticSource@Known

public record Diagnostic {
    public code: DiagnosticCodeOptionValue
    public severity: DiagnosticSeverityValue
    public source: DiagnosticSourceValue
    public message: string@View

    public procedure isError(~) -> bool {
        return diagnosticSeverityIsError(self.severity)
    }
}

public modal DiagnosticStream {
    @Empty {}

    @Entry {
        public diagnostic: Diagnostic
        public tail: DiagnosticStreamValue

        public procedure currentDiagnostic(~) -> Diagnostic {
            return self.diagnostic
        }

        public procedure remainingDiagnostics(~) -> DiagnosticStreamValue {
            return self.tail
        }
    }
}

public type DiagnosticStreamValue =
    DiagnosticStream@Empty | DiagnosticStream@Entry

public modal CommandFailureReason {
    @UnknownCommand {
        public command_name: string@View
    }

    @PipelineUnavailable {}

    @OutputWriteFailed {}

    @TestTargetRejected {
        public code: string@View
        public message: string@View
    }
}

public type CommandFailureReasonValue =
    CommandFailureReason@OutputWriteFailed |
    CommandFailureReason@PipelineUnavailable |
    CommandFailureReason@TestTargetRejected |
    CommandFailureReason@UnknownCommand

public modal CommandResult {
    @Succeeded {
        public exit_code: i32

        public procedure exitCode(~) -> i32 {
            return self.exit_code
        }
    }

    @Failed {
        public exit_code: i32
        public diagnostics: DiagnosticStreamValue

        public procedure exitCode(~) -> i32 {
            return self.exit_code
        }
    }
}

public type CommandResultValue =
    CommandResult@Failed | CommandResult@Succeeded

public procedure diagnosticCode(text: string@View) -> DiagnosticCode {
    return DiagnosticCode { text: text }
}

public procedure absentDiagnosticCode() -> DiagnosticCodeOptionValue {
    return DiagnosticCodeOption@Absent {}
}

public procedure presentDiagnosticCode(code: DiagnosticCode) -> DiagnosticCodeOptionValue {
    return DiagnosticCodeOption@Present { code: code }
}

public procedure diagnosticCodeFromText(text: string@View) -> DiagnosticCodeOptionValue {
    return presentDiagnosticCode(diagnosticCode(text))
}

public procedure diagnosticSeverityError() -> DiagnosticSeverityValue {
    return DiagnosticSeverity@Error {}
}

public procedure diagnosticSeverityIsError(severity: DiagnosticSeverityValue) -> bool {
    return if severity is {
        @Error {
            severity~>isError()
        }
        @Info {
            severity~>isError()
        }
    }
}

public procedure unknownDiagnosticSource() -> DiagnosticSourceValue {
    return DiagnosticSource@Unknown {}
}

public procedure diagnosticSourceIsUnknown(source: DiagnosticSourceValue) -> bool {
    return if source is {
        @Unknown {
            true
        }
        @Known {
            false
        }
    }
}

public procedure diagnostic(
    code: DiagnosticCodeOptionValue,
    severity: DiagnosticSeverityValue,
    source: DiagnosticSourceValue,
    message: string@View
) -> Diagnostic {
    return Diagnostic {
        code: code,
        severity: severity,
        source: source,
        message: message
    }
}

public procedure errorDiagnostic(
    code: string@View,
    message: string@View
) -> Diagnostic {
    return diagnostic(
        diagnosticCodeFromText(code),
        diagnosticSeverityError(),
        unknownDiagnosticSource(),
        message
    )
}

public procedure emptyDiagnosticStream(
    diagnostics_region: unique Region@Active
) -> DiagnosticStreamValue {
    return diagnostics_region ^ DiagnosticStream@Empty {}
}

public procedure singleDiagnosticStream(
    diagnostics_region: unique Region@Active,
    diagnostic: Diagnostic
) -> DiagnosticStreamValue {
    let empty_tail: DiagnosticStreamValue =
        emptyDiagnosticStream(diagnostics_region)
    return diagnostics_region ^ DiagnosticStream@Entry {
        diagnostic: diagnostic,
        tail: move empty_tail
    }
}

public procedure singleErrorDiagnosticStream(
    diagnostics_region: unique Region@Active,
    code: string@View,
    message: string@View
) -> DiagnosticStreamValue {
    return singleDiagnosticStream(
        diagnostics_region,
        errorDiagnostic(code, message)
    )
}

public procedure diagnosticMessage(diagnostic: Diagnostic) -> string@View {
    return diagnostic.message
}

public procedure diagnosticCodeMatches(
    diagnostic: Diagnostic,
    expected: string@View
) -> bool {
    return if diagnostic.code is {
        @Absent {
            false
        }
        @Present { code } {
            diagnosticTextEquals(code.text, expected)
        }
    }
}

public procedure unknownCommandFailure(
    command_name: string@View
) -> CommandFailureReasonValue {
    return CommandFailureReason@UnknownCommand {
        command_name: command_name
    }
}

public procedure rejectedTargetFailure(
    code: string@View,
    message: string@View
) -> CommandFailureReasonValue {
    return CommandFailureReason@TestTargetRejected {
        code: code,
        message: message
    }
}

public procedure commandFailureDiagnostics(
    diagnostics_region: unique Region@Active,
    reason: CommandFailureReasonValue
) -> DiagnosticStreamValue {
    return if reason is {
        @UnknownCommand {
            singleErrorDiagnosticStream(
                diagnostics_region,
                CODE_CLI_UNKNOWN_COMMAND,
                "unknown command"
            )
        }
        @PipelineUnavailable {
            singleErrorDiagnosticStream(
                diagnostics_region,
                CODE_CLI_PIPELINE_UNAVAILABLE,
                "compiler pipeline unavailable"
            )
        }
        @OutputWriteFailed {
            singleErrorDiagnosticStream(
                diagnostics_region,
                CODE_CLI_OUTPUT_WRITE_FAILED,
                "failed to write command output"
            )
        }
        @TestTargetRejected { code, message } {
            singleErrorDiagnosticStream(diagnostics_region, code, message)
        }
    }
}

public procedure commandFailed(
    diagnostics_region: unique Region@Active,
    reason: CommandFailureReasonValue
) -> CommandResultValue {
    return commandFailedWithDiagnostics(
        commandFailureDiagnostics(diagnostics_region, reason)
    )
}

public procedure commandFailedWithDiagnostics(
    move diagnostics: DiagnosticStreamValue
) -> CommandResultValue {
    return CommandResult@Failed {
        exit_code: 1,
        diagnostics: move diagnostics
    }
}

public procedure diagnosticTextEquals(left: string@View, right: string@View) -> bool {
    let left_length: usize = string::length(left)
    let right_length: usize = string::length(right)
    if (left_length != right_length) {
        return false
    }

    var index: usize = 0
    loop {
        if (index >= left_length) {
            break
        }

        if (diagnosticTextByte(left, index) != diagnosticTextByte(right, index)) {
            return false
        }

        index = index + 1
    }
    return true
}

public procedure diagnosticTextByte(text: string@View, index: usize) -> u8
|: index < string::length(text)
{
    let view: bytes@View = bytes::view_string(text)
    let data: const [u8] = bytes::as_slice(view)
    return data[index]
}

public procedure isDigitText(text: string@View) -> bool {
    if (diagnosticTextEquals(text, "0")) {
        return true
    }
    if (diagnosticTextEquals(text, "1")) {
        return true
    }
    if (diagnosticTextEquals(text, "2")) {
        return true
    }
    if (diagnosticTextEquals(text, "3")) {
        return true
    }
    if (diagnosticTextEquals(text, "4")) {
        return true
    }
    if (diagnosticTextEquals(text, "5")) {
        return true
    }
    if (diagnosticTextEquals(text, "6")) {
        return true
    }
    if (diagnosticTextEquals(text, "7")) {
        return true
    }
    if (diagnosticTextEquals(text, "8")) {
        return true
    }
    return diagnosticTextEquals(text, "9")
}

public record DecimalWriter {
    public marker: usize

    public procedure writeUnsignedDecimal(~, value: usize) -> bool {
        if (value < 10) {
            return isDigitText(decimalDigitText(value))
        }

        let prefix: usize = value / 10
        return self~>writeUnsignedDecimal(prefix)
    }
}

public procedure decimalDigitText(value: usize) -> string@View
|: value < 10
{
    if (value == 0) {
        return "0"
    }
    if (value == 1) {
        return "1"
    }
    if (value == 2) {
        return "2"
    }
    if (value == 3) {
        return "3"
    }
    if (value == 4) {
        return "4"
    }
    if (value == 5) {
        return "5"
    }
    if (value == 6) {
        return "6"
    }
    if (value == 7) {
        return "7"
    }
    if (value == 8) {
        return "8"
    }
    return "9"
}

public procedure classify(stream: DiagnosticStreamValue) -> i32 {
    if stream is {
        @Empty {
            return 1
        }
        @Entry {
            let current: Diagnostic = stream~>currentDiagnostic()
            if (!current~>isError()) {
                return 2
            }
            if (!diagnosticCodeMatches(current, "E-CLI-0001")) {
                return 3
            }
            if (!diagnosticTextEquals(diagnosticMessage(current), "unknown command")) {
                return 4
            }
            if (!diagnosticSourceIsUnknown(current.source)) {
                return 5
            }
            return if stream~>remainingDiagnostics() is {
                @Empty {
                    0
                }
                @Entry {
                    6
                }
            }
        }
    }
    return 7
}

public procedure classifyCommandResult(result: CommandResultValue) -> i32 {
    return if result is {
        @Succeeded {
            8
        }
        @Failed {
            if (result~>exitCode() != 1) {
                return 9
            }
            classify(result.diagnostics)
        }
    }
}

public procedure main(move ctx: Context) -> i32 {
    let _ = ctx
    region as diagnostics_region {
        let unknown: CommandResultValue =
            commandFailed(
                diagnostics_region,
                unknownCommandFailure("definitely_unknown_command")
            )
        let unknown_status: i32 = classifyCommandResult(unknown)
        if (unknown_status != 0) {
            return unknown_status
        }
        let writer: DecimalWriter = DecimalWriter {
            marker: 0usize
        }
        if (!writer~>writeUnsignedDecimal(42)) {
            return 10
        }

        let rejected: CommandResultValue =
            commandFailed(
                diagnostics_region,
                rejectedTargetFailure("E-CLI-0001", "unknown command")
            )
        return classifyCommandResult(rejected)
    }
    return 3
}
)cursive";
}

}  // namespace

int main() {
  const std::filesystem::path work_root = CURSIVE_TEST_WORK_ROOT;
  const std::filesystem::path project_root = work_root / "codegen_diagnostic_path_fixture";
  const std::filesystem::path source_root = project_root / "src";
  const std::filesystem::path out_root = project_root / "out";
  const std::filesystem::path tool_root = project_root / "tools";
  const std::filesystem::path compile_log = project_root / "compile.log";
  const std::filesystem::path run_log = project_root / "run.log";

  std::error_code ec;
  std::filesystem::remove_all(project_root, ec);
  if (ec) {
    std::cerr << "failed to remove old fixture: " << ec.message() << "\n";
    return 1;
  }
  std::filesystem::create_directories(source_root, ec);
  if (ec) {
    std::cerr << "failed to create fixture directories: " << ec.message()
              << "\n";
    return 1;
  }

  if (!WriteFile(project_root / "Cursive.toml", FixtureManifest()) ||
      !WriteFile(source_root / "Main.cursive", FixtureSource())) {
    return 1;
  }
  if (!InstallTool(tool_root, CURSIVE_TEST_LLD_LINK_PATH, "lld-link") ||
      !InstallTool(tool_root, CURSIVE_TEST_LLVM_LIB_PATH, "llvm-lib") ||
      !InstallTool(tool_root, CURSIVE_TEST_LLVM_AS_PATH, "llvm-as")) {
    return 1;
  }

  const std::string compile_command =
      CommandWithToolPath(
          tool_root,
          Quote(CURSIVE_TEST_COMPILER_PATH) + " --target-profile " +
              Quote(CURSIVE_TEST_TARGET_PROFILE) + " " +
              Quote(project_root.generic_string()) +
              " --assembly diagnostic_path --out-dir " +
              Quote(out_root.generic_string()) +
              " --build-progress on --incremental off > " +
              Quote(compile_log.generic_string()) + " 2>&1");
  const int compile_result = RunCommand(compile_command);
  if (compile_result != 0) {
    std::cerr << "fixture compile failed; see " << compile_log << "\n";
    return 1;
  }

  const std::filesystem::path executable =
      out_root / "bin" /
      (std::string("diagnostic_path") +
       std::string(CURSIVE_TEST_EXECUTABLE_SUFFIX));
  if (!std::filesystem::exists(executable)) {
    std::cerr << "fixture executable was not produced: " << executable << "\n";
    return 1;
  }

  const std::string run_command = RunExecutableCommand(executable, run_log);
  const int run_result = RunCommand(run_command);
  if (run_result != 0) {
    std::cerr << "fixture executable failed; see " << run_log << "\n";
    return 1;
  }

  const std::filesystem::path ir_file =
      out_root / "ir" / "diagnostic_x5fpath.ll";
  const auto ir_text = ReadFile(ir_file);
  if (!ir_text.has_value()) {
    return 1;
  }
  constexpr std::string_view single_error_signature =
      "@diagnostic_x5fpath_x3a_x3asingleErrorDiagnosticStream";
  const auto single_error_body =
      FunctionBody(*ir_text, single_error_signature);
  if (!single_error_body.has_value()) {
    std::cerr << "singleErrorDiagnosticStream was not emitted\n";
    return 1;
  }
  constexpr std::string_view diagnostic_poison =
      "poison_x3a_x3adiagnostic_x5fpath";
  constexpr std::string_view scope_enter =
      "@cursive_x3a_x3aruntime_x3a_x3aregion_x3a_x3ascope_x5fenter";
  constexpr std::string_view region_new_scoped =
      "@cursive_x3a_x3aruntime_x3a_x3aregion_x3a_x3anew_x5fscoped";
  if (!ContainsBefore(*single_error_body, diagnostic_poison, scope_enter)) {
    std::cerr << "singleErrorDiagnosticStream does not check module poison "
                 "before entering the body\n";
    return 1;
  }
  if (!ContainsBefore(*single_error_body, "store i8 0", scope_enter) ||
      !ContainsBefore(*single_error_body, "store i32 0", scope_enter)) {
    std::cerr << "singleErrorDiagnosticStream does not clear the panic record "
                 "before entering the body\n";
    return 1;
  }
  if (single_error_body->find(region_new_scoped) != std::string_view::npos) {
    std::cerr << "singleErrorDiagnosticStream emitted a synthetic procedure "
                 "region instead of relying on source region constructs\n";
    return 1;
  }
  constexpr std::string_view scope_exit =
      "@cursive_x3a_x3aruntime_x3a_x3aregion_x3a_x3ascope_x5fexit";
  if (ContainsAfterLast(*single_error_body, "panic.take", scope_exit)) {
    std::cerr << "singleErrorDiagnosticStream emits a panic propagation check "
                 "after non-panicking runtime scope cleanup\n";
    return 1;
  }

  constexpr std::string_view main_signature =
      "@diagnostic_x5fpath_x3a_x3amain";
  const auto main_body = FunctionBody(*ir_text, main_signature);
  if (!main_body.has_value()) {
    std::cerr << "diagnostic_path main was not emitted\n";
    return 1;
  }
  if (main_body->find(region_new_scoped) == std::string_view::npos) {
    std::cerr << "explicit region statement did not emit Region::new_scoped\n";
    return 1;
  }

  constexpr std::string_view code_global =
      "@diagnostic_x5fpath_x3a_x3aCODE_x5fCLI_x5fUNKNOWN_x5fCOMMAND";
  const std::size_t code_global_pos = ir_text->find(code_global);
  if (code_global_pos == std::string::npos) {
    std::cerr << "diagnostic code static was not emitted\n";
    return 1;
  }
  const std::size_t code_global_line_end = ir_text->find('\n', code_global_pos);
  const std::string_view code_global_line =
      std::string_view(*ir_text).substr(
          code_global_pos,
          code_global_line_end == std::string::npos
              ? std::string_view(*ir_text).size() - code_global_pos
              : code_global_line_end - code_global_pos);
  if (code_global_line.find("{ ptr, i64 } zeroinitializer") ==
          std::string_view::npos ||
      code_global_line.find("align 8") == std::string_view::npos) {
    std::cerr << "diagnostic code static is not emitted as an aligned "
                 "string@View global\n";
    return 1;
  }

  constexpr std::string_view init_signature =
      "define void @cursive_x3a_x3aruntime_x3a_x3ainit_x3a_x3a"
      "diagnostic_x5fpath(ptr %__panic)";
  const auto init_body = FunctionBody(*ir_text, init_signature);
  if (!init_body.has_value()) {
    std::cerr << "diagnostic_path module init was not emitted\n";
    return 1;
  }
  if (init_body->find("poison.take") != std::string_view::npos) {
    std::cerr << "string@View static cleanup emitted a module poison read "
                 "inside init panic handling\n";
    return 1;
  }

  constexpr std::string_view drop_entry_signature =
      "define linkonce_odr dso_local void "
      "@cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3a"
      "diagnostic_x5fpath_x3a_x3aDiagnosticStream_x3a_x3aEntry"
      "(ptr %data, ptr %__panic)";
  const auto drop_entry_body = FunctionBody(*ir_text, drop_entry_signature);
  if (drop_entry_body.has_value() &&
      drop_entry_body->find("load ptr, ptr %data1") == std::string_view::npos) {
    std::cerr << "DiagnosticStream@Entry drop glue does not read %data\n";
    return 1;
  }
  if (drop_entry_body.has_value() &&
      drop_entry_body->find("ptr null") != std::string_view::npos) {
    std::cerr << "DiagnosticStream@Entry drop glue still passes null data\n";
    return 1;
  }

  return 0;
}
