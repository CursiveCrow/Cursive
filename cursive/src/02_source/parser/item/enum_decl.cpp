// =============================================================================
// MIGRATION MAPPING: Enum Declaration Parsing
// =============================================================================
// Destination: cursive/src/02_source/parser/item/enum_decl.cpp
// Source:      cursive-bootstrap/src/02_syntax/parser_items.cpp (lines 1138-1288, 2067-2092)
//
// PURPOSE:
// Parse enum declarations with variants:
// public enum Result { Ok(i32), Err(string@View) }
// =============================================================================

// =============================================================================
// SPEC RULES IMPLEMENTED
// =============================================================================
// From C0updated.md Section 3.3.6.7:
//
// (Parse-Enum)
// Γ ⊢ ParseAttrListOpt(P) ⇓ (P_0, attrs_opt)
// Γ ⊢ ParseVis(P_0) ⇓ (P_1, vis)
// IsKw(Tok(P_1), `enum`)
// Γ ⊢ ParseIdent(Advance(P_1)) ⇓ (P_2, name)
// Γ ⊢ ParseGenericParamsOpt(P_2) ⇓ (P_3, gen_params_opt)
// Γ ⊢ ParseImplementsOpt(P_3) ⇓ (P_4, impls)
// Γ ⊢ ParseWhereClauseOpt(P_4) ⇓ (P_5, where_clause_opt)
// Γ ⊢ ParseEnumBody(P_5) ⇓ (P_6, variants)
// Γ ⊢ ParseInvariantOpt(P_6) ⇓ (P_7, invariant_opt)
// ────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseItem(P) ⇓ (P_7, ⟨EnumDecl, attrs_opt, vis, name,
//     gen_params_opt, where_clause_opt, impls, variants, invariant_opt,
//     SpanBetween(P, P_7), []⟩)
//
// (Parse-EnumBody)
// IsPunc(Tok(P), "{")
// Γ ⊢ ParseVariantList(Advance(P)) ⇓ (P_1, vars)
// IsPunc(Tok(P_1), "}")
// ────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseEnumBody(P) ⇓ (Advance(P_1), vars)

// =============================================================================
// SOURCE CODE MAPPING
// =============================================================================

/*
SOURCE: cursive-bootstrap/src/02_syntax/parser_items.cpp (lines 1138-1174)
FUNCTION: ParseVariantPayloadOpt

ORIGINAL:
ParseElemResult<VariantPayload> ParseVariantPayloadOpt(Parser parser, bool& has_payload) {
  if (!IsPunc(parser, "(") && !IsPunc(parser, "{")) {
    SPEC_RULE("Parse-VariantPayloadOpt-None");
    has_payload = false;
    VariantPayload payload = VariantPayloadTuple{};
    return {parser, payload};
  }
  if (IsPunc(parser, "(")) {
    SPEC_RULE("Parse-VariantPayloadOpt-Tuple");
    Parser next = parser;
    Advance(next);
    ParseElemResult<std::vector<std::shared_ptr<Type>>> types = ParseTypeList(next);
    if (!IsPunc(types.parser, ")")) {
      EmitParseSyntaxErr(types.parser, TokSpan(types.parser));
    } else {
      Advance(types.parser);
    }
    VariantPayloadTuple tuple;
    tuple.elements = std::move(types.elem);
    has_payload = true;
    return {types.parser, tuple};
  }
  SPEC_RULE("Parse-VariantPayloadOpt-Record");
  Parser next = parser;
  Advance(next);
  ParseElemResult<std::vector<FieldDecl>> fields = ParseFieldDeclList(next);
  if (!IsPunc(fields.parser, "}")) {
    EmitParseSyntaxErr(fields.parser, TokSpan(fields.parser));
  } else {
    Advance(fields.parser);
  }
  VariantPayloadRecord record;
  record.fields = std::move(fields.elem);
  has_payload = true;
  return {fields.parser, record};
}

IMPLEMENTATION NOTES:
- Variant payloads can be:
  1. None (unit variant)
  2. Tuple: Ok(i32, bool)
  3. Record: Error{ code: i32, message: string@View }
*/

