// ===========================================================================
// MIGRATION MAPPING: build_common.cpp
// ===========================================================================
//
// PURPOSE:
//   Common span construction utilities and shared builder infrastructure.
//   Provides foundational functions used by all category-specific builders.
//
// ===========================================================================
// SPEC REFERENCE: C:\Dev\Cursive\C0updated.md
// ===========================================================================
//   Section 3.3.2 - AST Node Catalog
//   Lines 2626-2638: Node construction helpers
//
//   Key definitions:
//   - SpanDefault(P, P') = SpanBetween(P, P') (2626)
//   - DocDefault = [] (2627)
//   - DocOptDefault = bottom (2628)
//   - FillSpan(n, P, P') = n[span := SpanDefault(P, P')] if SpanMissing(n), else n (2629-2631)
//   - FillDoc(n) = n[doc := DocDefault] if DocMissing(n), else n (2632-2634)
//   - FillDocOpt(n) = n[doc_opt := DocOptDefault] if DocOptMissing(n), else n (2635-2637)
//   - ParseCtor(n, P, P') = FillDocOpt(FillDoc(FillSpan(n, P, P'))) (2638)
//
// ---------------------------------------------------------------------------
// SOURCE FILE: C:\Dev\Cursive\cursive-bootstrap\src\02_syntax\parser.cpp
// ---------------------------------------------------------------------------
//   Lines 30-36: SpanFrom(start, end)
//     - Combines start token's start position with end token's end position
//     - Used to create spans covering multiple tokens
//
//   Lines 76-85: SpanBetween(start_parser, end_parser)
//     - Creates span between two parser positions
//     - Handles EOF token specially
//     - Used for SpanDefault in ParseCtor
//
// ---------------------------------------------------------------------------
// DEPENDENCIES
// ---------------------------------------------------------------------------
//   Internal:
//   - 00_core/span.h: core::Span, Position
//   - lexer/token.h: Token for span extraction
//
//   External:
//   - Parser state (for position tracking)
//
// ---------------------------------------------------------------------------
// REFACTORING NOTES
// ---------------------------------------------------------------------------
//   1. Span construction should be agnostic to AST node types
//   2. Consider caching computed spans for repeated access
//   3. Debug builds should validate span ordering (start <= end)
//   4. EOF handling is critical for spans at end-of-file
//
// ===========================================================================

// TODO: Migrate implementation from source files listed above
//
// namespace cursive::ast {
//
// // ---------------------------------------------------------------------------
// // SPAN CONSTRUCTION
// // ---------------------------------------------------------------------------
//
// /// Creates a span from a start token to an end token.
// /// Takes start position from start token, end position from end token.
// ///
// /// @param start Token marking span start
// /// @param end   Token marking span end
// /// @return Combined span covering both tokens
// Span span_from(const Token& start, const Token& end);
//
// /// Creates a span between two parser positions.
// /// Handles EOF token specially (uses last valid position).
// ///
// /// @param start_pos Starting parser position
// /// @param end_pos   Ending parser position
// /// @return Span covering the range
// Span span_between(Position start_pos, Position end_pos);
//
// // ---------------------------------------------------------------------------
// // DEFAULT INITIALIZATION
// // ---------------------------------------------------------------------------
//
// /// Returns the default (empty) documentation list.
// /// Per spec: DocDefault = []
// DocList doc_default();
//
// /// Returns the default (absent) optional documentation.
// /// Per spec: DocOptDefault = bottom (none)
// optional<DocList> doc_opt_default();
//
// // ---------------------------------------------------------------------------
// // PARSECTOR HELPERS
// // ---------------------------------------------------------------------------
//
// /// Fills span if missing.
// /// Per spec: FillSpan(n, P, P') = n[span := SpanDefault(P, P')] if SpanMissing(n)
// template<typename Node>
// Node fill_span(Node n, Position start, Position end);
//
// /// Fills documentation if missing.
// /// Per spec: FillDoc(n) = n[doc := DocDefault] if DocMissing(n)
// template<typename Node>
// Node fill_doc(Node n);
//
// /// Fills optional documentation if missing.
// /// Per spec: FillDocOpt(n) = n[doc_opt := DocOptDefault] if DocOptMissing(n)
// template<typename Node>
// Node fill_doc_opt(Node n);
//
// /// Complete node initialization for parser construction.
// /// Per spec: ParseCtor(n, P, P') = FillDocOpt(FillDoc(FillSpan(n, P, P')))
// template<typename Node>
// Node parse_ctor(Node n, Position start, Position end);
//
// // ---------------------------------------------------------------------------
// // SPAN VALIDATION (DEBUG)
// // ---------------------------------------------------------------------------
//
// /// Validates that a span has correct ordering (start <= end).
// /// Only active in debug builds.
// bool validate_span_ordering(const Span& span);
//
// /// Validates that child span is contained within parent span.
// /// Only active in debug builds.
// bool validate_span_containment(const Span& parent, const Span& child);
//
// } // namespace cursive::ast

