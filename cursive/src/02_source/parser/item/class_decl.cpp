// =============================================================================
// MIGRATION MAPPING: Class Declaration Parsing
// =============================================================================
// Destination: cursive/src/02_source/parser/item/class_decl.cpp
// Source:      cursive-bootstrap/src/02_syntax/parser_items.cpp (lines 1496-1693, 2120-2142)
//
// PURPOSE:
// Parse class (trait) declarations:
// public class Comparable { procedure compare(~, other: const Self) -> i32 }
// Also handles "modal class" for modal class declarations.
// =============================================================================

// =============================================================================
// SPEC RULES IMPLEMENTED
// =============================================================================
// From C0updated.md Section 3.3.6.8:
//
// (Parse-Class)
// Γ ⊢ ParseAttrListOpt(P) ⇓ (P_0, attrs_opt)
// Γ ⊢ ParseVis(P_0) ⇓ (P_1, vis)
// Γ ⊢ ParseModalOpt(P_1) ⇓ (P_2, modal_opt)
// IsKw(Tok(P_2), `class`)
// Γ ⊢ ParseIdent(Advance(P_2)) ⇓ (P_3, name)
// Γ ⊢ ParseGenericParamsOpt(P_3) ⇓ (P_4, gen_params_opt)
// Γ ⊢ ParseSuperclassOpt(P_4) ⇓ (P_5, supers)
// Γ ⊢ ParseWhereClauseOpt(P_5) ⇓ (P_6, where_clause_opt)
// Γ ⊢ ParseClassBody(P_6) ⇓ (P_7, items)
// ────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseItem(P) ⇓ (P_7, ⟨ClassDecl, attrs_opt, vis, modal_opt, name,
//     gen_params_opt, where_clause_opt, supers, items, SpanBetween(P, P_7), []⟩)
//
// (Parse-ClassBody)
// IsPunc(Tok(P), "{")
// Γ ⊢ ParseClassItemList(Advance(P)) ⇓ (P_1, items)
// IsPunc(Tok(P_1), "}")
// ────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseClassBody(P) ⇓ (Advance(P_1), items)
//
// (Parse-ClassItem-Method)
// [Parses abstract or concrete method in class body]
//
// (Parse-ClassItem-AssociatedType)
// [Parses associated type declaration: type Item]
//
// (Parse-ClassItem-Field)
// [Parses field declaration in class]

// =============================================================================
// SOURCE CODE MAPPING
// =============================================================================

/*
SOURCE: cursive-bootstrap/src/02_syntax/parser_items.cpp (lines 1496-1564)
FUNCTION: ParseClassItem

ORIGINAL:
ParseElemResult<ClassItem> ParseClassItem(Parser parser) {
  ParseElemResult<Visibility> vis = ParseVis(parser);
  Parser cur = vis.parser;
  bool saw_override = false;
  if (IsKw(cur, "override")) {
    SPEC_RULE("Unsupported-Construct");
    EmitUnsupportedConstruct(cur);
    saw_override = true;
    Advance(cur);
  }

  if (IsKw(cur, "procedure")) {
    SPEC_RULE("Parse-ClassItem-Method");
    Parser start = cur;
    Advance(start);
    ParseElemResult<Identifier> name = ParseIdent(start);
    MethodSignatureResult sig = ParseMethodSignature(name.parser);
    std::shared_ptr<Block> body = nullptr;
    Parser after_sig = sig.parser;
    if (IsPunc(after_sig, "{")) {
      SPEC_RULE("Parse-ClassMethodBody-Concrete");
      ParseElemResult<std::shared_ptr<Block>> block = ParseBlock(after_sig);
      body = block.elem;
      after_sig = block.parser;
    } else {
      SPEC_RULE("Parse-ClassMethodBody-Abstract");
      ConsumeTerminatorReq(after_sig);
    }
    ClassMethodDecl method;
    method.vis = vis.elem;
    method.name = name.elem;
    method.receiver = sig.receiver;
    method.params = sig.params;
    method.return_type_opt = sig.return_type_opt;
    method.body_opt = body;
    method.span = SpanBetween(parser, after_sig);
    method.doc_opt = std::nullopt;
    return {after_sig, method};
  }

  // ... type and field handling
}

IMPLEMENTATION NOTES:
- Class methods can be abstract (no body) or concrete (with body)
- Abstract methods end with terminator (;/newline) instead of block
- "override" keyword is recognized but unsupported in C0
*/

