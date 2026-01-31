// =============================================================================
// MIGRATION MAPPING: type_args.cpp
// =============================================================================
// This file should contain parsing logic for generic type arguments:
// <T1, T2, T3> used when instantiating generic types.
//
// SPEC REFERENCE: C0updated.md, Section 3.3.6.13, Lines 3936-3949
// Section 3.3.7.1 (Type Lists), Lines 4937-4962
// Section 3.3.7, Lines 4813-4816 (Parse-Type-Apply)
// =============================================================================
//
// FORMAL RULES FROM SPEC:
// -----------------------------------------------------------------------------
//
// **(Parse-GenericArgs)** Lines 3936-3939
// IsOp(Tok(P), "<")
// Γ ⊢ ParseTypeList(Advance(P)) ⇓ (P_1, args)
// IsOp(Tok(P_1), ">")
// ────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseGenericArgs(P) ⇓ (Advance(P_1), args)
//
// **(Parse-GenericArgsOpt-None)** Lines 3941-3944
// ¬ IsOp(Tok(P), "<")
// ────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseGenericArgsOpt(P) ⇓ (P, ⊥)
//
// **(Parse-GenericArgsOpt-Yes)** Lines 3946-3949
// Γ ⊢ ParseGenericArgs(P) ⇓ (P_1, args)
// ────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseGenericArgsOpt(P) ⇓ (P_1, args)
//
// TYPE LIST RULES (Lines 4939-4962):
// -----------------------------------------------------------------------------
//
// **(Parse-TypeList-Empty)** Lines 4939-4942
// IsPunc(Tok(P), ")")
// ────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseTypeList(P) ⇓ (P, [])
//
// **(Parse-TypeList-Cons)** Lines 4944-4947
// Γ ⊢ ParseType(P) ⇓ (P_1, t)
// Γ ⊢ ParseTypeListTail(P_1, [t]) ⇓ (P_2, ts)
// ────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseTypeList(P) ⇓ (P_2, ts)
//
// **(Parse-TypeListTail-End)** Lines 4949-4952
// Tok(P) ∈ {Punctuator(")"), Punctuator("}"), Operator(">")}
// ────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseTypeListTail(P, xs) ⇓ (P, xs)
//
// **(Parse-TypeListTail-TrailingComma)** Lines 4954-4957
// IsPunc(Tok(P), ",")
// Tok(Advance(P)) ∈ {Punctuator(")"), Punctuator("}"), Operator(">")}
// TrailingCommaAllowed(P_0, P, {Punctuator(")"), Punctuator("}"), Operator(">")})
// ────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseTypeListTail(P, xs) ⇓ (Advance(P), xs)
//
// **(Parse-TypeListTail-Comma)** Lines 4959-4962
// IsPunc(Tok(P), ",")
// Γ ⊢ ParseType(Advance(P)) ⇓ (P_1, t)
// Γ ⊢ ParseTypeListTail(P_1, xs ++ [t]) ⇓ (P_2, ys)
// ────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseTypeListTail(P, xs) ⇓ (P_2, ys)
//
// TYPE APPLICATION (Lines 4813-4816):
// -----------------------------------------------------------------------------
// **(Parse-Type-Apply)**
// Γ ⊢ ParseTypePath(P) ⇓ (P_1, path)
// IsOp(Tok(P_1), "<")
// Γ ⊢ ParseGenericArgs(P_1) ⇓ (P_2, args)
// ────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseNonPermType(P) ⇓ (P_2, TypeApply(path, args))
//
// SEMANTICS:
// - Generic arguments instantiate generic types: Vec<i32>, Map<string, i32>
// - Arguments use commas (,) as separators
// - Trailing commas allowed only if ">" is on different line
// - Must handle >> token splitting for nested generics
//
// =============================================================================
// SOURCE FILE: cursive-bootstrap/src/02_syntax/parser_types.cpp
// =============================================================================
//
// CONTENT TO MIGRATE:
// -----------------------------------------------------------------------------
//
// 1. ParseTypeListTail function (Lines 285-311)
//    ─────────────────────────────────────────────────────────────────────────
//    ParseElemResult<std::vector<std::shared_ptr<Type>>> ParseTypeListTail(
//        Parser parser,
//        std::vector<std::shared_ptr<Type>> xs) {
//      SkipNewlines(parser);
//      if (IsPunc(parser, ")") || IsPunc(parser, "}")) {
//        SPEC_RULE("Parse-TypeListTail-End");
//        return {parser, xs};
//      }
//      if (IsPunc(parser, ",")) {
//        const TokenKindMatch end_set[] = {MatchPunct(")"), MatchPunct("}")};
//        Parser after = parser;
//        Advance(after);
//        SkipNewlines(after);
//        if (IsPunc(after, ")") || IsPunc(after, "}")) {
//          SPEC_RULE("Parse-TypeListTail-TrailingComma");
//          EmitTrailingCommaErr(parser, end_set);
//          after.diags = parser.diags;
//          return {after, xs};
//        }
//        SPEC_RULE("Parse-TypeListTail-Comma");
//        ParseElemResult<std::shared_ptr<Type>> elem = ParseType(after);
//        xs.push_back(elem.elem);
//        return ParseTypeListTail(elem.parser, std::move(xs));
//      }
//      EmitParseSyntaxErr(parser, TokSpan(parser));
//      return {parser, xs};
//    }
//
// 2. Generic args parsing in ParseNonPermType (Lines 607-634)
//    ─────────────────────────────────────────────────────────────────────────
//    // Parse optional generic args
//    std::vector<std::shared_ptr<Type>> args;
//    Parser cur = path.parser;
//    const Token* next = Tok(cur);
//    if (next && IsOpTok(*next, "<")) {
//      SPEC_RULE("Parse-Type-Generic-Args");
//      Parser after_lt = cur;
//      Advance(after_lt);  // consume <
//
//      // Parse first type arg
//      ParseElemResult<std::shared_ptr<Type>> first_arg = ParseType(after_lt);
//      args.push_back(first_arg.elem);
//      cur = first_arg.parser;
//
//      // Parse additional args separated by ; or ,
//      while (IsPunc(cur, ";") || IsPunc(cur, ",")) {
//        Advance(cur);
//        ParseElemResult<std::shared_ptr<Type>> arg = ParseType(cur);
//        args.push_back(arg.elem);
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
//
// PARSING FLOW:
// -----------------------------------------------------------------------------
//   1. Check for "<" operator after type path
//   2. If found:
//      a. Advance past "<"
//      b. Parse first type argument
//      c. While "," (or ";" for compatibility):
//         - Advance past separator
//         - Parse next type argument
//      d. Expect ">" operator
//      e. Advance past ">"
//   3. Construct type list
//
// NOTE ON SEPARATORS:
// -----------------------------------------------------------------------------
// - Arguments use commas (,): Container<T, U>
// - Parameters use semicolons (;): record Container<T; U>
// - The source allows both for arguments, but spec requires commas
//
// DEPENDENCIES:
// =============================================================================
// - ParseType function (recursive calls for each argument)
// - ParseTypePath function (type_path.cpp)
// - MakeTypeNode helper (type_common.cpp)
// - TypeApply AST node type (path + args)
// - EmitTrailingCommaErr diagnostic helper
// - Token predicates: IsOp, IsOpTok, IsPunc
// - Parser utilities: Tok, Advance, SkipNewlines, TokSpan
//
// =============================================================================
// REFACTORING NOTES:
// =============================================================================
// - Generic args vs params: args use commas, params use semicolons
// - The source code accepts both separators; consider strictness
// - >> token splitting needed for nested generics: Vec<Vec<i32>>
// - Trailing comma validation uses standard pattern
// - TypeApply wraps a path with its type arguments
// - Consider whether to normalize the AST (TypePathType vs TypeApply)
// =============================================================================
