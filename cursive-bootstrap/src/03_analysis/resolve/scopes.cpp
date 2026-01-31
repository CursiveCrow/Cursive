#include "cursive0/03_analysis/resolve/scopes.h"

#include <string>
#include <string_view>
#include <vector>

#include "cursive0/00_core/assert_spec.h"
#include "cursive0/00_core/unicode.h"
#include "cursive0/02_syntax/keyword_policy.h"

namespace cursive0::analysis {

namespace {

static inline void SpecDefsIdKeys() {
  SPEC_DEF("IdKey", "3.1.6");
  SPEC_DEF("IdEq", "3.1.6");
  SPEC_DEF("PathKey", "3.1.6");
  SPEC_DEF("PathEq", "3.1.6");
}

static inline void SpecDefsScopeKeys() {
  SPEC_DEF("IdKeyRef", "5.1.1");
  SPEC_DEF("ScopeKey", "5.1.1");
}

static inline void SpecDefsNames() {
  SPEC_DEF("PrimTypeNames", "5.1.1");
  SPEC_DEF("SpecialTypeNames", "5.1.1");
  SPEC_DEF("AsyncTypeNames", "5.1.1");
  SPEC_DEF("PrimTypeKeys", "5.1.1");
  SPEC_DEF("SpecialTypeKeys", "5.1.1");
  SPEC_DEF("AsyncTypeKeys", "5.1.1");
}

static inline void SpecDefsReserved() {
  SPEC_DEF("UniverseProtectedRef", "5.1.1");
  SPEC_DEF("UniverseBindings", "5.1.1");
  SPEC_DEF("BytePrefix", "5.1.1");
  SPEC_DEF("Prefix", "5.1.1");
  SPEC_DEF("ReservedGen", "5.1.1");
  SPEC_DEF("ReservedCursive", "5.1.1");
  SPEC_DEF("ReservedId", "5.1.1");
  SPEC_DEF("ReservedModulePath", "5.1.1");
  SPEC_DEF("KeywordKey", "5.1.1");
}

const std::vector<std::string_view>& UniverseProtectedNames() {
  SpecDefsReserved();
  static const std::vector<std::string_view> names = {
      "i8",     "i16",    "i32",    "i64",    "i128",   "u8",
      "u16",    "u32",    "u64",    "u128",   "f16",    "f32",
      "f64",    "bool",   "char",   "usize",  "isize",  "Self",
      "Drop", "Bitcopy", "Clone", "Eq", "Hash", "Hasher", "Iterator", "Step",
      "FfiSafe", "string", "bytes",  "Modal",  "Region", "RegionOptions",
      "CancelToken", "Context", "System", "ExecutionDomain", "Reactor",
      "CpuSet", "Priority", "Async", "Future", "Sequence", "Stream", "Pipe",
      "Exchange", "Tracked", "Spawned"};
  return names;
}

std::vector<IdKey> MakeKeys(const std::vector<std::string_view>& names) {
  SpecDefsNames();
  std::vector<IdKey> keys;
  keys.reserve(names.size());
  for (const auto name : names) {
    keys.push_back(IdKeyOf(name));
  }
  return keys;
}

}  // namespace

IdKey IdKeyOf(std::string_view s) {
  SpecDefsIdKeys();
  return core::NFC(s);
}

bool IdEq(std::string_view s1, std::string_view s2) {
  SpecDefsIdKeys();
  return IdKeyOf(s1) == IdKeyOf(s2);
}

PathKey PathKeyOf(const syntax::Path& path) {
  SpecDefsIdKeys();
  PathKey out;
  out.reserve(path.size());
  for (const auto& comp : path) {
    out.push_back(IdKeyOf(comp));
  }
  return out;
}

bool PathEq(const syntax::Path& p, const syntax::Path& q) {
  SpecDefsIdKeys();
  return PathKeyOf(p) == PathKeyOf(q);
}

bool ScopeKey(const Scope& scope) {
  SpecDefsScopeKeys();
  for (const auto& [key, ent] : scope) {
    (void)ent;
    if (core::NFC(key) != key) {
      return false;
    }
  }
  return true;
}

bool BytePrefix(std::string_view p, std::string_view s) {
  SpecDefsReserved();
  return s.size() >= p.size() && s.compare(0, p.size(), p) == 0;
}

bool Prefix(std::string_view s, std::string_view p) {
  SpecDefsReserved();
  return BytePrefix(p, s);
}

bool ReservedGen(std::string_view x) {
  SpecDefsReserved();
  return Prefix(IdKeyOf(x), IdKeyOf("gen_"));
}

bool ReservedCursive(std::string_view x) {
  SpecDefsReserved();
  return IdEq(x, "cursive");
}

bool ReservedId(std::string_view x) {
  SpecDefsReserved();
  return ReservedGen(x) || ReservedCursive(x);
}

bool ReservedModulePath(const syntax::ModulePath& path) {
  SpecDefsReserved();
  if (!path.empty() && IdEq(path[0], "cursive")) {
    return true;
  }
  for (const auto& comp : path) {
    if (ReservedGen(comp)) {
      return true;
    }
  }
  return false;
}

const std::vector<std::string_view>& PrimTypeNames() {
  SpecDefsNames();
  static const std::vector<std::string_view> names = {
      "i8",   "i16",  "i32",  "i64",  "i128", "u8",  "u16", "u32",
      "u64",  "u128", "f16",  "f32",  "f64",  "bool", "char", "usize",
      "isize"};
  return names;
}

const std::vector<std::string_view>& SpecialTypeNames() {
  SpecDefsNames();
  static const std::vector<std::string_view> names = {
      "Self", "Drop", "Bitcopy", "Clone", "Eq", "Hash", "Hasher", "Iterator",
      "Step", "FfiSafe", "string", "bytes", "Modal", "Region",
      "RegionOptions", "CancelToken", "Context", "System", "ExecutionDomain",
      "CpuSet", "Priority", "Reactor"};
  return names;
}

const std::vector<std::string_view>& AsyncTypeNames() {
  SpecDefsNames();
  static const std::vector<std::string_view> names = {
      "Async", "Future", "Sequence", "Stream", "Pipe", "Exchange",
      "Tracked"};
  return names;
}

const std::vector<IdKey>& PrimTypeKeys() {
  SpecDefsNames();
  static const std::vector<IdKey> keys = MakeKeys(PrimTypeNames());
  return keys;
}

const std::vector<IdKey>& SpecialTypeKeys() {
  SpecDefsNames();
  static const std::vector<IdKey> keys = MakeKeys(SpecialTypeNames());
  return keys;
}

const std::vector<IdKey>& AsyncTypeKeys() {
  SpecDefsNames();
  static const std::vector<IdKey> keys = MakeKeys(AsyncTypeNames());
  return keys;
}

bool KeywordKey(std::string_view idkey) {
  SpecDefsReserved();
  return syntax::IsKeyword(idkey);
}

Scope UniverseBindings() {
  SpecDefsReserved();
  Scope scope;
  for (const auto name : UniverseProtectedNames()) {
    scope.emplace(IdKeyOf(name),
                  Entity{EntityKind::Type, std::nullopt, std::nullopt,
                         EntitySource::Decl});
  }
  scope.emplace(IdKeyOf("cursive"),
                Entity{EntityKind::ModuleAlias, syntax::ModulePath{"cursive"},
                       std::nullopt, EntitySource::Decl});
  return scope;
}

}  // namespace cursive0::analysis