/*
SOURCE: cursive-bootstrap/src/02_syntax/parser_items.cpp (lines 1596-1619)
FUNCTION: ParseClassBody

IMPLEMENTATION NOTES:
- Class body contains methods, associated types, and fields
- Items separated by newlines
*/

/*
SOURCE: cursive-bootstrap/src/02_syntax/parser_items.cpp (lines 1684-1693)
FUNCTION: ParseSuperclassOpt

ORIGINAL:
ParseElemResult<std::vector<ClassPath>> ParseSuperclassOpt(Parser parser) {
  if (!IsOp(parser, "<:")) {
    SPEC_RULE("Parse-Superclass-None");
    return {parser, {}};
  }
  SPEC_RULE("Parse-Superclass-Yes");
  Parser next = parser;
  Advance(next);
  return ParseSuperclassBounds(next);
}

IMPLEMENTATION NOTES:
- Superclasses use <: operator (same as implements)
- Multiple superclasses separated by + (plus operator)
*/

/*
SOURCE: cursive-bootstrap/src/02_syntax/parser_items.cpp (lines 2120-2142)
CONTEXT: Inside ParseItem function

ORIGINAL:
if (IsKw(cur, "class")) {
  SPEC_RULE("Parse-Class");
  Parser next = cur;
  Advance(next);
  ParseElemResult<Identifier> name = ParseIdent(next);
  ParseElemResult<std::optional<GenericParams>> gen_params = ParseGenericParamsOpt(name.parser);
  ParseElemResult<std::vector<ClassPath>> supers = ParseSuperclassOpt(gen_params.parser);
  ParseElemResult<std::optional<WhereClause>> where_clause = ParseWhereClauseOpt(supers.parser);
  ParseElemResult<std::vector<ClassItem>> items = ParseClassBody(where_clause.parser);
  ClassDecl decl;
  decl.vis = vis.elem;
  decl.modal = false;
  decl.name = name.elem;
  decl.generic_params = gen_params.elem;
  decl.supers = std::move(supers.elem);
  decl.where_clause = where_clause.elem;
  decl.items = std::move(items.elem);
  decl.span = SpanBetween(start, items.parser);
  decl.doc = {};
  return {items.parser, decl};
}
*/

/*
SOURCE: cursive-bootstrap/src/02_syntax/parser_items.cpp (lines 1753-1779)
CONTEXT: Modal class handling in ParseItem

IMPLEMENTATION NOTES:
- "modal class" creates a modal class (combines modal and class)
- Modal classes have state-specific behavior defined by class
*/

// =============================================================================
// TYPE DEFINITIONS USED
// =============================================================================

/*
STRUCT: ClassMethodDecl
struct ClassMethodDecl {
  Visibility vis;
  Identifier name;
  Receiver receiver;
  std::vector<Param> params;
  std::shared_ptr<Type> return_type_opt;
  std::shared_ptr<Block> body_opt;  // nullptr for abstract methods
  core::Span span;
  std::optional<std::string> doc_opt;
};

STRUCT: AssociatedTypeDecl
struct AssociatedTypeDecl {
  Visibility vis;
  Identifier name;
  std::shared_ptr<Type> type_opt;  // nullptr for abstract
  core::Span span;
};

STRUCT: ClassFieldDecl
struct ClassFieldDecl {
  Visibility vis;
  Identifier name;
  std::shared_ptr<Type> type;
  core::Span span;
  std::optional<std::string> doc_opt;
};

VARIANT: ClassItem
using ClassItem = std::variant<ClassMethodDecl, AssociatedTypeDecl, ClassFieldDecl>;

STRUCT: ClassDecl
struct ClassDecl {
  Visibility vis;
  bool modal;  // true for "modal class"
  Identifier name;
  std::optional<GenericParams> generic_params;
  std::vector<ClassPath> supers;  // Superclasses via <:
  std::optional<WhereClause> where_clause;
  std::vector<ClassItem> items;
  core::Span span;
  std::optional<std::string> doc;
};
*/

