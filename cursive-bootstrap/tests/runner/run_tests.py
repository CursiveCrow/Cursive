#!/usr/bin/env python3
"""
Cursive0 Spec Test Runner

A comprehensive test harness for the cursivec0 compiler, inspired by
Rust's compiletest and LLVM's lit test infrastructure.

Usage:
    python run_tests.py [options]

Options:
    --filter PATTERN    Only run tests matching glob pattern
    --jobs N            Run N tests in parallel (default: CPU count)
    --bless             Update expected outputs (baseline mode)
    --verbose           Show detailed output for each test
    --format FORMAT     Output format: human, junit, tap (default: human)
    --compiler PATH     Path to cursivec0.exe (default: auto-detect)
    --timeout SECONDS   Per-test timeout (default: 60)
    --coverage          Generate diagnostic coverage report
    --list              List all tests without running them
"""

from __future__ import annotations

import argparse
import concurrent.futures
import dataclasses
import enum
import fnmatch
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
import time
import tomllib
import xml.etree.ElementTree as ET
from pathlib import Path
from typing import Dict, List, Optional, Set, Tuple, Any

# =============================================================================
# Configuration
# =============================================================================

@dataclasses.dataclass
class Config:
    """Test runner configuration."""
    spec_tests_root: Path
    compiler_path: Path
    runtime_lib: Path
    llvm_bin: Path
    jobs: int
    timeout: float
    verbose: bool
    bless: bool
    filter_pattern: Optional[str]
    output_format: str
    coverage: bool
    list_only: bool


def load_config_toml(runner_dir: Path) -> dict:
    """Load configuration from config.toml."""
    config_path = runner_dir / "config.toml"
    if not config_path.exists():
        return {}

    with open(config_path, "rb") as f:
        return tomllib.load(f)


def find_compiler(root: Path, config_toml: dict) -> Path:
    """Locate cursivec0.exe relative to tests directory."""
    # Check config.toml first
    if "compiler" in config_toml and "path" in config_toml["compiler"]:
        config_path = config_toml["compiler"]["path"]
        # Resolve relative to tests root
        resolved = (root / config_path).resolve()
        if resolved.exists():
            return resolved

    # Fall back to auto-detection
    candidates = [
        root.parent / "build" / "Release" / "cursivec0.exe",
        root.parent / "build" / "Debug" / "cursivec0.exe",
        root.parent / "bootstrap" / "cursivec0.exe",
    ]
    for c in candidates:
        if c.exists():
            return c
    raise FileNotFoundError("Could not find cursivec0.exe. Use --compiler to specify path.")


def find_runtime(root: Path, config_toml: dict) -> Path:
    """Locate cursive0_rt.lib."""
    # Check config.toml first
    if "runtime" in config_toml and "path" in config_toml["runtime"]:
        config_path = config_toml["runtime"]["path"]
        # Resolve relative to tests root
        resolved = (root / config_path).resolve()
        if resolved.exists():
            return resolved

    # Fall back to auto-detection
    candidates = [
        root / "auxiliary" / "runtime" / "cursive0_rt.lib",
        root.parent / "build" / "Release" / "cursive0_rt.lib",
        root.parent / "build" / "runtime" / "cursive0_rt.lib",
    ]
    for c in candidates:
        if c.exists():
            return c
    raise FileNotFoundError("Could not find cursive0_rt.lib.")


def find_llvm_bin(root: Path) -> Path:
    """Locate LLVM bin directory."""
    candidates = [
        root.parent / "third_party" / "llvm" / "llvm-21.1.8-x86_64" / "bin",
        root.parent / "llvm" / "llvm-21.1.8-x86_64" / "bin",
    ]
    for c in candidates:
        if c.exists():
            return c
    raise FileNotFoundError("Could not find LLVM bin directory.")


# =============================================================================
# Test Directives
# =============================================================================

class RunMode(enum.Enum):
    COMPILE_PASS = "compile-pass"
    COMPILE_FAIL = "compile-fail"
    RUN_PASS = "run-pass"
    RUN_FAIL = "run-fail"


