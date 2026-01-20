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
    tests = []
    for path in root.rglob("Cursive.toml"):
        if path.is_file():
            tests.append(path.parent)
    return sorted(set(tests))


def _strip_spec_cov_lines(text: str) -> str:
    out: list[str] = []
    for line in text.splitlines():
        if line.startswith("# SPEC_COV:"):
            continue
        if line.startswith("# ASSEMBLY:"):
            continue
        if line.startswith("# OUTPUT_PIPELINE:"):
            continue
        out.append(line)
    return "\n".join(out)


def _copy_project(src_root: Path, dst_root: Path) -> None:
    dst_root.mkdir(parents=True, exist_ok=True)
    (dst_root / "src").mkdir(parents=True, exist_ok=True)

    manifest = src_root / "Cursive.toml"
    if manifest.exists():
        (dst_root / "Cursive.toml").write_bytes(manifest.read_bytes())

    src_dir = src_root / "src"
    if not src_dir.exists():
        return

    for path in src_dir.rglob("*"):
        rel = path.relative_to(src_dir)
        out_path = dst_root / "src" / rel
        if path.is_dir():
            out_path.mkdir(parents=True, exist_ok=True)
            continue
        if path.suffix == ".cursive":
            out_path.parent.mkdir(parents=True, exist_ok=True)
            text = path.read_text(encoding="utf-8", errors="replace")
            out_path.write_text(_strip_spec_cov_lines(text), encoding="utf-8")
        else:
            out_path.parent.mkdir(parents=True, exist_ok=True)
            out_path.write_bytes(path.read_bytes())


def _load_manifest(manifest_path: Path) -> dict | None:
    try:
        return tomllib.loads(manifest_path.read_text(encoding="utf-8"))
    except Exception:
        return None


def _select_assembly(manifest: dict) -> dict | None:
    assemblies = manifest.get("assembly") or manifest.get("assemblies")
    if isinstance(assemblies, dict):
        assemblies = [assemblies]
    if not isinstance(assemblies, list) or not assemblies:
        return None
    if len(assemblies) == 1:
        return assemblies[0]
    return assemblies[0]


def _exe_path(project_root: Path) -> Path | None:
    manifest = _load_manifest(project_root / "Cursive.toml")
    if manifest is None:
        return None
    assembly = _select_assembly(manifest)
    if assembly is None:
        return None
    name = assembly.get("name")
    if not isinstance(name, str) or not name:
        return None
    out_dir = assembly.get("out_dir")
    if isinstance(out_dir, str) and out_dir:
        root = project_root / out_dir
    else:
        root = project_root / "build"
    return root / "bin" / f"{name}.exe"


def _read_expected(path: Path) -> dict:
    if not path.exists():
        return {"stdout": "", "stderr": "", "exit_code": 0}
    data = json.loads(path.read_text(encoding="utf-8"))
    return {
        "stdout": data.get("stdout", ""),
        "stderr": data.get("stderr", ""),
        "exit_code": data.get("exit_code", 0),
    }


def _normalize(text: str) -> str:
    return text.replace("\r\n", "\n").replace("\r", "\n").strip()


def _has_spec_cov(src_root: Path) -> bool:
    for path in src_root.rglob("*.cursive"):
        try:
            with path.open("r", encoding="utf-8", errors="replace") as f:
                for line in f:
                    if line.startswith("# SPEC_COV:"):
                        return True
                    if not line.startswith("#"):
                        break
        except Exception:
            continue
    return False


def _copy_runtime_lib(repo_root: Path, dst_root: Path) -> bool:
    src = repo_root / "runtime" / "cursive0_rt.lib"
    if not src.exists():
        return False
    runtime_dir = dst_root / "runtime"
    runtime_dir.mkdir(parents=True, exist_ok=True)
    shutil.copy2(src, runtime_dir / "cursive0_rt.lib")
    return True


def _run_one(compiler: Path, test_root: Path, repo_root: Path) -> tuple[bool, str]:
    expect_path = test_root / "expect.json"
    expected = _read_expected(expect_path)

    if not _has_spec_cov(test_root / "src"):
        return False, f"missing SPEC_COV header in {test_root}"

    with tempfile.TemporaryDirectory() as tmpdir:
        tmp_root = Path(tmpdir) / "project"
        _copy_project(test_root, tmp_root)
        if not _copy_runtime_lib(repo_root, tmp_root):
            return False, "runtime library not found (runtime/cursive0_rt.lib)"

        cmd = [str(compiler), str(tmp_root / "Cursive.toml")]
        proc = subprocess.run(cmd, capture_output=True, text=True, cwd=str(tmp_root))
        if proc.returncode != 0:
            stderr = proc.stderr.strip()
            return False, f"compile failed (exit {proc.returncode}): {stderr}"

        exe = _exe_path(tmp_root)
        if exe is None or not exe.exists():
            return False, f"exe not found: {exe}"

        run = subprocess.run([str(exe)], capture_output=True, text=True, cwd=str(tmp_root))
        actual = {
            "stdout": _normalize(run.stdout),
            "stderr": _normalize(run.stderr),
            "exit_code": run.returncode,
        }
        want = {
            "stdout": _normalize(str(expected.get("stdout", ""))),
            "stderr": _normalize(str(expected.get("stderr", ""))),
            "exit_code": int(expected.get("exit_code", 0)),
        }

        if actual != want:
            return False, (
                f"{test_root}: mismatch\n"
                f"  want stdout='{want['stdout']}', stderr='{want['stderr']}', exit={want['exit_code']}\n"
                f"  got  stdout='{actual['stdout']}', stderr='{actual['stderr']}', exit={actual['exit_code']}'"
            )
        return True, ""


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--compiler", required=False)
    parser.add_argument("--tests-root", default="tests/e2e")
    parser.add_argument("--test", action="append", default=[])
    args = parser.parse_args()

    repo_root = Path(__file__).resolve().parents[2]
    compiler = Path(args.compiler) if args.compiler else repo_root / "build" / "cursivec0"
    if not compiler.exists():
        print(f"compiler not found: {compiler}")
        return 2

    tests_root = repo_root / args.tests_root
    if args.test:
        test_roots = [Path(p) for p in args.test]
    else:
        test_roots = _discover_tests(tests_root)

    failures = 0
    for test_root in test_roots:
        ok, msg = _run_one(compiler, test_root, repo_root)
        if not ok:
            print(msg)
            failures += 1

    if failures:
        print(f"{failures} e2e test(s) failed")
        return 1
    print("all e2e tests passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
