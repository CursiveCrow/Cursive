#pragma once

#include <optional>
#include <string_view>
#include <vector>

#include "cursive0/sema/context.h"
#include "cursive0/sema/types.h"
#include "cursive0/syntax/ast.h"

namespace cursive0::sema {

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

}  // namespace cursive0::sema
