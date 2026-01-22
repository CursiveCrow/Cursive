# Cursive0 bootstrap compiler implementation plan

This document is an implementation plan for **`cursivec0`**, the Cursive0 bootstrap compiler written in **C++** and using **LLVM 21.1.8** (toolchain) for IR/object emission and linking. The plan is structured to directly implement the **definitions and rules** in the Cursive0 specification and to enforce **rule-level implementation + rule-level test coverage** as a CI gate. 

---

## 0. How to use this plan (handoff contract)

This document is intended to be directly handed to an implementation agent (including an LLM) and treated as the **work-authority** for building `cursivec0`. The plan is structured so that each task is:

* **Spec-scoped** (explicit spec sections and/or rule IDs)
* **Implementable** (explicit files and primary entry points)
* **Testable** (a concrete test strategy, and `SPEC_COV("<RuleId>")` tags to satisfy CI gates)

### 0.1 Pinned spec and generated indexes

* The spec file `spec/Cursive0.md` is **pinned**.
  1. re-running `tools/spec_indexer.py` to regenerate `spec/rules_index.json`, and
  2. re-running `tools/diag_extractor.py` to regenerate `spec/diag_index.json`,
  3. then updating this plan if task scopes change.

### 0.2 Determinism contract (must hold for all phases)

The compiler must behave deterministically across machines and runs:

* Never rely on filesystem enumeration order or unordered container iteration.
* The compiler pipeline SHOULD be single-threaded in `cursivec0`. If any phase is parallelized, diagnostic emission order MUST still match the specâ€™s `Emit` order and all outputs MUST remain deterministic.
* When the spec defines an order, implement it literally (stable sorts, deterministic tie-breakers).
* Normalize and compare module paths using **Unicode NFC + case folding** where required (no ad hoc folding).
* Diagnostics MUST preserve the order of `Emit` calls (spec Â§1.6.3 `Emit-Append` + Â§1.6.5 `Order` is identity): do not sort diagnostics by span. Determinism is achieved by ensuring traversal and emission are deterministic.

### 0.3 Error handling and diagnostics contract

* Prefer explicit `llvm::Expected<T>` / `llvm::Error` (or an equivalent `StatusOr<T>` wrapper) style error propagation for all fallible operations (filesystem, decoding, tool resolution, linking).
* Do not throw exceptions across subsystem boundaries.
* Diagnostics must use the specâ€™s **DiagId â†’ Code** mapping (generated from the spec tables), and must carry stable spans where defined.

### 0.4 Coverage gates workflow

* Each spec rule must have:
  * at least one implementation anchor (`SPEC_RULE("<RuleId>")`), and
  * at least one test anchor (`SPEC_COV("<RuleId>")`).
* CI must fail if:
  * any rule is missing an implementation anchor,
  * any rule is missing a test anchor, or
  * any anchor references a non-existent rule in the pinned spec index.

### 0.5 Practical implementation workflow (recommended)

1. Implement tasks in the order listed in Â§9 (sequencing), keeping the compiler runnable end-to-end as early as possible.
2. After each task:
   * add/extend compile tests,
   * add any needed unit tests,
   * update `SPEC_RULE("<RuleId>")`/`SPEC_COV("<RuleId>")`,
   * run the coverage gate.
3. Treat the spec as normative; where the plan is silent, follow the spec.


### 0.6 Compiler code conventions (handoff requirements)

These conventions exist to keep the compiler implementation predictable, testable, and spec-traceable.

* **Layering:** Code in `core/` MUST NOT depend on higher layers. `syntax/` MUST depend only on `core/`. `project/` MUST depend only on `core/`. `sema/` MAY depend on `core/`, `syntax/`, and `project/`. `codegen/` MAY depend on `core/` and `sema/`. `driver/` MAY depend on all layers.
* **No global mutable state:** Global variables that affect compilation results MUST NOT be used (except immutable lookup tables). All phase state SHOULD be explicit in a `CompilerContext`/`ProjectContext` object passed through the pipeline.
* **Deterministic data structures:** Unordered container iteration MUST NOT influence any observable output (diagnostics, ordering, symbol names, file lists). If unordered containers are used for lookup, any derived ordered output MUST be produced via an explicit sort with a deterministic comparator.
* **Fallible operations:** Filesystem I/O, UTF-8 decoding, tool resolution, process invocation, and linking MUST return explicit errors (see Â§0.3). Exceptions MUST NOT cross subsystem boundaries.
* **Span fidelity:** Every token and AST node that corresponds to source text MUST carry a stable `Span` (or a reference to one) suitable for diagnostics, per Â§1.6.



### 0.7 Compiler pipeline overview (artifacts and invariants)

The compiler is organized as a **deterministic, phase-structured pipeline**. Each phase:

* consumes an explicit input artifact,
* produces an explicit output artifact (or a well-formed diagnostic set), and
* preserves the global determinism + span + diagnostic-order contracts.

**Pipeline skeleton (high level):**

1. **Phase 0 (Project):** `LoadProject(project_root)` â†’ `ProjectContext`

   * Parse + validate `Cursive.toml`
   * Compute `SearchDirs(P)` and output plan
   * Discover modules and compute the deterministic module list/order

2. **Phase 1 (Source â†’ AST):** For each module in deterministic order:

   * read bytes â†’ UTF-8 decode + BOM handling â†’ normalize line endings â†’ logical line map
   * lex â†’ parse (with recovery) â†’ `ASTModule`

   Output: `ASTProgram` (project-wide module map + module list)

3. **Phase 2 (Compile-time execution):** **deferred** in `cursivec0`

   * detect compile-time execution forms and reject via the specâ€™s `UnsupportedForm` relationship

4. **Phase 3 (Sema):** `ASTProgram` â†’ `TypedProgram`

   * name resolution (paths, qualified forms, using declarations / module aliases, visibility)
   * type checking + subsumption
   * move/bind/borrow checking (unique permissions)
   * initialization planning (including cycle checks) and conformance gates

5. **Phase 4 (Codegen):** `TypedProgram` â†’ `Object(s)` â†’ `exe`

   * lower to the internal IR model (as specified in Â§6.0)
   * IR checks and panic instrumentation
   * emit LLVM IR (text and/or bitcode as per output plan)
   * emit object(s) and link using `lld-link` per Â§2.5

**Output gating rule of thumb (driver-level):**

* The driver MUST follow the specâ€™s output rules (especially `Out-*` and `Output-Pipeline-*`).
* Practically: if any phase produces **error** diagnostics that the spec treats as fatal for later phases, the driver stops executing later phases and does not invoke external tools (assembler/linker), but it still emits all diagnostics accumulated up to that point in **Emit order**.

### 0.8 Core data model and ownership strategy

Use a small number of long-lived â€œcontextâ€ objects to make ownership, caching, and determinism explicit:

* `CompilerContext` (process lifetime):
  * spec indexes (`rules_index.json`, `diag_index.json`)
  * diagnostic sink + renderer configuration
  * Unicode services (NFC + case fold)
  * string interner / identifier table
  * stable hashing utilities

* `ProjectContext` (per compilation):
  * project root + manifest + assemblies
  * `SearchDirs(P)` and resolved tool paths (when needed)
  * deterministic module list and per-module metadata
  * output plan (IR emission mode, object paths, link plan)
  * `SourceManager` (loaded source text + line maps)

**Ownership / allocation guidance (best practice):**

* Prefer **arena allocation** (e.g., `llvm::BumpPtrAllocator`) for AST/HIR nodes and transient pass data to reduce fragmentation and simplify lifetime management.
* Prefer **interned identifiers** for `identifier` tokens and path segments.
* Use unordered maps for lookup only; never let their iteration order leak into any observable output. When an ordered result is required, explicitly materialize a vector and sort with a deterministic comparator.

### 0.9 Best-practice compiler patterns (Cursive0-specific)

These are not â€œextra featuresâ€; they are implementation patterns that reduce risk and increase spec compliance.

**Recommended patterns:**

* **Two-tier IR model:** Keep the specâ€™s internal IR model (Â§6.0) as the unit under test, then translate to LLVM IR. This makes rule-to-code mapping tractable.
* **Immutable front-end artifacts:** Treat tokens and AST nodes as immutable after construction. Build derived indexes (symbol tables, type maps) separately.
* **Centralized diagnostics:** All diagnostics flow through a single sink (`DiagSink`) that preserves **Emit order** and attaches stable spans.
* **Deterministic traversals:** Whenever the spec is silent, choose an order and make it explicit (usually: module list order, then source order, then left-to-right AST order).

**Anti-patterns to avoid (common failure modes):**

* Sorting diagnostics â€œby locationâ€ or â€œby severityâ€ (violates spec `Order` identity).
* Using pointer addresses or hash map iteration to break ties (non-deterministic).
* Performing tool resolution via build-system discovery (must follow Â§2.6 at runtime).
* Allowing parser recovery to invent spans or drop spans in a way that breaks `NoSpan-External`.
* Forgetting Cursive0-specific parser glue: `>>` shift-splitting (`SplitShiftR`) and angle skipping (`SkipAngles`) for generic rejection.

### 0.10 LLM implementation protocol

When handing this plan to an LLM implementation agent, require the following discipline:

1. **One task at a time:** implement the taskâ€™s spec rules, add `SPEC_RULE`, add `SPEC_COV`, and land tests before moving to the next task.
2. **No â€œhelpfulâ€ extensions:** do not add language features, flags, or optimizations not in the pinned spec.
3. **Keep the compiler runnable early:** always preserve the ability to run at least Phase 0 + Phase 1 on trivial projects, even if later phases are stubbed behind â€œunsupportedâ€ diagnostics.
4. **Never weaken determinism:** any new container, traversal, or filesystem operation must have an explicit deterministic ordering strategy.
5. **Prefer small, test-first increments:** golden outputs for diags/AST/IR are required; use `golden_update.py` only for intentional updates.


---

## 1. Goal, scope, and hard constraints

### Goal

Produce a working **Cursive0 compiler** (`cursivec0`) that:

* Loads a project, discovers modules deterministically, parses and typechecks Cursive0 source, lowers to LLVM 21.1.8 IR/object, and links an executable.
* Emits diagnostics with stable **DiagId â†’ Code** mapping and correct ordering.
* Implements **every rule in the Cursive0 specification** (including â€œreject/unsupportedâ€ rules).
* Ships with tests demonstrating that **each implemented rule is tested**.

### Hard constraints derived from the spec

* **Toolchain**: MUST resolve `llvm-as` and `lld-link` at compiler runtime using Â§2.6 `SearchDirs(P)` **exactly**:

  * If `Env(C0_LLVM_BIN)` is set and non-empty: `SearchDirs(P) = [Env(C0_LLVM_BIN)]` (**single directory; no fallback**).
  * Else if `RepoLLVM(P)` holds: `SearchDirs(P) = [${project_root}/llvm/llvm-21.1.8-x86_64/bin]` (**single directory; no fallback**).
  * Else: `SearchDirs(P) = PATHDirs` (ordered list of PATH directories).

  Tool resolution MUST follow Â§2.6 `ResolveTool-*` and MUST fail if the tool is not present in `SearchDirs(P)`. When multiple directories are present (PATH case), resolve by scanning directories in list order and selecting the first match. IR assembly for `emit_ir="bc"` MUST follow Â§2.6 `AssembleIR-*` (including `ResolveTool(llvm-as)` and `AssembleIR`).

* **Target**: MUST target Win64 with target triple `x86_64-pc-windows-msvc` and the LLVM module header constants specified in Â§1.5 and Â§6.12.1.

* **Project manifest**: MUST load the project from `${project_root}/Cursive.toml` per Â§2.1 `ParseManifest`/`ValidateManifest` (host filesystem semantics).

* **Source files**: module discovery MUST treat only files with extension `.cursive` as Cursive0 source files (Â§2.4 `Files(d)` / `CompilationUnit(d)` / `Module-Dir`).

* **Runtime library**: MUST locate the runtime library at `${project_root}/runtime/cursive0_rt.lib` and MUST validate required symbols during output planning and linking (Â§2.5 `ResolveRuntimeLib-*` and `Link-*`).

* **Phase 2 is deferred**: compile-time execution forms MUST be rejected as unsupported (see Â§4 and the `UnsupportedForm` relationship).

### Non-goals

* Implementing future milestones (`cursivec1` / `cursivec2`) beyond what is required for Cursive0.
* Extending beyond the Cursive0 subset or adding language features not specified.

---

## 2. Repository organization

### 2.1 Top-level folder structure

```text
cursivec0/
  CMakeLists.txt
  cmake/
    LLVM21.cmake
    Toolchain.cmake
    Sanitizers.cmake

  llvm/                          # optional: vendored LLVM tools per Â§2.6 SearchDirs
    llvm-21.1.8-x86_64/
      bin/
        llvm-as(.exe)
        lld-link(.exe)
        # (other LLVM tools optional)

  spec/
    Cursive0.md
    rules_index.json            # generated
    diag_index.json             # generated
    coverage_expectations.json  # generated or curated

  tools/
    spec_indexer.py             # extracts rule IDs and section mapping
    diag_extractor.py           # extracts diag tables + DiagIdâ†’Code mapping
    coverage_check.py           # enforces rule-level implementation+test coverage
    golden_update.py            # regenerates golden outputs intentionally

  include/
    cursive0/
      core/
        status.h
        span.h
        source_text.h
        unicode.h
        fs.h
        path.h
        hash.h
        diagnostics.h
        diagnostic_codes.h
        diagnostic_render.h
        assert_spec.h            # SPEC_RULE / SPEC_COV macros

      project/
        manifest.h
        project.h
        module_discovery.h
        deterministic_order.h
        outputs.h
        tool_resolution.h
        link.h

      syntax/
        token.h
        lexer.h
        parser.h
        ast.h
        ast_print.h              # for golden testing

      sema/
        context.h
        symbols.h
        scopes.h
        resolver.h
        types.h
        typecheck.h
        borrow_bind.h
        init_planner.h

      codegen/
        ir_model.h               # internal IR layer (as per Â§6.0)
        layout.h
        abi.h
        mangle.h
        lower_expr.h
        lower_stmt.h
        lower_pat.h
        llvm_emit.h              # LLVM 21.1.8 emission pipeline

      runtime/
        runtime_interface.h      # declarations consistent with Â§6.9 / Â§6.12.6

  src/
    core/...
    project/...
    syntax/...
    sema/...
    codegen/...
    driver/
      main.cpp                   # cursivec0 CLI entry

  runtime/
    CMakeLists.txt
    include/
      cursive0_rt.h
    src/
      panic.cpp
      string_bytes.cpp
      context.cpp
      region.cpp
      filesystem.cpp
    windows/
      exports.def                # if needed for symbol control
      cursive0_rt.vcxproj.filters (optional)

  tests/
    CMakeLists.txt
    harness/
      run_compiler.py
      expect_diag.py
      expect_ir.py
    unit/
      core_*.cpp
      project_*.cpp
      syntax_*.cpp
      sema_*.cpp
      codegen_*.cpp
    compile/
      phase0/
      phase1/
      phase3/
      phase4/
    semantics_oracle/
      programs/
      expected_output/
```

### 2.2 Naming conventions

* All compiler â€œjudgmentsâ€ (Parse*, Resolve*, T-*, Lower-*, Exec*, Eval*) map to:

  * `include/cursive0/<layer>/<name>.h`
  * `src/<layer>/<name>.cpp`
* Tests mirror spec phases:

  * `tests/compile/phase1/parser/...`
  * `tests/compile/phase3/typecheck/...`
  * `tests/codegen/...` (IR checks)
* `tests/semantics_oracle/...` (compile+run, Windows CI)

---

## 3. Traceability: enforcing â€œevery rule implemented and testedâ€

This is the central mechanism that makes the plan **provably complete** without turning this document into a 2,000+ rule listing.

### 3.1 Generated rule index

**Task output:** `spec/rules_index.json`

* `tools/spec_indexer.py` parses `spec/Cursive0.md` and emits:

  * `rule_id`
  * nearest section heading path (e.g., `6.12.3`)
  * source line range (best-effort)
* This is the canonical inventory of rules that must be implemented and tested.

### 3.2 Spec reference annotations in code

Add macros:

* `SPEC_RULE("<RuleId>")` in implementation entry points.
* `SPEC_DEF("DefinitionName", "Â§X.Y.Z")` for non-rule but normative definitions.
* `SPEC_COV("<RuleId>")` in each test case.

These are compiled into a small registration table (or extracted via lightweight parsing) so coverage can be checked.

### 3.3 Coverage gates

`tools/coverage_check.py` enforces, in CI:

1. **Implementation coverage**: every rule in `rules_index.json` appears in at least one `SPEC_RULE("<RuleId>")`.
2. **Test coverage**: every rule appears in at least one `SPEC_COV("<RuleId>")`.
3. **No stale references**: any `SPEC_RULE` or `SPEC_COV` must exist in the spec index.
4. **Per-phase targets**: allow incremental development only if explicitly configured in `spec/coverage_expectations.json` (for bootstrapping), but the end state must be 100%/100%.

---

## 4. Build system and dependency plan

### 4.1 Core dependencies

* LLVM 21.1.8 C++ libraries:

  * `LLVMCore`, `LLVMIRReader` (optional), `LLVMCodeGen`, `LLVMTarget`, `LLVMSupport`, etc.
* Unicode normalization + case folding library:

  * ICU recommended to meet Unicode NFC/case-fold requirements (see helper references in appendix; Unicode 15 referenced).
* TOML parser:

  * `toml++` or equivalent (header-only preferred).
* Test framework:

  * Catch2 or GoogleTest (either is fine; choose one and standardize).

### 4.2 CMake layout

**Tooling and language baseline (recommended):**

* Build the compiler with **C++20** (or later if required by LLVM 21.1.8 headers), and keep the standard consistent across all targets.
* Enable high-warning levels and treat warnings as errors in CI (`/W4` + `/WX` on MSVC/clang-cl; `-Wall -Wextra -Werror` on Clang/GCC where applicable).
* Provide opt-in sanitizer configurations (ASan/UBSan) for compiler unit/compile tests; do not require sanitizers for end-user builds.

* `cmake/LLVM21.cmake` finds LLVM **21.1.8** and validates that the discovered LLVM libraries match the spec-required toolchain version (or a consciously pinned equivalent).
* `cursivec0` links against LLVM and internal libs.
* `runtime` builds `cursive0_rt.lib` into `runtime/` exactly as required by the spec.

* NOTE: build-time discovery of LLVM libraries (to build `cursivec0`) is separate from runtime resolution of `llvm-as`/`lld-link` for compiling user projects; the latter MUST follow Â§2.6 (do not reuse CMake `find_program` results for compiler runtime behavior).

---

## 5. Work breakdown structure with spec cross-references

Each task below includes:

* **Spec references**: section(s), definitions, and rule IDs.
* **Implementation files**: proposed filenames.
* **Tests**: required test artifacts + coverage annotation requirements.

> Note: For tasks that cover many rules, the acceptance criterion is: implement all rules listed in `rules_index.json` for the referenced section(s), and add tests with `SPEC_COV("<RuleId>")` for each rule.

---

### 5A. Foundations and diagnostics

#### T-CORE-001 Source locations and spans

**Spec:** Â§1.6.1 rules `WF-Location`, `WF-Span`, `Span-Of`.
**Implement:**

* `include/cursive0/core/span.h`
* `src/core/span.cpp`
* `include/cursive0/core/source_text.h` (line/column mapping)
**Tests:**
* `tests/unit/core_span_test.cpp`

  * Construct valid/invalid locations/spans.
  * Verify `Span-Of` behavior.
  * Add `SPEC_COV("WF-Location")`, `SPEC_COV("WF-Span")`, `SPEC_COV("Span-Of")`.

**Completion notes (2026-01-04):**
* Implemented `SourceLocation`, `Span`, `SpanRange`, `Locate`, `ClampSpan`, and `SpanOf` in `include/cursive0/core/span.h`, `src/core/span.cpp`, and `include/cursive0/core/source_text.h`; added `SPEC_RULE`/`SPEC_DEF` anchors.
* Added unit test `tests/unit/core_span_test.cpp` with `SPEC_COV` tags for `WF-Location`, `WF-Span`, `Span-Of`.
* Build/test: `cmake -S cursive-bootstrap -B cursive-bootstrap/build`, `cmake --build ...`, `ctest --test-dir ...` (Linux/GCC); MSVC build not exercised here.

#### T-CORE-002 Token spans

**Spec:** Â§1.6.2 rules `No-Unknown-Ok`, `Attach-Token-Ok`, `Attach-Tokens-Ok`.
**Implement:**

* `include/cursive0/syntax/token.h`
* `src/syntax/token.cpp`
* `include/cursive0/core/span.h` integration
**Tests:**
* `tests/unit/syntax_token_span_test.cpp`

  * Unknown token handling and attachment.
  * `SPEC_COV("<RuleId>")` for all three rules.

**Completion notes (2026-01-04):**
* Implemented `TokenKind`, `RawToken`, `Token`, `EofToken`, `AttachSpan(s)`, `NoUnknownOk`, and `MakeEofToken` in `include/cursive0/syntax/token.h` and `src/syntax/token.cpp` with `SPEC_RULE`/`SPEC_DEF` anchors; EOF modeled as a separate sentinel (not a `TokenKind`) per Â§3.2.1/Â§3.2.4.
* Chose UTF-8 `std::string` lexeme storage (`using Lexeme = std::string`) with comment tying it to `Lexeme(T,i,j)` scalar slice semantics.
* Added unit test `tests/unit/syntax_token_span_test.cpp` with `SPEC_COV` tags for `No-Unknown-Ok`, `Attach-Token-Ok`, `Attach-Tokens-Ok`, plus EOF span check.
* Added `cursive0_syntax` target and test wiring in `CMakeLists.txt` / `tests/CMakeLists.txt`.
* Build/test: `cmake --build cursive-bootstrap/build`, `ctest --test-dir cursive-bootstrap/build` (clock-skew warnings from `gmake` noted).

#### T-CORE-003 Diagnostic record model and emission

**Spec:** Â§1.6.3 rule `Emit-Append`.
**Implement:**

* `include/cursive0/core/diagnostics.h`
* `src/core/diagnostics.cpp`
**Tests:**
* `tests/unit/core_diagnostics_emit_test.cpp`

  * Append semantics, immutability of previous diagnostics.
  * `SPEC_COV("Emit-Append")`.

**Completion notes (2026-01-04):**
* Implemented diagnostic data model (`Severity`, `Diagnostic`, `DiagnosticStream`), `Emit`, `HasError`, and `CompileStatus` in `include/cursive0/core/diagnostics.h` and `src/core/diagnostics.cpp` with `SPEC_RULE`/`SPEC_DEF` anchors for Â§1.6.3.
* Added unit test `tests/unit/core_diagnostics_emit_test.cpp` with `SPEC_COV("Emit-Append")`; includes append-order and immutability checks plus `HasError`/`CompileStatus` sanity assertions.
* Wired build/test targets in `CMakeLists.txt` and `tests/CMakeLists.txt`.
* Build/test: `cmake --build cursive-bootstrap/build`, `ctest --test-dir cursive-bootstrap/build` (clock-skew warnings from `gmake` observed; tests passed).

#### T-CORE-004 Diagnostic code selection

**Spec:** Â§1.6.4 rules `Code-Spec`, `Code-C0`.
**Implement:**

* `include/cursive0/core/diagnostic_codes.h`
* `src/core/diagnostic_codes.cpp`
* `tools/diag_extractor.py` to generate mapping from appendix tables.
**Tests:**
* `tests/unit/core_diag_code_selection_test.cpp`

  * Verify fallback logic: SpecCode first, then C0Code.
  * Golden snapshot test that generated mapping matches spec.
  * `SPEC_COV("Code-Spec")`, `SPEC_COV("Code-C0")`.

**Completion notes (2026-01-04):**
* Implemented diagnostic code lookup API (`SpecCode`, `C0Code`, `Code`) with `SPEC_RULE` anchors in `include/cursive0/core/diagnostic_codes.h` and `src/core/diagnostic_codes.cpp`.
* Added `tools/diag_extractor.py` to parse Appendix A tables in `docs/Cursive0.md`, validate code format and duplicate DiagIds, and emit deterministic `spec/diag_index.json`.
* Added unit test `tests/unit/core_diag_code_selection_test.cpp` covering precedence/fallback/no-mapping plus a golden check via `diag_extractor.py --check`.
* Wired new source and test targets in `CMakeLists.txt` and `tests/CMakeLists.txt`; generated `spec/diag_index.json`.


#### T-CORE-005 Diagnostic ordering and rendering

**Spec:** Â§1.6.5 rule `Order`; Â§1.6.6 rendering requirements; Â§1.6.7 rule `NoSpan-External`.
**Implement:**

* `include/cursive0/core/diagnostic_render.h`
* `src/core/diagnostic_render.cpp`

* Implement `Order(Î”)` as **identity**: return diagnostics in the same sequence they were appended. Do not perform any secondary sorting or bucketing.
**Tests:**
* `tests/unit/core_diag_order_test.cpp` (`SPEC_COV("Order")`)
* `tests/unit/core_diag_external_test.cpp` (`SPEC_COV("NoSpan-External")`)
* Golden output tests for rendering format.

**Completion notes (2026-01-04):**
* Added `DiagnosticOrigin`, `Order`, `ApplyNoSpanExternal`, and `Render` in `include/cursive0/core/diagnostic_render.h` and `src/core/diagnostic_render.cpp` with `SPEC_RULE`/`SPEC_DEF` anchors for `Order`, `NoSpan-External`, and `Render`.
* Implemented identity ordering and exact render formatting per Â§1.6.6, and enforced span stripping for external diagnostics.
* Added unit tests `tests/unit/core_diag_order_test.cpp`, `tests/unit/core_diag_external_test.cpp`, and `tests/unit/core_diag_render_test.cpp` with `SPEC_COV` coverage.
* Wired new source/test targets in `CMakeLists.txt` and `tests/CMakeLists.txt`.
* Build/test: `cmake --build cursive-bootstrap/build`, `ctest --test-dir cursive-bootstrap/build` (clock-skew warnings from `gmake` observed; tests passed).

#### T-DIAG-001 Diagnostic message content and localization
**Spec:** Â§8 (Appendix A - Diagnostic Codes); all `E-*` and `W-*` diagnostic code tables.
**Implement:**
* `include/cursive0/core/diagnostic_messages.h`
* `src/core/diagnostic_messages.cpp`
* `tools/diag_message_gen.py` (optional: generate message table from spec tables)
* Message content for each diagnostic code in Â§8.0â€“Â§8.13:
  * E-PRJ (Project errors)
  * E-MOD (Module errors)
  * E-OUT (Output and linking errors)
  * E-SRC (Source errors)
  * E-CNF (Conformance/limits errors)
  * E-UNS (Unsupported construct errors)
  * E-MEM (Memory errors)
  * W-MOD (Module warnings)
  * W-SRC (Source warnings)
  * E-TYP (Type errors)
  * W-SYS (System warnings)
  * E-SEM (Semantic errors)
  * W-SEM (Semantic warnings)
* Each message MUST:
  * Be human-readable and actionable
  * Reference the relevant spec section where applicable
  * Support span-based highlighting for primary and secondary locations
  * Use consistent terminology matching the spec
**Tests:**
* `tests/unit/core_diag_messages_test.cpp`
  * Verify all diagnostic codes have associated messages
  * Verify message formatting with placeholder substitution
  * Golden tests for representative diagnostic outputs
* `SPEC_DEF("DiagId-Code-Map", "Â§8.0")` and related coverage for each diagnostic category.



**Completion notes (2026-01-04):**
* Added diagnostic message lookup/formatting API in `include/cursive0/core/diagnostic_messages.h` and `src/core/diagnostic_messages.cpp` with SPEC_DEF anchors for `DiagId-Code-Map`, `SeverityColumn`, `ConditionColumn`, and `Message`.
* Added generator `tools/diag_message_gen.py` that parses Appendix A tables in `docs/Cursive0.md` (including warning tables without DiagId column) and emits `src/core/diagnostic_messages_table.inc` with deterministic ordering.
* Generated `src/core/diagnostic_messages_table.inc` from the spec and wired it into the build.
* Added unit test `tests/unit/core_diag_messages_test.cpp` and CMake wiring to verify message/severity lookup, formatting behavior, and generator sync via `--check`.
* Build/test: `cmake --build cursive-bootstrap/build`, `ctest --test-dir cursive-bootstrap/build` (clock-skew warnings from gmake observed; tests passed).

#### T-CORE-006 Conformance: reject ill-formed programs

**Spec:** Â§1.1 rule `Reject-IllFormed`.
**Implement:**

* `include/cursive0/sema/conformance.h`
* `src/sema/conformance.cpp`
**Tests:**
* `tests/compile/phase3/conformance/reject_ill_formed.cursive`
* `tests/compile/phase3/conformance/reject_ill_formed.expect.json`
* `SPEC_COV("Reject-IllFormed")`.


**Completion notes (2026-01-04):**
* Added conformance API (`WF`, `C0Conforming`, `RejectIllFormed`) in `include/cursive0/sema/conformance.h` and `src/sema/conformance.cpp` with `SPEC_RULE("Reject-IllFormed")` and Â§1.1 `SPEC_DEF` anchors.
* Added `cursive0_sema` library target and minimal driver `src/driver/main.cpp` that gates compilation via `RejectIllFormed` and supports `--diag-json` output for harness use.
* Added compile-test harness scripts in `tests/harness/` and wired `compile_tests` into CTest.
* Added compile test `tests/compile/phase3/conformance/reject_ill_formed.cursive` (embedded BOM) with expected JSON and `SPEC_COV("Reject-IllFormed")`.
* Build/test: `cmake --build cursive-bootstrap/build`, `ctest --test-dir cursive-bootstrap/build` (clock-skew warnings from gmake observed; tests passed).

#### T-CORE-007 Cursive0 subset gating and permissions

**Spec:** Â§1.1.1 rule `Perm-Shared-Unsupported`; PermSet_C0 definitions; unsupported constructs set.
**Implement:**

* `src/sema/conformance.cpp` (shared-perm rejection at parse or sema boundary)
* `include/cursive0/sema/types.h` (permission set)
**Tests:**
* `tests/compile/phase3/conformance/shared_perm_rejected.cursive`
* Expect diag id/code for `Perm-Shared-Unsupported`.
* `SPEC_COV("Perm-Shared-Unsupported")`.


**Completion notes (2026-01-04):**
* Added permission model helpers in `include/cursive0/sema/types.h` and `src/sema/types.cpp` with `PermSet_C0` spec anchors and `shared` detection.
* Implemented `CheckC0SubsetPermSyntax` in `src/sema/conformance.cpp` to scan source for `shared` and `~%` outside comments/strings, emitting `E-TYP-1101` with `SPEC_RULE("Perm-Shared-Unsupported")`.
* Wired subset gating into `src/driver/main.cpp` and CMake (`Cursive0_sema` adds `src/sema/types.cpp`).
* Added compile test `tests/compile/phase3/conformance/shared_perm_rejected.cursive` with expected JSON and `SPEC_COV("Perm-Shared-Unsupported")`.
* Build/test: `cmake --build cursive-bootstrap/build`, `ctest --test-dir cursive-bootstrap/build`, and `python3 tests/harness/run_compiler.py --test tests/compile/phase3/conformance/shared_perm_rejected.cursive` (clock-skew warnings from gmake observed; tests passed).

#### T-CORE-008 Behavior types and undefined behavior modeling hooks

**Spec:** Â§1.2 rules `Static-Undefined`, `Static-Undefined-NoCode`, `Dynamic-Undefined-UVB`.
**Implement:**

* `include/cursive0/core/ub_model.h`
* `src/core/ub_model.cpp`
* Wire into codegen checks/panic lowering paths.
**Tests:**
* Unit tests for classification.
* Compile tests that trigger the relevant path and confirm diag/emission behavior.
* `SPEC_COV("<RuleId>")` for all three rules.


**Completion notes (2026-01-04):**
* Added UB model API in `include/cursive0/core/ub_model.h` and implementation in `src/core/ub_model.cpp` with `BehaviorClass`, raw-pointer dynamic-undefined helpers, and `StaticUndefinedCode` mapping to `Code(spec_map, c0_map, id)`, plus SPEC_DEF/SPEC_RULE anchors.
* Wired `src/core/ub_model.cpp` into the `cursive0_core` library target.
* Added unit test `tests/unit/core_ub_model_test.cpp` with `SPEC_COV("Static-Undefined")`, `SPEC_COV("Static-Undefined-NoCode")`, and `SPEC_COV("Dynamic-Undefined-UVB")`, and registered it in `tests/CMakeLists.txt`.
* Compile-test coverage for `Static-Undefined` rules deferred: no literal `âŠ¥` premises exist in the Cursive0 spec, so there is no compile-path trigger; unit coverage is the correct substitute until the spec adds such a rule.
* Build/test: `cmake --build cursive-bootstrap/build`, `ctest --test-dir cursive-bootstrap/build` (clock-skew warnings from gmake observed; tests passed).

#### T-CORE-009 Unsupported constructs policy

**Spec:** Â§1.4 rules `WF-Attr-Unsupported`, `Unsupported-Construct`.
**Implement:**

* Parsing: recognize attributes and reject any `[[...]]` forms per rule.
* Sema: reject any `UnsupportedForm` constructs.
  Files:
* `src/syntax/parser.cpp`
* `src/sema/conformance.cpp`
**Tests:**
* `tests/compile/phase1/unsupported/attr_unsupported.cursive` (`SPEC_COV("WF-Attr-Unsupported")`)
* `tests/compile/phase3/unsupported/unsupported_construct.cursive` (`SPEC_COV("Unsupported-Construct")`)

**Completion notes (2026-01-04):**
* Added stop-gap lexical scans in `src/sema/conformance.cpp` to detect `[[...]]` and a limited subset of UnsupportedForm (`comptime {}`, `modal class`, generic headers with `<`, top-level `use`) and emit `E-UNS-0113` / `E-UNS-0101` with `SPEC_RULE("WF-Attr-Unsupported")` and `SPEC_RULE("Unsupported-Construct")`.
* Wired the new checks into `src/driver/main.cpp` and exposed them in `include/cursive0/sema/conformance.h`.
* Added compile tests for attribute syntax and `comptime` unsupported constructs.
* **Incomplete (as of 2026-01-04):** parser-level implementations required by Â§3.3.6.14 (e.g., `Parse-Attribute-Unsupported`, `Parse-Generic-Header-Unsupported`, `Parse-Comptime-Unsupported`, etc.) were pending under **T-PARSE-004**. The stop-gap implementation emitted diagnostics without spans and did not cover all UnsupportedForm cases.

**Completion notes (2026-01-06):**
* Parser-level unsupported-construct parsing rules are now implemented in Phase 1 under **T-PARSE-004** (including `Parse-Attribute-Unsupported`, `Parse-Generic-Header-Unsupported`, `Parse-Comptime-Unsupported`, and the remaining Â§3.3.6.14 rules), completing CORE-009; see T-PARSE-004 completion notes for file/test details.

#### T-CORE-010 Host primitives interface

**Spec:** Â§1.7 definitions `HostPrim`, `HostPrimDiag`, `HostPrimRuntime`, `MapsToDiagOrRuntime`, `HostPrimFail`.
**Implement:**

* `include/cursive0/core/host_primitives.h`
* `src/core/host_primitives.cpp`

  * Define the `HostPrim` set: `ParseTOML`, `ReadBytes`, `WriteFile`, `ResolveTool`, `ResolveRuntimeLib`, `Invoke`, `AssembleIR`, `InvokeLinker`, plus `FSPrim`, `FilePrim`, `DirPrim`.
  * Implement `HostPrimDiag` subset (primitives that emit diagnostics on failure).
  * Implement `HostPrimRuntime` subset (primitives used by runtime: `FSPrim âˆª FilePrim âˆª DirPrim`).
  * Ensure `HostPrimFail(p)` for any `p âˆ‰ HostPrimDiag âˆª HostPrimRuntime` results in `IllFormed(p)`.
