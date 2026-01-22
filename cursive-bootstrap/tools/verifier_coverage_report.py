#!/usr/bin/env python3
from __future__ import annotations

import json
import os
import sys
from pathlib import Path


def _load_rules_index(spec_path: Path) -> set[str]:
    data = json.loads((spec_path / "rules_index.json").read_text(encoding="utf-8"))
    return {entry.get("rule_id", "") for entry in data.get("entries", []) if entry.get("rule_id")}


def _load_diag_index(spec_path: Path) -> set[str]:
    data = json.loads((spec_path / "diag_index.json").read_text(encoding="utf-8"))
    return {entry.get("code", "") for entry in data.get("entries", []) if entry.get("code")}


def _read_tsv(path: Path) -> list[list[str]]:
    rows: list[list[str]] = []
    for raw in path.read_text(encoding="utf-8", errors="replace").splitlines():
        if not raw or raw.startswith("#"):
            continue
        rows.append(raw.split("\t"))
    return rows


def main() -> int:
    root = Path(__file__).resolve().parent.parent
    spec_path = root / "spec"
    rules_index = _load_rules_index(spec_path)
    diag_index = _load_diag_index(spec_path)

    rules_rows = _read_tsv(spec_path / "verifier_rules.tsv")
    edges_rows = _read_tsv(spec_path / "verifier_edges.tsv")
    diag_rows = _read_tsv(spec_path / "verifier_diag.tsv")

    verifier_rules = {row[0] for row in rules_rows if row and row[0]}
    missing_rules = sorted(rules_index - verifier_rules)
    unknown_rules = sorted(verifier_rules - rules_index)

    edge_rules: set[str] = set()
    for row in edges_rows:
        if len(row) >= 2:
            if row[0]:
                edge_rules.add(row[0])
            if row[1]:
                edge_rules.add(row[1])
    unknown_edge_rules = sorted(edge_rules - rules_index)

    diag_codes = {row[0] for row in diag_rows if row and row[0]}
    unknown_diag_codes = sorted(diag_codes - diag_index)
    diag_rule_ids = {row[1] for row in diag_rows if len(row) > 1 and row[1]}
    unknown_diag_rules = sorted(diag_rule_ids - rules_index)

    strict = os.environ.get("VERIFIER_COVERAGE_STRICT") == "1"
    failed = False

    if missing_rules:
        print("[verifier_coverage] missing verifier_rules.tsv entries:")
        for rule_id in missing_rules:
            print(f"  - {rule_id}")
        if strict:
            failed = True

    if unknown_rules:
        failed = True
        print("[verifier_coverage] unknown rule_id(s) in verifier_rules.tsv:")
        for rule_id in unknown_rules:
            print(f"  - {rule_id}")

    if unknown_edge_rules:
        failed = True
        print("[verifier_coverage] unknown rule_id(s) in verifier_edges.tsv:")
        for rule_id in unknown_edge_rules:
            print(f"  - {rule_id}")

    if unknown_diag_codes:
        failed = True
        print("[verifier_coverage] unknown diag_code(s) in verifier_diag.tsv:")
        for code in unknown_diag_codes:
            print(f"  - {code}")

    if unknown_diag_rules:
        failed = True
        print("[verifier_coverage] unknown rule_id(s) in verifier_diag.tsv:")
        for rule_id in unknown_diag_rules:
            print(f"  - {rule_id}")

    return 1 if failed else 0


if __name__ == "__main__":
    raise SystemExit(main())
