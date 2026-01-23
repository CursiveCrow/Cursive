# Cursive Language Specification
## Clause 5 — Names, Scopes, and Resolution

**Clause**: 5 — Names, Scopes, and Resolution
**File**: 06-1_Identifiers.md
**Section**: §5.1 Identifiers
**Stable label**: [name.identifier]  
**Forward references**: §2.3 [lex.tokens], §4.2 [decl.variable], §4.5 [decl.type], §5.2 [name.scope], §5.6 [name.predeclared]

---

### §5.1 Identifiers [name.identifier]

#### §5.1.1 Overview [name.identifier.overview]

[1] This subclause defines how identifiers are formed, classified, and interpreted within Cursive's unified namespace model. Identifiers name every declaration that participates in lexical scope: variables, procedures, types, modules, contracts, predicates, grants, and labels.

[2] Identifier formation relies on the lexical grammar specified in §2.3 [lex.tokens]; the present clause records the semantic obligations that arise once an identifier enters a scope. In particular, it clarifies case sensitivity, Unicode handling, reserved keywords, and the relationship between identifier spelling and the single-namespace rule introduced in §6.2.

#### §6.1.2 Syntax [name.identifier.syntax]

[3] Identifiers follow the lexical production `identifier` in §2.3.2, which recognises Unicode XID_Start followed by zero or more XID_Continue code points and excludes the reserved keywords listed in §2.3.3.

[4] Implementations shall treat identifiers as Unicode scalar sequences compared by code-point equality after the normalisation rules of §2.1 (source text preprocessing) have been applied.

#### §6.1.3 Constraints [name.identifier.constraints]

[5] Identifiers are case-sensitive. Distinct code-point sequences (including differences in case) denote distinct identifiers unless they normalise to identical sequences under the preprocessing rules of §2.1.2.

[6] Reserved keywords (e.g., `module`, `let`, `with`) shall not be used as identifiers. Attempts to declare or reference a reserved keyword as an identifier are ill-formed (diagnostic E02-208).

[7] Identifiers shall not be the empty string, shall not contain surrogate code points, and shall not embed U+0000. Violations inherit the diagnostics defined in §2.1 (E02-004) and §2.3 (lexical error family).

[8] Identifiers introduced by tooling (for example, code generation utilities) shall satisfy the same lexical rules and shall avoid the `__cursive` prefix reserved for implementation-defined helper symbols.

#### §6.1.4 Semantics [name.identifier.semantics]

[9] Every identifier enters exactly one lexical namespace per scope (§6.2) and is associated with the binding properties listed in §6.3. Identifiers used in value contexts, type contexts, or module contexts are resolved uniformly; category mismatches are diagnosed after lookup.

[10] Two identifiers that compare equal under the normalisation in §2.1 refer to the same binding; no additional case folding or locale-dependent equivalence is applied.

[11] When an identifier appears in qualified form (`Prefix::Name`), only `Name` is subject to the rules in this subclause; the qualification rules in §6.4 govern the prefix resolution.

#### §6.1.5 Examples (Informative) [name.identifier.examples]

**Example 6.1.5.1 (Identifier usage and reserved keywords):**
```cursive
// Valid identifiers (Unicode and ASCII)
let Δ = 0.1
let customer_total = 42
let 数据 = 512

// Case-sensitive distinctions
let user = "alice"
let User = "bob"  // different binding from `user`

// Invalid: reserved keyword
// let module = 5            // ERROR E02-206

// Invalid: contains forbidden control character
// let name = "bad\u0000"    // ERROR E02-004 (caught lexically)
```

#### §6.1.6 Conformance Requirements [name.identifier.requirements]

[12] Implementations shall accept any identifier that satisfies the lexical grammar of §2.3 and the normalisation rules of §2.1, rejecting all others with the diagnostics defined in those clauses.

[13] Implementations shall compare identifiers using exact code-point equality after preprocessing; no locale-dependent folding is permitted.

[14] Implementations shall enforce the prohibition on reserved keywords serving as identifiers and emit diagnostic E02-208 when violated.
