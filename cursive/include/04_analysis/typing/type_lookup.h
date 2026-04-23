// =================================================================
// File: 04_analysis/typing/type_lookup.h
// Construct: Type Lookup Utilities
// Spec Section: 5.2.12
// Spec Rules: Various field/record/enum lookups
// =================================================================
#pragma once

#include <optional>
#include <string_view>

#include "02_source/ast/ast.h"
#include "04_analysis/typing/context.h"
#include "04_analysis/typing/types.h"

namespace cursive::analysis {

// Record lookup by TypePath
const ast::RecordDecl* LookupRecordDecl(const ScopeContext& ctx,
                                           const TypePath& path);

// Enum lookup by TypePath
const ast::EnumDecl* LookupEnumDecl(const ScopeContext& ctx,
                                       const TypePath& path);

// Type declaration generic-parameter lookup by TypePath
const std::optional<ast::GenericParams>* TypeParamsOf(const ScopeContext& ctx,
                                                      const TypePath& path);

// Type declaration predicate-clause lookup by TypePath
const std::optional<ast::PredicateClause>* TypePredicateClauseOf(
    const ScopeContext& ctx,
    const TypePath& path);

// Check if a field exists in a record
bool FieldExists(const ast::RecordDecl& record, std::string_view field_name);

// Get field visibility
ast::Visibility FieldVis(const ast::RecordDecl& record,
                            std::string_view field_name);

// Check if a field is visible from current context
bool FieldVisible(const ScopeContext& ctx,
                  const ast::RecordDecl& record,
                  std::string_view field_name,
                  const TypePath& record_path);

// Get field type (requires LowerType internally)
std::optional<TypeRef> FieldType(const ast::RecordDecl& record,
                                 std::string_view field_name,
                                 const ScopeContext& ctx);

}  // namespace cursive::analysis
