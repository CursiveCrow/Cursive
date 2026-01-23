# The Cursive Language Specification

## Clause 2 — Lexical Structure and Translation

**Part**: II — Lexical Structure and Translation
**File**: 02-3_Lexical-Elements.md
**Section**: §2.3 Lexical Elements
**Stable label**: [lex.tokens]
**Forward references**: §2.1 [lex.source], §2.4 [lex.terminators], Annex A §A.2 [grammar.lexical]

---

### §2.3 Lexical Elements [lex.tokens]

#### §2.3.1 Overview

[1] Lexical analysis transforms the normalised character stream (§2.1 [lex.source]) into a sequence of tokens consumed by the parser (§2.2 [lex.phases]). Tokens carry a kind, lexeme, and source location, while whitespace and non-documentation comments are discarded.

[2] Documentation comments beginning with `///` or `//!` are retained for later association with declarations; all other comments are removed during this phase.

### §2.3.2 Syntax

[1]

> The lexical analysis phase **MUST** classify each non-whitespace, non-comment span of the normalised character stream into exactly one of the following token kinds: `identifier`, `keyword`, `literal`, `operator`, `punctuator`, or `newline`. Whitespace and comments **MUST NOT** be emitted as tokens. Each line feed (U+000A) code point that is not part of a string or character literal **MUST** be represented by a `NEWLINE` token in the token stream.

```ebnf
token
    ::= identifier
     | keyword
     | literal
     | operator
     | punctuator
     | newline
```

[ Note: See Annex A §A.2 [grammar.lexical] for complete lexical grammar.
— end note ]

[2]

> An identifier **MUST** begin with a Unicode scalar value whose Unicode property `XID_Start` is `Yes` or with U+005F LOW LINE (`'_'`), and **MUST** be followed by zero or more Unicode scalar values whose Unicode property `XID_Continue` is `Yes` or that are U+005F LOW LINE (`'_'`). Any lexeme used where an identifier is expected that does not satisfy this shape **MUST NOT** be treated as an identifier. Identifiers **MUST NOT** be equal in spelling to any reserved keyword listed in constraint [4].

### §2.3.3 Constraints

[1] _Whitespace._ Space (U+0020), horizontal tab (U+0009), and form feed (U+000C) act as token separators and are not emitted as tokens. Line feed (U+000A) is emitted as a `NEWLINE` token for use in §2.4 [lex.terminators].

[2] _Comments._

- (2.1) Line comments starting with `//` consume characters to the next line terminator and are discarded.
- (2.2) Block comments delimited by `/*` and `*/` shall nest; unclosed block comments raise diagnostic E-SRC-0306.

> A comment whose first three characters are `///` or `//!` **MUST** be classified as a documentation comment governed by constraint [3] and **MUST NOT** be processed as an ordinary line comment under (2.1).

[3] _Documentation comments._ Comments beginning with `///` or `//!` shall be preserved and attached to the following item or module, respectively. They participate in documentation tooling but do not appear in the token stream.

[4] _Keywords._ The following identifiers are reserved and may not be used as ordinary identifiers: `abstract`, `as`, `async`, `await`, `behavior`, `break`, `by`, `case`, `comptime`, `const`, `continue`, `contract`, `defer`, `else`, `enum`, `exists`, `false`, `forall`, `grant`, `if`, `import`, `internal`, `invariant`, `let`, `loop`, `match`, `modal`, `module`, `move`, `must`, `new`, `none`, `private`, `procedure`, `protected`, `public`, `record`, `region`, `result`, `select`, `self`, `Self`, `shadow`, `shared`, `state`, `static`, `true`, `type`, `unique`, `var`, `where`, `will`, `with`, `witness`.

[5] _Literals._ Numeric literals support decimal, hexadecimal (`0x`), octal (`0o`), and binary (`0b`) prefixes. Underscores `_` may separate digits but shall not appear at the start or end of the literal, immediately after a base prefix, or before a type suffix. Integer literals without a suffix default to type `i32`. Violations raise diagnostic E02-206. Floating-point literals consist of an integer part, optional fractional part, optional exponent, and optional suffix (`f32` or `f64`).

[6] _String and character literals._ Strings are delimited by double quotes and support escape sequences `\n`, `\r`, `\t`, `\\`, `\"`, `\'`, `\0`, `\xNN`, and `\u{...}`. Invalid escapes raise diagnostic E-SRC-0302. Character literals use single quotes and must correspond to a single Unicode scalar value; empty or multi-character literals raise diagnostic E-SRC-0303.

(6.1) _String literal line boundaries._ String literals may not span multiple lines unless escaped newlines (`\n`) are used. An unclosed string literal that reaches end-of-file or encounters a newline without being closed emits diagnostic E-SRC-0301 (unterminated string literal). The lexer shall not attempt to recover by inserting a closing quote; the compilation unit is ill-formed.

