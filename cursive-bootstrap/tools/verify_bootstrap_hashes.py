#!/usr/bin/env python3
from __future__ import annotations

import hashlib
import sys
from pathlib import Path


def _sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def main() -> int:
    root = Path(__file__).resolve().parent.parent
    bootstrap_dir = root / "bootstrap"
    sums_path = bootstrap_dir / "SHA256SUMS"
    if not sums_path.exists():
        print(f"[bootstrap] missing {sums_path}")
        return 1

    lines = sums_path.read_text(encoding="utf-8", errors="replace").splitlines()
    entries: list[tuple[str, str]] = []
    for line in lines:
        stripped = line.strip()
        if not stripped or stripped.startswith("#"):
            continue
        parts = stripped.split()
        if len(parts) < 2:
            print(f"[bootstrap] malformed line: {line}")
            return 1
        entries.append((parts[0], parts[-1]))

    if not entries:
        print("[bootstrap] SHA256SUMS has no entries")
        return 1

    ok = True
    for expected, rel in entries:
        target = bootstrap_dir / rel
        if not target.exists():
            print(f"[bootstrap] missing file: {target}")
            ok = False
            continue
        actual = _sha256(target)
        if actual.lower() != expected.lower():
            print(f"[bootstrap] hash mismatch: {rel}")
            print(f"  expected: {expected}")
            print(f"  actual:   {actual}")
            ok = False

    return 0 if ok else 1


if __name__ == "__main__":
    raise SystemExit(main())
