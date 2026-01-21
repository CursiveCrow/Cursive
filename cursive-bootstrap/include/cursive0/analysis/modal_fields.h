#pragma once

#include <string_view>

#include "cursive0/sema/context.h"
#include "cursive0/sema/modal.h"
#include "cursive0/sema/scopes.h"
#include "cursive0/sema/types.h"
#include "cursive0/syntax/ast.h"

namespace cursive0::sema {

const syntax::StateFieldDecl* LookupModalFieldDecl(const syntax::ModalDecl& decl,
                                                   std::string_view state,
                                                   std::string_view name);

bool ModalFieldVisible(const ScopeContext& ctx, const TypePath& modal_path);

}  // namespace cursive0::sema
