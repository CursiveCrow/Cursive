#include "cursive0/03_analysis/modal/modal.h"

#include <utility>

#include "cursive0/03_analysis/resolve/scopes.h"
#include "cursive0/03_analysis/resolve/scopes_lookup.h"

namespace cursive0::analysis {

const syntax::ModalDecl* LookupModalDecl(const ScopeContext& ctx,
                                         const TypePath& path) {
  syntax::Path syntax_path;
  syntax_path.reserve(path.size());
  for (const auto& comp : path) {
    syntax_path.push_back(comp);
  }
  const auto it = ctx.sigma.types.find(PathKeyOf(syntax_path));
  if (it == ctx.sigma.types.end()) {
    if (path.size() == 1) {
      const auto ent = ResolveTypeName(ctx, path[0]);
      if (ent.has_value() && ent->origin_opt.has_value()) {
        syntax::Path resolved = *ent->origin_opt;
        resolved.emplace_back(ent->target_opt.value_or(path[0]));
        const auto resolved_it = ctx.sigma.types.find(PathKeyOf(resolved));
        if (resolved_it != ctx.sigma.types.end()) {
          return std::get_if<syntax::ModalDecl>(&resolved_it->second);
        }
      }
    }
    return nullptr;
  }
  return std::get_if<syntax::ModalDecl>(&it->second);
}

const syntax::StateBlock* LookupModalState(const syntax::ModalDecl& decl,
                                           std::string_view state) {
  const auto key = IdKeyOf(state);
  for (const auto& block : decl.states) {
    if (IdKeyOf(block.name) == key) {
      return &block;
    }
  }
  return nullptr;
}

bool HasState(const syntax::ModalDecl& decl, std::string_view state) {
  return LookupModalState(decl, state) != nullptr;
}

std::unordered_set<IdKey> StateNameSet(const syntax::ModalDecl& decl) {
  std::unordered_set<IdKey> out;
  out.reserve(decl.states.size());
  for (const auto& state : decl.states) {
    out.insert(IdKeyOf(state.name));
  }
  return out;
}

}  // namespace cursive0::analysis
