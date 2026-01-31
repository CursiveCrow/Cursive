// =============================================================================
// MIGRATION MAPPING: Using Declaration Parsing
// =============================================================================
// Destination: cursive/src/02_source/parser/item/using_decl.cpp
// Source:      cursive-bootstrap/src/02_syntax/parser_items.cpp (lines 513-589, 1921-1963)
//
// PURPOSE:
// Parse using declarations with multiple forms:
// - using path::item            (bring single item into scope)
// - using path::item as alias   (bring with alias)
// - using path::{item1, item2}  (bring multiple items)
// - using path::*               (bring all items - wildcard)
// =============================================================================

// =============================================================================
// SPEC RULES IMPLEMENTED
// =============================================================================
// From C0updated.md Section 3.3.6.3:
//
// (Parse-Using-Wildcard)
// Γ ⊢ ParseAttrListOpt(P) ⇓ (P_0, attrs_opt)
// Γ ⊢ ParseVis(P_0) ⇓ (P_1, vis)
// IsKw(Tok(P_1), `using`)
// Γ ⊢ ParseModulePath(Advance(P_1)) ⇓ (P_2, mp)
// IsOp(Tok(P_2), "::")    IsOp(Tok(Advance(P_2)), "*")
// ────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseItem(P) ⇓ (Advance(Advance(P_2)), ⟨UsingDecl, attrs_opt, vis,
//                         ⟨UsingWildcard, mp⟩, SpanBetween(P, Advance(Advance(P_2))), []⟩)
//
// (Parse-Using-List)
// Γ ⊢ ParseAttrListOpt(P) ⇓ (P_0, attrs_opt)
// Γ ⊢ ParseVis(P_0) ⇓ (P_1, vis)
// IsKw(Tok(P_1), `using`)
// Γ ⊢ ParseModulePath(Advance(P_1)) ⇓ (P_2, mp)
// IsOp(Tok(P_2), "::")    IsPunc(Tok(Advance(P_2)), "{")
// Γ ⊢ ParseUsingList(Advance(Advance(P_2))) ⇓ (P_3, specs)
// ────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseItem(P) ⇓ (P_3, ⟨UsingDecl, attrs_opt, vis,
//                         ⟨UsingList, mp, specs⟩, SpanBetween(P, P_3), []⟩)
//
// (Parse-Using-Path)
// Γ ⊢ ParseAttrListOpt(P) ⇓ (P_0, attrs_opt)
// Γ ⊢ ParseVis(P_0) ⇓ (P_1, vis)
// IsKw(Tok(P_1), `using`)
// Γ ⊢ ParseModulePath(Advance(P_1)) ⇓ (P_2, path)
// ¬ IsOp(Tok(P_2), "::")
// Γ ⊢ ParseAliasOpt(P_2) ⇓ (P_3, alias_opt)
// ────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseItem(P) ⇓ (P_3, ⟨UsingDecl, attrs_opt, vis,
//                         ⟨UsingPath, path, alias_opt⟩, SpanBetween(P, P_3), []⟩)
//
// (Parse-UsingSpec)
// Γ ⊢ ParseIdent(P) ⇓ (P_1, name)
// Γ ⊢ ParseAliasOpt(P_1) ⇓ (P_2, alias_opt)
// ────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseUsingSpec(P) ⇓ (P_2, ⟨name, alias_opt⟩)

// =============================================================================
// SOURCE CODE MAPPING
// =============================================================================

/*
SOURCE: cursive-bootstrap/src/02_syntax/parser_items.cpp (lines 513-531)
FUNCTION: ParseUsingPath

ORIGINAL:
ParseElemResult<ModulePath> ParseUsingPath(Parser parser) {
  SPEC_RULE("Parse-Using-Path");
  ParseElemResult<Identifier> head = ParseIdent(parser);
  ModulePath path;
  path.push_back(head.elem);
  Parser cur = head.parser;
  while (IsOp(cur, "::")) {
    Parser after_colons = cur;
    Advance(after_colons);
    // Stop before using-list or wildcard: `using path::{...}` or `using path::*`
    if (IsPunc(after_colons, "{") || IsOp(after_colons, "*")) {
      break;
    }
    ParseElemResult<Identifier> seg = ParseIdent(after_colons);
    path.push_back(seg.elem);
    cur = seg.parser;
  }
  return {cur, path};
}

IMPLEMENTATION NOTES:
- Stops parsing path before { or * (these indicate list/wildcard form)
- Used to get the base module path before determining using variant
*/

/*
SOURCE: cursive-bootstrap/src/02_syntax/parser_items.cpp (lines 533-541)
FUNCTION: ParseUsingSpec

ORIGINAL:
ParseElemResult<UsingSpec> ParseUsingSpec(Parser parser) {
  SPEC_RULE("Parse-UsingSpec");
  ParseElemResult<Identifier> name = ParseIdent(parser);
  ParseElemResult<std::optional<Identifier>> alias = ParseAliasOpt(name.parser);
  UsingSpec spec;
  spec.name = name.elem;
  spec.alias_opt = alias.elem;
  return {alias.parser, spec};
}

IMPLEMENTATION NOTES:
- Used inside using lists: { item1 as alias1, item2 }
- Each spec is a name with optional alias
*/

/*
SOURCE: cursive-bootstrap/src/02_syntax/parser_items.cpp (lines 543-589)
FUNCTIONS: ParseUsingListTail, ParseUsingList

IMPLEMENTATION NOTES:
- Comma-separated list of UsingSpec items
- Enclosed in { }
- Handles trailing comma with error
*/

