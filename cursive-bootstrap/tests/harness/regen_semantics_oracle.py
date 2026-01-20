#!/usr/bin/env python3
import argparse
import json
import os
import subprocess
import tempfile
from pathlib import Path


def _discover_tests(root: Path) -> list[Path]:
    tests: list[Path] = []
    for manifest in root.rglob("Cursive.toml"):
        test_root = manifest.parent
        expect = test_root / "expect.json"
        if expect.exists():
            tests.append(test_root)
    return sorted(tests)


def _strip_spec_cov_lines(path: Path) -> str:
    lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
    out: list[str] = []
    for line in lines:
        if line.startswith("# SPEC_COV:"):
            continue
        out.append(line)
    return "\n".join(out)


def _copy_filtered_project(src_root: Path, dst_root: Path) -> None:
    for path in src_root.rglob("*"):
        rel = path.relative_to(src_root)
        out_path = dst_root / rel
        if path.is_dir():
            out_path.mkdir(parents=True, exist_ok=True)
            continue
        out_path.parent.mkdir(parents=True, exist_ok=True)
        if path.suffix == ".cursive":
            out_path.write_text(_strip_spec_cov_lines(path), encoding="utf-8")
        else:
            out_path.write_bytes(path.read_bytes())


def _find_entry_file(project_root: Path) -> Path | None:
    main_path = project_root / "src" / "main.cursive"
    if main_path.exists():
        return main_path
    for path in project_root.rglob("*.cursive"):
        return path
    return None


def _normalize_output(text: str) -> str:
    return text.replace("\r\n", "\n").replace("\r", "\n").strip()


def _run_interpreter(interpreter: Path, entry: Path) -> tuple[dict | None, str]:
    proc = subprocess.run(
        [str(interpreter), str(entry)],
        capture_output=True,
        text=True,
        env=os.environ.copy(),
    )
    if proc.returncode != 0:
        return None, f"interpreter failed: {proc.stderr.strip() or proc.stdout.strip()}"
    if proc.stderr.strip():
        return None, f"interpreter stderr not empty: {proc.stderr.strip()}"
    try:
        payload = json.loads(proc.stdout.strip())
    except Exception as exc:
        return None, f"interpreter JSON parse failed: {exc}"
    if not isinstance(payload, dict):
        return None, "interpreter JSON is not an object"
    return payload, ""


def _regen_test(interpreter: Path, test_root: Path) -> tuple[bool, str]:
    with tempfile.TemporaryDirectory() as tmpdir:
        temp_root = Path(tmpdir) / "project"
        temp_root.mkdir(parents=True, exist_ok=True)
        _copy_filtered_project(test_root, temp_root)

        entry = _find_entry_file(temp_root)
        if entry is None:
            return False, f"{test_root}: no .cursive files found"

        payload, msg = _run_interpreter(interpreter, entry)
        if payload is None:
            return False, f"{test_root}: {msg}"

        stdout = _normalize_output(str(payload.get("stdout", "")))
        stderr = _normalize_output(str(payload.get("stderr", "")))
        try:
            exit_code = int(payload.get("exit_code", 0))
        except Exception:
            return False, f"{test_root}: interpreter exit_code not an int"

        out = {
            "stdout": stdout,
            "stderr": stderr,
            "exit_code": exit_code,
        }
        expect_path = test_root / "expect.json"
        expect_path.write_text(json.dumps(out, indent=2) + "\n", encoding="utf-8")

    return True, ""


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--interpreter", required=True)
    parser.add_argument("--tests-root", default="tests/e2e")
    parser.add_argument("--test", action="append", default=[])
    args = parser.parse_args()

    repo_root = Path(__file__).resolve().parents[2]
    interpreter = Path(args.interpreter)
    if not interpreter.exists():
        print(f"interpreter not found: {interpreter}")
        return 2

    tests_root = repo_root / args.tests_root
    if args.test:
        test_paths = [Path(p) for p in args.test]
    else:
        test_paths = _discover_tests(tests_root)

    failures = 0
    for test_root in test_paths:
        ok, msg = _regen_test(interpreter, test_root)
        if not ok:
            print(msg)
            failures += 1

    if failures:
        print(f"{failures} semantics-oracle expectation(s) failed to regen")
        return 1
    print("all semantics-oracle expectations regenerated")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
