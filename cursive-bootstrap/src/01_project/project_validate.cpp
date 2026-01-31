#include "cursive0/01_project/project.h"

#include <string_view>
#include <system_error>
#include <unordered_set>

#include "cursive0/00_core/assert_spec.h"
#include "cursive0/00_core/diagnostic_messages.h"
#include "cursive0/00_core/diagnostics.h"
#include "cursive0/00_core/host_primitives.h"
#include "cursive0/00_core/path.h"
#include "cursive0/01_project/ident.h"

namespace cursive0::project {

namespace {

std::optional<core::Diagnostic> MakeExternalDiag(std::string_view code) {
  auto diag = core::MakeDiagnostic(code);
  if (diag) {
    diag->span.reset();
  }
  return diag;
}

void EmitExternal(core::DiagnosticStream& diags, std::string_view code) {
  if (auto diag = MakeExternalDiag(code)) {
    diags = core::Emit(diags, *diag);
  }
}

bool HasOnlyAssemblyKey(const TOMLTable& table) {
  for (const auto& [key, value] : table) {
    (void)value;
    if (key != "assembly") {
      return false;
    }
  }
  return true;
}

bool IsKnownAssemblyKey(std::string_view key) {
  return key == "name" || key == "kind" || key == "root" ||
         key == "out_dir" || key == "emit_ir";
}

enum class RelPathStatus {
  Ok,
  RelPathErr,
  ResolveErr,
};

RelPathStatus CheckRelPath(std::string_view p, std::string_view root) {
  if (!core::IsRelative(p)) {
    SPEC_RULE("WF-RelPath-Err");
    return RelPathStatus::RelPathErr;
  }
  const auto resolved = core::Resolve(root, p);
  if (!resolved.has_value()) {
    return RelPathStatus::ResolveErr;
  }
  if (!core::Prefix(resolved->path, resolved->root)) {
    SPEC_RULE("WF-RelPath-Err");
    return RelPathStatus::RelPathErr;
  }
  SPEC_RULE("WF-RelPath");
  return RelPathStatus::Ok;
}

struct AsmTablesResult {
  bool ok = false;
  std::vector<const toml::table*> tables;
};

AsmTablesResult AsmTables(const toml::node* assembly_node) {
  AsmTablesResult result;
  if (!assembly_node) {
    return result;
  }
  if (assembly_node->is_table()) {
    const auto* table = assembly_node->as_table();
    if (!table) {
      return result;
    }
    result.ok = true;
    result.tables.push_back(table);
    return result;
  }
  if (assembly_node->is_array_of_tables()) {
    const auto* array = assembly_node->as_array();
    if (!array) {
      return result;
    }
    for (const auto& entry : *array) {
      if (!entry.is_table()) {
        return result;
      }
      result.tables.push_back(entry.as_table());
    }
    result.ok = true;
    return result;
  }
  if (assembly_node->is_array()) {
    const auto* array = assembly_node->as_array();
    if (array && array->empty()) {
      result.ok = true;
      return result;
    }
  }
  return result;
}

}  // namespace

ManifestValidationResult ValidateManifest(const std::filesystem::path& project_root,
                                         const TOMLTable& table) {
  ManifestValidationResult result;

  auto fail = [&](std::string_view code) {
    SPEC_RULE("ValidateManifest-Err");
    EmitExternal(result.diags, code);
    return result;
  };

  if (!HasOnlyAssemblyKey(table)) {
    SPEC_RULE("WF-TopKeys-Err");
    return fail("E-PRJ-0104");
  }
  SPEC_RULE("WF-TopKeys");

  const toml::node* assembly_node = table.get("assembly");
  if (!assembly_node) {
    SPEC_RULE("WF-Assembly-Table-Err");
    return fail("E-PRJ-0103");
  }

  const AsmTablesResult asm_tables = AsmTables(assembly_node);
  if (!asm_tables.ok) {
    SPEC_RULE("WF-Assembly-Table-Err");
    return fail("E-PRJ-0103");
  }
  SPEC_RULE("WF-Assembly-Table");

  if (asm_tables.tables.empty()) {
    SPEC_RULE("WF-Assembly-Count-Err");
    return fail("E-PRJ-0103");
  }
  SPEC_RULE("WF-Assembly-Count");

  std::unordered_set<std::string> names;
  names.reserve(asm_tables.tables.size());

  const std::string root_str = project_root.generic_string();

  for (const auto* assembly : asm_tables.tables) {
    if (!assembly) {
      SPEC_RULE("WF-Assembly-Table-Err");
      return fail("E-PRJ-0103");
    }

    for (const auto& [key, value] : *assembly) {
      (void)value;
      if (!IsKnownAssemblyKey(key)) {
        SPEC_RULE("WF-Assembly-Keys-Err");
        return fail("E-PRJ-0104");
      }
    }
    SPEC_RULE("WF-Assembly-Keys");

    const toml::node* name_node = assembly->get("name");
    const toml::node* kind_node = assembly->get("kind");
    const toml::node* root_node = assembly->get("root");
    if (!name_node || !kind_node || !root_node ||
        !name_node->is_string() || !kind_node->is_string() ||
        !root_node->is_string()) {
      SPEC_RULE("WF-Assembly-Required-Types-Err");
      return fail("E-PRJ-0103");
    }
    SPEC_RULE("WF-Assembly-Required-Types");

    const auto name_value = name_node->value<std::string>();
    const auto kind_value = kind_node->value<std::string>();
    const auto root_value = root_node->value<std::string>();
    if (!name_value || !kind_value || !root_value) {
      SPEC_RULE("WF-Assembly-Required-Types-Err");
      return fail("E-PRJ-0103");
    }

    std::optional<std::string> out_dir_value;
    if (const toml::node* out_dir_node = assembly->get("out_dir")) {
      if (!out_dir_node->is_string()) {
        SPEC_RULE("WF-Assembly-OutDirType-Err");
        return fail("E-PRJ-0301");
      }
      out_dir_value = out_dir_node->value<std::string>();
    }

    std::optional<std::string> emit_ir_value;
    if (const toml::node* emit_ir_node = assembly->get("emit_ir")) {
      if (!emit_ir_node->is_string()) {
        SPEC_RULE("WF-Assembly-EmitIRType-Err");
        return fail("E-PRJ-0204");
      }
      emit_ir_value = emit_ir_node->value<std::string>();
    }

    SPEC_RULE("WF-Assembly-Optional-Types");

    if (!IsName(*name_value)) {
      SPEC_RULE("WF-Assembly-Name-Err");
      return fail("E-PRJ-0203");
    }
    SPEC_RULE("WF-Assembly-Name");

    if (!( *kind_value == "executable" || *kind_value == "library")) {
      SPEC_RULE("WF-Assembly-Kind-Err");
      return fail("E-PRJ-0201");
    }
    SPEC_RULE("WF-Assembly-Kind");

    if (emit_ir_value.has_value()) {
      const auto& emit_ir = *emit_ir_value;
      if (!(emit_ir == "none" || emit_ir == "ll" || emit_ir == "bc")) {
        SPEC_RULE("WF-Assembly-EmitIR-Err");
        return fail("E-PRJ-0204");
      }
    }
    SPEC_RULE("WF-Assembly-EmitIR");

    const RelPathStatus root_status = CheckRelPath(*root_value, root_str);
    if (root_status == RelPathStatus::RelPathErr) {
      SPEC_RULE("WF-Assembly-Root-Path-Err");
      return fail("E-PRJ-0301");
    }
    if (root_status == RelPathStatus::ResolveErr) {
      return fail("E-PRJ-0304");
    }
    SPEC_RULE("WF-Assembly-Root-Path");

    if (out_dir_value.has_value()) {
      const RelPathStatus out_dir_status =
          CheckRelPath(*out_dir_value, root_str);
      if (out_dir_status == RelPathStatus::RelPathErr) {
        SPEC_RULE("WF-Assembly-OutDir-Path-Err");
        return fail("E-PRJ-0301");
      }
      if (out_dir_status == RelPathStatus::ResolveErr) {
        return fail("E-PRJ-0304");
      }
    }
    SPEC_RULE("WF-Assembly-OutDir-Path");

    if (!names.insert(*name_value).second) {
      SPEC_RULE("WF-Assembly-Name-Dup-Err");
      return fail("E-PRJ-0202");
    }

    ValidatedAssembly validated{*name_value,
                                *kind_value,
                                *root_value,
                                out_dir_value,
                                emit_ir_value};
    result.assemblies.push_back(std::move(validated));
  }
  SPEC_RULE("WF-Assembly-Name-Dup");

  SPEC_RULE("ValidateManifest-Ok");
  return result;
}

bool IsProjectRoot(const std::filesystem::path& root) {
  std::error_code ec;
  const bool exists = std::filesystem::exists(root / "Cursive.toml", ec);
  if (ec) {
    core::HostPrimFail(core::HostPrim::FSExists, true);
    return false;
  }
  if (exists) {
    SPEC_RULE("WF-Project-Root");
  }
  return exists;
}

}  // namespace cursive0::project
