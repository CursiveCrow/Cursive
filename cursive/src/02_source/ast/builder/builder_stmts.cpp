// ===========================================================================
// MIGRATION MAPPING: build_stmts.cpp
// ===========================================================================
//
// PURPOSE:
//   Factory functions for constructing Statement and Block AST nodes.
//   Provides builders for all 17 statement variants and the Block type
//   defined in ast_stmts.h.
//
// ===========================================================================
// SPEC REFERENCE: C:\Dev\Cursive\C0updated.md
// ===========================================================================
//   Section 3.3.7 - ParseStmt rules
//   Defines parsing rules that construct statement nodes.
//
//   Statement categories (from spec Section 6):
//   - Bindings: let, var, shadow let, shadow var
//   - Assignment: simple, compound (+=, -=, etc.)
//   - Control: return, break, continue
//   - Effect: expression, defer
//   - Scope: region, frame, unsafe, key block
//
//   Statement variants:
//   - StmtLet: Immutable binding (let x = value)
//   - StmtVar: Mutable binding (var x = value)
//   - StmtShadowLet: Shadow immutable binding (shadow let x = value)
//   - StmtShadowVar: Shadow mutable binding (shadow var x = value)
//   - StmtAssign: Simple assignment (place = value)
//   - StmtCompoundAssign: Compound assignment (place += value)
//   - StmtExpr: Expression statement (expr;)
//   - StmtDefer: Defer block (defer { ... })
//   - StmtRegion: Region block (region { ... })
//   - StmtFrame: Frame block (frame { ... })
//   - StmtUnsafe: Unsafe block (unsafe { ... })
//   - StmtReturn: Return (return value_opt)
//   - StmtBreak: Break (break value_opt)
//   - StmtContinue: Continue (continue)
//   - StmtError: Error recovery placeholder
//
// ---------------------------------------------------------------------------
// SOURCE FILE: C:\Dev\Cursive\cursive-bootstrap\src\02_syntax\parser_stmt.cpp
// ---------------------------------------------------------------------------
//   Statement construction patterns:
//   - Binding construction for let/var
//   - Block construction with statements and optional tail expression
//   - Defer/region/frame/unsafe block handling
//
// ---------------------------------------------------------------------------
// DEPENDENCIES
// ---------------------------------------------------------------------------
//   Internal:
//   - ast/nodes/ast_fwd.h: ExprPtr, PatternPtr, Identifier
//   - ast/nodes/ast_stmts.h: All statement node variant definitions, Block
//   - ast/nodes/ast_common.h: Binding helper type
//   - build_common.cpp: span_from, span_between
//   - 00_core/span.h: Span
//   - lexer/token.h: Token for binding operator
//
// ---------------------------------------------------------------------------
// REFACTORING NOTES
// ---------------------------------------------------------------------------
//   1. Binding operator (= vs :=) determines movability
//   2. Block tail expression determines block's value type
//   3. Statement terminator (newline or ;) handled by parser, not builder
//   4. Error statement for parser error recovery
//
// ===========================================================================

