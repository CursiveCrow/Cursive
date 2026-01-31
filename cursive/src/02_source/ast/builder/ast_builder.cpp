// ===========================================================================
// MIGRATION MAPPING: ast_builder.cpp (INDEX FILE)
// ===========================================================================
//
// PURPOSE:
//   Central index for AST node construction utilities. The actual builder
//   implementations are split across modular files in the ast/build/ folder.
//
//   This file serves as:
//   1. Documentation index for the builder system
//   2. Central include point for all builders
//   3. Reference for spec mappings and source file origins
//
// ===========================================================================
// MODULAR BUILDER FILES (ast/build/)
// ===========================================================================
//
//   build_common.cpp   - Span construction and shared utilities
//   build_types.cpp    - Type node builders (make_type_*)
//   build_exprs.cpp    - Expression node builders (make_expr_*)
//   build_patterns.cpp - Pattern node builders (make_pattern_*)
//   build_stmts.cpp    - Statement node builders (make_stmt_*, make_block)
//   build_items.cpp    - Declaration node builders (make_*_decl)
//   build_module.cpp   - Module/file structure builders (make_ast_*)
//
// ===========================================================================
// HEADER DEPENDENCIES (Split AST Organization)
// ===========================================================================
//
// Builder files use headers from ast/nodes/:
//
//   #include "nodes/ast_fwd.h"        // Forward declarations, pointer aliases
//   #include "nodes/ast_enums.h"      // Enums (TypePerm, Visibility, etc.)
//   #include "nodes/ast_common.h"     // Shared types (Arg, FieldInit, KeyPathExpr)
//   #include "nodes/ast_attributes.h" // AttributeItem, AttributeList
//   #include "nodes/ast_types.h"      // Type node definitions
//   #include "nodes/ast_exprs.h"      // Expression node definitions
//   #include "nodes/ast_patterns.h"   // Pattern node definitions
//   #include "nodes/ast_stmts.h"      // Statement node definitions
//   #include "nodes/ast_items.h"      // Declaration node definitions
//   #include "nodes/ast_module.h"     // Module/file structure definitions
//
// BUILDER-TO-HEADER MAPPING:
// ─────────────────────────────────────────────────────────────────────────
//   build_types.cpp    -> uses nodes/ast_types.h
//   build_exprs.cpp    -> uses nodes/ast_exprs.h
//   build_patterns.cpp -> uses nodes/ast_patterns.h
//   build_stmts.cpp    -> uses nodes/ast_stmts.h
//   build_items.cpp    -> uses nodes/ast_items.h
//   build_module.cpp   -> uses nodes/ast_module.h
//
// ---------------------------------------------------------------------------
// SPEC REFERENCE: C:\Dev\Cursive\C0updated.md
// ---------------------------------------------------------------------------
//   Section 3.3.2 - AST Node Catalog
//   Lines 2626-2638: Node construction helpers
//
//   Key definitions:
//   - SpanDefault(P, P') = SpanBetween(P, P') (2626)
//   - DocDefault = [] (2627)
//   - DocOptDefault = bottom (2628)
//   - FillSpan(n, P, P') = n[span := SpanDefault(P, P')] if SpanMissing(n), else n (2629-2631)
//   - FillDoc(n) = n[doc := DocDefault] if DocMissing(n), else n (2632-2634)
//   - FillDocOpt(n) = n[doc_opt := DocOptDefault] if DocOptMissing(n), else n (2635-2637)
//   - ParseCtor(n, P, P') = FillDocOpt(FillDoc(FillSpan(n, P, P'))) (2638)
//
//   Section 3.3.4-3.3.9 - Parsing rules (for AST construction patterns)
//   Various rules specify how AST nodes are constructed during parsing:
//   - ParseExpr rules construct expression nodes (see ast_exprs.h)
//   - ParsePattern rules construct pattern nodes (see ast_patterns.h)
//   - ParseType rules construct type nodes (see ast_types.h)
//   - ParseStmt rules construct statement nodes (see ast_stmts.h)
//   - ParseItem rules construct item/declaration nodes (see ast_items.h)
//
// ---------------------------------------------------------------------------
// SOURCE FILE: C:\Dev\Cursive\cursive-bootstrap\src\02_syntax\parser.cpp
// ---------------------------------------------------------------------------
//   Lines 30-36: SpanFrom(start, end)
//     - Combines start token's start position with end token's end position
//     - Used to create spans covering multiple tokens
//
//   Lines 76-85: SpanBetween(start_parser, end_parser)
//     - Creates span between two parser positions
//     - Handles EOF token specially
//     - Used for SpanDefault in ParseCtor
//
// ---------------------------------------------------------------------------
// SOURCE FILE: C:\Dev\Cursive\cursive-bootstrap\src\02_syntax\parser_*.cpp files
// ---------------------------------------------------------------------------
//   AST construction patterns are distributed across parser files:
//
//   parser_expr.cpp: Expression construction -> ast_exprs.h types
//     - make_shared<Expr> with node and span
//     - Binary/unary expression construction with operator
//     - Literal expression wrapping Token
//
//   parser_types.cpp: Type construction -> ast_types.h types
//     - make_shared<Type> with TypeNode variant
//     - Permission-wrapped type construction
//     - Generic argument parsing into TypeApply
//
//   parser_patterns.cpp: Pattern construction -> ast_patterns.h types
//     - make_shared<Pattern> with PatternNode variant
//     - Destructuring pattern construction
//
//   parser_stmt.cpp: Statement construction -> ast_stmts.h types
//     - Binding construction for let/var
//     - Block construction with statements and optional tail expression
//
//   parser_items.cpp: Declaration construction -> ast_items.h types
//     - ProcedureDecl with all fields
//     - RecordDecl with members
//     - EnumDecl with variants
//     - Generic parameters and where clause construction
//
// ---------------------------------------------------------------------------
// DEPENDENCIES
// ---------------------------------------------------------------------------
//   Required headers (split organization):
//   - ast_fwd.h: ExprPtr, PatternPtr, Identifier, Path aliases
//   - ast_enums.h: TypePerm, Visibility, RangeKind, etc.
//   - ast_common.h: Arg, FieldInit helper types
//   - ast_types.h: All type node definitions
//   - ast_exprs.h: All expression node definitions
//   - ast_patterns.h: All pattern node definitions
//   - ast_stmts.h: All statement node definitions, Block
//   - ast_items.h: All declaration node definitions
//   - ast_module.h: ASTModule, ASTFile definitions
//
//   External dependencies:
//   - 00_core/span.h: core::Span
//   - lexer/token.h: Token for literal values
//   - parser state for SpanBetween
//
// ---------------------------------------------------------------------------
// REFACTORING NOTES (COMPLETED)
// ---------------------------------------------------------------------------
//   The builder system has been split into modular files:
//
//   1. build_common.cpp - Span construction utilities:
//      - span_from(start, end): Combines token positions
//      - span_between(start_pos, end_pos): Creates span from positions
//      - doc_default(): Returns empty documentation list
//      - fill_span(), fill_doc(), fill_doc_opt(): ParseCtor helpers
//      - Span validation functions (debug builds)
//
//   2. build_types.cpp - Type node builders:
//      - make_type_prim, make_type_path, make_type_opaque
//      - make_type_perm (permission-qualified)
//      - make_type_union, make_type_func, make_type_tuple
//      - make_type_array, make_type_slice
//      - make_type_ptr, make_type_raw_ptr
//      - make_type_string, make_type_bytes
//      - make_type_dynamic, make_type_modal_state
//      - make_type_refine
//
//   3. build_exprs.cpp - Expression node builders (50+ variants):
//      - Literals: make_expr_error, make_expr_literal, make_expr_null
//      - Names: make_expr_identifier, make_expr_qualified_name
//      - Operators: make_expr_binary, make_expr_unary, make_expr_cast
//      - Control: make_expr_if, make_expr_match, make_expr_loop_*
//      - Calls: make_expr_call, make_expr_method_call
//      - Access: make_expr_field, make_expr_index, make_expr_range
//      - Memory: make_expr_address_of, make_expr_deref, make_expr_alloc, make_expr_move
//      - Concurrency: make_expr_spawn, make_expr_wait, make_expr_yield, etc.
//      - Composites: make_expr_block, make_expr_tuple, make_expr_array, make_expr_record
//
//   4. build_patterns.cpp - Pattern node builders (10 variants):
//      - Irrefutable: make_pattern_wildcard, make_pattern_identifier, make_pattern_typed
//      - Irrefutable: make_pattern_tuple, make_pattern_record
//      - Refutable: make_pattern_literal, make_pattern_enum, make_pattern_modal
//      - Refutable: make_pattern_range, make_pattern_guard
//
//   5. build_stmts.cpp - Statement and block builders (17+ variants):
//      - Bindings: make_binding, make_stmt_let, make_stmt_var
//      - Bindings: make_stmt_shadow_let, make_stmt_shadow_var
//      - Assignment: make_stmt_assign, make_stmt_compound_assign
//      - Effect: make_stmt_expr, make_stmt_defer
//      - Scoped: make_stmt_region, make_stmt_frame, make_stmt_unsafe
//      - Control: make_stmt_return, make_stmt_break, make_stmt_continue
//      - Block: make_block
//
//   6. build_items.cpp - Declaration node builders (30+ variants):
//      - Imports: make_using_decl, make_import_decl
//      - Values: make_static_decl
//      - Procedures: make_procedure_decl
//      - Types: make_record_decl, make_enum_decl, make_modal_decl
//      - Types: make_class_decl, make_type_alias_decl
//      - Extern: make_extern_block, make_extern_procedure
//      - Helpers: make_generic_params, make_where_clause, make_contract_clause
//
//   7. build_module.cpp - Module structure builders:
//      - make_ast_file: Creates ASTFile from parsed content
//      - make_ast_module: Creates ASTModule (possibly from multiple files)
//      - merge_files_to_module: Aggregates multiple ASTFiles
//
//   Arena allocation and advanced features remain TODO:
//   - Builder class with arena reference
//   - Automatic span tracking from parser position
//   - Parallel builder for concurrent parsing
//
// ===========================================================================

