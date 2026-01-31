// =============================================================================
// MIGRATION MAPPING: Record Declaration Parsing
// =============================================================================
// Destination: cursive/src/02_source/parser/item/record_decl.cpp
// Source:      cursive-bootstrap/src/02_syntax/parser_items.cpp (lines 860-1136, 2040-2064)
//
// PURPOSE:
// Parse record declarations with fields and methods:
// public record Point { x: i32, y: i32, procedure distance(~) -> f64 { ... } }
// =============================================================================

// =============================================================================
// SPEC RULES IMPLEMENTED
// =============================================================================
// From C0updated.md Section 3.3.6.6:
//
// (Parse-Record)
// Γ ⊢ ParseAttrListOpt(P) ⇓ (P_0, attrs_opt)
// Γ ⊢ ParseVis(P_0) ⇓ (P_1, vis)
// IsKw(Tok(P_1), `record`)
// Γ ⊢ ParseIdent(Advance(P_1)) ⇓ (P_2, name)
// Γ ⊢ ParseGenericParamsOpt(P_2) ⇓ (P_3, gen_params_opt)
// Γ ⊢ ParseImplementsOpt(P_3) ⇓ (P_4, impls)
// Γ ⊢ ParseWhereClauseOpt(P_4) ⇓ (P_5, where_clause_opt)
// Γ ⊢ ParseRecordBody(P_5) ⇓ (P_6, members)
// Γ ⊢ ParseInvariantOpt(P_6) ⇓ (P_7, invariant_opt)
// ────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseItem(P) ⇓ (P_7, ⟨RecordDecl, attrs_opt, vis, name,
//     gen_params_opt, where_clause_opt, impls, members, invariant_opt,
//     SpanBetween(P, P_7), []⟩)
//
// (Parse-RecordBody)
// IsPunc(Tok(P), "{")
// Γ ⊢ ParseRecordMemberList(Advance(P)) ⇓ (P_1, members)
// IsPunc(Tok(P_1), "}")
// ────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseRecordBody(P) ⇓ (Advance(P_1), members)
//
// (Parse-RecordMember-Method)
// Γ ⊢ ParseAttrListOpt(P) ⇓ (P_0, attrs_opt)
// Γ ⊢ ParseVis(P_0) ⇓ (P_1, vis)
// IsKw(Tok(P_1), `procedure`)
// Γ ⊢ ParseMethodDefAfterVis(P_1, vis, attrs_opt) ⇓ (P_2, m)
// ────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseRecordMember(P) ⇓ (P_2, m)
//
// (Parse-RecordMember-Field)
// Γ ⊢ ParseAttrListOpt(P) ⇓ (P_0, attrs_opt)
// Γ ⊢ ParseVis(P_0) ⇓ (P_1, vis)
// ¬ IsKw(Tok(P_1), `procedure`)
// Γ ⊢ ParseRecordFieldDeclAfterVis(P_1, vis, attrs_opt) ⇓ (P_2, f)
// ────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseRecordMember(P) ⇓ (P_2, f)

// =============================================================================
// SOURCE CODE MAPPING
// =============================================================================

/*
SOURCE: cursive-bootstrap/src/02_syntax/parser_items.cpp (lines 860-896)
FUNCTION: ParseRecordFieldDeclAfterVis

ORIGINAL:
ParseElemResult<FieldDecl> ParseRecordFieldDeclAfterVis(Parser parser,
                                                       Visibility vis) {
  SPEC_RULE("Parse-RecordFieldDeclAfterVis");
  Parser start = parser;

  // C0X Extension: Check for key boundary marker (#)
  bool key_boundary = false;
  if (IsOp(parser, "#")) {
    SPEC_RULE("Parse-KeyBoundary");
    key_boundary = true;
    Advance(parser);
  }

  ParseElemResult<Identifier> name = ParseIdent(parser);
  if (!IsPunc(name.parser, ":")) {
    EmitParseSyntaxErr(name.parser, TokSpan(name.parser));
  } else {
    Advance(name.parser);
  }
  ParseElemResult<std::shared_ptr<Type>> ty = ParseType(name.parser);
  RecordFieldInitOptResult init = ParseRecordFieldInitOpt(ty.parser);
  FieldDecl field;
  field.vis = vis;
  field.key_boundary = key_boundary;  // C0X Extension
  field.name = name.elem;
  field.type = ty.elem;
  field.init_opt = init.init_opt;
  field.span = SpanBetween(start, init.parser);
  field.doc_opt = std::nullopt;
  return {init.parser, field};
}

IMPLEMENTATION NOTES:
- Fields can have key boundary marker (#) for synchronized access
- Field format: [#] name: Type [= default_expr]
- Default initializer is optional
*/

