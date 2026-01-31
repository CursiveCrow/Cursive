// =============================================================================
// MIGRATION MAPPING: args_ok.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 5.2.4: Procedure Calls (lines 8707-8777)
//   - ArgsOkTJudg = {ArgsOk_T}
//   - Rules: ArgsT-Empty, ArgsT-Cons, ArgsT-Cons-Ref
//   - Rules: T-Call, Call-Extern-Unsafe-Err, Call-Callee-NotFunc,
//            Call-ArgCount-Err, Call-ArgType-Err, Call-Move-Missing,
//            Call-Move-Unexpected, Call-Arg-Packed-Unsafe-Err, Call-Arg-NotPlace
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/memory/calls.cpp
//   (Primary source for argument checking logic)
//
// KEY CONTENT TO MIGRATE:
//   From spec definitions (lines 8709-8717):
//   - ParamMode(param) = mode extraction
//   - ParamType(param) = type extraction
//   - ArgMoved(arg) = moved flag
//   - ArgExpr(arg) = expression
//   - PlaceType(p) = place typing
//   - ArgType(a) = computed argument type
//
//   ARGUMENT MATCHING RULES:
//   - ArgsT-Empty (line 8719-8721): Base case for empty args
//   - ArgsT-Cons (lines 8723-8726): Move parameter case
//     * moved = true required
//     * Type via MovedArg judgment
//   - ArgsT-Cons-Ref (lines 8728-8731): Reference parameter case
//     * moved = false required
//     * AddrOfOk check for place
//     * Type via place typing
//
//   ERROR RULES:
//   - Call-Callee-NotFunc (lines 8743-8746): Callee not callable
//   - Call-ArgCount-Err (lines 8748-8751): Argument count mismatch
//   - Call-ArgType-Err (lines 8753-8756): Argument type mismatch
//   - Call-Move-Missing (lines 8758-8761): Move required but not provided
//   - Call-Move-Unexpected (lines 8763-8766): Move provided but not expected
//   - Call-Arg-Packed-Unsafe-Err (lines 8768-8771): Packed field ref outside unsafe
//   - Call-Arg-NotPlace (lines 8773-8776): Non-place for ref parameter
//
// DEPENDENCIES:
//   - TypeRef for parameter and argument types
//   - Subtyping() for type compatibility checks
//   - IsPlaceExpr() for place expression detection
//   - IsInUnsafeSpan() for unsafe context checking
//   - ExprTypeFn for expression typing
//   - PlaceTypeFn for place typing
//   - PackedField() predicate for packed struct fields
//   - AddrOfOk() for address-of validity
//
// REFACTORING NOTES:
//   1. This implements the ArgsOk_T judgment from the spec
//   2. Move semantics checking is critical for memory safety
//   3. Place expression detection is used for reference parameters
//   4. Consider returning detailed error info (which arg failed, why)
//   5. Packed field checking requires struct layout information
//   6. The extern unsafe check (Call-Extern-Unsafe-Err) should be
//      handled at the call site, not in args_ok
//
// RESULT TYPE:
//   ArgsOkResult { bool ok; optional<string_view> diag_id; vector<TypeRef> arg_types; }
//
// FUNCTION SIGNATURE:
//   ArgsOkResult ArgsOk(
//       const ScopeContext& ctx,
//       const vector<TypeFuncParam>& params,
//       const vector<syntax::CallArg>& args,
//       const ExprTypeFn& type_expr,
//       const PlaceTypeFn& type_place);
//
// =============================================================================

