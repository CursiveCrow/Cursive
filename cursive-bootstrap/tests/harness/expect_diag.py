#!/usr/bin/env python3
import json
import sys
from typing import Any


def _normalize_severity(value: str) -> str:
    return value.strip().lower()


def _compare_diag(actual: dict[str, Any], expected: dict[str, Any], idx: int) -> list[str]:
    errors: list[str] = []
    for key, expected_value in expected.items():
        if key not in actual:
            errors.append(f"diag[{idx}]: missing key '{key}'")
            continue
        actual_value = actual[key]
        if key == "severity":
            if _normalize_severity(str(actual_value)) != _normalize_severity(str(expected_value)):
                errors.append(
                    f"diag[{idx}]: severity mismatch (expected {expected_value}, got {actual_value})"
                )
            continue
        if actual_value != expected_value:
            errors.append(
                f"diag[{idx}]: {key} mismatch (expected {expected_value}, got {actual_value})"
            )
    return errors


def main() -> int:
    if len(sys.argv) != 3:
        print("usage: expect_diag.py <expected.json> <actual.json>")
        return 2

    expected_path = sys.argv[1]
    actual_path = sys.argv[2]

    with open(expected_path, "r", encoding="utf-8") as f:
        expected = json.load(f)

    with open(actual_path, "r", encoding="utf-8") as f:
        actual = json.load(f)

    errors: list[str] = []

    if "exit_code" in expected:
        if actual.get("exit_code") != expected["exit_code"]:
            errors.append(
                f"exit_code mismatch (expected {expected['exit_code']}, got {actual.get('exit_code')})"
            )

    expected_diags = expected.get("diagnostics", [])
    actual_diags = actual.get("diagnostics", [])
    required_keys = {"code", "severity"}
    for idx, exp in enumerate(expected_diags):
        if not isinstance(exp, dict):
            errors.append(f"diag[{idx}]: expected diagnostic is not an object")
            continue
        missing = required_keys - set(exp.keys())
        if missing:
            missing_list = ", ".join(sorted(missing))
            errors.append(f"diag[{idx}]: missing required keys in expected: {missing_list}")
    if len(actual_diags) != len(expected_diags):
        errors.append(
            f"diagnostics length mismatch (expected {len(expected_diags)}, got {len(actual_diags)})"
        )
    else:
        for idx, (act, exp) in enumerate(zip(actual_diags, expected_diags)):
            errors.extend(_compare_diag(act, exp, idx))

    if errors:
        for err in errors:
            print(err)
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
