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

## Compile suite layout

The compile suite is split into:

- `tests/compile/src/phase1..phase4/`: single-module cases inside one shared
  project (`tests/compile/Cursive.toml`).
- `tests/compile/phase0/`: per-case manifest tests (missing/invalid manifests
  are allowed and expected).
- `tests/compile/projects/`: multi-module compile tests that require their own
  `Cursive.toml`.

Single-module cases should **not** carry a per-case `Cursive.toml`.

Compile tests emit verifier inputs at:
- `spec/trace_current.tsv`
- `spec/diag_current.tsv`
- `spec/verifier_rules_current.tsv`
- `spec/verifier_edges_current.tsv`

`spec/verifier_rules.tsv` and `spec/verifier_edges.tsv` can be generated from
`docs/Cursive0.md` using:
```
python cursive-bootstrap/tools/verifier_indexer.py
```

## Semantics-oracle suite

The semantics-oracle suite validates **compiled runtime output** against
`expect.json` and checks runtime traces with the Cursive `spec_verifier`.
The interpreter is debug-only and not in the oracle chain.

- Harness: `cursive-bootstrap/tests/harness/run_semantics_oracle.py`
- Test root: `cursive-bootstrap/tests/semantics_oracle` (semantics-oracle projects)
- CTest target: `semantics_oracle_tests` (hard-gated)
- Outputs compared: `stdout`, `stderr`, `exit_code`
- `expect.json` must match the compiled runtime output
- Runtime traces are written to `spec/trace_current.tsv`

## Regenerating expectations

Use the regeneration helper to update `*.expect.json` files from compiler output:

```
python cursive-bootstrap/tests/harness/regen_expect.py --compiler build/Release/cursivec0.exe --tests-root cursive-bootstrap/tests/compile
```

This will normalize spans to original test paths so expectations remain stable.

To regenerate **semantics-oracle** `expect.json` files from compiled output:

```
python cursive-bootstrap/tests/harness/regen_semantics_oracle.py --compiler build/Release/cursivec0.exe --tests-root cursive-bootstrap/tests/semantics_oracle
```

## Spec coverage reporting

`tools/coverage_report.py` and `tools/diag_check_report.py` emit **report-only**
coverage/consistency checks:

- `coverage_report.py`: SPEC_RULE/SPEC_COV coverage report + unknown SPEC_COV IDs
- `diag_check_report.py`: verifies spec diagnostic index matches the spec tables

These are integrated into CTest as non-gating informational checks.

`tools/verifier_coverage_report.py` is a **gating** check for verifier metadata
coverage (rules/edges/diag entries vs spec indexes).
