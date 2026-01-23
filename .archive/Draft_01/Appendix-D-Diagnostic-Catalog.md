# Appendix D – Diagnostic Catalog (Normative)

This appendix provides a cross-index of specification-defined diagnostics. The complete normative catalog of diagnostic codes and their detailed descriptions appears in Appendix B, §B.9; this appendix summarizes commonly referenced codes so implementers and tool authors can audit coverage from a single location.

| Code         | Severity | Clause                               | Summary                                                      |
| ------------ | -------- | ------------------------------------ | ------------------------------------------------------------ |
| `E-SRC-0101` | error    | [source.encoding.utf8]               | Invalid UTF‑8 sequence in source input                       |
| `E-SRC-0102` | error    | [source.structure.size]              | Source file exceeds documented size limit                    |
| `E-SRC-0103` | error    | [source.encoding.bom]                | Embedded BOM found after the first scalar value              |
| `E-SRC-0104` | error    | [source.encoding.invalid]            | Forbidden control character outside literals                 |
| `E-SRC-0105` | error    | [source.structure.size]              | Maximum logical line count exceeded                          |
| `E-SRC-0106` | error    | [source.structure.size]              | Maximum line length exceeded                                 |
| `W-SRC-0101` | warning  | [source.encoding.bom]                | UTF‑8 BOM present at start of file                           |
| `E-SRC-0201` | error    | [translation.comptime]               | Comptime recursion depth exceeds guaranteed minimum          |
| `E-SRC-0202` | error    | [translation.comptime]               | Comptime evaluation step budget exceeded                     |
| `E-SRC-0203` | error    | [translation.comptime]               | Comptime heap limit exceeded                                 |
| `E-SRC-0204` | error    | [translation.comptime]               | String literal exceeds guaranteed size during comptime       |
| `E-SRC-0205` | error    | [translation.comptime]               | Collection cardinality exceeds comptime minimum              |
| `E-SRC-0206` | error    | [translation.comptime]               | Comptime body requested undeclared or disallowed grants      |
| `E-SRC-0207` | error    | [translation.comptime]               | Generated identifier collided with an existing declaration   |
| `E-SRC-0208` | error    | [translation.comptime]               | Cycle detected in comptime dependency graph                  |
| `E-SRC-0301` | error    | [lexical.literals.string]            | Unterminated string literal                                  |
| `E-SRC-0302` | error    | [lexical.literals.string]            | Invalid escape sequence                                      |
| `E-SRC-0303` | error    | [lexical.literals.character]         | Invalid character literal                                    |
| `E-SRC-0304` | error    | [lexical.literals.numeric]           | Malformed numeric literal                                    |
| `E-SRC-0305` | error    | [lexical.keywords.reserved]          | Reserved keyword used as identifier                          |
| `E-SRC-0306` | error    | [lexical.elements.comments]          | Unterminated block comment                                   |
| `E-SRC-0307` | error    | [lexical.identifiers.xid]            | Invalid Unicode in identifier                                |
| `W-SRC-0301` | warning  | [lexical.literals.numeric]           | Numeric literal with leading zeros                           |
| `W-SRC-0308` | warning  | [lexical.elements.security]          | Lexically sensitive Unicode character in identifier/boundary |
| `E-SRC-0308` | error    | [lexical.elements.security]          | Lexically sensitive Unicode character in strict mode         |
| `E-SRC-0401` | error    | [syntax.organization.statements]     | Unexpected EOF during unterminated statement or continuation |
| `E-SRC-0402` | error    | [syntax.limits.delimiters]           | Delimiter nesting depth exceeded supported bound             |
| `E-SRC-0501` | error    | [syntax.organization.declarations]   | Statement not permitted at module scope                      |
| `E-SRC-1101` | error    | [modules.manifest]                   | Manifest file not found or unreadable                        |
| `E-SRC-1102` | error    | [modules.manifest.requirements]      | Source root path in manifest is not unique                   |
| `E-SRC-1103` | error    | [modules.paths.grammar]              | Module path component is not a valid identifier              |
| `E-SRC-1104` | error    | [modules.paths.grammar]              | Module path case collision on case-insensitive filesystem    |
| `W-SRC-1105` | warning  | [modules.paths.grammar]              | Potential module path case collision                         |
| `E-SRC-1106` | error    | [modules.manifest.format]            | Malformed manifest structure                                 |
| `W-SRC-1106` | warning  | [modules.manifest.format]            | Unknown manifest key                                          |
| `E-SRC-1107` | error    | [modules.manifest.requirements]      | Manifest language version incompatible with compiler         |
| `E-SRC-1108` | error    | [modules.assemblies.linkage]         | Assembly dependency cycle detected                           |
| `E-SRC-1201` | error    | [names.imports.aliases]              | Import alias name already bound                              |
| `E-SRC-1202` | error    | [names.imports.forms]                | Undefined module path in import                              |
| `E-SRC-1203` | error    | [names.imports.policies]             | Assembly import policy violation                             |
| `E-SRC-1204` | error    | [modules.paths.grammar]              | Malformed module path syntax                                 |
| `E-SRC-1205` | error    | [modules.paths.limits]               | Module path exceeds maximum component count                  |
| `E-SRC-1301` | error    | [initialization.graph.cycles]        | Eager dependency cycle detected                              |
| `E-SRC-1302` | error    | [initialization.eager.timing]        | Access to uninitialized eager dependency                     |
| `E-SRC-1303` | error    | [initialization.failure.propagation] | Module initialization failure propagation                    |
| `E-SRC-1401` | error    | [syntax.limits.blocks] / [names.scopes] | Scope nesting exceeds implementation limit                |
| `E-SRC-1501` | error    | [names.bindings]                     | Redeclaration of binding in same scope                       |
| `E-SRC-1502` | error    | [names.bindings] / [conformance.reserved.universe] | Invalid shadowing of universe-protected binding   |
| `W-SRC-1501` | warning  | [names.bindings]                     | Variable shadows outer binding                               |
| `W-SRC-1502` | warning  | [names.bindings]                     | Unused binding                                               |
| `E-SRC-1601` | error    | [names.lookup]                       | Undefined name in unqualified lookup                         |
| `E-SRC-1602` | error    | [names.lookup]                       | Ambiguous unqualified name lookup                            |
| `E-SRC-1603` | error    | [names.lookup]                       | Undefined qualified name                                     |
| `E-SRC-1604` | error    | [names.lookup]                       | Item not visible in qualified access                         |
| `E-SRC-1605` | error    | [names.lookup]                       | Qualified name uses undefined alias                          |
| `E-CNF-0101` | error    | [evolution.extensions.flags]         | Unknown feature flag                                         |
| `E-CNF-0102` | error    | [evolution.extensions.flags]         | Conflicting feature flags                                    |
| `E-CNF-0103` | error    | [evolution.extensions.flags]         | Required feature not enabled                                 |
| `W-CNF-0101` | warning  | [evolution.lifecycle.stability]      | Experimental feature in use                                  |
| `W-CNF-0102` | warning  | [evolution.lifecycle.deprecation]    | Deprecated feature in use                                    |
| `E-CNF-0201` | error    | [conformance.obligations.modes] / [modules.manifest.format] | Invalid conformance mode                  |
| `E-CNF-0202` | error    | [conformance.obligations.modes] / [modules.manifest.format] | Conflicting conformance settings           |
| `E-CNF-0203` | error    | [conformance.obligations.modes]      | Conformance mode unsupported for target                      |
| `E-CNF-0204` | error    | [conformance.obligations.dossier]    | Missing or invalid conformance dossier                       |
| `E-CNF-0205` | error    | [conformance.obligations.programs]   | Missing UVB attestation for UVB site                         |
| `W-CNF-0201` | warning  | [conformance.obligations.modes]      | Permissive or compatibility mode used for production build   |
| `E-CNF-0301` | error    | [evolution.versioning.declaration]   | Incompatible language version                                |
| `E-CNF-0302` | error    | [evolution.versioning]               | Future language version syntax                               |
| `E-CNF-0303` | error    | [evolution.versioning]               | Edition mismatch                                             |
| `W-CNF-0301` | warning  | [evolution.lifecycle.deprecation]    | Deprecated syntax form                                       |
| `E-DIA-0101` | error    | [diagnostics.codes.stability]        | Invalid diagnostic code format                               |
| `E-DIA-0102` | error    | [appendix.c.json] / [diagnostics.requirements] | Diagnostic JSON schema violation                     |
| `E-DIA-0103` | error    | [diagnostics.codes.categories]       | Diagnostic category not defined                              |
| `E-DIA-0201` | error    | [diagnostics.requirements.content]   | Invalid or out-of-range source span                          |
| `E-DIA-0202` | error    | [appendix.f.lsp]                     | LSP bridge protocol violation                                |
| `E-DIA-0203` | error    | [appendix.f.tooling]                 | Tooling hook callback failure                                |
| `E-DIA-0204` | error    | [appendix.f.lsp]                     | Span encoding incompatible with tooling                      |
| `E-DIA-0301` | error    | [appendix.b.taxonomy]                | Diagnostic bucket overflow                                   |
| `E-DIA-0302` | error    | [diagnostics.codes.stability]        | Diagnostic code conflict                                     |
