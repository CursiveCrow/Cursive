#!/usr/bin/env python3
"""
Coverage checker for Cursive0 spec rules.

This script scans source files for SPEC_RULE anchors (implementation coverage)
and test files for SPEC_COV anchors (test coverage), then compares them against
the rules defined in spec/rules_index.json.

Usage:
  python tools/coverage_check.py [--report] [--phase N] [--phase3-only] [--help]

Options:
  --report       Generate detailed coverage report
  --phase N      Filter rules by phase (1, 3, 4, etc.)
  --phase3-only  Only check Phase 3 rules (sections 5.x)
  --help         Show this help message
"""

import json
import os
import re
import sys
from pathlib import Path
from typing import Dict, Set, Tuple, List


def load_rules_index(spec_path: Path) -> Dict[str, str]:
    """Load rules from spec/rules_index.json.
    
    Returns:
        Dict mapping rule_id to section number
    """
    rules_file = spec_path / "rules_index.json"
    if not rules_file.exists():
        print(f"Error: {rules_file} not found", file=sys.stderr)
        sys.exit(1)
    
    with open(rules_file, "r", encoding="utf-8") as f:
        data = json.load(f)
    
    rules = {}
    for entry in data.get("entries", []):
        rule_id = entry.get("rule_id", "")
        section = entry.get("section", "")
        if rule_id:
            rules[rule_id] = section
    return rules


def section_to_phase(section: str) -> int:
    """Map section number to phase.
    
    Sections 1.x = Phase 0 (no check)
    Sections 2.x = Phase 0 
    Sections 3.x = Phase 1
    Sections 4.x = Phase 2 (deferred)
    Sections 5.x = Phase 3
    Sections 6.x = Phase 4
    Sections 7.x = Phase 4 (dynamic semantics)
    """
    if not section:
        return -1
    
    try:
        major = int(section.split(".")[0])
    except (ValueError, IndexError):
        return -1
    
    if major == 3:
        return 1
    elif major == 4:
        return 2
    elif major == 5:
        return 3
    elif major in (6, 7):
        return 4
    else:
        return 0


def find_spec_anchors(root_path: Path, pattern: str, extensions: List[str]) -> Set[str]:
    """Find all SPEC_RULE or SPEC_COV anchors in source files.
    
    Args:
        root_path: Root directory to search
        pattern: Regex pattern to match (SPEC_RULE or SPEC_COV)
        extensions: File extensions to search
    
    Returns:
        Set of rule IDs found
    """
    anchors = set()
    regex = re.compile(pattern + r'\s*\(\s*"([^"]+)"\s*\)')
    
    for ext in extensions:
        for file_path in root_path.rglob(f"*{ext}"):
            # Skip third_party and build directories
            if "third_party" in str(file_path) or "build" in str(file_path):
                continue
            
            try:
                with open(file_path, "r", encoding="utf-8", errors="ignore") as f:
                    content = f.read()
                    for match in regex.finditer(content):
                        anchors.add(match.group(1))
            except Exception:
                pass
    
    return anchors


def find_spec_rules(root_path: Path) -> Set[str]:
    """Find SPEC_RULE anchors in implementation files."""
    return find_spec_anchors(
        root_path,
        r"SPEC_RULE",
        [".c", ".cpp", ".h", ".hpp"]
    )


def find_spec_cov(root_path: Path) -> Set[str]:
    """Find SPEC_COV anchors in test files and compile tests."""
    test_anchors = find_spec_anchors(
        root_path / "tests",
        r"SPEC_COV",
        [".cpp", ".h", ".cursive"]
    )
    
    # Also check for # SPEC_COV: lines in .cursive files
    cov_comment_regex = re.compile(r"#\s*SPEC_COV:\s*(\S+)")
    for file_path in (root_path / "tests").rglob("*.cursive"):
        try:
            with open(file_path, "r", encoding="utf-8", errors="ignore") as f:
                content = f.read()
                for match in cov_comment_regex.finditer(content):
                    test_anchors.add(match.group(1))
        except Exception:
            pass
    
    return test_anchors


def check_coverage(
    all_rules: Dict[str, str],
    impl_anchors: Set[str],
    test_anchors: Set[str],
    phase_filter: int = -1
) -> Tuple[Set[str], Set[str], Set[str], Set[str]]:
    """Check implementation and test coverage.
    
    Returns:
        Tuple of (missing_impl, missing_test, impl_ok, test_ok)
    """
    missing_impl = set()
    missing_test = set()
    impl_ok = set()
    test_ok = set()
    
    for rule_id, section in all_rules.items():
        phase = section_to_phase(section)
        
        # Skip if filtering by phase
        if phase_filter >= 0 and phase != phase_filter:
            continue
        
        if rule_id in impl_anchors:
            impl_ok.add(rule_id)
        else:
            missing_impl.add(rule_id)
        
        if rule_id in test_anchors:
            test_ok.add(rule_id)
        else:
            missing_test.add(rule_id)
    
    return missing_impl, missing_test, impl_ok, test_ok


