# Appendix B – Diagnostic Code Taxonomy (Normative) [appendix.b.taxonomy]

This appendix defines the complete taxonomy of diagnostic codes used throughout the specification. The code schema `K-CAT-FFNN` is defined in Part II, §12.2.1 of the main specification, and this appendix provides the detailed feature bucket assignments for each category and selected diagnostic catalog entries. The complete diagnostic catalog appears in Appendix D of the main specification.

## B.1 Feature Bucket Reference

> For the `SRC` category, feature buckets (`FF`) **MUST** follow this table so diagnostic codes remain stable across parts:

| Bucket | Scope (SRC)                                                                      | Examples                        |
| ------ | -------------------------------------------------------------------------------- | ------------------------------- |
| `01`   | Source ingestion and normalization (UTF-8, BOM, control characters, size limits) | `E-SRC-0101`                    |
| `02`   | Translation pipeline and comptime resource enforcement                           | `E-SRC-0201…0208`               |
| `03`   | Lexical tokens, literals, comments                                               | `E-SRC-0301…0307`               |
| `04`   | Statement termination, syntactic nesting limits, delimiter handling, EOF conditions | `E-SRC-0401…0402`           |
| `05`   | Module-scope form restrictions                                                   | `E-SRC-0501`                    |
| `11`   | Module discovery, manifests, and case collisions                                 | `E-SRC-1101…1104`, `W-SRC-1105` |
| `12`   | Imports and alias formation                                                      | `E-SRC-1201…1203`               |
| `13`   | Module dependency graphs and initialization ordering                             | `E-SRC-1301…1303`               |
| `14`   | Scope tree formation                                                             | `E-SRC-1401`                    |
| `15`   | Shadowing and binding metadata (including universe protections)                  | `E-SRC-1501…1504`               |
| `16`   | Name lookup (unqualified/qualified, alias conflicts)                             | `E-SRC-1601…1607`               |

> Each remaining category **MUST** define disjoint feature buckets with the same two-digit format.

## B.2 `TYP` Category Buckets

| Bucket | Scope (TYP)                                          | Examples          |
| ------ | ---------------------------------------------------- | ----------------- |
| `01`   | Type formation, kinding, alias expansion             | `E-TYP-0101`      |
| `02`   | Declaration well-formedness (records, enums, modals) | `E-TYP-0201…0210` |
| `03`   | Generic parameters, variance, instantiation          | `E-TYP-0301…0308` |
| `04`   | Permission annotations and defaults                  | `E-TYP-0401…0404` |
| `05`   | Contract/witness attachment to types                 | `E-TYP-0501…0505` |

## B.3 `MEM` Category Buckets

| Bucket | Scope (MEM)                                         | Examples          |
| ------ | --------------------------------------------------- | ----------------- |
| `01`   | Responsibility/move semantics                       | `E-MEM-0101…0108` |
| `02`   | Permission lattice violations at runtime boundaries | `E-MEM-0201`      |
| `03`   | Region creation/escape analysis                     | `E-MEM-0301…0306` |
| `04`   | Pointer provenance/dereference state errors         | `E-MEM-0401…0407` |
| `05`   | Concurrency and happens-before diagnostics          | `E-MEM-0501…0504` |

## B.4 `EXP` Category Buckets

| Bucket | Scope (EXP)                                       | Examples          |
| ------ | ------------------------------------------------- | ----------------- |
| `01`   | Expression typing/evaluation order                | `E-EXP-0101…0105` |
| `02`   | Statement/control-flow violations (if/match/loop) | `E-EXP-0201…0210` |
| `03`   | Pipeline/operator misuse                          | `E-EXP-0301…0306` |
| `04`   | Comptime execution failures inside expressions    | `E-EXP-0401…0403` |
| `05`   | Code-generation/quote misuse                      | `E-EXP-0501…0504` |

## B.5 `GRN` Category Buckets

| Bucket | Scope (GRN)                          | Examples          |
| ------ | ------------------------------------ | ----------------- |
| `01`   | Grant accumulation and leakage       | `E-GRN-0101…0104` |
| `02`   | Sequent syntax/verification failures | `E-GRN-0201…0208` |
| `03`   | Contract Liskov compliance           | `E-GRN-0301…0303` |
| `04`   | Witness grant propagation            | `E-GRN-0401…0404` |

## B.6 `FFI` Category Buckets

| Bucket | Scope (FFI)                                              | Examples          |
| ------ | -------------------------------------------------------- | ----------------- |
| `01`   | Signature/ABI conformance (unsupported types, variadics) | `E-FFI-0101…0106` |
| `02`   | Calling convention and linkage attributes                | `E-FFI-0201…0204` |
| `03`   | Panic/unwind boundaries and null-termination rules       | `E-FFI-0301…0305` |
| `04`   | Unsafe pointer obligations crossing FFI                  | `E-FFI-0401…0404` |

## B.7 `DIA` Category Buckets

| Bucket | Scope (DIA)                                | Examples          |
| ------ | ------------------------------------------ | ----------------- |
| `01`   | Diagnostic payload structure/JSON schema   | `E-DIA-0101…0103` |
| `02`   | Tooling hooks (LSP bridges, span encoding) | `E-DIA-0201…0204` |
| `03`   | Diagnostic taxonomy consistency            | `E-DIA-0301…0302` |

## B.8 `CNF` Category Buckets

| Bucket | Scope (CNF)                              | Examples                        |
| ------ | ---------------------------------------- | ------------------------------- |
| `01`   | Feature enablement/disablement controls  | `E-CNF-0101…0103`, `W-CNF-0101` |
| `02`   | Conformance-mode selection conflicts and dossier/UVB attestation conformance failures | `E-CNF-0201…0205`, `W-CNF-0201` |
| `03`   | Versioning/deprecation policy violations | `E-CNF-0301…0303`               |

---

## B.9 Complete Diagnostic Catalog

This section provides the complete catalog of all specification-defined diagnostics organized by category and bucket. Each diagnostic includes its code, severity, specification reference, and a detailed description.

### B.9.1 SRC Category - Source Text, Lexical, Translation

#### Bucket 01: Source Ingestion and Normalization

**E-SRC-0101: Invalid UTF-8 sequence in source input**
- **Severity**: Error
- **Reference**: Part II, §8.1.1 ([source.encoding.utf8])
- **Description**: The source file contains byte sequences that are not valid UTF-8. All Cursive source files MUST be encoded in UTF-8.
- **Triggering Condition**: Byte sequence fails UTF-8 validation
- **Example**: Binary file passed as source, file with Latin-1 encoding containing characters >U+007F

**E-SRC-0102: Source file exceeds documented size limit**
- **Severity**: Error
- **Reference**: Part II, §8.2.2 ([source.structure.size])
- **Description**: The source file size exceeds the implementation's documented maximum file size limit.
- **Triggering Condition**: File size > implementation-defined limit (must be at least 1MB per conformance requirements)
- **Example**: Attempting to compile a 100MB source file when implementation limit is 10MB

**E-SRC-0103: Embedded BOM found after the first scalar value**
- **Severity**: Error
- **Reference**: Part II, §8.1.2 ([source.encoding.bom])
- **Description**: A Byte Order Mark (U+FEFF) appears after the beginning of the file. BOMs are only permitted at the start of the file.
- **Triggering Condition**: U+FEFF appears at any position other than file offset 0
- **Example**: File with BOM inserted mid-content after a previous edit

**E-SRC-0104: Forbidden control character or null byte encountered**
- **Severity**: Error
- **Reference**: Part II, §8.1.3 ([source.encoding.invalid])
- **Description**: A prohibited code point appears in the normalized source file, as defined by §8.1.3. This includes the null character (U+0000) in any context (including inside literals) and any other forbidden control character that appears outside string or character literals.
- **Triggering Condition**: First occurrence of a prohibited code point per §8.1.3
- **Example**: U+0000 embedded in a string literal, or a disallowed control character between tokens

**E-SRC-0105: Maximum logical line count exceeded**
- **Severity**: Error
- **Reference**: Part II, §8.2.2 ([source.structure.size]); Part II, §8.1.5 ([source.encoding.locations])
- **Description**: The normalized source file contains more logical lines than the implementation’s documented maximum logical line count.
- **Triggering Condition**: `line_count(normalized_source) > max_lines` as documented in the implementation’s conformance dossier
- **Example**: Generated source file with millions of lines exceeding the documented maximum logical line count

**E-SRC-0106: Maximum line length exceeded**
- **Severity**: Error
- **Reference**: Part II, §8.2.2 ([source.structure.size]); Part II, §8.1.5 ([source.encoding.locations])
- **Description**: At least one logical line in the normalized source file contains more Unicode scalar values than the implementation’s documented maximum line length.
- **Triggering Condition**: `max_line_length(normalized_source) > max_columns` as documented in the implementation’s conformance dossier
- **Example**: A single line containing an entire generated program or very long literal that exceeds the documented maximum line length

**W-SRC-0101: UTF-8 BOM present**
- **Severity**: Warning
- **Reference**: Part II, §8.1.2 ([source.encoding.bom])
- **Description**: The source file begins with a UTF-8 Byte Order Mark. While permitted, BOMs are unnecessary for UTF-8 and may cause issues with some tools.
- **Triggering Condition**: File begins with bytes EF BB BF
- **Example**: File saved with "UTF-8 with BOM" encoding

#### Bucket 02: Translation Pipeline and Comptime Resource Enforcement

**E-SRC-0201: Comptime recursion depth exceeds guaranteed minimum**
- **Severity**: Error
- **Reference**: Part II, §11.6 ([translation.comptime])
- **Description**: Compile-time function recursion exceeded the implementation's guaranteed minimum recursion depth as required by §6.4.1 (at least the specified number of stack frames).
- **Triggering Condition**: Comptime call stack depth > implementation's documented recursion limit (which MUST meet or exceed §6.4.1)
- **Example**: Deeply recursive comptime computation whose call stack exceeds the implementation's documented recursion limit