@dataclasses.dataclass
class LineDirective:
    """An inline directive like //~ ERROR E-TYP-1901: message."""
    line_number: int
    severity: str  # ERROR, WARN, INFO
    code: str
    message: Optional[str]


@dataclasses.dataclass
class TestDirectives:
    """Parsed directives from a test file."""
    run_mode: RunMode
    expected_diags: List[str]
    forbidden_diags: List[str]
    line_directives: List[LineDirective]
    expected_stdout: List[str]
    expected_exit: Optional[int]
    spec_refs: List[str]
    description: Optional[str]
    skip_reason: Optional[str]
    xfail_reason: Optional[str]


def parse_directives(source: str, source_path: Path) -> TestDirectives:
    """Parse test directives from source file comments."""
    run_mode = RunMode.COMPILE_PASS  # default
    expected_diags: List[str] = []
    forbidden_diags: List[str] = []
    line_directives: List[LineDirective] = []
    expected_stdout: List[str] = []
    expected_exit: Optional[int] = None
    spec_refs: List[str] = []
    description: Optional[str] = None
    skip_reason: Optional[str] = None
    xfail_reason: Optional[str] = None

    lines = source.split('\n')
    for i, line in enumerate(lines, start=1):
        # Parse header directives: // RUN:, // DIAG:, etc.
        if m := re.match(r'//\s*RUN:\s*(\S+)', line):
            mode_str = m.group(1).strip()
            try:
                run_mode = RunMode(mode_str)
            except ValueError:
                print(f"Warning: Unknown RUN mode '{mode_str}' in {source_path}:{i}", file=sys.stderr)

        elif m := re.match(r'//\s*DIAG:\s*(\S+)', line):
            expected_diags.append(m.group(1).strip())

        elif m := re.match(r'//\s*DIAG-NOT:\s*(\S+)', line):
            forbidden_diags.append(m.group(1).strip())

        elif m := re.match(r'//\s*STDOUT:\s*(.+)', line):
            expected_stdout.append(m.group(1).strip())

        elif m := re.match(r'//\s*EXIT:\s*(\d+)', line):
            expected_exit = int(m.group(1))

        elif m := re.match(r'//\s*SPEC:\s*(.+)', line):
            spec_refs.append(m.group(1).strip())

        elif m := re.match(r'//\s*DESC:\s*(.+)', line):
            description = m.group(1).strip()

        elif m := re.match(r'//\s*SKIP:\s*(.+)', line):
            skip_reason = m.group(1).strip()

        elif m := re.match(r'//\s*XFAIL:\s*(.+)', line):
            xfail_reason = m.group(1).strip()

        # Parse inline directives: //~ ERROR E-TYP-1901: message
        if m := re.match(r'.*//~\s*(ERROR|WARN|INFO)\s+([A-Z]-[A-Z]{3}-\d{4})(?::\s*(.+))?', line):
            severity = m.group(1)
            code = m.group(2)
            message = m.group(3).strip() if m.group(3) else None
            line_directives.append(LineDirective(i, severity, code, message))

    return TestDirectives(
        run_mode=run_mode,
        expected_diags=expected_diags,
        forbidden_diags=forbidden_diags,
        line_directives=line_directives,
        expected_stdout=expected_stdout,
        expected_exit=expected_exit,
        spec_refs=spec_refs,
        description=description,
        skip_reason=skip_reason,
        xfail_reason=xfail_reason,
    )


# =============================================================================
# Test Discovery
# =============================================================================

@dataclasses.dataclass
class TestCase:
    """A single test case."""
    name: str
    path: Path
    category: str
    is_project: bool  # True if this is a multi-file project test


