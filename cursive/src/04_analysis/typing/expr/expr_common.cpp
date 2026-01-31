// =============================================================================
// MIGRATION MAPPING: expr/expr_common.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 5.2.12: Expression Typing
//   - ExprJudg (line 9787): Expression judgments
//   - Lift-Expr (lines 9789-9792): Context lifting
//   - Place-Check (lines 9794-9797): Place expression checking
//   - StripPerm (line 9799): Permission stripping
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/types/type_infer.cpp
//   Common expression typing utilities
//
// KEY CONTENT TO MIGRATE:
//   EXPRESSION COMMON UTILITIES:
//   - Expression dispatch to specific handlers
//   - Synthesis vs checking mode
//   - Place expression handling
//   - Permission operations
//
//   EXPRESSION JUDGMENTS (ExprJudg):
//   - Gamma; R; L |- e : T (synthesis)
//   - Gamma; R; L |- e <= T (checking)
//   - Gamma; R; L |- p :place T (place synthesis)
//   - Gamma; R; L |- p <=_place T (place checking)
//   - Gamma; R; L |- r : Range (range typing)
//
//   EXPRESSION DISPATCH:
//   TypeExpr(expr, Gamma, R, L) =
//     match expr.kind {
//       Literal => literal.cpp
//       Identifier => identifier.cpp
//       Binary => binary.cpp
//       Unary => unary.cpp
//       Call => call.cpp
//       MethodCall => method_call.cpp
//       FieldAccess => field_access.cpp
//       IndexAccess => index_access.cpp
//       If => if_expr.cpp
//       Match => match_expr.cpp
//       Block => block_expr.cpp
//       Loop* => loop_*.cpp
//       ...
//     }
//
//   LIFT-EXPR:
//   Simple judgment to full judgment
//   Gamma |- e : T => Gamma; R; L |- e : T
//
//   PLACE EXPRESSIONS:
//   - Identifiers
//   - Field access (p.f)
//   - Index access (p[i])
//   - Dereference (*p)
//
//   STRIP PERMISSION:
//   StripPerm(TypePerm(p, T)) = T
//   StripPerm(T) = T (if not permission type)
//
//   CONTEXT PARAMETERS:
//   - Gamma: typing environment
//   - R: return type context
//   - L: loop context (`loop` or not)
//
// DEPENDENCIES:
//   - All expression-specific typing modules
//   - InferExpr, CheckExpr
//   - PlaceTyping
//   - PermissionOps
//
// SPEC RULES:
//   - ExprJudg (line 9787)
//   - Lift-Expr (lines 9789-9792)
//   - Place-Check (lines 9794-9797)
//
// RESULT:
//   Dispatches to specific expression handlers
//   Manages typing contexts
//
// =============================================================================

