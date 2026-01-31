// =============================================================================
// MIGRATION MAPPING: Item Common Helpers
// =============================================================================
// Destination: cursive/src/02_source/parser/item/item_common.cpp
// Source:      cursive-bootstrap/src/02_syntax/parser_items.cpp
//
// PURPOSE:
// Common helper functions and utilities used across all item parsing files.
// Includes helper predicates, skipping functions, and error handling utilities.
// =============================================================================

// =============================================================================
// SPEC RULES IMPLEMENTED
// =============================================================================
// This file implements foundational helpers referenced by multiple item parsers.
// Not directly tied to a single spec rule, but supports all item parsing rules.
//
// Helper Functions from Source (lines 25-471):
// - SkipNewlines (line 25-29)
// - IsWhereTok (line 32-35)
// - IsKw (line 457-460)
// - IsOp (line 462-465)
// - IsPunc (line 467-470)
// - EmitUnsupportedConstruct (line 17-23)
// - MakeErrorType (line 215-220)
// - NormalizeBindingPattern (line 235-249)
// - EmitImportUnsupported (line 251-258)
// - EmitExternUnsupported (line 441-447)
// - EmitReturnAtModuleErr (line 449-455)

// =============================================================================
// SOURCE CODE MAPPING
// =============================================================================

/*
SOURCE: cursive-bootstrap/src/02_syntax/parser_items.cpp (lines 25-29)
FUNCTION: SkipNewlines

ORIGINAL:
void SkipNewlines(Parser& parser) {
  while (Tok(parser) && Tok(parser)->kind == TokenKind::Newline) {
    Advance(parser);
  }
}

IMPLEMENTATION NOTES:
- Consumes consecutive newline tokens
- Used before many item components to skip whitespace
- Modifies parser in-place
*/

/*
SOURCE: cursive-bootstrap/src/02_syntax/parser_items.cpp (lines 32-35)
FUNCTION: IsWhereTok

ORIGINAL:
bool IsWhereTok(const Parser& parser) {
  const Token* tok = Tok(parser);
  return tok && IsIdentTok(*tok) && tok->lexeme == "where";
}

IMPLEMENTATION NOTES:
- "where" is contextual keyword (parsed as identifier)
- Used to detect start of where clause vs type invariant
*/

/*
SOURCE: cursive-bootstrap/src/02_syntax/parser_items.cpp (lines 457-470)
FUNCTIONS: IsKw, IsOp, IsPunc

ORIGINAL:
bool IsKw(const Parser& parser, std::string_view kw) {
  const Token* tok = Tok(parser);
  return tok && IsKwTok(*tok, kw);
}

bool IsOp(const Parser& parser, std::string_view op) {
  const Token* tok = Tok(parser);
  return tok && IsOpTok(*tok, op);
}

bool IsPunc(const Parser& parser, std::string_view p) {
  const Token* tok = Tok(parser);
  return tok && IsPuncTok(*tok, p);
}

IMPLEMENTATION NOTES:
- Token kind predicates for parser lookahead
- IsPunc handles multi-char punctuators: "[[", "]]", "::", etc.
- These are fundamental to all parsing decisions
*/

/*
SOURCE: cursive-bootstrap/src/02_syntax/parser_items.cpp (lines 17-23)
FUNCTION: EmitUnsupportedConstruct

ORIGINAL:
void EmitUnsupportedConstruct(Parser& parser) {
  auto diag = core::MakeDiagnostic("E-UNS-0101", TokSpan(parser));
  if (!diag) {
    return;
  }
  parser.diags = core::Emit(parser.diags, *diag);
}

IMPLEMENTATION NOTES:
- Emits E-UNS-0101 diagnostic for unsupported constructs
- Used when encountering C0 features not yet implemented
*/

/*
SOURCE: cursive-bootstrap/src/02_syntax/parser_items.cpp (lines 215-220)
FUNCTION: MakeErrorType

ORIGINAL:
std::shared_ptr<Type> MakeErrorType(const core::Span& span) {
  auto ty = std::make_shared<Type>();
  ty->span = span;
  ty->node = TypePrim{Identifier{"!"}};
  return ty;
}

IMPLEMENTATION NOTES:
- Creates a "never" type (!) for error recovery
- Used when parsing fails but we need a valid Type node
*/

/*
SOURCE: cursive-bootstrap/src/02_syntax/parser_items.cpp (lines 235-249)
FUNCTION: NormalizeBindingPattern

ORIGINAL:
void NormalizeBindingPattern(std::shared_ptr<Pattern>& pat,
                             std::shared_ptr<Type>& type_opt) {
  if (!pat || type_opt) {
    return;
  }
  const auto* typed = std::get_if<TypedPattern>(&pat->node);
  if (!typed) {
    return;
  }
  auto normalized = std::make_shared<Pattern>();
  normalized->span = pat->span;
  normalized->node = IdentifierPattern{typed->name};
  type_opt = typed->type;
  pat = std::move(normalized);
}

IMPLEMENTATION NOTES:
- Normalizes binding patterns like "x: T" into separate pattern and type
- Handles the case where type annotation is part of pattern vs separate
- Important for consistent AST representation
*/

// =============================================================================
// TYPE DEFINITIONS USED
// =============================================================================

/*
TYPES FROM AST:
- Parser: Parser state with token stream and diagnostics
- Token: Lexical token with kind and lexeme
- TokenKind: Enum for token types (Keyword, Identifier, Operator, Punctuator, etc.)
- core::Span: Source location information
- core::Diagnostic: Diagnostic message with code and location

HELPER RESULT TYPES:
- ParseElemResult<T>: { Parser parser; T elem; }
- ParseItemResult: std::variant<Item, ErrorItem> with Parser
*/

// =============================================================================
// IMPLEMENTATION CHECKLIST
// =============================================================================
// [ ] SkipNewlines - consume newline tokens
// [ ] IsWhereTok - detect contextual "where" keyword
// [ ] IsKw - keyword predicate
// [ ] IsOp - operator predicate
// [ ] IsPunc - punctuator predicate
// [ ] EmitUnsupportedConstruct - E-UNS-0101 diagnostic
// [ ] MakeErrorType - create error type for recovery
// [ ] NormalizeBindingPattern - normalize typed patterns
// [ ] EmitImportUnsupported - E-UNS-0110 diagnostic
// [ ] EmitExternUnsupported - E-UNS-0112 diagnostic
// [ ] EmitReturnAtModuleErr - E-SEM-3165 diagnostic
