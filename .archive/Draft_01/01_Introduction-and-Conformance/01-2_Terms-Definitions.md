# Cursive Language Specification

## Clause 1 — Introduction and Conformance

**Part**: I — Introduction and Conformance
**File**: 01-2_Terms-Definitions.md  
**Section**: §1.3 Terms and Definitions  
**Stable label**: [intro.terms]  
**Forward references**: §2.5 [lex.units], Clause 5 [decl], Clause 6 [name], Clause 8 [expr], §11.3 [memory.region], §11.4 [memory.permission], Clause 12 [contract], Clause 13 [witness]

---

### §1.3 Terms and Definitions [intro.terms]

[1] The vocabulary below is normative. When a term is defined in this clause it shall carry the same meaning throughout Clauses 2–16 and the annexes. Paragraph numbering resets within each subclause per the organizational template described in §1.6 [intro.document].

#### §1.3.1 General Language Terms [intro.terms.general]

[2] **binding** — the association between an identifier and an entity within a particular scope.

[3] **compilation unit** — the smallest source artifact processed as a whole during translation (§2.5 [lex.units]).

[4] **conforming implementation** — any compiler, interpreter, or analysis tool that meets the requirements of §1.5.3 [intro.conformance.impl] and documents its implementation-defined behavior.

[5] **conforming program** — a program that satisfies the grammar, static semantics, and behavioral requirements outlined in §1.5.4 [intro.conformance.program]; conforming programs shall not rely on undefined behavior.

[6] **declaration** — a syntactic form that introduces a name and determines its category (§5 [decl]).

[7] **diagnostic** — a message issued when a program violates a rule that requires detection (§1.5.5 [intro.conformance.diagnostics]). Diagnostics include errors (rejecting) and warnings (permissive).

[8] **entity** — any value, type, module, predicate, contract, region, or grant that may be named or referenced (Clause 6 [name]).

[9] **expression** — a syntactic form that yields a value or place (§8 [expr]).

[10] **statement** — a syntactic form that executes for effects on program state without directly yielding a value (§9 [stmt]).

[11] **scope** — the syntactic region where a binding is visible (§6.2 [name.scope]).

#### §1.3.2 Cursive-Specific Concepts [intro.terms.language]

[12] **binding responsibility mode** — the axis that distinguishes responsible (`=`) and non-responsible (`<-`) initialisation for `let`/`var` bindings (§5.2 [decl.variable]).

[13] **permission** — a capability (`const`, `shared`, `unique`) describing how a value may be accessed (§11.4 [memory.permission]).

[14] **grant** — a token representing authority to perform a capability such as file I/O (§12.3 [contract.grant]).

[15] **grant set** — the finite set of grants required or produced by an expression, preserved throughout typing (§8.8 [expr.typing]).

[16] **sequent** — a formal specification of procedure requirements and guarantees, using the form `[[ grants |- must => will ]]` where `grants` lists capability requirements, `must` lists preconditions, and `will` lists postconditions. Also called "procedure sequent" for clarity (§12.2 [contract.sequent]). The semantic brackets `⟦ ⟧` (Unicode U+27E6, U+27E7) or their ASCII equivalent `[[ ]]` denote formal specification content, consistent with their use for type value sets throughout this specification. The turnstile `⊢` or its ASCII equivalent `|-` separates the grants component from the logical implications.

[17] **grant component** — the portion of a sequent before the turnstile (`|-`), listing zero or more grant identifiers (capability tokens) that must be available for the procedure to execute.

[18] **contract** — a user-defined abstract interface declaration that specifies required procedure signatures and associated types (§12.1 [contract.overview]). Contracts are distinct from sequents: contracts define abstract interfaces, while sequents specify procedure requirements and guarantees.

[19] **behavior** — a collection of callable signatures with concrete implementations that types may attach to gain functionality (§10.4 [generic.behavior]). Behaviors provide reusable code and differ from contracts (which specify abstract requirements) by providing procedure bodies. The term "behavior" is distinct from "predicate expression," which refers to boolean-valued expressions used in sequent clauses.

[20] **predicate expression** — a boolean-valued expression used in sequent clauses (`must`, `will`, `where`) to express logical constraints and conditions.

[21] **modal type** — a type whose values transition through named states validated at compile time (§7.6 [type.modal]).

[22] **witness** — runtime evidence that a contract or behavior obligation has been satisfied, or that a modal value is in a specific state (§13.2 [witness.kind]). Witnesses enable dynamic polymorphism through dense pointer representation with vtable-based dispatch.

[23] **region** — a lexically-scoped allocation arena released in strict LIFO order (§11.3 [memory.region]).

[24] **unsafe block** — an explicit region where the programmer assumes responsibility for safety constraints while interacting with raw operations (§11.8 [memory.unsafe]).

[25] **hole** — a placeholder for an inferred type, permission, or grant inserted by programmers or tools. Type inference and hole resolution are specified in §8.8 [expr.typing].

[26] **principal type** — in a type inference context, the most general type that satisfies all constraints. When type inference succeeds, the principal type is unique and is a supertype of all other valid types for the expression. Bidirectional type checking (§8.1.6) produces principal types for expressions at inference sites.

#### §1.3.3 Behavioral Classifications [intro.terms.behaviour]

[27] **implementation-defined behavior** — behavior where the specification provides a set of allowed outcomes and requires documentation (§1.5.4 [intro.conformance.impldef]).

[28] **unspecified behavior** — behavior for which multiple outcomes are permitted without documentation, yet each outcome is well-defined (§1.5.6 [intro.conformance.unspecified]).

[29] **undefined behavior (UB)** — behavior for which the specification imposes no requirements; UB entries are enumerated in Annex B.2 [behavior.undefined].

[30] **ill-formed program** — a program that violates a static semantic rule requiring diagnosis (§1.5.5 [intro.conformance.diagnostics]).

[31] **ill-formed, no diagnostic required (IFNDR)** — rare violations that implementations are not obligated to detect (§1.5.5 [intro.conformance.ifndr]).

#### §1.3.4 Symbols and Abbreviations [intro.terms.abbrev]

[32] **ABI** — Application Binary Interface.

[33] **AST** — Abstract Syntax Tree.

[34] **EBNF** — Extended Backus–Naur Form.

[35] **FFI** — Foreign Function Interface.

[36] **IFNDR** — Ill-Formed, No Diagnostic Required.

[37] **LPS** — Lexical Permission System.

[38] **RAII** — Resource Acquisition Is Initialisation.

[39] **UB** — Undefined Behavior.

[40] **UTF-8** — Unicode Transformation Format, 8-bit.

[41] Numerical and symbolic notations used in inference rules are catalogued in §1.4.3 [intro.notation.math].

---

**Previous**: §1.2 Normative References (§1.2 [intro.refs]) | **Next**: §1.4 Notation and Conventions (§1.4 [intro.notation])
