// =============================================================================
// MIGRATION MAPPING: tuple_type.cpp
// =============================================================================
// This file should contain parsing logic for tuple types: (T1, T2, ...) and
// single-element tuples (T;).
//
// SPEC REFERENCE: C0updated.md, Section 3.3.7, Lines 4758-4761
// Section 3.3.7.1 (Tuple Type Elements), Lines 4825-4845
// =============================================================================
//
// FORMAL RULES FROM SPEC:
// -----------------------------------------------------------------------------
//
// **(Parse-Tuple-Type)** Lines 4758-4761
// IsPunc(Tok(P), "(")
// Γ ⊢ ParseTupleTypeElems(Advance(P)) ⇓ (P_1, elems)
// elems ≠ []
// IsPunc(Tok(P_1), ")")
// ────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseNonPermType(P) ⇓ (Advance(P_1), TypeTuple(elems))
//
// TUPLE TYPE ELEMENT RULES (Lines 4827-4845):
// -----------------------------------------------------------------------------
//
// **(Parse-TupleTypeElems-Empty)** Lines 4827-4830
// IsPunc(Tok(P), ")")
// ────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseTupleTypeElems(P) ⇓ (P, [])
//
// **(Parse-TupleTypeElems-One)** Lines 4832-4835
// Γ ⊢ ParseType(P) ⇓ (P_1, t)    IsPunc(Tok(P_1), ";")
// ────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseTupleTypeElems(P) ⇓ (Advance(P_1), [t])
//
// **(Parse-TupleTypeElems-TrailingComma)** Lines 4837-4840
// Γ ⊢ ParseType(P) ⇓ (P_1, t)
// IsPunc(Tok(P_1), ",")
// IsPunc(Tok(Advance(P_1)), ")")
// TrailingCommaAllowed(P_0, P_1, {Punctuator(")")})
// ────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseTupleTypeElems(P) ⇓ (Advance(P_1), [t])
//
// **(Parse-TupleTypeElems-Many)** Lines 4842-4845
// Γ ⊢ ParseType(P) ⇓ (P_1, t_1)
// IsPunc(Tok(P_1), ",")
// Γ ⊢ ParseType(Advance(P_1)) ⇓ (P_2, t_2)
// Γ ⊢ ParseTypeListTail(P_2, [t_2]) ⇓ (P_3, ts)
// ────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseTupleTypeElems(P) ⇓ (P_3, [t_1] ++ ts)
//
// SEMANTICS:
// - Tuple types group multiple types: (i32, bool, string@View)
// - Single-element tuple requires semicolon: (i32;) to distinguish from parens
// - Empty parentheses () produce unit type, NOT empty tuple (see primitive_type)
// - Trailing comma allowed only if closing paren on different line
//
// =============================================================================
// SOURCE FILE: cursive-bootstrap/src/02_syntax/parser_types.cpp
// =============================================================================
//
// CONTENT TO MIGRATE:
// -----------------------------------------------------------------------------
//
// 1. ParseTupleTypeElems function (Lines 314-353)
//    ─────────────────────────────────────────────────────────────────────────
//    Lines 316-319: Empty case
//      SkipNewlines(parser);
//      if (IsPunc(parser, ")")) {
//        SPEC_RULE("Parse-TupleTypeElems-Empty");
//        return {parser, {}};
//      }
//
//    Lines 320-328: Single-element tuple (semicolon)
//      ParseElemResult<std::shared_ptr<Type>> first = ParseType(parser);
//      Parser after_first = first.parser;
//      SkipNewlines(after_first);
//      if (IsPunc(after_first, ";")) {
//        SPEC_RULE("Parse-TupleTypeElems-One");
//        Parser after = after_first;
//        Advance(after);
//        return {after, {first.elem}};
//      }
//
//    Lines 330-340: Trailing comma case
//      if (IsPunc(after_first, ",")) {
//        const TokenKindMatch end_set[] = {MatchPunct(")")};
//        Parser after = after_first;
//        Advance(after);
//        SkipNewlines(after);
//        if (IsPunc(after, ")")) {
//          SPEC_RULE("Parse-TupleTypeElems-TrailingComma");
//          EmitTrailingCommaErr(after_first, end_set);
//          after.diags = after_first.diags;
//          return {after, {first.elem}};
//        }
//
//    Lines 341-350: Multiple elements case
//        SPEC_RULE("Parse-TupleTypeElems-Many");
//        ParseElemResult<std::shared_ptr<Type>> second = ParseType(after);
//        ParseElemResult<std::vector<std::shared_ptr<Type>>> tail =
//            ParseTypeListTail(second.parser, {second.elem});
//        std::vector<std::shared_ptr<Type>> elems;
//        elems.reserve(1 + tail.elem.size());
//        elems.push_back(first.elem);
//        elems.insert(elems.end(), tail.elem.begin(), tail.elem.end());
//        return {tail.parser, elems};
//      }
//
//    Lines 351-352: Error case
//      EmitParseSyntaxErr(after_first, TokSpan(after_first));
//      return {after_first, {first.elem}};
//
// 2. Tuple type construction in ParseNonPermType (Lines 452-458)
//    ─────────────────────────────────────────────────────────────────────────
//    SPEC_RULE("Parse-Tuple-Type");
//    Parser after_r = elems.parser;
//    Advance(after_r);
//    TypeTuple tuple;
//    tuple.elements = std::move(elems.elem);
//    return {after_r, MakeTypeNode(SpanBetween(parser, after_r), tuple)};
//
// 3. Main tuple/unit dispatch in ParseNonPermType (Lines 426-458)
//    ─────────────────────────────────────────────────────────────────────────
//    if (IsPuncTok(*tok, "(")) {
//      if (HasFuncArrow(parser)) {
//        return ParseFuncType(parser);
//      }
//      Parser after = parser;
//      Advance(after);
//      ParseElemResult<std::vector<std::shared_ptr<Type>>> elems =
//          ParseTupleTypeElems(after);
//      // ... (unit type case at lines 434-446)
//      // ... (error check at lines 447-451)
//      // ... (tuple construction at lines 452-458)
//    }
//
// DEPENDENCIES:
// =============================================================================
// - ParseType function (recursive call for elements)
// - ParseTypeListTail function (type_args.cpp)
// - HasFuncArrow predicate (function_type.cpp)
// - MakeTypeNode helper (type_common.cpp)
// - TypeTuple AST node type
// - EmitTrailingCommaErr diagnostic
// - Token predicates: IsPunc, IsPuncTok
// - Parser utilities: Tok, Advance, SkipNewlines, SpanBetween, TokSpan
//
// =============================================================================
// REFACTORING NOTES:
// =============================================================================
// - Parentheses starting types require lookahead to distinguish:
//   1. Function type: (T1, T2) -> R
//   2. Tuple type: (T1, T2)
//   3. Single-element tuple: (T;)
//   4. Unit type: ()
// - HasFuncArrow performs lookahead to detect function types
// - SkipNewlines is called to allow multiline tuple types
// - Trailing comma validation uses EmitTrailingCommaErr
// - Consider refactoring HasFuncArrow to be more efficient
// =============================================================================
