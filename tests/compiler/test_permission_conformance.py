import json
import pathlib
import subprocess
import unittest


REPO_ROOT = pathlib.Path(__file__).resolve().parents[2]


def default_compiler_path():
    candidates = [
        REPO_ROOT / "cursive" / "build" / "windows" / "Debug" / "Cursive.exe",
        REPO_ROOT / "cursive" / "build" / "windows" / "Cursive.exe",
        REPO_ROOT / "cursive" / "build" / "linux" / "Cursive",
    ]
    for candidate in candidates:
        if candidate.exists():
            return candidate
    raise RuntimeError("Cursive compiler executable not found")


def diagnostics_payload_from(completed):
    for line in reversed((completed.stdout + "\n" + completed.stderr).splitlines()):
        if line.startswith('{"diagnostics"'):
            return json.loads(line)
    raise AssertionError("expected --diag-json diagnostics payload")


class PermissionConformanceTests(unittest.TestCase):
    def build_check(self, fixture_name):
        fixture = REPO_ROOT / "tests" / "compiler" / "fixtures" / fixture_name
        source = fixture / "src" / "Main.cursive"
        return subprocess.run(
            [
                str(default_compiler_path()),
                "build",
                str(source),
                "--check",
                "--incremental=off",
                "--diag-json",
            ],
            cwd=str(REPO_ROOT),
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            check=False,
        )

    def assert_rejects_permission_rebinding(self, fixture_name, source_line):
        completed = self.build_check(fixture_name)
        self.assertNotEqual(completed.returncode, 0, completed.stdout + completed.stderr)
        payload = diagnostics_payload_from(completed)
        diagnostics = payload["diagnostics"]
        self.assertTrue(
            any(
                diagnostic.get("code") == "E-MOD-2402"
                and source_line in diagnostic.get("source_line", "")
                for diagnostic in diagnostics
            ),
            completed.stdout + completed.stderr,
        )

    def assert_accepts(self, fixture_name):
        completed = self.build_check(fixture_name)
        self.assertEqual(completed.returncode, 0, completed.stdout + completed.stderr)
        payload = diagnostics_payload_from(completed)
        self.assertEqual(payload["diagnostics"], [])

    def test_unique_does_not_subtype_to_const_for_rebinding(self):
        self.assert_rejects_permission_rebinding(
            "permission_unique_to_const_project",
            "let view: const Cell = owner",
        )

    def test_unique_does_not_subtype_to_shared_for_rebinding(self):
        self.assert_rejects_permission_rebinding(
            "permission_unique_to_shared_project",
            "let view: shared Cell = owner",
        )

    def test_unique_can_satisfy_non_consuming_const_argument(self):
        self.assert_accepts("permission_unique_const_argument_project")


if __name__ == "__main__":
    unittest.main()
