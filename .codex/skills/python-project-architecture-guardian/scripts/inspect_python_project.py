#!/usr/bin/env python3

import argparse
import ast
import json
from pathlib import Path


ARCHITECTURE_DOCS = [
    "docs/backend/ARCHITECTURE.md",
    "docs/ARCHITECTURE.md",
    "ARCHITECTURE.md",
    "docs/architecture.md",
    "architecture.md",
]

SETUP_FILES = [
    "pyproject.toml",
    "setup.py",
    "requirements.txt",
]

FRAMEWORK_IMPORTS = {
    "fastapi": "FastAPI",
    "flask": "Flask",
    "django": "Django",
    "sqlalchemy": "SQLAlchemy",
    "pydantic": "Pydantic",
    "pytest": "Pytest",
    "typer": "Typer",
    "click": "Click",
    "celery": "Celery",
}

IGNORE_PARTS = {".git", ".venv", "venv", "__pycache__", "node_modules"}
COMMON_PYTHON_DIRS = [
    "",
    "backend-authenticator-qr-code",
]


def is_ignored(path: Path) -> bool:
    return any(part in IGNORE_PARTS for part in path.parts)


def relative(project_root: Path, path: Path) -> str:
    return str(path.relative_to(project_root))


def candidate_roots(project_root: Path):
    candidates = []
    for name in COMMON_PYTHON_DIRS:
        candidate = project_root / name if name else project_root
        if candidate.exists() and candidate.is_dir():
            candidates.append(candidate)
    return candidates


def unique_existing_paths(project_root: Path, candidates):
    seen = set()
    unique = []
    for candidate in candidates:
        path = project_root / candidate
        if not path.exists():
            continue
        resolved = str(path.resolve()).lower()
        if resolved in seen:
            continue
        seen.add(resolved)
        unique.append(candidate)
    return unique


def find_python_roots(project_root: Path):
    roots = []

    for candidate in candidate_roots(project_root):
        src_dir = candidate / "src"
        if src_dir.exists():
            roots.append(str(src_dir.relative_to(project_root)))

        for child in candidate.iterdir():
            if not child.is_dir() or is_ignored(child):
                continue
            if (child / "__init__.py").exists():
                roots.append(str(child.relative_to(project_root)))

    return sorted(set(roots))


def find_test_roots(project_root: Path):
    roots = []
    for candidate in candidate_roots(project_root):
        for name in ("tests", "test"):
            path = candidate / name
            if path.exists():
                roots.append(str(path.relative_to(project_root)))
    return sorted(set(roots))


def collect_python_files(project_root: Path):
    files = []
    for path in project_root.rglob("*.py"):
        if is_ignored(path):
            continue
        files.append(path)
    return files


def detect_frameworks(py_files):
    frameworks = set()

    for path in py_files:
        try:
            tree = ast.parse(path.read_text(encoding="utf-8"))
        except (SyntaxError, UnicodeDecodeError):
            continue

        for node in ast.walk(tree):
            if isinstance(node, ast.Import):
                names = [alias.name for alias in node.names]
            elif isinstance(node, ast.ImportFrom):
                names = [node.module] if node.module else []
            else:
                continue

            for imported in names:
                top_level = imported.split(".")[0]
                if top_level in FRAMEWORK_IMPORTS:
                    frameworks.add(FRAMEWORK_IMPORTS[top_level])

    return sorted(frameworks)


def main() -> int:
    parser = argparse.ArgumentParser(description="Inspect a Python project architecture.")
    parser.add_argument("--project-root", required=True, help="Target repository root")
    args = parser.parse_args()

    project_root = Path(args.project_root).expanduser().resolve()
    if not project_root.exists():
        parser.error(f"Project root does not exist: {project_root}")

    py_files = collect_python_files(project_root)
    architecture_docs = unique_existing_paths(project_root, ARCHITECTURE_DOCS)
    setup_files = []
    for candidate in candidate_roots(project_root):
        for path in SETUP_FILES:
            setup_path = candidate / path
            if setup_path.exists():
                setup_files.append(str(setup_path.relative_to(project_root)))
    setup_files = sorted(set(setup_files))
    python_roots = find_python_roots(project_root)
    test_roots = find_test_roots(project_root)

    result = {
        "project_root": str(project_root),
        "setup_files": setup_files,
        "architecture_docs": architecture_docs,
        "recommended_contract_path": architecture_docs[0] if architecture_docs else "docs/backend/ARCHITECTURE.md",
        "python_roots": python_roots,
        "test_roots": test_roots,
        "frameworks": detect_frameworks(py_files),
        "entrypoints": sorted(
            relative(project_root, path)
            for path in py_files
            if path.name in {"main.py", "app.py", "manage.py"} or path.name.startswith("run_")
        ),
        "python_file_count": len(py_files),
    }

    print(json.dumps(result, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
