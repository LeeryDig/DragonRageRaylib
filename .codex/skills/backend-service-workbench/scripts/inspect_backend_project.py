#!/usr/bin/env python3

import argparse
import json
from pathlib import Path


ARCHITECTURE_DOCS = [
    "docs/backend/ARCHITECTURE.md",
    "docs/BACKEND_ARCHITECTURE.md",
    "BACKEND_ARCHITECTURE.md",
    "docs/ARCHITECTURE.md",
    "ARCHITECTURE.md",
]

DOC_HINTS = [
    "docs/backend/COMO_USAR_API.md",
    "docs/COMO_USAR_API.md",
    "docs/API.md",
    "docs/api.md",
    "openapi.yaml",
    "openapi.yml",
]

TEST_DIRS = ["tests", "test", "spec"]

IGNORE_PARTS = {".git", ".venv", "venv", "__pycache__", "node_modules"}
COMMON_BACKEND_DIRS = [
    "",
    "backend-authenticator-qr-code",
    "backend",
    "api",
    "services",
]


def is_ignored(path: Path) -> bool:
    return any(part in IGNORE_PARTS for part in path.parts)


def read_text(path: Path):
    try:
        return path.read_text(encoding="utf-8-sig")
    except OSError:
        return ""


def find_files(project_root: Path, patterns):
    found = []
    for pattern in patterns:
        path = project_root / pattern
        if path.exists():
            found.append(pattern)
    return found


def candidate_roots(project_root: Path):
    candidates = []
    for name in COMMON_BACKEND_DIRS:
        candidate = project_root / name if name else project_root
        if candidate.exists() and candidate.is_dir():
            candidates.append(candidate)
    return candidates


def detect_python_stack(project_root: Path):
    for candidate in candidate_roots(project_root):
        markers = []
        for filename in ("requirements.txt", "setup.py", "pyproject.toml"):
            path = candidate / filename
            if path.exists():
                markers.append(str(path.relative_to(project_root)))

        combined = "\n".join(read_text(project_root / name) for name in markers)
        frameworks = []
        if "fastapi" in combined.lower():
            frameworks.append("FastAPI")
        if "flask" in combined.lower():
            frameworks.append("Flask")
        if "django" in combined.lower():
            frameworks.append("Django")
        if "sqlalchemy" in combined.lower():
            frameworks.append("SQLAlchemy")
        if "celery" in combined.lower():
            frameworks.append("Celery")
        if markers:
            return {"language": "Python", "markers": markers, "frameworks": frameworks}
    return None


def detect_node_stack(project_root: Path):
    for candidate in candidate_roots(project_root):
        package_json = candidate / "package.json"
        if not package_json.exists():
            continue

        content = read_text(package_json).lower()
        frameworks = []
        if "\"express\"" in content:
            frameworks.append("Express")
        if "\"fastify\"" in content:
            frameworks.append("Fastify")
        if "\"@nestjs/core\"" in content:
            frameworks.append("NestJS")
        if "\"prisma\"" in content:
            frameworks.append("Prisma")
        if "\"typeorm\"" in content:
            frameworks.append("TypeORM")

        return {
            "language": "Node.js",
            "markers": [str(package_json.relative_to(project_root))],
            "frameworks": frameworks,
        }
    
    return None


def detect_storage(project_root: Path):
    combined = []
    for candidate in candidate_roots(project_root):
        for filename in ("requirements.txt", "setup.py", "pyproject.toml", "package.json", ".env", ".env.example"):
            path = candidate / filename
            if path.exists():
                combined.append(read_text(path).lower())
    text = "\n".join(combined)

    storage = []
    if "sqlite" in text:
        storage.append("SQLite")
    if "postgres" in text:
        storage.append("PostgreSQL")
    if "mysql" in text:
        storage.append("MySQL")
    if "mongodb" in text:
        storage.append("MongoDB")
    if "redis" in text:
        storage.append("Redis")
    return storage


def detect_entrypoints(project_root: Path):
    entrypoints = []
    for path in project_root.rglob("*"):
        if is_ignored(path) or not path.is_file():
            continue
        name = path.name.lower()
        if name in {"main.py", "app.py", "server.js", "server.ts", "main.ts"} or name.startswith("run_"):
            entrypoints.append(str(path.relative_to(project_root)))
    return sorted(entrypoints)


def detect_test_roots(project_root: Path):
    found = []
    for candidate in candidate_roots(project_root):
        for name in TEST_DIRS:
            path = candidate / name
            if path.exists():
                found.append(str(path.relative_to(project_root)))
    return sorted(set(found))


def main() -> int:
    parser = argparse.ArgumentParser(description="Inspect a backend repository.")
    parser.add_argument("--project-root", required=True, help="Target repository root")
    args = parser.parse_args()

    project_root = Path(args.project_root).expanduser().resolve()
    if not project_root.exists():
        parser.error(f"Project root does not exist: {project_root}")

    python_stack = detect_python_stack(project_root)
    node_stack = detect_node_stack(project_root)
    stacks = [stack for stack in (python_stack, node_stack) if stack]

    result = {
        "project_root": str(project_root),
        "stacks": stacks,
        "architecture_docs": find_files(project_root, ARCHITECTURE_DOCS),
        "recommended_contract_path": find_files(project_root, ARCHITECTURE_DOCS)[0] if find_files(project_root, ARCHITECTURE_DOCS) else "docs/backend/ARCHITECTURE.md",
        "api_docs": find_files(project_root, DOC_HINTS),
        "test_roots": detect_test_roots(project_root),
        "entrypoints": detect_entrypoints(project_root),
        "storage": detect_storage(project_root),
    }

    print(json.dumps(result, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
