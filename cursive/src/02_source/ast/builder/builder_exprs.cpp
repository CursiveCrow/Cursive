// ===========================================================================
// MIGRATION MAPPING: build_exprs.cpp
// ===========================================================================
//
// PURPOSE:
//   Factory functions for constructing Expression AST nodes. Provides builders
//   for all 50+ expression node variants defined in ast_exprs.h.
//
// ===========================================================================
// SPEC REFERENCE: C:\Dev\Cursive\C0updated.md
// ===========================================================================
//   Section 3.3.5 - ParseExpr rules
//   Defines parsing rules that construct expression nodes.
//
//   Expression categories (from spec Section 3.3.2):
//   - Literals: integers, floats, bools, chars, strings, null
//   - Identifiers: simple names, qualified names
//   - Operators: binary, unary, cast
//   - Control flow: if, match, loop variants
//   - Calls: function call, method call (~>)
//   - Access: field (.), index ([])
//   - Memory: address-of (&), dereference (*), alloc (^), move
//   - Concurrency: spawn, wait, yield, dispatch, parallel, race, all, sync
//   - Misc: block, tuple, array, record literal, widen, transmute
//
// ---------------------------------------------------------------------------
// SOURCE FILE: C:\Dev\Cursive\cursive-bootstrap\src\02_syntax\parser_expr.cpp
// ---------------------------------------------------------------------------
//   Expression construction patterns:
//   - make_shared<Expr> with ExprNode variant
//   - Binary/unary expression construction with operator
//   - Literal expression wrapping Token
//   - Call expression with argument list
//   - Method call with receiver and ~> operator
//
// ---------------------------------------------------------------------------
// DEPENDENCIES
// ---------------------------------------------------------------------------
//   Internal:
//   - ast/nodes/ast_fwd.h: ExprPtr, Identifier, Path
//   - ast/nodes/ast_enums.h: RangeKind, etc.
//   - ast/nodes/ast_common.h: Arg, MatchArm, FieldInit
//   - ast/nodes/ast_exprs.h: All expression node variant definitions
//   - build_common.cpp: span_from, span_between
//   - 00_core/span.h: Span
//   - lexer/token.h: Token for literals
//
// ---------------------------------------------------------------------------
// REFACTORING NOTES
// ---------------------------------------------------------------------------
//   1. Each builder creates an Expr wrapper containing the specific variant
//   2. Binary/unary builders should validate operator is valid for context
//   3. Consider builders for common expression patterns (e.g., self.field)
//   4. Error expression builder for parser error recovery
//
// ===========================================================================

