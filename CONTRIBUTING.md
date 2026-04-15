# Contributing

## Ground rules

- Treat `docs/CursiveSpecification.md` as the canonical language definition.
- Preserve UTF-8 and the normative Unicode symbols in the specification.
- Prefer small, reversible diffs.
- Do not commit local IDE settings, caches, build outputs, or generated logs.
- Treat `main` as the canonical integration branch.
- Follow the Lore commit protocol from `AGENTS.md`: a why-first subject, a short
  narrative body, and native `Key: value` trailers when they add signal.

## Before opening a change

- Run the relevant platform build using the release or debug preset.
- Keep the vendored extern payload under `extern/` intact. If a local checkout
  is missing it, repopulate it with `scripts/setup_extern_linux.sh` or
  `scripts/setup_extern.ps1` before building.
- Ensure Git LFS objects are present before validating release or CI changes,
  because the vendored LLVM and ICU binary payloads are tracked through LFS.
- Keep bootstrap-only roots such as `tmp/`, `cursive-bootstrap/`,
  `docker-data/`, `release-artifacts/`, and `extern/icu/install/` out of Git.
- Keep audit CSV sources under `docs/audit/` versioned, but treat audit
  markdown summaries and backlog notes as local/generated artifacts.
- If you touch packaging or release automation, validate the staged `cursive_out`
  payload and the normalized archive produced by `scripts/package_release.py`.
- Keep alpha-scope documentation in sync when the supported surface changes.
- Validate commit-message hygiene before pushing if you rewrote or stacked
  commits locally:
  `python scripts/check_commit_messages.py origin/main..HEAD`

## Release-related changes

- GitHub Actions workflows should call repo-owned scripts and CMake presets.
- Do not hardcode machine-specific paths in tracked files.
- Keep the private alpha release flow draft-first so assets can be reviewed
  before publication.
