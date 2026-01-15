#pragma once

#include <string_view>
#include <unordered_set>

#include "cursive0/sema/context.h"
#include "cursive0/sema/types.h"
#include "cursive0/syntax/ast.h"

namespace cursive0::sema {

const syntax::ModalDecl* LookupModalDecl(const ScopeContext& ctx,
                                         const TypePath& path);

const syntax::StateBlock* LookupModalState(const syntax::ModalDecl& decl,
                                           std::string_view state);

bool HasState(const syntax::ModalDecl& decl, std::string_view state);

std::unordered_set<IdKey> StateNameSet(const syntax::ModalDecl& decl);

}  // namespace cursive0::sema
