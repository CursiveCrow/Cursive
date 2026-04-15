#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import re
import stat
import sys
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path

from resolve_spec_path import resolve_spec_path


RULE_PATTERN = re.compile(r'SPEC_RULE(?:_AT)?\("([^"]+)"\)')
DIAG_CODE_PATTERN = re.compile(r"^[EWI]-[A-Z]{3}-[0-9]{4}$")
RULE_HEADER_PATTERN = re.compile(r"^\*\*\(([^)]+)\)\*\*$")
RULE_BAR_PATTERN = re.compile(r"^[─-]{3,}$")
PREMISE_SPLIT_PATTERN = re.compile(r"\s{4,}")


@dataclass
class StaticRuleEntry:
    rule_id: str
    conclusion_family: str
    diag_id: str | None
    source_path: str
    premises_text: str | None


def normalize_rel_path(base_path: Path, path: Path) -> str:
    return path.resolve().relative_to(base_path.resolve()).as_posix()


def escape_cpp_string(value: str) -> str:
    return (
        value.replace("\\", "\\\\")
        .replace('"', '\\"')
        .replace("\n", "\\n")
        .replace("\r", "\\r")
    )


def write_text_if_changed(path: Path, content: str) -> bool:
    if path.exists() and path.read_text(encoding="utf-8") == content:
        return False
    try:
        path.write_text(content, encoding="utf-8", newline="\n")
    except PermissionError:
        if not path.exists():
            raise
        # Windows source checkouts can leave tracked generated files read-only.
        path.chmod(path.stat().st_mode | stat.S_IWRITE)
        path.write_text(content, encoding="utf-8", newline="\n")
    return True


def parse_spec_rule_premises(spec_path: Path) -> dict[str, list[str]]:
    premises_by_rule: dict[str, list[str]] = {}
    lines = spec_path.read_text(encoding="utf-8-sig").splitlines()
    index = 0

    while index < len(lines):
        match = RULE_HEADER_PATTERN.match(lines[index].strip())
        if match is None:
            index += 1
            continue

        rule_id = match.group(1).strip()
        index += 1
        premise_lines: list[str] = []

        while index < len(lines):
            stripped = lines[index].strip()
            if not stripped:
                index += 1
                continue
            if RULE_BAR_PATTERN.match(stripped):
                break
            premise_lines.append(stripped)
            index += 1

        premises: list[str] = []
        for premise_line in premise_lines:
            premises.extend(
                part.strip()
                for part in PREMISE_SPLIT_PATTERN.split(premise_line)
                if part.strip()
            )
        premises_by_rule[rule_id] = premises

    return premises_by_rule


def resolve_rule_family(
    rule_id: str,
    source_rel: str,
    default_family: str,
    path_family_defaults: list[tuple[re.Pattern[str], str]],
    family_overrides: dict[str, str],
) -> str:
    if rule_id in family_overrides:
        return family_overrides[rule_id]
    for pattern, family in path_family_defaults:
        if pattern.search(source_rel):
            return family
    return default_family


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--repo-root", required=True)
    parser.add_argument("--spec-path", default="")
    parser.add_argument("--mapping-path", required=True)
    parser.add_argument("--output-path", required=True)
    parser.add_argument("--report-path", default="")
    parser.add_argument("--strict", action="store_true")
    return parser.parse_args()


