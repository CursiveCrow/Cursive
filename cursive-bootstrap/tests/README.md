# Cursive0 Test Suite

## Layout

```
tests/
├── compile/              # Compiler phase tests
│   ├── Cursive.toml      # Shared manifest for single-module tests
│   ├── phase0/           # Manifest validation tests (per-case manifests)
│   ├── src/              # Single-module tests (use shared manifest)
│   │   ├── phase1/       # Lexing and parsing tests
│   │   ├── phase2/       # Early semantic analysis tests
│   │   ├── phase3/       # Type checking and semantic analysis tests
│   │   └── phase4/       # Code generation tests
│   └── projects/         # Multi-module tests (own manifests)
├── e2e/                  # Runtime tests (project dirs with expect.json)
├── unit/                 # C++ unit tests
├── fuzz/                 # Fuzzing tests
└── harness/              # Python test runners and utilities
```

## Test Types

### Single-Module Tests (`compile/src/`)

Use for tests that only need one `.cursive` file:

- Place in the appropriate phase directory under `compile/src/`
- No `Cursive.toml` needed (uses shared manifest)
- Name the file descriptively: `<test_name>.cursive`

### Multi-Module Tests (`compile/projects/`)

Use for tests that require multiple modules or a custom project structure:

- Create a directory under `compile/projects/phase<N>/`
- Include a `Cursive.toml` manifest
- Use standard project layout with `src/` directory

### E2E Runtime Tests (`e2e/`)

Use for tests that verify runtime behavior:

- Create a project directory with `Cursive.toml`
- Include `expect.json` at project root with expected `stdout`, `stderr`, `exit_code`
- Source files go in `src/`

## Phase3 Category Reference

Tests in `compile/src/phase3/` are organized into semantic categories:

| Category | Description |
|----------|-------------|
| `arrays_slices/` | Array and slice type tests |
| `builtins/` | String, bytes, and builtin function tests |
| `capabilities/` | Capability system (heap, filesystem, context) tests |
| `classes/` | Class definition, linearization, and member tests |
| `conformance/` | Language conformance and reserved name tests |
| `declarations/` | Declaration typing, visibility, and return tests |
| `drop/` | Drop trait and destructor tests |
| `dynamic/` | Dynamic dispatch tests |
| `enums/` | Enum and discriminant tests |
| `implementations/` | Class implementation tests |
| `memory/` | Pointers, regions, provenance, transmute tests |
| `methods/` | Method lookup and receiver permission tests |
| `modals/` | Modal type state, transitions, and widening tests |
| `module/` | Module resolution, visibility, and using tests |
| `ownership/` | Binding state, move semantics, bitcopy tests |
| `patterns/` | Pattern matching tests |
| `permissions/` | Const/unique permission tests |
| `records/` | Record type and method tests |
| `regression/` | Regression tests for fixed bugs |
| `resolve/` | Name resolution tests |
| `semantics/` | General semantic analysis tests |
| `types/` | Type checking, bounds, equivalence, inference tests |
| `unsupported/` | Tests for unsupported C0 constructs |
| `unwind/` | Unwind/panic tests |

## Naming Conventions

### File Naming

Use descriptive file names that indicate what the test covers:

- `<feature>_ok.cursive` - Tests that should compile successfully
- `<feature>_err.cursive` - Tests that should produce error diagnostics
- `<feature>_warn.cursive` - Tests that should produce warning diagnostics

Examples:
- `transition_ok.cursive` - Modal transition that should succeed
- `move_after_move_err.cursive` - Double-move that should error
- `widen_large_payload_warn.cursive` - Large payload widening warning

### Directory vs File Organization

**Preferred**: Use descriptive file names in category directories:
```
modals/
├── transition_ok.cursive
├── transition_ok.cursive.expect.json
├── widen_err.cursive
└── widen_err.cursive.expect.json
```

**Only use subdirectories** when:
- The test requires multiple source files
- The test requires a custom `Cursive.toml`
- The test has a special project structure

## Expectations

### Compile Diagnostics (`*.cursive.expect.json`)

```json
{
  "diagnostics": [
    {
      "code": "E-SEM-1234",
      "severity": "error",
      "message": "Description of the error",
      "span": {
        "file": "/path/to/test.cursive",
        "start_line": 5,
        "start_col": 10,
        "end_line": 5,
        "end_col": 20
      }
    }
  ],
  "exit_code": 1
}
```

### Compile IR (`*.expect.ll`)

LLVM IR output for phase4 code generation tests. Exact match after normalization.

### E2E Output (`expect.json`)

```json
{
  "stdout": "expected output",
  "stderr": "",
  "exit_code": 0
}
```

## SPEC_COV Requirements

All `.cursive` test files MUST include `# SPEC_COV: <rule_id>` comments near
the top of the file. This links tests to specification rules for coverage tracking.

```cursive
# SPEC_COV: ModalTransition-SourceState
# SPEC_COV: ModalTransition-TargetState
// Test that modal transitions validate source and target states
modal Counter { ... }
```

Use one or more `SPEC_COV` lines as needed to document all spec rules tested.

## Disabled Tests

To disable a test:

1. Rename the file to `*.cursive.disabled`
2. Add a reason comment in the file header or a nearby README
3. The harness only discovers `*.cursive` files

## Canonical Harness Entrypoints

- `harness/run_compiler.py` - Run compiler diagnostic tests
- `harness/run_e2e.py` - Run end-to-end runtime tests
- `harness/run_semantics_oracle.py` - Compare compiler vs interpreter output

## Regenerating Expectations

Update `*.expect.json` files from compiler output:

```bash
python tests/harness/regen_expect.py --compiler build/cursivec0 --tests-root tests/compile
```

Regenerate E2E `expect.json` from interpreter output:

```bash
python tests/harness/regen_semantics_oracle.py --interpreter build/cursive0_interpreter_cli --tests-root tests/e2e
```
