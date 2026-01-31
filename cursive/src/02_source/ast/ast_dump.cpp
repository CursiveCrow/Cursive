// ===========================================================================
// MIGRATION MAPPING: ast_dump.cpp
// ===========================================================================
//
// PURPOSE:
//   AST pretty-printing and debugging utilities. Provides functions to
//   serialize AST nodes to human-readable text format for debugging,
//   testing, and diagnostic output.
//
// ===========================================================================
// HEADER DEPENDENCIES (Split AST Organization)
// ===========================================================================
//
// This file provides dump functions for all AST node categories:
//
//   #include "ast_fwd.h"        // Forward declarations
//   #include "ast_enums.h"      // Enums for string conversion
//   #include "ast_common.h"     // Shared types
//   #include "ast_attributes.h" // Attribute dumping
//   #include "ast_types.h"      // Type node dumping
//   #include "ast_exprs.h"      // Expression node dumping
//   #include "ast_patterns.h"   // Pattern node dumping
//   #include "ast_stmts.h"      // Statement node dumping
//   #include "ast_items.h"      // Declaration node dumping
//   #include "ast_module.h"     // Module structure dumping
//
// DUMP FUNCTION ORGANIZATION:
// ─────────────────────────────────────────────────────────────────────────
//   ENUM DUMPERS (from ast_enums.h):
//     - to_string(TypePerm) -> "const" | "unique" | "shared"
//     - to_string(Visibility) -> "public" | "internal" | "private" | "protected"
//     - to_string(RangeKind) -> ".." | "..=" | etc.
//     - to_string(KeyMode) -> "read" | "write"
//     - etc.
//
//   TYPE DUMPERS (from ast_types.h):
//     - dump(ostream&, const Type&, DumpOptions)
//     - Handles TypePrim, TypePermType, TypeUnion, TypeFunc, etc.
//
//   EXPRESSION DUMPERS (from ast_exprs.h):
//     - dump(ostream&, const Expr&, DumpOptions)
//     - Handles all 50+ expression node types
//
//   PATTERN DUMPERS (from ast_patterns.h):
//     - dump(ostream&, const Pattern&, DumpOptions)
//     - Handles all pattern node types
//
//   STATEMENT DUMPERS (from ast_stmts.h):
//     - dump(ostream&, const Stmt&, DumpOptions)
//     - dump(ostream&, const Block&, DumpOptions)
//
//   DECLARATION DUMPERS (from ast_items.h):
//     - dump(ostream&, const ASTItem&, DumpOptions)
//     - Handles all declaration node types
//
//   MODULE DUMPERS (from ast_module.h):
//     - dump(ostream&, const ASTFile&, DumpOptions)
//     - dump(ostream&, const ASTModule&, DumpOptions)
//
// ---------------------------------------------------------------------------
// SPEC REFERENCE: C:\Dev\Cursive\C0updated.md
// ---------------------------------------------------------------------------
//   No direct spec reference for AST dumping (implementation detail).
//   However, the AST structure defined in Section 3.3.2 dictates what
//   information should be included in dumps.
//
//   Section 3.3.2 - AST Node Catalog (Lines 2620-2873)
//   Defines all node types that need dump support:
//   - ASTItem variants (2682) -> ast_items.h
//   - ASTExpr variants (in ExprNode) -> ast_exprs.h
//   - ASTPattern variants (in PatternNode) -> ast_patterns.h
//   - ASTType variants (in Type) -> ast_types.h
//   - ASTStmt variants (in Stmt) -> ast_stmts.h
//
// ---------------------------------------------------------------------------
// SOURCE FILE: No direct source in cursive-bootstrap
// ---------------------------------------------------------------------------
//   The bootstrap compiler does not have a dedicated AST dump facility.
//   AST debugging is done via:
//   - CURSIVE0_DEBUG_PARSE environment variable (parser.cpp:108-132)
//   - CURSIVE0_DEBUG_PHASES environment variable (parser.cpp:160-182)
//   - Ad-hoc debug output in various parser files
//
//   Related debug output patterns:
//   - parser.cpp lines 108-126: Token-level debug output
//     * Prints index, lexeme, kind, byte values
//   - parser.cpp lines 128-132: Post-parse debug output
//     * Prints next_index, advanced status, diag count
//
// ---------------------------------------------------------------------------
// DEPENDENCIES
// ---------------------------------------------------------------------------
//   Required headers (split organization):
//   - ast_fwd.h: Forward declarations
//   - ast_enums.h: Enums that need string conversion
//   - ast_types.h: Type node definitions
//   - ast_exprs.h: Expression node definitions
//   - ast_patterns.h: Pattern node definitions
//   - ast_stmts.h: Statement node definitions
//   - ast_items.h: Declaration node definitions
//   - ast_module.h: ASTModule, ASTFile definitions
//
//   External dependencies:
//   - 00_core/span.h: Span for location information
//   - std::ostream for output
//   - std::visit for variant traversal
//
//   Required helpers:
//   - String conversion for enum values (in ast_enums.h)
//   - Span formatting utilities (in ast_common.h or 00_core/span.h)
//
// ---------------------------------------------------------------------------
// DESIGN CONSIDERATIONS
// ---------------------------------------------------------------------------
//
//   Output Formats:
//   1. Tree format (indented hierarchy):
//      ProcedureDecl
//        name: "main"
//        visibility: public
//        params:
//          Param
//            name: "ctx"
//            type: TypePath["Context"]
//        return_type: TypePrim("i32")
//        body: Block
//          stmts: []
//          tail: ReturnStmt
//            value: LiteralExpr(0)
//
//   2. S-expression format:
//      (ProcedureDecl
//        (name "main")
//        (visibility public)
//        (params (Param (name "ctx") (type (TypePath "Context"))))
//        (return_type (TypePrim "i32"))
//        (body (Block () (ReturnStmt (LiteralExpr 0)))))
//
//   3. JSON format (for tooling integration):
//      {"kind": "ProcedureDecl", "name": "main", "visibility": "public", ...}
//
//   Configuration Options:
//   - include_spans: bool - whether to print span information
//   - include_docs: bool - whether to print doc comments
//   - max_depth: int - limit recursion depth for large ASTs
//   - indent_size: int - spaces per indentation level
//
// ---------------------------------------------------------------------------
// REFACTORING NOTES
// ---------------------------------------------------------------------------
//   1. Use visitor pattern for clean variant handling:
//      struct ASTDumper {
//        ostream& out;
//        int depth = 0;
//
//        void operator()(const ProcedureDecl& d) { ... }
//        void operator()(const RecordDecl& d) { ... }
//        // ... etc for all node types
//      };
//
//   2. Consider a streaming API for large ASTs:
//      ASTDumper dumper(cout);
//      dumper.begin_node("ProcedureDecl");
//      dumper.field("name", name);
//      dumper.end_node();
//
//   3. Add round-trip testing support:
//      - Dump AST to string
//      - Parse string back to AST
//      - Compare original and reconstructed AST
//      (Requires AST equality comparison)
//
//   4. Support filtering by node kind:
//      - Dump only expressions
//      - Dump only declarations
//      - Dump specific subtree
//
//   5. Integrate with diagnostic system:
//      - AST dump as diagnostic attachment
//      - Clickable spans in IDE integration
//
//   6. Performance considerations:
//      - Use string_view where possible
//      - Pre-allocate output buffer for known sizes
//      - Consider binary dump format for large ASTs
//
//   7. Add graph output for tools:
//      - DOT format for Graphviz visualization
//      - Mermaid format for documentation
//
// ===========================================================================

