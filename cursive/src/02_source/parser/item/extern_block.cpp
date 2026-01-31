// =============================================================================
// MIGRATION MAPPING: Extern Block Parsing
// =============================================================================
// Destination: cursive/src/02_source/parser/item/extern_block.cpp
// Source:      cursive-bootstrap/src/02_syntax/parser_items.cpp (lines 1781-1898)
//
// PURPOSE:
// Parse extern blocks for FFI declarations:
// extern "C" { procedure malloc(size: usize) -> *mut u8 }
// =============================================================================

// =============================================================================
// SPEC RULES IMPLEMENTED
// =============================================================================
// From C0updated.md Section 3.3.6.3:
//
// (Parse-ExternBlock)
// Γ ⊢ ParseAttrListOpt(P) ⇓ (P_0, attrs_opt)
// Γ ⊢ ParseVis(P_0) ⇓ (P_1, vis)
// IsKw(Tok(P_1), `extern`)
// Γ ⊢ ParseExternAbiOpt(Advance(P_1)) ⇓ (P_2, abi_opt)
// IsPunc(Tok(P_2), "{")
// Γ ⊢ ParseExternItemList(Advance(P_2)) ⇓ (P_3, items)
// IsPunc(Tok(P_3), "}")
// ────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseItem(P) ⇓ (Advance(P_3), ⟨ExternBlock, attrs_opt, vis, abi_opt,
//                         items, SpanBetween(P, Advance(P_3)), []⟩)
//
// (Parse-ExternAbiOpt-None)
// Tok(P).kind ∉ {StringLiteral, Identifier}
// ──────────────────────────────────────────────
// Γ ⊢ ParseExternAbiOpt(P) ⇓ (P, ⊥)
//
// (Parse-ExternAbiOpt-String)
// Tok(P).kind = StringLiteral
// ──────────────────────────────────────────────
// Γ ⊢ ParseExternAbiOpt(P) ⇓ (Advance(P), StringAbi(Tok(P)))
//
// (Parse-ExternAbiOpt-Ident)
// IsIdent(Tok(P))
// ──────────────────────────────────────────────
// Γ ⊢ ParseExternAbiOpt(P) ⇓ (Advance(P), IdentAbi(Lexeme(Tok(P))))
//
// (Parse-ExternItem)
// [Parses extern procedure declarations with signatures and optional contracts]

// =============================================================================
// SOURCE CODE MAPPING
// =============================================================================