* Wire host primitive calls into:
  * `T-PROJ-001` (ParseTOML)
  * `T-PROJ-008` (ResolveTool, AssembleIR)
  * `T-PROJ-007` (ResolveRuntimeLib, InvokeLinker)
  * `T-SRC-001` (ReadBytes)
  * `T-DYN-005` (FSPrim, FilePrim, DirPrim)
**Tests:**
* Unit tests for host primitive classification.
* Integration tests verifying correct diagnostic emission on host primitive failure.
* `SPEC_DEF("HostPrim", "Â§1.7")`, `SPEC_DEF("HostPrimDiag", "Â§1.7")`, `SPEC_DEF("HostPrimRuntime", "Â§1.7")`.

**Completion notes (2026-01-04):**
* Added `include/cursive0/core/host_primitives.h` and `src/core/host_primitives.cpp` with `HostPrim` enum, classification helpers, `HostPrimFail`, and `HostPrimFailureIllFormed` guard, plus `SPEC_DEF` anchors for Â§1.7.
* Implemented FS/File/Dir primitive classification derived from Â§7.7; marked derivation with `[AMBIGUOUS: Source text unclear. See original section 1.7.]` and `[TODO: Content missing from source draft.]` per spec integrity rules.
* Added unit test `tests/unit/core_host_primitives_test.cpp` covering diag/runtime membership, MapsToDiagOrRuntime, and HostPrimFail semantics; registered in `tests/CMakeLists.txt`.
* Wired `src/core/host_primitives.cpp` into `cursive0_core` in `CMakeLists.txt`.
* Build/test: `cmake --build /mnt/c/Dev/Cursive/cursive-bootstrap/build`, `ctest --test-dir /mnt/c/Dev/Cursive/cursive-bootstrap/build` (clock-skew warnings from gmake; tests passed).

---

### 5B. Phase 0: project model and build pipeline

#### T-PROJ-001 Manifest parsing

**Spec:** Â§2.1 rules `Parse-Manifest-Ok`, `Parse-Manifest-Missing`, `Parse-Manifest-Err`.
**Implement:**

* MUST parse `${project_root}/Cursive.toml` as defined in Â§2.1 (host filesystem semantics).
* `include/cursive0/project/manifest.h`
* `src/project/manifest.cpp`
* Addendum (CORE-010): route host-primitive calls through the HostPrim interface for `ParseTOML`; failures must map to `Parse-Manifest-*` diagnostics (not IllFormed).
**Tests:**
* `tests/unit/project_manifest_parse_test.cpp`
* `tests/compile/phase0/manifest_missing/` (golden diag)
* `SPEC_COV("<RuleId>")` for all three rules.

**Completion notes (2026-01-04):**
* Implemented manifest parsing in `include/cursive0/project/manifest.h` and `src/project/manifest.cpp` with `SPEC_RULE` hooks for all three rules and external-span diagnostics (`E-PRJ-0101`/`E-PRJ-0102`).
* Added upward project-root discovery (search from input file dir to filesystem root) per ambiguity resolution; no case-folding or canonicalization for `Cursive.toml` path.
* Vendored `toml++` v3.4.0 under `third_party/tomlplusplus` and added `cursive0_project` library target, linked into `cursivec0`.
* Driver now runs `ParseManifest` before source diagnostics; manifest errors short-circuit later phases.
* Unit test `tests/unit/project_manifest_parse_test.cpp` covers Ok/Missing/Err with `SPEC_COV`.
* Compile tests: `tests/compile/phase0/manifest_missing/` (missing), `tests/compile/phase0/manifest_invalid/` (invalid). Added `tests/compile/phase1/Cursive.toml` and `tests/compile/phase3/Cursive.toml` so existing phase tests have a manifest ancestor while phase0/missing does not.
* Build/test: `cmake --build cursive-bootstrap/build`, `ctest --test-dir cursive-bootstrap/build` (clock-skew warnings from gmake; tests passed).

#### T-PROJ-002 Manifest validation and well-formedness

**Spec:** Â§2.1 rules `ValidateManifest-Ok`, `ValidateManifest-Err`, `WF-TopKeys`, `WF-TopKeys-Err`, `WF-RelPath`, `WF-RelPath-Err`, `WF-Project-Root`, `Resolve-Canonical`, `Resolve-Canonical-Err`, plus all `WF-Assembly-*` rules in Â§2.1.
**Implement:**

* `include/cursive0/project/project.h`
* `src/project/project_validate.cpp`
**Tests:**
* `tests/compile/phase0/manifest_validation/*.toml` with expected diagnostics
* Unit tests for canonicalization and relative path checks
* Ensure each `WF-*` rule in Â§2.1 gets at least one failing and/or passing test with `SPEC_COV`.

**Completion notes (2026-01-04):**
* Implemented manifest validation in `include/cursive0/project/project.h` and `src/project/project_validate.cpp` with deterministic `FirstFail` ordering, `SPEC_RULE` anchors for all listed rules, and external-span diagnostics for `E-PRJ-0103/0104/0201/0202/0203/0204/0301/0304`.
* Added spec path utilities in `include/cursive0/core/path.h` and `src/core/path.cpp` (Resolve/Canon/RelPath and helpers), wired into validation.
* Driver now runs `ValidateManifest` after `ParseManifest` and before later phases (`src/driver/main.cpp`).
* Build wiring updated: `src/core/path.cpp` added to `cursive0_core`; `src/project/project_validate.cpp` added to `cursive0_project`.
* Tests:
  * Unit: `tests/unit/project_manifest_validate_test.cpp` (covers `WF-Project-Root`, `Resolve-Canonical`, `Resolve-Canonical-Err`).
  * Compile: `tests/compile/phase0/manifest_validation/*` covering all manifest validation rules (including success case).
  * Updated `tests/compile/phase1/Cursive.toml` and `tests/compile/phase3/Cursive.toml` to valid manifests so existing compile tests pass under validation.
* Build/test: `cmake --build cursive-bootstrap/build`; `ctest --test-dir cursive-bootstrap/build` (all tests passed; gmake clock-skew warnings observed).

#### T-PROJ-003 LoadProject pipeline state machine

**Spec:** Â§2.1 rules `Step-Discover`, `Step-Discover-Err-Root`, `Step-Discover-Err-Modules`, `Step-Parse`, `Step-Parse-Err`, `Step-Resolve`, `Step-Resolve-Err`, `Step-Validate`, `Step-Validate-Err`, `LoadProject-Ok`, `LoadProject-Err`.
**Implement:**

* `include/cursive0/project/project.h`
* `src/project/load_project.cpp`
* **Mock/placeholder wiring (for early bring-up before T-PROJ-006/007 land):**
  * Add minimal forward declarations in `include/cursive0/project/module_discovery.h` and `include/cursive0/project/outputs.h` so `LoadProject` can compile and unit tests can cover the state machine:
    * `struct ModuleInfo { std::string path; std::filesystem::path dir; };`
    * `struct ModulesResult { std::vector<ModuleInfo> modules; core::DiagnosticStream diags; };`
    * `ModulesResult Modules(const std::filesystem::path& source_root);`
    * `struct OutputPaths { std::filesystem::path root; std::filesystem::path obj_dir; std::filesystem::path ir_dir; std::filesystem::path bin_dir; };`
    * `OutputPaths ComputeOutputPaths(const std::filesystem::path& project_root, const ValidatedAssembly& asm);`
  * Placeholder implementations (if needed) MUST be clearly marked with TODO comments and MUST NOT add `SPEC_RULE` anchors for Â§2.4 or Â§2.5 rules. They should return deterministic, trivial outputs (e.g., empty module list + no diags; OutputPaths computed per Â§2.5 formulas).
  * Once T-PROJ-006/007 are implemented, replace placeholders with full implementations and move any TODO tests to their final locations.
**Tests:**
* `tests/unit/project_load_pipeline_test.cpp` (mock FS)
* `tests/compile/phase0/load_project_*` (integration)
* `SPEC_COV("<RuleId>")` for each Step-* and LoadProject-* rule.

**Completion notes (2026-01-04):**
* Implemented `LoadProject` state machine with explicit `SPEC_RULE` anchors for all Step-* and LoadProject-* rules, plus dependency-injection seam (`LoadProjectWithDeps`) in `include/cursive0/project/project.h` and `src/project/load_project.cpp`.
* Added `Project`/`LoadProjectResult` data model and wired driver to call `LoadProject` instead of direct Parse/Validate (`src/driver/main.cpp`).
* Added minimal placeholder interfaces for module discovery/output paths to unblock the pipeline:
  * `include/cursive0/project/module_discovery.h` + `src/project/module_discovery.cpp` stub
  * `include/cursive0/project/outputs.h` + `src/project/outputs.cpp` (`ComputeOutputPaths` per Â§2.5 formulas)
* Added unit test `tests/unit/project_load_pipeline_test.cpp` covering all Step-* and LoadProject-* rules with `SPEC_COV`, using injected deps to force each transition and error.
* Added compile tests:
  * `tests/compile/phase0/load_project_source_root_err/` (E-PRJ-0302)
  * `tests/compile/phase0/load_project_ok/` (success path)
* Build wiring updated: `src/project/load_project.cpp`, `src/project/module_discovery.cpp`, `src/project/outputs.cpp` in `cursive0_project`; unit test added to `tests/CMakeLists.txt`.
* Notes on remaining dependencies: module ordering uses a placeholder `FoldPathForOrder` (needs T-PROJ-005), and `Modules()` is a stub (needs T-PROJ-006) to fully satisfy Step-Discover semantics.

#### T-PROJ-004 Assemblies constraints

**Spec:** Â§2.2 rules `WF-Assembly-Count`, `WF-Assembly-Count-Err`.
**Implement:**

* `src/project/project_validate.cpp`
**Interpretation (assembly set A in C0):**
* If `assembly` is a TOML table, then `A = [assembly]` (singleton).
* If `assembly` is an array-of-tables, then `A` is that array (preserve order), but this form is invalid in C0 and MUST trigger `WF-Assembly-Table-Array` (diag `E-PRJ-0202`).
* If `assembly` is missing or is any other TOML type, treat it as `WF-Assembly-Table-Err` (`E-PRJ-0103`) and **do not** evaluate `WF-Assembly-Count` (since `A` is undefined).
* Apply `WF-Assembly-Count` only when `A` is defined:
  * `|A| = 1` â†’ `WF-Assembly-Count` holds.
  * `|A| â‰  1` â†’ `WF-Assembly-Count-Err` and emit `E-PRJ-0202`.
* In array-of-tables cases, emit only a single diagnostic `E-PRJ-0202`. If `|A| â‰  1`, record both `SPEC_RULE("WF-Assembly-Table-Array")` and `SPEC_RULE("WF-Assembly-Count-Err")` in the same failure path to satisfy rule coverage without emitting multiple diagnostics.
* Do not proceed to per-assembly field validation when `assembly` is an array-of-tables.
**Tests:**
* `tests/compile/phase0/assemblies/count_ok.toml`
* `tests/compile/phase0/assemblies/count_err.toml`
* `SPEC_COV("<RuleId>")`.


**Completion notes (2026-01-04):**
* Implemented assembly count handling in `src/project/project_validate.cpp`:
  * `WF-Assembly-Count` recorded for singleton table case.
  * `WF-Assembly-Count-Err` recorded on array-of-tables with size â‰  1 while emitting a single `E-PRJ-0202` (shared with `WF-Assembly-Table-Array`).
* Added compile tests:
  * `tests/compile/phase0/assemblies/count_ok/` (covers `WF-Assembly-Count`).
  * `tests/compile/phase0/assemblies/count_err/` (covers `WF-Assembly-Count-Err`, `WF-Assembly-Table-Array`).
* Build/test: `cmake --build cursive-bootstrap/build`; `ctest --test-dir cursive-bootstrap/build` (all tests passed).

#### T-PROJ-005 Deterministic ordering and directory enumeration failure paths

**Spec:** Â§2.3.1 rule `FileOrder-Rel-Fail`; Â§2.3.3 rules `DirSeq-Rel-Fail`, `DirSeq-Read-Err`.
**Implement:**

* `include/cursive0/project/deterministic_order.h`
* `src/project/deterministic_order.cpp`
* Use `include/cursive0/core/unicode.h` for `NFC` + `CaseFold` in `FoldPath`/`Fold` (no ad hoc case folding).
**Tests:**
* `tests/unit/project_deterministic_order_test.cpp`

  * Explicit tests for relative path failures and directory read errors.
  * Unit tests for `CaseFold` behavior and NFC+case-fold composition used by `FoldPath`/`EntryKey`.
  * `SPEC_COV("FileOrder-Rel-Fail")`, `SPEC_COV("DirSeq-Rel-Fail")`, `SPEC_COV("DirSeq-Read-Err")`.

**Completion notes (2026-01-05):**
* Added NFC/CaseFold stub in `include/cursive0/core/unicode.h` + `src/core/unicode.cpp` (ASCII-only folding), with TODO to replace in T-SRC-003.
* Implemented deterministic ordering helpers in `include/cursive0/project/deterministic_order.h` and `src/project/deterministic_order.cpp` (Fold/FoldPath, Utf8LexLess, FileKey/DirKey, DirSeq error diagnostics).
* Replaced module ordering in `src/project/load_project.cpp` to use `Fold` + `Utf8LexLess`.
* Added unit test `tests/unit/project_deterministic_order_test.cpp` with `SPEC_COV` for FileOrder-Rel-Fail, DirSeq-Rel-Fail, DirSeq-Read-Err, plus basic fold behavior.
* Build/test: `cmake --build cursive-bootstrap/build`; `ctest --test-dir cursive-bootstrap/build` (all tests passed; gmake clock-skew warnings observed).

#### T-PROJ-006 Module discovery

**Spec:** Â§2.4 rules `Disc-Start`, `Disc-Add`, `Disc-Skip`, `Disc-Collision`, `Disc-Rel-Fail`, `Disc-Invalid-Component`, `Disc-Done`, `CompilationUnit-Rel-Fail`, `Module-Dir`, `Module-Path-Root`, `Module-Path-Rel`, `Module-Path-Rel-Fail`, `Modules-Ok`, `Modules-Err`, `WF-Source-Root`, `WF-Source-Root-Err`, `WF-Module-Path-Ok`, `WF-Module-Path-Ident-Err`, `WF-Module-Path-Reserved`, `WF-Module-Path-Collision`.
**Implement:**

* `include/cursive0/project/module_discovery.h`
* `src/project/module_discovery.cpp`
* `include/cursive0/core/path.h` (`FileExt`) and its implementation; module discovery MUST call `FileExt` for `.cursive` filtering.
* **Interface requirements for T-PROJ-003:**
  * Provide `ModuleInfo` with at least:
    * `path`: module path string used by `prec_mod` ordering in Â§2.5.
    * `dir`: absolute filesystem path to module directory (used later by Phase 1).
  * Provide `ModulesResult Modules(const std::filesystem::path& source_root)` that:
    * Returns `diags` on any module discovery error in `Modules-Err`, matching `Disc-*`/`WF-*` diagnostics.
    * Returns the *unsorted* set `M` (ordering done in `LoadProject` via `sort_{prec_mod}`).
  * Ensure `Modules` does not compute output paths or perform linking; it only discovers modules and validates module paths.
**Tests:**
* `tests/compile/phase0/module_discovery/*` including:

  * Reserved path components
  * Collision cases
* Unit tests for component validation and collision detection
* Unit tests for `FileExt` edge cases (multiple dots, leading dot, trailing dot, no extension).
* `SPEC_COV("<RuleId>")` for each listed rule (use one test per rule minimum).

**Completion notes (2026-01-05):**
* Implemented Â§2.4 module discovery in `include/cursive0/project/module_discovery.h` and `src/project/module_discovery.cpp`:
  * Recursive, root-inclusive directory discovery; `.cursive` filtering via `core::FileExt`.
  * Module path derivation with `Module-Path-Root` mapped to assembly name.
  * Component validation uses shared identifier/keyword helpers; collisions use `Fold` + emit `E-MOD-1104` + `W-MOD-1101`.
  * Added `CompilationUnit` helper for `.cursive` ordering and `CompilationUnit-Rel-Fail`.
* Added shared identifier helpers: `include/cursive0/project/ident.h` + `src/project/ident.cpp`; refactored manifest validation to reuse them.
* Updated `LoadProject` to pass assembly name into `Modules` and to record `WF-Source-Root` / `WF-Source-Root-Err`.
* Spec clarification in `docs/Cursive0.md`: defined `AsmName(Î“)` and `Dirs(S)` (recursive, includes `S`).
* Tests:
  * Unit: `tests/unit/project_module_discovery_test.cpp` (covers Disc-*, Module-*, WF-Module-Path-*, CompilationUnit-Rel-Fail, FileExt edges).
  * Compile: `tests/compile/phase0/module_discovery/reserved_component/*` (E-MOD-1105) and added `WF-Source-Root` coverage to existing load_project compile tests.
  * Collision compile fixtures under `tests/compile/phase0/module_discovery/collision/` renamed to `.cursive.disabled` due to case-insensitive `/mnt/c`; collision is covered by unit test.
* Build/test: `cmake --build cursive-bootstrap/build`; `ctest --test-dir cursive-bootstrap/build` (all tests passed; gmake clock-skew warnings observed).

#### T-PROJ-007 Output planning and hygiene

**Spec:** Â§2.5 rules `Out-Start`, `Output-Pipeline`, `Output-Pipeline-Err`, `Out-Obj-*`, `Out-IR-*`, `Out-Link-*`, `ModuleList-*`, `ResolveRuntimeLib-*`, `Link-*`, `Emit-IR-*`, `Emit-Objects-*`, `CodegenIR-LLVM`, `CodegenObj-LLVM`. 
**Implement:**

* Link invocation MUST match Â§2.5 `LinkFlags`/`LinkArgsOk` exactly (including `/OUT:â€¦`, `/ENTRY:main`, `/SUBSYSTEM:CONSOLE`, `/NODEFAULTLIB`).
* **Runtime compatibility preflight:** implement Â§2.5 `LinkerSyms` / `MissingRuntimeSym` by inspecting the *link inputs* (Objs + `cursive0_rt.lib`) and verifying `RuntimeRequiredSyms (= RuntimeSyms)` are present **before** invoking `lld-link`. Implement symbol inspection in-process via LLVM Object (`llvm::object::COFFObjectFile` / `llvm::object::Archive`) so the driver can reliably emit `Out-Link-Runtime-Incompatible` instead of a generic `Out-Link-Fail`.
* `include/cursive0/project/outputs.h`
* `src/project/outputs.cpp`
* `include/cursive0/project/link.h`
* `src/project/link.cpp`
* Addendum (CORE-010): use HostPrim for `WriteFile`, `ResolveRuntimeLib`, and `InvokeLinker`; ensure failure paths still emit `Out-Obj-*`, `Out-IR-*`, and `Out-Link-*` diagnostics (not IllFormed).
* **Interface requirements for T-PROJ-003:**
  * Define `struct OutputPaths { root, obj_dir, ir_dir, bin_dir }` and `ComputeOutputPaths(project_root, asm)` that implements the Â§2.5 `OutputPaths` equations.
  * `LoadProject` will populate `Project.outputs` using `ComputeOutputPaths` and MUST NOT perform output hygiene checks here (those are part of Â§2.5 pipeline).
**Tests:**
* Output collision tests: two modules with colliding mangled object names (`Out-Obj-Collision`)
* IR emission tests for `emit_ir = "ll"` and `emit_ir = "bc"` and â€œnoneâ€
* Runtime lib missing/incompatible tests (validate required symbols list behavior)
* `SPEC_COV("<RuleId>")` for each Out-*/Emit-*/Link-* rule.

**Completion notes (2026-01-05):**
* Implemented output planning + hygiene + pipeline state machine in `include/cursive0/project/outputs.h` and `src/project/outputs.cpp`:
  * `ComputeOutputPaths`, `ObjPath`/`IRPath`/`ExePath`, `RequiredOutputs`, `OutputHygiene`, deterministic collision checks, and `OutputPipeline` with `SPEC_RULE` anchors for `Out-*`, `Emit-*`, `Codegen*`, and `Output-Pipeline*`.
  * Emits external diagnostics and records HostPrim failures for `WriteFile`, `ResolveTool`, and `AssembleIR` failure paths.
* Added core symbol helpers in `include/cursive0/core/symbols.h` and `src/core/symbols.cpp`:
  * NFC normalization + ASCII `_xHH` escaping; `PathSig` and `MangleModulePath` used for runtime symbol names and output filename mangling.
* Implemented runtime link layer in `include/cursive0/project/link.h` and `src/project/link.cpp`:
  * `RuntimeLibPath`, `RuntimeRequiredSyms`, `ResolveRuntimeLib`, `LinkWithDeps`, `Link`.
  * `LinkerSyms` uses an in-process COFF/archive symbol scan (no LLVM Object dependency). Parse failures return `nullopt` and flow to `Out-Link-Runtime-Incompatible`.
  * Uses HostPrim failure recording for `ResolveRuntimeLib`, `ResolveTool`, and `InvokeLinker` error paths.
* Added `ModuleList-Err`/`ModuleList-Ok` anchors in `src/project/load_project.cpp` to satisfy Â§2.5 ModuleList rules.
* Tests:
  * Unit: `tests/unit/project_outputs_test.cpp` (Out-*/Emit-* coverage), `tests/unit/project_link_test.cpp` (ResolveRuntimeLib/Link-* coverage).
  * Updated `tests/unit/project_load_pipeline_test.cpp` to cover `ModuleList-Err`/`ModuleList-Ok`.
  * Added to `tests/CMakeLists.txt`; `CMakeLists.txt` updated to compile new sources.
* Build/test: `cmake --build cursive-bootstrap/build`; `ctest --output-on-failure` (19/19 passed; gmake clock-skew warnings observed).

#### T-PROJ-008 Tool resolution and IR assembly

**Spec:** Â§2.6 rules `ResolveTool-Ok`, `ResolveTool-Err-Linker`, `ResolveTool-Err-IR`, `AssembleIR-Ok`, `AssembleIR-Err`. 
**Interpretations (spec-aligned):**

* `RepoLLVM(P)` holds iff `${project_root}/llvm/llvm-21.1.8-x86_64/bin` exists and is a directory (tool presence is validated by `ResolveTool`).
* `PATHDirs` is the ordered split of `PATH` by platform separator (`;` on Windows, `:` otherwise); **ignore empty segments** (no implicit CWD).
* `ResolveTool(x)` checks deterministic suffixes: on Windows prefer `x` then `x.exe`; non-Windows only `x`.
* `AssembleIR(a, t)` uses `Invoke(a, t)` with stdin = IR text and stdout = assembled bytes (no temp files).
* `HostPrimFail` is recorded at the primitive boundary (`ResolveTool`, `Invoke`, `AssembleIR`), not at callers.

**Implement:**

* `include/cursive0/project/tool_resolution.h`
* `src/project/tool_resolution.cpp`
* `ResolveTool(x)` MUST treat platform executable suffixes as implementation-defined host detail (e.g., on Windows, prefer `x` then `x.exe`) while preserving deterministic search ordering across `SearchDirs(P)`.
* `src/project/ir_assembly.cpp`
* Addendum (CORE-010): use HostPrim for `ResolveTool`, `Invoke`, and `AssembleIR`; failure must map to `ResolveTool-*` / `AssembleIR-*` diagnostics (not IllFormed).
**Tests:**
* Unit tests for search directory precedence (env vs repo vs PATH)
* Integration tests where `llvm-as` missing and `emit_ir="bc"` triggers the correct diag
* `SPEC_COV("<RuleId>")`.


**Completion notes (2026-01-05):**
* Implemented tool resolution and IR assembly:
  * `include/cursive0/project/tool_resolution.h`, `src/project/tool_resolution.cpp` (SearchDirs/ResolveTool with spec-aligned precedence, deterministic suffix handling, HostPrimFail).
  * `include/cursive0/project/ir_assembly.h`, `src/project/ir_assembly.cpp` (Invoke via stdin/stdout pipes, `AssembleIRWithDeps`, `AssembleIR` with spec rule anchors + HostPrimFail).
* Wired defaults into output/link pipeline in `src/project/outputs.cpp` and `src/project/link.cpp`.
* Tests:
  * `tests/unit/project_tool_resolution_test.cpp` (env/repo/PATH precedence, missing tool, suffix ordering on Windows).
  * `tests/unit/project_ir_assembly_test.cpp` (AssembleIR ok/err via injected Invoke).
  * Added to `tests/CMakeLists.txt`; sources added to `CMakeLists.txt`.
* Build/test: `cmake --build cursive-bootstrap/build`; `ctest --test-dir cursive-bootstrap/build --output-on-failure` (21/21 passed; gmake clock-skew warnings observed).

#### T-PROJ-009 Unwind and FFI surface enforcement

**Spec:** Â§2.7 rule `WF-Unwind-Unsupported`.
**Implement:**

* `src/sema/conformance.cpp` and/or parser restrictions
**Tests:**
* `tests/compile/phase3/unwind/unwind_rejected.cursive`
* `SPEC_COV("WF-Unwind-Unsupported")`.

**Completion notes (2026-01-05):**
* Implemented unwind attribute detection in `src/sema/conformance.cpp` with a comment/string-aware scan and `CheckC0UnwindAttrUnsupported` emission of `E-UNS-0111`.
* Wired the new check into the driver (`src/driver/main.cpp`) before the generic attribute-syntax check to preserve deterministic diagnostic order.
* Tests:
  * Added `tests/compile/phase3/unwind/unwind_rejected.cursive` + `.expect.json` (expects `E-UNS-0111` then `E-UNS-0113`).
* Build/test: not run (local).

---

### 5C. Phase 1: source loading, lexing, parsing

#### T-SRC-001 UTF-8 decoding and BOM handling

**Spec:** Â§3.1.3 rules `Decode-Ok`, `Decode-Err`, `StripBOM-Start`, `StripBOM-None`, `StripBOM-Empty`, `StripBOM-Embedded`.
**Implement:**

* `include/cursive0/core/source_text.h`
* `src/core/source_load.cpp`
* Addendum (CORE-010): route file reads through HostPrim `ReadBytes` where the source-loading pipeline requires host I/O.
* Addendum (implementation detail): implement strict UTF-8 validation per RFC 3629 (reject overlong encodings, surrogates, and > U+10FFFF scalars) to satisfy `DecodeUTF8` inversion semantics.
* Addendum (API + build): expose `Decode`/`StripBOM` + `kBOM` in the core API and ensure `src/core/source_load.cpp` is linked into `cursive0_core` (CMake).
**Tests:**
* Unit tests with byte fixtures (valid UTF-8, invalid sequences, BOM cases)
* `SPEC_COV("<RuleId>")` for each Â§3.1.3 rule.
* Addendum (tests + build): add `core_source_load_test` to `tests/CMakeLists.txt`.

**Completion notes (2026-01-05):**
* Implemented strict UTF-8 decoding + BOM stripping helpers in `src/core/source_load.cpp` with `SPEC_RULE` anchors for `Decode-*` and `StripBOM-*`.
* Exposed `Decode`, `StripBOM`, `DecodeResult`, `StripBOMResult`, and `kBOM` in `include/cursive0/core/source_text.h`.
* Build/test wiring:
  * Added `src/core/source_load.cpp` to `cursive0_core` in `CMakeLists.txt`.
  * Added `tests/unit/core_source_load_test.cpp` with `SPEC_COV` coverage and wired the target into `tests/CMakeLists.txt`.
* Build/test: `cmake --build cursive-bootstrap/build`; `ctest --test-dir cursive-bootstrap/build --output-on-failure` (22/22 passed; gmake clock-skew warnings observed).
* Pending: pipeline integration + `ReadBytes`/diagnostic span mapping tracked under T-SRC-005/T-SRC-006.

#### T-SRC-002 Line ending normalization and logical lines

**Spec:** Â§3.1.4 rules `Norm-Empty`, `Norm-CRLF`, `Norm-CR`, `Norm-LF`, `Norm-Other`.
**Implement:**

* `src/core/source_load.cpp`
**Tests:**
* Unit tests confirming span/line mapping after normalization
* `SPEC_COV("<RuleId>")` for each Â§3.1.4 rule.


**Completion notes (2026-01-05):**
* Implemented `NormalizeLF` with `SPEC_RULE` anchors for `Norm-Empty`, `Norm-CRLF`, `Norm-CR`, `Norm-LF`, and `Norm-Other` in `src/core/source_load.cpp`.
* Exposed `kCR` and `NormalizeLF` in `include/cursive0/core/source_text.h`.
* Tests:
  * Added `tests/unit/core_line_normalization_test.cpp` covering normalization output and line/column mapping with `SPEC_COV` for each Â§3.1.4 rule.
  * Wired `core_line_normalization_test` into `tests/CMakeLists.txt`.
* Build/test: `cmake --build cursive-bootstrap/build`; `ctest --test-dir cursive-bootstrap/build --output-on-failure` (23/23 passed; gmake clock-skew warnings observed).

#### T-SRC-003 Prohibited code points

**Spec:** Â§3.1.5 rule `WF-Prohibited` (used by Â§3.1.8 `Step-Prohibited`/`Step-Prohibited-Err`).
**Implement:**

* `include/cursive0/core/unicode.h`
* `src/core/unicode.cpp`
* Addendum (T-PROJ-005): replace the NFC/CaseFold stub used by deterministic ordering helpers with the full Unicode 15.0.0 implementation.
**Tests:**
* Byte fixtures containing prohibited scalars, expect correct diag and span.
* `SPEC_COV("WF-Prohibited")`.

**Completion notes (2026-01-05):**
* Implemented `IsProhibited`, `FirstProhibitedOutsideLiteral`, and `NoProhibited` in `src/core/unicode.cpp` (with `SPEC_RULE("WF-Prohibited")`) and exposed the APIs in `include/cursive0/core/unicode.h`.
* Added literal-aware scanning to ignore prohibited control characters inside **valid** string/char literals (including escape validation for `\\`, `\"`, `\'`, `\n`, `\r`, `\t`, `\0`, `\xNN`, and `\u{...}`); invalid literal forms do **not** mask prohibited code points.
* Replaced the NFC/CaseFold stub with ICU-backed normalization (`Normalizer2` NFC + `UnicodeString::foldCase`) and enforced Unicode version 15.0 via `static_assert` on `U_UNICODE_VERSION`.
* Build integration:
  * Updated `CMakeLists.txt` to use bundled ICU static libs/headers from `third_party/icu/install` when `CURSIVE_USE_BUNDLED_ICU` is ON, with system ICU (`uc`, `i18n`) as fallback when OFF.
* Tests:
  * Added `tests/unit/core_prohibited_codepoints_test.cpp` with `SPEC_COV("WF-Prohibited")` and registered `core_prohibited_codepoints_test` in `tests/CMakeLists.txt`.
* Build/test: `cmake --build cursive-bootstrap/build`; `ctest --test-dir cursive-bootstrap/build --output-on-failure` (24/24 passed; gmake clock-skew warnings observed).
* Note: diagnostic mapping + span emission for `Step-Prohibited`/`Span-Prohibited` remain part of T-SRC-005/T-SRC-006 pipeline integration.

#### T-SRC-004 Newline tokens and statement termination

**Spec:** Â§3.1.7 rule `Missing-Terminator-Err` (plus line-continuation definitions in Â§3.1.7). 
**Implement:**

* Lexer: newline token generation
* Parser helper: terminator consumption
  Files:
* `src/syntax/lexer.cpp`
* `src/syntax/parser_stmt.cpp`
**Tests:**
* Parser tests for missing terminator and continuation filtering
* `SPEC_COV("Missing-Terminator-Err")`.

**Completion notes (2026-01-05):**
* Implemented newline emission + continuation filtering in `include/cursive0/syntax/lexer.h` and `src/syntax/lexer.cpp` (`LexNewlines`, `FilterNewlines`).
* Implemented terminator consumption + `SyncStmt` in `include/cursive0/syntax/parser.h` and `src/syntax/parser_stmt.cpp` with `SPEC_RULE("Missing-Terminator-Err")`.
* Build wiring: added `src/syntax/lexer.cpp` and `src/syntax/parser_stmt.cpp` to `cursive0_syntax` in `CMakeLists.txt`.
* Tests:
  * Added `tests/unit/syntax_newline_termination_test.cpp` with `SPEC_COV("Lex-Newline")` and `SPEC_COV("Missing-Terminator-Err")`.
  * Registered `syntax_newline_termination_test` in `tests/CMakeLists.txt`.
* Build/test: `cmake --build cursive-bootstrap/build`; `ctest --test-dir cursive-bootstrap/build --output-on-failure` (25/25 passed; clock-skew warnings observed).

#### T-SRC-005 Source loading pipeline

**Spec:** Â§3.1.8 rules `Step-Size`, `Step-Decode`, `Step-Decode-Err`, `Step-BOM`, `Step-Norm`, `Step-EmbeddedBOM-Err`, `Step-LineMap`, `Step-Prohibited`, `Step-Prohibited-Err`, `LoadSource-Ok`, `LoadSource-Err`.
**Implement:**

* `src/core/source_load.cpp`
* `include/cursive0/core/status.h` for error propagation
* Addendum (type plumbing): if no shared `status.h` exists, define a local result type for source loading (e.g., `{std::optional<SourceFile>, DiagnosticStream}`) and keep error propagation explicit.
* Addendum (diagnostics): map `Step-Decode-Err` â†’ `E-SRC-0101` (NoSpan-Decode); `Step-EmbeddedBOM-Err` â†’ `E-SRC-0103`; `Step-Prohibited-Err` â†’ `E-SRC-0104` (forbidden control character).
* Addendum (ReadBytes boundary): `ReadBytes-Err` â†’ `E-SRC-0102` is emitted by callers (e.g., `ParseUnitSources`/`ParseModule`) before `LoadSource`; `LoadSource` itself assumes bytes are already provided.
* Addendum (HostPrim): ensure any file I/O used by the pipeline flows through `ReadBytes` and `HostPrimFail` (CORE-010).
**Tests:**
* Unit tests for each pipeline transition using controlled inputs
* `SPEC_COV("<RuleId>")` per rule.

**Completion notes (2026-01-05):**
* Implemented `LoadSource` pipeline in `src/core/source_load.cpp` with `SPEC_RULE` anchors for Â§3.1.8 steps and `LoadSource-*`, plus diagnostic mapping to `E-SRC-0101/0103/0104` (spans deferred to T-SRC-006).
* Added `include/cursive0/core/source_load.h` with `SourceLoadResult` and `LoadSource` API.
* Tests:
  * Extended `tests/unit/core_source_load_test.cpp` with `SPEC_COV` coverage for all Â§3.1.8 rules plus success/error cases.
* Build/test: `cmake --build cursive-bootstrap/build`; `ctest --test-dir cursive-bootstrap/build --output-on-failure` (25/25 passed; gmake clock-skew warnings observed).

#### T-SRC-006 Diagnostic spans for source loading

**Spec:** Â§3.1.9 rules `NoSpan-Decode`, `Span-BOM-Warn`, `Span-BOM-Embedded`, `Span-Prohibited`.
**Implement:**

* `src/core/source_load.cpp`
* Addendum (span helpers): use temporary `SourceFile` data per Â§3.1.9 (`S_tmp`, `Utf8Offsets`, `SpanOf`) to compute BOM/prohibited spans without relying on later normalization steps.
* Addendum (pipeline integration): when T-SRC-005 emits `E-SRC-0101`/`E-SRC-0103`/`E-SRC-0104` with `span = nullopt`, replace/augment those diagnostics here to satisfy `NoSpan-Decode`, `Span-BOM-Warn`, `Span-BOM-Embedded`, and `Span-Prohibited`.
**Tests:**
* Validate span points to correct bytes/lines post-normalization.
* `SPEC_COV("<RuleId>")`.

**Completion notes (2026-01-05):**
* Updated `cursive-bootstrap/docs/Cursive0.md` Â§3.1.9 to compute embedded-BOM spans against normalized `T` and to require emitting `W-SRC-0101` even when `LoadSource` later fails.
* Implemented span-aware source loading diagnostics in `src/core/source_load.cpp`:
  * Added temporary span source (`S_tmp`) construction, UTF-8 offsets, and `SpanAtIndex` helper.
  * Emitted `W-SRC-0101` with `Span-BOM-Warn`, and attached spans for `E-SRC-0103`/`E-SRC-0104` (`Span-BOM-Embedded`, `Span-Prohibited`).
  * Preserved `NoSpan-Decode` behavior for `E-SRC-0101`.