/*
SOURCE: cursive-bootstrap/src/02_syntax/parser_items.cpp (lines 1921-1963)
CONTEXT: Inside ParseItem function, using declaration branch

ORIGINAL:
if (IsKw(cur, "using")) {
  Parser next = cur;
  Advance(next);
  ParseElemResult<ModulePath> path = ParseUsingPath(next);
  if (IsOp(path.parser, "::")) {
    Parser after_colons = path.parser;
    Advance(after_colons);
    if (IsPunc(after_colons, "{")) {
      SPEC_RULE("Parse-Using-List");
      Parser after_brace = after_colons;
      Advance(after_brace);
      ParseElemResult<std::vector<UsingSpec>> specs = ParseUsingList(after_brace);
      UsingDecl decl;
      decl.vis = vis.elem;
      decl.clause = UsingList{path.elem, std::move(specs.elem)};
      decl.span = SpanBetween(start, specs.parser);
      decl.doc = {};
      return {specs.parser, decl};
    }
    if (IsOp(after_colons, "*")) {
      SPEC_RULE("Parse-Using-Wildcard");
      Parser after_star = after_colons;
      Advance(after_star);
      UsingDecl decl;
      decl.vis = vis.elem;
      decl.clause = UsingWildcard{path.elem};
      decl.span = SpanBetween(start, after_star);
      decl.doc = {};
      return {after_star, decl};
    }
    EmitParseSyntaxErr(after_colons, TokSpan(after_colons));
    // ...
  }
  SPEC_RULE("Parse-Using-Path");
  ParseElemResult<std::optional<Identifier>> alias = ParseAliasOpt(path.parser);
  UsingDecl decl;
  decl.vis = vis.elem;
  decl.clause = UsingPath{path.elem, alias.elem};
  decl.span = SpanBetween(start, alias.parser);
  decl.doc = {};
  return {alias.parser, decl};
}

IMPLEMENTATION NOTES:
- First parse the path up to (but not including) { or *
- Then determine which variant based on what follows
- If :: followed by { -> list form
- If :: followed by * -> wildcard form
- Otherwise -> path form with optional alias
*/

// =============================================================================
// TYPE DEFINITIONS USED
// =============================================================================

/*
STRUCT: UsingSpec
struct UsingSpec {
  Identifier name;
  std::optional<Identifier> alias_opt;
};

STRUCT: UsingPath
struct UsingPath {
  ModulePath path;          // Full path to item
  std::optional<Identifier> alias_opt;
};

STRUCT: UsingWildcard
struct UsingWildcard {
  ModulePath path;          // Path to module (items come from here)
};

STRUCT: UsingList
struct UsingList {
  ModulePath path;          // Path to module
  std::vector<UsingSpec> specs;  // Items to import
};

VARIANT: UsingClause
using UsingClause = std::variant<UsingPath, UsingWildcard, UsingList>;

STRUCT: UsingDecl
struct UsingDecl {
  Visibility vis;
  UsingClause clause;
  core::Span span;
  std::optional<std::string> doc;
};

RESULT TYPE:
ParseItemResult containing UsingDecl
*/

// =============================================================================
// GRAMMAR
// =============================================================================

/*
UsingDecl ::= Visibility? 'using' UsingClause
UsingClause ::=
    ModulePath '::' '*'                          -- wildcard
  | ModulePath '::' '{' UsingList '}'            -- list
  | ModulePath AliasOpt                          -- single path

UsingList ::= UsingSpec (',' UsingSpec)* ','?
UsingSpec ::= IDENTIFIER AliasOpt
AliasOpt ::= 'as' IDENTIFIER | ε
*/

// =============================================================================
// EXAMPLES
// =============================================================================

/*
// Bring single item into scope
using cursive::runtime::string

// With alias
using cursive::runtime::string as str

// Multiple items
using mymodule::{foo, bar, baz as qux}

// Wildcard - all items
using othermodule::*
*/

// =============================================================================
// IMPLEMENTATION CHECKLIST
// =============================================================================
// [ ] Integrate into ParseItem dispatch
//     - Check for "using" keyword after visibility
// [ ] ParseUsingPath function
//     - Parse path segments until { or * encountered
// [ ] ParseUsingSpec function
//     - Parse identifier with optional alias
// [ ] ParseUsingList function
//     - Parse comma-separated UsingSpec items
//     - Handle trailing comma
// [ ] Main using parsing logic
//     - Determine variant (wildcard, list, or path) after parsing base path
//     - Build appropriate UsingClause variant

// =============================================================================
// PSEUDO-IMPLEMENTATION
// =============================================================================

/*
ParseItemResult ParseUsingDecl(Parser parser, Visibility vis) {
  Parser start = parser;
  Advance(parser);  // consume 'using'

  auto path = ParseUsingPath(parser);
  parser = path.parser;

  UsingDecl decl;
  decl.vis = vis;

  if (IsOp(parser, "::")) {
    Advance(parser);

    if (IsPunc(parser, "{")) {
      // List form
      Advance(parser);
      auto specs = ParseUsingList(parser);
      parser = specs.parser;
      decl.clause = UsingList{path.elem, specs.elem};
    } else if (IsOp(parser, "*")) {
      // Wildcard form
      Advance(parser);
      decl.clause = UsingWildcard{path.elem};
    } else {
      EmitError(parser);
    }
  } else {
    // Simple path form
    auto alias = ParseAliasOpt(parser);
    parser = alias.parser;
    decl.clause = UsingPath{path.elem, alias.elem};
  }

  decl.span = SpanBetween(start, parser);
  return {parser, decl};
}
*/
