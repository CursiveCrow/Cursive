# Appendix A – Formal ANTLR Grammar (Normative)

**Part**: Appendix  
**Section**: Appendix A – Grammar (Part II – Source Text & Translation)  
**Stable label**: \[grammar.partII\]

---

This appendix defines the normative lexical and token grammar corresponding to Part II (§§8–11). It is written in ANTLR‑style EBNF. Character classes and Unicode property references are to be interpreted using the Unicode version and validation rules documented by each implementation’s conformance dossier (§6.2.4, §8.1.4).

> **Scope.** This Annex currently covers only the lexical categories and token grammar referenced from Part II (source ingestion, lexing, and tokenization). Later revisions of this specification will extend Appendix A to cover the full concrete syntax (declarations, statements, expressions) while maintaining the obligations stated in §10.1 and §10.3.

## A.1 Tokens and Lexical Categories

```ebnf
<token>
    ::= <identifier>
     |  <keyword>
     |  <literal>
     |  <operator>
     |  <punctuator>
     |  <newline>
```

The categories above correspond exactly to the token kinds defined in §9.1.1. Whitespace and comments are not tokens; they act only as separators and documentation (§9.1.2–§9.1.3).

### A.1.1 Identifiers

```ebnf
<identifier>
    ::= <ident_start> <ident_continue>*

<ident_start>
    ::= /* any Unicode scalar value that has Unicode property
           XID_Start = Yes, or U+005F LOW LINE ('_'), after the
           preprocessing pipeline of §8.1 */

<ident_continue>
    ::= /* any Unicode scalar value that has Unicode property
           XID_Continue = Yes, or U+005F LOW LINE ('_'), after
           preprocessing (§8.1) */
```

[ Note: The concrete sets corresponding to `<ident_start>` and `<ident_continue>` are determined by the Unicode version chosen by the implementation. That Unicode version, together with any normalization behavior, is documented in the conformance dossier (§8.1.4). — end note ]

### A.1.2 Newlines and Whitespace

After Phase 1 ingestion (§8.1.0) and line ending normalization (§8.2.1), only the Line Feed code point (U+000A) is used as a line terminator.

```ebnf
<newline>
    ::= U+000A          // single LF, the canonical line terminator

<whitespace>
    ::= (U+0020 | U+0009 | U+000C)+
```

The `<whitespace>` nonterminal represents one or more occurrences of space (U+0020), horizontal tab (U+0009), or form feed (U+000C). These characters act solely as token separators and never form tokens of their own (§9.1.2).

### A.1.3 Comments

Comments are removed from the parser token stream. Line comments are terminated (but do not consume) the next `<newline>`; documentation comments are preserved for association with declarations and modules, but never appear as `<token>`.

```ebnf
<line_comment>
    ::= "//" <comment_char>*

<doc_comment>
    ::= "///" <comment_char>*

<module_doc>
    ::= "//!" <comment_char>*

<block_comment>
    ::= "/*" ( <block_comment> | <comment_char> )* "*/"

<comment_char>
    ::= /* any Unicode scalar value other than:
            - U+000A LINE FEED, and
            - the start/end delimiters of the corresponding comment kind,
           subject to the prohibitions of §8.1.3 (forbidden control
           characters and U+0000) */
```

[ Note: `<block_comment>` is specified as a recursively‑nesting construct to reflect the requirement in §9.1.3 that block comments nest. Implementations MAY implement this either in the lexer or via a preprocessing pass, provided the observable behavior of tokenization matches §9.1.3 and this grammar. — end note ]

### A.1.4 Keywords

The lexical category `<keyword>` consists exactly of the reserved words listed in §9.2.1. These lexemes are never recognized as `<identifier>`.

```ebnf
<keyword>
    ::= "abstract"
     |  "as"
     |  "async"
     |  "await"
     |  "behavior"
     |  "break"
     |  "by"
     |  "case"
     |  "comptime"
     |  "const"
     |  "continue"
     |  "contract"
     |  "defer"
     |  "else"
     |  "enum"
     |  "exists"
     |  "false"
     |  "forall"
     |  "grant"
     |  "if"
     |  "import"
     |  "internal"
     |  "invariant"
     |  "let"
     |  "loop"
     |  "match"
     |  "modal"
     |  "module"
     |  "move"
     |  "must"
     |  "new"
     |  "none"
     |  "private"
     |  "procedure"
     |  "protected"
     |  "public"
     |  "record"
     |  "region"
     |  "result"
     |  "select"
     |  "self"
     |  "Self"
     |  "shadow"
     |  "shared"
     |  "state"
     |  "static"
     |  "true"
     |  "type"
     |  "unique"
     |  "var"
     |  "where"
     |  "will"
     |  "with"
     |  "witness"
```

### A.1.5 Literals

```ebnf
<literal>
    ::= <numeric_literal>
     |  <string_literal>
     |  <char_literal>
```

#### A.1.5.1 Numeric literals

