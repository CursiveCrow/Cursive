// =============================================================================
// MIGRATION MAPPING: linkage.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   - Section 6.3.4 Linkage for Generated Symbols (lines 15550-15663)
//   - LinkageKind = {internal, external} (line 15552)
//   - Linkage-UserItem rules (lines 15555-15568)
//   - Linkage-ExternProc rule (lines 15560-15563)
//   - Linkage-StaticBinding rules (lines 15570-15578)
//   - Linkage-ClassMethod rules (lines 15580-15588)
//   - Linkage-StateMethod rules (lines 15590-15598)
//   - Linkage-Transition rules (lines 15600-15608)
//   - Linkage-InitFn, Linkage-DeinitFn rules (lines 15610-15618)
//   - Linkage-VTable, Linkage-LiteralData rules (lines 15620-15628)
//   - Linkage-DropGlue, Linkage-DefaultImpl rules (lines 15630-15643)
//   - Linkage-PanicSym through Linkage-EntrySym (lines 15645-15663)
//
// SOURCE FILE: cursive-bootstrap/src/04_codegen/linkage.cpp
//   - IsExternalVisibility helper (lines 13-16)
//   - IsExternalVisibilityWithProtected helper (lines 21-26)
//   - LinkageOf overloads for various item types (lines 34-116)
//   - LinkageOfVTable, LinkageOfLiteral, etc. (lines 122-178)
//   - AnchorLinkageRules (lines 185-208)
//
// DEPENDENCIES:
//   - cursive/include/05_codegen/symbols/linkage.h (LinkageKind enum)
//   - cursive/include/02_syntax/ast.h (Visibility enum, declaration types)
//
// REFACTORING NOTES:
//   1. LinkageKind determines symbol visibility to linker
//   2. External: visible for linking across modules
//   3. Internal: private to compilation unit
//   4. Visibility mapping:
//      - public/internal -> External linkage
//      - private -> Internal linkage
//      - protected -> External (for class/modal items)
//   5. Generated symbols (vtables, literals, drop glue) are Internal
//   6. Entry symbol (main) is always External
//   7. Runtime symbols are Internal
//
// LINKAGE RULES SUMMARY:
//   - User items: visibility-based
//   - Extern procs: External
//   - Generated: Internal (vtables, literals, drop)
//   - Runtime: Internal (init/deinit, panic, region, builtin)
//   - Entry: External
// =============================================================================

#include "cursive/05_codegen/symbols/linkage.h"

// TODO: Migrate implementation from cursive-bootstrap/src/04_codegen/linkage.cpp