**E-SRC-0202: Comptime evaluation step budget exceeded**
- **Severity**: Error
- **Reference**: Part II, §11.6 ([translation.comptime])
- **Description**: Compile-time evaluation consumed more execution steps than the implementation's guaranteed minimum budget.
- **Triggering Condition**: Comptime instruction count > guaranteed minimum
- **Example**: Infinite loop in comptime code, or excessively expensive computation

**E-SRC-0203: Comptime heap limit exceeded**
- **Severity**: Error
- **Reference**: Part II, §11.6 ([translation.comptime])
- **Description**: Compile-time evaluation attempted to allocate more memory than the implementation's guaranteed minimum heap size for comptime execution.
- **Triggering Condition**: Comptime heap allocation > guaranteed minimum
- **Example**: Building very large arrays or data structures at compile time

**E-SRC-0204: String literal exceeds guaranteed size during comptime**
- **Severity**: Error
- **Reference**: Part II, §11.6 ([translation.comptime]); Part II, §9 ([lexical.literals])
- **Description**: A string literal generated or manipulated during compile-time execution exceeds the implementation's guaranteed maximum string length for comptime values.
- **Triggering Condition**: Comptime string length > guaranteed minimum limit
- **Example**: Generating megabyte-sized strings through comptime concatenation

**E-SRC-0205: Collection cardinality exceeds comptime minimum**
- **Severity**: Error
- **Reference**: Part II, §11.6 ([translation.comptime])
- **Description**: A compile-time collection (array, map, set) exceeded the implementation's guaranteed minimum size limit.
- **Triggering Condition**: Comptime collection size > guaranteed minimum
- **Example**: Building arrays with millions of elements at compile time

**E-SRC-0206: Comptime body requested undeclared or disallowed grants**
- **Severity**: Error
- **Reference**: Part VIII, §38 ([contracts.grants]); Part II, §11.6 ([translation.comptime])
- **Description**: A comptime block attempted to use a grant (capability) that was not declared or is not permitted in compile-time contexts.
- **Triggering Condition**: Comptime code requires grant not in declared grant set
- **Example**: Attempting file I/O in comptime without declaring `grant:io.file` capability

**E-SRC-0207: Generated identifier collided with an existing declaration**
- **Severity**: Error
- **Reference**: Part II, §11.6 ([translation.comptime]); Part X, §49 ([comptime.codegen])
- **Description**: Code generation or metaprogramming produced an identifier that conflicts with an existing declaration in the same scope.
- **Triggering Condition**: Generated name ∈ existing names in scope
- **Example**: Macro generating function `foo` when `foo` already exists

**E-SRC-0208: Cycle detected in comptime dependency graph**
- **Severity**: Error
- **Reference**: Part II, §11.6 ([translation.comptime])
- **Description**: Compile-time evaluation dependencies form a cycle, making it impossible to determine evaluation order.
- **Triggering Condition**: Circular dependency in comptime constant/function evaluation
- **Example**: `comptime X = Y + 1; comptime Y = X + 1;`

#### Bucket 03: Lexical Tokens, Literals, Comments

**E-SRC-0301: Unterminated string literal**
- **Severity**: Error
- **Reference**: Part II, §9
- **Description**: A string literal is not properly terminated before the end of the line or file.
- **Triggering Condition**: String opening quote without matching closing quote before newline/EOF
- **Example**: `let s = "hello;` (missing closing quote)

**E-SRC-0302: Invalid escape sequence**
- **Severity**: Error
- **Reference**: Part II, §9
- **Description**: A string or character literal contains an escape sequence that is not recognized by the language.
- **Triggering Condition**: Backslash followed by invalid escape character
- **Example**: `"hello\q"` (\\q is not a valid escape), `"\x"` (incomplete hex escape)

**E-SRC-0303: Invalid character literal**
- **Severity**: Error
- **Reference**: Part II, §9
- **Description**: A character literal is malformed (empty, multiple characters, or improperly terminated).
- **Triggering Condition**: Character literal violates syntax rules
- **Example**: `''` (empty), `'ab'` (multiple chars), `'a` (unterminated)

**E-SRC-0304: Malformed numeric literal**
- **Severity**: Error
- **Reference**: Part II, §9
- **Description**: A numeric literal has invalid syntax (invalid digits for base, malformed exponent, invalid suffix).
- **Triggering Condition**: Numeric literal fails lexical validation
- **Example**: `0x` (no digits), `0b2` (2 invalid in binary), `1e` (incomplete exponent), `123xyz` (invalid suffix)

**E-SRC-0305: Reserved keyword used as identifier**
- **Severity**: Error
- **Reference**: Part II, §9
- **Description**: A reserved keyword was used where an identifier was expected.
- **Triggering Condition**: Keyword appears in identifier position
- **Example**: `let let = 5;`, `procedure if() { }`

**E-SRC-0306: Unterminated block comment**
- **Severity**: Error
- **Reference**: Part II, §9
- **Description**: A block comment (`/* ... */`) is not properly terminated before end of file.
- **Triggering Condition**: `/*` without matching `*/` before EOF
- **Example**: `/* This comment never ends`