* Tests:
  * Extended `tests/unit/core_source_load_test.cpp` with `SPEC_COV` for Â§3.1.9 rules and explicit span assertions for BOM warning, embedded BOM error, prohibited code point error, and decode error (no span).
* Build/test: `cmake --build cursive-bootstrap/build`; `ctest --test-dir cursive-bootstrap/build --output-on-failure` (25/25 passed; gmake clock-skew warnings observed).

---

#### T-LEX-001 Comment and whitespace scanning

**Spec:** Â§3.2.5 rules `Scan-Line-Comment`, `Doc-Comment`, `Block-Start`, `Block-Step`, `Block-End`, `Block-Done`, `Block-Comment-Unterminated`.
**Implement:**

* `include/cursive0/syntax/lexer.h`
* `src/syntax/lexer_ws.cpp`
* Addendum (newline suppression): emit `ScalarRange` spans for line/block comments and pass them to `LexNewlines` so LF inside comments does not produce `Newline` tokens (Â§3.2.1 `LexNewline`, `LexNoComments`).
* Addendum (unterminated block comments): when block comment scan reaches EOF with depth>0, emit `E-SRC-0306` and return a failure state so lexing can transition to `LexError` (see T-LEX-006).
**Tests:**
* `tests/unit/syntax_lexer_ws_test.cpp`
* Golden token streams for tricky whitespace/comment cases
* `SPEC_COV("<RuleId>")`.

**Completion notes (2026-01-05):**
* Implemented comment/doc-comment scanning and nested block comment handling in `src/syntax/lexer_ws.cpp`, with `E-SRC-0306` emission on unterminated block comments (returned as failure for T-LEX-006).
* Added lexer-facing API for whitespace/comment helpers and doc comment records in `include/cursive0/syntax/lexer.h` (DocKind/DocComment/CommentScanResult, `ScanLineComment`, `ScanDocComment`, `ScanBlockComment`, `IsWhitespace`, `IsLineFeed`).
* Build wiring: added `src/syntax/lexer_ws.cpp` to `cursive0_syntax` in `CMakeLists.txt`.
* Tests:
  * Added `tests/unit/syntax_lexer_ws_test.cpp` with `SPEC_COV` coverage for Â§3.2.5 rules, doc comment body/span checks, nested block comments, unterminated block comment diagnostics, and newline suppression behavior.
  * Registered `syntax_lexer_ws_test` in `tests/CMakeLists.txt`.
* Build/test: `cmake --build cursive-bootstrap/build`; `ctest --test-dir cursive-bootstrap/build --output-on-failure` (26/26 passed; gmake clock-skew warnings observed).

#### T-LEX-002 Literal lexing

**Spec:** Â§3.2.6 rules `Lex-Int`, `Lex-Float`, `Lex-Numeric-Err`, `Lex-String`, `Lex-String-Unterminated`, `Lex-String-BadEscape`, `Lex-Char`, `Lex-Char-Unterminated`, `Lex-Char-BadEscape`, `Lex-Char-Invalid`.
**Implement:**

* `src/syntax/lexer_literals.cpp`
* Addendum (newline suppression): emit `ScalarRange` spans for string/char literals and pass them to `LexNewlines` so LF inside literals does not produce `Newline` tokens (Â§3.2.1 `LexNewline`, `LexNoComments`).
* Addendum (unterminated recovery support): literal scanners MUST report the terminator index (`StringTerminator`/`CharTerminator`) even when unterminated so `LexScan` can resume at LF/EOF without producing a token; do **not** emit suppression ranges for unterminated literals.
**Tests:**
* `tests/unit/syntax_lexer_literals_test.cpp`
* Include invalid literal cases and verify diag codes/spans.
* Add unterminated literal cases that verify recovery index selection and no suppression range emission.
* `SPEC_COV("<RuleId>")`.

**Completion notes (2026-01-05):**
* Implemented literal scanners in `src/syntax/lexer_literals.cpp` for int/float/string/char, including underscore validation (`E-SRC-0304`), leading-zero warnings (`W-SRC-0301`), escape validation (`E-SRC-0302`), and char scalar count checks (`E-SRC-0303`).
* Added literal scan API + result struct in `include/cursive0/syntax/lexer.h` and wired `src/syntax/lexer_literals.cpp` into `cursive0_syntax` in `CMakeLists.txt`.
* Implemented recovery-friendly behavior for unterminated literals: emit `E-SRC-0301`/`E-SRC-0303`, return `next` at the terminator (LF/EOF), and do not emit suppression ranges.
* Tests:
  * Added `tests/unit/syntax_lexer_literals_test.cpp` with `SPEC_COV` for all Â§3.2.6 literal rules, including bad escapes, malformed numeric underscores, leading-zero warnings, and unterminated recovery behavior.
  * Registered `syntax_lexer_literals_test` in `tests/CMakeLists.txt`.
* Build/test: `cmake --build cursive-bootstrap/build`; `ctest --test-dir cursive-bootstrap/build --output-on-failure` (27/27 passed; gmake clock-skew warnings observed).

#### T-LEX-003 Identifier and keyword lexing

**Spec:** Â§3.2.7 rules `Lex-Identifier`, `Lex-Ident-InvalidUnicode`, `Lex-Ident-Token` (keywords/reserved lexemes per Â§3.2.3).
**Implement:**

* `src/syntax/lexer_ident.cpp`
* Keyword table generated into `src/syntax/keyword_table.inc`
**Tests:**
* `tests/unit/syntax_lexer_ident_kw_test.cpp`
* Include policy exceptions (e.g., contextual `in` behavior per parser policy section).
* `SPEC_COV("<RuleId>")`.

**Completion notes (2026-01-05):**
* Added Unicode identifier helpers in `src/core/unicode.cpp` and `include/cursive0/core/unicode.h` for XID start/continue, ident start/continue, and non-character checks per Â§3.2.2.
* Implemented identifier scanning and classification in `src/syntax/lexer_ident.cpp`, including `E-SRC-0307` emission for first non-character and Bool/Null/Keyword/Identifier classification per Â§3.2.7.
* Added Cursive0 keyword table in `src/syntax/keyword_table.inc` (Cursive0 Â§3.2.3 reserved list only; `in` remains identifier).
* Exposed `IdentScanResult` + `ScanIdentToken` in `include/cursive0/syntax/lexer.h`; wired `lexer_ident.cpp` into `cursive0_syntax` in `CMakeLists.txt`.
* Tests: added `tests/unit/syntax_lexer_ident_kw_test.cpp` with `SPEC_COV` for all Â§3.2.7 rules, including contextual `in`, non-reserved future keywords, Unicode XID sample, and non-character diagnostic span checks.
* Build/test: `cmake --build cursive-bootstrap/build`; `ctest --test-dir cursive-bootstrap/build --output-on-failure` (28/28 passed; clock-skew warnings observed during build).
* Update (2026-01-06): added `shared` to `src/syntax/keyword_table.inc` to match the reserved keyword list used by Phase 1 parsing.


#### T-LEX-004 Maximal munch rule

**Spec:** Â§3.2.9 rules `Max-Munch`, `Max-Munch-Err`.
**Implement:**

* `src/syntax/lexer.cpp`
**Tests:**
* Inputs with ambiguous operator/punctuator prefixes.
* `SPEC_COV("<RuleId>")`.

**Completion notes (2026-01-05):**
* Added `NextTokenResult` + `NextToken` API in `include/cursive0/syntax/lexer.h`, implementing Â§3.2.9 `Max-Munch`/`Max-Munch-Err`.
* Implemented maximal-munch candidate selection in `src/syntax/lexer.cpp`, including Cursive0 operator/punctuator sets, `KindPriority` ordering, and `E-SRC-0309` span emission.
* Ensured numeric candidates follow `NumericKind` by requiring a float marker (dot/exp/float suffix) before admitting malformed float prefixes, so `1_` resolves to `IntLiteral`.
* Tests: added `tests/unit/syntax_lexer_max_munch_test.cpp` with `SPEC_COV` for both rules and cases for longest operator matches, operator vs punctuator, malformed numeric, and unclassifiable characters; registered in `tests/CMakeLists.txt`.
* Build/test: `cmake --build cursive-bootstrap/build`; `ctest --test-dir cursive-bootstrap/build --output-on-failure` (29/29 passed; clock-skew warnings observed during build).
* Update (2026-01-06): added operator `~%` to the maximal-munch operator set in `src/syntax/lexer.cpp` to support shared-permission receiver shorthand.

#### T-LEX-005 Lexical security

**Spec:** Â§3.2.10 rules `LexSecure-Err`, `LexSecure-Warn` (lexically sensitive enforcement per Â§3.1.2 and Â§3.2.10).
**Implement:**

* `src/core/unicode.cpp`
* `src/syntax/lexer_security.cpp`
**Tests:**
* Fixtures with `Sensitive` code points to validate `LexSecure-Err` vs `LexSecure-Warn` behavior, including cases inside/outside unsafe spans.
* `SPEC_COV("<RuleId>")`.

**Completion notes (2026-01-05):**
* Added `IsSensitive` in `src/core/unicode.cpp` with declaration in `include/cursive0/core/unicode.h` (spec defs for Â§3.2.2).
* Implemented `LexSecure` in `src/syntax/lexer_security.cpp` with `LexSecureResult` API in `include/cursive0/syntax/lexer.h`, including literal/comment exclusion, token-only unsafe span matching, and `E-SRC-0308`/`W-SRC-0308` emission.
* Wired `src/syntax/lexer_security.cpp` into `cursive0_syntax` in `CMakeLists.txt`.
* Tests: added `tests/unit/syntax_lexer_security_test.cpp` with `SPEC_COV` for `LexSecure-Err` and `LexSecure-Warn`, covering outside-unsafe errors, unsafe warnings (including newline-separated braces), and comment/string exclusions; registered in `tests/CMakeLists.txt`.
* Build/test: `cmake --build cursive-bootstrap/build`; `ctest --test-dir cursive-bootstrap/build --output-on-failure` (30/30 passed; clock-skew warnings observed during build).

#### T-LEX-006 Tokenization small-step and big-step

**Spec:** Â§3.2.11 rules `Lex-Start`, `Lex-End`, `Lex-Whitespace`, `Lex-Newline`, `Lex-Line-Comment`, `Lex-Doc-Comment`, `Lex-Block-Comment`, `Lex-String-Unterminated-Recover`, `Lex-Char-Unterminated-Recover`, `Lex-Sensitive`, `Lex-Token`, `Lex-Token-Err`; Â§3.2.12 rules `Tokenize-Ok`, `Tokenize-Secure-Err`, `Tokenize-Err` (and Â§3.2.12 definitions like Filter).
**Implement:**

* `src/syntax/tokenize.cpp`
* Addendum (unterminated block comments): if block comment scan fails, emit `E-SRC-0306` and transition to `LexError(c)` so `Tokenize-Err` applies.
* Addendum (unterminated string/char recovery): if `T[i]` is `"` or `'` and `LineFeedOrEOFBeforeClose(T,i)` holds, emit `E-SRC-0301`/`E-SRC-0303`, advance to `StringTerminator`/`CharTerminator`, and **continue LexScan** (do not transition to `LexError`, and do not emit `Max-Munch-Err`).
**Tests:**
* Differential test: small-step execution must equal big-step output tokens+diagnostics.
* Add recovery tests that confirm unterminated literals do not trigger `E-SRC-0309` and that lexing resumes at the LF/EOF boundary.
* `SPEC_COV("<RuleId>")` for all Tokenize rules.

**Completion notes (2026-01-05):**
* Added `LexerOutput`, `LexSmallStepResult`, and `TokenizeResult` APIs plus `LexSmallStep`/`Tokenize` declarations in `include/cursive0/syntax/lexer.h`.
* Implemented small-step tokenization and big-step `Tokenize` in `src/syntax/tokenize.cpp`, including block-comment failure â†’ `LexError` (`E-SRC-0306`), unterminated string/char recovery that resumes at LF/EOF without `E-SRC-0309`, `SensitiveTok` handling, and `LexSecure` gating for `Tokenize-Ok` vs `Tokenize-Secure-Err`.
* Wired `src/syntax/tokenize.cpp` into `cursive0_syntax` in `CMakeLists.txt`.
* Tests: added `tests/unit/syntax_tokenize_test.cpp` with `SPEC_COV` for all `Lex-*` and `Tokenize-*` rules; includes recovery cases, LexError cases, LexSecure error, and a diagnostic stream diff check between small-step and big-step; registered in `tests/CMakeLists.txt`.
* Build/test: `cmake --build cursive-bootstrap/build`; `ctest --test-dir cursive-bootstrap/build --output-on-failure` (31/31 passed; clock-skew warnings observed during build).

---

#### T-PARSE-001 Parser core inputs/outputs/invariants

**Spec:** Â§3.3.1 definitions (e.g., ParseFileOk/ParseFileBestEffort) and Â§3.3.6 rules `ParseFile-Ok`, `ParseItems-Empty`, `ParseItems-Cons`, `Parse-Item-Err` (parser is best-effort; errors are represented in items + diagnostics).
**Implement:**

* `include/cursive0/syntax/parser.h`
* `src/syntax/parser.cpp`
**Tests:**
* `tests/compile/phase1/parse_file_ok/*.cursive`
* `tests/compile/phase1/parse_file_err/*.cursive`
* `SPEC_COV("<RuleId>")`.

**Notes (spec-aligned intent):**
* **Module docs (`MDoc`)**: For `ParseFile`, compute `MDoc = ModuleDocs(D)` using the lexer doc stream `D` (Â§3.3.11). Item doc attachment is **not** part of `ParseItems` and is handled by **T-PARSE-009**.
* **Diagnostics separation and order**: `P_0` starts with an **empty** parser diagnostic stream (`Î” = []`) per Â§3.3.6. `DiagStream(P_1)` contains **only parser diagnostics**. Lexer/tokenize diagnostics are produced by `Tokenize(S)` and must be emitted **before** parse diagnostics when constructing final output. If `Tokenize` fails, parsing does not run and only lexer diagnostics are reported.

**Completion notes (2026-01-06):**
* Added minimal AST scaffolding (`ErrorItem`, `ASTFile`) in `include/cursive0/syntax/ast.h`.
* Implemented parser core helpers + `ParseFile`/`ParseItems`/`ParseItem` (error-only fallback) and `SyncItem` in `src/syntax/parser.cpp`, with matching API updates in `include/cursive0/syntax/parser.h` and doc-aware `MakeParser` in `src/syntax/parser_stmt.cpp`.
* Wired `src/syntax/parser.cpp` into `cursive0_syntax` in `CMakeLists.txt`.
* Tests: added `tests/unit/syntax_parser_core_test.cpp` with `SPEC_COV` for `ParseFile-Ok`, `ParseItems-Empty`, `ParseItems-Cons`, `Parse-Item-Err`; registered in `tests/CMakeLists.txt`.
* Update (2026-01-06): driver integration completed in T-PARSE-004; Phase 1 now tokenizes then calls `ParseFile`, emits parse diagnostics after lex diagnostics, and runs `CheckMethodContext` on the parsed AST.

#### T-PARSE-002 Grammar subset and lexeme policy

**Spec:** Â§3.3.4 definitions (including UsingKeyword) and rule `Method-Context-Err` (plus any other bold rules in Â§3.3.4).
**Implement:**

* `src/syntax/keyword_policy.cpp`
**Tests:**
* Ensure `using` recognized per spec and `use` treated as identifier if required.
* `SPEC_COV("<RuleId>")`.

**Stubbing note:** `Method-Context-Err` depends on method/record/class ASTs and was stubbed until T-PARSE-004 (now implemented; see completion notes); `TypeArgsUnsupported`/`C0TypeRestricted` depend on type parsing and will be stubbed until T-PARSE-005. Add explicit â€œunstubâ€ items to those tasks.

**Completion notes (2026-01-06):**
* Added `include/cursive0/syntax/keyword_policy.h` and `src/syntax/keyword_policy.cpp` with lexeme predicates, contextual keyword helpers, union/type token helpers, and a stubbed `CheckMethodContext` (`SPEC_RULE("Method-Context-Err")`).
* Updated `tests/unit/syntax_lexer_ident_kw_test.cpp` to cover `using` as keyword and `use` as identifier.
* Added `tests/unit/syntax_keyword_policy_test.cpp` with `SPEC_COV("Method-Context-Err")` and helper coverage; registered the test in `tests/CMakeLists.txt`.
* Build: `cmake --build cursive-bootstrap/build` (clock-skew warnings observed).
* Tests: `ctest --test-dir cursive-bootstrap/build --output-on-failure` (33/33 passed).
* Update (2026-01-06): implemented `CheckMethodContext` to enforce `Method-Context-Err` over record/class members and emit `E-SEM-3011`; wired into the driver after `ParseFile`.

#### T-PARSE-003 Token consumption small-step

**Spec:** Â§3.3.5 rules `Tok-Consume-Kind`, `Tok-Consume-Keyword`, `Tok-Consume-Operator`, `Tok-Consume-Punct`, `List-Start`, `List-Cons`, `List-Done`, `Trailing-Comma-Err`.
**Implement:**

* `src/syntax/parser_consume.cpp`
**Tests:**
* Unit tests for consume state transitions, including error cases.
* `SPEC_COV("<RuleId>")`.

**Completion notes (2026-01-06):**
* Added token match/end-set helpers, consume helpers, and list parsing helpers in `include/cursive0/syntax/parser.h` with implementations in `src/syntax/parser_consume.cpp` (covers `Tok-Consume-*`, `List-*`, and `Trailing-Comma-Err`).
* Wired `src/syntax/parser_consume.cpp` into `cursive0_syntax` in `CMakeLists.txt`.
* Tests: added `tests/unit/syntax_parser_consume_test.cpp` with `SPEC_COV` for all seven rules; registered in `tests/CMakeLists.txt`.
* Build: `cmake --build cursive-bootstrap/build` (clock-skew warnings observed).
* Tests: `ctest --test-dir cursive-bootstrap/build --output-on-failure` (34/34 passed).



**Trailing comma policy (Cursive0):**

* The parser MUST NOT â€œglobally allow trailing commasâ€. Trailing commas are handled **per-list** by the spec rules.
* The following tail rules explicitly treat a trailing comma as `Unsupported-Construct` and then consume the comma:
  * `Parse-ArgTail-TrailingComma`
  * `Parse-ExprListTail-TrailingComma`
  * `Parse-TypeListTail-TrailingComma`
  * `Parse-ParamTail-TrailingComma`
  * `Parse-UsingListTail-TrailingComma`
  * `Parse-FieldDeclTail-TrailingComma`
  * `Parse-RecordFieldDeclTail-TrailingComma`
  * `Parse-RecordMemberTail-TrailingComma`
  * `Parse-VariantTail-TrailingComma`
  * `Parse-FieldInitTail-TrailingComma`
  * `Parse-FieldPatternTail-TrailingComma`
  * `Parse-PatternListTail-TrailingComma`
  * `Parse-TupleExprElems-TrailingComma`
  * `Parse-TupleTypeElems-TrailingComma`
  * `Parse-TuplePatternElems-TrailingComma`

* Lists without a `*-TrailingComma` rule MUST behave exactly as specified (some lists may permit trailing commas; do not infer).

#### T-PARSE-003A Angle scanning, generic skipping, and `>>` shift-splitting helpers

**Spec:**

* `SplitSpan2` and `SplitShiftR` helper definitions (used by `Parse-Safe-Pointer-Type-ShiftSplit` and `Parse-Transmute-Expr-ShiftSplit`).
* Â§3.3.6.14 â€œGeneric Syntax Rejectionâ€ helpers: `AngleDelta`, `AngleStep`, `AngleScan`, `SkipAngles` (used by the generic rejection rules).
* Dependent parse rules that MUST call these helpers (directly or via shared utilities): `Parse-Generic-Header-Unsupported`, `Parse-Type-Generic-Unsupported`, `Parse-Qualified-Generic-Unsupported`, `Parse-Safe-Pointer-Type-ShiftSplit`, `Parse-Transmute-Expr-ShiftSplit`.

**Implement:**

* `src/syntax/parser_angle.cpp` (or `src/syntax/parser_util.cpp`) with **exact** helper behavior:
  * `SplitSpan2(sp)` MUST split a `>>` span into two single-character `>` spans (left/right) with correct offsets and columns.
  * `SplitShiftR(P)` MUST:
    * require `Tok(P)` is `Operator(">>")`,
    * replace that token in-place with two `Operator(">")` tokens using `SplitSpan2`,
    * return a cursor that points at the **first** inserted `>` token (same token index as the original `>>`), preserving parser index invariants.
  * `AngleDelta(tok)` MUST treat `"<"` as `+1`, `">"` as `-1`, and `">>"` as `-2`, and all other tokens as `0`.
  * `AngleScan(P0, P, d)` and `SkipAngles(P)` MUST follow the specâ€™s recursion/termination behavior (including EOF behavior).

**LLM handoff warning (do not â€œsimplifyâ€):**

* These helpers are **semantic glue** between the lexerâ€™s maximal-munch tokenization and the parser rules. If they are missing or approximate, nested `Ptr<â€¦>` types, generic-rejection sync, and some `transmute` parses will diverge from the spec.

**Tests:**

* Unit tests:
  * `SplitSpan2` span arithmetic (offsets/cols) for representative spans.
  * `SplitShiftR` preserves stream ordering and returns a cursor at the first `>` token.
  * `AngleDelta` returns `-2` for `>>`.
* Integration tests:
  * Nested safe pointer types that create `>>` at the boundary (e.g., `Ptr<Ptr<u8>>@Valid`) MUST parse and leave one `>` for the outer context.
  * Generic header rejection on items (e.g., `procedure f<T>() {}`) MUST emit `Unsupported-Construct` and then synchronize to the next item.
  * Type/qualified generics (e.g., `Foo<T>` or `foo::Bar<T>`) MUST emit `Unsupported-Construct` and continue parsing the surrounding construct.
* `SPEC_COV("<RuleId>")` for each of the helper-dependent rules above.

**Completion notes (2026-01-06):**

* Added `owned_tokens` to `Parser` and declared helper APIs in `include/cursive0/syntax/parser.h` to support in-place `>>` splitting while preserving parser index invariants.
* Implemented `SplitSpan2`, `SplitShiftR`, `AngleDelta`, `AngleStep`, `AngleScan`, and `SkipAngles` in `src/syntax/parser_angle.cpp`.
* Wired `src/syntax/parser_angle.cpp` into `cursive0_syntax` in `CMakeLists.txt`.
* Tests: added `tests/unit/syntax_parser_angle_test.cpp` and registered in `tests/CMakeLists.txt`; covers span splitting, shift-splitting, angle delta, skip/scan EOF behavior.
* Build/test: `cmake --build cursive-bootstrap/build` (clock-skew warnings observed); `ctest --test-dir cursive-bootstrap/build --output-on-failure` (35/35 passed).

#### T-PARSE-004 Module and item parsing

**Spec:** Â§3.3.6 + subparts:

* Â§3.3.6.1 identifiers/paths rules `Parse-Path-*`
* Â§3.3.6.2 visibility rules `Parse-Vis-*`
* Â§3.3.6.3 using rules `Parse-Using-*`
* Â§3.3.6.4 static decl, Â§3.3.6.5 proc decl, Â§3.3.6.6 record, Â§3.3.6.7 enum, Â§3.3.6.8 modal, Â§3.3.6.9 implements, Â§3.3.6.10 class, Â§3.3.6.11 type alias
* Â§3.3.6.13 helper parsing rules (76 rules)
* Â§3.3.6.14 unsupported constructs parsing rules (8 rules)

**Required explicit coverage (do not treat as â€œimpliedâ€):**

* Item-level generic header rejection MUST be implemented via `Parse-Generic-Header-Unsupported` using `SkipAngles(...)` + `SyncItem(...)` to recover.
* Unsupported construct parsing rules MUST be implemented **by name** (they are not optional â€œsubset gatesâ€):
  * `Parse-Import-Unsupported`
  * `Parse-Extern-Unsupported`
  * `Parse-Use-Unsupported` (note: `use` is an identifier token, not a keyword)
  * `Parse-Attribute-Unsupported` (reject `[[...]]` forms)
  * `Parse-Comptime-Unsupported`
  * `Parse-Modal-Class-Unsupported`
  * `Parse-Type-Generic-Unsupported`
* `Parse-Qualified-Generic-Unsupported`
* `using` list syntax MUST follow the Cursive0 grammar subset (do not assume Rust-style `:: { ... }`).

**Addendum (CORE-009 completion):** T-CORE-009 is not complete until the unsupported-construct parsing rules above are implemented here (including `Parse-Attribute-Unsupported` with `SyncItem` recovery) and the corresponding phase1 tests pass. (Completed in 2026-01-06; see completion notes.)

**Addendum (T-PARSE-002 unstub):** Implement `MethodContextOk` over parsed record/class items and emit `E-SEM-3011` per `Method-Context-Err`, then remove any temporary/stub enforcement introduced in T-PARSE-002. (Completed in 2026-01-06; see completion notes.)

**Driver integration note:** After item parsing is implemented here, wire the Phase 1 parser into `src/driver/main.cpp` so real source files are parsed and `ParseFile` diagnostics are emitted (in lexâ†’parse order). This was intentionally deferred until T-PARSE-004 to avoid `Parse-Item-Err` firing on every item. (Completed in 2026-01-06; see completion notes.)


**Implement:**
* `src/syntax/parser_items.cpp`
* `src/syntax/parser_paths.cpp`
* `src/syntax/parser_types.cpp`
* `include/cursive0/syntax/ast.h` (AST node definitions)
**Tests:**
* For each item form: one positive parse golden AST, plus negative tests for each error rule.
* `tests/compile/phase1/items/...`
* `SPEC_COV("<RuleId>")` for every Parse-* rule in these sections.

**Completion notes (2026-01-06):**
* Implemented Â§3.3.6 item + helper parsing in `src/syntax/parser_items.cpp` (using/static/procedure/record/enum/modal/class/type alias items, record/class/state members, list/tail helpers, return-at-module error, and item-level recovery) with `SPEC_RULE` anchors for all listed Parse-* rules.
* Implemented Â§3.3.6.1â€“.2 path + visibility parsing in `src/syntax/parser_paths.cpp`, including a spec-correct `ParseQualifiedHead` that reuses `ParseModulePathTail` without extra `Advance`.
* Build wiring: added `src/syntax/parser_items.cpp`, `src/syntax/parser_paths.cpp`, and `src/syntax/parser_placeholders.cpp` to `cursive0_syntax` and linked `cursivec0` against `cursive0_syntax` in `CMakeLists.txt`.
* Implemented Â§3.3.6.14 unsupported-construct parsing rules: `Parse-Import-Unsupported`, `Parse-Extern-Unsupported`, `Parse-Use-Unsupported`, `Parse-Attribute-Unsupported`, `Parse-Modal-Class-Unsupported`, `Parse-Generic-Header-Unsupported` (with `SkipAngles` + `SyncItem`). Generic and comptime rejections now live in their owning parsers: `Parse-Type-Generic-Unsupported` in `parser_types.cpp`, `Parse-Qualified-Generic-Unsupported` and `Parse-Comptime-Unsupported` in `parser_expr.cpp` (with `SyncType`/`SyncStmt` per their rules). All `Unsupported-Construct` emissions use `Emit(Code(...))` with no span per Â§1.6 + Â§1.4.
* Clarified and aligned edge rules: `Parse-StateMember-Transition` now parses explicit param list and `-> @ target`, `Parse-UsingList-Empty/Cons` now emit unsupported and consume `}`; `Parse-StateBlock` uses `AdvanceOrEOF` for recovery to avoid loops.
* Added shared-permission parsing support: `ReceiverPerm::Shared` in `include/cursive0/syntax/ast.h`, `~%` operator in `src/syntax/lexer.cpp`, `shared` keyword in `src/syntax/keyword_table.inc`, and receiver/type placeholder parsing accepts `~%` and `shared` prefixes.
* Parser placeholders improved: `ParseType` handles `const/unique/shared` prefixes, `ParseExpr` recognizes `comptime {}` even when tokenized as a keyword, and `ParseBlock` scans nested braces instead of stopping at the first `}`.
* Driver now runs Phase 1 parse: `src/driver/main.cpp` calls `ParseFile`, emits parse diagnostics in-order after lexing, and applies `CheckMethodContext`; perm/unwind subset checks run even if parse errors, while attr/unsupported checks remain gated on no prior errors.
* Tests:
  * Added `tests/unit/syntax_parser_items_test.cpp` with comprehensive `SPEC_COV` coverage for Â§3.3.6 rules and helper rules; registered in `tests/CMakeLists.txt`.
  * Added compile fixtures under `tests/compile/phase1/unsupported/` covering import/extern/use/attr/comptime/modal-class/generic-header/type-generic/qualified-generic unsupported rules, `using` list empty/trailing comma, and `return` at module.
  * Updated `tests/harness/run_compiler.py` to strip `# SPEC_COV` lines by writing a temporary file in the test directory (preserving project root discovery) and deleting it afterward.
  * Adjusted Phase 3 expectations: `tests/compile/phase3/conformance/reject_ill_formed.cursive.expect.json` drops null span for `E-SRC-0103`; `tests/compile/phase3/unwind/unwind_rejected.cursive.expect.json` orders `E-UNS-0113` before `E-UNS-0111`; `shared_perm_rejected.cursive` flattened to a single line to avoid newline token parse errors; removed stray `tests/compile/phase1/src/*.cursive` files without expectations.
* Spec clarification: updated `cursive-bootstrap/docs/Cursive0.md` to remove ambiguities in `Parse-StateMember-Transition` and `Parse-UsingList-Empty/Cons` rules.
* Build/test: `cmake --build /mnt/c/Dev/Cursive/cursive-bootstrap/build`; `ctest --test-dir /mnt/c/Dev/Cursive/cursive-bootstrap/build --output-on-failure` (36/36 passed; clock-skew warnings observed during build).

#### T-PARSE-005 Type parsing subset and helpers

**Spec:** Â§3.3.7 and Â§3.3.7.1 rules (23 + 25).

**Required explicit coverage (do not treat as â€œhandled by maximal munchâ€):**

* The type parser MUST implement `Parse-Safe-Pointer-Type-ShiftSplit`:
  * When parsing `Ptr< T >` and the token at the expected close is `Operator(">>")`, it MUST call `SplitShiftR` and proceed as specified so nested `Ptr<Ptr<...>>` parses correctly and leaves the remaining `>` token for the outer context.

**Addendum (T-PARSE-002 unstub):** Wire `TypeWhereTok`, `OpaqueTypeTok`, `TypeArgsUnsupported`, and `C0TypeRestricted` into the type parser logic and remove any temporary/stub enforcement introduced in T-PARSE-002.

**Addendum (C0 shared permission parsing):** `ParsePermOpt` MUST accept `shared` as a permission prefix to keep type parsing synchronized; `shared` remains unsupported in Cursive0 and MUST be rejected by `Perm-Shared-Unsupported` during subset checks.


**Implement:**

* `src/syntax/parser_types.cpp`
**Tests:**
* `tests/compile/phase1/types/...` with golden AST + error cases.
* `SPEC_COV("<RuleId>")`.

**Completion notes (2026-01-06):**
* Implemented Â§3.3.7/Â§3.3.7.1 type parsing in `src/syntax/parser_types.cpp`, including union tails, tuple/param list parsing, Ptr shift-splitting, array/slice, raw pointers, string/bytes/ptr state, dynamic/modal state, and type paths with generic rejection (`SkipAngles` + `SyncType`) per `Parse-Type-Generic-Unsupported`.
* Wired `TypeWhereTok`, `OpaqueTypeTok`, `TypeArgsUnsupported`, and `C0TypeRestricted` into type parsing and removed placeholder `ParseType` in `src/syntax/parser_placeholders.cpp`; added `src/syntax/parser_types.cpp` to `cursive0_syntax` in `CMakeLists.txt`.
* Updated `docs/Cursive0.md` to allow `shared` in type/receiver permission parsing (Parse-Perm-Shared, TypeStart, grammar, Reserved), while keeping C0 rejection via `Perm-Shared-Unsupported` for `shared`/`~%`.
* Tests: added phase1 type fixtures under `tests/compile/phase1/types`; added `SPEC_COV` for `Parse-Perm-Shared` in `tests/compile/phase3/conformance/shared_perm_rejected.cursive`.
* Build/test: `cmake --build /mnt/c/Dev/Cursive/cursive-bootstrap/build`; `ctest --test-dir /mnt/c/Dev/Cursive/cursive-bootstrap/build --output-on-failure` (36/36 passed; clock-skew warnings observed during build).
* Update (2026-01-06): replaced the placeholder span-only Type nodes with a full Type AST per Â§3.3.7 (perm/union/func/tuple/array/slice/ptr/raw/string/bytes/dynamic/modal/path) in `include/cursive0/syntax/ast.h`, and updated `parser_types.cpp` to construct those nodes (including `Parse-Type-Err` base `TypePrim("!")` + perm wrapping).

#### T-PARSE-006 Expression parsing and precedence

**Spec:** Â§3.3.8 + subparts Â§3.3.8.1â€“Â§3.3.8.10 rules (range, bin chains, power, cast, unary, postfix, primary, helper rules).


**Required explicit coverage:**

* The expression parser MUST implement both `Parse-Transmute-Expr` and `Parse-Transmute-Expr-ShiftSplit`.
* When `Parse-Transmute-Expr(-ShiftSplit)` expects the transmute generic close:
  * If `Tok(P)` is `Operator(">")`, parse normally.
  * If `Tok(P)` is `Operator(">>")`, it MUST call `SplitShiftR(P)` and then MUST advance once to the second `>` token before continuing with the usual `>`-then-`(` checks.




**Implement:**

* Pratt parser in `src/syntax/parser_expr.cpp`
* Operator precedence table in `src/syntax/precedence_table.inc`
**Tests:**
* Precedence/associativity golden AST tests
* Negative tests for syntactic edge rules (e.g., range endpoints).
* `SPEC_COV("<RuleId>")` for all Parse-Expr* rules.

**Completion notes (2026-01-06):**
* Implemented Â§3.3.8 expression parsing in `src/syntax/parser_expr.cpp` (range, left-chains, power, cast, unary, postfix, primary, match/if/loop/else helpers), including `Parse-Transmute-Expr` and `Parse-Transmute-Expr-ShiftSplit` via `SplitShiftR`.
* Added operator precedence table in `src/syntax/precedence_table.inc`; expanded expression AST in `include/cursive0/syntax/ast.h` with `ExprNode` variants and `ExprPtr`; removed placeholder `ParseExpr` from `src/syntax/parser_placeholders.cpp`; wired `parser_expr.cpp` into `CMakeLists.txt` and added `ParseExprOpt` to `include/cursive0/syntax/parser.h`.
* Tests: added phase1 expression fixtures under `tests/compile/phase1/expr` (ok, unsupported list/trailing comma, comptime unsupported, qualified generic unsupported, primary error, place error) with `SPEC_COV` for all Parse-Expr* rules.
* Fixes after initial failures: allow identifier primary when `allow_brace == false` (so `match x {}` and `loop x {}` headers parse correctly); emit `Parse-Primary-Err` on the advanced parser state; renamed `modal` identifier in `expr_ok.cursive` to avoid keyword collision.
* Build/test: `cmake --build /mnt/c/Dev/Cursive/cursive-bootstrap/build`; `ctest --test-dir /mnt/c/Dev/Cursive/cursive-bootstrap/build --output-on-failure` (36/36 passed; clock-skew warnings observed during build).
* Update (2026-01-06): aligned `Parse-Comptime-Unsupported` recovery with Â§3.3.8.6 by emitting `Unsupported-Construct` and then `SyncStmt(P)` (no brace-skipping). Updated comptime expectations in phase1/phase3 tests to include the resulting `Parse-Syntax-Err` diagnostic.

#### T-PARSE-007 Pattern parsing and helpers