/*
SOURCE: cursive-bootstrap/src/02_syntax/parser_items.cpp (lines 1017-1041)
FUNCTION: ParseMethodDefAfterVis

ORIGINAL:
ParseElemResult<MethodDecl> ParseMethodDefAfterVis(Parser parser, Visibility vis) {
  SPEC_RULE("Parse-MethodDefAfterVis");
  Parser start = parser;
  OverrideResult ov = ParseOverrideOpt(parser);
  if (!IsKw(ov.parser, "procedure")) {
    EmitParseSyntaxErr(ov.parser, TokSpan(ov.parser));
  } else {
    Advance(ov.parser);
  }
  ParseElemResult<Identifier> name = ParseIdent(ov.parser);
  MethodSignatureResult sig = ParseMethodSignature(name.parser);
  ParseElemResult<std::shared_ptr<Block>> body = ParseBlock(sig.parser);
  MethodDecl method;
  method.vis = vis;
  method.override_flag = ov.override_flag;
  method.name = name.elem;
  method.receiver = sig.receiver;
  method.params = sig.params;
  method.return_type_opt = sig.return_type_opt;
  method.body = body.elem;
  method.span = SpanBetween(start, body.parser);
  method.doc_opt = std::nullopt;
  return {body.parser, method};
}

IMPLEMENTATION NOTES:
- Methods have receivers (self parameter)
- Can be marked with "override" keyword
- Always have a body (concrete implementation)
*/

/*
SOURCE: cursive-bootstrap/src/02_syntax/parser_items.cpp (lines 1043-1136)
FUNCTIONS: ParseRecordMember, ParseRecordMemberTail, ParseRecordMemberList, ParseRecordBody

IMPLEMENTATION NOTES:
- Members can be fields or methods
- Comma or newline separates members
- Trailing comma handling with error
- Methods identified by "procedure" or "override" keyword
*/

/*
SOURCE: cursive-bootstrap/src/02_syntax/parser_items.cpp (lines 2040-2064)
CONTEXT: Inside ParseItem function

ORIGINAL:
if (IsKw(cur, "record")) {
  SPEC_RULE("Parse-Record");
  Parser next = cur;
  Advance(next);
  ParseElemResult<Identifier> name = ParseIdent(next);
  ParseElemResult<std::optional<GenericParams>> gen_params = ParseGenericParamsOpt(name.parser);
  ParseElemResult<std::vector<ClassPath>> impls = ParseImplementsOpt(gen_params.parser);
  ParseElemResult<std::optional<WhereClause>> where_clause = ParseWhereClauseOpt(impls.parser);
  ParseElemResult<std::vector<RecordMember>> members = ParseRecordBody(where_clause.parser);
  ParseElemResult<std::optional<TypeInvariant>> invariant = ParseInvariantOpt(members.parser);
  RecordDecl decl;
  decl.attrs = attrs.elem;
  decl.vis = vis.elem;
  decl.name = name.elem;
  decl.generic_params = gen_params.elem;
  decl.implements = std::move(impls.elem);
  decl.where_clause = where_clause.elem;
  decl.invariant = invariant.elem;
  decl.members = std::move(members.elem);
  decl.span = SpanBetween(start, invariant.parser);
  decl.doc = {};
  return {invariant.parser, decl};
}
*/

// =============================================================================
// TYPE DEFINITIONS USED
// =============================================================================

/*
STRUCT: FieldDecl
struct FieldDecl {
  AttributeList attrs_opt;
  Visibility vis;
  bool key_boundary;  // # prefix for key boundary
  Identifier name;
  std::shared_ptr<Type> type;
  std::shared_ptr<Expr> init_opt;  // Default value, nullable
  core::Span span;
  std::optional<std::string> doc_opt;
};

STRUCT: MethodDecl
struct MethodDecl {
  Visibility vis;
  bool override_flag;
  Identifier name;
  Receiver receiver;
  std::vector<Param> params;
  std::shared_ptr<Type> return_type_opt;
  std::shared_ptr<Block> body;
  core::Span span;
  std::optional<std::string> doc_opt;
};

VARIANT: RecordMember
using RecordMember = std::variant<FieldDecl, MethodDecl>;

STRUCT: TypeInvariant
struct TypeInvariant {
  std::shared_ptr<Expr> predicate;
  core::Span span;
};

STRUCT: RecordDecl
struct RecordDecl {
  AttributeList attrs;
  Visibility vis;
  Identifier name;
  std::optional<GenericParams> generic_params;
  std::vector<ClassPath> implements;  // <: Class1, Class2
  std::optional<WhereClause> where_clause;
  std::optional<TypeInvariant> invariant;  // where { pred }
  std::vector<RecordMember> members;
  core::Span span;
  std::optional<std::string> doc;
};
*/