/*
SOURCE: cursive-bootstrap/src/02_syntax/parser_items.cpp (lines 1781-1898)
CONTEXT: Inside ParseItem function, extern block branch

ORIGINAL:
if (const Token* tok = Tok(parser);
    tok && IsIdentTok(*tok) && tok->lexeme == "extern") {
  SPEC_RULE("Parse-Extern-Block");
  Parser next = parser;
  Advance(next);  // consume extern

  // Parse optional ABI: extern "C" or extern ABI_ID
  std::optional<ExternAbi> abi_opt;
  const Token* abi_tok = Tok(next);
  if (abi_tok) {
    if (abi_tok->kind == TokenKind::StringLiteral) {
      SPEC_RULE("Parse-Extern-Abi-String");
      ExternAbiString abi_str;
      abi_str.literal = *abi_tok;
      abi_opt = abi_str;
      Advance(next);
    } else if (IsIdentTok(*abi_tok)) {
      SPEC_RULE("Parse-Extern-Abi-Ident");
      ExternAbiIdent abi_id;
      abi_id.name = abi_tok->lexeme;
      abi_opt = abi_id;
      Advance(next);
    }
  }

  // Expect opening brace
  if (!IsPunc(next, "{")) {
    EmitParseSyntaxErr(next, TokSpan(next));
    Parser sync = next;
    SyncItem(sync);
    return {sync, ErrorItem{SpanBetween(start, sync)}};
  }
  Advance(next);

  // Parse extern items
  std::vector<ExternItem> items;
  while (!IsPunc(next, "}") && !AtEof(next)) {
    // Skip newlines
    while (Tok(next) && Tok(next)->kind == TokenKind::Newline) {
      Advance(next);
    }
    if (IsPunc(next, "}")) break;

    // Parse optional attributes for extern proc
    ParseElemResult<AttributeList> item_attrs = ParseAttributeListOpt(next);
    next = item_attrs.parser;

    // Skip newlines after attributes
    while (Tok(next) && Tok(next)->kind == TokenKind::Newline) {
      Advance(next);
    }

    // Parse visibility
    ParseElemResult<Visibility> item_vis = ParseVis(next);
    next = item_vis.parser;

    // Expect procedure keyword
    if (!IsKw(next, "procedure")) {
      EmitParseSyntaxErr(next, TokSpan(next));
      SyncStmt(next);
      continue;
    }
    Advance(next);  // consume procedure

    // Parse procedure name
    ParseElemResult<Identifier> proc_name = ParseIdent(next);

    // Parse optional generic parameters
    ParseElemResult<std::optional<GenericParams>> gen_params =
        ParseGenericParamsOpt(proc_name.parser);

    // Parse signature
    SignatureResult sig = ParseSignature(gen_params.parser);

    // Parse optional where clause
    ParseElemResult<std::optional<WhereClause>> where_clause =
        ParseWhereClauseOpt(sig.parser);

    // Parse optional contract clause
    ParseElemResult<std::optional<ContractClause>> contract =
        ParseContractClauseOpt(where_clause.parser);

    ExternProcDecl proc_decl;
    proc_decl.attrs = item_attrs.elem;
    proc_decl.vis = item_vis.elem;
    proc_decl.name = proc_name.elem;
    proc_decl.generic_params = gen_params.elem;
    proc_decl.where_clause = where_clause.elem;
    proc_decl.params = sig.params;
    proc_decl.return_type_opt = sig.return_type_opt;
    proc_decl.contract = contract.elem;
    proc_decl.span = SpanBetween(item_attrs.parser, contract.parser);

    items.push_back(proc_decl);
    next = contract.parser;

    // Consume terminator
    const Token* term = Tok(next);
    if (term && (term->kind == TokenKind::Newline ||
                 (term->kind == TokenKind::Punctuator && term->lexeme == ";"))) {
      Advance(next);
    }
  }

  // Expect closing brace
  if (!IsPunc(next, "}")) {
    EmitParseSyntaxErr(next, TokSpan(next));
  } else {
    Advance(next);
  }

  ExternBlock block;
  block.attrs = attrs.elem;
  block.vis = Visibility::Private;  // Extern blocks default to private visibility
  block.abi_opt = abi_opt;
  block.items = std::move(items);
  block.span = SpanBetween(start, next);
  return {next, block};
}

IMPLEMENTATION NOTES:
- "extern" is a contextual keyword (checked as identifier)
- ABI can be string literal ("C") or identifier (C)
- Block contains procedure declarations only
- Each proc can have attributes, visibility, generic params, where clause, contract
- No function body - these are foreign declarations
- Foreign contracts (|= @foreign_...) may be added later
*/

// =============================================================================
// TYPE DEFINITIONS USED
// =============================================================================

/*
STRUCT: ExternAbiString
struct ExternAbiString {
  Token literal;  // String literal token: "C"
};

STRUCT: ExternAbiIdent
struct ExternAbiIdent {
  std::string name;  // Identifier: C
};

VARIANT: ExternAbi
using ExternAbi = std::variant<ExternAbiString, ExternAbiIdent>;

STRUCT: ExternProcDecl
struct ExternProcDecl {
  AttributeList attrs;
  Visibility vis;
  Identifier name;
  std::optional<GenericParams> generic_params;
  std::optional<WhereClause> where_clause;
  std::vector<Param> params;
  std::shared_ptr<Type> return_type_opt;
  std::optional<ContractClause> contract;
  // std::vector<ForeignContractClause> foreign_contracts;  // Future
  core::Span span;
};

VARIANT: ExternItem
using ExternItem = std::variant<ExternProcDecl>;  // May expand later

STRUCT: ExternBlock
struct ExternBlock {
  AttributeList attrs;
  Visibility vis;
  std::optional<ExternAbi> abi_opt;
  std::vector<ExternItem> items;
  core::Span span;
};

RESULT TYPE:
ParseItemResult containing ExternBlock
*/

