// Unit tests for C0X extension parsing features.
// Tests: generic parameters, where clauses, attributes, contracts, key boundaries.

#include <cassert>
#include <string>
#include <vector>

#include "cursive0/core/assert_spec.h"
#include "cursive0/core/source_text.h"
#include "cursive0/syntax/ast.h"
#include "cursive0/syntax/lexer.h"
#include "cursive0/syntax/parser.h"

namespace {

using cursive0::core::LineStarts;
using cursive0::core::SourceFile;
using cursive0::core::UnicodeScalar;
using cursive0::syntax::ASTFile;
using cursive0::syntax::ASTItem;
using cursive0::syntax::ParseFile;
using cursive0::syntax::ProcDecl;
using cursive0::syntax::RecordDecl;
using cursive0::syntax::ClassDecl;

static std::vector<UnicodeScalar> AsScalars(const std::string& text) {
  std::vector<UnicodeScalar> out;
  out.reserve(text.size());
  for (unsigned char c : text) {
    out.push_back(static_cast<UnicodeScalar>(c));
  }
  return out;
}

static SourceFile MakeSourceFile(const std::string& path,
                                 const std::string& text) {
  SourceFile s;
  s.path = path;
  s.scalars = AsScalars(text);
  s.text = cursive0::core::EncodeUtf8(s.scalars);
  s.bytes.assign(s.text.begin(), s.text.end());
  s.byte_len = s.text.size();
  s.line_starts = LineStarts(s.scalars);
  s.line_count = s.line_starts.size();
  return s;
}

// Test: Parse generic type parameter on record
void TestParseGenericRecord() {
  SPEC_COV("C0X-Parse-GenericParams");
  
  const char* src = R"(
record Wrapper<T> {
  value: T
}
)";
  
  SourceFile file = MakeSourceFile("test.cursive", src);
  auto result = ParseFile(file);
  
  assert(result.ok && "Should parse successfully");
  assert(result.file.items.size() >= 1 && "Should have at least one item");
  
  // Find the record declaration
  bool found_record = false;
  for (const auto& item : result.file.items) {
    if (auto* rec = std::get_if<RecordDecl>(&item)) {
      found_record = true;
      assert(rec->name == "Wrapper" && "Record name should be Wrapper");
      assert(rec->generic_params.has_value() && "Should have generic params");
      assert(rec->generic_params->params.size() == 1 && "Should have one type param");
      assert(rec->generic_params->params[0].name == "T" && "Param name should be T");
    }
  }
  assert(found_record && "Should find record declaration");
}

// Test: Parse multiple generic parameters with semicolon separator
void TestParseMultipleGenericParams() {
  SPEC_COV("C0X-Parse-GenericParams-Multi");
  
  const char* src = R"(
record Pair<K; V> {
  key: K
  value: V
}
)";
  
  SourceFile file = MakeSourceFile("test.cursive", src);
  auto result = ParseFile(file);
  
  assert(result.ok && "Should parse successfully");
  
  for (const auto& item : result.file.items) {
    if (auto* rec = std::get_if<RecordDecl>(&item)) {
      assert(rec->generic_params.has_value() && "Should have generic params");
      assert(rec->generic_params->params.size() == 2 && "Should have two type params");
      assert(rec->generic_params->params[0].name == "K" && "First param should be K");
      assert(rec->generic_params->params[1].name == "V" && "Second param should be V");
    }
  }
}

// Test: Parse where clause on record
void TestParseWhereClause() {
  SPEC_COV("C0X-Parse-WhereClause");
  
  const char* src = R"(
record Container<T>
where T <: Bitcopy
{
  value: T
}
)";
  
  SourceFile file = MakeSourceFile("test.cursive", src);
  auto result = ParseFile(file);
  
  assert(result.ok && "Should parse successfully");
  
  for (const auto& item : result.file.items) {
    if (auto* rec = std::get_if<RecordDecl>(&item)) {
      assert(rec->where_clause.has_value() && "Should have where clause");
      assert(!rec->where_clause->predicates.empty() && "Where clause should have predicates");
    }
  }
}

// Test: Parse attribute syntax [[...]]
void TestParseAttribute() {
  SPEC_COV("C0X-Parse-Attribute");
  
  const char* src = R"(
[[dynamic]]
procedure test_proc() -> () { }
)";
  
  SourceFile file = MakeSourceFile("test.cursive", src);
  auto result = ParseFile(file);
  
  assert(result.ok && "Should parse successfully");
  
  for (const auto& item : result.file.items) {
    if (auto* proc = std::get_if<ProcDecl>(&item)) {
      assert(!proc->attrs.empty() && "Should have attributes");
      assert(proc->attrs[0].name == "dynamic" && "Attribute should be 'dynamic'");
    }
  }
}

