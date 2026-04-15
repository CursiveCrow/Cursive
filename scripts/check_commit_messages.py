#!/usr/bin/env python3
from __future__ import annotations

import re
import subprocess
import sys


BANNED_SUBJECTS = {
    "abi/ffi updates.",
    "audit complete.",
    "audit loop fixes",
    "audit loop updates",
    "before ralph",
    "bug fixing + comptime started",
    "build fixes",
    "cleanup",
    "comptime and audit work",
    "comptime and reflaction work",
    "diag and verification corrections.",
    "eof and nl fixes",
    "ffi",
    "fix cont",
    "harnass compiler fixes",
    "language audit work",
    "linux support",
    "loop fixes",
    "loop concurrency update",
    "place povenance fixes",
    "pre loop updates",
    "rebuild",
    "repo cleanup",
    "rt link fixes",
    "sourceload",
    "spec checks updates.",
    "spec corrections",
    "testing",
    "updates",
}

TRAILER_RE = re.compile(r"^[A-Z][A-Za-z-]*: .+$")


def git(*args: str) -> str:
    return subprocess.check_output(["git", *args], text=True, encoding="utf-8")


def load_commits(commit_range: str) -> list[tuple[str, str, str]]:
    payload = git(
        "log",
        "--format=%H%x1f%s%x1f%b%x1e",
        "--reverse",
        commit_range,
    )
    commits: list[tuple[str, str, str]] = []
    for record in payload.split("\x1e"):
        if not record.strip():
            continue
        sha, subject, body = record.split("\x1f", 2)
        commits.append((sha.strip(), subject.strip(), body.rstrip()))
    return commits


def validate_commit(sha: str, subject: str, body: str) -> list[str]:
    errors: list[str] = []
    subject_lower = subject.lower()

    if subject.startswith("Merge ") or subject.startswith("Revert "):
        return errors

    if not subject:
        errors.append("missing subject line")
    if len(subject) > 72:
        errors.append(f"subject exceeds 72 characters ({len(subject)})")
    if subject.endswith("."):
        errors.append("subject should not end with a period")
    if subject_lower in BANNED_SUBJECTS:
        errors.append("subject is too generic for the repo Lore policy")

    body_lines = [line.rstrip() for line in body.splitlines()]
    non_empty = [line for line in body_lines if line.strip()]
    if not non_empty:
        errors.append("missing narrative body")
        return errors

    trailer_start = None
    for index, line in enumerate(body_lines):
        if not line.strip():
            continue
        if TRAILER_RE.match(line):
            trailer_start = index
            break

    if trailer_start is None:
        return errors

    narrative = [
        line
        for line in body_lines[:trailer_start]
        if line.strip()
    ]
    if not narrative:
        errors.append("body contains trailers but no narrative context")

    for line in body_lines[trailer_start:]:
        if not line.strip():
            continue
        if not TRAILER_RE.match(line):
            errors.append(f"invalid trailer line: {line}")

    return errors


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: check_commit_messages.py <commit-range>", file=sys.stderr)
        return 2

    commit_range = sys.argv[1]
    commits = load_commits(commit_range)
    if not commits:
        print(f"no commits found in range {commit_range}")
        return 0

    failures: list[str] = []
    for sha, subject, body in commits:
        errors = validate_commit(sha, subject, body)
        if errors:
            failures.append(f"{sha[:12]} {subject}")
            failures.extend(f"  - {error}" for error in errors)

    if failures:
        print("commit message validation failed:")
        print("\n".join(failures))
        return 1

    print(f"validated {len(commits)} commit message(s) in {commit_range}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
