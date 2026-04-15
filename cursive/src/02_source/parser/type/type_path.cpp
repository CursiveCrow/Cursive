// =============================================================================
// type_path.cpp - Type Path Parsing with Generic Arguments
// =============================================================================
//
// SPEC REFERENCE: CursiveSpecification.md, Section 3.3.6.13, Lines 3786-3805
// Section 3.3.7, Lines 4818-4821 (Parse-Type-Path rule)
//
// Parses type paths with optional generic arguments: MyType<T, U>
// Note: ParseTypePath (without generics) is defined in parser_paths.cpp.
// This file handles the generic argument parsing for types.
//
// =============================================================================

#include "02_source/parser/type/type_parse_internal.h"

#include "00_core/assert_spec.h"
#include "00_core/diagnostic_messages.h"

namespace cursive::ast {

namespace {

bool IsGpuPtrHead(const TypePath& path) {
  return path.size() == 1 && path.front() == "GpuPtr";
}

bool IsGpuPtrAddrSpaceArg(const std::shared_ptr<Type>& arg) {
  if (!arg) {
    return false;
  }
  const auto* path = std::get_if<TypePathType>(&arg->node);
  if (!path || !path->generic_args.empty() || path->path.size() != 1) {
    return false;
  }
  return path->path[0] == "Global" || path->path[0] == "Shared" ||
         path->path[0] == "Private";
}

void ValidateGpuPtrArgs(Parser parser,
                        const TypePath& path,
                        const std::vector<std::shared_ptr<Type>>& args) {
  if (!IsGpuPtrHead(path) || args.empty()) {
    return;
  }
  if (args.size() != 2 || !IsGpuPtrAddrSpaceArg(args[1])) {
    EmitParseSyntaxErr(parser, TokSpan(parser));
  }
}

}  // namespace

// =============================================================================
// ParseGenericTypeArgs - Parse Optional Generic Type Arguments <T, U>
// =============================================================================
// SPEC: Generic arguments use commas to separate types.
// Returns the parser positioned after '>' and the vector of type arguments.
// If no '<' is present, returns empty vector and unchanged parser.

ParseGenericArgsResult ParseGenericTypeArgs(Parser parser) {
  std::vector<std::shared_ptr<Type>> args;

  const Token* tok = Tok(parser);
  if (!tok || !IsOpTok(*tok, "<")) {
    return {parser, args};
  }

  SPEC_RULE("Parse-Type-Generic-Args");
  Parser after_lt = parser;
  Advance(after_lt);  // consume '<'

  // Parse first type arg
  ParseElemResult<std::shared_ptr<Type>> first_arg = ParseType(after_lt);
  args.push_back(first_arg.elem);
  const EndSetToken end_set[] = {EndOperator(">"), EndOperator(">>")};
  ParseElemResult<std::vector<std::shared_ptr<Type>>> tail =
      ParseTypeListTailWithEndSet(first_arg.parser, std::move(args), end_set);
  Parser cur = tail.parser;
  args = std::move(tail.elem);

  // Accept either ">" or a split-able ">>" close token.
  if (IsOpType(cur, ">>")) {
    cur = SplitShiftR(cur);
  }

  if (!IsOpType(cur, ">")) {
    EmitParseSyntaxErr(cur, TokSpan(cur));
  } else {
    Advance(cur);
  }

  return {cur, args};
}

// =============================================================================
// ParseTypePathType - Parse Type Path or Type Apply
// =============================================================================
// SPEC: Lines 4818-4821
// Parses TypePath<Args> and returns TypePath or TypeApply.
// Does not handle @State suffix (that's in state_specific_type.cpp).

ParseElemResult<std::shared_ptr<Type>> ParseTypePathType(
    Parser parser,
    const Parser& start,
    TypePath path) {
  // Parse optional generic arguments
  ParseGenericArgsResult gen = ParseGenericTypeArgs(parser);
  if (!gen.args.empty()) {
    ValidateGpuPtrArgs(gen.parser, path, gen.args);
    SPEC_RULE("Parse-Type-Apply");
    TypeApply apply;
    apply.path = std::move(path);
    apply.args = std::move(gen.args);
    return {gen.parser, MakeTypeNode(SpanBetween(start, gen.parser), apply)};
  }

  SPEC_RULE("Parse-Type-Path");
  TypePathType ty_path;
  ty_path.path = std::move(path);
  return {gen.parser, MakeTypeNode(SpanBetween(start, gen.parser), ty_path)};
}

}  // namespace cursive::ast
