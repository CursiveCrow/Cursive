// =============================================================================
// MIGRATION MAPPING: Foreign Contract Clause Parsing
// =============================================================================
// Destination: cursive/src/02_source/parser/item/foreign_contract_clause.cpp
// Source:      cursive-bootstrap/src/02_syntax/parser_items.cpp (extern block section)
//
// PURPOSE:
// Parse foreign contract clauses for extern declarations:
// |= @foreign_assumes(pred, pred, ...)
// |= @foreign_ensures(pred)
// |= @foreign_ensures(@error: pred)
// |= @foreign_ensures(@null_result: pred)
// =============================================================================

// =============================================================================
// SPEC RULES IMPLEMENTED
// =============================================================================
// From C0updated.md Section 3.3.6.13:
//
// ForeignContractStart(P) ⇔ IsOp(Tok(P), "|=") ∧ IsOp(Tok(Advance(P)), "@")
//   ∧ IsIdent(Tok(Advance(Advance(P))))
//   ∧ Lexeme(Tok(Advance(Advance(P)))) ∈ {`foreign_assumes`, `foreign_ensures`}
//
// Foreign contracts are parsed within extern blocks for extern procedure
// declarations. They specify assumptions about foreign code behavior.

// =============================================================================
// SOURCE CODE MAPPING
// =============================================================================

/*
SOURCE: Not fully implemented in cursive-bootstrap
The source implementation parses optional contract clauses for extern procs
but does not have full foreign contract syntax support.

From C0updated.md spec:

Foreign contract syntax:
- |= @foreign_assumes(pred, pred, ...) - preconditions the foreign code expects
- |= @foreign_ensures(pred) - postconditions on success
- |= @foreign_ensures(@error: pred) - postconditions on error
- |= @foreign_ensures(@null_result: pred) - postconditions when result is null

IMPLEMENTATION NOTES:
- Foreign contracts start with |= @ followed by foreign_assumes or foreign_ensures
- @foreign_assumes takes comma-separated predicates
- @foreign_ensures can have condition prefixes: @error, @null_result
- Multiple foreign contract clauses can appear on a single declaration
*/

// =============================================================================
// TYPE DEFINITIONS USED
// =============================================================================

/*
ENUM: ForeignContractKind
enum class ForeignContractKind {
  Assumes,
  Ensures,
  EnsuresError,
  EnsuresNullResult
};

STRUCT: ForeignContractClause
struct ForeignContractClause {
  ForeignContractKind kind;
  std::vector<std::shared_ptr<Expr>> predicates;
  core::Span span;
};

RESULT TYPE:
ParseElemResult<std::vector<ForeignContractClause>>
*/

// =============================================================================
// GRAMMAR
// =============================================================================

/*
ForeignContractClause ::=
    '|=' '@' 'foreign_assumes' '(' PredicateList ')'
  | '|=' '@' 'foreign_ensures' '(' PredicateExpr ')'
  | '|=' '@' 'foreign_ensures' '(' '@error' ':' PredicateExpr ')'
  | '|=' '@' 'foreign_ensures' '(' '@null_result' ':' PredicateExpr ')'

PredicateList ::= PredicateExpr (',' PredicateExpr)*
PredicateExpr ::= Expr
*/

// =============================================================================
// VERIFICATION MODES
// =============================================================================

/*
| Mode                   | Behavior                                     |
| ---------------------- | -------------------------------------------- |
| [[static]] (default)   | Caller must prove predicates at compile time |
| [[dynamic]]            | Runtime checks before call                   |
| [[assume]]             | Assume true without checks (optimization)    |
*/

// =============================================================================
// EXAMPLES
// =============================================================================

/*
extern "C" {
    procedure read_file(path: *imm u8, buf: *mut u8, len: usize) -> i32
        |= @foreign_assumes(path != null, buf != null, len > 0)
        |= @foreign_ensures(@result >= 0)
        |= @foreign_ensures(@error: @result < 0)

    procedure get_data() -> *imm u8
        |= @foreign_ensures(@null_result: @result == null)
}
*/

// =============================================================================
// IMPLEMENTATION CHECKLIST
// =============================================================================
// [ ] IsForeignContractStart function
//     - Check |= @ foreign_assumes/ensures pattern
// [ ] ParseForeignContractClauseOpt function
//     - Consume |= @
//     - Parse foreign_assumes or foreign_ensures
//     - Handle condition prefixes (@error, @null_result)
//     - Parse predicate expressions
// [ ] ParseForeignContractList function
//     - Parse multiple foreign contract clauses

// =============================================================================
// PSEUDO-IMPLEMENTATION
// =============================================================================

/*
bool IsForeignContractStart(Parser parser) {
  if (!IsOp(parser, "|=")) return false;
  Parser probe = parser;
  Advance(probe);
  if (!IsOp(probe, "@")) return false;
  Advance(probe);
  const Token* tok = Tok(probe);
  if (!tok || !IsIdentTok(*tok)) return false;
  return tok->lexeme == "foreign_assumes" || tok->lexeme == "foreign_ensures";
}

ParseElemResult<std::optional<ForeignContractClause>>
ParseForeignContractClauseOpt(Parser parser) {
  if (!IsForeignContractStart(parser)) {
    return {parser, std::nullopt};
  }

  Parser start = parser;
  Advance(parser);  // |=
  Advance(parser);  // @

  const Token* tok = Tok(parser);
  ForeignContractKind kind;

  if (tok->lexeme == "foreign_assumes") {
    kind = ForeignContractKind::Assumes;
  } else {
    kind = ForeignContractKind::Ensures;
  }
  Advance(parser);

  // Expect (
  ExpectPunc(parser, "(");

  ForeignContractClause clause;
  clause.kind = kind;

  if (kind == ForeignContractKind::Ensures) {
    // Check for condition prefix
    if (IsOp(parser, "@")) {
      Advance(parser);
      const Token* prefix = Tok(parser);
      if (prefix->lexeme == "error") {
        clause.kind = ForeignContractKind::EnsuresError;
        Advance(parser);
        ExpectPunc(parser, ":");
      } else if (prefix->lexeme == "null_result") {
        clause.kind = ForeignContractKind::EnsuresNullResult;
        Advance(parser);
        ExpectPunc(parser, ":");
      }
    }
    auto pred = ParseExpr(parser);
    clause.predicates.push_back(pred.elem);
    parser = pred.parser;
  } else {
    // Parse comma-separated predicates for assumes
    auto first = ParseExpr(parser);
    clause.predicates.push_back(first.elem);
    parser = first.parser;

    while (IsPunc(parser, ",")) {
      Advance(parser);
      auto pred = ParseExpr(parser);
      clause.predicates.push_back(pred.elem);
      parser = pred.parser;
    }
  }

  // Expect )
  ExpectPunc(parser, ")");

  clause.span = SpanBetween(start, parser);
  return {parser, clause};
}
*/
