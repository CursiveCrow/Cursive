#pragma once

#include <string_view>
#include <unordered_set>

#include "cursive0/03_analysis/types/context.h"
#include "cursive0/03_analysis/types/types.h"
#include "cursive0/02_syntax/ast.h"

namespace cursive0::analysis {

const syntax::ModalDecl* LookupModalDecl(const ScopeContext& ctx,
                                         const TypePath& path);

const syntax::StateBlock* LookupModalState(const syntax::ModalDecl& decl,
                                           std::string_view state);

bool HasState(const syntax::ModalDecl& decl, std::string_view state);

std::unordered_set<IdKey> StateNameSet(const syntax::ModalDecl& decl);

}  // namespace cursive0::analysis
