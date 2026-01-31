// ===========================================================================
// MIGRATION MAPPING: ast_fwd.h
// ===========================================================================
//
// PURPOSE:
//   Forward declarations for all AST node types. This header enables files
//   to reference AST types without including full definitions, reducing
//   compilation dependencies and improving build times.
//
// ---------------------------------------------------------------------------
// SPEC REFERENCE: C:\Dev\Cursive\C0updated.md
// ---------------------------------------------------------------------------
//   Section 3.3.2 - AST Node Catalog (Lines 2620-2873)
//   Provides the complete list of AST node types to forward-declare:
//   - ASTItem variants (2682)
//   - ASTExpr variants (ExprNode)
//   - ASTPattern variants (PatternNode)
//   - ASTType variants (Type)
//   - ASTStmt variants (Stmt)
//
// ---------------------------------------------------------------------------
// SOURCE FILE: C:\Dev\Cursive\cursive-bootstrap\include\cursive0\02_syntax\ast.h
// ---------------------------------------------------------------------------
//   Lines 21-28: Forward declarations in original header
//     struct Expr;
//     struct Type;
//     struct Pattern;
//     struct Block;
//     struct LoopInvariant;
//     using ExprPtr = std::shared_ptr<Expr>;
//     using PatternPtr = std::shared_ptr<Pattern>;
//
// ---------------------------------------------------------------------------
// CONTENT TO MIGRATE
// ---------------------------------------------------------------------------
//
// 1. COMMON TYPE ALIASES (Lines 14-19)
//    - DocList, Identifier, Path, ModulePath, TypePath, ClassPath
//
// 2. FORWARD DECLARATIONS (Lines 21-28)
//    - struct Expr, Type, Pattern, Block, LoopInvariant
//    - ExprPtr, PatternPtr smart pointer aliases
//
// 3. ADD NEW FORWARD DECLARATIONS FOR ALL NODE TYPES
//    Types (from types.h):
//      TypePrim, TypePermType, TypeUnion, TypeFunc, TypeFuncParam,
//      TypeTuple, TypeArray, TypeSlice, TypePtr, TypeRawPtr,
//      TypeString, TypeBytes, TypeDynamic, TypeModalState,
//      TypePathType, TypeOpaque, TypeRefine
//
//    Expressions (from exprs.h):
//      ErrorExpr, LiteralExpr, IdentifierExpr, QualifiedNameExpr,
//      QualifiedApplyExpr, PathExpr, RangeExpr, BinaryExpr, CastExpr,
//      UnaryExpr, DerefExpr, AddressOfExpr, MoveExpr, AllocExpr,
//      PtrNullExpr, TupleExpr, ArrayExpr, ArrayRepeatExpr, SizeofExpr,
//      AlignofExpr, RecordExpr, EnumLiteralExpr, IfExpr, MatchExpr,
//      LoopInfiniteExpr, LoopConditionalExpr, LoopIterExpr, BlockExpr,
//      UnsafeBlockExpr, AttributedExpr, TransmuteExpr, FieldAccessExpr,
//      TupleAccessExpr, IndexAccessExpr, CallExpr, MethodCallExpr,
//      PropagateExpr, ResultExpr, EntryExpr, YieldExpr, YieldFromExpr,
//      SyncExpr, RaceExpr, AllExpr, ParallelExpr, SpawnExpr, WaitExpr,
//      DispatchExpr
//
//    Patterns (from patterns.h):
//      LiteralPattern, WildcardPattern, IdentifierPattern, TypedPattern,
//      TuplePattern, FieldPattern, RecordPattern, EnumPattern,
//      ModalPattern, RangePattern
//
//    Statements (from stmts.h):
//      Binding, LetStmt, VarStmt, ShadowLetStmt, ShadowVarStmt,
//      AssignStmt, CompoundAssignStmt, ExprStmt, DeferStmt,
//      RegionStmt, FrameStmt, ReturnStmt, BreakStmt, ContinueStmt,
//      UnsafeBlockStmt, KeyBlockStmt, StaticAssertStmt, ErrorStmt
//
//    Items/Declarations (from items.h):
//      UsingDecl, ImportDecl, ExternBlock, ExternProcDecl, StaticDecl,
//      ProcedureDecl, RecordDecl, FieldDecl, MethodDecl, EnumDecl,
//      VariantDecl, ModalDecl, StateBlock, ClassDecl, TypeAliasDecl,
//      ErrorItem
//
// ---------------------------------------------------------------------------
// DEPENDENCIES
// ---------------------------------------------------------------------------
//   - <memory> for shared_ptr forward declaration
//   - <string> for std::string in aliases
//   - <vector> for std::vector in aliases
//
// ---------------------------------------------------------------------------
// REFACTORING NOTES
// ---------------------------------------------------------------------------
//   1. This file should be the minimal include for code that only needs
//      to pass around AST pointers without accessing their contents.
//
//   2. Include order for AST files should be:
//      ast_fwd.h -> ast_enums.h -> ast_common.h -> (category headers)
//
//   3. The *Ptr type aliases should be defined here for consistency:
//      - ExprPtr = shared_ptr<Expr>
//      - PatternPtr = shared_ptr<Pattern>
//      - TypePtr = shared_ptr<Type>
//      - BlockPtr = shared_ptr<Block>
//
//   4. Consider using an opaque ID type instead of shared_ptr for
//      better memory locality in an arena-based AST:
//      - ExprId = uint32_t index into expression arena
//      - TypeId = uint32_t index into type arena
//
// ===========================================================================

// TODO: Implement forward declarations
//
// namespace cursive::ast {
//
// // Core wrapper types (forward-declared)
// struct Expr;
// struct Type;
// struct Pattern;
// struct Block;
//
// // Smart pointer aliases
// using ExprPtr = std::shared_ptr<Expr>;
// using PatternPtr = std::shared_ptr<Pattern>;
// using TypePtr = std::shared_ptr<Type>;
// using BlockPtr = std::shared_ptr<Block>;
//
// // Path aliases
// using Identifier = std::string;
// using Path = std::vector<Identifier>;
// using ModulePath = Path;
// using TypePath = Path;
// using ClassPath = Path;
//
// // Documentation
// struct DocComment;  // from lexer
// using DocList = std::vector<DocComment>;
//
// } // namespace cursive::ast

