#!/usr/bin/env python3
import argparse
import json
import os
import subprocess
import sys
import tempfile
import tomllib
from pathlib import Path


def _discover_tests(root: Path) -> list[Path]:
    """Discover test files that have corresponding expect files."""
    tests = []
    for p in root.rglob("*.cursive"):
        if not p.is_file():
            continue
        # Only include files that have a corresponding expect file
        expect_json = p.with_suffix(p.suffix + ".expect.json")
        expect_ll = p.with_suffix(".expect.ll")
        if expect_json.exists() or expect_ll.exists():
            tests.append(p)
    return sorted(tests)



def _read_spec_cov(path: Path) -> list[str]:
    cov: list[str] = []
    try:
        with path.open("r", encoding="utf-8", errors="replace") as f:
            for line in f:
                if line.startswith("# SPEC_COV:"):
                    cov.append(line.split(":", 1)[1].strip())
                if len(cov) > 0 and not line.startswith("#"):
                    break
    except FileNotFoundError:
        return []
    return cov


def _read_assembly_target(path: Path) -> str | None:
    try:
        with path.open("r", encoding="utf-8", errors="replace") as f:
            for line in f:
                if line.startswith("# ASSEMBLY:"):
                    return line.split(":", 1)[1].strip()
                if line.startswith("# SPEC_COV:"):
                    continue
                if line.startswith("# OUTPUT_PIPELINE:"):
                    continue
                if not line.startswith("#"):
                    break
    except FileNotFoundError:
        return None
    return None




def _read_output_pipeline(path: Path) -> bool | None:
    try:
        with path.open("r", encoding="utf-8", errors="replace") as f:
            for line in f:
                if line.startswith("# OUTPUT_PIPELINE:"):
                    value = line.split(":", 1)[1].strip().lower()
                    if value in {"on", "true", "1", "yes"}:
                        return True
                    if value in {"off", "false", "0", "no"}:
                        return False
                if line.startswith("# ASSEMBLY:") or line.startswith("# SPEC_COV:"):
                    continue
                if not line.startswith("#"):
                    break
    except FileNotFoundError:
        return None
    return None

def _strip_spec_cov_lines(path: Path) -> str:
    lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
    out: list[str] = []
    for line in lines:
        if line.startswith("# SPEC_COV:") or line.startswith("# ASSEMBLY:") or line.startswith("# OUTPUT_PIPELINE:"):
            continue
        out.append(line)
    return "\n".join(out)


def _find_project_root(path: Path) -> Path | None:
    cur = path.resolve()
    if cur.is_file():
        cur = cur.parent
    while True:
        if (cur / "Cursive.toml").exists():
            return cur
        if cur.parent == cur:
            return None
        cur = cur.parent


def _phase_dir(test_path: Path, tests_root: Path) -> str | None:
    try:
        rel = test_path.resolve().relative_to(tests_root.resolve())
    except ValueError:
        return None
    return rel.parts[0] if rel.parts else None


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


def _source_root_for(project_root: Path, assembly_target: str | None) -> Path | None:
    manifest = _load_manifest(project_root / "Cursive.toml")
    if manifest is None:
        return None
    assembly = _select_assembly(manifest, assembly_target)
    if assembly is None:
        return None
    root = assembly.get("root")
    if not isinstance(root, str):
        return None
    return (project_root / root).resolve()


def _should_isolate(
    test_path: Path,
    tests_root: Path,
    project_root: Path | None,
    assembly_target: str | None,
) -> bool:
    phase = _phase_dir(test_path, tests_root)
    if phase not in {"phase1", "phase2", "phase3", "phase4"}:
        return False

    if assembly_target is not None:
        return False
    if project_root is None:
        return True
    source_root = _source_root_for(project_root, assembly_target)
    if source_root is None:
        return False
    try:
        test_path.resolve().relative_to(source_root)
        return False
    except ValueError:
        return True


def _write_isolated_project(dst_root: Path, test_path: Path) -> Path:
    src_dir = dst_root / "src"
    src_dir.mkdir(parents=True, exist_ok=True)
    (dst_root / "Cursive.toml").write_text(
        "[assembly]\nname = \"demo\"\nkind = \"executable\"\nroot = \"src\"\n",
        encoding="utf-8",
    )
    filtered_src = _strip_spec_cov_lines(test_path)
    out_path = src_dir / test_path.name
    out_path.write_text(filtered_src, encoding="utf-8")
    return out_path


def _copy_filtered_project(src_root: Path, dst_root: Path) -> None:
    for path in src_root.rglob("*"):
        rel = path.relative_to(src_root)
        out_path = dst_root / rel
        if path.is_dir():
            out_path.mkdir(parents=True, exist_ok=True)
            continue
        if path.name == "Cursive.toml":
            out_path.parent.mkdir(parents=True, exist_ok=True)
            out_path.write_bytes(path.read_bytes())
            continue
        if path.suffix == ".cursive":
            out_path.parent.mkdir(parents=True, exist_ok=True)
            out_path.write_text(_strip_spec_cov_lines(path), encoding="utf-8")
            continue


