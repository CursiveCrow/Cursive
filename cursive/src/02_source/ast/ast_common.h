// ===========================================================================
// ast_common.h - Common AST types and utilities
// ===========================================================================
//
// PURPOSE:
//   Common types and utilities shared across AST node categories. Contains
//   helper types that don't fit in a single category and are used by
//   multiple AST node headers.
//
// SPEC REFERENCE: CursiveSpecification.md Section 3.3.2 - AST Node Catalog
//
// ===========================================================================

#pragma once

#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "cursive/src/02_source/ast/nodes/ast_fwd.h"
#include "cursive/src/02_source/ast/nodes/ast_enums.h"
#include "00_core/span.h"
#include "02_source/lexer/token.h"

namespace cursive::ast {

// Path aliases, core forward declarations, and pointer aliases live in ast_fwd.h.

// ===========================================================================
// Documentation
// ===========================================================================

// Reuse DocKind and DocComment from the lexer namespace.
using DocKind = cursive::lexer::DocKind;
using DocComment = cursive::lexer::DocComment;
using DocList = std::vector<DocComment>;

using Span = cursive::core::Span;
using Token = cursive::lexer::Token;
using TokenKind = cursive::lexer::TokenKind;

// ===========================================================================
// Argument Types (shared by multiple expression kinds)
// ===========================================================================

// Arg represents a function/method call argument, possibly moved.
// Used by: CallExpr, MethodCallExpr, QualifiedApplyExpr
struct Arg {
  bool moved = false;
  ExprPtr value;
  cursive::core::Span span;
};

// ParenArgs holds positional arguments in parentheses: f(a, b, c)
struct ParenArgs {
  std::vector<Arg> args;
};

// FieldInit represents a named field initialization: name: value
// Used by: RecordExpr, BraceArgs, EnumPayloadBrace
struct FieldInit {
  Identifier name;
  ExprPtr value;
  cursive::core::Span span;
};

// BraceArgs holds named field arguments in braces: Point{ x: 1, y: 2 }
struct BraceArgs {
  std::vector<FieldInit> fields;
};

// ApplyArgs represents either paren or brace argument style
using ApplyArgs = std::variant<ParenArgs, BraceArgs>;

// ===========================================================================
// Enum Payload Types (shared by expressions and patterns)
// ===========================================================================

// Tuple-style enum payload: Variant(a, b, c)
struct EnumPayloadParen {
  std::vector<ExprPtr> elements;
};

// Record-style enum payload: Variant{ x: 1, y: 2 }
struct EnumPayloadBrace {
  std::vector<FieldInit> fields;
};

using EnumPayload = std::variant<EnumPayloadParen, EnumPayloadBrace>;

// ===========================================================================
// Key System Types (shared by expressions and statements)
// ===========================================================================
// The key system provides synchronized access to shared data.
// Key paths identify the data being accessed; key mode specifies read/write.

// KeySegField represents a field access segment in a key path: .name or #.name
struct KeySegField {
  bool marked = false;  // true if # boundary marker present
  Identifier name;
};

// KeySegIndex represents an index access segment in a key path: [expr] or #[expr]
struct KeySegIndex {
  bool marked = false;  // true if # boundary marker present
  ExprPtr expr;
};

// KeySeg is a single segment in a key path (field or index access)
using KeySeg = std::variant<KeySegField, KeySegIndex>;

// KeyPathExpr represents a complete key path: root.field1[idx].field2
// Used by: DispatchExpr (key clause), KeyBlockStmt
struct KeyPathExpr {
  bool root_marked = false;
  Identifier root;
  std::vector<KeySeg> segs;
  cursive::core::Span span;
};

// ===========================================================================
// Generic Type References (shared by expressions and types)
// ===========================================================================

// GenericTypeRef represents a type path with generic arguments: Foo<T, U>
struct GenericTypeRef {
  TypePath path;
  std::vector<TypePtr> generic_args;
};

// ModalStateRef represents a modal type in a specific state: Connection@Open<T>
struct ModalStateRef {
  TypePath path;
  std::vector<TypePtr> generic_args;
  Identifier state;
};

// ===========================================================================
// Receiver Types (shared by method declarations)
// ===========================================================================

// ReceiverShorthand represents ~, ~!, or ~% receiver syntax.
struct ReceiverShorthand {
  ReceiverPerm perm;
};

// ReceiverExplicit represents an explicit receiver with optional mode and type.
struct ReceiverExplicit {
  std::optional<ParamMode> mode_opt;
  TypePtr type;
};

using Receiver = std::variant<ReceiverShorthand, ReceiverExplicit>;

// ===========================================================================
// Quote/Splice Helpers
// ===========================================================================

struct SpliceExprNode {
  ExprPtr expr;
  cursive::core::Span span;
};

struct SpliceIdentNode {
  ExprPtr name_expr;
  cursive::core::Span span;
};

// ===========================================================================
// Loop Invariant (shared by loop expressions)
// ===========================================================================

// LoopInvariant represents: where { predicate }
// Used by: LoopInfiniteExpr, LoopConditionalExpr, LoopIterExpr
struct LoopInvariant {
  ExprPtr predicate;
  cursive::core::Span span;
};

}  // namespace cursive::ast
