# Cursive Language Specification

## Clause 15 — Compile-Time Evaluation and Reflection

**Part**: XV — Compile-Time Evaluation and Reflection  
**File**: 16-01_Overview.md  
**Section**: §15.1 Overview and Integration  
**Stable label**: [comptime.overview]  
**Forward references**: §15.2 [comptime.procedures], §15.3 [comptime.blocks], §15.4 [comptime.intrinsics], §15.5 [comptime.reflect.optin], §15.6 [comptime.reflect.queries], §15.7 [comptime.quote], §15.8 [comptime.codegen.api], §15.9 [comptime.conformance], Clause 2 §2.2 [lex.phases], Clause 4 §4.4 [decl.function], Clause 6 §6.8 [type.introspection], Clause 7 §7.7 [expr.constant], Clause 9 [generic], Clause 11 [contract]

---

### §15.1 Overview and Integration [comptime.overview]

#### §15.1.1 Scope [comptime.overview.scope]

[1] This clause specifies Cursive's compile-time evaluation and reflection system: mechanisms for executing code during compilation, introspecting type structure, and programmatically generating declarations. The system provides compile-time execution through comptime procedures and blocks, opt-in type reflection, programmatic code generation through explicit APIs, and zero-cost abstraction (no runtime overhead when unused).

#### §15.1.2 Design Principles [comptime.overview.principles]

[2] Cursive's compile-time system adheres to core language principles (§1.9). Comptime code uses an explicit grant system (§11.3.3.9) that permits only compile-time-safe operations. All comptime computation must complete within implementation-defined resource limits.

#### §15.1.3 Integration with Language Features [comptime.overview.integration]

[3] The compile-time system integrates with translation phases, declarations, type system, expressions, generics, memory model, and contracts.

---

**Previous**: Clause 14 — Interoperability and ABI (§14.7 [interop.compatibility]) | **Next**: §15.2 Comptime Procedures (§15.2 [comptime.procedures])
