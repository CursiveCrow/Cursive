#include "cursive0/sema/region_type.h"

namespace cursive0::sema {

syntax::ModalDecl BuildRegionModalDecl() {
  syntax::ModalDecl decl{};
  decl.vis = syntax::Visibility::Public;
  decl.name = "Region";
  decl.implements = {};
  decl.doc = {};

  auto make_state = [](const char* name) {
    syntax::StateBlock state{};
    state.name = name;
    state.members = {};
    state.doc_opt = std::nullopt;
    return state;
  };

  decl.states = {make_state("Active"), make_state("Frozen"), make_state("Freed")};
  return decl;
}

}  // namespace cursive0::sema
