#include "cursive0/analysis/resolve/collect_toplevel.h"

#include <algorithm>
#include <set>
#include <type_traits>

#include "cursive0/core/assert_spec.h"
#include "cursive0/core/diagnostic_messages.h"
#include "cursive0/core/diagnostics.h"
#include "cursive0/core/symbols.h"
#include "cursive0/analysis/resolve/scopes.h"
#include "cursive0/analysis/resolve/visibility.h"

namespace cursive0::analysis {

namespace {

static inline void SpecDefsCollect() {
  SPEC_DEF("BindKind", "5.1.5");
  SPEC_DEF("BindSource", "5.1.5");
  SPEC_DEF("NameInfo", "5.1.5");
  SPEC_DEF("NameMap", "5.1.5");
  SPEC_DEF("AliasMap", "5.1.5");
  SPEC_DEF("UsingMap", "5.1.5");
  SPEC_DEF("UsingValueMap", "5.1.5");
  SPEC_DEF("UsingTypeMap", "5.1.5");
  SPEC_DEF("TypeMap", "5.1.5");
  SPEC_DEF("ClassMap", "5.1.5");
  SPEC_DEF("ModuleNames", "5.1.5");
  SPEC_DEF("IsModulePath", "5.1.5");
  SPEC_DEF("SplitLast", "5.1.5");
  SPEC_DEF("ModuleByPath", "5.1.5");
  SPEC_DEF("ItemNames", "5.1.5");
  SPEC_DEF("UsingSpecName", "5.1.5");
  SPEC_DEF("UsingSpecNames", "5.1.5");
  SPEC_DEF("DeclNames", "5.1.5");
}

static inline void SpecDefsPatNames() {
  SPEC_DEF("PatNames", "5.1.5");
}

bool HasModuleName(const ModuleNames& module_names,
                   const syntax::ModulePath& path) {
  SpecDefsCollect();
  return std::find(module_names.begin(), module_names.end(),
                   core::StringOfPath(path)) != module_names.end();
}

std::optional<std::pair<syntax::ModulePath, syntax::Identifier>> SplitLast(
    const syntax::ModulePath& path) {
  SpecDefsCollect();
  if (path.size() < 2) {
    return std::nullopt;
  }
  syntax::ModulePath prefix(path.begin(), path.end() - 1);
  return std::make_pair(prefix, path.back());
}

const syntax::ASTModule* FindModuleByPath(const ScopeContext& ctx,
                                          const syntax::ModulePath& path) {
  SpecDefsCollect();
  for (const auto& mod : ctx.sigma.mods) {
    if (PathEq(mod.path, path)) {
      return &mod;
    }
  }
  return nullptr;
}

std::optional<syntax::Visibility> ItemVisibility(const syntax::ASTItem& item) {
  return std::visit(
      [](const auto& it) -> std::optional<syntax::Visibility> {
        using T = std::decay_t<decltype(it)>;
        if constexpr (std::is_same_v<T, syntax::ErrorItem>) {
          return std::nullopt;
        } else {
          return it.vis;
        }
      },
      item);
}

bool NameMatches(const IdKey& key, std::string_view candidate) {
  return key == IdKeyOf(candidate);
}

bool PatternBindsName(const syntax::Pattern& pattern, const IdKey& key);

bool PatternBindsName(const syntax::PatternPtr& pattern, const IdKey& key) {
  if (!pattern) {
    return false;
  }
  return PatternBindsName(*pattern, key);
}

bool FieldPatternBindsName(const syntax::FieldPattern& field,
                           const IdKey& key) {
  if (field.pattern_opt) {
    return PatternBindsName(field.pattern_opt, key);
  }
  return NameMatches(key, field.name);
}

bool RecordFieldsBindName(const std::vector<syntax::FieldPattern>& fields,
                          const IdKey& key) {
  for (const auto& field : fields) {
    if (FieldPatternBindsName(field, key)) {
      return true;
    }
  }
  return false;
}

bool PatternBindsName(const syntax::Pattern& pattern, const IdKey& key) {
  return std::visit(
      [&](const auto& node) -> bool {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::IdentifierPattern>) {
          return NameMatches(key, node.name);
        } else if constexpr (std::is_same_v<T, syntax::TypedPattern>) {
          return NameMatches(key, node.name);
        } else if constexpr (std::is_same_v<T, syntax::WildcardPattern> ||
                             std::is_same_v<T, syntax::LiteralPattern>) {
          return false;
        } else if constexpr (std::is_same_v<T, syntax::TuplePattern>) {
          for (const auto& elem : node.elements) {
            if (PatternBindsName(elem, key)) {
              return true;
            }
          }
          return false;
        } else if constexpr (std::is_same_v<T, syntax::RecordPattern>) {
          return RecordFieldsBindName(node.fields, key);
        } else if constexpr (std::is_same_v<T, syntax::EnumPattern>) {
          if (!node.payload_opt) {
            return false;
          }
          return std::visit(
              [&](const auto& payload) -> bool {
                using P = std::decay_t<decltype(payload)>;
                if constexpr (std::is_same_v<P, syntax::TuplePayloadPattern>) {
                  for (const auto& elem : payload.elements) {
                    if (PatternBindsName(elem, key)) {
                      return true;
                    }
                  }
                  return false;
                } else {
                  return RecordFieldsBindName(payload.fields, key);
                }
              },
              *node.payload_opt);
        } else if constexpr (std::is_same_v<T, syntax::ModalPattern>) {
          if (!node.fields_opt) {
            return false;
          }
          return RecordFieldsBindName(node.fields_opt->fields, key);
        } else if constexpr (std::is_same_v<T, syntax::RangePattern>) {
          return PatternBindsName(node.lo, key) || PatternBindsName(node.hi, key);
        } else {
          return false;
        }
      },
      pattern.node);
}

bool UsingClauseBindsName(const syntax::UsingClause& clause,
                          const IdKey& key) {
  return std::visit(
      [&](const auto& node) -> bool {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::UsingPath>) {
          if (node.alias_opt) {
            return NameMatches(key, *node.alias_opt);
          }
          if (node.path.empty()) {
            return false;
          }
          return NameMatches(key, node.path.back());
        } else if constexpr (std::is_same_v<T, syntax::UsingWildcard>) {
          // Wildcard binds all visible names from the module.
          (void)node;
          return true;
        } else {
          for (const auto& spec : node.specs) {
            if (spec.alias_opt) {
              if (NameMatches(key, *spec.alias_opt)) {
                return true;
              }
              continue;
            }
            if (NameMatches(key, spec.name)) {
              return true;
            }
          }
          return false;
        }
      },
      clause);
}