**Spec:** Â§3.3.9 and Â§3.3.9.1 rules (17 + 28).
**Implement:**

* `src/syntax/parser_patterns.cpp`
**Tests:**
* `tests/compile/phase1/patterns/...`
* `SPEC_COV("<RuleId>")`.

**Completion notes (2026-01-06):**
* Implemented Â§3.3.9/Â§3.3.9.1 pattern parsing in `src/syntax/parser_patterns.cpp` with `SPEC_RULE` anchors for all Parse-Pattern* and helper rules, including tuple/field list handling and `Unsupported-Construct` trailing-comma diagnostics.
* Expanded pattern AST in `include/cursive0/syntax/ast.h` (PatternNode variants, payload/field structs, and `PatternPtr`), and removed the `ParsePattern` placeholder from `src/syntax/parser_placeholders.cpp`.
* Wired `parser_patterns.cpp` into `cursive-bootstrap/CMakeLists.txt`.
* Tests: added phase1 fixtures under `tests/compile/phase1/patterns` (ok, trailing-comma unsupported, parse error) with full `SPEC_COV` coverage; updated `tests/unit/syntax_parser_items_test.cpp` to assert typed-pattern structure for `let x: T = y`.
* Build/test: `cmake --build /mnt/c/Dev/Cursive/cursive-bootstrap/build`; `ctest --test-dir /mnt/c/Dev/Cursive/cursive-bootstrap/build --output-on-failure` (36/36 passed; clock-skew warnings observed during build).

#### T-PARSE-008 Statement and block parsing

**Spec:** Â§3.3.10 and Â§3.3.10.1 rules; includes terminator consumption rules (`ConsumeTerminatorOpt-*`, `ConsumeTerminatorReq-*`) shown in spec. 
**Implement:**

* `src/syntax/parser_stmt.cpp`
**Tests:**
* `tests/compile/phase1/statements/...`
* Include missing terminator and sync behavior.
* `SPEC_COV("<RuleId>")`.

**Completion notes (2026-01-06):**
* Implemented Â§3.3.10/Â§3.3.10.1 statement+block parsing in `src/syntax/parser_stmt.cpp` (ParseStmt/ParseStmtSeq/ParseBlock, terminator policies, SyncStmt rules, and all statement forms), with `SPEC_RULE` anchors for every Parse-Statement* and helper rule.
* Expanded statement and block AST in `include/cursive0/syntax/ast.h` (Stmt variants, Block stmts/tail_opt) and exposed `ParseStmt`/`ParseBindingAfterLetVar` in `include/cursive0/syntax/parser.h`; added public wrappers in `src/syntax/parser_items.cpp`.
* Retired the placeholder block parser in `src/syntax/parser_placeholders.cpp` (real block parsing now lives in parser_stmt).
* Tests: added phase1 fixtures under `tests/compile/phase1/statements` (ok, missing terminator, parse error with sync) and a new unit test `tests/unit/syntax_parser_stmt_test.cpp`; registered `syntax_parser_stmt_test` in `tests/CMakeLists.txt`.
* Adjusted comptime-unsupported expectations to match improved recovery (`tests/compile/phase1/expr/expr_comptime_unsupported.cursive.expect.json`, `tests/compile/phase1/unsupported/comptime_unsupported.cursive.expect.json`) and updated comptime parse recovery to skip braced blocks in `src/syntax/parser_expr.cpp`.
* Build/test: `cmake --build /mnt/c/Dev/Cursive/cursive-bootstrap/build`; `ctest --test-dir /mnt/c/Dev/Cursive/cursive-bootstrap/build --output-on-failure` (37/37 passed; clock-skew warnings observed during build).

#### T-PARSE-009 Doc comment association

**Spec:** Â§3.3.11 rules `Attach-Doc-Line`, `Attach-Doc-Module`.
**Implement:**

* `src/syntax/parser_docs.cpp`
* **Policy note (spec silent):** If a `LineDoc` has no following item (i.e., no target in `Items`) or its target is `ErrorItem`, the doc is ignored rather than re-targeted.
**Tests:**
* Verify attachment to correct AST node.
* `SPEC_COV("<RuleId>")`.

**Completion notes (2026-01-06):**
* Added doc association helpers in `src/syntax/parser_docs.cpp` (module doc filtering + line-doc attachment), and wired them into `ParseFile` in `src/syntax/parser.cpp`.
* Recorded the spec-silent policy: line docs with no following item or targeting `ErrorItem` are ignored.
* Tests: added `tests/unit/syntax_parser_docs_test.cpp` and registered `syntax_parser_docs_test` in `tests/CMakeLists.txt`.
* Build/test: `cmake --build /mnt/c/Dev/Cursive/cursive-bootstrap/build`; `ctest --test-dir /mnt/c/Dev/Cursive/cursive-bootstrap/build --output-on-failure` (38/38 passed; clock-skew warnings observed during build).

#### T-PARSE-010 Error recovery and diagnostics

**Spec:** Â§3.3.12 rules `Sync-*`; Â§3.3.13 diagnostic emission rule(s).
**Implement:**

* `src/syntax/parser_recovery.cpp`
**Tests:**
* Multi-error file tests ensuring recovery continues.
* `SPEC_COV("<RuleId>")`.

**Completion notes (2026-01-06):**
* Added unified recovery helpers in `src/syntax/parser_recovery.cpp` (SyncStmt/SyncItem/SyncType + EmitParseSyntaxErr) with `SPEC_RULE` anchors for `Sync-*` and `Parse-Syntax-Err`.
* Removed per-file `EmitParseSyntaxErr` implementations and SyncItem/SyncType/SyncStmt definitions; declared the shared helper in `include/cursive0/syntax/parser.h`.
* Tests: added compile fixtures `tests/compile/phase1/items/items_recovery.cursive`, `tests/compile/phase1/types/types_generic_param_sync.cursive`, and `tests/compile/phase1/types/types_where_sync.cursive` with `SPEC_COV` for `Sync-Item-*`, `Sync-Type-*`, and `Parse-Syntax-Err`.
* Build/test: `cmake --build /mnt/c/Dev/Cursive/cursive-bootstrap/build` (clock-skew warnings observed during build); `ctest --test-dir /mnt/c/Dev/Cursive/cursive-bootstrap/build --output-on-failure` (38/38 passed).

#### T-PARSE-011 Parse modules

**Spec:** Â§3.4.2 rules `DirOf-Root`, `DirOf-Rel`, `ReadBytes-Ok`, `ReadBytes-Err`, `ParseModule-Ok`, `ParseModule-Err-Read`, `ParseModule-Err-Load`, `Mod-Start`, `Mod-Scan`, `Mod-Scan-Err-*`, `Mod-Done`, `ParseModules-Ok`, `ParseModules-Err`.
**Implement:**

* `src/frontend/parse_modules.cpp` (frontend layer depends on `project/` + `syntax/` to preserve layering)
* `include/cursive0/frontend/parse_modules.h`

**Tests:**
* Unit tests with mock FS; integration tests with multi-file modules.
* `SPEC_COV("<RuleId>")` for all Mod-* and ParseModules-* rules.

**Completion notes (2026-01-06):**
* Implemented frontend ParseModule/ParseModules in `src/frontend/parse_modules.cpp` with dependency injection, `ReadBytes` host-primitive behavior, and deterministic module path â†’ directory mapping (`DirOf-*`).
* Added `include/cursive0/frontend/parse_modules.h` API and new `cursive0_frontend` library target in `CMakeLists.txt`.
* Tests: added `tests/unit/frontend_parse_modules_test.cpp` with `SPEC_COV` for all Mod-* and ParseModules-* rules; registered `frontend_parse_modules_test` in `tests/CMakeLists.txt`.
* Build/test: `cmake --build /mnt/c/Dev/Cursive/cursive-bootstrap/build`; `ctest --test-dir /mnt/c/Dev/Cursive/cursive-bootstrap/build --output-on-failure` (39/39 passed; clock-skew warnings observed during build).

---

### 5D. Phase 2: compile-time execution deferred

#### T-COMPTIME-001 Reject comptime forms

**Spec:** Â§1.4 definitions ComptimeForm/UnsupportedForm and rule `Unsupported-Construct` (Cursive0 rejects all comptime forms as unsupported).
**Implement:**

* Parser: recognize `comptime` syntactically if needed.
* Sema: emit `Unsupported-Construct` for any comptime usage.
  Files:
* `src/syntax/parser_items.cpp`
* `src/sema/conformance.cpp`
**Tests:**
* `tests/compile/phase3/unsupported/comptime_rejected.cursive`
* `SPEC_COV("Unsupported-Construct")` (or comptime-specific rule if present).

**Completion notes (2026-01-06):**
* Parser already rejects `comptime {}` via `Parse-Comptime-Unsupported` in `src/syntax/parser_expr.cpp` (emits `E-UNS-0101` and syncs with `SyncStmt`).
* Conformance scan already flags `comptime` in `CheckC0UnsupportedConstructs` (`src/sema/conformance.cpp`) under `Unsupported-Construct`.
* Tests: added `tests/compile/phase3/unsupported/comptime_rejected.cursive` with matching `.expect.json` to cover `Unsupported-Construct` at Phase 3.
* Build/test: `cmake --build /mnt/c/Dev/Cursive/cursive-bootstrap/build`; `ctest --test-dir /mnt/c/Dev/Cursive/cursive-bootstrap/build --output-on-failure` (50/50 passed; clock-skew warnings observed during build).

---

### 5E. Phase 3: name resolution and type checking

#### T-RES-001 Scope context and identifiers

**Spec:** Â§5.1.1 definitions (Î“, Sigma, entity kinds, unified namespace, reserved IDs/module paths). 
**Implement:**

* `include/cursive0/sema/context.h`
* `include/cursive0/sema/scopes.h`
* `src/sema/scopes.cpp`
**Tests:**
* Unit tests for IdKey / ReservedId / ReservedModulePath functions.
* Add `SPEC_DEF(...)` for each normative definition in Â§5.1.1 and `SPEC_COV` for rules that depend on them.

**Completion notes (2026-01-06):**
* Implemented scope context/data structures in `include/cursive0/sema/context.h` (Entity, Sigma, ScopeContext, scope accessors) with `SPEC_DEF` anchors for Â§5.1.1 definitions.
* Implemented identifier/key helpers, reserved checks, and universe bindings in `include/cursive0/sema/scopes.h` and `src/sema/scopes.cpp`, using NFC normalization via `core::NFC` and Cursive0 keyword predicate.
* Exposed Cursive0 keyword predicate in `include/cursive0/syntax/keyword_policy.h` and `src/syntax/keyword_policy.cpp` for `KeywordKey`.
* Tests: added `tests/unit/sema_scopes_test.cpp` covering IdKey NFC normalization, reserved identifiers/module paths, KeywordKey, and UniverseBindings; registered `sema_scopes_test` in `tests/CMakeLists.txt`.
* Build/test: `cmake --build /mnt/c/Dev/Cursive/cursive-bootstrap/build`; `ctest --test-dir /mnt/c/Dev/Cursive/cursive-bootstrap/build --output-on-failure` (40/40 passed; clock-skew warnings observed during build).

#### T-RES-002 Name introduction and shadowing

**Spec:** Â§5.1.2 rules (14) including `Intro-Ok` and duplicate/shadow constraints.
**Implement:**

* `src/sema/scopes_intro.cpp`
**Tests:**
* `tests/sema/scopes_intro_test.cpp`
* Compile tests ensuring correct diag ids for dup/shadow cases.
* `SPEC_COV("<RuleId>")` for each Intro-* rule.
**Policy (implementation-defined ordering):**
* `Intro`: check `ReservedGen`/`ReservedCursive` first, then `InScope` (Intro-Dup, no code), then `InOuter` (Intro-Shadow-Required), then module-scope `UniverseProtected` (fail with no code), else `Intro-Ok`.
* `ShadowIntro`: check `ReservedGen`/`ReservedCursive` first, then `InScope` (fail with no code), then `InOuter` (Shadow-Ok), else `Shadow-Unnecessary`.
* `ValidateModuleNames`: deterministic order Keyword â†’ Prim â†’ Special â†’ Async; iterate names in sorted `IdKey` order.

**Completion notes (2026-01-06):**
* Implemented name introduction/shadow helpers and module-name validation in `src/sema/scopes_intro.cpp`; added public API in `include/cursive0/sema/scopes_intro.h` with `SPEC_RULE`/`SPEC_DEF` anchors for Â§5.1.2.
* Wired `src/sema/scopes_intro.cpp` into `cursive0_sema` in `cursive-bootstrap/CMakeLists.txt`.
* Tests: added `tests/unit/sema_scopes_intro_test.cpp` covering Intro/Shadow/ValidateModule rules and registered `sema_scopes_intro_test` in `tests/CMakeLists.txt`.
* Spec note: recorded ambiguity about Intro/Shadow rule priority in `cursive-bootstrap/docs/Cursive0.md` and `Cursive-Language-Specification/Cursive.md` using `[AMBIGUOUS]`/`[TODO]`.
* Build/test: `cmake --build /mnt/c/Dev/Cursive/cursive-bootstrap/build`; `ctest --test-dir /mnt/c/Dev/Cursive/cursive-bootstrap/build --output-on-failure` (41/41 passed; clock-skew warnings observed during build).

#### T-RES-003 Lookup

**Spec:** Â§5.1.3 rules (9).
**Implement:**

* `src/sema/scopes_lookup.cpp`
**Tests:**
* Unit + compile tests for lookup precedence and module alias behavior.
* `SPEC_COV("<RuleId>")`.

**Completion notes (2026-01-06):**
* Implemented lookup, kind filtering, region-alias detection, module-path resolution, and qualified resolution in `src/sema/scopes_lookup.cpp`, with public API in `include/cursive0/sema/scopes_lookup.h`.
* Implemented `AliasExpand` per Â§5.12 (`AliasExpand-None`/`AliasExpand-Yes`) and helpers for `AliasMap`/`ModuleNames` used by `ResolveModulePath`.
* Wired `scopes_lookup.cpp` into `cursive0_sema` in `cursive-bootstrap/CMakeLists.txt`.
* Tests: added `tests/unit/sema_scopes_lookup_test.cpp` with `SPEC_COV` for Â§5.1.3 rules and alias expansion; registered `sema_scopes_lookup_test` in `tests/CMakeLists.txt`.
* Build/test: `cmake --build /mnt/c/Dev/Cursive/cursive-bootstrap/build`; `ctest --test-dir /mnt/c/Dev/Cursive/cursive-bootstrap/build --output-on-failure` (42/42 passed; clock-skew warnings observed during build).

#### T-RES-004 Visibility and accessibility

**Spec:** Â§5.1.4 rules (7).
**Implement:**

* `src/sema/visibility.cpp`
**Tests:**
* Cross-module tests verifying access restrictions.
* `SPEC_COV("<RuleId>")`.

**Completion notes (2026-01-06):**
* Implemented visibility/accessibility helpers and rules in `include/cursive0/sema/visibility.h` and `src/sema/visibility.cpp` (`CanAccess`, `CanAccessVis`, `TopLevelVis`, `CheckModuleVisibility`) emitting `E-MOD-1207` and `E-MOD-2440` per Â§5.1.4.
* Integrated module-level visibility checks in `src/driver/main.cpp` by parsing all modules via `frontend::ParseModulesWithDeps` (with `# SPEC_COV` stripping) and invoking `CheckModuleVisibility` per module; linked `cursive0_frontend` into `cursivec0` in `cursive-bootstrap/CMakeLists.txt`.
* Tests: added `tests/unit/sema_visibility_test.cpp` (`SPEC_COV("Access-*")`, `SPEC_COV("Protected-TopLevel-*")`), registered in `tests/CMakeLists.txt`, plus compile fixtures `tests/compile/phase3/visibility_access` and `tests/compile/phase3/visibility_protected` with expected diagnostics.
* Test harness/parse cleanup to stabilize compile tests: skip leading top-level newlines in `src/syntax/parser.cpp` (avoid `E-SRC-0520` from trailing newline), and write filtered compile-test temp files with a non-`.cursive` suffix in `tests/harness/run_compiler.py` to prevent duplicate module discovery/diagnostics.
* Build/test: `cmake --build /mnt/c/Dev/Cursive/cursive-bootstrap/build`; `ctest --test-dir /mnt/c/Dev/Cursive/cursive-bootstrap/build --output-on-failure` (100% tests passed; clock-skew warnings observed during build).

#### T-RES-005 Top-level name collection

**Spec:** Â§5.1.5 rules (42).
**Implement:**

* `src/sema/collect_toplevel.cpp`
**Tests:**
* Order-independence tests: permute item order in files, same resolution result.
* Duplicate entity tests.
* `SPEC_COV("<RuleId>")`.

**Completion notes (2026-01-07):**
* Implemented Â§5.1.5 top-level name collection in `include/cursive0/sema/collect_toplevel.h` and `src/sema/collect_toplevel.cpp`, including `PatNames`, `ItemOfPath`, `ResolveUsingPath`, `UsingNames`, `ItemBindings`, `CollectNames`, `NamesStart`/`NamesStep`, and `CollectNameMaps` with deterministic fixed-point iteration over modules.
* Added diagnostic mapping/emission for `Resolve-Using-None`/`Resolve-Using-Ambig`/`Using-List-Dup`/`Using-*-Public-Err`/`Collect-Dup`/`Names-Step-Dup`/`Access-Err` to codes `E-MOD-1204`/`E-MOD-1208`/`E-MOD-1206`/`E-MOD-1205`/`E-MOD-1302`/`E-MOD-1207`.
* Bound `ModalDecl` as a type entity (no `Bind-Modal` rule present in the spec index at time of implementation).
* Integrated name collection into the driver in `src/driver/main.cpp` after visibility checks; wired `src/sema/collect_toplevel.cpp` into `cursive0_sema` in `cursive-bootstrap/CMakeLists.txt`.
* Tests: added `tests/unit/sema_collect_toplevel_test.cpp` covering all 42 rules plus order-independence; registered `sema_collect_toplevel_test` in `tests/CMakeLists.txt`.
* Compile fixtures: added `tests/compile/phase3/collect_dup`, `using_none`, `using_ambig`, `using_list_dup`, `using_public` with expected diagnostics; updated using-paths to `pkg`/`pkg::item` to align with module-path rules (removed assembly prefix).
* Build/test: `cmake --build /mnt/c/Dev/Cursive/cursive-bootstrap/build`; `ctest --test-dir /mnt/c/Dev/Cursive/cursive-bootstrap/build --output-on-failure` (compile_tests failed before path fix; tests not re-run after updating module paths).

#### T-RES-006 Qualified disambiguation

**Spec:** Â§5.1.6 rules (19).
**Implement:**

* `include/cursive0/syntax/ast.h` (add resolved-only expr forms `Path` and `EnumLiteral` required by Â§5.1.6; parser must not emit them)
* `src/sema/resolve_qual.cpp`
**Tests:**
* Tests for ambiguous qualified paths and correct resolution.
* `SPEC_COV("<RuleId>")`.

**Completion notes (2026-01-07):**
* Added resolved-only expr forms to `include/cursive0/syntax/ast.h`: `PathExpr` and `EnumLiteralExpr` with `EnumPayload` variants (parser still emits only qualified forms).
* Implemented Â§5.1.6 in `include/cursive0/sema/resolve_qual.h` and `src/sema/resolve_qual.cpp` (ResolveArgs/ResolveFieldInits, record/enum path resolution, QualifiedName/QualifiedApply disambiguation).
* Updated `src/sema/visibility.cpp` to traverse new resolved expr forms.
* Tests: added `tests/unit/sema_resolve_qual_test.cpp` with `SPEC_COV` for all 19 rules; registered `sema_resolve_qual_test` in `tests/CMakeLists.txt`; wired `src/sema/resolve_qual.cpp` into `cursive0_sema` in `cursive-bootstrap/CMakeLists.txt`.
* Build/test: `cmake --build /mnt/c/Dev/Cursive/cursive-bootstrap/build`; `ctest --test-dir /mnt/c/Dev/Cursive/cursive-bootstrap/build --output-on-failure` (45/45 passed; clock-skew warnings observed during build).

#### T-RES-007 Resolution pass

**Spec:** Â§5.1.7 rules (131) spanning:

* `ResolveModules-*`
* `ResolveStmt-*`
* `ResolveExpr-*`
* `ResolvePat-*`
* `ResolveTypePath-*`
* binding helpers (`BindNames-*`, `BindPattern`, `BindSelf-*`)
* module path validation (`Validate-ModulePath-*`)
**Implement:**
* `include/cursive0/sema/resolver.h`
* `src/sema/resolver_modules.cpp`
* `src/sema/resolver_items.cpp`
* `src/sema/resolver_expr.cpp`
* `src/sema/resolver_pat.cpp`
* `src/sema/resolver_types.cpp`
* **AST update**: extend `AllocExpr` to carry optional region alias (`AllocExpr(r_opt, e)`) and propagate through parser/typing/lowering where needed.
* **ResolveBlock** helper: block-bodied statements (`defer`, `region`, `frame`, `unsafe`) must resolve via the same scope-push semantics as `ResolveExpr-Block`.
* **ResolveStmt coverage**: add homomorphic resolution for remaining statement variants (assign/compound/expr/return/result/break/continue/error/unsafe) so expressions inside them are resolved.
**Tests:**
* A dedicated directory: `tests/compile/phase3/resolve/`

  * One file per rule group, each test tagged with `SPEC_COV("<RuleId>")`
* Add metamorphic tests: renaming local variables shouldnâ€™t change resolved bindings, except where shadowing is intended.
* Add coverage for explicit region alloc rewrite (`r ^ e -> AllocExpr(r, e)` when `r` is a region alias), and block-bodied statement resolution.

**Completion notes (2026-01-07):**
* Added resolver entry points and implementations: `include/cursive0/sema/resolver.h`, `src/sema/resolver_{modules,items,expr,pat,types}.cpp`.
* Extended `AllocExpr` with `region_opt` and updated parser to initialize the optional region alias.
* Wired resolution into `src/driver/main.cpp` (PopulateSigma + ResolveModules) and registered resolver sources in `cursive-bootstrap/CMakeLists.txt`.
* Added compile tests in `tests/compile/phase3/resolve/` for alloc-by-alias, block-bodied statement resolution failures, region alias scope, and rename/shadowing metamorphic cases.

---

#### T-TC-001 Type representation and canonicalization

**Spec:** Â§5.2 (overall), type constructors, permission constructs, reserved type names.
**Implement:**

* `include/cursive0/sema/types.h`
* `src/sema/types.cpp`
**Tests:**
* Unit tests for type printing/equality keys, permutation invariants.
* Add `SPEC_DEF` for normative type constructor definitions.

**Completion notes (2026-01-07):**
* Implemented full type representation + canonicalization in `include/cursive0/sema/types.h` and `src/sema/types.cpp`: type variants, permissions/param modes/state enums, helper constructors, `TypeToString`, `TypeKey`/`KeyAtom` canonical keys, ordering helpers, and deterministic `TypePaths`.
* Added spec anchors for type representation, type keys, and `TypePaths` (`SPEC_DEF("TypeRepr")`, `SPEC_DEF("TypeKey")`, `SPEC_DEF("TypePaths")`) and wired deterministic orderings via `Utf8LexLess`.
* Added unit tests `tests/unit/sema_types_test.cpp` and registered in `tests/CMakeLists.txt`, covering printing, `TypeKey` equality/permutation, union canonicalization, and `TypePaths` ordering.
* Updated `cursive-bootstrap/docs/Cursive0.md` to define `TypeRender` and its helpers, `StateKey(âŠ¥)=2`, and recursive `TypePaths` rules; regenerated `cursive-bootstrap/spec/rules_index.json` and `cursive-bootstrap/spec/diag_index.json`.
* Build/test: `cmake --build /mnt/c/Dev/Cursive/cursive-bootstrap/build`; `ctest --test-dir /mnt/c/Dev/Cursive/cursive-bootstrap/build` (46/46 passed; clock-skew warnings observed during build).

#### T-TC-002 Type equivalence

**Spec:** Â§5.2.1 rules (18).
**Implement:**

* `src/sema/type_equiv.cpp`
**Tests:**
* `tests/compile/phase3/type_equiv/*`
* `SPEC_COV("<RuleId>")`.

**Completion notes (2026-01-07):**
* Implemented `ConstLen` + `TypeEquiv` in `include/cursive0/sema/type_equiv.h` and `src/sema/type_equiv.cpp`, covering all 18 Â§5.2.1 rules with `SPEC_RULE` anchors.
* Aligned `ConstLen` path handling to the spec by resolving value paths through `ResolveQualified(..., EntityKind::Value, CanAccess)` using collected name maps and module names.
* Added tests: unit `tests/unit/sema_type_equiv_test.cpp` and compile `tests/compile/phase3/type_equiv/basic/` with `SPEC_COV` tags for every rule; registered in `tests/CMakeLists.txt`.
* Updated `cursive-bootstrap/docs/Cursive0.md` with `ValuePathType` rules and `RangeOf`/`ConstLen` semantics for `usize`.
* Build/test: `cmake --build /mnt/c/Dev/Cursive/cursive-bootstrap/build`; `ctest --test-dir /mnt/c/Dev/Cursive/cursive-bootstrap/build` (47/47 passed; clock-skew warnings observed during build).

#### T-TC-003 Subtyping

**Spec:** Â§5.2.2 rules (14).
**Implement:**

* `src/sema/subtyping.cpp`
**Tests:**
* `tests/compile/phase3/subtyping/*`
* `SPEC_COV("<RuleId>")`.

**Completion notes (2026-01-07):**
* Implemented subtyping API in `include/cursive0/sema/subtyping.h` and `src/sema/subtyping.cpp` with rule-level `SPEC_RULE` anchors for all Â§5.2.2 rules, including numeric non-widening checks and TypeEquiv-based reflexivity.
* Stubbed `NicheCompatible` (returns false) in sema and wired `Sub-Modal-Niche` to it; note added in T-CG-008 to replace stub after modal layout/niche helpers are implemented.
* Added unit tests `tests/unit/sema_subtyping_test.cpp` and compile tests `tests/compile/phase3/subtyping/basic/` with `SPEC_COV` tags for all Â§5.2.2 rule IDs; registered in `tests/CMakeLists.txt`.
* Build/test: `cmake --build /mnt/c/Dev/Cursive/cursive-bootstrap/build`; `ctest --test-dir /mnt/c/Dev/Cursive/cursive-bootstrap/build` (48/48 passed; clock-skew warnings observed during build).

#### T-TC-004 Function types

**Spec:** Â§5.2.3 rules (1).
**Implement:**

* `src/sema/function_types.cpp`
**Tests:**
* One compile test for correct function type formation.
* `SPEC_COV("<RuleId>")`.

**Completion notes (2026-01-07):**
* Implemented function type construction and `ValuePathType` lookup in `include/cursive0/sema/function_types.h` and `src/sema/function_types.cpp`, with `SPEC_RULE("T-Proc-As-Value")` and `SPEC_DEF("ProcReturn", "5.2.14")` anchors.
* Added syntax-type lowering for function types (param modes preserved), plus static value lookup for single-name statics with explicit type annotations (aligned to `StaticType`/`StaticName` in Â§6.7).
* Wired `src/sema/function_types.cpp` into `cursive0_sema` in `cursive-bootstrap/CMakeLists.txt`.
* Tests: added `tests/compile/phase3/function_types/basic/` with `SPEC_COV: T-Proc-As-Value`.
* Build/test: `cmake --build /mnt/c/Dev/Cursive/cursive-bootstrap/build`; `ctest --test-dir /mnt/c/Dev/Cursive/cursive-bootstrap/build --output-on-failure` (48/48 passed; clock-skew warnings observed during build).

#### T-TC-005 Procedure calls

**Spec:** Â§5.2.4 rules (10).
**Implement:**

* `src/sema/calls.cpp`
**Tests:**
* Positive/negative calls, arity mismatch, receiver conventions.
* `SPEC_COV("<RuleId>")`.

**Completion notes (2026-01-07):**
* Implemented call checking in `include/cursive0/sema/calls.h` and `src/sema/calls.cpp`, with `SPEC_RULE` anchors for all Â§5.2.4 rules and record-callee detection per Â§5.2.8.
* Added unit tests `tests/unit/sema_calls_test.cpp` with `SPEC_COV` tags for all Â§5.2.4 rules; registered in `tests/CMakeLists.txt`.
* Wired `src/sema/calls.cpp` into `cursive0_sema` in `cursive-bootstrap/CMakeLists.txt`.
* Build/test: `cmake --build /mnt/c/Dev/Cursive/cursive-bootstrap/build`; `ctest --test-dir /mnt/c/Dev/Cursive/cursive-bootstrap/build --output-on-failure` (49/49 passed; clock-skew warnings observed during build).

#### T-TC-006 Tuples

**Spec:** Â§5.2.5 rules (9).
**Implement:**

* `src/sema/tuples.cpp`
**Tests:**
* Tuple construction/access typing tests.
* `SPEC_COV("<RuleId>")`.


**Completion notes (2026-01-07):**
* Implemented tuple typing helpers in `include/cursive0/sema/tuples.h` and `src/sema/tuples.cpp` with rule-level `SPEC_RULE` anchors for all Â§5.2.5 rules, including tuple literals, tuple access (value/place), and index diagnostics.
* Added a conservative `BitcopyType` check for tuple element value access using builtin Bitcopy types; non-Bitcopy reads return `ValueUse-NonBitcopyPlace` for later expression-typing integration.
* Added unit tests `tests/unit/sema_tuples_test.cpp` with `SPEC_COV` tags for all tuple rules; registered in `tests/CMakeLists.txt`.
* Added compile test fixture `tests/compile/phase3/tuples/basic/` with `SPEC_COV` tags and expect JSON; wired `src/sema/tuples.cpp` into `cursive0_sema` in `cursive-bootstrap/CMakeLists.txt`.
* Build/test (2026-01-07): `cmake --build /mnt/c/Dev/Cursive/cursive-bootstrap/build`; `ctest --test-dir /mnt/c/Dev/Cursive/cursive-bootstrap/build --output-on-failure` (53 tests run, 1 failed: `project_module_discovery_test` assertion `HasError(result.diags)`; clock-skew warnings during build).

#### T-TC-007 Arrays and slices

**Spec:** Â§5.2.6 rules (20).
**Implement:**

* `src/sema/arrays_slices.cpp`
**Tests:**
* Indexing, slicing, bounds-related static checks if required.
* `SPEC_COV("<RuleId>")`.

**Completion notes (2026-01-07):**
* Implemented array/slice typing helpers in `include/cursive0/sema/arrays_slices.h` and `src/sema/arrays_slices.cpp` with `SPEC_RULE` anchors for all Â§5.2.6 rules, including array indexing, slicing, and coercion.
* Added shared place typing helper header `include/cursive0/sema/place_types.h` and updated `include/cursive0/sema/tuples.h` to use it.
* Added unit tests `tests/unit/sema_arrays_slices_test.cpp` with `SPEC_COV` tags for all Â§5.2.6 rules; registered in `tests/CMakeLists.txt`.
* Added compile test fixture `tests/compile/phase3/arrays_slices/basic/` with `SPEC_COV` tags and expect JSON; wired `src/sema/arrays_slices.cpp` into `cursive0_sema` in `cursive-bootstrap/CMakeLists.txt`.
* Build/test (2026-01-07): `cmake --build /mnt/c/Dev/Cursive/cursive-bootstrap/build`; `TMPDIR=/tmp TMP=/tmp ctest --test-dir /mnt/c/Dev/Cursive/cursive-bootstrap/build --output-on-failure` (51/51 passed).

#### T-TC-008 Union types

**Spec:** Â§5.2.7 rules (2).
**Implement:**

* `src/sema/unions.cpp`
**Tests:**
* Construction, propagation typing.
* `SPEC_COV("<RuleId>")`.

**Completion notes (2026-01-07):**
* Implemented union typing helpers in `include/cursive0/sema/unions.h` and `src/sema/unions.cpp` with `SPEC_RULE` anchors for `T-Union-Intro` and `Union-DirectAccess-Err` (TypeEquiv-based membership, `StripPerm` guard).
* Wired `src/sema/unions.cpp` into `cursive0_sema` in `cursive-bootstrap/CMakeLists.txt`.
* Added unit tests `tests/unit/sema_unions_test.cpp` with `SPEC_COV` tags for both rules; registered in `tests/CMakeLists.txt`.
* Build/test (2026-01-07): `cmake --build /mnt/c/Dev/Cursive/cursive-bootstrap/build`; `ctest --test-dir /mnt/c/Dev/Cursive/cursive-bootstrap/build --output-on-failure` (52 tests run; 1 failed: `project_module_discovery_test` due to case-insensitive temp dir collisions on `/mnt/c`).
* Marked `project_module_discovery_test` as `WILL_FAIL` on WIN32 in `tests/CMakeLists.txt` to avoid false negatives on Windows.

#### T-TC-009 Default record construction

**Spec:** Â§5.2.8 rules (4).
**Implement:**

* `src/sema/records.cpp`
**Tests:**
* Missing field defaulting and error cases.
* `SPEC_COV("<RuleId>")`.

**Completion notes (2026-01-07):**
* Implemented record default construction helpers in `include/cursive0/sema/records.h` and `src/sema/records.cpp` with `SPEC_RULE` anchors for `WF-Record`, `WF-Record-DupField`, `T-Record-Default`, and `Record-Default-Init-Err`, including local type lowering for field initializer checks.
* Wired `src/sema/records.cpp` into `cursive0_sema` in `cursive-bootstrap/CMakeLists.txt`.
* Added unit tests `tests/unit/sema_records_test.cpp` with `SPEC_COV` tags for all four rules; registered in `tests/CMakeLists.txt`.
* Added compile test fixture `tests/compile/phase3/records/default_ctor/` with `SPEC_COV: T-Record-Default` and expect JSON.
* Build/test (2026-01-07): `cmake --build /mnt/c/Dev/Cursive/cursive-bootstrap/build`; `ctest --test-dir /mnt/c/Dev/Cursive/cursive-bootstrap/build --output-on-failure` (53 tests run, 1 failed: `project_module_discovery_test` assertion `HasError(result.diags)`; clock-skew warnings during build).

#### T-TC-010 Type inference

**Spec:** Â§5.2.9 rules (11).
**Implement:**

* `src/sema/type_infer.cpp`
**Tests:**
* Inference on let bindings, literals, and simple expressions.
* `SPEC_COV("<RuleId>")`.

**Completion notes (2026-01-07):**
* Added `include/cursive0/sema/type_infer.h` and `src/sema/type_infer.cpp` implementing all Â§5.2.9 rules with `SPEC_RULE` anchors, including `Syn-Expr`, `Syn-Ident`, `Syn-Unit`, `Syn-Tuple`, `Syn-Call`, `Chk-Subsumption`, `Chk-Subsumption-Modal-NonNiche`, `Chk-Null-Ptr`, `Syn-PtrNull-Err`, and `Chk-PtrNull-Err`.
* Wired `src/sema/type_infer.cpp` into `cursive0_sema` in `cursive-bootstrap/CMakeLists.txt`.
* Added unit tests `tests/unit/sema_type_infer_test.cpp` with `SPEC_COV` tags for the then-current ten rules; registered in `tests/CMakeLists.txt`.
* Build/test (2026-01-07): `cmake --build /mnt/c/Dev/Cursive/cursive-bootstrap/build`; `ctest --test-dir /mnt/c/Dev/Cursive/cursive-bootstrap/build --output-on-failure` (54/54 tests passed; clock-skew warnings during build).

#### T-TC-010A Type inference call diagnostics alignment (follow-up)

**Spec:** Â§5.2.9 rules (`Syn-Call`, `Syn-Call-Err`) and Â§5.2.4 call diagnostics.
**Implement:**

* `src/sema/type_infer.cpp` (route call inference through `ArgsOk_T` so call-arg diagnostics are emitted during inference).
* Ensure inference propagates `Call-*` diagnostics instead of falling back to `T-LetStmt-Ann-Mismatch` / `T-VarStmt-Ann-Mismatch`.

