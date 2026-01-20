#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
from typing import Any, Iterable


def _normalize_path(path: str) -> Path:
    try:
        return Path(path).resolve()
    except Exception:
        return Path(path)


def _map_span_file(
    file_path: str,
    source_root: Path | None,
    filtered_root: Path | None,
    test_path: Path | None,
    is_isolated: bool,
) -> str:
    if not file_path:
        return file_path
    src = _normalize_path(file_path)
    if filtered_root:
        try:
            rel = src.relative_to(filtered_root.resolve())
        except Exception:
            rel = None
        if rel is not None:
            if is_isolated and test_path:
                return str(test_path.resolve())
            if source_root:
                return str((source_root / rel).resolve())
            if test_path and rel.name == test_path.name:
                return str(test_path.resolve())
    return str(src)


def normalize_diagnostics(
    diagnostics: Iterable[dict[str, Any]],
    *,
    source_root: Path | None,
    filtered_root: Path | None,
    test_path: Path | None,
    is_isolated: bool = False,
) -> list[dict[str, Any]]:
    out: list[dict[str, Any]] = []
    for diag in diagnostics:
        if not isinstance(diag, dict):
            out.append(diag)
            continue
        span = diag.get("span")
        if isinstance(span, dict):
            file_path = span.get("file")
            if isinstance(file_path, str):
                span = dict(span)
                span["file"] = _map_span_file(
                    file_path, source_root, filtered_root, test_path, is_isolated
                )
                diag = dict(diag)
                diag["span"] = span
        out.append(diag)
    return out


def normalize_ir_text(
    text: str,
    *,
    source_root: Path | None,
    filtered_root: Path | None,
    test_path: Path | None = None,
    is_isolated: bool = False,
) -> str:
    normalized = text.replace("\r\n", "\n").replace("\r", "\n")
    if filtered_root and test_path and is_isolated:
        isolated_file = filtered_root / "src" / test_path.name
        normalized = normalized.replace(str(isolated_file), str(test_path.resolve()))
        normalized = normalized.replace(
            str(isolated_file).replace("\\", "/"),
            str(test_path.resolve()).replace("\\", "/"),
        )
    if source_root and filtered_root:
        src = str(source_root)
        src_slash = src.replace("\\", "/")
        filt = str(filtered_root)
        filt_slash = filt.replace("\\", "/")
        normalized = normalized.replace(filt, src)
        normalized = normalized.replace(filt_slash, src_slash)
    if source_root and test_path and is_isolated:
        src_file = source_root / "src" / test_path.name
        normalized = normalized.replace(str(src_file), str(test_path.resolve()))
        normalized = normalized.replace(
            str(src_file).replace("\\", "/"),
            str(test_path.resolve()).replace("\\", "/"),
        )
    lines = [line.rstrip() for line in normalized.split("\n")]
    while lines and lines[-1] == "":
        lines.pop()
    return "\n".join(lines)
