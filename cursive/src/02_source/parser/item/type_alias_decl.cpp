// =============================================================================
// MIGRATION MAPPING: Type Alias Declaration Parsing
// =============================================================================
// Destination: cursive/src/02_source/parser/item/type_alias_decl.cpp
// Source:      cursive-bootstrap/src/02_syntax/parser_items.cpp (lines 2144-2168)
//
// PURPOSE:
// Parse type alias declarations:
// public type Result = i32 | AllocationError
// public type Callback<T> = (T) -> ()
// =============================================================================

// =============================================================================
// SPEC RULES IMPLEMENTED
// =============================================================================
// From C0updated.md Section 3.3.6.11:
//
// (Parse-Type-Alias)
// Γ ⊢ ParseAttrListOpt(P) ⇓ (P_0, attrs_opt)
// Γ ⊢ ParseVis(P_0) ⇓ (P_1, vis)
// IsKw(Tok(P_1), `type`)
// Γ ⊢ ParseIdent(Advance(P_1)) ⇓ (P_2, name)
// Γ ⊢ ParseGenericParamsOpt(P_2) ⇓ (P_3, gen_params_opt)
// Γ ⊢ ParseWhereClauseOpt(P_3) ⇓ (P_4, where_clause_opt)
// IsOp(Tok(P_4), "=")
// Γ ⊢ ParseType(Advance(P_4)) ⇓ (P_5, ty)
// ────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseItem(P) ⇓ (P_5, ⟨TypeAliasDecl, attrs_opt, vis, name,
//     gen_params_opt, where_clause_opt, ty, SpanBetween(P, P_5), []⟩)

// =============================================================================
// SOURCE CODE MAPPING
// =============================================================================

/*
SOURCE: cursive-bootstrap/src/02_syntax/parser_items.cpp (lines 2144-2168)
CONTEXT: Inside ParseItem function

ORIGINAL:
if (IsKw(cur, "type")) {
  SPEC_RULE("Parse-Type-Alias");
  Parser next = cur;
  Advance(next);
  ParseElemResult<Identifier> name = ParseIdent(next);
  // C0X Extension: Parse optional generic parameters
  ParseElemResult<std::optional<GenericParams>> gen_params = ParseGenericParamsOpt(name.parser);
  if (!IsOp(gen_params.parser, "=")) {
    EmitParseSyntaxErr(gen_params.parser, TokSpan(gen_params.parser));
  } else {
    Advance(gen_params.parser);
  }
  ParseElemResult<std::shared_ptr<Type>> ty = ParseType(gen_params.parser);
  // C0X Extension: Parse optional where clause
  ParseElemResult<std::optional<WhereClause>> where_clause = ParseWhereClauseOpt(ty.parser);
  TypeAliasDecl decl;
  decl.vis = vis.elem;
  decl.name = name.elem;
  decl.generic_params = gen_params.elem;  // C0X Extension
  decl.type = ty.elem;
  decl.where_clause = where_clause.elem;  // C0X Extension
  decl.span = SpanBetween(start, where_clause.parser);
  decl.doc = {};
  return {where_clause.parser, decl};
}

IMPLEMENTATION NOTES:
- Type aliases introduce a new name for an existing type
- Can be generic: type Result<T> = T | Error
- Can have where clause constraints
- The = is required (unlike class associated types which can be abstract)
*/

// =============================================================================
// TYPE DEFINITIONS USED
// =============================================================================

/*
STRUCT: TypeAliasDecl
struct TypeAliasDecl {
  Visibility vis;
  Identifier name;
  std::optional<GenericParams> generic_params;
  std::shared_ptr<Type> type;
  std::optional<WhereClause> where_clause;
  core::Span span;
  std::optional<std::string> doc;
};

RESULT TYPE:
ParseItemResult containing TypeAliasDecl
*/

// =============================================================================
// GRAMMAR
// =============================================================================

/*
TypeAliasDecl ::= AttributeList? Visibility? 'type' IDENTIFIER
                  GenericParams? '=' Type WhereClause?

Note: The where clause comes AFTER the type in the source implementation,
      but the spec shows it before. Follow source for compatibility.
*/

// =============================================================================
// TYPE REFINEMENTS
// =============================================================================

/*
Types can have predicate refinements using where { predicate }:

// In type alias
public type PositiveInt = i32 where { self > 0 }

// In parameter
procedure sqrt(x: f64 where { x >= 0.0f64 }) -> f64 {
    // x is guaranteed non-negative
    return ...
}

// In return type
procedure abs(x: i32) -> i32 where { self >= 0 } {
    return if x < 0 { -x } else { x }
}

Note: This is different from where clauses on type aliases which constrain
generic parameters. The where { predicate } syntax is for type refinements.
*/

// =============================================================================
// EXAMPLES
// =============================================================================

/*
// Simple alias
public type Result = i32 | AllocationError

// Function type alias
public type Callback = (i32) -> ()

// Generic alias
public type Option<T> = T | ()

// Alias for complex type
public type AsyncResult<T; E = IOError> = Async<(), (), T, E>

// Built-in type aliases (from spec)
type Sequence<T> = Async<T, (), (), !>
type Future<T; E = !> = Async<(), (), T, E>
type Pipe<In; Out> = Async<Out, In, (), !>
type Exchange<T> = Async<T, T, T, !>
type Stream<T; E> = Async<T, (), (), E>

// With refinement (different from where clause!)
public type NonEmptyString = string@View where { self~>length() > 0 }
*/

// =============================================================================
// DISTINCTION: TYPE ALIAS vs ASSOCIATED TYPE
// =============================================================================

/*
Type alias (module level):
  type Foo = Bar

Associated type (in class):
  public class Container {
      type Item              // Abstract - no definition
      type Item = i32        // Concrete - with definition
  }

Key differences:
- Type aliases always have = Type
- Associated types can be abstract (no =)
- Type aliases are standalone items
- Associated types are inside class bodies
*/

// =============================================================================
// IMPLEMENTATION CHECKLIST
// =============================================================================
// [ ] Integrate into ParseItem dispatch
//     - Check for "type" keyword after visibility
//     - Distinguish from associated type in class (context matters)
// [ ] Parse type alias declaration
//     - Consume "type" keyword
//     - Parse identifier name
//     - Parse optional generic parameters
//     - Expect = operator
//     - Parse type
//     - Parse optional where clause
// [ ] Build TypeAliasDecl with all components

// =============================================================================
// PSEUDO-IMPLEMENTATION
// =============================================================================

/*
ParseItemResult ParseTypeAliasDecl(Parser parser, Visibility vis) {
  Parser start = parser;
  Advance(parser);  // consume 'type'

  auto name = ParseIdent(parser);
  parser = name.parser;

  auto gen_params = ParseGenericParamsOpt(parser);
  parser = gen_params.parser;

  // Expect =
  if (!IsOp(parser, "=")) {
    EmitError(parser);
  } else {
    Advance(parser);
  }

  auto type = ParseType(parser);
  parser = type.parser;

  auto where_clause = ParseWhereClauseOpt(parser);
  parser = where_clause.parser;

  TypeAliasDecl decl;
  decl.vis = vis;
  decl.name = name.elem;
  decl.generic_params = gen_params.elem;
  decl.type = type.elem;
  decl.where_clause = where_clause.elem;
  decl.span = SpanBetween(start, parser);

  return {parser, decl};
}
*/
