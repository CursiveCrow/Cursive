// =============================================================================
// MIGRATION MAPPING: expr/method_call.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 5.3.4: Method Resolution (search for T-MethodCall rules)
//   - receiver~>method(args) syntax
//   - Receiver permission must satisfy method receiver requirement
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/composite/record_methods.cpp
//   Method call typing
//
// KEY CONTENT TO MIGRATE:
//   METHOD CALL TYPING:
//   1. Type the receiver expression
//   2. Look up method in receiver type
//   3. Check receiver permission against method requirement:
//      - ~ (const): const, shared, or unique receiver OK
//      - ~! (unique): only unique receiver OK
//      - ~% (shared): shared or unique receiver OK
//   4. Substitute Self type in method signature
//   5. Check arguments via ArgsOk_T
//   6. Return method's return type (with substitution)
//
//   RECEIVER HANDLING:
//   - Auto-deref: Ptr<T>~>method() derefs to T
//   - Permission checking: receiver perm vs method receiver perm
//   - Self binding: method body sees self as receiver type
//
//   METHOD LOOKUP:
//   - Check receiver type for method
//   - Check implemented classes for method
//   - Handle generic methods with explicit type args
//
// DEPENDENCIES:
//   - ExprTypeFn for receiver and argument typing
//   - PlaceTypeFn for reference arguments
//   - LookupMethod() for method resolution
//   - PermSub() for permission checking
//   - SubstituteType() for Self replacement
//   - ArgsOk() for argument checking
//   - Monomorphize() for generic methods
//
// SPEC RULES:
//   - T-MethodCall: Method call typing
//   - MethodCall-RecvPerm-Err: Permission mismatch
//   - MethodCall-NotFound-Err: Method not found
//
// RESULT TYPE:
//   ExprTypeResult { bool ok; optional<string_view> diag_id; TypeRef type; }
//
// ERROR CASES:
//   - Method not found on receiver type
//   - Receiver permission insufficient
//   - Argument errors (count, type, move mode)
//   - Generic argument errors
//
// =============================================================================

