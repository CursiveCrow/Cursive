#!/usr/bin/env python3
from __future__ import annotations

import json
import re
import subprocess
import sys
from pathlib import Path


def _load_rules_index(spec_path: Path) -> set[str]:
    rules_file = spec_path / "rules_index.json"
    entries = json.loads(rules_file.read_text(encoding="utf-8"))
    return {entry.get("rule_id", "") for entry in entries.get("entries", []) if entry.get("rule_id")}


def _find_spec_cov_ids(tests_root: Path) -> set[str]:
    cov = set()
    cov_comment_regex = re.compile(r"#\s*SPEC_COV:\s*(\S+)")
    for file_path in tests_root.rglob("*.cursive"):
        try:
            content = file_path.read_text(encoding="utf-8", errors="ignore")
        except Exception:
            continue
        for match in cov_comment_regex.finditer(content):
            cov.add(match.group(1))
    return cov


def main() -> int:
    root = Path(__file__).resolve().parent.parent
    spec_path = root / "spec"
    tests_path = root / "tests"

    try:
        rule_ids = _load_rules_index(spec_path)
    except Exception as exc:
        print(f"[coverage_report] failed to read rules_index.json: {exc}")
        return 0

    cov_ids = _find_spec_cov_ids(tests_path)
    unknown = sorted(cov_ids - rule_ids)
    if unknown:
        print("[coverage_report] unknown SPEC_COV ids (not in rules_index.json):")
        for rule_id in unknown:
            print(f"  - {rule_id}")

    cmd = [sys.executable, str(root / "tools" / "coverage_check.py"), "--report"]
    result = subprocess.run(cmd, capture_output=False, text=True)
    if result.returncode != 0:
        print("[coverage_report] coverage_check.py reported missing coverage")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
