// =============================================================================
// MIGRATION MAPPING: Modal Declaration Parsing
// =============================================================================
// Destination: cursive/src/02_source/parser/item/modal_decl.cpp
// Source:      cursive-bootstrap/src/02_syntax/parser_items.cpp (lines 1290-1494, 2094-2118)
//
// PURPOSE:
// Parse modal type declarations with state blocks:
// public modal Connection { @Disconnected { ... } @Connected { ... } }
// =============================================================================

// =============================================================================
// SPEC RULES IMPLEMENTED
// =============================================================================
// From C0updated.md Section 3.3.6.8:
//
// (Parse-Modal)
// Γ ⊢ ParseAttrListOpt(P) ⇓ (P_0, attrs_opt)
// Γ ⊢ ParseVis(P_0) ⇓ (P_1, vis)
// IsKw(Tok(P_1), `modal`)
// Γ ⊢ ParseIdent(Advance(P_1)) ⇓ (P_2, name)
// Γ ⊢ ParseGenericParamsOpt(P_2) ⇓ (P_3, gen_params_opt)
// Γ ⊢ ParseImplementsOpt(P_3) ⇓ (P_4, impls)
// Γ ⊢ ParseWhereClauseOpt(P_4) ⇓ (P_5, where_clause_opt)
// Γ ⊢ ParseModalBody(P_5) ⇓ (P_6, states)
// Γ ⊢ ParseInvariantOpt(P_6) ⇓ (P_7, invariant_opt)
// ────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseItem(P) ⇓ (P_7, ⟨ModalDecl, attrs_opt, vis, name,
//     gen_params_opt, where_clause_opt, impls, states, invariant_opt,
//     SpanBetween(P, P_7), []⟩)
//
// (Parse-ModalBody)
// IsPunc(Tok(P), "{")
// Γ ⊢ ParseStateBlockList(Advance(P)) ⇓ (P_1, states)
// IsPunc(Tok(P_1), "}")
// ────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseModalBody(P) ⇓ (Advance(P_1), states)
//
// (Parse-StateBlock)
// IsOp(Tok(P), "@")
// Γ ⊢ ParseIdent(Advance(P)) ⇓ (P_1, name)
// IsPunc(Tok(P_1), "{")
// Γ ⊢ ParseStateMemberList(Advance(P_1)) ⇓ (P_2, members)
// IsPunc(Tok(P_2), "}")
// ────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseStateBlock(P) ⇓ (Advance(P_2), ⟨name, members⟩)

// =============================================================================
// SOURCE CODE MAPPING
// =============================================================================

/*
SOURCE: cursive-bootstrap/src/02_syntax/parser_items.cpp (lines 1290-1370)
FUNCTION: ParseStateMember

ORIGINAL:
ParseElemResult<StateMember> ParseStateMember(Parser parser) {
  ParseElemResult<Visibility> vis = ParseVis(parser);
  if (IsKw(vis.parser, "procedure") || IsKw(vis.parser, "override")) {
    SPEC_RULE("Parse-StateMember-Method");
    // ... parse method
  }
  if (IsKw(vis.parser, "transition")) {
    SPEC_RULE("Parse-StateMember-Transition");
    // ... parse transition
  }
  SPEC_RULE("Parse-StateMember-Field");
  // ... parse field
}

IMPLEMENTATION NOTES:
- State members can be:
  1. Fields: name: Type
  2. Methods: procedure name(~) -> Type { ... }
  3. Transitions: transition connect() -> @Connected { ... }
*/

/*
SOURCE: cursive-bootstrap/src/02_syntax/parser_items.cpp (lines 1309-1348)
CONTEXT: Inside ParseStateMember, transition parsing

IMPLEMENTATION NOTES:
- Transitions change state: transition name(params) -> @TargetState { ... }
- Target state is prefixed with @
- Transition has body that returns value in target state
*/