// TODO: Migrate implementation from source files listed above
//
// namespace cursive::ast {
//
// // ---------------------------------------------------------------------------
// // BINDING HELPER
// // ---------------------------------------------------------------------------
//
// /// Creates a binding structure for let/var statements.
// /// @param pat Binding pattern
// /// @param type_opt Optional type annotation
// /// @param op Binding operator (= for movable, := for immovable)
// /// @param init Initializer expression
// /// @param span Source location
// Binding make_binding(PatternPtr pat, shared_ptr<Type> type_opt, Token op, ExprPtr init, Span span);
//
// // ---------------------------------------------------------------------------
// // BINDING STATEMENTS
// // ---------------------------------------------------------------------------
//
// /// Creates a let statement (immutable binding).
// /// @param binding The binding structure
// /// @param span Source location
// Stmt make_stmt_let(Binding binding, Span span);
//
// /// Creates a var statement (mutable binding).
// /// @param binding The binding structure
// /// @param span Source location
// Stmt make_stmt_var(Binding binding, Span span);
//
// /// Creates a shadow let statement.
// /// Reuses name from outer scope with new immutable binding.
// /// @param name Identifier to shadow
// /// @param type_opt Optional type annotation
// /// @param init Initializer expression
// /// @param span Source location
// Stmt make_stmt_shadow_let(Identifier name, shared_ptr<Type> type_opt, ExprPtr init, Span span);
//
// /// Creates a shadow var statement.
// /// Reuses name from outer scope with new mutable binding.
// /// @param name Identifier to shadow
// /// @param type_opt Optional type annotation
// /// @param init Initializer expression
// /// @param span Source location
// Stmt make_stmt_shadow_var(Identifier name, shared_ptr<Type> type_opt, ExprPtr init, Span span);
//
// // ---------------------------------------------------------------------------
// // ASSIGNMENT STATEMENTS
// // ---------------------------------------------------------------------------
//
// /// Creates a simple assignment statement.
// /// @param place Place expression (must be mutable)
// /// @param value Value expression
// /// @param span Source location
// Stmt make_stmt_assign(ExprPtr place, ExprPtr value, Span span);
//
// /// Creates a compound assignment statement.
// /// @param place Place expression (must be mutable)
// /// @param op Compound operator (+=, -=, *=, /=, %=, &=, |=, ^=, <<=, >>=)
// /// @param value Value expression
// /// @param span Source location
// Stmt make_stmt_compound_assign(ExprPtr place, Identifier op, ExprPtr value, Span span);
//
// // ---------------------------------------------------------------------------
// // EXPRESSION AND DEFER STATEMENTS
// // ---------------------------------------------------------------------------
//
// /// Creates an expression statement.
// /// @param value Expression (value discarded unless unit)
// /// @param span Source location
// Stmt make_stmt_expr(ExprPtr value, Span span);
//
// /// Creates a defer statement.
// /// Block executes when enclosing scope exits.
// /// @param body Block to execute on scope exit
// /// @param span Source location
// Stmt make_stmt_defer(shared_ptr<Block> body, Span span);
//
// // ---------------------------------------------------------------------------
// // SCOPED BLOCK STATEMENTS
// // ---------------------------------------------------------------------------
//
// /// Creates a region statement.
// /// Establishes a memory region for arena allocation.
// /// @param opts_opt Optional region options expression
// /// @param alias Optional region alias (region as r { })
// /// @param body Block body
// /// @param span Source location
// Stmt make_stmt_region(ExprPtr opts_opt, optional<Identifier> alias, shared_ptr<Block> body, Span span);
//
// /// Creates a frame statement.
// /// Subdivides an existing region.
// /// @param body Block body
// /// @param span Source location
// Stmt make_stmt_frame(shared_ptr<Block> body, Span span);
//
// /// Creates an unsafe statement.
// /// Enables unsafe operations within block.
// /// @param body Block body
// /// @param span Source location
// Stmt make_stmt_unsafe(shared_ptr<Block> body, Span span);
//
// // ---------------------------------------------------------------------------
// // CONTROL FLOW STATEMENTS
// // ---------------------------------------------------------------------------
//
// /// Creates a return statement.
// /// @param value_opt Optional return value (nullptr for unit return)
// /// @param span Source location
// Stmt make_stmt_return(ExprPtr value_opt, Span span);
//
// /// Creates a break statement.
// /// @param value_opt Optional break value (for loop expressions)
// /// @param span Source location
// Stmt make_stmt_break(ExprPtr value_opt, Span span);
//
// /// Creates a continue statement.
// /// @param span Source location
// Stmt make_stmt_continue(Span span);
//
// // ---------------------------------------------------------------------------
// // ERROR STATEMENT
// // ---------------------------------------------------------------------------
//
// /// Creates an error statement for parser error recovery.
// /// @param span Error location
// Stmt make_stmt_error(Span span);
//
// // ---------------------------------------------------------------------------
// // BLOCK CONSTRUCTION
// // ---------------------------------------------------------------------------
//
// /// Creates a block.
// /// @param stmts Statement list
// /// @param tail_opt Optional tail expression (determines block value)
// /// @param span Source location
// Block make_block(vector<Stmt> stmts, ExprPtr tail_opt, Span span);
//
// } // namespace cursive::ast

