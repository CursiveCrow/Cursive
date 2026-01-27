---
active: true
iteration: 1
max_iterations: 150
completion_promise: "SPEC-TESTS-COMPLETE"
started_at: "2026-01-27T05:37:13Z"
---

Create a comprehensive, systematic compiler test suite for cursivec0 following industry-standard practices (Rust compiletest, LLVM lit). The test suite MUST validate ALL semantics and diagnostics from @cursive-bootstrap/docs/Cursive0.md.

DIRECTORY STRUCTURE:

cursive-bootstrap/spec-tests/
  runner/
    run_tests.py              - Main test harness (Python 3.10+)
    filecheck.py              - Inline directive parser
    config.toml               - Harness configuration
    README.md                 - Harness documentation
  ui/                         - Compile-time diagnostic tests
    E-PRJ/                    - Project errors
    E-MOD/                    - Module errors
    E-SRC/                    - Source/lexing/parsing
    E-CNF/                    - Conformance errors
    E-UNS/                    - Unsupported constructs
    E-MEM/                    - Memory/provenance
    E-CON/                    - Concurrency/keys
    E-TYP/                    - Type errors
    E-SEM/                    - Semantic errors
    E-OUT/                    - Output/linking
    E-SYS/                    - System errors
    W-CNF/                    - Conformance warnings
    W-MOD/                    - Module warnings
    W-SRC/                    - Source warnings
    W-SEM/                    - Semantic warnings
    W-CON/                    - Concurrency warnings
    W-SYS/                    - System warnings
  run-pass/                   - Compile AND execute successfully
    primitives/               - all primitive types
    operators/                - arithmetic, logical, bitwise
    control-flow/             - if, match, loop, break, continue
    bindings/                 - let, var, :=, =, shadow
    procedures/               - calls, return, recursion
    records/                  - fields, methods, literals
    enums/                    - discriminants, patterns
    tuples/                   - construction, indexing
    arrays/                   - fixed-size, indexing
    slices/                   - views
    unions/                   - matching, propagation
    pointers/                 - Ptr, address-of, deref
    permissions/              - const, unique, shared
    modals/                   - states, transitions, widen
    strings/                  - string, bytes, states
    memory/                   - regions, frames, alloc
    generics/                 - type parameters, bounds
    classes/                  - implementation, dispatch
    capabilities/             - Context, FileSystem, HeapAllocator
    parallelism/              - parallel, spawn, dispatch, wait
    keys/                     - hash blocks, acquisition
    async/                    - suspension, composition
    ffi/                      - extern, FfiSafe
    contracts/                - pre/post conditions
  run-fail/                   - Compile OK, runtime panic expected
    P-TYP/                    - Runtime type panics
    P-SEM/                    - Runtime semantic panics
  codegen/                    - IR verification tests
    layout/                   - type layouts
    abi/                      - calling conventions
    symbols/                  - mangling, linkage
  auxiliary/                  - Shared test libraries
    runtime/
      cursive0_rt.lib         - Symlink or copy

TEST FILE FORMAT:

Single-file tests use .cursive files with inline directives in comments.

DIRECTIVE SYNTAX (in source comments):

// RUN: compile-pass     - Must compile without errors
// RUN: compile-fail     - Must fail compilation
// RUN: run-pass         - Must compile and run (exit 0)
// RUN: run-fail         - Must compile, run, and panic
// DIAG: E-XXX-NNNN      - Expected diagnostic code
// DIAG-NOT: E-XXX-NNNN  - Diagnostic must NOT appear
//~ ERROR E-XXX-NNNN: msg - Error on THIS line with message substring
//~ WARN W-XXX-NNNN: msg  - Warning on THIS line
// STDOUT: text          - Expected stdout contains text
// EXIT: N               - Expected exit code
// SPEC: X.Y.Z           - Spec section reference
// DESC: description     - Human-readable test description
// SKIP: reason          - Skip test with reason
// XFAIL: reason         - Expected failure (known bug)

TEST RUNNER FEATURES:
- Discovery: Recursively find all .cursive files and project dirs
- Compilation: Invoke cursivec0.exe with appropriate flags
- Execution: Run compiled executables for run-pass/run-fail
- Validation: Check diagnostics, exit codes, output
- Reporting: JUnit XML, TAP, and human-readable formats
- Filtering: --filter for path patterns
- Parallelism: --jobs N for concurrent test execution
- Coverage: Track diagnostic codes hit vs total
- Baseline: --bless to update expected outputs

COVERAGE REQUIREMENTS:

Phase 1 - Diagnostic Code Coverage (MANDATORY):
Create ONE test per diagnostic code from the spec diagnostics section:
- E-PRJ-0101 through E-PRJ-0305 (15 codes)
- E-MOD-1104 through E-MOD-2455 (30+ codes)
- E-OUT-0401 through E-OUT-0418 (18 codes)
- E-SRC-0101 through E-SRC-0521 (15 codes)
- E-CNF-0401 through E-CNF-0406 (6 codes)
- E-UNS-0101 through E-UNS-0108 (8 codes)
- E-MEM-1206 through E-MEM-3030 (15 codes)
- E-CON-0001 through E-CON-0095 (40+ codes)
- E-TYP-1505 through E-TYP-2631 (100+ codes)
- E-SEM-* (all semantic errors)
- W-* (all warning categories)
- P-TYP-* and P-SEM-* (runtime panics)

Name pattern: {category}/{code}.cursive (e.g., ui/E-TYP/E-TYP-1901.cursive)

Phase 2 - Semantic Coverage (MANDATORY):
For each run-pass category, test:
- Basic functionality (happy path)
- Boundary values and limits
- Type combinations
- Permission interactions
- Error recovery paths

Phase 3 - Edge Cases:
- Empty constructs (empty record, empty enum, zero-length array)
- Maximum nesting depths
- Unicode identifiers (valid and invalid)
- Large literals (max int values)
- Deep recursion

VALIDATION CHECKLIST:

Before outputting completion promise:
1. All diagnostic codes from spec have corresponding tests
2. Test runner discovers and executes all tests
3. Coverage report shows diagnostic code hit rate
4. run-pass tests for all primitive types
5. run-pass tests for all operators
6. run-pass tests for all control flow constructs
7. run-pass tests for all type constructors
8. run-pass tests for all permission combinations
9. ui tests verify exact diagnostic codes and messages
10. run-fail tests for all P-TYP and P-SEM codes
11. No test has SKIP or XFAIL without documented reason
12. README.md documents test format and runner usage

Output SPEC-TESTS-COMPLETE when ALL of the above are satisfied.
