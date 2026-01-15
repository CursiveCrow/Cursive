#include <cassert>
#include <string>
#include <vector>
#include <variant>

#include "cursive0/core/assert_spec.h"
#include "cursive0/core/source_text.h"
#include "cursive0/syntax/ast.h"
#include "cursive0/syntax/lexer.h"
#include "cursive0/syntax/parser.h"

namespace {

using cursive0::core::LineStarts;
using cursive0::core::SourceFile;
using cursive0::core::UnicodeScalar;
using cursive0::syntax::ParseElemResult;
using cursive0::syntax::ParseType;
using cursive0::syntax::Parser;
using cursive0::syntax::Tokenize;
using cursive0::syntax::TokenizeResult;
using cursive0::syntax::Type;
using cursive0::syntax::TypeArray;
using cursive0::syntax::TypeBytes;
using cursive0::syntax::TypeDynamic;
using cursive0::syntax::TypeFunc;
using cursive0::syntax::TypeModalState;
using cursive0::syntax::TypePathType;
using cursive0::syntax::TypePrim;
using cursive0::syntax::TypePtr;
using cursive0::syntax::TypeRawPtr;
using cursive0::syntax::TypeSlice;
using cursive0::syntax::TypeString;
using cursive0::syntax::TypeTuple;
using cursive0::syntax::TypeUnion;
using cursive0::syntax::FilterNewlines;
using cursive0::syntax::MakeParser;
using cursive0::syntax::AtEof;
using cursive0::syntax::PStateOk;

static std::vector<UnicodeScalar> AsScalars(const std::string& text) {
  std::vector<UnicodeScalar> out;
  out.reserve(text.size());
  for (unsigned char c : text) {
    out.push_back(static_cast<UnicodeScalar>(c));
  }
  return out;
}

static SourceFile MakeSourceFile(const std::string& path,
                                 const std::string& text) {
  SourceFile s;
  s.path = path;
  s.scalars = AsScalars(text);
  s.text = cursive0::core::EncodeUtf8(s.scalars);
  s.bytes.assign(s.text.begin(), s.text.end());
  s.byte_len = s.text.size();
  s.line_starts = LineStarts(s.scalars);
  s.line_count = s.line_starts.size();
  return s;
}

static Parser MakeParserFor(const std::string& text) {
  SourceFile source = MakeSourceFile("type.cursive", text);
  TokenizeResult tok = Tokenize(source);
  assert(tok.output.has_value());
  auto tokens = std::make_shared<std::vector<cursive0::syntax::Token>>(
      FilterNewlines(tok.output->tokens));
  Parser parser = MakeParser(*tokens, source);
  parser.owned_tokens = tokens;
  return parser;
}

static ParseElemResult<std::shared_ptr<Type>> ParseTypeOk(const std::string& text) {
  Parser parser = MakeParserFor(text);
  ParseElemResult<std::shared_ptr<Type>> result = ParseType(parser);
  assert(result.elem != nullptr);
  assert(result.parser.diags.empty());
  assert(PStateOk(result.parser));
  assert(AtEof(result.parser));
  return result;
}

static ParseElemResult<std::shared_ptr<Type>> ParseTypeDiag(const std::string& text) {
  Parser parser = MakeParserFor(text);
  ParseElemResult<std::shared_ptr<Type>> result = ParseType(parser);
  assert(result.elem != nullptr);
  assert(!result.parser.diags.empty());
  assert(PStateOk(result.parser));
  return result;
}

}  // namespace