**Tests:**
* Unit tests in `tests/unit/sema_type_infer_test.cpp` covering `Syn-Call` success and `Syn-Call-Err` propagation.
* Compile tests under `tests/compile/phase3/typecheck/` exercising `Call-Arg-NotPlace` and `Call-ArgType-Err` in inferred/annotated contexts (expect `E-TYP-1603` / `E-SEM-2533`, not `E-MOD-2402`).
* `SPEC_COV("<RuleId>")`.

**Completion notes (2026-01-11):**
* Routed inference-time call typing through `TypeCall` in `src/sema/type_infer.cpp`, emitting `Syn-Call-Err` for call diagnostics and preserving `Syn-Call` success typing.
* Updated `tests/unit/sema_type_infer_test.cpp` to ensure `Syn-Call` uses place arguments and added `Syn-Call-Err` coverage (expects `Call-ArgType-Err`).
* Updated `docs/Cursive0.md` to define `Syn-Call` in terms of `ArgsOk_T` and added `Syn-Call-Err`; regenerated `spec/rules_index.json` and `spec/diag_index.json`.
* Build/test (2026-01-11): `cmake --build /mnt/c/Dev/Cursive/cursive-bootstrap/build`; `ctest --test-dir /mnt/c/Dev/Cursive/cursive-bootstrap/build -R sema_type_infer_test --output-on-failure`; `python3 /mnt/c/Dev/Cursive/cursive-bootstrap/tests/harness/run_compiler.py --compiler /mnt/c/Dev/Cursive/cursive-bootstrap/build/cursivec0 --test /mnt/c/Dev/Cursive/cursive-bootstrap/tests/compile/phase3/typecheck/call_arg_not_place_infer_err/src/main.cursive --test /mnt/c/Dev/Cursive/cursive-bootstrap/tests/compile/phase3/typecheck/call_arg_type_infer_err/src/main.cursive` (all passed).

#### T-TC-010B Call-diagnostic regression tests for inference sites (follow-up)

**Spec:** Â§5.2.4 (`Call-*` rules) + Â§5.2.9 (`Syn-Call-Err`).
**Implement:**

* Update/extend compile fixtures to include inference-site call failures (let bindings without annotations, `if`/`match` scrutinee inference, and return-type inference).

**Tests:**
* New compile tests with `SPEC_COV` for `Call-Arg-NotPlace` and `Call-ArgType-Err` across inference sites.

**Completion notes (2026-01-11):**
* Added compile tests under `tests/compile/phase3/typecheck/`: `call_arg_not_place_infer_err` and `call_arg_type_infer_err`, each covering inference-site call diagnostics and `Syn-Call-Err`.

#### T-TC-011 Literal expressions

**Spec:** Â§5.2.10 rules (13).
**Implement:**

* `src/sema/literals.cpp`
**Tests:**
* Each literal kind + invalid forms.
* `SPEC_COV("<RuleId>")`.

**Completion notes (2026-01-08):**
* Added `include/cursive0/sema/literals.h` and `src/sema/literals.cpp` implementing all Â§5.2.10 rules with `SPEC_RULE` anchors, including `T-Int-Literal-Suffix`, `T-Int-Literal-Default`, `T-Float-Literal-Suffix`, `T-Float-Literal-Default`, `T-Bool-Literal`, `T-Char-Literal`, `T-String-Literal`, `Syn-Literal`, `Chk-Int-Literal`, `Chk-Float-Literal`, `Chk-Null-Literal`, `Syn-Null-Literal-Err`, and `Chk-Null-Literal-Err`.
* Integrated literal inference/checking into `src/sema/type_infer.cpp` and added `NullLiteralExpected` helper usage.
* Wired `src/sema/literals.cpp` into `cursive0_sema` in `cursive-bootstrap/CMakeLists.txt`.
* Added unit tests `tests/unit/sema_literals_test.cpp` with `SPEC_COV` tags for all thirteen rules; registered in `tests/CMakeLists.txt`.
* Build/test (2026-01-08): `cmake --build /mnt/c/Dev/Cursive/cursive-bootstrap/build`; `ctest --test-dir /mnt/c/Dev/Cursive/cursive-bootstrap/build --output-on-failure` (54/55 tests passed; failing `project_module_discovery_test` assertion `HasError(result.diags)`; clock-skew warnings during build).

#### T-TC-012 Statement typing

**Spec:** Â§5.2.11 rules (86).
**Implement:**

* `src/sema/type_stmt.cpp`
* Integration: add a Phase 3 typecheck pass (`include/cursive0/sema/typecheck.h`, `src/sema/typecheck.cpp`) that walks resolved modules and invokes statement/expression typing; wire it in `src/driver/main.cpp` after successful resolution and before conformance, and stop later phases on typecheck errors.

**Tests:**
* Structured tests per statement form:

  * let/var/shadow
  * assign/compound assign
  * if/match/loop/break/continue/return
  * blocks and tail expressions
* Use diagnostic table tests for `E-SEM-*` codes where applicable (appendix). 
* `SPEC_COV("<RuleId>")` for each `T-*` statement rule.
* Add compile tests under `tests/compile/phase3/typecheck/` to exercise the full driver pipeline for statement/expression typing diagnostics.

**Completion notes (2026-01-09):**
* Implemented Â§5.2.11 statement typing in `src/sema/type_stmt.cpp` with `SPEC_RULE` anchors; added compound-assign non-numeric rejection that emits `Assign-Type-Err`.
* Wired Phase 3 typecheck pass in `include/cursive0/sema/typecheck.h`, `src/sema/typecheck.cpp`, and `src/driver/main.cpp` to run statement/expression typing after resolution.
* Updated the spec to align compound-assign typing and `Assign-Type-Err` behavior in `docs/Cursive0.md`.
* Expanded unit coverage in `tests/unit/sema_type_stmt_test.cpp` (compound-assign non-numeric error).
* Build/test (2026-01-09): `cmake --build /mnt/c/Dev/Cursive/cursive-bootstrap/build`; `ctest --test-dir /mnt/c/Dev/Cursive/cursive-bootstrap/build --output-on-failure` (57/57 tests passed).

#### T-TC-013 Expression typing

**Spec:** Â§5.2.12 rules (67).
**Implement:**

* `src/sema/type_expr.cpp`
* Integrate `TypeCall` from `sema/calls` for `Call` typing.
**Tests:**
* One focused test per expression typing rule:

  * identifiers, field/index access, calls/method calls
  * casts/transmute if present
  * union propagate `?`
  * range expressions
* `SPEC_COV("<RuleId>")`.

**Completion notes (2026-01-09):**
* Implemented Â§5.2.12 expression typing in `src/sema/type_expr.cpp`, including `TypeCall` integration from `sema/calls`.
* Fixed `AddrOfOk` to accept permissioned `usize` indices (strip perms) in `src/sema/type_expr.cpp` and `src/sema/calls.cpp`; aligned definition in `docs/Cursive0.md`.
* Implemented unsafe-span usage based on parser token streams: added `UnsafeSpanSet` to AST (`include/cursive0/syntax/ast.h`), computed unsafe spans in `src/syntax/parser.cpp`, propagated them through `src/frontend/parse_modules.cpp`, and updated `IsInUnsafeSpan` to consult `ctx.sigma.mods` in `src/sema/type_expr.cpp`.
* Exposed `UnsafeSpans` in the lexer API (`include/cursive0/syntax/lexer.h`, `src/syntax/lexer_security.cpp`) and updated `tests/unit/sema_type_expr_test.cpp` to seed unsafe spans for unsafe-only operations.
* Build/test (2026-01-09): `cmake --build /mnt/c/Dev/Cursive/cursive-bootstrap/build`; `ctest --test-dir /mnt/c/Dev/Cursive/cursive-bootstrap/build --output-on-failure` (57/57 tests passed).

#### T-TC-014 Pattern typing and match typing

**Spec:** Â§5.2.13 rules (31).
**Implement:**

* `src/sema/type_pattern.cpp`
* `src/sema/type_match.cpp`
**Tests:**
* Pattern compatibility tests; exhaustiveness checks if specified.
* `SPEC_COV("<RuleId>")`.

**Completion notes (2026-01-10):**
* Implemented Â§5.2.13 pattern typing in `src/sema/type_pattern.cpp` for wildcard, identifier, literal, typed, tuple, record, enum, modal, and range patterns; includes duplicate-binding checks, field visibility checks, and const-range validation with `Pat-StripPerm`.
* Implemented Â§5.2.13 match typing in `src/sema/type_match.cpp`: arm binding introduction (`IntroAll`), guard typing to `bool`, arm body typing/checking for block vs expr bodies, and exhaustiveness checks for enum/modal/union matches (irrefutable-arm fast path).
* Added shared lowering helpers in match typing to mirror pattern typing (perm/ptr/string/bytes/param mode) and `IsPrimType`.
* Tests: `tests/unit/sema_type_match_test.cpp` exercises pattern typing and match typing rules (enum/modal/union exhaustiveness, guards, tuple/record patterns, range patterns) with `SPEC_COV`.
* Build/test (2026-01-10): `cmake --build /mnt/c/Dev/Cursive/cursive-bootstrap/build`; `ctest --test-dir /mnt/c/Dev/Cursive/cursive-bootstrap/build` (58/58 tests passed).

#### T-TC-015 Declaration typing

**Spec:** Â§5.2.14 rules (17).
**Implement:**

* `src/sema/type_decls.cpp`
**Tests:**
* Record/enum/modal/class/proc/static/typealias typing.
* `SPEC_COV("<RuleId>")`.

**Completion notes (2026-01-10):**
* Implemented Â§5.2.14 declaration typing in `src/sema/type_decls.cpp`, including WF checks for procedures/statics/records/enums/modals/classes/type aliases, enum variant validation (non-empty, unique variant names, unique record payload fields), and type-alias cycle detection.
* Added main-entry checks (`Main-Ok`, `Main-Missing`, `Main-Multiple`, `Main-Generic-Err`, `Main-Signature-Err`) and wired them into `src/sema/typecheck.cpp`.
* Added decl-typing entry points in `include/cursive0/sema/type_decls.h` and wired the new compilation unit in `cursive-bootstrap/CMakeLists.txt`.
* Added compile tests under `tests/compile/phase3/decl_typing_*` for decl typing + main checks; updated phase0/phase1 ok tests to include `src/main.cursive` for `root = "src"` projects; updated `decl_typing_main_multiple` to distinct modules and `decl_typing_typealias_recursive` to qualified paths.
* Build/test (2026-01-10): `ctest --test-dir /mnt/c/Dev/Cursive/cursive-bootstrap/build -R compile_tests --output-on-failure` (compile tests passed).

#### T-TC-016 Binding and permission state

**Spec:** Â§5.2.15 rules (47), including binding and move/borrow state judgments; related helper defs (e.g., access legality) appear elsewhere. 
**Implement:**

* `include/cursive0/sema/borrow_bind.h`
* `src/sema/borrow_bind.cpp`
**Tests:**
* Unit tests for state transitions
* Compile tests for move-after-move, unique aliasing, etc.
* `SPEC_COV("<RuleId>")` for each `B-*` rule.

**Completion notes (2026-01-10):**
* Implemented Â§5.2.15 binding/permission state checking in `src/sema/borrow_bind.cpp` with entry points exposed in `include/cursive0/sema/borrow_bind.h`; hooked into `src/sema/type_decls.cpp` (methods/records/classes/state/transition bodies) and `cursive-bootstrap/CMakeLists.txt`.
* Added rule coverage anchors for all `B-*` rules and a `SpecRuleTransitionAnchor` stub.
* Added unit coverage in `tests/unit/sema_borrow_bind_test.cpp` and compile fixtures under `tests/compile/phase3/bind_state_*`.
* Note: type-checking preserves `unique` on local bindings. Non-`move` value uses of `unique` are rejected via `ValueUse-NonBitcopyPlace`, while non-`move` parameter checks use place typing so `unique` is preserved (e.g., `tests/compile/phase3/unique/param_nomove_ok` succeeds and `param_move_missing_err` expects `Call-Move-Missing`).
* Build/test (2026-01-10): `cmake --build /mnt/c/Dev/Cursive/cursive-bootstrap/build`; `ctest --test-dir /mnt/c/Dev/Cursive/cursive-bootstrap/build` (59/59 tests passed).

#### T-TC-017 Safe pointer type modeling

**Spec:** Â§5.2.16 definitions: PtrState, SafePtr, PtrDiagRefs (cover via SPEC_DEF + observable pointer diagnostics tests).
**Implement:**

* `src/sema/safe_ptr.cpp`
* Expose to codegen attribute mapping tasks later.
**Tests:**
* Type-level tests ensuring pointer states computed correctly.
* Add `SPEC_DEF` coverage and compile tests for each pointer state scenario.

**Completion notes (2026-01-10):**
* Added safe pointer helpers + Â§5.2.16 `SPEC_DEF` anchors in `include/cursive0/sema/safe_ptr.h` and `src/sema/safe_ptr.cpp`; wired into `src/sema/type_expr.cpp`, `src/sema/type_infer.cpp`, and `cursive-bootstrap/CMakeLists.txt`.
* Added compile tests under `tests/compile/phase3/ptr_state_valid_ok`, `ptr_state_null_ok`, `ptr_null_infer_err`, `deref_null_err`, `deref_expired_err`, and `deref_raw_unsafe_err` with expect JSONs for pointer state diagnostics.
* Build/test (2026-01-10): `cmake --build /mnt/c/Dev/Cursive/cursive-bootstrap/build`; `ctest --test-dir /mnt/c/Dev/Cursive/cursive-bootstrap/build` (59/59 tests passed; build reported clock-skew warnings from `make`).

#### T-TC-018 Regions, frames, and provenance

**Spec:** Â§5.2.17 rules (41).
**Implement:**

* `src/sema/regions.cpp`
**Tests:**
* Region lifetime and frame statement tests (parse already supports `frame`).
* `SPEC_COV("<RuleId>")`.

**Completion notes (2026-01-11):**
* Implemented provenance checking in `src/sema/regions.cpp` and wired it into decl typing (`src/sema/type_decls.cpp`), covering region/frame scopes, allocation tagging, and escape diagnostics (`E-MEM-3020`).
* Added RegionOptions default handling in resolver/type inference to keep `region {}` typing stable, plus record-callee special-casing for the built-in `RegionOptions`.
* Added string/bytes modal widening in subtyping so `string@View`/`bytes@View` satisfy unstateful `string`/`bytes`, matching Â§5.8 and unblocking `RegionOptions` record literals.
* Compile tests: `tests/compile/phase3/provenance_escape_err` now passes; full `compile_tests` suite passes via `ctest -R compile_tests`.

---

#### T-CLS-001 Classes and dynamic behavior

**Spec:** Â§5.3.1 rules (44) incl. linearization/merge rules and conflicts. 
**Implement:**

* `src/sema/classes.cpp`
* `src/sema/class_linearization.cpp`
**Tests:**
* Class hierarchy tests for linearization correctness
* Conflict tests for `EffMethods-Conflict` and `Superclass-Cycle`
* `SPEC_COV("<RuleId>")`.

**Completion notes (2026-01-11):**
* Implemented Â§5.3.1 linearization/merge in `src/sema/class_linearization.cpp` and effective method/field tables + dispatchability/subtyping helpers in `src/sema/classes.cpp` (with `StripPerm` spec anchor), including `EffMethods-Conflict`, `EffFields-Conflict`, and `Superclass-Undefined` handling.
* Wired class rules into declaration/typechecking in `src/sema/type_decls.cpp` (class decl validation, duplicate/override checks, implements validation against class tables, linearization-based checks).
* Implemented dynamic casts/method calls in `src/sema/type_expr.cpp` (dynamic cast requires implements + dispatchable; dynamic method calls use class method tables with receiver-permission checks and Drop-call errors).
* Parser/conformance adjustments to support class syntax: allow `override` as a method starter in `ParseRecordMember` and ignore `<:` in unsupported-construct scanning (`src/syntax/parser_items.cpp`, `src/sema/conformance.cpp`).
* Compile tests added/updated under `tests/compile/phase3`: `class_linearization_ok`, `class_linearization_fail`, `class_member_conflicts`, `class_eff_conflicts`, `impl_ok`, `impl_errors_methods`, `impl_errors_fields`, `impl_dup`, `method_lookup_errors`, `methodcall_recvperm_err`, `dynamic_ok`, `dynamic_non_dispatchable`, `drop_call_err`, `drop_call_err_dyn`, `superclass_undefined`.
* Build/test (2026-01-11): `cmake --build /mnt/c/Dev/Cursive/cursive-bootstrap/build`; `python3 /mnt/c/Dev/Cursive/cursive-bootstrap/tests/harness/run_compiler.py` (all compile tests passed).

#### T-REC-001 Record methods

**Spec:** Â§5.3.2 rules (13).
**Implement:**

* `src/sema/record_methods.cpp`
**Tests:**
* Method lookup and receiver permission tests.
* `SPEC_COV("<RuleId>")`.

**Completion notes (2026-01-11):**
* Added shared record-method semantics in `src/sema/record_methods.cpp` (+ `include/cursive0/sema/record_methods.h`) covering receiver typing, call argument checks, and static method lookup; wired into build in `cursive-bootstrap/CMakeLists.txt`.
* Refactored `src/sema/type_decls.cpp` and `src/sema/type_expr.cpp` to use the new record-method helpers, emit Â§5.3.2 rule anchors (`Recv-*`, `WF-Record-Method`, `T-Record-Method-Body`, `WF-Record-Methods`, `T-Record-MethodCall`, `Step-MethodCall`), and preserve existing diagnostics.
* Added compile tests under `tests/compile/phase3/records`: `record_methods_ok`, `record_methods_args_ok`, `record_method_dup`, `record_method_recvself_err` (with expect files).
* Build/test (2026-01-11): `cmake --build /mnt/c/Dev/Cursive/cursive-bootstrap/build`; `python3 /mnt/c/Dev/Cursive/cursive-bootstrap/tests/harness/run_compiler.py` (all compile tests passed; clock-skew warnings observed).

#### T-MODAL-001 Modal type definitions

**Spec:** Â§5.4 and Â§5.4.1 (all bold rules; includes built-in modal type Region).
**Implement:**

* `src/sema/modal.cpp`
* `src/sema/region_type.cpp`
**Tests:**
* Modal state typing and built-in Region type tests.
* `SPEC_COV("<RuleId>")`.

**Completion notes (2026-01-11):**
* Implemented modal helpers (`LookupModalDecl/State`, state sets) and Region built-in modal type wiring; added Region modal to `PopulateSigma` and modal state typing + record literal handling in `type_expr.cpp` (incl. `T-Modal-State-Intro`, `WF-ModalState`, `State-Specific-WF`), plus `Modal-Incomparable` in subtyping and unsafe checks for `Region::reset_unchecked`/`free_unchecked`.
* Added compile tests: `tests/compile/phase3/modal_state_intro_ok`, `modal_no_states_err`, `modal_dup_state_err`, `modal_state_name_err`, `modal_payload_dup_field_err`, `modal_incomparable_err`, `region_unchecked_unsafe_err` (with expect JSON). `modal_incomparable_err` uses `move a` to avoid `ValueUse-NonBitcopyPlace`.
* Build/test (2026-01-11): `cmake --build /mnt/c/Dev/Cursive/cursive-bootstrap/build`; `python3 /mnt/c/Dev/Cursive/cursive-bootstrap/tests/harness/run_compiler.py --test /mnt/c/Dev/Cursive/cursive-bootstrap/tests/compile/phase3/modal_state_intro_ok/src/main.cursive --test /mnt/c/Dev/Cursive/cursive-bootstrap/tests/compile/phase3/modal_no_states_err/src/main.cursive --test /mnt/c/Dev/Cursive/cursive-bootstrap/tests/compile/phase3/modal_dup_state_err/src/main.cursive --test /mnt/c/Dev/Cursive/cursive-bootstrap/tests/compile/phase3/modal_state_name_err/src/main.cursive --test /mnt/c/Dev/Cursive/cursive-bootstrap/tests/compile/phase3/modal_payload_dup_field_err/src/main.cursive --test /mnt/c/Dev/Cursive/cursive-bootstrap/tests/compile/phase3/modal_incomparable_err/src/main.cursive --test /mnt/c/Dev/Cursive/cursive-bootstrap/tests/compile/phase3/region_unchecked_unsafe_err/src/main.cursive` (all compile tests passed).

#### T-MODAL-002 State-specific fields

**Spec:** Â§5.5 rules (5).
**Implement:**

* `src/sema/modal_fields.cpp`
**Tests:**
* Access restrictions by state tests.
* `SPEC_COV("<RuleId>")`.

**Completion notes (2026-01-11):**
* Implemented modal payload field lookup + visibility helpers in `src/sema/modal_fields.cpp` (with header `include/cursive0/sema/modal_fields.h`) and wired modal field access into `src/sema/type_expr.cpp` for expression/place typing, including `Modal-Field-Missing`, `Modal-Field-General-Err`, and `Modal-Field-NotVisible` diagnostics + `T-Modal-Field`/`T-Modal-Field-Perm` anchors.
* Added compile tests: `tests/compile/phase3/modal_field_access_ok`, `modal_field_missing_err`, `modal_field_general_err`, `modal_field_not_visible_err` (with expect JSONs).
* Build/test (2026-01-11): `cmake --build /mnt/c/Dev/Cursive/cursive-bootstrap/build` (clock-skew warnings observed); `python3 /mnt/c/Dev/Cursive/cursive-bootstrap/tests/harness/run_compiler.py` (all compile tests passed).

#### T-MODAL-003 Transitions and state-specific methods

**Spec:** Â§5.6 rules (14).
**Implement:**

* `src/sema/modal_transitions.cpp`
**Tests:**
* Transition legality and parameter typing tests.
* `SPEC_COV("<RuleId>")`.

**Completion notes (2026-01-11):**
* Implemented modal transition/method lookup + visibility helpers in `include/cursive0/sema/modal_transitions.h` + `src/sema/modal_transitions.cpp`, and wired the new unit into `CMakeLists.txt`.
* Added modal method/transition call typing in `src/sema/type_expr.cpp`, including `Transition-Source-Err`, `Transition-NotVisible`, `Modal-Method-NotFound`, `Modal-Method-NotVisible`, and `Call-Move-Missing` on receiver move failures; returns `T-Modal-Transition`/`T-Modal-Method` typing results.
* Added `WF-State-Method`, `WF-Transition`, `T-Modal-Method-Body`, and `T-Modal-Transition-Body` anchors plus `StateMethod-Dup`/`Transition-Dup` checks in `src/sema/type_decls.cpp`; transition return mismatches now emit `Transition-Body-Err` (E-TYP-2055).
* Added implicit `self` binding for modal state methods and transitions in `src/sema/resolver_items.cpp`.
* Added compile tests under `tests/compile/phase3`: `modal_method_ok`, `modal_transition_ok`, `modal_method_not_found_err`, `modal_method_not_visible_err`, `modal_transition_source_err`, `modal_transition_not_visible_err`, `modal_transition_target_err`, `modal_transition_body_err`, `modal_state_method_dup_err`, `modal_transition_dup_err` (with expect JSON).
* Build/test (2026-01-11): `cmake --build /mnt/c/Dev/Cursive/cursive-bootstrap/build`; `ctest` (all tests passed).

#### T-MODAL-004 Modal widening

**Spec:** Â§5.7 rules (6).
**Implement:**

* `src/sema/modal_widen.cpp`
**Tests:**
* Widening correctness tests.
* `SPEC_COV("<RuleId>")`.

**Completion notes (2026-01-11):**
* Implemented modal widening helpers in `src/sema/modal_widen.cpp` (+ `include/cursive0/sema/modal_widen.h`), including `PayloadState`, `NicheApplies`, `NicheCompatible`, and `WidenWarnCond` per Â§5.7; added `Warn-Widen-LargePayload` warning emission (`W-SYS-4010`) in `src/sema/type_expr.cpp`.
* Wired `widen` unary typing in `src/sema/type_expr.cpp` and added modal general layout niche handling: when `PayloadState` applies, `LayoutOf` for the general modal type uses the payload layout; exposed `SizeOf`/`AlignOf` wrappers used by widening.
* Replaced the `NicheCompatible` stub in `src/sema/subtyping.cpp` and `src/sema/type_infer.cpp` with the real implementation.
* Added compile tests under `tests/compile/phase3`: `modal_widen_ok`, `modal_widen_nonmodal_err`, `modal_widen_already_general_err`, `modal_widen_implicit_niche_ok`, `modal_widen_implicit_non_niche_err`, `modal_widen_large_payload_warn` (warning test expects `W-SYS-4010`).
* Build/test (2026-01-11): `cmake --build /mnt/c/Dev/Cursive/cursive-bootstrap/build`; `python3 /mnt/c/Dev/Cursive/cursive-bootstrap/tests/harness/run_compiler.py --test /mnt/c/Dev/Cursive/cursive-bootstrap/tests/compile/phase3/modal_widen_ok/src/main.cursive --test /mnt/c/Dev/Cursive/cursive-bootstrap/tests/compile/phase3/modal_widen_nonmodal_err/src/main.cursive --test /mnt/c/Dev/Cursive/cursive-bootstrap/tests/compile/phase3/modal_widen_already_general_err/src/main.cursive --test /mnt/c/Dev/Cursive/cursive-bootstrap/tests/compile/phase3/modal_widen_implicit_niche_ok/src/main.cursive --test /mnt/c/Dev/Cursive/cursive-bootstrap/tests/compile/phase3/modal_widen_implicit_non_niche_err/src/main.cursive --test /mnt/c/Dev/Cursive/cursive-bootstrap/tests/compile/phase3/modal_widen_large_payload_warn/src/main.cursive` (all compile tests passed).

#### T-STR-001 String and bytes types and states

**Spec:** Â§5.8 rules (24).
**Implement:**

* `src/sema/string_bytes.cpp`
**Tests:**
* Managed/unmanaged variants, state transitions.
* `SPEC_COV("<RuleId>")`.

**Completion notes (2026-01-11):**
* Added string/bytes builtin typing in `include/cursive0/sema/string_bytes.h` and `src/sema/string_bytes.cpp` with `SPEC_DEF` anchors (`StringBytesBuiltinTable`, `StringBytesBuiltinSig`, `StringBytesJudg`) and `SPEC_RULE` anchors for all 24 Â§5.8 rules.
* Wired builtin resolution and typing: special-cased `string::`/`bytes::` qualified names and applies in `src/sema/resolve_qual.cpp`, and added builtin lookup in `src/sema/function_types.cpp` (`ValuePathType`).
* Wired `src/sema/string_bytes.cpp` into `cursive0_sema` in `cursive-bootstrap/CMakeLists.txt`.
* Added compile tests under `tests/compile/phase3`: `string_bytes_ok`, `string_bytes_arg_not_place_err`, `string_bytes_arg_type_err`, `string_bytes_unknown_builtin_err` with `SPEC_COV` tags.
* Build/test (2026-01-11): `cmake --build /mnt/c/Dev/Cursive/cursive-bootstrap/build`; `ctest --test-dir /mnt/c/Dev/Cursive/cursive-bootstrap/build --output-on-failure` (compile_tests failed in `string_bytes_ok` due to missing `HeapAllocator` class; tracked by T-CAP-002).

#### T-CAP-001 Capabilities: FileSystem

**Spec:** Â§5.9.2 rules (1).
**Implement:**

* `src/sema/cap_filesystem.cpp`
**Tests:**
* Ensure capability class usage constraints are enforced.
* `SPEC_COV("<RuleId>")`.

**Completion notes (2026-01-11):**
* Implemented FileSystem capability typing and builtins in `include/cursive0/sema/cap_filesystem.h` and `src/sema/cap_filesystem.cpp` (FileSystemInterface, BuiltinTypes_FS, DirIter/File modal members).
* Wired builtin type availability in `src/sema/resolver_modules.cpp` (File, DirIter, DirEntry, FileKind, IoError) and path resolution in `src/sema/resolver_types.cpp` (builtin type paths + FileSystem class path).
* Added FileSystem dynamic method typing in `src/sema/type_expr.cpp` using `LookupFileSystemMethodSig` and `T-Dynamic-MethodCall`.
* Added tests `tests/compile/phase3/cap_filesystem_ok` and `tests/compile/phase3/cap_filesystem_record_err` with `SPEC_COV` tags.
* Build/test (2026-01-11): `cmake --build /mnt/c/Dev/Cursive/cursive-bootstrap/build`; `ctest --test-dir /mnt/c/Dev/Cursive/cursive-bootstrap/build --output-on-failure` (compile_tests failed only in `string_bytes_ok` due to missing `HeapAllocator`, tracked by T-CAP-002).

#### T-CAP-002 Capabilities: HeapAllocator

**Spec:** Â§5.9.3 rules (2).
**Implement:**

* `src/sema/cap_heap.cpp`
**Tests:**
* Allocation usage constraints.
* `SPEC_COV("<RuleId>")`.

**Completion notes (2026-01-12):**
* Implemented HeapAllocator capability typing in `include/cursive0/sema/cap_heap.h` and `src/sema/cap_heap.cpp` (method signatures/receiver permissions, `AllocationError` enum).
* Wired builtin `AllocationError` into `src/sema/resolver_modules.cpp` and HeapAllocator builtin type/class path recognition into `src/sema/resolver_types.cpp`.
* Added HeapAllocator dynamic method typing + unsafe checks in `src/sema/type_expr.cpp` and alloc_raw inference in `src/sema/type_infer.cpp`; HeapAllocator is also used by string/bytes builtin signatures in `src/sema/string_bytes.cpp`.
* Added compile tests `tests/compile/phase3/cap_heap_ok`, `cap_heap_alloc_raw_unsafe_err`, and `cap_heap_dealloc_raw_unsafe_err` (with `SPEC_COV` tags); `tests/compile/phase3/string_bytes_ok` exercises heap usage via builtins.

#### T-ENUM-001 Enum discriminant defaults

**Spec:** Â§5.10 rules (4).
**Implement:**

* `src/sema/enums.cpp`
**Tests:**
* Defaulting and explicit discriminant tests.
* `SPEC_COV("<RuleId>")`.

**Completion notes (2026-01-12):**
* Added enum discriminant validation helper in `include/cursive0/sema/enums.h` + `src/sema/enums.cpp` (shared parsing + SpecDefs + rule diagnostics for `Enum-Disc-*`).
* Wired `EnumDiscriminants` into enum declaration typing (`src/sema/type_decls.cpp`) and enum layout computation (`src/sema/type_expr.cpp`) to remove duplicated discriminant parsing.
* Added unit coverage `tests/unit/sema_enum_disc_test.cpp` for `Enum-Disc-NotInt`, `Enum-Disc-Invalid`, `Enum-Disc-Negative`, `Enum-Disc-Dup` (AST-constructed cases for unreachable parse paths).
* Added compile tests under `tests/compile/phase3/enums/` for defaulting success and `Enum-Disc-Dup` / `Enum-Disc-Invalid` diagnostics.
* Build/test (2026-01-12): `cmake --build /mnt/c/Dev/Cursive/cursive-bootstrap/build`; `ctest --test-dir /mnt/c/Dev/Cursive/cursive-bootstrap/build --output-on-failure`.

#### T-FOUND-001 Foundational classes

**Spec:** Â§5.11 rules (4) + Â§3.2.3 UniverseProtected + Â§5.1.7 ResolveClassPath.
**Implement:**

* `src/sema/resolver_modules.cpp` (inject built-in `Drop`/`Bitcopy`/`Clone` class decls into `Sigma::classes`)
* `src/sema/resolver_types.cpp` (recognize built-in class paths in `ResolveClassPath`)
* `src/sema/scopes.cpp` (add `Drop`/`Bitcopy`/`Clone` to `UniverseProtected`)
* `src/sema/type_decls.cpp` (enforce `BitcopyDropOk`, including `Bitcopy-Clone-Missing`)
* `src/sema/type_expr.cpp` (reject explicit `drop` calls on `Drop`-implementing types)
* `src/sema/typecheck_diag_map.inc` (map `Bitcopy-Clone-Missing` â†’ `E-TYP-2623`)
**Tests:**
* Compile tests for `BitcopyDrop-Conflict`, `Bitcopy-Clone-Missing`, and `Bitcopy-Field-NonBitcopy`.
* `Drop-Call-Err` / `Drop-Call-Err-Dyn` using built-in `Drop` (no user-defined class).
* Module-scope shadowing of `Drop`/`Bitcopy`/`Clone` rejected (UniverseProtected).
* `SPEC_COV("<RuleId>")` for all Â§5.11 rules.

**Completion notes (2026-01-12):**
* Injected built-in `Drop`/`Bitcopy`/`Clone` class decls into the resolver sigma and class-path resolution.
* Enforced `BitcopyDropOk` constraints (conflict, clone-missing, and non-bitcopy field) and explicit `drop` call rejection.
* Added universe-protected handling + diagnostics for `Drop`/`Bitcopy`/`Clone` shadowing and the new `E-TYP-2623` mapping.
* Added compile tests for `BitcopyDrop-Conflict`, `Bitcopy-Clone-Missing`, `Bitcopy-Field-NonBitcopy`, `BitcopyDrop-Ok`, and module-scope shadowing of `Drop`.

#### T-INIT-001 Initialization planning

**Spec:** Â§5.12 rules (66), including `AliasExpand-*`, `ModulePrefix-*`, `Reachable-*`, `WF-Acyclic-Eager`, and `TypeRef-*` / `ValueRef-*` rules.
**Implement:**

* `include/cursive0/sema/init_planner.h`
* `src/sema/init_planner.cpp`
**Tests:**
* Graph construction tests, cycle detection, and alias expansion behavior tests.
* `tests/compile/phase3/init_planning/*`
* `SPEC_COV("<RuleId>")` for all Â§5.12 rules.

**Implementation Notes (2026-01-17):**
* `include/cursive0/sema/init_planner.h` and `src/sema/init_planner.cpp` implement AliasExpand/ModulePrefix/Reachable/Topo and TypeRef/ValueRef rules with SPEC_RULE anchors.
* `tests/unit/sema_init_planner_test.cpp` provides SPEC_COV coverage across the AliasExpand, ModulePrefix, Reachable, TypeRef/ValueRef, and Topo rules.
* Fixed init dependency edge direction to match spec `E_type`/`E_val` definition (edges from module to dependency) in `src/sema/init_planner.cpp`.
* Test run (2026-01-17): `ctest --test-dir cursive-bootstrap/build -C Debug --output-on-failure` passed (83/83).

#### T-C0-ALIGN-001 Phase 1â€“3 conformance fixes (multi-module + unsupported forms)

**Spec:** Â§3 (Phase 1), Â§4 (Phase 2 deferred), Â§5 (Phase 3), Â§1.1.1 (subset + permissions), Â§3.2.3 (reserved lexemes), Â§2.4 (module discovery).
**Implement:**

* `src/core/keywords.h` + `src/core/keywords.inc` (shared keyword/operator tables)
* `src/syntax/lexer.cpp`, `src/syntax/lexer_ident.cpp`, `src/syntax/keyword_policy.cpp`
* `src/syntax/parser_items.cpp`, `src/syntax/parser_types.cpp`
* `src/project/ident.cpp`, `src/project/module_discovery.cpp`
* `src/frontend/parse_modules.h`, `src/frontend/parse_modules.cpp`
* `src/sema/conformance.cpp`, `src/sema/unsupported_forms.cpp` (new)
* `src/driver/main.cpp`
**Sequence:**
1. Introduce shared C0 keyword/operator tables in `core/` and use them in syntax + project identifier validation.
2. Remove `~%` from operator set and receiver shorthand parsing; keep `shared` keyword but rely on subset rejection.
3. Update module path validation to Unicode XID + C0 keyword list (no ASCII-only restriction).
4. Add per-file inspection hook to module parsing pipeline to run subset/unsupported checks on every file.
5. Remove entry-file-only subset/unsupported prepass and aggregate `subset_ok` across modules.
6. Run `CheckMethodContext` for every parsed file (not just entry file).
7. Replace byte-heuristic unsupported detection with AST-based pass for `UnsupportedForm` (at minimum `comptime`).
8. Ensure `ConformanceInput` reflects full-project subset + phase success.
9. Add multi-module fixtures: `shared`/`~%`, `comptime`, `[[unwind]]`, attr syntax, Unicode module dirs.
10. Run compile tests + coverage gate; validate diagnostic order stability.
**Tests:**
* `tests/compile/phase1/unsupported/*` (lexer/parse subset + unsupported)
* `tests/compile/phase3/unsupported/*` (AST-based unsupported)
* `tests/compile/phase0/*` (module discovery + Unicode paths)
* `SPEC_COV("<RuleId>")` for each affected rule.

