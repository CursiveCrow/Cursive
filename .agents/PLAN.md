# Plan: Hook Planning And Stop Cleanup

Plan artifact status: complete
Execution status: passed
Timestamp: 2026-04-26T00:00:00-07:00
Work branch: hook-planning-stop-cleanup

1. Keep full prewrite plan/directive injection only in `systemMessage`, with short visible `reason` text.
2. Normalize missing-file and planning Stop block responses so visible `reason` is concise and detailed instructions are model-facing.
3. Tighten planning prompt detection so implementation prompts mentioning a proposal are not misclassified as planning.
4. Make Stop validation honor explicit Stop payload mode when available before falling back to stored prompt state.
5. Prefer `Plan artifact status: complete` in hook instructions while accepting the existing `Plan status: complete` format for compatibility.
6. Add regression tests for response shape, planning prompt classification, explicit Stop mode, and no duplicated block payloads.
7. Sync the updated hook and tests to WSL and verify both installed test suites.
