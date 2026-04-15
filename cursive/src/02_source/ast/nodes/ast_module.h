// ===========================================================================
// ast_module.h - Module and File-Level AST Structures
// ===========================================================================
//
// PURPOSE:
//   Module and file-level AST structures. Contains ASTModule and ASTFile
//   definitions that represent complete parsed compilation units, along
//   with supporting types like UnsafeSpanSet.
//
// SPEC REFERENCE: C:\Dev\Cursive\CursiveSpecification.md Section 3.3.2.2
//   ASTModule = (path: ModulePath, items: [ASTItem], module_doc: DocList)
//   ASTFile = (path: Path, items: [ASTItem], module_doc: DocList)
//
// ===========================================================================

#pragma once

#include <string>
#include <variant>
#include <vector>

#include "00_core/span.h"
#include "cursive/src/02_source/ast/ast_common.h"
#include "cursive/src/02_source/ast/nodes/ast_items.h"

namespace cursive::ast {

// ===========================================================================
// ASTItem Variant (Forward Reference)
// ===========================================================================
// ASTItem is defined in ast_items.h but forward-declared here for use in
// ASTModule and ASTFile. The full variant includes:
// UsingDecl, ImportDecl, ExternBlock, StaticDecl, ProcedureDecl,
// ComptimeProcedureDecl, RecordDecl, EnumDecl, ModalDecl,
// ClassDecl, TypeAliasDecl, DeriveTargetDecl, ErrorItem

using ASTItem = std::variant<
    UsingDecl,
    ImportDecl,
    ExternBlock,
    StaticDecl,
    ProcedureDecl,
    ComptimeProcedureDecl,
    RecordDecl,
    EnumDecl,
    ModalDecl,
    ClassDecl,
    TypeAliasDecl,
    DeriveTargetDecl,
    ErrorItem>;

// ===========================================================================
// Unsafe Span Tracking
// ===========================================================================
// UnsafeSpanSet tracks the locations of unsafe blocks within a single source
// file. This information is used by:
// - The C0 conformance checker (to warn about unsafe usage)
// - The safety analyzer (to track unsafe context)
// - The documentation generator (to annotate unsafe code)

/// UnsafeSpanSet: tracks unsafe block locations in a single file
/// - path: file system path to the source file
/// - spans: locations of unsafe { } blocks within that file
struct UnsafeSpanSet {
  std::string path;
  std::vector<cursive::core::Span> spans;
};

// ===========================================================================
// ASTModule
// ===========================================================================
// ASTModule represents a logical module (possibly spanning multiple files).
// Produced by the module loader that aggregates files belonging to the same
// module path in the namespace hierarchy.

/// ASTModule: a logical module in the namespace hierarchy
/// - path: the module path (e.g., ["mylib", "utils"])
/// - items: merged top-level declarations from all files in the module
/// - module_doc: merged //! module documentation comments
/// - unsafe_spans: unsafe block locations grouped by source file
struct ASTModule {
  Path path;
  std::vector<ASTItem> items;
  DocList module_doc;
  std::vector<UnsafeSpanSet> unsafe_spans;
};

// ===========================================================================
// ASTFile
// ===========================================================================
// ASTFile represents a single source file after parsing. Produced by the
// parser for each source file processed.

/// ASTFile: a single parsed source file
/// - path: source file path segments
/// - items: parsed top-level declarations in order of appearance
/// - module_doc: accumulated //! comments at the start of the file
/// - unsafe_spans: locations of unsafe { } blocks within the file
struct ASTFile {
  Path path;
  std::vector<ASTItem> items;
  DocList module_doc;
  std::vector<cursive::core::Span> unsafe_spans;
};

}  // namespace cursive::ast