def discover_tests(root: Path, filter_pattern: Optional[str] = None) -> List[TestCase]:
    """Discover all test cases under the tests directory."""
    tests: List[TestCase] = []

    # Single-file tests: .cursive files
    for category_dir in ["ui", "run-pass", "run-fail", "codegen"]:
        cat_path = root / category_dir
        if not cat_path.exists():
            continue
        for cursive_file in cat_path.rglob("*.cursive"):
            rel_path = cursive_file.relative_to(root)
            name = str(rel_path).replace("\\", "/")
            if filter_pattern and not fnmatch.fnmatch(name, filter_pattern):
                continue
            tests.append(TestCase(
                name=name,
                path=cursive_file,
                category=category_dir,
                is_project=False,
            ))

    # Multi-file project tests: directories with expect.toml but no Cursive.toml
    # (or directories that are test projects)
    for category_dir in ["ui"]:
        cat_path = root / category_dir
        if not cat_path.exists():
            continue
        for subdir in cat_path.rglob("*"):
            if not subdir.is_dir():
                continue
            expect_file = subdir / "expect.toml"
            if expect_file.exists():
                rel_path = subdir.relative_to(root)
                name = str(rel_path).replace("\\", "/")
                if filter_pattern and not fnmatch.fnmatch(name, filter_pattern):
                    continue
                tests.append(TestCase(
                    name=name,
                    path=subdir,
                    category=category_dir,
                    is_project=True,
                ))

    return sorted(tests, key=lambda t: t.name)


# =============================================================================
# Diagnostic Parsing
# =============================================================================

@dataclasses.dataclass
class Diagnostic:
    """A compiler diagnostic."""
    severity: str  # error, warning, info
    code: str
    message: str
    file: Optional[str]
    line: Optional[int]
    column: Optional[int]


def parse_compiler_output(output: str) -> List[Diagnostic]:
    """Parse diagnostics from compiler stderr output."""
    diagnostics: List[Diagnostic] = []

    # Pattern: CODE (severity): message @file:line:col
    # Or: CODE (severity): message (no location)
    # Example: E-SEM-3161 (error): `return` type mismatch with procedure @C:/path/file.cursive:21:49
    pattern = re.compile(
        r'([A-Z]-[A-Z]{3}-\d{4})\s+\((error|warning|info)\):\s*(.+?)(?:\s+@([^:]+):(\d+):(\d+))?$',
        re.IGNORECASE
    )

    for line in output.split('\n'):
        if m := pattern.match(line.strip()):
            diagnostics.append(Diagnostic(
                severity=m.group(2).lower(),
                code=m.group(1),
                message=m.group(3),
                file=m.group(4),
                line=int(m.group(5)) if m.group(5) else None,
                column=int(m.group(6)) if m.group(6) else None,
            ))

    return diagnostics


# =============================================================================
# Test Execution
# =============================================================================

class TestResult(enum.Enum):
    PASS = "pass"
    FAIL = "fail"
    SKIP = "skip"
    XFAIL = "xfail"  # expected failure
    XPASS = "xpass"  # unexpected pass (expected to fail but passed)


@dataclasses.dataclass
class TestOutcome:
    """Result of running a test."""
    test: TestCase
    result: TestResult
    duration: float
    message: Optional[str]
    diagnostics: List[Diagnostic]
    stdout: str
    stderr: str
    exit_code: int
    diag_codes_hit: Set[str]


def run_single_test(test: TestCase, config: Config) -> TestOutcome:
    """Run a single test case and return the outcome."""
    start_time = time.time()
    diag_codes_hit: Set[str] = set()

    try:
        if test.is_project:
            return run_project_test(test, config, start_time)
        else:
            return run_file_test(test, config, start_time)
    except Exception as e:
        return TestOutcome(
            test=test,
            result=TestResult.FAIL,
            duration=time.time() - start_time,
            message=f"Internal error: {e}",
            diagnostics=[],
            stdout="",
            stderr="",
            exit_code=-1,
            diag_codes_hit=set(),
        )


