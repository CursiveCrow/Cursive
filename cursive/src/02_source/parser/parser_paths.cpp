// =============================================================================
// MIGRATION MAPPING: parser_paths.cpp
// =============================================================================
//
// DESTINATION: cursive/src/02_source/parser/parser_paths.cpp
// PURPOSE: Path parsing (identifiers, module paths, type paths, visibility)
//
// =============================================================================
// SPEC REFERENCES
// =============================================================================
//
// C0updated.md Section 3.3.6.1 (Lines 3468-3493): Identifiers and Paths
//
// **Parse-Ident (lines 3470-3473)**:
//   IsIdent(Tok(P))
//   ----------------------------------------
//   ParseIdent(P) => (Advance(P), Lexeme(Tok(P)))
//
// **Parse-Ident-Err (lines 3475-3478)**:
//   !IsIdent(Tok(P))    c = Code(Parse-Syntax-Err)    Emit(c, Tok(P).span)
//   ----------------------------------------
//   ParseIdent(P) => (P, "_")
//
// **Parse-ModulePath (lines 3480-3483)**:
//   ParseIdent(P) => (P_1, id)    ParseModulePathTail(P_1, [id]) => (P_2, path)
//   ----------------------------------------
//   ParseModulePath(P) => (P_2, path)
//
// **Parse-ModulePathTail-End (lines 3485-3488)**:
//   !IsOp(Tok(P), "::")
//   ----------------------------------------
//   ParseModulePathTail(P, xs) => (P, xs)
//
// **Parse-ModulePathTail-Cons (lines 3490-3493)**:
//   IsOp(Tok(P), "::")    ParseIdent(Advance(P)) => (P_1, id)    ParseModulePathTail(P_1, xs ++ [id]) => (P_2, ys)
//   ----------------------------------------
//   ParseModulePathTail(P, xs) => (P_2, ys)
//
// C0updated.md Section 3.3.6.2 (Lines 3495-3515): Visibility Parsing
//
// **Parse-Vis-Opt (lines 3497-3500)**:
//   IsKw(Tok(P), v)    v in {public, internal, private, protected}
//   ----------------------------------------
//   ParseVis(P) => (Advance(P), v)
//
// **Parse-Vis-Default (lines 3502-3505)**:
//   !IsKw(Tok(P), v)
//   ----------------------------------------
//   ParseVis(P) => (P, internal)
//
// C0updated.md Section 3.3.2.1 (Lines 2646-2653): Path Types
//   Path = [identifier]
//   ModulePath = Path
//   TypePath = Path
//   ClassPath = Path
//
// =============================================================================
// SOURCE FILES
// =============================================================================
//
// PRIMARY SOURCE: cursive-bootstrap/src/02_syntax/parser_paths.cpp
//   Lines 1-149: Complete file
//
// FUNCTIONS TO MIGRATE:
//
// 1. Anonymous namespace helpers (lines 11-63):
//
//    a. IsVisKeyword (lines 13-16):
//       Checks if lexeme is a visibility keyword
//
//       Source:
//         bool IsVisKeyword(std::string_view lexeme) {
//           return lexeme == "public" || lexeme == "internal" ||
//                  lexeme == "private" || lexeme == "protected";
//         }
//
//    b. VisibilityFromLexeme (lines 18-32):
//       Converts lexeme to Visibility enum
//
//       Source:
//         std::optional<Visibility> VisibilityFromLexeme(std::string_view lexeme) {
//           if (lexeme == "public") { return Visibility::Public; }
//           if (lexeme == "internal") { return Visibility::Internal; }
//           if (lexeme == "private") { return Visibility::Private; }
//           if (lexeme == "protected") { return Visibility::Protected; }
//           return std::nullopt;
//         }
//
//    c. ParseModulePathTail (lines 34-47):
//       SPEC: Parse-ModulePathTail-End (line 37), Parse-ModulePathTail-Cons (line 41)
//       Recursive tail parsing for module paths
//
//       Source:
//         ParseElemResult<ModulePath> ParseModulePathTail(Parser parser, ModulePath xs) {
//           const Token* tok = Tok(parser);
//           if (!tok || !IsOpTok(*tok, "::")) {
//             SPEC_RULE("Parse-ModulePathTail-End");
//             return {parser, xs};
//           }
//           SPEC_RULE("Parse-ModulePathTail-Cons");
//           Parser next = parser;
//           Advance(next);
//           ParseElemResult<Identifier> id = ParseIdent(next);
//           xs.push_back(id.elem);
//           return ParseModulePathTail(id.parser, std::move(xs));
//         }
//
//    d. ParseTypePathTail (lines 49-62):
//       SPEC: Parse-TypePathTail-End (line 52), Parse-TypePathTail-Cons (line 56)
//       Recursive tail parsing for type paths
//
//       Source:
//         ParseElemResult<TypePath> ParseTypePathTail(Parser parser, TypePath xs) {
//           const Token* tok = Tok(parser);
//           if (!tok || !IsOpTok(*tok, "::")) {
//             SPEC_RULE("Parse-TypePathTail-End");
//             return {parser, xs};
//           }
//           SPEC_RULE("Parse-TypePathTail-Cons");
//           Parser next = parser;
//           Advance(next);
//           ParseElemResult<Identifier> id = ParseIdent(next);
//           xs.push_back(id.elem);
//           return ParseTypePathTail(id.parser, std::move(xs));
//         }
//
// 2. ParseIdent (lines 66-78):
//    SPEC: Parse-Ident (line 69), Parse-Ident-Err (line 75)
//
//    Source:
//      ParseElemResult<Identifier> ParseIdent(Parser parser) {
//        const Token* tok = Tok(parser);
//        if (tok && IsIdentTok(*tok)) {
//          SPEC_RULE("Parse-Ident");
//          Identifier name = tok->lexeme;
//          Advance(parser);
//          return {parser, name};
//        }
//        SPEC_RULE("Parse-Ident-Err");
//        EmitParseSyntaxErr(parser, TokSpan(parser));
//        return {parser, Identifier{"_"}};
//      }
//
// 3. ParseModulePath (lines 80-86):
//    SPEC: Parse-ModulePath (line 81)
//
//    Source:
//      ParseElemResult<ModulePath> ParseModulePath(Parser parser) {
//        SPEC_RULE("Parse-ModulePath");
//        ParseElemResult<Identifier> head = ParseIdent(parser);
//        ModulePath path;
//        path.push_back(head.elem);
//        return ParseModulePathTail(head.parser, std::move(path));
//      }
//
// 4. ParseTypePath (lines 88-94):
//    SPEC: Parse-TypePath (line 89)
//
//    Source:
//      ParseElemResult<TypePath> ParseTypePath(Parser parser) {
//        SPEC_RULE("Parse-TypePath");
//        ParseElemResult<Identifier> head = ParseIdent(parser);
//        TypePath path;
//        path.push_back(head.elem);
//        return ParseTypePathTail(head.parser, std::move(path));
//      }
//
// 5. ParseClassPath (lines 96-100):
//    SPEC: Parse-ClassPath (line 97)
//
//    Source:
//      ParseElemResult<ClassPath> ParseClassPath(Parser parser) {
//        SPEC_RULE("Parse-ClassPath");
//        ParseElemResult<TypePath> path = ParseTypePath(parser);
//        return {path.parser, path.elem};
//      }
//
// 6. ParseQualifiedHead (lines 102-120):
//    SPEC: Parse-QualifiedHead (line 103)
//    Parses qualified name prefix (module::name)
//
//    Source:
//      ParseQualifiedHeadResult ParseQualifiedHead(Parser parser) {
//        SPEC_RULE("Parse-QualifiedHead");
//        ParseElemResult<Identifier> head = ParseIdent(parser);
//        const Token* tok = Tok(head.parser);
//        if (!tok || !IsOpTok(*tok, "::")) {
//          EmitParseSyntaxErr(parser, TokSpan(parser));
//          return {parser, {}, "_"};
//        }
//        ParseElemResult<ModulePath> rest = ParseModulePathTail(head.parser, {head.elem});
//        ModulePath full = std::move(rest.elem);
//        if (full.size() < 2) {
//          EmitParseSyntaxErr(parser, TokSpan(parser));
//          return {rest.parser, full, "_"};
//        }
//        Identifier name = full.back();
//        full.pop_back();
//        return {rest.parser, full, name};
//      }
//
// 7. ParseVis (lines 122-133):
//    SPEC: Parse-Vis-Opt (line 125), Parse-Vis-Default (line 131)
//
//    Source:
//      ParseElemResult<Visibility> ParseVis(Parser parser) {
//        const Token* tok = Tok(parser);
//        if (tok && IsKwTok(*tok, tok->lexeme) && IsVisKeyword(tok->lexeme)) {
//          SPEC_RULE("Parse-Vis-Opt");
//          std::optional<Visibility> vis = VisibilityFromLexeme(tok->lexeme);
//          Advance(parser);
//          return {parser, vis.value_or(Visibility::Internal)};
//        }
//        SPEC_RULE("Parse-Vis-Default");
//        return {parser, Visibility::Internal};
//      }
//
// 8. ParseAliasOpt (lines 135-147):
//    SPEC: Parse-AliasOpt-Yes (line 138), Parse-AliasOpt-None (line 145)
//    Parses optional "as alias" suffix
//
//    Source:
//      ParseElemResult<std::optional<Identifier>> ParseAliasOpt(Parser parser) {
//        const Token* tok = Tok(parser);
//        if (tok && IsKwTok(*tok, "as")) {
//          SPEC_RULE("Parse-AliasOpt-Yes");
//          Parser next = parser;
//          Advance(next);
//          ParseElemResult<Identifier> id = ParseIdent(next);
//          return {id.parser, id.elem};
//        }
//        SPEC_RULE("Parse-AliasOpt-None");
//        return {parser, std::nullopt};
//      }
//
// =============================================================================
// DEPENDENCIES
// =============================================================================
//
// REQUIRED HEADERS:
//   - cursive/include/02_source/parser/parser.hpp
//   - cursive/include/02_source/lexer/token.hpp
//   - cursive/include/02_source/lexer/keyword_policy.hpp
//   - cursive/include/02_source/ast/ast.hpp (for Identifier, ModulePath, etc.)
//   - cursive/include/00_core/diagnostics.hpp
//   - <string_view>
//   - <optional>
//   - <vector>
//
// REQUIRED FUNCTIONS:
//   - Tok (parser_state.cpp)
//   - Advance (parser_state.cpp)
//   - TokSpan (parser_state.cpp)
//   - IsIdentTok (keyword_policy.cpp)
//   - IsKwTok (keyword_policy.cpp)
//   - IsOpTok (keyword_policy.cpp)
//   - EmitParseSyntaxErr (parser_recovery.cpp)
//
// TYPES REQUIRED:
//   - Parser
//   - Token
//   - Identifier (std::string or custom type)
//   - ModulePath, TypePath, ClassPath (std::vector<Identifier>)
//   - Visibility (enum)
//   - ParseElemResult<T>
//   - ParseQualifiedHeadResult
//
// =============================================================================
// REFACTORING NOTES
// =============================================================================
//
// 1. SPEC RULE MARKERS:
//    All SPEC_RULE calls must be preserved:
//    - "Parse-ModulePathTail-End" (line 37)
//    - "Parse-ModulePathTail-Cons" (line 41)
//    - "Parse-TypePathTail-End" (line 52)
//    - "Parse-TypePathTail-Cons" (line 56)
//    - "Parse-Ident" (line 69)
//    - "Parse-Ident-Err" (line 75)
//    - "Parse-ModulePath" (line 81)
//    - "Parse-TypePath" (line 89)
//    - "Parse-ClassPath" (line 97)
//    - "Parse-QualifiedHead" (line 103)
//    - "Parse-Vis-Opt" (line 125)
//    - "Parse-Vis-Default" (line 131)
//    - "Parse-AliasOpt-Yes" (line 138)
//    - "Parse-AliasOpt-None" (line 145)
//
// 2. PATH TYPES:
//    ModulePath, TypePath, and ClassPath are all aliases for std::vector<Identifier>.
//    Consider whether stronger typing would be beneficial.
//
// 3. ERROR RECOVERY:
//    ParseIdent returns "_" on error. This placeholder allows parsing to continue
//    while still producing diagnostics.
//
// 4. VISIBILITY DEFAULT:
//    The default visibility is Visibility::Internal when no visibility keyword
//    is present. This matches the spec.
//
// 5. QUALIFIED HEAD:
//    ParseQualifiedHead is used for expressions like `module::name(...)`. It
//    splits the path into module prefix and final name.
//
// 6. RECURSIVE TAIL PARSING:
//    ParseModulePathTail and ParseTypePathTail use recursion. Consider whether
//    iterative versions would be preferable for deep paths.
//
// 7. NAMESPACE:
//    Source uses cursive0::syntax. Update to target namespace.
//
// =============================================================================