// =============================================================================
// GRAMMAR
// =============================================================================

/*
RecordDecl ::= AttributeList? Visibility? 'record' IDENTIFIER
               GenericParams? Implements? WhereClause?
               RecordBody InvariantOpt

RecordBody ::= '{' RecordMemberList '}'
RecordMemberList ::= (RecordMember ((',' | NEWLINE) RecordMember)*)? ','?
RecordMember ::= FieldDecl | MethodDecl

FieldDecl ::= Visibility? KeyBoundary? IDENTIFIER ':' Type FieldInit?
KeyBoundary ::= '#'
FieldInit ::= '=' Expr

MethodDecl ::= Visibility? 'override'? 'procedure' IDENTIFIER
               MethodSignature Block

Implements ::= '<:' ClassList
ClassList ::= ClassPath (',' ClassPath)*
InvariantOpt ::= 'where' '{' Expr '}' | ε
*/

// =============================================================================
// KEY BOUNDARY FIELDS
// =============================================================================

/*
Fields prefixed with # establish key boundaries for synchronized access:

public record SharedContainer {
    #data: shared DataStore,     // # creates key boundary
    metadata: i32                // No boundary
}

// Key acquisition stops at # boundary
#container.data write {
    // Acquires key for container.data only
    // Not for nested paths beyond the boundary
}
*/

// =============================================================================
// EXAMPLES
// =============================================================================

/*
// Simple record
public record Point {
    x: i32,
    y: i32
}

// With methods
public record Counter {
    value: i32,

    procedure new(initial: i32) -> Counter {
        return Counter{ value: initial }
    }

    procedure get(~) -> i32 {
        return self.value
    }

    procedure increment(~!) -> () {
        self.value = self.value + 1
    }
}

// With implements
public record MyType <: Comparable, Printable {
    value: i32,

    override procedure compare(~, other: const Self) -> i32 {
        return self.value - other.value
    }

    override procedure print(~) -> string@Managed {
        // ...
    }
}

// With invariant
public record PositiveCounter where { self.value > 0 } {
    value: i32,

    procedure increment(~!) -> () {
        self.value = self.value + 1
    }
}

// With key boundary
public record SharedState {
    #data: shared Buffer,
    count: i32
}
*/

// =============================================================================
// IMPLEMENTATION CHECKLIST
// =============================================================================
// [ ] Integrate into ParseItem dispatch
//     - Check for "record" keyword after visibility
// [ ] ParseRecordFieldDeclAfterVis function
//     - Check for # key boundary
//     - Parse name : Type
//     - Parse optional = default
// [ ] ParseMethodDefAfterVis function
//     - Parse optional "override"
//     - Parse "procedure" name
//     - Parse method signature with receiver
//     - Parse body block
// [ ] ParseRecordMember function
//     - Dispatch to field or method based on keyword
// [ ] ParseRecordMemberList function
//     - Parse comma/newline separated members
// [ ] ParseRecordBody function
//     - Expect {, parse members, expect }
// [ ] ParseImplementsOpt function
//     - Check for <: operator
//     - Parse class list
// [ ] ParseInvariantOpt function
//     - Check for "where" followed by {
//     - Parse predicate expression
// [ ] Build RecordDecl with all components

// =============================================================================
// PSEUDO-IMPLEMENTATION
// =============================================================================

/*
ParseItemResult ParseRecordDecl(Parser parser, AttributeList attrs, Visibility vis) {
  Parser start = parser;
  Advance(parser);  // consume 'record'

  auto name = ParseIdent(parser);
  parser = name.parser;

  auto gen_params = ParseGenericParamsOpt(parser);
  parser = gen_params.parser;

  auto impls = ParseImplementsOpt(parser);
  parser = impls.parser;

  auto where_clause = ParseWhereClauseOpt(parser);
  parser = where_clause.parser;

  auto members = ParseRecordBody(parser);
  parser = members.parser;

  auto invariant = ParseInvariantOpt(parser);
  parser = invariant.parser;

  RecordDecl decl;
  decl.attrs = attrs;
  decl.vis = vis;
  decl.name = name.elem;
  decl.generic_params = gen_params.elem;
  decl.implements = impls.elem;
  decl.where_clause = where_clause.elem;
  decl.members = members.elem;
  decl.invariant = invariant.elem;
  decl.span = SpanBetween(start, parser);

  return {parser, decl};
}
*/