// Test: Parse contract clause |= pre => post
void TestParseContractClause() {
  SPEC_COV("C0X-Parse-Contract");
  
  const char* src = R"(
procedure positive(x: i32) -> i32
  |= x > 0
{
  return x
}
)";
  
  SourceFile file = MakeSourceFile("test.cursive", src);
  auto result = ParseFile(file);
  
  assert(result.ok && "Should parse successfully");
  
  for (const auto& item : result.file.items) {
    if (auto* proc = std::get_if<ProcDecl>(&item)) {
      assert(proc->contract.has_value() && "Should have contract clause");
      assert(proc->contract->precondition.has_value() && "Should have precondition");
    }
  }
}

// Test: Parse key boundary marker on field
void TestParseKeyBoundary() {
  SPEC_COV("C0X-Parse-KeyBoundary");
  
  const char* src = R"(
record WithBoundary {
  public_field: i32
  #private_field: i32
}
)";
  
  SourceFile file = MakeSourceFile("test.cursive", src);
  auto result = ParseFile(file);
  
  assert(result.ok && "Should parse successfully");
  
  for (const auto& item : result.file.items) {
    if (auto* rec = std::get_if<RecordDecl>(&item)) {
      assert(rec->members.size() >= 2 && "Should have at least 2 fields");
      // Check that the second field has the boundary marker
      bool found_boundary = false;
      for (const auto& member : rec->members) {
        if (auto* field = std::get_if<cursive0::syntax::FieldDecl>(&member)) {
          if (field->name == "private_field") {
            found_boundary = field->has_key_boundary;
          }
        }
      }
      assert(found_boundary && "Should find field with key boundary");
    }
  }
}

// Test: Parse shared receiver (~%)
void TestParseSharedReceiver() {
  SPEC_COV("C0X-Parse-SharedReceiver");
  
  const char* src = R"(
record Data { value: i32 }

procedure Data::shared_method(~%) -> i32 {
  return self.value
}
)";
  
  SourceFile file = MakeSourceFile("test.cursive", src);
  auto result = ParseFile(file);
  
  assert(result.ok && "Should parse successfully");
  
  for (const auto& item : result.file.items) {
    if (auto* proc = std::get_if<ProcDecl>(&item)) {
      if (proc->name == "shared_method" && proc->receiver.has_value()) {
        assert(proc->receiver->perm == cursive0::syntax::ReceiverPerm::Shared &&
               "Receiver should have shared permission");
      }
    }
  }
}

// Test: Parse modal class with abstract states
void TestParseModalClass() {
  SPEC_COV("C0X-Parse-ModalClass");
  
  const char* src = R"(
modal class StateMachine {
  @Idle { }
  @Running { ticks: i32 }
}
)";
  
  SourceFile file = MakeSourceFile("test.cursive", src);
  auto result = ParseFile(file);
  
  assert(result.ok && "Should parse successfully");
  
  for (const auto& item : result.file.items) {
    if (auto* cls = std::get_if<ClassDecl>(&item)) {
      assert(cls->is_modal && "Class should be modal");
      assert(cls->name == "StateMachine" && "Class name should be StateMachine");
      assert(!cls->abstract_states.empty() && "Should have abstract states");
    }
  }
}

// Test: Parse class with associated type
void TestParseAssociatedType() {
  SPEC_COV("C0X-Parse-AssociatedType");
  
  const char* src = R"(
class Container {
  type Element
  procedure get(~) -> Self::Element
}
)";
  
  SourceFile file = MakeSourceFile("test.cursive", src);
  auto result = ParseFile(file);
  
  assert(result.ok && "Should parse successfully");
  
  for (const auto& item : result.file.items) {
    if (auto* cls = std::get_if<ClassDecl>(&item)) {
      assert(cls->name == "Container" && "Class name should be Container");
      assert(!cls->associated_types.empty() && "Should have associated types");
      assert(cls->associated_types[0].name == "Element" && 
             "Associated type should be Element");
    }
  }
}

