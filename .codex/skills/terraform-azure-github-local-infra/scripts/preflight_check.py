#!/usr/bin/env python3

import argparse
import json
import shutil
import subprocess
from pathlib import Path


TOOLS = ["terraform", "az", "gh", "git"]


def run_command(command):
    executable = shutil.which(command[0])
    resolved_command = [executable or command[0], *command[1:]]
    try:
        result = subprocess.run(
            resolved_command,
            capture_output=True,
            text=True,
            check=False,
        )
        return {
            "ok": result.returncode == 0,
            "code": result.returncode,
            "stdout": result.stdout.strip(),
            "stderr": result.stderr.strip(),
        }
    except OSError as exc:
        return {
            "ok": False,
            "code": None,
            "stdout": "",
            "stderr": str(exc),
        }


def find_matches(project_root: Path, pattern: str):
    return sorted(str(path.relative_to(project_root)) for path in project_root.glob(pattern))


def main() -> int:
    parser = argparse.ArgumentParser(description="Check local Terraform/Azure/GitHub readiness.")
    parser.add_argument("--project-root", required=True, help="Target repository root")
    args = parser.parse_args()

    project_root = Path(args.project_root).expanduser().resolve()
    if not project_root.exists():
        parser.error(f"Project root does not exist: {project_root}")

    tools = {}
    for tool in TOOLS:
        tools[tool] = {
            "installed": shutil.which(tool) is not None,
        }

    azure_status = None
    if tools["az"]["installed"]:
        azure_status = run_command(["az", "account", "show", "--output", "json"])

    github_status = None
    if tools["gh"]["installed"]:
        github_status = run_command(["gh", "auth", "status"])

    terraform_files = find_matches(project_root, "**/*.tf")
    workflow_files = find_matches(project_root, ".github/workflows/*")

    result = {
        "project_root": str(project_root),
        "git_repo": (project_root / ".git").exists(),
        "tools": tools,
        "azure_cli_account": azure_status,
        "github_cli_auth": github_status,
        "signals": {
            "terraform_files": terraform_files,
            "workflow_files": workflow_files,
            "has_infra_dir": (project_root / "infra").exists(),
            "has_github_dir": (project_root / ".github").exists(),
        },
    }

    print(json.dumps(result, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
