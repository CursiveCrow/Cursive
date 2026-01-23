# Cursive Language Specification

## Clause 1 — Introduction and Conformance

**Part**: I — Introduction and Conformance
**File**: 01-0_Scope.md  
**Section**: §1.1 Scope  
**Stable label**: [intro.scope]  
**Forward references**: §2.1–§2.5 [lex.source]–[lex.units], Clauses 3–9, §10–§13, §15.6 [interop.abi], Annex B.2 [behavior.undefined]

---

### §1.1 Scope [intro.scope]

[1] This clause establishes the boundaries of the Cursive language specification, defines the actors it serves, and identifies complementary material external to the core standard. The scope applies to all implementations that claim adherence to Cursive version 1.0 and later compatible editions.

[2] The specification governs every program element required to compile and execute portable, deterministic Cursive code. The governed areas are:

- lexical structure and translation phases (§2.1–§2.5 [lex.source]–[lex.units]);
- foundational concepts: objects, values, storage duration, alignment, and name binding categories (specified in their authoritative clauses);
- modules, declarations, names and scopes, types, and expressions (Clauses 3–7);
- statements and control flow (Clause 8);
- the generics, permission, contract, and witness systems that ensure memory safety and semantic guarantees (Clauses 9–12);
- concurrency, interoperability, and compile-time evaluation (Clauses 13–15).

[3] The specification intentionally excludes topics whose evolution is decoupled from language conformance. Excluded topics include:

- the normative standard library catalogue (moved to separate documentation);
- platform-specific ABI and calling convention details left to implementation policy (§14.6 [interop.abi]);
- concurrency libraries beyond the surface guarantees described in §13.1 [concurrency.model];
- tooling, build systems, or project layout conventions (informative guidance may appear in companion documents).

[4] Certain behaviors are implementation-defined but remain within the specification's boundary. Conforming implementations shall document at least integer and floating-point representations, pointer and region sizes, alignment rules, diagnostic formatting, and resource ceilings consistent with §1.5.4 [intro.conformance.impldef].

[5] Undefined behavior is catalogued centrally in Annex B.2 [behavior.undefined]. Any construct not explicitly marked as undefined shall be diagnosed or defined. Implementers shall ensure that undefined entries referenced in later clauses map back to the Annex catalogue.

[6] The language's foundational concepts—objects (§10.2 [memory.object]), types (Clause 6 [type]), scopes (§5.2 [name.scope]), and storage duration (§10.2.3 [memory.object.storage])—are specified directly in their authoritative clauses. Each clause provides complete semantics without requiring external context beyond the common notation established in §1.4.

#### §1.1.1 Intended Audiences [intro.scope.audience]

[7] Implementers require a precise definition of syntax, typing, evaluation, and diagnostic obligations. Paragraphs marked **Normative** in Clauses 2–15 and Annexes A–B constitute their primary reference set.

[8] Tool developers (formatters, linters, debuggers, analysers) depend on the grammar (Annex A [grammar]), semantic judgments (§1.4 [intro.notation]), and diagnostic contracts (§1.5 [intro.conformance]).

[9] Language designers and researchers rely on the formal notation, meta-theory, and rationale supplied throughout Clause 1 and Annex C [formal].

[10] Programmers seeking educational material should use the informative Programming Guide; Clause 1 provides orientation but does not substitute for tutorials.

#### §1.1.2 Forward References [intro.scope.forward]

[11] Clause 1 references later material only when necessary to state the conformance boundary. Every forward reference is identified explicitly so that implementers can trace dependencies. Circular references are avoided by delegating operational detail to the destination clause.

---

**Previous**: _(start)_ | **Next**: §1.2 Normative References (§1.2 [intro.refs])
