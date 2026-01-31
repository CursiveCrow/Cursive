// =============================================================================
// MIGRATION MAPPING: call_type_args.cpp
// =============================================================================
// This file contains parsing logic for function calls with explicit type
// arguments: expr<T1, T2>(args) and identifier<T1, T2>(args).
//
// SPEC REFERENCE: C0updated.md, Lines 5409-5411, 5433-5436
// -----------------------------------------------------------------------------
// Helper Predicate (Lines 5409-5411):
// CallTypeArgsStart(P) ⇔ TypeArgsStartTok(Tok(P)) ∧ (Γ ⊢ ParseGenericArgs(P) ⇓ (P_1, args)) ∧ IsPunc(Tok(P_1), "(")
//
// **(Postfix-Call-TypeArgs)** Lines 5433-5436
// CallTypeArgsStart(P)    Γ ⊢ ParseGenericArgs(P) ⇓ (P_1, targs)    IsPunc(Tok(P_1), "(")
// Γ ⊢ ParseArgList(Advance(P_1)) ⇓ (P_2, args)    IsPunc(Tok(P_2), ")")
// ──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
// Γ ⊢ PostfixStep(P, e) ⇓ (Advance(P_2), CallTypeArgs(e, targs, args))
//
// SEMANTICS:
// - Typed function calls: expr<T1, T2>(arg1, arg2, ...)
// - Type arguments come after callee expression, before value arguments
// - Requires the "<" to be followed by valid type argument list AND "("
// - This is a lookahead-requiring production (speculative parsing)
// - Used for generic procedure instantiation with explicit type args
//
// SOURCE FILE: cursive-bootstrap/src/02_syntax/parser_expr.cpp
//
// CONTENT TO MIGRATE:
// -----------------------------------------------------------------------------
// NOTE: The bootstrap implementation handles this case differently:
// - Generic procedure calls are handled in ParsePrimary (identifier<types>(args))
// - The Postfix-Call-TypeArgs rule from spec is NOT directly implemented
// - Instead, generic args are parsed when an identifier is followed by "<"
//
// 1. Generic Procedure Call (in ParsePrimary)
//    Source: parser_expr.cpp, lines 1327-1433
//    This handles identifier<T1, T2>(args) as a primary expression:
//
//    Generic args parsing (lines 1327-1353):
//    ```cpp
//    // Parse optional generic args <T1, T2, ...>
//    std::vector<std::shared_ptr<Type>> generic_args;
//    Parser cur = path.parser;
//    if (IsOp(cur, "<")) {
//      Parser after_lt = cur;
//      Advance(after_lt);  // consume <
//
//      // Parse first type arg
//      ParseElemResult<std::shared_ptr<Type>> first_arg = ParseType(after_lt);
//      generic_args.push_back(first_arg.elem);
//      cur = first_arg.parser;
//
//      // Parse additional args separated by ; or ,
//      while (IsPunc(cur, ";") || IsPunc(cur, ",")) {
//        Advance(cur);
//        ParseElemResult<std::shared_ptr<Type>> arg = ParseType(cur);
//        generic_args.push_back(arg.elem);
//        cur = arg.parser;
//      }
//
//      // Expect >
//      if (!IsOp(cur, ">")) {
//        EmitParseSyntaxErr(cur, TokSpan(cur));
//      } else {
//        Advance(cur);
//      }
//    }
//    ```
//
//    Call with generic args (lines 1412-1434):
//    ```cpp
//    // Generic procedure call: identifier<types>(args)  (§13.1.2)
//    if (IsPunc(cur, "(") && !generic_args.empty() && path.elem.size() == 1) {
//      SPEC_RULE("Parse-Generic-Procedure-Call");
//      Parser after_l = cur;
//      Advance(after_l);
//      ParseElemResult<std::vector<Arg>> args = ParseArgList(after_l);
//      if (!IsPunc(args.parser, ")")) {
//        EmitParseSyntaxErr(args.parser, TokSpan(args.parser));
//        Parser sync = args.parser;
//        SyncStmt(sync);
//        return {sync, MakeExpr(SpanBetween(parser, sync), ErrorExpr{})};
//      }
//      Parser after = args.parser;
//      Advance(after);
//      // Create identifier expression for the callee
//      IdentifierExpr ident;
//      ident.name = path.elem[0];
//      auto callee = MakeExpr(SpanBetween(start, cur), ident);
//      CallExpr call;
//      call.callee = callee;
//      call.generic_args = std::move(generic_args);
//      call.args = std::move(args.elem);
//      return {after, MakeExpr(SpanBetween(parser, after), call)};
//    }
//    ```
//
// IMPLEMENTATION NOTE:
// The spec defines Postfix-Call-TypeArgs as a postfix step (applied to any
// expression), but the bootstrap implements it only for identifiers as a
// primary expression. This may need to be revisited for full spec compliance.
//
// AST DEFINITIONS (from ast.h):
// ```cpp
// struct CallExpr {      // lines 484-488
//   ExprPtr callee;
//   std::vector<std::shared_ptr<Type>> generic_args;  // The type arguments
//   std::vector<Arg> args;
// };
// ```
//
// DEPENDENCIES:
// - Requires: ParseType (for type argument parsing)
// - Requires: ParseArgList (for value argument parsing)
// - Requires: CallExpr AST node type
// - Requires: MakeExpr, SpanCover, SpanBetween helpers
// - Requires: IsOp, IsPunc, Tok, TokSpan, Advance helpers
// - Requires: EmitParseSyntaxErr, SyncStmt for error handling
//
// REFACTORING NOTES:
// - Type args use ";" OR "," as separator (both accepted)
// - The "<" is an operator token, not punctuation
// - The ">" is also an operator token
// - Need to handle ambiguity: is "<" a type arg list or less-than operator?
// - The lookahead predicate CallTypeArgsStart(P) does speculative parsing
// - Consider implementing as true postfix step for full spec compliance
// - Current bootstrap limits this to identifier<types>(...) only
// =============================================================================
