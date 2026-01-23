# The Cursive Language Specification

## Clause 2 — Lexical Structure and Translation

**Part**: II — Lexical Structure and Translation
**File**: 02-1_Source-Text-Encoding.md  
**Section**: §2.1 Source Text and Encoding  
**Stable label**: [lex.source]  
**Forward references**: §2.2 [lex.phases], §2.3 [lex.tokens], Annex A §A.2 [grammar.lexical]

---

### §2.1 Source Text and Encoding [lex.source]

#### §2.1.1 Overview

[1] Cursive source input is a sequence of Unicode scalar values that shall be encoded as UTF-8. All subsequent phases (§2.2 [lex.phases]) assume that the implementation has normalised line endings and removed optional metadata such as byte order marks before tokenisation (§2.3 [lex.tokens]).

[2] A _source file_ is the decoded Unicode scalar stream after UTF-8 validation and line-ending normalisation. A _compilation unit_ is the textual content of a single source file after this preprocessing step.

### §2.1.2 Syntax

[1] Source files conform to the following lexical skeleton prior to tokenisation:

```ebnf
source_file
    ::= bom? normalized_line*

normalized_line
    ::= code_point* line_terminator?

line_terminator
    ::= "\n"

bom
    ::= "\uFEFF"
```

[ Note: See Annex A §A.2 [grammar.lexical] for the complete lexical grammar including source file structure.
— end note ]

[2] `code_point` denotes any Unicode scalar value other than control characters disallowed by constraint [4].

### §2.1.3 Constraints

[1] _UTF-8 validation._

> Implementations **MUST** accept only byte streams that decode to legal UTF-8 sequences. Invalid byte sequences **MUST** elicit diagnostic `E-SRC-0101`.

[2] _Byte order mark handling._

> If the byte stream begins with U+FEFF, the implementation **MUST** strip the BOM before any further analysis. Any occurrence of U+FEFF after the first decoded scalar value **MUST** trigger diagnostic `E-SRC-0103`.

[3] _Line endings._ Implementations shall recognise LF, CR, and CRLF sequences and normalise each to a single LF code point before tokenisation. Mixed line endings are permitted; the normalisation process preserves the number of logical lines.

[4] _Prohibited code points._

> Outside string and character literals, only horizontal tab (U+0009), line feed (U+000A), carriage return (U+000D), and form feed (U+000C) **MAY** appear from Unicode category Cc. U+0000 **MUST NOT** appear anywhere in the source and **MUST** result in diagnostic `E-SRC-0104`.

[5] _File size._

> Implementations **MUST** document an implementation-defined maximum source size and **MUST** accept files of at least 1 MiB. Files exceeding the implementation limit **MUST** raise diagnostic `E-SRC-0102`.

> Diagnostics `E-SRC-0101`–`E-SRC-0104` **MUST** be assigned to the `SRC-01` feature bucket (Source ingestion and normalization). Any compilation unit that triggers one or more of these diagnostics **MUST** be treated as an ill-formed program (§4.2): the implementation **MUST NOT** advance later translation phases for that compilation unit and **MUST** reject it without producing executable artifacts.

### §2.1.4 Semantics

[1] After constraint checks succeed, the normalised LF characters become significant tokens that participate in automatic statement termination (§2.4 [lex.terminators]). Horizontal tabs and spaces serve as token separators but are otherwise ignored by lexical analysis.

[2] Unicode normalisation is implementation-defined. Implementations should accept any of NFC, NFD, NFKC, or NFKD input; if additional normalisation is performed, it shall not alter source line boundaries or byte offsets used for diagnostics.

Formal source-ingestion judgment:

```text
[WF-Src-OK]
decode_utf8(bytes) = scalars
strip_leading_BOM(scalars) = scalars'
normalize_line_endings(scalars') = S
no_prohibited_controls(S)        // per §2.1.3[4]
|bytes| ≤ max_source_size        // per §2.1.3[5]
------------------------------------------------
bytes ⊢src (S, ∅)


[WF-Src-Invalid-UTF8]
decode_utf8(bytes) fails at span s
------------------------------------------------
bytes ⊢src (⊥, { E-SRC-0101 @ s })


[WF-Src-Size-Limit]
decode_utf8(bytes) = scalars
strip_leading_BOM(scalars) = scalars'
normalize_line_endings(scalars') = S
no_prohibited_controls(S)
|bytes| > max_source_size
------------------------------------------------
bytes ⊢src (⊥, { E-SRC-0102 @ file_span })


[WF-Src-Embedded-BOM]
decode_utf8(bytes) = scalars
strip_leading_BOM(scalars) = scalars'
normalize_line_endings(scalars') = S
∃ position p > 0. S[p] = U+FEFF
------------------------------------------------
bytes ⊢src (⊥, { E-SRC-0103 @ p })


[WF-Src-Prohibited-Control]
decode_utf8(bytes) = scalars
strip_leading_BOM(scalars) = scalars'
normalize_line_endings(scalars') = S
∃ position p. S[p] is U+0000
  ∨ (S[p] ∈ Cc \ {U+0009,U+000A,U+000C,U+000D}
     ∧ p is not within a string/character literal span)
------------------------------------------------
bytes ⊢src (⊥, { E-SRC-0104 @ p })
```

### §2.1.5 Examples

**Example 2.1.5.1 (Valid Source Structure):**

```cursive
//! Module overview — documentation comment
let greeting = "Hello, world"  // UTF-8 literal
let delta = "Δ"                // Non-ASCII scalar value
```

[1] The example uses LF line endings, no BOM, and only permitted control characters, so it satisfies constraints [1]–[4].

**Example 2.1.5.2 - invalid (Embedded BOM):**

```cursive
let x = 1
\uFEFFlet y = 2  // error[E-SRC-0103]
```

[2] The second line embeds U+FEFF after decoding, violating constraint [2].

### §2.1.6 Conformance Requirements

[1] Implementations shall document the maximum supported source-file size and the Unicode version they validate against.

[2]

> Diagnostics `E-SRC-0101`–`E-SRC-0104` **MUST** be emitted with a source span that precisely identifies the offending byte sequence or code point. The span's byte offsets and line/column coordinates **MUST** be computed with respect to the preprocessed source file after UTF-8 validation, BOM stripping, and line-ending normalisation (§2.1.1–§2.1.3), and any further Unicode normalisation **MUST NOT** change these offsets (§2.1.4[2]). Suggestions in diagnostic text are informative; the failure remains normative.

> The implementation-defined maximum source size and the Unicode normalisation form used for source text **MUST** be documented as implementation-defined behaviors (IDB) in the implementation's conformance dossier, consistent with Annex B entries B.1.07 and B.1.17. Conforming programs **MUST NOT** rely on undocumented choices for these behaviors.

> When the byte stream begins with a UTF-8 BOM (U+FEFF) and otherwise satisfies the constraints in §2.1.3, a conforming implementation **SHOULD** emit diagnostic `W-SRC-0101` (UTF-8 BOM present) while still accepting the source file.

[3] Implementations may issue warnings when large files or mixed normalisation forms are detected, but such warnings shall not replace the mandatory diagnostics described above.
