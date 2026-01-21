#pragma once

#include <string_view>

#include "cursive0/analysis/types/context.h"
#include "cursive0/analysis/modal/modal.h"
#include "cursive0/analysis/resolve/scopes.h"
#include "cursive0/analysis/types/types.h"
#include "cursive0/syntax/ast.h"

namespace cursive0::analysis {

const syntax::StateFieldDecl* LookupModalFieldDecl(const syntax::ModalDecl& decl,
                                                   std::string_view state,
                                                   std::string_view name);

bool ModalFieldVisible(const ScopeContext& ctx, const TypePath& modal_path);

}  // namespace cursive0::analysis