// Test: Parse key block with read mode
void TestParseKeyBlockRead() {
  SPEC_COV("C0X-Parse-KeyBlock-Read");
  
  const char* src = R"(
record Data { value: i32 }
public procedure main(ctx: Context) -> i32 {
  var d: shared Data = Data { value: 42 }
  # d {
    return d.value
  }
  return 0
}
)";
  
  SourceFile file = MakeSourceFile("test.cursive", src);
  auto result = ParseFile(file);
  
  // Should parse (may have semantic errors, but syntax should be valid)
  assert(result.ok && "Should parse key block with read mode");
}

// Test: Parse key block with write mode
void TestParseKeyBlockWrite() {
  SPEC_COV("C0X-Parse-KeyBlock-Write");
  
  const char* src = R"(
record Data { value: i32 }
public procedure main(ctx: Context) -> i32 {
  var d: shared Data = Data { value: 42 }
  # d write {
    d.value = 100
  }
  return d.value
}
)";
  
  SourceFile file = MakeSourceFile("test.cursive", src);
  auto result = ParseFile(file);
  
  assert(result.ok && "Should parse key block with write mode");
}

// Test: Parse key block with multiple paths
void TestParseKeyBlockMultiPath() {
  SPEC_COV("C0X-Parse-KeyBlock-MultiPath");
  
  const char* src = R"(
record Data { x: i32, y: i32 }
public procedure main(ctx: Context) -> i32 {
  var d: shared Data = Data { x: 1, y: 2 }
  # d.x, d.y {
    return d.x + d.y
  }
  return 0
}
)";
  
  SourceFile file = MakeSourceFile("test.cursive", src);
  auto result = ParseFile(file);
  
  assert(result.ok && "Should parse key block with multiple paths");
}

// Test: Parse key block with dynamic modifier
void TestParseKeyBlockDynamic() {
  SPEC_COV("C0X-Parse-KeyBlock-Dynamic");
  
  const char* src = R"(
record Data { value: i32 }
public procedure main(ctx: Context) -> i32 {
  var d: shared Data = Data { value: 42 }
  # d dynamic write {
    d.value = 100
  }
  return d.value
}
)";
  
  SourceFile file = MakeSourceFile("test.cursive", src);
  auto result = ParseFile(file);
  
  assert(result.ok && "Should parse key block with dynamic modifier");
}

// Test: Parse key block with speculative modifier
void TestParseKeyBlockSpeculative() {
  SPEC_COV("C0X-Parse-KeyBlock-Speculative");
  
  const char* src = R"(
record Data { value: i32 }
public procedure main(ctx: Context) -> i32 {
  var d: shared Data = Data { value: 42 }
  # d speculative write {
    d.value = 100
  }
  return d.value
}
)";
  
  SourceFile file = MakeSourceFile("test.cursive", src);
  auto result = ParseFile(file);
  
  assert(result.ok && "Should parse key block with speculative modifier");
}

// Test: Parse key path with coarsening marker
void TestParseKeyPathCoarsen() {
  SPEC_COV("C0X-Parse-KeyPath-Coarsen");
  
  const char* src = R"(
record Inner { val: i32 }
record Outer { inner: Inner }
public procedure main(ctx: Context) -> i32 {
  var o: shared Outer = Outer { inner: Inner { val: 42 } }
  # o.#inner.val {
    return o.inner.val
  }
  return 0
}
)";
  
  SourceFile file = MakeSourceFile("test.cursive", src);
  auto result = ParseFile(file);
  
  assert(result.ok && "Should parse key path with coarsening marker");
}

// Test: Parse contract with postcondition only
void TestParseContractPostOnly() {
  SPEC_COV("C0X-Parse-Contract-PostOnly");
  
  const char* src = R"(
procedure non_negative(x: i32) -> i32
  |= => @result >= 0
{
  if x < 0 { return 0 - x }
  return x
}
)";
  
  SourceFile file = MakeSourceFile("test.cursive", src);
  auto result = ParseFile(file);
  
  assert(result.ok && "Should parse contract with postcondition only");
}

// Test: Parse type invariant
void TestParseTypeInvariant() {
  SPEC_COV("C0X-Parse-TypeInvariant");
  
  const char* src = R"(
record Positive { value: i32 }
where { self.value > 0 }
)";
  
  SourceFile file = MakeSourceFile("test.cursive", src);
  auto result = ParseFile(file);
  
  assert(result.ok && "Should parse type invariant");
}

