# Cursive Language Specification

## Clause 1 — Introduction and Conformance

**Part**: I — Introduction and Conformance
**File**: 01-1_Normative-References.md  
**Section**: §1.2 Normative References  
**Stable label**: [intro.refs]  
**Forward references**: §1.5 [intro.conformance], §2.1 [lex.source], §2.3 [lex.tokens], §15.1 [interop.ffi], §15.6 [interop.abi], Annex G.1 [portability.platform]

---
### §1.2 Normative References [intro.refs]

[1] The documents listed here contain provisions that, through citation, become requirements of the Cursive specification. Where a document is identified as normative, conforming implementations shall obey the referenced clauses exactly.

[2] When a referenced standard is revised, the most recent edition applies provided it remains backward compatible with the requirements stated in this specification. Implementations shall document which revision they use when a choice exists.

#### §1.2.1 ISO/IEC and ECMA Standards [intro.refs.iso]

[3] **ISO/IEC Directives, Part 2: Principles and rules for the structure and drafting of ISO and IEC documents (2021).** Establishes the normative vocabulary (`shall`, `should`, `may`, `can`) and organisational conventions adopted in §1.5 [intro.conformance].

[4] **ISO/IEC 10646:2020 — Information technology — Universal Coded Character Set (UCS).** Defines the character repertoire permitted in source text (§2.1 [lex.source]) and informs identifier classifications (§2.3 [lex.tokens]).

[5] **ISO/IEC 60559:2020 (IEEE 754-2019) — Floating-Point Arithmetic.** Governs semantics for binary32 and binary64 arithmetic used throughout the numeric type definitions (§7.2 [type.primitive]) and runtime semantics (§8.3 [expr.operator]).

[6] **ISO/IEC 9899:2018 — Programming Languages — C.** The foreign-function interface (§15.1 [interop.ffi]) relies on C definitions of object representation, calling conventions, and variadic semantics.

[7] **ECMA-334 — C# Language Specification (current edition).** Cited solely for the cross-reference and documentation practices mirrored in §1.6 [intro.document].

#### §1.2.2 Unicode and IETF Publications [intro.refs.unicode]

[8] **The Unicode Standard, Version 15.0.0 or later.** Supplies derived properties, normalization forms, and identifier recommendations leveraged by the lexical grammar and diagnostics (§2.1 [lex.source], Annex A.2 [grammar.lexical]).

[9] **Unicode Standard Annex #31 — Unicode Identifier and Pattern Syntax.** Specifies identifier composition rules adopted verbatim in §2.3 [lex.tokens].

[10] **Unicode Standard Annex #29 — Unicode Text Segmentation** and **UAX #44 — Unicode Character Database.** Provide segmentation and property metadata for tokenisation and editor guidance; they apply where §2.2 [lex.phases] references grapheme boundaries.

[11] **RFC 2119** and **RFC 8174** — Key words for use in RFCs. While Clause 1 follows ISO terminology, these RFCs define equivalent terminology for readers familiar with IETF documents and are referenced in §1.5.2 [intro.conformance.keywords].

#### §1.2.3 Platform and ABI References [intro.refs.abi]

[12] **System V Application Binary Interface** (generic and architecture supplements including AMD64 psABI and ARM AAPCS). These documents guide interoperability obligations in §15.6 [interop.abi] and Annex G [portability].

[13] **Microsoft x64 Calling Convention** documentation. Required for conforming implementations targeting Windows platforms; referenced alongside other ABI requirements in §15.6 [interop.abi].

#### §1.2.4 Availability and Access [intro.refs.access]

[14] ISO/IEC and IEEE publications typically require purchase; implementers shall secure legitimate access before claiming conformance. Unicode and IETF materials are freely available online and shall be consulted in their latest release form.

[15] Annex G.1 [portability.platform] summarises how to reconcile conflicts when platform-specific guidance diverges from these normative sources.

---

**Previous**: §1.1 Scope (§1.1 [intro.scope]) | **Next**: §1.3 Terms and Definitions (§1.3 [intro.terms])
