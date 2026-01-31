// =============================================================================
// MIGRATION MAPPING: keyword_policy.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 3.2.3 - Reserved Lexemes (lines 2086-2106)
//   Section 3.2.7 - Identifier and Keyword Lexing (ClassifyIdent at lines 2388-2392)
//
// SOURCE FILE: cursive-bootstrap/src/02_syntax/keyword_policy.cpp
//   Lines 1-101 (entire file)
//
// DEPENDENCIES:
//   - cursive/include/02_source/lexer.h (Token, TokenKind)
//   - cursive/include/02_source/ast.h (ASTFile, RecordDecl, ClassDecl, etc.)
//   - cursive/src/00_core/keywords.cpp (IsKeyword, IsFixedIdentifier)
//   - cursive/src/00_core/diagnostic_messages.cpp (MakeDiagnostic, Emit)
//
// =============================================================================
// CONTENT TO MIGRATE:
// =============================================================================
//
// TOKEN PREDICATES (source lines 15-65):
//
// 1. IsIdentTok() (lines 15-17)
//    Check if token is Identifier kind
//
// 2. IsKwTok() (lines 19-21)
//    Check if token is Keyword with specific lexeme
//
// 3. IsOpTok() (lines 23-25)
//    Check if token is Operator with specific lexeme
//
// 4. IsPuncTok() (lines 27-29)
//    Check if token is Punctuator with specific lexeme
//
// 5. LexemeText() (lines 31-33)
//    Extract lexeme string from token
//
// KEYWORD/IDENTIFIER CLASSIFICATION (lines 35-65):
//
// 6. IsKeyword() (lines 35-37)
//    Delegate to core::IsKeyword
//    Implements spec Keyword predicate from line 2094:
//    Keyword(s) = s in Reserved
//
// 7. IsFixedIdentifier() (lines 39-41)
//    Delegate to core::IsFixedIdentifier
//    NOT in spec - implementation convenience
//
// 8. IsFixedIdentTok() (lines 43-45)
//    Check token is identifier with specific lexeme
//
// 9. IsCtxKeyword() (lines 47-49)
//    Contextual keywords: "in", "key", "wait"
//    NOT reserved; identifier in most contexts
//
// 10. Ctx() (lines 51-53)
//     Check token is contextual keyword
//     Token must be Identifier AND lexeme in {in, key, wait}
//
// 11. UnionPropTok() (lines 55-57)
//     Check for "?" operator (union propagation)
//
// 12. TypeWhereTok() (lines 59-61)
//     Check for "where" as identifier (type context)
//     Note: "where" IS reserved but used contextually in type bounds
//
// 13. OpaqueTypeTok() (lines 63-65)
//     Check for "opaque" as identifier (type context)
//     Note: "opaque" is NOT reserved; used as type specifier contextually
//
// SEMANTIC VALIDATION (lines 67-98):
//
// 14. CheckMethodContext() (lines 67-98)
//     Implements spec Method-Context-Err rule
//     Validates that record/class members are valid member types:
//     - Records: MethodDecl or FieldDecl only
//     - Classes: ClassMethodDecl or ClassFieldDecl only
//     Emits E-SEM-3011 if invalid members found
//
// =============================================================================
// SPEC DEFINITIONS:
// =============================================================================
//
// Reserved keywords (line 2089):
//   {all, as, break, class, continue, dispatch, else, enum, false, defer,
//    frame, from, if, imm, import, internal, let, loop, match, modal, move,
//    mut, null, parallel, private, procedure, protected, public, race, record,
//    region, return, shadow, shared, spawn, sync, transition, transmute, true,
//    type, unique, unsafe, var, widen, where, using, yield, const, override}
//
// Contextual keywords (implementation-defined):
//   - "in" - used in loop iteration: `loop x in range`
//   - "key" - used in dispatch key clause: `dispatch i in range key path mode`
//   - "wait" - used in wait expression context
//   These are valid identifiers except in specific syntactic positions
//
// "opaque" is NOT in Reserved but has special meaning in type expressions
//
// =============================================================================
// REFACTORING NOTES:
// =============================================================================
//
// - Token predicate functions are simple wrappers; consider inlining
// - Contextual keyword handling could be centralized
// - "where" is reserved but used contextually - verify spec compliance
//
// - CheckMethodContext validates AST structure post-parsing
//   Consider: Should this be in a separate semantic validation file?
//
// - The spec mentions reserved namespace prefixes (line 2097-2099):
//   ReservedNamespacePrefix = {`cursive.`}
//   ReservedIdentPrefix = {`gen_`}
//   These are checked at Phase 3, not in lexer
//
// - UniverseProtected names (line 2102-2103) also checked at Phase 3
//
// =============================================================================

#include "cursive/include/02_source/lexer.h"

// TODO: Migrate from cursive-bootstrap/src/02_syntax/keyword_policy.cpp
