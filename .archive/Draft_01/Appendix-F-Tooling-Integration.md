# Appendix F – Tooling Integration [appendix.f.tooling]

This appendix gathers tooling-related requirements for compiler APIs, language servers, metadata generation, and IDE integration that were previously specified in Part III §16.

## F.1 Compiler APIs

### F.1.1 Programmatic Compilation

> Compiler frontends that expose programmatic compilation interfaces **MUST** preserve the same phase ordering and semantics defined in Part II and Part III as when invoked via command-line interfaces. Programmatic APIs **MUST NOT** permit bypassing mandatory diagnostics or altering the meaning of well-formed programs.

### F.1.2 Incremental Compilation Support

> When incremental compilation is supported, observable behavior **MUST** be equivalent to a full non-incremental compilation under the same configuration. Implementations **MUST** ensure that reused artifacts do not cause the omission of required diagnostics.

### F.1.3 Query-Based Compilation

> Query-based compilation interfaces **MUST** reflect the same module graph, scope structure, and binding information that the implementation uses to produce executable artifacts. Queries **MUST NOT** expose intermediate states that violate the invariants established in Part III.

## F.2 Language Server Protocol [appendix.f.lsp]

### F.2.1 LSP Conformance Requirements

> When a Cursive implementation exposes a Language Server Protocol (LSP) endpoint, the server **MUST** implement JSON‑RPC 2.0, follow the LSP initialization and shutdown handshakes, and **MUST NOT** send messages that violate the LSP message structure it advertises in its capabilities. Source positions reported over LSP **MUST** be derived from the same source coordinates used for diagnostics (§12.1.2): line numbers are 0‑based in LSP but 1‑based in the specification, and the `character` field in LSP `Position` objects **MUST** count UTF‑16 code units within the line. Any deviation that causes clients to misinterpret ranges or diagnostics **MUST** be treated as an `E-DIA-0202` (LSP bridge protocol violation).

> LSP diagnostics **MUST** carry the same diagnostic codes, severities, and primary spans that the implementation would emit on the command line for the same compilation unit. Implementations **MUST** document supported LSP capabilities and any limitations (for example, lack of workspace‑wide rename) in the conformance dossier’s `diagnostics` section (§C.2.6).

### F.2.2 Semantic Tokens

> Language servers that provide semantic tokens **MUST** classify tokens using a stable mapping from Cursive token categories (§9.1.1 and Appendix A) to the token types and modifiers defined by the LSP semantic tokens specification. At minimum, implementations **MUST** distinguish identifiers, keywords, literals, comments, types, procedures, and module names. Token ranges **MUST** be expressed in terms of the LSP `Range` type and **MUST** correspond exactly to the underlying token spans.

> Semantic token streams **MUST** be deterministic for a fixed source file and configuration. Changes to the semantic token mapping that would cause clients to mis‑color or mis‑classify tokens **SHOULD** be treated as taxonomy changes and recorded in the implementation’s tooling documentation.

### F.2.3 Code Actions

> Code actions (including quick‑fixes and refactorings) exposed over LSP **MUST NOT** silently change the meaning of well‑typed programs. A code action that may change observable behavior (for example, replacing a construct with a more efficient but semantically different form) **MUST** clearly describe the behavioral impact in its title or documentation and **MUST NOT** be offered as an automatic or “safe” fix.

> When a code action claims to fix a specific diagnostic, applying that action **MUST** eliminate at least one instance of the targeted diagnostic without introducing a new instance of the same diagnostic at the same location. Rewrites **MUST** preserve formatting and comments to the extent possible or clearly communicate any limitations to users.

## F.3 Metadata Generation [appendix.f.metadata]

### F.3.1 Symbol Metadata

> Implementations that emit symbol metadata for tooling (for example, index databases or navigation indices) **MUST** use stable identifiers that can be mapped back to source locations: at minimum, file path, line, column, and a symbol kind derived from the declaration category (value, type, module, behavior, grant, contract, label). Symbol metadata **MUST** reflect the same name resolution and visibility rules as Part III and **MUST NOT** expose symbols that are not reachable according to those rules.

> Symbol metadata formats **SHOULD** be JSON or another structured format and **SHOULD** be documented in the conformance dossier or tooling documentation so third‑party tools can consume them.

### F.3.2 Documentation Comments

> Documentation comments classified by §9.1.3 **MUST** be preserved in a form that tooling can associate with the declarations or modules they document. Extraction tools **MUST** maintain the association between documentation text and its target declaration, including generic parameters and overloads where applicable.

> When documentation metadata is exported (for example, as JSON or a markup format), the export **MUST** include enough information to link back to the documented symbol’s source location and fully qualified name.

### F.3.3 Cross-References

> Cross‑reference indices (for “go to definition”, “find all references”, and similar features) **MUST** be computed from the same name‑resolution results used for compilation (§14.4). Tools **MUST NOT** materialize references that would not be considered valid by the language’s name lookup rules, and **MUST** treat unresolved or ill‑formed references as such.

> When serialized, cross‑reference entries **SHOULD** include the referencing span, the referenced symbol identity as recorded in symbol metadata, and any additional qualifiers needed to disambiguate overloaded or generic items.

## F.4 IDE Integration Points [appendix.f.ide]

### F.4.1 Syntax Highlighting

> Syntax highlighting **SHOULD** be implemented in terms of the token categories and semantic token classifications defined in §9.1 and §F.2.2. IDEs and editors that rely on lexical tokenization **MUST** treat the same set of characters as token boundaries as the compiler’s lexer to avoid drift in highlighting.

### F.4.2 Code Completion

> Code completion engines that integrate with Cursive **MUST** respect the scope and visibility rules of Part III and **MUST NOT** suggest bindings that are not in scope at the completion position. Completions **SHOULD** include type information and available contracts where known, and **SHOULD NOT** suggest constructs that would be ill‑formed in the current context.

### F.4.3 Refactoring Support

> Refactoring operations (such as rename, extract function, or inline variable) **MUST** preserve program well‑formedness and **MUST NOT** introduce new diagnostics of severity error. When a refactoring cannot be applied without changing observable behavior (for example, in the presence of macros or generated code), the tool **MUST** either refuse the refactoring or clearly surface the potential behavior change to the user.

> Tooling hooks that perform refactorings and feed back into the compiler’s diagnostic or build pipeline **MUST** follow the same error‑handling and cancellation semantics as other compiler APIs; failures in such hooks **MUST** be surfaced using diagnostics in the `DIA-02` bucket (for example, `E-DIA-0203`).
