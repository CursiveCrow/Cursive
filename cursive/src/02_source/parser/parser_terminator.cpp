// =============================================================================
// MIGRATION MAPPING: parser_terminator.cpp
// =============================================================================
//
// DESTINATION: cursive/src/02_source/parser/parser_terminator.cpp
// PURPOSE: Statement terminator handling (newline or semicolon consumption)
//
// =============================================================================
// SPEC REFERENCES
// =============================================================================
//
// C0updated.md Section 3.2.3 - Statement Termination
//   Cursive uses newline OR semicolon as statement terminators.
//   Both are valid; semicolon allows multiple statements per line.
//
// C0updated.md Section 2.7 - Statement Termination (from LLM guide)
//   "Statements require a terminator (`;` or newline). Use newlines by default."
//
//   ```cursive
//   let x: i32 = 10       // newline terminates
//   let y: i32 = 20;      // semicolon terminates (same line allowed)
//   ```
//
// =============================================================================
// SOURCE FILES
// =============================================================================
//
// PRIMARY SOURCE: cursive-bootstrap/src/02_syntax/parser_stmt.cpp
//   Lines 834-868: Terminator consumption functions
//
// FUNCTIONS TO MIGRATE:
//
// 1. ConsumeTerminatorOpt (lines 834-856):
//    SPEC: Related to statement termination (Section 3.2.3)
//    - Handles optional/required terminator consumption based on policy
//    - Emits missing terminator diagnostic if required and missing
//    - Syncs to next statement on error
//
//    SPEC_RULE markers:
//    - "ConsumeTerminatorOpt-Req-Yes" (line 840): Required policy, terminator present
//    - "ConsumeTerminatorOpt-Req-No" (line 844): Required policy, terminator missing
//    - "ConsumeTerminatorOpt-Opt-Yes" (line 851): Optional policy, terminator present
//    - "ConsumeTerminatorOpt-Opt-No" (line 854): Optional policy, terminator missing
//
//    Source:
//      void ConsumeTerminatorOpt(Parser& parser, TerminatorPolicy policy) {
//        const Token* tok = Tok(parser);
//        if (tok && IsTerminatorToken(*tok)) {
//          if (policy == TerminatorPolicy::Required) {
//            SPEC_RULE("ConsumeTerminatorOpt-Req-Yes");
//          } else {
//            SPEC_RULE("ConsumeTerminatorOpt-Opt-Yes");
//          }
//          Advance(parser);
//          return;
//        }
//        if (policy == TerminatorPolicy::Required) {
//          SPEC_RULE("ConsumeTerminatorOpt-Req-No");
//          EmitMissingTerminator(parser, TokSpan(parser));
//          SyncStmt(parser);
//          return;
//        }
//        SPEC_RULE("ConsumeTerminatorOpt-Opt-No");
//      }
//
// 2. ConsumeTerminatorReq (lines 858-868):
//    SPEC: Required terminator consumption
//    - Always requires terminator; emits error and syncs if missing
//
//    SPEC_RULE markers:
//    - "ConsumeTerminatorReq-Yes" (line 861): Terminator present
//    - "ConsumeTerminatorReq-No" (line 865): Terminator missing
//
//    Source:
//      void ConsumeTerminatorReq(Parser& parser) {
//        const Token* tok = Tok(parser);
//        if (tok && IsTerminatorToken(*tok)) {
//          SPEC_RULE("ConsumeTerminatorReq-Yes");
//          Advance(parser);
//          return;
//        }
//        SPEC_RULE("ConsumeTerminatorReq-No");
//        EmitMissingTerminator(parser, TokSpan(parser));
//        SyncStmt(parser);
//      }
//
// =============================================================================
// DEPENDENCIES
// =============================================================================
//
// REQUIRED HEADERS:
//   - cursive/include/02_source/parser/parser.hpp
//   - cursive/include/02_source/lexer/token.hpp
//   - cursive/include/00_core/span.hpp
//   - cursive/include/00_core/assert_spec.hpp
//
// REQUIRED FUNCTIONS (from other files):
//   - Tok (parser_state.cpp): Get current token
//   - Advance (parser_state.cpp): Move to next token
//   - TokSpan (parser_state.cpp): Get current token span
//   - IsTerminatorToken (parser_recovery.cpp): Check if token is terminator
//   - EmitMissingTerminator (parser_recovery.cpp): Emit diagnostic
//   - SyncStmt (parser_recovery.cpp): Synchronize to next statement
//
// TYPES REQUIRED:
//   - Parser (struct)
//   - Token, TokenKind
//   - TerminatorPolicy (enum: Required, Optional)
//   - core::Span
//
// =============================================================================
// REFACTORING NOTES
// =============================================================================
//
// 1. TERMINATOR POLICY ENUM:
//    The TerminatorPolicy enum should be defined in parser.hpp:
//      enum class TerminatorPolicy { Required, Optional };
//
// 2. TERMINATOR TOKENS:
//    A terminator token is either:
//    - TokenKind::Newline
//    - TokenKind::Punctuator with lexeme ";"
//    IsTerminatorToken is defined in parser_recovery.cpp.
//
// 3. ERROR RECOVERY:
//    When a required terminator is missing:
//    - Emit E-SRC-0522 (missing terminator) diagnostic
//    - Call SyncStmt to skip to next statement boundary
//    This prevents cascading errors from a single missing terminator.
//
// 4. USAGE PATTERNS:
//    - ConsumeTerminatorReq: After statements that always need terminators
//    - ConsumeTerminatorOpt with Required: Same as ConsumeTerminatorReq
//    - ConsumeTerminatorOpt with Optional: After expressions in contexts
//      where terminator is syntactically optional (e.g., last stmt in block)
//
// 5. SPEC RULE COVERAGE:
//    This file has 6 SPEC_RULE markers covering all branches:
//    - ConsumeTerminatorOpt: 4 rules (2 policies × 2 outcomes)
//    - ConsumeTerminatorReq: 2 rules (2 outcomes)
//
// 6. NAMESPACE:
//    Source uses cursive0::syntax. Update to target namespace.
//
// =============================================================================

// TODO: Migrate implementation from source files listed above

