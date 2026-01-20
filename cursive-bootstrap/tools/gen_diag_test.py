#!/usr/bin/env python3
"""
Test template generator for Cursive0 diagnostic codes.

This script generates test file templates for a given diagnostic code.

Usage:
  python tools/gen_diag_test.py <diag-code> <test-name> [--phase N]

Example:
  python tools/gen_diag_test.py E-TYP-1601 const_mutation_err --phase 3

This will create:
  tests/compile/phase3/<test-name>/Cursive.toml
  tests/compile/phase3/<test-name>/src/main.cursive
  tests/compile/phase3/<test-name>/src/main.cursive.expect.json
"""

import json
import os
import re
import sys
from pathlib import Path


def parse_diag_code(code: str) -> dict:
    """Parse a diagnostic code into its components."""
    # Pattern: E-XXX-YYYY or W-XXX-YYYY
    match = re.match(r'^(E|W)-([A-Z]{2,4})-(\d{4})$', code)
    if not match:
        return None

    return {
        "severity": "error" if match.group(1) == "E" else "warning",
        "category": match.group(2),
        "number": match.group(3),
        "full": code
    }


def get_spec_rule_for_code(root_path: Path, code: str) -> str:
    """Try to find the spec rule ID for a diagnostic code."""
    spec_md = root_path / "docs" / "Cursive0.md"
    if not spec_md.exists():
        return ""

    # Pattern to find the diagnostic code and its associated rule
    pattern = re.compile(
        rf'\|\s*`{re.escape(code)}`\s*\|[^|]*\|[^|]*\|[^|]*\|([^|]*)\|'
    )

    try:
        with open(spec_md, "r", encoding="utf-8") as f:
            content = f.read()
        match = pattern.search(content)
        if match:
            return match.group(1).strip()
    except Exception:
        pass

    return ""


def generate_test(
    root_path: Path,
    diag_code: str,
    test_name: str,
    phase: int = 3
):
    """Generate test template files."""
    parsed = parse_diag_code(diag_code)
    if not parsed:
        print(f"Error: Invalid diagnostic code format: {diag_code}")
        print("Expected format: E-XXX-YYYY or W-XXX-YYYY")
        return False

    # Determine test directory
    test_dir = root_path / "tests" / "compile" / f"phase{phase}" / test_name
    src_dir = test_dir / "src"

    # Check if test already exists
    if test_dir.exists():
        print(f"Error: Test directory already exists: {test_dir}")
        return False

    # Get spec rule if available
    spec_rule = get_spec_rule_for_code(root_path, diag_code)

    # Create directories
    src_dir.mkdir(parents=True, exist_ok=True)

    # Generate Cursive.toml
    toml_content = """\
[assembly]
name = "demo"
kind = "executable"
root = "src"
"""
    (test_dir / "Cursive.toml").write_text(toml_content, encoding="utf-8")

    # Generate main.cursive
    spec_cov_line = f"# SPEC_COV: {spec_rule}" if spec_rule else "# SPEC_COV: TODO-AddRuleId"
    severity_word = "Error" if parsed["severity"] == "error" else "Warning"

    cursive_content = f"""\
{spec_cov_line}
# {diag_code}: {severity_word} test.
# TODO: Add description of what this test validates.
public procedure main(ctx: Context) -> i32 {{
  // TODO: Add code that triggers {diag_code}
  return 0i32;
}}
"""
    (src_dir / "main.cursive").write_text(cursive_content, encoding="utf-8")

    # Generate expect.json
    exit_code = 1 if parsed["severity"] == "error" else 0
    expect_content = {
        "diagnostics": [
            {
                "code": diag_code,
                "severity": parsed["severity"],
                "message": f"TODO: Add expected message for {diag_code}",
                "span": None
            }
        ],
        "exit_code": exit_code
    }
    (src_dir / "main.cursive.expect.json").write_text(
        json.dumps(expect_content, indent=2) + "\n",
        encoding="utf-8"
    )

    print(f"Generated test template at: {test_dir}")
    print(f"  - {test_dir / 'Cursive.toml'}")
    print(f"  - {src_dir / 'main.cursive'}")
    print(f"  - {src_dir / 'main.cursive.expect.json'}")
    print(f"\nNext steps:")
    print(f"  1. Edit main.cursive to add code that triggers {diag_code}")
    print(f"  2. Update the expect.json message to match expected output")
    print(f"  3. Update the SPEC_COV marker with the correct rule ID")
    return True


def main():
    args = sys.argv[1:]

    if "--help" in args or "-h" in args or len(args) < 2:
        print(__doc__)
        return 0 if "--help" in args or "-h" in args else 1

    diag_code = args[0]
    test_name = args[1]
    phase = 3

    # Parse --phase argument
    if "--phase" in args:
        idx = args.index("--phase")
        if idx + 1 < len(args):
            try:
                phase = int(args[idx + 1])
            except ValueError:
                print("Error: --phase requires a number", file=sys.stderr)
                return 1

    # Find project root
    script_path = Path(__file__).resolve()
    root_path = script_path.parent.parent

    if generate_test(root_path, diag_code, test_name, phase):
        return 0
    else:
        return 1


if __name__ == "__main__":
    sys.exit(main())
