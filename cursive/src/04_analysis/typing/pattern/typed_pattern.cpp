// =============================================================================
// MIGRATION MAPPING: pattern/typed_pattern.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 5.4: Pattern Matching
//   - Typed patterns: name: Type
//   - Used in match arms to narrow type
//   - Refutable in union context
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/types/type_pattern.cpp
//   Typed pattern typing
//
// KEY CONTENT TO MIGRATE:
//   TYPED PATTERN:
//   - Pattern: name: T_p
//   - Binds name with explicit type annotation
//   - Checks scrutinee type against pattern type
//   - Refutable if scrutinee is union
//
//   TYPING:
//   1. Lower pattern type T_p
//   2. Check scrutinee type T_s is compatible:
//      - If T_s = T_p: always matches (irrefutable)
//      - If T_s = Union containing T_p: may match (refutable)
//      - If T_s not compatible with T_p: error
//   3. Create binding (name, T_p)
//
//   UNION NARROWING:
//   - Scrutinee: i32 | bool
//   - Pattern: x: i32
//   - Narrows to i32 in arm body
//   - Other arms must cover remaining types
//
// DEPENDENCIES:
//   - LowerType() for pattern type
//   - TypeEquiv() for exact match
//   - Member() for union membership
//   - PatternResult { bool ok; vector<Binding> bindings; }
//
// SPEC RULES:
//   - P-Typed: Typed pattern
//   - P-Typed-Union: Typed pattern in union context
//   - Pattern-TypeMismatch-Err: Incompatible types
//
// RESULT TYPE:
//   PatternResult { ok: true, bindings: [(name, T_p)] }
//
// =============================================================================

