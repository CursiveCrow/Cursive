#pragma once

#include <cstddef>
#include <optional>
#include <string_view>
#include <vector>

#include "cursive0/core/diagnostics.h"
#include "cursive0/analysis/types/context.h"
#include "cursive0/analysis/resolve/scopes_lookup.h"
#include "cursive0/syntax/ast.h"

namespace cursive0::analysis {

struct BoundName {
  IdKey name;
  Entity ent;
  std::optional<core::Span> span;
};

using BindingList = std::vector<BoundName>;

struct BindingsResult {
  bool ok = false;
  std::optional<std::string_view> diag_id;
  std::optional<core::Span> span;
  BindingList bindings;
};

struct ResolveUsingPathResult {
  bool ok = false;
  std::optional<std::string_view> diag_id;
  bool is_module = false;
  syntax::ModulePath module_path;
  std::optional<syntax::Identifier> item;
};

struct UsingNamesResult {
  bool ok = false;
  std::optional<std::string_view> diag_id;
  std::optional<core::Span> span;
  BindingList bindings;
};

struct ItemOfPathResult {
  bool ok = false;
  syntax::ModulePath module_path;
  syntax::Identifier name;
};

struct CollectNamesResult {
  bool ok = false;
  std::optional<std::string_view> diag_id;
  std::optional<core::Span> span;
  NameMap names;
};

struct NamesState {
  enum class Kind {
    Start,
    Scan,
    Done,
    Error,
  };

  Kind kind = Kind::Start;
  const syntax::ASTModule* module = nullptr;
  std::size_t index = 0;
  NameMap names;
  std::optional<std::string_view> diag_id;
};

struct NameMapBuildResult {
  NameMapTable name_maps;
  core::DiagnosticStream diags;
};

std::vector<syntax::Identifier> PatNames(const syntax::Pattern& pat);
std::vector<syntax::Identifier> PatNames(const syntax::PatternPtr& pat);

std::vector<IdKey> DeclNames(const std::vector<syntax::ASTItem>& items,
                             const syntax::ModulePath& module_path);

std::vector<IdKey> DeclNames(const syntax::ASTModule& module);

ResolveUsingPathResult ResolveUsingPath(const ScopeContext& ctx,
                                        const NameMapTable& name_maps,
                                        const ModuleNames& module_names,
                                        const syntax::ModulePath& path);

ItemOfPathResult ItemOfPath(const ScopeContext& ctx,
                            const NameMapTable& name_maps,
                            const ModuleNames& module_names,
                            const syntax::ModulePath& path);

UsingNamesResult UsingNames(const ScopeContext& ctx,
                            const NameMapTable& name_maps,
                            const ModuleNames& module_names,
                            const syntax::UsingDecl& decl);

BindingsResult ItemBindings(const ScopeContext& ctx,
                            const NameMapTable& name_maps,
                            const ModuleNames& module_names,
                            const syntax::ASTItem& item,
                            const syntax::ModulePath& module_path);

CollectNamesResult CollectNames(const ScopeContext& ctx,
                                const NameMapTable& name_maps,
                                const ModuleNames& module_names,
                                const syntax::ASTModule& module);

NamesState NamesStart(const syntax::ASTModule& module);

NamesState NamesStep(const ScopeContext& ctx,
                     const NameMapTable& name_maps,
                     const ModuleNames& module_names,
                     const NamesState& state);

NameMapBuildResult CollectNameMaps(ScopeContext& ctx);

}  // namespace cursive0::analysis
