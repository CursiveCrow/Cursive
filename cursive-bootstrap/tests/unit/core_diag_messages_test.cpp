#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "cursive0/core/diagnostic_messages.h"

namespace {

using cursive0::core::FormatMessage;
using cursive0::core::MakeDiagnostic;
using cursive0::core::MessageArg;
using cursive0::core::MessageForCode;
using cursive0::core::Severity;
using cursive0::core::SeverityForCode;

static int RunDiagMessageGenCheck() {
  const std::filesystem::path tool_path =
      std::filesystem::path(__FILE__).parent_path().parent_path().parent_path() / "tools" /
      "diag_message_gen.py";
  const std::string tool = tool_path.string();
  const std::string cmd = "python3 \"" + tool + "\" --check";
  int rc = std::system(cmd.c_str());
  if (rc != 0) {
    const std::string fallback = "python \"" + tool + "\" --check";
    rc = std::system(fallback.c_str());
  }
  return rc;
}

}  // namespace

int main() {
  {
    const auto msg = MessageForCode("E-PRJ-0101");
    assert(msg.has_value());
    assert(*msg == "`Cursive.toml` not found at project root");

    const auto sev = SeverityForCode("E-PRJ-0101");
    assert(sev.has_value());
    assert(*sev == Severity::Error);
  }

  {
    const auto msg = MessageForCode("W-SRC-0301");
    assert(msg.has_value());
    assert(*msg == "Leading zeros in decimal literal");

    const auto sev = SeverityForCode("W-SRC-0301");
    assert(sev.has_value());
    assert(*sev == Severity::Warning);
  }

  {
    const auto msg = MessageForCode("W-MOD-1101");
    assert(msg.has_value());
    assert(*msg == "Potential module path collision on case-insensitive filesystem");
  }

  {
    const auto diag = MakeDiagnostic("E-OUT-0403");
    assert(diag.has_value());
    assert(diag->message ==
           "Failed to emit IR/bitcode (codegen, assemble, tool resolution, or write)");
    assert(diag->severity == Severity::Error);
  }

  {
    std::vector<MessageArg> args;
    args.push_back({"name", "module"});
    args.push_back({"count", "2"});

    assert(FormatMessage("Missing `{name}`", args) == "Missing `module`");
    assert(FormatMessage("Repeat {count} times", args) == "Repeat 2 times");
    assert(FormatMessage("Unknown {value}", args) == "Unknown {value}");
    assert(FormatMessage("No placeholders", args) == "No placeholders");

  }

  {
    const auto missing = MessageForCode("E-XXX-9999");
    assert(!missing.has_value());

    const auto missing_diag = MakeDiagnostic("E-XXX-9999");
    assert(!missing_diag.has_value());
  }

  {
    const int rc = RunDiagMessageGenCheck();
    assert(rc == 0);
  }

  return 0;
}
