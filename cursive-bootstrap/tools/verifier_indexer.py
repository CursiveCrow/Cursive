#!/usr/bin/env python3
from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path


RULE_RE = re.compile(r"^\*\*\(([^)]+)\)\*\*\s*$")
HEADING_RE = re.compile(r"^#+\s+([0-9]+)")
VERIFIER_RULE_RE = re.compile(r"<!--\s*VERIFIER_RULE:\s*(.*?)\s*-->")
VERIFIER_EDGE_RE = re.compile(r"<!--\s*VERIFIER_EDGE:\s*(.*?)\s*-->")


def _parse_kv(text: str) -> dict[str, str]:
    out: dict[str, str] = {}
    for part in text.split():
        if "=" not in part:
            continue
        key, value = part.split("=", 1)
        out[key.strip()] = value.strip()
    return out


def _default_domain(section: str) -> str:
    try:
        major = int(section.split(".")[0])
    except Exception:
        return "-"
    if major >= 7:
        return "runtime"
    return "compile"


def parse_spec(spec_path: Path):
    text = spec_path.read_text(encoding="utf-8")
    lines = text.splitlines()

    rules: list[tuple[str, str]] = []
    seen: set[str] = set()
    current_section = ""

    overrides: dict[str, dict[str, str]] = {}
    edges: list[dict[str, str]] = []

    for idx, line in enumerate(lines, start=1):
        head = HEADING_RE.match(line)
        if head:
            current_section = head.group(1)

        rule_m = RULE_RE.match(line.strip())
        if rule_m:
            rule_id = rule_m.group(1).strip()
            if rule_id in seen:
                raise ValueError(f"duplicate rule id '{rule_id}' at line {idx}")
            seen.add(rule_id)
            rules.append((rule_id, current_section))

        vr = VERIFIER_RULE_RE.search(line)
        if vr:
            content = vr.group(1).strip()
            if not content:
                continue
            parts = content.split()
            rule_id = parts[0]
            kv = _parse_kv(" ".join(parts[1:]))
            overrides[rule_id] = kv

        ve = VERIFIER_EDGE_RE.search(line)
        if ve:
            kv = _parse_kv(ve.group(1))
            if "before" not in kv or "after" not in kv:
                raise ValueError(f"missing before/after in VERIFIER_EDGE at line {idx}")
            edges.append(kv)

    return rules, overrides, edges


def write_rules(out_path: Path, rules: list[tuple[str, str]], overrides: dict[str, dict[str, str]]) -> None:
    lines = ["# rule_id\tdomain\tmin_count\tmax_count\tpayload_keys"]
    for rule_id, section in rules:
        ov = overrides.get(rule_id, {})
        domain = ov.get("domain", _default_domain(section))
        min_count = ov.get("min", "-")
        max_count = ov.get("max", "-")
        keys = ov.get("keys", "-")
        lines.append(f"{rule_id}\t{domain}\t{min_count}\t{max_count}\t{keys}")
    out_path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def write_edges(out_path: Path, edges: list[dict[str, str]]) -> None:
    lines = ["# before_rule\tafter_rule\tdomain\tgroup_by"]
    for edge in edges:
        before = edge.get("before", "")
        after = edge.get("after", "")
        domain = edge.get("domain", "-")
        group_by = edge.get("group_by", "-")
        lines.append(f"{before}\t{after}\t{domain}\t{group_by}")
    out_path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> int:
    root = Path(__file__).resolve().parent.parent
    default_spec = root / "docs" / "Cursive0.md"
    default_rules = root / "spec" / "verifier_rules.tsv"
    default_edges = root / "spec" / "verifier_edges.tsv"

    parser = argparse.ArgumentParser(description="Generate verifier rules/edges from Cursive0.md")
    parser.add_argument("--spec", default=str(default_spec), help="Path to Cursive0.md")
    parser.add_argument("--out-rules", default=str(default_rules), help="Output verifier_rules.tsv")
    parser.add_argument("--out-edges", default=str(default_edges), help="Output verifier_edges.tsv")
    parser.add_argument("--check", action="store_true", help="Check outputs match existing files")
    args = parser.parse_args()

    spec_path = Path(args.spec)
    out_rules = Path(args.out_rules)
    out_edges = Path(args.out_edges)

    rules, overrides, edges = parse_spec(spec_path)

    rules_text = "\n".join(
        ["# rule_id\tdomain\tmin_count\tmax_count\tpayload_keys"]
        + [
            f"{rule_id}\t{overrides.get(rule_id, {}).get('domain', _default_domain(section))}\t"
            f"{overrides.get(rule_id, {}).get('min', '-')}\t"
            f"{overrides.get(rule_id, {}).get('max', '-')}\t"
            f"{overrides.get(rule_id, {}).get('keys', '-')}"
            for rule_id, section in rules
        ]
    ) + "\n"

    edges_text = "\n".join(
        ["# before_rule\tafter_rule\tdomain\tgroup_by"]
        + [
            f"{edge.get('before','')}\t{edge.get('after','')}\t"
            f"{edge.get('domain','-')}\t{edge.get('group_by','-')}"
            for edge in edges
        ]
    ) + "\n"

    if args.check:
        if not out_rules.exists() or not out_edges.exists():
            print("[verifier_indexer] missing output file(s)", file=sys.stderr)
            return 1
        if out_rules.read_text(encoding="utf-8") != rules_text:
            print("[verifier_indexer] verifier_rules.tsv is out of date", file=sys.stderr)
            return 1
        if out_edges.read_text(encoding="utf-8") != edges_text:
            print("[verifier_indexer] verifier_edges.tsv is out of date", file=sys.stderr)
            return 1
        return 0

    out_rules.parent.mkdir(parents=True, exist_ok=True)
    out_edges.parent.mkdir(parents=True, exist_ok=True)
    out_rules.write_text(rules_text, encoding="utf-8")
    out_edges.write_text(edges_text, encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
