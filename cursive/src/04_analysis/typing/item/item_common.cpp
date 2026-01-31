// =============================================================================
// MIGRATION MAPPING: item/item_common.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 5.3: Declarations
//   - Common item processing utilities
//   - Declaration environment management
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/types/type_decls.cpp
//   Common item typing utilities
//
// KEY CONTENT TO MIGRATE:
//   ITEM COMMON UTILITIES:
//   - Item dispatch to specific handlers
//   - Declaration registration
//   - Scope management
//   - Two-pass coordination
//
//   ITEM DISPATCH:
//   TypeItem(item) =
//     match item.kind {
//       ProcedureDecl => procedure_decl.cpp
//       RecordDecl => record_decl.cpp
//       EnumDecl => enum_decl.cpp
//       ModalDecl => modal_decl.cpp
//       ClassDecl => class_decl.cpp
//       TypeAliasDecl => type_alias_decl.cpp
//       StaticDecl => static_decl.cpp
//       ExternBlock => extern_block.cpp
//       ImportDecl => import_decl.cpp
//       UsingDecl => using_decl.cpp
//     }
//
//   DECLARATION REGISTRATION:
//   - Add declaration to current scope
//   - Handle duplicate detection
//   - Manage visibility
//
//   SCOPE MANAGEMENT:
//   - Module scope for top-level items
//   - Nested scopes for local items
//   - Scope lookup order
//
//   TWO-PASS SUPPORT:
//   Pass 1: Collect signatures
//   - Process declaration headers
//   - Build name -> type mapping
//   - Don't type bodies
//
//   Pass 2: Type bodies
//   - All names available
//   - Type procedure/method bodies
//   - Verify contracts
//
// DEPENDENCIES:
//   - All item-specific typing modules
//   - Environment management
//   - ScopeStack
//   - DeclarationRegistry
//
// SPEC RULES:
//   - Declaration processing order
//   - Two-pass semantics
//
// RESULT:
//   Dispatches to specific item handlers
//   Coordinates declaration registration
//
// =============================================================================