def main() -> int:
    args = parse_args()

    repo_root = Path(args.repo_root).resolve()
    spec_path = Path(args.spec_path).resolve() if args.spec_path else resolve_spec_path(repo_root)
    mapping_path = Path(args.mapping_path).resolve()
    output_path = Path(args.output_path).resolve()
    report_path = Path(args.report_path).resolve() if args.report_path else None

    if not repo_root.exists():
        print(f"RepoRoot not found: {repo_root}", file=sys.stderr)
        return 1
    if not spec_path.exists():
        print(f"SpecPath not found: {spec_path}", file=sys.stderr)
        return 1
    if not mapping_path.exists():
        print(f"MappingPath not found: {mapping_path}", file=sys.stderr)
        return 1

    mapping = json.loads(mapping_path.read_text(encoding="utf-8"))
    premises_by_rule = parse_spec_rule_premises(spec_path)
    default_family = str(mapping.get("default_family", "")).strip()
    if not default_family:
        print("Mapping file missing default_family", file=sys.stderr)
        return 1

    path_family_defaults = [
        (re.compile(str(entry["regex"])), str(entry["family"]))
        for entry in mapping.get("path_family_defaults", [])
    ]
    family_overrides = {str(key): str(value) for key, value in mapping.get("rule_family_overrides", {}).items()}
    diag_overrides = {str(key): str(value) for key, value in mapping.get("rule_diag_overrides", {}).items()}
    rule_source_overrides = {str(key): str(value) for key, value in mapping.get("rule_source_overrides", {}).items()}

    source_root = repo_root / "cursive" / "src"
    if not source_root.exists():
        print(f"Source root not found: {source_root}", file=sys.stderr)
        return 1

    files = sorted(path for path in source_root.rglob("*") if path.suffix in {".cpp", ".h"} and path.is_file())

    rule_to_entry: dict[str, StaticRuleEntry] = {}
    rule_to_sources: dict[str, list[str]] = {}
    family_conflicts: list[object] = []
    unmapped_rules: list[str] = []

    for file_path in files:
        content = file_path.read_text(encoding="utf-8")
        matches = list(RULE_PATTERN.finditer(content))
        if not matches:
            continue

        source_rel = normalize_rel_path(source_root, file_path)

        for match in matches:
            rule_id = match.group(1)
            family = resolve_rule_family(rule_id, source_rel, default_family, path_family_defaults, family_overrides)
            if not family.strip():
                unmapped_rules.append(rule_id)
                continue

            diag_id = None
            if rule_id in diag_overrides:
                diag_id = diag_overrides[rule_id]
            elif DIAG_CODE_PATTERN.match(rule_id):
                diag_id = rule_id

            existing_sources = rule_to_sources.setdefault(rule_id, [])
            if source_rel not in existing_sources:
                existing_sources.append(source_rel)

            if rule_id in rule_to_entry:
                continue

            premises = premises_by_rule.get(rule_id)
            rule_to_entry[rule_id] = StaticRuleEntry(
                rule_id=rule_id,
                conclusion_family=family,
                diag_id=diag_id,
                source_path=source_rel,
                premises_text=None if premises is None else "\n".join(premises),
            )

    invalid_source_overrides: list[str] = []
    applied_source_overrides: dict[str, str] = {}

    for rule_id, preferred_source in rule_source_overrides.items():
        if rule_id not in rule_to_entry:
            invalid_source_overrides.append(f"{rule_id}:missing-rule")
            continue
        if preferred_source not in rule_to_sources.get(rule_id, []):
            invalid_source_overrides.append(f"{rule_id}:missing-source:{preferred_source}")
            continue

        entry = rule_to_entry[rule_id]
        if entry.source_path != preferred_source:
            rule_to_entry[rule_id] = StaticRuleEntry(
                rule_id=entry.rule_id,
                conclusion_family=resolve_rule_family(
                    rule_id,
                    preferred_source,
                    default_family,
                    path_family_defaults,
                    family_overrides,
                ),
                diag_id=entry.diag_id,
                source_path=preferred_source,
                premises_text=entry.premises_text,
            )
        applied_source_overrides[rule_id] = preferred_source

    sorted_rule_ids = sorted(rule_to_entry)
    output_path.parent.mkdir(parents=True, exist_ok=True)

    lines = [
        "// Auto-generated by cursive/tools/generate_static_rule_registry.py",
        "// DO NOT EDIT MANUALLY.",
        "static const StaticRuleMeta kStaticRules[] = {",
    ]
    for rule_id in sorted_rule_ids:
        entry = rule_to_entry[rule_id]
        diag_field = "std::nullopt"
        if entry.diag_id:
            diag_field = f'std::string_view("{escape_cpp_string(entry.diag_id)}")'
        premises_field = "std::nullopt"
        if entry.premises_text is not None:
            premises_field = f'std::string_view("{escape_cpp_string(entry.premises_text)}")'
        lines.append(
            '    {{"{0}", "{1}", {2}, "{3}", {4}}},'.format(
                escape_cpp_string(entry.rule_id),
                escape_cpp_string(entry.conclusion_family),
                diag_field,
                escape_cpp_string(entry.source_path),
                premises_field,
            )
        )
    lines.append("};")
    write_text_if_changed(output_path, "\n".join(lines) + "\n")

    duplicate_rule_ids = []
    for rule_id in sorted_rule_ids:
        sources = rule_to_sources[rule_id]
        if len(sources) > 1 and rule_id not in applied_source_overrides:
            duplicate_rule_ids.append(
                {
                    "rule_id": rule_id,
                    "source_count": len(sources),
                    "sources": sources,
                }
            )

    report = {
        "generated_at": datetime.now(timezone.utc).astimezone().isoformat(),
        "source_root": str(source_root),
        "rule_count": len(sorted_rule_ids),
        "unique_rule_count": len(sorted_rule_ids),
        "duplicate_rule_ids": duplicate_rule_ids,
        "applied_source_overrides": applied_source_overrides,
        "invalid_source_overrides": invalid_source_overrides,
        "family_conflicts": family_conflicts,
        "unmapped_rules": unmapped_rules,
        "output_path": str(output_path),
    }

    if report_path is not None:
        report_path.parent.mkdir(parents=True, exist_ok=True)
        report_path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8", newline="\n")

    if args.strict:
        if unmapped_rules:
            print(f"Strict mode failed: unmapped rules detected ({len(unmapped_rules)}).", file=sys.stderr)
            return 1
        if invalid_source_overrides:
            joined = ", ".join(invalid_source_overrides)
            print(f"Strict mode failed: invalid source overrides detected ({joined}).", file=sys.stderr)
            return 1

    print(f"[static-rule-registry] rules={len(sorted_rule_ids)} unmapped={len(unmapped_rules)} conflicts={len(family_conflicts)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
