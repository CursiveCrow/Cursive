#!/usr/bin/env python3
"""
FileCheck-style directive parser for Cursive0 spec tests.

This module provides utilities for parsing and validating inline test
directives in Cursive source files, similar to LLVM's FileCheck tool.

Directive Syntax:
    //~ ERROR E-XXX-NNNN: message substring
    //~ WARN W-XXX-NNNN: message
    //~ INFO I-XXX-NNNN

    //~^ ERROR E-XXX-NNNN     (refers to previous line)
    //~v ERROR E-XXX-NNNN     (refers to next line)
    //~N ERROR E-XXX-NNNN     (refers to line N lines away)

Usage:
    from filecheck import FileChecker

    checker = FileChecker(source_text)
    errors = checker.validate(compiler_diagnostics)
"""

from __future__ import annotations

import dataclasses
import re
from typing import List, Optional, Dict, Set, Tuple


@dataclasses.dataclass
class ExpectedDiagnostic:
    """An expected diagnostic from a source directive."""
    source_line: int      # Line where the directive appears
    target_line: int      # Line the diagnostic should appear on
    severity: str         # ERROR, WARN, INFO
    code: str             # E-XXX-NNNN format
    message: Optional[str]  # Optional message substring to match


@dataclasses.dataclass
class ActualDiagnostic:
    """A diagnostic emitted by the compiler."""
    file: Optional[str]
    line: Optional[int]
    column: Optional[int]
    severity: str  # error, warning, info
    code: str
    message: str


@dataclasses.dataclass
class CheckError:
    """An error found during directive validation."""
    line: int
    message: str
    expected: Optional[ExpectedDiagnostic]
    actual: Optional[ActualDiagnostic]


class FileChecker:
    """Validates compiler output against inline directives."""

    # Directive pattern: //~ [^|v|N] SEVERITY CODE: message
    DIRECTIVE_PATTERN = re.compile(
        r'//~(?P<offset>[v^]|\d+)?\s*'
        r'(?P<severity>ERROR|WARN|INFO)\s+'
        r'(?P<code>[A-Z]-[A-Z]{3}-\d{4})'
        r'(?::\s*(?P<message>.+))?'
    )

    def __init__(self, source: str, filename: str = "<test>"):
        self.source = source
        self.filename = filename
        self.lines = source.split('\n')
        self.expected: List[ExpectedDiagnostic] = []
        self._parse_directives()

    def _parse_directives(self) -> None:
        """Parse all directives from source."""
        for line_num, line in enumerate(self.lines, start=1):
            match = self.DIRECTIVE_PATTERN.search(line)
            if not match:
                continue

            offset_str = match.group('offset')
            severity = match.group('severity')
            code = match.group('code')
            message = match.group('message')
            if message:
                message = message.strip()

            # Determine target line based on offset
            if offset_str is None:
                # //~ ERROR - same line
                target_line = line_num
            elif offset_str == '^':
                # //~^ ERROR - previous line
                target_line = line_num - 1
            elif offset_str == 'v':
                # //~v ERROR - next line
                target_line = line_num + 1
            else:
                # //~N ERROR - N lines offset
                target_line = line_num + int(offset_str)

            self.expected.append(ExpectedDiagnostic(
                source_line=line_num,
                target_line=target_line,
                severity=severity,
                code=code,
                message=message,
            ))

    def validate(self, diagnostics: List[ActualDiagnostic]) -> List[CheckError]:
        """
        Validate actual diagnostics against expected directives.

        Returns a list of check errors (empty if all validations pass).
        """
        errors: List[CheckError] = []
        matched: Set[int] = set()  # Indices of matched actual diagnostics

        # For each expected diagnostic, find a matching actual diagnostic
        for exp in self.expected:
            found = False
            for i, actual in enumerate(diagnostics):
                if i in matched:
                    continue

                # Check if this actual diagnostic matches the expected one
                if self._matches(exp, actual):
                    matched.add(i)
                    found = True
                    break

            if not found:
                errors.append(CheckError(
                    line=exp.target_line,
                    message=f"Expected {exp.severity} {exp.code}" +
                            (f": {exp.message}" if exp.message else "") +
                            f" at line {exp.target_line}, not found",
                    expected=exp,
                    actual=None,
                ))

        return errors

    def _matches(self, expected: ExpectedDiagnostic, actual: ActualDiagnostic) -> bool:
        """Check if an actual diagnostic matches an expected one."""
        # Normalize severity comparison
        severity_map = {
            'ERROR': 'error',
            'WARN': 'warning',
            'INFO': 'info',
        }
        expected_severity = severity_map.get(expected.severity, expected.severity.lower())

        if actual.severity.lower() != expected_severity:
            return False

        if actual.code != expected.code:
            return False

        # Line must match (if actual has line info)
        if actual.line is not None and actual.line != expected.target_line:
            return False

        # Message must contain expected substring (if specified)
        if expected.message and expected.message not in actual.message:
            return False

        return True

    def get_expected_codes(self) -> Set[str]:
        """Get all expected diagnostic codes."""
        return {exp.code for exp in self.expected}

    def get_expected_at_line(self, line: int) -> List[ExpectedDiagnostic]:
        """Get all expected diagnostics for a specific line."""
        return [exp for exp in self.expected if exp.target_line == line]


