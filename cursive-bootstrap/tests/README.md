# Cursive0 Test Suite

## Layout

- `compile/`: compiler-phase tests
  - `compile/Cursive.toml`: shared project for compile tests
  - `compile/src/phase1..phase4/`: single-module cases (no per-case manifest)
  - `compile/phase0/`: per-case manifest tests (missing/invalid manifest allowed)
  - `compile/projects/`: multi-module compile tests with their own `Cursive.toml`
- `e2e/`: runtime tests (project dirs with `expect.json`)
- `unit/`: C++ unit tests
- `fuzz/`: fuzzers
- `harness/`: Python test runners and utilities

## Expectations

- Compile diagnostics: `*.cursive.expect.json`
- Compile IR: `*.expect.ll`
- E2E output: `expect.json` at project root

## SPEC_COV

All `.cursive` test files MUST include `# SPEC_COV: <rule_id>` comments near
the top of the file. Use one or more SPEC_COV lines as needed.

## Disabled tests

To disable a test, rename the file to `*.cursive.disabled` and add a short
reason in a nearby README or in the file header. The harness only discovers
`*.cursive` files.

## Canonical harness entrypoints

- `harness/run_compiler.py`
- `harness/run_e2e.py`
- `harness/run_semantics_oracle.py`
