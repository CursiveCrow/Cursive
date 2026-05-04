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


class AggregateBranchTailCodegenTests(unittest.TestCase):
    def test_nested_aggregate_if_tail_survives_outer_branch_capture(self):
        fixture = (
            REPO_ROOT
            / "tests"
            / "compiler"
            / "fixtures"
            / "aggregate_branch_tail_project"
        )
        source = fixture / "src" / "Main.cursive"
        completed = subprocess.run(
            [
                str(default_compiler_path()),
                "build",
                str(source),
                "--incremental=off",
                "--diag-json",
            ],
            cwd=str(REPO_ROOT),
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            check=False,
        )

        self.assertEqual(completed.returncode, 0, completed.stdout + completed.stderr)

        executable = (
            fixture
            / "build"
            / "aggregate_branch_tail"
            / "bin"
            / "aggregate_branch_tail.exe"
        )
        self.assertTrue(executable.exists(), f"missing executable: {executable}")
        run = subprocess.run(
            [str(executable)],
            cwd=str(fixture),
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            check=False,
        )

        self.assertEqual(run.returncode, 0, run.stdout + run.stderr)


if __name__ == "__main__":
    unittest.main()
