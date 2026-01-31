// =================================================================
// File: 03_analysis/types/type_lookup.h
// Construct: Type Lookup Utilities
// Spec Section: 5.2.12
// Spec Rules: Various field/record/enum lookups
// =================================================================
#pragma once

#include <optional>
#include <string_view>

#include "cursive0/02_syntax/ast.h"
#include "cursive0/03_analysis/types/context.h"
#include "cursive0/03_analysis/types/types.h"

namespace cursive0::analysis {

// Record lookup by TypePath
const syntax::RecordDecl* LookupRecordDecl(const ScopeContext& ctx,
                                           const TypePath& path);

// Enum lookup by TypePath
const syntax::EnumDecl* LookupEnumDecl(const ScopeContext& ctx,
                                       const TypePath& path);

// Check if a field exists in a record
bool FieldExists(const syntax::RecordDecl& record, std::string_view field_name);

// Get field visibility
syntax::Visibility FieldVis(const syntax::RecordDecl& record,
                            std::string_view field_name);

// Check if a field is visible from current context
bool FieldVisible(const ScopeContext& ctx,
                  const syntax::RecordDecl& record,
                  std::string_view field_name,
                  const TypePath& record_path);

// Get field type (requires LowerType internally)
std::optional<TypeRef> FieldType(const syntax::RecordDecl& record,
                                 std::string_view field_name,
                                 const ScopeContext& ctx);

}  // namespace cursive0::analysis
