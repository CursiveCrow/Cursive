# Bootstrap Compiler Remediation Plan

## Scope and Goals
- Bring the bootstrap compiler in `cursive-bootstrap` into conformance with `cursive-bootstrap/docs/Cursive0.md`.
- Eliminate the 472 currently failing tests by addressing root causes (parser, conformance, type system, diagnostics, codegen, runtime, and harness issues).
- Preserve the Cursive0 subset semantics unless explicitly approved to extend beyond C0.

## Guiding Principles
- **Spec-first**: every fix must trace to a specific rule or diagnostic in `Cursive0.md`.
- **Minimal semantics**: implement only what C0 requires unless a C0X feature is explicitly adopted.
- **Diagnostic fidelity**: correct codes *and* spans.
- **Small, verifiable steps**: each change should be paired with a focused test subset.

## Phase 0 — Decisions and Baseline (Day 0–1)
**Outcome:** locked scope and reproducible baseline metrics.

1. **Scope decision (C0 vs C0X):**
   - Confirm whether tests using C0X constructs (closures, `class for`, `module`, `parallel` without domain, key `#` blocks, `dispatch` pattern forms, `static` keyword, default args) should be **implemented** or **trimmed/adjusted** to C0.
   - Document the decision in `cursive-bootstrap/docs/` so changes are traceable.

2. **Baseline capture:**
   - Keep a baseline summary of failing categories and test lists (current classification is sufficient).
   - Define a “must-pass” subset after each phase (parser ? resolver ? types ? codegen).

## Phase 1 — Parser & Lexer Compliance (Highest priority)
**Primary target:** eliminate E-SRC-0520/0510 parse failures and stabilize AST shape.

### 1A. Grouping Newlines in Lists
- **Root cause:** list parsers do not skip `Newline` tokens inside groupings (§3.3.4).
- **Fix:** update list-tail parsers to treat newline as whitespace.
- **Files:** `src/syntax/parser_items.cpp`, `parser_expr.cpp`, `parser_patterns.cpp`.
- **Tests impacted:** `parser_newlines_in_list` (36 tests).

### 1B. Array Literal Parsing
- **Root cause:** array literals use `ParseExpr` instead of element lists; commas cause E-SRC-0520.
- **Fix:** implement element list parsing with correct comma handling.
- **Files:** `src/syntax/parser_expr.cpp`.
- **Tests impacted:** `parser_array_literal_list` (14 tests).

### 1C. Generic `<` Ambiguity
- **Root cause:** `<` is parsed as operator, not generic-arg opener.
- **Fix:** implement lookahead or tentative parse for generic args before treating `<` as binary op.
- **Files:** `parser_expr.cpp`, `parser_types.cpp`.
- **Tests impacted:** `parser_generic_ambiguity_lt` (19 tests).

### 1D. Trailing Comma Rules
- **Root cause:** parser emits “unsupported construct” instead of `TrailingCommaAllowed` rule (§3.3.5).
- **Fix:** implement `TrailingCommaAllowed` in list tails and emit E-SRC-0521 when disallowed.
- **Files:** list tail parsers across `parser_items.cpp`, `parser_expr.cpp`, `parser_patterns.cpp`.
- **Tests impacted:** `trailing_comma_unsupported_in_parser` (1 test).

### 1E. Expression Starts
- **Root cause:** `@` is not in `IsExprStart`, so contract intrinsics fail.
- **Fix:** include `@` in expression start classification and align token kind (`Operator` vs `Punctuator`).
- **Files:** `parser_expr.cpp`, `lexer.cpp`.
- **Tests impacted:** `contract_intrinsic_expr_start_missing` (1 test).

### 1F. Grammar Incompatibilities
Handle each explicitly (either implement or produce correct diagnostic):
- **`procedure (..) -> ..` function type** (C0 uses `(…) -> …`).
- **`static` keyword** (C0 uses top-level `let/var`).
- **`module` keyword** (not in C0 grammar).
- **`class for` syntax** (C0 uses `<:` for bounds).
- **`parallel on` syntax** (C0 uses `parallel <domain_expr>`).
- **`dispatch` pattern syntax** (C0 requires `dispatch pattern in range_expr`).
- **default argument values** (not in C0 grammar).
- **class item attributes / associated types / `@State` in class** (confirm C0 support; implement or reject with correct diag).

## Phase 2 — Conformance Gate & Reserved Lexemes
**Primary target:** stop false E-UNS for constructs that are in C0.

1. **Conformance blacklist alignment**
   - Remove `extern` and `import` from unsupported tokens (C0 grammar includes them).
   - Recheck any other blocked lexemes against `Cursive0.md`.
   - **File:** `src/analysis/types/conformance.cpp`.
   - **Tests:** `unsupported_token_extern`, `unsupported_token_import`, `unsupported_token_async`.

2. **Keywords vs identifiers**
   - Confirm whether keywords are allowed in member names (e.g., `string::from`).
   - If spec allows in qualified names, update parser/resolver accordingly; else align tests.
   - **Files:** `parser_paths.cpp`, resolver.

