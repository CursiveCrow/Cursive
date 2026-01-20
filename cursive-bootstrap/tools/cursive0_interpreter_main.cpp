#include <cstdint>
#include <filesystem>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "cursive0/core/diagnostic_render.h"
#include "cursive0/core/diagnostics.h"
#include "cursive0/core/int128.h"
#include "cursive0/core/source_load.h"
#include "cursive0/frontend/parse_modules.h"
#include "cursive0/interpreter/interpreter.h"
#include "cursive0/project/module_discovery.h"
#include "cursive0/project/project.h"
#include "cursive0/sema/collect_toplevel.h"
#include "cursive0/sema/conformance.h"
#include "cursive0/sema/resolver.h"
#include "cursive0/sema/scopes_lookup.h"
#include "cursive0/sema/typecheck.h"
#include "cursive0/sema/visibility.h"
#include "cursive0/syntax/lexer.h"
#include "cursive0/syntax/parser.h"

namespace {

using cursive0::core::DiagnosticStream;
using cursive0::core::Emit;
using cursive0::core::HasError;
using cursive0::core::Render;
using cursive0::frontend::InspectResult;
using cursive0::sema::CheckC0AttrSyntaxUnsupportedTokens;
using cursive0::sema::CheckC0SubsetPermTokens;
using cursive0::sema::CheckC0UnwindAttrUnsupportedTokens;
using cursive0::sema::CheckC0UnsupportedConstructsTokens;
using cursive0::sema::ConformanceInput;
using cursive0::sema::PhaseOrderResult;
using cursive0::sema::RejectIllFormed;

struct CliOptions {
  bool show_help = false;
  std::optional<std::string> assembly_target;
  std::string input_path;
};

static std::optional<CliOptions> ParseArgs(int argc, char** argv) {
  CliOptions opts;
  for (int i = 1; i < argc; ++i) {
    std::string_view arg = argv[i];
    if (arg == "--help" || arg == "-h") {
      opts.show_help = true;
      return opts;
    }
    if (arg == "--assembly") {
      if (i + 1 >= argc) {
        return std::nullopt;
      }
      opts.assembly_target = std::string(argv[++i]);
      continue;
    }
    if (arg.rfind("--assembly=", 0) == 0) {
      opts.assembly_target = std::string(arg.substr(std::string_view("--assembly=").size()));
      continue;
    }
    if (arg == "run") {
      continue;
    }
    if (!arg.empty() && arg[0] == '-') {
      return std::nullopt;
    }
    if (!opts.input_path.empty()) {
      return std::nullopt;
    }
    opts.input_path = std::string(arg);
  }
  if (!opts.show_help && opts.input_path.empty()) {
    return std::nullopt;
  }
  return opts;
}

static void AppendDiags(DiagnosticStream& out,
                        const DiagnosticStream& add) {
  for (const auto& diag : add) {
    out = Emit(out, diag);
  }
}

static InspectResult InspectC0Subset(const cursive0::core::SourceFile& source) {
  InspectResult result;
  const auto tok = cursive0::syntax::Tokenize(source);
  if (!tok.output.has_value()) {
    return result;
  }
  const auto tokens = cursive0::syntax::FilterNewlines(tok.output->tokens);
  const auto perms = CheckC0SubsetPermTokens(tokens);
  AppendDiags(result.diags, perms.diags);
  const auto unwind = CheckC0UnwindAttrUnsupportedTokens(tokens);
  AppendDiags(result.diags, unwind.diags);
  const auto attrs = CheckC0AttrSyntaxUnsupportedTokens(tokens);
  AppendDiags(result.diags, attrs.diags);
  const auto unsupported = CheckC0UnsupportedConstructsTokens(tokens);
  AppendDiags(result.diags, unsupported.diags);
  result.subset_ok = perms.subset_ok && unwind.subset_ok &&
                     attrs.subset_ok && unsupported.subset_ok;
  return result;
}

static std::string EscapeJson(std::string_view value) {
  std::string out;
  out.reserve(value.size() + 8);
  for (char c : value) {
    switch (c) {
      case '\\':
        out += "\\\\";
        break;
      case '"':
        out += "\\\"";
        break;
      case '\n':
        out += "\\n";
        break;
      case '\r':
        out += "\\r";
        break;
      case '\t':
        out += "\\t";
        break;
      default:
        if (static_cast<unsigned char>(c) < 0x20) {
          std::ostringstream oss;
          oss << "\\u";
          oss << std::hex << std::uppercase;
          oss.width(4);
          oss.fill('0');
          oss << static_cast<int>(static_cast<unsigned char>(c));
          out += oss.str();
        } else {
          out.push_back(c);
        }
        break;
    }
  }
  return out;
}

static std::optional<int> ExitCodeFromValue(const cursive0::semantics::Value& value) {
  const auto* int_val = std::get_if<cursive0::semantics::IntVal>(&value.node);
  if (!int_val || int_val->type != "i32") {
    return std::nullopt;
  }
  if (!cursive0::core::UInt128FitsU64(int_val->magnitude)) {
    return std::nullopt;
  }
  const std::uint64_t mag = cursive0::core::UInt128ToU64(int_val->magnitude);
  const std::int64_t signed_val =
      int_val->negative ? -static_cast<std::int64_t>(mag)
                        : static_cast<std::int64_t>(mag);
  return static_cast<int>(signed_val);
}

}  // namespace

