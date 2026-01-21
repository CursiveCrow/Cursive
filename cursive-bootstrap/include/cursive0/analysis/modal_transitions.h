#pragma once

#include <string_view>

#include "cursive0/sema/context.h"
#include "cursive0/sema/modal.h"
#include "cursive0/sema/scopes.h"
#include "cursive0/syntax/ast.h"

namespace cursive0::sema {

const syntax::StateMethodDecl* LookupStateMethodDecl(const syntax::ModalDecl& decl,
                                                     std::string_view state,
                                                     std::string_view name);

const syntax::TransitionDecl* LookupTransitionDecl(const syntax::ModalDecl& decl,
                                                   std::string_view state,
                                                   std::string_view name);

bool StateMemberVisible(const ScopeContext& ctx,
                        const TypePath& modal_path,
                        syntax::Visibility vis);

}  // namespace cursive0::sema
