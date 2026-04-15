Phase: Cross-Phase / Build Setup and Infrastructure
Rule Name: `Subject(Γ ⊢ j)`
Spec Location: CursiveSpecification.md:213
Compiler Location: cursive/include/00_core/behavior_model.h; cursive/src/00_core/behavior_model.cpp; cursive/tools/generate_static_rule_registry.py; cursive/tools/generate_static_rule_registry.ps1
Ambiguity: `Subject(Γ ⊢ j) = j_0 where j_0 is the leftmost term to the right of ⊢` does not define how to segment multi-token judgment subjects used elsewhere in the specification, including forms such as ``widen` e`, `e ~> m(args)`, `R record wf`, `T satisfies Bounds`, and `@result as T_variant : T_variant`. Without a formal grammar for metatheory judgments or operator/binding rules for the “leftmost term”, implementing `Subject(...)` would require invented semantics.
---
Phase: Phase 1 - Parse and Aggregate
Rule Name: Missing-Terminator-Err
Spec Location: CursiveSpecification.md:1854-1857
Compiler Location: cursive/src/02_source/parser/parser_terminator.cpp:65-107; cursive/src/02_source/lexer/lexer.cpp:458-518; docs/SpecDecisionsNeeded.md (HasTerminator(F, i) ambiguity entry)
Ambiguity: This row depends on the unresolved `HasTerminator(Filter(K), i)` judgment. The current parser emits missing-terminator diagnostics, but the exact spec rule combines that emission with the ambiguous `HasTerminator(F, i)` paragraph, whose lexer/parser decomposition is not defined precisely enough to map onto the live `Filter(K)` token stream and parser boundary-token checks without invention. Until the `HasTerminator(F, i)` row is resolved, this derived error-judgment row cannot be advanced safely.
---
Phase: Phase 1 - Parse and Aggregate
Rule Name: HasTerminator(F, i)
Spec Location: CursiveSpecification.md:1873-1882
Compiler Location: cursive/src/02_source/lexer/lexer.cpp:458-518; cursive/src/02_source/parser/parser_terminator.cpp:36-39,65-107; cursive/src/02_source/parser/parser_consume.cpp:183-244
Ambiguity: The selected row merges multiple distinct obligations into one audit item: the formal `HasTerminator(F, i)` relation over `BoundaryTokens(F, i)`, the requirement that newline continuation inside braces reuse `Continue(K, i)` unchanged, the prohibition on commas as statement terminators, and the cross-reference to `TrailingCommaAllowed (§5.5)`. The live implementation splits those behaviors across the lexer newline filter, parser terminator consumption, and list-parsing helpers, but the specification does not define a single implementation-facing decomposition for this combined paragraph or how the `F` in `HasTerminator(F, i)` maps onto the filtered token stream versus parser-side boundary-token queries. Advancing this selected row would require inventing where those mixed lexer/parser obligations belong.
---
Phase: Phase 1 - Parse and Aggregate
Rule Name: Reserved
Spec Location: CursiveSpecification.md:2048
Compiler Location: cursive/include/00_core/keywords.h:23-76; cursive/src/02_source/parser/expr/if_expr.cpp:221-223; cursive/src/02_source/parser/item/where_clause.cpp:80-99; cursive/src/02_source/parser/item/record_decl.cpp:505-516; cursive/src/02_source/parser/expr/loop_infinite.cpp:210-219; cursive/src/02_source/parser/expr/primary.cpp:332-338
Ambiguity: The selected `Reserved` set excludes `is`, `where`, and `protected`, but the specification later uses `is` as a terminal in `if ... is` syntax and `where` as a terminal in invariants/grammar productions, while the only explicit contextual-keyword set is `{"in", "key", "wait"}`. The live compiler therefore treats `is`/`where` as reserved keywords today, and `protected` is still present in the shared keyword table even though it does not appear in the selected spec set. The spec does not state whether `is` and `where` are intended to be additional contextual keywords, whether the `Reserved` row is incomplete, or whether the later grammar should instead consume identifier tokens with those lexemes. Advancing this row would require inventing which of those conflicting interpretations is normative.
---
Phase: Phase 1 - Parse and Aggregate
Rule Name: Keyword(s)
Spec Location: CursiveSpecification.md:2053
Compiler Location: cursive/include/00_core/keywords.h:23-76,169-173
Ambiguity: `Keyword(s)` is defined directly as membership in `Reserved`, but the selected `Reserved` row is already ambiguous: the written reserved set excludes `is` and `where`, while later grammar still uses them as terminals and the explicit contextual-keyword set does not include them. Because `IsKeyword(s)` in the compiler is implemented as membership in `kCursive0Keywords`, this row cannot be corrected safely until the specification resolves whether `is`/`where` belong in `Reserved`, are additional contextual keywords, or should be parsed as ordinary identifiers in those later grammar positions.
---
Phase: Phase 1 - Parse and Aggregate
Rule Name: `Drop`, `Bitcopy`, `Clone`, and `FfiSafe` are reserved predicate names. They MUST NOT be declared as classes or used as
Spec Location: CursiveSpecification.md:2064
Compiler Location: cursive/src/04_analysis/resolve/scopes_intro.cpp:88-103,134-149; cursive/src/04_analysis/typing/if_case_check.cpp:532-548; cursive/src/04_analysis/typing/stmt/stmt_common.cpp:210-229,237-255; docs/CursiveSpecification.md:4530-4578
Ambiguity: The selected row says `Drop`, `Bitcopy`, `Clone`, and `FfiSafe` MUST NOT be declared as classes or used as user-defined type/value bindings, which reads as a prohibition across all binding scopes. But the later formal `UniverseBindings`, `Intro`, and `ShadowIntro` rules define those names via `UniverseProtected` and then explicitly gate that protection to module scope only with `(S_cur ≠ S_module ∨ x ∉ UniverseProtected)`. The live compiler follows the later formal rules by rejecting those names at module scope only. Advancing this row would require choosing whether the prose sentence overrides the later formal intro rules or whether the prose is shorthand for the module-scope restriction only.
---
Phase: Phase 1 - Parse and Aggregate
Rule Name: Cursive Language Specification > 4. Source Text and Lexical Structure > 4.2 Lexical Analysis > 4.2.6 Literal Lexing paragraph @2256
Spec Location: CursiveSpecification.md:2256
Compiler Location: n/a
Ambiguity: The selected row’s metadata is internally inconsistent. Its `Rule Content` is the heading label `**Leading Zeros.**`, but CursiveSpecification.md:2256 is inside the `NumericCoreEnd(T, i)` rule (`OctRun(T, i+2) if T[i..i+2] = "0o"`), not a heading-only paragraph. The actual `**Leading Zeros.**` heading appears later at CursiveSpecification.md:2288. Advancing this row would require inventing whether the audit should target the heading at line 2288, the `NumericCoreEnd` clause at line 2256, or some other shifted location.
---
Phase: Phase 1 - Parse and Aggregate
Rule Name: Cursive Language Specification > 4. Source Text and Lexical Structure > 4.2 Lexical Analysis > 4.2.6 Literal Lexing paragraph @2260
Spec Location: CursiveSpecification.md:2260-2261
Compiler Location: n/a
Ambiguity: The selected row’s metadata is internally inconsistent. Its `Rule Content` is the `DecimalLeadingZero(T, i, j)` warning judgment with the conclusion `Γ ⊢ Emit(W-SRC-0301, SpanOfText(S, i, j))`, but CursiveSpecification.md:2260-2261 is the `NumericScanEnd(T, i)` equation plus a blank line. The actual warning judgment appears later at CursiveSpecification.md:2292-2294. Advancing this row would require inventing whether the audit should target the `NumericScanEnd` row, the later warning judgment, or some other shifted location.
---
Phase: Phase 1 - Parse and Aggregate
Rule Name: Cursive Language Specification > 4. Source Text and Lexical Structure > 4.2 Lexical Analysis > 4.2.6 Literal Lexing paragraph @2264
Spec Location: CursiveSpecification.md:2264
Compiler Location: n/a
Ambiguity: The selected row’s metadata is internally inconsistent. Its `Rule Content` is the heading label `**EscapeSequences.**`, but CursiveSpecification.md:2264 is the `HasFloatCore(T, i, j)` rule, not a heading paragraph. The actual `**EscapeSequences.**` heading appears later at CursiveSpecification.md:2296. Advancing this row would require inventing whether the audit should target the `HasFloatCore` rule at line 2264, the later heading at line 2296, or some other shifted location.
---
Phase: Phase 1 - Parse and Aggregate
Rule Name: Cursive Language Specification > 4. Source Text and Lexical Structure > 4.2 Lexical Analysis > 4.2.6 Literal Lexing paragraph @2303
Spec Location: CursiveSpecification.md:2303
Compiler Location: n/a
Ambiguity: The selected row’s metadata is internally inconsistent. Its `Rule Content` is the heading label `**Character Literal Encoding.**`, but CursiveSpecification.md:2303 is the `EscapeValue(\0) = 0x00` rule, not a heading paragraph. The actual `**Character Literal Encoding.**` heading appears later at CursiveSpecification.md:2335. Advancing this row would require inventing whether the audit should target the `EscapeValue(\0)` rule at line 2303, the later heading at line 2335, or some other shifted location.
---
Phase: Phase 1 - Parse and Aggregate
Rule Name: Cursive Language Specification > 4. Source Text and Lexical Structure > 4.2 Lexical Analysis > 4.2.6 Literal Lexing paragraph @2333
Spec Location: CursiveSpecification.md:2333
Compiler Location: n/a
Ambiguity: The selected row’s metadata is internally inconsistent. Its `Rule Content` is the heading label `**Literal Tokenization Helpers.**`, but CursiveSpecification.md:2333 is the `Γ ⊢ CharLiteral(T, i) ⇓ q + 1` rule, not a heading paragraph. The actual `**Literal Tokenization Helpers.**` heading appears later at CursiveSpecification.md:2366. Advancing this row would require inventing whether the audit should target the `CharLiteral` rule at line 2333, the later heading at line 2366, or some other shifted location.
---
Phase: Phase 1 - Parse and Aggregate
Rule Name: Cursive Language Specification > 4. Source Text and Lexical Structure > 4.2 Lexical Analysis > 4.2.7 Identifier and Keyword Lexing paragraph @2343
Spec Location: CursiveSpecification.md:2343
Compiler Location: n/a
Ambiguity: The selected row’s metadata is internally inconsistent. Its `Rule Content` is the heading label `**Identifier Scan.**`, but CursiveSpecification.md:2343 is the `FirstBadCharEscape(T, i)` rule inside character-literal encoding, not an identifier-lexing heading. The actual `**Identifier Scan.**` heading appears later at CursiveSpecification.md:2377. Advancing this row would require inventing whether the audit should target the `FirstBadCharEscape` rule at line 2343, the later heading at line 2377, or some other shifted location.
---
Phase: Phase 1 - Parse and Aggregate
Rule Name: Cursive Language Specification > 4. Source Text and Lexical Structure > 4.2 Lexical Analysis > 4.2.7 Identifier and Keyword Lexing paragraph @2361
Spec Location: CursiveSpecification.md:2361
Compiler Location: n/a
Ambiguity: The selected row’s metadata is internally inconsistent. Its `Rule Content` is the heading label `**Keyword Classification.**`, but CursiveSpecification.md:2361 is the `**(Lex-Char-Invalid)**` rule inside character-literal diagnostics, not a heading paragraph. The actual `**Keyword Classification.**` heading appears later at CursiveSpecification.md:2395. Advancing this row would require inventing whether the audit should target the `Lex-Char-Invalid` rule at line 2361, the later heading at line 2395, or some other shifted location.
---
Phase: Phase 1 - Parse and Aggregate
Rule Name: GenericCloseException
Spec Location: CursiveSpecification.md:2416
Compiler Location: n/a
Ambiguity: The selected row’s metadata is internally inconsistent. Its `Rule Content` is `GenericCloseException = false`, but CursiveSpecification.md:2416 is just the blank line before `IsQuote(c)`, not the `GenericCloseException` rule. The actual `GenericCloseException = false` definition appears later at CursiveSpecification.md:2450. Advancing this row would require inventing whether the audit should target the later rule at line 2450 or some other shifted location.
---
Phase: Phase 1 - Parse and Aggregate
Rule Name: Phase1-Forward-Refs
Spec Location: CursiveSpecification.md:2607-2609
Compiler Location: n/a
Ambiguity: The selected row’s metadata is internally inconsistent. Its `Rule Content` is the `**(Phase1-Forward-Refs)**` judgment with conclusion `Γ ⊢ ParsePhase(S) ⇓ NoResolutionConstraints`, but CursiveSpecification.md:2607-2609 covers the blank line after the `4.3` heading and the ownership sentence for source-loading/lexical diagnostics. The actual `**(Phase1-Forward-Refs)**` rule appears later at CursiveSpecification.md:2644-2646. Advancing this row would require inventing whether the audit should target the early prose paragraph or the later inference rule.
---
Phase: Phase 1 - Parse and Aggregate
Rule Name: Cursive Language Specification > 5. Parsing and AST Infrastructure > 5.2 AST Meta-Conventions paragraph @2642
Spec Location: CursiveSpecification.md:2642
Compiler Location: n/a
Ambiguity: The selected row’s metadata is internally inconsistent. Its `Rule Content` is the heading label `**Type.**`, but CursiveSpecification.md:2642 is the conclusion `Γ ⊢ ParsePhase(S) ⇓ F` of `**(Phase1-File)**`, not the AST meta-conventions heading for `Type`. The actual `**Type.**` heading appears later at CursiveSpecification.md:2679. Advancing this row would require inventing whether the audit should target the `Phase1-File` conclusion at line 2642, the later `Type` heading at line 2679, or some other shifted location.
---
Phase: Phase 1 - Parse and Aggregate
Rule Name: Cursive Language Specification > 5. Parsing and AST Infrastructure > 5.3 Parser State and Judgments paragraph @2673
Spec Location: CursiveSpecification.md:2673
Compiler Location: n/a
Ambiguity: The selected row’s metadata is internally inconsistent. Its `Rule Content` is the heading label `**Helper Functions.**`, but CursiveSpecification.md:2673 is the blank line after `ASTItem ∈ {...}`, not the parser-state helper-functions heading. The actual `**Helper Functions.**` heading appears later at CursiveSpecification.md:2711. Advancing this row would require inventing whether the audit should target the blank separator at line 2673, the later heading at line 2711, or some other shifted location.
---
Phase: Phase 1 - Parse and Aggregate
Rule Name: Cursive Language Specification > 4. Source Text and Lexical Structure > 4.3 Source Loading and Lexical Diagnostics paragraph @2571
Spec Location: CursiveSpecification.md:2571
Compiler Location: n/a
Ambiguity: The selected row’s metadata is internally inconsistent. Its `Rule Content` is the prose sentence `This section owns source-loading and lexical diagnostics not reintroduced by later feature chapters.`, but CursiveSpecification.md:2571 is just the blank line before `SensitiveTok(T, i, j, k)`. The actual sentence appears later at CursiveSpecification.md:2608. Advancing this row would require inventing whether the audit should target the blank separator at line 2571, the later ownership sentence at line 2608, or some other shifted location.
---
Phase: Phase 1 - Parse and Aggregate
Rule Name: Cursive Language Specification > 4. Source Text and Lexical Structure > 4.2 Lexical Analysis > 4.2.10 Lexical Security paragraph @2430
Spec Location: CursiveSpecification.md:2430
Compiler Location: n/a
Ambiguity: The selected row’s metadata is internally inconsistent. Its `Rule Content` is the heading label `**Sensitive Positions in a Span.**`, but CursiveSpecification.md:2430 is the `KindPriority(CharLiteral) = 3` rule, not a lexical-security heading paragraph. The actual `**Sensitive Positions in a Span.**` heading appears later at CursiveSpecification.md:2464. Advancing this row would require inventing whether the audit should target the `KindPriority(CharLiteral)` rule at line 2430, the later heading at line 2464, or some other shifted location.
---
Phase: Phase 1 - Parse and Aggregate
Rule Name: Cursive Language Specification > 4. Source Text and Lexical Structure > 4.2 Lexical Analysis > 4.2.10 Lexical Security paragraph @2453
Spec Location: CursiveSpecification.md:2453
Compiler Location: n/a
Ambiguity: The selected row’s metadata is internally inconsistent. Its `Rule Content` is the heading label `**Lexical Security Check.**`, but CursiveSpecification.md:2453 is just the blank line between the `#### 4.2.10 Lexical Security` section title and the `T = S.scalars` binding. The actual `**Lexical Security Check.**` heading appears later at CursiveSpecification.md:2487. Advancing this row would require inventing whether the audit should target the blank separator at line 2453, the later heading at line 2487, or some other shifted location.
---
Phase: Phase 1 - Parse and Aggregate
Rule Name: Cursive Language Specification > 5. Parsing and AST Infrastructure > 5.1 Parsing Inputs, Outputs, and Invariants paragraph @2600
Spec Location: CursiveSpecification.md:2600
Compiler Location: n/a
Ambiguity: The selected row’s metadata is internally inconsistent. Its `Rule Content` is the heading label `**Parsing Phase Invariants.**`, but CursiveSpecification.md:2600 is just the blank line before `**(Tokenize-Err)**`, not a parsing-invariants heading paragraph. The actual `**Parsing Phase Invariants.**` heading appears later at CursiveSpecification.md:2637. Advancing this row would require inventing whether the audit should target the blank separator at line 2600, the later heading at line 2637, or some other shifted location.
---
Phase: Phase 1 - Parse and Aggregate
Rule Name: ASTNode
Spec Location: CursiveSpecification.md:2615
Compiler Location: n/a
Ambiguity: The selected row’s metadata is internally inconsistent. Its `Rule Content` is `ASTNode = ASTItem ∪ ASTExpr ∪ ASTPattern ∪ ASTType ∪ ASTStmt`, but CursiveSpecification.md:2615 is the diagnostics-table row for `E-SRC-0104`, not the AST-node union definition. The actual `ASTNode` definition appears later in the parsing and AST infrastructure section. Advancing this row would require inventing which shifted spec location the audit should actually target.
---
Phase: Phase 1 - Parse and Aggregate
Rule Name: Type
Spec Location: CursiveSpecification.md:2657
Compiler Location: cursive/src/02_source/ast/nodes/ast_types.h:236-254; cursive/src/02_source/ast/ast_common.h:161; docs/CursiveSpecification.md:24749; docs/CursiveSpecification.md:24791; docs/CursiveSpecification.md:24802
Ambiguity: The selected `Type` row defines the ordinary AST type sum without `SpliceExprNode`, but the later quote/splice chapter explicitly states that quoted type parsing is extended with `SpliceExprNode` and `SpliceIdentNode`, and that quoted type position MUST use `$(e)` rather than ordinary `$TypeDynamic` syntax. The live compiler represents that quote-only extension by including `SpliceExprNode` in the active `TypeNode` variant. Advancing this row would require choosing whether §5.2’s `Type` sum is intended to exclude quote-only parse artifacts entirely, or whether Chapter 22 normatively widens the effective type-node family during quoted parsing.
---
Phase: Phase 1 - Parse and Aggregate
Rule Name: Cursive Language Specification > 5. Parsing and AST Infrastructure > 5.1 Parsing Inputs, Outputs, and Invariants paragraph @2611
Spec Location: CursiveSpecification.md:2611
Compiler Location: n/a
Ambiguity: The selected row’s metadata is internally inconsistent. Its `Rule Content` is the prose sentence `Construct-specific ParseItem, ParseExpr, ParsePattern, ParseType, and ParseStmt rules are defined by the owning feature chapters. ParseModule and ParseModules are defined by §11.5.4.`, but CursiveSpecification.md:2611 is the separator row of the lexical diagnostics table, not that paragraph. The actual sentence appears later at CursiveSpecification.md:2648. Advancing this row would require inventing whether the audit should target the table header location at line 2611, the later prose paragraph at line 2648, or some other shifted location.
---
Phase: Phase 1 - Parse and Aggregate
Rule Name: PState
Spec Location: CursiveSpecification.md:2664
Compiler Location: /mnt/c/dev/cursive/cursive/include/02_source/parser/parser.h:25; /mnt/c/dev/cursive/cursive/src/02_source/parser/parser_state.cpp:39-49; /mnt/c/dev/cursive/cursive/src/02_source/parser/parser_angle.cpp:74-108
Ambiguity: The selected row’s location is shifted relative to the current spec: `PState = ⟨K, i, D, j, d, Δ⟩` appears later at CursiveSpecification.md:2702, while CursiveSpecification.md:2664 is part of the `FillDoc` equations. Separately, the live `Parser` struct carries two extra fields, `owned_tokens` and `eof`, which act as implementation-private backing state for `SplitShiftR` token rewriting and `Tok(P)` EOF synthesis. The spec does not say whether the six-field `PState` tuple is an exact concrete representation requirement or an abstract observable state that may have hidden implementation storage. Advancing this row would require inventing whether those backing fields are forbidden or permitted as non-semantic implementation detail.
---
Phase: Phase 1 - Parse and Aggregate
Rule Name: Cursive Language Specification > 5. Parsing and AST Infrastructure > 5.2 AST Meta-Conventions paragraph @2637
Spec Location: CursiveSpecification.md:2637
Compiler Location: n/a
Ambiguity: The selected row’s metadata is internally inconsistent. Its `Rule Content` is the heading label `**ErrorItem.**`, but CursiveSpecification.md:2637 is the `**Parsing Phase Invariants.**` heading, not the AST meta-conventions heading for `ErrorItem`. The actual `**ErrorItem.**` heading appears later at CursiveSpecification.md:2674. Advancing this row would require inventing whether the audit should target the earlier parsing-phase heading at line 2637, the later `ErrorItem` heading at line 2674, or some other shifted location.
---
Phase: Phase 1 - Parse and Aggregate
Rule Name: Cursive Language Specification > 5. Parsing and AST Infrastructure > 5.3 Parser State and Judgments paragraph @2663
Spec Location: CursiveSpecification.md:2663
Compiler Location: n/a
Ambiguity: The selected row’s metadata is internally inconsistent. Its `Rule Content` is the heading label `**Parser State.**`, but CursiveSpecification.md:2663 is the `FillDoc(n)` equation branch `n[doc := DocDefault] if DocMissing(n)`, not the parser-state heading. The actual `**Parser State.**` heading appears later at CursiveSpecification.md:2701. Advancing this row would require inventing whether the audit should target the fill-defaults equation at line 2663, the later parser-state heading at line 2701, or some other shifted location.
---
Phase: Phase 1 - Parse and Aggregate
Rule Name: Phase1-File
Spec Location: CursiveSpecification.md:2602-2605
Compiler Location: n/a
Ambiguity: The selected row’s metadata is internally inconsistent. Its `Rule Content` is the `**(Phase1-File)**` judgment, but CursiveSpecification.md:2602-2605 covers the `Tokenize-Err` rule plus the `### 4.3 Source Loading and Lexical Diagnostics` header. The actual `**(Phase1-File)**` rule appears later at CursiveSpecification.md:2639-2642. Advancing this row would require inventing whether the audit should target the earlier `Tokenize-Err` block, the later `Phase1-File` rule, or some other shifted location.
---
Phase: Phase 1 - Parse and Aggregate
Rule Name: Cursive Language Specification > 4. Source Text and Lexical Structure > 4.2 Lexical Analysis > 4.2.10 Lexical Security paragraph @2466
Spec Location: CursiveSpecification.md:2466
Compiler Location: n/a
Ambiguity: The selected row’s metadata is internally inconsistent. Its `Rule Content` is the heading label `**Confusable Identifier Checks.**`, but CursiveSpecification.md:2466 is just the blank line between `SensitiveInSpan(T, i, j)` and the `**Unsafe Spans (Token-Only).**` heading. The actual `**Confusable Identifier Checks.**` heading appears later at CursiveSpecification.md:2500. Advancing this row would require inventing whether the audit should target that blank separator at line 2466, the later heading at line 2500, or some other shifted location.
---
Phase: Phase 1 - Parse and Aggregate
Rule Name: Tokenize-Ok
Spec Location: CursiveSpecification.md:2554-2557
Compiler Location: /mnt/c/dev/cursive/cursive/src/02_source/lexer/tokenize.cpp:453-471
Ambiguity: The selected `Tokenize-Ok` rule requires only `Γ ⊢ LexSecure(S, K, Sens) ⇓ ok` before `Tokenize(S) ⇓ (K, D)`, but the same specification chapter separately defines `Γ ⊢ ConfusableCheck(S) ⇑ c` for confusable and mixed-script identifier errors and assigns them lexical diagnostics (`E-SRC-0310`, `E-SRC-0311`). The live compiler runs `ConfusableCheck(source, lexed.output.tokens)` after `LexSecure` and rejects tokenization when it fails. Advancing this row would require inventing whether `ConfusableCheck` is intended to be an additional premise of `Tokenize-Ok` despite not being written there, or whether successful tokenization should ignore confusable-identifier failures and defer them to some later phase.
---
Phase: Phase 1 - Parse and Aggregate
Rule Name: Cursive Language Specification > 4. Source Text and Lexical Structure > 4.2 Lexical Analysis > 4.2.10 Lexical Security paragraph @2433
Spec Location: CursiveSpecification.md:2433
Compiler Location: n/a
Ambiguity: The selected row’s metadata is internally inconsistent. Its `Rule Content` is the heading label `**Unsafe Spans (Token-Only).**`, but CursiveSpecification.md:2433 is the `KindPriority(Identifier) = 2` rule, not a lexical-security heading paragraph. The actual `**Unsafe Spans (Token-Only).**` heading appears later at CursiveSpecification.md:2467. Advancing this row would require inventing whether the audit should target the `KindPriority(Identifier)` rule at line 2433, the later heading at line 2467, or some other shifted location.
---
Phase: Phase 1 - Parse and Aggregate
Rule Name: Cursive Language Specification > 4. Source Text and Lexical Structure > 4.2 Lexical Analysis > 4.2.10 Lexical Security paragraph @2423
Spec Location: CursiveSpecification.md:2423
Compiler Location: n/a
Ambiguity: The selected row’s metadata is internally inconsistent. Its `Rule Content` is the heading label `**Literal and Comment Ranges.**`, but CursiveSpecification.md:2423 is the `∅ otherwise` branch of `Candidates(T, i)`, not a lexical-security heading paragraph. The actual `**Literal and Comment Ranges.**` heading appears later at CursiveSpecification.md:2457. Advancing this row would require inventing whether the audit should target the `Candidates(T, i)` branch at line 2423, the later heading at line 2457, or some other shifted location.
---
Phase: Phase 1 - Parse and Aggregate
Rule Name: Longest(C)
Spec Location: CursiveSpecification.md:2391
Compiler Location: n/a
Ambiguity: The selected row’s metadata is internally inconsistent. Its `Rule Content` is the `Longest(C)` set definition, but CursiveSpecification.md:2391 is the premise line of `**(Lex-Ident-Token)**`, not the `Longest(C)` rule. The actual `Longest(C)` definition appears later at CursiveSpecification.md:2425. Advancing this row would require inventing whether the audit should target the `Lex-Ident-Token` premise at line 2391, the later `Longest(C)` rule at line 2425, or some other shifted location.
---
Phase: Phase 1 - Parse and Aggregate
Rule Name: SourceOf(K)
Spec Location: CursiveSpecification.md:2678
Compiler Location: n/a
Ambiguity: The selected row’s metadata is internally inconsistent. Its `Rule Content` is `SourceOf(K) = S ⇔ Γ ⊢ Tokenize(S) ⇓ (K_raw, D) ∧ K = Filter(K_raw)`, but CursiveSpecification.md:2678 is just the blank line between the `Tok(P)` helper block and the later `EOFSpan(K)` equation. The actual `SourceOf(K)` rule appears later at CursiveSpecification.md:2716. Advancing this row would require inventing whether the audit should target that blank separator at line 2678, the later `SourceOf(K)` rule at line 2716, or some other shifted location.
---
Phase: Phase 1 - Parse and Aggregate
Rule Name: ParseRejectRules
Spec Location: CursiveSpecification.md:2837
Compiler Location: n/a
Ambiguity: The selected row’s metadata is internally inconsistent. CursiveSpecification.md:2837 is the `**(Parse-ModalOpt-No)**` rule, not `ParseRejectRules`, and the row’s `Rule Content` records `ParseRejectRules = ?` even though the only `ParseRejectRules` definition in the specification appears later at CursiveSpecification.md:2875 as `ParseRejectRules = ∅`. Advancing this row would require inventing whether the audit should target the mislocated `Parse-ModalOpt-No` helper rule, the later `ParseRejectRules = ∅` definition, or some other shifted location.
---
Phase: Phase 1 - Parse and Aggregate
Rule Name: ParseRejectRules
Spec Location: CursiveSpecification.md:2837
Compiler Location: n/a
Ambiguity: This selected-row iteration reconfirmed that the audit metadata still does not identify a single implementable target. The selected row names `ParseRejectRules`, points at CursiveSpecification.md:2837, and reports `ParseRejectRules = ?`, but the live specification places `ParseRejectRules = ∅` at CursiveSpecification.md:2875 while line 2837 falls inside the earlier optional-parser helper block. Marking the row complete would require guessing whether the audit should follow the stale line number, the later empty-set rule, or an unrecorded intermediate spec revision.
---
Phase: Phase 1 - Parse and Aggregate
Rule Name: AttrList-Target-Err
Spec Location: CursiveSpecification.md:6493-6496
Compiler Location: cursive/src/04_analysis/attributes/attribute_registry.cpp:434-442, 934-936; cursive/src/04_analysis/typing/item/record_decl.cpp:509-514,1040-1044; cursive/src/04_analysis/typing/item/enum_decl.cpp:280-286; cursive/src/04_analysis/typing/item/class_decl.cpp:475-481,715-721
Ambiguity: §9.1.4 states that any attribute whose target kind is not in `AttrTargets(name)` must fail `AttrListWf(A, τ)` with `Code(Attr-Target-Err)`, which the compiler maps to `E-MOD-2452`. But later spec sections assign different dedicated diagnostics to some of the same invalid-target situations: §9.5.4 says `[[dynamic]]` on a type alias or field is ill-formed, and §9.5.7 assigns `E-CON-0411` / `E-CON-0412`; §23.4.4.6 says `[[library]]` is valid only on `extern` blocks, and §23.4.7 assigns `E-SYS-3345` for `[[library]]` outside an `extern` block. The current compiler follows those later specialized diagnostics. Advancing this row would require inventing whether the generic `AttrList-Target-Err` code must override the later feature-specific diagnostics, or whether the later diagnostics are intended exceptions to the generic attribute-list judgment.
---
Phase: Phase 1 - Parse and Aggregate
Rule Name: Cursive Language Specification > 12. Concrete Data Types > 12.2 Tuples > 12.2.2 Parsing paragraph @8642
Spec Location: CursiveSpecification.md:8642
Compiler Location: cursive/src/02_source/parser/expr/tuple_literal.cpp:97-166; cursive/src/02_source/parser/expr/primary.cpp:102-193; HelloCursive/Compiler/RunCompilerStaticConformance.ps1:14873-14908; HelloCursive/Types/ValueData/TuplesArraysSlicesRanges.cursive:88-92
Ambiguity: The literal `TupleScan(P, d)` paragraph exposes only parenthesis depth `d`, but the current spec-conformance surface also requires two behaviors that cannot be derived from that state alone: `(e,)` must not classify as a tuple expression, and commas nested inside parenthesized array or record expressions such as `([7, 11])` and `(TupleParenRecordProbe { left: 4, right: 6 })` must not trigger tuple classification. The live parser therefore tracks `[]` and `{}` nesting and special-cases the singleton-comma form. Applying the selected paragraph literally would regress those already-landed conformance requirements, while preserving those requirements means inventing extra semantics that the selected rule does not state. The specification needs to clarify whether `TupleScan` intentionally ignores non-paren delimiters and singleton-comma tuples, or whether the existing completed conformance requirements should change.
---
Phase: Phase 1 - Parse and Aggregate
Rule Name: Tok(P)
Spec Location: CursiveSpecification.md:8649-8654
Compiler Location: cursive/src/02_source/parser/expr/primary.cpp:147-171; cursive/src/02_source/parser/expr/tuple_literal.cpp:134-159; HelloCursive/Types/ValueData/TuplesArraysSlicesRanges.cursive:88-92
Ambiguity: This row is a concrete subrule of the already-ambiguous `TupleScan(P, d)` paragraph. A literal implementation of `Tok(P) in {",", ";"} and d = 1 => TupleScan(P, d) = true` plus `d + ParenDelta(Tok(P))` recursion would force commas inside `([7, 11])` and `(TupleParenRecordProbe { left: 4, right: 6 })` to classify the outer form as a tuple, contradicting the existing HelloCursive conformance coverage for parenthesized array and record expressions. The same paragraph family also conflicts with the current singleton-comma handling. Advancing this row without clarification would require inventing whether the spec really intends paren-only scanning or whether non-paren delimiter nesting is an unstated part of the judgment.
