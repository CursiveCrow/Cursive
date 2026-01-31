#pragma once

#include <string_view>

#include "cursive0/00_core/diagnostics.h"
#include "cursive0/03_analysis/types/context.h"
#include "cursive0/03_analysis/resolve/scopes_lookup.h"
#include "cursive0/02_syntax/ast.h"

namespace cursive0::analysis {

AccessResult CanAccess(const ScopeContext& ctx,
                       const syntax::ModulePath& module_path,
                       std::string_view name);

AccessResult CanAccessVis(const syntax::ModulePath& accessor_module,
                          const syntax::ModulePath& decl_module,
                          syntax::Visibility vis);

AccessResult TopLevelVis(const syntax::ASTItem& item);

core::DiagnosticStream CheckModuleVisibility(const ScopeContext& ctx,
                                             const syntax::ASTModule& module);

}  // namespace cursive0::analysis
