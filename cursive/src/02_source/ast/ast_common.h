// ===========================================================================
// MIGRATION MAPPING: ast_common.h
// ===========================================================================
//
// PURPOSE:
//   Common types and utilities shared across AST node categories. Contains
//   helper types that don't fit in a single category and are used by
//   multiple AST node headers.
//
// ---------------------------------------------------------------------------
// SPEC REFERENCE: C:\Dev\Cursive\C0updated.md
// ---------------------------------------------------------------------------
//   Section 3.3.2 - AST Node Catalog (Lines 2620-2653)
//
//   Common definitions:
//   - DocList = [DocComment]                            (Line 2643)
//   - Path, ModulePath, TypePath, ClassPath             (Lines 2648-2651)
//   - SpanOfNode, DocOf helpers                         (Lines 2623-2624)
//
//   Section 3.3.2.1 - Doc lists and paths (Lines 2641-2653)
//   - DocComment definition
//   - Path type aliases
//
// ---------------------------------------------------------------------------
// SOURCE FILE: C:\Dev\Cursive\cursive-bootstrap\include\cursive0\02_syntax\ast.h
// ---------------------------------------------------------------------------
//
// 1. TYPE ALIASES (Lines 14-19)
//    ─────────────────────────────────────────────────────────────────────────
//    using DocList = vector<DocComment>;
//    using Identifier = string;
//    using Path = vector<Identifier>;
//    using ModulePath = Path;
//    using TypePath = Path;
//    using ClassPath = Path;
//
// 2. FORWARD DECLARATIONS (Lines 21-28)
//    ─────────────────────────────────────────────────────────────────────────
//    struct Expr;
//    struct Type;
//    struct Pattern;
//    struct Block;
//    struct LoopInvariant;
//    using ExprPtr = shared_ptr<Expr>;
//    using PatternPtr = shared_ptr<Pattern>;
//
// ---------------------------------------------------------------------------
// SOURCE FILE: C:\Dev\Cursive\cursive-bootstrap\include\cursive0\02_syntax\lexer.h
// ---------------------------------------------------------------------------
//   DocComment is defined in the lexer:
//
//   struct DocComment {
//     enum class Kind { Line, Module };
//     Kind kind;
//     string content;
//     Span span;
//   };
//
// ---------------------------------------------------------------------------
// SHARED HELPER TYPES
// ---------------------------------------------------------------------------
//   These types are used across multiple AST categories and should be
//   defined in this shared header:
//
//   1. Arg - Used by CallExpr, MethodCallExpr, QualifiedApplyExpr
//      struct Arg { bool moved; ExprPtr value; Span span; };
//
//   2. FieldInit - Used by RecordExpr, BraceArgs, EnumPayloadBrace
//      struct FieldInit { Identifier name; ExprPtr value; Span span; };
//
//   3. KeyPathExpr - Used by DispatchExpr (exprs.h) and KeyBlockStmt (stmts.h)
//      struct KeyPathExpr { Identifier root; vector<KeySeg> segs; Span span; };
//      struct KeySegField { bool marked; Identifier name; };
//      struct KeySegIndex { bool marked; ExprPtr expr; };
//      using KeySeg = variant<KeySegField, KeySegIndex>;
//
// ---------------------------------------------------------------------------
// DEPENDENCIES
// ---------------------------------------------------------------------------
//   From fwd.h:
//     - Forward declarations
//
//   From 01_core/span.h:
//     - core::Span
//
//   From lexer:
//     - DocComment (for DocList)
//
// ---------------------------------------------------------------------------
// REFACTORING NOTES
// ---------------------------------------------------------------------------
//   1. This file exists to break circular dependencies between AST headers.
//      For example, KeyPathExpr is used by both expressions (DispatchExpr)
//      and statements (KeyBlockStmt).
//
//   2. Consider moving argument-related types here:
//      - Arg, ParenArgs, BraceArgs, ApplyArgs
//      - FieldInit, FieldPattern
//      These are used by multiple expression and pattern types.
//
//   3. The Path type aliases are semantically identical but named
//      differently for clarity. Consider a strongly-typed wrapper:
//      struct ModulePath : Path { using Path::Path; };
//
//   4. DocComment is from the lexer but DocList is AST-level.
//      Consider whether this coupling is acceptable or if DocList
//      should be in the lexer module.
//
//   5. Span utilities could live here:
//      Span span_union(const Span& a, const Span& b);
//      Span span_from(const Token& start, const Token& end);
//      bool spans_overlap(const Span& a, const Span& b);
//
// ===========================================================================

// TODO: Migrate common definitions from ast.h
//
// namespace cursive::ast {
//
// // Path type aliases
// using Identifier = std::string;
// using Path = std::vector<Identifier>;
// using ModulePath = Path;
// using TypePath = Path;
// using ClassPath = Path;
//
// // Documentation
// using DocList = std::vector<lexer::DocComment>;
//
// // Shared argument types
// struct Arg {
//   bool moved = false;
//   ExprPtr value;
//   core::Span span;
// };
//
// struct FieldInit {
//   Identifier name;
//   ExprPtr value;
//   core::Span span;
// };
//
// // Key path types (shared by exprs and stmts)
// struct KeySegField {
//   bool marked;  // # boundary marker
//   Identifier name;
// };
//
// struct KeySegIndex {
//   bool marked;  // # boundary marker
//   ExprPtr expr;
// };
//
// using KeySeg = std::variant<KeySegField, KeySegIndex>;
//
// struct KeyPathExpr {
//   Identifier root;
//   std::vector<KeySeg> segs;
//   core::Span span;
// };
//
// } // namespace cursive::ast