bool ItemBindsName(const syntax::ASTItem& item, const IdKey& key) {
  return std::visit(
      [&](const auto& it) -> bool {
        using T = std::decay_t<decltype(it)>;
        if constexpr (std::is_same_v<T, syntax::UsingDecl>) {
          return UsingClauseBindsName(it.clause, key);
        } else if constexpr (std::is_same_v<T, syntax::StaticDecl>) {
          if (!it.binding.pat) {
            return false;
          }
          return PatternBindsName(*it.binding.pat, key);
        } else if constexpr (std::is_same_v<T, syntax::ProcedureDecl> ||
                             std::is_same_v<T, syntax::RecordDecl> ||
                             std::is_same_v<T, syntax::EnumDecl> ||
                             std::is_same_v<T, syntax::ModalDecl> ||
                             std::is_same_v<T, syntax::ClassDecl> ||
                             std::is_same_v<T, syntax::TypeAliasDecl>) {
          return NameMatches(key, it.name);
        } else {
          return false;
        }
      },
      item);
}

const syntax::ASTItem* FindDeclByName(const ScopeContext& ctx,
                                      const syntax::ModulePath& module_path,
                                      std::string_view name) {
  const auto* module = FindModuleByPath(ctx, module_path);
  if (!module) {
    return nullptr;
  }
  const auto key = IdKeyOf(name);
  const syntax::ASTItem* using_fallback = nullptr;
  for (const auto& item : module->items) {
    if (ItemBindsName(item, key)) {
      if (std::holds_alternative<syntax::UsingDecl>(item)) {
        if (!using_fallback) {
          using_fallback = &item;
        }
        continue;
      }
      return &item;
    }
  }
  return using_fallback;
}

std::optional<core::Span> SpanOfItem(const syntax::ASTItem& item) {
  return std::visit(
      [](const auto& it) -> std::optional<core::Span> { return it.span; },
      item);
}

std::vector<IdKey> NamesOf(const BindingList& bindings) {
  std::vector<IdKey> names;
  names.reserve(bindings.size());
  for (const auto& binding : bindings) {
    names.push_back(binding.name);
  }
  return names;
}

bool NoDup(const BindingList& bindings) {
  std::vector<IdKey> names = NamesOf(bindings);
  std::sort(names.begin(), names.end());
  auto dup = std::adjacent_find(names.begin(), names.end());
  return dup == names.end();
}

bool DisjointNames(const BindingList& bindings, const NameMap& names) {
  for (const auto& binding : bindings) {
    if (names.find(binding.name) != names.end()) {
      return false;
    }
  }
  return true;
}

void InsertBindings(NameMap& out, const BindingList& bindings) {
  for (const auto& binding : bindings) {
    out.emplace(binding.name, binding.ent);
  }
}