// Test: Parse key block with release modifier (§7.6.3)
void TestParseKeyBlockRelease() {
  SPEC_COV("C0X-7.6.3-KeyBlock-Release");
  
  const char* src = R"(
record Data { value: i32 }
public procedure main(ctx: Context) -> i32 {
  var d: shared Data = Data { value: 42 }
  # d write {
    d.value = 100
    # d release write {
    }
    d.value = d.value + 1
  }
  return d.value
}
)";
  
  SourceFile file = MakeSourceFile("test.cursive", src);
  auto result = ParseFile(file);
  
  assert(result.ok && "Should parse key block with release modifier");
}

// Test: Parse contract with @entry (§14.5.4)
void TestParseContractWithEntry() {
  SPEC_COV("C0X-14.5.4-Contract-Entry");
  
  const char* src = R"(
procedure increment(x: i32) -> i32
  |= => @result == @entry(x) + 1
{
  return x + 1
}
)";
  
  SourceFile file = MakeSourceFile("test.cursive", src);
  auto result = ParseFile(file);
  
  assert(result.ok && "Should parse contract with @entry intrinsic");
}

// Test: Parse loop invariant (§14.6.2)
void TestParseLoopInvariant() {
  SPEC_COV("C0X-14.6.2-LoopInvariant");
  
  const char* src = R"(
public procedure main(ctx: Context) -> i32 {
  var i: i32 = 0
  loop i < 10 where { i >= 0 && i <= 10 } {
    i = i + 1
  }
  return i
}
)";
  
  SourceFile file = MakeSourceFile("test.cursive", src);
  auto result = ParseFile(file);
  
  assert(result.ok && "Should parse loop invariant");
}

// Test: Parse import declaration (§9.2)
void TestParseImport() {
  SPEC_COV("C0X-9.2-Import");
  
  const char* src = R"(
import cursive::runtime

public procedure main(ctx: Context) -> i32 {
  return 0
}
)";
  
  SourceFile file = MakeSourceFile("test.cursive", src);
  auto result = ParseFile(file);
  
  assert(result.ok && "Should parse import declaration");
}

// Test: Parse import with alias (§9.2)
void TestParseImportAs() {
  SPEC_COV("C0X-9.2-Import-As");
  
  const char* src = R"(
import cursive::runtime as rt

public procedure main(ctx: Context) -> i32 {
  return 0
}
)";
  
  SourceFile file = MakeSourceFile("test.cursive", src);
  auto result = ParseFile(file);
  
  assert(result.ok && "Should parse import with alias");
}

// Test: Parse using list (§9.3)
void TestParseUsingList() {
  SPEC_COV("C0X-9.3-Using-List");
  
  const char* src = R"(
using cursive::runtime::{string, bytes}

public procedure main(ctx: Context) -> i32 {
  return 0
}
)";
  
  SourceFile file = MakeSourceFile("test.cursive", src);
  auto result = ParseFile(file);
  
  assert(result.ok && "Should parse using list");
}

// Test: Parse generic procedure (§6)
void TestParseGenericProcedure() {
  SPEC_COV("C0X-6-GenericProcedure");
  
  const char* src = R"(
procedure identity<T>(x: T) -> T {
  return x
}

public procedure main(ctx: Context) -> i32 {
  return identity<i32>(42)
}
)";
  
  SourceFile file = MakeSourceFile("test.cursive", src);
  auto result = ParseFile(file);
  
  assert(result.ok && "Should parse generic procedure");
}

// Test: Parse class implementation (§8.2)
void TestParseClassImpl() {
  SPEC_COV("C0X-8.2-ClassImpl");
  
  const char* src = R"(
class Printable {
  procedure print(~) -> i32
}

record MyType { value: i32 }

MyType <: Printable {
  procedure print(~) -> i32 { return self.value }
}
)";
  
  SourceFile file = MakeSourceFile("test.cursive", src);
  auto result = ParseFile(file);
  
  assert(result.ok && "Should parse class implementation");
}

// Test: Parse override keyword (§8.3)
void TestParseOverride() {
  SPEC_COV("C0X-8.3-Override");
  
  const char* src = R"(
class WithDefault {
  procedure value(~) -> i32 { return 0 }
}

record Custom { v: i32 }

Custom <: WithDefault {
  override procedure value(~) -> i32 { return self.v }
}
)";
  
  SourceFile file = MakeSourceFile("test.cursive", src);
  auto result = ParseFile(file);
  
  assert(result.ok && "Should parse override keyword");
}

// Test: Parse class with superclass (§8.1)
void TestParseSuperclass() {
  SPEC_COV("C0X-8.1-Superclass");
  
  const char* src = R"(
class Base { }
class Derived <: Base { }
)";
  
  SourceFile file = MakeSourceFile("test.cursive", src);
  auto result = ParseFile(file);
  
  assert(result.ok && "Should parse class with superclass");
}

