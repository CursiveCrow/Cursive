# HelloCursive Cases

`HelloCursive/Cases` contains hand-authored conformance cases. Each case is a
standalone Cursive project plus a strict manifest that binds the project to
spec-derived obligation ids.

The Cursive specification remains the source of truth. A case may demonstrate or
probe an obligation, but the case file, fixture name, compiler output, and current
compiler behavior are never conformance truth.

## Layout

Use this directory shape for every case:

```text
HelloCursive/Cases/<Area>/<Feature>/<CaseId>/
  Case.cursive-test
  Cursive.toml
  Main/
    Main.cursive
```

Path segments under `Cases` use descriptive `PascalCase`. Keep each case focused
on one semantic point or one small cluster of directly related obligations.

`Main` is the case-local source root for executable cases. Inside that root, use
normal Cursive file and module conventions: `Main.cursive` for an executable
entry file, `PascalCase.cursive` for other source files, and `PascalCase`
directories for submodules.

## Case Manifest

`Case.cursive-test` is a UTF-8, TOML-style sidecar file. Use `snake_case` keys,
quoted strings, and TOML arrays. Do not use colon fields such as `case-id:` or
duplicate outcome encodings such as `kind: compile-pass`.

Required fields, in order:

```toml
schema_version = 1
case_id = "case.area_feature_case"
mode = "check"
source = "Main/Main.cursive"
expected_compile_status = "pass"
obligations = [
    "req.RealObligationId"
]
```

Field rules:

- `schema_version` is currently `1`.
- `case_id` is a stable dotted id. Do not derive conformance meaning from it.
- `mode` is `check`, `build`, or `run`.
- `source` is relative to the case directory and normally points at
  `Main/Main.cursive`.
- `expected_compile_status` is `pass` or `fail`.
- `obligations` lists real ids regenerated from `docs/CursiveSpecification.md`.
  Do not invent ids and do not repeat an id inside one case.

Run cases add runtime expectations:

```toml
expected_runtime_status = "pass"
expected_exit_code = 0
expected_stdout = "Hello, Cursive\n"
expected_stderr = ""
```

Compile-fail cases add expected diagnostics:

```toml
expected_diagnostics = [
    "E-SRC-0000|error"
]
```

Blocked obligation entries are explicit and evidence-backed:

```toml
blockers = [
    "req.RealObligationId|current_compiler|HelloCursive/Oracle/Reports/evidence.txt"
]
```

Allowed blocker reasons are `current_compiler`, `test_infra`, and
`not_directly_testable`.

The oracle must reject unknown fields, missing required fields, unsupported enum
values, unknown obligation ids, duplicate case ids, and duplicate obligations
inside one case.

## Cursive Project Manifest

Each case has its own `Cursive.toml`:

```toml
[toolchain]
target_profile = "x86_64-win64"

[build]
incremental = false
progress = false

[[assembly]]
name = "<CaseId>"
kind = "executable"
root = "./Main"
out_dir = "build/main"
emit_ir = "ll"
```

Use the case directory's `PascalCase` name for the assembly, such as
`MinimalExecutable`. The executable entry procedure inside the source remains
`main` because that name is language-mandated.

## Cursive Source Style

Case source should be idiomatic Cursive except for the single construct being
tested in a negative case:

```cursive
public procedure main(move ctx: Context) -> i32 {
    return 0
}
```

Follow the user guide:

- Write explicit visibility where allowed.
- Give every procedure an explicit return type.
- Use explicit `return` in non-unit procedures.
- Use `~>` for method calls and `.` for field or tuple access.
- Use 4-space indentation and same-line braces.
- Avoid single-line trailing commas.
- Do not use Rust, Go, JavaScript, or C# syntax as Cursive.
- Keep examples self-contained unless the test explicitly targets imports or
  multi-module behavior.

For negative cases, keep the invalid source as small as possible and make the
manifest's cited obligation explain why rejection is required.
