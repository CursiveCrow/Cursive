#include <cassert>
#include <memory>
#include <string>
#include <vector>

#include "cursive0/codegen/mangle.h"
#include "cursive0/core/assert_spec.h"
#include "cursive0/core/symbols.h"
#include "cursive0/sema/types.h"
#include "cursive0/syntax/ast.h"

namespace {

using cursive0::codegen::AnchorMangleRules;
using cursive0::codegen::ItemPathClassMethod;
using cursive0::codegen::ItemPathDefaultImpl;
using cursive0::codegen::ItemPathMethod;
using cursive0::codegen::ItemPathProc;
using cursive0::codegen::ItemPathStateMethod;
using cursive0::codegen::ItemPathStatic;
using cursive0::codegen::ItemPathStaticBinding;
using cursive0::codegen::ItemPathTransition;
using cursive0::codegen::ItemPathVTable;
using cursive0::codegen::MangleClassMethod;
using cursive0::codegen::MangleDefaultImpl;
using cursive0::codegen::MangleLiteral;
using cursive0::codegen::MangleMethod;
using cursive0::codegen::MangleProc;
using cursive0::codegen::MangleStateMethod;
using cursive0::codegen::MangleStatic;
using cursive0::codegen::MangleStaticBinding;
using cursive0::codegen::MangleTransition;
using cursive0::codegen::MangleVTable;
using cursive0::codegen::PathOfType;
using cursive0::codegen::ScopedSym;
using cursive0::core::Mangle;
using cursive0::core::StringOfPath;
using cursive0::sema::MakeTypePath;
using cursive0::sema::MakeTypePrim;
using cursive0::sema::MakeTypeString;
using cursive0::sema::StringState;
using cursive0::syntax::ProcedureDecl;
using cursive0::syntax::Visibility;

// Helper to create a minimal ProcedureDecl
ProcedureDecl MakeProc(const std::string& name, Visibility vis = Visibility::Public) {
  ProcedureDecl proc;
  proc.name = name;
  proc.vis = vis;
  return proc;
}

// Helper to create a minimal MethodDecl
cursive0::syntax::MethodDecl MakeMethod(const std::string& name,
                                        Visibility vis = Visibility::Public) {
  cursive0::syntax::MethodDecl method;
  method.name = name;
  method.vis = vis;
  return method;
}

// Helper to create a minimal ClassMethodDecl
cursive0::syntax::ClassMethodDecl MakeClassMethod(const std::string& name,
                                                  Visibility vis = Visibility::Public) {
  cursive0::syntax::ClassMethodDecl method;
  method.name = name;
  method.vis = vis;
  return method;
}

// Helper to create a minimal StateMethodDecl
cursive0::syntax::StateMethodDecl MakeStateMethod(const std::string& name,
                                                  Visibility vis = Visibility::Public) {
  cursive0::syntax::StateMethodDecl method;
  method.name = name;
  method.vis = vis;
  return method;
}

// Helper to create a minimal TransitionDecl
cursive0::syntax::TransitionDecl MakeTransition(const std::string& name,
                                                const std::string& target,
                                                Visibility vis = Visibility::Public) {
  cursive0::syntax::TransitionDecl trans;
  trans.name = name;
  trans.target_state = target;
  trans.vis = vis;
  return trans;
}

void TestItemPathProc() {
  SPEC_COV("ItemPath");
  SPEC_COV("PathOfModule");

  const std::vector<std::string> module_path = {"foo", "bar"};
  auto proc = MakeProc("baz");

  const auto path = ItemPathProc(module_path, proc);

  assert(path.size() == 3);
  assert(path[0] == "foo");
  assert(path[1] == "bar");
  assert(path[2] == "baz");
}

void TestItemPathMethod() {
  SPEC_COV("ItemPath");
  SPEC_COV("RecordPath");

  const std::vector<std::string> record_path = {"mymod", "MyRecord"};
  auto method = MakeMethod("do_thing");

  const auto path = ItemPathMethod(record_path, method);

  assert(path.size() == 3);
  assert(path[0] == "mymod");
  assert(path[1] == "MyRecord");
  assert(path[2] == "do_thing");
}

void TestItemPathClassMethod() {
  SPEC_COV("ItemPath");
  SPEC_COV("ClassPath");

  const std::vector<std::string> class_path = {"traits", "Comparable"};
  auto method = MakeClassMethod("compare");

  const auto path = ItemPathClassMethod(class_path, method);

  assert(path.size() == 3);
  assert(path[0] == "traits");
  assert(path[1] == "Comparable");
  assert(path[2] == "compare");
}

void TestItemPathStateMethod() {
  SPEC_COV("ItemPath");
  SPEC_COV("ModalPath");

  const std::vector<std::string> modal_path = {"net", "Connection"};
  auto method = MakeStateMethod("read_data");

  const auto path = ItemPathStateMethod(modal_path, "Connected", method);

  assert(path.size() == 4);
  assert(path[0] == "net");
  assert(path[1] == "Connection");
  assert(path[2] == "Connected");
  assert(path[3] == "read_data");
}

void TestItemPathTransition() {
  SPEC_COV("ItemPath");

  const std::vector<std::string> modal_path = {"net", "Connection"};
  auto trans = MakeTransition("connect", "Connected");

  const auto path = ItemPathTransition(modal_path, "Idle", trans);

  assert(path.size() == 4);
  assert(path[0] == "net");
  assert(path[1] == "Connection");
  assert(path[2] == "Idle");
  assert(path[3] == "connect");
}

void TestItemPathStaticBinding() {
  SPEC_COV("ItemPath");
  SPEC_COV("StaticBinding");

  const std::vector<std::string> module_path = {"config"};
  const auto path = ItemPathStaticBinding(module_path, "MAX_SIZE");

  assert(path.size() == 2);
  assert(path[0] == "config");
  assert(path[1] == "MAX_SIZE");
}

void TestItemPathVTable() {
  SPEC_COV("ItemPath");
  SPEC_COV("VTableDecl");

  const auto type = MakeTypePath({"mymod", "MyRecord"});
  const std::vector<std::string> class_path = {"traits", "Comparable"};

  const auto path = ItemPathVTable(type, class_path);

  // ["vtable"] ++ PathOfType(T) ++ ["cl"] ++ ClassPath(Cl)
  assert(path.size() == 6);
  assert(path[0] == "vtable");
  assert(path[1] == "mymod");
  assert(path[2] == "MyRecord");
  assert(path[3] == "cl");
  assert(path[4] == "traits");
  assert(path[5] == "Comparable");
}

void TestItemPathDefaultImpl() {
  SPEC_COV("ItemPath");
  SPEC_COV("DefaultImpl");

  const auto type = MakeTypePath({"mymod", "MyRecord"});
  const std::vector<std::string> class_path = {"traits", "Comparable"};

  const auto path = ItemPathDefaultImpl(type, class_path, "compare");

  // ["default"] ++ PathOfType(T) ++ ["cl"] ++ ClassPath(Cl) ++ [m.name]
  assert(path.size() == 7);
  assert(path[0] == "default");
  assert(path[1] == "mymod");
  assert(path[2] == "MyRecord");
  assert(path[3] == "cl");
  assert(path[4] == "traits");
  assert(path[5] == "Comparable");
  assert(path[6] == "compare");
}

void TestPathOfTypePrim() {
  SPEC_COV("PathOfType");

  const auto type = MakeTypePrim("i32");
  const auto path = PathOfType(type);

  assert(path.size() == 2);
  assert(path[0] == "prim");
  assert(path[1] == "i32");
}

void TestPathOfTypeString() {
  SPEC_COV("PathOfType");
  SPEC_COV("TypeStateName");

  const auto managed = MakeTypeString(StringState::Managed);
  const auto managed_path = PathOfType(managed);

  assert(managed_path.size() == 2);
  assert(managed_path[0] == "string");
  assert(managed_path[1] == "managed");

  const auto view = MakeTypeString(StringState::View);
  const auto view_path = PathOfType(view);

  assert(view_path.size() == 2);
  assert(view_path[0] == "string");
  assert(view_path[1] == "view");
}

void TestPathOfTypePath() {
  SPEC_COV("PathOfType");

  const auto type = MakeTypePath({"foo", "bar", "Baz"});
  const auto path = PathOfType(type);

  assert(path.size() == 3);
  assert(path[0] == "foo");
  assert(path[1] == "bar");
  assert(path[2] == "Baz");
}

void TestScopedSym() {
  SPEC_COV("ScopedSym");
  SPEC_COV("PathSig");

  // ScopedSym(item) = PathSig(ItemPath(item))
  // PathSig(p) = mangle(StringOfPath(p))

  const std::vector<std::string> path = {"foo", "bar", "baz"};
  const std::string sym = ScopedSym(path);

  // Should equal mangle("foo::bar::baz")
  assert(sym == Mangle(StringOfPath(path)));
  assert(sym == Mangle("foo::bar::baz"));
}

void TestMangleProc() {
  SPEC_COV("Mangle-Proc");
  SPEC_COV("Mangle-Main");

  const std::vector<std::string> module_path = {"myapp"};

  // Regular procedure
  auto proc = MakeProc("helper");
  const std::string sym = MangleProc(module_path, proc);
  assert(sym == Mangle("myapp::helper"));

  // Main procedure (same mangling but different rule)
  auto main_proc = MakeProc("main");
  const std::string main_sym = MangleProc(module_path, main_proc);
  assert(main_sym == Mangle("myapp::main"));
}

void TestMangleMethod() {
  SPEC_COV("Mangle-Record-Method");

  const std::vector<std::string> record_path = {"mod", "MyRecord"};
  auto method = MakeMethod("get_value");

  const std::string sym = MangleMethod(record_path, method);
  assert(sym == Mangle("mod::MyRecord::get_value"));
}

void TestMangleClassMethod() {
  SPEC_COV("Mangle-Class-Method");

  const std::vector<std::string> class_path = {"traits", "Equatable"};
  auto method = MakeClassMethod("equals");

  const std::string sym = MangleClassMethod(class_path, method);
  assert(sym == Mangle("traits::Equatable::equals"));
}

void TestMangleStateMethod() {
  SPEC_COV("Mangle-State-Method");

  const std::vector<std::string> modal_path = {"io", "File"};
  auto method = MakeStateMethod("read");

  const std::string sym = MangleStateMethod(modal_path, "Open", method);
  assert(sym == Mangle("io::File::Open::read"));
}

void TestMangleTransition() {
  SPEC_COV("Mangle-Transition");

  const std::vector<std::string> modal_path = {"io", "File"};
  auto trans = MakeTransition("close", "Closed");

  const std::string sym = MangleTransition(modal_path, "Open", trans);
  assert(sym == Mangle("io::File::Open::close"));
}
void TestMangleStatic() {
  SPEC_COV("Mangle-Static");

  const std::vector<std::string> module_path = {"config"};

  cursive0::syntax::StaticDecl decl;
  decl.vis = Visibility::Public;
  decl.mut = cursive0::syntax::Mutability::Let;
  decl.binding.pat = std::make_shared<cursive0::syntax::Pattern>();
  decl.binding.pat->node = cursive0::syntax::IdentifierPattern{"MAX_SIZE"};

  const std::string sym = MangleStatic(module_path, decl);
  assert(sym == Mangle("config::MAX_SIZE"));
}


void TestMangleStaticBinding() {
  SPEC_COV("Mangle-StaticBinding");

  const std::vector<std::string> module_path = {"config"};
  const std::string sym = MangleStaticBinding(module_path, "VERSION");

  assert(sym == Mangle("config::VERSION"));
}

void TestMangleVTable() {
  SPEC_COV("Mangle-VTable");

  const auto type = MakeTypePath({"mod", "MyRecord"});
  const std::vector<std::string> class_path = {"traits", "Show"};

  const std::string sym = MangleVTable(type, class_path);
  assert(sym == Mangle("vtable::mod::MyRecord::cl::traits::Show"));
}

void TestMangleLiteral() {
  SPEC_COV("Mangle-Literal");
  SPEC_COV("LiteralData");

  // MangleLiteral produces PathSig(["cursive", "runtime", "literal", LiteralID(kind, contents)])

  const std::vector<std::uint8_t> hello = {'h', 'e', 'l', 'l', 'o'};
  const std::string sym = MangleLiteral("string", std::span<const std::uint8_t>(hello));

  // Verify it starts with the expected prefix
  const std::string prefix = Mangle("cursive::runtime::literal::string_");
  assert(sym.substr(0, prefix.length()) == prefix);
}

void TestMangleDefaultImpl() {
  SPEC_COV("Mangle-DefaultImpl");

  const auto type = MakeTypePath({"mod", "MyRecord"});
  const std::vector<std::string> class_path = {"traits", "Default"};

  const std::string sym = MangleDefaultImpl(type, class_path, "default_value");
  assert(sym == Mangle("default::mod::MyRecord::cl::traits::Default::default_value"));
}

void TestPathToPrefixHexLowercase() {
  SPEC_COV("BMap");
  SPEC_COV("Hex2");

  // "_" (0x5F) should be escaped to _x5f with lowercase hex digits.
  const std::string sym = Mangle("a_b");
  assert(sym == "a_x5fb");
}

void TestAnchorCoverage() {
  // Ensure all rule anchors are emitted
  AnchorMangleRules();
}

}  // namespace

int main() {
  // ItemPath tests
  TestItemPathProc();
  TestItemPathMethod();
  TestItemPathClassMethod();
  TestItemPathStateMethod();
  TestItemPathTransition();
  TestItemPathStaticBinding();
  TestItemPathVTable();
  TestItemPathDefaultImpl();

  // PathOfType tests
  TestPathOfTypePrim();
  TestPathOfTypeString();
  TestPathOfTypePath();

  // Core mangle tests
  TestScopedSym();
  TestMangleProc();
  TestMangleMethod();
  TestMangleClassMethod();
  TestMangleStateMethod();
  TestMangleTransition();
  TestMangleStatic();
  TestMangleStaticBinding();
  TestMangleVTable();
  TestMangleLiteral();
  TestMangleDefaultImpl();
  TestPathToPrefixHexLowercase();

  // Coverage
  TestAnchorCoverage();

  return 0;
}
