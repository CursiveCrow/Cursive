# Charter

Charter is a Codex prewrite hook that gates write-capable tool calls on a
per-session charter file.

It reads `.agents/DIRECTIVES.md` and `.agents/charters/<session-id>.md`, injects
compact charter context before writes, checks write targets against the active
action item, and blocks stop after implementation writes until the written task
has concrete completion notes plus matching verification.

## How It Works

Charter:

- requires a formal charter before implementation writes;
- validates parsed charter tasks against a schema-equivalent contract;
- injects compact charter context, previous completion notes, the active action
  item, and directives before opening a bounded write window;
- blocks writes outside the active action item's typed `Allowed writes`;
- blocks writes whose targets cannot be mechanically verified;
- analyzes composite shell commands by segment so read-first commands cannot hide
  later write operations;
- scans added patch and inline write content for task-local blocked patterns;
- reinjects when a new user prompt arrives, the write window expires, or
  charter/directive/task context changes;
- tracks successful implementation writes by active task id;
- blocks stop after implementation writes until completion notes and explicit
  passing verification match the task that received the write.

## Install

Clone or copy this repository's `.codex` and `.agents` folders into the root of
a workspace that supports Codex hooks:

```text
your-project/
  .codex/
    hooks.json
    hooks/
      prewrite-gate.mjs
      prewrite-gate/
        main.mjs
        ...
  .agents/
    DIRECTIVES.md
    charters/
      example.md
```

The entrypoint and module directory must be installed together:

```text
.codex/hooks/prewrite-gate.mjs
.codex/hooks/prewrite-gate/**
```

The hook command is repo-relative:

```json
"command": "node \".codex/hooks/prewrite-gate.mjs\""
```

Node.js must be available on `PATH`.

Generated charter files belong in `.agents/charters/`. The bundled `.gitignore`
ignores generated `*.md` files while keeping `.agents/charters/example.md`
tracked.

For a global Codex install, copy the same hook files under
`%USERPROFILE%\.codex\hooks\` and point the global hook configuration at
`prewrite-gate.mjs`.

## Charter Format

Before implementation writes, create the active charter file at:

```text
.agents/charters/<session-id>.md
```

The charter is a formal Markdown document. `## Action Items` starts the ordered
task list.

Required top-level sections:

```md
# <Charter Title>

## Objective

- <what this change is trying to accomplish>

## Success Criteria

- <observable outcome required for completion>

## Source Tree

- path: <file-or-glob>
  owns:
  - <responsibility owned by this path>

## Technical Stack

- <runtime, language, framework, or toolchain facts>

## Constraints

- <rules that shape the implementation>

## Non-Goals

- <explicitly out-of-scope work>

## References

- <docs, files, issues, or user requests that define authority>

## Verification Strategy

- <commands or checks expected to prove completion>

## Action Items
```

Every action item must use typed `Allowed writes` entries:

```md
### TASK-001: <First action item title>
Status: pending
Observed problem:
- <what failed or what behavior was requested>
Required behavior:
- <the general behavior that must be true>
Source of truth:
- <user request, spec, API contract, design doc, or canonical test contract>
Canonical owner:
- <file/module/type/function/service responsible for the behavior>
Why this is the owner:
- <short reason>
Downstream consumers:
- <files/modules that may consume the behavior but must not reimplement it>
Forbidden implementation paths:
- <task-specific shortcut or workaround paths that are not allowed>
Rejected shortcuts:
- <shortcut> | <why rejected>
Allowed writes:
- path: <relative/path/or/glob>
  role: canonical-owner
  permitted:
  - <allowed responsibility>
  forbidden:
  - <disallowed responsibility>
Verification:
- <command/check proving the canonical path is exercised>
Action: <Concrete action needed to execute this task.>
Completion notes: pending
```

The Markdown parser validates each task against a stable schema-equivalent
contract. Markdown remains the supported authoring format; JSON or YAML schema
blocks do not override contradictory Markdown task fields. Every typed
`Allowed writes` entry must include non-empty `path`, `role`, `permitted`, and
`forbidden` metadata.

Valid write roles are:

- `canonical-owner`: may implement or change behavior owned by this task.
- `consumer`: may consume canonical behavior; must not duplicate, infer,
  recover, validate, parse, normalize, route, lower, or decide behavior that
  belongs to another owner.
- `test`: may add or update coverage; must not weaken expected behavior without
  a source-of-truth justification.
- `docs`: may document the canonical behavior.
- `adapter-boundary`: may represent a real external boundary and must delegate
  inward to canonical behavior.
- `generated-output`: may receive generated or copied output from the canonical
  source.
- `build-config`: may update build/tooling configuration.
- `deletion-only`: may remove code or files but may not add replacement behavior.
- `plan`: may update the active charter artifact.

Bare path-only entries are invalid:

```md
Allowed writes:
- src/**
```

Use typed entries instead:

```md
Allowed writes:
- path: src/**
  role: canonical-owner
  permitted:
  - implement the requested behavior at the source of truth
  forbidden:
  - patch only the visible symptom
```

Planning-mode stop is blocked unless the active charter:

