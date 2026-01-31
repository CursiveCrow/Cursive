// =============================================================================
// MIGRATION MAPPING: Static Declaration Parsing
// =============================================================================
// Destination: cursive/src/02_source/parser/item/static_decl.cpp
// Source:      cursive-bootstrap/src/02_syntax/parser_items.cpp (lines 1966-1977)
//
// PURPOSE:
// Parse module-level static declarations:
// public let MAX_SIZE: usize = 1024
// public var counter: i32 = 0
// =============================================================================

// =============================================================================
// SPEC RULES IMPLEMENTED
// =============================================================================
// From C0updated.md Section 3.3.6.4:
//
// (Parse-Static-Decl)
// Γ ⊢ ParseAttrListOpt(P) ⇓ (P_0, attrs_opt)
// Γ ⊢ ParseVis(P_0) ⇓ (P_1, vis)
// Tok(P_1) = Keyword(kw)    kw ∈ {`let`, `var`}    mut = kw
// Γ ⊢ ParseBindingAfterLetVar(P_1) ⇓ (P_2, bind)
// ────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseItem(P) ⇓ (P_2, ⟨StaticDecl, attrs_opt, vis, mut, bind,
//                         SpanBetween(P, P_2), []⟩)

// =============================================================================
// SOURCE CODE MAPPING
// =============================================================================

/*
SOURCE: cursive-bootstrap/src/02_syntax/parser_items.cpp (lines 1966-1977)
CONTEXT: Inside ParseItem function

ORIGINAL:
if (IsKw(cur, "let") || IsKw(cur, "var")) {
  SPEC_RULE("Parse-Static-Decl");
  Mutability mut = IsKw(cur, "let") ? Mutability::Let : Mutability::Var;
  ParseElemResult<Binding> binding = ParseBindingAfterLetVar(cur);
  StaticDecl decl;
  decl.vis = vis.elem;
  decl.mut = mut;
  decl.binding = binding.elem;
  decl.span = SpanBetween(start, binding.parser);
  decl.doc = {};
  return {binding.parser, decl};
}

IMPLEMENTATION NOTES:
- Static declarations are module-level bindings
- let = immutable, var = mutable
- Uses same binding parsing as local let/var statements
- Must have initializer expression
*/

/*
SOURCE: cursive-bootstrap/src/02_syntax/parser_items.cpp (lines 779-804)
FUNCTION: ParseBindingAfterLetVarImpl

ORIGINAL:
ParseElemResult<Binding> ParseBindingAfterLetVarImpl(Parser parser) {
  SPEC_RULE("Parse-BindingAfterLetVar");
  Parser start = parser;
  Parser after_kw = parser;
  Advance(after_kw);
  ParseElemResult<std::shared_ptr<Pattern>> pat = ParsePattern(after_kw);
  ParseElemResult<std::shared_ptr<Type>> ty = ParseTypeAnnotOpt(pat.parser);
  NormalizeBindingPattern(pat.elem, ty.elem);
  const Token* tok = Tok(ty.parser);
  Token op;
  if (tok && tok->kind == TokenKind::Operator &&
      (tok->lexeme == "=" || tok->lexeme == ":=")) {
    op = *tok;
    Advance(ty.parser);
  } else {
    EmitParseSyntaxErr(ty.parser, TokSpan(ty.parser));
  }
  ParseElemResult<std::shared_ptr<Expr>> init = ParseExpr(ty.parser);
  Binding binding;
  binding.pat = pat.elem;
  binding.type_opt = ty.elem;
  binding.op = op;
  binding.init = init.elem;
  binding.span = SpanBetween(start, init.parser);
  return {init.parser, binding};
}

IMPLEMENTATION NOTES:
- Parses: let/var pattern [: Type] (= | :=) expr
- Pattern is typically identifier but can be destructuring
- Type annotation is optional
- Binding operator: = (movable) or := (immovable)
- NormalizeBindingPattern handles "name: Type" patterns
*/

// =============================================================================
// TYPE DEFINITIONS USED
// =============================================================================

