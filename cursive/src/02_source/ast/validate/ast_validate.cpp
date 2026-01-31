// ===========================================================================
// MIGRATION MAPPING: ast_validate.cpp (INDEX FILE)
// ===========================================================================
//
// PURPOSE:
//   Central index for AST validation utilities. The actual validation
//   implementations are split across modular files in the ast/validate/ folder.
//
//   This file serves as:
//   1. Documentation index for the validation system
//   2. Central include point for all validators
//   3. Reference for spec mappings and source file origins
//
// ===========================================================================
// MODULAR VALIDATION FILES (ast/validate/)
// ===========================================================================
//
//   validate_common.cpp   - Common infrastructure (ValidationResult, context, visitor base)
//   validate_types.cpp    - Type node validation (permission, union, etc.)
//   validate_exprs.cpp    - Expression node validation (operators, control flow, etc.)
//   validate_patterns.cpp - Pattern node validation (irrefutability, bindings)
//   validate_stmts.cpp    - Statement/block validation (bindings, control flow)
//   validate_items.cpp    - Declaration validation (method context, generics)
//   validate_module.cpp   - Module/file validation (doc attachment)
//   validate_names.cpp    - Name validation (reserved keywords, namespaces)
//   validate_c0.cpp       - C0 subset conformance validation
//
// ===========================================================================
// HEADER DEPENDENCIES (Split AST Organization)
// ===========================================================================
//
// Validation files use headers from ast/nodes/:
//
//   #include "nodes/ast_fwd.h"        // Forward declarations
//   #include "nodes/ast_enums.h"      // Enums for validation
//   #include "nodes/ast_common.h"     // Shared types
//   #include "nodes/ast_attributes.h" // Attribute validation
//   #include "nodes/ast_types.h"      // Type node definitions
//   #include "nodes/ast_exprs.h"      // Expression node definitions
//   #include "nodes/ast_patterns.h"   // Pattern node definitions
//   #include "nodes/ast_stmts.h"      // Statement node definitions
//   #include "nodes/ast_items.h"      // Declaration node definitions
//   #include "nodes/ast_module.h"     // Module structure definitions
//
// VALIDATOR-TO-HEADER MAPPING:
// ─────────────────────────────────────────────────────────────────────────
//   validate_types.cpp    -> validates nodes/ast_types.h
//   validate_exprs.cpp    -> validates nodes/ast_exprs.h
//   validate_patterns.cpp -> validates nodes/ast_patterns.h
//   validate_stmts.cpp    -> validates nodes/ast_stmts.h
//   validate_items.cpp    -> validates nodes/ast_items.h
//   validate_module.cpp   -> validates nodes/ast_module.h
//
// ---------------------------------------------------------------------------
// SPEC REFERENCE: C:\Dev\Cursive\C0updated.md
// ---------------------------------------------------------------------------
//   Section 3.3.11 - Documentation Association (referenced at line 2639)
//   Defines how doc comments are attached to AST items.
//
//   Section 3.3.2 - AST Node Catalog
//   Lines 2870-2873: ErrorItem definition (in ast_items.h)
//     - IsDecl(ErrorItem(_)) = false (2873)
//     - ErrorItems should be validated/reported
//
//   Section 2 - Cursive0 Conformance Subset
//   Lines 242-400+: UnsupportedForm checks
//     - Various syntax constructs that are unsupported in C0
//     - Validation should detect use of unsupported forms
//
//   Section 3.3.10 - Trailing Comma Rules (line 1881 area)
//     - Trailing commas only allowed when closing delimiter on different line
//
//   Section 3.2.3 - Reserved Names (lines 2086-2102)
//     - Reserved keyword validation
//     - Universe-protected binding validation
//     - Reserved namespace/prefix validation
//
// ---------------------------------------------------------------------------
// SOURCE FILE: C:\Dev\Cursive\cursive-bootstrap\src\02_syntax\parser_docs.cpp
// ---------------------------------------------------------------------------
//   Lines 59-71: ModuleDocs function
//     - Filters DocComments with kind == ModuleDoc
//     - SPEC_RULE("Attach-Doc-Module")
//
//   Lines 73-106: AttachLineDocs function
//     - Attaches line docs to following AST items
//     - Iterates items and docs together
//     - Skips ErrorItems (uses ast_items.h ErrorItem)
//     - SPEC_RULE("Attach-Doc-Line")
//
//   Lines 13-16: ItemStartOffset helper
//     - Extracts start offset from any ASTItem variant (ast_items.h)
//     - Uses std::visit
//
//   Lines 18-20: IsErrorItem helper
//     - Checks if item is ErrorItem variant (ast_items.h)
//
//   Lines 22-55: SetItemDoc helper
//     - Sets doc field on item (handles each variant from ast_items.h)
//
// ---------------------------------------------------------------------------
// SOURCE FILE: C:\Dev\Cursive\cursive-bootstrap\src\02_syntax\keyword_policy.cpp
// ---------------------------------------------------------------------------
//   (Location for CheckMethodContext and similar validation)
//
//   Referenced in parse_modules.cpp line 210:
//     CheckMethodContext(*parsed.file, result.diags)
//
// ---------------------------------------------------------------------------
// SOURCE FILE: C:\Dev\Cursive\cursive-bootstrap\src\03_analysis\types\conformance.cpp
// ---------------------------------------------------------------------------
//   (Referenced for C0 unsupported form checking)
//
//   Referenced in parse_modules.cpp lines 177-182:
//     CheckC0UnsupportedFormsAst(*parsed.file, *load.source)
//
// ---------------------------------------------------------------------------
// DEPENDENCIES
// ---------------------------------------------------------------------------
//   Required headers (split organization):
//   - ast_fwd.h: Forward declarations
//   - ast_enums.h: Enums for validation checks
//   - ast_common.h: Shared helper types
//   - ast_types.h: Type node definitions for type validation
//   - ast_exprs.h: Expression node definitions for expr validation
//   - ast_patterns.h: Pattern node definitions for pattern validation
//   - ast_stmts.h: Statement/Block definitions for stmt validation
//   - ast_items.h: Declaration definitions for item validation
//   - ast_module.h: ASTFile, ASTModule for module validation
//
//   External dependencies:
//   - lexer: DocComment for documentation attachment
//   - 00_core/diagnostics.h: DiagnosticStream for error reporting
//   - 00_core/span.h: Span for error locations
//
//   Required helpers:
//   - std::visit for variant traversal
//   - std::holds_alternative for type checking
//   - Diagnostic emission functions
//
// ---------------------------------------------------------------------------
// VALIDATION CATEGORIES
// ---------------------------------------------------------------------------
//
//   1. Structural Validation (all categories)
//      - No null pointers where required
//      - Lists have correct lengths
//      - Variants contain expected alternatives
//
//   2. Documentation Attachment (ast_items.h, ast_module.h)
//      - Module docs collected from file-level //! comments
//      - Line docs (///) attached to following item
//      - Doc ordering preserved
//
//   3. Name Validation (ast_items.h, ast_exprs.h)
//      - No use of reserved keywords as identifiers
//      - No shadowing of universe-protected names (in appropriate contexts)
//      - No reserved prefixes (gen_, cursive::)
//
//   4. Syntactic Constraint Validation (ast_items.h)
//      - Receiver only in method context (MethodDecl)
//      - Override only on method declarations (MethodDecl.override_flag)
//      - Transition only in modal state blocks (TransitionDecl in StateBlock)
//      - Generic parameters use semicolon separator (GenericParams)
//
//   5. C0 Subset Conformance (all categories)
//      - No unsupported forms (closure, metaprogramming, etc.)
//      - No unsupported type syntax (ast_types.h)
//      - No unsupported expression syntax (ast_exprs.h)
//
//   6. Span Consistency (all categories)
//      - All nodes have valid spans
//      - Child spans contained in parent spans
//      - No overlapping sibling spans
//
// ---------------------------------------------------------------------------
// REFACTORING NOTES (COMPLETED)
// ---------------------------------------------------------------------------
//   The validation system has been split into modular files:
//
//   1. validate_common.cpp - Common infrastructure:
//      - ValidationResult: success/failure with diagnostics
//      - ValidationContext: scope tracking, parent nodes
//      - ASTValidator base class (visitor pattern)
//      - Composable validation with compose()
//      - Span validation utilities
//      - Diagnostic helpers
//
//   2. validate_types.cpp - Type validation:
//      - Permission nesting rules
//      - Union type well-formedness
//      - Function type validation
//      - Pointer/array/slice validation
//      - Modal state type validation
//      - Refinement type validation
//
//   3. validate_exprs.cpp - Expression validation (50+ types):
//      - Operator validity (binary, unary, cast)
//      - Control flow (if, match, loop)
//      - Call expressions (function, method ~>)
//      - Memory expressions (alloc in region, transmute in unsafe)
//      - Concurrency (spawn in parallel, wait/yield key constraints)
//      - Key block validation (keys_held tracking)
//      - Contract intrinsics (@result, @entry)
//
//   4. validate_patterns.cpp - Pattern validation:
//      - Irrefutability checking
//      - Let/var binding pattern requirements
//      - Match arm pattern validation
//      - Guard pattern handling
//      - Duplicate binding detection
//
//   5. validate_stmts.cpp - Statement/block validation:
//      - Binding statement validation
//      - Assignment validation
//      - Control flow context (return, break, continue)
//      - Scoped blocks (region, frame, unsafe)
//      - Key block statement validation
//      - Block tail expression validation
//
//   6. validate_items.cpp - Declaration validation:
//      - Method context (receiver, override, transition)
//      - Procedure validation (main signature)
//      - Generic parameter separator validation (<T; U>)
//      - Where clause syntax validation
//      - FFI/FfiSafe type checking
//      - Contract clause validation
//      - Attribute and visibility validation
//
//   7. validate_module.cpp - Module/file validation:
//      - Doc attachment (module docs //!, line docs ///)
//      - Error item collection and reporting
//      - Unsafe span collection
//      - Module/file path validation
//
//   8. validate_names.cpp - Name validation:
//      - Reserved keyword checking
//      - Contextual keyword handling
//      - Universe-protected name checking
//      - Reserved prefix (gen_) checking
//      - Reserved namespace (cursive::, std) checking
//      - Unicode identifier validation
//      - "Did you mean" suggestions
//
//   9. validate_c0.cpp - C0 subset conformance:
//      - Unsupported construct detection (closure, pipeline)
//      - Capability validation (no Network, GPUFactory, CPUFactory)
//      - Trailing comma validation
//      - Float literal suffix validation
//      - Generic separator validation
//
//   Validation phases (recommended order):
//   - Phase 1: validate_common (structure)
//   - Phase 2: validate_module (doc attachment)
//   - Phase 3: validate_names, validate_types, validate_exprs,
//              validate_patterns, validate_stmts, validate_items
//   - Phase 4: validate_c0 (C0 conformance)
//
// ===========================================================================

