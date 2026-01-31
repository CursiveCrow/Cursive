// =============================================================================
// MIGRATION MAPPING: Import Declaration Parsing
// =============================================================================
// Destination: cursive/src/02_source/parser/item/import_decl.cpp
// Source:      cursive-bootstrap/src/02_syntax/parser_items.cpp (lines 1739-1751)
//
// PURPOSE:
// Parse import declarations: import mylib::networking as net
// Imports bring a module into scope as a namespace.
// =============================================================================

// =============================================================================
// SPEC RULES IMPLEMENTED
// =============================================================================
// From C0updated.md Section 3.3.6.3:
//
// (Parse-Import)
// Γ ⊢ ParseAttrListOpt(P) ⇓ (P_0, attrs_opt)
// Γ ⊢ ParseVis(P_0) ⇓ (P_1, vis)
// IsKw(Tok(P_1), `import`)
// Γ ⊢ ParseModulePath(Advance(P_1)) ⇓ (P_2, path)
// Γ ⊢ ParseAliasOpt(P_2) ⇓ (P_3, alias_opt)
// ────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseItem(P) ⇓ (P_3, ⟨ImportDecl, attrs_opt, vis, path, alias_opt,
//                         SpanBetween(P, P_3), []⟩)

// =============================================================================
// SOURCE CODE MAPPING
// =============================================================================

/*
SOURCE: cursive-bootstrap/src/02_syntax/parser_items.cpp (lines 1739-1751)
CONTEXT: Inside ParseItem function

ORIGINAL:
if (IsKw(parser, "import")) {
  // C0X Extension: Import declarations - parse them
  SPEC_RULE("Parse-Import");
  Parser next = parser;
  Advance(next);  // consume import
  ParseElemResult<ModulePath> path = ParseModulePath(next);
  ParseElemResult<std::optional<Identifier>> alias = ParseAliasOpt(path.parser);
  ImportDecl decl;
  decl.vis = Visibility::Private;  // imports are private by default
  decl.path = path.elem;
  decl.alias = alias.elem;
  decl.span = SpanBetween(start, alias.parser);
  return {alias.parser, decl};
}

IMPLEMENTATION NOTES:
- Imports default to private visibility
- Module path is a sequence of identifiers separated by ::
- Optional alias via "as identifier"
- Note: attributes are parsed BEFORE checking for import keyword
*/

// =============================================================================
// TYPE DEFINITIONS USED
// =============================================================================

/*
TYPE ALIAS: ModulePath
using ModulePath = std::vector<Identifier>;

STRUCT: ImportDecl
struct ImportDecl {
  AttributeList attrs;  // Optional attributes (C0X extension)
  Visibility vis;       // Usually Private
  ModulePath path;      // e.g., ["mylib", "networking"]
  std::optional<Identifier> alias;  // e.g., "net"
  core::Span span;
};

RESULT TYPE:
ParseItemResult containing ImportDecl
*/

// =============================================================================
// GRAMMAR
// =============================================================================

/*
ImportDecl ::= AttributeList? Visibility? 'import' ModulePath AliasOpt
ModulePath ::= IDENTIFIER ('::' IDENTIFIER)*
AliasOpt ::= 'as' IDENTIFIER | ε
*/

// =============================================================================
// DISTINCTION: IMPORT vs USING
// =============================================================================

/*
import and using serve different purposes:

import mylib::networking
  - Brings module into scope as a namespace
  - Access items as: networking::foo()

import mylib::utils as u
  - Alias the namespace
  - Access items as: u::foo()

using cursive::runtime::string
  - Brings specific item directly into scope
  - Access as: string (no namespace prefix)

using mymodule::*
  - Brings ALL items from module into scope
  - Access items directly without prefix
*/

// =============================================================================
// EXAMPLES
// =============================================================================

/*
// Basic import
import mylib::networking

// Import with alias
import mylib::networking as net

// Access after import
let client: net::Client = net::create_client()
*/

// =============================================================================
// IMPLEMENTATION CHECKLIST
// =============================================================================
// [ ] Integrate into ParseItem dispatch
//     - Check for "import" keyword after attributes and visibility
// [ ] Parse import declaration
//     - Consume "import" keyword
//     - Parse module path
//     - Parse optional alias
//     - Build ImportDecl
// [ ] ParseModulePath helper (may be shared with using_decl)
//     - Parse first identifier
//     - Loop on :: separator
//     - Collect path segments
// [ ] ParseAliasOpt helper (may be shared)
//     - Check for "as" keyword
//     - Parse identifier if present

// =============================================================================
// PSEUDO-IMPLEMENTATION
// =============================================================================

/*
ParseItemResult ParseImportDecl(Parser parser, AttributeList attrs, Visibility vis) {
  Parser start = parser;

  // Consume 'import'
  Advance(parser);

  // Parse module path
  auto path = ParseModulePath(parser);
  parser = path.parser;

  // Parse optional alias
  auto alias = ParseAliasOpt(parser);
  parser = alias.parser;

  ImportDecl decl;
  decl.attrs = attrs;
  decl.vis = vis;  // Note: typically Private for imports
  decl.path = path.elem;
  decl.alias = alias.elem;
  decl.span = SpanBetween(start, parser);

  return {parser, decl};
}

ParseElemResult<ModulePath> ParseModulePath(Parser parser) {
  ModulePath path;

  auto first = ParseIdent(parser);
  path.push_back(first.elem);
  parser = first.parser;

  while (IsOp(parser, "::")) {
    Advance(parser);
    auto segment = ParseIdent(parser);
    path.push_back(segment.elem);
    parser = segment.parser;
  }

  return {parser, path};
}

ParseElemResult<std::optional<Identifier>> ParseAliasOpt(Parser parser) {
  if (!IsKw(parser, "as")) {
    return {parser, std::nullopt};
  }
  Advance(parser);
  auto name = ParseIdent(parser);
  return {name.parser, name.elem};
}
*/