/*
SOURCE: cursive-bootstrap/src/02_syntax/parser_items.cpp (lines 1181-1197)
FUNCTION: ParseVariantDiscriminantOpt

ORIGINAL:
VariantDiscriminantResult ParseVariantDiscriminantOpt(Parser parser) {
  if (!IsOp(parser, "=")) {
    SPEC_RULE("Parse-VariantDiscriminantOpt-None");
    return {parser, std::nullopt};
  }
  SPEC_RULE("Parse-VariantDiscriminantOpt-Yes");
  Parser next = parser;
  Advance(next);
  const Token* tok = Tok(next);
  if (!tok || tok->kind != TokenKind::IntLiteral) {
    EmitParseSyntaxErr(next, TokSpan(next));
    return {next, std::nullopt};
  }
  Token lit = *tok;
  Advance(next);
  return {next, lit};
}

IMPLEMENTATION NOTES:
- Explicit discriminant: Active = 1
- Must be integer literal
*/

/*
SOURCE: cursive-bootstrap/src/02_syntax/parser_items.cpp (lines 1199-1216)
FUNCTION: ParseVariant

ORIGINAL:
ParseElemResult<VariantDecl> ParseVariant(Parser parser) {
  SPEC_RULE("Parse-Variant");
  Parser start = parser;
  ParseElemResult<Identifier> name = ParseIdent(parser);
  bool has_payload = false;
  ParseElemResult<VariantPayload> payload = ParseVariantPayloadOpt(name.parser, has_payload);
  VariantDiscriminantResult disc = ParseVariantDiscriminantOpt(payload.parser);
  VariantDecl var;
  var.name = name.elem;
  if (has_payload) {
    var.payload_opt = payload.elem;
  }
  var.discriminant_opt = disc.disc_opt;
  var.span = SpanBetween(start, disc.parser);
  var.doc_opt = std::nullopt;
  return {disc.parser, var};
}
*/

/*
SOURCE: cursive-bootstrap/src/02_syntax/parser_items.cpp (lines 1218-1288)
FUNCTIONS: ParseVariantTail, ParseVariantList, ParseEnumBody

IMPLEMENTATION NOTES:
- Variants separated by comma or newline
- Handles trailing comma with error
*/

/*
SOURCE: cursive-bootstrap/src/02_syntax/parser_items.cpp (lines 2067-2092)
CONTEXT: Inside ParseItem function

ORIGINAL:
if (IsKw(cur, "enum")) {
  SPEC_RULE("Parse-Enum");
  Parser next = cur;
  Advance(next);
  ParseElemResult<Identifier> name = ParseIdent(next);
  ParseElemResult<std::optional<GenericParams>> gen_params = ParseGenericParamsOpt(name.parser);
  ParseElemResult<std::vector<ClassPath>> impls = ParseImplementsOpt(gen_params.parser);
  ParseElemResult<std::optional<WhereClause>> where_clause = ParseWhereClauseOpt(impls.parser);
  ParseElemResult<std::vector<VariantDecl>> vars = ParseEnumBody(where_clause.parser);
  ParseElemResult<std::optional<TypeInvariant>> invariant = ParseInvariantOpt(vars.parser);
  EnumDecl decl;
  decl.attrs = attrs.elem;
  decl.vis = vis.elem;
  decl.name = name.elem;
  decl.generic_params = gen_params.elem;
  decl.implements = std::move(impls.elem);
  decl.where_clause = where_clause.elem;
  decl.invariant = invariant.elem;
  decl.variants = std::move(vars.elem);
  decl.span = SpanBetween(start, invariant.parser);
  decl.doc = {};
  return {invariant.parser, decl};
}
*/

// =============================================================================
// TYPE DEFINITIONS USED
// =============================================================================

