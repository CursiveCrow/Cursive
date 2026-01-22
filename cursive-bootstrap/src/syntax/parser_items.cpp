#include "cursive0/syntax/parser.h"

#include <string_view>

#include "cursive0/core/assert_spec.h"
#include "cursive0/core/diagnostic_messages.h"
#include "cursive0/core/diagnostics.h"
#include "cursive0/syntax/keyword_policy.h"

namespace cursive0::syntax {

ParseElemResult<std::shared_ptr<Type>> ParseTypeAnnotOpt(Parser parser);

namespace {
void EmitUnsupportedConstruct(Parser& parser) {
  auto diag = core::MakeDiagnostic("E-UNS-0101", TokSpan(parser));
  if (!diag) {
    return;
  }
  parser.diags = core::Emit(parser.diags, *diag);
}

bool IsWhereTok(const Parser& parser) {
  const Token* tok = Tok(parser);
  return tok && IsIdentTok(*tok) && tok->lexeme == "where";
}

// Forward declarations for helper functions used by generic parsing
bool IsKw(const Parser& parser, std::string_view kw);
bool IsOp(const Parser& parser, std::string_view op);
bool IsPunc(const Parser& parser, std::string_view p);

// C0X Extension: Generic parameter parsing

// Parse type bounds: <: Class1 + Class2
ParseElemResult<std::vector<TypeBound>> ParseTypeBounds(Parser parser) {
  std::vector<TypeBound> bounds;
  if (!IsOp(parser, "<:")) {
    return {parser, bounds};
  }
  Parser next = parser;
  Advance(next);  // consume <:
  
  // Parse first bound
  ParseElemResult<Identifier> first_name = ParseIdent(next);
  TypeBound first_bound;
  first_bound.class_path.push_back(first_name.elem);
  bounds.push_back(first_bound);
  next = first_name.parser;
  
  // Parse additional bounds separated by +
  while (IsOp(next, "+")) {
    Advance(next);
    ParseElemResult<Identifier> bound_name = ParseIdent(next);
    TypeBound bound;
    bound.class_path.push_back(bound_name.elem);
    bounds.push_back(bound);
    next = bound_name.parser;
  }
  
  return {next, bounds};
}

// Parse a single type parameter: T <: Bound = DefaultType
ParseElemResult<TypeParam> ParseTypeParam(Parser parser) {
  Parser start = parser;
  
  // Parse name
  ParseElemResult<Identifier> name = ParseIdent(parser);
  
  // Parse optional bounds
  ParseElemResult<std::vector<TypeBound>> bounds = ParseTypeBounds(name.parser);
  
  // Parse optional default type
  std::shared_ptr<Type> default_type;
  Parser after_bounds = bounds.parser;
  if (IsOp(after_bounds, "=")) {
    Advance(after_bounds);
    ParseElemResult<std::shared_ptr<Type>> ty = ParseType(after_bounds);
    default_type = ty.elem;
    after_bounds = ty.parser;
  }
  
  TypeParam param;
  param.name = name.elem;
  param.bounds = bounds.elem;
  param.default_type = default_type;
  param.span = SpanBetween(start, after_bounds);
  
  return {after_bounds, param};
}

// Parse generic parameters: <T; U <: Clone>
ParseElemResult<std::optional<GenericParams>> ParseGenericParamsOpt(Parser parser) {
  if (!IsOp(parser, "<")) {
    return {parser, std::nullopt};
  }
  
  SPEC_RULE("Parse-Generic-Params");
  Parser start = parser;
  Parser next = parser;
  Advance(next);  // consume <
  
  GenericParams params;
  
  // Parse first type param
  ParseElemResult<TypeParam> first = ParseTypeParam(next);
  params.params.push_back(first.elem);
  next = first.parser;
  
  // Parse additional params separated by ;
  while (IsPunc(next, ";")) {
    Advance(next);
    ParseElemResult<TypeParam> param = ParseTypeParam(next);
    params.params.push_back(param.elem);
    next = param.parser;
  }
  
  // Expect >
  if (!IsOp(next, ">")) {
    EmitParseSyntaxErr(next, TokSpan(next));
  } else {
    Advance(next);
  }
  
  params.span = SpanBetween(start, next);
  return {next, params};
}

// Parse where clause: where T <: Ord, U <: Clone
ParseElemResult<std::optional<WhereClause>> ParseWhereClauseOpt(Parser parser) {
  // Skip any newlines before where
  while (Tok(parser) && Tok(parser)->kind == TokenKind::Newline) {
    Advance(parser);
  }
  if (!IsKw(parser, "where")) {
    return {parser, std::nullopt};
  }
  
  SPEC_RULE("Parse-Where-Clause");
  Parser start = parser;
  Parser next = parser;
  Advance(next);  // consume where
  
  // Skip newlines after where
  while (Tok(next) && Tok(next)->kind == TokenKind::Newline) {
    Advance(next);
  }
  
  WhereClause clause;
  
  // Parse first predicate
  ParseElemResult<Identifier> first_name = ParseIdent(next);
  ParseElemResult<std::vector<TypeBound>> first_bounds = ParseTypeBounds(first_name.parser);
  WherePredicate first_pred;
  first_pred.type_param = first_name.elem;
  first_pred.bounds = first_bounds.elem;
  first_pred.span = SpanBetween(next, first_bounds.parser);
  clause.predicates.push_back(first_pred);
  next = first_bounds.parser;
  
  // Parse additional predicates after newline/terminator
  // For simplicity, just check for another identifier after consuming optional terminator
  const Token* tok = Tok(next);
  while (tok && (tok->kind == TokenKind::Newline || tok->lexeme == ";")) {
    Parser after_term = next;
    Advance(after_term);
    const Token* next_tok = Tok(after_term);
    if (next_tok && IsIdentTok(*next_tok) && 
        !IsPunc(after_term, "{") && !IsKw(after_term, "where")) {
      // Check if followed by <: (indicates another predicate)
      Parser probe = after_term;
      Advance(probe);
      if (IsOp(probe, "<:")) {
        next = after_term;
        ParseElemResult<Identifier> pred_name = ParseIdent(next);
        ParseElemResult<std::vector<TypeBound>> pred_bounds = ParseTypeBounds(pred_name.parser);
        WherePredicate pred;
        pred.type_param = pred_name.elem;
        pred.bounds = pred_bounds.elem;
        pred.span = SpanBetween(next, pred_bounds.parser);
        clause.predicates.push_back(pred);
        next = pred_bounds.parser;
        tok = Tok(next);
        continue;
      }
    }
    break;
  }
  
  clause.span = SpanBetween(start, next);
  return {next, clause};
}

// Legacy reject function - now just returns error without blocking
ParseItemResult RejectWhereClause(Parser start, Parser parser) {
  // C0X Extension: Where clauses are now supported, this function should not be called
  // If we get here, it means there's a where clause in an unexpected context
  SPEC_RULE("Unsupported-Construct");
  EmitUnsupportedConstruct(parser);
  Parser next = parser;
  SyncItem(next);
  return {next, ErrorItem{SpanBetween(start, next)}};
}

std::shared_ptr<Type> MakeErrorType(const core::Span& span) {
  auto ty = std::make_shared<Type>();
  ty->span = span;
  ty->node = TypePrim{Identifier{"!"}};
  return ty;
}

ParseElemResult<ClassItem> ParseUnsupportedClassItem(Parser parser,
                                                     Visibility vis) {
  Parser sync = parser;
  SyncStmt(sync);
  ClassFieldDecl field;
  field.vis = vis;
  field.name = Identifier{"_"};
  field.type = MakeErrorType(SpanBetween(parser, sync));
  field.span = SpanBetween(parser, sync);
  field.doc_opt = std::nullopt;
  return {sync, field};
}

void NormalizeBindingPattern(std::shared_ptr<Pattern>& pat,
                             std::shared_ptr<Type>& type_opt) {
  if (!pat || type_opt) {
    return;
  }
  const auto* typed = std::get_if<TypedPattern>(&pat->node);
  if (!typed) {
    return;
  }
  auto normalized = std::make_shared<Pattern>();
  normalized->span = pat->span;
  normalized->node = IdentifierPattern{typed->name};
  type_opt = typed->type;
  pat = std::move(normalized);
}

void EmitImportUnsupported(Parser& parser) {
  SPEC_RULE("WF-Import-Unsupported");
  auto diag = core::MakeDiagnostic("E-UNS-0110");
  if (!diag) {
    return;
  }
  parser.diags = core::Emit(parser.diags, *diag);
}

// C0X Extension: Attribute parsing

// Parse single attribute item: attr_name or attr_name(args)
// Note: Attribute names can be keywords (like "dynamic") or identifiers
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

// Parse attribute list: [[item, item, ...]]
// Returns empty list if no attributes
ParseElemResult<AttributeList> ParseAttributeListOpt(Parser parser) {
  AttributeList attrs;
  
  while (IsPunc(parser, "[")) {
    Parser probe = parser;
    Advance(probe);
    if (!IsPunc(probe, "[")) {
      break;  // Not an attribute
    }
    
    // Consume [[
    Parser next = parser;
    Advance(next);
    Advance(next);
    
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
    
    // Expect ]]
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
    
    parser = next;
  }
  
  return {parser, attrs};
}

// C0X Extension: Contract clause parsing

// Parse contract clause: |= pre => post
ParseElemResult<std::optional<ContractClause>> ParseContractClauseOpt(Parser parser) {
  // Skip any newlines before contract
  while (Tok(parser) && Tok(parser)->kind == TokenKind::Newline) {
    Advance(parser);
  }
  if (!IsOp(parser, "|=")) {
    return {parser, std::nullopt};
  }
  
  SPEC_RULE("Parse-Contract-Clause");
  Parser start = parser;
  Parser next = parser;
  Advance(next);  // consume |=
  
  ContractClause clause;
  
  // Check for => (postcondition only case)
  if (IsOp(next, "=>")) {
    Advance(next);
    ParseElemResult<ExprPtr> post = ParseExpr(next);
    clause.postcondition = post.elem;
    clause.span = SpanBetween(start, post.parser);
    return {post.parser, clause};
  }
  
  // Parse precondition
  ParseElemResult<ExprPtr> pre = ParseExpr(next);
  clause.precondition = pre.elem;
  next = pre.parser;
  
  // Check for => postcondition
  if (IsOp(next, "=>")) {
    Advance(next);
    ParseElemResult<ExprPtr> post = ParseExpr(next);
    clause.postcondition = post.elem;
    next = post.parser;
  }
  
  clause.span = SpanBetween(start, next);
  return {next, clause};
}

void EmitExternUnsupported(Parser& parser) {
  auto diag = core::MakeDiagnostic("E-UNS-0112");
  if (!diag) {
    return;
  }
  parser.diags = core::Emit(parser.diags, *diag);
}

void EmitReturnAtModuleErr(Parser& parser) {
  auto diag = core::MakeDiagnostic("E-SEM-3165");
  if (!diag) {
    return;
  }
  parser.diags = core::Emit(parser.diags, *diag);
}

bool IsKw(const Parser& parser, std::string_view kw) {
  const Token* tok = Tok(parser);
  return tok && IsKwTok(*tok, kw);
}

bool IsOp(const Parser& parser, std::string_view op) {
  const Token* tok = Tok(parser);
  return tok && IsOpTok(*tok, op);
}

bool IsPunc(const Parser& parser, std::string_view p) {
  const Token* tok = Tok(parser);
  return tok && IsPuncTok(*tok, p);
}

ParseElemResult<std::vector<std::shared_ptr<Type>>> ParseTypeListTail(
    Parser parser,
    std::vector<std::shared_ptr<Type>> xs) {
  if (IsPunc(parser, ")") || IsPunc(parser, "}")) {
    SPEC_RULE("Parse-TypeListTail-End");
    return {parser, xs};
  }
  if (IsPunc(parser, ",")) {
    Parser after = parser;
    Advance(after);
    if (IsPunc(after, ")") || IsPunc(after, "}")) {
      SPEC_RULE("Parse-TypeListTail-TrailingComma");
      EmitUnsupportedConstruct(after);
      return {after, xs};
    }
    SPEC_RULE("Parse-TypeListTail-Comma");
    ParseElemResult<std::shared_ptr<Type>> elem = ParseType(after);
    xs.push_back(elem.elem);
    return ParseTypeListTail(elem.parser, std::move(xs));
  }
  EmitParseSyntaxErr(parser, TokSpan(parser));
  return {parser, xs};
}

ParseElemResult<std::vector<std::shared_ptr<Type>>> ParseTypeList(Parser parser) {
  if (IsPunc(parser, ")")) {
    SPEC_RULE("Parse-TypeList-Empty");
    return {parser, {}};
  }
  SPEC_RULE("Parse-TypeList-Cons");
  ParseElemResult<std::shared_ptr<Type>> first = ParseType(parser);
  std::vector<std::shared_ptr<Type>> types;
  types.push_back(first.elem);
  return ParseTypeListTail(first.parser, std::move(types));
}

ParseElemResult<UsingSpec> ParseUsingSpec(Parser parser) {
  SPEC_RULE("Parse-UsingSpec");
  ParseElemResult<Identifier> name = ParseIdent(parser);
  ParseElemResult<std::optional<Identifier>> alias = ParseAliasOpt(name.parser);
  UsingSpec spec;
  spec.name = name.elem;
  spec.alias_opt = alias.elem;
  return {alias.parser, spec};
}

ParseElemResult<std::vector<UsingSpec>> ParseUsingListTail(
    Parser parser,
    std::vector<UsingSpec> xs) {
  if (IsPunc(parser, "}")) {
    SPEC_RULE("Parse-UsingListTail-End");
    return {parser, xs};
  }
  if (IsPunc(parser, ",")) {
    Parser after_comma = parser;
    Advance(after_comma);
    if (IsPunc(after_comma, "}")) {
      SPEC_RULE("Parse-UsingListTail-TrailingComma");
      EmitUnsupportedConstruct(after_comma);
      return {after_comma, xs};
    }
    SPEC_RULE("Parse-UsingListTail-Comma");
    ParseElemResult<UsingSpec> spec = ParseUsingSpec(after_comma);
    xs.push_back(spec.elem);
    return ParseUsingListTail(spec.parser, std::move(xs));
  }
  EmitParseSyntaxErr(parser, TokSpan(parser));
  return {parser, xs};
}

ParseElemResult<std::vector<UsingSpec>> ParseUsingList(Parser parser) {
  if (IsPunc(parser, "}")) {
    SPEC_RULE("Parse-UsingList-Empty");
    EmitUnsupportedConstruct(parser);
    Advance(parser);
    return {parser, {}};
  }
  SPEC_RULE("Parse-UsingList-Cons");
  ParseElemResult<UsingSpec> spec = ParseUsingSpec(parser);
  ParseElemResult<std::vector<UsingSpec>> tail =
      ParseUsingListTail(spec.parser, {spec.elem});
  if (!IsPunc(tail.parser, "}")) {
    EmitParseSyntaxErr(tail.parser, TokSpan(tail.parser));
    return {tail.parser, tail.elem};
  }
  Advance(tail.parser);
  return {tail.parser, tail.elem};
}


ParseElemResult<std::shared_ptr<Type>> ParseReturnOpt(Parser parser) {
  if (!IsOp(parser, "->")) {
    SPEC_RULE("Parse-ReturnOpt-None");
    return {parser, nullptr};
  }
  SPEC_RULE("Parse-ReturnOpt-Arrow");
  Parser next = parser;
  Advance(next);
  ParseElemResult<std::shared_ptr<Type>> ty = ParseType(next);
  return {ty.parser, ty.elem};
}

ParseElemResult<std::optional<ParamMode>> ParseParamModeOpt(Parser parser) {
  if (IsKw(parser, "move")) {
    SPEC_RULE("Parse-ParamMode-Move");
    Parser next = parser;
    Advance(next);
    return {next, ParamMode::Move};
  }
  SPEC_RULE("Parse-ParamMode-None");
  return {parser, std::nullopt};
}

ParseElemResult<Param> ParseParam(Parser parser) {
  SPEC_RULE("Parse-Param");
  Parser start = parser;
  ParseElemResult<std::optional<ParamMode>> mode = ParseParamModeOpt(parser);
  ParseElemResult<Identifier> name = ParseIdent(mode.parser);
  if (!IsPunc(name.parser, ":")) {
    EmitParseSyntaxErr(name.parser, TokSpan(name.parser));
  } else {
    Advance(name.parser);
  }
  ParseElemResult<std::shared_ptr<Type>> ty = ParseType(name.parser);
  Param param;
  param.mode = mode.elem;
  param.name = name.elem;
  param.type = ty.elem;
  param.span = SpanBetween(start, ty.parser);
  return {ty.parser, param};
}

ParseElemResult<std::vector<Param>> ParseParamTail(Parser parser,
                                                  std::vector<Param> xs) {
  if (IsPunc(parser, ")")) {
    SPEC_RULE("Parse-ParamTail-End");
    return {parser, xs};
  }
  if (IsPunc(parser, ",")) {
    Parser after = parser;
    Advance(after);
    if (IsPunc(after, ")")) {
      SPEC_RULE("Parse-ParamTail-TrailingComma");
      EmitUnsupportedConstruct(after);
      return {after, xs};
    }
    SPEC_RULE("Parse-ParamTail-Comma");
    ParseElemResult<Param> param = ParseParam(after);
    xs.push_back(param.elem);
    return ParseParamTail(param.parser, std::move(xs));
  }
  EmitParseSyntaxErr(parser, TokSpan(parser));
  return {parser, xs};
}

ParseElemResult<std::vector<Param>> ParseParamList(Parser parser) {
  if (IsPunc(parser, ")")) {
    SPEC_RULE("Parse-ParamList-Empty");
    return {parser, {}};
  }
  SPEC_RULE("Parse-ParamList-Cons");
  ParseElemResult<Param> param = ParseParam(parser);
  std::vector<Param> params;
  params.push_back(param.elem);
  return ParseParamTail(param.parser, std::move(params));
}

ParseElemResult<std::vector<Param>> ParseMethodParams(Parser parser) {
  if (IsPunc(parser, ")")) {
    SPEC_RULE("Parse-MethodParams-None");
    return {parser, {}};
  }
  if (IsPunc(parser, ",")) {
    SPEC_RULE("Parse-MethodParams-Comma");
    Parser next = parser;
    Advance(next);
    return ParseParamList(next);
  }
  EmitParseSyntaxErr(parser, TokSpan(parser));
  return {parser, {}};
}

ParseElemResult<Receiver> ParseReceiver(Parser parser) {
  if (IsOp(parser, "~")) {
    SPEC_RULE("Parse-Receiver-Short-Const");
    Parser next = parser;
    Advance(next);
    return {next, ReceiverShorthand{ReceiverPerm::Const}};
  }
  if (IsOp(parser, "~!")) {
    SPEC_RULE("Parse-Receiver-Short-Unique");
    Parser next = parser;
    Advance(next);
    return {next, ReceiverShorthand{ReceiverPerm::Unique}};
  }
  // C0X Extension: shared receiver shorthand
  if (IsOp(parser, "~%")) {
    SPEC_RULE("Parse-Receiver-Short-Shared");
    Parser next = parser;
    Advance(next);
    return {next, ReceiverShorthand{ReceiverPerm::Shared}};
  }

  SPEC_RULE("Parse-Receiver-Explicit");
  Parser start = parser;
  ParseElemResult<std::optional<ParamMode>> mode = ParseParamModeOpt(parser);
  ParseElemResult<Identifier> name = ParseIdent(mode.parser);
  if (!(name.elem == "self")) {
    EmitParseSyntaxErr(start, TokSpan(start));
  }
  if (!IsPunc(name.parser, ":")) {
    EmitParseSyntaxErr(name.parser, TokSpan(name.parser));
  } else {
    Advance(name.parser);
  }
  ParseElemResult<std::shared_ptr<Type>> ty = ParseType(name.parser);
  ReceiverExplicit recv;
  recv.mode_opt = mode.elem;
  recv.type = ty.elem;
  return {ty.parser, recv};
}

struct MethodSignatureResult {
  Parser parser;
  Receiver receiver;
  std::vector<Param> params;
  std::shared_ptr<Type> return_type_opt;
};

MethodSignatureResult ParseMethodSignature(Parser parser) {
  SPEC_RULE("Parse-MethodSignature");
  if (!IsPunc(parser, "(")) {
    EmitParseSyntaxErr(parser, TokSpan(parser));
    return {parser, ReceiverShorthand{ReceiverPerm::Const}, {}, nullptr};
  }
  Parser next = parser;
  Advance(next);
  ParseElemResult<Receiver> receiver = ParseReceiver(next);
  ParseElemResult<std::vector<Param>> params = ParseMethodParams(receiver.parser);
  if (!IsPunc(params.parser, ")")) {
    EmitParseSyntaxErr(params.parser, TokSpan(params.parser));
  } else {
    Advance(params.parser);
  }
  ParseElemResult<std::shared_ptr<Type>> ret = ParseReturnOpt(params.parser);
  return {ret.parser, receiver.elem, params.elem, ret.elem};
}

struct SignatureResult {
  Parser parser;
  std::vector<Param> params;
  std::shared_ptr<Type> return_type_opt;
};

SignatureResult ParseSignature(Parser parser) {
  SPEC_RULE("Parse-Signature");
  if (!IsPunc(parser, "(")) {
    EmitParseSyntaxErr(parser, TokSpan(parser));
    return {parser, {}, nullptr};
  }
  Parser next = parser;
  Advance(next);
  ParseElemResult<std::vector<Param>> params = ParseParamList(next);
  if (!IsPunc(params.parser, ")")) {
    EmitParseSyntaxErr(params.parser, TokSpan(params.parser));
  } else {
    Advance(params.parser);
  }
  ParseElemResult<std::shared_ptr<Type>> ret = ParseReturnOpt(params.parser);
  return {ret.parser, params.elem, ret.elem};
}

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

ParseElemResult<Stmt> ParseShadowBindingImpl(Parser parser) {
  SPEC_RULE("Parse-ShadowBinding");
  Parser start = parser;
  const Token* tok = Tok(parser);
  if (!tok || tok->kind != TokenKind::Keyword ||
      (tok->lexeme != "let" && tok->lexeme != "var")) {
    EmitParseSyntaxErr(parser, TokSpan(parser));
    Parser next = AdvanceOrEOF(parser);
    return {next, ErrorStmt{SpanBetween(start, next)}};
  }
  const bool is_let = tok->lexeme == "let";
  Parser next = parser;
  Advance(next);
  ParseElemResult<Identifier> name = ParseIdent(next);
  ParseElemResult<std::shared_ptr<Type>> ty = ParseTypeAnnotOpt(name.parser);
  if (!IsOp(ty.parser, "=")) {
    EmitParseSyntaxErr(ty.parser, TokSpan(ty.parser));
  } else {
    Advance(ty.parser);
  }
  ParseElemResult<std::shared_ptr<Expr>> init = ParseExpr(ty.parser);
  if (is_let) {
    ShadowLetStmt stmt;
    stmt.name = name.elem;
    stmt.type_opt = ty.elem;
    stmt.init = init.elem;
    stmt.span = SpanBetween(start, init.parser);
    return {init.parser, stmt};
  }
  ShadowVarStmt stmt;
  stmt.name = name.elem;
  stmt.type_opt = ty.elem;
  stmt.init = init.elem;
  stmt.span = SpanBetween(start, init.parser);
  return {init.parser, stmt};
}

struct RecordFieldInitOptResult {
  Parser parser;
  std::shared_ptr<Expr> init_opt;
};

RecordFieldInitOptResult ParseRecordFieldInitOpt(Parser parser) {
  if (!IsOp(parser, "=")) {
    SPEC_RULE("Parse-RecordFieldInitOpt-None");
    return {parser, nullptr};
  }
  SPEC_RULE("Parse-RecordFieldInitOpt-Yes");
  Parser next = parser;
  Advance(next);
  ParseElemResult<std::shared_ptr<Expr>> init = ParseExpr(next);
  return {init.parser, init.elem};
}

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

ParseElemResult<FieldDecl> ParseRecordFieldDecl(Parser parser) {
  SPEC_RULE("Parse-RecordFieldDecl");
  ParseElemResult<Visibility> vis = ParseVis(parser);
  return ParseRecordFieldDeclAfterVis(vis.parser, vis.elem);
}

ParseElemResult<std::vector<FieldDecl>> ParseRecordFieldDeclTail(
    Parser parser,
    std::vector<FieldDecl> xs) {
  if (IsPunc(parser, "}")) {
    SPEC_RULE("Parse-RecordFieldDeclTail-End");
    return {parser, xs};
  }
  if (IsPunc(parser, ",")) {
    Parser after = parser;
    Advance(after);
    if (IsPunc(after, "}")) {
      SPEC_RULE("Parse-RecordFieldDeclTail-TrailingComma");
      EmitUnsupportedConstruct(after);
      return {after, xs};
    }
    SPEC_RULE("Parse-RecordFieldDeclTail-Comma");
    ParseElemResult<FieldDecl> field = ParseRecordFieldDecl(after);
    xs.push_back(field.elem);
    return ParseRecordFieldDeclTail(field.parser, std::move(xs));
  }
  EmitParseSyntaxErr(parser, TokSpan(parser));
  return {parser, xs};
}

ParseElemResult<std::vector<FieldDecl>> ParseRecordFieldDeclList(Parser parser) {
  if (IsPunc(parser, "}")) {
    SPEC_RULE("Parse-RecordFieldDeclList-Empty");
    return {parser, {}};
  }
  SPEC_RULE("Parse-RecordFieldDeclList-Cons");
  ParseElemResult<FieldDecl> field = ParseRecordFieldDecl(parser);
  std::vector<FieldDecl> fields;
  fields.push_back(field.elem);
  return ParseRecordFieldDeclTail(field.parser, std::move(fields));
}

ParseElemResult<FieldDecl> ParseFieldDecl(Parser parser) {
  SPEC_RULE("Parse-FieldDecl");
  Parser start = parser;
  ParseElemResult<Visibility> vis = ParseVis(parser);
  ParseElemResult<Identifier> name = ParseIdent(vis.parser);
  if (!IsPunc(name.parser, ":")) {
    EmitParseSyntaxErr(name.parser, TokSpan(name.parser));
  } else {
    Advance(name.parser);
  }
  ParseElemResult<std::shared_ptr<Type>> ty = ParseType(name.parser);
  FieldDecl field;
  field.vis = vis.elem;
  field.name = name.elem;
  field.type = ty.elem;
  field.init_opt = nullptr;
  field.span = SpanBetween(start, ty.parser);
  field.doc_opt = std::nullopt;
  return {ty.parser, field};
}

ParseElemResult<std::vector<FieldDecl>> ParseFieldDeclTail(
    Parser parser,
    std::vector<FieldDecl> xs) {
  if (IsPunc(parser, "}")) {
    SPEC_RULE("Parse-FieldDeclTail-End");
    return {parser, xs};
  }
  if (IsPunc(parser, ",")) {
    Parser after = parser;
    Advance(after);
    if (IsPunc(after, "}")) {
      SPEC_RULE("Parse-FieldDeclTail-TrailingComma");
      EmitUnsupportedConstruct(after);
      return {after, xs};
    }
    SPEC_RULE("Parse-FieldDeclTail-Comma");
    ParseElemResult<FieldDecl> field = ParseFieldDecl(after);
    xs.push_back(field.elem);
    return ParseFieldDeclTail(field.parser, std::move(xs));
  }
  EmitParseSyntaxErr(parser, TokSpan(parser));
  return {parser, xs};
}

ParseElemResult<std::vector<FieldDecl>> ParseFieldDeclList(Parser parser) {
  if (IsPunc(parser, "}")) {
    SPEC_RULE("Parse-FieldDeclList-Empty");
    return {parser, {}};
  }
  SPEC_RULE("Parse-FieldDeclList-Cons");
  ParseElemResult<FieldDecl> field = ParseFieldDecl(parser);
  std::vector<FieldDecl> fields;
  fields.push_back(field.elem);
  return ParseFieldDeclTail(field.parser, std::move(fields));
}

struct OverrideResult {
  Parser parser;
  bool override_flag;
};

OverrideResult ParseOverrideOpt(Parser parser) {
  if (IsKw(parser, "override")) {
    SPEC_RULE("Parse-Override-Yes");
    Parser next = parser;
    Advance(next);
    return {next, true};
  }
  SPEC_RULE("Parse-Override-No");
  return {parser, false};
}

ParseElemResult<MethodDecl> ParseMethodDefAfterVis(Parser parser,
                                                  Visibility vis) {
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

ParseElemResult<RecordMember> ParseRecordMember(Parser parser) {
  ParseElemResult<Visibility> vis = ParseVis(parser);
  if (IsKw(vis.parser, "procedure") || IsKw(vis.parser, "override")) {
    SPEC_RULE("Parse-RecordMember-Method");
    ParseElemResult<MethodDecl> method = ParseMethodDefAfterVis(vis.parser, vis.elem);
    return {method.parser, method.elem};
  }
  SPEC_RULE("Parse-RecordMember-Field");
  ParseElemResult<FieldDecl> field = ParseRecordFieldDeclAfterVis(vis.parser, vis.elem);
  return {field.parser, field.elem};
}

ParseElemResult<std::vector<RecordMember>> ParseRecordMemberTail(
    Parser parser,
    std::vector<RecordMember> xs) {
  // Skip any newline terminators
  while (Tok(parser) && Tok(parser)->kind == TokenKind::Newline) {
    Advance(parser);
  }
  
  if (IsPunc(parser, "}")) {
    SPEC_RULE("Parse-RecordMemberTail-End");
    return {parser, xs};
  }
  if (IsPunc(parser, ",")) {
    Parser after = parser;
    Advance(after);
    // Skip newlines after comma
    while (Tok(after) && Tok(after)->kind == TokenKind::Newline) {
      Advance(after);
    }
    if (IsPunc(after, "}")) {
      SPEC_RULE("Parse-RecordMemberTail-TrailingComma");
      EmitUnsupportedConstruct(after);
      return {after, xs};
    }
    SPEC_RULE("Parse-RecordMemberTail-Comma");
    ParseElemResult<RecordMember> mem = ParseRecordMember(after);
    xs.push_back(mem.elem);
    return ParseRecordMemberTail(mem.parser, std::move(xs));
  }
  // Newlines work as separators between members (no comma required)
  // Check if the next token starts a new member (procedure, override, or identifier for field).
  ParseElemResult<Visibility> next_vis = ParseVis(parser);
  if (IsKw(next_vis.parser, "procedure") || IsKw(next_vis.parser, "override")) {
    SPEC_RULE("Parse-RecordMemberTail-MethodNoComma");
    ParseElemResult<RecordMember> mem = ParseRecordMember(parser);
    xs.push_back(mem.elem);
    return ParseRecordMemberTail(mem.parser, std::move(xs));
  }
  // Check if it's a field (identifier followed by : or # for key boundary)
  const Token* tok = Tok(next_vis.parser);
  if (tok && (tok->kind == TokenKind::Identifier || IsOp(next_vis.parser, "#"))) {
    SPEC_RULE("Parse-RecordMemberTail-FieldNoComma");
    ParseElemResult<RecordMember> mem = ParseRecordMember(parser);
    xs.push_back(mem.elem);
    return ParseRecordMemberTail(mem.parser, std::move(xs));
  }
  EmitParseSyntaxErr(parser, TokSpan(parser));
  return {parser, xs};
}

ParseElemResult<std::vector<RecordMember>> ParseRecordMemberList(Parser parser) {
  // Skip leading newlines
  while (Tok(parser) && Tok(parser)->kind == TokenKind::Newline) {
    Advance(parser);
  }
  if (IsPunc(parser, "}")) {
    SPEC_RULE("Parse-RecordMemberList-End");
    return {parser, {}};
  }
  SPEC_RULE("Parse-RecordMemberList-Cons");
  ParseElemResult<RecordMember> mem = ParseRecordMember(parser);
  std::vector<RecordMember> members;
  members.push_back(mem.elem);
  return ParseRecordMemberTail(mem.parser, std::move(members));
}

ParseElemResult<std::vector<RecordMember>> ParseRecordBody(Parser parser) {
  SPEC_RULE("Parse-RecordBody");
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
  ParseElemResult<std::vector<RecordMember>> members = ParseRecordMemberList(next);
  if (!IsPunc(members.parser, "}")) {
    EmitParseSyntaxErr(members.parser, TokSpan(members.parser));
    return {members.parser, members.elem};
  }
  Advance(members.parser);
  return {members.parser, members.elem};
}

ParseElemResult<VariantPayload> ParseVariantPayloadOpt(Parser parser,
                                                     bool& has_payload) {
  if (!IsPunc(parser, "(") && !IsPunc(parser, "{")) {
    SPEC_RULE("Parse-VariantPayloadOpt-None");
    has_payload = false;
    VariantPayload payload = VariantPayloadTuple{};
    return {parser, payload};
  }
  if (IsPunc(parser, "(")) {
    SPEC_RULE("Parse-VariantPayloadOpt-Tuple");
    Parser next = parser;
    Advance(next);
    ParseElemResult<std::vector<std::shared_ptr<Type>>> types = ParseTypeList(next);
    if (!IsPunc(types.parser, ")")) {
      EmitParseSyntaxErr(types.parser, TokSpan(types.parser));
    } else {
      Advance(types.parser);
    }
    VariantPayloadTuple tuple;
    tuple.elements = std::move(types.elem);
    has_payload = true;
    return {types.parser, tuple};
  }
  SPEC_RULE("Parse-VariantPayloadOpt-Record");
  Parser next = parser;
  Advance(next);
  ParseElemResult<std::vector<FieldDecl>> fields = ParseFieldDeclList(next);
  if (!IsPunc(fields.parser, "}")) {
    EmitParseSyntaxErr(fields.parser, TokSpan(fields.parser));
  } else {
    Advance(fields.parser);
  }
  VariantPayloadRecord record;
  record.fields = std::move(fields.elem);
  has_payload = true;
  return {fields.parser, record};
}

struct VariantDiscriminantResult {
  Parser parser;
  std::optional<Token> disc_opt;
};

VariantDiscriminantResult ParseVariantDiscriminantOpt(Parser parser) {
  if (!IsOp(parser, "=")) {
    SPEC_RULE("Parse-VariantDiscriminantOpt-None");
    return {parser, std::nullopt};
  }
  SPEC_RULE("Parse-VariantDiscriminantOpt-Yes");
  Parser next = parser;
  Advance(next);
  const Token* tok = Tok(next);
  if (!tok || tok->kind != TokenKind::IntLiteral) {
    EmitParseSyntaxErr(next, TokSpan(next));
    return {next, std::nullopt};
  }
  Token lit = *tok;
  Advance(next);
  return {next, lit};
}

ParseElemResult<VariantDecl> ParseVariant(Parser parser) {
  SPEC_RULE("Parse-Variant");
  Parser start = parser;
  ParseElemResult<Identifier> name = ParseIdent(parser);
  bool has_payload = false;
  ParseElemResult<VariantPayload> payload =
      ParseVariantPayloadOpt(name.parser, has_payload);
  VariantDiscriminantResult disc = ParseVariantDiscriminantOpt(payload.parser);
  VariantDecl var;
  var.name = name.elem;
  if (has_payload) {
    var.payload_opt = payload.elem;
  }
  var.discriminant_opt = disc.disc_opt;
  var.span = SpanBetween(start, disc.parser);
  var.doc_opt = std::nullopt;
  return {disc.parser, var};
}

ParseElemResult<std::vector<VariantDecl>> ParseVariantTail(Parser parser,
                                                          std::vector<VariantDecl> xs) {
  if (IsPunc(parser, "}")) {
    SPEC_RULE("Parse-VariantTail-End");
    return {parser, xs};
  }
  if (IsPunc(parser, ",")) {
    Parser after = parser;
    Advance(after);
    if (IsPunc(after, "}")) {
      SPEC_RULE("Parse-VariantTail-TrailingComma");
      EmitUnsupportedConstruct(after);
      return {after, xs};
    }
    SPEC_RULE("Parse-VariantTail-Comma");
    ParseElemResult<VariantDecl> var = ParseVariant(after);
    xs.push_back(var.elem);
    return ParseVariantTail(var.parser, std::move(xs));
  }
  EmitParseSyntaxErr(parser, TokSpan(parser));
  return {parser, xs};
}

ParseElemResult<std::vector<VariantDecl>> ParseVariantList(Parser parser) {
  if (IsPunc(parser, "}")) {
    SPEC_RULE("Parse-VariantList-Empty");
    return {parser, {}};
  }
  SPEC_RULE("Parse-VariantList-Cons");
  ParseElemResult<VariantDecl> var = ParseVariant(parser);
  std::vector<VariantDecl> vars;
  vars.push_back(var.elem);
  return ParseVariantTail(var.parser, std::move(vars));
}

ParseElemResult<std::vector<VariantDecl>> ParseEnumBody(Parser parser) {
  SPEC_RULE("Parse-EnumBody");
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
  ParseElemResult<std::vector<VariantDecl>> vars = ParseVariantList(next);
  if (!IsPunc(vars.parser, "}")) {
    EmitParseSyntaxErr(vars.parser, TokSpan(vars.parser));
    return {vars.parser, vars.elem};
  }
  Advance(vars.parser);
  return {vars.parser, vars.elem};
}

ParseElemResult<StateMember> ParseStateMember(Parser parser) {
  ParseElemResult<Visibility> vis = ParseVis(parser);
  if (IsKw(vis.parser, "procedure") || IsKw(vis.parser, "override")) {
    SPEC_RULE("Parse-StateMember-Method");
    Parser start = vis.parser;
    Advance(start);
    ParseElemResult<Identifier> name = ParseIdent(start);
    SignatureResult sig = ParseSignature(name.parser);
    ParseElemResult<std::shared_ptr<Block>> body = ParseBlock(sig.parser);
    StateMethodDecl method;
    method.vis = vis.elem;
    method.name = name.elem;
    method.params = sig.params;
    method.return_type_opt = sig.return_type_opt;
    method.body = body.elem;
    method.span = SpanBetween(parser, body.parser);
    method.doc_opt = std::nullopt;
    return {body.parser, method};
  }
  if (IsKw(vis.parser, "transition")) {
    SPEC_RULE("Parse-StateMember-Transition");
    Parser start = vis.parser;
    Advance(start);
    ParseElemResult<Identifier> name = ParseIdent(start);
    Parser cur = name.parser;
    if (!IsPunc(cur, "(")) {
      EmitParseSyntaxErr(cur, TokSpan(cur));
    } else {
      Advance(cur);
    }
    ParseElemResult<std::vector<Param>> params = ParseParamList(cur);
    cur = params.parser;
    if (!IsPunc(cur, ")")) {
      EmitParseSyntaxErr(cur, TokSpan(cur));
    } else {
      Advance(cur);
    }
    if (!IsOp(cur, "->")) {
      EmitParseSyntaxErr(cur, TokSpan(cur));
    } else {
      Advance(cur);
    }
    if (!IsOp(cur, "@")) {
      EmitParseSyntaxErr(cur, TokSpan(cur));
    } else {
      Advance(cur);
    }
    ParseElemResult<Identifier> target = ParseIdent(cur);
    ParseElemResult<std::shared_ptr<Block>> body = ParseBlock(target.parser);
    TransitionDecl trans;
    trans.vis = vis.elem;
    trans.name = name.elem;
    trans.params = params.elem;
    trans.target_state = target.elem;
    trans.body = body.elem;
    trans.span = SpanBetween(parser, body.parser);
    trans.doc_opt = std::nullopt;
    return {body.parser, trans};
  }
  SPEC_RULE("Parse-StateMember-Field");
  ParseElemResult<Identifier> name = ParseIdent(vis.parser);
  if (!IsPunc(name.parser, ":")) {
    EmitParseSyntaxErr(name.parser, TokSpan(name.parser));
  } else {
    Advance(name.parser);
  }
  ParseElemResult<std::shared_ptr<Type>> ty = ParseType(name.parser);
  StateFieldDecl field;
  field.vis = vis.elem;
  field.name = name.elem;
  field.type = ty.elem;
  field.span = SpanBetween(parser, ty.parser);
  field.doc_opt = std::nullopt;
  return {ty.parser, field};
}

ParseElemResult<std::vector<StateMember>> ParseStateMemberList(Parser parser) {
  if (IsPunc(parser, "}")) {
    SPEC_RULE("Parse-StateMemberList-End");
    return {parser, {}};
  }
  SPEC_RULE("Parse-StateMemberList-Cons");
  std::vector<StateMember> members;
  Parser cur = parser;
  while (!IsPunc(cur, "}")) {
    if (IsPunc(cur, ",")) {
      EmitParseSyntaxErr(cur, TokSpan(cur));
      Advance(cur);
      continue;
    }
    Parser before = cur;
    ParseElemResult<StateMember> mem = ParseStateMember(cur);
    members.push_back(mem.elem);
    cur = mem.parser;
    if (cur.tokens == before.tokens && cur.index == before.index) {
      EmitParseSyntaxErr(cur, TokSpan(cur));
      cur = AdvanceOrEOF(cur);
    }
  }
  return {cur, members};
}

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

ParseElemResult<std::vector<StateBlock>> ParseStateBlockList(Parser parser) {
  if (IsPunc(parser, "}")) {
    SPEC_RULE("Parse-StateBlockList-Empty");
    return {parser, {}};
  }
  SPEC_RULE("Parse-StateBlockList-Cons");
  ParseElemResult<StateBlock> block = ParseStateBlock(parser);
  std::vector<StateBlock> blocks;
  blocks.push_back(block.elem);
  Parser cur = block.parser;
  while (!IsPunc(cur, "}")) {
    ParseElemResult<StateBlock> next = ParseStateBlock(cur);
    blocks.push_back(next.elem);
    cur = next.parser;
  }
  return {cur, blocks};
}

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

  if (saw_override) {
    return ParseUnsupportedClassItem(cur, vis.elem);
  }

  if (IsKw(cur, "type") || IsKw(cur, "modal")) {
    SPEC_RULE("Unsupported-Construct");
    EmitUnsupportedConstruct(cur);
    return ParseUnsupportedClassItem(cur, vis.elem);
  }

  SPEC_RULE("Parse-ClassItem-Field");
  Parser start = cur;
  ParseElemResult<Identifier> name = ParseIdent(start);
  if (!IsPunc(name.parser, ":")) {
    EmitParseSyntaxErr(name.parser, TokSpan(name.parser));
  } else {
    Advance(name.parser);
  }
  ParseElemResult<std::shared_ptr<Type>> ty = ParseType(name.parser);
  Parser after_type = ty.parser;
  ConsumeTerminatorReq(after_type);
  ClassFieldDecl field;
  field.vis = vis.elem;
  field.name = name.elem;
  field.type = ty.elem;
  field.span = SpanBetween(parser, after_type);
  field.doc_opt = std::nullopt;
  return {after_type, field};
}

ParseElemResult<std::vector<ClassItem>> ParseClassItemList(Parser parser) {
  if (IsPunc(parser, "}")) {
    SPEC_RULE("Parse-ClassItemList-End");
    return {parser, {}};
  }
  SPEC_RULE("Parse-ClassItemList-Cons");
  ParseElemResult<ClassItem> item = ParseClassItem(parser);
  std::vector<ClassItem> items;
  items.push_back(item.elem);
  Parser cur = item.parser;
  while (!IsPunc(cur, "}")) {
    ParseElemResult<ClassItem> next = ParseClassItem(cur);
    items.push_back(next.elem);
    cur = next.parser;
  }
  return {cur, items};
}

ParseElemResult<std::vector<ClassItem>> ParseClassBody(Parser parser) {
  SPEC_RULE("Parse-ClassBody");
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
  ParseElemResult<std::vector<ClassItem>> items = ParseClassItemList(next);
  if (!IsPunc(items.parser, "}")) {
    EmitParseSyntaxErr(items.parser, TokSpan(items.parser));
    return {items.parser, items.elem};
  }
  Advance(items.parser);
  return {items.parser, items.elem};
}

ParseElemResult<std::vector<ClassPath>> ParseClassListTail(Parser parser,
                                                          std::vector<ClassPath> xs) {
  if (!IsPunc(parser, ",")) {
    SPEC_RULE("Parse-ClassListTail-End");
    return {parser, xs};
  }
  const TokenKindMatch end_set[] = {MatchPunct("{")};
  if (EmitTrailingCommaErr(parser, end_set)) {
    Parser after = parser;
    Advance(after);
    return {after, xs};
  }
  SPEC_RULE("Parse-ClassListTail-Comma");
  Parser next = parser;
  Advance(next);
  ParseElemResult<ClassPath> cls = ParseClassPath(next);
  xs.push_back(cls.elem);
  return ParseClassListTail(cls.parser, std::move(xs));
}

ParseElemResult<std::vector<ClassPath>> ParseClassList(Parser parser) {
  SPEC_RULE("Parse-ClassList-Cons");
  ParseElemResult<ClassPath> first = ParseClassPath(parser);
  std::vector<ClassPath> xs;
  xs.push_back(first.elem);
  return ParseClassListTail(first.parser, std::move(xs));
}

ParseElemResult<std::vector<ClassPath>> ParseImplementsOpt(Parser parser) {
  if (!IsOp(parser, "<:")) {
    SPEC_RULE("Parse-Implements-None");
    return {parser, {}};
  }
  SPEC_RULE("Parse-Implements-Yes");
  Parser next = parser;
  Advance(next);
  return ParseClassList(next);
}

ParseElemResult<std::vector<ClassPath>> ParseSuperclassBoundsTail(
    Parser parser,
    std::vector<ClassPath> xs) {
  if (!IsOp(parser, "+")) {
    SPEC_RULE("Parse-SuperclassBoundsTail-End");
    return {parser, xs};
  }
  SPEC_RULE("Parse-SuperclassBoundsTail-Plus");
  Parser next = parser;
  Advance(next);
  ParseElemResult<ClassPath> cls = ParseClassPath(next);
  xs.push_back(cls.elem);
  return ParseSuperclassBoundsTail(cls.parser, std::move(xs));
}

ParseElemResult<std::vector<ClassPath>> ParseSuperclassBounds(Parser parser) {
  SPEC_RULE("Parse-SuperclassBounds-Cons");
  ParseElemResult<ClassPath> first = ParseClassPath(parser);
  std::vector<ClassPath> xs;
  xs.push_back(first.elem);
  return ParseSuperclassBoundsTail(first.parser, std::move(xs));
}

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

}  // namespace

ParseElemResult<Binding> ParseBindingAfterLetVar(Parser parser) {
  return ParseBindingAfterLetVarImpl(parser);
}

ParseElemResult<Stmt> ParseShadowBinding(Parser parser) {
  return ParseShadowBindingImpl(parser);
}

ParseElemResult<std::shared_ptr<Type>> ParseTypeAnnotOpt(Parser parser) {
  if (!IsPunc(parser, ":")) {
    SPEC_RULE("Parse-TypeAnnotOpt-None");
    return {parser, nullptr};
  }
  SPEC_RULE("Parse-TypeAnnotOpt-Yes");
  Parser next = parser;
  Advance(next);
  ParseElemResult<std::shared_ptr<Type>> ty = ParseType(next);
  return {ty.parser, ty.elem};
}


ParseItemResult ParseItem(Parser parser) {
  Parser start = parser;
  
  // C0X Extension: Parse optional attributes first
  ParseElemResult<AttributeList> attrs = ParseAttributeListOpt(parser);
  parser = attrs.parser;
  
  // Skip newlines after attributes
  while (Tok(parser) && Tok(parser)->kind == TokenKind::Newline) {
    Advance(parser);
  }
  
  if (IsWhereTok(parser)) {
    // Stray where clause at top level is an error
    SPEC_RULE("Parse-Stray-Where");
    EmitParseSyntaxErr(parser, TokSpan(parser));
    Parser next = parser;
    SyncItem(next);
    return {next, ErrorItem{SpanBetween(start, next)}};
  }
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
  if (IsKw(parser, "modal")) {
    Parser after_modal = parser;
    Advance(after_modal);
    const Token* next_tok = Tok(after_modal);
    if (IsKw(after_modal, "class") ||
        (next_tok && IsIdentTok(*next_tok) && next_tok->lexeme == "class")) {
      // C0X Extension: Modal class - parse it
      SPEC_RULE("Parse-Modal-Class");
      Parser next = after_modal;
      Advance(next);  // consume class
      ParseElemResult<Identifier> name = ParseIdent(next);
      ParseElemResult<std::optional<GenericParams>> gen_params = ParseGenericParamsOpt(name.parser);
      ParseElemResult<std::vector<ClassPath>> supers = ParseSuperclassOpt(gen_params.parser);
      ParseElemResult<std::optional<WhereClause>> where_clause = ParseWhereClauseOpt(supers.parser);
      ParseElemResult<std::vector<ClassItem>> items = ParseClassBody(where_clause.parser);
      ClassDecl decl;
      decl.vis = Visibility::Private;
      decl.modal = true;
      decl.name = name.elem;
      decl.generic_params = gen_params.elem;
      decl.supers = std::move(supers.elem);
      decl.where_clause = where_clause.elem;
      decl.items = std::move(items.elem);
      decl.span = SpanBetween(start, items.parser);
      decl.doc = {};
      return {items.parser, decl};
    }
  }
  if (const Token* tok = Tok(parser);
      tok && IsIdentTok(*tok) && tok->lexeme == "extern") {
    SPEC_RULE("Parse-Extern-Unsupported");
    EmitExternUnsupported(parser);
    Parser next = parser;
    Advance(next);
    SyncItem(next);
    return {next, ErrorItem{SpanBetween(start, next)}};
  }
  if (const Token* tok = Tok(parser);
      tok && IsIdentTok(*tok) && tok->lexeme == "use") {
    SPEC_RULE("Parse-Use-Unsupported");
    EmitUnsupportedConstruct(parser);
    Parser next = parser;
    Advance(next);
    SyncItem(next);
    return {next, ErrorItem{SpanBetween(start, next)}};
  }
  if (IsKw(parser, "return")) {
    SPEC_RULE("Return-At-Module-Err");
    EmitReturnAtModuleErr(parser);
    Parser next = parser;
    Advance(next);
    SyncItem(next);
    return {next, ErrorItem{SpanBetween(start, next)}};
  }

  ParseElemResult<Visibility> vis = ParseVis(parser);
  Parser cur = vis.parser;

  if (IsKw(cur, "using")) {
    Parser next = cur;
    Advance(next);
    ParseElemResult<ModulePath> path = ParseModulePath(next);
    if (IsPunc(path.parser, "{")) {
      SPEC_RULE("Parse-Using-List");
      Parser after_brace = path.parser;
      Advance(after_brace);
      ParseElemResult<std::vector<UsingSpec>> specs = ParseUsingList(after_brace);
      UsingDecl decl;
      decl.vis = vis.elem;
      decl.clause = UsingList{path.elem, std::move(specs.elem)};
      decl.span = SpanBetween(start, specs.parser);
      decl.doc = {};
      return {specs.parser, decl};
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

  if (IsKw(cur, "procedure")) {
    SPEC_RULE("Parse-Procedure");
    Parser next = cur;
    Advance(next);
    ParseElemResult<Identifier> name = ParseIdent(next);
    // C0X Extension: Parse optional generic parameters
    ParseElemResult<std::optional<GenericParams>> gen_params = ParseGenericParamsOpt(name.parser);
    SignatureResult sig = ParseSignature(gen_params.parser);
    // C0X Extension: Parse optional where clause
    ParseElemResult<std::optional<WhereClause>> where_clause = ParseWhereClauseOpt(sig.parser);
    // C0X Extension: Parse optional contract clause
    ParseElemResult<std::optional<ContractClause>> contract = ParseContractClauseOpt(where_clause.parser);
    ParseElemResult<std::shared_ptr<Block>> body = ParseBlock(contract.parser);
    ProcedureDecl decl;
    decl.attrs = attrs.elem;  // C0X Extension
    decl.vis = vis.elem;
    decl.name = name.elem;
    decl.generic_params = gen_params.elem;  // C0X Extension
    decl.params = sig.params;
    decl.return_type_opt = sig.return_type_opt;
    decl.where_clause = where_clause.elem;  // C0X Extension
    decl.contract = contract.elem;  // C0X Extension
    decl.body = body.elem;
    decl.span = SpanBetween(start, body.parser);
    decl.doc = {};
    return {body.parser, decl};
  }

  if (IsKw(cur, "record")) {
    SPEC_RULE("Parse-Record");
    Parser next = cur;
    Advance(next);
    ParseElemResult<Identifier> name = ParseIdent(next);
    // C0X Extension: Parse optional generic parameters
    ParseElemResult<std::optional<GenericParams>> gen_params = ParseGenericParamsOpt(name.parser);
    ParseElemResult<std::vector<ClassPath>> impls = ParseImplementsOpt(gen_params.parser);
    // C0X Extension: Parse optional where clause
    ParseElemResult<std::optional<WhereClause>> where_clause = ParseWhereClauseOpt(impls.parser);
    ParseElemResult<std::vector<RecordMember>> members = ParseRecordBody(where_clause.parser);
    RecordDecl decl;
    decl.attrs = attrs.elem;  // C0X Extension
    decl.vis = vis.elem;
    decl.name = name.elem;
    decl.generic_params = gen_params.elem;  // C0X Extension
    decl.implements = std::move(impls.elem);
    decl.where_clause = where_clause.elem;  // C0X Extension
    decl.members = std::move(members.elem);
    decl.span = SpanBetween(start, members.parser);
    decl.doc = {};
    return {members.parser, decl};
  }

  if (IsKw(cur, "enum")) {
    SPEC_RULE("Parse-Enum");
    Parser next = cur;
    Advance(next);
    ParseElemResult<Identifier> name = ParseIdent(next);
    // C0X Extension: Parse optional generic parameters
    ParseElemResult<std::optional<GenericParams>> gen_params = ParseGenericParamsOpt(name.parser);
    ParseElemResult<std::vector<ClassPath>> impls = ParseImplementsOpt(gen_params.parser);
    // C0X Extension: Parse optional where clause
    ParseElemResult<std::optional<WhereClause>> where_clause = ParseWhereClauseOpt(impls.parser);
    ParseElemResult<std::vector<VariantDecl>> vars = ParseEnumBody(where_clause.parser);
    EnumDecl decl;
    decl.attrs = attrs.elem;  // C0X Extension
    decl.vis = vis.elem;
    decl.name = name.elem;
    decl.generic_params = gen_params.elem;  // C0X Extension
    decl.implements = std::move(impls.elem);
    decl.where_clause = where_clause.elem;  // C0X Extension
    decl.variants = std::move(vars.elem);
    decl.span = SpanBetween(start, vars.parser);
    decl.doc = {};
    return {vars.parser, decl};
  }

  if (IsKw(cur, "modal")) {
    SPEC_RULE("Parse-Modal");
    Parser next = cur;
    Advance(next);
    ParseElemResult<Identifier> name = ParseIdent(next);
    // C0X Extension: Parse optional generic parameters
    ParseElemResult<std::optional<GenericParams>> gen_params = ParseGenericParamsOpt(name.parser);
    ParseElemResult<std::vector<ClassPath>> impls = ParseImplementsOpt(gen_params.parser);
    // C0X Extension: Parse optional where clause
    ParseElemResult<std::optional<WhereClause>> where_clause = ParseWhereClauseOpt(impls.parser);
    ParseElemResult<std::vector<StateBlock>> states = ParseModalBody(where_clause.parser);
    ModalDecl decl;
    decl.vis = vis.elem;
    decl.name = name.elem;
    decl.generic_params = gen_params.elem;  // C0X Extension
    decl.implements = std::move(impls.elem);
    decl.where_clause = where_clause.elem;  // C0X Extension
    decl.states = std::move(states.elem);
    decl.span = SpanBetween(start, states.parser);
    decl.doc = {};
    return {states.parser, decl};
  }

  if (IsKw(cur, "class")) {
    SPEC_RULE("Parse-Class");
    Parser next = cur;
    Advance(next);
    ParseElemResult<Identifier> name = ParseIdent(next);
    // C0X Extension: Parse optional generic parameters
    ParseElemResult<std::optional<GenericParams>> gen_params = ParseGenericParamsOpt(name.parser);
    ParseElemResult<std::vector<ClassPath>> supers = ParseSuperclassOpt(gen_params.parser);
    // C0X Extension: Parse optional where clause
    ParseElemResult<std::optional<WhereClause>> where_clause = ParseWhereClauseOpt(supers.parser);
    ParseElemResult<std::vector<ClassItem>> items = ParseClassBody(where_clause.parser);
    ClassDecl decl;
    decl.vis = vis.elem;
    decl.modal = false;
    decl.name = name.elem;
    decl.generic_params = gen_params.elem;  // C0X Extension
    decl.supers = std::move(supers.elem);
    decl.where_clause = where_clause.elem;  // C0X Extension
    decl.items = std::move(items.elem);
    decl.span = SpanBetween(start, items.parser);
    decl.doc = {};
    return {items.parser, decl};
  }

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
    decl.type = ty.elem;
    decl.span = SpanBetween(start, where_clause.parser);
    decl.doc = {};
    return {where_clause.parser, decl};
  }

  SPEC_RULE("Parse-Item-Err");
  EmitParseSyntaxErr(parser, TokSpan(parser));
  Parser next = AdvanceOrEOF(parser);
  SyncItem(next);
  return {next, ErrorItem{SpanBetween(start, next)}};
}

}  // namespace cursive0::syntax