bool EntityEquals(const Entity& lhs, const Entity& rhs) {
  if (lhs.kind != rhs.kind) {
    return false;
  }
  if (lhs.source != rhs.source) {
    return false;
  }
  if (lhs.origin_opt.has_value() != rhs.origin_opt.has_value()) {
    return false;
  }
  if (lhs.origin_opt.has_value() &&
      !PathEq(*lhs.origin_opt, *rhs.origin_opt)) {
    return false;
  }
  if (lhs.target_opt.has_value() != rhs.target_opt.has_value()) {
    return false;
  }
  if (lhs.target_opt.has_value() &&
      !IdEq(*lhs.target_opt, *rhs.target_opt)) {
    return false;
  }
  return true;
}

bool NameMapEquals(const NameMap& lhs, const NameMap& rhs) {
  if (lhs.size() != rhs.size()) {
    return false;
  }
  for (const auto& [key, ent] : lhs) {
    const auto it = rhs.find(key);
    if (it == rhs.end()) {
      return false;
    }
    if (!EntityEquals(ent, it->second)) {
      return false;
    }
  }
  return true;
}

std::vector<IdKey> UsingSpecNames(const std::vector<syntax::UsingSpec>& specs) {
  SpecDefsCollect();
  std::vector<IdKey> names;
  names.reserve(specs.size());
  for (const auto& spec : specs) {
    names.push_back(IdKeyOf(spec.name));
  }
  return names;
}

bool DistinctUsingSpecNames(const std::vector<syntax::UsingSpec>& specs) {
  std::vector<IdKey> names = UsingSpecNames(specs);
  std::sort(names.begin(), names.end());
  auto dup = std::adjacent_find(names.begin(), names.end());
  return dup == names.end();
}

std::set<IdKey> ItemNamesOf(const NameMapTable& name_maps,
                            const syntax::ModulePath& module_path) {
  SpecDefsCollect();
  std::set<IdKey> names;
  const auto it = name_maps.find(PathKeyOf(module_path));
  if (it == name_maps.end()) {
    return names;
  }
  for (const auto& [key, ent] : it->second) {
    if (ent.kind == EntityKind::Value || ent.kind == EntityKind::Type ||
        ent.kind == EntityKind::Class) {
      names.insert(key);
    }
  }
  return names;
}

std::optional<std::string_view> CodeForCollectDiag(
    std::string_view diag_id) {
  if (diag_id == "Resolve-Using-None") {
    return "E-MOD-1204";
  }
  if (diag_id == "Resolve-Using-Ambig") {
    return "E-MOD-1208";
  }
  if (diag_id == "Using-List-Dup") {
    return "E-MOD-1206";
  }
  if (diag_id == "Using-Path-Item-Public-Err" ||
      diag_id == "Using-List-Public-Err") {
    return "E-MOD-1205";
  }
  if (diag_id == "Collect-Dup" || diag_id == "Names-Step-Dup") {
    return "E-MOD-1302";
  }
  if (diag_id == "Access-Err") {
    return "E-MOD-1207";
  }
  return std::nullopt;
}

core::DiagnosticStream EmitCollectDiag(core::DiagnosticStream diags,
                                      std::string_view diag_id,
                                      const std::optional<core::Span>& span) {
  const auto code = CodeForCollectDiag(diag_id);
  if (!code.has_value()) {
    return diags;
  }
  if (auto diag = core::MakeDiagnostic(*code, span)) {
    diags = core::Emit(diags, *diag);
  }
  return diags;
}

NameMap BuildDeclNameMap(const syntax::ModulePath& module_path,
                         const std::vector<syntax::ASTItem>& items) {
  NameMap names;
  for (const auto& item : items) {
    std::visit(
        [&](const auto& node) {
          using T = std::decay_t<decltype(node)>;
          if constexpr (std::is_same_v<T, syntax::ProcedureDecl>) {
            names.emplace(IdKeyOf(node.name),
                          Entity{EntityKind::Value, module_path, std::nullopt,
                                 EntitySource::Decl});
          } else if constexpr (std::is_same_v<T, syntax::RecordDecl>) {
            names.emplace(IdKeyOf(node.name),
                          Entity{EntityKind::Type, module_path, std::nullopt,
                                 EntitySource::Decl});
          } else if constexpr (std::is_same_v<T, syntax::EnumDecl>) {
            names.emplace(IdKeyOf(node.name),
                          Entity{EntityKind::Type, module_path, std::nullopt,
                                 EntitySource::Decl});
          } else if constexpr (std::is_same_v<T, syntax::ModalDecl>) {
            names.emplace(IdKeyOf(node.name),
                          Entity{EntityKind::Type, module_path, std::nullopt,
                                 EntitySource::Decl});
          } else if constexpr (std::is_same_v<T, syntax::ClassDecl>) {
            names.emplace(IdKeyOf(node.name),
                          Entity{EntityKind::Class, module_path, std::nullopt,
                                 EntitySource::Decl});
          } else if constexpr (std::is_same_v<T, syntax::TypeAliasDecl>) {
            names.emplace(IdKeyOf(node.name),
                          Entity{EntityKind::Type, module_path, std::nullopt,
                                 EntitySource::Decl});
          } else if constexpr (std::is_same_v<T, syntax::StaticDecl>) {
            if (node.binding.pat) {
              for (const auto& name : PatNames(*node.binding.pat)) {
                names.emplace(IdKeyOf(name),
                              Entity{EntityKind::Value, module_path,
                                     std::nullopt, EntitySource::Decl});
              }
            }
          }
        },
        item);
  }
  return names;
}

