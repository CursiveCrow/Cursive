#include "cursive0/syntax/parser.h"

#include <optional>
#include <string_view>
#include <utility>
#include <vector>

#include "cursive0/core/assert_spec.h"
#include "cursive0/core/diagnostic_messages.h"
#include "cursive0/core/diagnostics.h"
#include "cursive0/syntax/keyword_policy.h"

namespace cursive0::syntax {

namespace {
void EmitUnsupportedConstruct(Parser& parser) {
  auto diag = core::MakeDiagnostic("E-UNS-0101", TokSpan(parser));
  if (!diag) {
    return;
  }
  parser.diags = core::Emit(parser.diags, *diag);
}
void SkipNewlines(Parser& parser) {
  while (Tok(parser) && Tok(parser)->kind == TokenKind::Newline) {
    Advance(parser);
  }
}


std::shared_ptr<Type> MakeTypeNode(const core::Span& span, TypeNode node) {
  auto ty = std::make_shared<Type>();
  ty->span = span;
  ty->node = std::move(node);
  return ty;
}

std::shared_ptr<Type> MakeTypePrim(const core::Span& span,
                                   std::string_view name) {
  return MakeTypeNode(span, TypePrim{Identifier{name}});
}

bool IsOp(const Parser& parser, std::string_view op) {
  const Token* tok = Tok(parser);
  return tok && IsOpTok(*tok, op);
}

bool IsPunc(const Parser& parser, std::string_view punc) {
  const Token* tok = Tok(parser);
  return tok && IsPuncTok(*tok, punc);
}

bool IsKw(const Parser& parser, std::string_view kw) {
  const Token* tok = Tok(parser);
  return tok && IsKwTok(*tok, kw);
}

bool IsIntTypeLexeme(std::string_view lexeme) {
  return lexeme == "i8" || lexeme == "i16" || lexeme == "i32" ||
         lexeme == "i64" || lexeme == "i128" || lexeme == "u8" ||
         lexeme == "u16" || lexeme == "u32" || lexeme == "u64" ||
         lexeme == "u128" || lexeme == "isize" || lexeme == "usize";
}

bool IsFloatTypeLexeme(std::string_view lexeme) {
  return lexeme == "f16" || lexeme == "f32" || lexeme == "f64";
}

bool IsPrimLexemeSet(std::string_view lexeme) {
  return IsIntTypeLexeme(lexeme) || IsFloatTypeLexeme(lexeme) ||
         lexeme == "bool" || lexeme == "char";
}

bool BuiltinTypePath(const TypePath& path) {
  if (path.size() != 1) {
    return false;
  }
  const std::string_view name = path[0];
  if (IsPrimLexemeSet(name)) {
    return true;
  }
  return name == "string" || name == "bytes";
}

bool TypeArgsUnsupported(const TypePath& path, const Parser& parser) {
  // C0X Extension: Generic type arguments are now supported on all types
  (void)path;
  (void)parser;
  return false;
}

bool C0TypeRestricted(const Token* tok, const TypePath& path, const Parser& parser) {
  if (tok && (TypeWhereTok(*tok) || OpaqueTypeTok(*tok))) {
    return true;
  }
  return TypeArgsUnsupported(path, parser);
}

bool HasFuncArrow(const Parser& parser) {
  if (!IsPunc(parser, "(")) {
    return false;
  }
  Parser cur = parser;
  int depth = 0;
  while (!AtEof(cur)) {
    const Token* tok = Tok(cur);
    if (!tok) {
      return false;
    }
    if (tok->kind == TokenKind::Punctuator) {
      if (tok->lexeme == "(") {
        ++depth;
      } else if (tok->lexeme == ")") {
        --depth;
        if (depth == 0) {
          Parser after = cur;
          Advance(after);
          return IsOp(after, "->");
        }
      }
    }
    Advance(cur);
  }
  return false;
}

struct PermOptResult {
  Parser parser;
  std::optional<TypePerm> perm;
};

PermOptResult ParsePermOpt(Parser parser) {
  const Token* tok = Tok(parser);
  if (tok && IsKwTok(*tok, "const")) {
    SPEC_RULE("Parse-Perm-Const");
    Parser next = parser;
    Advance(next);
    return {next, TypePerm::Const};
  }
  if (tok && IsKwTok(*tok, "unique")) {
    SPEC_RULE("Parse-Perm-Unique");
    Parser next = parser;
    Advance(next);
    return {next, TypePerm::Unique};
  }
  if (tok && IsKwTok(*tok, "shared")) {
    SPEC_RULE("Parse-Perm-Shared");
    Parser next = parser;
    Advance(next);
    return {next, TypePerm::Shared};
  }
  SPEC_RULE("Parse-Perm-None");
  return {parser, std::nullopt};
}

ParseElemResult<TypeFuncParam> ParseParamType(Parser parser);

ParseElemResult<std::vector<TypeFuncParam>> ParseParamTypeListTail(
    Parser parser,
    std::vector<TypeFuncParam> ps) {
  SkipNewlines(parser);
  if (!IsPunc(parser, ",")) {
    SPEC_RULE("Parse-ParamTypeListTail-End");
    return {parser, ps};
  }
  const TokenKindMatch end_set[] = {MatchPunct(")")};
  Parser after = parser;
  Advance(after);
  SkipNewlines(after);
  if (IsPunc(after, ")")) {
    EmitTrailingCommaErr(parser, end_set);
    after.diags = parser.diags;
    return {after, ps};
  }
  SPEC_RULE("Parse-ParamTypeListTail-Comma");
  ParseElemResult<TypeFuncParam> param = ParseParamType(after);
  ps.push_back(param.elem);
  return ParseParamTypeListTail(param.parser, std::move(ps));
}


ParseElemResult<std::vector<TypeFuncParam>> ParseParamTypeList(Parser parser) {
  SkipNewlines(parser);
  if (IsPunc(parser, ")")) {
    SPEC_RULE("Parse-ParamTypeList-Empty");
    return {parser, {}};
  }
  SPEC_RULE("Parse-ParamTypeList-Cons");
  ParseElemResult<TypeFuncParam> first = ParseParamType(parser);
  std::vector<TypeFuncParam> params;
  params.push_back(first.elem);
  return ParseParamTypeListTail(first.parser, std::move(params));
}


ParseElemResult<TypeFuncParam> ParseParamType(Parser parser) {
  if (IsKw(parser, "move")) {
    SPEC_RULE("Parse-ParamType-Move");
    Parser next = parser;
    Advance(next);
    ParseElemResult<std::shared_ptr<Type>> ty = ParseType(next);
    TypeFuncParam param;
    param.mode = ParamMode::Move;
    param.type = ty.elem;
    return {ty.parser, param};
  }
  SPEC_RULE("Parse-ParamType-Plain");
  ParseElemResult<std::shared_ptr<Type>> ty = ParseType(parser);
  TypeFuncParam param;
  param.mode = std::nullopt;
  param.type = ty.elem;
  return {ty.parser, param};
}

ParseElemResult<std::optional<StringState>> ParseStringState(Parser parser) {
  if (!IsOp(parser, "@")) {
    SPEC_RULE("Parse-StringState-None");
    return {parser, std::nullopt};
  }
  Parser next = parser;
  Advance(next);
  const Token* tok = Tok(next);
  if (tok && IsIdentTok(*tok) && tok->lexeme == "Managed") {
    SPEC_RULE("Parse-StringState-Managed");
    Advance(next);
    return {next, StringState::Managed};
  }
  if (tok && IsIdentTok(*tok) && tok->lexeme == "View") {
    SPEC_RULE("Parse-StringState-View");
    Advance(next);
    return {next, StringState::View};
  }
  EmitParseSyntaxErr(next, TokSpan(next));
  return {next, std::nullopt};
}

ParseElemResult<std::optional<BytesState>> ParseBytesState(Parser parser) {
  if (!IsOp(parser, "@")) {
    SPEC_RULE("Parse-BytesState-None");
    return {parser, std::nullopt};
  }
  Parser next = parser;
  Advance(next);
  const Token* tok = Tok(next);
  if (tok && IsIdentTok(*tok) && tok->lexeme == "Managed") {
    SPEC_RULE("Parse-BytesState-Managed");
    Advance(next);
    return {next, BytesState::Managed};
  }
  if (tok && IsIdentTok(*tok) && tok->lexeme == "View") {
    SPEC_RULE("Parse-BytesState-View");
    Advance(next);
    return {next, BytesState::View};
  }
  EmitParseSyntaxErr(next, TokSpan(next));
  return {next, std::nullopt};
}

ParseElemResult<std::optional<PtrState>> ParsePtrState(Parser parser) {
  if (!IsOp(parser, "@")) {
    SPEC_RULE("Parse-PtrState-None");
    return {parser, std::nullopt};
  }
  Parser next = parser;
  Advance(next);
  const Token* tok = Tok(next);
  if (tok && IsIdentTok(*tok) && tok->lexeme == "Valid") {
    SPEC_RULE("Parse-PtrState-Valid");
    Advance(next);
    return {next, PtrState::Valid};
  }
  if (tok && IsIdentTok(*tok) && tok->lexeme == "Null") {
    SPEC_RULE("Parse-PtrState-Null");
    Advance(next);
    return {next, PtrState::Null};
  }
  if (tok && IsIdentTok(*tok) && tok->lexeme == "Expired") {
    SPEC_RULE("Parse-PtrState-Expired");
    Advance(next);
    return {next, PtrState::Expired};
  }
  EmitParseSyntaxErr(next, TokSpan(next));
  return {next, std::nullopt};
}

ParseElemResult<std::vector<std::shared_ptr<Type>>> ParseTypeListTail(
    Parser parser,
    std::vector<std::shared_ptr<Type>> xs) {
  SkipNewlines(parser);
  if (IsPunc(parser, ")") || IsPunc(parser, "}")) {
    SPEC_RULE("Parse-TypeListTail-End");
    return {parser, xs};
  }
  if (IsPunc(parser, ",")) {
    const TokenKindMatch end_set[] = {MatchPunct(")"), MatchPunct("}")};
    Parser after = parser;
    Advance(after);
    SkipNewlines(after);
    if (IsPunc(after, ")") || IsPunc(after, "}")) {
      SPEC_RULE("Parse-TypeListTail-TrailingComma");
      EmitTrailingCommaErr(parser, end_set);
      after.diags = parser.diags;
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


ParseElemResult<std::vector<std::shared_ptr<Type>>> ParseTupleTypeElems(
    Parser parser) {
  SkipNewlines(parser);
  if (IsPunc(parser, ")")) {
    SPEC_RULE("Parse-TupleTypeElems-Empty");
    return {parser, {}};
  }
  ParseElemResult<std::shared_ptr<Type>> first = ParseType(parser);
  Parser after_first = first.parser;
  SkipNewlines(after_first);
  if (IsPunc(after_first, ";")) {
    SPEC_RULE("Parse-TupleTypeElems-One");
    Parser after = after_first;
    Advance(after);
    return {after, {first.elem}};
  }
  if (IsPunc(after_first, ",")) {
    const TokenKindMatch end_set[] = {MatchPunct(")")};
    Parser after = after_first;
    Advance(after);
    SkipNewlines(after);
    if (IsPunc(after, ")")) {
      SPEC_RULE("Parse-TupleTypeElems-TrailingComma");
      EmitTrailingCommaErr(after_first, end_set);
      after.diags = after_first.diags;
      return {after, {first.elem}};
    }
    SPEC_RULE("Parse-TupleTypeElems-Many");
    ParseElemResult<std::shared_ptr<Type>> second = ParseType(after);
    ParseElemResult<std::vector<std::shared_ptr<Type>>> tail =
        ParseTypeListTail(second.parser, {second.elem});
    std::vector<std::shared_ptr<Type>> elems;
    elems.reserve(1 + tail.elem.size());
    elems.push_back(first.elem);
    elems.insert(elems.end(), tail.elem.begin(), tail.elem.end());
    return {tail.parser, elems};
  }
  EmitParseSyntaxErr(after_first, TokSpan(after_first));
  return {after_first, {first.elem}};
}


ParseElemResult<std::shared_ptr<Type>> ParseFuncType(Parser parser) {
  SPEC_RULE("Parse-Func-Type");
  Parser start = parser;
  Parser next = parser;
  Advance(next);
  ParseElemResult<std::vector<TypeFuncParam>> params = ParseParamTypeList(next);
  if (!IsPunc(params.parser, ")")) {
    EmitParseSyntaxErr(params.parser, TokSpan(params.parser));
    return {params.parser, MakeTypePrim(SpanBetween(start, params.parser), "!")};
  }
  Parser after_rparen = params.parser;
  Advance(after_rparen);
  if (!IsOp(after_rparen, "->")) {
    EmitParseSyntaxErr(after_rparen, TokSpan(after_rparen));
    return {after_rparen,
            MakeTypePrim(SpanBetween(start, after_rparen), "!")};
  }
  Parser after_arrow = after_rparen;
  Advance(after_arrow);
  ParseElemResult<std::shared_ptr<Type>> ret = ParseType(after_arrow);
  TypeFunc func;
  func.params = std::move(params.elem);
  func.ret = ret.elem;
  return {ret.parser, MakeTypeNode(SpanBetween(start, ret.parser), func)};
}

ParseElemResult<std::shared_ptr<Type>> ParseSafePointerType(Parser parser) {
  Parser start = parser;
  Parser after_ident = parser;
  Advance(after_ident);
  if (!IsOp(after_ident, "<")) {
    EmitParseSyntaxErr(after_ident, TokSpan(after_ident));
    return {after_ident, MakeTypePrim(SpanBetween(start, after_ident), "!")};
  }
  Parser after_lt = after_ident;
  Advance(after_lt);
  ParseElemResult<std::shared_ptr<Type>> elem = ParseType(after_lt);
  const Token* close = Tok(elem.parser);
  if (close && IsOpTok(*close, ">>")) {
    SPEC_RULE("Parse-Safe-Pointer-Type-ShiftSplit");
    Parser split = SplitShiftR(elem.parser);
    Parser after_left = split;
    Advance(after_left);
    ParseElemResult<std::optional<PtrState>> st = ParsePtrState(after_left);
    TypePtr ptr;
    ptr.element = elem.elem;
    ptr.state = st.elem;
    return {st.parser, MakeTypeNode(SpanBetween(start, st.parser), ptr)};
  }
  if (close && IsOpTok(*close, ">")) {
    SPEC_RULE("Parse-Safe-Pointer-Type");
    Parser after_gt = elem.parser;
    Advance(after_gt);
    ParseElemResult<std::optional<PtrState>> st = ParsePtrState(after_gt);
    TypePtr ptr;
    ptr.element = elem.elem;
    ptr.state = st.elem;
    return {st.parser, MakeTypeNode(SpanBetween(start, st.parser), ptr)};
  }
  EmitParseSyntaxErr(elem.parser, TokSpan(elem.parser));
  return {elem.parser, MakeTypePrim(SpanBetween(start, elem.parser), "!")};
}

ParseElemResult<std::shared_ptr<Type>> ParseNonPermType(Parser parser) {
  const Token* tok = Tok(parser);
  if (!tok) {
    EmitParseSyntaxErr(parser, TokSpan(parser));
    return {parser, MakeTypePrim(TokSpan(parser), "!")};
  }

  if (IsPuncTok(*tok, "(")) {
    if (HasFuncArrow(parser)) {
      return ParseFuncType(parser);
    }
    Parser after = parser;
    Advance(after);
    ParseElemResult<std::vector<std::shared_ptr<Type>>> elems =
        ParseTupleTypeElems(after);
    if (elems.elem.empty()) {
      if (!IsPunc(elems.parser, ")")) {
        EmitParseSyntaxErr(elems.parser, TokSpan(elems.parser));
        return {elems.parser,
                MakeTypePrim(SpanBetween(parser, elems.parser), "!")};
      }
      SPEC_RULE("Parse-Unit-Type");
      Parser after_r = elems.parser;
      Advance(after_r);
      return {after_r,
              MakeTypeNode(SpanBetween(parser, after_r),
                           TypePrim{Identifier{"()"}})};
    }
    if (!IsPunc(elems.parser, ")")) {
      EmitParseSyntaxErr(elems.parser, TokSpan(elems.parser));
      return {elems.parser,
              MakeTypePrim(SpanBetween(parser, elems.parser), "!")};
    }
    SPEC_RULE("Parse-Tuple-Type");
    Parser after_r = elems.parser;
    Advance(after_r);
    TypeTuple tuple;
    tuple.elements = std::move(elems.elem);
    return {after_r, MakeTypeNode(SpanBetween(parser, after_r), tuple)};
  }

  if (IsPuncTok(*tok, "[")) {
    Parser start = parser;
    Parser next = parser;
    Advance(next);
    ParseElemResult<std::shared_ptr<Type>> elem = ParseType(next);
    if (IsPunc(elem.parser, ";")) {
      SPEC_RULE("Parse-Array-Type");
      Parser after_semi = elem.parser;
      Advance(after_semi);
      ParseElemResult<std::shared_ptr<Expr>> len = ParseExpr(after_semi);
      if (!IsPunc(len.parser, "]")) {
        EmitParseSyntaxErr(len.parser, TokSpan(len.parser));
        return {len.parser, MakeTypePrim(SpanBetween(start, len.parser), "!")};
      }
      Parser after_r = len.parser;
      Advance(after_r);
      TypeArray arr;
      arr.element = elem.elem;
      arr.length = len.elem;
      return {after_r, MakeTypeNode(SpanBetween(start, after_r), arr)};
    }
    if (IsPunc(elem.parser, "]")) {
      SPEC_RULE("Parse-Slice-Type");
      Parser after_r = elem.parser;
      Advance(after_r);
      TypeSlice slice;
      slice.element = elem.elem;
      return {after_r, MakeTypeNode(SpanBetween(start, after_r), slice)};
    }
    EmitParseSyntaxErr(elem.parser, TokSpan(elem.parser));
    return {elem.parser, MakeTypePrim(SpanBetween(start, elem.parser), "!")};
  }

  if (IsOpTok(*tok, "!")) {
    SPEC_RULE("Parse-Never-Type");
    Parser next = parser;
    Advance(next);
    return {next,
            MakeTypeNode(SpanBetween(parser, next),
                         TypePrim{Identifier{"!"}})};
  }

  if (IsOpTok(*tok, "*")) {
    Parser start = parser;
    Parser next = parser;
    Advance(next);
    const Token* qual = Tok(next);
    if (qual && qual->kind == TokenKind::Keyword &&
        (qual->lexeme == "imm" || qual->lexeme == "mut")) {
      SPEC_RULE("Parse-Raw-Pointer-Type");
      const RawPtrQual q =
          qual->lexeme == "imm" ? RawPtrQual::Imm : RawPtrQual::Mut;
      Advance(next);
      ParseElemResult<std::shared_ptr<Type>> elem = ParseType(next);
      TypeRawPtr ptr;
      ptr.qual = q;
      ptr.element = elem.elem;
      return {elem.parser, MakeTypeNode(SpanBetween(start, elem.parser), ptr)};
    }
    EmitParseSyntaxErr(next, TokSpan(next));
    return {next, MakeTypePrim(SpanBetween(start, next), "!")};
  }

  if (IsOpTok(*tok, "$")) {
    SPEC_RULE("Parse-Dynamic-Type");
    Parser start = parser;
    Parser next = parser;
    Advance(next);
    ParseElemResult<TypePath> path = ParseTypePath(next);
    TypeDynamic dyn;
    dyn.path = std::move(path.elem);
    return {path.parser, MakeTypeNode(SpanBetween(start, path.parser), dyn)};
  }

  if (IsIdentTok(*tok)) {
    if (TypeWhereTok(*tok)) {
      Parser start = parser;
      Parser next = parser;
      Advance(next);
      EmitUnsupportedConstruct(next);
      TypePath path;
      path.push_back(Identifier{tok->lexeme});
      return {next, MakeTypeNode(SpanBetween(start, next),
                                 TypePathType{std::move(path)})};
    }
    if (OpaqueTypeTok(*tok)) {
      Parser start = parser;
      Parser after_kw = parser;
      Advance(after_kw);
      ParseElemResult<TypePath> opaque_path = ParseTypePath(after_kw);
      SPEC_RULE("Parse-Opaque-Type");
      TypeOpaque opaque;
      opaque.path = std::move(opaque_path.elem);
      return {opaque_path.parser,
              MakeTypeNode(SpanBetween(start, opaque_path.parser), opaque)};
    }
    const std::string_view lexeme = tok->lexeme;
    if (IsPrimLexemeSet(lexeme)) {
      SPEC_RULE("Parse-Prim-Type");
      Parser next = parser;
      Advance(next);
      return {next,
              MakeTypeNode(SpanBetween(parser, next),
                           TypePrim{Identifier{lexeme}})};
    }
    if (lexeme == "string") {
      SPEC_RULE("Parse-String-Type");
      Parser start = parser;
      Parser next = parser;
      Advance(next);
      ParseElemResult<std::optional<StringState>> st = ParseStringState(next);
      TypeString str;
      str.state = st.elem;
      return {st.parser, MakeTypeNode(SpanBetween(start, st.parser), str)};
    }
    if (lexeme == "bytes") {
      SPEC_RULE("Parse-Bytes-Type");
      Parser start = parser;
      Parser next = parser;
      Advance(next);
      ParseElemResult<std::optional<BytesState>> st = ParseBytesState(next);
      TypeBytes bytes;
      bytes.state = st.elem;
      return {st.parser, MakeTypeNode(SpanBetween(start, st.parser), bytes)};
    }
    if (lexeme == "Ptr") {
      Parser after_ident = parser;
      Advance(after_ident);
      if (IsOp(after_ident, "<")) {
        return ParseSafePointerType(parser);
      }
    }

    Parser start = parser;
    ParseElemResult<TypePath> path = ParseTypePath(parser);
    if (C0TypeRestricted(tok, path.elem, path.parser) &&
        TypeArgsUnsupported(path.elem, path.parser)) {
      SPEC_RULE("Parse-Type-Generic-Unsupported");
      Parser skip = SkipAngles(path.parser);
      EmitUnsupportedConstruct(skip);
      SyncType(skip);
      return {skip,
              MakeTypeNode(SpanBetween(start, skip),
                           TypePathType{std::move(path.elem)})};
    }

    // Parse optional generic args, then optional modal state.
    std::vector<std::shared_ptr<Type>> args;
    Parser cur = path.parser;
    const Token* next = Tok(cur);
    if (next && IsOpTok(*next, "<")) {
      SPEC_RULE("Parse-Type-Generic-Args");
      Parser after_lt = cur;
      Advance(after_lt);  // consume <

      // Parse first type arg
      ParseElemResult<std::shared_ptr<Type>> first_arg = ParseType(after_lt);
      args.push_back(first_arg.elem);
      cur = first_arg.parser;

      // Parse additional args separated by ; or ,
      while (IsPunc(cur, ";") || IsPunc(cur, ",")) {
        Advance(cur);
        ParseElemResult<std::shared_ptr<Type>> arg = ParseType(cur);
        args.push_back(arg.elem);
        cur = arg.parser;
      }

      // Expect >
      if (!IsOp(cur, ">")) {
        EmitParseSyntaxErr(cur, TokSpan(cur));
      } else {
        Advance(cur);
      }
    }

    const Token* after_args = Tok(cur);
    if (after_args && IsOpTok(*after_args, "@")) {
      SPEC_RULE("Parse-Modal-State-Type");
      Parser after_at = cur;
      Advance(after_at);
      ParseElemResult<Identifier> state = ParseIdent(after_at);
      TypeModalState modal;
      modal.path = std::move(path.elem);
      modal.generic_args = std::move(args);
      modal.state = state.elem;
      return {state.parser, MakeTypeNode(SpanBetween(start, state.parser), modal)};
    }

    if (!after_args || (!IsOpTok(*after_args, "@") && !IsOpTok(*after_args, "<") &&
                  !BuiltinTypePath(path.elem))) {
      SPEC_RULE("Parse-Type-Path");
      TypePathType ty_path;
      ty_path.path = std::move(path.elem);
      ty_path.generic_args = std::move(args);
      return {cur, MakeTypeNode(SpanBetween(start, cur), ty_path)};
    }

    TypePathType ty_path;
    ty_path.path = std::move(path.elem);
    ty_path.generic_args = std::move(args);
    return {cur, MakeTypeNode(SpanBetween(start, cur), ty_path)};
  }

  EmitParseSyntaxErr(parser, TokSpan(parser));
  return {parser, MakeTypePrim(TokSpan(parser), "!")};
}

ParseElemResult<std::vector<std::shared_ptr<Type>>> ParseUnionTail(
    Parser parser) {
  if (!IsOp(parser, "|")) {
    SPEC_RULE("Parse-UnionTail-None");
    return {parser, {}};
  }
  SPEC_RULE("Parse-UnionTail-Cons");
  Parser next = parser;
  Advance(next);
  ParseElemResult<std::shared_ptr<Type>> head = ParseNonPermType(next);
  ParseElemResult<std::vector<std::shared_ptr<Type>>> tail =
      ParseUnionTail(head.parser);
  std::vector<std::shared_ptr<Type>> elems;
  elems.reserve(1 + tail.elem.size());
  elems.push_back(head.elem);
  elems.insert(elems.end(), tail.elem.begin(), tail.elem.end());
  return {tail.parser, elems};
}

}  // namespace

ParseElemResult<std::shared_ptr<Type>> ParseType(Parser parser) {
  Parser start = parser;
  PermOptResult perm = ParsePermOpt(parser);
  Parser after_perm = perm.parser;
  const Token* tok = Tok(after_perm);
  if (!tok) {
    SPEC_RULE("Parse-Type-Err");
    EmitParseSyntaxErr(after_perm, TokSpan(after_perm));
    std::shared_ptr<Type> base =
        MakeTypePrim(SpanBetween(start, after_perm), "!");
    if (perm.perm.has_value()) {
      TypePermType perm_type;
      perm_type.perm = *perm.perm;
      perm_type.base = base;
      return {after_perm,
              MakeTypeNode(SpanBetween(start, after_perm), perm_type)};
    }
    return {after_perm, base};
  }
  const bool non_perm_start = tok->kind == TokenKind::Identifier ||
                              (tok->kind == TokenKind::Punctuator &&
                               (tok->lexeme == "(" || tok->lexeme == "[")) ||
                              (tok->kind == TokenKind::Operator &&
                               (tok->lexeme == "*" || tok->lexeme == "$" ||
                                tok->lexeme == "!"));
  if (!non_perm_start) {
    SPEC_RULE("Parse-Type-Err");
    EmitParseSyntaxErr(after_perm, TokSpan(after_perm));
    std::shared_ptr<Type> base =
        MakeTypePrim(SpanBetween(start, after_perm), "!");
    if (perm.perm.has_value()) {
      TypePermType perm_type;
      perm_type.perm = *perm.perm;
      perm_type.base = base;
      return {after_perm,
              MakeTypeNode(SpanBetween(start, after_perm), perm_type)};
    }
    return {after_perm, base};
  }

  ParseElemResult<std::shared_ptr<Type>> base = ParseNonPermType(after_perm);
  ParseElemResult<std::vector<std::shared_ptr<Type>>> tail =
      ParseUnionTail(base.parser);

  Parser out = tail.parser;

  SPEC_RULE("Parse-Type");
  std::shared_ptr<Type> merged = base.elem;
  if (!tail.elem.empty()) {
    TypeUnion uni;
    uni.types.reserve(1 + tail.elem.size());
    uni.types.push_back(base.elem);
    uni.types.insert(uni.types.end(), tail.elem.begin(), tail.elem.end());
    merged = MakeTypeNode(SpanBetween(start, out), uni);
  }
  if (perm.perm.has_value()) {
    TypePermType perm_type;
    perm_type.perm = *perm.perm;
    perm_type.base = merged;
    merged = MakeTypeNode(SpanBetween(start, out), perm_type);
  }

  const Token* where_tok = Tok(out);
  if (where_tok && TypeWhereTok(*where_tok)) {
    SPEC_RULE("Parse-Refinement-Type");
    Parser after_where = out;
    Advance(after_where);  // consume where
    if (!IsPunc(after_where, "{")) {
      EmitParseSyntaxErr(after_where, TokSpan(after_where));
      Parser sync = after_where;
      SyncType(sync);
      return {sync, merged};
    }
    Parser after_l = after_where;
    Advance(after_l);
    ParseElemResult<std::shared_ptr<Expr>> pred = ParseExpr(after_l);
    if (!IsPunc(pred.parser, "}")) {
      EmitParseSyntaxErr(pred.parser, TokSpan(pred.parser));
      Parser sync = pred.parser;
      SyncType(sync);
      return {sync, merged};
    }
    Parser after_r = pred.parser;
    Advance(after_r);
    TypeRefine refine;
    refine.base = merged;
    refine.predicate = pred.elem;
    merged = MakeTypeNode(SpanBetween(start, after_r), refine);
    out = after_r;
  }
  return {out, merged};
}

}  // namespace cursive0::syntax
