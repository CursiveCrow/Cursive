#include "cursive0/03_analysis/modal/modal_transitions.h"

#include "cursive0/00_core/assert_spec.h"

namespace cursive0::analysis {

namespace {

static inline void SpecDefsModalTransitions() {
  SPEC_DEF("Methods", "5.6");
  SPEC_DEF("Transitions", "5.6");
  SPEC_DEF("LookupStateMethod", "5.6");
  SPEC_DEF("LookupTransition", "5.6");
  SPEC_DEF("StateMemberVisible", "5.6");
  SPEC_DEF("MethodSig", "5.6");
  SPEC_DEF("TransitionSig", "5.6");
}

static syntax::ModulePath ModuleOfModalPath(const TypePath& path) {
  if (path.size() <= 1) {
    return {};
  }
  return syntax::ModulePath(path.begin(), path.end() - 1);
}

}  // namespace

const syntax::StateMethodDecl* LookupStateMethodDecl(const syntax::ModalDecl& decl,
                                                     std::string_view state,
                                                     std::string_view name) {
  SpecDefsModalTransitions();
  const auto* block = LookupModalState(decl, state);
  if (!block) {
    return nullptr;
  }
  for (const auto& member : block->members) {
    const auto* method = std::get_if<syntax::StateMethodDecl>(&member);
    if (!method) {
      continue;
    }
    if (IdEq(method->name, name)) {
      return method;
    }
  }
  return nullptr;
}

const syntax::TransitionDecl* LookupTransitionDecl(const syntax::ModalDecl& decl,
                                                   std::string_view state,
                                                   std::string_view name) {
  SpecDefsModalTransitions();
  const auto* block = LookupModalState(decl, state);
  if (!block) {
    return nullptr;
  }
  for (const auto& member : block->members) {
    const auto* transition = std::get_if<syntax::TransitionDecl>(&member);
    if (!transition) {
      continue;
    }
    if (IdEq(transition->name, name)) {
      return transition;
    }
  }
  return nullptr;
}

bool StateMemberVisible(const ScopeContext& ctx,
                        const TypePath& modal_path,
                        syntax::Visibility vis) {
  SpecDefsModalTransitions();
  if (vis == syntax::Visibility::Public || vis == syntax::Visibility::Internal) {
    return true;
  }
  const auto modal_module = ModuleOfModalPath(modal_path);
  return PathEq(modal_module, ctx.current_module);
}

}  // namespace cursive0::analysis
