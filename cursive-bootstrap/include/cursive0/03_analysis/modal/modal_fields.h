#pragma once

#include <string_view>

#include "cursive0/03_analysis/types/context.h"
#include "cursive0/03_analysis/modal/modal.h"
#include "cursive0/03_analysis/resolve/scopes.h"
#include "cursive0/03_analysis/types/types.h"
#include "cursive0/02_syntax/ast.h"

namespace cursive0::analysis {

const syntax::StateFieldDecl* LookupModalFieldDecl(const syntax::ModalDecl& decl,
                                                   std::string_view state,
                                                   std::string_view name);

bool ModalFieldVisible(const ScopeContext& ctx, const TypePath& modal_path);

}  // namespace cursive0::analysis