def run_file_test(test: TestCase, config: Config, start_time: float) -> TestOutcome:
    """Run a single-file test."""
    source = test.path.read_text(encoding="utf-8")
    directives = parse_directives(source, test.path)

    # Check for skip
    if directives.skip_reason:
        return TestOutcome(
            test=test,
            result=TestResult.SKIP,
            duration=time.time() - start_time,
            message=f"Skipped: {directives.skip_reason}",
            diagnostics=[],
            stdout="",
            stderr="",
            exit_code=0,
            diag_codes_hit=set(),
        )

    # Create temporary project for single-file test
    with tempfile.TemporaryDirectory() as tmpdir:
        tmp_path = Path(tmpdir)

        # Create Cursive.toml
        manifest = f'''[assembly]
name = "test"
kind = "executable"
root = "src"
'''
        (tmp_path / "Cursive.toml").write_text(manifest)

        # Create src directory and copy source
        src_dir = tmp_path / "src"
        src_dir.mkdir()
        main_file = src_dir / "main.cursive"
        main_file.write_text(source)

        # Copy runtime library
        runtime_dest = tmp_path / "runtime"
        runtime_dest.mkdir()
        if config.runtime_lib.exists():
            shutil.copy(config.runtime_lib, runtime_dest / "cursive0_rt.lib")

        # Run compiler with LLVM path set
        env = os.environ.copy()
        env["C0_LLVM_BIN"] = str(config.llvm_bin)
        compile_result = subprocess.run(
            [str(config.compiler_path), "build", str(tmp_path / "Cursive.toml")],
            capture_output=True,
            text=True,
            timeout=config.timeout,
            cwd=str(tmp_path),
            env=env,
        )

        diagnostics = parse_compiler_output(compile_result.stderr)
        diag_codes = {d.code for d in diagnostics}
        compile_success = compile_result.returncode == 0

        # Check compilation outcome based on run mode
        if directives.run_mode == RunMode.COMPILE_FAIL:
            if compile_success:
                if directives.xfail_reason:
                    return TestOutcome(
                        test=test,
                        result=TestResult.XPASS,
                        duration=time.time() - start_time,
                        message=f"Expected to fail (XFAIL: {directives.xfail_reason}) but compiled successfully",
                        diagnostics=diagnostics,
                        stdout=compile_result.stdout,
                        stderr=compile_result.stderr,
                        exit_code=compile_result.returncode,
                        diag_codes_hit=diag_codes,
                    )
                return TestOutcome(
                    test=test,
                    result=TestResult.FAIL,
                    duration=time.time() - start_time,
                    message="Expected compilation to fail, but it succeeded",
                    diagnostics=diagnostics,
                    stdout=compile_result.stdout,
                    stderr=compile_result.stderr,
                    exit_code=compile_result.returncode,
                    diag_codes_hit=diag_codes,
                )

            # Verify expected diagnostics
            missing_diags = set(directives.expected_diags) - diag_codes
            if missing_diags:
                if directives.xfail_reason:
                    return TestOutcome(
                        test=test,
                        result=TestResult.XFAIL,
                        duration=time.time() - start_time,
                        message=f"XFAIL: {directives.xfail_reason}. Missing diagnostics: {missing_diags}",
                        diagnostics=diagnostics,
                        stdout=compile_result.stdout,
                        stderr=compile_result.stderr,
                        exit_code=compile_result.returncode,
                        diag_codes_hit=diag_codes,
                    )
                return TestOutcome(
                    test=test,
                    result=TestResult.FAIL,
                    duration=time.time() - start_time,
                    message=f"Missing expected diagnostics: {missing_diags}",
                    diagnostics=diagnostics,
                    stdout=compile_result.stdout,
                    stderr=compile_result.stderr,
                    exit_code=compile_result.returncode,
                    diag_codes_hit=diag_codes,
                )

            # Check forbidden diagnostics
            forbidden_found = set(directives.forbidden_diags) & diag_codes
            if forbidden_found:
                return TestOutcome(
                    test=test,
                    result=TestResult.FAIL,
                    duration=time.time() - start_time,
                    message=f"Found forbidden diagnostics: {forbidden_found}",
                    diagnostics=diagnostics,
                    stdout=compile_result.stdout,
                    stderr=compile_result.stderr,
                    exit_code=compile_result.returncode,
                    diag_codes_hit=diag_codes,
                )

            # Verify line directives
            for ld in directives.line_directives:
                matching = [d for d in diagnostics if d.code == ld.code and d.line == ld.line_number]
                if not matching:
                    return TestOutcome(
                        test=test,
                        result=TestResult.FAIL,
                        duration=time.time() - start_time,
                        message=f"Line {ld.line_number}: Expected {ld.severity} {ld.code}, not found",
                        diagnostics=diagnostics,
                        stdout=compile_result.stdout,
                        stderr=compile_result.stderr,
                        exit_code=compile_result.returncode,
                        diag_codes_hit=diag_codes,
                    )
                if ld.message:
                    msg_match = any(ld.message in d.message for d in matching)
                    if not msg_match:
                        return TestOutcome(
                            test=test,
                            result=TestResult.FAIL,
                            duration=time.time() - start_time,
                            message=f"Line {ld.line_number}: {ld.code} message mismatch, expected substring '{ld.message}'",
                            diagnostics=diagnostics,
                            stdout=compile_result.stdout,
                            stderr=compile_result.stderr,
                            exit_code=compile_result.returncode,
                            diag_codes_hit=diag_codes,
                        )

            if directives.xfail_reason:
                return TestOutcome(
                    test=test,
                    result=TestResult.XPASS,
                    duration=time.time() - start_time,
                    message=f"Expected XFAIL ({directives.xfail_reason}) but test passed",
                    diagnostics=diagnostics,
                    stdout=compile_result.stdout,
                    stderr=compile_result.stderr,
                    exit_code=compile_result.returncode,
                    diag_codes_hit=diag_codes,
                )

            return TestOutcome(
                test=test,
                result=TestResult.PASS,
                duration=time.time() - start_time,
                message=None,
                diagnostics=diagnostics,
                stdout=compile_result.stdout,
                stderr=compile_result.stderr,
                exit_code=compile_result.returncode,
                diag_codes_hit=diag_codes,
            )

        elif directives.run_mode in (RunMode.COMPILE_PASS, RunMode.RUN_PASS, RunMode.RUN_FAIL):
            if not compile_success:
                if directives.xfail_reason:
                    return TestOutcome(
                        test=test,
                        result=TestResult.XFAIL,
                        duration=time.time() - start_time,
                        message=f"XFAIL: {directives.xfail_reason}. Compilation failed.",
                        diagnostics=diagnostics,
                        stdout=compile_result.stdout,
                        stderr=compile_result.stderr,
                        exit_code=compile_result.returncode,
                        diag_codes_hit=diag_codes,
                    )
                return TestOutcome(
                    test=test,
                    result=TestResult.FAIL,
                    duration=time.time() - start_time,
                    message=f"Compilation failed: {compile_result.stderr[:500]}",
                    diagnostics=diagnostics,
                    stdout=compile_result.stdout,
                    stderr=compile_result.stderr,
                    exit_code=compile_result.returncode,
                    diag_codes_hit=diag_codes,
                )

            if directives.run_mode == RunMode.COMPILE_PASS:
                return TestOutcome(
                    test=test,
                    result=TestResult.PASS,
                    duration=time.time() - start_time,
                    message=None,
                    diagnostics=diagnostics,
                    stdout=compile_result.stdout,
                    stderr=compile_result.stderr,
                    exit_code=compile_result.returncode,
                    diag_codes_hit=diag_codes,
                )

            # Run the executable
            exe_path = tmp_path / "build" / "bin" / "test.exe"
            if not exe_path.exists():
                return TestOutcome(
                    test=test,
                    result=TestResult.FAIL,
                    duration=time.time() - start_time,
                    message=f"Executable not found at {exe_path}",
                    diagnostics=diagnostics,
                    stdout=compile_result.stdout,
                    stderr=compile_result.stderr,
                    exit_code=compile_result.returncode,
                    diag_codes_hit=diag_codes,
                )

            run_result = subprocess.run(
                [str(exe_path)],
                capture_output=True,
                text=True,
                timeout=config.timeout,
                cwd=str(tmp_path),
            )

            if directives.run_mode == RunMode.RUN_PASS:
                expected_exit = directives.expected_exit if directives.expected_exit is not None else 0
                if run_result.returncode != expected_exit:
                    return TestOutcome(
                        test=test,
                        result=TestResult.FAIL,
                        duration=time.time() - start_time,
                        message=f"Expected exit code {expected_exit}, got {run_result.returncode}",
                        diagnostics=diagnostics,
                        stdout=run_result.stdout,
                        stderr=run_result.stderr,
                        exit_code=run_result.returncode,
                        diag_codes_hit=diag_codes,
                    )

                # Check stdout expectations
                for expected in directives.expected_stdout:
                    if expected not in run_result.stdout:
                        return TestOutcome(
                            test=test,
                            result=TestResult.FAIL,
                            duration=time.time() - start_time,
                            message=f"Expected stdout to contain: {expected}",
                            diagnostics=diagnostics,
                            stdout=run_result.stdout,
                            stderr=run_result.stderr,
                            exit_code=run_result.returncode,
                            diag_codes_hit=diag_codes,
                        )

                return TestOutcome(
                    test=test,
                    result=TestResult.PASS,
                    duration=time.time() - start_time,
                    message=None,
                    diagnostics=diagnostics,
                    stdout=run_result.stdout,
                    stderr=run_result.stderr,
                    exit_code=run_result.returncode,
                    diag_codes_hit=diag_codes,
                )

            else:  # RUN_FAIL
                if run_result.returncode == 0:
                    return TestOutcome(
                        test=test,
                        result=TestResult.FAIL,
                        duration=time.time() - start_time,
                        message="Expected runtime panic, but program exited successfully",
                        diagnostics=diagnostics,
                        stdout=run_result.stdout,
                        stderr=run_result.stderr,
                        exit_code=run_result.returncode,
                        diag_codes_hit=diag_codes,
                    )

                # Check for expected panic diagnostic
                runtime_diags = parse_compiler_output(run_result.stderr)
                runtime_codes = {d.code for d in runtime_diags}

                missing_diags = set(directives.expected_diags) - runtime_codes
                if missing_diags:
                    return TestOutcome(
                        test=test,
                        result=TestResult.FAIL,
                        duration=time.time() - start_time,
                        message=f"Missing expected runtime panic diagnostics: {missing_diags}",
                        diagnostics=runtime_diags,
                        stdout=run_result.stdout,
                        stderr=run_result.stderr,
                        exit_code=run_result.returncode,
                        diag_codes_hit=runtime_codes,
                    )

                return TestOutcome(
                    test=test,
                    result=TestResult.PASS,
                    duration=time.time() - start_time,
                    message=None,
                    diagnostics=runtime_diags,
                    stdout=run_result.stdout,
                    stderr=run_result.stderr,
                    exit_code=run_result.returncode,
                    diag_codes_hit=runtime_codes,
                )

    return TestOutcome(
        test=test,
        result=TestResult.FAIL,
        duration=time.time() - start_time,
        message="Unhandled run mode",
        diagnostics=[],
        stdout="",
        stderr="",
        exit_code=-1,
        diag_codes_hit=set(),
    )


