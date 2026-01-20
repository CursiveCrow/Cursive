#!/usr/bin/env python3
from __future__ import annotations

import subprocess
import sys
from pathlib import Path


def main() -> int:
    root = Path(__file__).resolve().parent.parent
    cmd = [
        sys.executable,
        str(root / "tools" / "diag_extractor.py"),
        "--check",
    ]
    result = subprocess.run(cmd, capture_output=False, text=True)
    if result.returncode != 0:
        print("[diag_check_report] diag_extractor.py --check reported mismatches")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
