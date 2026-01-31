// =============================================================================
// MIGRATION MAPPING: parser.cpp
// =============================================================================
//
// DESTINATION: cursive/src/02_source/parser/parser.cpp
// PURPOSE: Top-level parsing entry points (ParseFile, ParseItems, ParseItem)
//
// =============================================================================
// SPEC REFERENCES
// =============================================================================
//
// C0updated.md Section 3.3.1 (Lines 2589-2619): Inputs, Outputs, and Invariants
//   - ParseInputs, ParseUnitSources, ParseOutputs definitions
//   - Phase1-Complete, Phase1-Declarations, Phase1-Forward-Refs invariants
//
// C0updated.md Section 3.3.6 (Lines 3440-3467): Module and Item Parsing
//   - ParseFile-Ok rule (lines 3447-3450)
//   - ParseItems-Empty rule (lines 3456-3459)
//   - ParseItems-Cons rule (lines 3461-3464)
//
// C0updated.md Section 3.3.3 (Lines 2966-3018): Parser State and Judgments
//   - PState structure definition
//   - Helper functions: Tok, Advance, Clone, MergeDiag
//   - AdvanceOrEOF, SpanBetween definitions
//
// =============================================================================
// SOURCE FILES
// =============================================================================
//
// PRIMARY SOURCE: cursive-bootstrap/src/02_syntax/parser.cpp
//   Lines 1-206: Complete file
//
// FUNCTIONS TO MIGRATE:
//
// 1. Anonymous namespace helpers (lines 21-45):
//    - AppendDiags (lines 23-28): Merge diagnostic streams
//    - SpanFrom (lines 30-36): Create span from start/end tokens
//    - EofAsToken (lines 38-44): Convert EOF marker to Token
//
// 2. AdvanceOrEOF (lines 47-54):
//    SPEC: Section 3.3.3 lines 2995-2997
//    - Advance parser if not at EOF, otherwise return unchanged
//
// 3. Clone (lines 56-60):
//    SPEC: Section 3.3.3 line 2989
//    - Clone parser state with cleared diagnostics
//
// 4. MergeDiag (lines 62-67):
//    SPEC: Section 3.3.3 line 2990
//    - Merge diagnostics from base and diag parsers into src parser
//
// 5. PStateOk (lines 69-74):
//    SPEC: Section 3.3.3 line 2993
//    - Parser index invariant validation
//
// 6. SpanBetween (lines 76-85):
//    SPEC: Section 3.3.3 line 3002
//    - Compute span between two parser states
//
// 7. ParseItemsInternal (lines 87-152) [static]:
//    SPEC: Section 3.3.6 lines 3454-3464
//    - Internal recursive item parsing with module docs
//    - Implements ParseItems-Empty and ParseItems-Cons rules
//    - Debug output via CURSIVE0_DEBUG_PARSE environment variable
//    - Error recovery with ErrorItem insertion
//
// 8. ParseItems (lines 154-156):
//    SPEC: Section 3.3.6 lines 3454-3464
//    - Public entry point for item sequence parsing
//    - Extracts module docs before parsing
//
// 9. ParseFile (lines 158-196):
//    SPEC: Section 3.3.6 lines 3442-3450
//    - Complete file parsing entry point
//    - Orchestrates: Tokenize -> FilterNewlines -> ParseItems -> AttachLineDocs
//    - Returns ASTFile with items, module docs, unsafe spans
//
// 10. ParseFileBestEffort (lines 198-200):
//     - Returns true if file was parsed (even with errors)
//
// 11. ParseFileOk (lines 202-204):
//     - Returns true if file parsed without errors
//
// =============================================================================
// DEPENDENCIES
// =============================================================================
//
// REQUIRED HEADERS:
//   - cursive/include/02_source/parser/parser.hpp
//   - cursive/include/00_core/diagnostics.hpp
//   - cursive/include/00_core/span.hpp
//   - cursive/include/02_source/lexer/lexer.hpp
//   - cursive/include/02_source/ast/ast.hpp
//
// REQUIRED FUNCTIONS (from other files):
//   - Tokenize (lexer.cpp)
//   - FilterNewlines (lexer.cpp)
//   - UnsafeSpans (lexer.cpp)
//   - MakeParser (parser_state.cpp)
//   - Tok, Advance, AtEof (parser_state.cpp)
//   - TokSpan (parser_state.cpp)
//   - ParseItem (parser_items.cpp)
//   - ModuleDocs, AttachLineDocs (parser_docs.cpp)
//   - EmitParseSyntaxErr (parser_recovery.cpp)
//
// TYPES REQUIRED:
//   - Parser, Token, TokenKind
//   - ParseItemResult, ParseItemsResult, ParseFileResult
//   - ASTFile, ASTItem, DocComment
//   - core::DiagnosticStream, core::Span, core::SourceFile
//
// =============================================================================
// REFACTORING NOTES
// =============================================================================
//
// 1. DIAGNOSTIC HELPER:
//    The AppendDiags helper appears in multiple source files. Consider moving
//    to a shared utility in 00_core/diagnostics.cpp with signature:
//      void AppendDiags(core::DiagnosticStream& out, const core::DiagnosticStream& add);
//
// 2. DEBUG INFRASTRUCTURE:
//    The CURSIVE0_DEBUG_PARSE and CURSIVE0_DEBUG_PHASES environment checks
//    should be consolidated into a debug configuration system in 00_core/.
//
// 3. SPEC RULES:
//    All SPEC_RULE markers must be preserved for traceability:
//    - "ParseItems-Empty" (line 100)
//    - "ParseItems-Cons" (line 107)
//    - "ParseFile-Ok" (line 185)
//
// 4. ERROR RECOVERY:
//    The error recovery logic in ParseItemsInternal (lines 133-143) creates
//    ErrorItem nodes when parsing stalls. Ensure this matches the spec's
//    recovery semantics.
//
// 5. SPAN CALCULATION:
//    SpanBetween uses EofAsToken as fallback. Verify edge cases where both
//    start and end are at EOF.
//
// 6. NAMESPACE:
//    Source uses namespace cursive0::syntax. Migrate to appropriate namespace
//    in the refactored codebase (likely cursive::syntax or cursive::parser).
//
// =============================================================================
