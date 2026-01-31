// =============================================================================
// MIGRATION MAPPING: Procedure Declaration Parsing
// =============================================================================
// Destination: cursive/src/02_source/parser/item/procedure_decl.cpp
// Source:      cursive-bootstrap/src/02_syntax/parser_items.cpp (lines 1979-2038)
//
// PURPOSE:
// Parse procedure declarations:
// public procedure add(a: i32, b: i32) -> i32 { return a + b }
// =============================================================================

// =============================================================================
// SPEC RULES IMPLEMENTED
// =============================================================================
// From C0updated.md Section 3.3.6.5:
//
// (Parse-Procedure)
// Γ ⊢ ParseAttrListOpt(P) ⇓ (P_0, attrs_opt)
// Γ ⊢ ParseVis(P_0) ⇓ (P_1, vis)
// IsKw(Tok(P_1), `procedure`)
// Γ ⊢ ParseIdent(Advance(P_1)) ⇓ (P_2, name)
// Γ ⊢ ParseGenericParamsOpt(P_2) ⇓ (P_3, gen_params_opt)
// Γ ⊢ ParseSignature(P_3) ⇓ (P_4, params, ret_opt)
// Γ ⊢ ParseWhereClauseOpt(P_4) ⇓ (P_5, where_clause_opt)
// Γ ⊢ ParseContractClauseOpt(P_5) ⇓ (P_6, contract_opt)
// Γ ⊢ ParseBlock(P_6) ⇓ (P_7, body)
// ────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseItem(P) ⇓ (P_7, ⟨ProcedureDecl, attrs_opt, vis, name,
//     gen_params_opt, where_clause_opt, params, ret_opt, contract_opt,
//     body, SpanBetween(P, P_7), []⟩)

// =============================================================================
// SOURCE CODE MAPPING
// =============================================================================

/*
SOURCE: cursive-bootstrap/src/02_syntax/parser_items.cpp (lines 1979-2038)
CONTEXT: Inside ParseItem function

ORIGINAL:
if (IsKw(cur, "procedure")) {
  SPEC_RULE("Parse-Procedure");
  // Debug output omitted for brevity
  Parser next = cur;
  Advance(next);
  ParseElemResult<Identifier> name = ParseIdent(next);
  // C0X Extension: Parse optional generic parameters
  ParseElemResult<std::optional<GenericParams>> gen_params = ParseGenericParamsOpt(name.parser);
  SignatureResult sig = ParseSignature(gen_params.parser);
  // C0X Extension: Parse optional where clause
  ParseElemResult<std::optional<WhereClause>> where_clause = ParseWhereClauseOpt(sig.parser);
  // C0X Extension: Parse optional contract clause
  ParseElemResult<std::optional<ContractClause>> contract =
      ParseContractClauseOpt(where_clause.parser);
  ParseElemResult<std::shared_ptr<Block>> body = ParseBlock(contract.parser);
  ProcedureDecl decl;
  decl.attrs = attrs.elem;  // C0X Extension
  decl.vis = vis.elem;
  decl.name = name.elem;
  decl.generic_params = gen_params.elem;  // C0X Extension
  decl.params = sig.params;
  decl.return_type_opt = sig.return_type_opt;
  decl.where_clause = where_clause.elem;  // C0X Extension
  decl.contract = contract.elem;  // C0X Extension
  decl.body = body.elem;
  decl.span = SpanBetween(start, body.parser);
  decl.doc = {};
  return {body.parser, decl};
}

IMPLEMENTATION NOTES:
- Procedures always have a body (block)
- Return type must be explicit (no implicit unit return in spec)
- Non-unit procedures must end with explicit return statement
- Generic parameters use semicolons: <T; U>
- Contract clauses use |= syntax
*/

// =============================================================================
// TYPE DEFINITIONS USED
// =============================================================================

/*
STRUCT: ProcedureDecl
struct ProcedureDecl {
  AttributeList attrs;
  Visibility vis;
  Identifier name;
  std::optional<GenericParams> generic_params;
  std::vector<Param> params;
  std::shared_ptr<Type> return_type_opt;  // nullable
  std::optional<WhereClause> where_clause;
  std::optional<ContractClause> contract;
  std::shared_ptr<Block> body;
  core::Span span;
  std::optional<std::string> doc;
};

RESULT TYPE:
ParseItemResult containing ProcedureDecl
*/

// =============================================================================
// GRAMMAR
// =============================================================================

/*
ProcedureDecl ::= AttributeList? Visibility? 'procedure' IDENTIFIER
                  GenericParams? Signature WhereClause? ContractClause?
                  Block

Signature ::= '(' ParamList ')' ReturnOpt
ReturnOpt ::= '->' Type | ε
*/

// =============================================================================
// MAIN SIGNATURE
// =============================================================================

/*
The main procedure has a special signature:
  public procedure main(move ctx: Context) -> i32

Requirements:
- Must be public
- Must take Context parameter by move
- Must return i32 (exit code)
*/

// =============================================================================
// EXAMPLES
// =============================================================================

/*
// Simple procedure
public procedure greet() -> () {
    // No return needed for ()
}

// With parameters and return
public procedure add(a: i32, b: i32) -> i32 {
    return a + b
}

// With move parameter
procedure consume(move data: unique Buffer) -> () {
    // data is consumed here
}

// With generic parameters
procedure swap<T; U>(a: T, b: U) -> (U, T) {
    return (b, a)
}

// With where clause
procedure clone<T>(x: T) -> T
    where T <: Clone
{
    return x~>clone()
}

// With contract
procedure divide(a: i32, b: i32) -> i32
    |= b != 0
{
    return a / b
}

// With everything
[[inline]]
public procedure clamp<T <: Comparable>(x: T, min: T, max: T) -> T
    where T <: Clone
    |= min <= max => @result >= min && @result <= max
{
    return if x < min { min~>clone() }
           else if x > max { max~>clone() }
           else { x }
}
*/

// =============================================================================
// IMPLEMENTATION CHECKLIST
// =============================================================================
// [ ] Integrate into ParseItem dispatch
//     - Check for "procedure" keyword after visibility
// [ ] Parse procedure declaration
//     - Consume "procedure" keyword
//     - Parse identifier name
//     - Parse optional generic parameters
//     - Parse signature (params + return type)
//     - Parse optional where clause
//     - Parse optional contract clause
//     - Parse body block
// [ ] Build ProcedureDecl
//     - Set all fields
//     - Set span

// =============================================================================
// PSEUDO-IMPLEMENTATION
// =============================================================================

/*
ParseItemResult ParseProcedureDecl(Parser parser, AttributeList attrs, Visibility vis) {
  Parser start = parser;
  Advance(parser);  // consume 'procedure'

  auto name = ParseIdent(parser);
  parser = name.parser;

  auto gen_params = ParseGenericParamsOpt(parser);
  parser = gen_params.parser;

  auto sig = ParseSignature(parser);
  parser = sig.parser;

  auto where_clause = ParseWhereClauseOpt(parser);
  parser = where_clause.parser;

  auto contract = ParseContractClauseOpt(parser);
  parser = contract.parser;

  auto body = ParseBlock(parser);
  parser = body.parser;

  ProcedureDecl decl;
  decl.attrs = attrs;
  decl.vis = vis;
  decl.name = name.elem;
  decl.generic_params = gen_params.elem;
  decl.params = sig.params;
  decl.return_type_opt = sig.return_type_opt;
  decl.where_clause = where_clause.elem;
  decl.contract = contract.elem;
  decl.body = body.elem;
  decl.span = SpanBetween(start, parser);

  return {parser, decl};
}
*/
