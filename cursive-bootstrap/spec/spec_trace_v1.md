# spec_trace_v1

This document defines the trace file format used by the spec verifier.

## File format

- First line MUST be: `spec_trace_v1`
- Each subsequent line is one event with TAB-separated fields:

```
domain<TAB>phase<TAB>rule_id<TAB>file<TAB>start_line<TAB>start_col<TAB>end_line<TAB>end_col<TAB>payload
```

## Field definitions

- `domain`: `compile` or `runtime`
- `phase`: `parse|resolve|typecheck|codegen|runtime`
- `rule_id`: SPEC rule identifier (string)
- `file`: project-relative path or `-`
- `start_line`, `start_col`, `end_line`, `end_col`: 1-based source positions, or `0` when unknown
- `payload`: `key=value;key=value` with percent-encoding for `\t`, `\n`, `%`, `;`, `=`

## Payload encoding

- Percent-encode these characters: `\t`, `\n`, `%`, `;`, `=`
- Use `%09` for TAB, `%0A` for LF, `%25` for `%`, `%3B` for `;`, `%3D` for `=`

## Example

```
spec_trace_v1
compile	typecheck	T-TC-012	main.cursive	12	5	12	18	entity=let;type=i32
runtime	runtime	Cleanup-Step-Drop-Ok	main.cursive	19	5	19	21	scope_id=3;order=1
```