def run_project_test(test: TestCase, config: Config, start_time: float) -> TestOutcome:
    """Run a multi-file project test."""
    # Read expect.toml for expected behavior
    expect_path = test.path / "expect.toml"
    if not expect_path.exists():
        return TestOutcome(
            test=test,
            result=TestResult.FAIL,
            duration=time.time() - start_time,
            message="Missing expect.toml",
            diagnostics=[],
            stdout="",
            stderr="",
            exit_code=-1,
            diag_codes_hit=set(),
        )

    # Simple TOML parsing for expect.toml
    expect_content = expect_path.read_text()
    expected_diags: List[str] = []

    for line in expect_content.split('\n'):
        if m := re.match(r'diag\s*=\s*"([^"]+)"', line):
            expected_diags.append(m.group(1))
        elif m := re.match(r'diags\s*=\s*\[([^\]]+)\]', line):
            codes = re.findall(r'"([^"]+)"', m.group(1))
            expected_diags.extend(codes)

    # Copy project to temp directory
    with tempfile.TemporaryDirectory() as tmpdir:
        tmp_path = Path(tmpdir)
        shutil.copytree(test.path, tmp_path / "project", dirs_exist_ok=True)
        project_path = tmp_path / "project"

        # Run compiler with LLVM path set
        env = os.environ.copy()
        env["C0_LLVM_BIN"] = str(config.llvm_bin)
        compile_result = subprocess.run(
            [str(config.compiler_path), "build", str(project_path / "Cursive.toml")],
            capture_output=True,
            text=True,
            timeout=config.timeout,
            cwd=str(project_path),
            env=env,
        )

        diagnostics = parse_compiler_output(compile_result.stderr)
        diag_codes = {d.code for d in diagnostics}

        missing = set(expected_diags) - diag_codes
        if missing:
            return TestOutcome(
                test=test,
                result=TestResult.FAIL,
                duration=time.time() - start_time,
                message=f"Missing expected diagnostics: {missing}",
                diagnostics=diagnostics,
                stdout=compile_result.stdout,
                stderr=compile_result.stderr,
                exit_code=compile_result.returncode,
                diag_codes_hit=diag_codes,
            )

        return TestOutcome(
            test=test,
            result=TestResult.PASS,
            duration=time.time() - start_time,
            message=None,
            diagnostics=diagnostics,
            stdout=compile_result.stdout,
            stderr=compile_result.stderr,
            exit_code=compile_result.returncode,
            diag_codes_hit=diag_codes,
        )