ModuleNames ModuleNamesFromContext(const ScopeContext& ctx) {
  if (ctx.project) {
    return ModuleNamesOf(*ctx.project);
  }
  ModuleNames names;
  for (const auto& mod : ctx.sigma.mods) {
    names.push_back(core::StringOfPath(mod.path));
  }
  return names;
}

NameMapTable DeclNameMaps(const ScopeContext& ctx) {
  NameMapTable maps;
  for (const auto& mod : ctx.sigma.mods) {
    maps.emplace(PathKeyOf(mod.path), BuildDeclNameMap(mod.path, mod.items));
  }
  return maps;
}

}  // namespace

std::vector<syntax::Identifier> PatNames(const syntax::Pattern& pat) {
  SpecDefsPatNames();
  return std::visit(
      [&](const auto& node) -> std::vector<syntax::Identifier> {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::IdentifierPattern>) {
          return {node.name};
        } else if constexpr (std::is_same_v<T, syntax::TypedPattern>) {
          SPEC_RULE("Pat-Typed");
          return {node.name};
        } else if constexpr (std::is_same_v<T, syntax::WildcardPattern>) {
          SPEC_RULE("Pat-Wild");
          return {};
        } else if constexpr (std::is_same_v<T, syntax::LiteralPattern>) {
          SPEC_RULE("Pat-Lit");
          return {};
        } else if constexpr (std::is_same_v<T, syntax::TuplePattern>) {
          std::vector<syntax::Identifier> out;
          for (const auto& elem : node.elements) {
            const auto names = PatNames(elem);
            out.insert(out.end(), names.begin(), names.end());
          }
          return out;
        } else if constexpr (std::is_same_v<T, syntax::RecordPattern>) {
          std::vector<syntax::Identifier> out;
          for (const auto& field : node.fields) {
            if (field.pattern_opt) {
              SPEC_RULE("Pat-Record-Field-Explicit");
              const auto names = PatNames(field.pattern_opt);
              out.insert(out.end(), names.begin(), names.end());
            } else {
              SPEC_RULE("Pat-Record-Field-Implicit");
              out.push_back(field.name);
            }
          }
          return out;
        } else if constexpr (std::is_same_v<T, syntax::EnumPattern>) {
          if (!node.payload_opt) {
            SPEC_RULE("Pat-Enum-None");
            return {};
          }
          return std::visit(
              [&](const auto& payload) -> std::vector<syntax::Identifier> {
                using P = std::decay_t<decltype(payload)>;
                if constexpr (std::is_same_v<P, syntax::TuplePayloadPattern>) {
                  SPEC_RULE("Pat-Enum-Tuple");
                  std::vector<syntax::Identifier> out;
                  for (const auto& elem : payload.elements) {
                    const auto names = PatNames(elem);
                    out.insert(out.end(), names.begin(), names.end());
                  }
                  return out;
                } else {
                  SPEC_RULE("Pat-Enum-Record");
                  std::vector<syntax::Identifier> out;
                  for (const auto& field : payload.fields) {
                    if (field.pattern_opt) {
                      SPEC_RULE("Pat-Record-Field-Explicit");
                      const auto names = PatNames(field.pattern_opt);
                      out.insert(out.end(), names.begin(), names.end());
                    } else {
                      SPEC_RULE("Pat-Record-Field-Implicit");
                      out.push_back(field.name);
                    }
                  }
                  return out;
                }
              },
              *node.payload_opt);
        } else if constexpr (std::is_same_v<T, syntax::ModalPattern>) {
          if (!node.fields_opt) {
            return {};
          }
          std::vector<syntax::Identifier> out;
          for (const auto& field : node.fields_opt->fields) {
            if (field.pattern_opt) {
              const auto names = PatNames(field.pattern_opt);
              out.insert(out.end(), names.begin(), names.end());
            } else {
              out.push_back(field.name);
            }
          }
          return out;
        } else if constexpr (std::is_same_v<T, syntax::RangePattern>) {
          SPEC_RULE("Pat-Range");
          std::vector<syntax::Identifier> out = PatNames(node.lo);
          const auto high = PatNames(node.hi);
          out.insert(out.end(), high.begin(), high.end());
          return out;
        } else {
          return {};
        }
      },
      pat.node);
}

