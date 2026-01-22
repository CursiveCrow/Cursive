#!/usr/bin/env python3
import argparse
import json
import os
import shutil
import subprocess
import sys
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


def _read_spec_cov(path: Path) -> list[str]:
    cov: list[str] = []
    try:
        with path.open("r", encoding="utf-8", errors="replace") as f:
            for line in f:
                if line.startswith("# SPEC_COV:"):
                    cov.append(line.split(":", 1)[1].strip())
    except FileNotFoundError:
        return []
    return cov


def _has_any_spec_cov(project_root: Path) -> bool:
    for path in project_root.rglob("*.cursive"):
        if _read_spec_cov(path):
            return True
    return False


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


def _find_entry_file(project_root: Path) -> Path | None:
    main_path = project_root / "src" / "main.cursive"
    if main_path.exists():
        return main_path
    for path in project_root.rglob("*.cursive"):
        return path
    return None


def _normalize_output(text: str) -> str:
    return text.replace("\r\n", "\n").replace("\r", "\n").strip()


def _spec_verifier_path(repo_root: Path) -> Path:
    exe = "spec_verifier.exe" if os.name == "nt" else "spec_verifier"
    return repo_root / "spec_verifier" / "build" / "bin" / exe


def _run_spec_verifier(repo_root: Path) -> tuple[bool, str]:
    verifier = _spec_verifier_path(repo_root)
    if not verifier.exists():
        return False, f"spec_verifier not found: {verifier}"
    proc = subprocess.run(
        [str(verifier)],
        capture_output=True,
        text=True,
        cwd=repo_root,
    )
    if proc.returncode != 0:
        message = proc.stdout.strip() or proc.stderr.strip() or "verification failed"
        return False, f"spec_verifier failed: {message}"
    return True, ""


def _clear_diag_codes(repo_root: Path) -> None:
    (repo_root / "spec" / "diag_current.tsv").write_text("", encoding="utf-8")


def _write_rules_current(repo_root: Path, cov: list[str], domain: str) -> None:
    path = repo_root / "spec" / "verifier_rules_current.tsv"
    lines = ["# rule_id\tdomain\tmin_count\tmax_count\tpayload_keys"]
    for rule_id in cov:
        lines.append(f"{rule_id}\t{domain}\t1\t-\t-")
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def _write_edges_current(repo_root: Path) -> None:
    path = repo_root / "spec" / "verifier_edges_current.tsv"
    path.write_text("# before_rule\tafter_rule\tdomain\tgroup_by\n", encoding="utf-8")


def _run_compiled(
    compiler: Path,
    entry: Path,
    manifest: dict,
    project_root: Path,
    repo_root: Path,
) -> tuple[dict | None, str]:
    proc = subprocess.run(
        [str(compiler), str(entry)],
        capture_output=True,
        text=True,
        env=os.environ.copy(),
    )
    if proc.returncode != 0:
        return None, f"compiler failed: {proc.stderr.strip()}"
    if proc.stderr.strip():
        return None, f"compiler stderr not empty: {proc.stderr.strip()}"

    exe_path = _exe_path(project_root, manifest)
    if exe_path is None:
        return None, "unable to determine exe path"
    if not exe_path.exists():
        return None, f"exe not found: {exe_path}"

    env = os.environ.copy()
    env["CURSIVE_SPEC_TRACE_RUNTIME"] = str(repo_root / "spec" / "trace_current.tsv")
    run = subprocess.run(
        [str(exe_path)],
        capture_output=True,
        text=True,
        env=env,
        cwd=repo_root,
    )
    return {
        "stdout": run.stdout,
        "stderr": run.stderr,
        "exit_code": run.returncode,
    }, ""


def _run_test(
    compiler: Path,
    test_root: Path,
    repo_root: Path,
) -> tuple[bool, str]:
    if not _has_any_spec_cov(test_root):
        return False, f"{test_root}: missing SPEC_COV tag in .cursive sources"

    cov: list[str] = []
    for path in test_root.rglob("*.cursive"):
        cov.extend(_read_spec_cov(path))
    _write_rules_current(repo_root, cov, "runtime")
    _write_edges_current(repo_root)

    manifest = _load_manifest(test_root / "Cursive.toml")
    if manifest is None:
        return False, f"{test_root}: failed to parse Cursive.toml"

    expect_path = test_root / "expect.json"
    try:
        expect = json.loads(expect_path.read_text(encoding="utf-8"))
    except Exception as exc:
        return False, f"{test_root}: failed to parse expect.json: {exc}"

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

        compiled, msg = _run_compiled(compiler, entry, manifest, temp_root, repo_root)
        if compiled is None:
            return False, f"{test_root}: {msg}"

        comp_stdout = _normalize_output(str(compiled.get("stdout", "")))
        comp_stderr = _normalize_output(str(compiled.get("stderr", "")))
        comp_exit = int(compiled.get("exit_code", 0))

        exp_stdout = _normalize_output(str(expect.get("stdout", "")))
        exp_stderr = _normalize_output(str(expect.get("stderr", "")))
        exp_exit = int(expect.get("exit_code", 0))

        if exp_stdout != comp_stdout:
            return False, (
                f"{test_root}: stdout mismatch\n"
                f"expected: {exp_stdout!r}\n"
                f"actual:   {comp_stdout!r}"
            )
        if exp_stderr != comp_stderr:
            return False, (
                f"{test_root}: stderr mismatch\n"
                f"expected: {exp_stderr!r}\n"
                f"actual:   {comp_stderr!r}"
            )
        if exp_exit != comp_exit:
            return False, (
                f"{test_root}: exit_code mismatch\n"
                f"expected: {exp_exit}\n"
                f"actual:   {comp_exit}"
            )

        _clear_diag_codes(repo_root)
        ok, msg = _run_spec_verifier(repo_root)
        if not ok:
            return False, f"{test_root}: {msg}"

    return True, ""


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--compiler", required=False)
    parser.add_argument("--tests-root", default="tests/semantics_oracle")
    parser.add_argument("--test", action="append", default=[])
    args = parser.parse_args()

    repo_root = Path(__file__).resolve().parents[2]
    compiler = Path(args.compiler) if args.compiler else repo_root / "build" / "cursivec0"
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
        ok, msg = _run_test(compiler, test_root, repo_root)
        if not ok:
            print(msg)
            failures += 1

    if failures:
        print(f"{failures} semantics-oracle test(s) failed")
        return 1
    print("all semantics-oracle tests passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