# =============================================================================
# Reporting
# =============================================================================

def report_human(outcomes: List[TestOutcome], config: Config) -> None:
    """Print human-readable test results."""
    passed = sum(1 for o in outcomes if o.result == TestResult.PASS)
    failed = sum(1 for o in outcomes if o.result == TestResult.FAIL)
    skipped = sum(1 for o in outcomes if o.result == TestResult.SKIP)
    xfailed = sum(1 for o in outcomes if o.result == TestResult.XFAIL)
    xpassed = sum(1 for o in outcomes if o.result == TestResult.XPASS)

    for o in outcomes:
        status_char = {
            TestResult.PASS: ".",
            TestResult.FAIL: "F",
            TestResult.SKIP: "S",
            TestResult.XFAIL: "x",
            TestResult.XPASS: "X",
        }[o.result]

        if config.verbose or o.result in (TestResult.FAIL, TestResult.XPASS):
            print(f"\n{status_char} {o.test.name} ({o.duration:.2f}s)")
            if o.message:
                print(f"  {o.message}")
            if o.result == TestResult.FAIL and o.stderr:
                print(f"  stderr: {o.stderr[:500]}")
        else:
            print(status_char, end="", flush=True)

    print(f"\n\n{'='*60}")
    print(f"Results: {passed} passed, {failed} failed, {skipped} skipped, {xfailed} xfailed, {xpassed} xpassed")
    print(f"Total: {len(outcomes)} tests in {sum(o.duration for o in outcomes):.2f}s")