std::vector<syntax::Identifier> PatNames(const syntax::PatternPtr& pat) {
  SpecDefsPatNames();
  if (!pat) {
    return {};
  }
  return PatNames(*pat);
}

std::vector<IdKey> DeclNames(const std::vector<syntax::ASTItem>& items,
                             const syntax::ModulePath& module_path) {
  SpecDefsCollect();
  if (items.empty()) {
    SPEC_RULE("DeclNames-Empty");
    return {};
  }
  std::set<IdKey> names;
  for (const auto& item : items) {
    if (std::holds_alternative<syntax::UsingDecl>(item)) {
      SPEC_RULE("DeclNames-Using");
      continue;
    }
    BindingsResult bindings = ItemBindings(ScopeContext{}, NameMapTable{},
                                           ModuleNames{}, item, module_path);
    if (!bindings.ok) {
      continue;
    }
    for (const auto& binding : bindings.bindings) {
      names.insert(binding.name);
    }
    SPEC_RULE("DeclNames-Item");
  }
  return std::vector<IdKey>(names.begin(), names.end());
}

std::vector<IdKey> DeclNames(const syntax::ASTModule& module) {
  SpecDefsCollect();
  return DeclNames(module.items, module.path);
}

ResolveUsingPathResult ResolveUsingPath(const ScopeContext& ctx,
                                        const NameMapTable& name_maps,
                                        const ModuleNames& module_names,
                                        const syntax::ModulePath& path) {
  SpecDefsCollect();
  ResolveUsingPathResult result;
  const auto item = ItemOfPath(ctx, name_maps, module_names, path);
  const bool is_module = HasModuleName(module_names, path);
  result.is_module = is_module;

  if (item.ok && !is_module) {
    SPEC_RULE("Resolve-Using-Item");
    result.ok = true;
    result.module_path = item.module_path;
    result.item = item.name;
    return result;
  }
  if (is_module && !item.ok) {
    SPEC_RULE("Resolve-Using-Module");
    result.ok = true;
    result.module_path = path;
    result.item.reset();
    return result;
  }
  if (is_module && item.ok) {
    SPEC_RULE("Resolve-Using-Ambig");
    result.ok = false;
    result.diag_id = "Resolve-Using-Ambig";
    return result;
  }
  SPEC_RULE("Resolve-Using-None");
  result.ok = false;
  result.diag_id = "Resolve-Using-None";
  return result;
}

ItemOfPathResult ItemOfPath(const ScopeContext& ctx,
                            const NameMapTable& name_maps,
                            const ModuleNames& module_names,
                            const syntax::ModulePath& path) {
  SpecDefsCollect();
  ItemOfPathResult result;
  const auto split = SplitLast(path);
  if (!split.has_value()) {
    SPEC_RULE("ItemOfPath-None");
    return result;
  }
  const auto& module_path = split->first;
  const auto& name = split->second;
  if (!HasModuleName(module_names, module_path)) {
    SPEC_RULE("ItemOfPath-None");
    return result;
  }
  if (!FindModuleByPath(ctx, module_path)) {
    SPEC_RULE("ItemOfPath-None");
    return result;
  }
  const auto item_names = ItemNamesOf(name_maps, module_path);
  if (item_names.find(IdKeyOf(name)) == item_names.end()) {
    SPEC_RULE("ItemOfPath-None");
    return result;
  }
  SPEC_RULE("ItemOfPath");
  result.ok = true;
  result.module_path = module_path;
  result.name = name;
  return result;
}

