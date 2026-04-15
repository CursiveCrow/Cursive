#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import shutil
import tarfile
import tempfile
import zipfile
from dataclasses import dataclass
from pathlib import Path


@dataclass(frozen=True)
class BundleLayout:
    compiler: Path
    runtime_files: tuple[Path, ...]
    lib_dir: Path | None
    tool_dir: Path | None
    bin_sidecars: tuple[Path, ...] = ()
    lib_files: tuple[Path, ...] = ()


def copy_file(src: Path, dst: Path) -> None:
    dst.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(src, dst)


def copy_tree_contents(src: Path, dst: Path) -> None:
    if not src.exists():
        raise FileNotFoundError(f"Missing directory: {src}")
    dst.mkdir(parents=True, exist_ok=True)
    for item in src.iterdir():
        target = dst / item.name
        if item.is_dir():
            shutil.copytree(item, target, dirs_exist_ok=True)
        else:
            shutil.copy2(item, target)


def require_file(path: Path) -> Path:
    if not path.is_file():
        raise FileNotFoundError(f"Missing required file: {path}")
    return path


def require_dir(path: Path) -> Path:
    if not path.is_dir():
        raise FileNotFoundError(f"Missing required directory: {path}")
    return path


def detect_layout(platform: str, staging_root: Path) -> BundleLayout:
    if platform == "linux":
        return BundleLayout(
            compiler=require_file(staging_root / "Cursive"),
            runtime_files=(
                require_file(staging_root / "CursiveRT.a"),
                require_file(staging_root / "linux" / "runtime" / "cursive0_start_x86_64_sysv.o"),
            ),
            lib_dir=require_dir(staging_root / "linux" / "lib"),
            tool_dir=require_dir(staging_root / "linux" / "tools"),
        )

    return BundleLayout(
        compiler=require_file(staging_root / "Cursive.exe"),
        runtime_files=(require_file(staging_root / "CursiveRT.lib"),),
        lib_dir=None,
        tool_dir=require_dir(staging_root / "windows" / "tools"),
        bin_sidecars=(
            require_file(staging_root / "windows" / "bin" / "icudt72.dll"),
            require_file(staging_root / "windows" / "bin" / "icuin72.dll"),
            require_file(staging_root / "windows" / "bin" / "icuuc72.dll"),
        ),
        lib_files=(require_file(staging_root / "windows" / "lib" / "delayimp.lib"),),
    )


def bundle_name(version: str, platform: str, arch: str) -> str:
    return f"cursive-{version}-{platform}-{arch}"


def build_bundle(args: argparse.Namespace) -> int:
    staging_root = args.staging_root.resolve()
    repo_root = args.repo_root.resolve()
    output_dir = args.output_dir.resolve()
    layout = detect_layout(args.platform, staging_root)
    name = bundle_name(args.version, args.platform, args.arch)

    output_dir.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="cursive-release-") as tmp_dir_str:
        tmp_dir = Path(tmp_dir_str)
        bundle_root = tmp_dir / name
        bin_dir = bundle_root / "bin"
        runtime_dir = bundle_root / "runtime"
        lib_dir = bundle_root / "lib"
        tools_dir = bundle_root / "tools"

        copy_file(layout.compiler, bin_dir / layout.compiler.name)
        for runtime_file in layout.runtime_files:
            copy_file(runtime_file, runtime_dir / runtime_file.name)
        for bin_sidecar in layout.bin_sidecars:
            copy_file(bin_sidecar, bin_dir / bin_sidecar.name)
        for lib_file in layout.lib_files:
            copy_file(lib_file, lib_dir / lib_file.name)
        if layout.lib_dir is not None:
            copy_tree_contents(layout.lib_dir, lib_dir)
        if layout.tool_dir is not None:
            copy_tree_contents(layout.tool_dir, tools_dir)

        copy_file(repo_root / "README.md", bundle_root / "README.md")
        copy_file(repo_root / "SUPPORTED_ALPHA_SCOPE.md", bundle_root / "SUPPORTED_ALPHA_SCOPE.md")

        version_path = bundle_root / "VERSION.json"
        version_path.write_text(
            json.dumps(
                {
                    "version": args.version,
                    "platform": args.platform,
                    "arch": args.arch,
                    "bundle": name,
                    "channel": "alpha" if "alpha" in args.version else "custom",
                },
                indent=2,
            )
            + "\n",
            encoding="utf-8",
        )

        if args.platform == "linux":
            archive_path = output_dir / f"{name}.tar.gz"
            with tarfile.open(archive_path, "w:gz") as archive:
                archive.add(bundle_root, arcname=name)
        else:
            archive_path = output_dir / f"{name}.zip"
            with zipfile.ZipFile(archive_path, "w", compression=zipfile.ZIP_DEFLATED) as archive:
                for path in sorted(bundle_root.rglob("*")):
                    archive.write(path, arcname=path.relative_to(tmp_dir))

    print(archive_path)
    return 0


def write_checksums(args: argparse.Namespace) -> int:
    files = [path.resolve() for path in args.files]
    output = args.output.resolve()
    output.parent.mkdir(parents=True, exist_ok=True)

    lines: list[str] = []
    for path in sorted(files):
        if not path.is_file():
            raise FileNotFoundError(f"Missing checksum input: {path}")
        digest = hashlib.sha256(path.read_bytes()).hexdigest()
        lines.append(f"{digest}  {path.name}")

    output.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(output)
    return 0


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Package Cursive release bundles.")
    subparsers = parser.add_subparsers(dest="command", required=True)

    bundle = subparsers.add_parser("bundle", help="Create a platform bundle archive")
    bundle.add_argument("--platform", choices=("linux", "windows"), required=True)
    bundle.add_argument("--version", required=True)
    bundle.add_argument("--arch", default="x86_64")
    bundle.add_argument("--staging-root", type=Path, required=True)
    bundle.add_argument("--output-dir", type=Path, required=True)
    bundle.add_argument(
        "--repo-root",
        type=Path,
        default=Path(__file__).resolve().parent.parent,
    )
    bundle.set_defaults(func=build_bundle)

    checksums = subparsers.add_parser("checksums", help="Write a SHA256SUMS file")
    checksums.add_argument("--output", type=Path, required=True)
    checksums.add_argument("files", nargs="+", type=Path)
    checksums.set_defaults(func=write_checksums)

    return parser


def main() -> int:
    parser = build_parser()
    args = parser.parse_args()
    return args.func(args)


if __name__ == "__main__":
    raise SystemExit(main())