def report_junit(outcomes: List[TestOutcome], config: Config) -> None:
    """Generate JUnit XML report."""
    root = ET.Element("testsuites")
    suite = ET.SubElement(root, "testsuite", name="cursive0-tests")

    for o in outcomes:
        testcase = ET.SubElement(suite, "testcase", name=o.test.name, time=str(o.duration))
        if o.result == TestResult.FAIL:
            failure = ET.SubElement(testcase, "failure", message=o.message or "Test failed")
            failure.text = o.stderr
        elif o.result == TestResult.SKIP:
            ET.SubElement(testcase, "skipped", message=o.message or "Test skipped")

    tree = ET.ElementTree(root)
    ET.indent(tree, space="  ")
    tree.write(config.spec_tests_root / "junit-results.xml", encoding="unicode")
    print(f"JUnit report written to {config.spec_tests_root / 'junit-results.xml'}")


def report_tap(outcomes: List[TestOutcome], config: Config) -> None:
    """Generate TAP (Test Anything Protocol) report."""
    print(f"1..{len(outcomes)}")
    for i, o in enumerate(outcomes, 1):
        status = "ok" if o.result in (TestResult.PASS, TestResult.XFAIL) else "not ok"
        skip = " # SKIP" if o.result == TestResult.SKIP else ""
        todo = " # TODO" if o.result == TestResult.XFAIL else ""
        print(f"{status} {i} - {o.test.name}{skip}{todo}")


