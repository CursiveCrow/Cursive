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


class ResolverDiagnosticsTests(unittest.TestCase):
    def test_resolver_failure_emits_json_diagnostic(self):
        fixture = (
            REPO_ROOT
            / "tests"
            / "compiler"
            / "fixtures"
            / "resolver_unmapped_diag_project"
        )
        source = fixture / "src" / "Main.cursive"
        completed = subprocess.run(
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

        self.assertNotEqual(completed.returncode, 0)
        payload = diagnostics_payload_from(completed)
        diagnostics = payload["diagnostics"]
        self.assertTrue(diagnostics, completed.stdout)
        self.assertEqual(diagnostics[0]["severity"], "error")
        self.assertTrue(diagnostics[0]["message"])
        self.assertTrue(diagnostics[0]["span"]["file"].endswith("Main.cursive"))


if __name__ == "__main__":
    unittest.main()
