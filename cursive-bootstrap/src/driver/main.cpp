#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "cursive0/core/assert_spec.h"
#include "cursive0/core/diagnostic_messages.h"
#include "cursive0/core/diagnostic_render.h"
#include "cursive0/core/diagnostics.h"
#include "cursive0/frontend/parse_modules.h"
#include "cursive0/project/project.h"
#include "cursive0/sema/conformance.h"
#include "cursive0/sema/collect_toplevel.h"
#include "cursive0/sema/resolver.h"
#include "cursive0/sema/scopes_lookup.h"
#include "cursive0/sema/typecheck.h"
#include "cursive0/sema/visibility.h"
#include "cursive0/syntax/keyword_policy.h"
#include "cursive0/syntax/lexer.h"
#include "cursive0/syntax/lexer.h"
#include "cursive0/syntax/parser.h"
#include "cursive0/codegen/lower_module.h"
#include "cursive0/codegen/ir_dump.h"

namespace {

using cursive0::core::CompileStatus;
using cursive0::core::CompileStatusResult;
using cursive0::core::Diagnostic;
using cursive0::core::DiagnosticStream;
using cursive0::core::Emit;
using cursive0::core::HasError;
using cursive0::core::Render;
using cursive0::core::Severity;
using cursive0::frontend::InspectResult;
using cursive0::sema::ConformanceInput;
using cursive0::sema::CheckC0SubsetPermTokens;
using cursive0::sema::CheckC0AttrSyntaxUnsupportedTokens;
using cursive0::sema::CheckC0UnwindAttrUnsupportedTokens;
using cursive0::sema::CheckC0UnsupportedConstructsTokens;
using cursive0::sema::PhaseOrderResult;
using cursive0::sema::RejectIllFormed;

struct CliOptions {
  bool diag_json = false;
  bool show_help = false;
  bool phase1_only = false;
  bool no_output = false;
  std::optional<std::string> assembly_target;
  std::string input_path;
  bool emit_ir = false;
};


static void SpecDefsDriver() {
  SPEC_DEF("Status", "0.3.2");
  SPEC_DEF("ExitCode", "0.3.2");
}

static bool InternalFlagsEnabled() {
  const char* value = std::getenv("CURSIVE0_INTERNAL_FLAGS");
  return value != nullptr && *value != '\0';
}

static bool StartsWith(std::string_view value, std::string_view prefix) {
  return value.size() >= prefix.size() && value.substr(0, prefix.size()) == prefix;
}


static std::optional<CliOptions> ParseArgs(int argc, char** argv) {
  CliOptions opts;
  for (int i = 1; i < argc; ++i) {
    std::string_view arg = argv[i];
    if (arg == "--help" || arg == "-h") {
      opts.show_help = true;
      return opts;
    }
    if (arg == "--diag-json") {
      opts.diag_json = true;
      continue;
    }
    if (arg == "--phase1-only") {
      if (!InternalFlagsEnabled()) {
        return std::nullopt;
      }
      opts.phase1_only = true;
      continue;
    }
    if (arg == "--no-output") {
      if (!InternalFlagsEnabled()) {
        return std::nullopt;
      }
      opts.no_output = true;
      continue;
    }
    if (arg == "--assembly") {
      if (i + 1 >= argc) {
        return std::nullopt;
      }
      opts.assembly_target = std::string(argv[++i]);
      continue;
    }
    if (StartsWith(arg, "--assembly=")) {
      opts.assembly_target = std::string(arg.substr(std::string_view("--assembly=").size()));
      continue;
    }
    if (arg == "--emit-ir") {
      if (!InternalFlagsEnabled()) {
        return std::nullopt;
      }
      opts.emit_ir = true;
      continue;
    }
    if (arg == "build") {
      continue;
    }
    if (StartsWith(arg, "-")) {
      return std::nullopt;
    }
    if (!opts.input_path.empty()) {
      return std::nullopt;
    }
    opts.input_path = std::string(arg);
  }
  if (opts.show_help) {
    return opts;
  }
  if (opts.input_path.empty()) {
    return std::nullopt;
  }
  return opts;
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

static std::string SeverityString(Severity severity) {
  switch (severity) {
    case Severity::Error:
      return "error";
    case Severity::Warning:
      return "warning";
  }
  return "error";
}

static std::string DiagnosticToJson(const Diagnostic& diag) {
  std::ostringstream oss;
  oss << "{";
  oss << "\"code\":\"" << EscapeJson(diag.code) << "\",";
  oss << "\"severity\":\"" << SeverityString(diag.severity) << "\",";
  oss << "\"message\":\"" << EscapeJson(diag.message) << "\",";
  if (!diag.span.has_value()) {
    oss << "\"span\":null";
  } else {
    const auto& sp = *diag.span;
    oss << "\"span\":{";
    oss << "\"file\":\"" << EscapeJson(sp.file) << "\",";
    oss << "\"start_line\":" << sp.start_line << ",";
    oss << "\"start_col\":" << sp.start_col << ",";
    oss << "\"end_line\":" << sp.end_line << ",";
    oss << "\"end_col\":" << sp.end_col;
    oss << "}";
  }
  oss << "}";
  return oss.str();
}

static std::string DiagnosticStreamToJson(const DiagnosticStream& stream) {
  std::ostringstream oss;
  oss << "{";
  oss << "\"diagnostics\":[";
  for (std::size_t i = 0; i < stream.size(); ++i) {
    if (i > 0) {
      oss << ",";
    }
    oss << DiagnosticToJson(stream[i]);
  }
  oss << "]";
  oss << "}";
  return oss.str();
}

static void AppendDiags(DiagnosticStream& out,
                 const DiagnosticStream& add) {
  for (const auto& diag : add) {
    out = Emit(out, diag);
  }
}

static bool HasDiagCode(const DiagnosticStream& diags,
                      std::string_view code) {
  for (const auto& diag : diags) {
    if (diag.code == code) {
      return true;
    }
  }
  return false;
}

static InspectResult InspectC0Subset(const cursive0::core::SourceFile& source) {
  InspectResult result;
  const auto tok = cursive0::syntax::Tokenize(source);
  if (!tok.output.has_value()) {
    return result;
  }
  const auto tokens = cursive0::syntax::FilterNewlines(tok.output->tokens);
  const auto perms = CheckC0SubsetPermTokens(tokens);
  for (const auto& diag : perms.diags) {
    result.diags = Emit(result.diags, diag);
  }
  const auto unwind = CheckC0UnwindAttrUnsupportedTokens(tokens);
  for (const auto& diag : unwind.diags) {
    result.diags = Emit(result.diags, diag);
  }
  const auto attrs = CheckC0AttrSyntaxUnsupportedTokens(tokens);
  for (const auto& diag : attrs.diags) {
    result.diags = Emit(result.diags, diag);
  }
  const auto unsupported = CheckC0UnsupportedConstructsTokens(tokens);
  for (const auto& diag : unsupported.diags) {
    result.diags = Emit(result.diags, diag);
  }

  result.subset_ok = perms.subset_ok && unwind.subset_ok &&
                     attrs.subset_ok && unsupported.subset_ok;
  return result;
}

}  // namespace

int main(int argc, char** argv) {
  SpecDefsDriver();

  const auto opts = ParseArgs(argc, argv);
  if (!opts.has_value()) {
    std::cerr << "usage: cursivec0 build <file> [--assembly <name>] [--diag-json]\n";
    return 2;
  }
  if (opts->show_help) {
    std::cout << "cursivec0 build <file> [--assembly <name>] [--diag-json]\n";
    return 0;
  }

  DiagnosticStream diags;
  bool phase1_ok = false;
  bool resolve_ok = false;
  bool typecheck_ok = false;
  bool phase4_ok = false;
  bool subset_ok = true;
  const std::filesystem::path input_path = opts->input_path;
  const auto project_root = cursive0::project::FindProjectRoot(input_path);
  const auto project_result = cursive0::project::LoadProject(project_root, opts->assembly_target);

  for (const auto& diag : project_result.diags) {
    diags = Emit(diags, diag);
  }

  if (!HasError(diags) && project_result.project.has_value()) {
    const auto& project = *project_result.project;
    cursive0::frontend::ParseModuleDeps deps;
    deps.compilation_unit = cursive0::project::CompilationUnit;
    deps.read_bytes = cursive0::frontend::ReadBytesDefault;
    deps.load_source = cursive0::core::LoadSource;
    deps.parse_file = cursive0::syntax::ParseFile;
    deps.inspect_source = InspectC0Subset;
    const auto parsed = cursive0::frontend::ParseModulesWithDeps(project.modules,
                                                                 project.source_root,
                                                                 project.assembly.name,
                                                                 deps);
    for (const auto& diag : parsed.diags) {
      diags = Emit(diags, diag);
    }
    phase1_ok = parsed.modules.has_value();
    subset_ok = parsed.subset_ok;
    if (!parsed.modules.has_value()) {
      SPEC_RULE("ResolveModules-Err-Parse");
    }
    if (!HasError(diags) && parsed.modules.has_value() && !opts->phase1_only) {
      cursive0::sema::ScopeContext ctx;
      ctx.project = &project;
      ctx.sigma.mods = *parsed.modules;
      ctx.scopes = {cursive0::sema::Scope{},
                    cursive0::sema::Scope{},
                    cursive0::sema::Scope{}};
      for (const auto& module : *parsed.modules) {
        ctx.current_module = module.path;
        const auto vis_diags =
            cursive0::sema::CheckModuleVisibility(ctx, module);
        for (const auto& diag : vis_diags) {
          diags = Emit(diags, diag);
        }
      }
      const auto name_maps = cursive0::sema::CollectNameMaps(ctx);
      for (const auto& diag : name_maps.diags) {
        diags = Emit(diags, diag);
      }
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
        for (const auto& diag : resolved.diags) {
          diags = Emit(diags, diag);
        }
        if (resolved.ok) {
          ctx.sigma.mods = resolved.modules;
          cursive0::sema::PopulateSigma(ctx);
        }
        if (!HasError(diags) && resolve_ok) {
          const auto typechecked =
              cursive0::sema::TypecheckModules(ctx, ctx.sigma.mods);
          for (const auto& diag : typechecked.diags) {
            diags = Emit(diags, diag);
          }
          typecheck_ok = typechecked.ok;
          if (typecheck_ok) {
            cursive0::codegen::LowerCtx lower_ctx;
            lower_ctx.sigma = &ctx.sigma;

            const auto* expr_types = &typechecked.expr_types;
            lower_ctx.expr_type =
                [expr_types](const cursive0::syntax::Expr& expr)
                    -> cursive0::sema::TypeRef {
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
                  cursive0::sema::PathKeyOf(lower_ctx.module_path);
              const auto map_it = name_maps.name_maps.find(module_key);
              if (map_it == name_maps.name_maps.end()) {
                return std::nullopt;
              }
              const auto ent_it = map_it->second.find(cursive0::sema::IdKeyOf(name));
              if (ent_it == map_it->second.end()) {
                return std::nullopt;
              }
              const auto& ent = ent_it->second;
              if (ent.kind != cursive0::sema::EntityKind::Value ||
                  !ent.origin_opt.has_value()) {
                return std::nullopt;
              }
              std::vector<std::string> full = *ent.origin_opt;
              const std::string resolved_name = ent.target_opt.value_or(name);
              full.push_back(resolved_name);
              return full;
            };

            if (opts->emit_ir) {
              for (const cursive0::syntax::ASTModule& module : ctx.sigma.mods) {
                lower_ctx.module_path = module.path;
                lower_ctx.resolve_failed = false;
                lower_ctx.codegen_failed = false;
                lower_ctx.resolve_failures.clear();
                auto decls = cursive0::codegen::LowerModule(module, lower_ctx);
                if (lower_ctx.resolve_failed || lower_ctx.codegen_failed) {
                  if (const auto diag = cursive0::core::MakeDiagnostic("E-OUT-0403")) {
                    diags = Emit(diags, *diag);
                  }
                  phase4_ok = false;
                  break;
                }
                std::cout << cursive0::codegen::DumpIR(decls) << "\n";
              }
              if (!HasError(diags)) {
                phase4_ok = true;
              }
            } else if (!opts->no_output) {
              const auto output = cursive0::project::OutputPipeline(project);
              AppendDiags(diags, output.diags);
              phase4_ok = output.artifacts.has_value();
            } else {
              phase4_ok = true;
            }
          }
        }
      }
    }
  }


  PhaseOrderResult phases;
  phases.phase1_ok = phase1_ok;
  phases.phase3_ok = opts->phase1_only ? phase1_ok
                                       : (phase1_ok && resolve_ok && typecheck_ok);
  phases.phase4_ok = opts->phase1_only ? false : phase4_ok;

  ConformanceInput input;
  input.phase_orders = phases;
  input.subset_ok = subset_ok;

  const bool rejected = opts->phase1_only ? false : RejectIllFormed(input);

  if (opts->diag_json) {
    std::cout << DiagnosticStreamToJson(diags) << "\n";
  } else {
    for (const auto& diag : diags) {
      std::cerr << Render(diag) << "\n";
    }
  }

  const CompileStatusResult status = CompileStatus(diags);
  const bool ok = status == CompileStatusResult::Ok && !rejected;
  return ok ? 0 : 1;
}