UsingNamesResult UsingNames(const ScopeContext& ctx,
                            const NameMapTable& name_maps,
                            const ModuleNames& module_names,
                            const syntax::UsingDecl& decl) {
  SpecDefsCollect();
  UsingNamesResult result;
  return std::visit(
      [&](const auto& clause) -> UsingNamesResult {
        using T = std::decay_t<decltype(clause)>;
        if constexpr (std::is_same_v<T, syntax::UsingPath>) {
          const auto resolved =
              ResolveUsingPath(ctx, name_maps, module_names, clause.path);
          if (!resolved.ok) {
            return {false, resolved.diag_id, decl.span, {}};
          }
          if (resolved.item.has_value()) {
            const auto map_it = name_maps.find(PathKeyOf(resolved.module_path));
            if (map_it == name_maps.end()) {
              return {false, "Resolve-Using-None", decl.span, {}};
            }
            const auto ent_it = map_it->second.find(IdKeyOf(*resolved.item));
            if (ent_it == map_it->second.end()) {
              return {false, "Resolve-Using-None", decl.span, {}};
            }
            const auto kind = ent_it->second.kind;
            if (kind != EntityKind::Value && kind != EntityKind::Type &&
                kind != EntityKind::Class) {
              return {false, "Resolve-Using-None", decl.span, {}};
            }
            const auto* decl_item = FindDeclByName(ctx, resolved.module_path,
                                                   *resolved.item);
            if (decl.vis == syntax::Visibility::Public) {
              if (!decl_item || ItemVisibility(*decl_item) !=
                                   syntax::Visibility::Public) {
                SPEC_RULE("Using-Path-Item-Public-Err");
                return {false, "Using-Path-Item-Public-Err", decl.span, {}};
              }
            }
            const auto access =
                CanAccess(ctx, resolved.module_path, *resolved.item);
            if (!access.ok) {
              return {false, access.diag_id, decl.span, {}};
            }
            SPEC_RULE("Using-Path-Item");
            const auto bind_name = clause.alias_opt.value_or(*resolved.item);
            BindingList bindings;
            bindings.push_back(BoundName{
                IdKeyOf(bind_name),
                Entity{kind, resolved.module_path, *resolved.item,
                       EntitySource::Using},
                decl.span,
            });
            return {true, std::nullopt, std::nullopt, bindings};
          }
          SPEC_RULE("Using-Path-Module");
          syntax::Identifier bind_name = clause.alias_opt.value_or(
              resolved.module_path.empty() ? syntax::Identifier{}
                                            : resolved.module_path.back());
          BindingList bindings;
          bindings.push_back(BoundName{
              IdKeyOf(bind_name),
              Entity{EntityKind::ModuleAlias, resolved.module_path, std::nullopt,
                     EntitySource::Using},
              decl.span,
          });
          return {true, std::nullopt, std::nullopt, bindings};
        } else if constexpr (std::is_same_v<T, syntax::UsingWildcard>) {
          if (!HasModuleName(module_names, clause.module_path)) {
            return {false, "Resolve-Using-None", decl.span, {}};
          }
          const auto map_it = name_maps.find(PathKeyOf(clause.module_path));
          if (map_it == name_maps.end()) {
            return {false, "Resolve-Using-None", decl.span, {}};
          }
          BindingList bindings;
          for (const auto& [key, ent] : map_it->second) {
            if (ent.kind != EntityKind::Value && ent.kind != EntityKind::Type &&
                ent.kind != EntityKind::Class) {
              continue;
            }
            const std::string name = key;
            if (decl.vis == syntax::Visibility::Public) {
              const auto* decl_item =
                  FindDeclByName(ctx, clause.module_path, name);
              if (!decl_item || ItemVisibility(*decl_item) !=
                                   syntax::Visibility::Public) {
                SPEC_RULE("Using-List-Public-Err");
                return {false, "Using-List-Public-Err", decl.span, {}};
              }
            }
            const auto access = CanAccess(ctx, clause.module_path, name);
            if (!access.ok) {
              return {false, access.diag_id, decl.span, {}};
            }
            bindings.push_back(BoundName{
                IdKeyOf(name),
                Entity{ent.kind, clause.module_path, name, EntitySource::Using},
                decl.span,
            });
          }
          SPEC_RULE("Using-Wildcard");
          return {true, std::nullopt, std::nullopt, bindings};
        } else {
          if (!HasModuleName(module_names, clause.module_path)) {
            return {false, "Resolve-Using-None", decl.span, {}};
          }
          if (!DistinctUsingSpecNames(clause.specs)) {
            SPEC_RULE("Using-List-Dup");
            return {false, "Using-List-Dup", decl.span, {}};
          }
          const auto map_it = name_maps.find(PathKeyOf(clause.module_path));
          if (map_it == name_maps.end()) {
            return {false, "Resolve-Using-None", decl.span, {}};
          }
          std::optional<syntax::UsingSpec> self_spec;
          for (const auto& spec : clause.specs) {
            if (IdEq(spec.name, "self")) {
              if (self_spec.has_value()) {
                SPEC_RULE("Using-List-Dup");
                return {false, "Using-List-Dup", decl.span, {}};
              }
              self_spec = spec;
            }
          }
          BindingList bindings;
          if (self_spec.has_value()) {
            syntax::Identifier bind_name = self_spec->alias_opt.value_or(
                clause.module_path.empty() ? syntax::Identifier{}
                                           : clause.module_path.back());
            bindings.push_back(BoundName{
                IdKeyOf(bind_name),
                Entity{EntityKind::ModuleAlias, clause.module_path, std::nullopt,
                       EntitySource::Using},
                decl.span,
            });
          }
          for (const auto& spec : clause.specs) {
            if (IdEq(spec.name, "self")) {
              continue;
            }
            const auto ent_it = map_it->second.find(IdKeyOf(spec.name));
            if (ent_it == map_it->second.end()) {
              return {false, "Resolve-Using-None", decl.span, {}};
            }
            const auto kind = ent_it->second.kind;
            if (kind != EntityKind::Value && kind != EntityKind::Type &&
                kind != EntityKind::Class) {
              return {false, "Resolve-Using-None", decl.span, {}};
            }
            if (decl.vis == syntax::Visibility::Public) {
              const auto* decl_item =
                  FindDeclByName(ctx, clause.module_path, spec.name);
              if (!decl_item || ItemVisibility(*decl_item) !=
                                   syntax::Visibility::Public) {
                SPEC_RULE("Using-List-Public-Err");
                return {false, "Using-List-Public-Err", decl.span, {}};
              }
            }
            const auto access = CanAccess(ctx, clause.module_path, spec.name);
            if (!access.ok) {
              return {false, access.diag_id, decl.span, {}};
            }
            const auto bind_name = spec.alias_opt.value_or(spec.name);
            bindings.push_back(BoundName{
                IdKeyOf(bind_name),
                Entity{kind, clause.module_path, spec.name, EntitySource::Using},
                decl.span,
            });
          }
          SPEC_RULE("Using-List");
          return {true, std::nullopt, std::nullopt, bindings};
        }
      },
      decl.clause);
}