/*
SOURCE: cursive-bootstrap/src/02_syntax/parser_items.cpp (lines 1411-1443)
FUNCTION: ParseStateBlock

ORIGINAL:
ParseElemResult<StateBlock> ParseStateBlock(Parser parser) {
  SPEC_RULE("Parse-StateBlock");
  Parser start = parser;
  if (!IsOp(parser, "@")) {
    EmitParseSyntaxErr(parser, TokSpan(parser));
    // ... error recovery
  }
  Parser next = parser;
  Advance(next);
  ParseElemResult<Identifier> name = ParseIdent(next);
  if (!IsPunc(name.parser, "{")) {
    EmitParseSyntaxErr(name.parser, TokSpan(name.parser));
  } else {
    Advance(name.parser);
  }
  ParseElemResult<std::vector<StateMember>> members = ParseStateMemberList(name.parser);
  if (!IsPunc(members.parser, "}")) {
    EmitParseSyntaxErr(members.parser, TokSpan(members.parser));
  } else {
    Advance(members.parser);
  }
  StateBlock blk;
  blk.name = name.elem;
  blk.members = std::move(members.elem);
  blk.span = SpanBetween(start, members.parser);
  blk.doc_opt = std::nullopt;
  return {members.parser, blk};
}

IMPLEMENTATION NOTES:
- State blocks start with @StateName { ... }
- Each state has its own fields, methods, and transitions
- Fields in a state block are only accessible when modal is in that state
*/

/*
SOURCE: cursive-bootstrap/src/02_syntax/parser_items.cpp (lines 1475-1494)
FUNCTION: ParseModalBody

ORIGINAL:
ParseElemResult<std::vector<StateBlock>> ParseModalBody(Parser parser) {
  SPEC_RULE("Parse-ModalBody");
  // Skip newlines before opening brace
  while (Tok(parser) && Tok(parser)->kind == TokenKind::Newline) {
    Advance(parser);
  }
  if (!IsPunc(parser, "{")) {
    EmitParseSyntaxErr(parser, TokSpan(parser));
    return {parser, {}};
  }
  Parser next = parser;
  Advance(next);
  ParseElemResult<std::vector<StateBlock>> blocks = ParseStateBlockList(next);
  if (!IsPunc(blocks.parser, "}")) {
    EmitParseSyntaxErr(blocks.parser, TokSpan(blocks.parser));
    return {blocks.parser, blocks.elem};
  }
  Advance(blocks.parser);
  return {blocks.parser, blocks.elem};
}
*/

/*
SOURCE: cursive-bootstrap/src/02_syntax/parser_items.cpp (lines 2094-2118)
CONTEXT: Inside ParseItem function

ORIGINAL:
if (IsKw(cur, "modal")) {
  SPEC_RULE("Parse-Modal");
  Parser next = cur;
  Advance(next);
  ParseElemResult<Identifier> name = ParseIdent(next);
  ParseElemResult<std::optional<GenericParams>> gen_params = ParseGenericParamsOpt(name.parser);
  ParseElemResult<std::vector<ClassPath>> impls = ParseImplementsOpt(gen_params.parser);
  ParseElemResult<std::optional<WhereClause>> where_clause = ParseWhereClauseOpt(impls.parser);
  ParseElemResult<std::vector<StateBlock>> states = ParseModalBody(where_clause.parser);
  ParseElemResult<std::optional<TypeInvariant>> invariant = ParseInvariantOpt(states.parser);
  ModalDecl decl;
  decl.vis = vis.elem;
  decl.name = name.elem;
  decl.generic_params = gen_params.elem;
  decl.implements = std::move(impls.elem);
  decl.where_clause = where_clause.elem;
  decl.invariant = invariant.elem;
  decl.states = std::move(states.elem);
  decl.span = SpanBetween(start, invariant.parser);
  decl.doc = {};
  return {invariant.parser, decl};
}
*/

// =============================================================================
// TYPE DEFINITIONS USED
// =============================================================================

/*
STRUCT: StateFieldDecl
struct StateFieldDecl {
  Visibility vis;
  Identifier name;
  std::shared_ptr<Type> type;
  core::Span span;
  std::optional<std::string> doc_opt;
};

STRUCT: StateMethodDecl
struct StateMethodDecl {
  Visibility vis;
  Identifier name;
  std::vector<Param> params;
  std::shared_ptr<Type> return_type_opt;
  std::shared_ptr<Block> body;
  core::Span span;
  std::optional<std::string> doc_opt;
};

STRUCT: TransitionDecl
struct TransitionDecl {
  Visibility vis;
  Identifier name;
  std::vector<Param> params;
  Identifier target_state;  // The @TargetState
  std::shared_ptr<Block> body;
  core::Span span;
  std::optional<std::string> doc_opt;
};

VARIANT: StateMember
using StateMember = std::variant<StateFieldDecl, StateMethodDecl, TransitionDecl>;

STRUCT: StateBlock
struct StateBlock {
  Identifier name;  // State name (without @)
  std::vector<StateMember> members;
  core::Span span;
  std::optional<std::string> doc_opt;
};

STRUCT: ModalDecl
struct ModalDecl {
  Visibility vis;
  Identifier name;
  std::optional<GenericParams> generic_params;
  std::vector<ClassPath> implements;
  std::optional<WhereClause> where_clause;
  std::optional<TypeInvariant> invariant;
  std::vector<StateBlock> states;
  core::Span span;
  std::optional<std::string> doc;
};
*/

