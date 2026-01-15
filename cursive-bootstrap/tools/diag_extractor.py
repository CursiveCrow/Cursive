#!/usr/bin/env python3
import argparse
import json
import re
import sys
from pathlib import Path

CODE_RE = re.compile(r"^[EWI]-[A-Z]{3}-[0-9]{4}$")
TABLE_RE = re.compile(r"^###\s+8\.\d+\.\s+([A-Z]-[A-Z]{3})")


def strip_ticks(text: str) -> str:
    text = text.strip()
    if text.startswith("`") and text.endswith("`") and len(text) >= 2:
        return text[1:-1].strip()
    return text


def parse_table_row(line: str) -> list:
    raw = line.strip()
    if not raw.startswith("|"):
        return []
    raw = raw.strip("|")
    return [cell.strip() for cell in raw.split("|")]


def parse_spec(spec_path: Path):
    text = spec_path.read_text(encoding="utf-8")
    lines = text.splitlines()

    entries = []
    seen = {}
    in_appendix = False
    current_table = None
    i = 0

    while i < len(lines):
        line = lines[i]

        if line.startswith("## 8. Appendix A - Diagnostic Codes"):
            in_appendix = True
            i += 1
            continue

        if in_appendix and line.startswith("## ") and not line.startswith("###"):
            break

        if in_appendix:
            m = TABLE_RE.match(line)
            if m:
                current_table = m.group(1)

            if line.lstrip().startswith("|") and "Code" in line and "DiagId" in line:
                headers = [h.lower().replace(" ", "") for h in parse_table_row(line)]
                if i + 1 >= len(lines):
                    raise ValueError("Unexpected end of file after table header")
                i += 1  # skip separator row
                i += 1
                if i >= len(lines):
                    break
                for j in range(i, len(lines)):
                    row_line = lines[j]
                    if not row_line.lstrip().startswith("|"):
                        i = j - 1
                        break
                    cells = parse_table_row(row_line)
                    if not cells:
                        continue
                    if len(cells) < len(headers):
                        raise ValueError(f"Row has too few cells: {row_line}")
                    if any(set(cell.strip()) <= set(":- ") for cell in cells):
                        continue

                    def idx(name: str) -> int:
                        name = name.lower().replace(" ", "")
                        if name not in headers:
                            raise ValueError(f"Missing column '{name}' in table header")
                        return headers.index(name)

                    code = strip_ticks(cells[idx("code")])
                    severity = strip_ticks(cells[idx("severity")])
                    detection = strip_ticks(cells[idx("detection")])
                    condition = strip_ticks(cells[idx("condition")])
                    diag_ids_cell = cells[idx("diagid")]

                    if not CODE_RE.match(code):
                        raise ValueError(f"Invalid diagnostic code format: {code}")

                    diag_ids = [strip_ticks(part) for part in diag_ids_cell.split(",")]
                    diag_ids = [d for d in (d.strip() for d in diag_ids) if d]
                    if not diag_ids:
                        raise ValueError(f"Missing DiagId in row: {row_line}")

                    for diag_id in diag_ids:
                        if diag_id in seen:
                            raise ValueError(
                                f"Duplicate DiagId '{diag_id}' in {current_table} and {seen[diag_id]}"
                            )
                        seen[diag_id] = current_table or ""
                        entries.append(
                            {
                                "diag_id": diag_id,
                                "code": code,
                                "severity": severity,
                                "detection": detection,
                                "condition": condition,
                                "table": current_table or "",
                            }
                        )
                i += 1
                continue

        i += 1

    entries.sort(key=lambda e: (e["diag_id"], e["code"]))
    return entries


def main() -> int:
    script_dir = Path(__file__).resolve().parent
    default_spec = script_dir / ".." / "docs" / "Cursive0.md"
    default_out = script_dir / ".." / "spec" / "diag_index.json"

    parser = argparse.ArgumentParser(description="Extract diagnostic mapping from Cursive0.md")
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
            print("diag_index.json is out of date. Re-run diag_extractor.py.", file=sys.stderr)
            return 1
        return 0

    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(out_text, encoding="utf-8")
    return 0


if __name__ == "__main__":
    sys.exit(main())
