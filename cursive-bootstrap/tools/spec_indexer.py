#!/usr/bin/env python3
import argparse
import json
import re
import sys
from pathlib import Path

RULE_RE = re.compile(r"^\*\*\(([^)]+)\)\*\*\s*$")
HEADING_RE = re.compile(r"^#+\s+([0-9]+(Sigma:\.[0-9]+)*)")


def parse_spec(spec_path: Path):
    text = spec_path.read_text(encoding="utf-8")
    lines = text.splitlines()

    entries = []
    seen = {}
    current_section = ""

    for idx, line in enumerate(lines, start=1):
        m_head = HEADING_RE.match(line)
        if m_head:
            current_section = m_head.group(1)

        m_rule = RULE_RE.match(line.strip())
        if not m_rule:
            continue
        rule_id = m_rule.group(1).strip()
        if rule_id in seen:
            raise ValueError(
                f"Duplicate rule id '{rule_id}' at line {idx} (previous line {seen[rule_id]})"
            )
        seen[rule_id] = idx
        entries.append(
            {
                "rule_id": rule_id,
                "section": current_section,
                "line_start": idx,
                "line_end": idx,
            }
        )

    entries.sort(key=lambda e: (e["rule_id"], e["line_start"]))
    return entries


def main() -> int:
    script_dir = Path(__file__).resolve().parent
    default_spec = script_dir / ".." / "docs" / "Cursive0.md"
    default_out = script_dir / ".." / "spec" / "rules_index.json"

    parser = argparse.ArgumentParser(description="Extract rule ids from Cursive0.md")
    parser.add_argument("--spec", default=str(default_spec), help="Path to Cursive0.md")
    parser.add_argument("--out", default=str(default_out), help="Output JSON path")
    parser.add_argument("--check", action="store_true", help="Check output matches existing file")

    args = parser.parse_args()

    spec_path = Path(args.spec)
    out_path = Path(args.out)

    entries = parse_spec(spec_path)
    payload = {
        "source": str(spec_path.name),
        "entries": entries,
    }

    out_text = json.dumps(payload, indent=2, sort_keys=True, ensure_ascii=True)
    out_text = out_text + "\n"

    if args.check:
        if not out_path.exists():
            print(f"Missing output file: {out_path}", file=sys.stderr)
            return 1
        current = out_path.read_text(encoding="utf-8")
        if current != out_text:
            print("rules_index.json is out of date. Re-run spec_indexer.py.", file=sys.stderr)
            return 1
        return 0

    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(out_text, encoding="utf-8")
    return 0


if __name__ == "__main__":
    sys.exit(main())
