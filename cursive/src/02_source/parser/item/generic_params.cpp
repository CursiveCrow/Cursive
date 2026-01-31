// =============================================================================
// MIGRATION MAPPING: Generic Parameters Parsing
// =============================================================================
// Destination: cursive/src/02_source/parser/item/generic_params.cpp
// Source:      cursive-bootstrap/src/02_syntax/parser_items.cpp (lines 42-137)
//
// PURPOSE:
// Parse generic type parameters: <T; U <: Clone = DefaultType>
// Note: Parameters use SEMICOLONS (;) as separators, not commas!
// =============================================================================

// =============================================================================
// SPEC RULES IMPLEMENTED
// =============================================================================
// From C0updated.md Section 3.3.6.13:
//
// (Parse-GenericParamsOpt-None)
// ¬ IsOp(Tok(P), "<")
// ──────────────────────────────────────────────
// Γ ⊢ ParseGenericParamsOpt(P) ⇓ (P, ⊥)
//
// (Parse-GenericParamsOpt-Yes)
// Γ ⊢ ParseGenericParams(P) ⇓ (P_1, params)
// ──────────────────────────────────────────────
// Γ ⊢ ParseGenericParamsOpt(P) ⇓ (P_1, params)
//
// (Parse-GenericParams)
// IsOp(Tok(P), "<")    Γ ⊢ ParseTypeParam(Advance(P)) ⇓ (P_1, p_1)
// Γ ⊢ ParseTypeParamTail(P_1, [p_1]) ⇓ (P_2, ps)    IsOp(Tok(P_2), ">")
// ────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseGenericParams(P) ⇓ (Advance(P_2), ps)
//
// (Parse-TypeParamTail-End)
// ¬ IsPunc(Tok(P), ";")
// ──────────────────────────────────────────────
// Γ ⊢ ParseTypeParamTail(P, ps) ⇓ (P, ps)
//
// (Parse-TypeParamTail-Cons)
// IsPunc(Tok(P), ";")    Γ ⊢ ParseTypeParam(Advance(P)) ⇓ (P_1, p)
// Γ ⊢ ParseTypeParamTail(P_1, ps ++ [p]) ⇓ (P_2, ps')
// ────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseTypeParamTail(P, ps) ⇓ (P_2, ps')
//
// (Parse-TypeBoundsOpt-None)
// ¬ IsOp(Tok(P), "<:")
// ──────────────────────────────────────────────
// Γ ⊢ ParseTypeBoundsOpt(P) ⇓ (P, [])
//
// (Parse-TypeBoundsOpt-Yes)
// IsOp(Tok(P), "<:")    Γ ⊢ ParseClassBoundList(Advance(P)) ⇓ (P_1, bounds)
// ────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseTypeBoundsOpt(P) ⇓ (P_1, bounds)

// =============================================================================
// SOURCE CODE MAPPING
// =============================================================================

/*
SOURCE: cursive-bootstrap/src/02_syntax/parser_items.cpp (lines 44-71)
FUNCTION: ParseTypeBounds

ORIGINAL:
ParseElemResult<std::vector<TypeBound>> ParseTypeBounds(Parser parser) {
  std::vector<TypeBound> bounds;
  if (!IsOp(parser, "<:")) {
    return {parser, bounds};
  }
  Parser next = parser;
  Advance(next);  // consume <:

  // Parse first bound
  ParseElemResult<Identifier> first_name = ParseIdent(next);
  TypeBound first_bound;
  first_bound.class_path.push_back(first_name.elem);
  bounds.push_back(first_bound);
  next = first_name.parser;

  // Parse additional bounds separated by ","
  while (IsOp(next, ",")) {
    Advance(next);
    ParseElemResult<Identifier> bound_name = ParseIdent(next);
    TypeBound bound;
    bound.class_path.push_back(bound_name.elem);
    bounds.push_back(bound);
    next = bound_name.parser;
  }

  return {next, bounds};
}

IMPLEMENTATION NOTES:
- Bounds are separated by commas: T <: Clone, Eq
- Bounds use <: operator (subtype relation)
- Each bound is a class path (potentially qualified: module::Class)
*/

/*
SOURCE: cursive-bootstrap/src/02_syntax/parser_items.cpp (lines 73-100)
FUNCTION: ParseTypeParam

ORIGINAL:
ParseElemResult<TypeParam> ParseTypeParam(Parser parser) {
  Parser start = parser;

  // Parse name
  ParseElemResult<Identifier> name = ParseIdent(parser);

  // Parse optional bounds
  ParseElemResult<std::vector<TypeBound>> bounds = ParseTypeBounds(name.parser);

  // Parse optional default type
  std::shared_ptr<Type> default_type;
  Parser after_bounds = bounds.parser;
  if (IsOp(after_bounds, "=")) {
    Advance(after_bounds);
    ParseElemResult<std::shared_ptr<Type>> ty = ParseType(after_bounds);
    default_type = ty.elem;
    after_bounds = ty.parser;
  }

  TypeParam param;
  param.name = name.elem;
  param.bounds = bounds.elem;
  param.default_type = default_type;
  param.span = SpanBetween(start, after_bounds);

  return {after_bounds, param};
}

IMPLEMENTATION NOTES:
- Type parameter has: name, optional bounds (<: ...), optional default (= Type)
- Order matters: name first, then bounds, then default
- Example: T <: Clone = i32
*/

