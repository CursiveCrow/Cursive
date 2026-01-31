// =============================================================================
// MIGRATION MAPPING: scopes.cpp
// =============================================================================
//
// SPEC REFERENCE:
//   C0updated.md §5.1.1 "Scope Context and Identifiers" (Lines 6659-6717)
//   C0updated.md §3.1.6 "IdKey, IdEq, PathKey, PathEq"
//
// SOURCE FILE:
//   cursive-bootstrap/src/03_analysis/resolve/scopes.cpp (Lines 1-213)
//
// DEPENDENCIES:
//   - cursive/include/04_analysis/resolve/scopes.h (header)
//   - cursive/src/00_core/unicode.h (NFC normalization)
//   - cursive/src/02_source/keyword_policy.h (IsKeyword)
//
// =============================================================================
// CONTENT TO MIGRATE:
// =============================================================================
//
// 1. IdKey/IdEq Functions (Source Lines 74-97)
//    - IdKeyOf(string_view) -> IdKey: NFC-normalize identifier
//    - IdEq(s1, s2) -> bool: Compare via IdKey
//    - PathKeyOf(Path) -> PathKey: Map each component through IdKey
//    - PathEq(p1, p2) -> bool: Compare via PathKey
//
// 2. ScopeKey Validation (Source Lines 99-108)
//    - ScopeKey(Scope) -> bool: Verify all keys are NFC-normalized
//
// 3. Reserved Identifier Functions (Source Lines 110-146)
//    - BytePrefix(p, s) -> bool: Check string prefix (byte-level)
//    - Prefix(s, p) -> bool: Wrapper for BytePrefix
//    - ReservedGen(x) -> bool: Check if starts with "gen_"
//    - ReservedCursive(x) -> bool: Check if equals "cursive"
//    - ReservedId(x) -> bool: ReservedGen OR ReservedCursive
//    - ReservedModulePath(path) -> bool: Check module path for reserved
//
// 4. Type Name Constants (Source Lines 148-191)
//    - PrimTypeNames(): i8-i128, u8-u128, f16-f64, bool, char, usize, isize
//    - SpecialTypeNames(): Self, Drop, Bitcopy, Clone, Eq, Hash, etc.
//    - AsyncTypeNames(): Async, Future, Sequence, Stream, Pipe, Exchange, Tracked
//    - PrimTypeKeys(), SpecialTypeKeys(), AsyncTypeKeys(): Cached key vectors
//
// 5. KeywordKey Function (Source Lines 193-196)
//    - KeywordKey(idkey) -> bool: Check if key is a keyword
//
// 6. UniverseBindings (Source Lines 198-210)
//    - UniverseBindings() -> Scope: Built-in universe scope with:
//      - All protected type names mapped to Type entities
//      - "cursive" mapped to ModuleAlias entity
//
// =============================================================================
// SPEC DEFINITIONS TO IMPLEMENT:
// =============================================================================
//
// From §5.1.1:
//   IdKeyRef = {"3.1.6"}
//   ScopeKey(S) ⇔ dom(S) ⊆ {IdKey(x) | x ∈ Identifier}
//
//   EntityKind = {Value, Type, Class, ModuleAlias}
//   EntitySource = {Decl, Using, RegionAlias}
//   Entity = ⟨kind, origin_opt, target_opt, source⟩
//
//   S : IdKey ⇀ Entity
//   Scopes(Γ) = [S_1, …, S_k, S_proc, S_module, S_universe]
//
//   UniverseProtectedRef = {"3.2.3"}
//   UniverseBindings = { IdKey(x) ↦ ⟨Type, ⊥, ⊥, Decl⟩ | x ∈ UniverseProtected }
//                    ∪ { IdKey(`cursive`) ↦ ⟨ModuleAlias, `cursive`, ⊥, Decl⟩ }
//
//   BytePrefix(p, s) ⇔ ∃ r. s = p ++ r
//   Prefix(s, p) ⇔ BytePrefix(p, s)
//   ReservedGen(x) ⇔ Prefix(IdKey(x), IdKey(`gen_`))
//   ReservedCursive(x) ⇔ IdEq(x, `cursive`)
//   ReservedId(x) ⇔ ReservedGen(x) ∨ ReservedCursive(x)
//   ReservedModulePath(path) ⇔ (|path| ≥ 1 ∧ IdEq(path[0], `cursive`))
//                            ∨ (∃ i. ReservedGen(path[i]))
//
//   PrimTypeNames = {`i8`, `i16`, ..., `isize`}
//   SpecialTypeNames = {`Self`, `Drop`, `Bitcopy`, ..., `Reactor`}
//   AsyncTypeNames = {`Async`, `Future`, `Sequence`, ..., `Tracked`}
//
//   KeywordKey(n) ⇔ ∃ s. n = IdKey(s) ∧ Keyword(s)
//
// =============================================================================
// REFACTORING NOTES:
// =============================================================================
//
// 1. Use std::unordered_map instead of map for Scope if performance critical
// 2. Consider constexpr for name arrays if C++20 available
// 3. MakeKeys helper can be inline or removed if using initializer lists
// 4. Ensure Entity struct matches spec tuple definition exactly
// 5. UniverseProtectedNames should match SpecialTypeNames union
//
// =============================================================================

