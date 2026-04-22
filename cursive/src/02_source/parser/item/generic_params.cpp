// =============================================================================
// generic_params.cpp - Generic Parameter Parsing
// =============================================================================
//
// SPEC REFERENCE: CursiveSpecification.md Section 3.3.6.13 (Generic Parameter Rules)
//
// This file implements generic type parameter parsing:
//   - ParseTypeBounds: Parse type bounds <: Class1, Class2
//   - ParseTypeParam: Parse single type parameter T <: Bound = Default
//   - ParseGenericParamsOpt: Parse optional generic parameters <T; U; V>
//
// CRITICAL DISTINCTION:
//   - Generic PARAMETERS: <T; U; V>  (SEMICOLONS)
//   - Generic ARGUMENTS:  <T, U, V>  (COMMAS)
//
// =============================================================================

#include "02_source/parser/parser.h"

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "00_core/assert_spec.h"
#include "00_core/diagnostic_messages.h"

namespace cursive::ast {

// Use lexer types
using cursive::lexer::Token;
using cursive::lexer::TokenKind;

// Forward declarations for helper functions
bool IsOp(const Parser& parser, std::string_view op);
bool IsPunc(const Parser& parser, std::string_view p);

// Forward declaration for type parsing
ParseElemResult<std::shared_ptr<Type>> ParseType(Parser parser);
ParseElemResult<ClassPath> ParseClassPath(Parser parser);

namespace {

std::string GenericParamsPayload(std::string_view params_opt,
                                 std::size_t param_count) {
  std::string payload;
  payload.reserve(params_opt.size() + 56);
  payload += "params_opt=";
  payload += params_opt;
  payload += ";param_count=";
  payload += std::to_string(param_count);
  return payload;
}

void RecordGenericParamsRule(std::string_view rule_id,
                             const core::Span& span,
                             std::string_view params_opt,
                             std::size_t param_count) {
  if (!core::Conformance::Enabled()) {
    return;
  }
  core::Conformance::Record(rule_id, span,
                            GenericParamsPayload(params_opt, param_count));
}

void EmitCommaSeparatorErr(Parser& parser) {
  auto diag = core::MakeDiagnosticById("E-SRC-0520", TokSpan(parser));
  if (!diag) {
    return;
  }
  diag->children.push_back({core::SubDiagnosticKind::FixIt,
                            "replace `,` with `;`",
                            TokSpan(parser), ";"});
  core::Emit(parser.diags, *diag);
}

}  // namespace

// =============================================================================
// ParseTypeBounds - Parse type bounds: <: Class1, Class2
// =============================================================================
//
// SPEC: Parse-TypeBoundsOpt-None
//   ¬ IsOp(Tok(P), "<:")
//   ──────────────────────────────────────────────
//   Γ ⊢ ParseTypeBoundsOpt(P) ⇓ (P, [])
//
// SPEC: Parse-TypeBoundsOpt-Yes
//   IsOp(Tok(P), "<:")    Γ ⊢ ParseClassBoundList(Advance(P)) ⇓ (P_1, bounds)
//   ────────────────────────────────────────────────────────────────────
//   Γ ⊢ ParseTypeBoundsOpt(P) ⇓ (P_1, bounds)

ParseElemResult<std::vector<TypeBound>> ParseTypeBounds(Parser parser) {
  auto parse_class_bound = [&](Parser p) -> ParseElemResult<TypeBound> {
    ParseElemResult<ClassPath> class_path = ParseClassPath(p);
    Parser next = class_path.parser;

    // class_bound ::= type_path generic_args?
    // Generic arguments are consumed here to match syntax, but TypeBound
    // currently records only the class path.
    if (IsOp(next, "<")) {
      Parser arg = next;
      Advance(arg);  // consume '<'

      if (!IsOp(arg, ">")) {
        ParseElemResult<std::shared_ptr<Type>> first_arg = ParseType(arg);
        arg = first_arg.parser;
        while (IsPunc(arg, ",")) {
          Advance(arg);
          ParseElemResult<std::shared_ptr<Type>> next_arg = ParseType(arg);
          arg = next_arg.parser;
        }
      } else {
        EmitParseSyntaxErr(arg, TokSpan(arg));
      }

      if (!IsOp(arg, ">")) {
        EmitParseSyntaxErr(arg, TokSpan(arg));
      } else {
        Advance(arg);
      }
      next = arg;
    }

    TypeBound bound;
    bound.class_path = class_path.elem;
    return {next, bound};
  };

  std::vector<TypeBound> bounds;
  if (!IsOp(parser, "<:")) {
    return {parser, bounds};
  }
  Parser next = parser;
  Advance(next);  // consume <:

  // Parse first bound
  ParseElemResult<TypeBound> first_bound = parse_class_bound(next);
  bounds.push_back(first_bound.elem);
  next = first_bound.parser;

  // Parse additional bounds separated by ","
  while (IsPunc(next, ",")) {
    Advance(next);
    ParseElemResult<TypeBound> bound = parse_class_bound(next);
    bounds.push_back(bound.elem);
    next = bound.parser;
  }

  return {next, bounds};
}

// =============================================================================
// ParseTypeParam - Parse single type parameter
// =============================================================================
//
// Parses: T <: Bound = DefaultType
//
// Components:
//   - name: identifier
//   - bounds: optional <: clause
//   - default_type: optional = Type

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

// =============================================================================
// ParseTypeParamTail - Parse type parameter tail
// =============================================================================
//
// SPEC: Parse-TypeParamTail-End
//   ¬ IsPunc(Tok(P), ";")
//   ──────────────────────────────────────────────
//   Γ ⊢ ParseTypeParamTail(P, ps) ⇓ (P, ps)
//
// SPEC: Parse-TypeParamTail-Cons
//   IsPunc(Tok(P), ";")    Γ ⊢ ParseTypeParam(Advance(P)) ⇓ (P_1, p)
//   Γ ⊢ ParseTypeParamTail(P_1, ps ++ [p]) ⇓ (P_2, ps')
//   ────────────────────────────────────────────────────────────────────
//   Γ ⊢ ParseTypeParamTail(P, ps) ⇓ (P_2, ps')
//
// CRITICAL: Parameters are separated by SEMICOLONS (;), not commas.
// This is different from generic arguments which use commas.

ParseElemResult<std::vector<TypeParam>> ParseTypeParamTail(
    Parser parser,
    std::vector<TypeParam> ps) {
  if (!IsPunc(parser, ";")) {
    SPEC_RULE("Parse-TypeParamTail-End");
    return {parser, std::move(ps)};
  }

  SPEC_RULE("Parse-TypeParamTail-Cons");
  Parser next = parser;
  Advance(next);

  if (IsOp(next, ">")) {
    EmitParseSyntaxErr(parser, TokSpan(parser));
    return {next, std::move(ps)};
  }

  ParseElemResult<TypeParam> param = ParseTypeParam(next);
  ps.push_back(param.elem);
  return ParseTypeParamTail(param.parser, std::move(ps));
}

// =============================================================================
// ParseGenericParams - Parse required generic parameters
// =============================================================================
//
// SPEC: Parse-GenericParams
//   IsOp(Tok(P), "<")    Γ ⊢ ParseTypeParam(Advance(P)) ⇓ (P_1, p_1)
//   Γ ⊢ ParseTypeParamTail(P_1, [p_1]) ⇓ (P_2, ps)    IsOp(Tok(P_2), ">")
//   ────────────────────────────────────────────────────────────────────
//   Γ ⊢ ParseGenericParams(P) ⇓ (Advance(P_2), ps)

ParseElemResult<GenericParams> ParseGenericParams(Parser parser) {
  Parser start = parser;
  GenericParams params;

  if (!IsOp(parser, "<")) {
    EmitParseSyntaxErr(parser, TokSpan(parser));
    params.span = TokSpan(parser);
    SPEC_RULE("Parse-GenericParams");
    RecordGenericParamsRule("Parse-GenericParams", params.span, "required", 0);
    return {parser, params};
  }

  Parser next = parser;
  Advance(next);

  if (IsOp(next, ">")) {
    EmitParseSyntaxErr(next, TokSpan(next));
    Advance(next);
    params.span = SpanBetween(start, next);
    SPEC_RULE("Parse-GenericParams");
    RecordGenericParamsRule("Parse-GenericParams", params.span, "required", 0);
    return {next, params};
  }

  ParseElemResult<TypeParam> first = ParseTypeParam(next);
  std::vector<TypeParam> parsed;
  parsed.push_back(first.elem);
  ParseElemResult<std::vector<TypeParam>> tail =
      ParseTypeParamTail(first.parser, std::move(parsed));
  next = tail.parser;
  params.params = std::move(tail.elem);

  // Detect Rust-style comma separators in generic parameter lists.
  if (IsPunc(next, ",")) {
    EmitCommaSeparatorErr(next);
  }

  if (!IsOp(next, ">")) {
    EmitParseSyntaxErr(next, TokSpan(next));
  } else {
    Advance(next);
  }

  params.span = SpanBetween(start, next);
  SPEC_RULE("Parse-GenericParams");
  RecordGenericParamsRule("Parse-GenericParams", params.span, "required",
                          params.params.size());
  return {next, params};
}

// =============================================================================
// ParseGenericParamsOpt - Parse optional generic parameters
// =============================================================================
//
// SPEC: Parse-GenericParamsOpt-None
//   ¬ IsOp(Tok(P), "<")
//   ──────────────────────────────────────────────
//   Γ ⊢ ParseGenericParamsOpt(P) ⇓ (P, ⊥)
//
// SPEC: Parse-GenericParamsOpt-Yes
//   Γ ⊢ ParseGenericParams(P) ⇓ (P_1, params)
//   ──────────────────────────────────────────────
//   Γ ⊢ ParseGenericParamsOpt(P) ⇓ (P_1, params)

ParseElemResult<std::optional<GenericParams>> ParseGenericParamsOpt(
    Parser parser) {
  if (!IsOp(parser, "<")) {
    SPEC_RULE("Parse-GenericParamsOpt-None");
    RecordGenericParamsRule("Parse-GenericParamsOpt-None", TokSpan(parser),
                            "none", 0);
    return {parser, std::nullopt};
  }

  ParseElemResult<GenericParams> parsed = ParseGenericParams(parser);
  SPEC_RULE("Parse-GenericParamsOpt-Yes");
  RecordGenericParamsRule("Parse-GenericParamsOpt-Yes",
                          SpanBetween(parser, parsed.parser), "some",
                          parsed.elem.params.size());
  return {parsed.parser, std::move(parsed.elem)};
}

}  // namespace cursive::ast