BindingsResult ItemBindings(const ScopeContext& ctx,
                            const NameMapTable& name_maps,
                            const ModuleNames& module_names,
                            const syntax::ASTItem& item,
                            const syntax::ModulePath& module_path) {
  SpecDefsCollect();
  return std::visit(
      [&](const auto& node) -> BindingsResult {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::UsingDecl>) {
          UsingNamesResult names = UsingNames(ctx, name_maps, module_names, node);
          if (!names.ok) {
            SPEC_RULE("Bind-Using-Err");
            return {false, names.diag_id, names.span, {}};
          }
          SPEC_RULE("Bind-Using");
          return {true, std::nullopt, std::nullopt, names.bindings};
        } else if constexpr (std::is_same_v<T, syntax::ProcedureDecl>) {
          SPEC_RULE("Bind-Procedure");
          return {true,
                  std::nullopt,
                  std::nullopt,
                  {BoundName{IdKeyOf(node.name),
                             Entity{EntityKind::Value, module_path, std::nullopt,
                                    EntitySource::Decl},
                             node.span}}};
        } else if constexpr (std::is_same_v<T, syntax::RecordDecl>) {
          SPEC_RULE("Bind-Record");
          return {true,
                  std::nullopt,
                  std::nullopt,
                  {BoundName{IdKeyOf(node.name),
                             Entity{EntityKind::Type, module_path, std::nullopt,
                                    EntitySource::Decl},
                             node.span}}};
        } else if constexpr (std::is_same_v<T, syntax::EnumDecl>) {
          SPEC_RULE("Bind-Enum");
          return {true,
                  std::nullopt,
                  std::nullopt,
                  {BoundName{IdKeyOf(node.name),
                             Entity{EntityKind::Type, module_path, std::nullopt,
                                    EntitySource::Decl},
                             node.span}}};
        } else if constexpr (std::is_same_v<T, syntax::ModalDecl>) {
          return {true,
                  std::nullopt,
                  std::nullopt,
                  {BoundName{IdKeyOf(node.name),
                             Entity{EntityKind::Type, module_path, std::nullopt,
                                    EntitySource::Decl},
                             node.span}}};
        } else if constexpr (std::is_same_v<T, syntax::ClassDecl>) {
          SPEC_RULE("Bind-Class");
          return {true,
                  std::nullopt,
                  std::nullopt,
                  {BoundName{IdKeyOf(node.name),
                             Entity{EntityKind::Class, module_path, std::nullopt,
                                    EntitySource::Decl},
                             node.span}}};
        } else if constexpr (std::is_same_v<T, syntax::TypeAliasDecl>) {
          SPEC_RULE("Bind-TypeAlias");
          return {true,
                  std::nullopt,
                  std::nullopt,
                  {BoundName{IdKeyOf(node.name),
                             Entity{EntityKind::Type, module_path, std::nullopt,
                                    EntitySource::Decl},
                             node.span}}};
        } else if constexpr (std::is_same_v<T, syntax::StaticDecl>) {
          SPEC_RULE("Bind-Static");
          BindingList bindings;
          if (node.binding.pat) {
            for (const auto& name : PatNames(*node.binding.pat)) {
              bindings.push_back(
                  BoundName{IdKeyOf(name),
                            Entity{EntityKind::Value, module_path, std::nullopt,
                                   EntitySource::Decl},
                            node.span});
            }
          }
          return {true, std::nullopt, std::nullopt, bindings};
        } else if constexpr (std::is_same_v<T, syntax::ErrorItem>) {
          SPEC_RULE("Bind-ErrorItem");
          return {true, std::nullopt, std::nullopt, {}};
        } else {
          return {true, std::nullopt, std::nullopt, {}};
        }
      },
      item);
}

