// =============================================================================
// MIGRATION MAPPING: typecheck.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 5.2: Static Semantics - Type Checking (lines 8490+)
//   Section 5.3: Declarations
//   - Module-level typechecking entry point
//   - Item typechecking dispatch
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/types/typecheck.cpp
//   Lines 1-71: Main entry point
//   Key functions:
//   - TypecheckModules(vector<Module>) -> DiagnosticList
//   - TypecheckModule(Module, Env) -> Env
//   - TypecheckItem(Item, Env) -> Env
//
// KEY CONTENT TO MIGRATE:
//   MODULE TYPECHECKING ENTRY:
//   1. Initialize global environment
//   2. For each module in topological order:
//      a. Process imports (populate environment)
//      b. Process using declarations
//      c. First pass: collect declarations (signatures)
//      d. Second pass: typecheck bodies
//   3. Return accumulated diagnostics
//
//   TWO-PASS APPROACH:
//   Pass 1 (Signature collection):
//   - Process procedure signatures without bodies
//   - Process record/enum/modal/class declarations
//   - Process type aliases
//   - Process static declarations
//   - Build environment with all visible names
//
//   Pass 2 (Body typechecking):
//   - Typecheck procedure bodies
//   - Typecheck method bodies
//   - Verify type invariants
//   - Check contracts
//
//   ITEM DISPATCH:
//   - ProcedureDecl -> procedure_decl.cpp
//   - RecordDecl -> record_decl.cpp
//   - EnumDecl -> enum_decl.cpp
//   - ModalDecl -> modal_decl.cpp
//   - ClassDecl -> class_decl.cpp
//   - TypeAlias -> type_alias_decl.cpp
//   - StaticDecl -> static_decl.cpp
//   - ExternBlock -> extern_block.cpp
//   - ImportDecl -> import_decl.cpp
//   - UsingDecl -> using_decl.cpp
//
// DEPENDENCIES:
//   - Environment (Gamma) management
//   - LowerType() for type processing
//   - All item-specific typechecking modules
//   - Diagnostic accumulation
//   - Module dependency resolution
//
// SPEC RULES:
//   - All WF-* rules for declarations
//   - All T-* rules for expressions/statements
//   - Module ordering per dependency graph
//
// RESULT:
//   Updated environment with all declarations
//   Diagnostics for any errors
//
// NOTES:
//   - Two-pass needed for forward references
//   - Topological ordering ensures dependencies processed first
//   - Diagnostics accumulated, not thrown
//
// =============================================================================

