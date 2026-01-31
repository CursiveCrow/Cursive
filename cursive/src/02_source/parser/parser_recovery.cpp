// =============================================================================
// MIGRATION MAPPING: parser_recovery.cpp
// =============================================================================
//
// DESTINATION: cursive/src/02_source/parser/parser_recovery.cpp
// PURPOSE: Error recovery and synchronization during parsing
//
// =============================================================================
// SPEC REFERENCES
// =============================================================================
//
// C0updated.md Section 3.3.12 (Lines 6452-6508): Error Recovery and Synchronization
//
// **Statement Synchronization Set (lines 6454-6456)**:
//   SyncStmt = {Punctuator(";"), Newline, Punctuator("}"), EOF}
//
// **Item Synchronization Set (lines 6458-6460)**:
//   SyncItem = {Keyword(procedure), Keyword(record), Keyword(enum), Keyword(modal),
//               Keyword(class), Keyword(type), Keyword(using), Keyword(let),
//               Keyword(var), Punctuator("}"), EOF}
//
// **Type Synchronization Set (lines 6462-6464)**:
//   SyncType = {Punctuator(","), Punctuator(";"), Newline, Punctuator(")"),
//               Punctuator("]"), Punctuator("}"), EOF}
//
// **Sync-Stmt-Stop (lines 6466-6469)**:
//   Tok(P) in {Punctuator("}"), EOF}
//   ----------------------------------------
//   SyncStmt(P) => P
//
// **Sync-Stmt-Consume (lines 6471-6474)**:
//   Tok(P) in {Punctuator(";"), Newline}
//   ----------------------------------------
//   SyncStmt(P) => Advance(P)
//
// **Sync-Stmt-Advance (lines 6476-6479)**:
//   Tok(P) not in SyncStmt
//   ----------------------------------------
//   SyncStmt(P) => SyncStmt(Advance(P))
//
// **Sync-Item-Stop (lines 6481-6484)**:
//   Tok(P) in SyncItem
//   ----------------------------------------
//   SyncItem(P) => P
//
// **Sync-Item-Advance (lines 6486-6489)**:
//   Tok(P) not in SyncItem
//   ----------------------------------------
//   SyncItem(P) => SyncItem(Advance(P))
//
// **Sync-Type-Stop (lines 6491-6494)**:
//   Tok(P) in {Punctuator(")"), Punctuator("]"), Punctuator("}"), EOF}
//   ----------------------------------------
//   SyncType(P) => P
//
// **Sync-Type-Consume (lines 6496-6499)**:
//   Tok(P) in {Punctuator(","), Punctuator(";"), Newline}
//   ----------------------------------------
//   SyncType(P) => Advance(P)
//
// **Sync-Type-Advance (lines 6501-6504)**:
//   Tok(P) not in SyncType
//   ----------------------------------------
//   SyncType(P) => SyncType(Advance(P))
//
// C0updated.md Section 3.3.13 (Lines 6509-6518): Diagnostics (Phase 1)
//
// **Parse-Syntax-Err (lines 6513-6518)**:
//   GenericParseRules = {Parse-Ident-Err, Parse-Type-Err, Parse-Pattern-Err,
//                        Parse-Primary-Err, Parse-Statement-Err, Parse-Item-Err}
//   r in GenericParseRules    PremisesHold(r, P)
//   ----------------------------------------
//   Emit(Code(Parse-Syntax-Err))
//
// =============================================================================
// SOURCE FILES
// =============================================================================
//
// PRIMARY SOURCE: cursive-bootstrap/src/02_syntax/parser_recovery.cpp
//   Lines 1-116: Complete file
//
// FUNCTIONS TO MIGRATE:
//
// 1. Anonymous namespace helpers (lines 11-27):
//
//    a. IsSyncItemToken (lines 13-21):
//       Checks if token is in item synchronization set
//
//       Source:
//         bool IsSyncItemToken(const Token& tok) {
//           if (tok.kind == TokenKind::Keyword) {
//             const std::string_view lex = tok.lexeme;
//             return lex == "procedure" || lex == "record" || lex == "enum" ||
//                    lex == "modal" || lex == "class" || lex == "type" ||
//                    lex == "using" || lex == "let" || lex == "var";
//           }
//           return tok.kind == TokenKind::Punctuator && tok.lexeme == "}";
//         }
//
//    b. IsTerminatorToken (lines 23-26):
//       Checks if token is a statement terminator
//
//       Source:
//         bool IsTerminatorToken(const Token& tok) {
//           return tok.kind == TokenKind::Newline ||
//                  (tok.kind == TokenKind::Punctuator && tok.lexeme == ";");
//         }
//
// 2. EmitParseSyntaxErr (lines 30-37):
//    SPEC: Parse-Syntax-Err (line 31)
//    Emits E-SRC-0520 diagnostic for syntax errors
//
//    Source:
//      void EmitParseSyntaxErr(Parser& parser, const core::Span& span) {
//        SPEC_RULE("Parse-Syntax-Err");
//        auto diag = core::MakeDiagnostic("E-SRC-0520", span);
//        if (!diag) { return; }
//        parser.diags = core::Emit(parser.diags, *diag);
//      }
//
// 3. SyncStmt (lines 39-62):
//    SPEC: Sync-Stmt-Stop, Sync-Stmt-Consume, Sync-Stmt-Advance
//    Synchronizes parser to next statement boundary
//
//    Source:
//      void SyncStmt(Parser& parser) {
//        for (;;) {
//          if (AtEof(parser)) {
//            SPEC_RULE("Sync-Stmt-Stop");
//            return;
//          }
//          const Token* tok = Tok(parser);
//          if (!tok) {
//            SPEC_RULE("Sync-Stmt-Stop");
//            return;
//          }
//          if (tok->kind == TokenKind::Punctuator && tok->lexeme == "}") {
//            SPEC_RULE("Sync-Stmt-Stop");
//            return;
//          }
//          if (IsTerminatorToken(*tok)) {
//            SPEC_RULE("Sync-Stmt-Consume");
//            Advance(parser);
//            return;
//          }
//          SPEC_RULE("Sync-Stmt-Advance");
//          Advance(parser);
//        }
//      }
//
// 4. SyncItem (lines 64-82):
//    SPEC: Sync-Item-Stop, Sync-Item-Advance
//    Synchronizes parser to next item boundary
//
//    Source:
//      void SyncItem(Parser& parser) {
//        for (;;) {
//          if (AtEof(parser)) {
//            SPEC_RULE("Sync-Item-Stop");
//            return;
//          }
//          const Token* tok = Tok(parser);
//          if (!tok) {
//            SPEC_RULE("Sync-Item-Stop");
//            return;
//          }
//          if (IsSyncItemToken(*tok)) {
//            SPEC_RULE("Sync-Item-Stop");
//            return;
//          }
//          SPEC_RULE("Sync-Item-Advance");
//          Advance(parser);
//        }
//      }
//
// 5. SyncType (lines 84-114):
//    SPEC: Sync-Type-Stop, Sync-Type-Consume, Sync-Type-Advance
//    Synchronizes parser to next type boundary
//
//    Source:
//      void SyncType(Parser& parser) {
//        for (;;) {
//          if (AtEof(parser)) {
//            SPEC_RULE("Sync-Type-Stop");
//            return;
//          }
//          const Token* tok = Tok(parser);
//          if (!tok) {
//            SPEC_RULE("Sync-Type-Stop");
//            return;
//          }
//          if (tok->kind == TokenKind::Punctuator) {
//            if (tok->lexeme == ")" || tok->lexeme == "]" || tok->lexeme == "}") {
//              SPEC_RULE("Sync-Type-Stop");
//              return;
//            }
//            if (tok->lexeme == "," || tok->lexeme == ";") {
//              SPEC_RULE("Sync-Type-Consume");
//              Advance(parser);
//              return;
//            }
//          }
//          if (tok->kind == TokenKind::Newline) {
//            SPEC_RULE("Sync-Type-Consume");
//            Advance(parser);
//            return;
//          }
//          SPEC_RULE("Sync-Type-Advance");
//          Advance(parser);
//        }
//      }
//
// =============================================================================
// DEPENDENCIES
// =============================================================================
//
// REQUIRED HEADERS:
//   - cursive/include/02_source/parser/parser.hpp
//   - cursive/include/02_source/lexer/token.hpp
//   - cursive/include/00_core/diagnostics.hpp
//   - cursive/include/00_core/diagnostic_messages.hpp
//   - cursive/include/00_core/assert_spec.hpp
//   - <string_view>
//
// REQUIRED FUNCTIONS:
//   - Tok (parser_state.cpp)
//   - Advance (parser_state.cpp)
//   - AtEof (parser_state.cpp)
//   - core::MakeDiagnostic (00_core/diagnostic_messages.cpp)
//   - core::Emit (00_core/diagnostics.cpp)
//
// TYPES REQUIRED:
//   - Parser
//   - Token, TokenKind
//   - core::DiagnosticStream, core::Span
//
// =============================================================================
// REFACTORING NOTES
// =============================================================================
//
// 1. SPEC RULE MARKERS:
//    All SPEC_RULE calls must be preserved:
//    - "Parse-Syntax-Err" (line 31)
//    - "Sync-Stmt-Stop" (lines 42, 47, 51)
//    - "Sync-Stmt-Consume" (line 55)
//    - "Sync-Stmt-Advance" (line 59)
//    - "Sync-Item-Stop" (lines 67, 72, 76)
//    - "Sync-Item-Advance" (line 79)
//    - "Sync-Type-Stop" (lines 87, 92, 97)
//    - "Sync-Type-Consume" (lines 101, 107)
//    - "Sync-Type-Advance" (line 111)
//
// 2. DIAGNOSTIC CODE:
//    EmitParseSyntaxErr uses E-SRC-0520. Ensure this code is defined in
//    00_core/diagnostic_messages with appropriate message text.
//
// 3. TERMINATOR TOKEN:
//    IsTerminatorToken is also used in parser_state.cpp (ConsumeTerminatorOpt/Req).
//    Consider moving to a shared location or ensuring consistency.
//
// 4. SYNC PATTERNS:
//    The sync functions follow a common pattern:
//    - Loop until EOF or sync token found
//    - Stop: Return without consuming (at boundary)
//    - Consume: Consume terminator and return (past boundary)
//    - Advance: Skip non-sync token and continue
//
// 5. SYNC SETS:
//    The synchronization sets are defined in the spec but implemented inline.
//    Consider extracting to named constants for clarity:
//
//      constexpr std::array kSyncStmtStop = {";", "}"};
//      constexpr std::array kSyncItemKeywords = {"procedure", "record", ...};
//
// 6. ERROR RECOVERY PHILOSOPHY:
//    The sync functions implement panic-mode recovery:
//    - On error, skip tokens until a sync point
//    - Resume parsing from the sync point
//    - This may miss errors but prevents cascading failures
//
// 7. NEWLINE HANDLING:
//    Newlines are terminators in Cursive (like statements in Go/Python).
//    SyncStmt consumes newlines; SyncType also consumes newlines.
//    SyncItem does NOT consume newlines (items are delimited by keywords).
//
// 8. NAMESPACE:
//    Source uses cursive0::syntax. Update to target namespace.
//
// =============================================================================