- exists and is non-empty;
- contains all required top-level sections;
- contains `## Action Items`;
- contains ordered `### TASK-<number>: <title>` blocks;
- gives every task the required authority fields, typed `Allowed writes`,
  `Action`, and `Completion notes`;
- includes at least one `canonical-owner` allowed-write entry per task;
- was written or updated during the current planning turn.

Task statuses are:

- `pending`
- `in-progress`
- `blocked`
- `complete`

A task is considered complete only when it has `Status: complete` and
`Completion notes:` is not empty, `pending`, `none`, `n/a`, or `na`.

After implementation writes, the task that received the write needs concrete
completion notes before stop is allowed:

```md
Completion notes:
Changed:
- <what changed>
Verified:
- <verification command/check that passed after the write>
```

The Stop hook also requires a passing command after the last successful
implementation write, and that command must match one of the task's
`Verification` entries. A passing unrelated command is not enough. Unknown or
empty tool responses are recorded as unknown, not passed, and assistant prose
does not count as verification evidence. Matching is exact after whitespace and
case normalization; a command that merely echoes or contains the expected
verification command does not satisfy Stop.

## Runtime Behavior

Charter allows control writes to:

- `.agents/`
- `.agents/charters/`
- `.agents/charters/<current-session-id>.md`
- `.agents/DIRECTIVES.md`

Other writes are gated. On the first implementation write after a user prompt,
or when the write window expires, the hook blocks the write and injects:

- compact parsed charter context from the top-level sections;
- completion notes from previous completed action items;
- the active task contract: required behavior, source of truth, ownership
  rationale, forbidden implementation paths, rejected shortcuts, action,
  canonical owner, typed allowed writes, and verification;
- the current line/token write-window budget;
- the full `.agents/DIRECTIVES.md` file.

The full charter file remains the validation and hashing source of truth, but the
model-facing injection deliberately does not dump future pending action items.

After injection, retrying the write is allowed only while the attempted write
fits the configured line/token budget and the write window still belongs to the
same session, turn, active task, charter path, charter/directives hash, and
prompt context. Oversized writes keep blocking until the model sees the current
context again and splits the write into authorized chunks.

The active action item is the first unfinished task in file order. Later action
item writes are blocked until all earlier action items are complete with
completion notes. A `blocked` task also stops progression until the charter is
updated.

Every extracted write target must match one of the active action item's typed
`Allowed writes` entries. Allowed write paths support exact relative paths,
directory prefixes ending in `/`, `*`, and `**`. If the hook cannot mechanically
extract a target for a write command, it blocks instead of opening a write
window. Use `apply_patch` or an explicit path-bearing write command when that
happens.

The hook scans added patch lines and known shell/API inline write content for
task-local forbidden patterns, common shortcut terms, consumer ownership
patterns, and test weakening patterns. Build or test commands with output
redirects are gated as writes because they create or update files. These checks
are intentionally generic; they do not know about any one codebase.

The target extractor understands patch targets, output redirection, common
PowerShell/file commands, nested parallel tool calls, and JavaScript write APIs
such as `writeFileSync`, `appendFileSync`, `fs.writeFile`, and
`fs.promises.writeFile`. Command classification shares the same segment-aware
shell analysis as target extraction: a composite command receives a read-only
exemption only when every parsed segment is read-only and no segment writes,
redirects output, or runs a recognized build/test command. JavaScript write
targets are extracted even when the content argument is a variable or buffer;
literal content is associated with the target for shortcut scanning when it can
be read safely.

After a successful non-control implementation write, the Stop hook blocks
completion until the task that received the write is marked complete, its
completion notes include `Changed:` and `Verified:` entries, and a matching
verification command has passed after the last successful write.

If critical state used for Stop validation is corrupt, Charter fails closed with
a repairable diagnostic instead of treating the missing state as proof that no
post-write obligations remain. Event log write/prune failures remain non-fatal.

## Configuration

- `CHARTER_STATE_DIR`: override where hook state and event logs are stored.
- `CODEX_HOME`: override the default Codex home used for hook state.
- `OMX_NATIVE_HOOK_PATH`: optionally chain to another native hook after Charter
  runs.

If `OMX_NATIVE_HOOK_PATH` is unset or points to a missing file, Charter runs
standalone.

## Verification

Charter has no runtime npm dependencies. The package scripts provide the
standard local verification entrypoints:

```sh
npm test
npm run syntax
npm run check
```

`npm test` runs the hook regression suite, including deterministic adversarial
fixtures for prompt injection, task-order skipping, scope broadening, fake
verification, and untrusted repository content. `npm run syntax` runs
`node --check` across repository `.mjs` files, excluding `.git`, `.omx`, and
`node_modules`. `npm run check` runs both commands. CI runs the same package
scripts on Node.js 22.

## Limits

Charter is not a sandbox, permission system, or security boundary.

It only gates writes that pass through the configured Codex hook.

Charter treats repository text and other model-consumed content as untrusted by
using deterministic schema validation, typed write authority, exact verification
evidence, and adversarial regression fixtures. Those checks reduce reliance on
prompt-only instructions, but they do not make live model output trustworthy by
themselves and do not protect writes that bypass the hook.

## License

MIT
