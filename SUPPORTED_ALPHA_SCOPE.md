# Supported Alpha Scope

This repository is in private alpha. The compiler and runtime are usable for
targeted evaluation, but the implementation is not yet fully conformant with the
language specification.

## Alpha commitments

- Private GitHub prereleases are produced for Linux and Windows.
- Standalone bundles include the compiler, runtime, and required toolchain
  sidecars.
- The language specification remains the source of truth for intended behavior.
- The repo maintains smoke validation for packaged compiler startup and selected
  runtime paths.

## Current limitations

- The implementation is not spec complete.
- Audit reports under `docs/audit/` still track substantial missing and
  incorrect implementation work.
- Full conformance should not be assumed outside the explicitly validated alpha
  paths and examples.
- Release automation is aimed at controlled alpha evaluation, not public stable
  distribution.

## Expected alpha use

- Compiler and runtime evaluation
- Feedback on language ergonomics and release packaging
- Validation of targeted language areas using `HelloCursive`

## Out of scope for the alpha contract

- Public stable support commitments
- Full specification conformance guarantees
- ABI or package-manager compatibility guarantees across alpha releases
