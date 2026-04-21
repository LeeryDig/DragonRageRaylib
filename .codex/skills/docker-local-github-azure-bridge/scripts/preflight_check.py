#!/usr/bin/env python3

import argparse
import json
import shutil
import subprocess
from pathlib import Path


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
    parser = argparse.ArgumentParser(description="Check local Docker/GitHub/Azure readiness.")
    parser.add_argument("--project-root", required=True, help="Target repository root")
    args = parser.parse_args()

    project_root = Path(args.project_root).expanduser().resolve()
    if not project_root.exists():
        parser.error(f"Project root does not exist: {project_root}")

    docker_path = shutil.which("docker")
    gh_path = shutil.which("gh")
    az_path = shutil.which("az")
    git_path = shutil.which("git")

    docker_compose = None
    if docker_path:
        docker_compose = run_command(["docker", "compose", "version"])
    elif shutil.which("docker-compose"):
        docker_compose = run_command(["docker-compose", "version"])

    github_status = run_command(["gh", "auth", "status"]) if gh_path else None
    azure_status = run_command(["az", "account", "show", "--output", "json"]) if az_path else None

    current_skill_dir = Path(__file__).resolve().parents[1]
    sibling_terraform_skill = current_skill_dir.parent / "terraform-azure-github-local-infra" / "SKILL.md"

    result = {
        "project_root": str(project_root),
        "tools": {
            "docker": {"installed": docker_path is not None, "path": docker_path},
            "docker_compose": docker_compose,
            "gh": {"installed": gh_path is not None, "path": gh_path},
            "az": {"installed": az_path is not None, "path": az_path},
            "git": {"installed": git_path is not None, "path": git_path},
        },
        "github_cli_auth": github_status,
        "azure_cli_account": azure_status,
        "signals": {
            "dockerfiles": find_matches(project_root, "**/Dockerfile*"),
            "compose_files": find_matches(project_root, "**/docker-compose*.y*ml"),
            "workflow_files": find_matches(project_root, ".github/workflows/*"),
            "terraform_files": find_matches(project_root, "**/*.tf"),
            "has_dockerignore": (project_root / ".dockerignore").exists(),
            "has_github_dir": (project_root / ".github").exists(),
            "has_infra_dir": (project_root / "infra").exists(),
            "terraform_skill_available": sibling_terraform_skill.exists(),
            "terraform_skill_path": str(sibling_terraform_skill) if sibling_terraform_skill.exists() else None,
        },
    }

    print(json.dumps(result, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