int main() {
  SPEC_COV("Parse-Type");
  SPEC_COV("Parse-Type-Err");
  SPEC_COV("Parse-Type-Path");
  SPEC_COV("Parse-Prim-Type");
  SPEC_COV("Parse-String-Type");
  SPEC_COV("Parse-StringState-None");
  SPEC_COV("Parse-StringState-Managed");
  SPEC_COV("Parse-StringState-View");
  SPEC_COV("Parse-Bytes-Type");
  SPEC_COV("Parse-BytesState-None");
  SPEC_COV("Parse-BytesState-Managed");
  SPEC_COV("Parse-BytesState-View");
  SPEC_COV("Parse-PtrState-None");
  SPEC_COV("Parse-PtrState-Valid");
  SPEC_COV("Parse-PtrState-Null");
  SPEC_COV("Parse-PtrState-Expired");
  SPEC_COV("Parse-Perm-Const");
  SPEC_COV("Parse-Perm-Unique");
  SPEC_COV("Parse-Perm-Shared");
  SPEC_COV("Parse-Perm-None");
  SPEC_COV("Parse-Unit-Type");
  SPEC_COV("Parse-Tuple-Type");
  SPEC_COV("Parse-TupleTypeElems-Empty");
  SPEC_COV("Parse-TupleTypeElems-Many");
  SPEC_COV("Parse-TupleTypeElems-One");
  SPEC_COV("Parse-TupleTypeElems-TrailingComma");
  SPEC_COV("Parse-Func-Type");
  SPEC_COV("Parse-ParamType-Move");
  SPEC_COV("Parse-ParamType-Plain");
  SPEC_COV("Parse-ParamTypeList-Cons");
  SPEC_COV("Parse-ParamTypeList-Empty");
  SPEC_COV("Parse-ParamTypeListTail-Comma");
  SPEC_COV("Parse-ParamTypeListTail-End");
  SPEC_COV("Parse-Array-Type");
  SPEC_COV("Parse-Slice-Type");
  SPEC_COV("Parse-Raw-Pointer-Type");
  SPEC_COV("Parse-Safe-Pointer-Type");
  SPEC_COV("Parse-Safe-Pointer-Type-ShiftSplit");
  SPEC_COV("Parse-Dynamic-Type");
  SPEC_COV("Parse-Never-Type");
  SPEC_COV("Parse-Modal-State-Type");
  SPEC_COV("Parse-UnionTail-Cons");
  SPEC_COV("Parse-UnionTail-None");

  {
    ParseElemResult<std::shared_ptr<Type>> ty = ParseTypeOk("i32");
    assert(std::holds_alternative<TypePrim>(ty.elem->node));
  }

  ParseTypeOk("const i32");
  ParseTypeOk("unique i32");
  ParseTypeOk("shared i32");

  {
    ParseElemResult<std::shared_ptr<Type>> ty = ParseTypeOk("string");
    assert(std::holds_alternative<TypeString>(ty.elem->node));
  }
  ParseTypeOk("string@Managed");
  ParseTypeOk("string@View");

  {
    ParseElemResult<std::shared_ptr<Type>> ty = ParseTypeOk("bytes");
    assert(std::holds_alternative<TypeBytes>(ty.elem->node));
  }
  ParseTypeOk("bytes@Managed");
  ParseTypeOk("bytes@View");

  {
    ParseElemResult<std::shared_ptr<Type>> ty = ParseTypeOk("()");
    assert(std::holds_alternative<TypePrim>(ty.elem->node));
  }
  {
    ParseElemResult<std::shared_ptr<Type>> ty = ParseTypeOk("(i32, bool)");
    assert(std::holds_alternative<TypeTuple>(ty.elem->node));
  }
  ParseTypeOk("(i32;)");
  ParseTypeDiag("(i32,)");

  {
    ParseElemResult<std::shared_ptr<Type>> ty = ParseTypeOk("() -> i32");
    assert(std::holds_alternative<TypeFunc>(ty.elem->node));
  }
  ParseTypeOk("(i32) -> i32");
  ParseTypeOk("(move i32, i32) -> i32");

  {
    ParseElemResult<std::shared_ptr<Type>> ty = ParseTypeOk("[u8; 4]");
    assert(std::holds_alternative<TypeArray>(ty.elem->node));
  }
  {
    ParseElemResult<std::shared_ptr<Type>> ty = ParseTypeOk("[u8]");
    assert(std::holds_alternative<TypeSlice>(ty.elem->node));
  }

  {
    ParseElemResult<std::shared_ptr<Type>> ty = ParseTypeOk("*imm i32");
    assert(std::holds_alternative<TypeRawPtr>(ty.elem->node));
  }
  {
    ParseElemResult<std::shared_ptr<Type>> ty = ParseTypeOk("Ptr<i32>");
    assert(std::holds_alternative<TypePtr>(ty.elem->node));
  }
  ParseTypeOk("Ptr<i32>@Valid");
  ParseTypeOk("Ptr<i32>@Null");
  ParseTypeOk("Ptr<i32>@Expired");
  ParseTypeOk("Ptr<Ptr<i32>>@Valid");

  {
    ParseElemResult<std::shared_ptr<Type>> ty = ParseTypeOk("$FileSystem");
    assert(std::holds_alternative<TypeDynamic>(ty.elem->node));
  }

  {
    ParseElemResult<std::shared_ptr<Type>> ty = ParseTypeOk("File@Open");
    assert(std::holds_alternative<TypeModalState>(ty.elem->node));
  }

  {
    ParseElemResult<std::shared_ptr<Type>> ty = ParseTypeOk("Foo::Bar");
    assert(std::holds_alternative<TypePathType>(ty.elem->node));
  }

  ParseTypeOk("!");

  {
    ParseElemResult<std::shared_ptr<Type>> ty = ParseTypeOk("i32 | u32");
    assert(std::holds_alternative<TypeUnion>(ty.elem->node));
  }

  ParseTypeDiag("");

  return 0;
}
