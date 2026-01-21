#include "cursive0/analysis/memory/region_type.h"

namespace cursive0::analysis {

namespace {

static std::shared_ptr<syntax::Type> MakeTypeNode(const syntax::TypeNode& node) {
  auto ty = std::make_shared<syntax::Type>();
  ty->span = core::Span{};
  ty->node = node;
  return ty;
}

static std::shared_ptr<syntax::Type> MakeTypePrimAst(const char* name) {
  return MakeTypeNode(syntax::TypePrim{syntax::Identifier{name}});
}

static syntax::StateFieldDecl MakeStateField(const char* name,
                                             std::shared_ptr<syntax::Type> type) {
  syntax::StateFieldDecl field{};
  field.vis = syntax::Visibility::Public;
  field.name = name;
  field.type = type;
  field.span = core::Span{};
  field.doc_opt = std::nullopt;
  return field;
}

}  // namespace

syntax::ModalDecl BuildRegionModalDecl() {
  syntax::ModalDecl decl{};
  decl.vis = syntax::Visibility::Public;
  decl.name = "Region";
  decl.implements = {};
  decl.doc = {};

  auto handle_type = MakeTypePrimAst("usize");
  auto make_state = [&](const char* name) {
    syntax::StateBlock state{};
    state.name = name;
    state.members = {MakeStateField("handle", handle_type)};
    state.span = core::Span{};
    state.doc_opt = std::nullopt;
    return state;
  };

  decl.states = {make_state("Active"), make_state("Frozen"), make_state("Freed")};
  return decl;
}

}  // namespace cursive0::analysis
