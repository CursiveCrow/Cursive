# CursiveOracle

`CursiveOracle` is the clean-room Cursive conformance oracle project.

The previous oracle attempt is discarded. This project must derive compiler and
runtime expectations from `docs/CursiveSpecification.md` through a structured spec
model, an obligation graph, and executable reference semantics. Fixtures, trace
labels, audit tables, and copied diagnostic strings are not sources of truth.

This directory intentionally starts with only project identity. Module directories
are added only when the same task adds real implementation files for that module.
Empty hierarchy buckets, TODO-only stubs, and `not implemented` placeholder modules
are not part of the project.

Global conformance cannot pass while any required spec obligation is unparsed,
unmodeled, uncovered, blocked, unsupported, or failing.
