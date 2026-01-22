#!/usr/bin/env python3
"""
Diagnostic code coverage report for Cursive0 tests.

This script extracts all diagnostic codes from the specification (Cursive0.md)
and compares them against diagnostic codes tested in the test suite.

Usage:
  python tools/diag_coverage_report.py [--verbose] [--gaps-only]

Options:
  --verbose     Show detailed information about each diagnostic code
  --gaps-only   Only show missing diagnostic codes
"""

import json
import os
import re
import sys
from pathlib import Path
from typing import Dict, Set, List, Tuple


def extract_spec_diag_codes(spec_path: Path) -> Dict[str, Dict[str, str]]:
    """Extract all diagnostic codes from the specification.

    Returns:
        Dict mapping code to {severity, message, rule_ids}
    """
    diag_codes = {}
    cursive_md = spec_path / "docs" / "Cursive0.md"

    if not cursive_md.exists():
        print(f"Error: {cursive_md} not found", file=sys.stderr)
        return diag_codes

    # Pattern for diagnostic code table rows:
    # | `E-XXX-YYYY` | Error/Warning | ... | message | rule_ids |
    table_pattern = re.compile(
        r'\|\s*`((?:E|W)-[A-Z]{2,4}-\d{4})`\s*\|\s*(Error|Warning)\s*\|[^|]*\|\s*([^|]+)\s*\|([^|]*)\|'
    )

    with open(cursive_md, "r", encoding="utf-8") as f:
        content = f.read()

    for match in table_pattern.finditer(content):
        code = match.group(1)
        severity = match.group(2).lower()
        message = match.group(3).strip()
        rule_ids = match.group(4).strip()

        diag_codes[code] = {
            "severity": severity,
            "message": message,
            "rule_ids": rule_ids
        }

    return diag_codes


def extract_tested_diag_codes(tests_path: Path) -> Set[str]:
    """Extract all diagnostic codes that are tested in expect.json files.

    Returns:
        Set of diagnostic codes found in test expectations
    """
    tested_codes = set()

    # Find all .expect.json files
    for expect_file in tests_path.rglob("*.expect.json"):
        try:
            with open(expect_file, "r", encoding="utf-8") as f:
                data = json.load(f)

            # Extract codes from diagnostics array
            for diag in data.get("diagnostics", []):
                code = diag.get("code", "")
                if code:
                    tested_codes.add(code)
        except (json.JSONDecodeError, Exception):
            pass

    # Also check semantics-oracle expect.json files (they have different structure)
    for expect_file in tests_path.rglob("expect.json"):
        try:
            with open(expect_file, "r", encoding="utf-8") as f:
                data = json.load(f)

            # Semantics-oracle tests don't typically have diagnostic codes, skip
        except Exception:
            pass

    return tested_codes


def categorize_codes(codes: Set[str]) -> Dict[str, Set[str]]:
    """Categorize diagnostic codes by prefix."""
    categories = {}
    for code in codes:
        # Extract prefix like "E-SRC", "E-TYP", "W-MOD"
        parts = code.split("-")
        if len(parts) >= 2:
            prefix = f"{parts[0]}-{parts[1]}"
            categories.setdefault(prefix, set()).add(code)
    return categories


def print_report(
    spec_codes: Dict[str, Dict[str, str]],
    tested_codes: Set[str],
    verbose: bool = False,
    gaps_only: bool = False
):
    """Print the diagnostic coverage report."""
    all_spec_codes = set(spec_codes.keys())

    covered = all_spec_codes & tested_codes
    missing = all_spec_codes - tested_codes
    extra = tested_codes - all_spec_codes

    total = len(all_spec_codes)
    coverage_pct = len(covered) / total * 100 if total > 0 else 0

    print("\n" + "=" * 60)
    print("DIAGNOSTIC CODE COVERAGE REPORT")
    print("=" * 60)
    print(f"\nTotal diagnostic codes in spec: {total}")
    print(f"Tested diagnostic codes: {len(covered)}")
    print(f"Missing diagnostic codes: {len(missing)}")
    print(f"Coverage: {coverage_pct:.1f}%")

    if missing:
        print("\n" + "-" * 60)
        print("MISSING DIAGNOSTIC CODE TESTS")
        print("-" * 60)

        # Group by category
        by_category = categorize_codes(missing)
        for category in sorted(by_category.keys()):
            print(f"\n{category}:")
            for code in sorted(by_category[category]):
                info = spec_codes[code]
                if verbose:
                    print(f"  {code}: {info['message']}")
                    if info['rule_ids']:
                        print(f"    Rules: {info['rule_ids']}")
                else:
                    print(f"  - {code}")

    if not gaps_only and covered:
        print("\n" + "-" * 60)
        print("COVERED DIAGNOSTIC CODES")
        print("-" * 60)

        by_category = categorize_codes(covered)
        for category in sorted(by_category.keys()):
            count = len(by_category[category])
            print(f"\n{category}: {count} tests")
            if verbose:
                for code in sorted(by_category[category]):
                    print(f"  - {code}")

    if extra:
        print("\n" + "-" * 60)
        print("EXTRA CODES (not in spec)")
        print("-" * 60)
        for code in sorted(extra):
            print(f"  - {code}")

    # Print summary by error category
    print("\n" + "-" * 60)
    print("COVERAGE BY CATEGORY")
    print("-" * 60)

    all_categories = set()
    for code in all_spec_codes:
        parts = code.split("-")
        if len(parts) >= 2:
            all_categories.add(f"{parts[0]}-{parts[1]}")

    for category in sorted(all_categories):
        spec_in_cat = {c for c in all_spec_codes if c.startswith(category)}
        tested_in_cat = {c for c in covered if c.startswith(category)}
        cat_pct = len(tested_in_cat) / len(spec_in_cat) * 100 if spec_in_cat else 0

        status = "COMPLETE" if len(tested_in_cat) == len(spec_in_cat) else f"{len(tested_in_cat)}/{len(spec_in_cat)}"
        print(f"  {category}: {status} ({cat_pct:.0f}%)")


def main():
    args = sys.argv[1:]

    if "--help" in args or "-h" in args:
        print(__doc__)
        return 0

    verbose = "--verbose" in args or "-v" in args
    gaps_only = "--gaps-only" in args

    # Find project root
    script_path = Path(__file__).resolve()
    root_path = script_path.parent.parent

    print("Extracting diagnostic codes from specification...")
    spec_codes = extract_spec_diag_codes(root_path)
    print(f"Found {len(spec_codes)} diagnostic codes in spec")

    print("Scanning test expectations for tested diagnostic codes...")
    tested_codes = extract_tested_diag_codes(root_path / "tests")
    print(f"Found {len(tested_codes)} tested diagnostic codes")

    print_report(spec_codes, tested_codes, verbose, gaps_only)

    # Exit with error if not 100% coverage
    all_spec = set(spec_codes.keys())
    missing = all_spec - tested_codes
    if missing:
        return 1

    print("\nAll diagnostic codes are tested!")
    return 0


if __name__ == "__main__":
    sys.exit(main())