**E-SRC-0307: Invalid Unicode in identifier**
- **Severity**: Error
- **Reference**: Part II, §9; References §5.2 (UAX31)
- **Description**: An identifier contains Unicode characters that are not permitted by UAX31 XID_Start/XID_Continue rules.
- **Triggering Condition**: Identifier character ∉ XID_Start ∪ XID_Continue
- **Example**: Identifier starting with combining character, identifier containing emoji (unless they're defined as valid)

**W-SRC-0301: Numeric literal with leading zeros**
- **Severity**: Warning
- **Reference**: Part II, §9
- **Description**: A decimal numeric literal has leading zeros, which may indicate confusion with octal notation from other languages.
- **Triggering Condition**: Decimal literal begins with `0` followed by more digits
- **Example**: `let x = 0123;` (might be intended as 123 or confused with octal 0o123)

**W-SRC-0308: Lexically sensitive Unicode character in identifier or token boundary**
- **Severity**: Warning
- **Reference**: Part II, §9.1.4 ([lexical.elements.security])
- **Description**: A lexically sensitive Unicode character (such as bidirectional formatting characters or zero-width joiners) appears unescaped in an identifier, operator or punctuator lexeme, or immediately adjacent to token boundaries in non-comment, non-literal context.
- **Triggering Condition**: First occurrence of a lexically sensitive character in the contexts described by §9.1.4
- **Example**: Identifier containing U+202E RIGHT-TO-LEFT OVERRIDE, or zero-width joiner inserted between two tokens to alter their visual appearance

**E-SRC-0308: Lexically sensitive Unicode character treated as error in strict mode**
- **Severity**: Error
- **Reference**: Part II, §9.1.4 ([lexical.elements.security]); Part I, §6.2.7 ([conformance.obligations.modes])
- **Description**: A lexically sensitive Unicode character is present in a context that would trigger `W-SRC-0308`, but the compilation is running in a strict conformance mode that upgrades this condition to an error.
- **Triggering Condition**: `W-SRC-0308` condition occurs while a strict conformance mode is in effect
- **Example**: Strict-mode build of code that contains a bidirectional formatting character inside an identifier

#### Bucket 04: Statement Termination, Delimiter Handling, EOF Conditions

**E-SRC-0401: Unexpected EOF during unterminated statement or continuation**
- **Severity**: Error
- **Reference**: Part II, §10.1.2 ([syntax.organization.statements]); Part II, §11.4 ([translation.parsing])
- **Description**: The file ended while a statement or expression was incomplete.
- **Triggering Condition**: EOF while parser expects continuation
- **Example**: `procedure foo() {` (no closing brace), `let x = (1 + 2` (unclosed parenthesis)

**E-SRC-0402: Delimiter nesting depth exceeded supported bound**
- **Severity**: Error
- **Reference**: Part II, §10.2.4 ([syntax.limits.delimiters]); Part II, §11.4 ([translation.parsing])
- **Description**: Nested delimiters (parentheses, brackets, braces) exceeded the implementation's guaranteed minimum delimiter nesting depth as required by §6.4.1.
- **Triggering Condition**: Delimiter nesting depth > implementation's documented delimiter limit (which MUST meet or exceed §6.4.1)
- **Example**: Extremely deeply nested array literals or function calls that push delimiter nesting beyond the documented limit

#### Bucket 05: Module-Scope Form Restrictions

**E-SRC-0501: Statement not permitted at module scope**
- **Severity**: Error
- **Reference**: Part II, §10.1.1 ([syntax.organization.declarations])
- **Description**: A statement form that is only permitted inside function/procedure bodies appeared at module scope.
- **Triggering Condition**: Module-level syntax contains non-declaration statement
- **Example**: `return 5;` at module scope, `while true { }` outside procedure

#### Bucket 11: Module Discovery, Manifests, and Case Collisions

**E-SRC-1101: Manifest file not found or unreadable**
- **Severity**: Error
- **Reference**: Part III, §13.3 ([modules.manifest])
- **Description**: The required `Cursive.toml` manifest file could not be found in the project root or could not be read.
- **Triggering Condition**: Missing or inaccessible Cursive.toml
- **Example**: Project directory without manifest file, manifest with incorrect permissions

**E-SRC-1102: Source root path in manifest is not unique**
- **Severity**: Error
- **Reference**: Part III, §13.3.1 ([modules.manifest.requirements])
- **Description**: The manifest declares multiple source roots that resolve to the same normalized path.
- **Triggering Condition**: Duplicate normalized paths in `[roots]` section
- **Example**: `[roots]` containing both `src = "source"` and `src2 = "./source"` pointing to same directory

**E-SRC-1103: Module path component is not a valid identifier**
- **Severity**: Error
- **Reference**: Part III, §13.4.1 ([modules.paths.grammar])
- **Description**: A folder name in the module path is not a valid Cursive identifier.
- **Triggering Condition**: Directory name violates identifier rules
- **Example**: Directory named `my-module` (hyphens not allowed), `2ndmodule` (starts with digit)

**E-SRC-1104: Module path case collision on case-insensitive filesystem**
- **Severity**: Error
- **Reference**: Part III, §13.4.1 ([modules.paths.grammar])
- **Description**: On a case-insensitive filesystem, two module paths differ only in case, creating ambiguity.
- **Triggering Condition**: Paths collide when case-normalized on Windows/macOS
- **Example**: Directories `myModule` and `mymodule` in same parent on Windows

**W-SRC-1105: Potential module path case collision**
- **Severity**: Warning
- **Reference**: Part III, §13.4.1 ([modules.paths.grammar])
- **Description**: Module paths differ only in case, which may cause issues on case-insensitive filesystems even though the current system is case-sensitive.
- **Triggering Condition**: Multiple paths with same case-normalized form
- **Example**: `MyModule` and `mymodule` directories on Linux (warning for cross-platform portability)

**E-SRC-1106: Malformed manifest structure**
- **Severity**: Error
- **Reference**: Part III, §13.3.2 ([modules.manifest.format])
- **Description**: The manifest file contains TOML syntax errors or violates the required schema structure.
- **Triggering Condition**: Invalid TOML or missing required fields
- **Example**: Missing `[language]` section, `language.version` field not a string, malformed TOML syntax

**W-SRC-1106: Unknown manifest key**
- **Severity**: Warning
- **Reference**: Part III, §13.3.2 ([modules.manifest.format])
- **Description**: The manifest contains keys that are not recognized by this implementation.
- **Triggering Condition**: Key not in recognized set and not in vendor namespace
- **Example**: Typo in key name, future specification feature used with older compiler

**E-SRC-1107: Manifest language version incompatible with compiler**
- **Severity**: Error
- **Reference**: Part III, §13.3.1 ([modules.manifest.requirements]); Part I, §7.1.3 ([evolution.versioning.declaration])
- **Description**: The `language.version` in the manifest has a MAJOR version that doesn't match the compiler's MAJOR version.
- **Triggering Condition**: MAJOR(manifest.version) ≠ MAJOR(compiler.version)
- **Example**: Manifest specifies `version = "2.0.0"` but compiler is version 1.x

**E-SRC-1108: Assembly dependency cycle detected**
- **Severity**: Error
- **Reference**: Part III, §13.3.3 ([modules.assemblies.linkage])
- **Description**: Assembly dependency declarations form a cycle, making it impossible to determine build order.
- **Triggering Condition**: Cycle in assembly dependency graph
- **Example**: Assembly A depends on B, B depends on C, C depends on A

#### Bucket 12: Imports and Alias Formation

**E-SRC-1201: Import alias name already bound**
- **Severity**: Error
- **Reference**: Part III, §14.1.4 ([names.imports.aliases])
- **Description**: An import alias conflicts with an existing alias or declaration in the current scope.
- **Triggering Condition**: Alias name ∈ existing names in module scope
- **Example**: `import foo as bar; import baz as bar;` (duplicate alias)

**E-SRC-1202: Undefined module path in import**
- **Severity**: Error
- **Reference**: Part III, §14.1.2 ([names.imports.forms])
- **Description**: An import statement references a module path that cannot be resolved.
- **Triggering Condition**: Module path not found in project or dependencies
- **Example**: `import nonexistent::module;` when module doesn't exist

**E-SRC-1203: Assembly import policy violation**
- **Severity**: Error
- **Reference**: Part III, §14.1.7 ([names.imports.policies])
- **Description**: An import violates assembly-level import restrictions or policies.
- **Triggering Condition**: Import violates assembly configuration rules
- **Example**: Attempting to import from restricted assembly, circular assembly reference

**E-SRC-1204: Malformed module path syntax**
- **Severity**: Error
- **Reference**: Part III, §13.4.1 ([modules.paths.grammar])
- **Description**: The module path has invalid syntax (leading/trailing `::`, empty components).
- **Triggering Condition**: Module path fails grammar validation
- **Example**: `import ::foo;` (leading ::), `import foo::;` (trailing ::), `import foo::::bar;` (empty component)

**E-SRC-1205: Module path exceeds maximum component count**
- **Severity**: Error
- **Reference**: Part III, §13.4.2 ([modules.paths.limits])
- **Description**: The module path contains more than the maximum allowed 32 components.
- **Triggering Condition**: Path component count > 32
- **Example**: `import a::b::c::...::deeply::nested::module;` with 33+ components

#### Bucket 13: Module Dependency Graphs and Initialization Ordering

**E-SRC-1301: Eager dependency cycle detected**
- **Severity**: Error
- **Reference**: Part III, §15.1.3 ([initialization.graph.cycles])
- **Description**: Module initialization dependencies form a cycle that cannot be resolved.
- **Triggering Condition**: Cycle in eager dependency edges
- **Example**: Module A's initializer reads constant from B, B's initializer reads constant from A

**E-SRC-1302: Access to uninitialized eager dependency**
- **Severity**: Error
- **Reference**: Part III, §15.2.3 ([initialization.eager.timing])
- **Description**: Code attempted to access a module-level value from a module that has not yet been initialized.
- **Triggering Condition**: Eager dependency consumed before initialization
- **Example**: Module A initializer uses value from B before B is initialized in topological order

**E-SRC-1303: Module initialization failure propagation**
- **Severity**: Error
- **Reference**: Part III, §15.4.1 ([initialization.failure.propagation])
- **Description**: Module initialization failed, blocking all modules that depend on it via eager dependencies.
- **Triggering Condition**: Module init panic/error with dependent modules
- **Example**: Module A panics during initialization, modules B and C that depend on A are blocked

#### Bucket 14: Scope Tree Formation

**E-SRC-1401: Scope nesting exceeds implementation limit**
- **Severity**: Error
- **Reference**: Part II, §10.2.2 ([syntax.limits.blocks]); Part III, §14.2.3 ([names.scopes])
- **Description**: Nested scopes (corresponding to blocks and other scope-forming constructs) exceeded the implementation's guaranteed minimum scope depth as required by §6.4.1.
- **Triggering Condition**: Scope nesting depth > implementation's documented scope limit (which MUST meet or exceed §6.4.1)
- **Example**: Extremely deeply nested blocks or function definitions that exceed the documented maximum scope depth

#### Bucket 15: Shadowing and Binding Metadata

**E-SRC-1501: Redeclaration of binding in same scope**
- **Severity**: Error
- **Reference**: Part III, §14.3.1 ([names.bindings])
- **Description**: A name is declared multiple times in the same scope.
- **Triggering Condition**: Duplicate name in single scope
- **Example**: `let x = 1; let x = 2;` in same block

**E-SRC-1502: Invalid shadowing of universe-protected binding**
- **Severity**: Error
- **Reference**: Part III, §14.3.4 ([names.bindings])
- **Description**: Attempted to shadow a compiler-intrinsic or universe-level protected name.
- **Triggering Condition**: Shadowing of protected name
- **Example**: `let i32 = "string";` (shadowing built-in type)

**W-SRC-1501: Variable shadows outer binding**
- **Severity**: Warning
- **Reference**: Part III, §14.3.3 ([names.bindings])
- **Description**: A binding shadows another binding from an outer scope, which may be unintentional.
- **Triggering Condition**: Inner scope binding has same name as outer scope binding
- **Example**: `let x = 1; { let x = 2; }` (inner x shadows outer x)

**W-SRC-1502: Unused binding**
- **Severity**: Warning
- **Reference**: Part III, §14.3 ([names.bindings])
- **Description**: A binding is declared but never used.
- **Triggering Condition**: Binding not referenced after declaration
- **Example**: `let x = compute_value();` where x is never read

#### Bucket 16: Name Lookup (Unqualified/Qualified, Alias Conflicts)

**E-SRC-1601: Undefined name in unqualified lookup**
- **Severity**: Error
- **Reference**: Part III, §14.4.2 ([names.lookup])
- **Description**: An unqualified name could not be resolved in any visible scope.
- **Triggering Condition**: Name ∉ accessible scopes
- **Example**: `let y = undefined_variable;`

**E-SRC-1602: Ambiguous unqualified name lookup**
- **Severity**: Error
- **Reference**: Part III, §14.4.3 ([names.lookup])
- **Description**: An unqualified name resolves to multiple candidates with equal priority.
- **Triggering Condition**: Multiple matching names in lookup
- **Example**: Using name imported from two different modules without qualification

**E-SRC-1603: Undefined qualified name**
- **Severity**: Error
- **Reference**: Part III, §14.4.1 ([names.lookup])
- **Description**: A qualified name `module::item` references a non-existent item in the module.
- **Triggering Condition**: Item not found in specified module
- **Example**: `foo::nonexistent` when module foo exists but item doesn't

**E-SRC-1604: Item not visible in qualified access**
- **Severity**: Error
- **Reference**: Part III, §14.4.1 ([names.lookup])
- **Description**: The qualified name references an item that exists but is not publicly exported.
- **Triggering Condition**: Item is private/not exported
- **Example**: Accessing `other_module::private_function` from outside the module

**E-SRC-1605: Qualified name uses undefined alias**
- **Severity**: Error
- **Reference**: Part III, §14.4.1 ([names.lookup])
- **Description**: A qualified name uses an alias that was not declared.
- **Triggering Condition**: Alias ∉ declared imports
- **Example**: `undefined_alias::item` when alias was never imported

### B.9.2 TYP Category - Type System and Type Checking

#### Bucket 01: Type Formation, Kinding, Alias Expansion

**E-TYP-0101: Invalid type constructor**
- **Severity**: Error
- **Reference**: Part III, §11 (Type Foundations)
- **Description**: A type expression uses invalid syntax or references an undefined type constructor.
- **Triggering Condition**: Type constructor not found or malformed
- **Example**: `let x: UndefinedType = ...;`

**E-TYP-0102: Type alias expansion cycle**
- **Severity**: Error
- **Reference**: Part III, §11
- **Description**: Type aliases form a cycle that prevents expansion.
- **Triggering Condition**: Cycle in type alias dependency graph
- **Example**: `type A = B; type B = A;`

**E-TYP-0103: Type parameter count mismatch**
- **Severity**: Error
- **Reference**: Part III, §15 (Generics)
- **Description**: A generic type is instantiated with the wrong number of type parameters.
- **Triggering Condition**: Argument count ≠ parameter count
- **Example**: `Vec<i32, i32>` when Vec takes one parameter

**E-TYP-0104: Type well-formedness violation**
- **Severity**: Error
- **Reference**: Part III, §11
- **Description**: A type expression is not well-formed according to the type system rules.
- **Triggering Condition**: Type fails well-formedness judgment
- **Example**: Function type with invalid parameter types, infinite-sized type

**E-TYP-0105: Type recursion without indirection**
- **Severity**: Error
- **Reference**: Part III, §13 (Composite Types)
- **Description**: A recursive type definition lacks necessary indirection (pointer/reference), creating infinite size.
- **Triggering Condition**: Type size computation yields ∞
- **Example**: `record Node { value: i32, next: Node }` (should use pointer for next)

#### Bucket 02: Declaration Well-Formedness (Records, Enums, Modals)

**E-TYP-0201: Duplicate field name in record**
- **Severity**: Error
- **Reference**: Part III, §13
- **Description**: A record type declares the same field name multiple times.
- **Triggering Condition**: Field name appears >1 time in record
- **Example**: `record Point { x: i32, x: i64 }`

**E-TYP-0202: Missing required record field in initializer**
- **Severity**: Error
- **Reference**: Part III, §13
- **Description**: A record initializer omits a required field.
- **Triggering Condition**: Field ∈ record fields ∧ field ∉ initializer
- **Example**: `Point { x: 10 }` when Point requires both x and y

**E-TYP-0203: Unknown field in record initializer**
- **Severity**: Error
- **Reference**: Part III, §13
- **Description**: A record initializer includes a field that doesn't exist in the record type.
- **Triggering Condition**: Field ∈ initializer ∧ field ∉ record fields
- **Example**: `Point { x: 10, y: 20, z: 30 }` when Point only has x and y

**E-TYP-0204: Duplicate variant name in enum**
- **Severity**: Error
- **Reference**: Part III, §13
- **Description**: An enum type declares the same variant name multiple times.
- **Triggering Condition**: Variant name appears >1 time
- **Example**: `enum Result { Ok, Error, Ok }`

**E-TYP-0205: Modal type missing initial state**
- **Severity**: Error
- **Reference**: Part III, §13 (Modal Types)
- **Description**: A modal type declaration doesn't specify which state is the initial state.
- **Triggering Condition**: No state marked as initial in modal definition
- **Example**: Modal type with states but no `@Initial` annotation

**E-TYP-0206: Modal type has multiple initial states**
- **Severity**: Error
- **Reference**: Part III, §13
- **Description**: A modal type declares more than one initial state.
- **Triggering Condition**: >1 state marked as initial
- **Example**: Two states both annotated with `@Initial`

**E-TYP-0207: Invalid modal state transition**
- **Severity**: Error
- **Reference**: Part III, §13
- **Description**: Code attempts a state transition that is not declared in the modal type's state machine.
- **Triggering Condition**: Transition edge not in allowed transitions
- **Example**: Transitioning File from `@Closed` to `@Reading` when only `@Closed -> @Open` is allowed

**E-TYP-0208: Empty enum type**
- **Severity**: Error
- **Reference**: Part III, §13
- **Description**: An enum type declares no variants, making it uninhabitable.
- **Triggering Condition**: Enum variant count = 0
- **Example**: `enum Empty { }`

**E-TYP-0209: Record field type is not sized**
- **Severity**: Error
- **Reference**: Part III, §13
- **Description**: A record field has a type whose size cannot be determined at compile time.
- **Triggering Condition**: Field type not in Sized behavior
- **Example**: Field of unsized slice type without indirection

**E-TYP-0210: Enum variant payload type mismatch**
- **Severity**: Error
- **Reference**: Part III, §13
- **Description**: An enum variant is constructed with a payload value of the wrong type.
- **Triggering Condition**: Payload type ≠ declared variant type
- **Example**: `Option::Some("string")` when Some expects i32

#### Bucket 03: Generic Parameters, Variance, Instantiation

**E-TYP-0301: Type parameter not in scope**
- **Severity**: Error
- **Reference**: Part III, §15
- **Description**: A type parameter is referenced but not declared in the current generic context.
- **Triggering Condition**: Type variable ∉ generic parameter list
- **Example**: Using `T` in type expression when T not declared

**E-TYP-0302: Generic parameter bound not satisfied**
- **Severity**: Error
- **Reference**: Part III, §15; Part VII, §17 (Behaviors)
- **Description**: A type argument doesn't satisfy the behavior bounds required by the generic parameter.
- **Triggering Condition**: Type argument ∉ required behavior
- **Example**: `sort<T>(items: [T])` where T: Comparable, called with non-comparable type

**E-TYP-0303: Variance annotation conflict**
- **Severity**: Error
- **Reference**: Part III, §15
- **Description**: A type parameter's declared variance is inconsistent with how it's used in the type definition.
- **Triggering Condition**: Declared variance ≠ inferred variance from usage
- **Example**: Parameter declared covariant but used in contravariant position

**E-TYP-0304: Higher-kinded type misuse**
- **Severity**: Error
- **Reference**: Part III, §15
- **Description**: A type constructor is used in a context expecting a different kind.
- **Triggering Condition**: Kind mismatch in type expression
- **Example**: Using `Vec` (kind `* -> *`) where `*` expected

**E-TYP-0305: Generic recursion limit exceeded**
- **Severity**: Error
- **Reference**: Part III, §15
- **Description**: Generic instantiation recursion exceeded implementation limits, possibly due to infinite template expansion.
- **Triggering Condition**: Instantiation depth > limit
- **Example**: Recursive generic type instantiation that doesn't terminate

**E-TYP-0306: Associated type constraint not satisfied**
- **Severity**: Error
- **Reference**: Part III, §15; Part VII, §17
- **Description**: A generic parameter's associated type doesn't meet required constraints.
- **Triggering Condition**: Associated type fails constraint
- **Example**: `where T: Iterator, T::Item: Display` when Item doesn't implement Display

**E-TYP-0307: Ambiguous type parameter inference**
- **Severity**: Error
- **Reference**: Part III, §15
- **Description**: Type inference cannot uniquely determine type parameters.
- **Triggering Condition**: Multiple valid type parameter solutions
- **Example**: Calling generic function without sufficient context to infer T

**E-TYP-0308: Type parameter escapes its scope**
- **Severity**: Error
- **Reference**: Part III, §15
- **Description**: A type parameter appears in the return type or signature in a way that would leak beyond its declaration scope.
- **Triggering Condition**: Type variable in type escapes quantifier scope
- **Example**: Returning value containing local generic type parameter

#### Bucket 04: Permission Annotations and Defaults

**E-TYP-0401: Invalid permission annotation**
- **Severity**: Error
- **Reference**: Part IV, §17 (Lexical Permission System)
- **Description**: A type uses an undefined or malformed permission annotation.
- **Triggering Condition**: Permission ∉ valid permission set
- **Example**: `@invalid_permission T`

**E-TYP-0402: Permission conflict in type definition**
- **Severity**: Error
- **Reference**: Part IV, §17
- **Description**: A type definition has conflicting permission requirements.
- **Triggering Condition**: Incompatible permissions in type
- **Example**: Type requiring both `@unique` and `@shared` simultaneously

**E-TYP-0403: Permission downgrade not permitted**
- **Severity**: Error
- **Reference**: Part IV, §17
- **Description**: Code attempts to assign a value with stronger permissions to a location with weaker permissions without explicit conversion.
- **Triggering Condition**: Permission downcast without explicit coercion
- **Example**: Assigning `@unique` value to `@shared` binding without using explicit permission cast

**E-TYP-0404: Const type modified**
- **Severity**: Error
- **Reference**: Part IV, §17
- **Description**: Code attempts to modify a value with const permission.
- **Triggering Condition**: Write operation on const-permission value
- **Example**: Mutating field of `@const` reference

#### Bucket 05: Contract/Witness Attachment to Types

**E-TYP-0501: Contract annotation on non-procedure type**
- **Severity**: Error
- **Reference**: Part VI, §14 (Contract Language)
- **Description**: A contract annotation (requires/ensures) appears on a type that doesn't support contracts.
- **Triggering Condition**: Contract on non-callable type
- **Example**: `record Point @requires(x > 0) { x: i32 }` (contracts on records invalid)

**E-TYP-0502: Invalid contract expression**
- **Severity**: Error
- **Reference**: Part VI, §14
- **Description**: A contract clause contains an expression that violates contract language rules.
- **Triggering Condition**: Contract expression not well-formed
- **Example**: Contract referencing variables not in scope, non-boolean contract predicate

**E-TYP-0503: Witness requirement not satisfied**
- **Severity**: Error
- **Reference**: Part VII, §18 (Witness System)
- **Description**: A type or function requires a witness (proof of behavior satisfaction) that is not provided.
- **Triggering Condition**: Required witness not in context
- **Example**: Calling function requiring `Ord` witness without providing one

**E-TYP-0504: Contract parameter reference invalid**
- **Severity**: Error
- **Reference**: Part VI, §14
- **Description**: A contract clause references a parameter in an invalid way (e.g., using output parameter in precondition).
- **Triggering Condition**: Contract references inappropriate parameters
- **Example**: `ensures` clause referencing parameter value before call

**E-TYP-0505: Recursive contract without termination measure**
- **Severity**: Error
- **Reference**: Part VI, §14
- **Description**: A recursive procedure has contracts but no decreasing measure annotation.
- **Triggering Condition**: Recursive function without @decreases
- **Example**: Recursive function without termination proof

### B.9.3 MEM Category - Memory Model, Permissions, Regions

#### Bucket 01: Responsibility/Move Semantics

**E-MEM-0101: Move from non-responsible binding**
- **Severity**: Error
- **Reference**: Part IV, §19 (Moves, Assignments, and Destruction)
- **Description**: Code attempts to move a value from a binding that doesn't have responsibility for cleanup.
- **Triggering Condition**: Move from `<-` binding
- **Example**: `let x <- value; let y = x;` (cannot move from non-responsible binding)

**E-MEM-0102: Use after move**
- **Severity**: Error
- **Reference**: Part IV, §19
- **Description**: Code attempts to use a value after it has been moved.
- **Triggering Condition**: Variable use after move operation
- **Example**: `let x = vec; let y = x; print(x);` (x moved to y)

**E-MEM-0103: Partial move from aggregate**
- **Severity**: Error
- **Reference**: Part IV, §19
- **Description**: Code moves a field out of a struct without moving the entire struct.
- **Triggering Condition**: Field move from non-Copy type
- **Example**: `let p = Point{x: vec1, y: vec2}; let a = p.x;` (partial move)

**E-MEM-0104: Move of borrowed value**
- **Severity**: Error
- **Reference**: Part IV, §17; Part IV, §19
- **Description**: Code attempts to move a value that is currently borrowed.
- **Triggering Condition**: Move while active borrow exists
- **Example**: `let r = &x; let y = x;` (cannot move x while borrowed)

**E-MEM-0105: Double free via responsibility conflict**
- **Severity**: Error
- **Reference**: Part IV, §16 (Ownership & Responsibility)
- **Description**: Multiple bindings would attempt to free the same resource.
- **Triggering Condition**: Resource has >1 responsible binding
- **Example**: Two `=` bindings to same resource without proper move semantics

**E-MEM-0106: Return of local reference**
- **Severity**: Error
- **Reference**: Part IV, §17
- **Description**: A procedure returns a reference to a local variable that will be destroyed.
- **Triggering Condition**: Returned reference outlives referent
- **Example**: `procedure foo() -> &i32 { let x = 5; return &x; }`

**E-MEM-0107: Move in loop without re-initialization**
- **Severity**: Error
- **Reference**: Part IV, §19
- **Description**: A variable is moved inside a loop but not re-initialized before the next iteration.
- **Triggering Condition**: Move in loop, variable used in subsequent iteration
- **Example**: `while cond { consume(x); }` (x moved in first iteration)

**E-MEM-0108: Responsibility transfer in unsafe context failed**
- **Severity**: Error
- **Reference**: Part IV, §16
- **Description**: An unsafe responsibility transfer violated safety invariants.
- **Triggering Condition**: Invalid unsafe ownership operation
- **Example**: Unsafe code creating multiple responsible bindings to same allocation

#### Bucket 02: Permission Lattice Violations at Runtime Boundaries

**E-MEM-0201: Permission lattice violation**
- **Severity**: Error
- **Reference**: Part IV, §17
- **Description**: An operation violates the permission lattice ordering.
- **Triggering Condition**: Invalid permission transition
- **Example**: Creating mutable reference from immutable reference without permission escalation

**E-MEM-0202: Conflicting permissions on same value**
- **Severity**: Error
- **Reference**: Part IV, §17
- **Description**: Multiple references to the same value have conflicting permissions.
- **Triggering Condition**: Simultaneous incompatible permissions
- **Example**: Holding both `@unique` and `@shared` references to same value

**E-MEM-0203: Permission requirement not met**
- **Severity**: Error
- **Reference**: Part IV, §17
- **Description**: An operation requires a certain permission level that the value doesn't have.
- **Triggering Condition**: Value permission < required permission
- **Example**: Attempting mutation through `@const` reference

#### Bucket 03: Region Creation/Escape Analysis

**E-MEM-0301: Region parameter mismatch**
- **Severity**: Error
- **Reference**: Part IV, §18 (Region-Based Memory & Arenas)
- **Description**: A function is called with region arguments that don't match parameter requirements.
- **Triggering Condition**: Region argument ≠ parameter region
- **Example**: Passing value from region 'a where region 'b expected

**E-MEM-0302: Value escapes region scope**
- **Severity**: Error
- **Reference**: Part IV, §18
- **Description**: A value allocated in a region outlives the region's scope.
- **Triggering Condition**: Value lifetime > region lifetime
- **Example**: Returning reference to arena-allocated value after arena is destroyed

**E-MEM-0303: Region outlives referenced data**
- **Severity**: Error
- **Reference**: Part IV, §18
- **Description**: A region contains references to data that will be destroyed before the region.
- **Triggering Condition**: Referenced data lifetime < region lifetime
- **Example**: Arena containing pointers to stack variables

**E-MEM-0304: Ambiguous region inference**
- **Severity**: Error
- **Reference**: Part IV, §18
- **Description**: Region parameters cannot be uniquely inferred from context.
- **Triggering Condition**: Multiple valid region assignments
- **Example**: Function call with insufficient region context

**E-MEM-0305: Region access after destruction**
- **Severity**: Error
- **Reference**: Part IV, §18
- **Description**: Code attempts to access a region that has been destroyed.
- **Triggering Condition**: Region use after destruction
- **Example**: Using arena allocator after arena destroy

**E-MEM-0306: Cross-region reference without annotation**
- **Severity**: Error
- **Reference**: Part IV, §18
- **Description**: A value in one region references a value in another region without proper lifetime annotation.
- **Triggering Condition**: Cross-region reference without explicit lifetime relationship
- **Example**: Arena-allocated struct pointing to value in different arena without lifetime bounds

#### Bucket 04: Pointer Provenance/Dereference State Errors

**E-MEM-0401: Dereference of null pointer**
- **Severity**: Error
- **Reference**: Part III, §14 (Pointer & Reference Model)
- **Description**: Code attempts to dereference a pointer that may be null in safe code.
- **Triggering Condition**: Dereference of optional pointer without null check
- **Example**: `let p: ?*i32 = null; let x = *p;`

**E-MEM-0402: Dereference of invalid provenance**
- **Severity**: Error
- **Reference**: Part III, §14
- **Description**: A pointer created through integer-to-pointer conversion lacks valid provenance.
- **Triggering Condition**: Dereference of pointer without memory provenance
- **Example**: `let p = 0x1000 as *i32; let x = *p;` (in safe code)

**E-MEM-0403: Pointer arithmetic out of bounds**
- **Severity**: Error
- **Reference**: Part III, §14
- **Description**: Pointer arithmetic results in a pointer outside the bounds of the allocated object.
- **Triggering Condition**: Pointer offset > allocation size
- **Example**: `let p = &array[0]; let q = p.offset(100);` when array size < 100

**E-MEM-0404: Use of dangling pointer**
- **Severity**: Error
- **Reference**: Part III, §14
- **Description**: A pointer is used after the memory it points to has been freed.
- **Triggering Condition**: Pointer use after pointee deallocation
- **Example**: Reference to stack variable after function return

**E-MEM-0405: Pointer cast safety violation**
- **Severity**: Error
- **Reference**: Part III, §14
- **Description**: A pointer cast violates safety requirements (alignment, size, mutability).
- **Triggering Condition**: Invalid pointer type cast
- **Example**: Casting `*i32` to `*i64` without alignment guarantee

**E-MEM-0406: Unaligned pointer dereference**
- **Severity**: Error
- **Reference**: Part III, §14; Part IX, §23 (Memory Layout & Alignment)
- **Description**: Dereferencing a pointer that doesn't meet the type's alignment requirement.
- **Triggering Condition**: pointer address % type alignment ≠ 0
- **Example**: Dereferencing i32 pointer at odd address

**E-MEM-0407: Pointer to unsized type without metadata**
- **Severity**: Error
- **Reference**: Part III, §14
- **Description**: A pointer to an unsized type (slice, trait object) lacks required metadata.
- **Triggering Condition**: Fat pointer missing length/vtable
- **Example**: Incomplete fat pointer construction

#### Bucket 05: Concurrency and Happens-Before Diagnostics

**E-MEM-0501: Data race detected**
- **Severity**: Error
- **Reference**: Part IX, §23 (Memory Model)
- **Description**: Static analysis detected a potential data race.
- **Triggering Condition**: Concurrent access without synchronization, at least one write
- **Example**: Two threads accessing shared variable without mutex

**E-MEM-0502: Sync requirement not satisfied**
- **Severity**: Error
- **Reference**: Part IX, §23
- **Description**: A type sent across threads doesn't satisfy Sync behavior requirement.
- **Triggering Condition**: Type not Sync used in concurrent context
- **Example**: Sending Rc across thread boundary

**E-MEM-0503: Send requirement not satisfied**
- **Severity**: Error
- **Reference**: Part IX, §23
- **Description**: A type transferred to another thread doesn't satisfy Send behavior requirement.
- **Triggering Condition**: Type not Send moved across threads
- **Example**: Transferring raw pointer between threads

**E-MEM-0504: Lock order violation detected**
- **Severity**: Error
- **Reference**: Part IX, §23
- **Description**: Lock acquisition order violates declared lock ordering, risking deadlock.
- **Triggering Condition**: Lock acquired in wrong order relative to annotations
- **Example**: Acquiring lock B then A when annotation specifies A before B

**W-MEM-0501: Potential data race**
- **Severity**: Warning
- **Reference**: Part IX, §23
- **Description**: Code pattern suggests possible data race that cannot be definitively proven.
- **Triggering Condition**: Suspicious concurrent access pattern
- **Example**: Unsynchronized atomic operations that may have ordering issues

### B.9.4 EXP Category - Expressions, Statements, Control Flow

#### Bucket 01: Expression Typing/Evaluation Order

**E-EXP-0101: Type mismatch in expression**
- **Severity**: Error
- **Reference**: Part V, §20 (Expressions)
- **Description**: An expression's type doesn't match the expected type in context.
- **Triggering Condition**: Expression type ≠ expected type, no valid coercion
- **Example**: `let x: i32 = "string";`

**E-EXP-0102: Binary operator type mismatch**
- **Severity**: Error
- **Reference**: Part V, §20
- **Description**: Binary operator applied to incompatible types.
- **Triggering Condition**: Left type × right type not in operator domain
- **Example**: `5 + "hello"`

**E-EXP-0103: Unary operator type mismatch**
- **Severity**: Error
- **Reference**: Part V, §20
- **Description**: Unary operator applied to incompatible type.
- **Triggering Condition**: Operand type not in operator domain
- **Example**: `-"hello"`, `!5` (when ! is logical not)

**E-EXP-0104: Function argument type mismatch**
- **Severity**: Error
- **Reference**: Part V, §20
- **Description**: Function called with argument of wrong type.
- **Triggering Condition**: Argument type ≠ parameter type
- **Example**: `takes_int("string")` when takes_int expects i32

**E-EXP-0105: Function argument count mismatch**
- **Severity**: Error
- **Reference**: Part V, §20
- **Description**: Function called with wrong number of arguments.
- **Triggering Condition**: Argument count ≠ parameter count
- **Example**: `add(1, 2, 3)` when add takes 2 parameters

**W-EXP-0101: Unused expression result**
- **Severity**: Warning
- **Reference**: Part V, §20
- **Description**: An expression producing a value is evaluated but the result is discarded.
- **Triggering Condition**: Non-unit expression in statement position
- **Example**: `compute_value();` when function returns important value

**W-EXP-0102: Comparison with always-same result**
- **Severity**: Warning
- **Reference**: Part V, §20
- **Description**: A comparison always evaluates to the same boolean value.
- **Triggering Condition**: Comparison provably constant
- **Example**: `if 5 > 3 { }` (always true), `x < x` (always false)

#### Bucket 02: Statement/Control-Flow Violations

**E-EXP-0201: Missing return value**
- **Severity**: Error
- **Reference**: Part V, §21 (Statements)
- **Description**: A non-unit procedure doesn't return a value on all code paths.
- **Triggering Condition**: ∃ path without return in non-unit procedure
- **Example**: `procedure foo() -> i32 { if cond { return 5; } }` (missing else branch return)

**E-EXP-0202: Return value in unit procedure**
- **Severity**: Error
- **Reference**: Part V, §21
- **Description**: A procedure declared as returning unit (no return type) attempts to return a value.
- **Triggering Condition**: Return with value in unit procedure
- **Example**: `procedure foo() { return 5; }`

**E-EXP-0203: Unreachable code detected**
- **Severity**: Error
- **Reference**: Part V, §21
- **Description**: Code appears after a definitely-diverging expression (return, panic, infinite loop).
- **Triggering Condition**: Statement after diverging expression
- **Example**: `return 5; let x = 10;` (x declaration unreachable)

**E-EXP-0204: Break outside loop**
- **Severity**: Error
- **Reference**: Part V, §22 (Control Flow and Pattern Matching)
- **Description**: A `break` statement appears outside of a loop context.
- **Triggering Condition**: Break not within loop scope
- **Example**: `if cond { break; }`

**E-EXP-0205: Continue outside loop**
- **Severity**: Error
- **Reference**: Part V, §22
- **Description**: A `continue` statement appears outside of a loop context.
- **Triggering Condition**: Continue not within loop scope
- **Example**: `procedure foo() { continue; }`

**E-EXP-0206: Match not exhaustive**
- **Severity**: Error
- **Reference**: Part V, §22
- **Description**: A match expression doesn't cover all possible values of the scrutinee.
- **Triggering Condition**: ∃ value not matched by any pattern
- **Example**: `match x { 1 => ..., 2 => ... }` when x: i32 (missing other values)

**E-EXP-0207: Unreachable match arm**
- **Severity**: Error
- **Reference**: Part V, §22
- **Description**: A match arm is unreachable because previous patterns already cover its cases.
- **Triggering Condition**: Pattern subsumed by earlier patterns
- **Example**: `match x { _ => ..., 1 => ... }` (second arm unreachable)

**E-EXP-0208: Pattern type mismatch**
- **Severity**: Error
- **Reference**: Part V, §22
- **Description**: A pattern's type doesn't match the scrutinee type.
- **Triggering Condition**: Pattern type ≠ scrutinee type
- **Example**: `match int_value { "string" => ... }`

**E-EXP-0209: Invalid pattern destructuring**
- **Severity**: Error
- **Reference**: Part V, §22
- **Description**: A pattern attempts invalid destructuring (wrong number of fields, wrong variant).
- **Triggering Condition**: Pattern structure incompatible with type
- **Example**: `let (x, y, z) = point;` when Point only has x and y fields

**E-EXP-0210: Infinite loop without divergence annotation**
- **Severity**: Error
- **Reference**: Part V, §22
- **Description**: A provably infinite loop lacks the required divergence annotation.
- **Triggering Condition**: Infinite loop without ! return type
- **Example**: `loop { }` in function returning non-!

**W-EXP-0201: Loop may not terminate**
- **Severity**: Warning
- **Reference**: Part V, §22
- **Description**: Loop termination cannot be proven but may be infinite.
- **Triggering Condition**: Unable to prove termination
- **Example**: Complex while condition that might always be true

#### Bucket 03: Pipeline/Operator Misuse

**E-EXP-0301: Pipeline type mismatch**
- **Severity**: Error
- **Reference**: Part V, §20
- **Description**: Pipeline operator chains incompatible types.
- **Triggering Condition**: Pipeline output type ≠ next stage input type
- **Example**: `5 |> string_function` when string_function expects &str

**E-EXP-0302: Division by zero**
- **Severity**: Error
- **Reference**: Part V, §20
- **Description**: Division or modulo operation by a constant zero value.
- **Triggering Condition**: Divisor provably = 0
- **Example**: `let x = 5 / 0;`

**E-EXP-0303: Bitshift overflow**
- **Severity**: Error
- **Reference**: Part V, §20
- **Description**: Bitshift amount exceeds bit width of type.
- **Triggering Condition**: Shift amount ≥ type bit width
- **Example**: `let x: i32 = 1 << 32;` (i32 is 32 bits)

**E-EXP-0304: Integer overflow in constant expression**
- **Severity**: Error
- **Reference**: Part V, §20
- **Description**: Constant evaluation produced integer overflow.
- **Triggering Condition**: Compile-time arithmetic overflow
- **Example**: `const X: i8 = 127 + 1;`

**E-EXP-0305: Operator not defined for type**
- **Severity**: Error
- **Reference**: Part V, §20
- **Description**: An operator is used with a type that doesn't support it.
- **Triggering Condition**: Type doesn't implement required operator behavior
- **Example**: `"hello" * 5` when string doesn't support multiplication

**W-EXP-0301: Potential integer overflow**
- **Severity**: Warning
- **Reference**: Part V, §20
- **Description**: Runtime arithmetic operation may overflow.
- **Triggering Condition**: Operation may exceed type bounds
- **Example**: `let x: i8 = y + z;` when y and z could sum > 127

#### Bucket 04: Comptime Execution Failures Inside Expressions

**E-EXP-0401: Comptime evaluation failed**
- **Severity**: Error
- **Reference**: Part VIII, §20
- **Description**: A comptime expression failed to evaluate during compilation.
- **Triggering Condition**: Comptime panic or error
- **Example**: `comptime { panic("failed"); }`

**E-EXP-0402: Comptime expression is not constant**
- **Severity**: Error
- **Reference**: Part VIII, §20
- **Description**: An expression in a const context is not a compile-time constant.
- **Triggering Condition**: Expression requires runtime information
- **Example**: `const X = read_file();` when read_file is runtime-only

**E-EXP-0403: Comptime evaluation timeout**
- **Severity**: Error
- **Reference**: Part VIII, §20
- **Description**: Compile-time evaluation exceeded time limits.
- **Triggering Condition**: Comptime execution time > threshold
- **Example**: Infinite loop or extremely expensive computation in comptime

#### Bucket 05: Code-Generation/Quote Misuse

**E-EXP-0501: Invalid code generation context**
- **Severity**: Error
- **Reference**: Part VIII, §22
- **Description**: Code generation API used in invalid context.
- **Triggering Condition**: Codegen outside permitted context
- **Example**: Using code generation APIs in runtime code

**E-EXP-0502: Generated code is ill-formed**
- **Severity**: Error
- **Reference**: Part VIII, §22
- **Description**: Code generation produced syntactically or semantically invalid code.
- **Triggering Condition**: Generated AST fails validation
- **Example**: Macro generating syntactically invalid expression

**E-EXP-0503: Quote hygiene violation**
- **Severity**: Error
- **Reference**: Part VIII, §22
- **Description**: Generated code violates lexical scoping hygiene rules.
- **Triggering Condition**: Generated identifier capture
- **Example**: Generated code shadowing user variables unintentionally

**E-EXP-0504: Splice type mismatch**
- **Severity**: Error
- **Reference**: Part VIII, §22
- **Description**: A splice operation in code generation has mismatched types.
- **Triggering Condition**: Splice value type ≠ target context type
- **Example**: Splicing string expression where integer expected

### B.9.5 GRN Category - Grants, Contracts, Effects

#### Bucket 01: Grant Accumulation and Leakage

**E-GRN-0101: Insufficient grants for operation**
- **Severity**: Error
- **Reference**: Part VI, §15 (Grants)
- **Description**: An operation requires a grant (capability) that is not present in the current context.
- **Triggering Condition**: Required grant ∉ current grant set
- **Example**: File I/O operation without `grant:io.file`

**E-GRN-0102: Grant not declared in procedure signature**
- **Severity**: Error
- **Reference**: Part VI, §15
- **Description**: A procedure uses grants that are not declared in its signature.
- **Triggering Condition**: Used grant ∉ declared grants
- **Example**: Procedure doing network I/O without declaring `grant:io.net`

**E-GRN-0103: Grant leakage through callback**
- **Severity**: Error
- **Reference**: Part VI, §15
- **Description**: A callback or higher-order function could leak grants beyond their intended scope.
- **Triggering Condition**: Grant escape through function parameter
- **Example**: Passing capability-requiring closure to untrusted code

**E-GRN-0104: Grant downgrade not permitted**
- **Severity**: Error
- **Reference**: Part VI, §15
- **Description**: Attempted to call a function requiring fewer grants from a context with more grants without explicit downgrade.
- **Triggering Condition**: Implicit grant narrowing
- **Example**: Calling pure function from I/O context without grant isolation

#### Bucket 02: Sequent Syntax/Verification Failures

**E-GRN-0201: Precondition not satisfied**
- **Severity**: Error
- **Reference**: Part VI, §14
- **Description**: A procedure is called but its precondition cannot be proven.
- **Triggering Condition**: Cannot prove @requires clause
- **Example**: Calling `divide(x, y)` with `@requires(y != 0)` when y might be 0

**E-GRN-0202: Postcondition not established**
- **Severity**: Error
- **Reference**: Part VI, §14
- **Description**: A procedure's implementation doesn't establish its declared postcondition.
- **Triggering Condition**: Cannot prove @ensures clause from implementation
- **Example**: Function promising `@ensures(result > 0)` but returning negative values

**E-GRN-0203: Loop invariant not maintained**
- **Severity**: Error
- **Reference**: Part VI, §14
- **Description**: A loop invariant is not preserved by the loop body.
- **Triggering Condition**: Invariant true before iteration but may be false after
- **Example**: Loop invariant `@invariant(i < length)` but loop increments i without bounds check

**E-GRN-0204: Invariant violated in method**
- **Severity**: Error
- **Reference**: Part VI, §14
- **Description**: A type invariant is violated by a method implementation.
- **Triggering Condition**: Type invariant may be false after method
- **Example**: Method setting field to value violating type's invariant

**E-GRN-0205: Contract expression references invalid state**
- **Severity**: Error
- **Reference**: Part VI, §14
- **Description**: A contract expression references program state that is not accessible in the contract context.
- **Triggering Condition**: Contract references out-of-scope state
- **Example**: Postcondition referencing local variables from procedure body

**E-GRN-0206: Termination measure not decreasing**
- **Severity**: Error
- **Reference**: Part VI, §14
- **Description**: A recursive function's termination measure doesn't strictly decrease.
- **Triggering Condition**: @decreases measure not provably smaller in recursive call
- **Example**: Recursive call without making progress toward base case

**E-GRN-0207: Contract mode mismatch**
- **Severity**: Error
- **Reference**: Part VI, §14
- **Description**: Contract checking mode declaration conflicts with usage.
- **Triggering Condition**: Runtime checking attempted on static-only contract
- **Example**: Dynamic check on contract marked static-only

**E-GRN-0208: Quantifier range unbounded**
- **Severity**: Error
- **Reference**: Part VI, §14
- **Description**: A contract quantifier (forall/exists) lacks necessary bounds.
- **Triggering Condition**: Unbounded quantification
- **Example**: `@requires(forall x: i32. x > 0)` without domain restriction

#### Bucket 03: Contract Liskov Compliance

**E-GRN-0301: Precondition strengthened in override**
- **Severity**: Error
- **Reference**: Part VI, §14; Part VII, §17
- **Description**: An overriding method has a stronger precondition than the overridden method, violating Liskov substitution.
- **Triggering Condition**: Override precondition not weaker than parent
- **Example**: Base method requires x > 0, override requires x > 10

**E-GRN-0302: Postcondition weakened in override**
- **Severity**: Error
- **Reference**: Part VI, §14; Part VII, §17
- **Description**: An overriding method has a weaker postcondition than the overridden method, violating Liskov substitution.
- **Triggering Condition**: Override postcondition not stronger than parent
- **Example**: Base method ensures result > 0, override only ensures result >= 0

**E-GRN-0303: Invariant weakened in subtype**
- **Severity**: Error
- **Reference**: Part VI, §14; Part VII, §17
- **Description**: A subtype has a weaker invariant than its parent type, violating substitutability.
- **Triggering Condition**: Subtype invariant not stronger than parent invariant
- **Example**: Base requires field > 0, subtype only requires field >= 0

#### Bucket 04: Witness Grant Propagation

**E-GRN-0401: Witness missing for behavior requirement**
- **Severity**: Error
- **Reference**: Part VII, §18
- **Description**: A witness (proof of behavior satisfaction) is required but not provided.
- **Triggering Condition**: Required witness ∉ context
- **Example**: Using type as Comparable without providing comparison witness

**E-GRN-0402: Witness grant set incompatible**
- **Severity**: Error
- **Reference**: Part VII, §18; Part VI, §15
- **Description**: A witness requires grants that are not available in the current context.
- **Triggering Condition**: Witness grants ⊄ current grants
- **Example**: Serialization witness requiring I/O grant in pure context

**E-GRN-0403: Witness coherence violation**
- **Severity**: Error
- **Reference**: Part VII, §18
- **Description**: Multiple witnesses for the same behavior on the same type are provided, violating coherence.
- **Triggering Condition**: >1 witness for same (type, behavior) pair
- **Example**: Two different Comparable implementations for same type in scope

**E-GRN-0404: Orphan witness rule violation**
- **Severity**: Error
- **Reference**: Part VII, §18
- **Description**: A witness is defined for a type/behavior pair where neither is local to the current crate.
- **Triggering Condition**: Witness for (foreign type, foreign behavior)
- **Example**: Implementing external behavior for external type

### B.9.6 FFI Category - Foreign Function Interface

#### Bucket 01: Signature/ABI Conformance

**E-FFI-0101: FFI type not representable**
- **Severity**: Error
- **Reference**: Part IX, §24 (Interoperability & FFI)
- **Description**: A type used in FFI signature cannot be represented in the target foreign ABI.
- **Triggering Condition**: Type has no C/foreign representation
- **Example**: Using modal types or behaviors directly in extern "C" function

**E-FFI-0102: Unsized type in FFI signature**
- **Severity**: Error
- **Reference**: Part IX, §24
- **Description**: An unsized type appears in FFI signature where foreign ABI requires size.
- **Triggering Condition**: Unsized type in FFI parameter/return
- **Example**: `extern "C" procedure foo(x: [i32]);` (slice unsized)

**E-FFI-0103: Invalid FFI calling convention**
- **Severity**: Error
- **Reference**: Part IX, §24
- **Description**: The specified calling convention is not supported or invalid for the target platform.
- **Triggering Condition**: Calling convention unsupported on target
- **Example**: `extern "vectorcall"` on platform without vectorcall support

**E-FFI-0104: Variadic FFI function parameter**
- **Severity**: Error
- **Reference**: Part IX, §24
- **Description**: Cursive-defined variadic functions are not permitted in FFI contexts.
- **Triggering Condition**: Variadic function definition in extern block
- **Example**: Defining `extern "C" procedure printf(fmt: *const u8, ...);` as Cursive implementation

**E-FFI-0105: FFI type alignment mismatch**
- **Severity**: Error
- **Reference**: Part IX, §24; Part IX, §23
- **Description**: Type alignment in Cursive doesn't match foreign ABI requirements.
- **Triggering Condition**: Cursive alignment ≠ ABI alignment
- **Example**: Struct with different padding than C counterpart

**E-FFI-0106: FFI type size mismatch**
- **Severity**: Error
- **Reference**: Part IX, §24
- **Description**: Type size in Cursive doesn't match foreign ABI requirements.
- **Triggering Condition**: Cursive size ≠ ABI size
- **Example**: Enum with different representation than C enum

#### Bucket 02: Calling Convention and Linkage Attributes

**E-FFI-0201: Conflicting FFI attributes**
- **Severity**: Error
- **Reference**: Part IX, §24
- **Description**: Multiple conflicting FFI attributes are applied to the same item.
- **Triggering Condition**: Incompatible FFI attributes on same declaration
- **Example**: Both `[[extern(C)]]` and `[[extern(stdcall)]]` on same function

**E-FFI-0202: Invalid linkage attribute**
- **Severity**: Error
- **Reference**: Part IX, §25 (Linkage, ODR, and Binary Compatibility)
- **Description**: A linkage attribute is invalid or unsupported.
- **Triggering Condition**: Malformed or unsupported linkage specifier
- **Example**: `[[link_name = ""]]` (empty name), unsupported linkage kind

**E-FFI-0203: FFI attribute on non-FFI item**
- **Severity**: Error
- **Reference**: Part IX, §24
- **Description**: An FFI-specific attribute is applied to an item that is not part of FFI interface.
- **Triggering Condition**: FFI attribute on pure Cursive item
- **Example**: `[[extern(C)]]` on internal helper function not exported

**E-FFI-0204: Missing required FFI attribute**
- **Severity**: Error
- **Reference**: Part IX, §24
- **Description**: An FFI declaration is missing a required attribute for correct interoperation.
- **Triggering Condition**: FFI item missing mandatory attribute
- **Example**: External function without calling convention specification

#### Bucket 03: Panic/Unwind Boundaries and Null-Termination Rules

**E-FFI-0301: Panic across FFI boundary**
- **Severity**: Error
- **Reference**: Part IX, §24; Part V, §23 (Error Handling and Panics)
- **Description**: A panic could propagate across FFI boundary, which is undefined behavior.
- **Triggering Condition**: Potentially panicking code in extern "C" function
- **Example**: `extern "C" procedure foo() { panic("error"); }`

**E-FFI-0302: Unwind across FFI boundary**
- **Severity**: Error
- **Reference**: Part IX, §24
- **Description**: Exception/unwind propagation across FFI boundary is not supported.
- **Triggering Condition**: Unwinding code path in extern function
- **Example**: Exception thrown in extern "C" function without catch

**E-FFI-0303: Missing null terminator in C string**
- **Severity**: Error
- **Reference**: Part IX, §24
- **Description**: A string passed to C code is not null-terminated as required by C conventions.
- **Triggering Condition**: Non-null-terminated string in C string context
- **Example**: Passing Cursive string slice to C function expecting null-terminated char*

**E-FFI-0304: Null pointer passed for non-nullable parameter**
- **Severity**: Error
- **Reference**: Part IX, §24
- **Description**: A null pointer is passed to FFI function for a parameter that cannot be null.
- **Triggering Condition**: Null passed to non-nullable FFI parameter
- **Example**: Passing null to C function with non-null annotation

**E-FFI-0305: FFI callback panic safety**
- **Severity**: Error
- **Reference**: Part IX, §24
- **Description**: A callback provided to foreign code could panic, violating FFI safety.
- **Triggering Condition**: Potentially panicking callback
- **Example**: Callback closure containing panic path without catch_unwind

#### Bucket 04: Unsafe Pointer Obligations Crossing FFI

**E-FFI-0401: FFI pointer provenance violation**
- **Severity**: Error
- **Reference**: Part IX, §24; Part III, §14
- **Description**: A pointer passed to or from FFI lacks valid provenance.
- **Triggering Condition**: Invalid provenance in FFI call
- **Example**: Creating pointer from arbitrary integer and passing to C

**E-FFI-0402: FFI pointer lifetime violation**
- **Severity**: Error
- **Reference**: Part IX, §24
- **Description**: A pointer passed to FFI could outlive the data it points to.
- **Triggering Condition**: Pointer lifetime > pointee lifetime in FFI call
- **Example**: Passing pointer to stack variable to C function that stores it globally

**E-FFI-0403: FFI pointer mutability violation**
- **Severity**: Error
- **Reference**: Part IX, §24
- **Description**: A const pointer from Cursive is cast to mutable in FFI or vice versa incorrectly.
- **Triggering Condition**: Incompatible mutability in FFI pointer cast
- **Example**: Passing `*const` to C function expecting non-const pointer

**E-FFI-0404: FFI pointer alignment guarantee missing**
- **Severity**: Error
- **Reference**: Part IX, §24
- **Description**: A pointer passed to FFI lacks required alignment guarantees.
- **Triggering Condition**: Pointer alignment < required alignment
- **Example**: Passing potentially unaligned pointer to C function with alignment requirements

### B.9.7 DIA Category - Diagnostic System Meta-Diagnostics

#### Bucket 01: Diagnostic Payload Structure/JSON Schema

**E-DIA-0101: Invalid diagnostic code format**
- **Severity**: Error
- **Reference**: Part I, §7.2 (Diagnostic Code System)
- **Description**: A diagnostic code doesn't follow the required K-CAT-FFNN format.
- **Triggering Condition**: Diagnostic code fails format validation
- **Example**: Implementation emitting code "ERR-123" instead of "E-CAT-0123"

**E-DIA-0102: Diagnostic JSON schema violation**
- **Severity**: Error
- **Reference**: Part I, §7.5 (Implementation-Defined Behavior)
- **Description**: Machine-readable diagnostic output doesn't conform to documented JSON schema.
- **Triggering Condition**: JSON diagnostic output fails schema validation
- **Example**: Missing required field in diagnostic JSON

**E-DIA-0103: Diagnostic category not defined**
- **Severity**: Error
- **Reference**: Part I, §7.2.2 (Category Codes)
- **Description**: A diagnostic uses a category code that is not defined in the specification.
- **Triggering Condition**: Category code ∉ defined categories
- **Example**: Diagnostic code "E-XYZ-0101" when XYZ is not a valid category

#### Bucket 02: Tooling Hooks (LSP Bridges, Span Encoding)

**E-DIA-0201: Invalid source span**
- **Severity**: Error
- **Reference**: Part II, §12 (Tooling hooks)
- **Description**: A diagnostic references an invalid source location span.
- **Triggering Condition**: Span outside file bounds or malformed
- **Example**: Line number > file line count, negative column number

**E-DIA-0202: LSP bridge protocol violation**
- **Severity**: Error
- **Reference**: Part II, §12
- **Description**: LSP integration violates Language Server Protocol requirements.
- **Triggering Condition**: Invalid LSP message format
- **Example**: Malformed LSP JSON-RPC message

**E-DIA-0203: Tooling hook callback failure**
- **Severity**: Error
- **Reference**: Part II, §12
- **Description**: A registered tooling hook callback failed during execution.
- **Triggering Condition**: Hook callback panics or returns error
- **Example**: IDE integration callback throwing exception

**E-DIA-0204: Span encoding incompatible**
- **Severity**: Error
- **Reference**: Part II, §12
- **Description**: Source span encoding is incompatible with tooling requirements.
- **Triggering Condition**: Span encoding mismatch
- **Example**: Byte offsets when tool expects UTF-16 code units (LSP)

#### Bucket 03: Diagnostic Taxonomy Consistency

**E-DIA-0301: Diagnostic bucket overflow**
- **Severity**: Error
- **Reference**: Appendix B
- **Description**: A feature bucket has exceeded its allocated 00-99 sequence number range.
- **Triggering Condition**: Sequence number > 99 in bucket
- **Example**: Attempting to define E-SRC-01100 (should be in different bucket)

**E-DIA-0302: Diagnostic code conflict**
- **Severity**: Error
- **Reference**: Part I, §7.2.4 (Diagnostic Code Stability); Appendix B
- **Description**: A diagnostic code is reused for a different error, violating stability requirements.
- **Triggering Condition**: Code assigned to multiple distinct diagnostics
- **Example**: Using E-TYP-0101 for two different type errors

### B.9.8 CNF Category - Conformance, Extensions, Versioning

#### Bucket 01: Feature Enablement/Disablement Controls

**E-CNF-0101: Unknown feature flag**
- **Severity**: Error
- **Reference**: Part I, §8 (Governance); Part II, §10.1.5
- **Description**: A feature flag referenced in manifest or command line is not defined.
- **Triggering Condition**: Feature name ∉ defined features
- **Example**: `--enable-feature=nonexistent_feature`

**E-CNF-0102: Conflicting feature flags**
- **Severity**: Error
- **Reference**: Part I, §8
- **Description**: Multiple feature flags have conflicting requirements.
- **Triggering Condition**: Incompatible features both enabled
- **Example**: Enabling mutually exclusive experimental features

**E-CNF-0103: Required feature not enabled**
- **Severity**: Error
- **Reference**: Part I, §8
- **Description**: Code uses a feature that requires explicit enablement but the feature is not enabled.
- **Triggering Condition**: Feature-gated code without feature enabled
- **Example**: Using experimental syntax without `#[enable(experimental)]`

**W-CNF-0101: Experimental feature in use**
- **Severity**: Warning
- **Reference**: Part I, §8
- **Description**: Code uses an experimental feature that may change or be removed.
- **Triggering Condition**: Experimental feature enabled
- **Example**: Using feature marked as experimental in specification

**W-CNF-0102: Deprecated feature in use**
- **Severity**: Warning
- **Reference**: Part I, §8
- **Description**: Code uses a feature that has been deprecated and will be removed in future version.
- **Triggering Condition**: Deprecated feature used
- **Example**: Using language feature marked for removal in next major version

#### Bucket 02: Conformance-Mode Selection Conflicts and Conformance Dossier/UVB Attestation Errors

**E-CNF-0201: Invalid conformance mode**
- **Severity**: Error
- **Reference**: Part I, §6.2.7 ([conformance.obligations.modes]); Part III, §13.3.2 ([modules.manifest.format])
- **Description**: The selected conformance mode (for example via `[build].conformance` or an equivalent configuration option) is not one of the strict, permissive, or documented non-conforming compatibility modes recognized by the implementation.
- **Triggering Condition**: Conformance mode name ∉ implementation’s recognized mode set
- **Example**: `conformance = "invalid"` in manifest

**E-CNF-0202: Conflicting conformance settings**
- **Severity**: Error
- **Reference**: Part I, §6.2.7 ([conformance.obligations.modes]); Part III, §13.3.2 ([modules.manifest.format])
- **Description**: Multiple configuration sources select conflicting conformance modes for the same build (for example, manifest vs command-line).
- **Triggering Condition**: Manifest and command-line (or other configuration sources) specify different modes after normalization
- **Example**: Manifest says `strict`, command line says `--conformance=permissive`

**E-CNF-0203: Conformance mode unsupported for target**
- **Severity**: Error
- **Reference**: Part I, §6.2.7 ([conformance.obligations.modes])
- **Description**: The requested strict or permissive conformance mode, or a documented non-conforming compatibility mode, is not supported for the current target platform.
- **Triggering Condition**: Requested mode not available for selected target
- **Example**: Requesting strict verification on an embedded target without sufficient resources

**E-CNF-0204: Missing or invalid conformance dossier**
- **Severity**: Error
- **Reference**: Part I, §6.2.4 ([conformance.obligations.dossier]); Appendix C
- **Description**: A conformance dossier required for the selected target triple or build configuration is missing or fails the normative schema.
- **Triggering Condition**: Required dossier absent, malformed, or missing required implementation-defined behavior or limit entries
- **Example**: Strict conformance build for `x86_64-unknown-linux-gnu` without a matching dossier file

**E-CNF-0205: Missing UVB attestation for UVB site**
- **Severity**: Error
- **Reference**: Part I, §6.2.2–§6.2.4 ([conformance.obligations.programs], [conformance.obligations.dossier]); Appendix C
- **Description**: A UVB site present in the program or its linked artifacts lacks a corresponding attestation entry in the conformance dossier used for this build.
- **Triggering Condition**: UVB operation identified by tooling has no matching dossier entry
- **Example**: Unsafe FFI pointer dereference without an attestation record in the project’s dossier


**W-CNF-0201: Permissive conformance mode in production**
- **Severity**: Warning
- **Reference**: Part I, §6.2.7 ([conformance.obligations.modes]); Part III, §13.3.2 ([modules.manifest.format])
- **Description**: A permissive (yet conforming) conformance mode, or a documented non-conforming compatibility mode, is being used for a production or release build instead of the strict conformance mode.
- **Triggering Condition**: Selected mode is permissive or a compatibility mode, and the build profile is release/production
- **Example**: Release build with relaxed conformance checking enabled via `[build].conformance = "permissive"` or an equivalent compatibility mode

#### Bucket 03: Versioning/Deprecation Policy Violations

**E-CNF-0301: Incompatible language version**
- **Severity**: Error
- **Reference**: Part I, §8; Part II, §10.1.5
- **Description**: The source code or manifest specifies a language version incompatible with this compiler.
- **Triggering Condition**: MAJOR version mismatch
- **Example**: Source specifies version 2.0, compiler is 1.x

**E-CNF-0302: Future language version syntax**
- **Severity**: Error
- **Reference**: Part I, §8
- **Description**: Code uses syntax from a future language version not supported by this compiler.
- **Triggering Condition**: Syntax from MINOR version > compiler version
- **Example**: Using new v1.5 syntax with v1.4 compiler

**E-CNF-0303: Edition mismatch**
- **Severity**: Error
- **Reference**: Part I, §8
- **Description**: Dependencies use incompatible language editions.
- **Triggering Condition**: Incompatible editions in dependency graph
- **Example**: Edition 2025 code importing Edition 2030 with breaking changes

**W-CNF-0301: Deprecated syntax form**
- **Severity**: Warning
- **Reference**: Part I, §8
- **Description**: Code uses a syntax form that is deprecated in favor of a newer alternative.
- **Triggering Condition**: Deprecated syntax detected
- **Example**: Using old-style function syntax when new syntax is preferred