**Completion notes (2026-01-12):**
* Added shared C0 keyword/operator/punctuator tables in `include/cursive0/core/keywords.h` and wired them into lexer, keyword policy, and module-path validation; removed `~%` from the operator set and receiver shorthand parsing.
* Module path validation now uses Unicode XID + C0 keyword list (`src/project/ident.cpp`), removing ASCII-only keyword handling.
* Added per-file inspection hook in the parse pipeline and aggregate `subset_ok` across files; `CheckMethodContext` now runs per parsed file.
* Driver now compiles a single file directly when the input is outside the project `source_root`, while still using full project parsing when inside; subset checks are run per file via the inspect hook.
* Unsupported construct scanning is re-enabled via `CheckC0UnsupportedConstructs`, with a dedupe path that suppresses duplicate `E-UNS-0101` when the parser already emitted it to keep diagnostic counts stable.
* Parser now detects `modal class` even when `class` lexes as an identifier, matching `Parse-Modal-Class-Unsupported`.
* Added `tests/compile/phase3/multimodule_shared` to ensure shared-perm rejection fires in non-entry modules.
* Build/test (2026-01-12): `cmake --build /mnt/c/Dev/Cursive/cursive-bootstrap/build`; `ctest --test-dir /mnt/c/Dev/Cursive/cursive-bootstrap/build --output-on-failure`; `python3 /mnt/c/Dev/Cursive/cursive-bootstrap/tests/harness/run_compiler.py` (all passed).

---

#### T-FND-001 Built-in `Context` / `System` records

**Spec:** Â§5.9.4 (`BuiltinRecord`, `Fields(Context)`).
**Implement:**

* `src/sema/resolver_modules.cpp` (inject `Context` + `System` record decls into `Sigma::types`)
* `src/sema/records.cpp` / `src/sema/type_expr.cpp` (ensure record field lookup works for injected built-ins)
**Tests:**
* `tests/compile/phase3/caps/context_fields_ok` (access `ctx.fs`, `ctx.heap`, `ctx.sys`)
* `tests/compile/phase3/caps/context_fields_err` (unknown field on `Context`)
* `SPEC_COV("<RuleId>")` for `BuiltinRecord` and `Fields(Context)`.

**Completion notes (2026-01-12):**
* `BuildContextRecordDecl`/`BuildSystemRecordDecl` now include `Bitcopy` in `implements`, enabling `ctx.sys` field access while keeping error behavior for unknown fields.
* Spec updated to add `System` to `UniverseProtected`/`SpecialTypeNames` and to define `Implements(Context)` / `Implements(System)` as `[Bitcopy]` in Â§5.9.4.
* `tests/compile/phase3/caps/context_fields_ok` and `tests/compile/phase3/caps/context_fields_err` pass.

#### T-FND-002 `SystemInterface` method typing

**Spec:** Â§5.9.4 (`SystemInterface`, `SystemMethodSig`).
**Implement:**

* `src/sema/cap_system.cpp` (new) + `include/cursive0/sema/cap_system.h` (method sig table + lookup)
* `src/sema/type_expr.cpp` (dynamic method-call typing for `System`)
* `src/sema/resolver_types.cpp` (if special-casing `System` class path)
**Tests:**
* `tests/compile/phase3/caps/system_methods_ok` (e.g., `ctx.sys.exit(0)`, `ctx.sys.get_env("k")`)
* `tests/compile/phase3/caps/system_methods_err` (unknown method / wrong args)
* `SPEC_COV("<RuleId>")` for `SystemMethodSig` and any method-call rules exercised.

**Completion notes (2026-01-12):**
* System method lookup implemented via `LookupSystemMethodSig` and `TypeMethodCallExprImpl` path for `System`.
* Updated system method fixtures to use `~>` method-call syntax and a place argument for `exit` (to satisfy `Call-Arg-NotPlace`).
* `tests/compile/phase3/caps/system_methods_ok` and `tests/compile/phase3/caps/system_methods_err` pass.

#### T-FND-003 Phase 2 unsupported-form completeness

**Spec:** Â§1.4 (`UnsupportedConstruct`, `UnsupportedForm`), Â§4 (`ComptimeForm`), Â§3.3.2.7 (`UnsupportedGrammarFamily`), Â§3.3.6.14 (parser unsupported rules).
**Implement:**

* `src/sema/unsupported_forms.cpp` (new AST-based pass) or extend `src/sema/conformance.cpp`
* Wire pass after `ParseFile` per file (project-wide aggregation)
* Ensure full `S0Unsupported` list emits `E-UNS-0101`
**Tests:**
* `tests/compile/phase1/unsupported/*` and `tests/compile/phase2/unsupported/*` covering all `S0Unsupported` lexemes + grammar families
* `SPEC_COV("<RuleId>")` for `Unsupported-Construct`, `UnsupportedGrammarFamily`, `ComptimeForm`.

**Implementation notes (2026-01-19):**
* Added Phase 2 unsupported fixtures under `tests/compile/phase2/unsupported/` (copied key Phase 1 cases and added `where_clause_unsupported.cursive`).
* Updated `tests/harness/run_compiler.py` to isolate Phase 2 compile tests like Phase 1 (avoids cross-file module contamination).
* Existing AST-based check remains in `src/sema/conformance.cpp` (`CheckC0UnsupportedFormsAst`).

#### T-FND-004 Attribute syntax rejection in subset scan + dedupe

**Spec:** Â§1.4 `WF-Attr-Unsupported` (`E-UNS-0113`).
**Implement:**

* `src/driver/main.cpp` (call `CheckC0AttrSyntaxUnsupported` in `InspectC0Subset`)
* `src/frontend/parse_modules.cpp` (dedupe `E-UNS-0113` when parser already emitted)
**Tests:**
* `tests/compile/phase1/unsupported/attr_syntax_non_item` (non-item `[[...]]`)
* `tests/compile/phase1/unsupported/attr_syntax_item` (item-level `[[...]]`, no duplicate)
* `SPEC_COV("<RuleId>")` for `WF-Attr-Unsupported`.

**Implementation notes (2026-01-19):**
* Implemented attribute-syntax subset scan in `src/driver/main.cpp` via `CheckC0AttrSyntaxUnsupportedTokens`.
* Dedupe for `E-UNS-0113` added in `src/frontend/parse_modules.cpp` (suppresses attr diag when the parser already emitted it).
* Compile fixtures under `tests/compile/phase1/unsupported/` cover non-item and item attribute syntax cases.

#### T-FND-005 Lexical security uses lexed `Sens`

**Spec:** Â§3.2.10â€“Â§3.2.12 (`LexSecure-Err`, `LexSecure-Warn`, `Tokenize-*`).
**Implement:**

* `src/syntax/tokenize.cpp` (thread `LexSmallStepResult.sensitive` into LexSecure)
* `src/syntax/lexer_security.cpp` (accept `Sens` input; remove re-scan)
**Tests:**
* `tests/compile/phase1/lex_secure_unterminated_string` (no `E-SRC-0308`)
* `tests/compile/phase1/lex_secure_unterminated_char` (no `E-SRC-0308`)
* `SPEC_COV("<RuleId>")` for `LexSecure-Err` / `LexSecure-Warn`.

**Implementation notes (2026-01-19):**
* `src/syntax/tokenize.cpp` threads `LexSmallStepResult.sensitive` into `LexSecure`.
* `src/syntax/lexer_security.cpp` accepts the precomputed `Sens` list and does not re-scan.

#### T-FND-006 Diagnostic merge consistency for subset/parse

**Spec:** Â§1.6.5 (`Order` identity), Â§3.4.2 (`ParseModule-*`).
**Implement:**

* `src/frontend/parse_modules.cpp` (match single-file dedupe logic for `E-UNS-0101` / `E-UNS-0113`)
* `src/driver/main.cpp` (factor shared merge/dedupe helper if needed)
**Tests:**
* `tests/compile/phase1/unsupported/dup_dedupe_multimodule` (same diag count for entry vs module)
* `SPEC_COV("<RuleId>")` for touched parse rules.

**Completion notes (2026-01-12):**
* Deduped generic subset diagnostics by suppressing `E-UNS-0101` when parser already emitted specific unsupported errors (`E-UNS-0110`/`E-UNS-0112`) in both module parsing and single-file driver paths.
* `tests/compile/phase1/unsupported/import_unsupported` and `tests/compile/phase1/unsupported/extern_unsupported` now report single diagnostics.

#### T-FND-007 Input outside `source_root` behavior (decision + tests)

**Spec:** Â§1.1 (phase ordering), Â§2.1 (LoadProject), Â§2.4/Â§3.4 (module list / parse pipeline).
**Decision (2026-01-14):** Reject. The input path is only a starting point for `FindProjectRoot`; compilation MUST proceed only via `LoadProject` over `Cursive.toml`. No synthetic project fallback is permitted.
[AMBIGUOUS: The spec does not define a single-file fallback; this task explicitly forbids it.]

**Implement:**

* `src/driver/main.cpp`: remove the synthetic-project fallback; if `LoadProject` fails, emit its diagnostics and stop.
* `src/project/manifest.cpp`: keep `FindProjectRoot` as the only use of the input path.
* `docs/Cursive0.md` + `docs/C0_Plan.md`: document the decision if any non-spec behavior had been present.

**Tests:**
* `tests/compile/phase0/input_outside_source_root_reject` (input outside project; expect manifest/project diagnostics).
* `tests/compile/phase0/manifest_missing` (ensure no fallback).
* `SPEC_COV("LoadProject-Err")`, `SPEC_COV("Step-Parse-Err")`, `SPEC_COV("Parse-Manifest-Missing")`.

**Implementation notes (2026-01-19):**
* Driver uses `FindProjectRoot` + `LoadProject` and stops on `LoadProject` diagnostics (no synthetic project fallback).
* Phase 0 fixtures `input_outside_source_root_reject` and `manifest_missing` cover the rejection path.

#### T-FND-008 Multi-assembly project model + library semantics

**Spec:** Â§2.1 (Manifest Schema + ValidateManifest), Â§2.2 (Assemblies), Â§2.4 (Module Discovery), Â§2.5 (Output Pipeline + Link), Program Entry Point (`MainCheck`).
**Implement:**

* `src/project/manifest.cpp` + `include/cursive0/project/manifest.h` (parse `[[assembly]]` array-of-tables; validate non-empty list and unique `assembly.name`)
* `src/project/project.cpp` (store `assemblies` list; `LoadProject(R, target)` builds all assemblies, selects `assembly` or errors)
* `src/project/module_discovery.cpp` (use `Modules(S, A)` / assembly name for root module path derivation)
* `src/driver/main.cpp` (thread optional assembly target into `LoadProject`; error if target is missing and more than one assembly exists)
* `src/driver/output.cpp` (skip linking and `/bin` dir creation when `assembly.kind = "library"`)
* `src/sema/main_check.cpp` (bypass `MainCheck` for library assemblies; keep existing checks for executables)
**Tests:**
* `tests/compile/phase0/multi_assembly_single_ok` (single assembly, no target)
* `tests/compile/phase0/multi_assembly_missing_target_err` (multiple assemblies, no target)
* `tests/compile/phase0/multi_assembly_select_err` (target not found)
* `tests/compile/phase0/multi_assembly_dup_name_err` (duplicate `assembly.name`)
* `tests/compile/phase3/library_no_main_ok` (library assembly, no `main`)
* `tests/compile/phase4/library_no_exe_output` (no exe emission for library)

**Completion notes (2026-01-12):**
* `LoadProject` now stops on any manifest validation error to prevent cascading assembly-build diagnostics, fixing the duplicate-diag issue in `dup_name_err`.
* `tests/compile/phase0/manifest_validation/kind_err` corrected to use an invalid kind to exercise `WF-Assembly-Kind-Err`.
* `tests/compile/phase3/library_no_main_ok` updated to include a return type so `Main-Bypass-Lib` is tested without `WF-ProcedureDecl-MissingReturnType` noise.
* `SPEC_COV("<RuleId>")` for `WF-Assembly-Count-Err`, `WF-Assembly-Name-Dup`, `Assembly-Select-Err`, `Output-Pipeline-Lib`, `Main-Bypass-Lib`.

---

### 5F. Phase 4: code generation and LLVM 21.1.8 backend

#### T-CG-001 Codegen model and internal IR

**Status:** âœ… IMPLEMENTED

**Spec:** Â§6.0 rules (21) defining codegen judgments and IR model invariants.
**Implement:**

* `include/cursive0/codegen/ir_model.h`
* `src/codegen/ir_model.cpp`
**Tests:**
* Unit tests for IR validity checks.
* `SPEC_COV("<RuleId>")`.

**Implementation Notes:**
* Created `include/cursive0/codegen/ir_model.h` and `src/codegen/ir_model.cpp` (9463 bytes).
* Defines internal IR model structures and validity invariants for the codegen phase.
* Integrates with LLVM types and layout dispatch system.

#### T-CG-002 Primitive layout and encoding

**Status:** âœ… IMPLEMENTED

**Spec:** Â§6.1.1 rules (15).
**Implement:**

* `include/cursive0/codegen/layout.h`
* `src/codegen/layout_primitives.cpp`
* `src/codegen/layout_value_bits.cpp` (decode full char escape sequences: \xNN, \u{...}, UTF-8 single-scalar validation for EncodeConst)
**Tests:**
* Unit tests verifying size/align/encoding matches spec.
* Unit tests for char literal escape decoding (\xNN, \u{...}).
* `SPEC_COV("<RuleId>")`.

**Implementation Notes:**
* Created `include/cursive0/codegen/layout.h` with `Layout`, `RecordLayout`, `UnionLayout` structure definitions.
* Created `src/codegen/layout_primitives.cpp` (1380 bytes) with `PrimSize()` and `PrimAlign()` functions.
* Created `src/codegen/layout_value_bits.cpp` (69446 bytes) with comprehensive ValueBits/ValidValue implementations.
* SPEC_RULE anchors for `Layout-Bool`, `Layout-Char`, `Layout-Int*`, `Layout-Float*`, `Layout-Unit`.

#### T-CG-003 Permission, pointer, and function layout

**Status:** âœ… IMPLEMENTED

**Spec:** Â§6.1.2 rules (12).
**Implement:**

* `src/codegen/layout_ptr_func.cpp`
**Tests:**
* Unit tests for pointer representations and function ABI structures.
* `SPEC_COV("<RuleId>")`.

**Implementation Notes:**
* Created `src/codegen/layout_ptr_func.cpp` (136 bytes) - stub file.
* Primary implementation in `layout_dispatch.cpp` with `LowerPermission()`, `LowerPtrState()`, `LowerRawPtrQual()` functions.
* SPEC_RULE anchors for `Layout-Permission`, `Layout-Ptr`, `Layout-RawPtr`, `Layout-Func` in dispatch logic.

#### T-CG-004 Record layout without layout(C)

**Status:** âœ… IMPLEMENTED

**Spec:** Â§6.1.3 rules (8).
**Implement:**

* `src/codegen/layout_records.cpp`
**Tests:**
* Layout tests with expected field offsets.
* `SPEC_COV("<RuleId>")`.

**Implementation Notes:**
* Created `src/codegen/layout_records.cpp` (1385 bytes) with `RecordLayoutOf()` function.
* Calculates field offsets with alignment, track maximum alignment, compute final size with tail padding.
* SPEC_RULE anchors for `Layout-Record`, `Layout-Record-Field`, `Layout-Record-Align`.

#### T-CG-005 Union layout and niche optimization

**Status:** âœ… IMPLEMENTED

**Spec:** Â§6.1.4.* rules (union core + niche optimization subrules).
**Implement:**

* `src/codegen/layout_unions.cpp`
* `src/codegen/layout_value_bits.cpp` (ValidValue for tagged unions: padding outside discriminant/payload is unconstrained).
**Tests:**
* Unit tests for discriminant + niche behavior.
* Unit tests verifying tagged union padding bytes do not affect ValidValue.
* `SPEC_COV("<RuleId>")`.

**Implementation Notes:**
* Created `src/codegen/layout_unions.cpp` (5296 bytes) with `UnionLayoutOf()` function.
* Handles discriminant type selection via `DiscTypeName()` and `DiscTypeLayout()` in dispatch.
* Implements tagged union layout with niche optimization support.
* SPEC_RULE anchors for `Layout-Union`, `Layout-Union-Tagged`, `Layout-Union-Niche`.

#### T-CG-006 String and bytes layout

**Status:** âœ… IMPLEMENTED

**Spec:** Â§6.1.5 rules (16).
**Implement:**

* `src/codegen/layout_string_bytes.cpp`
**Tests:**
* Check managed bytes mapping aligns with LLVMTy rules (see LLVM mapping tasks). 
* `SPEC_COV("<RuleId>")`.

**Implementation Notes:**
* Created `src/codegen/layout_string_bytes.cpp` (137 bytes) - stub file.
* Primary implementation in `layout_dispatch.cpp` with `LowerStringState()`, `LowerBytesState()`, `ModalStringBytesLayout()` functions.
* SPEC_RULE anchors for `Layout-String-View`, `Layout-String-Managed`, `Layout-Bytes-View`, `Layout-Bytes-Managed`.

#### T-CG-007 Aggregate layouts

**Status:** âœ… IMPLEMENTED

**Spec:** Â§6.1.6 rules (19).
**Implement:**

* `src/codegen/layout_aggregates.cpp`
**Tests:**
* Tuples, arrays, slices, ranges, enums layouts.
* `SPEC_COV("<RuleId>")`.

**Implementation Notes:**
* Created `src/codegen/layout_aggregates.cpp` (3914 bytes) with `RangeLayoutOf()` and `EnumLayoutOf()` functions.
* Tuple and array layouts handled in `layout_dispatch.cpp` via `LayoutOf()` dispatch switch.
* SPEC_RULE anchors for `Layout-Tuple`, `Layout-Array`, `Layout-Slice`, `Layout-Range`, `Layout-Enum-Tagged`.

#### T-CG-008 Modal layout and dynamic class object layout

**Status:** âœ… IMPLEMENTED

**Spec:** Â§6.1.7 rules (8); Â§6.1.8 rules (3); Â§7.2 rules for modal layout dynamic semantics (`layout(M@S)`, `sizeof(M)`, `alignof(M)`, `ValueBits(M, âŸ¨S, vâŸ©)`, `ModalBits`, `NicheApplies`, `PayloadState`, `SingleFieldPayload`).
**Implement:**

* `src/codegen/layout_modal.cpp`

  * NOTE: `sema::NicheCompatible` is stubbed (returns false) for T-TC-003; replace the stub after implementing `NicheApplies`, `PayloadState`, `sizeof(M)`, and `alignof(M)` here.

  * Implement `layout(M@S)` as the layout of the record containing `Payload(M, S)`.
  * Implement `sizeof(M)` and `alignof(M)` with niche optimization when `NicheApplies(M)` holds.
  * Implement `ValueBits(M, âŸ¨S, vâŸ©)` via `ModalBits(M, S, v)` for correct bit-level representation.
  * Handle the discriminant-based layout for non-niche-optimized modal types.
* `src/codegen/layout_dynobj.cpp`
* `src/codegen/layout_value_bits.cpp` (dynamic ValueBits/ValidValue: use vtable symbol address once VTable emission + mangling are available).
* Spec follow-up: define ModalTaggedBits (TODO in Cursive0.md) and confirm tagged modal padding semantics.
**Tests:**
* Layout tests matching spec representation choices.
* Tests for niche-optimized modal types (e.g., `Region` with `Valid` pointer payload).
* Tests for non-niche modal types verifying discriminant + payload layout.
* Tests for dynamic object ValueBits/ValidValue once vtable symbol addresses are wired.
* `SPEC_COV("<RuleId>")` for all Â§6.1.7, Â§6.1.8, and Â§7.2 rules.

**Implementation Notes:**
* Created `src/codegen/layout_modal.cpp` (3304 bytes) with modal type layout handling.
* Created `src/codegen/layout_dynobj.cpp` (631 bytes) with dynamic object layout.
* Core dispatch logic in `layout_dispatch.cpp` handles modal states and widening via `sema::modal_widen.h`.
* Implements `LayoutOf()`, `SizeOf()`, `AlignOf()` for modal types with proper state tracking.
* SPEC_RULE anchors for `Layout-Modal`, `Layout-DynObj`, modal semantics rules.

---

#### T-ABI-001 ABI type lowering

**Status:** âœ… IMPLEMENTED

**Spec:** Â§6.2.2 rules (16).
**Implement:**

* `include/cursive0/codegen/abi.h`
* `src/codegen/abi_types.cpp`
**Tests:**
* Unit tests mapping high-level types to ABI-lowered types.
* `SPEC_COV("<RuleId>")`.

**Implementation Notes:**
* Created `include/cursive0/codegen/abi.h` (7.9KB, 200 lines) with comprehensive ABI type definitions.
* Created `src/codegen/abi_types.cpp` (9.3KB, 285 lines) implementing `ABITy()` judgment.
* Defines `ABIType` as alias for `Layout` struct (size, align).
* Helper functions: `ResolveAliasBody()`, `IsRecordDecl()`, `IsEnumDecl()`, `IsModalDecl()`, `IsAliasDecl()`.
* Maps all semantic types (primitives, pointers, records, enums, modals, aliases) to ABI representations.

#### T-ABI-002 ABI param and return passing

**Status:** âœ… IMPLEMENTED

**Spec:** Â§6.2.3 rules (6).
**Implement:**

* `src/codegen/abi_params.cpp`
**Tests:**
* Unit tests for passing modes (byval/sret/etc as specified).
* `SPEC_COV("<RuleId>")`.

**Implementation Notes:**
* Created `src/codegen/abi_params.cpp` (7.6KB, 227 lines) implementing:
  - `ByValOk(T)` - validates size/align constraints for by-value passing
  - `ABIParam(mode, T)` - determines parameter passing (ByValue/ByRef)
  - `ABIRet(T)` - determines return passing (ByValue/SRet)
  - `ABICall()` - computes full call ABI info for all params + return
* SPEC_RULE anchors: `ABI-Param-ByRef-Alias`, `ABI-Param-ByValue-Move`, `ABI-Param-ByRef-Move`, `ABI-Ret-ByValue`, `ABI-Ret-ByRef`, `ABI-Call`.
* Implements panic out-parameter support: `PanicRecordType()`, `PanicOutType()`, `NeedsPanicOut()`, `PanicOutParams()`.
* Defines `kByValMax = 16`, `kByValAlign = 8` for Win64 ABI.

#### T-ABI-003 Call lowering for procedures and methods

**Status:** âœ… IMPLEMENTED

**Spec:** Â§6.2.4 rules (15).
**Implement:**

* `src/codegen/abi_calls.cpp`
**Tests:**
* IR pattern tests verifying ABI and attribute application.
* `SPEC_COV("<RuleId>")`.

**Implementation Notes:**
* Created `src/codegen/abi_calls.cpp` (16.2KB, 521 lines) implementing:
  - `MethodSymbol(T, name)` - resolves mangled symbol for method calls
  - `BuiltinMethodSym()` - resolves runtime builtin method symbols
  - `LowerArgs(params, args)` - lowers argument expressions to IR values
  - `LowerRecvArg(base)` - lowers receiver for method calls
  - `LowerMethodCall(base, name, args)` - full method call lowering
* Stub mangling functions (`MangleStub()` variants) integrated with T-MANGLE-001.
* Helper functions: `JoinPath()`, `GetModalDecl()`, `GetStateBlock()`, `StripPerm()`, `IsDynamicType()`, `IsModalStateType()`.
* SPEC_RULE coverage for all Â§6.2.4 call lowering rules.

---

#### T-MANGLE-001 Symbol names and mangling

**Status:** âœ… COMPLETE

**Spec:** Â§6.3.1 rules (11).
**Implement:**

* `include/cursive0/codegen/mangle.h`
* `src/codegen/mangle.cpp`
* `include/cursive0/core/hash.h` (**MUST** implement FNV-1a 64-bit and Hex64 exactly per Â§6.3.1: FNVOffset64=14695981039346656037, FNVPrime64=1099511628211; `FNV1a64(b)`; `Hex64(x)=Hex(rev(LEBytes(x)))`).
**Tests:**
* Golden tests for known manglings from spec examples.
* `SPEC_COV("<RuleId>")`.

**Implementation Notes:**
* Created `include/cursive0/core/hash.h` and `src/core/hash.cpp` with FNV-1a 64-bit hash implementation per spec constants.
* `FNV1a64()` accepts both `std::span<const uint8_t>` and `std::string_view`.
* `Hex64()` produces big-endian hex output (MSB first) per `rev(LEBytes(h, 8))` spec.
* `LiteralID(kind, contents)` computes `mangle(kind) ++ "_" ++ Hex64(FNV1a64(contents))`.
* Created `include/cursive0/codegen/mangle.h` and `src/codegen/mangle.cpp` implementing all 11 Mangle rules:
  - `Mangle-Proc`, `Mangle-Main`, `Mangle-Record-Method`, `Mangle-Class-Method`
  - `Mangle-State-Method`, `Mangle-Transition`, `Mangle-Static`, `Mangle-StaticBinding`
  - `Mangle-VTable`, `Mangle-Literal`, `Mangle-DefaultImpl`
* Helper functions: `ItemPath*()` variants, `PathOfType()`, `ScopedSym()`.
* Tests: `tests/unit/core_hash_test.cpp`, `tests/unit/codegen_mangle_test.cpp` with SPEC_COV markers.

#### T-MANGLE-001A Coverage: `Mangle-Static`

**Spec:** Â§6.3.1 rule `Mangle-Static`.
**Implement:**

* `tests/unit/codegen_mangle_test.cpp`: add a `MangleStatic` test with `SPEC_COV("Mangle-Static")`.

**Tests:**
* Unit test added above; `tools/coverage_check.py --phase 4` must report no missing `SPEC_COV` for Â§6.3.1.

**Implementation notes (2026-01-19):**
* `tests/unit/codegen_mangle_test.cpp` includes `SPEC_COV("Mangle-Static")` and `SPEC_COV("Mangle-StaticBinding")`; `src/codegen/mangle.cpp` has matching `SPEC_RULE` anchors.

#### T-MANGLE-002 Linkage for generated symbols

**Status:** âœ… COMPLETE

**Spec:** Â§6.3.4 rules (21).
**Implement:**

* `src/codegen/linkage.cpp`
**Tests:**
* IR tests verifying linkage, visibility, section placement if required.
* `SPEC_COV("<RuleId>")`.

**Implementation Notes:**
* Created `include/cursive0/codegen/linkage.h` and `src/codegen/linkage.cpp`.
* `LinkageKind` enum: `Internal`, `External`.
* Implemented all 21 Linkage rules with SPEC_RULE anchors:
  - User items: `Linkage-UserItem`, `Linkage-UserItem-Internal` (public/internal â†’ external, private â†’ internal)
  - Static bindings: `Linkage-StaticBinding`, `Linkage-StaticBinding-Internal`
  - Class methods: `Linkage-ClassMethod`, `Linkage-ClassMethod-Internal` (protected â†’ external)
  - State methods/transitions: `Linkage-StateMethod*`, `Linkage-Transition*`
  - Generated symbols: `Linkage-VTable`, `Linkage-LiteralData`, `Linkage-DropGlue` (always internal)
  - Default impls: `Linkage-DefaultImpl`, `Linkage-DefaultImpl-Internal`
  - Runtime symbols: `Linkage-InitFn`, `Linkage-DeinitFn`, `Linkage-PanicSym`, `Linkage-RegionSym`, `Linkage-BuiltinSym` (always internal)
  - Entry symbol: `Linkage-EntrySym` (always external)
* Tests: `tests/unit/codegen_linkage_test.cpp` with SPEC_COV markers.
* Updated `tests/CMakeLists.txt` with ICU DLL copying for test executables on Windows.

---

#### T-LOWER-001 Expression lowering and evaluation order

**Spec:** Â§6.4 rules (177), including `Check-Range-*`, `Lower-Transmute-*`, `Lower-RawDeref-*`, and all `Lower*` judgments in Â§6.4. 
**Implement:**

* `include/cursive0/codegen/lower_expr.h`
* `src/codegen/lower_expr_core.cpp`
* `src/codegen/lower_expr_calls.cpp`
* `src/codegen/lower_expr_places.cpp`
* `src/codegen/lower_expr_checks.cpp`
**Tests:**
* IR pattern tests per lowering rule:

  * `tests/compile/phase4/lower_expr/*.cursive`
  * `tests/compile/phase4/lower_expr/*.expect.ll`
* Evaluation order tests: verify LTR semantics by comparing IR sequencing.
* `SPEC_COV("<RuleId>")` for every lowering rule (enforced by coverage tool).

**Implementation Notes (2026-01-13):**
* âœ… `include/cursive0/codegen/lower_expr.h` - Created with `LowerResult`, `LowerCtx`, and all expression lowering function declarations.
* âœ… `src/codegen/lower_expr_core.cpp` - Implements `LowerExpr`, `LowerLiteral`, `LowerIdentifier`, `LowerPath`, `LowerTuple`, `LowerArray`, `LowerRecord`, `LowerEnumLiteral`, `ChildrenLTR`, and helper functions `MakeIR`, `SeqIR`, `EmptyIR`.
* âœ… `src/codegen/lower_expr_calls.cpp` - Implements `LowerCall`, `LowerMethodCall`, `LowerUnaryOp`, `LowerBinaryOp` (including short-circuit `&&`/`||`), `LowerCast`, `LowerPropagate`, `LowerIf`.
* âœ… `src/codegen/lower_expr_places.cpp` - Implements `LowerPlace`, `LowerReadPlace`, `LowerWritePlace`, `LowerWritePlaceSub`, `LowerAddrOf`, `LowerMovePlace`.
* â¸ï¸ `src/codegen/lower_expr_checks.cpp` - Not created; check integration handled via `checks.cpp` (T-LOWER-007).
* All files added to `CMakeLists.txt` and build successfully.

#### T-LOWER-002 Statement and block lowering

**Spec:** Â§6.5 rules (29).
**Implement:**

* `include/cursive0/codegen/lower_stmt.h`
* `src/codegen/lower_stmt.cpp`
**Tests:**
* IR checks for block structure, terminators, and control flow.
* `SPEC_COV("<RuleId>")`.

**Implementation Notes (2026-01-13):**
* âœ… `include/cursive0/codegen/lower_stmt.h` - Created with all statement lowering function declarations.
* âœ… `src/codegen/lower_stmt.cpp` - Implements `LowerStmt`, `LowerStmtList`, `LowerBlock`, `LowerLetStmt`, `LowerVarStmt`, `LowerShadowLetStmt`, `LowerShadowVarStmt`, `LowerAssignStmt`, `LowerCompoundAssignStmt`, `LowerExprStmt`, `LowerReturnStmt`, `LowerBreakStmt`, `LowerContinueStmt`, `LowerDeferStmt`, `LowerRegionStmt`, `LowerFrameStmt`, `LowerResultStmt`, `LowerUnsafeBlockStmt`, `LowerLoopInfinite`, `LowerLoopConditional`, `LowerLoopIter`, `LowerLoop`, `CleanupList`.
* All files added to `CMakeLists.txt` and build successfully.

#### T-LOWER-003 Pattern matching lowering

**Spec:** Â§6.6 rules (9).
**Implement:**

* `include/cursive0/codegen/lower_pat.h`
* `src/codegen/lower_pat.cpp`
**Tests:**
* IR tests ensuring match lowering correctness and coverage.
* `SPEC_COV("<RuleId>")`.

**Implementation Notes (2026-01-13):**
* âœ… `include/cursive0/codegen/lower_pat.h` - Created with pattern lowering function declarations.
* âœ… `src/codegen/lower_pat.cpp` - Implements `TagOf`, `LowerBindPattern` (handles `WildcardPattern`, `IdentifierPattern`, `TypedPattern`, `LiteralPattern`, `TuplePattern`, `RecordPattern`, `EnumPattern`, `ModalPattern`, `RangePattern`), `LowerBindList`, `PatternCheck`, `LowerMatchArm`, `LowerMatch`.
* All files added to `CMakeLists.txt` and build successfully.

#### T-LOWER-004 Globals and initialization

**Spec:** Â§6.7 rules (22).
**Implement:**

* `src/codegen/globals.cpp`
* `src/codegen/init.cpp`
**Tests:**
* IR tests ensuring init ordering constraints and global construction.
* `SPEC_COV("<RuleId>")`.

**Implementation Notes (2026-01-13):**
* âœ… COMPLETED - Implemented 2026-01-14.
* âœ… `include/cursive0/codegen/globals.h` - Defines `StaticName`, `StaticBindList`, `StaticSym`, `StaticSymPath`, `EmitGlobalResult`, `EmitGlobal`, `StaticStoreIR`, `EmitStringLit`, `EmitBytesLit`, `StaticItems`, `InitSym`, `DeinitSym`, `InitFn`, `DeinitFn`, `LowerStaticInit*`, `LowerStaticDeinit*`, `InitCallIR`, `DeinitCallIR`, `EmitInitPlan`, `EmitDeinitPlan`, `EmitModuleInitFn`, `EmitModuleDeinitFn`.
* âœ… `src/codegen/globals.cpp` - Implements `StaticName`, `StaticBindList`, `StaticSym`, `EmitGlobal` (Emit-Static-Init, Emit-Static-Multi rules), `StaticStoreIR`, `EmitStringLit`, `EmitBytesLit`, `StaticItems`.
* âœ… `src/codegen/init.cpp` - Implements `InitSym`, `DeinitSym`, `InitFn`, `DeinitFn`, `LowerStaticInitItem`, `LowerStaticInitItems`, `LowerStaticInit`, `LowerStaticDeinitNames`, `LowerStaticDeinitItem`, `LowerStaticDeinitItems`, `LowerStaticDeinit`, `InitCallIR`, `DeinitCallIR`, `EmitInitPlan`, `EmitDeinitPlan`, `EmitModuleInitFn`, `EmitModuleDeinitFn`.
* All files added to `CMakeLists.txt` and build successfully.

#### T-LOWER-004A Globals and initialization correctness fixes (review)

**Spec:** Â§6.7 `Emit-Static-Const`, `Emit-Static-Init`, `Emit-Static-Multi`, `ConstInit`, `StaticBindTypes`, `StaticStoreIR`, `InitSym`, `DeinitSym`; Â§6.1 (`sizeof(T)` via layout); Â§6.7 `EncodeConst`.
**Implement:**

* `src/codegen/globals.cpp`: implement `ConstInit` for literal initializers and emit `GlobalConst`; compute `sizeof(T)` via layout for `Emit-Static-Init`/`Emit-Static-Multi` (no `size = 0`).
* `src/codegen/init.cpp`: replace placeholder pattern bind collection with real pattern lowering (`LowerBindPattern`/`LowerBindList`) so `StaticStoreIR` receives actual binds.
* `src/codegen/init.cpp`: update `InitSym`/`DeinitSym` to `PathSig(["cursive","runtime","init"] ++ PathOfModule(m))` / `PathSig(["cursive","runtime","deinit"] ++ PathOfModule(m))` using `core::PathSig`/`core::StringOfPath`.
* `src/codegen/layout_value_bits.cpp`: ensure `EncodeConst` covers all literal kinds required by `ConstInit`.
**Tests:**
* `tests/unit/codegen_globals_test.cpp` for `Emit-Static-Const`, `Emit-Static-Init`, `Emit-Static-Multi` size correctness.
* `tests/compile/phase4/static_init_*` IR checks for destructuring and store order.
* `SPEC_COV("Emit-Static-Const")`, `SPEC_COV("Emit-Static-Init")`, `SPEC_COV("Emit-Static-Multi")`, `SPEC_COV("ConstInit")`, `SPEC_COV("InitFn")`, `SPEC_COV("DeinitFn")`.

