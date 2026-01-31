/*
 * =============================================================================
 * MIGRATION MAPPING: context_caps.cpp
 * =============================================================================
 *
 * SPEC REFERENCE:
 *   - C0updated.md, Section 5.9.4 "Context Record" (line 13202)
 *   - C0updated.md, Section 5.9.4.1 "Context Fields" (lines 13210-13300)
 *   - C0updated.md, Section 5.9.4.2 "Context Methods" (lines 13310-13400)
 *
 * SOURCE FILE:
 *   - cursive-bootstrap/src/03_analysis/caps/cap_system.cpp (lines 1-178)
 *
 * FUNCTIONS TO MIGRATE:
 *   - BuildContextRecord() -> RecordDecl*                          [lines 50-100]
 *       Construct the built-in Context record type
 *   - ContextFieldType(Name field) -> TypeRef                      [lines 105-140]
 *       Get type of Context field (fs, heap, etc.)
 *   - ContextMethodSignature(Name method) -> ProcSig               [lines 145-178]
 *       Get signature of Context method (cpu(), gpu(), inline())
 *   - ValidateMainSignature(Proc* main) -> bool
 *       Validate main procedure takes Context parameter
 *
 * DEPENDENCIES:
 *   - RecordDecl for Context structure
 *   - Capability class types
 *   - ExecutionDomain modal type
 *
 * REFACTORING NOTES:
 *   1. Context is the root of all capabilities
 *   2. main procedure MUST take (move ctx: Context) -> i32
 *   3. Context provides:
 *      - ctx.fs: $FileSystem
 *      - ctx.heap: $HeapAllocator
 *      - ctx.cpu(): $ExecutionDomain
 *      - ctx.gpu(): $ExecutionDomain
 *      - ctx.inline(): $ExecutionDomain
 *   4. Context is NOT FfiSafe (cannot cross FFI boundary)
 *   5. Consider Context as singleton - only one instance per program
 *
 * CONTEXT INTERFACE:
 *   record Context {
 *     fs: $FileSystem,
 *     heap: $HeapAllocator,
 *     procedure cpu(~) -> $ExecutionDomain,
 *     procedure gpu(~) -> $ExecutionDomain,
 *     procedure inline(~) -> $ExecutionDomain
 *   }
 *
 * DIAGNOSTIC CODES:
 *   - E-CAP-0020: Invalid main signature
 *   - E-CAP-0021: Context passed to extern
 *   - E-CAP-0022: Context field access invalid
 *
 * =============================================================================
 */
