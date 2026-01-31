// =============================================================================
// MIGRATION MAPPING: mangle.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   - Section 6.3.1 Symbol Names and Mangling (lines 15392-15548)
//   - MangleJudg = {Mangle} (line 15396)
//   - Join, PathSig helpers (lines 15401-15404)
//   - ItemPath definitions (lines 15406-15415)
//   - TypeStateName (lines 15417-15418)
//   - PathOfType (lines 15419-15425)
//   - Literal Identity (lines 15427-15434)
//   - LinkName rules (lines 15460-15466)
//   - Mangle-* rules for each item type (lines 15472-15546)
//   - Closure Mangling (lines 15532-15548)
//
// SOURCE FILE: cursive-bootstrap/src/04_codegen/mangle.cpp
//   - LinkName function (lines 16-65)
//   - TypeStateName helpers (lines 72-90)
//   - StaticName helper (lines 95-113)
//   - ItemPath* functions (lines 121-234)
//   - PathOfType function (lines 240-284)
//   - ScopedSym function (lines 290-297)
//   - Mangle* functions (lines 303-387)
//   - AnchorMangleRules (lines 393-406)
//
// DEPENDENCIES:
//   - cursive/include/05_codegen/symbols/mangle.h (mangle function declarations)
//   - cursive/include/00_core/hash.h (FNV1a64 for literal IDs)
//   - cursive/include/00_core/symbols.h (Mangle, StringOfPath)
//   - cursive/include/02_syntax/ast.h (declaration types)
//   - cursive/include/04_analysis/types/types.h (TypeRef)
//
// REFACTORING NOTES:
//   1. ScopedSym(item) = PathSig(ItemPath(item))
//   2. PathSig(p) = mangle(StringOfPath(p))
//   3. ItemPath varies by item type:
//      - Procedures: module_path ++ [name]
//      - Methods: record_path ++ [method_name]
//      - State methods: modal_path ++ [state] ++ [method_name]
//      - VTables: ["vtable"] ++ PathOfType(T) ++ ["cl"] ++ ClassPath
//   4. LinkName checks attributes first:
//      - [[symbol("name")]] -> raw name
//      - [[no_mangle]] -> raw identifier
//      - [[export(link_name="name")]] -> specified name
//   5. LiteralID uses FNV1a64 hash of contents
//   6. Closures mangled with enclosing scope + index
//
// MANGLE FUNCTIONS:
//   - MangleProc, MangleMethod, MangleClassMethod
//   - MangleStateMethod, MangleTransition
//   - MangleStatic, MangleStaticBinding
//   - MangleVTable, MangleLiteral, MangleDefaultImpl
// =============================================================================

#include "cursive/05_codegen/symbols/mangle.h"

// TODO: Migrate implementation from cursive-bootstrap/src/04_codegen/mangle.cpp