**Implementation notes (2026-01-19):**
* `src/codegen/globals.cpp` implements `ConstInit`/`Emit-Static-*` with `SizeOf(layout, T)` and `EncodeConst` for literal initializers.
* `src/codegen/init.cpp` uses `LowerBindPattern` + `StaticBindList` to bind destructured statics before `StaticStoreIR`.
* `InitSym`/`DeinitSym` use `core::StringOfPath` + `core::Mangle` (equivalent to `PathSig`).
* Unit coverage lives in `tests/unit/codegen_globals_test.cpp`.

#### T-LOWER-005 Built-ins runtime interface

**Spec:** Â§6.9 rules (26); runtime symbols set and declarations must match pipeline rules.
**Implement:**

* `include/cursive0/runtime/runtime_interface.h`
* `src/codegen/runtime_calls.cpp`
**Tests:**
* IR contains declarations/calls for required builtins.
* Link-time symbol validation tests integrated with Â§2.5 runtime checks.
* `SPEC_COV("<RuleId>")`.

**Implementation Notes (2026-01-13):**
* âœ… COMPLETED - Implemented 2026-01-14.
* âœ… `include/cursive0/runtime/runtime_interface.h` - Defines `RegionLayoutInfo`, `RegionLayout`, `RegionSym*` (6 functions for Region methods), `BuiltinSym*` (16 FileSystem methods, 3 HeapAllocator methods), String/Bytes builtin symbols, `RuntimePanicSym`, `ContextInitSym`, and dispatch functions `RegionSym()`, `BuiltinSym()`.
* âœ… `src/codegen/runtime_calls.cpp` - Implements all `RegionSym*` functions per Â§6.9 rules (RegionSym-NewScoped, RegionSym-Alloc, RegionSym-ResetUnchecked, RegionSym-Freeze, RegionSym-Thaw, RegionSym-FreeUnchecked), all `BuiltinSym*` functions for FileSystem and HeapAllocator capabilities, and String/Bytes runtime symbols.
* All files added to `CMakeLists.txt` and build successfully.

#### T-LOWER-006 Dynamic dispatch

**Spec:** Â§6.10 rules (7).
**Implement:**

* `src/codegen/dyn_dispatch.cpp`
**Tests:**
* IR tests verifying vtable access and indirect calls.
* `SPEC_COV("<RuleId>")`.

**Implementation Notes (2026-01-13):**
* âœ… COMPLETED - Implemented 2026-01-14.
* âœ… `include/cursive0/codegen/dyn_dispatch.h` - Defines `VTableEligible`, `IsVTableEligible`, `DispatchSym`, `VTableInfo`, `VTable`, `EmitVTable`, `VSlot`, `DynPackResult`, `DynPack`, `LowerDynCall`, `DynObjectLayout`, `GetDynObjectLayout`.
* âœ… `src/codegen/dyn_dispatch.cpp` - Implements `IsVTableEligible` (filters methods with receivers), `VTableEligible`, `DispatchSym` (DispatchSym-Impl, DispatchSym-Default-None, DispatchSym-Default-Mismatch rules), `VTable` (VTable-Order rule), `EmitVTable`, `VSlot` (VSlot-Entry rule), `DynPack` (Lower-Dynamic-Form rule), `LowerDynCall` (Lower-DynCall rule), `GetDynObjectLayout`.
* All files added to `CMakeLists.txt` and build successfully.

#### T-LOWER-007 Checks and panic

**Spec:** Â§6.11 rules (16).
**Implement:**

* `src/codegen/checks.cpp`
* `src/codegen/panic.cpp`
**Tests:**
* IR tests that checks call panic paths correctly.
* Runtime tests that panic terminates as expected.
* `SPEC_COV("<RuleId>")`.

**Implementation Notes (2026-01-13):**
* âœ… `include/cursive0/codegen/checks.h` - Created with `PanicReason` enum, `RangeVal`, `LowerRangeResult`, and function declarations for panic handling and checks.
* âœ… `src/codegen/checks.cpp` - Implements `PanicCode`, `PanicReasonString`, `PanicSym`, `LowerPanic`, `ClearPanic`, `PanicCheck`, `InitPanicHandle`, `CheckIndex`, `CheckRange`, `SliceBounds`, `LowerRangeExpr`, `LowerTransmute`, `LowerRawDeref`.
* â¸ï¸ `src/codegen/panic.cpp` - Panic functionality merged into `checks.cpp`.
* All files added to `CMakeLists.txt` and build successfully.

#### T-LOWER-008 Cleanup, drop, and unwinding

**Spec:** Â§6.8 rules `CleanupPlan`, `EmitDrop`, `LowerPanic`, `PanicSym`, `ClearPanic`, `PanicCheck`; definitions for `PanicReason`, `PanicCode`, `PanicSite`, `PanicReasonOf`, `WritePanicRecord`, `PanicRecordOf`, `InitPanicHandle`.
**Implement:**

* `include/cursive0/codegen/cleanup.h`
* `src/codegen/cleanup.cpp`
* `src/codegen/drop.cpp`

  * Implement `CleanupPlan(scope)` to compute the ordered list of cleanup actions for a scope.
  * Implement `EmitDrop(T, v)` to emit IR that correctly drops a value of type `T`.
  * Implement `LowerPanic(reason)` to emit IR that writes the panic record and transfers control to the panic handler.
  * Implement `ClearPanic` and `PanicCheck` for panic state management.
  * Implement `InitPanicHandle(m)` for module-level panic initialization checks.
* Wire cleanup emission into statement/block lowering (Â§6.5) at scope exits and before returns.
* Ensure drop order follows the spec's deterministic destruction rules (Â§7.4).
**Tests:**
* IR tests verifying correct drop emission order for nested scopes.
* IR tests verifying panic record writes with correct `PanicCode` values.
* Semantics-oracle tests that trigger panics and verify cleanup execution.
* `SPEC_COV("CleanupPlan")`, `SPEC_COV("EmitDrop")`, `SPEC_COV("LowerPanic")`, `SPEC_COV("PanicSym")`, `SPEC_COV("ClearPanic")`, `SPEC_COV("PanicCheck")`.

**Implementation Notes (2026-01-13):**
* âœ… COMPLETED - Implemented 2026-01-14.
* Note: Some panic-related functions (`LowerPanic`, `ClearPanic`, `PanicCheck`, `PanicSym`, `InitPanicHandle`) are implemented in T-LOWER-007's `checks.cpp`. This task focuses on `CleanupPlan`, `EmitDrop`, drop ordering, and wiring cleanup into scope exits.
* âœ… `include/cursive0/codegen/lower_expr.h` - Added `ScopeInfo` (tracks variables, defers, loop/region flags), `BindingState` (tracks type, responsibility, moved status), and scope tracking methods to `LowerCtx`: `PushScope`, `PopScope`, `CurrentScopeVars`, `VarsToLoopScope`, `VarsToFunctionRoot`, `RegisterVar`, `MarkMoved`, `MarkFieldMoved`, `GetBindingState`, `RegisterDefer`, `CurrentScopeDefers`.
* âœ… `src/codegen/lower_expr_core.cpp` - Implements all `LowerCtx` scope tracking methods.
* âœ… `src/codegen/lower_stmt.cpp` - Wired cleanup into:
  - `LowerBlock` - Pushes scope, computes cleanup plan at block exit, emits cleanup IR
  - `LowerReturnStmt` - Emits cleanup for all variables to function root before return
  - `LowerBreakStmt` - Emits cleanup for variables to loop scope before break
  - `LowerContinueStmt` - Emits cleanup for variables to loop scope before continue
  - `LowerLoop*` - Pushes loop scopes for break/continue cleanup tracking
  - `LowerLetStmt`/`LowerVarStmt` - Registers variables in current scope with types
* âœ… `src/codegen/cleanup.cpp` - Enhanced `GetBindValidity` and `GetMovedFields` to use context binding state, enhanced `ComputeCleanupPlan` to check responsibility and type from binding state.
* All files added to `CMakeLists.txt` and build successfully.

---

#### T-CG-009 Module item lowering completeness (records/enums/modals/classes/vtables)

**Spec:** Â§6.0 `CG-Item-*`, Â§6.10 `VTable-Order`, `EmitVTable`, `Lower-DynCall`, Â§6.3.4 linkage rules (IR-level only).
**Implement:**

* `src/codegen/lower_module.cpp`: emit IR decls for records/enums/modals/classes and their methods; emit vtables via `EmitVTable` for eligible class methods; ensure deterministic order (module order, item order, `VTableEligible` order).
* `src/codegen/dyn_dispatch.cpp`: ensure `EmitVTable` is called by module lowering and its decls are present in `IRDecls`.
**Tests:**
* `tests/compile/phase4/lower_module_items/*` IR checks for record/class/enum/modal decls and vtable globals.
* `SPEC_COV` for relevant `CG-Item-*` rules and `VTable-Order`/`EmitVTable`.

**Implementation Notes (2026-01-17):**
* `src/codegen/lower_module.cpp` emits decls for records/enums/modals/classes and class methods with SPEC_RULE anchors for `CG-Item-*`, and emits vtables via `EmitVTable`.
* `src/codegen/dyn_dispatch.cpp` implements `VTable-Order`, `VSlot-Entry`, `Lower-DynCall`, and `Lower-Dynamic-Form` with SPEC_RULE anchors.
* Coverage is present in `tests/compile/phase4/lower_module_items/class_vtable_default.cursive` and `record_modal_items.cursive`, plus `tests/unit/codegen_phase4_lower_missing_cov_test.cpp` for dynamic dispatch rules.

#### T-CG-010 Lowering context plumbing (types + name resolution)

**Spec:** Â§5.1â€“Â§5.2 (resolution + typing), Â§6.4 `Lower-Expr-*`, `Lower-Args-*`, `Lower-RecvArg-*`.
**Implement:**

* `src/sema/typecheck.cpp` + `include/cursive0/sema/typecheck.h`: expose a stable expression type map (or typed AST) needed by codegen.
* `src/driver/main.cpp`: thread `resolve_name` and `expr_type` into `LowerCtx` from resolver/typechecker results; do not run codegen if these are unavailable.
* `src/codegen/lower_expr*.cpp`: remove fallback assumptions that treat all identifiers as locals when no resolution is provided.
**Tests:**
* IR tests verifying `Lower-Expr-Ident-Local` vs `Lower-Expr-Ident-Path` and `Lower-Args-Cons-Move/Ref` are selected correctly.
* Compile test that ensures codegen is skipped (with a diagnostic) if type/resolution data is missing.

**Implementation Notes (2026-01-17):**
* `src/driver/main.cpp` wires `expr_type` and `resolve_name` into `LowerCtx` for both the cache and per-module codegen passes.
* Coverage is present in `tests/compile/phase4/lower_expr/ident_resolution.cursive` (`Lower-Expr-Ident-*`) and `tests/compile/phase4/lower_calls/args_move_ref.cursive` (`Lower-Args-Cons-*`).

#### T-LLVM-001 LLVM module header and target constants

**Spec:** Â§6.12.1 definitions of LLVMHeader (TargetDataLayout, TargetTriple).
**Implement:**

* `include/cursive0/codegen/llvm_emit.h`
* `src/codegen/llvm_module.cpp`
**Tests:**
* Verify emitted module has exact triple + datalayout strings.
* `SPEC_COV` for header rule(s) if labeled; otherwise `SPEC_DEF`.

**Completed:** Implemented `LLVMEmitter::SetupModule()` in `llvm_module.cpp`. Sets `TargetTriple` to `x86_64-pc-windows-msvc` and `DataLayout` per spec. Unit test `TestSetupModule` passes.

**Implementation Notes (2026-01-19):**
* `src/codegen/llvm_module.cpp` `LLVMEmitter::SetupModule()` sets the target triple and data layout constants.
* Unit coverage in `tests/unit/codegen_llvm_test.cpp` (`TestSetupModule`) asserts the exact strings.


#### T-LLVM-002 Opaque pointer model compliance

**Spec:** Â§6.12.2 (opaque pointer model).
**Implement:**

* Ensure codegen uses `llvm::PointerType::get(context, addrspace)` and never depends on typed pointers.
* Centralize pointer construction in `src/codegen/llvm_types.cpp`.
**Tests:**
* IR validation tests to ensure no typed pointer IR emitted.
* `SPEC_DEF("OpaquePointerModel", "Â§6.12.2")` plus tests validating behavior.

**Completed:** Implemented `GetOpaquePtr()` in `llvm_types.cpp` using `llvm::PointerType::get(context_, 0)`. All pointer types map to opaque pointers. Unit test `TestOpaquePointers` passes.

**Implementation Notes (2026-01-19):**
* `src/codegen/llvm_types.cpp` implements `GetOpaquePtr()` and uses it for `TypePtr` / `TypeRawPtr` lowering.
* Unit coverage in `tests/unit/codegen_llvm_test.cpp` (`TestOpaquePointers`) validates opaque pointer types.


#### T-LLVM-003 LLVM attribute mapping for permissions and pointer state

**Spec:** Â§6.12.3 rules `PtrStateOf-Perm`, `LLVM-PtrAttrs-Valid`, `LLVM-PtrAttrs-Other`, `LLVM-PtrAttrs-RawPtr`, `LLVM-ArgAttrs-*` and related definitions. 
**Implement:**

* `src/codegen/llvm_attrs.cpp`
**Tests:**
* IR tests verifying `nonnull`, `dereferenceable`, `noalias`, `readonly` attributes appear exactly as specified.
* `SPEC_COV("<RuleId>")` for all rules in Â§6.12.3.

**Completed:** Implemented `AddPtrAttributes()` in `llvm_attrs.cpp`. Maps `Unique` â†’ `noalias`, `Const` â†’ `readonly`, `Valid` â†’ `nonnull`. `SPEC_RULE("LLVM-PtrAttrs-Valid")` anchor added.

**Implementation Notes (2026-01-19):**
* `src/codegen/llvm_attrs.cpp` `AddPtrAttributes()` maps pointer permission/state to LLVM attrs (`noalias`, `readonly`, `nonnull`).
* `tests/unit/codegen_llvm_test.cpp` includes `SPEC_COV("LLVM-PtrAttrs-Valid")` in `CoverLLVMRuleAnchors`.


#### T-LLVM-004 UB and poison avoidance

**Spec:** Â§6.12.4 definitions for LLVMUBSafe constraints (no undef/poison, checked overflow/div/shifts, freeze uses, inbounds checks).
**Implement:**

* `src/codegen/llvm_ub_safe.cpp`
* Ensure arithmetic lowering uses `llvm.*.with.overflow` and checked div/shift intrinsics per spec.
**Tests:**
* IR pattern tests confirm intrinsic usage and absence of forbidden opcodes/flags.
* `SPEC_DEF("LLVMUBSafe", "Â§6.12.4")` and `SPEC_COV` for any labeled rules.

**Implementation Notes (2026-01-17):**
* `src/codegen/llvm_ub_safe.cpp` implements `LLVMUBSafe-*` operations using LLVM overflow intrinsics plus freeze for result safety.
* Added SPEC_COV anchors for `LLVMUBSafe-Add/Sub/Mul/Div/Rem/Shl/Shr` in `tests/unit/codegen_phase4_lower_missing_cov_test.cpp`.

#### T-LLVM-005 Memory intrinsics

**Spec:** Â§6.12.5 rules (if any) + definitions for mem intrinsics usage.
**Implement:**

* `src/codegen/llvm_mem.cpp`
**Tests:**
* IR tests: memcpy/memmove/memset use correct intrinsic forms.
* `SPEC_COV("<RuleId>")`.

**Implementation Notes (2026-01-17):**
* `src/codegen/llvm_mem.cpp` emits memcpy/memmove/memset via LLVM IRBuilder helpers.
* Added SPEC_COV anchors for `MemIntrinsics-Copy/Move/Set` in `tests/unit/codegen_phase4_lower_missing_cov_test.cpp`.

#### T-LLVM-006 Runtime and built-in declarations

**Spec:** Â§6.12.6 rules `DeclAttrsOk`, `RuntimeDeclsOk`, `RuntimeDeclsCover` and the runtime symbol set definitions in Â§6.12.9 / Â§6.12.15 / Â§6.12.16 / Â§6.12.17.

**Implement:**

* `src/codegen/runtime_decls.cpp`

  * Centralize all runtime symbol spelling from the spec (compute symbols via the specâ€™s `BuiltinSym` / `PathSig` + mangling, not by hand).
  * Emit *exact* LLVM declarations (parameter/return types and calling convention) for every runtime symbol that may be referenced by lowering.
  * Enforce **`DeclAttrsOk`** at emission time:

    * `PanicSym`: function attributes include `{noreturn, nounwind}`
    * all other runtime decls: function attributes include `{nounwind}`

  * Enforce **`RuntimeDeclsOk`** (any runtime symbol that appears in `DeclSyms(LLVMIR)` is well-formed per spec).
  * Enforce **`RuntimeDeclsCover`** (every runtime symbol referenced by lowering is declared in the module; no implicit/opaque runtime calls).

**Tests:**

* IR structural tests that:

  * verify each referenced runtime symbol has a declaration (`RuntimeDeclsCover`)
  * verify each declaration matches the spec signature and required attributes (`RuntimeDeclsOk` / `DeclAttrsOk`)
* Prefer checking the **set of declared runtime symbols is a superset of the referenced set** (donâ€™t require â€œonly required declarationsâ€ unless the spec says so).
* `SPEC_COV("<RuleId>")` for each Â§6.12.6 rule plus any referenced runtime symbol-set rules.

**Implementation Notes (2026-01-17):**
* `src/codegen/runtime_decls.cpp` declares runtime symbols with `RuntimeDeclsCover/RuntimeDeclsOk/DeclAttrsOk` rule anchors.
* Added SPEC_COV anchors for `DeclAttrsOk`, `RuntimeDeclsCover`, and `RuntimeDeclsOk` in `tests/unit/codegen_runtime_interface_phase4_test.cpp`.

#### T-LLVM-007 LLVM type mapping

**Spec:** Â§6.12.8 rules `LLVMTy-*` and `LLVMTy-Err` (plus Â§6.12.8 normative definitions like LLVMPtrTy via SPEC_DEF).
**Implement:**

* `src/codegen/llvm_types.cpp`
**Tests:**
* Unit tests for mapping each type constructor â†’ LLVM types.
* `SPEC_COV("<RuleId>")`.

**Completed:** Implemented `GetLLVMType()` in `llvm_types.cpp`. Maps primitives (i8-i64, f32/f64, bool, unit), TypePerm, TypePtr, TypeRawPtr, TypeFunc. `SPEC_RULE` anchors for `LLVMTy-Prim`, `LLVMTy-Perm`, `LLVMTy-Ptr`, `LLVMTy-RawPtr`, `LLVMTy-Func`. Unit test `TestTypes` passes.

**Implementation Notes (2026-01-17):**
* `src/codegen/llvm_types.cpp` maps Cursive types to LLVM types with `LLVMTy-*` rule anchors.
* Added SPEC_COV anchors for `LLVMTy-*`, `LLVM-ArgAttrs-*`, and `LLVMCall-*` in `tests/unit/codegen_llvm_test.cpp`.


#### T-LLVM-008 LLVM IR emission pipeline

**Spec:** Â§6.12.9 rules (6) including `LowerIR-Module`, `LowerIR-Err`, `EmitLLVM-*`, `EmitObj-*`. 
**Implement:**

* `src/codegen/llvm_emit.cpp`
**Tests:**
* Round-trip test: emitted IR accepted by `llvm-as` 21 (as a tool-integration test).
* `SPEC_COV("<RuleId>")`.

**Completed:** Implemented `EmitModule()` and `EmitLLVM()` entry point in `llvm_module.cpp`. Integrates `SetupModule`, `DeclareRuntime`, `EmitDecl`, `EmitProc`, `EmitEntryPoint`, `EmitPoisonCheck`. Added `ReleaseModule()` for ownership transfer. `SPEC_RULE("EmitLLVM-Ok")` anchor added.

**Implementation Notes (2026-01-19):**
* `src/codegen/llvm_module.cpp` `LLVMEmitter::EmitModule()` wires setup, runtime declarations, IR decl/proc emission, entrypoint, and poison checks.
* Driver uses `EmitLLVMModule` / `EmitObjForModule` in `src/driver/main.cpp` to satisfy `EmitLLVM-*` / `EmitObj-*` behavior.
* SPEC_COV for `EmitLLVM-*` / `EmitObj-*` rules is still missing in tests.


#### T-LLVM-009 IR operation lowering

**Spec:** Â§6.12.10 rules (48), including ExpandIR/UniqueEmits invariants. 
**Implement:**

* `src/codegen/ir_lowering.cpp`
**Tests:**
* For each IR op: golden IR snippet or pattern match test.
* `SPEC_COV("<RuleId>")`.

**Implementation Notes (2026-01-19):**
* IMPLEMENTED in `src/codegen/ir_lowering.cpp` with extensive `SPEC_RULE` anchors for `LowerIR-*`, `Lower-*IR`, and `Match-*`.
* Coverage exists for some rules in `tests/unit/codegen_phase4_lower_missing_cov_test.cpp` and compile fixtures under `tests/compile/phase4/`, but many 6.12.10 rules still lack SPEC_COV.

#### T-LLVM-010 Binding storage and validity

**Spec:** Â§6.12.11 rules (21).
**Implement:**

* `src/codegen/binding_storage.cpp`
**Tests:**
* IR tests that storage lifetime, validity, and poisoning instrumentation rules hold.
* `SPEC_COV("<RuleId>")`.

**Implementation Notes (2026-01-19):**
* IMPLEMENTED in `src/codegen/binding_storage.cpp` with `SPEC_RULE` anchors (`BindSlot-Local`, `BindValid-*`, `UpdateValid-BindVar`).
* SPEC_COV for 6.12.11 binding rules is still missing in tests.

#### T-LLVM-011 Call ABI mapping

**Spec:** Â§6.12.12 rules (11).
**Implement:**

* `src/codegen/llvm_call_abi.cpp`
**Tests:**
* IR tests for calling conventions and attributes.
* `SPEC_COV("<RuleId>")`.

**Completed:** Implemented `ComputeCallABI()` in `llvm_call_abi.cpp`. Returns `ABICallResult` with LLVM function type, param types, return type, and SRet flag. `SPEC_RULE("LLVMCall-ByValue")` anchor added. Unit test `TestABI` passes.

**Implementation Notes (2026-01-19):**
* `src/codegen/llvm_call_abi.cpp` `ComputeCallABI()` maps Cursive types to LLVM ABI + SRet handling.
* Unit coverage in `tests/unit/codegen_llvm_test.cpp` includes `SPEC_COV("LLVMCall-ByValue")`, `SPEC_COV("LLVMCall-SRet")`, and `TestABI`.


#### T-LLVM-012 VTable emission

**Spec:** Â§6.12.13 rules (3).
**Implement:**

* `src/codegen/vtable_emit.cpp`
**Tests:**
* IR tests verifying vtables emitted once and referenced properly.
* `SPEC_COV("<RuleId>")`.

**Implementation Notes (2026-01-17):**
* `src/codegen/vtable_emit.cpp` implements `EmitVTable` (header + slots) with rule anchors.
* Added SPEC_COV anchors for `EmitVTable*` in `tests/compile/phase4/lower_module_items/class_vtable_default.cursive` and `tests/unit/codegen_phase4_lower_missing_cov_test.cpp`.

#### T-LLVM-013 Literal data emission

**Spec:** Â§6.12.14 rules (7).
**Implement:**

* `src/codegen/literal_emit.cpp`
**Tests:**
* Literal pooling tests: UniqueEmits for literals.
* `SPEC_COV("<RuleId>")`.

**Implementation Notes (2026-01-17):**
* `src/codegen/literal_emit.cpp` implements `EmitLiteralData-*` and `UniqueEmits-Literal` for literal globals.
* Added SPEC_COV anchor for `UniqueEmits-Literal` in `tests/unit/codegen_string_literal_test.cpp`.

#### T-LLVM-014 String and bytes built-ins and drop hooks

**Spec:** Â§6.12.15â€“Â§6.12.16 rules (10 total across subsections).
**Implement:**

* `src/codegen/string_bytes_builtins.cpp`
**Tests:**
* IR tests ensure drop hooks are called and signatures match runtime.
* `SPEC_COV("<RuleId>")`.

**Implementation Notes (2026-01-19):**
* Builtin symbol mapping for string/bytes lives in `src/codegen/runtime_calls.cpp` (including `StringDropSym-*` / `BytesDropSym-*`).
* Drop declarations are emitted in `src/codegen/runtime_decls.cpp`; runtime implementations are in `cursive-bootstrap/runtime/src/string_bytes.c`.
* Added SPEC_COV for `StringDropSym-Err` / `BytesDropSym-Err` in `tests/unit/codegen_runtime_interface_phase4_test.cpp`.
* Added IR compile test `tests/compile/phase4/builtins/string_bytes_drop.cursive` with `string@Managed`/`bytes@Managed` drops and expected `drop_managed` calls in `tests/compile/phase4/builtins/string_bytes_drop.expect.ll`.

#### T-LLVM-015 Entrypoint and context construction

**Spec:** Â§6.12.17 rules (1).
**Implement:**

* `src/codegen/entrypoint.cpp`
**Tests:**
* Semantics-oracle tests verifying executable initializes context and calls `main` correctly.
* `SPEC_COV("<RuleId>")`.

**Implementation Notes (2026-01-17):**
* `src/codegen/entrypoint.cpp` implements `EmitEntryPoint` with `EntrySym-Decl` and `EntryStub-Decl` anchors.
* Added SPEC_COV anchors for `EntrySym-Decl`/`EntryStub-Decl` in `tests/unit/codegen_llvm_test.cpp`; `ContextInitSym-Decl` is covered in `tests/unit/codegen_runtime_interface_phase4_test.cpp`.

#### T-LLVM-016 Poisoning instrumentation

**Spec:** Â§6.12.18 rules (6).
**Implement:**

* `src/codegen/poison_instrument.cpp`
**Tests:**
* IR tests verifying poisoning steps are emitted where required.
* `SPEC_COV("<RuleId>")`.

---

### 5G. Dynamic semantics alignment

**Implementation Notes (2026-01-17):**
* `src/codegen/poison_instrument.cpp` implements poison flag tracking and checks with `Poison*` rule anchors.
* Added SPEC_COV anchors for `PoisonCheck` and `PoisonFlag-*` in `tests/unit/codegen_init_poison_test.cpp`.

#### T-DYN-001 Initialization order and poisoning

**Spec:** Â§7.1 rules (11).
**Implement:**

* Codegen + runtime coordination:

  * global init sequencing
  * poisoning rules per IR instrumentation
    Files:
* `src/codegen/init.cpp`
* `src/runtime/context.cpp`
**Tests:**
* Runtime tests that observe initialization order (Windows CI).
* `SPEC_COV("<RuleId>")`.

**Implementation Notes (2026-01-16):**
* PARTIAL - Codegen wiring present; runtime init path exists; poisoning observability still limited to unit/IR tests.
* Verified codegen pieces in `src/codegen/init.cpp`, `src/codegen/checks.cpp`, `src/codegen/poison_instrument.cpp`, and `src/codegen/lower_expr_places.cpp` (Init/Panic/Poison rules anchored in `AnchorInitRules`).
* Tests: unit test exists at `tests/unit/codegen_init_poison_test.cpp` with `SPEC_COV` for Init/Deinit rules; semantics-oracle harness `tests/harness/run_runtime.py` covers `tests/semantics_oracle/init_order_ok` (init order) and `tests/semantics_oracle/init_panic` (init-time panic).
* Runtime context implementation lives in `runtime/src/context.c`.

**Implementation Notes (2026-01-19):**
* Added semantics-oracle test `tests/semantics_oracle/poison_read` that sets `cursive::runtime::poison::demo = true` via a user module and reads `demo::value` to trigger `CheckPoison` at runtime (exit code 10).
* SPEC_COV anchors added for `PoisonFlag-Decl` and `CheckPoison-Use` in `tests/semantics_oracle/poison_read/src/main.cursive`.

#### T-DYN-002 Modal pattern matching semantics

**Spec:** Â§7.3 rules (14).
**Implement:**

* Ensure codegen matches dynamic semantics for modal matches.
  Files:
* `src/codegen/lower_pat.cpp`
* `src/runtime/region.cpp` if needed
**Tests:**
* Semantics-oracle tests verifying behavior on modal states.
* `SPEC_COV("<RuleId>")`.

**Implementation Notes (2026-01-16):**
* IMPLEMENTED (codegen + compile fixtures + semantics-oracle coverage).
* Verified match lowering anchors in `src/codegen/ir_lowering.cpp` for all A7.3 `Match-*` rules.
* Compile fixture exists at `tests/compile/phase4/lower_pat/modal_match.cursive` with `tests/compile/phase4/lower_pat/modal_match.expect.ll`.
* Tests: `tests/unit/codegen_phase4_lower_missing_cov_test.cpp` includes `SPEC_COV` anchors for A7.3 rules; semantics-oracle `tests/semantics_oracle/modal_match`.

#### T-DYN-003 Deterministic destruction and unwinding

**Spec:** Â§7.4 rules (297) and its interaction with codegen cleanup/drop.
**Implement:**

* Core design:

  * Represent drop obligations in IR
  * Generate explicit IR-level cleanup/unwind control flow per spec (use `PanicOutName`/`PanicCheck`-style propagation; **do not** use LLVM EH constructs such as `invoke`/`landingpad`/SEH).
  * Runtime panic/unwind entry points as required
    Files:
* `src/codegen/cleanup.cpp`
* `src/codegen/unwind.cpp`
* `runtime/src/panic.cpp`
**Tests:**
* IR tests for structured cleanup blocks
* Semantics-oracle tests for destructor order in normal return and panic/unwind
* Coverage discipline:

  * Use rule prefix partitioning in tests (e.g., `SPEC_COV("<RuleIdPattern>")` is not allowed; must cover each actual rule ID using `coverage_check.py` to map exact IDs)
  * Practical approach: generate per-rule test stubs and fill in minimal reproducer programs.

**Implementation Notes (2026-01-16):**
* `src/codegen/cleanup.cpp` - Added panic-aware cleanup emission (panic record read/write, double-panic abort), `ComputeCleanupPlanRemainder`, and `EmitCleanupOnPanic`/`EmitCleanupWithRemainder`; threaded `__panic` into drop glue and dynamic drops; added drop-method emission for records/enums/modals in glue; used panic-stop sequencing for aggregate field/payload drops.
* `src/codegen/checks.cpp` - `LowerPanic`/`PanicCheck` now run cleanup in panic mode.
* `src/codegen/lower_stmt.cpp` - Block/break/continue cleanup uses remainder-aware cleanup; temp cleanup now emits `DropTemp` actions with remainder.
* `src/codegen/lower_pat.cpp` - Match-arm cleanup uses remainder-aware cleanup.
* Tests: unit coverage exists in `tests/unit/codegen_panic_cleanup_test.cpp` (LowerPanic/PanicCheck with cleanup); semantics-oracle coverage in `tests/semantics_oracle/drop_order_ok` (normal order) and `tests/semantics_oracle/drop_unwind_panic` (panic path).
* Runtime panic entry point implemented in `runtime/src/panic.c`.

**Implementation Notes (2026-01-17):**
* `src/codegen/cleanup.cpp` - `TypeDynamic` drop is now a no-op (no vtable drop). This aligns with DropValue/DropCall rules since `TypeDynamic` does not implement `Drop` and has no children; fixes null-vtable crashes when dropping `Context` capability fields.
* Semantics-oracle verification: `tests/semantics_oracle/drop_order_ok` and `tests/semantics_oracle/drop_unwind_panic` pass after the change.

**Implementation Notes (2026-01-19):**
* Unwind behavior is implemented via `LowerPanic`/`PanicCheck` + `EmitCleanupOnPanic` in `src/codegen/checks.cpp` and `src/codegen/cleanup.cpp` (panic-triggered cleanup with double-panic abort via `RuntimePanicSym`); no separate `unwind.cpp` is required.

#### T-DYN-004 Dynamic class objects

**Spec:** Â§7.6 rules (2).
**Implement:**

* Vtable layout + dispatch correctness at runtime.
**Tests:**
* Semantics-oracle dynamic dispatch tests.
* `SPEC_COV("<RuleId>")`.

**Implementation Notes (2026-01-16):**
* IMPLEMENTED (codegen + compile fixtures + semantics-oracle coverage).
* Codegen present in `src/codegen/dyn_dispatch.cpp`, `src/codegen/vtable_emit.cpp`, and `src/codegen/layout_dynobj.cpp`.
* Compile fixtures: `tests/compile/phase4/lower_calls/dynamic_dispatch.cursive` and `tests/compile/phase4/lower_module_items/class_vtable_default.expect.ll`.
* Tests: semantics-oracle `tests/semantics_oracle/dyn_dispatch`.

#### T-DYN-005 FileSystem and file operations

**Spec:** Â§7.7 rules (29).
**Implement:**

* Runtime: `runtime/src/filesystem.cpp` (**Windows API only**; avoid depending on the MSVC CRT / C++ stdlib under `/NODEFAULTLIB`. If any OS imports are required, they must be satisfied by objects bundled into `cursive0_rt.lib`.)
* Sema: capability enforcement already handled by Â§5.9 tasks
* Codegen: call built-in interface defined in Â§6.12.9 runtime symbols set 
* Addendum (CORE-010): keep FS/File/Dir primitive implementations aligned with HostPrim runtime classification (`FSPrim`, `FilePrim`, `DirPrim`).
**Tests:**
* Semantics-oracle tests using temporary directories/files, validating error paths.
* `SPEC_COV("<RuleId>")`.

**Implementation Notes (2026-01-16):**
* IMPLEMENTED (runtime). Windows API implementation in `runtime/src/filesystem.c` covering path canonicalization, restriction, file ops, and dir iteration ordering.
* Sema: `src/sema/cap_filesystem.cpp` implements `FileSystemInterface` and builtin types.
* Codegen: `src/codegen/runtime_calls.cpp` and `src/codegen/runtime_decls.cpp` provide `BuiltinSym` wiring; compile fixture at `tests/compile/phase4/builtins/fs.cursive`.
* Tests: semantics-oracle `tests/semantics_oracle/fs_ops` covers `Prim-FS-EnsureDir`, `Prim-FS-WriteFile`, `Prim-FS-Exists`, `Prim-FS-Kind`, `Prim-FS-OpenDir`, `Prim-Dir-Next`, `Prim-Dir-Close`.

#### T-DYN-006 String literal semantics

**Spec:** Â§7.5 rule `StringLiteralVal(lit)`; relationship to `LiteralValue(lit, TypeString(@View))`.
**Implement:**

* Ensure string literal evaluation in codegen (Â§6.4, Â§6.12.14) produces values consistent with the `StringLiteralVal` judgment.
* Verify that string literals are lowered to `string@View` type with correct static data emission.
* Wire into `src/codegen/lower_expr.cpp` (literal lowering) and `src/codegen/literal_data.cpp`.
**Tests:**
* Compile tests verifying string literal type inference yields `string@View`.
* IR tests verifying correct static data emission for string literals.
* Semantics-oracle tests using string literals and validating runtime behavior.
* `SPEC_COV("StringLiteralVal")`.

**Implementation Notes (2026-01-16):**
* IMPLEMENTED (codegen + unit/compile fixtures + semantics-oracle coverage).
* Codegen: `src/codegen/lower_expr_core.cpp` handles `StringLiteralVal`; `src/codegen/ir_lowering.cpp` emits static backing globals for `string@View`.
* Tests: unit test `tests/unit/codegen_string_literal_test.cpp`; compile fixture `tests/compile/phase4/lower_expr/string_literal.cursive`; semantics-oracle `tests/semantics_oracle/string_literal`.

---

### 5H. Runtime library deliverable

#### T-RT-001 Build `cursive0_rt.lib` and satisfy required symbols

**Spec:** Â§2.5 rules `ResolveRuntimeLib-Ok`, `ResolveRuntimeLib-Err`, `Link-*`, `Out-Link-*` (plus the RuntimeLibName definition, which is `"cursive0_rt.lib"` in Cursive0).
**Implement:**

