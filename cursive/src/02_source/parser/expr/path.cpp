// =============================================================================
// MIGRATION MAPPING: path.cpp
// =============================================================================
// This file should contain parsing logic for qualified names and qualified
// apply expressions (module::name, module::name(...), module::name{...}).
//
// SPEC REFERENCE: C0updated.md
// -----------------------------------------------------------------------------
// **Qualified Head.** (Lines 4075-4081)
//
// **(Parse-QualifiedHead)** Line 4078
//   Γ ⊢ ParseIdent(P) ⇓ (P_1, id_0)    IsOp(Tok(P_1), "::")
//   Γ ⊢ ParseModulePathTail(P_1, [id_0]) ⇓ (P_2, xs)    xs = ys ++ [name]    |xs| ≥ 2
//   ────────────────────────────────────────────────────────────────────────────
//   Γ ⊢ ParseQualifiedHead(P) ⇓ (P_2, ys, name)
//
// SEMANTICS:
// - Parses a qualified head: path::name where path has at least one segment
// - Returns module path (all segments except last) and name (last segment)
// - Requires at least 2 segments (path::name minimum)
//
// **Primary Expression Rules (Qualified Forms)** (Lines 5185-5202)
//
// **(Parse-Identifier-Expr)** Lines 5185-5187
//   IsIdent(Tok(P))    ¬ IsOp(Tok(Advance(P)), "::")
//   ¬ IsOp(Tok(Advance(P)), "@")    ¬ IsPunc(Tok(Advance(P)), "{")
//   ────────────────────────────────────────────────────────────────────────────
//   Γ ⊢ ParsePrimary(P) ⇓ (Advance(P), Identifier(Lexeme(Tok(P))))
//
// **(Parse-Qualified-Name)** Lines 5189-5192
//   Γ ⊢ ParseQualifiedHead(P) ⇓ (P_1, path, name)
//   Tok(P_1) ∉ {Punctuator("("), Punctuator("{")}    ¬ IsOp(Tok(P_1), "@")
//   ────────────────────────────────────────────────────────────────────────────
//   Γ ⊢ ParsePrimary(P) ⇓ (P_1, QualifiedName(path, name))
//
// **(Parse-Qualified-Apply-Paren)** Lines 5194-5197
//   Γ ⊢ ParseQualifiedHead(P) ⇓ (P_1, path, name)    IsPunc(Tok(P_1), "(")
//   Γ ⊢ ParseArgList(Advance(P_1)) ⇓ (P_2, args)    IsPunc(Tok(P_2), ")")
//   ────────────────────────────────────────────────────────────────────────────
//   Γ ⊢ ParsePrimary(P) ⇓ (Advance(P_2), QualifiedApply(path, name, Paren(args)))
//
// **(Parse-Qualified-Apply-Brace)** Lines 5199-5202
//   Γ ⊢ ParseQualifiedHead(P) ⇓ (P_1, path, name)    IsPunc(Tok(P_1), "{")
//   Γ ⊢ ParseFieldInitList(Advance(P_1)) ⇓ (P_2, fields)    IsPunc(Tok(P_2), "}")
//   ────────────────────────────────────────────────────────────────────────────
//   Γ ⊢ ParsePrimary(P) ⇓ (Advance(P_2), QualifiedApply(path, name, Brace(fields)))
//
// SEMANTICS:
// - QualifiedName: path::name (no call arguments)
// - QualifiedApply(Paren): path::name(args) (function call syntax)
// - QualifiedApply(Brace): path::name{fields} (record/enum literal syntax)
// - allow_brace parameter controls whether brace forms are parsed
//
// SOURCE FILE: cursive-bootstrap/src/02_syntax/parser_paths.cpp
//              cursive-bootstrap/src/02_syntax/parser_expr.cpp
//
// CONTENT TO MIGRATE:
// -----------------------------------------------------------------------------
// 1. ParseQualifiedHeadResult struct
//    Source: parser_paths.cpp (struct defined in header parser_internal.h)
//    Purpose: Result type containing parser, module_path, and name
//
// 2. ParseQualifiedHead function
//    Source: parser_paths.cpp, lines 102-120
//    Purpose: Parse "ident::ident::...::name" into module path and final name
//    SPEC_RULE: "Parse-QualifiedHead"
//
// 3. Qualified name/apply parsing in ParsePrimary
//    Source: parser_expr.cpp, lines 1142-1202
//    Purpose: Dispatch between QualifiedName and QualifiedApply variants
//    SPEC_RULE: "Parse-Qualified-Name", "Parse-Qualified-Apply-Paren",
//               "Parse-Qualified-Apply-Brace"
//
// DEPENDENCIES:
// - Requires: ParseIdent (from parser_paths.cpp)
// - Requires: ParseModulePathTail (from parser_paths.cpp)
// - Requires: ParseArgList (from parser_expr.cpp or args.cpp)
// - Requires: ParseFieldInitList (from record_literal.cpp)
// - Requires: QualifiedNameExpr, QualifiedApplyExpr AST node types
// - Requires: ParenArgs, BraceArgs variant types
// - Requires: ModulePath, Identifier types
// - Requires: IsOp, IsPunc, Tok, Advance helpers
// - Requires: MakeExpr, SpanBetween helpers
// - Requires: allow_brace parameter propagation
//
// REFACTORING NOTES:
// - ParseQualifiedHead is in parser_paths.cpp but used by expr and pattern parsers
// - Consider keeping path parsing centralized or splitting to path.cpp
// - The bootstrap checks for unsupported generic qualifiers (path<T>::name)
// - Error recovery via SyncStmt on parse failures
// - QualifiedApply with brace only parsed when allow_brace=true
// =============================================================================
