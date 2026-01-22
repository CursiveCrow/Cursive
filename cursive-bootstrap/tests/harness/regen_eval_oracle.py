#!/usr/bin/env python3
import argparse
import json
import os
import shutil
import subprocess
import tempfile
import tomllib
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
        if line.startswith("#"):
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


def _load_manifest(manifest_path: Path) -> dict | None:
    try:
        return tomllib.loads(manifest_path.read_text(encoding="utf-8"))
    except Exception:
        return None


def _select_assembly(manifest: dict, target: str | None) -> dict | None:
    assemblies = manifest.get("assembly") or manifest.get("assemblies")
    if isinstance(assemblies, dict):
        assemblies = [assemblies]
    if not isinstance(assemblies, list):
        return None
    if target is None:
        return assemblies[0] if len(assemblies) == 1 else None
    for asm in assemblies:
        if asm.get("name") == target:
            return asm
    return None


def _exe_path(project_root: Path, manifest: dict) -> Path | None:
    assembly = _select_assembly(manifest, None)
    if assembly is None:
        return None
    name = assembly.get("name")
    kind = assembly.get("kind")
    if not isinstance(name, str) or not isinstance(kind, str):
        return None
    if kind != "executable":
        return None
    out_dir = assembly.get("out_dir")
    root = project_root / out_dir if isinstance(out_dir, str) else project_root / "build"
    return root / "bin" / f"{name}.exe"


def _regen_test(compiler: Path, test_root: Path, repo_root: Path) -> tuple[bool, str]:
    manifest = _load_manifest(test_root / "Cursive.toml")
    if manifest is None:
        return False, f"{test_root}: failed to parse Cursive.toml"

    runtime_lib = repo_root / "runtime" / "cursive0_rt.lib"
    if not runtime_lib.exists():
        return False, f"runtime library not found: {runtime_lib}"

    with tempfile.TemporaryDirectory() as tmpdir:
        temp_root = Path(tmpdir) / "project"
        temp_root.mkdir(parents=True, exist_ok=True)
        _copy_filtered_project(test_root, temp_root)

        runtime_dir = temp_root / "runtime"
        runtime_dir.mkdir(parents=True, exist_ok=True)
        shutil.copy2(runtime_lib, runtime_dir / "cursive0_rt.lib")

        entry = _find_entry_file(temp_root)
        if entry is None:
            return False, f"{test_root}: no .cursive files found"

        proc = subprocess.run(
            [str(compiler), str(entry)],
            capture_output=True,
            text=True,
            env=os.environ.copy(),
        )
        if proc.returncode != 0:
            return False, f"{test_root}: compiler failed: {proc.stderr.strip()}"
        if proc.stderr.strip():
            return False, f"{test_root}: compiler stderr not empty: {proc.stderr.strip()}"

        exe_path = _exe_path(temp_root, manifest)
        if exe_path is None:
            return False, f"{test_root}: unable to determine exe path"
        if not exe_path.exists():
            return False, f"{test_root}: exe not found: {exe_path}"

        run = subprocess.run([str(exe_path)], capture_output=True, text=True)
        stdout = _normalize_output(str(run.stdout))
        stderr = _normalize_output(str(run.stderr))
        exit_code = run.returncode

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
    parser.add_argument("--compiler", required=True)
    parser.add_argument("--tests-root", default="tests/semantics_oracle")
    parser.add_argument("--test", action="append", default=[])
    args = parser.parse_args()

    repo_root = Path(__file__).resolve().parents[2]
    compiler = Path(args.compiler)
    if not compiler.exists():
        print(f"compiler not found: {compiler}")
        return 2

    tests_root = repo_root / args.tests_root
    if args.test:
        test_paths = [Path(p) for p in args.test]
    else:
        test_paths = _discover_tests(tests_root)

    failures = 0
    for test_root in test_paths:
        ok, msg = _regen_test(compiler, test_root, repo_root)
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
