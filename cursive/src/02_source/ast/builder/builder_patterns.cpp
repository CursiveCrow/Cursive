// ===========================================================================
// MIGRATION MAPPING: build_patterns.cpp
// ===========================================================================
//
// PURPOSE:
//   Factory functions for constructing Pattern AST nodes. Provides builders
//   for all 10 pattern node variants defined in ast_patterns.h.
//
// ===========================================================================
// SPEC REFERENCE: C:\Dev\Cursive\C0updated.md
// ===========================================================================
//   Section 3.3.6 - ParsePattern rules
//   Defines parsing rules that construct pattern nodes.
//
//   Pattern categories (from spec Section 7):
//   - Irrefutable: wildcard, identifier, typed, tuple, record
//   - Refutable: literal, enum, modal state, range, guard
//
//   Pattern variants:
//   - PatternWildcard: _ (matches anything)
//   - PatternIdentifier: name (binds value)
//   - PatternTyped: name: Type (binds with type)
//   - PatternLiteral: 42, true, 'x' (matches literal)
//   - PatternTuple: (a, b) (destructures tuple)
//   - PatternRecord: Point{ x, y } (destructures record)
//   - PatternEnum: Enum::Variant(payload) (matches enum)
//   - PatternModal: @State{ fields } (matches modal state)
//   - PatternRange: 0..10, 0..=10 (matches range)
//
// ---------------------------------------------------------------------------
// SOURCE FILE: C:\Dev\Cursive\cursive-bootstrap\src\02_syntax\parser_patterns.cpp
// ---------------------------------------------------------------------------
//   Pattern construction patterns:
//   - make_shared<Pattern> with PatternNode variant
//   - Destructuring pattern construction
//   - Guard attachment to refutable patterns
//
// ---------------------------------------------------------------------------
// DEPENDENCIES
// ---------------------------------------------------------------------------
//   Internal:
//   - ast/nodes/ast_fwd.h: PatternPtr, Identifier
//   - ast/nodes/ast_enums.h: RangeKind
//   - ast/nodes/ast_patterns.h: All pattern node variant definitions
//   - build_common.cpp: span_from, span_between
//   - 00_core/span.h: Span
//   - lexer/token.h: Token for literal patterns
//
// ---------------------------------------------------------------------------
// REFACTORING NOTES
// ---------------------------------------------------------------------------
//   1. Irrefutable patterns: wildcard, identifier, typed, tuple, record
//   2. Refutable patterns: literal, enum, modal, range, guard
//   3. Guard patterns wrap another pattern with a boolean condition
//   4. Validate pattern is appropriate for context (let vs match)
//
// ===========================================================================

// TODO: Migrate implementation from source files listed above
//
// namespace cursive::ast {
//
// // ---------------------------------------------------------------------------
// // IRREFUTABLE PATTERNS (for let/var bindings)
// // ---------------------------------------------------------------------------
//
// /// Creates a wildcard pattern (_).
// /// Matches any value without binding.
// /// @param span Source location
// PatternPtr make_pattern_wildcard(Span span);
//
// /// Creates an identifier pattern.
// /// Binds the matched value to the given name.
// /// @param name Binding identifier
// /// @param span Source location
// PatternPtr make_pattern_identifier(Identifier name, Span span);
//
// /// Creates a typed pattern (name: Type).
// /// Binds value with explicit type annotation.
// /// @param name Binding identifier
// /// @param ty Type annotation
// /// @param span Source location
// PatternPtr make_pattern_typed(Identifier name, shared_ptr<Type> ty, Span span);
//
// /// Creates a tuple destructuring pattern ((a, b)).
// /// @param elements Sub-patterns for each tuple element
// /// @param span Source location
// PatternPtr make_pattern_tuple(vector<PatternPtr> elements, Span span);
//
// /// Creates a record destructuring pattern (Point{ x, y }).
// /// @param path Optional type path for the record
// /// @param fields Field patterns
// /// @param span Source location
// PatternPtr make_pattern_record(optional<TypePath> path, vector<FieldPattern> fields, Span span);
//
// // ---------------------------------------------------------------------------
// // REFUTABLE PATTERNS (for match arms)
// // ---------------------------------------------------------------------------
//
// /// Creates a literal pattern.
// /// Matches exact literal value.
// /// @param lit Literal token (integer, float, bool, char, string)
// /// @param span Source location
// PatternPtr make_pattern_literal(Token lit, Span span);
//
// /// Creates an enum variant pattern (Enum::Variant(payload)).
// /// @param path Enum type path
// /// @param name Variant name
// /// @param payload Optional payload pattern
// /// @param span Source location
// PatternPtr make_pattern_enum(TypePath path, Identifier name, optional<EnumPayloadPattern> payload, Span span);
//
// /// Creates a modal state pattern (@State{ fields }).
// /// @param state State identifier
// /// @param fields Optional field patterns
// /// @param span Source location
// PatternPtr make_pattern_modal(Identifier state, optional<ModalRecordPayload> fields, Span span);
//
// /// Creates a range pattern (lo..hi or lo..=hi).
// /// @param kind Range kind (exclusive or inclusive)
// /// @param lo Lower bound pattern
// /// @param hi Upper bound pattern
// /// @param span Source location
// PatternPtr make_pattern_range(RangeKind kind, PatternPtr lo, PatternPtr hi, Span span);
//
// // ---------------------------------------------------------------------------
// // GUARD PATTERNS
// // ---------------------------------------------------------------------------
//
// /// Creates a guard pattern (pattern if condition).
// /// @param pattern Inner pattern to match first
// /// @param guard Guard condition expression
// /// @param span Source location
// PatternPtr make_pattern_guard(PatternPtr pattern, ExprPtr guard, Span span);
//
// } // namespace cursive::ast

