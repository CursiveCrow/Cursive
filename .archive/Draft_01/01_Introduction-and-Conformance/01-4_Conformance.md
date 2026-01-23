# Cursive Language Specification

## Clause 1 — Introduction and Conformance

**Part**: I — Introduction and Conformance
**File**: 01-4_Conformance.md  
**Section**: §1.5 Conformance  
**Stable label**: [intro.conformance]  
**Forward references**: §1.5.3–§1.5.6 [intro.conformance.impldef]–[intro.conformance.unspecified], §1.7 [intro.versioning], Clause 2 [lex], Clauses 5–14, Annex B.2 [behavior.undefined]

---

### §1.5 Conformance [intro.conformance]

[1] This subclause defines what it means to conform to the Cursive specification. Conformance applies both to implementations and to programs.

#### §1.5.1 Normative Vocabulary [intro.conformance.keywords]

[2] The keywords **shall** and **shall not** express absolute requirements and prohibitions. **Should** and **should not** express strong recommendations; implementations may deviate provided the deviation is documented and justified. **May** and **may not** describe permissions. **Can** and **cannot** describe capability, and are informative. These meanings follow ISO/IEC Directives, Part 2 and mirror RFC 2119 terminology.

[3] When a paragraph is marked Informative it contains guidance, examples, or rationale that does not impose requirements. Notes, warnings, and rationale blocks are informative by default.

#### §1.5.2 Conforming Implementations [intro.conformance.impl]

[4] An implementation conforms if and only if it satisfies all of the following conditions:

- **Acceptance**: It shall accept every program that satisfies the rules of §1.5.4 [intro.conformance.program].
- **Rejection**: It shall issue a diagnostic for any ill-formed program (§1.5.5 [intro.conformance.diagnostics]).
- **Semantics**: It shall implement the runtime behavior described in Clauses 2–16 for well-formed programs that avoid undefined behavior.
- **Documentation**: It shall document every implementation-defined behavior identified in §1.5.3 [intro.conformance.impldef].
- **Extension safety**: Extensions shall not alter the meaning of conforming programs nor mask diagnostics that the base language would require.

[5] Conforming implementations may provide additional tools, optimisations, and analyses. Such features shall be disableable when they would otherwise change language semantics.

#### §1.5.3 Implementation-Defined Behavior [intro.conformance.impldef]

[6] Implementation-defined behavior occurs when this specification allows multiple outcomes but requires that an implementation choose one deterministically and document it. The following categories are implementation-defined:

- Numeric representation widths beyond mandated minima (§11.6 [memory.layout]);
- Pointer and region size, alignment, and layout details (§11.6 [memory.layout]);
- Resource ceilings such as maximum recursion depth or compilation unit size;
- Diagnostic formatting and display conventions (§1.5.5 [intro.conformance.diagnostics]);
- ABI choices for each supported platform (§15.6 [interop.abi]).

[7] Implementations shall publish these choices in user-facing documentation and apply them consistently.

#### §1.5.4 Conforming Programs [intro.conformance.program]

[8] A program is conforming when it:

- Satisfies the grammar of Annex A and the static semantics of Clauses 2–12;
- Abides by all contract, grant, permission, and modal requirements (§12–§14);
- Avoids undefined behaviour as catalogued in Annex B.2;
- Respects implementation-defined limits documented by the chosen implementation;
- Indicates the language edition it targets when required by the build manifest (§1.7 [intro.versioning]).

[9] Conforming programs may rely on unspecified behaviour, but shall not assume a particular outcome when the specification leaves choices open.

#### §1.5.5 Diagnostics and IFNDR [intro.conformance.diagnostics]

[10] A program that violates a static semantic rule is _ill-formed_ and shall provoke a diagnostic. Diagnostics shall include at minimum an error code, source location, and a brief description of the violation.

[11] Some violations are classified _ill-formed, no diagnostic required_ (IFNDR). These cases are documented explicitly at the point of definition; implementations are encouraged, but not required, to diagnose them. IFNDR is reserved for situations that are impractical to detect statically, such as exceeding translation limits that are only observable post-link.

[12] Diagnostic presentation style is implementation-defined, yet tools should strive for clarity and actionable guidance. Informative advice, suggested fixes, and machine-readable output formats are quality-of-implementation features, not conformance requirements.

#### §1.5.6 Behavioural Classifications [intro.conformance.unspecified]

[13] **Unspecified behaviour** permits multiple well-defined outcomes without documentation. **Undefined behaviour** confers no guarantees; once triggered, any result is permitted, and the program ceases to be conforming. Undefined behaviour sites are labelled `[UB: B.2 #N]` and mapped to Annex B.2 [behavior.undefined].

[14] Distinguishing these categories enables implementers to provide stronger diagnostics and enables programmers to reason about portability.

---

**Previous**: §1.4 Notation and Conventions (§1.4 [intro.notation]) | **Next**: §1.6 Document Conventions (§1.6 [intro.document])
