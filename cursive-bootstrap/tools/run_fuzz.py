#!/usr/bin/env python3
import argparse
import os
import subprocess
import sys
from pathlib import Path

DEFAULT_TARGETS = ["lexer_fuzz", "parser_fuzz", "source_load_fuzz"]


def _exe_path(build_dir: Path, name: str) -> Path:
    path = build_dir / name
    if os.name == "nt":
        path = path.with_suffix(".exe")
    return path


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--build-dir", default="build")
    parser.add_argument("--target", action="append", default=[])
    parser.add_argument("--corpus", default="tests/fuzz/corpus")
    parser.add_argument("--artifact-dir", default="tests/fuzz/artifacts")
    parser.add_argument("--duration", type=int, default=60)
    parser.add_argument("--runs", type=int, default=0)
    args = parser.parse_args()

    repo_root = Path(__file__).resolve().parents[1]
    build_dir = (repo_root / args.build_dir).resolve()
    corpus_root = (repo_root / args.corpus).resolve()
    artifact_root = (repo_root / args.artifact_dir).resolve()

    targets = args.target if args.target else DEFAULT_TARGETS
    failures = 0

    for target in targets:
        exe = _exe_path(build_dir, target)
        if not exe.exists():
            print(f"missing fuzz target: {exe}")
            failures += 1
            continue

        corpus_dir = corpus_root / target
        corpus_dir.mkdir(parents=True, exist_ok=True)
        artifact_root.mkdir(parents=True, exist_ok=True)

        cmd = [
            str(exe),
            str(corpus_dir),
            f"-artifact_prefix={artifact_root / (target + '_')}",
        ]
        if args.duration > 0:
            cmd.append(f"-max_total_time={args.duration}")
        if args.runs > 0:
            cmd.append(f"-runs={args.runs}")

        print(f"running {target}...")
        proc = subprocess.run(cmd)
        if proc.returncode != 0:
            failures += 1

    if failures:
        print(f"{failures} fuzz target(s) failed")
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
