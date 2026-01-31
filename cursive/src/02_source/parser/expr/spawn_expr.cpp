// =============================================================================
// MIGRATION MAPPING: spawn_expr.cpp
// =============================================================================
// This file should contain parsing logic for spawn expressions, which create
// asynchronous tasks within a parallel block.
//
// SPEC REFERENCE: C0updated.md, Section 3.3.8.6, Lines 5269-5272
// HELPER RULES: Section 3.3.8.7, Lines 5641-5691
// =============================================================================
//
// FORMAL RULE - Parse-Spawn-Expr (Lines 5269-5272):
// -----------------------------------------------------------------------------
// IsKw(Tok(P), `spawn`)
// Gamma |- ParseSpawnOptsOpt(Advance(P)) => (P_1, opts)
// Gamma |- ParseBlock(P_1) => (P_2, body)
// ────────────────────────────────────────────────────────────────────────────
// Gamma |- ParsePrimary(P) => (P_2, SpawnExpr(opts, body))
//
// HELPER RULES - Spawn Options (Lines 5643-5691):
// -----------------------------------------------------------------------------
//
// **(Parse-SpawnOptsOpt-None)** Lines 5643-5646
// NOT IsPunc(Tok(P), "[")
// ────────────────────────────────────────────────────────────────────────────
// Gamma |- ParseSpawnOptsOpt(P) => (P, [])
//
// **(Parse-SpawnOptsOpt-Yes)** Lines 5648-5651
// IsPunc(Tok(P), "[")
// Gamma |- ParseSpawnOptList(Advance(P)) => (P_1, opts)
// IsPunc(Tok(P_1), "]")
// ────────────────────────────────────────────────────────────────────────────
// Gamma |- ParseSpawnOptsOpt(P) => (Advance(P_1), opts)
//
// **(Parse-SpawnOptList-Empty)** Lines 5653-5656
// IsPunc(Tok(P), "]")    c = Code(Parse-Syntax-Err)    Emit(c, Tok(P).span)
// ────────────────────────────────────────────────────────────────────────────
// Gamma |- ParseSpawnOptList(P) => (P, [])
//
// **(Parse-SpawnOptList-Cons)** Lines 5658-5661
// Gamma |- ParseSpawnOpt(P) => (P_1, opt)
// Gamma |- ParseSpawnOptListTail(P_1, [opt]) => (P_2, opts)
// ────────────────────────────────────────────────────────────────────────────
// Gamma |- ParseSpawnOptList(P) => (P_2, opts)
//
// **(Parse-SpawnOptListTail-End)** Lines 5663-5666
// IsPunc(Tok(P), "]")
// ────────────────────────────────────────────────────────────────────────────
// Gamma |- ParseSpawnOptListTail(P, xs) => (P, xs)
//
// **(Parse-SpawnOptListTail-TrailingComma)** Lines 5668-5671
// IsPunc(Tok(P), ",")    IsPunc(Tok(Advance(P)), "]")
// TrailingCommaAllowed(P_0, P, {Punctuator("]")})
// ────────────────────────────────────────────────────────────────────────────
// Gamma |- ParseSpawnOptListTail(P, xs) => (Advance(P), xs)
//
// **(Parse-SpawnOptListTail-Comma)** Lines 5673-5676
// IsPunc(Tok(P), ",")
// Gamma |- ParseSpawnOpt(Advance(P)) => (P_1, opt)
// Gamma |- ParseSpawnOptListTail(P_1, xs ++ [opt]) => (P_2, ys)
// ────────────────────────────────────────────────────────────────────────────
// Gamma |- ParseSpawnOptListTail(P, xs) => (P_2, ys)
//
// **(Parse-SpawnOpt-Name)** Lines 5678-5681
// IsIdent(Tok(P))    Lexeme(Tok(P)) = `name`
// IsPunc(Tok(Advance(P)), ":")
// Tok(Advance(Advance(P))).kind = StringLiteral
// ────────────────────────────────────────────────────────────────────────────
// Gamma |- ParseSpawnOpt(P) => (Advance(Advance(Advance(P))), Name(Tok(...)))
//
// **(Parse-SpawnOpt-Affinity)** Lines 5683-5686
// IsIdent(Tok(P))    Lexeme(Tok(P)) = `affinity`
// IsPunc(Tok(Advance(P)), ":")
// Gamma |- ParseExpr(Advance(Advance(P))) => (P_1, e)
// ────────────────────────────────────────────────────────────────────────────
// Gamma |- ParseSpawnOpt(P) => (P_1, Affinity(e))
//
// **(Parse-SpawnOpt-Priority)** Lines 5688-5691
// IsIdent(Tok(P))    Lexeme(Tok(P)) = `priority`
// IsPunc(Tok(Advance(P)), ":")
// Gamma |- ParseExpr(Advance(Advance(P))) => (P_1, e)
// ────────────────────────────────────────────────────────────────────────────
// Gamma |- ParseSpawnOpt(P) => (P_1, Priority(e))
//
// SEMANTICS:
// - `spawn [options]? { body }`
// - Creates an asynchronous task within enclosing parallel block
// - Returns Spawned<T> handle that can be awaited with `wait`
// - Options: `name: "string"`, `affinity: expr`, `priority: expr`
// - Body captures const/shared by reference, unique requires explicit `move`
//
// =============================================================================
// SOURCE FILE: cursive-bootstrap/src/02_syntax/parser_expr.cpp
// =============================================================================
//
// CONTENT TO MIGRATE:
// -----------------------------------------------------------------------------
//
// 1. ParseSpawnExpr (Lines 1551-1621)
//    ─────────────────────────────────────────────────────────────────────────
//    Lines 1552-1555: Keyword check and initialization
//      - Check for `spawn` keyword
//      - SPEC_RULE("Parse-Spawn-Expr")
//      - Advance past keyword
//
//    Lines 1556-1614: Parse optional [options]
//      - Check for "[" to start options
//      - Loop parsing options until "]"
//
//      Lines 1568-1580: Special handling for `move` option
//        * Check for `move` keyword (not in spec but in source)
//        * SpawnOptionKind::MoveCapture
//        * Parse expression for what to move
//        * Continue to next option
//
//      Lines 1582-1595: Option kind dispatch (identifier options)
//        * "name" -> SpawnOptionKind::Name
//        * "affinity" -> SpawnOptionKind::Affinity
//        * "priority" -> SpawnOptionKind::Priority
//        * Other -> EmitParseSyntaxErr
//
//      Lines 1596-1609: Parse option value
//        * Expect ":" after option name
//        * Parse expression value
//        * Handle comma separator
//
//      Lines 1611-1613: Consume closing "]"
//
//    Lines 1615-1620: Parse body and construct result
//      - ParseBlock(after_opts) for body
//      - Construct SpawnExpr{opts, body}
//      - Return with span covering entire expression
//
// =============================================================================
// DEPENDENCIES:
// =============================================================================
// - ParseBlock: Block statement parser
// - ParseExpr: General expression parser
// - SpawnExpr AST node (from ast.hpp)
// - SpawnOption struct with kind enum and value
// - SpawnOptionKind enum: Name, Affinity, Priority, MoveCapture
// - MakeExpr, SpanBetween, SpanCover helpers
// - Token predicates: IsKwTok, IsPunc, IsIdentTok
// - EmitParseSyntaxErr for error recovery
//
// =============================================================================
// REFACTORING NOTES:
// =============================================================================
// 1. Source includes MoveCapture option not in spec - verify if this is a spec
//    addition needed or implementation-specific extension.
//
// 2. Consider extracting ParseSpawnOptsOpt as a separate function matching
//    the spec structure rather than inline parsing.
//
// 3. The option parsing loop structure could be factored into a reusable
//    option list parser template (similar to parallel options).
//
// 4. Trailing comma handling should follow TrailingCommaAllowed predicate.
//
// 5. The spec shows Name option takes a StringLiteral directly, but the source
//    appears to parse it as an expression - verify which is correct.
//
// 6. Error recovery could be improved for malformed options.
//
// =============================================================================