int main(int argc, char** argv) {
  const auto opts = ParseArgs(argc, argv);
  if (!opts.has_value()) {
    std::cerr << "usage: cursive0_interpreter <file> [--assembly <name>]\n";
    return 2;
  }
  if (opts->show_help) {
    std::cout << "cursive0_interpreter <file> [--assembly <name>]\n";
    return 0;
  }

  DiagnosticStream diags;
  bool phase1_ok = false;
  bool resolve_ok = false;
  bool typecheck_ok = false;
  bool subset_ok = true;

  const std::filesystem::path input_path = opts->input_path;
  const auto project_root = cursive0::project::FindProjectRoot(input_path);
  const auto project_result =
      cursive0::project::LoadProject(project_root, opts->assembly_target);
  AppendDiags(diags, project_result.diags);

  std::optional<cursive0::project::Project> project_opt =
      project_result.project;

  std::optional<std::vector<cursive0::syntax::ASTModule>> modules_opt;
  cursive0::sema::ScopeContext ctx;
  std::optional<cursive0::sema::TypecheckResult> typechecked;

  if (!HasError(diags) && project_opt.has_value()) {
    const auto& project = *project_opt;
    cursive0::frontend::ParseModuleDeps deps;
    deps.compilation_unit = cursive0::project::CompilationUnit;
    deps.read_bytes = cursive0::frontend::ReadBytesDefault;
    deps.load_source = cursive0::core::LoadSource;
    deps.parse_file = cursive0::syntax::ParseFile;
    deps.inspect_source = InspectC0Subset;

    const auto parsed =
        cursive0::frontend::ParseModulesWithDeps(project.modules,
                                                 project.source_root,
                                                 project.assembly.name,
                                                 deps);
    AppendDiags(diags, parsed.diags);
    phase1_ok = parsed.modules.has_value();
    subset_ok = parsed.subset_ok;
    modules_opt = parsed.modules;

    if (!HasError(diags) && modules_opt.has_value()) {
      ctx.project = &project;
      ctx.sigma.mods = *modules_opt;
      ctx.scopes = {cursive0::sema::Scope{},
                    cursive0::sema::Scope{},
                    cursive0::sema::Scope{}};
      for (const auto& module : *modules_opt) {
        ctx.current_module = module.path;
        const auto vis_diags =
            cursive0::sema::CheckModuleVisibility(ctx, module);
        AppendDiags(diags, vis_diags);
      }
      const auto name_maps = cursive0::sema::CollectNameMaps(ctx);
      AppendDiags(diags, name_maps.diags);
      if (!HasError(diags)) {
        cursive0::sema::PopulateSigma(ctx);
        const auto module_names = cursive0::sema::ModuleNamesOf(project);
        cursive0::sema::ResolveContext res_ctx;
        res_ctx.ctx = &ctx;
        res_ctx.name_maps = &name_maps.name_maps;
        res_ctx.module_names = &module_names;
        res_ctx.can_access = cursive0::sema::CanAccess;
        const auto resolved = cursive0::sema::ResolveModules(res_ctx);
        resolve_ok = resolved.ok;
        AppendDiags(diags, resolved.diags);
        if (resolved.ok) {
          ctx.sigma.mods = resolved.modules;
          cursive0::sema::PopulateSigma(ctx);
        }
        if (!HasError(diags) && resolve_ok) {
          typechecked = cursive0::sema::TypecheckModules(ctx, ctx.sigma.mods);
          AppendDiags(diags, typechecked->diags);
          typecheck_ok = typechecked->ok;
          if (typecheck_ok) {
            ctx.expr_types = &typechecked->expr_types;
          }
        }
      }
    }
  }

  PhaseOrderResult phases;
  phases.phase1_ok = phase1_ok;
  phases.phase3_ok = phase1_ok && resolve_ok && typecheck_ok;
  phases.phase4_ok = phases.phase3_ok;

  ConformanceInput input;
  input.phase_orders = phases;
  input.subset_ok = subset_ok;
  const bool rejected = RejectIllFormed(input);

  if (HasError(diags) || rejected || !project_opt.has_value() || !typecheck_ok) {
    for (const auto& diag : diags) {
      std::cerr << Render(diag) << "\n";
    }
    if (rejected) {
      std::cerr << "interpreter rejected ill-formed program\n";
    }
    return 2;
  }

  cursive0::semantics::Sigma sigma;
  const auto result =
      cursive0::semantics::InterpretProject(*project_opt, sigma, ctx);

  int exit_code = 0;
  if (result.has_value) {
    exit_code = ExitCodeFromValue(result.value).value_or(0);
  } else if (result.has_control &&
             (result.control.kind == cursive0::semantics::ControlKind::Panic ||
              result.control.kind == cursive0::semantics::ControlKind::Abort)) {
    exit_code = static_cast<int>(cursive0::semantics::PanicCodeOrDefault(sigma));
  } else {
    exit_code = static_cast<int>(cursive0::semantics::PanicCodeOrDefault(sigma));
  }

  std::cout << "{"
            << "\"stdout\":\"" << EscapeJson(sigma.stdout_buffer) << "\","
            << "\"stderr\":\"" << EscapeJson(sigma.stderr_buffer) << "\","
            << "\"exit_code\":" << exit_code
            << "}\n";
  return 0;
}
