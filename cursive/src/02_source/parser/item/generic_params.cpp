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
// ParseGenericParamsOpt - Parse optional generic parameters
// =============================================================================
//
// SPEC: Parse-GenericParamsOpt-None
//   ¬ IsOp(Tok(P), "<")
//   ──────────────────────────────────────────────
//   Γ ⊢ ParseGenericParamsOpt(P) ⇓ (P, ⊥)
//
// SPEC: Parse-GenericParams
//   IsOp(Tok(P), "<")    Γ ⊢ ParseTypeParam(Advance(P)) ⇓ (P_1, p_1)
//   Γ ⊢ ParseTypeParamTail(P_1, [p_1]) ⇓ (P_2, ps)    IsOp(Tok(P_2), ">")
//   ────────────────────────────────────────────────────────────────────
//   Γ ⊢ ParseGenericParams(P) ⇓ (Advance(P_2), ps)
//
// CRITICAL: Parameters are separated by SEMICOLONS (;), not commas!
// This is different from generic arguments which use commas.

ParseElemResult<std::optional<GenericParams>> ParseGenericParamsOpt(
    Parser parser) {
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

  // Parse additional params separated by ; (SEMICOLON!)
  while (IsPunc(next, ";")) {
    Advance(next);
    ParseElemResult<TypeParam> param = ParseTypeParam(next);
    params.params.push_back(param.elem);
    next = param.parser;
  }

  // Detect Rust-style comma separators in generic parameter lists
  if (IsPunc(next, ",")) {
    auto diag = core::MakeDiagnosticById("E-SRC-0520", TokSpan(next));
    if (diag) {
      diag->children.push_back({core::SubDiagnosticKind::FixIt,
                                "replace `,` with `;`",
                                TokSpan(next), ";"});
      // Emit on the active parser state so the diagnostic propagates.
      core::Emit(next.diags, *diag);
    }
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

}  // namespace cursive::ast
