// =============================================================================
// MIGRATION MAPPING: Attribute List Parsing
// =============================================================================
// Destination: cursive/src/02_source/parser/item/attribute_list.cpp
// Source:      cursive-bootstrap/src/02_syntax/parser_items.cpp (lines 260-360)
//
// PURPOSE:
// Parse attribute lists: [[attr1, attr2(args), ...]]
// Attributes can have optional arguments in parentheses.
// =============================================================================

// =============================================================================
// SPEC RULES IMPLEMENTED
// =============================================================================
// From C0updated.md Section 3.3.6.13 (Item Helper Parsing Rules):
//
// (Parse-Attribute)
// IsAttrStart(P)    Γ ⊢ ParseAttrItem(Advance(Advance(P))) ⇓ (P_1, item)
// Γ ⊢ ParseAttrItemTail(P_1, [item]) ⇓ (P_2, items)
// IsOp(Tok(P_2), "]]")
// ────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseAttrListOpt(P) ⇓ (Advance(P_2), items)
//
// (Parse-AttrListOpt-None)
// ¬ IsAttrStart(P)
// ──────────────────────────────────────────────
// Γ ⊢ ParseAttrListOpt(P) ⇓ (P, [])

// =============================================================================
// SOURCE CODE MAPPING
// =============================================================================

/*
SOURCE: cursive-bootstrap/src/02_syntax/parser_items.cpp (lines 262-297)
FUNCTION: ParseAttributeItem

ORIGINAL:
ParseElemResult<AttributeItem> ParseAttributeItem(Parser parser) {
  AttributeItem item;
  const Token* tok = Tok(parser);
  if (!tok) {
    EmitParseSyntaxErr(parser, TokSpan(parser));
    return {parser, item};
  }

  // Accept both identifiers and keywords as attribute names
  if (tok->kind != TokenKind::Identifier && tok->kind != TokenKind::Keyword) {
    EmitParseSyntaxErr(parser, TokSpan(parser));
    return {parser, item};
  }

  item.name = std::string(tok->lexeme);
  Parser next = parser;
  Advance(next);

  // Check for arguments in parentheses
  if (IsPunc(next, "(")) {
    Advance(next);  // consume (
    // For now, skip to closing paren - full arg parsing can be added later
    int depth = 1;
    while (depth > 0 && Tok(next)) {
      if (IsPunc(next, "(")) depth++;
      else if (IsPunc(next, ")")) depth--;
      if (depth > 0) Advance(next);
    }
    if (IsPunc(next, ")")) Advance(next);
  }

  item.span = SpanBetween(parser, next);
  return {next, item};
}

IMPLEMENTATION NOTES:
- Attribute names can be keywords (e.g., "dynamic") or identifiers
- Arguments are skipped by counting parenthesis depth
- Full argument parsing (string literals, nested structures) can be added later
*/

/*
SOURCE: cursive-bootstrap/src/02_syntax/parser_items.cpp (lines 299-360)
FUNCTION: ParseAttributeListOpt

ORIGINAL:
ParseElemResult<AttributeList> ParseAttributeListOpt(Parser parser) {
  AttributeList attrs;

  auto is_attr_start = [](const Parser& cur) -> bool {
    if (IsPunc(cur, "[[")) {
      return true;
    }
    if (IsPunc(cur, "[")) {
      Parser probe = cur;
      Advance(probe);
      return IsPunc(probe, "[");
    }
    return false;
  };

  while (is_attr_start(parser)) {
    // Consume [[ (either as a single punctuator or two '[' tokens)
    Parser next = parser;
    if (IsPunc(next, "[[")) {
      Advance(next);
    } else {
      Advance(next);
      Advance(next);
    }

    SPEC_RULE("Parse-Attribute");

    // Parse items
    ParseElemResult<AttributeItem> first = ParseAttributeItem(next);
    attrs.push_back(first.elem);
    next = first.parser;

    while (IsPunc(next, ",")) {
      Advance(next);
      ParseElemResult<AttributeItem> item = ParseAttributeItem(next);
      attrs.push_back(item.elem);
      next = item.parser;
    }

    // Expect ]] (either as a single punctuator or two ']' tokens)
    if (IsPunc(next, "]]") ) {
      Advance(next);
    } else {
      if (!IsPunc(next, "]")) {
        EmitParseSyntaxErr(next, TokSpan(next));
      } else {
        Advance(next);
      }
      if (!IsPunc(next, "]")) {
        EmitParseSyntaxErr(next, TokSpan(next));
      } else {
        Advance(next);
      }
    }

    parser = next;
  }

  return {parser, attrs};
}

IMPLEMENTATION NOTES:
- Supports both [[ ]] as single tokens and [ [ ] ] as separate tokens
- Multiple attribute blocks can appear in sequence: [[a]] [[b]]
- Items within a block are comma-separated
- Returns empty list if no attributes present
*/