// =============================================================================
// GRAMMAR
// =============================================================================

/*
ClassDecl ::= AttributeList? Visibility? 'modal'? 'class' IDENTIFIER
              GenericParams? Superclass? WhereClause?
              ClassBody

Superclass ::= '<:' ClassBound ('+' ClassBound)*
ClassBody ::= '{' ClassItem* '}'

ClassItem ::= ClassMethod | AssociatedType | ClassField

ClassMethod ::= Visibility? 'procedure' IDENTIFIER MethodSignature MethodBody
MethodBody ::= Block | Terminator

AssociatedType ::= Visibility? 'type' IDENTIFIER ('=' Type)? Terminator

ClassField ::= Visibility? KeyBoundary? IDENTIFIER ':' Type Terminator
*/

// =============================================================================
// EXAMPLES
// =============================================================================

/*
// Simple class (interface)
public class Comparable {
    procedure compare(~, other: const Self) -> i32
}

// Class with default implementation
public class Printable {
    procedure print(~) -> string@Managed
}

// Class with associated type
public class Container {
    type Item                    // Abstract associated type
    procedure get(~) -> Self::Item
}

// Class extending other classes
public class Ordered <: Comparable + Eq {
    procedure less_than(~, other: const Self) -> bool {
        return self~>compare(other) < 0
    }
}

// Record implementing classes
public record MyType <: Comparable, Printable {
    value: i32,

    override procedure compare(~, other: const Self) -> i32 {
        return self.value - other.value
    }

    override procedure print(~) -> string@Managed {
        // ... implementation
    }
}

// Modal class
public modal class Lifecycle {
    type State
    procedure current_state(~) -> Self::State
}
*/

// =============================================================================
// IMPLEMENTATION CHECKLIST
// =============================================================================
// [ ] Integrate into ParseItem dispatch
//     - Check for optional "modal" keyword
//     - Check for "class" keyword
// [ ] ParseClassItem function
//     - Dispatch to method, associated type, or field
// [ ] ParseClassMethod function
//     - Parse method signature
//     - Handle abstract (no body) vs concrete (with body)
// [ ] ParseAssociatedType function
//     - Parse "type" name
//     - Parse optional = Type
// [ ] ParseClassField function
//     - Parse name : Type
// [ ] ParseSuperclassOpt function
//     - Check for <: operator
//     - Parse class bounds separated by +
// [ ] ParseClassBody function
//     - Expect {, parse items, expect }
// [ ] Build ClassDecl with all components

// =============================================================================
// PSEUDO-IMPLEMENTATION
// =============================================================================

/*
ParseItemResult ParseClassDecl(Parser parser, Visibility vis, bool is_modal) {
  Parser start = parser;
  Advance(parser);  // consume 'class'

  auto name = ParseIdent(parser);
  parser = name.parser;

  auto gen_params = ParseGenericParamsOpt(parser);
  parser = gen_params.parser;

  auto supers = ParseSuperclassOpt(parser);
  parser = supers.parser;

  auto where_clause = ParseWhereClauseOpt(parser);
  parser = where_clause.parser;

  auto items = ParseClassBody(parser);
  parser = items.parser;

  ClassDecl decl;
  decl.vis = vis;
  decl.modal = is_modal;
  decl.name = name.elem;
  decl.generic_params = gen_params.elem;
  decl.supers = supers.elem;
  decl.where_clause = where_clause.elem;
  decl.items = items.elem;
  decl.span = SpanBetween(start, parser);

  return {parser, decl};
}

ParseElemResult<ClassItem> ParseClassItem(Parser parser) {
  auto vis = ParseVis(parser);
  parser = vis.parser;

  if (IsKw(parser, "procedure")) {
    return ParseClassMethod(parser, vis.elem);
  }
  if (IsKw(parser, "type")) {
    return ParseAssociatedType(parser, vis.elem);
  }
  return ParseClassField(parser, vis.elem);
}
*/