// ===========================================================================
// IMPLEMENTATION STATUS
// ===========================================================================
//
// The validation API has been split into modular files in ast/validate/:
//
//   validate_common.cpp   -> Infrastructure (ValidationResult, ValidationContext, ASTValidator)
//   validate_types.cpp    -> Type validation (20+ functions)
//   validate_exprs.cpp    -> Expression validation (30+ functions)
//   validate_patterns.cpp -> Pattern validation (15+ functions)
//   validate_stmts.cpp    -> Statement/block validation (20+ functions)
//   validate_items.cpp    -> Declaration validation (25+ functions)
//   validate_module.cpp   -> Module validation, doc attachment (15+ functions)
//   validate_names.cpp    -> Name validation (20+ functions)
//   validate_c0.cpp       -> C0 conformance validation (15+ functions)
//
// Each modular file contains:
//   - PURPOSE section with file description
//   - SPEC REFERENCE section with C0updated.md line numbers
//   - SOURCE FILE section with cursive-bootstrap locations
//   - DEPENDENCIES section listing required headers
//   - REFACTORING NOTES for implementation guidance
//   - Documented API skeleton in comments
//
// To implement validators:
//   1. Start with validate_common.cpp (foundational infrastructure)
//   2. Implement category validators in dependency order:
//      - validate_types.cpp (no expr dependency)
//      - validate_patterns.cpp (needs types)
//      - validate_exprs.cpp (needs patterns, types)
//      - validate_stmts.cpp (needs exprs, patterns, types)
//      - validate_items.cpp (needs stmts, types)
//      - validate_module.cpp (needs items)
//      - validate_names.cpp (standalone)
//      - validate_c0.cpp (uses all categories)
//
// Central header for all validators:
//   Consider creating ast/validate/ast_validators.h that includes all modular
//   validators and provides the cursive::ast::validate namespace.
//
// ===========================================================================

// This file now serves as an index. See ast/validate/*.cpp for implementations.

