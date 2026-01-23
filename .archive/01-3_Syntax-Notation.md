# 2.3 Syntax Notation

## Overview

This section specifies the metasyntactic notation used throughout this specification to describe the syntactic structure of Cursive programs. The notation employs Extended Backus-Naur Form (EBNF) to express grammar productions formally and unambiguously.

The complete normative grammar for Cursive is consolidated in Annex A (normative). Grammar productions appearing within the body of this specification are informative illustrations; in case of conflict with Annex A, the Annex A production is authoritative.

---

## 2.3.1 Grammar Organization

### 2.3.1.1 Dual Presentation Model

**Definition 2.17 (Dual Presentation)**: The Cursive Language Specification employs a dual presentation model for grammar productions:

1. **In-context grammar**: Grammar fragments appearing within semantic sections of this specification (informative)
2. **Consolidated grammar**: The complete grammar specified in Annex A (normative, authoritative)

**Rationale**: In-context grammar productions provide convenient reference within semantic discussions, while Annex A provides a unified, authoritative reference for implementers.

### 2.3.1.2 Grammar Authority

The following normative statement establishes the authority of Annex A:

> **Normative Statement 2.3.1**: The complete grammar for Cursive is specified in Annex A (normative). In-context grammar productions throughout this specification are informative illustrations only. For complete production rules, an implementation shall consult Annex A.

**Synchronization requirement**:

> **Requirement 2.3.1**: In-context grammar productions shall match the corresponding productions in Annex A. In case of conflict, Annex A is authoritative.

**Implication**: If a conforming implementation discovers a discrepancy between an in-context production and the corresponding Annex A production, the implementation shall follow Annex A and should report the discrepancy to the language specification maintainers.

### 2.3.1.3 Extended Backus-Naur Form (EBNF)

**Definition 2.18 (EBNF Notation)**: Grammar productions in this specification employ Extended Backus-Naur Form with the following metasyntactic operators:

| Symbol | Meaning | Formal Semantics |
|--------|---------|------------------|
| `::=` | is defined as | Production definition |
| `\|` | alternatives | Disjunction (A \| B matches A or B) |
| `*` | zero or more | Kleene star (A* matches ε \| A \| AA \| AAA \| ...) |
| `+` | one or more | Kleene plus (A+ matches A \| AA \| AAA \| ...) |
| `?` | optional | Option (A? matches ε \| A) |
| `~` | negation | Complement (~A matches any input not matching A) |
| `[ ]` | character class or optional | Context-dependent: [a-z] is character class, [A] is optional A |
| `( )` | grouping | Grouping (precedence override) |

**Well-formedness constraints**:

```
[EBNF-Well-Formed-1]
For all nonterminals N appearing on the right-hand side of any production,
N must be defined by some production in the grammar.
───────────────────────────────────────────────────────────────
Grammar is well-formed with respect to nonterminal references
```

```
[EBNF-Well-Formed-2]
For all productions P,
P must be finite and unambiguous after applying ambiguity resolution rules.
───────────────────────────────────────────────────────────────
Grammar is well-formed with respect to ambiguity
```

**Notation conventions**:

1. **Nonterminal symbols**: Written in PascalCase (e.g., `Expression`, `TypeAnnotation`)
2. **Terminal symbols**: Written in double quotes (e.g., `"let"`, `"+"`, `"("`)
3. **Character classes**: Written in square brackets (e.g., `[a-z]`, `[0-9]`)
4. **Productions**: One production per line, with alternatives separated by `|`
5. **Whitespace**: Implicitly allowed between tokens unless explicitly constrained

### 2.3.1.4 Grammar Presentation Style

**In-context productions** typically include:
- 2-5 production rules sufficient for the semantic discussion
- Contextual commentary explaining constraints
- Cross-references to related productions

**Example (informative)**:
```ebnf
LetDeclaration ::= "let" Pattern (":" Type)? "=" Expr Separator
```

**Consolidated productions** (Annex A) include:
- All production rules without omission
- Complete alternatives for each nonterminal
- No explanatory text within production blocks

