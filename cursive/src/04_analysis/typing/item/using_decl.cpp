// =============================================================================
// MIGRATION MAPPING: item/using_decl.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 4: Module System
//   - using declaration grammar
//   - Direct scope injection
//   - Glob imports
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/types/imports.cpp
//   Using declaration processing
//
// KEY CONTENT TO MIGRATE:
//   USING DECLARATION:
//   1. Resolve the path to item(s)
//   2. Verify visibility
//   3. Inject item(s) directly into current scope
//   4. Register in environment
//
//   USING FORMS:
//   - using path::to::Item
//   - using path::to::{Item1, Item2}
//   - using path::to::*
//   - using path::to::Item as Alias
//
//   TYPING (single):
//   Item at path exists
//   Item is visible
//   --------------------------------------------------
//   Gamma |- using path::to::Item
//   Gamma' = Gamma + {Item -> path::to::Item}
//
//   TYPING (glob):
//   Module at path exists
//   --------------------------------------------------
//   Gamma |- using path::to::*
//   Gamma' = Gamma + all public items from module
//
//   TYPING (selective):
//   Items exist and visible
//   --------------------------------------------------
//   Gamma |- using path::to::{A, B, C}
//   Gamma' = Gamma + {A, B, C}
//
//   WITH ALIAS:
//   Gamma |- using path::to::Item as Alias
//   Gamma' = Gamma + {Alias -> path::to::Item}
//
//   IMPORT VS USING:
//   - import: creates namespace, access via module::item
//   - using: direct injection, access as item
//
//   CONFLICT RESOLUTION:
//   - Local definitions shadow using imports
//   - Error on conflicting using imports
//   - Explicit alias resolves conflicts
//
// DEPENDENCIES:
//   - PathResolver for item resolution
//   - VisibilityCheck for access validation
//   - ConflictDetection for name clashes
//   - Environment update for scope injection
//
// SPEC RULES:
//   - Using declaration semantics
//   - Name resolution order
//
// RESULT:
//   Items injected into current scope
//   Diagnostics for resolution/conflict errors
//
// =============================================================================