## Phase 3 — Name Resolution & Prelude
**Primary target:** eliminate unresolved identifiers due to missing universe bindings.

- Add required builtins and prelude entries (panic/assert/unreachable, `Point::default`, `string`/`bytes` helpers, `$HeapAllocator` methods).
- Ensure scope/visibility rules for modal fields are correct (private/protected scope).
- **Files:** `src/analysis/resolve/scopes.cpp`, `resolve_*`, runtime bindings.
- **Tests:** `unresolved_name_missing_prelude`, `missing_capability_methods`, `bytes_string_method_mismatch`.

## Phase 4 — Type System & Semantics
**Primary target:** correct core typing rules and eliminate systemic mismatches.

1. **Permissions and pointers**
   - Default permission = `const` (C0 §5.2.12).
   - `&` yields `Ptr<unique T>@Valid`.
   - `^` yields `Ptr<T>@Valid`.

2. **Bitcopy & permissions**
   - `BitcopyType` should ignore permission qualifiers (unique is still bitcopy-capable if the base is).

3. **Call arguments**
   - Allow non-`move` args to be rvalues when permitted by spec (remove place-only restriction).

4. **Type aliases & tuple indices**
   - Expand aliases before tuple-index typing.

5. **Monomorphization depth**
   - Implement E-TYP-2308 limit and report at correct site.

6. **Range patterns**
   - Ensure range bounds are evaluated as constants, not bindings; emit E-SEM-2721/2722.

7. **Shift rhs type**
   - Enforce `u32` shift RHS or adjust default literal typing if spec dictates.

**Files:** `type_expr.cpp`, `type_stmt.cpp`, `type_pattern.cpp`, `type_equiv.cpp`, `type_infer.cpp`.

## Phase 5 — Diagnostics Fidelity
**Primary target:** correct diagnostic codes and spans.

1. **Code mapping mismatches**
   - Fix mapping table and emit the expected code per spec.
   - Examples: E-CON-0101 vs E-CON-0130, E-MEM-3003 vs E-MOD-2401, E-TYP-1821 vs E-TYP-1820, etc.

2. **Span precision**
   - Emit diagnostics on the precise offending token or node (not outer statement/block spans).

3. **Missing diagnostics**
   - Implement checks flagged in `missing_diagnostic_check` list.

**Files:** `core/diagnostic_messages_table.inc`, `analysis/types/typecheck_diag_map.inc`, `type_decls.cpp`, `type_stmt.cpp`, resolver.

## Phase 6 — Codegen and Runtime Correctness
**Primary target:** remove E-OUT-0402 and runtime crashes/miscompilations.

1. **Enum ABI & match lowering**
   - Fix enum layout and tag handling (crash in enum passing and match lowering).

2. **Unions and floats**
   - Complete IR lowering paths for unions, float ops, compound assign.

3. **Defer & cleanup**
   - Ensure defer blocks run on all exits and in correct order.

4. **Loop lowering**
   - Fix `break` values and loop exit conditions to avoid timeouts.

5. **String ops IR**
   - Fix string comparison and arithmetic lowering causing LLVM errors.

6. **Linking issues**
   - Ensure runtime symbols (e.g., pow) are available or properly linked.

**Files:** `codegen/*` (especially `lower_*`, `cleanup.cpp`, enum/union ABI files).

## Phase 7 — Test Harness & Infrastructure
**Primary target:** eliminate harness-caused failures and improve signal.

1. **Encoding handling**
   - Use UTF-8 with surrogate handling when reading tests; allow bidi controls in lexer tests.

2. **Project tests**
   - Run `expect.toml` test projects as true projects (not single-file stubs).

3. **Test filtering & output**
   - Add targeted filter modes for faster iteration (by category/diag).

**Files:** `tests/runner/run_tests.py`.

## Phase 8 — Verification and Closure
**Primary target:** confirm resolution of each root-cause category.

- Re-run targeted subsets after each phase.
- Track elimination of categories in a running summary table.
- Full-suite run when parser + type system are stable.

## Prioritization Summary (Suggested Order)
1. Parser & lexer compliance (unblocks most failures).
2. Conformance blacklist and grammar alignment decisions.
3. Prelude/resolution.
4. Type system fundamentals (permissions, pointer rules, call args).
5. Diagnostics mapping and spans.
6. Codegen/runtime stability.
7. Harness fixes.

## Risk & Dependency Notes
- Parser fixes have the highest leverage and lowest risk.
- Conformance blacklist changes may expose many new errors; stabilize parser first.
- Codegen fixes should be deferred until typing and resolution are stable.
- If C0X features are adopted, a parallel spec-delta document is required.

## Acceptance Criteria
- All parsing-related categories eliminated.
- All diagnostic code mismatches resolved.
- No runtime crashes/timeouts in run-pass/codegen suites.
- Full test suite passes (or remaining failures explicitly approved with rationale).
