#!/usr/bin/env python3

from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path


def main() -> int:
    parser = argparse.ArgumentParser(description="Run unittest discovery for a Python project.")
    parser.add_argument("--project-root", required=True, help="Python project root")
    parser.add_argument("--tests-dir", default="tests", help="Tests directory relative to the project root")
    parser.add_argument("--pattern", default="test_*.py", help="Glob pattern for unittest discovery")
    args = parser.parse_args()

    project_root = Path(args.project_root).expanduser().resolve()
    tests_dir = project_root / args.tests_dir

    if not project_root.exists():
        parser.error(f"Project root does not exist: {project_root}")
    if not tests_dir.exists():
        parser.error(f"Tests directory does not exist: {tests_dir}")

    command = [
        sys.executable,
        "-m",
        "unittest",
        "discover",
        "-s",
        str(tests_dir.relative_to(project_root)),
        "-p",
        args.pattern,
        "-v",
    ]
    completed = subprocess.run(command, cwd=project_root)
    return completed.returncode


if __name__ == "__main__":
    raise SystemExit(main())