CollectNamesResult CollectNames(const ScopeContext& ctx,
                                const NameMapTable& name_maps,
                                const ModuleNames& module_names,
                                const syntax::ASTModule& module) {
  SpecDefsCollect();
  NameMap names;
  for (const auto& item : module.items) {
    const auto bindings =
        ItemBindings(ctx, name_maps, module_names, item, module.path);
    if (!bindings.ok) {
      SPEC_RULE("Collect-Err");
      return {false, bindings.diag_id, bindings.span, {}};
    }
    if (!DisjointNames(bindings.bindings, names) ||
        !NoDup(bindings.bindings)) {
      SPEC_RULE("Collect-Dup");
      return {false, "Collect-Dup", SpanOfItem(item), {}};
    }
    SPEC_RULE("Collect-Scan");
    InsertBindings(names, bindings.bindings);
  }
  SPEC_RULE("Collect-Ok");
  return {true, std::nullopt, std::nullopt, names};
}

NamesState NamesStart(const syntax::ASTModule& module) {
  SpecDefsCollect();
  SPEC_RULE("Names-Start");
  NamesState state;
  state.kind = NamesState::Kind::Scan;
  state.module = &module;
  state.index = 0;
  state.names = NameMap{};
  return state;
}

NamesState NamesStep(const ScopeContext& ctx,
                     const NameMapTable& name_maps,
                     const ModuleNames& module_names,
                     const NamesState& state) {
  SpecDefsCollect();
  if (state.kind == NamesState::Kind::Error ||
      state.kind == NamesState::Kind::Done) {
    return state;
  }
  if (state.kind == NamesState::Kind::Start) {
    return NamesStart(*state.module);
  }
  NamesState next = state;
  if (!state.module) {
    next.kind = NamesState::Kind::Error;
    next.diag_id = "Names-Step-Err";
    return next;
  }
  const auto& items = state.module->items;
  if (state.index >= items.size()) {
    SPEC_RULE("Names-Done");
    next.kind = NamesState::Kind::Done;
    return next;
  }
  const auto& item = items[state.index];
  const auto bindings =
      ItemBindings(ctx, name_maps, module_names, item, state.module->path);
  if (!bindings.ok) {
    SPEC_RULE("Names-Step-Err");
    next.kind = NamesState::Kind::Error;
    next.diag_id = bindings.diag_id;
    return next;
  }
  if (!DisjointNames(bindings.bindings, state.names) ||
      !NoDup(bindings.bindings)) {
    SPEC_RULE("Names-Step-Dup");
    next.kind = NamesState::Kind::Error;
    next.diag_id = "Names-Step-Dup";
    return next;
  }
  SPEC_RULE("Names-Step");
  InsertBindings(next.names, bindings.bindings);
  next.index++;
  return next;
}

NameMapBuildResult CollectNameMaps(ScopeContext& ctx) {
  NameMapBuildResult result;
  const ModuleNames module_names = ModuleNamesFromContext(ctx);
  NameMapTable current = DeclNameMaps(ctx);
  std::map<PathKey, CollectNamesResult> last_results;
  bool changed = false;
  do {
    changed = false;
    last_results.clear();
    NameMapTable next = current;
    for (const auto& module : ctx.sigma.mods) {
      ctx.current_module = module.path;
      const auto collected =
          CollectNames(ctx, current, module_names, module);
      const auto key = PathKeyOf(module.path);
      last_results.emplace(key, collected);
      if (!collected.ok) {
        continue;
      }
      auto it = current.find(key);
      if (it == current.end() || !NameMapEquals(it->second, collected.names)) {
        next[key] = collected.names;
        changed = true;
      }
    }
    current = std::move(next);
  } while (changed);

  for (const auto& module : ctx.sigma.mods) {
    const auto key = PathKeyOf(module.path);
    const auto it = last_results.find(key);
    if (it != last_results.end() && !it->second.ok &&
        it->second.diag_id.has_value()) {
      result.diags =
          EmitCollectDiag(result.diags, *it->second.diag_id, it->second.span);
    }
  }

  result.name_maps = std::move(current);
  return result;
}

}  // namespace cursive0::analysis
