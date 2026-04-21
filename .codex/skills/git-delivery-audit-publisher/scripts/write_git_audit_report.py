#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import subprocess
from pathlib import Path


def run_git(project_root: Path, *args: str) -> str:
    completed = subprocess.run(
        ["git", *args],
        cwd=project_root,
        capture_output=True,
        text=True,
        check=False,
    )
    if completed.returncode != 0:
        return completed.stderr.strip() or completed.stdout.strip()
    return completed.stdout.strip()


def section(title: str, body: str) -> str:
    content = body.strip() or "(vazio)"
    return f"## {title}\n\n```\n{content}\n```\n"


def main() -> int:
    parser = argparse.ArgumentParser(description="Write a Git delivery audit report.")
    parser.add_argument("--project-root", required=True, help="Repository root")
    parser.add_argument("--audit-dir", required=True, help="Prepared audit directory")
    parser.add_argument("--tests-summary", default="", help="Optional test/validation summary")
    parser.add_argument("--commit-message", default="", help="Optional proposed or executed commit message")
    parser.add_argument("--push-target", default="", help="Optional remote/branch push target")
    args = parser.parse_args()

    project_root = Path(args.project_root).expanduser().resolve()
    audit_dir = Path(args.audit_dir).expanduser().resolve()
    if not project_root.exists():
        parser.error(f"Project root does not exist: {project_root}")
    if not audit_dir.exists():
        parser.error(f"Audit dir does not exist: {audit_dir}")

    branch = run_git(project_root, "branch", "--show-current")
    remotes = run_git(project_root, "remote", "-v")
    status = run_git(project_root, "status", "--short")
    diff_unstaged = run_git(project_root, "diff", "--stat")
    diff_staged = run_git(project_root, "diff", "--cached", "--stat")
    files_unstaged = run_git(project_root, "diff", "--name-only")
    files_staged = run_git(project_root, "diff", "--cached", "--name-only")
    head_short = run_git(project_root, "rev-parse", "--short", "HEAD")

    report = [
        "# Git Delivery Audit",
        "",
        f"- Branch: `{branch or '(desconhecida)'}`",
        f"- HEAD: `{head_short or '(sem commit)'}`",
        f"- Commit message proposta/executada: `{args.commit_message or '(nao informada)'}`",
        f"- Push target: `{args.push_target or '(nao informado)'}`",
        "",
        section("Remotes", remotes),
        section("Working Tree Status", status),
        section("Unstaged Files", files_unstaged),
        section("Staged Files", files_staged),
        section("Diff Stat Unstaged", diff_unstaged),
        section("Diff Stat Staged", diff_staged),
        section("Validation", args.tests_summary or "Nenhuma validacao informada."),
    ]

    report_path = audit_dir / "report.md"
    report_path.write_text("\n".join(report), encoding="utf-8")

    summary = {
        "branch": branch,
        "head_short": head_short,
        "commit_message": args.commit_message,
        "push_target": args.push_target,
        "status": status.splitlines(),
        "unstaged_files": files_unstaged.splitlines(),
        "staged_files": files_staged.splitlines(),
    }
    (audit_dir / "summary.json").write_text(json.dumps(summary, indent=2), encoding="utf-8")

    print(json.dumps({"report_path": str(report_path), "summary_path": str(audit_dir / 'summary.json')}, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
