# Spec-Authoritative Test Harness

This harness is designed to keep tests aligned with the Cursive0 specification.
Tests should encode **spec-required behavior**, not current compiler quirks.

## Expectations

- Diagnostics in `*.expect.json` must include:
  - `code`
  - `severity`
  - `message`
  - `span` (object or `null` only when a span is truly unavailable)
- Diagnostic spans are normalized to the **original test file path** even when
  tests run in isolated/filtered temp directories.
- IR tests (`*.expect.ll`) are **exact-match** after deterministic normalization
  (line endings + trailing whitespace; temp paths normalized).

## Semantics-oracle suite

The semantics-oracle suite compares **compiler output vs interpreter output**
for the same test project, using the interpreter as the spec-truth oracle.

- Harness: `tests/harness/run_semantics_oracle.py`
- Test root: `tests/e2e` (semantics-oracle projects)
- CTest target: `semantics_oracle_tests` (hard-gated)
- Interpreter runner: `cursive0_interpreter_cli` (CMake target)
- Outputs compared: `stdout`, `stderr`, `exit_code`
- `expect.json` must match the interpreter output (acts as documentation)

## Regenerating expectations

Use the regeneration helper to update `*.expect.json` files from compiler output:

```
python tests/harness/regen_expect.py --compiler build/Release/cursivec0.exe --tests-root tests/compile
```

This will normalize spans to original test paths so expectations remain stable.

To regenerate **semantics-oracle** `expect.json` files from interpreter output:

```
python tests/harness/regen_semantics_oracle.py --interpreter build/Release/cursive0_interpreter_cli.exe --tests-root tests/e2e
```

## Spec coverage reporting

`tools/coverage_report.py` and `tools/diag_check_report.py` emit **report-only**
coverage/consistency checks:

- `coverage_report.py`: SPEC_RULE/SPEC_COV coverage report + unknown SPEC_COV IDs
- `diag_check_report.py`: verifies spec diagnostic index matches the spec tables

These are integrated into CTest as non-gating informational checks.
