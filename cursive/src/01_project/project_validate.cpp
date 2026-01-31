// =============================================================================
// MIGRATION MAPPING: project_validate.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md Section 2.1 (lines 806-1027)
//   - 2.1. Project Root and Manifest
//   - Manifest Schema (Cursive0)
//   - Manifest Validation rules (WF-*)
//   - Path Resolution
//
// SOURCE FILE: cursive-bootstrap/src/01_project/project_validate.cpp
//   - Lines 1-285 (entire file)
//
// =============================================================================
// CONTENT TO MIGRATE
// =============================================================================
//
// 1. Helper functions (lines 16-68)
//    - MakeExternalDiag() / EmitExternal() (lines 18-30)
//
//    - HasOnlyAssemblyKey() (lines 32-40)
//      PURPOSE: Check that only "assembly" key exists at top level
//      SPEC RULE: Keys(T) subseteq {`assembly`}
//      SPEC REF: Lines 873-876
//
//    - IsKnownAssemblyKey() (lines 42-45)
//      PURPOSE: Check if key is valid assembly table key
//      SPEC: Req = {`name`, `kind`, `root`}, Opt = {`out_dir`, `emit_ir`}
//      SPEC REF: Lines 913-914
//
//    - RelPathStatus enum and CheckRelPath() (lines 47-68)
//      PURPOSE: Validate relative path within project root
//      SPEC RULES:
//        * WF-RelPath (spec lines 1018-1021)
//        * WF-RelPath-Err (spec lines 1023-1026)
//
// 2. AsmTablesResult and AsmTables() (lines 70-111)
//    - PURPOSE: Extract assembly tables from manifest
//    - SPEC RULE: AsmTables(T) extraction logic
//    - SPEC REF: Lines 867-871
//    - Handles both single table and array of tables
//
// 3. ValidateManifest() (lines 115-269)
//    - PURPOSE: Validate parsed manifest against schema
//    - SPEC RULES (in order of checking):
//      * WF-TopKeys / WF-TopKeys-Err (spec lines 873-881)
//      * WF-Assembly-Table / WF-Assembly-Table-Err (spec lines 883-891)
//      * WF-Assembly-Count / WF-Assembly-Count-Err (spec lines 893-901)
//      * WF-Assembly-Keys / WF-Assembly-Keys-Err (spec lines 916-924)
//      * WF-Assembly-Required-Types / WF-Assembly-Required-Types-Err (spec lines 926-934)
//      * WF-Assembly-Optional-Types (spec lines 936-948)
//      * WF-Assembly-OutDirType-Err (spec lines 941-943)
//      * WF-Assembly-EmitIRType-Err (spec lines 950-953)
//      * WF-Assembly-Name / WF-Assembly-Name-Err (spec lines 814-822)
//      * WF-Assembly-Kind / WF-Assembly-Kind-Err (spec lines 824-832)
//      * WF-Assembly-EmitIR / WF-Assembly-EmitIR-Err (spec lines 854-862)
//      * WF-Assembly-Root-Path / WF-Assembly-Root-Path-Err (spec lines 834-842)
//      * WF-Assembly-OutDir-Path / WF-Assembly-OutDir-Path-Err (spec lines 844-852)
//      * WF-Assembly-Name-Dup / WF-Assembly-Name-Dup-Err (spec lines 903-911)
//      * ValidateManifest-Ok / ValidateManifest-Err (spec lines 1068-1076)
//    - DIAGNOSTICS:
//      * E-PRJ-0103: Missing or invalid assembly table
//      * E-PRJ-0104: Unknown top-level key
//      * E-PRJ-0201: Invalid assembly kind
//      * E-PRJ-0202: Duplicate assembly name
//      * E-PRJ-0203: Invalid assembly name
//      * E-PRJ-0204: Invalid emit_ir value
//      * E-PRJ-0301: Invalid path (out_dir or root)
//      * E-PRJ-0304: Path resolution error
//    - DEPENDENCIES: IsName() from ident.h
//
// 4. IsProjectRoot() (lines 271-282)
//    - PURPOSE: Check if directory contains Cursive.toml
//    - SPEC RULE: WF-Project-Root (spec lines 1156-1159)
//
// =============================================================================
// DEPENDENCIES
// =============================================================================
//
// Headers required:
//   - "cursive0/01_project/project.h" (types)
//   - "cursive0/00_core/assert_spec.h" (SPEC_RULE macro)
//   - "cursive0/00_core/diagnostic_messages.h" (MakeDiagnostic)
//   - "cursive0/00_core/diagnostics.h" (DiagnosticStream, Emit)
//   - "cursive0/00_core/host_primitives.h" (HostPrimFail)
//   - "cursive0/00_core/path.h" (IsRelative, Resolve, Prefix)
//   - "cursive0/01_project/ident.h" (IsName, IsIdentifier, IsKeyword)
//   - <string_view>
//   - <system_error>
//   - <unordered_set>
//   - <filesystem>
//   - toml++ library (toml::table, toml::node, toml::array)
//
// Types from header (project.h):
//   - ValidatedAssembly { name, kind, root, out_dir, emit_ir }
//   - ManifestValidationResult { std::vector<ValidatedAssembly> assemblies; DiagnosticStream diags; }
//   - TOMLTable = toml::table
//
// =============================================================================
// REFACTORING NOTES
// =============================================================================
//
// 1. The validation follows a deterministic order as specified:
//    ChecksAsm(t) = [KnownKeys, ReqTypes, OutDirType, EmitIRType, Name, Kind, EmitIR, RootPath, OutDirPath]
//    SPEC REF: Lines 1057-1062
//
// 2. The implementation uses early return on first error (FirstFail pattern)
//    SPEC RULE: FirstFail([J::Js]) = c if J fails, else FirstFail(Js)
//    SPEC REF: Lines 1064-1066
//
// 3. The CheckRelPath helper handles three cases:
//    - Ok: path is relative and resolves within root
//    - RelPathErr: path is absolute or escapes root
//    - ResolveErr: path resolution itself failed
//
// 4. TOML handling uses toml++ library conventions:
//    - is_table(), is_array(), is_array_of_tables()
//    - as_table(), as_array()
//    - value<T>() for extracting values
//
// =============================================================================
// SPEC RULE ANNOTATIONS (use SPEC_RULE macro)
// =============================================================================
//
// Line 55: SPEC_RULE("WF-RelPath-Err");
// Line 64: SPEC_RULE("WF-RelPath-Err");
// Line 66: SPEC_RULE("WF-RelPath");
// Line 120: SPEC_RULE("ValidateManifest-Err");
// Line 126: SPEC_RULE("WF-TopKeys-Err");
// Line 129: SPEC_RULE("WF-TopKeys");
// Line 133: SPEC_RULE("WF-Assembly-Table-Err");
// Line 139: SPEC_RULE("WF-Assembly-Table-Err");
// Line 142: SPEC_RULE("WF-Assembly-Table");
// Line 145: SPEC_RULE("WF-Assembly-Count-Err");
// Line 148: SPEC_RULE("WF-Assembly-Count");
// Line 157: SPEC_RULE("WF-Assembly-Table-Err");
// Line 164: SPEC_RULE("WF-Assembly-Keys-Err");
// Line 168: SPEC_RULE("WF-Assembly-Keys");
// Line 176: SPEC_RULE("WF-Assembly-Required-Types-Err");
// Line 179: SPEC_RULE("WF-Assembly-Required-Types");
// Line 185: SPEC_RULE("WF-Assembly-Required-Types-Err");
// Line 192: SPEC_RULE("WF-Assembly-OutDirType-Err");
// Line 202: SPEC_RULE("WF-Assembly-EmitIRType-Err");
// Line 207: SPEC_RULE("WF-Assembly-Optional-Types");
// Line 210: SPEC_RULE("WF-Assembly-Name-Err");
// Line 213: SPEC_RULE("WF-Assembly-Name");
// Line 216: SPEC_RULE("WF-Assembly-Kind-Err");
// Line 219: SPEC_RULE("WF-Assembly-Kind");
// Line 224: SPEC_RULE("WF-Assembly-EmitIR-Err");
// Line 228: SPEC_RULE("WF-Assembly-EmitIR");
// Line 231: SPEC_RULE("WF-Assembly-Root-Path-Err");
// Line 238: SPEC_RULE("WF-Assembly-Root-Path");
// Line 244: SPEC_RULE("WF-Assembly-OutDir-Path-Err");
// Line 251: SPEC_RULE("WF-Assembly-OutDir-Path");
// Line 254: SPEC_RULE("WF-Assembly-Name-Dup-Err");
// Line 265: SPEC_RULE("WF-Assembly-Name-Dup");
// Line 267: SPEC_RULE("ValidateManifest-Ok");
// Line 279: SPEC_RULE("WF-Project-Root");
//
// =============================================================================