/*
ENUM: Mutability
enum class Mutability {
  Let,  // Immutable
  Var   // Mutable
};

STRUCT: Binding
struct Binding {
  std::shared_ptr<Pattern> pat;
  std::shared_ptr<Type> type_opt;  // nullable
  Token op;                        // = or :=
  std::shared_ptr<Expr> init;
  core::Span span;
};

STRUCT: StaticDecl
struct StaticDecl {
  AttributeList attrs;  // C0X extension
  Visibility vis;
  Mutability mut;
  Binding binding;
  core::Span span;
  std::optional<std::string> doc;
};

RESULT TYPE:
ParseItemResult containing StaticDecl
*/

// =============================================================================
// GRAMMAR
// =============================================================================

/*
StaticDecl ::= AttributeList? Visibility? ('let' | 'var') Binding
Binding ::= Pattern TypeAnnotation? BindingOp Expr
TypeAnnotation ::= ':' Type
BindingOp ::= '=' | ':='
*/

// =============================================================================
// BINDING OPERATORS
// =============================================================================

/*
| Syntax       | Mutability | Movability |
| ------------ | ---------- | ---------- |
| let x = v    | Immutable  | Movable    |
| let x := v   | Immutable  | Immovable  |
| var x = v    | Mutable    | Movable    |
| var x := v   | Mutable    | Immovable  |

Movability affects whether the binding can be moved from:
- Movable (=): Value can be moved to another binding
- Immovable (:=): Value cannot be moved, stays in place
*/

// =============================================================================
// EXAMPLES
// =============================================================================

/*
// Immutable static
public let MAX_SIZE: usize = 1024

// Mutable static
public var counter: i32 = 0

// With destructuring pattern
public let (x, y): (i32, i32) = (10, 20)

// Immovable binding
public let config := load_config()
*/

// =============================================================================
// IMPLEMENTATION CHECKLIST
// =============================================================================
// [ ] Integrate into ParseItem dispatch
//     - Check for "let" or "var" keyword after visibility
// [ ] Determine mutability
//     - let -> Mutability::Let
//     - var -> Mutability::Var
// [ ] Call ParseBindingAfterLetVar
//     - This is a shared function used by both static and local bindings
// [ ] Build StaticDecl
//     - Set visibility, mutability, binding
//     - Set span

// =============================================================================
// PSEUDO-IMPLEMENTATION
// =============================================================================

/*
ParseItemResult ParseStaticDecl(Parser parser, AttributeList attrs, Visibility vis) {
  Parser start = parser;

  Mutability mut;
  if (IsKw(parser, "let")) {
    mut = Mutability::Let;
  } else {
    mut = Mutability::Var;
  }

  auto binding = ParseBindingAfterLetVar(parser);
  parser = binding.parser;

  StaticDecl decl;
  decl.attrs = attrs;
  decl.vis = vis;
  decl.mut = mut;
  decl.binding = binding.elem;
  decl.span = SpanBetween(start, parser);

  return {parser, decl};
}

// ParseBindingAfterLetVar is in a shared location (possibly binding.cpp)
ParseElemResult<Binding> ParseBindingAfterLetVar(Parser parser) {
  Parser start = parser;
  Advance(parser);  // consume let/var

  auto pat = ParsePattern(parser);
  parser = pat.parser;

  auto ty = ParseTypeAnnotOpt(parser);
  parser = ty.parser;

  NormalizeBindingPattern(pat.elem, ty.elem);

  // Expect = or :=
  Token op;
  if (IsOp(parser, "=") || IsOp(parser, ":=")) {
    op = *Tok(parser);
    Advance(parser);
  } else {
    EmitError(parser);
  }

  auto init = ParseExpr(parser);
  parser = init.parser;

  Binding binding;
  binding.pat = pat.elem;
  binding.type_opt = ty.elem;
  binding.op = op;
  binding.init = init.elem;
  binding.span = SpanBetween(start, parser);

  return {parser, binding};
}
*/
