// Unit tests for C0X semantic analysis features.
// Tests: shared permission subtyping, generic instantiation, contract validation.

#include <cassert>
#include <string>
#include <vector>

#include "cursive0/core/assert_spec.h"
#include "cursive0/core/diagnostics.h"
#include "cursive0/core/source_text.h"
#include "cursive0/syntax/ast.h"
#include "cursive0/syntax/parser.h"
#include "cursive0/analysis/types/types.h"
#include "cursive0/analysis/types/subtyping.h"

namespace {

using cursive0::core::LineStarts;
using cursive0::core::SourceFile;
using cursive0::core::UnicodeScalar;
using cursive0::analysis::TypeRef;
using cursive0::analysis::MakeTypePrim;
using cursive0::analysis::MakeTypeConst;
using cursive0::analysis::MakeTypeUnique;
using cursive0::analysis::MakeTypeShared;
using cursive0::analysis::IsSubtype;

static std::vector<UnicodeScalar> AsScalars(const std::string& text) {
  std::vector<UnicodeScalar> out;
  out.reserve(text.size());
  for (unsigned char c : text) {
    out.push_back(static_cast<UnicodeScalar>(c));
  }
  return out;
}

// Test: Permission lattice subtyping (unique <: shared <: const)
void TestPermissionLatticeSubtyping() {
  SPEC_COV("C0X-Perm-Lattice");
  SPEC_COV("C0X-Perm-Subtype");
  
  TypeRef i32 = MakeTypePrim("i32");
  
  TypeRef const_i32 = MakeTypeConst(i32);
  TypeRef unique_i32 = MakeTypeUnique(i32);
  TypeRef shared_i32 = MakeTypeShared(i32);
  
  // unique <: shared
  assert(IsSubtype(unique_i32, shared_i32) && "unique should be subtype of shared");
  
  // shared <: const
  assert(IsSubtype(shared_i32, const_i32) && "shared should be subtype of const");
  
  // unique <: const (transitivity)
  assert(IsSubtype(unique_i32, const_i32) && "unique should be subtype of const");
  
  // NOT: const <: unique
  assert(!IsSubtype(const_i32, unique_i32) && "const should NOT be subtype of unique");
  
  // NOT: shared <: unique
  assert(!IsSubtype(shared_i32, unique_i32) && "shared should NOT be subtype of unique");
  
  // NOT: const <: shared
  assert(!IsSubtype(const_i32, shared_i32) && "const should NOT be subtype of shared");
}

// Test: Self-subtyping reflexivity
void TestPermissionReflexivity() {
  SPEC_COV("C0X-Perm-Reflexive");
  
  TypeRef i32 = MakeTypePrim("i32");
  
  TypeRef const_i32 = MakeTypeConst(i32);
  TypeRef unique_i32 = MakeTypeUnique(i32);
  TypeRef shared_i32 = MakeTypeShared(i32);
  
  assert(IsSubtype(const_i32, const_i32) && "const should be subtype of itself");
  assert(IsSubtype(unique_i32, unique_i32) && "unique should be subtype of itself");
  assert(IsSubtype(shared_i32, shared_i32) && "shared should be subtype of itself");
}

// Test: Permission covariance through type constructors
void TestPermissionCovariance() {
  SPEC_COV("C0X-Perm-Covariance");
  
  TypeRef i32 = MakeTypePrim("i32");
  TypeRef bool_t = MakeTypePrim("bool");
  
  // Different base types should not be subtypes regardless of permission
  TypeRef unique_i32 = MakeTypeUnique(i32);
  TypeRef unique_bool = MakeTypeUnique(bool_t);
  
  assert(!IsSubtype(unique_i32, unique_bool) && 
         "Different base types should not be subtypes");
}

// Test: Permission lattice matches §7 spec definition
// §7: unique (~!) > shared (~%) > const (~)
void TestPermissionLatticeSpec() {
  SPEC_COV("C0X-7-PermissionLattice");
  
  TypeRef i32 = MakeTypePrim("i32");
  
  TypeRef const_i32 = MakeTypeConst(i32);
  TypeRef unique_i32 = MakeTypeUnique(i32);
  TypeRef shared_i32 = MakeTypeShared(i32);
  
  // §7: unique > shared > const (in terms of subtyping direction: unique <: shared <: const)
  // The lattice order means unique is "below" shared which is "below" const
  // In subtyping terms: more specific <: more general
  
  // Verify the complete lattice
  assert(IsSubtype(unique_i32, shared_i32) && "unique <: shared");
  assert(IsSubtype(unique_i32, const_i32) && "unique <: const");
  assert(IsSubtype(shared_i32, const_i32) && "shared <: const");
  
  // Verify anti-symmetry (not equal, so reverse should not hold)
  assert(!IsSubtype(shared_i32, unique_i32) && "shared is NOT <: unique");
  assert(!IsSubtype(const_i32, unique_i32) && "const is NOT <: unique");
  assert(!IsSubtype(const_i32, shared_i32) && "const is NOT <: shared");
}

// Test: Nested permission types
void TestNestedPermissionTypes() {
  SPEC_COV("C0X-Perm-Nested");
  
  TypeRef i32 = MakeTypePrim("i32");
  
  // const (const i32) should be valid
  TypeRef const_i32 = MakeTypeConst(i32);
  TypeRef const_const_i32 = MakeTypeConst(const_i32);
  
  // Reflexivity should still hold
  assert(IsSubtype(const_const_i32, const_const_i32) && 
         "Nested const should be subtype of itself");
}

// Test: Permission subtyping with same permission different types
void TestPermissionDifferentBaseTypes() {
  SPEC_COV("C0X-Perm-DifferentBase");
  
  TypeRef i32 = MakeTypePrim("i32");
  TypeRef i64 = MakeTypePrim("i64");
  
  TypeRef unique_i32 = MakeTypeUnique(i32);
  TypeRef shared_i64 = MakeTypeShared(i64);
  
  // Different base types should not be subtypes even with subtype permissions
  assert(!IsSubtype(unique_i32, shared_i64) && 
         "unique i32 should NOT be subtype of shared i64");
}

// Test: Permission with unit type
void TestPermissionUnitType() {
  SPEC_COV("C0X-Perm-Unit");
  
  TypeRef unit = MakeTypePrim("()");
  
  TypeRef const_unit = MakeTypeConst(unit);
  TypeRef unique_unit = MakeTypeUnique(unit);
  
  // Unit type should follow same rules
  assert(IsSubtype(unique_unit, const_unit) && 
         "unique () should be subtype of const ()");
}

}  // namespace

int main() {
  // §7 Permission lattice tests
  TestPermissionLatticeSubtyping();
  TestPermissionLatticeSpec();
  TestPermissionReflexivity();
  TestPermissionCovariance();
  TestNestedPermissionTypes();
  TestPermissionDifferentBaseTypes();
  TestPermissionUnitType();
  
  return 0;
}
