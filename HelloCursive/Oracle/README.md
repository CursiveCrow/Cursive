# HelloCursive Oracle

`HelloCursive/Oracle` is the in-project conformance oracle for the
`HelloCursive` test surface. It is intentionally not a second compiler and no
longer owns a generated-case pipeline or reference-model implementation.

The oracle responsibilities are:

- ingest `docs/CursiveSpecification.md`;
- regenerate the spec-unit and obligation ledgers on demand under
  `HelloCursive/build/oracle`;
- run hand-authored cases under `HelloCursive/Cases`;
- compare compiler behavior against each case manifest; and
- write coverage reports under `HelloCursive/Oracle/Reports`.

Global conformance cannot pass while any required spec obligation remains
uncovered, blocked, unsupported, or failing. New coverage should be added by
hand-writing cases under `HelloCursive/Cases` and binding each case manifest to
the relevant obligation ids from the regenerated obligation ledger.
