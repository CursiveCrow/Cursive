// =================================================================
// File: 03_analysis/types/type_lookup.cpp
// Construct: Type Lookup Utilities
// Spec Section: 5.2.12
// Spec Rules: Various field/record/enum lookups
// =================================================================
#include "cursive0/03_analysis/types/type_lookup.h"

#include "cursive0/00_core/assert_spec.h"
#include "cursive0/03_analysis/resolve/scopes.h"
#include "cursive0/03_analysis/types/type_lower.h"

namespace cursive0::analysis {

namespace {

static inline void SpecDefsTypeLookup() {
  SPEC_DEF("FieldVisible", "5.2.12");
}

syntax::ModulePath ModuleOfRecordPath(const TypePath& path) {
  if (path.size() <= 1) {
    return {};
  }
  return syntax::ModulePath(path.begin(), path.end() - 1);
}

}  // namespace

const syntax::RecordDecl* LookupRecordDecl(const ScopeContext& ctx,
                                           const TypePath& path) {
  syntax::Path syntax_path;
  syntax_path.reserve(path.size());
  for (const auto& comp : path) {
    syntax_path.push_back(comp);
  }
  const auto it = ctx.sigma.types.find(PathKeyOf(syntax_path));
  if (it == ctx.sigma.types.end()) {
    return nullptr;
  }
  return std::get_if<syntax::RecordDecl>(&it->second);
}

const syntax::EnumDecl* LookupEnumDecl(const ScopeContext& ctx,
                                       const TypePath& path) {
  syntax::Path syntax_path;
  syntax_path.reserve(path.size());
  for (const auto& comp : path) {
    syntax_path.push_back(comp);
  }
  const auto it = ctx.sigma.types.find(PathKeyOf(syntax_path));
  if (it == ctx.sigma.types.end()) {
    return nullptr;
  }
  return std::get_if<syntax::EnumDecl>(&it->second);
}

bool FieldExists(const syntax::RecordDecl& record, std::string_view field_name) {
  for (const auto& member : record.members) {
    if (const auto* field = std::get_if<syntax::FieldDecl>(&member)) {
      if (IdEq(field->name, field_name)) {
        return true;
      }
    }
  }
  return false;
}

syntax::Visibility FieldVis(const syntax::RecordDecl& record,
                            std::string_view field_name) {
  SpecDefsTypeLookup();
  for (const auto& member : record.members) {
    if (const auto* field = std::get_if<syntax::FieldDecl>(&member)) {
      if (IdEq(field->name, field_name)) {
        return field->vis;
      }
    }
  }
  return syntax::Visibility::Private;
}

bool FieldVisible(const ScopeContext& ctx,
                  const syntax::RecordDecl& record,
                  std::string_view field_name,
                  const TypePath& record_path) {
  SpecDefsTypeLookup();
  const auto vis = FieldVis(record, field_name);
  if (vis == syntax::Visibility::Public ||
      vis == syntax::Visibility::Internal) {
    return true;
  }
  const auto record_module = ModuleOfRecordPath(record_path);
  return PathEq(record_module, ctx.current_module);
}

std::optional<TypeRef> FieldType(const syntax::RecordDecl& record,
                                 std::string_view field_name,
                                 const ScopeContext& ctx) {
  for (const auto& member : record.members) {
    if (const auto* field = std::get_if<syntax::FieldDecl>(&member)) {
      if (IdEq(field->name, field_name)) {
        const auto lowered = LowerType(ctx, field->type);
        if (lowered.ok) {
          return lowered.type;
        }
        return std::nullopt;
      }
    }
  }
  return std::nullopt;
}

}  // namespace cursive0::analysis