(6.2) _Nested block comments._ Block comments nest arbitrarily: `/* outer /* inner */ still outer */` is valid and consumes all characters between the outermost `/*` and `*/`. Unclosed nested comments emit diagnostic E-SRC-0306. Implementations shall track nesting depth and report the nesting level at the point of failure to aid debugging.

(6.3) _Invalid Unicode in identifiers._

> Identifiers **MUST** consist only of Unicode scalar values permitted by §2.3.2[2]. If an identifier contains a scalar value that does not satisfy those rules, the implementation **MUST** emit diagnostic `E-SRC-0307` (invalid Unicode in identifier). The lexer **MUST NOT** attempt to repair such identifiers; they are rejected and produce no tokens.

[7] _Operators and punctuators._ Multi-character operators (e.g., `==`, `!=`, `=>`, `..=`, `<-`) are recognised using maximal munch (§2.4 [lex.terminators]). The reference-binding operator `<-` participates in the same precedence and continuation rules as `=` so that Clause 5 bindings parse unambiguously. Implementations shall disambiguate closing angle brackets in generic type contexts by treating `>>` as two tokens when syntactically required. The glyph `~` is reserved for procedure receiver shorthand (§5.4 [decl.function]) and is tokenised as an operator so that combinations such as `~%` and `~!` are available.

### §2.3.4 Semantics

[1] Tokens retain source-span metadata (file, line, column) so that later phases can provide precise diagnostics and tooling hooks.

[2]

> A documentation comment preserved by constraint [3] **MUST** be associated with the immediately following declaration or module in the same compilation unit if there is no intervening blank line containing non-comment characters. If no such declaration or module exists—because the comment appears at end of file or is separated from the next declaration by one or more blank lines—the documentation comment **MUST** be ignored for semantic purposes while remaining available to tooling; it **MUST NOT** affect the token stream or program well-formedness.

[3]

> Lexical analysis under this subclause **MUST** be applied only to compilation units that have passed the source-ingestion checks of §2.1. Violations of UTF-8 validity, file-size limits, BOM placement, or forbidden control characters **MUST** be diagnosed using `E-SRC-0101`–`E-SRC-0104` (with `W-SRC-0101` permitted for an initial BOM) and **MUST** prevent further translation of the affected compilation unit. Any compilation unit that contains a violation of the lexical rules in §2.3—including identifier, keyword, literal, or comment constraints—**MUST** be classified as ill-formed (§4.2). Implementations **MUST** emit at least one `E-SRC-03xx` diagnostic for such units and **MUST NOT** produce translation artifacts for them, although they **MAY** continue analysis to report additional diagnostics.

### §2.3.5 Examples

**Example 2.3.5.1 (Token Stream):**

```cursive
// comment
let answer = 42
```

[1] Tokens emitted: `NEWLINE`, `KEYWORD("let")`, `IDENTIFIER("answer")`, `OPERATOR("=")`, `INTEGER_LITERAL("42")`, `NEWLINE`.

**Example 2.3.5.2 - invalid (Keyword misuse):**

```cursive
let let = 5  // error[E-SRC-0305]
```

[2] The second `let` is rejected because keywords cannot serve as identifiers (constraint [4]).

**Example 2.3.5.3 (Numeric formatting):**

```cursive
let mask = 0b1111_0000u8
let size = 1_024
```

[3] Underscore placement satisfies constraint [5], so both literals are accepted.

### §2.3.6 Diagnostic Summary

| Code    | Condition                                            | Constraint |
| ------- | ---------------------------------------------------- | ---------- |
| E-SRC-0305 | Reserved keyword used as identifier                  | [4]        |
| E-SRC-0301 | Unterminated string literal                          | [6]        |
| E-SRC-0306 | Unterminated block comment                           | [2], (6.2) |
| E-SRC-0304 | Malformed numeric literal                            | [5]        |
| E-SRC-0302 | Invalid escape sequence                              | [6]        |
| E-SRC-0303 | Invalid character literal                            | [6]        |
| E-SRC-0307 | Invalid Unicode in identifier                        | [6.3]      |

> The conditions in §2.3.3 **MUST** be reported using the `SRC-03` diagnostics defined in this table. Implementations **MUST NOT** repurpose these codes for any other conditions.

### §2.3.7 Conformance Requirements

[1] Implementations shall expose token streams (or equivalent APIs) to tooling with location metadata preserved.

[2] Implementations shall detect and report the diagnostics in §2.3.6 at the earliest phase that can do so without suppressing additional diagnostics.

[3] Implementations may extend the set of contextual keywords provided they do not conflict with the reserved set in constraint [4] and provided the contextual keywords remain usable as identifiers outside the specialised contexts that introduce them.