def report_coverage(outcomes: List[TestOutcome], config: Config) -> None:
    """Generate diagnostic code coverage report."""
    # All diagnostic codes from spec
    all_codes = set()
    hit_codes: Set[str] = set()

    for o in outcomes:
        hit_codes.update(o.diag_codes_hit)

    # Load all expected codes from test files
    for test in discover_tests(config.spec_tests_root):
        if test.path.suffix == ".cursive":
            source = test.path.read_text(encoding="utf-8")
            for m in re.finditer(r'[A-Z]-[A-Z]{3}-\d{4}', source):
                all_codes.add(m.group())

    print("\n" + "="*60)
    print("DIAGNOSTIC CODE COVERAGE")
    print("="*60)

    covered = len(hit_codes)
    total = len(all_codes) if all_codes else 1
    percent = (covered / total) * 100 if total else 0

    print(f"Codes hit: {covered}/{total} ({percent:.1f}%)")

    missing = all_codes - hit_codes
    if missing:
        print(f"\nMissing coverage for:")
        for code in sorted(missing):
            print(f"  - {code}")


# =============================================================================
# Main Entry Point
# =============================================================================

def main() -> int:
    parser = argparse.ArgumentParser(description="Cursive0 Spec Test Runner")
    parser.add_argument("--filter", dest="filter_pattern", help="Only run tests matching pattern")
    parser.add_argument("--jobs", "-j", type=int, default=os.cpu_count(), help="Parallel jobs")
    parser.add_argument("--bless", action="store_true", help="Update expected outputs")
    parser.add_argument("--verbose", "-v", action="store_true", help="Verbose output")
    parser.add_argument("--format", default="human", choices=["human", "junit", "tap"])
    parser.add_argument("--compiler", type=Path, help="Path to cursivec0.exe")
    parser.add_argument("--timeout", type=float, default=60.0, help="Per-test timeout")
    parser.add_argument("--coverage", action="store_true", help="Generate coverage report")
    parser.add_argument("--list", dest="list_only", action="store_true", help="List tests only")
    args = parser.parse_args()

    # Determine paths
    script_dir = Path(__file__).parent
    spec_tests_root = script_dir.parent

    # Load config.toml
    config_toml = load_config_toml(script_dir)

    compiler_path = args.compiler or find_compiler(spec_tests_root, config_toml)
    runtime_lib = find_runtime(spec_tests_root, config_toml)
    llvm_bin = find_llvm_bin(spec_tests_root)

    config = Config(
        spec_tests_root=spec_tests_root,
        compiler_path=compiler_path,
        runtime_lib=runtime_lib,
        llvm_bin=llvm_bin,
        jobs=args.jobs,
        timeout=args.timeout,
        verbose=args.verbose,
        bless=args.bless,
        filter_pattern=args.filter_pattern,
        output_format=args.format,
        coverage=args.coverage,
        list_only=args.list_only,
    )

    # Discover tests
    tests = discover_tests(spec_tests_root, args.filter_pattern)

    if args.list_only:
        print(f"Found {len(tests)} tests:")
        for t in tests:
            print(f"  {t.name}")
        return 0

    print(f"Running {len(tests)} tests with {config.jobs} parallel jobs...")
    print(f"Compiler: {config.compiler_path}")
    print()

    # Run tests
    outcomes: List[TestOutcome] = []

    with concurrent.futures.ThreadPoolExecutor(max_workers=config.jobs) as executor:
        futures = {executor.submit(run_single_test, test, config): test for test in tests}
        for future in concurrent.futures.as_completed(futures):
            outcome = future.result()
            outcomes.append(outcome)

    # Sort outcomes by test name for consistent output
    outcomes.sort(key=lambda o: o.test.name)

    # Report results
    if config.output_format == "human":
        report_human(outcomes, config)
    elif config.output_format == "junit":
        report_junit(outcomes, config)
    elif config.output_format == "tap":
        report_tap(outcomes, config)

    if config.coverage:
        report_coverage(outcomes, config)

    # Exit code: 0 if all passed, 1 if any failed
    failed_count = sum(1 for o in outcomes if o.result == TestResult.FAIL)
    return 1 if failed_count > 0 else 0


if __name__ == "__main__":
    sys.exit(main())
