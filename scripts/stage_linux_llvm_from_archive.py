#!/usr/bin/env python3
from __future__ import annotations

import argparse
import pathlib
import posixpath
import re
import tarfile


LLVM_LIB_RE = re.compile(r"^lib/libLLVM.+\.(?:a|so(?:\..+)?)$")


def should_stage(rel_path: str) -> bool:
    if rel_path.startswith("include/llvm/") or rel_path == "include/llvm":
        return True
    if rel_path.startswith("include/llvm-c/") or rel_path == "include/llvm-c":
        return True
    if rel_path.startswith("lib/cmake/llvm/") or rel_path == "lib/cmake/llvm":
        return True
    if LLVM_LIB_RE.match(rel_path):
        return True
    return rel_path in {
        "bin/ld.lld",
        "bin/lld",
        "bin/clang++",
        "bin/clang",
        "bin/clang-21",
        "bin/llvm-ar",
        "bin/llvm-as",
    }


def write_bytes(target_root: pathlib.Path, rel_path: str, data: bytes, mode: int) -> None:
    target_path = target_root.joinpath(*pathlib.PurePosixPath(rel_path).parts)
    target_path.parent.mkdir(parents=True, exist_ok=True)
    target_path.write_bytes(data)
    try:
        target_path.chmod(mode)
    except OSError:
        pass


def patch_llvm_import_checks(target_root: pathlib.Path) -> None:
    exports_path = target_root / "lib" / "cmake" / "llvm" / "LLVMExports.cmake"
    if not exports_path.exists():
        raise SystemExit(f"missing LLVM exports file: {exports_path}")

    original = exports_path.read_text(encoding="utf-8")
    patched = original.replace(
        "# Loop over all imported files and verify that they actually exist\n"
        "foreach(_cmake_target IN LISTS _cmake_import_check_targets)\n",
        "# Vendored Cursive extern keeps only the build-consumed LLVM subset.\n"
        "set(_cmake_import_check_targets)\n"
        "# Loop over all imported files and verify that they actually exist\n"
        "foreach(_cmake_target IN LISTS _cmake_import_check_targets)\n",
        1,
    )
    patched = patched.replace(
        "# Loop over all imported files and verify that they actually exist\n"
        "foreach(target ${_IMPORT_CHECK_TARGETS} )\n",
        "# Vendored Cursive extern keeps only the build-consumed LLVM subset.\n"
        "set(_IMPORT_CHECK_TARGETS)\n"
        "# Loop over all imported files and verify that they actually exist\n"
        "foreach(target ${_IMPORT_CHECK_TARGETS} )\n",
        1,
    )
    if patched != original:
        exports_path.write_text(patched, encoding="utf-8", newline="\n")


def validate_target(target_root: pathlib.Path) -> None:
    required = (
        target_root / "bin" / "ld.lld",
        target_root / "bin" / "clang++",
        target_root / "bin" / "llvm-ar",
        target_root / "bin" / "llvm-as",
        target_root / "include" / "llvm",
        target_root / "include" / "llvm-c",
        target_root / "lib" / "cmake" / "llvm",
    )
    for path in required:
        if not path.exists():
            raise SystemExit(f"missing staged LLVM path: {path}")

    lib_dir = target_root / "lib"
    if not any(lib_dir.glob("libLLVMCore.a")) and not any(lib_dir.glob("libLLVMCore.so*")):
        raise SystemExit(f"missing staged LLVM core library in {lib_dir}")


def stage_archive(archive_path: pathlib.Path, target_root: pathlib.Path) -> None:
    alias_targets: dict[str, list[str]] = {}
    tool_outputs = {
        "bin/ld.lld",
        "bin/clang++",
        "bin/llvm-ar",
        "bin/llvm-as",
    }

    def write_tool_aliases(source_rel: str, data: bytes) -> None:
        for alias in alias_targets.get(source_rel, ()):
            if alias in tool_outputs:
                write_bytes(target_root, alias, data, 0o755)
            write_tool_aliases(alias, data)

    with tarfile.open(archive_path, "r|xz") as archive:
        for member in archive:
            parts = pathlib.PurePosixPath(member.name).parts
            if len(parts) < 2:
                continue

            rel_path = pathlib.PurePosixPath(*parts[1:]).as_posix()
            if not should_stage(rel_path):
                continue

            if member.issym():
                source_rel = posixpath.normpath(
                    posixpath.join(posixpath.dirname(rel_path), member.linkname)
                )
                alias_targets.setdefault(source_rel, []).append(rel_path)
                continue

            if member.isdir():
                target_root.joinpath(*pathlib.PurePosixPath(rel_path).parts).mkdir(
                    parents=True, exist_ok=True
                )
                continue

            if not member.isfile():
                continue

            file_obj = archive.extractfile(member)
            if file_obj is None:
                raise SystemExit(f"unable to read archive member: {member.name}")
            data = file_obj.read()

            if rel_path.startswith("bin/"):
                if rel_path in tool_outputs:
                    write_bytes(target_root, rel_path, data, 0o755)
                write_tool_aliases(rel_path, data)
                continue

            write_bytes(target_root, rel_path, data, member.mode or 0o644)

    if "bin/lld" in alias_targets and not (target_root / "bin" / "ld.lld").exists():
        raise SystemExit("failed to materialize bin/ld.lld from archive symlink target")
    if "bin/clang-21" in alias_targets and not (target_root / "bin" / "clang++").exists():
        raise SystemExit("failed to materialize bin/clang++ from archive symlink target")

    patch_llvm_import_checks(target_root)
    validate_target(target_root)


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Stage the Linux LLVM subset used by Cursive from a cached upstream archive."
    )
    parser.add_argument("--archive", required=True, help="Path to LLVM-21.1.8-Linux-X64.tar.xz")
    parser.add_argument("--target", required=True, help="Target llvm/llvm-21.1.8-x86_64-sysv root")
    args = parser.parse_args()

    archive_path = pathlib.Path(args.archive).resolve()
    target_root = pathlib.Path(args.target).resolve()

    if not archive_path.is_file():
        raise SystemExit(f"missing archive: {archive_path}")

    target_root.mkdir(parents=True, exist_ok=True)
    stage_archive(archive_path, target_root)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
