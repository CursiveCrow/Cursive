#include <cassert>

#include "cursive0/codegen/linkage.h"
#include "cursive0/core/assert_spec.h"
#include "cursive0/syntax/ast.h"

namespace {

using cursive0::codegen::AnchorLinkageRules;
using cursive0::codegen::LinkageKind;
using cursive0::codegen::LinkageOf;
using cursive0::codegen::LinkageOfBuiltinSym;
using cursive0::codegen::LinkageOfDeinitFn;
using cursive0::codegen::LinkageOfDefaultImpl;
using cursive0::codegen::LinkageOfDropGlue;
using cursive0::codegen::LinkageOfEntrySym;
using cursive0::codegen::LinkageOfInitFn;
using cursive0::codegen::LinkageOfLiteral;
using cursive0::codegen::LinkageOfPanicSym;
using cursive0::codegen::LinkageOfRegionSym;
using cursive0::codegen::LinkageOfStaticBinding;
using cursive0::codegen::LinkageOfVTable;
using cursive0::syntax::ClassMethodDecl;
using cursive0::syntax::MethodDecl;
using cursive0::syntax::ProcedureDecl;
using cursive0::syntax::StateMethodDecl;
using cursive0::syntax::StaticDecl;
using cursive0::syntax::TransitionDecl;
using cursive0::syntax::Visibility;

// Helper to create a minimal ProcedureDecl
ProcedureDecl MakeProc(Visibility vis) {
  ProcedureDecl proc;
  proc.name = "test_proc";
  proc.vis = vis;
  return proc;
}

// Helper to create a minimal StaticDecl
StaticDecl MakeStatic(Visibility vis) {
  StaticDecl decl;
  decl.vis = vis;
  return decl;
}

// Helper to create a minimal MethodDecl
MethodDecl MakeMethod(Visibility vis) {
  MethodDecl method;
  method.name = "test_method";
  method.vis = vis;
  return method;
}

// Helper to create a minimal ClassMethodDecl
ClassMethodDecl MakeClassMethod(Visibility vis) {
  ClassMethodDecl method;
  method.name = "test_class_method";
  method.vis = vis;
  return method;
}

// Helper to create a minimal StateMethodDecl
StateMethodDecl MakeStateMethod(Visibility vis) {
  StateMethodDecl method;
  method.name = "test_state_method";
  method.vis = vis;
  return method;
}

// Helper to create a minimal TransitionDecl
TransitionDecl MakeTransition(Visibility vis) {
  TransitionDecl trans;
  trans.name = "test_transition";
  trans.target_state = "TargetState";
  trans.vis = vis;
  return trans;
}

// =============================================================================
// User Item Linkage Tests
// =============================================================================

void TestLinkageProcedure() {
  SPEC_COV("Linkage-UserItem");
  SPEC_COV("Linkage-UserItem-Internal");

  // Public -> external
  auto pub_proc = MakeProc(Visibility::Public);
  assert(LinkageOf(pub_proc) == LinkageKind::External);

  // Internal -> external
  auto internal_proc = MakeProc(Visibility::Internal);
  assert(LinkageOf(internal_proc) == LinkageKind::External);

  // Private -> internal
  auto private_proc = MakeProc(Visibility::Private);
  assert(LinkageOf(private_proc) == LinkageKind::Internal);
}

void TestLinkageStatic() {
  SPEC_COV("Linkage-UserItem");
  SPEC_COV("Linkage-UserItem-Internal");

  // Public -> external
  auto pub_static = MakeStatic(Visibility::Public);
  assert(LinkageOf(pub_static) == LinkageKind::External);

  // Internal -> external
  auto internal_static = MakeStatic(Visibility::Internal);
  assert(LinkageOf(internal_static) == LinkageKind::External);

  // Private -> internal
  auto private_static = MakeStatic(Visibility::Private);
  assert(LinkageOf(private_static) == LinkageKind::Internal);
}

void TestLinkageMethod() {
  SPEC_COV("Linkage-UserItem");
  SPEC_COV("Linkage-UserItem-Internal");

  // Public -> external
  auto pub_method = MakeMethod(Visibility::Public);
  assert(LinkageOf(pub_method) == LinkageKind::External);

  // Internal -> external
  auto internal_method = MakeMethod(Visibility::Internal);
  assert(LinkageOf(internal_method) == LinkageKind::External);

  // Private -> internal
  auto private_method = MakeMethod(Visibility::Private);
  assert(LinkageOf(private_method) == LinkageKind::Internal);
}

void TestLinkageStaticBinding() {
  SPEC_COV("Linkage-StaticBinding");
  SPEC_COV("Linkage-StaticBinding-Internal");

  // Public -> external
  assert(LinkageOfStaticBinding(Visibility::Public) == LinkageKind::External);

  // Internal -> external
  assert(LinkageOfStaticBinding(Visibility::Internal) == LinkageKind::External);

  // Private -> internal
  assert(LinkageOfStaticBinding(Visibility::Private) == LinkageKind::Internal);
}

// =============================================================================
// Class Method Linkage Tests
// =============================================================================

void TestLinkageClassMethod() {
  SPEC_COV("Linkage-ClassMethod");
  SPEC_COV("Linkage-ClassMethod-Internal");

  // Public -> external
  auto pub_method = MakeClassMethod(Visibility::Public);
  assert(LinkageOf(pub_method) == LinkageKind::External);

  // Internal -> external
  auto internal_method = MakeClassMethod(Visibility::Internal);
  assert(LinkageOf(internal_method) == LinkageKind::External);

  // Protected -> external (special case for class methods)
  auto protected_method = MakeClassMethod(Visibility::Protected);
  assert(LinkageOf(protected_method) == LinkageKind::External);

  // Private -> internal
  auto private_method = MakeClassMethod(Visibility::Private);
  assert(LinkageOf(private_method) == LinkageKind::Internal);
}

// =============================================================================
// State Method and Transition Linkage Tests
// =============================================================================

void TestLinkageStateMethod() {
  SPEC_COV("Linkage-StateMethod");
  SPEC_COV("Linkage-StateMethod-Internal");

  // Public -> external
  auto pub_method = MakeStateMethod(Visibility::Public);
  assert(LinkageOf(pub_method) == LinkageKind::External);

  // Internal -> external
  auto internal_method = MakeStateMethod(Visibility::Internal);
  assert(LinkageOf(internal_method) == LinkageKind::External);

  // Protected -> external
  auto protected_method = MakeStateMethod(Visibility::Protected);
  assert(LinkageOf(protected_method) == LinkageKind::External);

  // Private -> internal
  auto private_method = MakeStateMethod(Visibility::Private);
  assert(LinkageOf(private_method) == LinkageKind::Internal);
}

void TestLinkageTransition() {
  SPEC_COV("Linkage-Transition");
  SPEC_COV("Linkage-Transition-Internal");

  // Public -> external
  auto pub_trans = MakeTransition(Visibility::Public);
  assert(LinkageOf(pub_trans) == LinkageKind::External);

  // Internal -> external
  auto internal_trans = MakeTransition(Visibility::Internal);
  assert(LinkageOf(internal_trans) == LinkageKind::External);

  // Protected -> external
  auto protected_trans = MakeTransition(Visibility::Protected);
  assert(LinkageOf(protected_trans) == LinkageKind::External);

  // Private -> internal
  auto private_trans = MakeTransition(Visibility::Private);
  assert(LinkageOf(private_trans) == LinkageKind::Internal);
}

// =============================================================================
// Generated Symbol Linkage Tests
// =============================================================================

void TestLinkageVTable() {
  SPEC_COV("Linkage-VTable");

  // VTables are always internal
  assert(LinkageOfVTable() == LinkageKind::Internal);
}

void TestLinkageLiteral() {
  SPEC_COV("Linkage-LiteralData");

  // Literal data is always internal
  assert(LinkageOfLiteral() == LinkageKind::Internal);
}

void TestLinkageDropGlue() {
  SPEC_COV("Linkage-DropGlue");

  // Drop glue is always internal
  assert(LinkageOfDropGlue() == LinkageKind::Internal);
}

void TestLinkageDefaultImpl() {
  SPEC_COV("Linkage-DefaultImpl");
  SPEC_COV("Linkage-DefaultImpl-Internal");

  // Public method -> external
  assert(LinkageOfDefaultImpl(Visibility::Public) == LinkageKind::External);

  // Internal method -> external
  assert(LinkageOfDefaultImpl(Visibility::Internal) == LinkageKind::External);

  // Protected method -> external
  assert(LinkageOfDefaultImpl(Visibility::Protected) == LinkageKind::External);

  // Private method -> internal
  assert(LinkageOfDefaultImpl(Visibility::Private) == LinkageKind::Internal);
}

// =============================================================================
// Runtime Symbol Linkage Tests
// =============================================================================

void TestLinkageInitFn() {
  SPEC_COV("Linkage-InitFn");

  // Init functions are always internal
  assert(LinkageOfInitFn() == LinkageKind::Internal);
}

void TestLinkageDeinitFn() {
  SPEC_COV("Linkage-DeinitFn");

  // Deinit functions are always internal
  assert(LinkageOfDeinitFn() == LinkageKind::Internal);
}

void TestLinkagePanicSym() {
  SPEC_COV("Linkage-PanicSym");

  // Panic symbols are always internal
  assert(LinkageOfPanicSym() == LinkageKind::Internal);
}

void TestLinkageRegionSym() {
  SPEC_COV("Linkage-RegionSym");

  // Region symbols are always internal
  assert(LinkageOfRegionSym() == LinkageKind::Internal);
}

void TestLinkageBuiltinSym() {
  SPEC_COV("Linkage-BuiltinSym");

  // Builtin symbols are always internal
  assert(LinkageOfBuiltinSym() == LinkageKind::Internal);
}

void TestLinkageEntrySym() {
  SPEC_COV("Linkage-EntrySym");

  // Entry symbol is always external
  assert(LinkageOfEntrySym() == LinkageKind::External);
}

void TestAnchorCoverage() {
  // Ensure all rule anchors are emitted
  AnchorLinkageRules();
}

}  // namespace

int main() {
  // User item linkage
  TestLinkageProcedure();
  TestLinkageStatic();
  TestLinkageMethod();
  TestLinkageStaticBinding();

  // Class method linkage
  TestLinkageClassMethod();

  // State method and transition linkage
  TestLinkageStateMethod();
  TestLinkageTransition();

  // Generated symbol linkage
  TestLinkageVTable();
  TestLinkageLiteral();
  TestLinkageDropGlue();
  TestLinkageDefaultImpl();

  // Runtime symbol linkage
  TestLinkageInitFn();
  TestLinkageDeinitFn();
  TestLinkagePanicSym();
  TestLinkageRegionSym();
  TestLinkageBuiltinSym();
  TestLinkageEntrySym();

  // Coverage
  TestAnchorCoverage();

  return 0;
}
