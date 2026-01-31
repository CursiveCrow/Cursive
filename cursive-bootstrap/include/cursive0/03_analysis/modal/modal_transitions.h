#pragma once

#include <string_view>

#include "cursive0/03_analysis/types/context.h"
#include "cursive0/03_analysis/modal/modal.h"
#include "cursive0/03_analysis/resolve/scopes.h"
#include "cursive0/02_syntax/ast.h"

namespace cursive0::analysis {

const syntax::StateMethodDecl* LookupStateMethodDecl(const syntax::ModalDecl& decl,
                                                     std::string_view state,
                                                     std::string_view name);

const syntax::TransitionDecl* LookupTransitionDecl(const syntax::ModalDecl& decl,
                                                   std::string_view state,
                                                   std::string_view name);

bool StateMemberVisible(const ScopeContext& ctx,
                        const TypePath& modal_path,
                        syntax::Visibility vis);

}  // namespace cursive0::analysis
