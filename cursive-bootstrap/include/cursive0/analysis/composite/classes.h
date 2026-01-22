#pragma once

#include <optional>
#include <string_view>
#include <vector>

#include "cursive0/analysis/types/context.h"
#include "cursive0/analysis/types/types.h"
#include "cursive0/syntax/ast.h"

namespace cursive0::analysis {

struct LinearizationResult {
  bool ok = false;
  std::optional<std::string_view> diag_id;
  std::vector<syntax::ClassPath> order;
};

LinearizationResult LinearizeClass(const ScopeContext& ctx,
                                   const syntax::ClassPath& path);

struct ClassMethodTableResult {
  bool ok = false;
  std::optional<std::string_view> diag_id;
  struct Entry {
    const syntax::ClassMethodDecl* method = nullptr;
    syntax::ClassPath owner;
  };
  std::vector<Entry> methods;
};

struct ClassFieldTableResult {
  bool ok = false;
  std::optional<std::string_view> diag_id;
  std::vector<const syntax::ClassFieldDecl*> fields;
};

ClassMethodTableResult ClassMethodTable(const ScopeContext& ctx,
                                        const syntax::ClassPath& path);

ClassFieldTableResult ClassFieldTable(const ScopeContext& ctx,
                                      const syntax::ClassPath& path);

const syntax::ClassMethodDecl* LookupClassMethod(const ScopeContext& ctx,
                                                 const syntax::ClassPath& path,
                                                 std::string_view name);

bool ClassDispatchable(const ScopeContext& ctx, const syntax::ClassPath& path);

bool ClassSubtypes(const ScopeContext& ctx,
                   const syntax::ClassPath& sub,
                   const syntax::ClassPath& sup);

bool TypeImplementsClass(const ScopeContext& ctx,
                         const TypeRef& type,
                         const syntax::ClassPath& path);

// C0X Extension: Full classes

// Get associated types from a class
std::vector<const syntax::AssociatedTypeDecl*> ClassAssociatedTypes(
    const syntax::ClassDecl& decl);

// Get abstract states from a modal class
std::vector<const syntax::AbstractStateDecl*> ClassAbstractStates(
    const syntax::ClassDecl& decl);

// Check if class is a modal class
bool IsModalClass(const syntax::ClassDecl& decl);

// Check if method is vtable-eligible
bool VTableEligible(const syntax::ClassMethodDecl& method);

// Check if class is dispatchable (for dynamic dispatch)
bool Dispatchable(const ScopeContext& ctx, const syntax::ClassDecl& decl);

// Implementation completeness check
struct CompletenessResult {
  bool ok = true;
  std::optional<std::string_view> diag_id;
  std::vector<std::string> missing_methods;
  std::vector<std::string> missing_types;
  std::vector<std::string> missing_states;
};

CompletenessResult CheckImplCompleteness(
    const ScopeContext& ctx,
    const syntax::ClassPath& class_path,
    const syntax::RecordDecl& impl);

// Orphan rule check
bool CheckOrphanRule(const ScopeContext& ctx,
                     const TypePath& type_path,
                     const syntax::ClassPath& class_path,
                     const syntax::ModulePath& current_module);

}  // namespace cursive0::analysis