// TODO: Migrate implementation from source files listed above
//
// namespace cursive::ast {
//
// // ---------------------------------------------------------------------------
// // ERROR AND LITERAL EXPRESSIONS
// // ---------------------------------------------------------------------------
//
// /// Creates an error expression for parser error recovery.
// /// @param span Error location
// ExprPtr make_expr_error(Span span);
//
// /// Creates a literal expression from a token.
// /// @param lit Literal token (integer, float, bool, char, string)
// /// @param span Source location
// ExprPtr make_expr_literal(Token lit, Span span);
//
// /// Creates a null pointer literal.
// /// Note: Requires type annotation context for type inference.
// /// @param span Source location
// ExprPtr make_expr_null(Span span);
//
// // ---------------------------------------------------------------------------
// // IDENTIFIER AND PATH EXPRESSIONS
// // ---------------------------------------------------------------------------
//
// /// Creates a simple identifier expression.
// /// @param name Identifier
// /// @param span Source location
// ExprPtr make_expr_identifier(Identifier name, Span span);
//
// /// Creates a qualified name expression.
// /// @param path Module path
// /// @param name Final identifier
// /// @param span Source location
// ExprPtr make_expr_qualified_name(ModulePath path, Identifier name, Span span);
//
// // ---------------------------------------------------------------------------
// // OPERATOR EXPRESSIONS
// // ---------------------------------------------------------------------------
//
// /// Creates a binary operator expression.
// /// @param op Operator identifier (+, -, *, /, etc.)
// /// @param lhs Left-hand side expression
// /// @param rhs Right-hand side expression
// /// @param span Source location
// ExprPtr make_expr_binary(Identifier op, ExprPtr lhs, ExprPtr rhs, Span span);
//
// /// Creates a unary operator expression.
// /// @param op Operator identifier (!, -, *, &)
// /// @param value Operand expression
// /// @param span Source location
// ExprPtr make_expr_unary(Identifier op, ExprPtr value, Span span);
//
// /// Creates a cast expression.
// /// @param value Expression to cast
// /// @param type Target type
// /// @param span Source location
// ExprPtr make_expr_cast(ExprPtr value, shared_ptr<Type> type, Span span);
//
// // ---------------------------------------------------------------------------
// // CONTROL FLOW EXPRESSIONS
// // ---------------------------------------------------------------------------
//
// /// Creates an if expression.
// /// @param cond Condition expression
// /// @param then_expr Then branch
// /// @param else_expr Optional else branch (nullptr for if without else)
// /// @param span Source location
// ExprPtr make_expr_if(ExprPtr cond, ExprPtr then_expr, ExprPtr else_expr, Span span);
//
// /// Creates a match expression.
// /// Note: Match arms use commas as separators.
// /// @param value Expression being matched
// /// @param arms Match arms with patterns and bodies
// /// @param span Source location
// ExprPtr make_expr_match(ExprPtr value, vector<MatchArm> arms, Span span);
//
// /// Creates an infinite loop expression.
// /// @param body Loop body block
// /// @param invariant_opt Optional loop invariant
// /// @param span Source location
// ExprPtr make_expr_loop_infinite(shared_ptr<Block> body, ExprPtr invariant_opt, Span span);
//
// /// Creates a conditional loop expression (loop condition { }).
// /// @param cond Loop condition
// /// @param body Loop body block
// /// @param invariant_opt Optional loop invariant
// /// @param span Source location
// ExprPtr make_expr_loop_cond(ExprPtr cond, shared_ptr<Block> body, ExprPtr invariant_opt, Span span);
//
// /// Creates an iterator loop expression (loop x in range { }).
// /// @param binding Loop variable pattern
// /// @param iter Iterator expression
// /// @param body Loop body block
// /// @param invariant_opt Optional loop invariant
// /// @param span Source location
// ExprPtr make_expr_loop_iter(PatternPtr binding, ExprPtr iter, shared_ptr<Block> body, ExprPtr invariant_opt, Span span);
//
// // ---------------------------------------------------------------------------
// // CALL EXPRESSIONS
// // ---------------------------------------------------------------------------
//
// /// Creates a function call expression.
// /// Note: No trailing comma on single-line argument lists.
// /// @param callee Function expression
// /// @param args Argument list
// /// @param span Source location
// ExprPtr make_expr_call(ExprPtr callee, vector<Arg> args, Span span);
//
// /// Creates a method call expression (receiver~>method(args)).
// /// @param receiver Receiver expression
// /// @param name Method name
// /// @param args Argument list
// /// @param span Source location
// ExprPtr make_expr_method_call(ExprPtr receiver, Identifier name, vector<Arg> args, Span span);
//
// // ---------------------------------------------------------------------------
// // ACCESS EXPRESSIONS
// // ---------------------------------------------------------------------------
//
// /// Creates a field access expression (expr.field).
// /// @param value Record expression
// /// @param field Field name
// /// @param span Source location
// ExprPtr make_expr_field(ExprPtr value, Identifier field, Span span);
//
// /// Creates a tuple index expression (tuple.0).
// /// @param value Tuple expression
// /// @param index Tuple index
// /// @param span Source location
// ExprPtr make_expr_tuple_index(ExprPtr value, size_t index, Span span);
//
// /// Creates an index expression (array[index]).
// /// @param value Array/slice expression
// /// @param index Index expression
// /// @param span Source location
// ExprPtr make_expr_index(ExprPtr value, ExprPtr index, Span span);
//
// /// Creates a range expression (start..end or start..=end).
// /// @param kind Range kind (exclusive or inclusive)
// /// @param start Start expression (optional)
// /// @param end End expression (optional)
// /// @param span Source location
// ExprPtr make_expr_range(RangeKind kind, ExprPtr start, ExprPtr end, Span span);
//
// // ---------------------------------------------------------------------------
// // MEMORY EXPRESSIONS
// // ---------------------------------------------------------------------------
//
// /// Creates an address-of expression (&value).
// /// @param value Expression to take address of
// /// @param span Source location
// ExprPtr make_expr_address_of(ExprPtr value, Span span);
//
// /// Creates a dereference expression (*ptr).
// /// @param value Pointer expression
// /// @param span Source location
// ExprPtr make_expr_deref(ExprPtr value, Span span);
//
// /// Creates a region allocation expression (^value).
// /// Note: Only valid inside region blocks.
// /// @param value Value to allocate
// /// @param region_opt Optional explicit region reference
// /// @param span Source location
// ExprPtr make_expr_alloc(ExprPtr value, ExprPtr region_opt, Span span);
//
// /// Creates a move expression (move value).
// /// @param value Expression to move
// /// @param span Source location
// ExprPtr make_expr_move(ExprPtr value, Span span);
//
// /// Creates a widen expression (widen modal).
// /// @param value Modal expression in specific state
// /// @param span Source location
// ExprPtr make_expr_widen(ExprPtr value, Span span);
//
// /// Creates a transmute expression (transmute<From, To>(value)).
// /// Note: Only valid inside unsafe blocks.
// /// @param value Expression to transmute
// /// @param from_type Source type
// /// @param to_type Target type
// /// @param span Source location
// ExprPtr make_expr_transmute(ExprPtr value, shared_ptr<Type> from_type, shared_ptr<Type> to_type, Span span);
//
// // ---------------------------------------------------------------------------
// // CONCURRENCY EXPRESSIONS
// // ---------------------------------------------------------------------------
//
// /// Creates a spawn expression (spawn { ... }).
// /// @param body Spawned task body
// /// @param options Optional spawn options (name, affinity, priority)
// /// @param span Source location
// ExprPtr make_expr_spawn(shared_ptr<Block> body, SpawnOptions options, Span span);
//
// /// Creates a wait expression (wait handle).
// /// Note: Keys MUST NOT be held across wait.
// /// @param value Spawned handle expression
// /// @param span Source location
// ExprPtr make_expr_wait(ExprPtr value, Span span);
//
// /// Creates a yield expression (yield value or yield release value).
// /// Note: Keys MUST NOT be held unless using release modifier.
// /// @param value Yielded value
// /// @param release Whether to release held keys
// /// @param span Source location
// ExprPtr make_expr_yield(ExprPtr value, bool release, Span span);
//
// /// Creates a yield-from expression (yield from async or yield release from async).
// /// @param value Async expression to delegate to
// /// @param release Whether to release held keys
// /// @param span Source location
// ExprPtr make_expr_yield_from(ExprPtr value, bool release, Span span);
//
// /// Creates a dispatch expression (dispatch i in range [opts] { ... }).
// /// @param binding Loop variable
// /// @param iter Range expression
// /// @param body Dispatch body
// /// @param options Dispatch options (reduce, ordered, chunk)
// /// @param key_clause Optional key clause
// /// @param span Source location
// ExprPtr make_expr_dispatch(PatternPtr binding, ExprPtr iter, shared_ptr<Block> body, DispatchOptions options, optional<KeyClause> key_clause, Span span);
//
// /// Creates a parallel expression (parallel domain { ... }).
// /// @param domain Execution domain expression
// /// @param body Parallel block body
// /// @param options Parallel options (name, cancel)
// /// @param span Source location
// ExprPtr make_expr_parallel(ExprPtr domain, shared_ptr<Block> body, ParallelOptions options, Span span);
//
// /// Creates a race expression (race { ... }).
// /// @param arms Race arms with handlers
// /// @param span Source location
// ExprPtr make_expr_race(vector<RaceArm> arms, Span span);
//
// /// Creates an all expression (all { ... }).
// /// @param arms All arms
// /// @param span Source location
// ExprPtr make_expr_all(vector<ExprPtr> arms, Span span);
//
// /// Creates a sync expression (sync async_expr).
// /// @param value Async expression to run synchronously
// /// @param span Source location
// ExprPtr make_expr_sync(ExprPtr value, Span span);
//
// // ---------------------------------------------------------------------------
// // COMPOSITE LITERAL EXPRESSIONS
// // ---------------------------------------------------------------------------
//
// /// Creates a block expression ({ stmts; tail }).
// /// @param block Block with statements and optional tail expression
// /// @param span Source location
// ExprPtr make_expr_block(shared_ptr<Block> block, Span span);
//
// /// Creates a tuple expression ((a, b)).
// /// Note: Single-element tuple requires semicolon: (a;)
// /// @param elements Tuple element expressions
// /// @param span Source location
// ExprPtr make_expr_tuple(vector<ExprPtr> elements, Span span);
//
// /// Creates an array expression ([a, b, c]).
// /// @param elements Array element expressions
// /// @param span Source location
// ExprPtr make_expr_array(vector<ExprPtr> elements, Span span);
//
// /// Creates a record literal expression (Point{ x: 1, y: 2 }).
// /// Note: No trailing comma on single-line field lists.
// /// @param path Record type path
// /// @param fields Field initializers
// /// @param span Source location
// ExprPtr make_expr_record(TypePath path, vector<FieldInit> fields, Span span);
//
// /// Creates a modal state literal (Connection@Connected{ ... }).
// /// @param path Modal type path
// /// @param state State identifier
// /// @param fields Field initializers
// /// @param span Source location
// ExprPtr make_expr_modal(TypePath path, Identifier state, vector<FieldInit> fields, Span span);
//
// // ---------------------------------------------------------------------------
// // PROPAGATION AND SPECIAL EXPRESSIONS
// // ---------------------------------------------------------------------------
//
// /// Creates a union propagation expression (expr?).
// /// @param value Expression that may be a union
// /// @param span Source location
// ExprPtr make_expr_propagate(ExprPtr value, Span span);
//
// /// Creates a key block expression (#path { ... }).
// /// @param paths Key paths to acquire
// /// @param mode Key mode (read or write)
// /// @param modifiers Key modifiers (dynamic, speculative, release)
// /// @param body Block body
// /// @param span Source location
// ExprPtr make_expr_key_block(vector<KeyPathExpr> paths, KeyMode mode, KeyModifiers modifiers, shared_ptr<Block> body, Span span);
//
// // ---------------------------------------------------------------------------
// // CONTRACT INTRINSICS
// // ---------------------------------------------------------------------------
//
// /// Creates an @result intrinsic expression (for postconditions).
// /// @param span Source location
// ExprPtr make_expr_result_intrinsic(Span span);
//
// /// Creates an @entry intrinsic expression (for postconditions).
// /// @param value Expression to capture at entry
// /// @param span Source location
// ExprPtr make_expr_entry_intrinsic(ExprPtr value, Span span);
//
// } // namespace cursive::ast