// ===========================================================================
// IMPLEMENTATION STATUS
// ===========================================================================
//
// The builder API has been split into modular files in ast/build/:
//
//   build_common.cpp   -> Span utilities, ParseCtor helpers
//   build_types.cpp    -> Type node builders (16 functions)
//   build_exprs.cpp    -> Expression node builders (50+ functions)
//   build_patterns.cpp -> Pattern node builders (10 functions)
//   build_stmts.cpp    -> Statement/block builders (17 functions)
//   build_items.cpp    -> Declaration builders (15+ functions)
//   build_module.cpp   -> Module structure builders (5 functions)
//
// Each modular file contains:
//   - PURPOSE section with file description
//   - SPEC REFERENCE section with C0updated.md line numbers
//   - SOURCE FILE section with cursive-bootstrap locations
//   - DEPENDENCIES section listing required headers
//   - REFACTORING NOTES for implementation guidance
//   - Documented API skeleton in comments
//
// To implement builders:
//   1. Start with build_common.cpp (foundational utilities)
//   2. Implement category builders in dependency order:
//      - build_types.cpp (no expr dependency)
//      - build_patterns.cpp (needs types)
//      - build_exprs.cpp (needs patterns, types)
//      - build_stmts.cpp (needs exprs, patterns, types)
//      - build_items.cpp (needs stmts, types)
//      - build_module.cpp (needs items)
//
// Central header for all builders:
//   Consider creating ast/build/ast_builders.h that includes all modular
//   builders and provides the cursive::ast::build namespace.
//
// ===========================================================================

// This file now serves as an index. See ast/build/*.cpp for implementations.