/*
SOURCE: cursive-bootstrap/src/02_syntax/parser_items.cpp (lines 102-137)
FUNCTION: ParseGenericParamsOpt

ORIGINAL:
ParseElemResult<std::optional<GenericParams>> ParseGenericParamsOpt(Parser parser) {
  if (!IsOp(parser, "<")) {
    return {parser, std::nullopt};
  }

  SPEC_RULE("Parse-Generic-Params");
  Parser start = parser;
  Parser next = parser;
  Advance(next);  // consume <

  GenericParams params;

  // Parse first type param
  ParseElemResult<TypeParam> first = ParseTypeParam(next);
  params.params.push_back(first.elem);
  next = first.parser;

  // Parse additional params separated by ;
  while (IsPunc(next, ";")) {
    Advance(next);
    ParseElemResult<TypeParam> param = ParseTypeParam(next);
    params.params.push_back(param.elem);
    next = param.parser;
  }

  // Expect >
  if (!IsOp(next, ">")) {
    EmitParseSyntaxErr(next, TokSpan(next));
  } else {
    Advance(next);
  }

  params.span = SpanBetween(start, next);
  return {next, params};
}

IMPLEMENTATION NOTES:
- CRITICAL: Parameters separated by SEMICOLONS, not commas!
- This is different from generic ARGUMENTS which use commas
- <T; U; V> for parameters vs <T, U, V> for arguments
- Returns std::nullopt if no < found (no generic params)
*/

// =============================================================================
// TYPE DEFINITIONS USED
// =============================================================================

/*
STRUCT: TypeBound
struct TypeBound {
  std::vector<Identifier> class_path;  // e.g., ["Clone"] or ["module", "Class"]
};

STRUCT: TypeParam
struct TypeParam {
  Identifier name;
  std::vector<TypeBound> bounds;
  std::shared_ptr<Type> default_type;  // nullable
  core::Span span;
};

STRUCT: GenericParams
struct GenericParams {
  std::vector<TypeParam> params;
  core::Span span;
};

RESULT TYPE:
ParseElemResult<std::optional<GenericParams>>
*/

// =============================================================================
// GRAMMAR
// =============================================================================

/*
GenericParams ::= '<' TypeParam (';' TypeParam)* '>'
TypeParam ::= IDENTIFIER TypeBounds? TypeDefault?
TypeBounds ::= '<:' ClassBound (',' ClassBound)*
TypeDefault ::= '=' Type
ClassBound ::= TypePath GenericArgs?

CRITICAL DISTINCTION:
- Generic PARAMETERS: <T; U; V>  (semicolons)
- Generic ARGUMENTS:  <T, U, V>  (commas)
*/

// =============================================================================
// EXAMPLES
// =============================================================================

/*
// Simple type parameter
procedure identity<T>(x: T) -> T

// With bounds
procedure clone<T <: Clone>(x: T) -> T

// Multiple parameters (semicolon separated!)
procedure swap<T; U>(a: T, b: U) -> (U, T)

// With default type
record Container<T; Alloc = DefaultAllocator> {
  data: T,
  alloc: Alloc
}

// Multiple bounds on single parameter
procedure compare<T <: Comparable, Printable>(a: T, b: T) -> i32
*/

// =============================================================================
// IMPLEMENTATION CHECKLIST
// =============================================================================
// [ ] ParseTypeBounds function
//     - Check for <: operator
//     - Parse comma-separated class bounds
//     - Return empty vector if no bounds
// [ ] ParseTypeParam function
//     - Parse identifier name
//     - Parse optional bounds (<: ...)
//     - Parse optional default (= Type)
//     - Build TypeParam with span
// [ ] ParseGenericParamsOpt function
//     - Return nullopt if no <
//     - Parse first type param
//     - Loop on semicolon (;) separators for additional params
//     - Expect closing >
//     - Track span for GenericParams

// =============================================================================
// PSEUDO-IMPLEMENTATION
// =============================================================================

/*
ParseElemResult<std::optional<GenericParams>> ParseGenericParamsOpt(Parser parser) {
  if (!IsOp(parser, "<")) {
    return {parser, std::nullopt};
  }

  Parser start = parser;
  Advance(parser);  // consume <

  GenericParams params;

  // First param
  auto first = ParseTypeParam(parser);
  params.params.push_back(first.elem);
  parser = first.parser;

  // Additional params separated by ;
  while (IsPunc(parser, ";")) {
    Advance(parser);
    auto param = ParseTypeParam(parser);
    params.params.push_back(param.elem);
    parser = param.parser;
  }

  // Expect >
  ExpectOp(parser, ">");

  params.span = SpanBetween(start, parser);
  return {parser, params};
}
*/