def _run_one(
    compiler: Path,
    test_path: Path,
    expect_path: Path,
    expect_script: Path,
    tests_root: Path,
    mode: str = "diag",
) -> tuple[bool, str]:
    with tempfile.TemporaryDirectory() as tmpdir:
        filtered_path = None
        project_root = None
        filtered_root = None
        cleanup_temp_file = False
        try:
            assembly_target = _read_assembly_target(test_path)
            output_pipeline = _read_output_pipeline(test_path)
            phase = _phase_dir(test_path, tests_root)
            project_root = _find_project_root(test_path)
            if _should_isolate(test_path, tests_root, project_root, assembly_target):
                filtered_root = Path(tmpdir) / "isolated_project"
                filtered_path = _write_isolated_project(filtered_root, test_path)
                project_root = filtered_root
            elif project_root is not None:
                filtered_root = Path(tmpdir) / "filtered_project"
                _copy_filtered_project(project_root, filtered_root)
                filtered_path = filtered_root / test_path.relative_to(project_root)
            else:
                filtered_src = _strip_spec_cov_lines(test_path)
                with tempfile.NamedTemporaryFile(
                    mode="w",
                    encoding="utf-8",
                    delete=False,
                    dir=test_path.parent,
                    suffix=test_path.suffix + ".tmp",
                ) as tmp_src:
                    tmp_src.write(filtered_src)
                    filtered_path = Path(tmp_src.name)
                    cleanup_temp_file = True
            
            cmd = [str(compiler)]
            use_internal_flags = False
            if mode == "ir":
                cmd.append("--emit-ir")
                use_internal_flags = True
            else:
                cmd.append("--diag-json")
            if mode != "ir" and output_pipeline is not True:
                cmd.append("--no-output")
                use_internal_flags = True
                
            if phase == "phase1":
                cmd.append("--phase1-only")
                use_internal_flags = True
            if assembly_target:
                cmd += ["--assembly", assembly_target]
            cmd.append(str(filtered_path))

            env = os.environ.copy()
            if use_internal_flags:
                env["CURSIVE0_INTERNAL_FLAGS"] = "1"

            proc = subprocess.run(cmd, capture_output=True, text=True, env=env)
            stdout = proc.stdout.strip()
            stderr = proc.stderr.strip()
            
            if mode == "ir":
                if proc.returncode != 0:
                    return False, f"{test_path}: compiler failed with exit code {proc.returncode}. Stderr: {stderr}"
                
                expected = expect_path.read_text(encoding="utf-8").strip()
                # Simple normalization: remove trailing whitespace from lines
                actual_lines = [l.rstrip() for l in stdout.splitlines() if l.strip()]
                expect_lines = [l.rstrip() for l in expected.splitlines() if l.strip()]
                
                # Check line by line containment/equality
                # For now, let's require that 'expect_lines' logic acts as a substring match or structural match
                # But to keep it simple and robust, let's just dump exact mismatch if they differ significantly.
                # Actually, better strategy: Check that every expected line appears in actual output in order.
                
                act_idx = 0
                for exp_line in expect_lines:
                    found = False
                    while act_idx < len(actual_lines):
                        if exp_line in actual_lines[act_idx]:
                            found = True
                            act_idx += 1
                            break
                        act_idx += 1
                    
                    if not found:
                         return False, f"{test_path}: IR mismatch. Expected line not found after previous matches:\n'{exp_line}'\n\nFull Actual:\n{stdout}"
                return True, ""

            if stderr:
                return False, f"{test_path}: compiler stderr not empty: {stderr}"
            try:
                data = json.loads(stdout) if stdout else {}
            except json.JSONDecodeError as exc:
                return False, f"{test_path}: invalid JSON output: {exc}"
            
            actual_path = Path(tmpdir) / "actual.json"
            data["exit_code"] = proc.returncode
            actual_path.write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")
            check = subprocess.run(
                [sys.executable, str(expect_script), str(expect_path), str(actual_path)],
                capture_output=True,
                text=True,
            )
            if check.returncode != 0:
                message = check.stdout.strip() or check.stderr.strip() or "mismatch"
                return False, f"{test_path}: {message}"
            return True, ""
        finally:
            if cleanup_temp_file and filtered_path and filtered_path.exists():
                filtered_path.unlink()


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--compiler", required=False)
    parser.add_argument("--tests-root", default="tests/compile")
    parser.add_argument("--test", action="append", default=[])
    args = parser.parse_args()

    repo_root = Path(__file__).resolve().parents[2]
    compiler = Path(args.compiler) if args.compiler else repo_root / "build" / "cursivec0"
    expect_script = repo_root / "tests" / "harness" / "expect_diag.py"

    if not compiler.exists():
        print(f"compiler not found: {compiler}")
        return 2

    tests_root = repo_root / args.tests_root
    if args.test:
        test_paths = [Path(p) for p in args.test]
    else:
        test_paths = _discover_tests(tests_root)

    failures = 0
    for test_path in test_paths:
        expect_json = test_path.with_suffix(test_path.suffix + ".expect.json")
        expect_ll = test_path.with_suffix(".expect.ll")
        
        mode = "diag"
        expect_path = expect_json
        
        if expect_ll.exists():
            mode = "ir"
            expect_path = expect_ll
        elif not expect_path.exists():
            print(f"missing expect file: {expect_path} or {expect_ll}")
            failures += 1
            continue
            
        cov = _read_spec_cov(test_path)
        if not cov:
            print(f"missing SPEC_COV header: {test_path}")
            failures += 1
            continue
            
        ok, msg = _run_one(compiler, test_path, expect_path, expect_script, tests_root, mode)
        if not ok:
            print(msg)
            failures += 1

    if failures:
        print(f"{failures} compile test(s) failed")
        return 1
    print("all compile tests passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
