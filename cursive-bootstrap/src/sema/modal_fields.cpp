#include "cursive0/sema/modal_fields.h"

#include "cursive0/sema/scopes.h"

namespace cursive0::sema {

namespace {

static syntax::ModulePath ModuleOfModalPath(const TypePath& path) {
  if (path.size() <= 1) {
    return {};
  }
  return syntax::ModulePath(path.begin(), path.end() - 1);
}

}  // namespace

const syntax::StateFieldDecl* LookupModalFieldDecl(const syntax::ModalDecl& decl,
                                                   std::string_view state,
                                                   std::string_view name) {
  const auto* block = LookupModalState(decl, state);
  if (!block) {
    return nullptr;
  }
  for (const auto& member : block->members) {
    const auto* field = std::get_if<syntax::StateFieldDecl>(&member);
    if (!field) {
      continue;
    }
    if (IdEq(field->name, name)) {
      return field;
    }
  }
  return nullptr;
}

bool ModalFieldVisible(const ScopeContext& ctx, const TypePath& modal_path) {
  const auto modal_module = ModuleOfModalPath(modal_path);
  return PathEq(modal_module, ctx.current_module);
}

}  // namespace cursive0::sema
