# The Cursive Language Specification

## Clause 2 — Lexical Structure and Translation

**Part**: II — Lexical Structure and Translation
**File**: 02-4_Tokenization-and-Statement-Termination.md  
**Section**: §2.4 Tokenization and Statement Termination  
**Stable label**: [lex.terminators]  
**Forward references**: §2.3 [lex.tokens], Clause 8 §8.4 [expr.structured], Annex A §A.2 [grammar.lexical]

---

### §2.4 Tokenization and Statement Termination [lex.terminators]

#### §2.4.1 Overview

[1] This clause refines the behavior of the lexer around newline handling, statement continuation, and the maximal-munch rule used to recognise multi-character operators. It builds on the token categories introduced in §2.3 [lex.tokens] and defines when newline tokens terminate statements implicitly.

### §2.4.2 Syntax

[1] Statement termination operates on the following abstract grammar:

```ebnf
statement_sequence
    ::= statement (newline separator)*

separator
    ::= newline
     | semicolon

continuation
    ::= trailing_operator
     | leading_dot
     | leading_pipeline
     | unclosed_delimiter
```

[ Note: See Annex A §A.2 [grammar.lexical] for the complete lexical grammar including statement termination rules.
— end note ]

[2] A `newline` that satisfies `continuation` does not terminate the preceding statement; otherwise it behaves as a separator.

### §2.4.3 Constraints

[1] _Implicit termination._ A newline shall terminate the current statement unless one of the continuation predicates in §2.4.4 evaluates to true. The same rule applies at end-of-file.

(1.1) _Multiple continuation rules._ When multiple continuation rules apply simultaneously (e.g., a trailing operator followed by a leading dot on the next line), the statement continues. The rules are evaluated independently; any matching rule prevents termination. This allows flexible formatting while maintaining predictable behavior.

(1.2) _EOF in middle of statement._ If end-of-file occurs while a statement is incomplete (unclosed delimiters, trailing operator, or other continuation condition), diagnostic E02-211 (unexpected end of file) is emitted. The lexer shall report the location where the statement began and which continuation condition was active.

[2] _Semicolons._ Semicolons may be used to separate multiple statements on a single line but are never required at line breaks.

[3] _Maximal munch._ When multiple tokenisations are possible at a character position, the lexer shall emit the longest valid token. In generic type contexts the parser may reinterpret `>>` as two closing angle brackets to satisfy the grammar of §7.2 [type.primitive].

(3.1) _Delimiter nesting depth._ Implementations shall support delimiter nesting to at least depth 256. Nested delimiters beyond this limit may be rejected with diagnostic E02-300 (delimiter nesting too deep). This limit prevents stack overflow in pathological cases while accommodating realistic code structures.

### §2.4.4 Semantics

#### §2.4.4.1 Unclosed Delimiters

[1] If the lexer encounters a newline while an opening delimiter `(`, `[`, `{`, or `<` remains unmatched, the newline is treated as whitespace and the statement continues until the delimiter stack is balanced. Diagnostic context highlights the location of the unmatched delimiter when exhaustion occurs.

#### §2.4.4.2 Trailing Operators

[1] When a line ends with a binary or assignment operator (`+`, `-`, `*`, `/`, `%`, `**`, `==`, `!=`, `<`, `<=`, `>`, `>=`, `&&`, `||`, `&`, `|`, `^`, `<<`, `>>`, `..`, `..=`, `=>`, `=`, `+=`, `-=`, `*=`, `/=`, `%=`, `&=`, `|=`, `^=`, `<<=`, `>>=`), the following newline is ignored and the expression continues on the next line.

#### §2.4.4.3 Leading Dot

[1] A line beginning with `.` is treated as a continuation of the previous expression to support fluent member access. The token `::` does not activate this rule; code using the scope operator shall remain on the same line or employ explicit parentheses.

#### §2.4.4.4 Leading Pipeline

[1] A line beginning with the pipeline operator `=>` continues the preceding expression. Pipelines are commonly chained with one entry per line for readability.

#### §2.4.4.5 Maximal Munch

[1] The lexer applies the maximal-munch rule globally. Examples include recognising `<<=` as a single left-shift assignment operator and `..=` as an inclusive range operator. When maximal munch conflicts with generic parsing, the parser reinterprets tokens without altering original lexemes, preserving tooling fidelity.

### §2.4.5 Examples

**Example 2.4.5.1 (Continuation Rules):**

```cursive
let sum = calculate(
    input
)  // newline ignored because '(' was unclosed

let transformed = source
    => normalize
    => render  // leading pipeline

let value = builder
    .step_one()
    .step_two()
```

**Example 2.4.5.2 (Maximal Munch vs Generics):**

```cursive
let shift = value >> 3        // lexer emits single '>>'
let ptr: Ptr<Ptr<i32>> = make_ptr()  // parser splits '>>' into two closers
```

### §2.4.6 Diagnostic Summary

[1] Diagnostics associated with this subclause:

| Code    | Condition                                | Constraint |
| ------- | ---------------------------------------- | ---------- |
| E02-211 | Unexpected end of file during statement  | [1.2]      |
| E02-300 | Delimiter nesting too deep (exceeds 256) | [3.1]      |

### §2.4.7 Conformance Requirements

[1] Implementations shall expose diagnostics that identify the continuation rule responsible when statement termination behaves unexpectedly (e.g., pointing to a trailing operator or unclosed delimiter).

[2] Implementations shall preserve original lexemes even when the parser reinterprets tokens for generic closings so tooling can reconstruct the exact source text.
