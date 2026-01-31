// ===========================================================================
// MIGRATION MAPPING: ast_patterns.h
// ===========================================================================
//
// PURPOSE:
//   Pattern AST node definitions. Contains all struct definitions for
//   pattern nodes used in match expressions, let bindings, and loop
//   iteration. Also includes MatchArm for match expression support.
//
// ---------------------------------------------------------------------------
// SPEC REFERENCE: C:\Dev\Cursive\C0updated.md
// ---------------------------------------------------------------------------
//   Section 3.3.2.5 - Pattern Nodes (Lines referenced in 3.3.9)
//
//   Pattern = (Span, PatternNode)
//   PatternNode variants:
//
//   BASIC:
//     LiteralPattern    - literal value (42, "hello", true)
//     WildcardPattern   - underscore (_)
//     IdentifierPattern - simple binding (x)
//     TypedPattern      - typed binding (x: T)
//
//   COMPOUND:
//     TuplePattern  - tuple destructure ((a, b))
//     RecordPattern - record destructure (Point{x, y})
//     EnumPattern   - enum destructure (Result::Ok(v))
//     ModalPattern  - modal state destructure (@State{fields})
//
//   SPECIAL:
//     RangePattern - range match (0..10, 0..=10)
//
//   Supporting types:
//     FieldPattern       - record field with optional nested pattern
//     EnumPayloadPattern - tuple or record payload for enum patterns
//     ModalRecordPayload - fields for modal pattern
//
// ---------------------------------------------------------------------------
// SOURCE FILE: C:\Dev\Cursive\cursive-bootstrap\include\cursive0\02_syntax\ast.h
// ---------------------------------------------------------------------------
//
// 1. BASIC PATTERNS (Lines 335-348)
//    ─────────────────────────────────────────────────────────────────────────
//    struct LiteralPattern { Token literal; };          (Lines 335-337)
//    struct WildcardPattern {};                         (Line 339)
//    struct IdentifierPattern { Identifier name; };     (Lines 341-343)
//    struct TypedPattern {                              (Lines 345-348)
//      Identifier name;
//      shared_ptr<Type> type;
//    };
//
// 2. COMPOUND PATTERNS (Lines 350-388)
//    ─────────────────────────────────────────────────────────────────────────
//    struct TuplePattern {                              (Lines 350-352)
//      vector<PatternPtr> elements;
//    };
//
//    struct FieldPattern {                              (Lines 354-358)
//      Identifier name;
//      PatternPtr pattern_opt;
//      Span span;
//    };
//
//    struct RecordPattern {                             (Lines 360-363)
//      TypePath path;
//      vector<FieldPattern> fields;
//    };
//
//    struct TuplePayloadPattern {                       (Lines 365-367)
//      vector<PatternPtr> elements;
//    };
//
//    struct RecordPayloadPattern {                      (Lines 369-371)
//      vector<FieldPattern> fields;
//    };
//
//    using EnumPayloadPattern = variant<TuplePayloadPattern, RecordPayloadPattern>;
//                                                       (Line 373)
//
//    struct EnumPattern {                               (Lines 375-379)
//      TypePath path;
//      Identifier name;
//      optional<EnumPayloadPattern> payload_opt;
//    };
//
//    struct ModalRecordPayload {                        (Lines 381-383)
//      vector<FieldPattern> fields;
//    };
//
//    struct ModalPattern {                              (Lines 385-388)
//      Identifier state;
//      optional<ModalRecordPayload> fields_opt;
//    };
//
// 3. RANGE PATTERN (Lines 390-394)
//    ─────────────────────────────────────────────────────────────────────────
//    struct RangePattern {
//      RangeKind kind;
//      PatternPtr lo;
//      PatternPtr hi;
//    };
//
// 4. PATTERN NODE VARIANT (Lines 396-405)
//    ─────────────────────────────────────────────────────────────────────────
//    using PatternNode = variant<LiteralPattern, WildcardPattern,
//                                 IdentifierPattern, TypedPattern,
//                                 TuplePattern, RecordPattern,
//                                 EnumPattern, ModalPattern,
//                                 RangePattern>;
//
// 5. PATTERN WRAPPER (Lines 407-410)
//    ─────────────────────────────────────────────────────────────────────────
//    struct Pattern {
//      Span span;
//      PatternNode node;
//    };
//
// 6. MATCH ARM (Lines 412-417) - Also used by exprs.h
//    ─────────────────────────────────────────────────────────────────────────
//    struct MatchArm {
//      shared_ptr<Pattern> pattern;
//      ExprPtr guard_opt;
//      ExprPtr body;
//      Span span;
//    };
//
// ---------------------------------------------------------------------------
// DEPENDENCIES
// ---------------------------------------------------------------------------
//   From ast_fwd.h:
//     - PatternPtr, ExprPtr, TypePath, Identifier
//
//   From ast_enums.h:
//     - RangeKind
//
//   From 00_core/span.h:
//     - core::Span
//
//   From lexer:
//     - Token (for LiteralPattern)
//
// ---------------------------------------------------------------------------
// REFACTORING NOTES
// ---------------------------------------------------------------------------
//   1. MatchArm is technically an expression support structure but is
//      defined here since it contains a pattern. Both exprs.h and this
//      file need access to it.
//
//   2. The FieldPattern struct has pattern_opt which is optional.
//      This supports shorthand like Point{x, y} where field name
//      implies the binding name.
//
//   3. Pattern exhaustiveness checking is done in analysis phase,
//      not in the AST. The AST just represents the syntactic structure.
//
//   4. Consider adding a PatternContext enum to track where patterns
//      are used (match, let, loop) for better error messages.
//
//   5. EnumPayloadPattern and ModalRecordPayload are variants/structs
//      that mirror expression equivalents. Consider unifying with
//      common Field/Element types.
//
//   6. RangePattern uses RangeKind but only Exclusive (..) and
//      Inclusive (..=) are valid in patterns. Other RangeKind values
//      are expression-only.
//
// ===========================================================================

// TODO: Migrate pattern definitions from ast.h lines 335-417