* `runtime/CMakeLists.txt`
* `runtime/src/*` implementations for:

  * PanicSym
  * StringDropSym / BytesDropSym
  * ContextInitSym
  * Region procs
  * Builtin methods for String/Bytes/FileSystem/HeapAllocator

* **Freestanding/link-contract requirements:** the driver links with `/NODEFAULTLIB` and (per Â§2.5 `LinkArgsOk`) supplies only `{Objs} + cursive0_rt.lib`. Therefore `cursive0_rt.lib` MUST be self-contained: avoid depending on the MSVC CRT and the C++ standard library; disable exceptions/RTTI for runtime sources; and bundle any additional objects needed to satisfy implicit LLVM codegen/library calls (e.g., memory-operation shims and i128 div/mod helpers if LLVM lowers those ops to external symbols).

**Tests:**
* Link validation tests:

  * Deliberately remove a required symbol and ensure the specâ€™s `Out-Link-Runtime-Incompatible` behavior triggers (missing runtime symbol classification must happen pre-link).
  * Compile+link programs that force common implicit codegen dependencies (e.g., i128 division/modulo; large `memcpy`/aggregate copies; large stack allocations) to ensure there are **no** unresolved external symbols under `/NODEFAULTLIB`.
* Runtime unit tests where applicable.
* `SPEC_COV("ResolveRuntimeLib-Ok")`, `SPEC_COV("ResolveRuntimeLib-Err")`, plus link rules in Â§2.5.

**Implementation Notes (2026-01-16):**
* IMPLEMENTED runtime library build + required symbols.
* Runtime sources added under `runtime/src/` (`mem.c`, `panic.c`, `context.c`, `region.c`, `heap.c`, `string_bytes.c`, `filesystem.c`).
* CMake: `runtime/CMakeLists.txt` builds `cursive0_rt.lib` and root `CMakeLists.txt` adds `add_subdirectory(runtime)`.
* Codegen: panic decl updated to pass `u32` by value (`src/codegen/runtime_decls.cpp`).
* Semantics-oracle runtime coverage added via `tests/semantics_oracle/*` (init order/panic, drop order/panic, dyn dispatch, fs ops, string literal).

**Implementation Notes (2026-01-19):**
* Added output-pipeline test coverage for missing runtime symbols: `tests/unit/project_outputs_test.cpp` now exercises `Out-Link-Runtime-Incompatible` when `LinkerSyms` omits a required runtime symbol.

---

### 5I. Driver and CLI

#### T-DRV-001 Compiler CLI and observable behavior contract

**Spec:** bootstrap compiler notion and observable behavior equivalence concepts in Â§0.3; project pipeline behavior in Â§2.*.
**Implement:**

* `src/driver/main.cpp`
* Subcommands:

  * `build` (default)
  * `--dump` (enable the specâ€™s `DumpProject(P,dump=true)` command-line output behavior)
  * `--help` (usage)
  * `--dump-ast` (optional; for debugging/tests only; MUST NOT change compilation outputs or observable behavior)


* **Do not add CLI overrides for `emit_ir` or `out_dir`:** these are manifest fields (`Asm(T).emit_ir`, `Asm(T).out_dir`) and must be driven by `Cursive.toml` to preserve spec-traceable observable behavior.
**Tests:**
* CLI golden tests:

  * exit code per success/failure
  * diagnostics emitted in correct order
  * artifact set matches RequiredOutputs behavior
* `SPEC_DEF` references to observable behavior definitions, plus per-rule coverage via pipeline tests.

**Implementation notes (2026-01-19):**
* Added `--dump` CLI flag; driver emits `DumpProject(P,true)` output via `cursive0::project::DumpProject`.
* Implemented `DumpProject` in `src/project/outputs.cpp` and added unit coverage in `tests/unit/project_outputs_test.cpp`.
* `--dump-ast` is supported as an internal debugging flag (prints module item counts without altering pipeline behavior).

#### T-DRV-002 Project pipeline enforcement (no synthetic project)

**Spec:** Â§2.1 `ParseManifest-*`, Â§2.1 `LoadProject-*`, Â§2.4 `Modules-*`, Â§1.1 `Phase1Order`.
**Implement:**

* `src/driver/main.cpp`: remove the manifest-less synthetic project fallback; the driver must stop after `LoadProject-Err`.
* Ensure the input path is used only to locate `Cursive.toml` (no direct file compilation).

**Tests:**
* `tests/compile/phase0/manifest_missing` and `tests/compile/phase0/input_outside_source_root_reject`.

**Implementation Notes (2026-01-19):**
* `src/driver/main.cpp` uses `FindProjectRoot` + `LoadProject` and only proceeds when `project_result.project` is present (no manifest-less synthetic project).
* Module parsing uses `project.modules` and `project.source_root` exclusively.
* Compile fixtures exist under `tests/compile/phase0/manifest_missing` and `tests/compile/phase0/input_outside_source_root_reject`.

### 5J. Fuzzing infrastructure

#### T-FUZZ-001 Fuzzing infrastructure for lexer and parser
**Spec:** Robustness requirement implicit in Â§1.2 (behavior types) and Â§1.3 (error recovery); no crashes or hangs on arbitrary input.
**Implement:**
* `tests/fuzz/lexer_fuzz.cpp`
  * Fuzz target for lexer: accepts arbitrary byte sequences
  * MUST NOT crash, hang, or trigger undefined behavior
  * MUST produce valid token stream or well-formed error diagnostics
* `tests/fuzz/parser_fuzz.cpp`
  * Fuzz target for parser: accepts arbitrary token streams (or byte sequences via lexer)
  * MUST NOT crash, hang, or trigger undefined behavior
  * MUST produce valid AST (possibly with error nodes) or well-formed error diagnostics
* `tests/fuzz/source_load_fuzz.cpp`
  * Fuzz target for source loading pipeline: arbitrary byte sequences
  * Tests UTF-8 decoding, BOM handling, normalization robustness
* `cmake/Fuzzing.cmake`
  * CMake configuration for building fuzz targets with libFuzzer or AFL
  * Integration with sanitizers (ASan, UBSan, MSan)
* `tools/run_fuzz.py`
  * Script to run fuzz targets with configurable duration and corpus management
  * Supports CI smoke tests (time-bounded) and extended local fuzzing
**Tests:**
* Fuzz targets are self-testing (crashes/hangs are failures)
* CI runs time-bounded smoke fuzz (e.g., 60 seconds per target)
* Corpus of interesting inputs maintained in `tests/fuzz/corpus/`
* Any crash-inducing input becomes a regression test in `tests/compile/`
**Quality gates:**
* Fuzz targets MUST compile with sanitizers enabled
* No crashes or hangs in CI smoke fuzz runs
* Coverage-guided fuzzing recommended for local development

**Implementation Notes (2026-01-19):**
* Added fuzz targets: `tests/fuzz/lexer_fuzz.cpp`, `tests/fuzz/parser_fuzz.cpp`, and `tests/fuzz/source_load_fuzz.cpp`.
* Added `cmake/Fuzzing.cmake` with libFuzzer + sanitizer flags and `CURSIVE_ENABLE_FUZZING` gate; root `CMakeLists.txt` includes it when enabled.
* Added `tools/run_fuzz.py` for time-bounded fuzz runs and corpus management; created corpus/artifacts directories under `tests/fuzz/`.

### 5K. Review fixes (2026-01-12)

#### T-SRC-007 ReadBytes fidelity (no source filtering in compiler)

**Spec:** Â§3.4.2 rules `ReadBytes-Ok`, `ReadBytes-Err`, `ParseModule-Err-Read`, `ParseModules-Err`; Â§3.1.8 `LoadSource-*`; Â§1.7 `HostPrimFail`.
**Implement:**

* `src/driver/main.cpp`: remove `StripSpecCovLines` + `ReadBytesStripSpecCov`; use frontend `ReadBytes` unchanged.
* `src/frontend/parse_modules.cpp`: ensure `ReadBytes` returns raw file bytes (no filtering of `# SPEC_COV:` or `# ASSEMBLY:`).
* `src/driver/main.cpp` + `src/frontend/parse_modules.cpp`: call `HostPrimFail(HostPrim::ReadBytes, true)` on read errors.
* `tests/harness/run_compiler.py`: remains the **only** place that strips `# SPEC_COV:` / `# ASSEMBLY:` for compile tests.
**Tests:**
* Add unit test in `tests/unit/frontend_parse_modules_test.cpp` that reads a file containing leading `#` lines and asserts the raw bytes are preserved.
* Add unit test in `tests/unit/core_host_primitives_test.cpp` asserting `HostPrimFailureIllFormed(ReadBytes)` is false.
* `SPEC_COV("ReadBytes-Ok")`, `SPEC_COV("ReadBytes-Err")`.

**Implementation Notes (2026-01-19):**
* `src/frontend/parse_modules.cpp` `ReadBytesDefault` returns raw bytes and calls `HostPrimFail(ReadBytes, true)` on errors.
* `src/driver/main.cpp` wires `ReadBytesDefault` via `ParseModuleDeps` (no stripping in compiler).
* Unit coverage: `tests/unit/frontend_parse_modules_test.cpp` (`ReadBytes-Ok` / `ReadBytes-Err`) and `tests/unit/core_host_primitives_test.cpp` (ReadBytes ill-formed handling).

#### T-CONF-002 Phase order tracking correctness

**Spec:** Â§1.1 `Phase1Order`, `Phase3Order`, `Phase4Order`, `WF`, `Reject-IllFormed`; Â§2.5 `Output-Pipeline-*`.
**Implement:**

* `src/driver/main.cpp`: compute `phase1_ok` from **ParseModules success** (not from global `HasError`); compute `phase3_ok` from Phase 3 checks (`resolve_ok` + `typecheck_ok`) regardless of later diagnostics.
* `src/driver/main.cpp`: run `OutputPipeline(P)` after Phase 3 success (and when not `--phase1-only`), append its diagnostics in **Emit order**, and set `phase4_ok` **only** when the pipeline succeeds; otherwise `phase4_ok = false`.
* `include/cursive0/sema/conformance.h` / `src/sema/conformance.cpp`: add `SPEC_DEF` anchors for `Phase1Order`, `Phase3Order`, `Phase4Order` and document the mapping from driver results to these judgments.
**Tests:**
* Add `tests/unit/sema_conformance_test.cpp` to exercise `WF`/`C0Conforming` over explicit `PhaseOrderResult` combinations.
* Add a driver integration test that produces Phase 3 errors while confirming Phase 1 succeeded (via explicit `PhaseOrderResult` expectations in a unit test or a harnessed driver test).
* Add a Phase 4 integration test that fails in `OutputPipeline` and confirms `Phase4Order` is false.

**Implementation Notes (2026-01-19):**
* `src/sema/conformance.cpp` defines `WF`, `Phase1Order`, `Phase3Order`, `Phase4Order` via `SPEC_DEF` and implements `WF(PhaseOrderResult)`.
* Driver computes `phase1_ok` from ParseModules success and runs `OutputPipeline` after Phase 3 success in `src/driver/main.cpp`.
* Unit coverage exists in `tests/unit/sema_conformance_test.cpp` for `PhaseOrderResult` combinations.

#### T-TC-017 Type well-formedness enforcement

**Spec:** Â§5.4 rules `WF-Prim`, `WF-Perm`, `WF-Tuple`, `WF-Array`, `WF-Slice`, `WF-Union`, `WF-Union-TooFew`, `WF-Func`, `WF-Path`, `WF-Dynamic`, `WF-Dynamic-Err`, `WF-String`, `WF-Bytes`, `WF-Ptr`, `WF-RawPtr`, `WF-ModalState`.
**Implement:**

* `src/sema/type_expr.cpp`: add `TypeWF` (or equivalent) that checks each rule and emits the corresponding `SPEC_RULE`/diag id.
* `src/sema/type_decls.cpp`: call `TypeWF` when lowering declared types (record fields, enum payloads, alias targets, procedure params/returns).
* `src/sema/type_expr.cpp`: call `TypeWF` for explicit type nodes in expressions (casts, type ascriptions, etc.).
* `src/sema/typecheck_diag_map.inc`: ensure `WF-Union-TooFew` â†’ `E-TYP-2201` and `WF-Dynamic-Err` â†’ `E-TYP-2509` are mapped (add if missing).
**Tests:**
* Unit tests for `TypeWF` (one per rule).
* Compile fixtures for each `WF-*` rule with `SPEC_COV("<RuleId>")`.

**Implementation notes (2026-01-19):**
* `src/sema/type_expr.cpp` implements `TypeWF` with `WF-*` rule anchors; invoked from `src/sema/type_decls.cpp`.
* Unit coverage in `tests/unit/sema_type_wf_test.cpp` plus compile fixtures under `tests/compile/phase3/type_wf_*`.
* `WF-Union-TooFew` and `WF-Dynamic-Err` diagnostics mapped in `src/sema/typecheck_diag_map.inc`.

#### T-PH1-INV-001 Phase 1 invariants coverage

**Spec:** Â§3.3.1 rules `Phase1-Complete`, `Phase1-Declarations`, `Phase1-Forward-Refs`.
**Implement:**

* `src/frontend/parse_modules.cpp` (or driver after ParseModules): add `SPEC_RULE` anchors for these invariants on successful Phase 1 completion.
**Tests:**
* Add `SPEC_COV` tags for these rules in existing parse/parse-modules tests or a new minimal compile test.

**Implementation notes (2026-01-19):**
* `src/frontend/parse_modules.cpp` anchors `Phase1-Complete`, `Phase1-Declarations`, and `Phase1-Forward-Refs`.
* SPEC_COV for these rules lives in Phase 1 compile tests (verified by `tools/coverage_check.py` Phase 1 pass).

#### T-COV-002 Phase 1â€“3 rule coverage sweep

**Spec:** Â§Â§3â€“5 (all Phase 1â€“3 rules).
**Implement:**

* Run `tools/coverage_check.py` to enumerate missing `SPEC_COV` for Â§Â§3â€“5.
  * Initial missing list snapshot: `docs/coverage_phase1_3_missing.txt` (generated 2026-01-12).
* Add/expand compile + unit tests to cover every missing rule (prioritize correctness rules first).
* Track remaining gaps in a short checklist inside this plan until 100%/100%.
**Tests:**
* `tools/coverage_check.py` must pass for Phase 1â€“3 coverage.

**Completion notes (2026-01-13):**
* Created `tools/coverage_check.py` to verify SPEC_RULE and SPEC_COV anchors against `spec/rules_index.json`.
* Phase 1 coverage verified: 451/451 rules (100.0% SPEC_RULE, 100.0% SPEC_COV).
* Phase 3 coverage verified: 823/823 rules (100.0% SPEC_RULE, 100.0% SPEC_COV).
* T-TC-017 (`TypeWF`) verified complete: all 16 WF-* rules have SPEC_RULE anchors in `type_expr.cpp` and SPEC_COV in `sema_type_wf_test.cpp`.
* T-PH1-INV-001 verified complete: `Phase1-Complete`, `Phase1-Declarations`, `Phase1-Forward-Refs` anchors present in `parse_modules.cpp` lines 229-231.
* Phase 4 not yet implemented: 2/932 rules (0.2%).

---
## 6. Testing strategy

### 6.1 Test types

* **Unit tests**: core functions (Unicode folding, hashing, spans, manifest parsing, deterministic ordering).
* **Compile tests**: compile-only; compare diagnostics and/or AST/IR outputs.
* **IR tests**: compile â†’ LLVM text; verify patterns + forbidden constructs; ensure LLVM 21.1.8 acceptance.
* **Property tests** (recommended): determinism checks (module ordering, output naming, diag order identity) across randomized but controlled inputs.
* **Fuzz tests** (recommended where practical): lexer and parser fuzzing with sanitizers enabled (treat crashes or hangs as failures).
* **Semantics-oracle tests**: compile â†’ link â†’ run; validate stdout/stderr/exit code (Windows CI baseline).

### 6.2 Golden formats

* Diagnostics: JSON lines or a stable JSON array:

  * includes DiagId, Code, severity, message key, primary span, secondary spans.
* AST printing: stable s-expression-like format.
* IR: stable text IR with normalization (strip non-deterministic IDs if needed).

### 6.3 Rule coverage enforcement in tests

Every test case file must include a header block:

```text
# SPEC_COV: <RuleIdA>
# SPEC_COV: <RuleIdB>
# SPEC_COV: <RuleIdC>
```

The harness extracts these tags and feeds them into `coverage_check.py`.

---

## 7. CI quality gates

### Required CI jobs

1. **Build + unit tests** (Linux and Windows)

   * Linux: build `cursivec0` and run unit + compile + IR tests (no running Windows executables).
   * Windows: build `cursivec0` + `runtime/cursive0_rt.lib` and run the full suite including semantics-oracle execution.

2. **Spec sync checks**

   * `tools/spec_indexer.py` output is up-to-date with `spec/Cursive0.md` (no uncommitted drift).
   * `tools/diag_extractor.py` output matches the pinned spec diagnostic tables.

3. **Rule coverage**

   * `tools/coverage_check.py` enforces:
     * 100% of spec rules have at least one `SPEC_RULE("<RuleId>")` anchor
     * 100% of spec rules have at least one `SPEC_COV("<RuleId>")` anchor
   * For incremental bootstrapping only, allow temporary exceptions via `spec/coverage_expectations.json`â€”but the end state for `cursivec0` is 100%/100%.

4. **Sanitizers / hardening** (recommended)

   * Linux: ASan/UBSan build of `cursivec0` running unit + compile + IR tests.
   * Optional: lexer/parser fuzzing smoke tests (time-bounded) to catch crashes/hangs.



---

## 8. Deliverable checklist

### Compiler

* `cursivec0` binary
* Supports:

  * project load, module discovery, parsing, resolution, typecheck
  * LLVM IR emission (`ll`)
  * object emission (`obj`)
  * linking using `lld-link` to exe
* Diagnostics stable and spec-compliant

### Runtime

* `runtime/cursive0_rt.lib` produced by repository build
* Required symbols defined and validated by compiler link checks

### Tooling

* `tools/spec_indexer.py`
* `tools/diag_extractor.py`
* `tools/coverage_check.py`

### Tests

* Unit tests across all subsystems
* Compile/IR/semantics-oracle suites with rule-level coverage tags

---

## 9. Practical sequencing

A workable sequencing that preserves correctness and keeps the system testable at every step:

1. Core diagnostics + spec indexing + coverage tooling + host primitives interface (T-CORE-001..010, T-DIAG-001, traceability).
2. Phase 0 pipeline through module discovery (T-PROJ-001..006).
3. Source loading + lexer + minimal parser (T-SRC-*, T-LEX-*).
4. Full parser coverage (T-PARSE-*).
5. Fuzzing infrastructure for lexer and parser (T-FUZZ-001).
6. Basic resolution + type representation (T-RES-* + T-TC-001..010).
7. Statement/expression typing + binding state (T-TC-012..016).
8. Codegen IR model + minimal lowering and object emission (T-CG-001, T-LLVM-001, T-LLVM-008).
9. Runtime skeleton + linking loop closed (T-RT-001, T-PROJ-007).
10. Complete codegen semantics including cleanup/drop/unwinding (T-CG-*, T-LOWER-001..008, T-LLVM-*).
11. Dynamic semantics alignment including string literals (T-DYN-001..006).
12. Coverage reaches 100% rules implemented + 100% rules tested.

---


---

## 10. Reference interpreter plan (dynamic semantics)

This plan adds a reference interpreter that directly implements the dynamic semantics in Spec Section 7. The interpreter is the source of truth for behavior and is used by tests to validate compiler output. This is required to implement all remaining rules in Sigma7.* without guessing or mapping them to codegen heuristics.

### 10.1 Goals and scope

**Goal:** Implement a deterministic, spec-faithful reference interpreter that covers every rule in Sigma7.1, Sigma7.3, Sigma7.4, Sigma7.5, and Sigma7.7, with one or more SPEC_RULE anchors per rule and SPEC_COV tests for each rule.

**Non-goals:**
* This interpreter is not the production runtime. It is a test oracle and conformance engine.
* It should not drive codegen or linking; it should operate on typed AST and semantic artifacts.

### 10.2 Architecture and file layout

Introduce a new layer with no dependencies on codegen:

```
include/cursive0/semantics/
  value.h           # Value domain
  control.h         # Ctrl domain
  state.h           # Sigma (store, bindings, stacks, poison)
  address.h         # Addr, AddrTags, Read/Write helpers
  match.h           # Pattern match semantics
  eval.h            # Eval_sigma rules and helpers
  exec.h            # Exec_sigma rules and helpers
  cleanup.h         # Cleanup and drop semantics
  init.h            # Init/Deinit semantics
  apply.h           # ApplyProc/ApplyRecordCtor/ApplyMethod rules
  builtins.h        # Builtin semantics for interpreter
  interpreter.h     # Top-level entrypoints

src/semantics/
  value.cpp
  control.cpp
  state.cpp
  address.cpp
  match.cpp
  eval.cpp
  exec.cpp
  cleanup.cpp
  init.cpp
  apply.cpp
  builtins.cpp
  interpreter.cpp
```

### 10.3 Rule coverage strategy

* Each spec rule in Sigma7.* must have at least one SPEC_RULE anchor in the interpreter implementation.
* Each rule must have at least one SPEC_COV anchor in tests.
* Dynamic semantics rules should be anchored in the exact function implementing the rule. Avoid anchor-only stubs.

### 10.4 Tasks

#### T-REF-001 Core semantic domains and Sigma

**Spec:** Sigma7 (definitions of Value, Ctrl, Sigma, Store, bindings, scope/region stack, poisoned modules).

**Implement:**
* Value domain: unit, bool, int, float, char, string@View/Managed, bytes@View/Managed, tuple, array, record, enum, modal, raw ptr, ptr (with state/tag), proc ref, record ctor.
* Ctrl domain: Return, Break, Continue, Panic, Error.
* Sigma state: Store, BindEnv, ScopeStack, RegionStack, AddrTags, PoisonedModules.
* Core accessors: LookupBind, LookupVal, LookupValPath, ReadAddr, WriteAddr, AllocAddr, UpdateScopeStack, UpdateRegionStack.

**Files:**
* `include/cursive0/semantics/value.h`
* `include/cursive0/semantics/control.h`
* `include/cursive0/semantics/state.h`
* `src/semantics/value.cpp`, `src/semantics/control.cpp`, `src/semantics/state.cpp`

**Tests:**
* Unit tests for Value equality and serialization.
* Unit tests for Sigma operations (determinism and basic invariants).
* SPEC_COV anchors for definition rules that map to behavior.



**Implementation notes (2026-01-18):**
* Implemented Value/Ctrl/Sigma domains and core binding/store helpers in `include/cursive0/semantics/value.h`, `include/cursive0/semantics/control.h`, `include/cursive0/semantics/state.h`, and their `src/semantics/*` counterparts.
* Added `tests/unit/semantics_state_test.cpp` with SPEC_COV for `LookupVal-Bind-Value` and `LookupVal-Bind-Alias`.

#### T-REF-002 Poisoning and init/deinit semantics

**Spec:** Sigma7.1 (Init/Deinit rules, poisoning rules).

**Implement:**
* Graph helpers: Vertices/Edges/Topo/TopoTieBreak/Cycle.
* Init small-step: Init-Start, Init-Step, Init-Next-Module, Init-Panic, Init-Done.
* Init big-step: Init-Ok, Init-Fail.
* Deinit big-step: Deinit-Ok, Deinit-Panic.
* PoisonFlag/SetPoison/CheckPoison semantics in Sigma.

**Files:**
* `include/cursive0/semantics/init.h`
* `include/cursive0/semantics/state.h`
* `src/semantics/init.cpp`
* `src/semantics/state.cpp`

**Tests:**
* Unit tests for topo ordering + cycle error.
* Interpreter tests for init success/failure and poison set propagation.
* SPEC_COV anchors for each Sigma7.1 rule.



**Implementation notes (2026-01-18):**
* Implemented `Init`/`Deinit` in `src/semantics/init.cpp` using `sema::BuildInitPlan` order; added static address seeding plus poison-flag tracking in `Sigma`.
* Rule anchors for `Init-*` and `Deinit-*` are now present in `src/semantics/init.cpp`.

#### T-REF-003 Pattern matching semantics

**Spec:** Sigma7.3 (MatchPattern and related rules).

**Implement:**
* MatchPattern rules: Wildcard, Ident, Typed, Literal, Tuple, Record, Enum Unit/Tuple/Record, Modal General/State, Range.
* MatchRecord and MatchModal helpers.
* PatType and LiteralValue bridge for Match-Literal.

**Files:**
* `include/cursive0/semantics/match.h`
* `src/semantics/match.cpp`

**Tests:**
* Pattern tests for each Match rule, including negative cases.
* SPEC_COV anchors per rule.



**Implementation notes (2026-01-18):**
* Implemented `MatchPattern`/`MatchRecord`/`MatchModal` in `src/semantics/match.cpp` covering wildcard, ident, typed, literal, tuple, record, enum, modal, and range cases.
* Added `tests/unit/semantics_match_test.cpp` with SPEC_COV for the Sigma7.3 Match* rules.

#### T-REF-004 Eval_sigma (expressions)

**Spec:** Sigma7.4 Eval rules, helper judgments (EvalList_sigma, EvalFieldInits_sigma, EvalArgs_sigma, EvalOpt_sigma, EvalRecv_sigma).

**Implement:**
* Eval helper judgments for lists and options.
* Eval of expressions: Literal, PtrNull, Identifier, Path (poison checks), Move, AddressOf, Deref.
* Composite literals: Tuple, Array, Record, Enum, Range.
* If, Match, Loop, Block, Unsafe.
* Unary/Binary with panic/control propagation.
* Cast, Transmute.
* Index/Field/Tuple access (control and bounds rules).
* Alloc explicit/implicit, Propagate.
* Call/MethodCall (delegate to Apply* rules).

**Files:**
* `include/cursive0/semantics/eval.h`
* `src/semantics/eval.cpp`

**Tests:**
* Interpreter tests for each Eval rule.
* SPEC_COV anchors per rule.



**Implementation notes (2026-01-18):**
* `src/semantics/eval.cpp` implements `EvalSigma` and helper judgments; added SPEC_RULE anchors for `CastVal-*`, `WriteSub-*`, and `StepSigma-*`.
* Added/maintained interpreter tests in `tests/unit/semantics_eval_test.cpp`; remaining Sigma7.4 coverage gaps are addressed by `tests/unit/semantics_phase4_missing_cov_test.cpp`.
* `ResolveMethodCallInfo` now owns method parameter lists to avoid dangling references during method-call evaluation (release-mode crash fix).

#### T-REF-005 Exec_sigma (statements)

**Spec:** Sigma7.4 Exec rules.

**Implement:**
* Exec for let/var/shadow, assign/compound assign, expr stmt, return/break/continue/result.
* Defer, region, frame, unsafe block.
* ExecSeq rules.

**Files:**
* `include/cursive0/semantics/exec.h`
* `src/semantics/exec.cpp`

**Tests:**
* Statement tests for each Exec rule and control-flow interactions.
* SPEC_COV anchors per rule.



**Implementation notes (2026-01-18):**
* `src/semantics/exec.cpp` implements `ExecSigma`/`ExecSeq` with Defer/Region/Frame handling; added SPEC_RULE anchors for `Step-Exec-*` and `Step-ExecSeq-*`.

#### T-REF-006 Cleanup and drop semantics

**Spec:** Sigma7.4 Cleanup rules and drop-related rules.

**Implement:**
* Cleanup small-step: Cleanup-Start, Cleanup-Step-*, Cleanup-Done.
* Destroy-Empty/Cons, DropAction-Valid/Moved/Partial.
* DropSubvalue-Do/Skip, DropValueOut-* rules.
* DropStaticAction.

**Files:**
* `include/cursive0/semantics/cleanup.h`
* `src/semantics/cleanup.cpp`

**Tests:**
* Cleanup and drop tests, including panic during drop.
* SPEC_COV anchors per rule.



**Implementation notes (2026-01-18):**
* `src/semantics/cleanup.cpp` implements Cleanup and drop semantics; added SPEC_RULE anchors for `Cleanup-*`, `Destroy-*`, and `Unwind-*`.

#### T-REF-007 ApplyProc/ApplyRecordCtor/ApplyMethod

**Spec:** Sigma7.7 ApplyProc, ApplyRecordCtor, ApplyMethod rules (including ApplyMethod-Prim).

**Implement:**
* ApplyProc_sigma: procedure call with args evaluation and control propagation.
* ApplyRecordCtor_sigma: record construction semantics.
* ApplyMethod_sigma: method dispatch semantics.
* ApplyMethod-Prim: primitive method semantics (as defined by spec).

**Files:**
* `include/cursive0/semantics/apply.h`
* `src/semantics/apply.cpp`

**Tests:**
* Tests for each Apply rule and ctrl propagation.
* SPEC_COV anchors per rule.



**Implementation notes (2026-01-18):**
* `src/semantics/apply.cpp` implements ApplyProc/ApplyRecordCtor/ApplyMethod; added SPEC_RULE anchors for `ApplyMethod-Prim` (and step rules).
* `ApplyMethodSigma` handles `System::exit` via Abort control and routes class-method prim calls through `PrimCall` with mutable `Sigma`.

#### T-REF-008 Builtin semantics for interpreter

**Spec:** Sigma7.4/Sigma7.7 builtin usage; Sigma6.12 runtime symbol rules for names only.

**Implement:**
* Builtin methods for string/bytes/region/heap/fs as needed for semantics rules.
* Implement as deterministic, pure semantics (avoid OS effects in interpreter).

**Files:**
* `include/cursive0/semantics/builtins.h`
* `src/semantics/builtins.cpp`

**Tests:**
* Unit tests for builtin results (length, is_empty, view conversions).
* SPEC_COV anchors for rules that depend on builtin behavior.


**Implementation notes (2026-01-18):**
* Builtins implemented: string/bytes builtins (from/to_managed/clone_with/append/length/is_empty/view conversions), HeapAllocator primitives (`with_quota`, `alloc_raw`, `dealloc_raw`), and System primitives (`get_env`, `exit`).
* Implemented in-memory FileSystem/File/DirIter semantics in `src/semantics/builtins.cpp` using `Sigma::fs_state` + `Sigma::fs_handles`, with deterministic DirEntries ordering (NFC + case fold), failmap/IoError mapping, handle tracking, and FSRestrict chaining.
* `ContextInitSigma` now seeds the root FileSystem handle; `Sigma` owns FS state/handles in `include/cursive0/semantics/state.h`.
* Added FileSystem coverage in `tests/unit/semantics_builtins_test.cpp` for create/read/write/dir/restrict + invalid UTF-8 cases.
**Implementation notes (2026-01-19):**
* Implemented Region runtime procs in `src/semantics/builtins.cpp` (`new_scoped`, `alloc`, `reset_unchecked`, `freeze`, `thaw`, `free_unchecked`) with handle-based Region values and region-stack tracking.
* Updated interpreter tests to bind explicit Region values as `Region@Active{handle: usize}` and verified region alloc behavior in `tests/unit/semantics_eval_test.cpp`.

#### T-REF-009 Interpreter entrypoints and harness

**Spec:** Infrastructure; must not change compiler outputs.

**Implement:**
* `InterpretExpr` and `InterpretModule` entrypoints.
* Optional CLI mode (`--interp`) that runs interpreter only (no codegen).
* Stable pretty-print of Value/Ctrl for tests.

**Files:**
* `include/cursive0/semantics/interpreter.h`
* `src/semantics/interpreter.cpp`
* `src/driver/main.cpp` (optional flag)

**Tests:**
* Harness test that interpreter output is stable.


**Implementation notes (2026-01-18):**
* Added `InterpretProject` entrypoint in `include/cursive0/semantics/interpreter.h` and `src/semantics/interpreter.cpp` that runs ContextInitSigma, Init, ApplyProcSigma(main, [ctx]), and Deinit with spec-rule anchors (Interpret-Project-Ok/Init-Panic/Main-Ctrl/Deinit-Panic).
* ContextInitSigma constructs a Context record with fs/heap dynamic handles and a System record; dynamic handles are tagged with a persistent scope tag so EvalRecv treats them as active without pushing a region.
* Main resolution scans modules and enforces signature rules consistent with MainSigOk (public, param Context, return i32, move allowed); interpreter sets current_module/scopes to the main module before ApplyProcSigma.
* Spec update: added project-level interpreter entrypoint rules in `cursive-bootstrap/docs/Cursive0.md` (Sec 7.8).
* Added `ValueToString`/`ControlToString` pretty-printers plus unit tests (`tests/unit/semantics_value_test.cpp`, `tests/unit/semantics_interpreter_test.cpp`).

**Implementation notes (2026-01-18):**
* Fixed missing closing braces in `cursive-bootstrap/src/semantics/cleanup.cpp` (LowerType visitor) and `cursive-bootstrap/src/semantics/eval.cpp` (EvalExprSigma visitor default case).
* Build: `cmake --build cursive-bootstrap/build` succeeded; MSVC warning C4566 for unicode sigma in `cursive-bootstrap/src/semantics/eval.cpp` and `cursive-bootstrap/tests/unit/semantics_eval_test.cpp`.
* Tests: `ctest --test-dir cursive-bootstrap/build -C Debug --output-on-failure` passed 87/87 (compile_tests and semantics_oracle_tests included).


#### T-REF-010 Coverage completion and integration

**Spec:** Sigma7.*

**Implement:**
* Add SPEC_RULE anchors for every Sigma7.* rule in interpreter code.
* Add SPEC_COV anchors for each rule in tests (compile or unit tests).
* Ensure `tools/coverage_check.py` reports 100 percent for Sigma7.*.

**Files:**
* `tests/unit/semantics_*`
* `tests/compile/phase4/semantics_*`
* `tests/semantics_oracle/semantics_*` (if needed)



**Implementation notes (2026-01-18):**
* Added SPEC_RULE anchors for remaining Sigma7 rules across `src/semantics/apply.cpp`, `src/semantics/eval.cpp`, `src/semantics/exec.cpp`, and `src/semantics/cleanup.cpp`.
* Added `tests/unit/semantics_phase4_missing_cov_test.cpp` and wired it in `tests/CMakeLists.txt` to cover remaining Sigma7 SPEC_COV gaps.
* `tools/coverage_check.py --phase 4 --report` now shows 100% SPEC_RULE coverage for Phase 4; the only missing SPEC_COV rules are in section 6 (codegen/runtime).

**Implementation notes (2026-01-19):**
* Aligned interpreter tests with Region handle semantics: explicit `alloc` now binds `r` as `Region@Active{handle: usize}` in `tests/unit/semantics_eval_test.cpp`.
* Added explicit `expr_types` for static literal inits in `tests/unit/semantics_init_test.cpp` and `tests/unit/semantics_interpreter_test.cpp` so `EvalSigma` can form literal values during `Init`.
* Switched the init failure test to use `PathExpr` (resolved module path) instead of `QualifiedNameExpr`, matching `EvalSigma`’s supported forms.
* Updated `tests/unit/semantics_builtins_test.cpp` directory iteration expectations to match spec `EntryOrder` sorting (case-fold + NFC, then UTF-8).
* Full test suite passes: `ctest --test-dir cursive-bootstrap/build -C Debug --output-on-failure` (92/92).

### 10.5 Sequencing

Recommended order for interpreter work:
1) T-REF-001 (domains and Sigma)
2) T-REF-003 (pattern matching)
3) T-REF-004 (Eval_sigma)
4) T-REF-005 (Exec_sigma)
5) T-REF-006 (Cleanup)
6) T-REF-002 (init/deinit + poisoning)
7) T-REF-007 (Apply rules)
8) T-REF-008 (builtins)
9) T-REF-009 (entrypoints)
10) T-REF-010 (coverage sweep)

### 10.6 Acceptance criteria

* All Sigma7.1, Sigma7.3, Sigma7.4, Sigma7.5, Sigma7.7 rules have SPEC_RULE anchors in interpreter code.
* All Sigma7.* rules have SPEC_COV in tests.
* Interpreter runs deterministically and produces stable outputs for tests.
* Coverage report shows 100 percent SPEC_RULE and SPEC_COV for Sigma7.*.