// =============================================================================
// GRAMMAR
// =============================================================================

/*
ModalDecl ::= AttributeList? Visibility? 'modal' IDENTIFIER
              GenericParams? Implements? WhereClause?
              ModalBody InvariantOpt

ModalBody ::= '{' StateBlock* '}'

StateBlock ::= '@' IDENTIFIER '{' StateMember* '}'

StateMember ::= StateField | StateMethod | Transition

StateField ::= Visibility? IDENTIFIER ':' Type Terminator

StateMethod ::= Visibility? 'procedure' IDENTIFIER Signature Block

Transition ::= Visibility? 'transition' IDENTIFIER '(' ParamList ')' '->' '@' IDENTIFIER Block
*/

// =============================================================================
// BUILT-IN MODAL TYPES
// =============================================================================

/*
The following modal types are built into Cursive:

Region: Memory arena lifecycle
  States: @Active, @Frozen, @Freed

CancelToken: Cooperative cancellation
  States: @Active, @Cancelled

Spawned<T>: Parallel task result
  States: @Pending, @Ready

Tracked<T, E>: Tracked async result
  States: @Pending, @Ready

Async<Out, In, Result, E>: Async state machine
  States: @Suspended, @Completed, @Failed
*/

// =============================================================================
// EXAMPLES
// =============================================================================

/*
public modal Connection {
    @Disconnected {
        host: string@View

        transition connect(port: i32) -> @Connected {
            // ... connection logic
            return Connection@Connected{ socket: ... }
        }
    }

    @Connected {
        socket: i32

        procedure send(~!, data: bytes@View) -> i32 {
            // ... send logic
            return bytes_sent
        }

        transition disconnect() -> @Disconnected {
            // ... cleanup
            return Connection@Disconnected{ host: self.host }
        }
    }
}

// Using modal types
let conn: Connection@Disconnected = Connection@Disconnected{ host: "example.com" }
let connected: Connection@Connected = conn~>connect(8080)
let general: Connection = widen connected  // Widening to general type
*/

// =============================================================================
// IMPLEMENTATION CHECKLIST
// =============================================================================
// [ ] Integrate into ParseItem dispatch
//     - Check for "modal" keyword after visibility
//     - Also check for "modal class" (modal class is different!)
// [ ] ParseStateMember function
//     - Dispatch to field, method, or transition
// [ ] ParseTransition function
//     - Parse "transition" name(params) -> @State { body }
// [ ] ParseStateBlock function
//     - Parse @StateName { members }
// [ ] ParseStateBlockList function
//     - Parse multiple state blocks
// [ ] ParseModalBody function
//     - Expect {, parse state blocks, expect }
// [ ] Build ModalDecl with all components

// =============================================================================
// PSEUDO-IMPLEMENTATION
// =============================================================================

/*
ParseItemResult ParseModalDecl(Parser parser, Visibility vis) {
  Parser start = parser;
  Advance(parser);  // consume 'modal'

  auto name = ParseIdent(parser);
  parser = name.parser;

  auto gen_params = ParseGenericParamsOpt(parser);
  parser = gen_params.parser;

  auto impls = ParseImplementsOpt(parser);
  parser = impls.parser;

  auto where_clause = ParseWhereClauseOpt(parser);
  parser = where_clause.parser;

  auto states = ParseModalBody(parser);
  parser = states.parser;

  auto invariant = ParseInvariantOpt(parser);
  parser = invariant.parser;

  ModalDecl decl;
  decl.vis = vis;
  decl.name = name.elem;
  decl.generic_params = gen_params.elem;
  decl.implements = impls.elem;
  decl.where_clause = where_clause.elem;
  decl.states = states.elem;
  decl.invariant = invariant.elem;
  decl.span = SpanBetween(start, parser);

  return {parser, decl};
}

ParseElemResult<StateBlock> ParseStateBlock(Parser parser) {
  Parser start = parser;
  Advance(parser);  // consume @

  auto name = ParseIdent(parser);
  parser = name.parser;

  ExpectPunc(parser, "{");

  auto members = ParseStateMemberList(parser);
  parser = members.parser;

  ExpectPunc(parser, "}");

  StateBlock block;
  block.name = name.elem;
  block.members = members.elem;
  block.span = SpanBetween(start, parser);

  return {parser, block};
}
*/