// Test: Parse associated type with default (§8)
void TestParseAssociatedTypeDefault() {
  SPEC_COV("C0X-8-AssociatedType-Default");
  
  const char* src = R"(
class Result {
  type Error = ()
  type Value
}
)";
  
  SourceFile file = MakeSourceFile("test.cursive", src);
  auto result = ParseFile(file);
  
  assert(result.ok && "Should parse associated type with default");
}

// Test: Parse generic type parameter with default (§6.2)
void TestParseGenericDefault() {
  SPEC_COV("C0X-6.2-GenericDefault");
  
  const char* src = R"(
record Container<T = i32> { value: T }
)";
  
  SourceFile file = MakeSourceFile("test.cursive", src);
  auto result = ParseFile(file);
  
  assert(result.ok && "Should parse generic param with default");
}

// Test: Parse key boundary in enum (§7.8)
void TestParseKeyBoundaryEnum() {
  SPEC_COV("C0X-7.8-KeyBoundary-Enum");
  
  const char* src = R"(
enum Maybe {
  Some { #value: i32 }
  None
}
)";
  
  SourceFile file = MakeSourceFile("test.cursive", src);
  auto result = ParseFile(file);
  
  assert(result.ok && "Should parse key boundary in enum");
}

// Test: Parse key block with index path (§7.2)
void TestParseKeyPathIndex() {
  SPEC_COV("C0X-7.2-KeyPath-Index");
  
  const char* src = R"(
public procedure main(ctx: Context) -> i32 {
  var arr: shared [i32; 3] = [1, 2, 3]
  # arr[1] {
    return arr[1]
  }
  return 0
}
)";
  
  SourceFile file = MakeSourceFile("test.cursive", src);
  auto result = ParseFile(file);
  
  assert(result.ok && "Should parse key path with index");
}

// Test: Parse [[static_dispatch_only]] attribute (§8.5)
void TestParseStaticDispatchOnly() {
  SPEC_COV("C0X-8.5-StaticDispatchOnly");
  
  const char* src = R"(
class WithGeneric {
  [[static_dispatch_only]]
  procedure generic_method<T>(~, x: T) -> T
}
)";
  
  SourceFile file = MakeSourceFile("test.cursive", src);
  auto result = ParseFile(file);
  
  assert(result.ok && "Should parse [[static_dispatch_only]] attribute");
}

// Test: Parse full contract with pre and post (§14.4)
void TestParseFullContract() {
  SPEC_COV("C0X-14.4-FullContract");
  
  const char* src = R"(
procedure divide(a: i32, b: i32) -> i32
  |= b != 0 => @result * b == a
{
  return a / b
}
)";
  
  SourceFile file = MakeSourceFile("test.cursive", src);
  auto result = ParseFile(file);
  
  assert(result.ok && "Should parse full contract with pre and post");
}

}  // namespace

int main() {
  // §6 - Generics
  TestParseGenericRecord();
  TestParseMultipleGenericParams();
  TestParseWhereClause();
  TestParseGenericProcedure();
  TestParseGenericDefault();
  
  // Attributes
  TestParseAttribute();
  TestParseStaticDispatchOnly();
  
  // §14 - Contracts
  TestParseContractClause();
  TestParseContractPostOnly();
  TestParseFullContract();
  TestParseContractWithEntry();
  TestParseTypeInvariant();
  TestParseLoopInvariant();
  
  // §7.8 - Key system boundaries
  TestParseKeyBoundary();
  TestParseKeyBoundaryEnum();
  
  // §7.6 - Key system blocks
  TestParseKeyBlockRead();
  TestParseKeyBlockWrite();
  TestParseKeyBlockMultiPath();
  TestParseKeyBlockDynamic();
  TestParseKeyBlockSpeculative();
  TestParseKeyBlockRelease();
  
  // §7.2 - Key paths
  TestParseKeyPathCoarsen();
  TestParseKeyPathIndex();
  
  // Permissions
  TestParseSharedReceiver();
  
  // §8 - Classes
  TestParseModalClass();
  TestParseAssociatedType();
  TestParseAssociatedTypeDefault();
  TestParseSuperclass();
  TestParseClassImpl();
  TestParseOverride();
  
  // §9 - Import/using
  TestParseImport();
  TestParseImportAs();
  TestParseUsingList();
  
  return 0;
}