### 2.3.1.5 Ambiguity Resolution

**Definition 2.19 (Ambiguity)**: A grammar is ambiguous if there exists an input string that admits multiple distinct parse trees.

**Disambiguation mechanisms** employed in this specification:

1. **Operator precedence**: Specified explicitly in §[REF_TBD] (Expression Precedence)
2. **Operator associativity**: Left-associative unless explicitly specified otherwise
3. **Longest match rule**: Lexical analysis applies maximal munch (§2.2.11)
4. **Context sensitivity**: Certain productions are resolved by semantic context (e.g., generics vs comparison operators)

**Requirement 2.3.2**: Where a production admits syntactic ambiguity, the specification shall provide explicit disambiguation rules either inline or via cross-reference.

---

## 2.3.2 Syntax Categories

The Cursive grammar is organized into nine major syntactic categories, each addressing a distinct phase of syntactic analysis and semantic interpretation.

### 2.3.2.1 Major Syntax Categories

**Definition 2.20 (Syntax Category)**: A syntax category is a collection of related production rules addressing a coherent aspect of program syntax.

The following major syntax categories are defined:

1. **Lexical syntax** (Annex A.1): Tokens, identifiers, literals, keywords, operators, punctuators
2. **Type syntax** (Annex A.2): Type expressions, type constructors, permission types, modal types, function types
3. **Pattern syntax** (Annex A.3): Match patterns, destructuring patterns, wildcard patterns, literal patterns
4. **Expression syntax** (Annex A.4): Value-producing constructs, precedence hierarchy, operator expressions
5. **Statement syntax** (Annex A.5): Control flow statements, declaration statements, loop constructs
6. **Declaration syntax** (Annex A.6): Top-level declarations, module-level constructs, visibility qualifiers

### 2.3.2.2 Specialized Syntax Categories

In addition to major categories, the following specialized syntax categories address domain-specific language features:

7. **Contract syntax** (Annex A.7): Preconditions (`must`), postconditions (`will`), invariants (`invariant`), assertions
8. **Assertion syntax** (Annex A.8): Logical predicates, quantifiers (`forall`, `exists`), propositions
9. **Effect syntax** (Annex A.9): Effect paths, effect sets, effect annotations

**Cross-category dependencies**:

```
[Category-Dependencies]
Category C₁ depends on category C₂ if:
  Productions in C₁ reference nonterminals defined in C₂
─────────────────────────────────────────────────────
C₁ may not be parsed without C₂
```

**Example dependencies**:
- Expression syntax depends on Type syntax (type annotations in expressions)
- Statement syntax depends on Expression syntax (expression statements)
- Declaration syntax depends on Statement syntax (function bodies)

### 2.3.2.3 Attribute Syntax

**Attributes** provide a mechanism for attaching metadata to declarations and items. Attribute syntax is integrated into the lexical grammar (Annex A.1):

```ebnf
Attribute     ::= "[[" AttributeBody "]]"
AttributeBody ::= Ident ("(" AttrArgs? ")")?
```

Attribute semantics are specified in §[REF_TBD] (Attributes).

---

## 2.3.3 Reference to Appendix A (Complete Grammar)

### 2.3.3.1 Annex A Organization

**Annex A** consolidates all grammar productions into a single normative reference organized as follows:

| Section | Title | Coverage |
|---------|-------|----------|
| **A.1** | Lexical Grammar | Tokens, identifiers, literals, keywords, operators, punctuators, comments |
| **A.2** | Type Grammar | All type constructors, permission types, modal types, function types, generics |
| **A.3** | Pattern Grammar | All pattern matching forms including tuple, record, enum, modal patterns |
| **A.4** | Expression Grammar | Complete expression syntax with precedence hierarchy, all operators |
| **A.5** | Statement Grammar | All statement forms including control flow, loops, declarations |
| **A.6** | Declaration Grammar | Top-level declarations: functions, types, modules, predicates, implementations |
| **A.7** | Contract Grammar | Contract clauses: `must`, `will`, `where`, `invariant` |
| **A.8** | Assertion Grammar | Logical predicates, quantifiers, propositions, refinement types |
| **A.9** | Effect Grammar | Effect paths, effect sets, effect annotations, effect polymorphism |

