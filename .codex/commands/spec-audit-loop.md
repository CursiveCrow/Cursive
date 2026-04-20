---
description: "Fix one audited spec-conformance item in an assigned worktree, add row proof, and stop for launcher integration."
---

Work one audit item at a time.

`docs/CursiveSpecification.md` is the source of truth for compiler definitions and behaviors.

If this prompt includes a `Selected Audit Row` block, do not rescan `docs/audit/SPEC_RULE_TABLE_BY_PHASE.csv` for this iteration. Use only the provided row.

Execution contract:

1. Read `docs/audit/SPEC_RULE_TABLE_BY_PHASE.csv` unless a `Selected Audit Row` block was provided.
2. Treat actionable rows as those whose `implemented` value is neither `complete`, `ambiguous`, nor `in_progress`.
3. Treat `complete` and `ambiguous` as terminal row states that the loop must skip.
4. Treat `in_progress` as an already-assigned row state that must be skipped.
5. Select the first remaining actionable row, or use the preselected row from the `Selected Audit Row` block.
6. Read `docs/CursiveSpecification.md` carefully. Preserve UTF-8 normative symbols exactly.
7. Read the current implementation files cited by and related to the audit row, plus any corollary files that the change will impact.
8. Correct the implementation so it conforms to the spec for the issue identified in that single row. Your implementation must abide by the following rules.
   - Your correction must be *complete and comprehensive*, maintaining extremely high code quality standards for correctness, cleanliness, and performance.
   - Any new files must adhere to the project's organization schema.
   - Your fix must not introduce alternative or further spec deviation.
9. Add or extend the smallest HelloCursive or static-conformance test that would have failed before the fix.
   - Name the exact test file/function in your final `SPEC_AUDIT_NOTE`.
   - Explain the pre-fix failure mechanism in your final `SPEC_AUDIT_NOTE` and commit `Tested:` trailer.
   - For heading-only or ambiguous rows, explain why executable proof is not appropriate.
10. Multiple audit agents may be active at once.
    - Ignore rows whose `implemented` value is `in_progress`; those rows are already assigned to another active audit worker or to this assigned row.
    - Treat the assigned worktree as the only writable workspace for this row.
    - Do not assume other audit work is idle or that the shared checkout is exclusively yours.
11. All implementation changes for this row are isolated in the assigned worktree.
    - Complete the row in that worktree only.
    - Create exactly one git commit per worker turn in that worktree. The initial turn creates one row commit. If the launcher resumes you after a verification failure, that retry turn may create exactly one follow-up commit.
    - Do not run any build, configure, test, package, bootstrap, or verification command from the assigned worktree.
    - Do not run `cmake`, `ctest`, `ninja`, `make`, `gmake`, `msbuild`, `scripts/build_cursive_all.sh`, `RunCompilerStaticConformance.ps1`, `RunHelloCursive*.ps1`, `setup_extern.ps1`, or any equivalent command from the worktree.
    - Do not create or rely on worktree-local `build/`, `extern/`, or other generated validation artifacts.
    - Stop after that turn's commit. The launcher will transfer the row result back into the main repo, verify the already-configured Windows CMake build and HelloCursive project there, destroy this worktree, and create a fresh worktree for the next row if needed.
12. If the row is a heading-only line in the spec, with no associated rule or spec content, mark the row as `complete`, create the required row-item commit, and skip to step 18.
13. If the spec is ambiguous or the correct interpretation is unclear:
    - Append a new entry to `docs/SpecDecisionsNeeded.md`.
    - Start the entry with `---` on its own line.
    - Record the phase, rule name, spec location, compiler location, and the exact ambiguity.
    - Update that audit row in `docs/audit/SPEC_RULE_TABLE_BY_PHASE.csv` so `implemented` becomes `ambiguous`.
    - Do not invent semantics.
    - If this iteration was launched with a `Selected Audit Row` block, stop after recording the ambiguity and committing the row item so the launcher can integrate it and move on.
    - Otherwise, continue only within this row-item flow and still stop after the single committed row result.
14. After completing a non-ambiguous fix, update that audit row in `docs/audit/SPEC_RULE_TABLE_BY_PHASE.csv` so:
    - `implemented` becomes `complete`
    - `compiler location` reflects the current evidence for the fix
15. The launcher is the sole owner of verification.
16. The launcher will verify the row after integrating your committed row result by running:
    - `cmake --build --preset windows-debug --target cursive_out`
    - `HelloCursive/RunHelloCursive.ps1 -CompilerPath <freshly built compiler>`
17. If you believe the row cannot survive that existing launcher-owned verification, treat the row as blocked and explain why in `SPEC_AUDIT_NOTE`.
18. Create exactly one git commit for this worker turn. The commit message must identify which audit item was corrected, follow the repository Lore commit protocol, and include a `Tested:` trailer naming the row-specific test file/function plus the pre-fix failure mechanism it proves.
19. End the final response with exactly these trailing status lines and nothing after them:
   - `SPEC_AUDIT_STATUS: continue` when one item was fixed and verified, or when an ambiguity was recorded and the launcher should integrate the row result and move to the next row
   - `SPEC_AUDIT_STATUS: complete` when the selected row item was fixed and verified and no remaining actionable row exists outside `{complete, ambiguous, in_progress}`
   - `SPEC_AUDIT_STATUS: blocked` when no further safe progress can be made in the current row iteration
   - `SPEC_AUDIT_ITEM: <rule name> @ <spec location>` or `SPEC_AUDIT_ITEM: none`
   - `SPEC_AUDIT_NOTE: <single-line summary>`

Do not claim completion unless the row item is committed and ready for launcher-side main-repo Windows build and HelloCursive verification.
