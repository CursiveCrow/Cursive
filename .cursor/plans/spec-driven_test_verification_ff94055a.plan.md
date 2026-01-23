---
name: Spec-Driven Test Verification
overview: Perform a full, per-test verification that tests assert the behaviors in `Cursive0.md`, keep a running progress log, and update tests where they do not correctly encode spec behavior.
todos:
  - id: spec-inventory
    content: Extract rule/diag IDs from Cursive0.md + indices
    status: in_progress
  - id: progress-log
    content: Create and maintain a running verification log
    status: pending
  - id: test-assertion-review
    content: Verify every test’s assertions against the spec
    status: pending
  - id: coverage-audit
    content: Map SPEC_COV + diag expectations to rule IDs
    status: pending
  - id: org-alignment
    content: Propose spec-aligned test organization map
    status: pending
  - id: gap-tests
    content: Draft/add tests for uncovered rules
    status: pending
isProject: false
---

# Spec-Driven Test Verification

## Scope and Inputs

- Spec source: [c:\Dev\Cursive\cursive-bootstrap\docs\Cursive0.md](c:\Dev\Cursive\cursive-bootstrap\docs\Cursive0.md)
- Spec indices: [c:\Dev\Cursive\cursive-bootstrap\spec\rules_index.json](c:\Dev\Cursive\cursive-bootstrap\spec\rules_index.json), [c:\Dev\Cursive\cursive-bootstrap\spec\diag_index.json](c:\Dev\Cursive\cursive-bootstrap\spec\diag_index.json)
- Test suite entrypoints/docs: [c:\Dev\Cursive\cursive-bootstrap\tests\README.md](c:\Dev\Cursive\cursive-bootstrap\tests\README.md), [c:\Dev\Cursive\cursive-bootstrap\tests\harness\README.md](c:\Dev\Cursive\cursive-bootstrap\tests\harness\README.md)
- Harnesses: [c:\Dev\Cursive\cursive-bootstrap\tests\harness\run_compiler.py](c:\Dev\Cursive\cursive-bootstrap\tests\harness\run_compiler.py), [c:\Dev\Cursive\cursive-bootstrap\tests\harness\run_e2e.py](c:\Dev\Cursive\cursive-bootstrap\tests\harness\run_e2e.py), [c:\Dev\Cursive\cursive-bootstrap\tests\harness\run_semantics_oracle.py](c:\Dev\Cursive\cursive-bootstrap\tests\harness\run_semantics_oracle.py)
- Coverage tooling: [c:\Dev\Cursive\cursive-bootstrap\tools\coverage_report.py](c:\Dev\Cursive\cursive-bootstrap\tools\coverage_report.py), [c:\Dev\Cursive\cursive-bootstrap\tools\diag_check_report.py](c:\Dev\Cursive\cursive-bootstrap\tools\diag_check_report.py)
- Progress log: [c:\Dev\Cursive\cursive-bootstrap\docs\TEST_VERIFICATION_PROGRESS.md](c:\Dev\Cursive\cursive-bootstrap\docs\TEST_VERIFICATION_PROGRESS.md)

## Plan

1. **Spec rule inventory + diagnostics mapping**

- Parse rule IDs from `Cursive0.md` (rule labels) and cross‑validate against `rules_index.json` + `diag_index.json`.
- Output a normalized rule list grouped by spec section (0–9) and diagnostic category (E‑PRJ, E‑MOD, E‑OUT, E‑SRC, E‑UNS, E‑TYP, E‑SEM, W‑*).

1. **Create and maintain the running progress log**

- Create `docs/TEST_VERIFICATION_PROGRESS.md` with a per-suite checklist (compile/e2e/unit/fuzz/harness) and per-test rows.
- Track status: verified, mismatch (spec), needs update, updated, blocked (spec ambiguity).
- Record the exact spec rule(s) and diagnostic IDs validated by each test.

1. **Full per-test spec assertion review**

- For every test file, read the test logic and expectations (diagnostics/IR/stdout/stderr/exit code).
- Compare the asserted behavior to the spec rule text and required diagnostics in `Cursive0.md`.
- Update tests that do not precisely assert spec behavior (no weakening; only corrections to align with spec).
- Record each test’s verification outcome in the progress log.

1. **Test suite coverage audit (SPEC_COV + diagnostics)**

- Scan all `.cursive` tests for `# SPEC_COV:` tags and map them to spec rule IDs.
- Identify: missing SPEC_COV tags, unknown rule IDs, and rules with zero tests.
- For compile tests, map diagnostic expectation files (`*.expect.json`) to spec diagnostics in `diag_index.json` and flag mismatches.

1. **Organization alignment with spec structure**

- Propose a spec‑section mapping for each test area:
- `tests/compile/phase0–phase4` ↔ spec Phases 0–4
- `tests/e2e` and semantics oracle ↔ Sections 7 (dynamic semantics) and runtime/ABI in Section 6
- `tests/unit` and `tests/fuzz` ↔ rule clusters with coverage gaps
- Produce a lightweight structure map (no thin wrappers/indirection) that keeps path → spec rule proximity obvious.

1. **Gap‑closing test additions (spec‑correct)**

- For each uncovered rule set, draft concrete test cases with precise `SPEC_COV` tags and expected diagnostics/output.
- Add tests only where the spec defines observable behavior/diagnostics; if a rule is untestable in C0, mark it explicitly as such.

1. **Harness verification + invariants**

- Ensure harnesses enforce:
- SPEC_COV presence and validity
- deterministic diagnostics normalization
- proper isolation for compile tests
- Confirm coverage and diagnostics reports are aligned with the spec indices.

1. **Validation runs (post‑changes)**

- Run coverage/diag reports and focused test subsets to confirm improved coverage and that failures indicate compiler bugs, not weakened tests.

## Notes

- Failing tests will be treated as compiler bugs (no test weakening). Any change to tests must be justified by a spec correction or unambiguous rule application.

