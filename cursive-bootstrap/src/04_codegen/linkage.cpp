#include "cursive0/04_codegen/linkage.h"

#include "cursive0/00_core/assert_spec.h"

namespace cursive0::codegen {

namespace {

// Helper to determine if visibility implies external linkage for user items.
// Per §6.3.4:
//   Vis(item) in {public, internal} -> external
//   Vis(item) = private -> internal
bool IsExternalVisibility(syntax::Visibility vis) {
  return vis == syntax::Visibility::Public ||
         vis == syntax::Visibility::Internal;
}

// Helper for class/modal items where protected also implies external.
// Per §6.3.4:
//   Vis(item) in {public, internal, protected} -> external
//   Vis(item) = private -> internal
bool IsExternalVisibilityWithProtected(syntax::Visibility vis) {
  return vis == syntax::Visibility::Public ||
         vis == syntax::Visibility::Internal ||
         vis == syntax::Visibility::Protected;
}

}  // namespace

// =============================================================================
// User Item Linkage
// =============================================================================

LinkageKind LinkageOf(const syntax::ProcedureDecl& proc) {
  SPEC_DEF("LinkageJudg", "6.3.4");
  SPEC_DEF("Vis", "5.1.4");

  if (IsExternalVisibility(proc.vis)) {
    SPEC_RULE("Linkage-UserItem");
    return LinkageKind::External;
  }

  SPEC_RULE("Linkage-UserItem-Internal");
  return LinkageKind::Internal;
}

LinkageKind LinkageOf(const syntax::StaticDecl& decl) {
  if (IsExternalVisibility(decl.vis)) {
    SPEC_RULE("Linkage-UserItem");
    return LinkageKind::External;
  }

  SPEC_RULE("Linkage-UserItem-Internal");
  return LinkageKind::Internal;
}

LinkageKind LinkageOf(const syntax::MethodDecl& method) {
  if (IsExternalVisibility(method.vis)) {
    SPEC_RULE("Linkage-UserItem");
    return LinkageKind::External;
  }

  SPEC_RULE("Linkage-UserItem-Internal");
  return LinkageKind::Internal;
}

LinkageKind LinkageOfStaticBinding(syntax::Visibility vis) {
  if (IsExternalVisibility(vis)) {
    SPEC_RULE("Linkage-StaticBinding");
    return LinkageKind::External;
  }

  SPEC_RULE("Linkage-StaticBinding-Internal");
  return LinkageKind::Internal;
}

// =============================================================================
// Class Method Linkage
// =============================================================================

LinkageKind LinkageOf(const syntax::ClassMethodDecl& method) {
  // Note: This function should only be called for methods with bodies.
  // Abstract class methods (body_opt = nullptr) don't generate symbols.

  if (IsExternalVisibilityWithProtected(method.vis)) {
    SPEC_RULE("Linkage-ClassMethod");
    return LinkageKind::External;
  }

  SPEC_RULE("Linkage-ClassMethod-Internal");
  return LinkageKind::Internal;
}

// =============================================================================
// State Method and Transition Linkage
// =============================================================================

LinkageKind LinkageOf(const syntax::StateMethodDecl& method) {
  if (IsExternalVisibilityWithProtected(method.vis)) {
    SPEC_RULE("Linkage-StateMethod");
    return LinkageKind::External;
  }

  SPEC_RULE("Linkage-StateMethod-Internal");
  return LinkageKind::Internal;
}

LinkageKind LinkageOf(const syntax::TransitionDecl& trans) {
  if (IsExternalVisibilityWithProtected(trans.vis)) {
    SPEC_RULE("Linkage-Transition");
    return LinkageKind::External;
  }

  SPEC_RULE("Linkage-Transition-Internal");
  return LinkageKind::Internal;
}

// =============================================================================
// Generated Symbol Linkage
// =============================================================================

LinkageKind LinkageOfVTable() {
  SPEC_RULE("Linkage-VTable");
  return LinkageKind::Internal;
}

LinkageKind LinkageOfLiteral() {
  SPEC_RULE("Linkage-LiteralData");
  return LinkageKind::Internal;
}

LinkageKind LinkageOfDropGlue() {
  SPEC_RULE("Linkage-DropGlue");
  return LinkageKind::Internal;
}

LinkageKind LinkageOfDefaultImpl(syntax::Visibility method_vis) {
  if (IsExternalVisibilityWithProtected(method_vis)) {
    SPEC_RULE("Linkage-DefaultImpl");
    return LinkageKind::External;
  }

  SPEC_RULE("Linkage-DefaultImpl-Internal");
  return LinkageKind::Internal;
}

// =============================================================================
// Runtime Symbol Linkage
// =============================================================================

LinkageKind LinkageOfInitFn() {
  SPEC_RULE("Linkage-InitFn");
  return LinkageKind::Internal;
}

LinkageKind LinkageOfDeinitFn() {
  SPEC_RULE("Linkage-DeinitFn");
  return LinkageKind::Internal;
}

LinkageKind LinkageOfPanicSym() {
  SPEC_RULE("Linkage-PanicSym");
  return LinkageKind::Internal;
}

LinkageKind LinkageOfRegionSym() {
  SPEC_RULE("Linkage-RegionSym");
  return LinkageKind::Internal;
}

LinkageKind LinkageOfBuiltinSym() {
  SPEC_RULE("Linkage-BuiltinSym");
  return LinkageKind::Internal;
}

LinkageKind LinkageOfEntrySym() {
  SPEC_RULE("Linkage-EntrySym");
  return LinkageKind::External;
}

// =============================================================================
// Spec Rule Anchors
// =============================================================================

void AnchorLinkageRules() {
  // §6.3.4 Linkage Rules
  SPEC_RULE("Linkage-UserItem");
  SPEC_RULE("Linkage-UserItem-Internal");
  SPEC_RULE("Linkage-StaticBinding");
  SPEC_RULE("Linkage-StaticBinding-Internal");
  SPEC_RULE("Linkage-ClassMethod");
  SPEC_RULE("Linkage-ClassMethod-Internal");
  SPEC_RULE("Linkage-StateMethod");
  SPEC_RULE("Linkage-StateMethod-Internal");
  SPEC_RULE("Linkage-Transition");
  SPEC_RULE("Linkage-Transition-Internal");
  SPEC_RULE("Linkage-InitFn");
  SPEC_RULE("Linkage-DeinitFn");
  SPEC_RULE("Linkage-VTable");
  SPEC_RULE("Linkage-LiteralData");
  SPEC_RULE("Linkage-DropGlue");
  SPEC_RULE("Linkage-DefaultImpl");
  SPEC_RULE("Linkage-DefaultImpl-Internal");
  SPEC_RULE("Linkage-PanicSym");
  SPEC_RULE("Linkage-RegionSym");
  SPEC_RULE("Linkage-BuiltinSym");
  SPEC_RULE("Linkage-EntrySym");
}

}  // namespace cursive0::codegen
