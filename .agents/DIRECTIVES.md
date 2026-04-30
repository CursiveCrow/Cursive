## Prewrite Checklist

Before any non-control write, verify all of these are true:

- The edit follows the active plan injected by the prewrite hook: `.agents/plans/<session-id>.md`.
- The edit satisfies the user request at the real source of truth for the behavior.
- The edit does not simplify, narrow, bypass, shim, special-case, duplicate, or defer the requested behavior to avoid doing the real implementation work.
- The edit preserves or improves existing tests and assertions.
- The edit can be verified with a relevant build, test, typecheck, lint, static analysis, or targeted inspection command.
- If verification cannot run, the blocker and remaining risk will be reported explicitly.

If any item is false, adjust the approach before writing.

## No Work-Avoidance Shortcuts

Do not avoid the real implementation work by simplifying the requirement, narrowing the case, bypassing the broken path, or adding a parallel path.

Unacceptable shortcuts include:

- replacing a required general implementation with narrower behavior that only handles the observed case;
- moving logic out of the canonical implementation instead of fixing the canonical implementation;
- adding a wrapper, adapter, redirect, compatibility layer, fallback, alias, or facade to avoid modifying the responsible code;
- special-casing the current test, fixture, command, filename, exact input string, or visible failure;
- deleting, weakening, or rewriting tests to match incomplete behavior;
- claiming a limitation is acceptable when the user asked for the behavior to work;
- fixing a downstream symptom while leaving the upstream bug in place;
- choosing a smaller diff when the smaller diff is less correct, less complete, or less integrated.

If the correct fix requires touching more files, updating call sites, reconciling duplicate implementations, or replacing an inadequate abstraction, do that work.

## Canonical Implementation Rule

Implement requested behavior at the source of truth: the canonical module, parser, lowering pass, runtime path, command, API, data model, or test surface responsible for that behavior.

Before writing, identify the responsible source-of-truth path and make the change there. Do not route around the real implementation to make only the visible case pass.

After writing, verify that existing public entry points naturally exercise the corrected implementation. There must not be a stale duplicate path, fallback path, temporary shim, test-only branch, or redirect-only fix left behind unless the specification or user explicitly requires that architecture.

Wrappers, adapters, facades, redirects, and compatibility layers are allowed only when they are the correct long-term architecture or required by an external boundary. If used, they must delegate to the canonical implementation and must not hide incomplete behavior.

## Completeness And Generality

Fully satisfy the current user request within the authoritative specification, existing architecture, and real repository constraints.

Prefer a complete root-cause fix over a smaller diff that preserves incorrect behavior.

Do not expand into unrelated work. Complete only the requested behavior and directly necessary supporting work: tests, validation, diagnostics, documentation, integration updates, and error handling.

When existing code is incomplete or incorrect relative to the current authoritative specification, update the canonical implementation. Do not preserve older behavior unless the specification, compatibility contract, migration plan, or explicit user request requires it.

Use appropriate data structures and algorithms. Do not brute-force behavior that has a known better implementation.

## Tests And Verification

Before claiming completion:

- run the relevant tests, build, type checks, linters, static analysis, or targeted verification commands that exist;
- add or update tests for new behavior, regressions, and important edge cases;
- verify existing public entry points exercise the corrected implementation naturally;
- verify no stale duplicate implementation, fallback path, temporary shim, test-only special case, or redirect-only fix remains.

Do not delete tests, weaken assertions, or change expected results to fit incomplete behavior.

If verification cannot run, report:

- the exact command attempted or missing;
- the concrete blocker;
- the remaining risk.

## Cursive-Specific Constraint

For Cursive work, `docs/CursiveSpecification.md` is authoritative. Do not edit the specification or normalize its UTF-8 mathematical symbols unless the user explicitly requests a spec change.

## Final Response Requirements

When reporting completion, include:

- the canonical implementation path changed;
- why the change fixes the root cause rather than a symptom;
- what verification ran and whether it passed;
- any remaining blocker or risk.

## Priority Order

When instructions conflict, use this order:

1. Current authoritative specification and user request.
2. Correctness, semantic completeness, and root-cause implementation.
3. Integration with the existing canonical architecture.
4. Tests, diagnostics, validation, and documentation required by the change.
5. Minimal unnecessary churn.

A small diff is valuable only when it is also correct, complete, integrated, and durable.