```ebnf
<numeric_literal>
    ::= <integer_literal>
     |  <float_literal>

<integer_literal>
    ::= <decimal_integer>
     |  <hex_integer>
     |  <octal_integer>
     |  <binary_integer>

<decimal_integer>
    ::= <dec_digit> ("_"? <dec_digit>)*

<hex_integer>
    ::= "0x" <hex_digit> ("_"? <hex_digit>)*

<octal_integer>
    ::= "0o" <oct_digit> ("_"? <oct_digit>)*

<binary_integer>
    ::= "0b" <bin_digit> ("_"? <bin_digit>)*

<float_literal>
    ::= <decimal_integer> "." <decimal_integer>? <exponent_part>? <float_suffix>?
     |  <decimal_integer> <exponent_part> <float_suffix>?
     |  "." <decimal_integer> <exponent_part>? <float_suffix>?

<exponent_part>
    ::= ("e" | "E") ("+" | "-")? <decimal_integer>

<float_suffix>
    ::= "f32" | "f64"

<dec_digit>
    ::= "0" | "1" | "2" | "3" | "4" | "5" | "6" | "7" | "8" | "9"

<hex_digit>
    ::= <dec_digit>
     |  "a" | "b" | "c" | "d" | "e" | "f"
     |  "A" | "B" | "C" | "D" | "E" | "F"

<oct_digit>
    ::= "0" | "1" | "2" | "3" | "4" | "5" | "6" | "7"

<bin_digit>
    ::= "0" | "1"
```

These productions are required to describe the same set of lexemes as the numeric literal rules in §9.4.1, including the underscore placement and exponent constraints used by `E-SRC-0304`.

#### A.1.5.2 String literals

```ebnf
<string_literal>
    ::= "\"" <string_char>* "\""

<string_char>
    ::= <string_escape>
     |  <string_non_escape>

<string_non_escape>
    ::= /* any Unicode scalar value except:
            - U+0022 QUOTATION MARK ('"'),
            - U+005C REVERSE SOLIDUS ('\\'),
            - control characters prohibited by §8.1.3 (including U+0000) */

<string_escape>
    ::= "\\n"
     |  "\\r"
     |  "\\t"
     |  "\\\\"
     |  "\\\""
     |  "\\'"
     |  "\\0"
     |  "\\x" <hex_digit> <hex_digit>
     |  "\\u{" <hex_digit>+ "}"
```

This grammar matches the permitted escape sequences in §9.4.2. Any other sequence beginning with `\` inside a string literal is malformed and is diagnosed as `E-SRC-0302`.

#### A.1.5.3 Character literals

```ebnf
<char_literal>
    ::= "'" <char_body> "'"

<char_body>
    ::= <string_escape>
     |  <char_non_escape>

<char_non_escape>
    ::= /* exactly one Unicode scalar value other than:
            - U+0027 APOSTROPHE ('\''),
            - U+005C REVERSE SOLIDUS ('\\'),
            - control characters prohibited by §8.1.3 (including U+0000) */
```

Character literals that are empty, contain more than one scalar value after escape interpretation, or are unterminated are ill‑formed as described in §9.4.3 (`E-SRC-0303`).

#### A.1.5.4 Boolean literals

```ebnf
<boolean_literal>
    ::= "true"
     |  "false"
```

The lexemes `true` and `false` are tokens of kind `<keyword>` as defined in §9.2.1; the nonterminal `<boolean_literal>` is used by the concrete syntax to refer to these keyword tokens when they appear in expression positions.

### A.1.6 Operators and Punctuators

The operator and punctuator categories mirror §9.2.2.

```ebnf
<operator>
    ::= "=="
     |  "!="
     |  "<="
     |  ">="
     |  "&&"
     |  "||"
     |  "<<"
     |  ">>"
     |  "<<="
     |  ">>="
     |  ".."
     |  "..="
     |  "=>"
     |  "**"
     |  "->"
     |  "::"

<punctuator>
    ::= "+" | "-" | "*" | "/" | "%"
     |  "<" | ">" | "=" | "!" | "&" | "|" | "^"
     |  "." | "," | ":" | ";"
     |  "(" | ")" | "[" | "]" | "{" | "}"
```

When multiple tokenizations are possible at a character position, the lexer applies the maximal‑munch rule over `<operator>` and `<punctuator>` as described in §9.2.2 and §10.2; the exceptional handling of `>>` in generic contexts is governed by the parsing rules rather than by this lexical grammar.

## A.2 Source File Physical Structure

The physical structure of a preprocessed source file matches §8.2.3.

```ebnf
<source_file>
    ::= <normalized_line>*

<normalized_line>
    ::= <code_point>* <newline>?

<code_point>
    ::= /* any Unicode scalar value that is not prohibited by §8.1.3
           (including the constraints on control characters and U+0000) */
```

There is a one‑to‑one correspondence between source files and compilation units (§8.3.1). Phase 1 of the translation pipeline (§11.2) produces a normalized `<source_file>` which is then tokenized according to the grammar in §A.1 and §9.