/*
STRUCT: VariantPayloadTuple
struct VariantPayloadTuple {
  std::vector<std::shared_ptr<Type>> elements;
};

STRUCT: VariantPayloadRecord
struct VariantPayloadRecord {
  std::vector<FieldDecl> fields;
};

VARIANT: VariantPayload
using VariantPayload = std::variant<VariantPayloadTuple, VariantPayloadRecord>;

STRUCT: VariantDecl
struct VariantDecl {
  Identifier name;
  std::optional<VariantPayload> payload_opt;
  std::optional<Token> discriminant_opt;  // Explicit discriminant value
  core::Span span;
  std::optional<std::string> doc_opt;
};

STRUCT: EnumDecl
struct EnumDecl {
  AttributeList attrs;
  Visibility vis;
  Identifier name;
  std::optional<GenericParams> generic_params;
  std::vector<ClassPath> implements;
  std::optional<WhereClause> where_clause;
  std::optional<TypeInvariant> invariant;
  std::vector<VariantDecl> variants;
  core::Span span;
  std::optional<std::string> doc;
};
*/

// =============================================================================
// GRAMMAR
// =============================================================================

/*
EnumDecl ::= AttributeList? Visibility? 'enum' IDENTIFIER
             GenericParams? Implements? WhereClause?
             EnumBody InvariantOpt

EnumBody ::= '{' VariantList '}'
VariantList ::= (Variant ((',' | NEWLINE) Variant)*)? ','?

Variant ::= IDENTIFIER VariantPayload? Discriminant?
VariantPayload ::= TuplePayload | RecordPayload
TuplePayload ::= '(' TypeList ')'
RecordPayload ::= '{' FieldDeclList '}'
Discriminant ::= '=' INT_LITERAL
*/

// =============================================================================
// EXAMPLES
// =============================================================================

/*
// Simple enum
public enum Direction {
    North,
    South,
    East,
    West
}

// With explicit discriminants
public enum Status {
    Active = 1,
    Inactive = 0
}

// With tuple payloads
public enum Result {
    Ok(i32),
    Err(string@View)
}

// With record payloads
public enum Event {
    Click{ x: i32, y: i32 },
    KeyPress{ code: u8, modifiers: u8 }
}

// Generic enum
public enum Option<T> {
    Some(T),
    None
}

// Using enum values
procedure unwrap_or(opt: Option, default: i32) -> i32 {
    return match opt {
        Option::Some(v) => { v },
        Option::None => { default }
    }
}
*/

// =============================================================================
// IMPLEMENTATION CHECKLIST
// =============================================================================
// [ ] Integrate into ParseItem dispatch
//     - Check for "enum" keyword after visibility
// [ ] ParseVariantPayloadOpt function
//     - Check for ( or {
//     - Parse tuple payload: (Type, Type)
//     - Parse record payload: { field: Type }
// [ ] ParseVariantDiscriminantOpt function
//     - Check for = operator
//     - Parse integer literal
// [ ] ParseVariant function
//     - Parse name
//     - Parse optional payload
//     - Parse optional discriminant
// [ ] ParseVariantList function
//     - Parse comma/newline separated variants
// [ ] ParseEnumBody function
//     - Expect {, parse variants, expect }
// [ ] Build EnumDecl with all components

// =============================================================================
// PSEUDO-IMPLEMENTATION
// =============================================================================

/*
ParseItemResult ParseEnumDecl(Parser parser, AttributeList attrs, Visibility vis) {
  Parser start = parser;
  Advance(parser);  // consume 'enum'

  auto name = ParseIdent(parser);
  parser = name.parser;

  auto gen_params = ParseGenericParamsOpt(parser);
  parser = gen_params.parser;

  auto impls = ParseImplementsOpt(parser);
  parser = impls.parser;

  auto where_clause = ParseWhereClauseOpt(parser);
  parser = where_clause.parser;

  auto variants = ParseEnumBody(parser);
  parser = variants.parser;

  auto invariant = ParseInvariantOpt(parser);
  parser = invariant.parser;

  EnumDecl decl;
  decl.attrs = attrs;
  decl.vis = vis;
  decl.name = name.elem;
  decl.generic_params = gen_params.elem;
  decl.implements = impls.elem;
  decl.where_clause = where_clause.elem;
  decl.variants = variants.elem;
  decl.invariant = invariant.elem;
  decl.span = SpanBetween(start, parser);

  return {parser, decl};
}
*/
