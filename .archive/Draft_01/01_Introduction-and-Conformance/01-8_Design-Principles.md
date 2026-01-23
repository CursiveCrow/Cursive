# Cursive Language Specification

## Clause 1 — Introduction and Conformance

**Part**: I — Introduction and Conformance
**File**: 01-8_Design-Principles.md  
**Section**: §1.9 Design Principles  
**Stable label**: [intro.principles]  
**Forward references**: Clause 4 [decl], Clause 6 [type], Clause 10 [memory], Clause 11 [contract], Clause 12 [witness], Clause 15 [comptime]

---

### §1.9 Design Principles [intro.principles]

[1] Cursive embodies the following core design principles that inform decisions throughout this specification:

#### §1.9.1 Explicit Over Implicit [intro.principles.explicit]

[2] Every effect is visible in code. Cleanup responsibility uses explicit operators (`=` vs `<-`). Transfer requires explicit `move`. Grants require explicit declaration. Permissions require explicit annotation when not defaulted. All grants, preconditions, and postconditions are explicitly written in procedure signatures. There are no hidden effects, implicit preconditions, or unspecified guarantees. All compile-time execution is marked with the `comptime` keyword. Reflection requires explicit `[[reflect]]` attribute.

#### §1.9.2 Zero-Cost Abstractions [intro.principles.zerocost]

[3] Abstractions compile to code equivalent to hand-written alternatives. Safety mechanisms operate at compile time. Generic dispatch uses monomorphization. Permissions have no runtime representation. Static dispatch through monomorphization remains the zero-cost default. Witnesses are an explicit opt-in for scenarios requiring runtime flexibility. Types without `[[reflect]]` have no reflection metadata overhead.

#### §1.9.3 Local Reasoning [intro.principles.local]

[4] Program properties are derivable from local context. Procedure contracts specify complete requirements. Permissions prevent action at a distance. Region escape analysis is local to region blocks. Witness types show complete dispatch strategy. Metaprogramming code is readable without global context.

#### §1.9.4 Deterministic Semantics [intro.principles.deterministic]

[5] Language behavior is fully specified and deterministic. Evaluation order is left-to-right. No undefined evaluation order. Comptime evaluation is reproducible. Compilation order and comptime evaluation are deterministic.

---

**Previous**: §1.8 Design Decisions and Absent Features (§1.8 [intro.absent]) | **Next**: Clause 2 Lexical Structure (§2.1 [lex.source])