// =============================================================================
// TYPE DEFINITIONS USED
// =============================================================================

/*
STRUCT: AttributeItem
struct AttributeItem {
  std::string name;          // Attribute name (e.g., "inline", "dynamic")
  // Arguments are currently skipped; future: std::vector<AttrArg> args;
  core::Span span;
};

TYPE ALIAS: AttributeList
using AttributeList = std::vector<AttributeItem>;

RESULT TYPE:
ParseElemResult<AttributeList> { Parser parser; AttributeList elem; }
*/

// =============================================================================
// GRAMMAR
// =============================================================================

/*
AttributeList ::= ('[[' AttributeItem (',' AttributeItem)* ']]')*
AttributeItem ::= (IDENTIFIER | KEYWORD) ('(' AttrArgs ')')?
AttrArgs ::= (balanced content until matching ')')
*/

// =============================================================================
// VALID ATTRIBUTES (from spec)
// =============================================================================

/*
| Attribute                  | Purpose                                            |
| -------------------------- | -------------------------------------------------- |
| [[inline]]                 | Suggest inlining                                   |
| [[inline(always)]]         | Force inlining                                     |
| [[inline(never)]]          | Never inline                                       |
| [[inline(hint)]]           | Hint to inline                                     |
| [[cold]]                   | Mark as rarely executed                            |
| [[export]]                 | Make callable from foreign code                    |
| [[no_mangle]]              | Disable name mangling (FFI)                        |
| [[symbol("name")]]         | Custom symbol name                                 |
| [[unwind]]                 | Control unwinding at FFI boundary                  |
| [[layout(C)]]              | C-compatible memory layout                         |
| [[ffi_pass_by_value]]      | Allow by-value passing of Drop types in FFI        |
| [[weak]]                   | Weak linkage                                       |
| [[dynamic]]                | Allow runtime contract/refinement verification     |
| [[static_dispatch_only]]   | Prevent dynamic dispatch for generic class methods |
*/

// =============================================================================
// IMPLEMENTATION CHECKLIST
// =============================================================================
// [ ] ParseAttributeItem function
//     - Accept identifiers and keywords as attribute names
//     - Parse optional parenthesized arguments (skip by depth counting)
//     - Track span
// [ ] ParseAttributeListOpt function
//     - Detect [[ start (single or double token)
//     - Loop to parse multiple attribute blocks
//     - Parse comma-separated items within block
//     - Handle ]] end (single or double token)
//     - Return empty list if no attributes

// =============================================================================
// PSEUDO-IMPLEMENTATION
// =============================================================================

/*
ParseElemResult<AttributeList> ParseAttributeListOpt(Parser parser) {
  AttributeList attrs;

  while (IsAttrStart(parser)) {
    // Consume [[
    ConsumeDoubleOpen(parser);

    // Parse first item
    auto first = ParseAttributeItem(parser);
    attrs.push_back(first.elem);
    parser = first.parser;

    // Parse remaining items
    while (IsPunc(parser, ",")) {
      Advance(parser);
      auto item = ParseAttributeItem(parser);
      attrs.push_back(item.elem);
      parser = item.parser;
    }

    // Consume ]]
    ConsumeDoubleClose(parser);
  }

  return {parser, attrs};
}
*/
