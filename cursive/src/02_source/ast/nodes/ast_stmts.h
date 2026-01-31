// ===========================================================================
// MIGRATION MAPPING: ast_stmts.h
// ===========================================================================
//
// PURPOSE:
//   Statement AST node definitions. Contains all struct definitions for
//   statement nodes in the Cursive grammar, plus the Stmt variant and
//   Block structure.
//
// ---------------------------------------------------------------------------
// SPEC REFERENCE: C:\Dev\Cursive\C0updated.md
// ---------------------------------------------------------------------------
//   Section 3.3.2.6 - Statement Nodes (Lines referenced in 3.3.7)
//
//   Stmt variant types:
//
//   BINDING:
//     LetStmt       - let binding (let x: T = v)
//     VarStmt       - var binding (var x: T = v)
//     ShadowLetStmt - shadow let (shadow let x = v)
//     ShadowVarStmt - shadow var (shadow var x = v)
//
//   ASSIGNMENT:
//     AssignStmt         - simple assignment (x = v)
//     CompoundAssignStmt - compound assignment (x += v)
//
//   EXPRESSION:
//     ExprStmt - expression statement (expr;)
//
//   CONTROL:
//     ReturnStmt   - return statement (return v)
//     BreakStmt    - break statement (break v)
//     ContinueStmt - continue statement (continue)
//
//   RESOURCE:
//     DeferStmt  - defer statement (defer { })
//     RegionStmt - region statement (region { }, region as r { })
//     FrameStmt  - frame statement (frame { })
//
//   UNSAFE/SPECIAL:
//     UnsafeBlockStmt  - unsafe block (unsafe { })
//     KeyBlockStmt     - key block (#path mode { })
//     StaticAssertStmt - static assert (static_assert(cond))
//
//   ERROR:
//     ErrorStmt - parse error placeholder
//
//   Block structure:
//     Block = { stmts: [Stmt], tail_opt: Expr?, span: Span }
//
// ---------------------------------------------------------------------------
// SOURCE FILE: C:\Dev\Cursive\cursive-bootstrap\include\cursive0\02_syntax\ast.h
// ---------------------------------------------------------------------------
//
// 1. BINDING SUPPORT (Lines 719-725)
//    ─────────────────────────────────────────────────────────────────────────
//    struct Binding {
//      shared_ptr<Pattern> pat;
//      shared_ptr<Type> type_opt;
//      Token op;                   // = or :=
//      shared_ptr<Expr> init;
//      Span span;
//    };
//
// 2. BINDING STATEMENTS (Lines 727-749)
//    ─────────────────────────────────────────────────────────────────────────
//    struct LetStmt { Binding binding; Span span; };       (Lines 727-730)
//    struct VarStmt { Binding binding; Span span; };       (Lines 732-735)
//    struct ShadowLetStmt {                                (Lines 737-742)
//      Identifier name;
//      shared_ptr<Type> type_opt;
//      shared_ptr<Expr> init;
//      Span span;
//    };
//    struct ShadowVarStmt {                                (Lines 744-749)
//      Identifier name;
//      shared_ptr<Type> type_opt;
//      shared_ptr<Expr> init;
//      Span span;
//    };
//
// 3. ASSIGNMENT STATEMENTS (Lines 751-762)
//    ─────────────────────────────────────────────────────────────────────────
//    struct AssignStmt {                                   (Lines 751-755)
//      ExprPtr place;
//      ExprPtr value;
//      Span span;
//    };
//    struct CompoundAssignStmt {                           (Lines 757-762)
//      ExprPtr place;
//      Identifier op;              // +=, -=, *=, etc.
//      ExprPtr value;
//      Span span;
//    };
//
// 4. EXPRESSION STATEMENT (Lines 764-767)
//    ─────────────────────────────────────────────────────────────────────────
//    struct ExprStmt { ExprPtr value; Span span; };
//
// 5. RESOURCE STATEMENTS (Lines 769-785)
//    ─────────────────────────────────────────────────────────────────────────
//    struct DeferStmt {                                    (Lines 769-772)
//      shared_ptr<Block> body;
//      Span span;
//    };
//    struct RegionStmt {                                   (Lines 774-779)
//      ExprPtr opts_opt;
//      optional<Identifier> alias_opt;
//      shared_ptr<Block> body;
//      Span span;
//    };
//    struct FrameStmt {                                    (Lines 781-785)
//      optional<Identifier> target_opt;
//      shared_ptr<Block> body;
//      Span span;
//    };
//
// 6. CONTROL STATEMENTS (Lines 787-799)
//    ─────────────────────────────────────────────────────────────────────────
//    struct ReturnStmt { ExprPtr value_opt; Span span; };  (Lines 787-790)
//    struct BreakStmt { ExprPtr value_opt; Span span; };   (Lines 792-795)
//    struct ContinueStmt { Span span; };                   (Lines 797-799)
//
// 7. UNSAFE AND SPECIAL STATEMENTS (Lines 801-828)
//    ─────────────────────────────────────────────────────────────────────────
//    struct UnsafeBlockStmt {                              (Lines 801-804)
//      shared_ptr<Block> body;
//      Span span;
//    };
//    struct ErrorStmt { Span span; };                      (Lines 806-808)
//    struct StaticAssertStmt {                             (Lines 810-813)
//      ExprPtr condition;
//      Span span;
//    };
//
// 8. KEY BLOCK STATEMENT (Lines 815-828) - C0X Extension
//    ─────────────────────────────────────────────────────────────────────────
//    enum class KeyBlockMod { Dynamic, Speculative, Release };
//                                                          (Lines 816-820)
//    struct KeyBlockStmt {                                 (Lines 822-828)
//      vector<KeyPathExpr> paths;
//      vector<KeyBlockMod> mods;
//      optional<KeyMode> mode;
//      shared_ptr<Block> body;
//      Span span;
//    };
//
// 9. STMT VARIANT (Lines 830-846)
//    ─────────────────────────────────────────────────────────────────────────
//    using Stmt = variant<LetStmt, VarStmt, ShadowLetStmt, ShadowVarStmt,
//                          AssignStmt, CompoundAssignStmt, ExprStmt,
//                          DeferStmt, RegionStmt, FrameStmt,
//                          ReturnStmt, BreakStmt, ContinueStmt,
//                          UnsafeBlockStmt, KeyBlockStmt,
//                          StaticAssertStmt, ErrorStmt>;
//
// 10. BLOCK STRUCTURE (Lines 848-852)
//     ────────────────────────────────────────────────────────────────────────
//     struct Block {
//       vector<Stmt> stmts;
//       ExprPtr tail_opt;
//       Span span;
//     };
//
// ---------------------------------------------------------------------------
// DEPENDENCIES
// ---------------------------------------------------------------------------
//   From ast_fwd.h:
//     - ExprPtr, PatternPtr, Identifier
//     - Forward declaration of Block
//
//   From ast_enums.h:
//     - KeyMode, KeyBlockMod
//
//   From ast_common.h:
//     - KeyPathExpr (used by KeyBlockStmt)
//
//   From 00_core/span.h:
//     - core::Span
//
//   From lexer:
//     - Token (for Binding op)
//
// ---------------------------------------------------------------------------
// REFACTORING NOTES
// ---------------------------------------------------------------------------
//   1. Block is defined here but used by many expression types.
//      Consider moving to a shared location or fwd.h with full
//      definition in a separate file.
//
//   2. Binding struct is only used by LetStmt and VarStmt.
//      ShadowLetStmt/ShadowVarStmt have a simpler structure
//      (no pattern, just identifier).
//
//   3. KeyPathExpr is defined in exprs.h but used by KeyBlockStmt.
//      This creates a dependency. Consider moving KeyPathExpr to
//      a shared location (common.h or a dedicated keys.h).
//
//   4. The Binding.op field stores the Token for = or :=.
//      This is used to distinguish movable (=) vs immovable (:=)
//      bindings. Consider using an enum instead for clarity.
//
//   5. ExprStmt wraps any expression as a statement. The expression's
//      result is discarded. Consider adding a field to track whether
//      a warning should be emitted for unused results.
//
//   6. ContinueStmt has no value field (unlike break which can have
//      a value). This is intentional per the language design.
//
// ===========================================================================

// TODO: Migrate statement definitions from ast.h lines 719-852

