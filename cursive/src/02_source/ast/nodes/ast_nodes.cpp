// ===========================================================================
// MIGRATION MAPPING: ast_nodes.cpp
// ===========================================================================
//
// PURPOSE:
//   AST node utility functions. This file provides helper functions for
//   AST node traversal, inspection, and span extraction. The actual node
//   type definitions have been split into category-specific headers.
//
// ===========================================================================
// AST REORGANIZATION
// ===========================================================================
//
// The original monolithic ast.h (1300+ lines) has been split into the
// following category-based files to improve maintainability and reduce
// compilation dependencies:
//
// FOUNDATION HEADERS:
// ─────────────────────────────────────────────────────────────────────────
//   ast_fwd.h        - Forward declarations for all AST node types
//                      (Identifier, Path, ExprPtr, PatternPtr, etc.)
//
//   ast_enums.h      - All enumeration types used across AST categories
//                      (TypePerm, Visibility, RangeKind, KeyMode, etc.)
//
//   ast_common.h     - Shared helper types (Arg, FieldInit, KeyPathExpr)
//                      Types used by multiple categories
//
//   ast_attributes.h - Attribute system (AttributeArg, AttributeItem, AttributeList)
//                      C0X extension for [[attr]] annotations
//
// CATEGORY HEADERS:
// ─────────────────────────────────────────────────────────────────────────
//   ast_types.h      - Type AST nodes (15 variants)
//                      Source: ast.h lines 79-177
//                      TypePrim, TypePermType, TypeUnion, TypeFunc, TypeTuple,
//                      TypeArray, TypeSlice, TypePtr, TypeRawPtr, TypeString,
//                      TypeBytes, TypeDynamic, TypeModalState, TypePathType,
//                      TypeOpaque, TypeRefine, TypeNode, Type
//
//   ast_exprs.h      - Expression AST nodes (50+ variants)
//                      Source: ast.h lines 179-717
//                      All expression nodes including basic, operators, memory,
//                      aggregate, control flow, postfix, async, concurrency
//                      ExprNode, Expr
//
//   ast_patterns.h   - Pattern AST nodes (10 variants)
//                      Source: ast.h lines 335-417
//                      LiteralPattern, WildcardPattern, IdentifierPattern,
//                      TypedPattern, TuplePattern, RecordPattern, EnumPattern,
//                      ModalPattern, RangePattern, PatternNode, Pattern, MatchArm
//
//   ast_stmts.h      - Statement AST nodes (17 variants)
//                      Source: ast.h lines 719-852
//                      Binding, LetStmt, VarStmt, ShadowLetStmt, ShadowVarStmt,
//                      AssignStmt, CompoundAssignStmt, ExprStmt, DeferStmt,
//                      RegionStmt, FrameStmt, ReturnStmt, BreakStmt, ContinueStmt,
//                      UnsafeBlockStmt, KeyBlockStmt, StaticAssertStmt, ErrorStmt,
//                      Stmt, Block
//
//   ast_items.h      - Declaration AST nodes (30+ types)
//                      Source: ast.h lines 854-1285
//                      UsingDecl, ImportDecl, StaticDecl, ProcedureDecl,
//                      ExternBlock, RecordDecl, FieldDecl, MethodDecl,
//                      EnumDecl, VariantDecl, ModalDecl, StateBlock,
//                      ClassDecl, TypeAliasDecl, GenericParams, WhereClause,
//                      ContractClause, TypeInvariant, ASTItem
//
//   ast_module.h     - Module-level structures
//                      Source: ast.h lines 1287-1306
//                      ASTModule, ASTFile, UnsafeSpanSet
//
// INCLUDE ORDER:
// ─────────────────────────────────────────────────────────────────────────
//   #include "ast_fwd.h"         // Always first - forward declarations
//   #include "ast_enums.h"       // Enum definitions
//   #include "ast_common.h"      // Shared helper types
//   #include "ast_attributes.h"  // Attribute system
//   #include "ast_types.h"       // Type nodes
//   #include "ast_exprs.h"       // Expression nodes
//   #include "ast_patterns.h"    // Pattern nodes
//   #include "ast_stmts.h"       // Statement nodes
//   #include "ast_items.h"       // Declaration nodes
//   #include "ast_module.h"      // Module structures
//
// UMBRELLA HEADER:
// ─────────────────────────────────────────────────────────────────────────
//   An umbrella header "ast.h" can be provided for convenience:
//
//   #pragma once
//   #include "ast_fwd.h"
//   #include "ast_enums.h"
//   #include "ast_common.h"
//   #include "ast_attributes.h"
//   #include "ast_types.h"
//   #include "ast_exprs.h"
//   #include "ast_patterns.h"
//   #include "ast_stmts.h"
//   #include "ast_items.h"
//   #include "ast_module.h"
//
// ---------------------------------------------------------------------------
// SPEC REFERENCE: C:\Dev\Cursive\C0updated.md
// ---------------------------------------------------------------------------
//   Section 3.3.2 - AST Node Catalog (Lines 2620-2873)
//
//   Node utility functions (Lines 2623-2638):
//   - SpanOfNode : ASTNode -> Span              (2623)
//   - DocOf : ASTNode -> (DocList | bottom)     (2624)
//   - SpanDefault, DocDefault, DocOptDefault    (2626-2628)
//   - FillSpan, FillDoc, FillDocOpt, ParseCtor  (2629-2638)
//
// ---------------------------------------------------------------------------
// SOURCE FILE: C:\Dev\Cursive\cursive-bootstrap\include\cursive0\02_syntax\ast.h
// ---------------------------------------------------------------------------
//   Node utility functions are implicit in the struct definitions.
//   This file implements explicit utility functions for:
//   - Extracting spans from any AST node
//   - Extracting documentation from any AST node
//   - Getting node kind strings for debugging
//   - AST traversal helpers
//
// ---------------------------------------------------------------------------
// DEPENDENCIES
// ---------------------------------------------------------------------------
//   Required headers:
//   - All AST category headers (ast_types.h, ast_exprs.h, ast_patterns.h,
//     ast_stmts.h, ast_items.h, ast_module.h)
//   - core::Span (from 00_core/span.h)
//   - std::visit, std::holds_alternative
//
// ---------------------------------------------------------------------------
// REFACTORING BENEFITS
// ---------------------------------------------------------------------------
//   1. COMPILATION SPEED: Changes to one category (e.g., expressions) don't
//      trigger recompilation of code using other categories.
//
//   2. PARALLEL WITH PARSER: The AST structure now mirrors the parser
//      directory structure (expr/, stmt/, type/, pattern/, item/).
//
//   3. NAVIGATION: Easier to find specific node definitions when they're
//      organized by category rather than in one 1300-line file.
//
//   4. SCALABILITY: Adding new node types affects only the relevant file.
//
//   5. DEPENDENCY CLARITY: Each header explicitly lists its dependencies,
//      making it clear what needs to be included.
//
// ===========================================================================

// TODO: Implement AST utility functions
//
// namespace cursive::ast {
//
// // Extract span from any AST node type
// Span span_of(const Expr& e);
// Span span_of(const Type& t);
// Span span_of(const Pattern& p);
// Span span_of(const Stmt& s);
// Span span_of(const ASTItem& i);
// Span span_of(const Block& b);
//
// // Extract documentation from any AST item
// const DocList* doc_of(const ASTItem& item);
//
// // Get node kind as string for debugging
// const char* node_kind(const Expr& e);
// const char* node_kind(const Type& t);
// const char* node_kind(const Pattern& p);
// const char* node_kind(const Stmt& s);
// const char* node_kind(const ASTItem& i);
//
// // Check node category
// bool is_expression(const ExprNode& n);
// bool is_literal(const ExprNode& n);
// bool is_binary_op(const ExprNode& n);
// bool is_control_flow(const ExprNode& n);
//
// bool is_irrefutable(const PatternNode& p);
// bool is_refutable(const PatternNode& p);
//
// bool is_binding(const Stmt& s);
// bool is_control(const Stmt& s);
//
// bool is_declaration(const ASTItem& i);
// bool is_type_definition(const ASTItem& i);
//
// } // namespace cursive::ast

