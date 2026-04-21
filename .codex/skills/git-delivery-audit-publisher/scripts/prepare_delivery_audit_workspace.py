#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import re
from datetime import datetime
from pathlib import Path


AUDIT_ROOT_NAME = "delivery-audits"
EXCLUDE_ENTRY = f"{AUDIT_ROOT_NAME}/"


def slugify(value: str) -> str:
    slug = re.sub(r"[^a-zA-Z0-9]+", "-", value.strip().lower()).strip("-")
    return slug or "git-delivery"


def ensure_exclude(project_root: Path) -> bool:
    exclude_path = project_root / ".git" / "info" / "exclude"
    if not exclude_path.exists():
        return False

    current = exclude_path.read_text(encoding="utf-8")
    lines = [line.strip() for line in current.splitlines()]
    if EXCLUDE_ENTRY not in lines:
        if current and not current.endswith("\n"):
            current += "\n"
        current += f"{EXCLUDE_ENTRY}\n"
        exclude_path.write_text(current, encoding="utf-8")
    return True


def main() -> int:
    parser = argparse.ArgumentParser(description="Prepare a local Git delivery audit workspace.")
    parser.add_argument("--project-root", required=True, help="Repository root")
    parser.add_argument("--scope", default="git-delivery", help="Short label for the delivery audit")
    args = parser.parse_args()

    project_root = Path(args.project_root).expanduser().resolve()
    if not project_root.exists():
        parser.error(f"Project root does not exist: {project_root}")

    timestamp = datetime.now().strftime("%Y%m%d-%H%M%S")
    slug = slugify(args.scope)
    audit_root = project_root / AUDIT_ROOT_NAME
    audit_dir = audit_root / f"{timestamp}-{slug}"
    artifacts_dir = audit_dir / "artifacts"
    audit_dir.mkdir(parents=True, exist_ok=True)
    artifacts_dir.mkdir(parents=True, exist_ok=True)

    excluded = ensure_exclude(project_root)

    metadata = {
        "created_at": datetime.now().isoformat(),
        "scope": args.scope,
        "project_root": str(project_root),
        "audit_root": str(audit_root),
        "audit_dir": str(audit_dir),
        "artifacts_dir": str(artifacts_dir),
        "exclude_updated": excluded,
    }
    (audit_dir / "metadata.json").write_text(json.dumps(metadata, indent=2), encoding="utf-8")

    report_path = audit_dir / "report.md"
    if not report_path.exists():
        report_path.write_text("# Git Delivery Audit\n\n", encoding="utf-8")

    print(json.dumps(metadata, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
