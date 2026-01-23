#!/usr/bin/env python3
"""
Concatenate all Draft_01 specification fragments into a single Markdown file,
stripping redundant per-file frontmatter and navigation blocks.

By default this script:
- Walks Cursive-Language-Specification/Draft_01 in lexical order
  (01_..., 02_..., ..., 15_..., Annex)
- Processes each *.md file in lexical order within its clause directory
- Keeps the full frontmatter of the very first file
- Strips per-file frontmatter (header metadata before the first '---' rule)
  from all subsequent files
- Strips trailing navigation blocks starting with '**Previous**:' or
  '**Navigation**:' (and their preceding '---' rule) when present
- Writes the combined document to:
    Cursive-Language-Specification/Cursive-Language-Specification-Complete.md

Run from the repository root:

    python concat_draft_01.py
"""

from __future__ import annotations

import argparse
from pathlib import Path
from typing import Iterable, List


REPO_ROOT = Path(__file__).resolve().parent
SPEC_ROOT = REPO_ROOT / "Cursive-Language-Specification"
DRAFT_ROOT = SPEC_ROOT / "Draft_01"
DEFAULT_OUTPUT = SPEC_ROOT / "Cursive-Language-Specification-Complete.md"


def strip_trailing_navigation(lines: List[str]) -> List[str]:
    """
    Remove a trailing navigation block, if present.

    We look for the last line that starts with either '**Previous**:' or
    '**Navigation**:' and drop it plus any immediately preceding horizontal
    rule ('---') and blank lines.
    """
    nav_index = None
    for i in range(len(lines) - 1, -1, -1):
        stripped = lines[i].strip()
        if stripped.startswith("**Previous**:") or stripped.startswith("**Navigation**:"):
            nav_index = i
            break

    if nav_index is None:
        return lines

    # Walk backwards to include preceding blank lines and a horizontal rule if present.
    start = nav_index
    j = nav_index - 1
    # Skip trailing blank lines immediately before the nav line.
    while j >= 0 and lines[j].strip() == "":
        j -= 1

    # If the first non-blank line before the nav is a horizontal rule, drop it too.
    if j >= 0 and lines[j].strip() == "---":
        start = j

    return lines[:start]


def split_frontmatter(lines: List[str]) -> tuple[List[str], List[str]]:
    """
    Split a file into (frontmatter, body) based on the first '---' rule.

    Frontmatter includes everything from the start of the file up to and
    including the first line that is exactly '---' (ignoring surrounding
    whitespace). If no such line exists, frontmatter is empty.
    """
    for idx, line in enumerate(lines):
        if line.strip() == "---":
            # Include the horizontal rule in the frontmatter.
            return lines[: idx + 1], lines[idx + 1 :]
    return [], lines


def strip_leading_blank_lines(lines: List[str]) -> List[str]:
    """
    Remove leading blank lines from a list of lines.
    """
    start = 0
    while start < len(lines) and lines[start].strip() == "":
        start += 1
    return lines[start:]


def iter_draft_markdown_files(root: Path) -> Iterable[Path]:
    """
    Yield all Markdown files in Draft_01 in a stable specification order.

    The order is:
    - Clause/annex directories in lexical order
    - Within each directory, *.md files in lexical order
    """
    if not root.is_dir():
        raise SystemExit(f"Draft_01 directory not found at: {root}")

    clause_dirs = sorted(
        (p for p in root.iterdir() if p.is_dir()),
        key=lambda p: p.name,
    )

    for clause_dir in clause_dirs:
        md_files = sorted(
            (p for p in clause_dir.iterdir() if p.is_file() and p.suffix == ".md"),
            key=lambda p: p.name,
        )
        for path in md_files:
            yield path


def read_file_lines(path: Path) -> List[str]:
    """
    Read a UTF-8 text file into a list of lines, preserving newlines.
    """
    try:
        text = path.read_text(encoding="utf-8")
    except UnicodeDecodeError as exc:
        raise SystemExit(f"Failed to decode file as UTF-8: {path}") from exc
    # Keep newline characters so we can splice blocks without re-wrapping lines.
    return text.splitlines(keepends=True)


def process_file(path: Path, keep_frontmatter: bool) -> List[str]:
    """
    Load a Draft_01 fragment and return the processed lines.

    Processing steps:
    - Strip trailing navigation block (Previous/Next/Navigation footer) if present
    - Optionally remove frontmatter (header metadata before first '---')
    - Trim leading blank lines in the remaining body
    """
    lines = read_file_lines(path)

    # Always strip navigation footers; they are redundant in a combined document.
    lines = strip_trailing_navigation(lines)

    front, body = split_frontmatter(lines)

    if keep_frontmatter:
        kept = front + body
    else:
        kept = body

    return strip_leading_blank_lines(kept)


def concatenate_draft(output_path: Path) -> None:
    """
    Concatenate all Draft_01 Markdown fragments into a single file.
    """
    output_lines: List[str] = []
    first = True

    for md_path in iter_draft_markdown_files(DRAFT_ROOT):
        processed = process_file(md_path, keep_frontmatter=first)
        first = False

        if not processed:
            continue

        # Ensure a blank line separator between files to keep Markdown structure clear.
        if output_lines and not output_lines[-1].endswith("\n"):
            output_lines[-1] = output_lines[-1] + "\n"

        if output_lines:
            # Two newlines between fragments for readability.
            if not output_lines[-1].strip():
                # Already blank; add one more.
                output_lines.append("\n")
            else:
                output_lines.append("\n\n")

        output_lines.extend(processed)

    if not output_lines:
        raise SystemExit("No Draft_01 markdown files were found to concatenate.")

    output_path.write_text("".join(output_lines), encoding="utf-8")


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Concatenate Draft_01 specification fragments into a single Markdown file.",
    )
    parser.add_argument(
        "-o",
        "--output",
        type=Path,
        default=DEFAULT_OUTPUT,
        help=f"Output Markdown file (default: {DEFAULT_OUTPUT.relative_to(REPO_ROOT)})",
    )

    args = parser.parse_args()
    output_path: Path = args.output

    # Ensure parent directory exists.
    output_path.parent.mkdir(parents=True, exist_ok=True)

    concatenate_draft(output_path)
    print(f"Wrote combined specification to: {output_path}")


if __name__ == "__main__":
    main()

