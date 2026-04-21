#!/usr/bin/env python3

import argparse
import json
from pathlib import Path


ARCHITECTURE_DOCS = [
    "docs/frontend/FRONTEND_ARCHITECTURE.md",
    "docs/FRONTEND_ARCHITECTURE.md",
    "FRONTEND_ARCHITECTURE.md",
    "docs/ARCHITECTURE.md",
    "ARCHITECTURE.md",
]

LOCKFILES = {
    "pnpm-lock.yaml": "pnpm",
    "package-lock.json": "npm",
    "yarn.lock": "yarn",
    "bun.lockb": "bun",
}

COMMON_FRONTEND_DIRS = [
    "",
    "frontend-backoffice",
    "frontend-authenticator-qr-code",
    "frontend",
    "web",
    "app",
]

DOC_HINTS = [
    "docs/frontend/README.md",
    "docs/frontend/IMPLEMENTACAO_MVP.md",
    "docs/UI.md",
    "docs/FRONTEND.md",
    "storybook",
]

TEST_HINTS = [
    "vitest.config.ts",
    "vitest.config.js",
    "jest.config.js",
    "jest.config.ts",
    "playwright.config.ts",
    "playwright.config.js",
]


def read_json(path: Path):
    try:
        return json.loads(path.read_text(encoding="utf-8-sig"))
    except (OSError, json.JSONDecodeError):
        return None


def candidate_roots(project_root: Path):
    candidates = []
    for name in COMMON_FRONTEND_DIRS:
        candidate = project_root / name if name else project_root
        if candidate.exists() and candidate.is_dir():
            candidates.append(candidate)
    return candidates


def detect_workspace_root(project_root: Path):
    for candidate in candidate_roots(project_root):
        if (candidate / "package.json").exists():
            return candidate

    for candidate in candidate_roots(project_root):
        if any((candidate / path).exists() for path in ("src", "public", "app", "pages")):
            return candidate

    return project_root


def detect_package_manager(workspace_root: Path):
    for filename, manager in LOCKFILES.items():
        if (workspace_root / filename).exists():
            return manager
    return None


def merged_dependencies(package_json):
    if not package_json:
        return {}
    deps = {}
    for section in ("dependencies", "devDependencies", "peerDependencies"):
        deps.update(package_json.get(section, {}))
    return deps


def detect_stack(project_root: Path, package_json):
    deps = merged_dependencies(package_json)
    stack = []

    if "react" in deps:
        stack.append("React")
    if "next" in deps or any((project_root / name).exists() for name in ("next.config.js", "next.config.ts", "next.config.mjs")):
        stack.append("Next.js")
    if "vite" in deps or any((project_root / name).exists() for name in ("vite.config.ts", "vite.config.js", "vite.config.mjs")):
        stack.append("Vite")
    if "typescript" in deps or (project_root / "tsconfig.json").exists():
        stack.append("TypeScript")
    if "react-router-dom" in deps:
        stack.append("React Router")
    if "redux" in deps or "@reduxjs/toolkit" in deps:
        stack.append("Redux")
    if "zustand" in deps:
        stack.append("Zustand")
    if "tanstack-query" in " ".join(deps.keys()) or "@tanstack/react-query" in deps:
        stack.append("TanStack Query")
    if "tailwindcss" in deps or (project_root / "tailwind.config.js").exists() or (project_root / "tailwind.config.ts").exists():
        stack.append("Tailwind CSS")
    if "styled-components" in deps:
        stack.append("styled-components")
    if "@testing-library/react" in deps:
        stack.append("React Testing Library")
    if "vitest" in deps:
        stack.append("Vitest")
    if "jest" in deps:
        stack.append("Jest")
    if "@playwright/test" in deps:
        stack.append("Playwright")

    return stack


def detect_project_type(project_root: Path, package_json):
    deps = merged_dependencies(package_json)
    if "next" in deps:
        if (project_root / "app").exists():
            return "next-app-router"
        if (project_root / "pages").exists():
            return "next-pages-router"
        return "next"
    if "react" in deps and "vite" in deps:
        return "vite-react"
    if "react" in deps:
        return "react"
    return "not-detected"


def detect_frontend_paths(project_root: Path):
    paths = []
    for candidate in ("src", "app", "pages", "components", "public", "styles", "tests"):
        if (project_root / candidate).exists():
            paths.append(candidate)
    return paths


def find_existing(project_root: Path, names):
    return [name for name in names if (project_root / name).exists()]


def main() -> int:
    parser = argparse.ArgumentParser(description="Inspect a frontend repository.")
    parser.add_argument("--project-root", required=True, help="Target repository root")
    args = parser.parse_args()

    project_root = Path(args.project_root).expanduser().resolve()
    if not project_root.exists():
        parser.error(f"Project root does not exist: {project_root}")

    workspace_root = detect_workspace_root(project_root)
    package_json = read_json(workspace_root / "package.json") if (workspace_root / "package.json").exists() else None
    stack = detect_stack(workspace_root, package_json)

    result = {
        "project_root": str(project_root),
        "workspace_root": str(workspace_root.relative_to(project_root)) if workspace_root != project_root else ".",
        "frontend_detected": bool(stack),
        "package_json_present": package_json is not None,
        "package_manager": detect_package_manager(workspace_root),
        "project_type": detect_project_type(workspace_root, package_json),
        "stack": stack,
        "frontend_paths": detect_frontend_paths(workspace_root),
        "architecture_docs": find_existing(project_root, ARCHITECTURE_DOCS),
        "recommended_contract_path": find_existing(project_root, ARCHITECTURE_DOCS)[0] if find_existing(project_root, ARCHITECTURE_DOCS) else "docs/frontend/FRONTEND_ARCHITECTURE.md",
        "doc_hints": find_existing(project_root, DOC_HINTS),
        "test_hints": find_existing(project_root, TEST_HINTS),
        "scripts": package_json.get("scripts", {}) if package_json else {},
    }

    print(json.dumps(result, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
