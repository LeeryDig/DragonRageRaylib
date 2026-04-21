#!/usr/bin/env python3

import argparse
import json
from pathlib import Path


NAME_HINTS = [
    "*prd*",
    "*PRD*",
    "*product*requirements*",
    "*requirements*doc*",
    "*spec*",
    "*roadmap*",
    "*product*",
]

FALLBACK_FILES = [
    "README.md",
    "docs/README.md",
    "docs/PRODUCT.md",
    "docs/SPEC.md",
    "docs/requirements.md",
]


def add_candidate(candidates, project_root: Path, path: Path, reason: str, priority: int):
    if not path.exists() or not path.is_file():
        return

    relative = str(path.relative_to(project_root))
    if relative not in candidates:
        candidates[relative] = {
            "path": relative,
            "reason": reason,
            "priority": priority,
        }
        return

    if priority > candidates[relative]["priority"]:
        candidates[relative]["reason"] = reason
        candidates[relative]["priority"] = priority


def main() -> int:
    parser = argparse.ArgumentParser(description="Discover PRD and product-planning source files.")
    parser.add_argument("--project-root", required=True, help="Target repository root")
    args = parser.parse_args()

    project_root = Path(args.project_root).expanduser().resolve()
    if not project_root.exists():
        parser.error(f"Project root does not exist: {project_root}")

    candidates = {}

    for hint in NAME_HINTS:
        for path in project_root.rglob(hint):
            if ".git" in path.parts or "venv" in path.parts or ".venv" in path.parts:
                continue
            add_candidate(candidates, project_root, path, f"name match: {hint}", 100)

    for fallback in FALLBACK_FILES:
        add_candidate(candidates, project_root, project_root / fallback, f"fallback: {fallback}", 40)

    ordered = sorted(
        candidates.values(),
        key=lambda item: (-item["priority"], item["path"].lower()),
    )

    result = {
        "project_root": str(project_root),
        "found_prd_like_documents": ordered,
        "recommended_primary_source": ordered[0]["path"] if ordered else None,
        "used_fallback": bool(ordered and ordered[0]["priority"] < 100),
    }

    print(json.dumps(result, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