// =============================================================================
// GRAMMAR
// =============================================================================

/*
ExternBlock ::= 'extern' ExternAbi? '{' ExternItem* '}'
ExternAbi ::= STRING_LITERAL | IDENTIFIER
ExternItem ::= AttributeList? Visibility? 'procedure' IDENTIFIER
               GenericParams? Signature WhereClause? ContractClause?
               ForeignContractClause* Terminator
*/

// =============================================================================
// SUPPORTED ABI STRINGS
// =============================================================================

/*
| ABI String     | Meaning                        |
| -------------- | ------------------------------ |
| "C"            | C calling convention (default) |
| "C-unwind"     | C calling with unwinding       |
| "system"       | System calling convention      |
| "stdcall"      | Windows stdcall                |
| "fastcall"     | Windows fastcall               |
| "vectorcall"   | Vectorcall convention          |
*/

// =============================================================================
// FFISAFE TYPES
// =============================================================================

/*
Only FfiSafe types can appear in extern procedure signatures:

ALLOWED:
- Primitives: i8-i128, u8-u128, isize, usize, f16, f32, f64, char, ()
- Raw pointers: *imm T, *mut T
- Arrays of FfiSafe types
- Function pointers with FfiSafe signature
- Records/Enums with [[layout(C)]] and all FfiSafe fields

NOT ALLOWED:
- bool (use i8 instead)
- Modal types
- Safe pointers Ptr<T>
- Dynamic class objects $ClassName
- Tuples, Unions, Slices
- String and bytes types
- Context
*/

// =============================================================================
// EXAMPLES
// =============================================================================

/*
// Basic extern block
extern "C" {
    procedure malloc(size: usize) -> *mut u8
    procedure free(ptr: *mut u8) -> ()
}

// With foreign contracts
extern "C" {
    procedure read_file(path: *imm u8, buf: *mut u8, len: usize) -> i32
        |= @foreign_assumes(path != null, buf != null, len > 0)
        |= @foreign_ensures(@result >= 0)
        |= @foreign_ensures(@error: @result < 0)
}

// With identifier ABI
extern C {
    procedure puts(s: *imm i8) -> i32
}
*/

// =============================================================================
// IMPLEMENTATION CHECKLIST
// =============================================================================
// [ ] Integrate into ParseItem dispatch
//     - Check for "extern" identifier (not keyword!)
// [ ] ParseExternAbiOpt function
//     - Check for string literal or identifier
//     - Return nullopt if neither
// [ ] ParseExternItemList function
//     - Loop until closing brace
//     - Skip newlines
//     - Parse extern procedure declarations
// [ ] ParseExternProcDecl function
//     - Parse attributes
//     - Parse visibility
//     - Expect "procedure" keyword
//     - Parse name, generic params, signature
//     - Parse where clause, contract clause
//     - (Future: parse foreign contracts)
// [ ] Build ExternBlock
//     - Collect all items
//     - Set span

// =============================================================================
// PSEUDO-IMPLEMENTATION
// =============================================================================

/*
ParseItemResult ParseExternBlock(Parser parser, AttributeList attrs) {
  Parser start = parser;
  Advance(parser);  // consume 'extern'

  // Parse optional ABI
  auto abi_opt = ParseExternAbiOpt(parser);
  parser = abi_opt.parser;

  // Expect {
  ExpectPunc(parser, "{");

  // Parse items
  std::vector<ExternItem> items;
  while (!IsPunc(parser, "}") && !AtEof(parser)) {
    SkipNewlines(parser);
    if (IsPunc(parser, "}")) break;

    auto proc = ParseExternProcDecl(parser);
    items.push_back(proc.elem);
    parser = proc.parser;

    ConsumeTerminator(parser);
  }

  // Expect }
  ExpectPunc(parser, "}");

  ExternBlock block;
  block.attrs = attrs;
  block.vis = Visibility::Private;
  block.abi_opt = abi_opt.elem;
  block.items = std::move(items);
  block.span = SpanBetween(start, parser);

  return {parser, block};
}
*/
