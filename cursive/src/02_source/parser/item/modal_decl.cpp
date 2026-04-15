// =============================================================================
// modal_decl.cpp - Modal Declaration Parsing
// =============================================================================
//
// SPEC REFERENCE: CursiveSpecification.md Section 3.3.6.8 (Modal Declaration Rules)
//
// This file implements modal type declaration parsing:
//   - ParseStateMember: Parse field, method, or transition in state block
//   - ParseStateMemberList: Parse list of state members
//   - ParseStateBlock: Parse @StateName { members }
//   - ParseStateBlockList: Parse multiple state blocks
//   - ParseModalBody: Parse modal body in braces
//   - ParseModalDecl: Parse complete modal declaration
//
// SYNTAX:
//   public modal Connection {
//       @Disconnected { host: string@View, transition connect() -> @Connected { ... } }
//       @Connected { socket: i32, procedure send(~!) -> i32 { ... } }
//   }
//
// =============================================================================

#include "02_source/parser/parser.h"

#include <memory>
#include <optional>
#include <vector>

#include "00_core/assert_spec.h"

namespace cursive::ast {

// Use lexer types
using cursive::lexer::Token;
using cursive::lexer::TokenKind;

// Forward declarations for helper functions
bool IsKw(const Parser& parser, std::string_view kw);
bool IsOp(const Parser& parser, std::string_view op);
bool IsPunc(const Parser& parser, std::string_view p);
void SkipNewlines(Parser& parser);

// Forward declarations for type and expression parsing
ParseElemResult<std::shared_ptr<Type>> ParseType(Parser parser);
ParseElemResult<std::shared_ptr<Block>> ParseBlock(Parser parser);

// Forward declarations for generic params and where clause parsing
ParseElemResult<std::optional<GenericParams>> ParseGenericParamsOpt(
    Parser parser);
ParseElemResult<std::optional<WhereClause>> ParsePredicateClauseOpt(
    Parser parser);
ParseElemResult<AttrOpt> ParseAttributeListOpt(Parser parser);
ParseElemResult<std::optional<ContractClause>> ParseContractClauseOpt(
    Parser parser);

// Forward declarations from record_decl.cpp
ParseElemResult<std::vector<ClassPath>> ParseImplementsOpt(Parser parser);
ParseElemResult<std::optional<TypeInvariant>> ParseInvariantOpt(Parser parser);

// Forward declaration from signature.cpp
struct SignatureResult {
  Parser parser;
  std::vector<Param> params;
  TypePtr return_type_opt;
};

ParseElemResult<std::vector<Param>> ParseParamList(Parser parser);
SignatureResult ParseSignature(Parser parser);
struct MethodSignatureResult {
  Parser parser;
  std::optional<Receiver> receiver;
  std::vector<Param> params;
  TypePtr return_type_opt;
};
MethodSignatureResult ParseStateMethodSignature(Parser parser);

// =============================================================================
// ParseStateMember - Parse field, method, or transition in state block
// =============================================================================

ParseElemResult<StateMember> ParseStateMember(Parser parser) {
  ParseElemResult<AttrOpt> attrs = ParseAttributeListOpt(parser);
  ParseElemResult<Visibility> vis = ParseVis(attrs.parser);
  Parser cur = vis.parser;
  AttributeList attrs_list = attrs.elem.value_or(AttributeList{});

  // Check for method (procedure keyword)
  if (IsKw(cur, "procedure")) {
    SPEC_RULE("Parse-StateMember-Method");
    Parser start = cur;
    Advance(start);  // consume 'procedure'

    ParseElemResult<Identifier> name = ParseIdent(start);
    ParseElemResult<std::optional<GenericParams>> gen_params =
        ParseGenericParamsOpt(name.parser);
    MethodSignatureResult sig = ParseStateMethodSignature(gen_params.parser);
    ParseElemResult<std::optional<ContractClause>> contract =
        ParseContractClauseOpt(sig.parser);
    ParseElemResult<std::shared_ptr<Block>> body =
        ParseBlock(contract.parser);

    StateMethodDecl method;
    method.attrs = attrs_list;
    method.vis = vis.elem;
    method.name = name.elem;
    method.generic_params = gen_params.elem;
    method.receiver =
        sig.receiver.value_or(ReceiverShorthand{ReceiverPerm::Const});
    method.params = sig.params;
    method.return_type_opt = sig.return_type_opt;
    method.contract = contract.elem;
    method.body = body.elem;
    method.span = SpanBetween(parser, body.parser);
    method.doc_opt = std::nullopt;

    return {body.parser, method};
  }

  // Check for transition
  if (IsKw(cur, "transition")) {
    SPEC_RULE("Parse-StateMember-Transition");
    Parser start = cur;
    Advance(start);  // consume 'transition'

    ParseElemResult<Identifier> name = ParseIdent(start);
    Parser cur = name.parser;

    // Expect (
    if (!IsPunc(cur, "(")) {
      EmitParseSyntaxErr(cur, TokSpan(cur));
    } else {
      Advance(cur);
    }

    // Parse parameters
    ParseElemResult<std::vector<Param>> params = ParseParamList(cur);
    cur = params.parser;
    if (params.elem.empty()) {
      EmitParseSyntaxErr(cur, TokSpan(cur));
    }

    // Expect )
    if (!IsPunc(cur, ")")) {
      EmitParseSyntaxErr(cur, TokSpan(cur));
    } else {
      Advance(cur);
    }

    // Expect ->
    if (!IsOp(cur, "->")) {
      EmitParseSyntaxErr(cur, TokSpan(cur));
    } else {
      Advance(cur);
    }

    // Expect @
    if (!IsOp(cur, "@")) {
      EmitParseSyntaxErr(cur, TokSpan(cur));
    } else {
      Advance(cur);
    }

    // Parse target state name
    ParseElemResult<Identifier> target = ParseIdent(cur);

    // Parse body
    ParseElemResult<std::shared_ptr<Block>> body = ParseBlock(target.parser);

    TransitionDecl trans;
    trans.attrs = attrs_list;
    trans.vis = vis.elem;
    trans.name = name.elem;
    trans.params = params.elem;
    trans.target_state = target.elem;
    trans.body = body.elem;
    trans.span = SpanBetween(parser, body.parser);
    trans.doc_opt = std::nullopt;

    return {body.parser, trans};
  }

  // Default: parse field
  SPEC_RULE("Parse-StateMember-Field");

  ParseElemResult<bool> boundary = ParseKeyBoundaryOpt(cur);
  ParseElemResult<Identifier> name = ParseIdent(boundary.parser);

  if (!IsPunc(name.parser, ":")) {
    EmitParseSyntaxErr(name.parser, TokSpan(name.parser));
  } else {
    Advance(name.parser);
  }

  ParseElemResult<std::shared_ptr<Type>> ty = ParseType(name.parser);
  Parser after_type = ty.parser;

  // Consume optional terminator (semicolon or newline)
  const Token* tok = Tok(after_type);
  if (tok && (tok->kind == TokenKind::Newline ||
              (tok->kind == TokenKind::Punctuator && tok->lexeme == ";"))) {
    Advance(after_type);
  }

  StateFieldDecl field;
  field.attrs = attrs_list;
  field.vis = vis.elem;
  field.key_boundary = boundary.elem;
  field.name = name.elem;
  field.type = ty.elem;
  field.span = SpanBetween(parser, after_type);
  field.doc_opt = std::nullopt;

  return {after_type, field};
}

// =============================================================================
// ParseStateMemberList - Parse list of state members
// =============================================================================

ParseElemResult<std::vector<StateMember>> ParseStateMemberList(Parser parser) {
  // Skip leading newlines
  while (Tok(parser) && Tok(parser)->kind == TokenKind::Newline) {
    Advance(parser);
  }

  if (IsPunc(parser, "}")) {
    SPEC_RULE("Parse-StateMemberList-End");
    return {parser, {}};
  }

  SPEC_RULE("Parse-StateMemberList-Cons");
  std::vector<StateMember> members;
  Parser cur = parser;

  while (!IsPunc(cur, "}")) {
    // Skip newlines between members
    while (Tok(cur) && Tok(cur)->kind == TokenKind::Newline) {
      Advance(cur);
    }
    if (IsPunc(cur, "}")) break;

    if (IsPunc(cur, ",")) {
      EmitParseSyntaxErr(cur, TokSpan(cur));
      Advance(cur);
      continue;
    }

    Parser before = cur;
    ParseElemResult<StateMember> mem = ParseStateMember(cur);
    members.push_back(mem.elem);
    cur = mem.parser;

    // Skip newlines after member
    while (Tok(cur) && Tok(cur)->kind == TokenKind::Newline) {
      Advance(cur);
    }

    // Prevent infinite loop
    if (cur.tokens == before.tokens && cur.index == before.index) {
      EmitParseSyntaxErr(cur, TokSpan(cur));
      cur = AdvanceOrEOF(cur);
    }
  }

  return {cur, members};
}

// =============================================================================
// ParseStateBlock - Parse @StateName { members }
// =============================================================================

ParseElemResult<StateBlock> ParseStateBlock(Parser parser) {
  SPEC_RULE("Parse-StateBlock");
  Parser start = parser;

  if (!IsOp(parser, "@")) {
    EmitParseSyntaxErr(parser, TokSpan(parser));
    Parser next = AdvanceOrEOF(parser);
    StateBlock blk;
    blk.name = "_";
    blk.span = SpanBetween(start, next);
    blk.doc_opt = std::nullopt;
    return {next, blk};
  }

  Parser next = parser;
  Advance(next);  // consume @

  // Parse state name
  ParseElemResult<Identifier> name = ParseIdent(next);

  // Expect {
  if (!IsPunc(name.parser, "{")) {
    EmitParseSyntaxErr(name.parser, TokSpan(name.parser));
  } else {
    Advance(name.parser);
  }

  // Parse members
  ParseElemResult<std::vector<StateMember>> members =
      ParseStateMemberList(name.parser);

  // Expect }
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

// =============================================================================
// ParseStateBlockList - Parse multiple state blocks
// =============================================================================

ParseElemResult<std::vector<StateBlock>> ParseStateBlockList(Parser parser) {
  // Skip leading newlines
  while (Tok(parser) && Tok(parser)->kind == TokenKind::Newline) {
    Advance(parser);
  }

  if (IsPunc(parser, "}")) {
    SPEC_RULE("Parse-StateBlockList-Empty");
    return {parser, {}};
  }

  SPEC_RULE("Parse-StateBlockList-Cons");
  ParseElemResult<StateBlock> block = ParseStateBlock(parser);
  std::vector<StateBlock> blocks;
  blocks.push_back(block.elem);
  Parser cur = block.parser;

  // Skip newlines between state blocks
  while (Tok(cur) && Tok(cur)->kind == TokenKind::Newline) {
    Advance(cur);
  }

  while (!IsPunc(cur, "}")) {
    ParseElemResult<StateBlock> next_blk = ParseStateBlock(cur);
    blocks.push_back(next_blk.elem);
    cur = next_blk.parser;

    // Skip newlines after state block
    while (Tok(cur) && Tok(cur)->kind == TokenKind::Newline) {
      Advance(cur);
    }
  }

  return {cur, blocks};
}

// =============================================================================
// ParseModalBody - Parse modal body in braces
// =============================================================================

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
  if (blocks.elem.empty()) {
    EmitParseSyntaxErr(blocks.parser, TokSpan(blocks.parser));
  }

  if (!IsPunc(blocks.parser, "}")) {
    EmitParseSyntaxErr(blocks.parser, TokSpan(blocks.parser));
    return {blocks.parser, blocks.elem};
  }

  Advance(blocks.parser);
  return {blocks.parser, blocks.elem};
}

// =============================================================================
// ParseModalDecl - Parse complete modal declaration
// =============================================================================
//
// SPEC: Parse-Modal
//   Γ ⊢ ParseAttrListOpt(P) ⇓ (P_0, attrs_opt)
//   Γ ⊢ ParseVis(P_0) ⇓ (P_1, vis)
//   IsKw(Tok(P_1), `modal`)
//   Γ ⊢ ParseIdent(Advance(P_1)) ⇓ (P_2, name)
//   Γ ⊢ ParseGenericParamsOpt(P_2) ⇓ (P_3, gen_params_opt)
//   Γ ⊢ ParseImplementsOpt(P_3) ⇓ (P_4, impls)
//   Γ ⊢ ParseWhereClauseOpt(P_4) ⇓ (P_5, where_clause_opt)
//   Γ ⊢ ParseModalBody(P_5) ⇓ (P_6, states)
//   Γ ⊢ ParseInvariantOpt(P_6) ⇓ (P_7, invariant_opt)
//   ────────────────────────────────────────────────────────────────────
//   Γ ⊢ ParseItem(P) ⇓ (P_7, ⟨ModalDecl, ...⟩)

ParseItemResult ParseModalDecl(Parser parser, Visibility vis,
                               AttributeList attrs) {
  SPEC_RULE("Parse-Modal");
  Parser start = parser;

  // Already know we're at "modal" keyword
  Advance(parser);  // consume "modal"

  // Parse modal name
  ParseElemResult<Identifier> name = ParseIdent(parser);
  parser = name.parser;

  // Parse optional generic parameters
  ParseElemResult<std::optional<GenericParams>> gen_params =
      ParseGenericParamsOpt(parser);
  parser = gen_params.parser;

  // Parse optional implements list
  ParseElemResult<std::vector<ClassPath>> impls = ParseImplementsOpt(parser);
  parser = impls.parser;

  // Parse optional where clause
  ParseElemResult<std::optional<WhereClause>> where_clause =
      ParsePredicateClauseOpt(parser);
  parser = where_clause.parser;

  // Parse modal body
  ParseElemResult<std::vector<StateBlock>> states = ParseModalBody(parser);
  parser = states.parser;

  // Parse optional type invariant
  ParseElemResult<std::optional<TypeInvariant>> invariant =
      ParseInvariantOpt(parser);
  parser = invariant.parser;

  ModalDecl decl;
  decl.attrs = std::move(attrs);
  decl.vis = vis;
  decl.name = name.elem;
  decl.generic_params = gen_params.elem;
  decl.implements = std::move(impls.elem);
  decl.where_clause = where_clause.elem;
  decl.invariant = invariant.elem;
  decl.states = std::move(states.elem);
  decl.span = SpanBetween(start, parser);
  decl.doc = {};

  return {parser, decl};
}

}  // namespace cursive::ast
