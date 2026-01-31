// =============================================================================
// MIGRATION MAPPING: pattern/identifier_pattern.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 5.4: Pattern Matching
//   - Identifier patterns bind a name to the matched value
//   - Irrefutable - always matches
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/types/type_pattern.cpp
//   Identifier pattern typing
//
// KEY CONTENT TO MIGRATE:
//   IDENTIFIER PATTERN:
//   - Pattern: name
//   - Binds the entire matched value to name
//   - Type is the scrutinee type
//   - Always irrefutable (matches any value)
//
//   TYPING:
//   1. Receive scrutinee type T
//   2. Create binding (name, T)
//   3. Return binding list [(name, T)]
//
//   CONTEXT:
//   - In let/var: binds init expression
//   - In match arm: binds scrutinee
//   - In for loop: binds iteration element
//   - In function param: binds argument
//
// DEPENDENCIES:
//   - PatternResult { bool ok; vector<Binding> bindings; }
//   - Binding = (name, type)
//
// SPEC RULES:
//   - P-Ident: Identifier pattern
//     pat = Identifier(name), T scrutinee type
//     => bindings = [(name, T)]
//
// RESULT TYPE:
//   PatternResult { ok: true, bindings: [(name, T)] }
//
// =============================================================================