### 2.3.3.2 Usage Guidance for Implementers

> **Guidance 2.3.1**: Implementers should consult Annex A for the authoritative and complete grammar specification when constructing parsers or syntax analyzers.

**Implementation strategies**:

1. **Top-down parsing**: Use Annex A productions as the basis for recursive descent parsers
2. **Bottom-up parsing**: Use Annex A productions to construct LR parser tables
3. **Precedence climbing**: Use Annex A.4 precedence annotations for expression parsing

**Validation**: A conforming implementation's parser should accept exactly the set of programs described by Annex A productions, subject to semantic constraints specified elsewhere in this specification.

### 2.3.3.3 Cross-Reference Convention

Throughout this specification, grammar productions are referenced using the following convention:

- **Full production reference**: `Type` (Annex A.2) - refers to the `Type` nonterminal in Annex A.2
- **Section reference**: §A.4.3 - refers to a subsection within Annex A.4
- **Informal reference**: "type expression" - prose description, see Annex A.2 for formal definition

---

## 2.3.4 Canonical Example: EBNF Operators

The following canonical example demonstrates all EBNF metasyntactic operators in a single production:

```ebnf
Expression ::=
    Literal                                  // Terminal symbol
  | Identifier                               // Terminal symbol (nonterminal reference)
  | "(" Expression ")"                       // Grouping
  | Expression BinaryOp Expression           // Sequencing
  | UnaryOp Expression                       // Prefix operator
  | Expression "." Identifier                // Postfix (member access)
  | Expression "[" Expression "]"            // Indexing
  | "if" Expression BlockExpr ("else" BlockExpr)?  // Optional (? operator)
  | "match" Expression "{" MatchArm* "}"     // Zero or more (* operator)
  | Expression ("," Expression)*             // Zero or more repetitions
  | TupleExpr "(" ArgumentList+ ")"          // One or more (+ operator)
  | ~Keyword Identifier                      // Negation (~ operator)
  | Expression (("+" | "-" | "*") Expression)  // Alternatives (| operator)
  | Expression ("." [0-9]+)                  // Character class (tuple index)
```

**Explanation**:

1. **Terminal symbols**: `"("`, `")"`, `"."`, `"["`, `"]"`, `"if"`, `"else"`, `"match"` - literal tokens
2. **Nonterminal symbols**: `Expression`, `Literal`, `Identifier`, `BlockExpr`, `MatchArm`, `ArgumentList`, `UnaryOp`, `BinaryOp`, `Keyword`, `TupleExpr` - references to other productions
3. **Grouping** `( )`: Used to override default precedence, e.g., `("else" BlockExpr)?`
4. **Optional** `?`: `("else" BlockExpr)?` matches zero or one `else` clause
5. **Zero or more** `*`: `MatchArm*` matches zero or more match arms; `("," Expression)*` matches comma-separated expressions
6. **One or more** `+`: `ArgumentList+` matches one or more argument lists; `[0-9]+` matches one or more digits
7. **Alternatives** `|`: `("+" | "-" | "*")` matches any one of the three operators
8. **Negation** `~`: `~Keyword Identifier` matches an identifier that is not a keyword
9. **Character class** `[ ]`: `[0-9]` matches any single digit character

**Parsing semantics**:

- **Precedence**: Inner groupings bind tighter than outer constructs
- **Associativity**: Left-to-right for sequences of operators (e.g., `Expression BinaryOp Expression`)
- **Longest match**: Lexical analysis consumes the longest matching token (§2.2.11)

---

## 2.3.5 Grammar Well-Formedness

### 2.3.5.1 Completeness

**Requirement 2.3.3**: Every nonterminal symbol referenced on the right-hand side of any production shall be defined by at least one production in the grammar.

