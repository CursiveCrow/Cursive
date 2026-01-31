// =============================================================================
// MIGRATION MAPPING: pattern/pattern_common.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 5.4: Pattern Matching
//   - PatJudg (line 9727): Pattern judgment
//   - Pat-Dup-Err (lines 9729-9732): Duplicate pattern name
//   - Distinct(PatNames(pat)) requirement
//   - Irrefutable vs refutable patterns
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/types/type_pattern.cpp
//   Common pattern typing utilities
//
// KEY CONTENT TO MIGRATE:
//   PATTERN COMMON UTILITIES:
//   - Pattern dispatch to specific handlers
//   - PatternResult type
//   - Binding collection and merging
//   - Duplicate name detection
//   - Irrefutability checking
//
//   PATTERN DISPATCH:
//   TypePattern(pattern, scrutinee_type) =
//     match pattern.kind {
//       Wildcard => wildcard_pattern.cpp
//       Identifier => identifier_pattern.cpp
//       Typed => typed_pattern.cpp
//       Literal => literal_pattern.cpp
//       Tuple => tuple_pattern.cpp
//       Record => record_pattern.cpp
//       Enum => enum_pattern.cpp
//       Modal => modal_pattern.cpp
//       Range => range_pattern.cpp
//     }
//
//   PATTERN RESULT:
//   PatternResult {
//     ok: bool,              // Pattern well-formed
//     bindings: [(name, type)], // Introduced bindings
//   }
//
//   DUPLICATE NAME CHECK (Pat-Dup-Err):
//   - All pattern names must be distinct
//   - Checked via Distinct(PatNames(pat))
//   - Error if duplicate name found
//
//   BINDING MERGE:
//   - Collect bindings from sub-patterns
//   - Check for conflicts (same name)
//   - Produce unified binding list
//
//   IRREFUTABILITY:
//   Irrefutable patterns (can use in let/var):
//   - Wildcard (_)
//   - Identifier (x)
//   - Tuple of irrefutable patterns
//   - Record pattern (exact type match)
//
//   Refutable patterns (require match):
//   - Literal (42, true)
//   - Typed (x: T)
//   - Enum (Result::Ok)
//   - Modal (@Connected)
//   - Range (0..10)
//
// DEPENDENCIES:
//   - All pattern-specific typing modules
//   - PatNames extraction
//   - Distinct check
//   - BindingMerge utility
//
// SPEC RULES:
//   - PatJudg (line 9727)
//   - Pat-Dup-Err (lines 9729-9732)
//   - Let-Refutable-Pattern-Err (lines 9773-9776)
//
// RESULT:
//   Dispatches to specific pattern handlers
//   Validates pattern well-formedness
//
// =============================================================================

