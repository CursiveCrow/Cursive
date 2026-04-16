// =================================================================
// File: 04_analysis/typing/type_lookup.cpp
// Construct: Type Lookup Utilities
// Spec Section: 5.2.12
// Spec Rules: Various field/record/enum lookups
// =================================================================
//
// MIGRATED FROM: cursive-bootstrap/src/03_analysis/types/type_lookup.cpp
//
// =================================================================

#include "04_analysis/typing/type_lookup.h"

#include <mutex>
#include <unordered_map>

#include "00_core/assert_spec.h"
#include "00_core/symbols.h"
#include "04_analysis/resolve/scopes.h"
#include "04_analysis/typing/type_lower.h"

namespace cursive::analysis {

namespace {

static inline void SpecDefsTypeLookup() {
  SPEC_DEF("FieldVisible", "5.2.12");
  SPEC_DEF("FieldExists", "5.2.12");
  SPEC_DEF("FieldType", "5.2.12");
  SPEC_DEF("LookupRecordDecl", "5.2.12");
  SPEC_DEF("LookupEnumDecl", "5.2.12");
  SPEC_DEF("TypeParamsOf", "14.1.3");
  SPEC_DEF("TypePredicateClauseOf", "14.1.3");
}

struct RecordFieldIndex {
  std::unordered_map<std::string, const ast::FieldDecl*> by_name;
};

std::mutex g_record_field_index_mu;
std::unordered_map<const ast::RecordDecl*, RecordFieldIndex> g_record_field_index;

const ast::FieldDecl* LookupFieldDecl(const ast::RecordDecl& record,
                                      std::string_view field_name) {
  const auto key = IdKeyOf(field_name);
  {
    std::lock_guard<std::mutex> lock(g_record_field_index_mu);
    const auto it = g_record_field_index.find(&record);
    if (it != g_record_field_index.end()) {
      const auto field_it = it->second.by_name.find(key);
      return field_it != it->second.by_name.end() ? field_it->second : nullptr;
    }
  }

  RecordFieldIndex index;
  for (const auto& member : record.members) {
    const auto* field = std::get_if<ast::FieldDecl>(&member);
    if (!field) {
      continue;
    }
    index.by_name.emplace(IdKeyOf(field->name), field);
  }

  std::lock_guard<std::mutex> lock(g_record_field_index_mu);
  auto [it, _inserted] = g_record_field_index.emplace(&record, std::move(index));
  const auto field_it = it->second.by_name.find(key);
  return field_it != it->second.by_name.end() ? field_it->second : nullptr;
}

}  // namespace

const ast::RecordDecl* LookupRecordDecl(const ScopeContext& ctx,
                                        const TypePath& path) {
  SpecDefsTypeLookup();
  ast::Path syntax_path;
  syntax_path.reserve(path.size());
  for (const auto& comp : path) {
    syntax_path.push_back(comp);
  }
  const auto it = ctx.sigma.types.find(PathKeyOf(syntax_path));
  if (it == ctx.sigma.types.end()) {
    return nullptr;
  }
  return std::get_if<ast::RecordDecl>(&it->second);
}

const ast::EnumDecl* LookupEnumDecl(const ScopeContext& ctx,
                                    const TypePath& path) {
  SpecDefsTypeLookup();
  ast::Path syntax_path;
  syntax_path.reserve(path.size());
  for (const auto& comp : path) {
    syntax_path.push_back(comp);
  }
  const auto it = ctx.sigma.types.find(PathKeyOf(syntax_path));
  if (it == ctx.sigma.types.end()) {
    return nullptr;
  }
  return std::get_if<ast::EnumDecl>(&it->second);
}

const std::optional<ast::GenericParams>* TypeParamsOf(const ScopeContext& ctx,
                                                      const TypePath& path) {
  SpecDefsTypeLookup();
  ast::Path syntax_path;
  syntax_path.reserve(path.size());
  for (const auto& comp : path) {
    syntax_path.push_back(comp);
  }
  const auto it = ctx.sigma.types.find(PathKeyOf(syntax_path));
  if (it == ctx.sigma.types.end()) {
    return nullptr;
  }

  return std::visit(
      [](const auto& decl) -> const std::optional<ast::GenericParams>* {
        return &decl.generic_params;
      },
      it->second);
}

const std::optional<ast::WhereClause>* TypePredicateClauseOf(
    const ScopeContext& ctx,
    const TypePath& path) {
  SpecDefsTypeLookup();
  ast::Path syntax_path;
  syntax_path.reserve(path.size());
  for (const auto& comp : path) {
    syntax_path.push_back(comp);
  }
  const auto it = ctx.sigma.types.find(PathKeyOf(syntax_path));
  if (it == ctx.sigma.types.end()) {
    return nullptr;
  }

  return std::visit(
      [](const auto& decl) -> const std::optional<ast::WhereClause>* {
        return &decl.predicate_clause_opt;
      },
      it->second);
}

bool FieldExists(const ast::RecordDecl& record, std::string_view field_name) {
  SpecDefsTypeLookup();
  return LookupFieldDecl(record, field_name) != nullptr;
}

ast::Visibility FieldVis(const ast::RecordDecl& record,
                         std::string_view field_name) {
  SpecDefsTypeLookup();
  const auto* field = LookupFieldDecl(record, field_name);
  return field ? field->vis : ast::Visibility::Private;
}

bool FieldVisible(const ScopeContext& ctx,
                  const ast::RecordDecl& record,
                  std::string_view field_name,
                  const TypePath& record_path) {
  SpecDefsTypeLookup();
  const auto vis = FieldVis(record, field_name);
  if (vis == ast::Visibility::Public || vis == ast::Visibility::Internal) {
    return true;
  }
  // Protected: visible in declaring module and submodules
  // Private: visible only in declaring module
  if (record_path.empty()) {
    return false;
  }
  ast::ModulePath declaring_module = record_path;
  declaring_module.pop_back();  // Remove the record name to get the module
  const auto& current = ctx.current_module;
  if (declaring_module == current) {
    return true;  // Same module: both protected and private are visible
  }
  return false;
}

std::optional<TypeRef> FieldType(const ast::RecordDecl& record,
                                 std::string_view field_name,
                                 const ScopeContext& ctx) {
  SpecDefsTypeLookup();
  const auto* field = LookupFieldDecl(record, field_name);
  if (!field) {
    return std::nullopt;
  }
  const auto lowered = LowerType(ctx, field->type);
  if (lowered.ok) {
    return lowered.type;
  }
  return std::nullopt;
}

}  // namespace cursive::analysis