**Verification**: A grammar is complete if the transitive closure of nonterminal references from the start symbol (`CompilationUnit`) reaches every nonterminal defined in the grammar.

### 2.3.5.2 Reachability

**Requirement 2.3.4**: Every production in the grammar should be reachable from the start symbol through some derivation sequence.

**Rationale**: Unreachable productions indicate dead syntax and may represent specification errors.

### 2.3.5.3 Ambiguity Detection

**Requirement 2.3.5**: Where syntactic ambiguity is inherent (e.g., expression precedence, generic vs comparison), the specification shall provide explicit disambiguation rules.

**Disambiguation approaches**:
1. **Precedence tables**: Explicit operator precedence (§[REF_TBD])
2. **Associativity declarations**: Left, right, or non-associative
3. **Lookahead constraints**: Context-sensitive parsing (e.g., turbofish `::<` for generics)
4. **Semantic resolution**: Type-directed disambiguation during semantic analysis

---

## 2.3.6 Lexical vs Syntactic Grammar

**Definition 2.21 (Lexical Grammar)**: The lexical grammar (Annex A.1) describes the formation of tokens from a character stream during lexical analysis (§2.2.1).

**Definition 2.22 (Syntactic Grammar)**: The syntactic grammar (Annex A.2-A.9) describes the formation of syntactic constructs from a token stream during parsing.

**Phase separation**:

```
[Lexical-Syntactic-Separation]
Character stream -> Lexical Analysis (§2.2) -> Token stream
Token stream -> Syntactic Analysis (§[REF_TBD]) -> Abstract Syntax Tree (AST)
─────────────────────────────────────────────────────────────────────
Lexical and syntactic analysis are distinct phases
```

**Whitespace handling**: Whitespace tokens (§2.2.3) are discarded after lexical analysis and do not participate in syntactic analysis, except where explicitly required (e.g., newline-based statement termination, §2.2.10).

---

## 2.3.7 Conformance Requirements

A conforming implementation shall satisfy the following requirements with respect to syntax notation:

1. **Grammar adherence** (§2.3.1.2): The implementation's parser shall accept exactly the set of programs described by Annex A productions.

2. **EBNF interpretation** (§2.3.1.3): The implementation shall interpret EBNF metasyntactic operators according to their formal semantics specified in Definition 2.18.

3. **Ambiguity resolution** (§2.3.1.5): Where productions admit syntactic ambiguity, the implementation shall apply disambiguation rules as specified in this specification.

4. **Error reporting**: When rejecting syntactically invalid input, the implementation should provide diagnostic information identifying the location and nature of the syntax error.

---

## 2.3.8 Error Codes

This section defines error codes related to syntactic analysis and grammar interpretation:

- **E2012**: Syntax error: unexpected token
- **E2013**: Syntax error: missing expected token
- **E2014**: Syntax error: ambiguous parse (implementation error - grammar should be unambiguous)
- **E2015**: Syntax error: nonterminal not defined (implementation error - grammar should be complete)

**Diagnostic requirements**:

**Requirement 2.3.6**: When emitting error E2012 or E2013, the implementation should provide:
1. File name and line:column position of the error
2. The unexpected or missing token
3. A list of expected tokens at that position (if computable)

---

## 2.3.9 Notes and Examples

### Informative Note 1: Grammar Maintenance

In-context grammar productions in this specification are manually synchronized with Annex A. Future revisions of this specification should employ automated tools to verify synchronization and detect discrepancies.

### Informative Note 2: EBNF Variants

This specification uses a specific dialect of EBNF. Other EBNF dialects (ISO/IEC 14977, W3C EBNF) employ different metasyntactic operators. Implementers should not assume compatibility with other EBNF dialects without careful translation.

### Informative Note 3: Parser Generator Compatibility

Annex A productions are designed to be compatible with LL(k) and LR(1) parser generators. Implementers may need to left-factor or otherwise transform productions for specific parser generator tools.

---

**End of Section 2.3: Syntax Notation**