def print_report(
    all_rules: Dict[str, str],
    missing_impl: Set[str],
    missing_test: Set[str],
    impl_ok: Set[str],
    test_ok: Set[str],
    phase_filter: int
):
    """Print detailed coverage report."""
    total = len(missing_impl) + len(impl_ok)
    if total == 0:
        print("No rules found for selected filter.")
        return
    
    impl_pct = len(impl_ok) / total * 100
    test_pct = len(test_ok) / total * 100
    
    phase_name = "All" if phase_filter < 0 else f"Phase {phase_filter}"
    print(f"\n=== Coverage Report ({phase_name}) ===")
    print(f"Total rules: {total}")
    print(f"Implementation coverage: {len(impl_ok)}/{total} ({impl_pct:.1f}%)")
    print(f"Test coverage: {len(test_ok)}/{total} ({test_pct:.1f}%)")
    
    if missing_impl:
        print(f"\n--- Missing SPEC_RULE ({len(missing_impl)}) ---")
        # Group by section
        by_section: Dict[str, List[str]] = {}
        for rule_id in sorted(missing_impl):
            section = all_rules.get(rule_id, "Sigma")
            by_section.setdefault(section, []).append(rule_id)
        
        for section in sorted(by_section.keys()):
            print(f"\n§{section}:")
            for rule_id in by_section[section]:
                print(f"  - {rule_id}")
    
    if missing_test:
        print(f"\n--- Missing SPEC_COV ({len(missing_test)}) ---")
        by_section: Dict[str, List[str]] = {}
        for rule_id in sorted(missing_test):
            section = all_rules.get(rule_id, "Sigma")
            by_section.setdefault(section, []).append(rule_id)
        
        for section in sorted(by_section.keys()):
            print(f"\n§{section}:")
            for rule_id in by_section[section]:
                print(f"  - {rule_id}")


def main():
    args = sys.argv[1:]
    
    if "--help" in args or "-h" in args:
        print(__doc__)
        return 0
    
    report_mode = "--report" in args
    phase_filter = -1
    
    if "--phase3-only" in args:
        phase_filter = 3
    elif "--phase" in args:
        idx = args.index("--phase")
        if idx + 1 < len(args):
            try:
                phase_filter = int(args[idx + 1])
            except ValueError:
                print("Error: --phase requires a number", file=sys.stderr)
                return 1
    
    # Find project root
    script_path = Path(__file__).resolve()
    root_path = script_path.parent.parent
    spec_path = root_path / "spec"
    
    if not spec_path.exists():
        print(f"Error: spec directory not found at {spec_path}", file=sys.stderr)
        return 1
    
    print("Loading rules index...")
    all_rules = load_rules_index(spec_path)
    print(f"Found {len(all_rules)} rules in spec/rules_index.json")
    
    print("Scanning implementation files for SPEC_RULE anchors...")
    impl_anchors = find_spec_rules(root_path)
    print(f"Found {len(impl_anchors)} SPEC_RULE anchors")
    
    print("Scanning test files for SPEC_COV anchors...")
    test_anchors = find_spec_cov(root_path)
    print(f"Found {len(test_anchors)} SPEC_COV anchors")
    
    missing_impl, missing_test, impl_ok, test_ok = check_coverage(
        all_rules, impl_anchors, test_anchors, phase_filter
    )
    
    total = len(missing_impl) + len(impl_ok)
    if total == 0:
        print("No rules match the selected filter.")
        return 0
    
    impl_pct = len(impl_ok) / total * 100
    test_pct = len(test_ok) / total * 100
    
    phase_name = "All phases" if phase_filter < 0 else f"Phase {phase_filter}"
    print(f"\n{phase_name} Summary:")
    print(f"  SPEC_RULE: {len(impl_ok)}/{total} ({impl_pct:.1f}%)")
    print(f"  SPEC_COV:  {len(test_ok)}/{total} ({test_pct:.1f}%)")
    
    if report_mode:
        print_report(all_rules, missing_impl, missing_test, impl_ok, test_ok, phase_filter)
    
    # Exit with error if not 100% coverage
    if missing_impl or missing_test:
        return 1
    
    print("\n✅ Full coverage achieved!")
    return 0


if __name__ == "__main__":
    sys.exit(main())