// TODO: Implement AST dump functionality
//
// Suggested public API:
//
// namespace cursive::ast {
//
// struct DumpOptions {
//   bool include_spans = true;
//   bool include_docs = false;
//   int indent_size = 2;
//   int max_depth = -1;  // -1 = unlimited
// };
//
// // ─────────────────────────────────────────────────────────────────────────
// // ENUM STRING CONVERSION (from ast_enums.h)
// // ─────────────────────────────────────────────────────────────────────────
// const char* to_string(TypePerm perm);
// const char* to_string(RawPtrQual qual);
// const char* to_string(PtrState state);
// const char* to_string(StringState state);
// const char* to_string(BytesState state);
// const char* to_string(Visibility vis);
// const char* to_string(Mutability mut);
// const char* to_string(ReceiverPerm perm);
// const char* to_string(RangeKind kind);
// const char* to_string(KeyMode mode);
// const char* to_string(KeyBlockMod mod);
// const char* to_string(RaceHandlerKind kind);
// const char* to_string(ReduceOp op);
// const char* to_string(Variance var);
//
// // ─────────────────────────────────────────────────────────────────────────
// // TYPE DUMPERS (from ast_types.h)
// // ─────────────────────────────────────────────────────────────────────────
// void dump(ostream& out, const Type& type, const DumpOptions& opts = {});
// string to_string(const Type& type, const DumpOptions& opts = {});
//
// // ─────────────────────────────────────────────────────────────────────────
// // EXPRESSION DUMPERS (from ast_exprs.h)
// // ─────────────────────────────────────────────────────────────────────────
// void dump(ostream& out, const Expr& expr, const DumpOptions& opts = {});
// string to_string(const Expr& expr, const DumpOptions& opts = {});
//
// // ─────────────────────────────────────────────────────────────────────────
// // PATTERN DUMPERS (from ast_patterns.h)
// // ─────────────────────────────────────────────────────────────────────────
// void dump(ostream& out, const Pattern& pattern, const DumpOptions& opts = {});
// string to_string(const Pattern& pattern, const DumpOptions& opts = {});
//
// // ─────────────────────────────────────────────────────────────────────────
// // STATEMENT DUMPERS (from ast_stmts.h)
// // ─────────────────────────────────────────────────────────────────────────
// void dump(ostream& out, const Stmt& stmt, const DumpOptions& opts = {});
// void dump(ostream& out, const Block& block, const DumpOptions& opts = {});
// string to_string(const Stmt& stmt, const DumpOptions& opts = {});
//
// // ─────────────────────────────────────────────────────────────────────────
// // DECLARATION DUMPERS (from ast_items.h)
// // ─────────────────────────────────────────────────────────────────────────
// void dump(ostream& out, const ASTItem& item, const DumpOptions& opts = {});
// string to_string(const ASTItem& item, const DumpOptions& opts = {});
//
// // ─────────────────────────────────────────────────────────────────────────
// // MODULE DUMPERS (from ast_module.h)
// // ─────────────────────────────────────────────────────────────────────────
// void dump(ostream& out, const ASTFile& file, const DumpOptions& opts = {});
// void dump(ostream& out, const ASTModule& module, const DumpOptions& opts = {});
//
// // JSON output (for tooling)
// void dump_json(ostream& out, const ASTFile& file);
// void dump_json(ostream& out, const ASTModule& module);
//
// } // namespace cursive::ast

