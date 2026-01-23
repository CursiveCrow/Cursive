# Cursive Language Specification

## Clause 1 — Introduction and Conformance

**Part**: I — Introduction and Conformance
**File**: 01-5_Document-Conventions.md  
**Section**: §1.6 Document Conventions  
**Stable label**: [intro.document]  
**Forward references**: §1.4.5 [intro.notation.style], Annex A [grammar], Annex E §E.2.1 [implementation.algorithms]

---

### §1.6 Document Conventions [intro.document]

[1] This subclause prescribes the structural conventions used throughout the specification: stable labels, cross-references, example formatting, and validation expectations. Digital and print editions shall follow the same logical structure.

#### §1.6.1 Stable Labels [intro.document.labels]

[2] Each clause and subclause carries a stable label of the form `[clause.topic[.subtopic]]`. Clause 1 uses the following labels:

- `[intro.scope]`, `[intro.refs]`, `[intro.terms]`, `[intro.notation]`, `[intro.conformance]`, `[intro.document]`, `[intro.versioning]`.
- Subclauses append descriptive suffixes, e.g. `[intro.document.labels]` or `[intro.conformance.impl]`.

[3] Later clauses follow the same style: clause prefix (e.g., `lex`, `decl`, `memory`) plus a concise topic. Labels are unique and stable across editions. When content moves, the label moves with it to preserve hyperlinks.

#### §1.6.2 Cross-Reference Format [intro.document.references]

[4] Cross-references shall use the format `§X.Y[label]`. For example, “§5.2 [decl.variable]” references the variable binding rules in Clause 5. Within the same clause the leading clause number may be omitted when unambiguous (`§1.5 [intro.conformance]`).

[5] Digital publications shall hyperlink the rendered text to the referenced section. PDF editions should provide bidirectional navigation where feasible.

[6] Forward references are permitted when necessary to inform the reader about required prerequisites. Authors shall explicitly mark the forward reference (e.g., “Forward reference: §12.4 [memory.permission]”). Cycles should be avoided; when unavoidable they must be documented in both target sections.

#### §1.6.3 Validation Requirements [intro.document.validation]

[7] Prior to publication, editors shall verify that every label resolves to an existing target and that no dangling references remain. Automated tooling should generate:

- an outbound reference table listing, for each section, the labels it references;
- an inbound reference table to identify orphans (sections with no references);
- a report for unresolved labels or malformed cross-references.

[8] Failing references shall block publication. Errata releases may fix discovered issues; patch releases (e.g., 1.0.1) shall include updated validation artefacts.

#### §1.6.4 Examples and Annotations [intro.document.examples]

[9] Examples are informative unless clearly marked otherwise. Code examples use the `cursive` fence. Diagnostics may annotate expected errors using comments (`// ERROR E4001: …`). When examples demonstrate invalid code the surrounding prose shall identify the normative rule being violated.

[10] Notes and warnings employ the bracketed ISO format described in §1.4.5 [intro.notation.style]. Informative commentary shall never contradict normative rules.

#### §1.6.5 Document Metadata [intro.document.metadata]

[11] Each file includes navigation metadata (**Previous**, **Next**) to aid editors working in split files. These links are informative; they shall mirror the logical order determined by Clause 1 but impose no additional requirements on implementations.

[12] Version history entries belong in dedicated change logs (Annex H [changes]). Individual clause files should reference the change log instead of embedding historical commentary.

#### §1.6.6 Diagnostic Code Format [intro.document.diagnostics]

[13] All diagnostic codes in this specification shall use the canonical format `E[CC]-[NNN]`, where:

- `E` denotes an error diagnostic;
- `[CC]` is a two-digit clause number with leading zero (01, 02, ..., 16);
- `-` is a hyphen separator for visual clarity;
- `[NNN]` is a three-digit sequential number with leading zeros (001, 002, ..., 999).

**Example**: `E02-001` indicates the first diagnostic in Clause 02 (Lexical Structure and Translation).

[14] Each clause allocates sequential diagnostic codes starting from 001. Within a clause, diagnostic codes should be allocated by subsection to facilitate maintenance and avoid conflicts. For instance, Clause 07 (Type System) may reserve:

- `E07-001` through `E07-099` for §7.1 (Type foundations);
- `E07-100` through `E07-299` for §7.2 (Primitive types);
- `E07-300` through `E07-499` for §7.3 (Composite types);
- and so forth.

[15] Implementations shall report diagnostic codes using this exact format in compiler output, error messages, and diagnostic payloads. Diagnostic payloads shall follow structured schemas defined in Annex E §E.5.3 [implementation.diagnostics.payloads].

[16] Reserved ranges within a clause may be documented to accommodate future expansion of specific subsections without renumbering existing diagnostics. Implementations should not define diagnostic codes outside the ranges specified by this document.

[17] Annex E §E.5 [implementation.diagnostics] provides the authoritative diagnostic code registry, including:

- Complete enumeration of all diagnostic codes across all clauses;
- Structured payload schemas for machine-readable diagnostic output;
- Suggested diagnostic message templates and quality guidelines;
- Severity levels (error, warning, note) for each code.

[18] Deprecated diagnostic codes from earlier specification versions shall be documented in the migration mapping (Annex H [changes]). Implementations may recognize legacy codes for backward compatibility but shall normalize them to the canonical format in output.

**NOTE 1**: The two-digit clause prefix supports clauses up to 99. Current specification planning does not anticipate exceeding 20 clauses.

**NOTE 2**: The three-digit sequential number provides 999 codes per clause. If a clause exhausts its allocation, the specification editors should consider subdividing the clause or consolidating related diagnostics.

**NOTE 3**: Warning and note diagnostics may use prefix `W` or `N` in future versions (e.g., `W02-001`, `N02-001`). Currently, all diagnostics use the `E` prefix.

#### §1.6.7 Example Guidelines [intro.document.examples.guidelines]

[19] All examples in the specification should follow these guidelines:

**Completeness:**
- Examples should be self-contained (define all referenced types)
- Include necessary imports and declarations
- Show expected output or error for invalid examples

**Clarity:**
- Use descriptive names (`buffer`, `owner`, `viewer`) not single letters (`a`, `b`, `c`)
- Include inline comments explaining key concepts
- Mark invalid code with `// error[E##-###]` showing expected diagnostic

**Relevance:**
- Each example demonstrates exactly one concept (or a deliberate interaction)
- Examples are minimal - no extraneous code
- Real-world patterns preferred over contrived examples

**Consistency:**
- Reuse common type names across specification (Buffer, Point, Data, Resource)
- Follow language style guide (naming conventions, formatting)
- Use consistent comment style

---

**Previous**: §1.5 Conformance (§1.5 [intro.conformance]) | **Next**: §1.7 Versioning and Evolution (§1.7 [intro.versioning])