def parse_header_directives(source: str) -> Dict[str, any]:
    """
    Parse header-style directives from the top of a test file.

    Header directives:
        // RUN: compile-fail
        // DIAG: E-XXX-NNNN
        // DIAG-NOT: E-XXX-NNNN
        // SPEC: X.Y.Z
        // DESC: description text
        // SKIP: reason
        // XFAIL: reason
        // STDOUT: expected text
        // EXIT: N

    Returns a dictionary of parsed values.
    """
    result = {
        'run_mode': 'compile-pass',
        'expected_diags': [],
        'forbidden_diags': [],
        'spec_refs': [],
        'description': None,
        'skip_reason': None,
        'xfail_reason': None,
        'expected_stdout': [],
        'expected_exit': None,
    }

    for line in source.split('\n'):
        line = line.strip()

        if m := re.match(r'//\s*RUN:\s*(\S+)', line):
            result['run_mode'] = m.group(1)
        elif m := re.match(r'//\s*DIAG:\s*(\S+)', line):
            result['expected_diags'].append(m.group(1))
        elif m := re.match(r'//\s*DIAG-NOT:\s*(\S+)', line):
            result['forbidden_diags'].append(m.group(1))
        elif m := re.match(r'//\s*SPEC:\s*(.+)', line):
            result['spec_refs'].append(m.group(1).strip())
        elif m := re.match(r'//\s*DESC:\s*(.+)', line):
            result['description'] = m.group(1).strip()
        elif m := re.match(r'//\s*SKIP:\s*(.+)', line):
            result['skip_reason'] = m.group(1).strip()
        elif m := re.match(r'//\s*XFAIL:\s*(.+)', line):
            result['xfail_reason'] = m.group(1).strip()
        elif m := re.match(r'//\s*STDOUT:\s*(.+)', line):
            result['expected_stdout'].append(m.group(1).strip())
        elif m := re.match(r'//\s*EXIT:\s*(\d+)', line):
            result['expected_exit'] = int(m.group(1))

    return result


def extract_all_diagnostic_codes(source: str) -> Set[str]:
    """Extract all diagnostic codes mentioned in a source file."""
    pattern = r'[EWIP]-[A-Z]{3}-\d{4}'
    return set(re.findall(pattern, source))


# Self-test
if __name__ == "__main__":
    test_source = '''// RUN: compile-fail
// DIAG: E-TYP-1901
// SPEC: 5.2.3
// DESC: Duplicate field name in record declaration

record Point {
    x: i32
    x: i32  //~ ERROR E-TYP-1901: Duplicate field name
}

public procedure main(move ctx: Context) -> i32 {
    let y: i32 = 0  //~ WARN W-SEM-1001: unused variable
    return 0
}
'''

    checker = FileChecker(test_source, "test.cursive")
    print("Expected diagnostics:")
    for exp in checker.expected:
        print(f"  Line {exp.target_line}: {exp.severity} {exp.code}" +
              (f": {exp.message}" if exp.message else ""))

    print("\nHeader directives:")
    headers = parse_header_directives(test_source)
    for k, v in headers.items():
        if v:
            print(f"  {k}: {v}")

    print("\nAll diagnostic codes in file:")
    codes = extract_all_diagnostic_codes(test_source)
    print(f"  {sorted(codes)}")
